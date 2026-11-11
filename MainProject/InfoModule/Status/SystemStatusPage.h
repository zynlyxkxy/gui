#ifndef SYSTEMSTATUSPAGE_H
#define SYSTEMSTATUSPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QHash>
#include <QLabel>
#include <QTimer>
#include "mctrl_client.h"
#include "RPCModule/MCTRLClientManager.h"
#include "Common/UiTranslator.h"

namespace Ui {
class SystemStatusPage;
}

class SystemStatusPage : public QWidget
{
    Q_OBJECT

public:
    enum Type
    {
        SYS_MC = 1,
        SYS_EH,
        SYS_IO,
        SYS_RP,
        SYS_Motion1,
        SYS_Motion2,
        SYS_Motion3,
        SYS_Motion4,
        SYS_RM,
        SYS_RM2,
        SYS_Vision,
        SYS_CF,
    };

    explicit SystemStatusPage(QWidget *parent = nullptr);
    ~SystemStatusPage();

    void initData();
    void initTable(QTableWidget *table);
    void updateTableHeader(QTableWidget *table, QStringList lst_Header);
    void addTableRowData(QTableWidget *table, int index);

    void refreshTexts();

signals:

private:
    void setConnect();

    void initSystemStatus();
    void stopSystemStatus();
    void onMctrlMessage(const MctrlModule::ProcessMessage &data);
    void updateStatus(const SystemInfo &data);


private:
    Ui::SystemStatusPage *ui;
    UiTranslator m_uiTranslator;

    QHash<ModuleAliveState, QString> m_SubSysStatus;       //状态-样式表 : 颜色显示
    QHash<int, QString> m_SubSysNames;                     //分系统名称表
    QMap<int, ModuleAliveState> m_dic_SubSysStatus;        //分系统状态缓存
    QMap<int, QLabel *> m_dic_Lable;                       //分系统控件定位

    QTimer m_requestTimeout;   // 10s 请求超时：Response 没到就恢复 UI（控制层挂死兜底）

};

#endif // SYSTEMSTATUSPAGE_H
