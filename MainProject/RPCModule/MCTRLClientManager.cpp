#include "MCTRLClientManager.h"

#include <istar_log.h>

// 底层客户端 / 数据 / 回调 funCode 均来自本工程自生成的 mctrl 协议
#include <mctrl_client.h>
#include <mctrl_define.h>

MCTRLClientManager::MCTRLClientManager()
{
    // observer 回调在 RPC 线程触发, 跨线程 queued 连接需先注册元类型
    qRegisterMetaType<MctrlModule::StageObserverDataList>("MctrlModule::StageObserverDataList");
    qRegisterMetaType<MctrlModule::IoObserverData>("MctrlModule::IoObserverData");
    qRegisterMetaType<MctrlModule::ProcessMessage>("MctrlModule::ProcessMessage");
    qRegisterMetaType<MctrlModule::SystemInfo>("MctrlModule::SystemInfo");
}

MCTRLClientManager::~MCTRLClientManager()
{
    Shutdown();
}

MCTRLClientManager *MCTRLClientManager::getInstance()
{
    static MCTRLClientManager instance;
    return &instance;
}

bool MCTRLClientManager::Initialize(const std::string &configFile, const std::string &port)
{
    if (m_mc)
    {
        return true;
    }

    m_mc = std::make_unique<MctrlModule::MCTRL_Client_Inherit>();
    if (m_mc->MCTRLClientInit(configFile, port) != ISTAR_RESULT_SUCCESS)
    {
        ISTARLogError("MCTRLClientManager: MCTRLClientInit failed");
        m_mc.reset();
        return false;
    }

    // 注册 observer 回调, 统一转发为信号
    // 运动台 observer: 一次推全部 stage 的列表
    m_mc->RegisterCallback(MOTION_OBSERVER_DATA_ID,
        std::function<ISTARResultStatus(MctrlModule::StageObserverDataList &)>(
            [this](MctrlModule::StageObserverDataList &data) -> ISTARResultStatus
    {
        emit sig_StageObserverData(data);
        return ISTAR_RESULT_SUCCESS;
    }));

    // IO observer: 一次推 ACS + PLC 全部缓存
    m_mc->RegisterCallback(I_O_OBSERVER_DATA_ID,
        std::function<ISTARResultStatus(MctrlModule::IoObserverData &)>(
            [this](MctrlModule::IoObserverData &data) -> ISTARResultStatus
    {
        emit sig_IoObserverData(data);
        return ISTAR_RESULT_SUCCESS;
    }));

    // Process observer
    m_mc->RegisterCallback(MCTRL_TO_GUI_COMMAND_ID,
        std::function<ISTARResultStatus(MctrlModule::ProcessMessage &)>(
            [this](MctrlModule::ProcessMessage &data) -> ISTARResultStatus
    {
        emit sig_ProcessObserverData(data);
        return ISTAR_RESULT_SUCCESS;
    }));

    //SystemInfo observer
    m_mc->RegisterCallback(MCTRL_TO_GUI_SYSTEM_INFO_ID,
        std::function<ISTARResultStatus(MctrlModule::SystemInfo &)>(
            [this](MctrlModule::SystemInfo &data) -> ISTARResultStatus
    {
        emit sig_SystemInfoObserverData(data);
        return ISTAR_RESULT_SUCCESS;
    }));

    return true;
}

void MCTRLClientManager::Shutdown()
{
    if (m_mc)
    {
        m_mc->ExitRun();
        m_mc.reset();
    }
}
