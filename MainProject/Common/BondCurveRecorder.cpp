#include "BondCurveRecorder.h"

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QHash>
#include <QPointF>

#include "RPCModule/MCTRLClientManager.h"
#include "StatusModule/StatusManager.h"   // 整机状态：任务结束/停止 → 收尾

namespace
{
/// 从 PLC AI 推送里取某个 varId 的值；找不到返回 false
bool findPlcAi(const MctrlModule::PlcObserverData &plc, int varId, double &out)
{
    for (const MctrlModule::ObserverDoubleEntry &e : plc.AI)
    {
        if (e.varId == varId)
        {
            out = e.value;
            return true;
        }
    }
    return false;
}
}  // namespace

namespace BondCurve
{
void applyAxisRange(ISTAR::GUI::CustomChart *chart, double tempCeil, double forceCeil)
{
    if (chart == nullptr)
    {
        return;
    }

    // 轴名/单位/颜色只在这里定义一份：页面的勾选框用 kAxis* 常量，两边名字必须一致。
    // X 轴：自适应(mode=1)，不设上限 —— range 只在没数据时显示，有数据就按数据自己长。
    ISTAR::GUI::CustomChart::AxisParameter axisX;
    axisX.name = QStringLiteral("Time");
    axisX.unit = QStringLiteral("s");
    axisX.mode = 1;
    axisX.range.start = 0;
    axisX.range.end = kMaxSeconds;

    // 三条 Y 轴：固定(mode=0)，量程取 tempCeil / forceCeil；超基线时调用方再调一次本函数
    ISTAR::GUI::CustomChart::AxisParameter up;
    up.name = QString::fromLatin1(kAxisUpTemp);
    up.unit = QStringLiteral("℃");
    up.mode = 0;
    up.range.start = 0;
    up.range.end = tempCeil;
    up.color = QColor(200, 0, 0);

    ISTAR::GUI::CustomChart::AxisParameter dw;
    dw.name = QString::fromLatin1(kAxisDownTemp);
    dw.unit = QStringLiteral("℃");
    dw.mode = 0;
    dw.range.start = 0;
    dw.range.end = tempCeil;   // 与上吸盘同一个上限 → 两条温度尺度一致，可横向对比
    dw.color = QColor(0, 200, 0);

    ISTAR::GUI::CustomChart::AxisParameter force;
    force.name = QString::fromLatin1(kAxisForce);
    force.unit = QStringLiteral("N");
    force.mode = 0;
    force.range.start = 0;
    force.range.end = forceCeil;
    force.color = QColor(0, 0, 200);

    chart->SetAxisParameter(axisX, { up, dw, force });
}
}  // namespace BondCurve

BondCurveRecorder::BondCurveRecorder(ISTAR::GUI::CustomChart *chart, QObject *parent)
    : QObject(parent)
    , m_chart(chart)
{
    // 同一信号已有其它消费者（IOPage 等），这里再加一个即可（Qt 信号多接收者是常态）
    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_IoObserverData,
            this, &BondCurveRecorder::onIoObserver);
    // 整机状态：任务结束/操作员按"停止"都要收尾（stopTask 不发"流程完成"事件）
    connect(StatusManager::instance(), &StatusManager::sigStatusChanged,
            this, &BondCurveRecorder::onStatusChanged);
}

void BondCurveRecorder::startSession()
{
    if (m_chart == nullptr)
    {
        return;
    }
    m_chart->ClearView();   // 先清图，再开窗
    // 新的一段从基线量程重新开始（上一段可能已经往外扩过）
    m_tempCeil = BondCurve::kTempAxisBaselineMax;
    m_forceCeil = BondCurve::kForceAxisBaselineMax;
    applyRange();
    m_chart->replot();      // 立即重绘：否则"已清空"要等下一个采样点才可见
    m_clock.start();
    // 开窗时整机若已在跑（自动链在"键合"之前就 BUSY 了），先记下来，
    // 免得随后那条滞后推送来的 IDLE 把刚开的窗口误关
    m_sawRunning = (StatusManager::instance()->status() != MctrlModule::MCTRL_STATUS_IDLE);
    m_recording = true;
}

void BondCurveRecorder::finishSession()
{
    m_recording = false;    // 只关窗：图上已有的点留在原地（定格）
}

void BondCurveRecorder::setCurveVisible(const QString &name, bool visible)
{
    m_visible[name] = visible;
    if (m_chart != nullptr)
    {
        m_chart->SetAxisYVisible(name, visible);
    }
}

void BondCurveRecorder::applyRange()
{
    if (m_chart == nullptr)
    {
        return;
    }
    BondCurve::applyAxisRange(m_chart, m_tempCeil, m_forceCeil);

    // SetAxisParameter 会把隐藏过的 Y 轴重新显示出来（曲线本身还是隐藏的），
    // 按勾选框记下来的状态补一次，免得取消勾选的轴自己冒出来。
    const char *names[] = { BondCurve::kAxisUpTemp, BondCurve::kAxisDownTemp, BondCurve::kAxisForce };
    for (const char *n : names)
    {
        const QString name = QString::fromLatin1(n);
        m_chart->SetAxisYVisible(name, m_visible.value(name, true));
    }
}

void BondCurveRecorder::onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr)
{
    Q_UNUSED(prev);

    if (curr == MctrlModule::MCTRL_STATUS_BUSY || curr == MctrlModule::MCTRL_STATUS_ALARM)
    {
        m_sawRunning = true;   // 本段确实在跑
        return;                // 报警不关窗：操作员 RETRY 后本段还要接着记
    }

    // 其余状态（IDLE=流程跑完或按了停止 / DISCONNECTED=断链 / 其它非运行态）= 任务不在了 → 收尾。
    // 只在"本段跑过"时才关：Start 之后控制层还有一条滞后推送会报 IDLE，
    // 不加这个条件会把刚开的窗口立刻关掉（什么也记不到）。
    if (m_recording && m_sawRunning)
    {
        finishSession();
    }
}

void BondCurveRecorder::onIoObserver(const MctrlModule::IoObserverData &data)
{
    if (!m_recording || m_chart == nullptr)
    {
        return;
    }

    // 到量程就无条件收尾：流程即使还在跑也不再收点
    if (m_clock.elapsed() > BondCurve::kMaxSeconds * 1000)
    {
        finishSession();
        return;
    }

    double up = 0.0;
    double dw = 0.0;
    double force = 0.0;
    // 三个通道任一读不到就跳过这一拍，不画半截（下一拍再补）
    if (!findPlcAi(data.plc, BondCurve::kVarUpTemp, up) ||
        !findPlcAi(data.plc, BondCurve::kVarDownTemp, dw) ||
        !findPlcAi(data.plc, BondCurve::kVarForce, force))
    {
        return;
    }

    // 【先固定基线，超过了再扩】：量程在我们手里（Y 轴 mode=0），只有真超了才重设一次，
    // 而且按整数档往上跳（不贴着数据走），一段之内只放大不缩小。
    // 顺序：必须在 AddAxisData 之前重设。重设会把 X 一起写回配置值（瞬回 kMaxSeconds），
    // 紧接着这次加点就把它拉回自适应 —— 中间不 replot，所以画面不会闪回 0~300s。
    const double tempTop = std::max(up, dw);   // 两条温度共用一个上限，尺度保持一致
    if (tempTop > m_tempCeil || force > m_forceCeil)
    {
        if (tempTop > m_tempCeil)
        {
            m_tempCeil = std::ceil(tempTop / BondCurve::kTempAxisStep) * BondCurve::kTempAxisStep;
        }
        if (force > m_forceCeil)
        {
            m_forceCeil = std::ceil(force / BondCurve::kForceAxisStep) * BondCurve::kForceAxisStep;
        }
        applyRange();
    }

    const double t = static_cast<double>(m_clock.elapsed()) / 1000.0;
    QHash<QString, QPointF> point;
    point[QString::fromLatin1(BondCurve::kAxisUpTemp)]   = QPointF(t, up);
    point[QString::fromLatin1(BondCurve::kAxisDownTemp)] = QPointF(t, dw);
    point[QString::fromLatin1(BondCurve::kAxisForce)]    = QPointF(t, force);
    m_chart->AddAxisData(point);
    m_chart->replot(QCustomPlot::rpQueuedReplot);
}
