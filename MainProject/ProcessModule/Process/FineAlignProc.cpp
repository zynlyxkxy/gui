#include "FineAlignProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/FineAlignProtocol.h"
#include "dialog/messagedialog.h"
#include "istar_language.h"

#include <QDoubleSpinBox>
#include <QtMath>
#include <utility>

using ISTAR::GUI::MessageDialog;

FineAlignProc::FineAlignProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_upTable(ui->tw_upChipAlignMarkPoint)
    , m_dwTable(ui->tw_downChipAlignMarkPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_upChipAlignMarkPoint->SetHeaderText(QStringLiteral("Up Chip Align Mark Point Recognition (Align Stage XY)"));
    ui->gb_upChipAlignMarkPointDiagram->SetHeaderText(QStringLiteral("Up Chip Align Mark Points 2D Diagram"));
    ui->gb_downChipAlignMarkPoint->SetHeaderText(QStringLiteral("Down Chip Align Mark Point Recognition (Align Stage XY)"));
    ui->gb_downChipAlignMarkPointDiagram->SetHeaderText(QStringLiteral("Down Chip Align Mark Points 2D Diagram"));
    ui->gb_operateStepDesc_8->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_6->SetHeaderText(QStringLiteral("Result"));

    m_upTable.setReadOnly();
    m_dwTable.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_6->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void FineAlignProc::setConnect()
{
}

void FineAlignProc::clearData()
{
    m_upTable.clearColumns({ ColActual, ColPixel });
    m_dwTable.clearColumns({ ColActual, ColPixel });
    for (QDoubleSpinBox *dsb : ui->gb_result_6->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_rowUp = 0;
    m_rowDw = 0;
}

void FineAlignProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const auto &upMat = m_rcpChip.material.upChipMaterialInfo;
    const auto &dwMat = m_rcpChip.material.dwChipMaterialInfo;
    const std::vector<S_Point2D> upPts = toPoints(m_rcpChip.align.lst_upAlignPoints);
    const std::vector<S_Point2D> dwPts = toPoints(m_rcpChip.align.lst_downAlignPoints);
    ViewData now;
    now.valid = true;
    now.upChipW = upMat.chipWidth;
    now.dwChipW = dwMat.chipWidth;
    now.upPoints = upPts;
    now.dwPoints = dwPts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 上芯片
    ui->gv_upChipAlignMarkPoint->drawSquare(upMat.chipWidth, upMat.chipWidth, DiagramColor::Green);
    m_upTable.removeAllRows();
    ui->gv_upChipAlignMarkPoint->clearPoints();
    for (int i = 0; i < int(upPts.size()); ++i)
    {
        m_upTable.setCell(i, ColNominal, upPts[i].x_, upPts[i].y_);
    }
    ui->gv_upChipAlignMarkPoint->drawPoints(upPts);

    // 下芯片
    ui->gv_downChipAlignMarkPoint->drawSquare(dwMat.chipWidth, dwMat.chipWidth, DiagramColor::Green);
    m_dwTable.removeAllRows();
    ui->gv_downChipAlignMarkPoint->clearPoints();
    for (int i = 0; i < int(dwPts.size()); ++i)
    {
        m_dwTable.setCell(i, ColNominal, dwPts[i].x_, dwPts[i].y_);
    }
    ui->gv_downChipAlignMarkPoint->drawPoints(dwPts);
}

void FineAlignProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case FineAlignCMD::FineAlignCMD_UpMarkLocated:
        {
            S_FineAlignMarkData res = Serializer::from_rawJson<S_FineAlignMarkData>(data.json);
            if (res.ok)
            {
                m_upTable.setCell(m_rowUp, ColActual, res.actualX, res.actualY);
                m_upTable.setCell(m_rowUp, ColPixel, res.pixelX, res.pixelY);
                m_rowUp = ++m_rowUp % m_upTable.rows();
            }
            else
            {
                MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                                  QString::fromStdString(Tr("Up chip align mark not recognized")));
                dlg.exec();
            }
        }
        break;

        case FineAlignCMD::FineAlignCMD_DownMarkLocated:
        {
            S_FineAlignMarkData res = Serializer::from_rawJson<S_FineAlignMarkData>(data.json);
            if (res.ok)
            {
                m_dwTable.setCell(m_rowDw, ColActual, res.actualX, res.actualY);
                m_dwTable.setCell(m_rowDw, ColPixel, res.pixelX, res.pixelY);
                m_rowDw = ++m_rowDw % m_dwTable.rows();
            }
            else
            {
                MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                                  QString::fromStdString(Tr("Down chip align mark not recognized")));
                dlg.exec();
            }
        }
        break;

        case FineAlignCMD::FineAlignCMD_Result:
        {
            // X/Y 为 mm、Rz 为弧度（按度显示）
            S_FineAlignResultData res = Serializer::from_rawJson<S_FineAlignResultData>(data.json);
            ui->dsb_xyAlignThresholdX->setValue(res.xTolerance);
            ui->dsb_xyAlignThresholdY->setValue(res.yTolerance);
            ui->dsb_upChipPositionX->setValue(res.upChipX);
            ui->dsb_upChipPositionY->setValue(res.upChipY);
            ui->dsb_upChipPositionRz->setValue(qRadiansToDegrees(res.upChipRz));
            ui->dsb_downChipPositionX->setValue(res.downChipX);
            ui->dsb_downChipPositionY->setValue(res.downChipY);
            ui->dsb_downChipPositionRz->setValue(qRadiansToDegrees(res.downChipRz));
            ui->dsb_CompensationValueX->setValue(res.compX);
            ui->dsb_CompensationValueY->setValue(res.compY);
            ui->dsb_CompensationValueRz->setValue(qRadiansToDegrees(res.compRz));
            ui->dsb_runElapsedTime_fineAlign->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
