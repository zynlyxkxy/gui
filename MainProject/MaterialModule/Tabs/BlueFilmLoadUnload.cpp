#include "BlueFilmLoadUnload.h"

#include "ui_MaterialPage.h"
#include "dialog/messagedialog.h"
#include <QDoubleSpinBox>
#include <QPushButton>

using namespace ISTAR::GUI;

BlueFilmLoadUnload::BlueFilmLoadUnload(int id, QWidget *tabPage, Ui::MaterialPage *ui,
                                       QWidget *dialogParent)
    : MaterialTabBase(id, tabPage, ui, dialogParent)
{
    // 分组框标题：（语言切换由 UiTranslator 懒惰捕获，不在此加 TR_STR）
    ui->gb_blueFilmInfo->SetHeaderText(QStringLiteral("Blue Film Wafer Info"));
    ui->gb_blueFilemLoadUnload->SetHeaderText(QStringLiteral("Blue Film Status Diagram"));

    // 只读展示项：本 tab 只有展示，没有输入
    ui->dsb_blueExpansionDepth->setReadOnly(true);   // 扩膜深度
    ui->dsb_waferNotchOrient->setReadOnly(true);     // 缺口角度

    setConnect();
}

void BlueFilmLoadUnload::setConnect()
{
    QObject::connect(ui->btn_blueMoveToLoadPos, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onMoveToLoadPos();
    });
    QObject::connect(ui->btn_ExpansionMapping, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onExpansionMapping();
    });
    QObject::connect(ui->btn_blueMoveToPickPos, &QPushButton::clicked, m_dialogParent, [this]()
    {
        onMoveToPickPos();
    });
}

void BlueFilmLoadUnload::clearData()
{
    // TODO 重跑前复位：参考 BlueFilmChipHandoverProc::clearData()
    //      （已取走置黑 → 回未取走黄，并清点选标注）
}

void BlueFilmLoadUnload::updateRecipe()
{
    ui->gv_blueFilemLoadUnload->clearBackdrop();

    ui->dsb_blueExpansionDepth->setValue(m_rcpChip.material.blueWaferInfo.expansionDepth);
    ui->dsb_waferNotchOrient->setValue(m_rcpChip.material.blueWaferInfo.waferNotchOrient);

    ui->gv_blueFilemLoadUnload->setChipSize(MCRecipeManager::instance()->chipRecipe().material.dwChipMaterialInfo.chipWidth,
                                            MCRecipeManager::instance()->chipRecipe().material.dwChipMaterialInfo.chipHeight);
}

// ---- 手动操作：控制层命令协议（MaterialProtocol）定好后在这里下发 ----

void BlueFilmLoadUnload::onMoveToLoadPos()
{
    // 下发"移动到上片位"
    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_BlueFilmLoad);
    msg.command = ProcessManagerCMD_Start;
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommand(msg);

    ui->btn_blueMoveToLoadPos->setEnabled(false);
}

void BlueFilmLoadUnload::onExpansionMapping()
{
    // TODO 下发"扩膜与映射"
}

void BlueFilmLoadUnload::onMoveToPickPos()
{
    // 下发"移动到取片位"
    MctrlModule::ProcessMessage msg;
    msg.process = static_cast<int>(ProcessID::SingleProc_BlueFilmUnload);
    msg.command = ProcessManagerCMD_Start;
    MCTRLClientManager::getInstance()->getClient()->GuiToMctrlCommand(msg);

    ui->btn_blueMoveToPickPos->setEnabled(false);
}

void BlueFilmLoadUnload::ReceiveMctrlCommand(const ProcessMessage &data)
{
    if(data.process == static_cast<int>(ProcessID::SingleProc_BlueFilmLoad))
    {
        ui->btn_blueMoveToLoadPos->setEnabled(true);
    }
    else if(data.process == static_cast<int>(ProcessID::SingleProc_BlueFilmUnload))
    {
        ui->btn_blueMoveToPickPos->setEnabled(true);
    }
}
