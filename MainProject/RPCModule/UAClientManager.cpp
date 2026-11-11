#include "UAClientManager.h"

#include "ua_define.h"
#include "istar_log.h"

UAClientManager::UAClientManager()
{
    // 订阅/异步回调在 RPC 线程触发，跨线程 queued 连接需先注册元类型
    qRegisterMetaType<UserInfo>("UserInfo");
    qRegisterMetaType<LoginResponse>("LoginResponse");
}

UAClientManager::~UAClientManager()
{
    Shutdown();
}

UAClientManager *UAClientManager::getInstance()
{
    static UAClientManager instance;
    return &instance;
}

bool UAClientManager::Initialize(const std::string &configFile)
{
    if (m_bRunning)
    {
        ISTARLogWarn("UAClientManager already initialized");
        return true;
    }

    m_ua = std::make_unique<UA_Client_Inherit>();
    if (m_ua->UAClientInit(configFile, "ua") != ISTAR_RESULT_SUCCESS)
    {
        ISTARLogError("UAClientManager: UAClientInit failed (mw_config middleware.ua 缺失或 UA 服务未起?)");
        m_ua.reset();
        return false;
    }

    // 服务端就绪心跳（约 3s，RPC 线程回调）→ 转发为信号
    m_ua->RegisterCallback(SUBSCRIBE_SERVER_STARTED_ID,
        std::function<ISTARResultStatus(bool &)>(
            [this](bool &ready) -> ISTARResultStatus
    {
        emit sig_ServerStarted(ready);
        return ISTAR_RESULT_SUCCESS;
    }));

    // 当前用户变化订阅（登录/登出/会话失效）→ 转发为信号
    m_ua->RegisterCallback(SUBSCRIBE_USER_CHANGED_ID,
        std::function<ISTARResultStatus(UserInfo &)>(
            [this](UserInfo &user) -> ISTARResultStatus
    {
        emit sig_UserChanged(user);
        return ISTAR_RESULT_SUCCESS;
    }));

    m_bRunning = true;
    ISTARLogInfo("UAClientManager initialized");
    return true;
}

void UAClientManager::Shutdown()
{
    if (!m_bRunning)
    {
        return;
    }
    if (m_ua)
    {
        m_ua->ExitRun();
    }
    m_ua.reset();
    m_bRunning = false;
    ISTARLogInfo("UAClientManager shutdown");
}
