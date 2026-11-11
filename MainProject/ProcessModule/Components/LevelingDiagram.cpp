#include "LevelingDiagram.h"

#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QResizeEvent>

namespace {
// 晶圆尺寸单位换算：inch → mm
const qreal kInchToMm = 25.4;
// 点位半径 = 形状特征尺寸 / 40：点与形状的视觉比例恒定，不设下限
// （视图按形状自适应放大，过小的点仍会随缩放保持可见）
const qreal kPointRadiusRatio = 40.0;
// 坐标文字固定屏幕像素大小：忽略视图变换，任何缩放/形状下文字大小一致
const int kLabelFontPx = 15;
// 坐标文字相对点的偏移（屏幕像素，不随视图缩放变化）
const qreal kLabelOffsetX = 7.0;
const qreal kLabelOffsetY = 6.0;
// 坐标轴屏幕像素线宽：小图形被放大时轴宽按缩放反比变细，视觉恒定
const qreal kAxisPenPx = 1.0;
// 图形边线宽（场景单位）：细线避免小图形放大后边过粗
// const qreal kShapePenWidth = 1.0;
}

LevelingDiagram::LevelingDiagram(QWidget *parent)
    : QGraphicsView(parent)
    , m_pScene(new QGraphicsScene(this))
{
    setScene(m_pScene);
    setRenderHint(QPainter::Antialiasing);
    rebuild();
}

void LevelingDiagram::setShapeType(ShapeType type)
{
    if (m_shape == type)
    {
        return;
    }
    m_shape = type;
    rebuild();
}

void LevelingDiagram::setPoints(const QList<QPointF> &points)
{
    m_points = points;
    rebuild();
}

void LevelingDiagram::setShapeSize(double chipWidth, double chipHeight)
{
    if (chipWidth > 0.0)
    {
        m_chipWidth = chipWidth;
    }
    if (chipHeight > 0.0)
    {
        m_chipHeight = chipHeight;
    }
    rebuild();
}

void LevelingDiagram::setShapeSize(double waferSizeInch)
{
    if (waferSizeInch > 0.0)
    {
        m_waferSizeInch = waferSizeInch;
    }
    rebuild();
}

void LevelingDiagram::rebuild()
{
    m_pScene->clear();
    drawAxes();
    drawShape();
    drawPoints();

    // 视野只按内容（图形+点位）适配，坐标轴参考线与坐标标注不参与缩放，
    // 避免轴范围/标注把整个图撑小；轴与标注超出部分由视口裁剪
    QRectF content;
    const QList<QGraphicsItem*> items = m_pScene->items();
    for (QGraphicsItem *it : items)
    {
        if (it->data(0).toInt() != 0)
        {
            continue;   // data(0)==1 坐标轴，==2 坐标标注
        }
        content = content.united(it->sceneBoundingRect());
    }
    // 边距取相对比例（最小 5）：芯片/晶圆很小时图形也能尽量放大
    const qreal pad = qMax(5.0, qMax(content.width(), content.height()) * 0.1);
    m_pScene->setSceneRect(content.adjusted(-pad, -pad, pad, pad));
    fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    adjustAxisWidths();
    adjustLabelOffsets();
}

void LevelingDiagram::adjustAxisWidths()
{
    // 轴宽以屏幕像素计：换算为当前缩放下的场景线宽，避免小图形被
    // fitInView 放大后坐标轴显得过粗；resize 缩放变化时同样重算
    const qreal scale = transform().m11();
    if (scale <= 0.0)
    {
        return;
    }
    const qreal width = kAxisPenPx / scale;
    const QList<QGraphicsItem*> items = m_pScene->items();
    for (QGraphicsItem *it : items)
    {
        if (it->data(0).toInt() != 1)
        {
            continue;   // data(0)==1 坐标轴与刻度
        }
        if (QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem *>(it))
        {
            QPen pen = line->pen();
            pen.setWidthF(width);
            line->setPen(pen);
        }
    }
}

void LevelingDiagram::drawAxes()
{
    QPen pen(QColor("#808080"));
    pen.setWidth(1);

    const qreal range = 200.0;
    QGraphicsLineItem *axisX = m_pScene->addLine(-range, 0, range, 0, pen);
    QGraphicsLineItem *axisY = m_pScene->addLine(0, -range, 0, range, pen);
    axisX->setZValue(1);
    axisY->setZValue(1);
    axisX->setData(0, 1);
    axisY->setData(0, 1);

    const qreal step = 20.0;
    const qreal tickHalf = 2.0;
    for (qreal v = -range; v <= range; v += step)
    {
        if (qFuzzyIsNull(v))
        {
            continue;
        }
        QGraphicsLineItem *tickX = m_pScene->addLine(v, -tickHalf, v, tickHalf, pen);
        QGraphicsLineItem *tickY = m_pScene->addLine(-tickHalf, v, tickHalf, v, pen);
        tickX->setZValue(1);
        tickY->setZValue(1);
        tickX->setData(0, 1);
        tickY->setData(0, 1);
    }
}

void LevelingDiagram::drawShape()
{
    if (m_shape == GreenSquare)
    {
        // 方形大小 = 芯片宽高（mm）
        QPen pen(QColor("#4CAF50"));
        pen.setWidthF((m_chipWidth+m_chipHeight)/10 > 1 ? 1 : (m_chipWidth+m_chipHeight)/10.);
        QBrush brush(QColor("#E8F5E9"));
        m_pScene->addRect(-m_chipWidth / 2, -m_chipHeight / 2,
                          m_chipWidth, m_chipHeight, pen, brush);
    }
    else
    {
        // 晶圆尺寸以 inch 给出，先换算为 mm 再绘制
        const qreal radius = m_waferSizeInch * kInchToMm / 2.0;
        QPen pen(QColor("#2196F3"));
        pen.setWidthF((m_chipWidth+m_chipHeight)/10 > 1 ? 1 : (m_chipWidth+m_chipHeight)/10.);
        QBrush brush(QColor("#E3F2FD"));
        m_pScene->addEllipse(-radius, -radius, radius * 2, radius * 2, pen, brush);
    }
}

void LevelingDiagram::drawPoints()
{
    QPen dotPen(QColor("#F9A825"));
    dotPen.setWidthF((m_chipWidth+m_chipHeight)/10 > 1 ? 1 : (m_chipWidth+m_chipHeight)/10.);
    QBrush dotBrush(QColor("#FFEB3B"));

    // 点半径随形状大小按比例变化：方形取短边，圆形取直径（inch 换算为 mm）
    const qreal featureSize = (m_shape == GreenSquare)
                                  ? qMin(m_chipWidth, m_chipHeight)
                                  : m_waferSizeInch * kInchToMm;
    const qreal radius = featureSize / kPointRadiusRatio;

    // 坐标文字固定屏幕像素大小，忽略视图变换：小图形被放大时文字不会跟着过大
    QFont font;
    font.setPixelSize(kLabelFontPx);

    for (const QPointF &p : m_points)
    {
        QGraphicsEllipseItem *dot = m_pScene->addEllipse(
            p.x() - radius, - p.y() - radius, radius * 2, radius * 2, dotPen, dotBrush);
        dot->setZValue(2);

        QGraphicsSimpleTextItem *label = m_pScene->addSimpleText(
            QString("(%1, %2)").arg(p.x(), 0, 'f', 3).arg(p.y(), 0, 'f', 3), font);
        label->setZValue(3);
        label->setBrush(QColor("#333333"));
        label->setPos(p.x(), -p.y());   // 先落在点上，rebuild 末尾按缩放换算像素偏移
        label->setData(0, 2);           // 坐标标注不参与视野适配
        label->setData(1, p.x());       // 记录点坐标，供 adjustLabelOffsets 重算偏移
        label->setData(2, p.y());
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    }
}

void LevelingDiagram::adjustLabelOffsets()
{
    // 标注偏移以屏幕像素计：换算为当前缩放下的场景偏移，避免小图形被
    // fitInView 放大后文字离点很远；resize 缩放变化时同样按新缩放重算
    const qreal scale = transform().m11();
    if (scale <= 0.0)
    {
        return;
    }
    const qreal dx = kLabelOffsetX / scale;
    const qreal dy = kLabelOffsetY / scale;
    const QList<QGraphicsItem*> items = m_pScene->items();
    for (QGraphicsItem *it : items)
    {
        if (it->data(0).toInt() != 2)
        {
            continue;
        }
        const QPointF pt(it->data(1).toReal(), it->data(2).toReal());
        it->setPos(pt.x() + dx, -pt.y() - dy);
    }
}

void LevelingDiagram::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    adjustAxisWidths();
    adjustLabelOffsets();
}
