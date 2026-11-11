#ifndef UACLIENTMANAGER_H
#define UACLIENTMANAGER_H

#include <QObject>
#include <memory>
#include <string>

#include "ua_client.h"

// UAClientManager: UA(权限管理)客户端连接管理（单例）。
// 统一管理 UA_Client_Inherit 的生命周期：
//   - Initialize 建连（mw_config.yaml → middleware.ua）并注册 RPC 线程的订阅回调，
//     把回调转成 Qt 信号（服务端就绪心跳 / 当前用户变化）；
//   - 具体 RPC 请求（UALogin / UAGetLastLogin / UALogout / 用户/等级/权限管理…）
//     由业务模块经 getClient() 直接调用协议方法，本类不做透传。
class UAClientManager : public QObject
{
    Q_OBJECT

public:
    static UAClientManager *getInstance();

    // 建连 + 注册订阅回调；失败返回 false（如 middleware.ua 配置缺失/服务未起）
    bool Initialize(const std::string &configFile);

    // 释放底层客户端（停止 ZMQ 线程）；退出前调用
    void Shutdown();

    // 底层 UA 客户端，业务直接调协议接口
    UA_Client_Inherit *getClient() { return m_ua.get(); }

signals:
    // 服务端就绪心跳（约 3s 一次，RPC 线程回调转发；覆盖 GUI 先于 UA 服务启动/服务重启复位）
    void sig_ServerStarted(bool ready);

    // 当前登录用户变化订阅（登录/登出/会话失效时服务端推送）
    void sig_UserChanged(const UserInfo &user);

private:
    UAClientManager();
    ~UAClientManager();

    UAClientManager(const UAClientManager &) = delete;
    UAClientManager &operator=(const UAClientManager &) = delete;

    std::unique_ptr<UA_Client_Inherit> m_ua;
    bool m_bRunning = false;
};

#endif // UACLIENTMANAGER_H
