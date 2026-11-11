#ifndef MATERIALPAGE_H
#define MATERIALPAGE_H

#include <QWidget>

#include "Common/UiTranslator.h"
#include "Common/RecipeChooseWidget.h"
#include "RPCModule/MCTRLClientManager.h"
#include "StatusModule/StatusManager.h"
#include "mctrl_client.h"

#include "MaterialModule/Tabs/DownWaferLoadUnload.h"
#include "MaterialModule/Tabs/BlueFilmLoadUnload.h"

#include <map>
#include <memory>

namespace Ui {
class MaterialPage;
}

class MaterialTabBase;


/**
 * 材料页（manager）：与 ProcessModule/ProcessPage 同构 ——
 * 页面只负责装配（tab 下标 → tab 对象，直取路由）、状态广播、处方时机；
 * 每个 tab 的手动操作各自在 Tabs/ 下的类里。
 *
 * 处方：订阅 MCRecipeManager::sigRecipeUpdated，收到后只刷本页显示；
 * 本页选中处方时另外追发一次蓝膜查询（广播回调里不查、不弹窗）。
 */
class MaterialPage : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialPage(QWidget *parent = nullptr);
    ~MaterialPage() override;

    // 整页翻译（语言切换时 MainWindow 调用）
    void refreshTexts();

private:
    // ---- 初始化 ----
    void initStyle();        // 样式铺底
    void initUi();           // 页面级控件声明 + 只读设置
    void createTabs();       // tab 装配 + 枚举与 tab 布局自检
    void initConnect();

    // 注册一个 tab：id = 主流程 id（枚举 = tab 下标），id_ = 同一 tab 认领的第二个流程 id
    //（上下片一对共用一个 tab 对象：一个 tab 页里同时有上片/下片按钮与消息）
    template<typename TabT>
    void registerTab(ProcessID id, ProcessID id_);

    // ---- 状态 ----
    void onStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);

    // ---- 处方 ----
    void onRecipeUpdated();       // 预览填充 + 全员 updateRecipe（sigRecipeUpdated 触发，只刷显示）
    void onMcUpdated();

    // 向 mctrl 查询蓝膜芯片全量快照（本页选中处方时发；答复走广播，各页面自行消费）
    void requestBlueFilmChips();

    // ---- 控制层 ----
    void onMctrlMessage(const MctrlModule::ProcessMessage &data);

private:
    Ui::MaterialPage *ui;

    RecipeChooseWidget *m_pRecipeDlg;
    UiTranslator m_uiTranslator;
    bool m_isSelectRecipe = false;   // 已选处方（控制层重启后要求重选）
    bool m_blueFilmReplyPending = false;   // 本页刚发过蓝膜查询，等它的答复（谁发谁报错）

    // 流程 id → tab 对象；上下片一对的 12/13 指向同一对象（shared_ptr：允许两个 key 共享所有权，
    // 从而保证同一批 ui 控件只被一个 tab 实例 connect 一次）
    std::map<int, std::shared_ptr<MaterialTabBase>> m_tabs;
};

#endif // MATERIALPAGE_H
