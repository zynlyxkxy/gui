#include "DownWaferDiagram.h"

#include "Common/CommonFunc.h"          // S_Point2DtoQPoints（基类 setDies 收 QPointF）

namespace
{
// 状态颜色唯一定义处（图例与芯片上色都走 stateColor 取这里，避免两处漂移）
const QColor kNotSelected(128, 128, 128);   // 未选择：灰
const QColor kBonding(255, 255, 0);         // 键合中：黄
const QColor kBonded(0, 200, 0);            // 键合完毕：绿
const QColor kSelected(0, 90, 255);         // 选择但未键合：蓝
}  // namespace

DownWaferDiagram::DownWaferDiagram(QWidget *parent)
    : DiagramBase(parent)
{
    // 图例（顺序按需求给定：未选择 / 键合中 / 键合完毕 / 选择但未键合；文字 = 英文原稿翻译 key）
    setLegendItems({ LegendItem{ stateColor(ChipState::NotSelected), SymbolShape::Square, "Down Wafer Not Selected" },
                     LegendItem{ stateColor(ChipState::Bonding), SymbolShape::Square, "Down Wafer Bonding" },
                     LegendItem{ stateColor(ChipState::Bonded), SymbolShape::Square, "Down Wafer Bonded" },
                     LegendItem{ stateColor(ChipState::Selected), SymbolShape::Square, "Down Wafer Selected Not Bonded" } });
    setLegendVisible(true);

    connect(this, &DiagramBase::dieClicked, this, &DownWaferDiagram::onDieClicked);
}

QColor DownWaferDiagram::stateColor(ChipState state)
{
    switch (state)
    {
    case ChipState::Bonding:
        return kBonding;
    case ChipState::Bonded:
        return kBonded;
    case ChipState::Selected:
        return kSelected;
    case ChipState::NotSelected:
    default:
        return kNotSelected;
    }
}

void DownWaferDiagram::setChips(const std::vector<S_Point2D> &chipCenters, double chipW, double chipH)
{
    setDies(S_Point2DtoQPoints(chipCenters), chipW, chipH, stateColor(ChipState::NotSelected));
}

bool DownWaferDiagram::setChipState(const QPointF &realPos, ChipState state)
{
    return setDieColor(realPos, stateColor(state));
}

bool DownWaferDiagram::setChipState(const S_Point2D &realPos, ChipState state)
{
    return setChipState(QPointF(realPos.x_, realPos.y_), state);
}

bool DownWaferDiagram::setChipState(const std::vector<S_Point2D> &realPos, ChipState state)
{
    // 逐点走单点接口：命中判定、改色、重绘合并都由基类负责
    // （基类的批量改色会合并成一次重绘，这里不必另做批处理优化）
    bool allHit = true;
    for (const S_Point2D &p : realPos)
    {
        if (!setChipState(p, state))
        {
            allHit = false;      // 有点对不上就记下来，但已命中的照样改
        }
    }
    return allHit;
}

void DownWaferDiagram::onDieClicked(const QPointF &real, const QColor &color, Qt::MouseButton btn)
{
    Q_UNUSED(color);
    if (btn == Qt::LeftButton)
    {
        const bool wasMarked = dieMarked(real);
        clearDieMarks();
        if (!wasMarked)
        {
            markChip(real);          // 点了已显示的 → 全清；否则清其他、显当前
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
            markChip(real);           // 未标注 → 追加（不影响其他）
        }
    }
}

void DownWaferDiagram::setBondInfoProvider(BondInfoProvider provider)
{
    m_bondInfoProvider = std::move(provider);
}


void DownWaferDiagram::markChip(const QPointF &real)
{
    // 坐标
    QString text = QString("(%1, %2)").arg(real.x(), 0, 'f', 3)
                                      .arg(real.y(), 0, 'f', 3);

    if (m_bondInfoProvider)          // 次数与时间都在外部，这里只是取来排版
    {
        const ChipBondInfo info = m_bondInfoProvider(real);
        text += QString("\n键合次数：%1").arg(info.count);
        for (size_t i = 0; i < info.times.size(); ++i)
        {
            text += QString("\n第%1次：%2").arg(i + 1)
                        .arg(info.times[i].toString("yyyy-MM-dd HH:mm:ss"));
        }
    }

    markDieText(real, text.toStdString());

}
