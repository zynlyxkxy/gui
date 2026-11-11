#ifndef MCTRLCLIENTMANAGER_H
#define MCTRLCLIENTMANAGER_H

#include <QObject>
#include <memory>
#include <string>

// 数据结构来自本工程自生成的 mctrl 协议 (RPCModule/include/mctrlc_data.h)
#include <mctrl_client.h>

// MCTRLClientManager: mctrl 连接管理（单例）。
// 持有唯一的 MCTRL_Client_Inherit 并统一注册 observer 回调转信号；
// 请求接口由业务模块直接通过 getClient() 调用协议方法，本类不做透传。
class MCTRLClientManager : public QObject
{
    Q_OBJECT

public:
    static MCTRLClientManager *getInstance();

    // 初始化单例并建立与 mctrl 的连接；失败返回 false。
    bool Initialize(const std::string &configFile, const std::string &port);

    // 释放底层客户端 (停止 ZMQ 线程)；退出前调用。
    void Shutdown();

    // 底层 client，业务模块直接调协议接口（Motion* / IO* / GetCurRecipeInfo 等）
    MctrlModule::MCTRL_Client_Inherit *getClient() { return m_mc.get(); }

signals:
    void sig_StageObserverData(const MctrlModule::StageObserverDataList &data);
    void sig_IoObserverData(const MctrlModule::IoObserverData &data);
    void sig_ProcessObserverData(const MctrlModule::ProcessMessage &data);
    void sig_SystemInfoObserverData(const MctrlModule::SystemInfo &data);

private:
    MCTRLClientManager();
    ~MCTRLClientManager();

    MCTRLClientManager(const MCTRLClientManager &) = delete;
    MCTRLClientManager &operator=(const MCTRLClientManager &) = delete;

    std::unique_ptr<MctrlModule::MCTRL_Client_Inherit> m_mc;
};

#endif // MCTRLCLIENTMANAGER_H
