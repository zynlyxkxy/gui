#ifndef PROCESSBASE_H
#define PROCESSBASE_H

#include "mctrl_client.h"        // ProcessMessage
#include "MCRecipeModule/MCRecipeManager.h"   // 处方数据单例（m_rcpChip / m_rcpWafer 的引用源）
#include <functional>
#include <string>
#include <vector>

class QWidget;                   // 仅前向声明：弹窗父窗口 / connect context（传 ProcessPage）

namespace Ui {
class ProcessPage;               // 仅前向声明：控件统一经 ui 指针访问
}

// 流程状态直接复用 mctrl 主状态机：MctrlModule::MctrlStatus
//（mctrl_client.h 已引入并 using，取值 MCTRL_STATUS_UNKNOWN / LOADING / LOAD_FAIL /
//  LOAD_OK / INITIALIZING / INI_FAIL / IDLE / BUSY / ALARM）

class ProcessBase
{
public:
    ProcessBase(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);
    virtual ~ProcessBase();

    // 流程状态：默认 IDLE/BUSY 亮 tab，其余灰（含 DISCONNECTED / ALARM / UNKNOWN...）；
    // 需要额外行为的流程覆写（如 ChipPosCalib 的 BUSY 锁交互）
    virtual void setStatus(MctrlStatus status);

    // 清空运行结果 / 已填的值（流程重跑前调用；处方数据不在此清）
    virtual void clearData();

    // 更新处方：把 m_rcpChip / m_rcpWafer（单例数据的只读引用，选处方后自动是新值）
    // 刷新到本 tab 控件；何时触发由 ProcessPage 控制（订阅 MCRecipeManager::sigRecipeUpdated）
    virtual void updateRecipe();

    virtual void updateMc();

    // 注入 GUI → mctrl 的上报回调（命令 + json 载荷），子类经 m_mctrlReporter 上报
    void SetMctrlReporter(const std::function<void(int, const std::string &)> &reporter);

    // 收到 mctrl 流程消息（ProcessPage 按流程 id 路由转发）
    virtual void ReceiveMctrlCommand(const ProcessMessage &data);

    // 接口全部默认空实现：无界面的流程（如 Init）可直接以基类实例注册；
    // 有界面的流程按需覆写

protected:
    // 本 tab 控件的信号槽连接：子类覆写并自行调用（外部不可调用）
    virtual void setConnect();

protected:
    Ui::ProcessPage *ui = nullptr;               // 钥匙：整页控件的类型安全访问（所有 tab 共用 ProcessPage.ui）
    QWidget *m_tabPage = nullptr;                // 地盘：本流程所在 tab 页（ProcessPage 构造时注入）
    QWidget *m_dialogParent = nullptr;           // 弹窗父窗口 / connect context（传 ProcessPage）
    std::function<void(int, const std::string &)> m_mctrlReporter;   // GUI → mctrl 上报（命令 + json 载荷）

    // ---- 数据小工具（纯工具，非判断接口；快照与比较归各流程自己维护）----
    // 处方点表提取：list 元素带 .point（S_Point2D）
    template <typename T>
    static std::vector<S_Point2D> toPoints(const std::vector<T> &list)
    {
        std::vector<S_Point2D> out;
        out.reserve(list.size());
        for (const T &item : list)
        {
            out.push_back(item.point);
        }
        return out;
    }

    // 点表逐项比较（S_Point2D 无 operator==；同源 JSON 解析位级相同）
    static bool samePoints(const std::vector<S_Point2D> &a, const std::vector<S_Point2D> &b);

    // 处方数据（单例成员的只读引用：选处方后单例原地赋值，引用自动指向新值）
    const S_ChipRecipe &m_rcpChip;
    const S_RecipeContentWafer &m_rcpWafer;
    const S_MachineConstant &m_mc;

    int m_id = -1;   // 流程 id（构造注入，暂无对外读取）

    // 本 tab 整体置灰 / 恢复（报警态 setStatus 用）
    void setTabEnabled(bool enabled);
};

#endif // PROCESSBASE_H
