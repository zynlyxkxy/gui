#ifndef CHUCKLEVELPROC_H
#define CHUCKLEVELPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"

/**
 * 吸盘调平（Chuck Level）流程。
 * 表格与结果栏只读、可复制。
 */
class ChuckLevelProc : public ProcessBase
{
public:
    ChuckLevelProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;
    void updateMc() override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColConfocal = 2 };   // 第 1 列处方，第 2/3 列结果

    TableComponent m_upTable;
    TableComponent m_downTable;

    int m_rowDown = 0;   // 下吸盘点当前行
    int m_rowUp = 0;     // 上吸盘点当前行

    // 视图消费数据快照（字段 = updateRecipe 重建消费的全部数据；漏项 = 变了不刷新）
    struct ViewData
    {
        bool valid = false;
        std::vector<S_Point2D> upPoints;
        std::vector<S_Point2D> downPoints;

        bool sameAs(const ViewData &o) const
        {
            return samePoints(upPoints, o.upPoints) && samePoints(downPoints, o.downPoints);
        }
    };
    ViewData m_view;
};

#endif // CHUCKLEVELPROC_H
