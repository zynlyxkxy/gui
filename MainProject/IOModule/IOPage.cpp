#include "IOModule/IOPage.h"
#include "IOModule/IOGroup/IOGroup.h"
#include "RPCModule/MCTRLClientManager.h"

#include <QVBoxLayout>
#include <algorithm>
#include <iostream>

namespace
{

// 过滤保留给控制层内部使用、不暴露给普通用户的 IO 变量：
// 常规号段按 mod 1000 ∈ [500, 999]；setting 号段按 mod 500 ∈ [250, 499]。
// 规则与原控制层过滤一致，现改由 GUI 按用户权限决定是否裁剪。
template <typename T>
void filterReservedIoVars(std::vector<T> &vars, int modulus, int lo, int hi)
{
    vars.erase(std::remove_if(vars.begin(), vars.end(),
        [modulus, lo, hi](const T &v)
        {
            int r = v.id % modulus;
            return r >= lo && r <= hi;
        }),
        vars.end());
}

// 按权限裁剪 IO 列表：超级管理员看全量，其余用户过滤内部号段。
MctrlModule::IoServerInfo visibleIo(const MctrlModule::IoServerInfo &full, bool superAdmin)
{
    MctrlModule::IoServerInfo info = full;
    if (superAdmin)
    {
        return info;
    }

    MctrlModule::IoInfo &io = info.io;
    filterReservedIoVars(io.AI, 1000, 500, 999);
    filterReservedIoVars(io.DI, 1000, 500, 999);
    filterReservedIoVars(io.AO, 1000, 500, 999);
    filterReservedIoVars(io.DO, 1000, 500, 999);
    filterReservedIoVars(io.SettingDouble, 500, 250, 499);
    filterReservedIoVars(io.SettingBool, 500, 250, 499);
    filterReservedIoVars(io.Cmd, 1000, 500, 999);
    filterReservedIoVars(io.Buffer, 1000, 500, 999);
    return info;
}

} // namespace

IOPage::IOPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color:#F3F4FA;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_ioGroup = new IOGroup(this);
    layout->addWidget(m_ioGroup);

    registerCallbacks();
    discoverIo();
}

IOPage::~IOPage()
{
}

void IOPage::refreshTexts()
{
    if (m_ioGroup != nullptr)
    {
        m_ioGroup->refreshTexts();
    }
}

// 权限变化入口：超级管理员看全量，其余用户隐藏控制层内部号段。由 MainWindow 在会话变化时推送。
void IOPage::setSuperAdmin(bool superAdmin)
{
    if (m_superAdmin == superAdmin)
    {
        return;
    }
    m_superAdmin = superAdmin;
    rebuildIoView();
}

void IOPage::registerCallbacks()
{
    // observer 回调已由 MCTRLClientManager 统一注册并 emit 信号;
    // 这里只把 MCTRLClientManager 信号连到 IOGroup, 不再像旧代码那样逐个 RegisterCallback。
    // RPC 回调线程 emit -> queued 信号 -> GUI 主线程执行, 跨线程安全。
    MCTRLClientManager *mc = MCTRLClientManager::getInstance();
    connect(mc, &MCTRLClientManager::sig_IoObserverData,
            m_ioGroup, &IOGroup::sig_IoObserverReceived);
}

// 拉取全量 IO 列表（控制层不再过滤），并按当前权限构建 tab
void IOPage::discoverIo()
{
    if (m_ioDiscovered)
    {
        return;
    }

    MCTRLClientManager *mc = MCTRLClientManager::getInstance();

    if (mc->getClient()->IOGetServerInfo(m_fullIo) != ISTAR_RESULT_SUCCESS)
    {
        std::cout << "[IO] GetIOServerInfo failed" << std::endl;
        return;
    }

    std::cout << "[IO] Discovered IO:"
              << " AI=" << m_fullIo.io.AI.size()
              << " DI=" << m_fullIo.io.DI.size()
              << " AO=" << m_fullIo.io.AO.size()
              << " DO=" << m_fullIo.io.DO.size()
              << " SettingDouble=" << m_fullIo.io.SettingDouble.size()
              << " SettingBool=" << m_fullIo.io.SettingBool.size()
              << " Buffer=" << m_fullIo.io.Buffer.size() << std::endl;

    m_ioGroup->setRpcClient(mc);
    m_ioDiscovered = true;
    rebuildIoView();

    std::cout << "[IO] IO ready" << std::endl;
}

// 用缓存的全量列表按当前权限重建 tab
void IOPage::rebuildIoView()
{
    if (!m_ioDiscovered)
    {
        return;
    }
    m_ioGroup->buildTabs(visibleIo(m_fullIo, m_superAdmin));
}
