#include "StatusManager.h"

#include "RPCModule/MCTRLClientManager.h"

namespace {
const int kWatchdogMs = 3000;

// 单机调试开关：true = 无控制层单独跑 GUI —— 不启看门狗、初始直接报 IDLE，
// 状态机永不进 DISCONNECTED，界面不整体下使能（控制层真开着时推送照收，只是不监控心跳）。
// 只影响本文件；调试完提交代码前记得改回 false。
const bool kDebugNoCtrl = false;
}

StatusManager *StatusManager::instance()
{
    static StatusManager s_instance;
    return &s_instance;
}

StatusManager::StatusManager()
{
    m_watchdog.setInterval(kWatchdogMs);
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, &StatusManager::onWatchdogTimeout);

    if (kDebugNoCtrl)
    {
        // 调试开关开：初始直接报 IDLE 让界面按正常运行态使能，看门狗不启动；
        // 事件循环起来后再发，确保订阅者都已连上
        m_info.mctrlStatus = MCTRL_STATUS_IDLE;
        QTimer::singleShot(0, this, [this]()
        {
            emit sigStatusChanged(MCTRL_STATUS_UNKNOWN, MCTRL_STATUS_IDLE);
        });
    }
    else
    {
        m_watchdog.start();   // 启动即计时
    }

    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_SystemInfoObserverData,
            this, &StatusManager::onSystemInfo);
}

MctrlModule::MctrlStatus StatusManager::status() const
{
    return m_info.mctrlStatus;
}

MctrlModule::SystemInfo StatusManager::systemInfo() const
{
    return m_info;
}

void StatusManager::onSystemInfo(const MctrlModule::SystemInfo &info)
{
    m_watchdog.start();   // 复位看门狗

    const auto prev = m_info.mctrlStatus;
    m_info = info;
    emit sigSystemInfoUpdated(info);

    if (info.mctrlStatus != prev)
    {
        emit sigStatusChanged(prev, info.mctrlStatus);
    }
}

void StatusManager::onWatchdogTimeout()
{
    if (kDebugNoCtrl)
    {
        return;   // 调试开关开：不监控心跳，永不判离线
    }

    if (m_info.mctrlStatus == MctrlModule::MCTRL_STATUS_DISCONNECTED)
    {
        return;   // 已在断链态
    }

    const auto prev = m_info.mctrlStatus;
    m_info.mctrlStatus = MctrlModule::MCTRL_STATUS_DISCONNECTED;
    emit sigStatusChanged(prev, m_info.mctrlStatus);
}
