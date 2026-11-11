#ifndef CHIPPOSCALIBPROC_H
#define CHIPPOSCALIBPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"
#include "ProcessModule/Protocol/ChipPosCalibProtocol.h"

#include <vector>

/**
 * 芯片位置标定（Chip Pos Calib）流程：交互式流程。
 * 用户自行选行：拍照 / 移动 / 清空均要求先选中行；确认提交全表名义 + 实测坐标
 * （至少 2 点；存在未测点时二次确认）。
 */
class ChipPosCalibProc : public ProcessBase
{
public:
    ChipPosCalibProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void setStatus(MctrlStatus status) override;
    void clearData() override;
    void updateRecipe() override;
    void updateMc() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    enum Col { ColNominal = 0, ColActual = 1, ColPixel = 2 };

    void sendCommand(int command, const std::string &json = std::string());
    void moveStage(double x, double y, int moveMode);
    void submitAccept();        // 确认：校验 + 全表提交
    void clearSelectedRow();    // 清选中行（本地）
    void setActionsEnabled(bool enabled); // 动作区总开关（上/下使能）：交互期全亮，其余全灰
    void refreshCalibDiagram(); // 图重建（处方 + MC 圆心，快照脏检查；updateRecipe/updateMc 共调）

    TableComponent m_table;

    // 表格消费数据快照（只吃处方点表）
    struct TableData
    {
        bool valid = false;
        std::vector<S_Point2D> points;

        bool sameAs(const TableData &o) const
        {
            return samePoints(points, o.points);
        }
    };
    TableData m_tableData;

    // 图消费数据快照（处方布局/点表/尺寸 + MC 圆心）
    struct DiagramData
    {
        bool valid = false;
        double waferSize = 0.0;
        double chipW = 0.0;
        double chipH = 0.0;
        double centerX = 0.0;    // MC 圆心
        double centerY = 0.0;
        std::vector<S_Point2D> chips;
        std::vector<S_Point2D> marks;      // 图上全部标记点（处方全量）
        std::vector<S_Point2D> redMarks;   // 需标红的子集（待标定标记点）

        bool sameAs(const DiagramData &o) const
        {
            return waferSize == o.waferSize && chipW == o.chipW && chipH == o.chipH &&
                   centerX == o.centerX && centerY == o.centerY &&
                   samePoints(chips, o.chips) && samePoints(marks, o.marks) &&
                   samePoints(redMarks, o.redMarks);
        }
    };
    DiagramData m_diagramData;

    bool m_interact = false;   // EnterInteract 后允许操作（数据以表格为唯一来源，读格取原始值）
};

#endif // CHIPPOSCALIBPROC_H
