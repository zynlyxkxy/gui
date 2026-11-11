#ifndef FINELEVELPROC_H
#define FINELEVELPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

/**
 * 精密调平（Fine Level）流程。
 * 产率模式下隐藏下半芯片组（透明占位）；表格与结果栏只读、可复制。
 */
class FineLevelProc : public ProcessBase
{
public:
    FineLevelProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColConfocal = 2 };

    TableComponent m_upTable;     // 上芯片调平点位表
    TableComponent m_dwTable;     // 下芯片调平点位表（产率模式隐藏）

    int m_rowUp = 0;
    int m_rowDw = 0;

    // 视图消费数据快照（字段 = updateRecipe 重建消费的全部数据；漏项 = 变了不刷新）
    struct ViewData
    {
        bool valid = false;
        double upChipW = 0.0;
        double dwChipW = 0.0;
        bool yieldMode = false;   // 产率模式（控制下半芯片组显隐）
        std::vector<S_Point2D> upPoints;
        std::vector<S_Point2D> dwPoints;

        bool sameAs(const ViewData &o) const
        {
            return upChipW == o.upChipW && dwChipW == o.dwChipW && yieldMode == o.yieldMode &&
                   samePoints(upPoints, o.upPoints) && samePoints(dwPoints, o.dwPoints);
        }
    };
    ViewData m_view;
};

#endif // FINELEVELPROC_H
