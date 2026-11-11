#ifndef DOWNWAFERDIAGRAM_H
#define DOWNWAFERDIAGRAM_H

#include "ProcessModule/Components/DiagramComponent/Base/DiagramBase.h"
#include "istar_basic_value_point.h"   // S_Point2D（处方/流程层的点类型）

#include <QColor>
#include <QPointF>
#include <vector>
#include <QDateTime>

using ISTAR::BasicPlatform::BasicValue::S_Point2D;

/**
 * 下晶圆状态图：基于 DiagramBase 的具体图，展示与 BlueFilmChipDiagram 一致（同样的芯片方块、
 * 同样的点击标注、缩放平移全部继承基类），只有图例不同 —— 按键合状态着色：
 *   未选择 = 灰（初始）、键合中 = 黄、键合完毕 = 绿、选择但未键合 = 蓝。
 * 点击芯片显示真实坐标标注（贪心避让）：左键 = 清其他只显当前，点了已显示的则全清；
 * 右键 = 切换该芯片标注（已标注清除，未标注追加），不影响其他。
 * 颜色只有 stateColor() 一处定义，图例与上色同源，改色改这一处即可。
 */
class DownWaferDiagram : public DiagramBase
{
    Q_OBJECT

public:
    // ---- 芯片键合状态（与图例一一对应）----
    enum class ChipState
    {
        NotSelected,    // 未选择（灰）
        Bonding,        // 键合中（黄）
        Bonded,         // 键合完毕（绿）
        Selected,       // 选择但未键合（蓝）
    };

    struct ChipBondInfo
    {
        int count = 0;                    // 键合次数（外部算）
        std::vector<QDateTime> times;     // 每次键合时间（外部记录）
    };

    using BondInfoProvider = std::function<ChipBondInfo(const QPointF &chipPos)>;

    explicit DownWaferDiagram(QWidget *parent = nullptr);

    // ---- 状态 → 颜色（图例与芯片上色共用）----
    static QColor stateColor(ChipState state);

    // ---- 芯片（真实坐标中心点，收处方/流程层的 S_Point2D；重调 = 整图重置为「未选择」）----
    void setChips(const std::vector<S_Point2D> &chipCenters, double chipW, double chipH);

    // ---- 改单个芯片状态：false = 没命中芯片 ----
    // QPointF 版供点击回调等现成坐标用；S_Point2D 版供处方/流程层点位直接用
    bool setChipState(const QPointF &realPos, ChipState state);
    bool setChipState(const S_Point2D &realPos, ChipState state);

    // ---- 批量改状态（同一状态一次刷一批）：false = 有点没命中 ----
    // 命中的照常改色，只有全部命中才返回 true；空列表返回 true（没有点要改，不算失败）
    bool setChipState(const std::vector<S_Point2D> &realPos, ChipState state);

    void setBondInfoProvider(BondInfoProvider provider);   // 不注册 → 标注只画坐标

private:
    void onDieClicked(const QPointF &real, const QColor &color, Qt::MouseButton btn);
    void markChip(const QPointF &real); // 标注：坐标 + 次数 + 每次时间

    BondInfoProvider m_bondInfoProvider;

};

#endif // DOWNWAFERDIAGRAM_H
