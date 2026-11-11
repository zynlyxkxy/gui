#ifndef BONDCURVERECORDER_H
#define BONDCURVERECORDER_H

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>

#include "chart/customchart.h"   // ISTAR::GUI::CustomChart
#include "mctrl_client.h"        // MctrlModule::IoObserverData（槽参数类型）

// ============================================================================
//  键合曲线采集器
//
//  数据不用另采：io 模块以 500ms 周期把全部 IO 值推给 mctrl（IoObserverData =
//  ACS + PLC 的 AI/DI/AO/DO），mctrl 原样透传给 GUI。本类订阅
//  MCTRLClientManager::sig_IoObserverData，在"键合窗口"内把 PLC AI 的上吸盘温度 /
//  下吸盘温度 / 键合实际压力 追加到 CustomChart 上（X 用 GUI 本地时钟）。
//
//  窗口规则（与需求约定）：
//    - startSession()：清图 + 记起点 + 开窗。顺序是【先清后开】，所以不会把新一段
//      的头几个点擦掉；
//    - finishSession()：关窗，图上数据定格（不清图）；
//    - 开窗后最长 kMaxSeconds 秒，超时无条件收尾（流程没结束也停）—— 既对齐 X 轴
//      量程，也避免窗口一直不关导致数据无界增长；
//    - 下一次 startSession() 覆盖上一段（一次键合一段）。
//
//  窗口何时开/关由调用方决定（各流程自己的时机），本类不判断流程语义。
// ============================================================================

namespace BondCurve
{
/// 三条 Y 轴的名字：applyAxisRange() 与采集器共用，避免两边字符串写岔。
///（AddAxisData 的键必须与 Y 轴名完全一致，写岔了曲线会静默不显示。）
inline constexpr char kAxisUpTemp[]   = "Up Chuck Temperature";
inline constexpr char kAxisDownTemp[] = "Down Chuck Temperature";
inline constexpr char kAxisForce[]    = "Bond Force";

/// PLC AI 点位（id 来自 mctrl 的 IOModule/Command/io_var_id.h）
inline constexpr int kVarUpTemp   = 500;   // AI_PLC_PulseHeat_Up_oCurTemp
inline constexpr int kVarDownTemp = 501;   // AI_PLC_Heat_Dw_oCurTemp
inline constexpr int kVarForce    = 502;   // AI_PLC_M_BS_oActualForce

/// 一段最长秒数（5 分钟）：采集到这里无条件收尾，也是没数据时 X 轴显示的范围。
inline constexpr int kMaxSeconds = 300;

// ============================================================================
//  量程策略
//
//  X 轴：自适应(mode=1)，不设上限 —— 随数据自己长（没数据时停在 kMaxSeconds）。
//  Y 轴：【先固定基线，超过了再往外扩】，mode=0（固定大小）。控件的 mode=1 不是
//  "range 当窗口下限"，而是"range 只是配置时的初始值，一来数据就整个按数据重算"，
//  基线会被第一个点顶掉，做不到"先固定"。所以 Y 的量程只能由我们自己在
//  applyAxisRange() 里给定：
//    - 温度两条共用一个上限（尺度一致、可横向对比），力轴单独一个上限；
//    - 超基线后一次扩一档（整数档），不会每来一个点就抖一次；
//    - 一段之内只放大不缩小，下一段 startSession() 回到基线。
//
//  重设量程会把 X 一起写回配置值（X 会瞬回 kMaxSeconds），所以采集器在加点之前重设，
//  紧接着同一个采样点的 AddAxisData 就把 X 拉回自适应 —— 中间不 replot，画面不闪。
// ============================================================================
inline constexpr int kTempAxisBaselineMax  = 400;   // ℃：两条温度轴共用的基线
inline constexpr int kForceAxisBaselineMax = 5;     // N：真实使用只有 ~1N，5N 才看得清
inline constexpr int kTempAxisStep         = 100;   // 超基线后一次扩一档（℃）
inline constexpr int kForceAxisStep        = 5;     // 超基线后一次扩一档（N）

/// 把 X 轴 + 三条 Y 轴按给定上限写进 chart（mode=0 固定）。
/// tempCeil / forceCeil 是当前上限：首次配置传两个 BaselineMax。
/// 重复调用是安全的（已有点保留、不会加出重复轴），采集器就是靠再调一次把量程扩出去。
/// 副作用：控件的 SetAxisParameter 会把隐藏的 Y 轴重新显示出来 —— 调用方若用过
/// SetAxisYVisible，之后要自己补一次显隐（采集器见 BondCurveRecorder::setCurveVisible）。
void applyAxisRange(ISTAR::GUI::CustomChart *chart, double tempCeil, double forceCeil);
}  // namespace BondCurve

class BondCurveRecorder : public QObject
{
    Q_OBJECT

public:
    /// chart 由调用方持有：本类只在窗口内往里加点，不负责它的生命周期
    explicit BondCurveRecorder(ISTAR::GUI::CustomChart *chart, QObject *parent = nullptr);

    /// 开一段：清图 + 记起点 + 开始收点
    void startSession();

    /// 收一段：关窗（图上数据定格，不清图）
    void finishSession();

    /// 曲线/纵轴显隐：勾选框改调这个而不是直接调控件的 SetAxisYVisible ——
    /// 采集器要靠记住的显隐在重设量程之后把轴补回来（SetAxisParameter 会把隐藏的轴
    /// 重新显示出来）。
    void setCurveVisible(const QString &name, bool visible);

    bool isRecording() const { return m_recording; }

private slots:
    void onIoObserver(const MctrlModule::IoObserverData &data);
    // 整机状态变化：任务不在跑（流程结束 / 操作员按了"停止"）就收尾 ——
    // 键合图不能一直往图里塞点（stopTask 不发"流程完成"事件，只能靠状态判）
    void onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);

private:
    /// 按当前上限重设量程，并把三条轴的显隐补回来
    void applyRange();

    ISTAR::GUI::CustomChart *m_chart = nullptr;
    bool m_recording = false;
    bool m_sawRunning = false;   // 本段内是否见过 BUSY/ALARM。用来区分"任务真的结束了"
                                 // 与"Start 之后那条滞后推送来的 IDLE"，避免刚开窗就被误关
    // 当前量程上限（一段之内只放大，startSession 时回到基线）
    double m_tempCeil = BondCurve::kTempAxisBaselineMax;     // 两条温度轴共用
    double m_forceCeil = BondCurve::kForceAxisBaselineMax;
    QHash<QString, bool> m_visible;   // 轴名 → 勾选框状态（缺省 = 显示）
    QElapsedTimer m_clock;   // 本段起点（GUI 本地时钟，不依赖控制层时间戳）
};

#endif // BONDCURVERECORDER_H
