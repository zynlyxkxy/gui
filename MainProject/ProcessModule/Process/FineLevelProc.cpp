#include "FineLevelProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/FineLevelProtocol.h"

#include <QDoubleSpinBox>
#include <QGraphicsOpacityEffect>
#include <QtMath>
#include <utility>

FineLevelProc::FineLevelProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_upTable(ui->tw_upChipLevelingPoint)
    , m_dwTable(ui->tw_downChipLevelingPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_upChipLevelingPoint->SetHeaderText(QStringLiteral("Up Chip Level Point Collect (Align Stage XY)"));
    ui->gb_upChipLevelingPointDiagram->SetHeaderText(QStringLiteral("Up Chip Level Points 2D Diagram"));
    ui->gb_downChipLevelingPoint->SetHeaderText(QStringLiteral("Down Chip Level Point Collect (Wafer Stage XY) (Precision Mode)"));
    ui->gb_downChipLevelingPointDiagram->SetHeaderText(QStringLiteral("Down Chip Level Points 2D Diagram (Precision Mode)"));
    ui->gb_operateStepDesc_7->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_5->SetHeaderText(QStringLiteral("Result"));

    m_upTable.setReadOnly();
    m_dwTable.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_5->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void FineLevelProc::setConnect()
{
}

void FineLevelProc::clearData()
{
    m_upTable.clearColumns({ ColActual, ColConfocal });
    m_dwTable.clearColumns({ ColActual, ColConfocal });
    for (QDoubleSpinBox *dsb : ui->gb_result_5->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_rowUp = 0;
    m_rowDw = 0;
}

void FineLevelProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const auto &upMat = m_rcpChip.material.upChipMaterialInfo;
    const std::vector<S_Point2D> upPts = toPoints(m_rcpChip.level.lst_upLevelPoints);
    const std::vector<S_Point2D> dwPts = toPoints(m_rcpChip.level.lst_dwLevelPoints);
    ViewData now;
    now.valid = true;
    now.upChipW = upMat.chipWidth;
    now.dwChipW = m_rcpChip.material.dwChipMaterialInfo.chipWidth;
    now.yieldMode = m_rcpWafer.operationMode.operationMode != 0;   // 产率模式开关
    now.upPoints = upPts;
    now.dwPoints = dwPts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 上芯片：底图 + 点位表
    ui->gv_upChipLevelingPoint->drawSquare(upMat.chipWidth, upMat.chipWidth, DiagramColor::Green);
    m_upTable.removeAllRows();
    ui->gv_upChipLevelingPoint->clearPoints();
    for (int i = 0; i < int(upPts.size()); ++i)
    {
        m_upTable.setCell(i, ColNominal, upPts[i].x_, upPts[i].y_);
    }
    ui->gv_upChipLevelingPoint->drawPoints(upPts);

    // 产率模式：下半芯片组隐藏（透明度 0 占位，setHidden 会塌陷布局）
    if (m_rcpWafer.operationMode.operationMode)
    {
        auto *effPt = new QGraphicsOpacityEffect(ui->gb_downChipLevelingPoint);
        effPt->setOpacity(0.0);
        ui->gb_downChipLevelingPoint->setGraphicsEffect(effPt);

        auto *effDg = new QGraphicsOpacityEffect(ui->gb_downChipLevelingPointDiagram);
        effDg->setOpacity(0.0);
        ui->gb_downChipLevelingPointDiagram->setGraphicsEffect(effDg);
        return;
    }

    ui->gb_downChipLevelingPoint->setGraphicsEffect(nullptr);
    ui->gb_downChipLevelingPointDiagram->setGraphicsEffect(nullptr);

    // 下芯片：底图 + 点位表
    ui->gv_downChipLevelingPoint->drawSquare(m_rcpChip.material.dwChipMaterialInfo.chipWidth,
                                             m_rcpChip.material.dwChipMaterialInfo.chipWidth,
                                             DiagramColor::Green);
    m_dwTable.removeAllRows();
    ui->gv_downChipLevelingPoint->clearPoints();
    for (int i = 0; i < int(dwPts.size()); ++i)
    {
        m_dwTable.setCell(i, ColNominal, dwPts[i].x_, dwPts[i].y_);
    }
    ui->gv_downChipLevelingPoint->drawPoints(dwPts);
}

void FineLevelProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case FineLevelCMD::FineLevelCMD_UpMeasurePoint:
        {
            S_FineLevelPointData res = Serializer::from_rawJson<S_FineLevelPointData>(data.json);
            m_upTable.setCell(m_rowUp, ColActual, res.alignX, res.alignY);
            m_upTable.setCell(m_rowUp, ColConfocal, res.cfValue);
            m_rowUp = ++m_rowUp % m_upTable.rows();
        }
        break;

        case FineLevelCMD::FineLevelCMD_DownMeasurePoint:
        {
            S_FineLevelPointData res = Serializer::from_rawJson<S_FineLevelPointData>(data.json);
            m_dwTable.setCell(m_rowDw, ColActual, res.alignX, res.alignY);
            m_dwTable.setCell(m_rowDw, ColConfocal, res.cfValue);
            m_rowDw = ++m_rowDw % m_dwTable.rows();
        }
        break;

        case FineLevelCMD::FineLevelCMD_Result:
        {
            // Rx/Ry 为弧度按度显示；alignZComp / bondHeadZComp 为直线量 mm 不换算
            S_FineLevelResultData res = Serializer::from_rawJson<S_FineLevelResultData>(data.json);
            ui->dsb_rxRyLevelingThresholdRx_fine->setValue(qRadiansToDegrees(res.rxTolerance));
            ui->dsb_rxRyLevelingThresholdRy_fine->setValue(qRadiansToDegrees(res.ryTolerance));
            ui->dsb_upChipMeasuredValueRx_fine->setValue(qRadiansToDegrees(res.upChuckRx));
            ui->dsb_upChipMeasuredValueRy_fine->setValue(qRadiansToDegrees(res.upChuckRy));
            ui->dsb_downChipMeasuredValueRx_fine->setValue(qRadiansToDegrees(res.downChuckRx));
            ui->dsb_downChipMeasuredValueRy_fine->setValue(qRadiansToDegrees(res.downChuckRy));
            ui->dsb_AlignStageCompensationValue_fine->setValue(res.alignZComp);
            ui->dsb_bondHeadCompensationValueRx_fine->setValue(qRadiansToDegrees(res.bondHeadRxComp));
            ui->dsb_bondHeadCompensationValueRy_fine->setValue(qRadiansToDegrees(res.bondHeadRyComp));
            ui->dsb_bondHeadCompensationValueZ_fine->setValue(res.bondHeadZComp);
            ui->dsb_runElapsedTime_fineLeveling->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
