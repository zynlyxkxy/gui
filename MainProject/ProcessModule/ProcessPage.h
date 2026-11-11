#ifndef PROCESSPAGE_H
#define PROCESSPAGE_H

#include <QWidget>

#include "mctrl_client.h"                       // ProcessMessage / MctrlStatus
#include "RPCModule/MCTRLClientManager.h"
#include "Protocol/ProcessManagerProtocol.h"    // ProcessID / ProcessManagerCMD
#include "Common/UiTranslator.h"
#include "StatusModule/StatusManager.h"

#include "Process/ChuckLevelProc.h"
#include "Process/WaferLevelCollectProc.h"
#include "Process/ChipPosCalibProc.h"
#include "Process/CoarseLevelProc.h"
#include "Process/DownLevelSelectProc.h"
#include "Process/UpLevelSelectProc.h"
#include "Process/FineLevelProc.h"
#include "Process/FineAlignProc.h"
#include "Process/BondProc.h"
#include "Process/BlueFilmChipHandoverProc.h"

#include <map>
#include <memory>
#include <vector>

namespace Ui {
class ProcessPage;
}
class RecipeChooseWidget;

/**
 * 流程管理页（manager）：
 * - 流程装配：m_procs 下标 = ProcessID = tab 下标，vector 直取路由；
 * - 运行控制：Start = 当前 tab 流程（先 clearData 再发命令）；Stop 一发停所有
 *   在跑流程（控制层语义，不区分 id），GUI 不记录运行中的流程；
 * - 按钮使能：refreshRunControls 按状态簇摊开处理（原 onStatusChanged 的 switch），
 *   Response 失败 / SingleProcFinished 以"无迁移"方式重刷（不触发广播）；
 *   Start/Stop 发出即全灰防连点，恢复交给回包或状态信号；
 * - 进度：只显示控制层上报的 ProcessManagerCMD_Progress，GUI 不决定进度值；
 *   运行态收尾由状态回 IDLE 驱动，Response 失败即时按当前状态恢复；
 * - 处方：选择后交给 MCRecipeManager（数据/变更判定归单例），收到 sigRecipeUpdated
 *   再填预览 tab 并触发全员 updateRecipe（只控时机）；本页选中处方时另外追发一次
 *   蓝膜查询，广播回调里不做任何查询/弹窗（谁选谁查、谁发谁报错）。
 * - 状态：订阅 StatusManager 信号（sigStatusChanged），刷新 UI + 广播全部流程。
 */
class ProcessPage : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessPage(QWidget *parent = nullptr);
    ~ProcessPage() override;

    // 整页翻译（语言切换时 MainWindow 调用）
    void refreshTexts();

private:
    // ---- 初始化 ----
    void initStyle();             // 样式铺底
    void initUi();                 // 标题声明 + 只读设置
    void createProcesses();       // 流程装配 + 枚举与 tab 布局自检
    void initConnect();

    // 注册一个流程（id = 枚举 = tab 下标）
    template<typename ProcT>
    void registerProcess(ProcessID id);

    // ---- 运行控制 ----
    void startCurrentProcess();
    void stopCurrentProcess();
    void sendManagerCommand(int processId, int command);
    void onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);
    void refreshRunControls(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);   // 按状态簇刷运行控制区按钮

    // ---- 处方 ----
    void onRecipeUpdated();       // 预览填充 + 全员 updateRecipe（sigRecipeUpdated 触发，只刷显示）
    void requestBlueFilmChips();  // 向 mctrl 查一次蓝膜芯片全量快照（本页选中处方时发）

    void onMcUpdated();

    // ---- 控制层 ----
    void onMctrlMessage(const MctrlModule::ProcessMessage &data);

    Ui::ProcessPage *ui;
    RecipeChooseWidget *m_pRecipeDlg;
    UiTranslator m_uiTranslator;

    bool m_isSelectRecipe = false;   // 已选处方（Start 门槛；控制层重启后要求重选）
    bool m_blueFilmReplyPending = false;   // 本页刚发过蓝膜查询，等它的答复（谁发谁报错）
    std::map<int, std::unique_ptr<ProcessBase>> m_procs;   // 流程 ID → 流程（仅注册有 tab 的单步流程）

};

#endif // PROCESSPAGE_H
