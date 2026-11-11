#ifndef BLUEFILMCHIPDIAGRAM_H
#define BLUEFILMCHIPDIAGRAM_H

#include "ProcessModule/Components/DiagramComponent/Base/DiagramBase.h"
#include "ProcessModule/Protocol/BlueFilmChipHandoverProtocol.h"   // ChipPosition（外部全量快照直送）

#include <QColor>
#include <QPointF>
#include <vector>

/**
 * 蓝膜芯片状态图：基于 DiagramBase 的具体图。
 * 用法：先 setChipSize 存芯片尺寸，之后每次全量快照 setPoints（vector<ChipPosition> 直接送入）。
 * 内部 diff：与上次快照一致直接返回（高频全量刷新的护栏），有变化才一次重建重绘
 * （批量改色由基类合并为一次重绘，无逐点刷新开销）。
 * 芯片方块：黄 = 未取走，黑 = 已取走（图例自动给出含义）。
 * 点击芯片显示真实坐标标注（贪心避让）：左键 = 清其他只显当前，点了已显示的则全清；
 * 右键 = 切换该芯片标注（已标注清除，未标注追加），不影响其他。
 */
class BlueFilmChipDiagram : public DiagramBase
{
    Q_OBJECT

public:
    // ---- 芯片状态（与图例一一对应；取值对应 ChipPosition::status）----
    enum class ChipState
    {
        Unknown = 0,
        Taken = 1,      // 已取走（黑）
        NotTaken = 2,   // 未取走（黄）
    };

    explicit BlueFilmChipDiagram(QWidget *parent = nullptr);

    // ---- 先存芯片尺寸（真实 mm，die 方块尺寸；setPoints 时用）----
    void setChipSize(double chipW, double chipH);

    // ---- 全量快照：坐标 + 状态一起送，status → ChipState ----
    // 与上次快照一致直接返回；有变化才整表重建。不画晶圆边（点位本身可信，圆心不做估计）
    void setPoints(const std::vector<ChipPosition> &chips);

private:
    void onDieClicked(const QPointF &real, const QColor &color, Qt::MouseButton btn);
    static ChipState toState(int status);

    // ---- 状态 → 颜色（图例与芯片上色共用）----
    QColor stateColor(ChipState state);

    double m_chipW = 10.0, m_chipH = 10.0;       // 芯片尺寸（setChipSize 存，setPoints 用）
    std::vector<ChipPosition> m_lastChips;       // 上一次快照（diff 用）
};

#endif // BLUEFILMCHIPDIAGRAM_H
