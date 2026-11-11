#ifndef IOGROUP_H
#define IOGROUP_H

#include <QWidget>
#include <QTabWidget>
#include <QGridLayout>
#include <QMap>

#include "mctrl_client.h"   // MctrlModule 协议类型（限定名访问）
#include "Common/UiTranslator.h"

class MCTRLClientManager;
class DIWidget;
class DOWidget;
class AIWidget;
class AOWidget;
class SettingWidget;
class BufferWidget;

class IOGroup : public QWidget
{
    Q_OBJECT

public:
    explicit IOGroup(QWidget *parent = nullptr);
    ~IOGroup();

    void buildTabs(const MctrlModule::IoServerInfo &info);
    void setRpcClient(MCTRLClientManager *client);
    void refreshTexts();

signals:
    void sig_IoObserverReceived(const MctrlModule::IoObserverData &data);

private slots:
    void onIoObserverData(const MctrlModule::IoObserverData &data);

private:
    void connectSignals();
    void dispatchAcs(const MctrlModule::AcsObserverData &data);
    void dispatchPlc(const MctrlModule::PlcObserverData &data);

    QTabWidget *m_tabWidget;
    MCTRLClientManager *m_client = nullptr;
    UiTranslator m_uiTranslator;

    QMap<int, AIWidget *> m_aiWidgets;
    QMap<int, AOWidget *> m_aoWidgets;
    QMap<int, DIWidget *> m_diWidgets;
    QMap<int, DOWidget *> m_doWidgets;
    QMap<int, SettingWidget *> m_settingDoubleWidgets;
    QMap<int, SettingWidget *> m_settingBoolWidgets;
    QMap<int, BufferWidget *> m_bufferWidgets;
};

#endif // IOGROUP_H
