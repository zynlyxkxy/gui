#include "WaferLevelCollectProc.h"

#include "ui_ProcessPage.h"
#include "ProcessModule/Components/DiagramComponent/SimpleDiagram.h"
#include "istar_serializer.hpp"
#include "ProcessModule/Protocol/WaferLevelCollectProtocol.h"

#include <QDoubleSpinBox>
#include <utility>

WaferLevelCollectProc::WaferLevelCollectProc(int id, QWidget *tabPage, Ui::ProcessPage *ui,
                                             QWidget *dialogParent)
    : ProcessBase(id, tabPage, ui, dialogParent)
    , m_table(ui->tw_downWaferLevelingPoint)
{
    // 分组框标题：英文原稿声明（语言切换由 UiTranslator 处理）
    ui->gb_downWaferLevelingPoint->SetHeaderText(QStringLiteral("Down Wafer Level Point Collect (Wafer Stage XY)"));
    ui->gb_downWaferLevelingPointDiagram->SetHeaderText(QStringLiteral("Down Wafer Level Points 2D Diagram"));
    ui->gb_operateStepDesc_2->SetHeaderText(QStringLiteral("Operation Steps"));
    ui->gb_result_3->SetHeaderText(QStringLiteral("Result"));

    m_table.setReadOnly();
    for (QDoubleSpinBox *dsb : ui->gb_result_3->findChildren<QDoubleSpinBox *>())
    {
        dsb->setReadOnly(true);
    }

    setConnect();
}

void WaferLevelCollectProc::setConnect()
{
}

void WaferLevelCollectProc::clearData()
{
    m_table.clearColumns({ ColActual, ColConfocal });
    for (QDoubleSpinBox *dsb : ui->gb_result_3->findChildren<QDoubleSpinBox *>())
    {
        dsb->setValue(0);
    }
    m_row = 0;
}

void WaferLevelCollectProc::updateRecipe()
{
    // 脏检查：消费数据没变就不重建（表格选择/图视野保持）
    const std::vector<S_Point2D> pts = toPoints(m_rcpWafer.waferLevel.lst_waferLevelingPoints);
    ViewData now;
    now.valid = true;
    now.waferSize = m_rcpWafer.material.oneChipTypeInfo.waferSize;
    now.points = pts;
    if (m_view.valid && m_view.sameAs(now))
    {
        return;
    }
    m_view = now;

    // 底图：晶圆圆形，直径 = 处方晶圆尺寸（inch 转 mm）
    ui->gv_downWaferLeveling->drawCircle(m_rcpWafer.material.oneChipTypeInfo.waferSize * 25.4,
                                         DiagramColor::Blue);

    m_table.removeAllRows();
    ui->gv_downWaferLeveling->clearPoints();
    for (int i = 0; i < int(pts.size()); ++i)
    {
        m_table.setCell(i, ColNominal, pts[i].x_, pts[i].y_);
    }
    ui->gv_downWaferLeveling->drawPoints(pts);
}

void WaferLevelCollectProc::ReceiveMctrlCommand(const ProcessMessage &data)
{
    switch (data.command)
    {
        case WaferLevelCollectCMD::WaferLevelCollectCMD_MeasurePoint:
        {
            S_WaferLevelPointData res = Serializer::from_rawJson<S_WaferLevelPointData>(data.json);
            m_table.setCell(m_row, ColActual, res.waferX, res.waferY);
            m_table.setCell(m_row, ColConfocal, res.downCf);
            m_row = ++m_row % m_table.rows();
        }
        break;

        case WaferLevelCollectCMD::WaferLevelCollectCMD_Result:
        {
            S_WaferLevelResultData res = Serializer::from_rawJson<S_WaferLevelResultData>(data.json);
            ui->dsb_downWaferMeasuredValueRx->setValue(res.waferRx);
            ui->dsb_downWaferMeasuredValueRy->setValue(res.waferRy);
            ui->dsb_runElapsedTime_waferLevel->setValue(res.elapsedMs / 1000);
        }
        break;

        default:
            break;
    }
}
