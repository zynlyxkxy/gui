#include "BlueFilmChipDiagram.h"

#include <algorithm>

namespace
{
// 状态颜色唯一定义处（图例、setPoints 初值/改色都走 stateColor 取这里，避免两处漂移）
const QColor kTaken(0, 0, 0);           // 已取走：黑
const QColor kNotTaken(255, 255, 0);    // 未取走：黄
}  // namespace

BlueFilmChipDiagram::BlueFilmChipDiagram(QWidget *parent)
    : DiagramBase(parent)
{
    // 黄 = 未取走，黑 = 已取走（图例默认关，本图需要，显式打开；文字 = 英文原稿翻译 key）
    setLegendItems({ LegendItem{ stateColor(ChipState::NotTaken), SymbolShape::Square, "Blue Film Not Taken" },
                     LegendItem{ stateColor(ChipState::Taken), SymbolShape::Square, "Blue Film Taken" } });
    setLegendVisible(true);

    connect(this, &DiagramBase::dieClicked, this, &BlueFilmChipDiagram::onDieClicked);
}

QColor BlueFilmChipDiagram::stateColor(ChipState state)
{
    switch (state)
    {
    case ChipState::Taken:
        return kTaken;
    case ChipState::NotTaken:
    default:
        return kNotTaken;
    }
}

BlueFilmChipDiagram::ChipState BlueFilmChipDiagram::toState(int status)
{
    switch (status)
    {
    case static_cast<int>(ChipState::Taken):
        return ChipState::Taken;
    case static_cast<int>(ChipState::NotTaken):
        return ChipState::NotTaken;
    default:
        return ChipState::Unknown;   // 未知状态按默认色画（stateColor 兜底黄）
    }
}

void BlueFilmChipDiagram::setChipSize(double chipW, double chipH)
{
    m_chipW = chipW;
    m_chipH = chipH;
}

void BlueFilmChipDiagram::setPoints(const std::vector<ChipPosition> &chips)
{
    // 全量快照 diff：与上次一致 → 什么都不动（高频刷新的主要护栏）。
    // ChipPosition 无 operator==（C++17），逐字段比较
    const bool same = (chips.size() == m_lastChips.size()) &&
            std::equal(chips.begin(), chips.end(), m_lastChips.begin(),
                       [](const ChipPosition &a, const ChipPosition &b)
    {
        return a.posX == b.posX && a.posY == b.posY && a.status == b.status;
    });
    if (same)
    {
        return;
    }
    m_lastChips = chips;

    // 一次重建：先按默认色摆全部 die，再把已取走的批量改黑（基类合并为一次重绘）
    std::vector<QPointF> centers;
    centers.reserve(chips.size());
    for (const ChipPosition &c : chips)
    {
        centers.emplace_back(c.posX, c.posY);
    }
    setDies(centers, m_chipW, m_chipH, stateColor(ChipState::NotTaken));

    for (const ChipPosition &c : chips)
    {
        if (toState(c.status) == ChipState::Taken)
        {
            setDieColor(QPointF(c.posX, c.posY), stateColor(ChipState::Taken));
        }
    }
}

void BlueFilmChipDiagram::onDieClicked(const QPointF &real, const QColor &color,
                                       Qt::MouseButton btn)
{
    Q_UNUSED(color);
    if (btn == Qt::LeftButton)
    {
        const bool wasMarked = dieMarked(real);
        clearDieMarks();
        if (!wasMarked)
        {
            markDie(real);          // 点了已显示的 → 全清；否则清其他、显当前
        }
    }
    else if (btn == Qt::RightButton)
    {
        if (dieMarked(real))
        {
            unmarkDie(real);         // 已标注 → 清掉这条
        }
        else
        {
            markDie(real);           // 未标注 → 追加（不影响其他）
        }
    }
}
