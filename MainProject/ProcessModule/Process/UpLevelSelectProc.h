#ifndef UPLEVELSELECTPROC_H
#define UPLEVELSELECTPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

/**
 * 上调平标记点识别（Up Level Select）流程。
 * 表格与结果栏只读、可复制。
 */
class UpLevelSelectProc : public ProcessBase
{
public:
    UpLevelSelectProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColPixel = 2 };

    TableComponent m_table;     // 上调平标记点位表

    int m_row = 0;   // 结果当前填充行

    // 视图消费数据快照（字段 = updateRecipe 重建消费的全部数据；漏项 = 变了不刷新）
    struct ViewData
    {
        bool valid = false;
        double chipW = 0.0;
        std::vector<S_Point2D> points;

        bool sameAs(const ViewData &o) const
        {
            return chipW == o.chipW && samePoints(points, o.points);
        }
    };
    ViewData m_view;
};

#endif // UPLEVELSELECTPROC_H
