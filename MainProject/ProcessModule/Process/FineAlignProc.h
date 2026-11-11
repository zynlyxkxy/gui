#ifndef FINEALIGNPROC_H
#define FINEALIGNPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

/**
 * 精对准（Fine Align）流程。
 * 表格与结果栏只读、可复制。
 */
class FineAlignProc : public ProcessBase
{
public:
    FineAlignProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColPixel = 2 };

    TableComponent m_upTable;     // 上芯片对准标记表
    TableComponent m_dwTable;     // 下芯片对准标记表

    int m_rowUp = 0;
    int m_rowDw = 0;

    // 视图消费数据快照（字段 = updateRecipe 重建消费的全部数据；漏项 = 变了不刷新）
    struct ViewData
    {
        bool valid = false;
        double upChipW = 0.0;
        double dwChipW = 0.0;
        std::vector<S_Point2D> upPoints;
        std::vector<S_Point2D> dwPoints;

        bool sameAs(const ViewData &o) const
        {
            return upChipW == o.upChipW && dwChipW == o.dwChipW &&
                   samePoints(upPoints, o.upPoints) && samePoints(dwPoints, o.dwPoints);
        }
    };
    ViewData m_view;
};

#endif // FINEALIGNPROC_H
