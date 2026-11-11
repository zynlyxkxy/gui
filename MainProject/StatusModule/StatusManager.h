#ifndef STATUSMANAGER_H
#define STATUSMANAGER_H

#include "mctrl_client.h"   // MctrlModule::SystemInfo / MctrlStatus

#include <QObject>
#include <QTimer>

/**
 * 机器状态管理单例：SystemInfo 的唯一订阅者和解析者，状态只来自控制层推送。
 *
 * 链路检测：3s 看门狗超时 → 自行切 DISCONNECTED 并广播；
 * 推送恢复 → 自然回到真实状态。同一个信号通道，不额外发明信号。
 *
 * 单机调试：StatusManager.cpp 顶部 kDebugNoCtrl = true —— 不启看门狗、初始直接
 * 广播 IDLE，无控制层单独跑 GUI 时界面不整体下使能；推送来了照收，但心跳永不超时。
 *
 * 所有消费者只订阅本类的信号，不直接碰 RPC。
 */
class StatusManager : public QObject
{
    Q_OBJECT

public:
    static StatusManager *instance();

    MctrlModule::MctrlStatus status() const;
    MctrlModule::SystemInfo systemInfo() const;

signals:
    // 整机状态值变了才发（含 DISCONNECTED）
    void sigStatusChanged(MctrlModule::MctrlStatus prev, MctrlModule::MctrlStatus curr);

    // 每次推送都发（模块存活可能变了但整机状态没变，也需要刷显示）
    void sigSystemInfoUpdated(const MctrlModule::SystemInfo &info);

private:
    StatusManager();
    ~StatusManager() override = default;
    StatusManager(const StatusManager &) = delete;
    StatusManager &operator=(const StatusManager &) = delete;

    void onSystemInfo(const MctrlModule::SystemInfo &info);
    void onWatchdogTimeout();

    MctrlModule::SystemInfo m_info;
    QTimer m_watchdog;   // 3s 单次，收到推送即复位
};

#endif // STATUSMANAGER_H
