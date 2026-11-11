#include "DownWaferLoadUnload.h"

#include "ui_MaterialPage.h"
#include "dialog/messagedialog.h"
#include "istar_language.h"
#include <QPushButton>

using namespace ISTAR::GUI;

DownWaferLoadUnload::DownWaferLoadUnload(int id, QWidget *tabPage, Ui::MaterialPage *ui,
                                         QWidget *dialogParent)
    : MaterialTabBase(id, tabPage, ui, dialogParent)
{
    // 本 tab 无 GroupBox、无输入控件：只挂信号
    setConnect();

    ui->btn_moveToLoadPos->setEnabled(true);
    ui->btn_manualLoadComplete->setEnabled(false);

    ui->btn_moveToUnloadPos->setEnabled(true);
    ui->btn_manualUnLoadComplete->setEnabled(false);
}

void DownWaferLoadUnload::setConnect()
{
    // 本类非 QObject：connect 的 context 用 m_dialogParent（MaterialPage），
    // 与 Process/ 下各 Proc 的写法一致
    QObject::connect(ui->btn_moveToLoadPos, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onMoveToLoadPos();
    }, Qt::UniqueConnection);
    QObject::connect(ui->btn_manualLoadComplete, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onManualLoadComplete();
    });
    QObject::connect(ui->btn_moveToUnloadPos, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onMoveToPickPos();
    });
    QObject::connect(ui->btn_manualUnLoadComplete, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onManualUnLoadComplete();
    });

}

void DownWaferLoadUnload::sendManagerCommand(int processId, int command)
{
    MctrlModule::ProcessMessage msg;
    msg.process = processId;
    msg.command = command;
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommand(msg);
}

void DownWaferLoadUnload::clearData()
{
    // 本 tab 无运行结果显示，无需复位
}

// ---- 手动操作：控制层命令协议（MaterialProtocol）定好后在这里下发 ----

void DownWaferLoadUnload::onMoveToLoadPos()
{
    // TODO 下发"移动到上片位"
    sendManagerCommand(static_cast<int>(ProcessID::SingleProc_WaferLoad), ProcessManagerCMD_Start);
    ui->btn_moveToLoadPos->setEnabled(false);
}

void DownWaferLoadUnload::onManualLoadComplete()
{
    // TODO 下发"手动上片完成"
    sendManagerCommand(static_cast<int>(ProcessID::SingleProc_WaferLoad), WaferLoadCMD_ManualLoadFinish);
}

void DownWaferLoadUnload::onMoveToPickPos()
{
    // TODO 下发"移动到取片位"
    sendManagerCommand(static_cast<int>(ProcessID::SingleProc_WaferUnload), ProcessManagerCMD_Start);
    ui->btn_moveToUnloadPos->setEnabled(false);
}

void DownWaferLoadUnload::onManualUnLoadComplete()
{
    // TODO 下发"手动下片完成"
    sendManagerCommand(static_cast<int>(ProcessID::SingleProc_WaferUnload), WaferUnLoadCMD_ManualUnLoadFinish);
}

void DownWaferLoadUnload::ReceiveMctrlCommand(const ProcessMessage &data)
{ 
    if(data.process == static_cast<int>(ProcessID::SingleProc_WaferLoad))
    {

        if(data.command == WaferLoadCMD_MoveToLoadingPosFinish)
        {
            MessageDialog dlg(MessageDialog::Success, m_dialogParent
                              , QString::fromStdString("Reached the target Position"));
            dlg.exec();

            ui->btn_moveToLoadPos->setEnabled(true);
            ui->btn_manualLoadComplete->setEnabled(true);
            return;
        }
        else
        {
            ui->btn_moveToLoadPos->setEnabled(true);
            ui->btn_manualLoadComplete->setEnabled(false);
            return;
        }
    }

    if(data.process == static_cast<int>(ProcessID::SingleProc_WaferUnload))
    {
        if(data.command == WaferUnLoadCMD_MoveToUnLoadingPosFinish)
        {
            MessageDialog dlg(MessageDialog::Success, m_dialogParent
                              , QString::fromStdString("Reached the target Position"));
            dlg.exec();

            ui->btn_moveToUnloadPos->setEnabled(true);
            ui->btn_manualUnLoadComplete->setEnabled(true);
            return;
        }
        else
        {
            ui->btn_moveToUnloadPos->setEnabled(true);
            ui->btn_manualUnLoadComplete->setEnabled(false);
            return;
        }

    }

}
