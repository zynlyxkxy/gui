#include "SystemStatusPage.h"
#include "ui_SystemStatusPage.h"

#include "dialog/messagedialog.h"
#include <QString>
#include "istar_ui_stylesheet.h"
#include <QPushButton>
#include "groupbox/groupbox.h"
#include "istar_enumconverter.hpp"
#include "StatusModule/StatusManager.h"
#include "ProcessModule/Protocol/ProcessManagerProtocol.h"
#include "istar_language.h"

#define TR_STR(msg)  QString::fromStdString(Tr(msg))

using namespace ISTAR::GUI;
using namespace ISTAR::UIStyle;

SystemStatusPage::SystemStatusPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemStatusPage)
{
    ui->setupUi(this);


    ui->gb_SubsystemStatus->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
    ui->gb_SubsystemStatus->SetheaderFontSize(12);

    // 左边栏 GroupBox — 标题/装饰条/平台样式
    ui->sideGroup->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle);
    ui->sideGroup->SetDecorateColor("transparent");
    ui->sideGroup->SetHeaderText("");

    QString btnStyle = QString(
        "QPushButton{background-color:#FFFFFF;color:#808080;border-radius:5px;}"
        "QPushButton:hover{border:2px solid #A3B6FF;color:#A3B6FF;}"
        "QPushButton:pressed{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:checked{border:2px solid #4C70FA;color:#4C70FA;}"
        "QPushButton:disabled{color:#E0E0E0;}")
        + iStarUIStyleSheet->fontStyle_bold_12;
    ui->btn_Init->setStyleSheet(btnStyle);
    ui->btn_Stop->setStyleSheet(btnStyle);

    ui->gb_SubsystemStatus->SetHeaderText(QStringLiteral("Subsystem Status"));
    ui->gb_SubsystemStatus->setStyleSheet(iStarUIStyleSheet->GroupBoxStyle + iStarUIStyleSheet->DoubleSpinBox_Dark);
    ui->gb_SubsystemStatus->SetheaderFontSize(14);

    initData();
    //更新Table
    initTable(ui->tw_1);
    initTable(ui->tw_2);

    QStringList lst_Header;
    lst_Header << TR_STR("SubSystem") << TR_STR("Status");
    updateTableHeader(ui->tw_1, lst_Header);
    updateTableHeader(ui->tw_2, lst_Header);

    QList<int> keys = m_dic_SubSysStatus.keys();
    int index = 0;
    for(auto &key : keys)
    {
        if(index % 2 == 0)
        {
            addTableRowData(ui->tw_1, key);
        }
        else
        {
            addTableRowData(ui->tw_2, key);
        }
        index++;

        m_dic_Lable[key]->setStyleSheet(m_SubSysStatus[MODULE_ALIVE_STATE_UNKNOWN]);
        QString status = QString::fromStdString(ISTAR::ISTAREnumConverter::ConvertToString(MODULE_ALIVE_STATE_UNKNOWN)).split("_").last();
        // 英文原稿挂成 key：显示文字是时刻变的，不挂 key 的话翻成中文后就没法再切回去
        m_dic_Lable[key]->setProperty("trKey", status);
        m_dic_Lable[key]->setText(TR_STR(status.toStdString()));
    }

    m_requestTimeout.setInterval(10000);
    m_requestTimeout.setSingleShot(true);

    setConnect();
}

void SystemStatusPage::setConnect()
{
    // 订阅状态模块（纯显示，不直接碰 RPC）
    connect(StatusManager::instance(), &StatusManager::sigSystemInfoUpdated, this,
            [this](const MctrlModule::SystemInfo &data)
    {
        updateStatus(data);
    });

    connect(&m_requestTimeout, &QTimer::timeout, this, [this]()  // 10 秒后若未 stop 则触发
    {
        ui->contentWidget->setEnabled(true);
    });

    connect(ui->btn_Init, &QPushButton::clicked, this, [this]()
    {
        initSystemStatus();
    });

    connect(ui->btn_Stop, &QPushButton::clicked, this, [this]()
    {
        stopSystemStatus();
    });

    connect(MCTRLClientManager::getInstance(), &MCTRLClientManager::sig_ProcessObserverData, this,
            [this](const MctrlModule::ProcessMessage &data)
    {
        onMctrlMessage(data);
    });

}

void SystemStatusPage::onMctrlMessage(const MctrlModule::ProcessMessage &data)
{
    if(data.process != static_cast<int>(ProcessID::SingleProc_Init))
    {
        return;
    }
    // 任务层命令：manager 自己消化，不下发流程
    switch (data.command)
    {
        case ProcessManagerCMD_Response:
        {
            S_ProcessManagerResponseData res =
                    Serializer::from_rawJson<S_ProcessManagerResponseData>(data.json);
            m_requestTimeout.stop();
            ui->contentWidget->setEnabled(true);
            if (!res.success)
            {
                MessageDialog dlg(MessageDialog::Error, this,
                                  QString::fromStdString(res.errorInfo));
                dlg.exec();
            }
            return;
        }

        case ProcessManagerCMD_SingleProcFinished:
        {
            S_SingleProcFinishedData res =
                    Serializer::from_rawJson<S_SingleProcFinishedData>(data.json);

            const auto type = (res.status == 0)
                                 ? MessageDialog::Success : MessageDialog::Error;
            const std::string msg = (res.status == 0) ? Tr("Success") : Tr(res.reason);
            MessageDialog dlg(type, this, QString::fromStdString(msg));
            dlg.exec();
            return;
        }
    }
}

void SystemStatusPage::initSystemStatus()
{
    ui->contentWidget->setEnabled(false);
    m_requestTimeout.start();

    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_Init);
    msg.command = ProcessManagerCMD_Start;
    // 异步发送：应答走 ProcessManagerCMD_Response 广播（onMctrlMessage 已接，含超时兜底）
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                msg, [](ResponseNoDataMessageStruct &)
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

void SystemStatusPage::stopSystemStatus()
{
    ui->contentWidget->setEnabled(false);
    m_requestTimeout.start();

    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_Init);
    msg.command = ProcessManagerCMD_Stop;
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommandAsync(
                msg, [](ResponseNoDataMessageStruct &)
    {
        return ISTAR_RESULT_SUCCESS;
    });
}

void SystemStatusPage::updateStatus(const SystemInfo &data)
{
    // 形参必须 const 引用：调用侧是 for (const auto &sub : ...)，sub.name 自带 const
    auto keyForModuleName = [](const std::string &name)->int{
        if (name == "rm")        return SYS_RM;
        if (name == "rm2")       return SYS_RM2;
        if (name == "mc")        return SYS_MC;
        if (name == "eh")        return SYS_EH;
        if (name == "io")        return SYS_IO;
        if (name == "rp")        return SYS_RP;
        if (name == "vision")    return SYS_Vision;
        if (name == "cf")        return SYS_CF;
        if (name == "motion1")   return SYS_Motion1;
        if (name == "motion2")   return SYS_Motion2;
        if (name == "motion3")   return SYS_Motion3;
        if (name == "motion4")   return SYS_Motion4;
        return 0;               // 未知模块：不显示（下面按无效键跳过）
       };

       for (const auto &sub : data.modules)
       {
           if (sub.state == MODULE_ALIVE_STATE_UNKNOWN)
           {
               continue;                       // 非 online/offline 不刷新
           }

           const int key = keyForModuleName(sub.name);    // 一次算好，别一行调三遍
           QLabel *lab = m_dic_Lable.value(key, nullptr);
           if (lab == nullptr)
           {
               continue;                                  // 表里没有的模块：忽略，绝不插空指针
           }

           const ModuleAliveState st = (sub.state == 1) ? MODULE_ALIVE_STATE_ONLINE
                                                        : MODULE_ALIVE_STATE_OFFLINE;
           lab->setStyleSheet(m_SubSysStatus[st]);
           const QString status = QString::fromStdString(
                       ISTAR::ISTAREnumConverter::ConvertToString(st)).split("_").last();
           lab->setProperty("trKey", status);
           lab->setText(TR_STR(status.toStdString()));
       }

}

void SystemStatusPage::initData()
{
    // 分系统名称表
    m_SubSysNames[SYS_MC]     = "MC";
    m_SubSysNames[SYS_EH]     = "EH";
    m_SubSysNames[SYS_IO]     = "IO";
    m_SubSysNames[SYS_RP]     = "RP";
    m_SubSysNames[SYS_Motion1] = "Motion1";
    m_SubSysNames[SYS_Motion2] = "Motion2";
    m_SubSysNames[SYS_Motion3] = "Motion3";
    m_SubSysNames[SYS_Motion4] = "Motion4";
    m_SubSysNames[SYS_RM]     = "RM";
    m_SubSysNames[SYS_RM2]     = "RM2";
    m_SubSysNames[SYS_Vision] = "Vision";
    m_SubSysNames[SYS_CF]     = "CF";

    //颜色
    m_SubSysStatus[MODULE_ALIVE_STATE_ONLINE] = QString("background-color: rgb(0, 255, 127); border:1px solid #666666; border-radius:15px; color:rgb(255,255,255);");
    m_SubSysStatus[MODULE_ALIVE_STATE_UNKNOWN] = QString("background-color: rgb(220, 220, 220); border:1px solid #666666; border-radius:15px; color:rgb(255,255,255);");
    m_SubSysStatus[MODULE_ALIVE_STATE_OFFLINE] = QString("background-color: rgb(255, 70, 70); border:1px solid #666666; border-radius:15px; color:rgb(255,255,255);");

    QList<int> keys = m_SubSysNames.keys();
    for(auto &key : keys)
    {
        m_dic_SubSysStatus.insert(key, MODULE_ALIVE_STATE_UNKNOWN);
    }
}

void SystemStatusPage::initTable(QTableWidget *table)
{
    if(table)
    {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setStyleSheet(iStarUIStyleSheet->tableStyle);
    }
}

void SystemStatusPage::updateTableHeader(QTableWidget *table, QStringList lst_Header)
{
    if(table)
    {
        table->setRowCount(0);
        table->setColumnCount(lst_Header.count());
        table->setHorizontalHeaderLabels(lst_Header);
        if(lst_Header.count() == 3)
        {
            table->setColumnWidth(0, 300);
            table->setColumnHidden(2, true);
        }
    }
}

void SystemStatusPage::addTableRowData(QTableWidget *table, int index)
{
    QString sheet = QString("background-color: rgb(255, 255, 255); border:1px solid #666666; border-radius:15px;");
    if(table)
    {
        int count = table->rowCount();
        table->insertRow(count);

        QLabel *lab_Name = new QLabel();
        lab_Name->setText(TR_STR(m_SubSysNames[index].toStdString()));
        lab_Name->setAlignment(Qt::AlignCenter);
        lab_Name->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
        table->setCellWidget(count, 0, lab_Name);

        QLabel *lab_State = new QLabel();
        lab_State->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
        lab_State->setText("");
        lab_State->setFixedSize(150, 30);
        lab_State->setAlignment(Qt::AlignCenter);
        lab_State->setStyleSheet(sheet);
        m_dic_Lable.insert(index, lab_State);

        QWidget *wgt = new QWidget(this);
        QHBoxLayout *layout = new QHBoxLayout;
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(lab_State);
        wgt->setLayout(layout);
        table->setCellWidget(count, 1, wgt);
        table->setRowHeight(count, 40);
    }

}

SystemStatusPage::~SystemStatusPage()
{
    delete ui;
}

void SystemStatusPage::refreshTexts()
{
    m_uiTranslator.refresh(this);
}
