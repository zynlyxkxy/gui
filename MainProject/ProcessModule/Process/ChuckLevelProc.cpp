#include "ChuckLevelProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/ChuckLevelProtocol.h"

#include <QDoubleSpinBox>
#include <QtMath>
#include <utility>

ChuckLevelProc::ChuckLevelProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_upTable(ui->tw_upChuckLevelingPoint)
    , m_downTable(ui->tw_downChuckLevelingPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_upChuckLevelingPoint->SetHeaderText(QStringLiteral("Up Chuck Level Point Collect (Align Stage XY)"));
    ui->gb_upChuckLevelingPointDiagram->SetHeaderText(QStringLiteral("Up Chuck Level Points 2D Diagram"));
    ui->gb_downChuckLevelingPoint->SetHeaderText(QStringLiteral("Down Chuck Level Point Collect (Wafer Stage XY)"));
    ui->gb_downChuckLevelingPointDiagram->SetHeaderText(QStringLiteral("Down Chuck Level Points 2D Diagram"));
    ui->gb_operateStepDesc->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result->SetHeaderText(QStringLiteral("Result"));

    m_upTable.setReadOnly();
    m_downTable.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }
    setConnect();
}

void ChuckLevelProc::setConnect()
{
}

void ChuckLevelProc::clearData()
{
    m_upTable.clearColumns({ ColActual, ColConfocal });
    m_downTable.clearColumns({ ColActual, ColConfocal });
    for (QDoubleSpinBox *dsb : ui->gb_result->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_rowUp = 0;
    m_rowDown = 0;
}

void ChuckLevelProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const auto &chuck = m_rcpWafer.chuckLevel;
    const std::vector<S_Point2D> upPts = toPoints(chuck.lst_upChuckLevelingPoints);
    const std::vector<S_Point2D> downPts = toPoints(chuck.lst_downChuckLevelingPoints);
    ViewData now;
    now.valid = true;
    now.upPoints = upPts;
    now.downPoints = downPts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    m_upTable.removeAllRows();
    ui->gv_upChuckLeveling->clearPoints();
    for (int i = 0; i < int(upPts.size()); ++i)
    {
        m_upTable.setCell(i, ColNominal, upPts[i].x_, upPts[i].y_);
    }
    ui->gv_upChuckLeveling->drawPoints(upPts);

    m_downTable.removeAllRows();
    ui->gv_downChuckLeveling->clearPoints();
    for (int i = 0; i < int(downPts.size()); ++i)
    {
        m_downTable.setCell(i, ColNominal, downPts[i].x_, downPts[i].y_);
    }
    ui->gv_downChuckLeveling->drawPoints(downPts);
}

void ChuckLevelProc::updateMc()
{
    // 底图：上 = 上芯片方形，下 = 晶圆圆形（直径 inch 转 mm）
    ui->gv_upChuckLeveling->drawSquare(m_mc.bondArm.chuckWidth,
                                       m_mc.bondArm.chuckLength,
                                       DiagramColor::Green);
    ui->gv_downChuckLeveling->drawCircle(m_mc.siliconStage.chuckDiameter * 25.4,
                                         DiagramColor::Blue);

}

void ChuckLevelProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case ChuckLevelCMD::ChuckLevelCMD_UpMeasurePoint:
        {
            S_ChuckLevelPointData res = Serializer::from_rawJson<S_ChuckLevelPointData>(data.json);
            m_upTable.setCell(m_rowUp, ColActual, res.posX, res.posY);
            m_upTable.setCell(m_rowUp, ColConfocal, res.cfValue);
            m_rowUp = ++m_rowUp % m_upTable.rows();
        }
        break;

        case ChuckLevelCMD::ChuckLevelCMD_DownMeasurePoint:
        {
            S_ChuckLevelPointData res = Serializer::from_rawJson<S_ChuckLevelPointData>(data.json);
            m_downTable.setCell(m_rowDown, ColActual, res.posX, res.posY);
            m_downTable.setCell(m_rowDown, ColConfocal, res.cfValue);
            m_rowDown = ++m_rowDown % m_downTable.rows();
        }
        break;

        case ChuckLevelCMD::ChuckLevelCMD_Result:
        {
            // Mctrl 结果 Rx/Ry 为弧度，界面按度显示
            S_ChuckLevelResultData res = Serializer::from_rawJson<S_ChuckLevelResultData>(data.json);
            ui->dsb_rxRyLevelingThresholdRx->setValue(qRadiansToDegrees(res.rxTolerance));
            ui->dsb_rxRyLevelingThresholdRy->setValue(qRadiansToDegrees(res.ryTolerance));
            ui->dsb_upChuckMeasuredValueRx->setValue(qRadiansToDegrees(res.upChuckRx));
            ui->dsb_upChuckMeasuredValueRy->setValue(qRadiansToDegrees(res.upChuckRy));
            ui->dsb_downChuckMeasuredValueRx->setValue(qRadiansToDegrees(res.downChuckRx));
            ui->dsb_downChuckMeasuredValueRy->setValue(qRadiansToDegrees(res.downChuckRy));
            ui->dsb_bondHeadCompensationValueRx->setValue(qRadiansToDegrees(res.bondHeadRxComp));
            ui->dsb_bondHeadCompensationValueRy->setValue(qRadiansToDegrees(res.bondHeadRyComp));
            ui->dsb_runElapsedTime_chuckLeveling->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
