#include "CoarseLevelProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/CoarseLevelProtocol.h"

#include <QDoubleSpinBox>
#include <QtMath>
#include <utility>

CoarseLevelProc::CoarseLevelProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_coarseLevelingPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_coarseLevelingPoint->SetHeaderText(QStringLiteral("Coarse Level Point Collect (Align Stage XY)"));
    ui->gb_coarseLevelingPointDiagram->SetHeaderText(QStringLiteral("Coarse Level Points 2D Diagram"));
    ui->gb_operateStepDesc_4->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_4->SetHeaderText(QStringLiteral("Result"));

    m_table.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_4->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void CoarseLevelProc::setConnect()
{
}

void CoarseLevelProc::clearData()
{
    m_table.clearColumns({ ColActual, ColConfocalUp, ColConfocalDown });
    for (QDoubleSpinBox *dsb : ui->gb_result_4->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_row = 0;
}

void CoarseLevelProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const std::vector<S_Point2D> pts = toPoints(m_rcpChip.level.lst_coarseLevelPoints);
    ViewData now;
    now.valid = true;
    now.chipW = m_rcpChip.material.upChipMaterialInfo.chipWidth;
    now.chipH = m_rcpChip.material.upChipMaterialInfo.chipHeight;
    now.points = pts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 底图：上芯片方形
    ui->gv_coarseLevelingPointDiagram->drawSquare(m_rcpChip.material.upChipMaterialInfo.chipWidth,
                                                  m_rcpChip.material.upChipMaterialInfo.chipHeight,
                                                  DiagramColor::Green);

    m_table.removeAllRows();
    ui->gv_coarseLevelingPointDiagram->clearPoints();
    for (int i = 0; i < int(pts.size()); ++i)
    {
        m_table.setCell(i, ColNominal, pts[i].x_, pts[i].y_);
    }
    ui->gv_coarseLevelingPointDiagram->drawPoints(pts);
}

void CoarseLevelProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case CoarseLevelCMD::CoarseLevelCMD_MeasurePoint:
        {
            S_CoarseLevelPointData res = Serializer::from_rawJson<S_CoarseLevelPointData>(data.json);
            m_table.setCell(m_row, ColActual, res.alignX, res.alignY);
            m_table.setCell(m_row, ColConfocalUp, res.upCf);
            m_table.setCell(m_row, ColConfocalDown, res.downCf);
            m_row = ++m_row % m_table.rows();
        }
        break;

        case CoarseLevelCMD::CoarseLevelCMD_Result:
        {
            // Rx/Ry 为弧度按度显示；alignZComp / bondHeadZComp 为直线量 mm 不换算
            S_CoarseLevelResultData res = Serializer::from_rawJson<S_CoarseLevelResultData>(data.json);
            ui->dsb_rxRyLevelingThresholdRx_coarse->setValue(qRadiansToDegrees(res.rxTolerance));
            ui->dsb_rxRyLevelingThreshold_coarse_2->setValue(qRadiansToDegrees(res.ryTolerance));
            ui->dsb_upChuckMeasuredValueRx_coarse->setValue(qRadiansToDegrees(res.upChuckRx));
            ui->dsb_upChuckMeasuredValueRy_coarse->setValue(qRadiansToDegrees(res.upChuckRy));
            ui->dsb_downChuckMeasuredValueRx_coarse->setValue(qRadiansToDegrees(res.downChuckRx));
            ui->dsb_downChuckMeasuredValueRy_coarse->setValue(qRadiansToDegrees(res.downChuckRy));
            ui->dsb_AlignStageCompensationValue_coarse->setValue(res.alignZComp);
            ui->dsb_bondHeadCompensationValueRx_coarse->setValue(qRadiansToDegrees(res.bondHeadRxComp));
            ui->dsb_bondHeadCompensationValueRy_coarse->setValue(qRadiansToDegrees(res.bondHeadRyComp));
            ui->dsb_bondHeadCompensationValueZ_coarse->setValue(res.bondHeadZComp);
            ui->dsb_runElapsedTime_coarseLeveling->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
