#include "UpLevelSelectProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/UpLevelSelectProtocol.h"
#include "dialog/messagedialog.h"
#include "istar_language.h"

#include <QDoubleSpinBox>
#include <utility>

using ISTAR::GUI::MessageDialog;

UpLevelSelectProc::UpLevelSelectProc(int id, QWidget *tabPage, Ui::ProcessPage *ui,
                                     QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_upLevelingMarkPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_upLevelingMarkPoint->SetHeaderText(QStringLiteral("Up Level Mark Point Recognition (Align Stage XY)"));
    ui->gb_upLevelingMarkPointDiagram->SetHeaderText(QStringLiteral("Up Level Mark Points 2D Diagram"));
    ui->gb_operateStepDesc_6->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_8->SetHeaderText(QStringLiteral("Result"));

    m_table.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_8->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void UpLevelSelectProc::setConnect()
{
}

void UpLevelSelectProc::clearData()
{
    m_table.clearColumns({ ColActual, ColPixel });
    for (QDoubleSpinBox *dsb : ui->gb_result_8->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_row = 0;
}

void UpLevelSelectProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const std::vector<S_Point2D> pts = toPoints(m_rcpChip.level.lst_upLevelPoints);
    ViewData now;
    now.valid = true;
    now.chipW = m_rcpChip.material.upChipMaterialInfo.chipWidth;
    now.points = pts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 底图：上芯片方形
    ui->gv_upLevelingMarkPoint->drawSquare(m_rcpChip.material.upChipMaterialInfo.chipWidth,
                                           m_rcpChip.material.upChipMaterialInfo.chipWidth,
                                           DiagramColor::Green);

    m_table.removeAllRows();
    ui->gv_upLevelingMarkPoint->clearPoints();
    for (int i = 0; i < int(pts.size()); ++i)
    {
        m_table.setCell(i, ColNominal, pts[i].x_, pts[i].y_);
    }
    ui->gv_upLevelingMarkPoint->drawPoints(pts);
}

void UpLevelSelectProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case UpLevelSelectCMD::UpLevelSelectCMD_MarkLocated:
        {
            S_UpLevelSelectMarkData res = Serializer::from_rawJson<S_UpLevelSelectMarkData>(data.json);
            if (res.ok)
            {
                m_table.setCell(m_row, ColActual, res.actualX, res.actualY);
                m_table.setCell(m_row, ColPixel, res.pixelX, res.pixelY);
                m_row = ++m_row % m_table.rows();
            }
            else
            {
                MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                                  QString::fromStdString(Tr("Up level mark point not recognized")));
                dlg.exec();
            }
        }
        break;

        case UpLevelSelectCMD::UpLevelSelectCMD_Result:
        {
            S_UpLevelSelectResultData res =
                    Serializer::from_rawJson<S_UpLevelSelectResultData>(data.json);
            ui->dsb_runElapsedTime_downlevel_2->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
