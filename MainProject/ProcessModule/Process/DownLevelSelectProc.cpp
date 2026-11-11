#include "DownLevelSelectProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/DownLevelSelectProtocol.h"
#include "dialog/messagedialog.h"
#include "istar_language.h"

#include <QDoubleSpinBox>
#include <utility>

using ISTAR::GUI::MessageDialog;

DownLevelSelectProc::DownLevelSelectProc(int id, QWidget *tabPage, Ui::ProcessPage *ui,
                                         QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_downLevelingMarkPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_downLevelingMarkPoint->SetHeaderText(QStringLiteral("Down Level Mark Point Recognition (Wafer Stage XY)"));
    ui->gb_downLevelingMarkPointDiagram->SetHeaderText(QStringLiteral("Down Level Mark Points 2D Diagram"));
    ui->gb_operateStepDesc_5->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_7->SetHeaderText(QStringLiteral("Result"));

    m_table.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_7->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void DownLevelSelectProc::setConnect()
{
}

void DownLevelSelectProc::clearData()
{
    m_table.clearColumns({ ColActual, ColPixel });
    for (QDoubleSpinBox *dsb : ui->gb_result_7->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_row = 0;
}

void DownLevelSelectProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const std::vector<S_Point2D> pts = toPoints(m_rcpChip.level.lst_dwLevelPoints);
    ViewData now;
    now.valid = true;
    now.chipW = m_rcpChip.material.dwChipMaterialInfo.chipWidth;
    now.points = pts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 底图：下芯片方形
    ui->gv_downLevelingMarkPoint->drawSquare(m_rcpChip.material.dwChipMaterialInfo.chipWidth,
                                             m_rcpChip.material.dwChipMaterialInfo.chipWidth,
                                             DiagramColor::Green);

    m_table.removeAllRows();
    ui->gv_downLevelingMarkPoint->clearPoints();
    for (int i = 0; i < int(pts.size()); ++i)
    {
        m_table.setCell(i, ColNominal, pts[i].x_, pts[i].y_);
    }
    ui->gv_downLevelingMarkPoint->drawPoints(pts);
}

void DownLevelSelectProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case DownLevelSelectCMD::DownLevelSelectCMD_MarkLocated:
        {
            S_DownLevelSelectMarkData res = Serializer::from_rawJson<S_DownLevelSelectMarkData>(data.json);
            if (res.ok)
            {
                m_table.setCell(m_row, ColActual, res.actualX, res.actualY);
                m_table.setCell(m_row, ColPixel, res.pixelX, res.pixelY);
                m_row = ++m_row % m_table.rows();
            }
            else
            {
                MessageDialog dlg(MessageDialog::Error, m_dialogParent,
                                  QString::fromStdString(Tr("Down level mark point not recognized")));
                dlg.exec();
            }
        }
        break;

        case DownLevelSelectCMD::DownLevelSelectCMD_Result:
        {
            S_DownLevelSelectResultData res =
                    Serializer::from_rawJson<S_DownLevelSelectResultData>(data.json);
            ui->dsb_runElapsedTime_downlevel->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
