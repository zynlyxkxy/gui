#ifndef WAFERLEVELCOLLECTPROC_H
#define WAFERLEVELCOLLECTPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

/**
 * 晶圆调平数据采集（Wafer Level Collect）流程。
 * 表格与结果栏只读、可复制。
 */
class WaferLevelCollectProc : public ProcessBase
{
public:
    WaferLevelCollectProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColConfocal = 2 };   // 第 1 列处方，第 2/3 列结果

    TableComponent m_table;     // 下晶圆调平点位表

    int m_row = 0;   // 结果当前填充行

    // 视图消费数据快照（字段 = updateRecipe 重建消费的全部数据；漏项 = 变了不刷新）
    struct ViewData
    {
        bool valid = false;
        double waferSize = 0.0;
        std::vector<S_Point2D> points;

        bool sameAs(const ViewData &o) const
        {
            return waferSize == o.waferSize && samePoints(points, o.points);
        }
    };
    ViewData m_view;
};

#endif // WAFERLEVELCOLLECTPROC_H
