#include "LevelingMarkItem.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>

namespace {
constexpr qreal kHalfDefault = 7.0; // 未指定芯片尺寸时退回的默认半边长
}

LevelingMarkItem::LevelingMarkItem(const QPointF &sceneCenter, double chipWidth,
                                   double chipHeight, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_half(calcHalfSize(chipWidth, chipHeight))
{
    setPos(sceneCenter);
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::PointingHandCursor));   // 悬停提示可点击
}

qreal LevelingMarkItem::calcHalfSize(double chipWidth, double chipHeight) const
{
    // 标记半边长随芯片尺寸缩放：常规芯片取 min(宽,高) 的 15%；
    // 小芯片（<=5mm）用更小比例（8%），避免标记相对芯片过大
    const double minDim = qMin(chipWidth, chipHeight);
    if (minDim <= 0.0)
        return kHalfDefault;
    const double ratio = (minDim <= 5.0) ? 0.08 : 0.15;
    return minDim * ratio;
}

QRectF LevelingMarkItem::boundingRect() const
{
    return QRectF(-m_half, -m_half, m_half * 2, m_half * 2);
}

void LevelingMarkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);

    // 小圆点标记：蓝色填充圆点，尺寸随芯片缩放（设上限避免大芯片时圆点过大）
    const qreal r = qMin(m_half * 0.42, 2.0);
    painter->setPen(QPen(Qt::blue, 1.0));
    painter->setBrush(QBrush(Qt::blue));
    painter->drawEllipse(QPointF(0, 0), r, r);
}

void LevelingMarkItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 通知视图在图上该点旁显示坐标文本（场景坐标 y 翻转回数学坐标由视图换算）
    emit sig_clicked(pos());
    QGraphicsObject::mousePressEvent(event);
}
