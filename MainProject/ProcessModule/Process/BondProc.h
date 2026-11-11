#ifndef BONDPROC_H
#define BONDPROC_H

#include "ProcessModule/Base/ProcessBase.h"
#include "ProcessModule/Components/TableComponent/TableComponent.h"
#include "ProcessModule/Protocol/BondProtocol.h"   // BondCMD（控制层"即将执行第几条工艺"的汇报）
#include "chart/customchart.h"   // ISTAR::GUI::CustomChart / AxisParameter（键合曲线）

#include <QString>

/**
 * 键合（Bond）流程：
 * - 键合工序表只读展示处方数据（表头行 + 数据行成对），有啥填啥：
 *   命令/参数名/值均为处方原文，不做翻译或 Yes/No 加工；
 * - 工序高亮：控制层每做一条工序之前报一次序号（BondCMD_StepBegin，0 基 =
 *   处方 lst_BondingSequence 的下标），据此把该工序的两行（命令/参数名一行 +
 *   值一行）高亮，做到哪条亮哪条；一条工序一段，高亮随下一条前移；
 * - 键合曲线：数据来自已周期推送的 IO 观察者（PLC AI 500/501/502），由
 *   BondCurveRecorder 在"本流程在跑"这段窗口内采集。clearData 是新一轮的开始
 *   （ProcessPage 在 Start 之前调用），所以顺带开一段新的采集；
 * - 表格为纯展示，编辑只能编辑处方本身。
 */

class BondCurveRecorder;

class BondProc : public ProcessBase
{
public:
    BondProc(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent);

    void clearData() override;
    void updateRecipe() override;
    void ReceiveMctrlCommand(const ProcessMessage &data) override;

protected:
    void setConnect() override;

private:
    // ---- 键合工序表 ----
    void bonding_loadUi(const S_RecipeChipBond &bond);
    void highlightStep(int stepNo);   // 工序序号（0 起）→ 高亮该工序的两行
    TableComponent m_table;   // 工序表包装（有啥填啥，setCellText 越界自动扩行列）

    // ---- 键合曲线 ----
    void ensureChart();      // 建图 + 固定轴（空数据也有坐标轴）
    void configureAxes();    // 只转发到 BondCurve::applyAxisRange()（轴定义在那里）

    ISTAR::GUI::CustomChart *p_Chart = nullptr;

    BondCurveRecorder *m_curve = nullptr;   // 曲线采集器（父对象 = m_tabPage，随 tab 销毁）
};

#endif // BONDPROC_H
