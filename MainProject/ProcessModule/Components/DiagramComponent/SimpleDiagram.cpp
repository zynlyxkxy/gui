#include "SimpleDiagram.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QTimer>
#include <QToolTip>

namespace {

const qreal kPointRadiusRatio = 40.0;   // 点半径 = 底图特征尺寸 / 40
const qreal kAxisPenPx = 1.0;           // 轴线宽（屏幕像素）
const int kTipDelayMs = 150;            // 悬停提示延时（毫秒）
const int kTipDataKey = 9;              // 图元上存提示文本的 data 槽位（0 已被轴线占用）

struct ColorPair
{
    QColor pen;
    QColor fill;
};

ColorPair colorOf(DiagramColor color)
{
    switch (color)
    {
        case DiagramColor::Green:  return { QColor("#4CAF50"), QColor("#E8F5E9") };
        case DiagramColor::Blue:   return { QColor("#2196F3"), QColor("#E3F2FD") };
        case DiagramColor::Yellow: return { QColor("#F9A825"), QColor("#FFEB3B") };
        case DiagramColor::Red:    return { QColor("#F44336"), QColor("#FFEBEE") };
    }
    return { QColor("#2196F3"), QColor("#E3F2FD") };
}

}  // namespace

SimpleDiagram::SimpleDiagram(QWidget *parent)
    : QGraphicsView(parent)
    , m_pScene(new QGraphicsScene(this))
{
    setScene(m_pScene);
    setRenderHint(QPainter::Antialiasing);

    // 悬停坐标提示：自带延时（比 Qt 原生 tooltip 的 700ms 快）
    viewport()->setMouseTracking(true);
    m_tipTimer = new QTimer(this);
    m_tipTimer->setSingleShot(true);
    m_tipTimer->setInterval(kTipDelayMs);
    connect(m_tipTimer, &QTimer::timeout, this, [this]()
    {
        if (!m_tipText.isEmpty())
        {
            QToolTip::showText(viewport()->mapToGlobal(m_tipPos), m_tipText, viewport());
        }
    });

    rebuild();
}

void SimpleDiagram::drawSquare(double widthMm, double heightMm, DiagramColor color)
{
    if (widthMm <= 0.0 || heightMm <= 0.0)
    {
        return;
    }
    m_shape = Shape::Square;
    m_w = widthMm;
    m_h = heightMm;
    const ColorPair cp = colorOf(color);
    m_outlinePen = cp.pen;
    m_outlineFill = cp.fill;
    rebuild();
}

void SimpleDiagram::drawCircle(double diameterMm, DiagramColor color)
{
    if (diameterMm <= 0.0)
    {
        return;
    }
    m_shape = Shape::Circle;
    m_w = diameterMm;
    m_h = diameterMm;
    const ColorPair cp = colorOf(color);
    m_outlinePen = cp.pen;
    m_outlineFill = cp.fill;
    rebuild();
}

void SimpleDiagram::drawPoints(const std::vector<S_Point2D> &points, DiagramColor color)
{
    for (const S_Point2D &p : points)
    {
        m_points.push_back(PointRec{ p, color });
    }
    rebuild();
}

void SimpleDiagram::clearPoints()
{
    m_points.clear();
    rebuild();
}

void SimpleDiagram::rebuild()
{
    m_tipTimer->stop();     // 场景要重建：撤掉待弹 / 已弹的坐标提示
    QToolTip::hideText();
    m_tipText.clear();

    m_pScene->clear();
    drawAxes();
    drawShape();
    drawPoints();

    const QRectF basis = viewBasis();
    const qreal pad = qMax(5.0, qMax(basis.width(), basis.height()) * 0.1);
    m_pScene->setSceneRect(basis.adjusted(-pad, -pad, pad, pad));
    fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    adjustAxisWidths();
}

QRectF SimpleDiagram::viewBasis() const
{
    if (m_shape != Shape::None)
    {
        return QRectF(-m_w / 2, -m_h / 2, m_w, m_h);   // 底图定视野
    }

    if (!m_points.empty())
    {
        QRectF content(m_points.front().pos.x_, m_points.front().pos.y_, 0.0, 0.0);
        for (const PointRec &rec : m_points)
        {
            content = content.united(QRectF(rec.pos.x_, rec.pos.y_, 0.0, 0.0));
        }
        return content;
    }

    return QRectF(-100.0, -100.0, 200.0, 200.0);
}

void SimpleDiagram::drawAxes()
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

void SimpleDiagram::drawShape()
{
    if (m_shape == Shape::None)
    {
        return;
    }

    QPen pen(m_outlinePen);
    pen.setWidthF(1.0);
    QBrush brush(m_outlineFill);

    if (m_shape == Shape::Square)
    {
        m_pScene->addRect(-m_w / 2, -m_h / 2, m_w, m_h, pen, brush);
    }
    else
    {
        const qreal radius = m_w / 2.0;
        m_pScene->addEllipse(-radius, -radius, radius * 2, radius * 2, pen, brush);
    }
}

void SimpleDiagram::drawPoints()
{
    if (m_points.empty())
    {
        return;
    }

    const qreal featureSize = (m_shape == Shape::Square) ? qMin(m_w, m_h) : m_w;
    const qreal radius = featureSize / kPointRadiusRatio;

    for (const PointRec &rec : m_points)
    {
        const ColorPair cp = colorOf(rec.color);
        QPen dotPen(cp.pen);
        dotPen.setWidthF(1.0);
        QBrush dotBrush(cp.fill);

        const S_Point2D &p = rec.pos;
        QGraphicsEllipseItem *dot = m_pScene->addEllipse(
            p.x_ - radius, -p.y_ - radius, radius * 2, radius * 2, dotPen, dotBrush);
        dot->setZValue(2);
        // 提示文本挂在图元上（延时与显示由视图控制，见 mouseMoveEvent）
        dot->setData(kTipDataKey, QString("(%1, %2)").arg(p.x_, 0, 'f', 3).arg(p.y_, 0, 'f', 3));
    }
}

void SimpleDiagram::adjustAxisWidths()
{
    const qreal scale = transform().m11();
    if (scale <= 0.0)
    {
        return;
    }
    const qreal width = kAxisPenPx / scale;
    const QList<QGraphicsItem *> items = m_pScene->items();
    for (QGraphicsItem *it : items)
    {
        if (it->data(0).toInt() != 1)
        {
            continue;
        }
        if (QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem *>(it))
        {
            QPen pen = line->pen();
            pen.setWidthF(width);
            line->setPen(pen);
        }
    }
}

void SimpleDiagram::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    adjustAxisWidths();
}

// 悬停点位：光标下的提示文本变了就重新计时，静止 kTipDelayMs 后弹出坐标
void SimpleDiagram::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);

    const QGraphicsItem *item = itemAt(event->pos());
    const QString tip = item ? item->data(kTipDataKey).toString() : QString();
    m_tipPos = event->pos();
    if (tip == m_tipText)
    {
        return;   // 还在同一个点上：保持原计时，位置已更新
    }
    m_tipText = tip;
    m_tipTimer->stop();
    QToolTip::hideText();
    if (!m_tipText.isEmpty())
    {
        m_tipTimer->start();
    }
}

void SimpleDiagram::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    m_tipTimer->stop();
    QToolTip::hideText();
    m_tipText.clear();
}
