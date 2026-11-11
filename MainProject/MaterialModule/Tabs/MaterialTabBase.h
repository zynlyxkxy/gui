#ifndef MATERIALTABBASE_H
#define MATERIALTABBASE_H

#include "mctrl_client.h"        // MctrlStatus（主状态机，与 ProcessBase 同一套取值）
#include "MCRecipeModule/MCRecipeManager.h"   // 处方数据单例（与 ProcessBase 同款只读引用）

#include <functional>            // m_mctrlReporter（与 ProcessBase 同款注入回调）
#include <string>

class QWidget;                   // 仅前向声明：弹窗父窗口 / connect context（传 MaterialPage）

namespace Ui {
class MaterialPage;              // 仅前向声明：控件统一经 ui 指针访问
}

/**
 * Material 页 tab 基类：与 ProcessModule/Base/ProcessBase 同构。
 * 差别：Material 页一个 tab 认领"上下片一对"两个流程（Load / Unload），
 * 所以流程 id 是构造注入的 m_procLoad / m_procUnload，消息由 MaterialPage 路由转发、
 * 上报回调由 MaterialPage 注入（子类不直接依赖 RPC 单例）。
 * 其余（ui 钥匙 + tab 地盘 + 状态联动 + 处方只读引用）与 ProcessBase 一致。
 */
class MaterialTabBase
{
public:
    MaterialTabBase(int id, QWidget *tabPage, Ui::MaterialPage *ui, QWidget *dialogParent);
    virtual ~MaterialTabBase();

    // tab 状态：默认 IDLE/BUSY 亮 tab，其余（UNKNOWN/LOADING/ALARM...）整页灰；
    // 需要额外行为的 tab 覆写
    virtual void setStatus(MctrlStatus status);

    // 运行结果复位（重跑前调用）；默认空实现
    virtual void clearData();

    // 处方更新：把处方值刷到本 tab 控件；默认空实现
    virtual void updateRecipe();

    // 收到 mctrl 流程消息（MaterialPage 按流程 id 路由转发）
    virtual void ReceiveMctrlCommand(const ProcessMessage &data);

    // 注入 GUI → mctrl 的上报回调（流程 id + 命令 + json 载荷），子类经 m_mctrlReporter 上报。
    void SetMctrlReporter(const std::function<void(int, const std::string &)> &reporter);

protected:
    // 本 tab 控件的信号槽连接：子类覆写并在构造末尾自行调用（外部不可调用）
    virtual void setConnect();

    // 本 tab 整体置灰 / 恢复
    void setTabEnabled(bool enabled);

protected:
    Ui::MaterialPage *ui = nullptr;      // 钥匙：整页控件的类型安全访问（所有 tab 共用 MaterialPage.ui）
    QWidget *m_tabPage = nullptr;        // 地盘：本 tab 页（MaterialPage 构造时注入）
    QWidget *m_dialogParent = nullptr;   // 弹窗父窗口 / connect context（传 MaterialPage）
    int m_id = -1;                       // tab 下标（构造注入，暂无对外读取）

    // GUI → mctrl 上报（流程 id + 命令 + json 载荷）；由 MaterialPage 在 tab 装配时注入
    std::function<void(int, const std::string &)> m_mctrlReporter;

    // 处方数据（单例成员的只读引用：选处方后单例原地赋值，引用自动指向新值）
    const S_ChipRecipe &m_rcpChip;
    const S_RecipeContentWafer &m_rcpWafer;
    const S_MachineConstant &m_mc;
};

#endif // MATERIALTABBASE_H
