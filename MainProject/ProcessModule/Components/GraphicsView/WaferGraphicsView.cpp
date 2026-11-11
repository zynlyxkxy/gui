#include "ProcessModule/Components/GraphicsView/WaferGraphicsView.h"

#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QPixmap>
#include <QBrush>
#include <QPen>
#include <QPainter>
#include <QtMath>

WaferGraphicsView::WaferGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    // 抗锯齿渲染
    setRenderHint(QPainter::Antialiasing);
    // 白色网格背景
    QPixmap tilePixmap(64, 64);
    tilePixmap.fill(Qt::white);
    setBackgroundBrush(tilePixmap);
    // 场景范围 ±1000
    m_pScene = new QGraphicsScene(this);
    m_pScene->setSceneRect(-1000, -1000, 2000, 2000);
    setScene(m_pScene);
}

void WaferGraphicsView::ClearScene()
{
    // 清空全部图元（坐标系由 drawAxis 重建）；
    // scene->clear() 已删除全部图元，仅清空图元指针表（数据源 m_waferMarkPoints 保留）
    m_pScene->clear();
    m_lstWaferMarkItems.clear();
    m_pCoordLabel = nullptr;   // 坐标文本图元随 scene 一起被删除
}

void WaferGraphicsView::drawAxis()
{
    QPen axisPen(Qt::blue);
    axisPen.setWidth(2);

    // X 轴（红色）
    QPen xAxisPen(Qt::red);
    xAxisPen.setWidth(2);
    QGraphicsLineItem *xAxis = new QGraphicsLineItem(-1000, 0, 1000, 0);
    xAxis->setPen(xAxisPen);
    xAxis->setZValue(1000);
    m_pScene->addItem(xAxis);

    // Y 轴（绿色）
    QPen yAxisPen(Qt::green);
    yAxisPen.setWidth(2);
    QGraphicsLineItem *yAxis = new QGraphicsLineItem(0, -1000, 0, 1000);
    yAxis->setPen(yAxisPen);
    yAxis->setZValue(1000);
    m_pScene->addItem(yAxis);

    // 原点标签
    QGraphicsSimpleTextItem *originText = new QGraphicsSimpleTextItem("0");
    originText->setPos(5, 5);
    originText->setZValue(1001);
    m_pScene->addItem(originText);

    // 刻度与标签（每 100 画粗刻度并标数值）
    for (int i = -1000; i <= 1000; i += 50)
    {
        if (i == 0)
            continue;

        // X 轴刻度
        if (i % 100 == 0)
        {
            QGraphicsLineItem *xTick = new QGraphicsLineItem(i, -5, i, 5);
            xTick->setPen(axisPen);
            xTick->setZValue(1001);
            m_pScene->addItem(xTick);

            QGraphicsSimpleTextItem *xLabel = new QGraphicsSimpleTextItem(QString::number(i));
            xLabel->setPos(i - 10, 10);
            xLabel->setZValue(1001);
            m_pScene->addItem(xLabel);
        }

        // Y 轴刻度
        if (i % 100 == 0)
        {
            QGraphicsLineItem *yTick = new QGraphicsLineItem(-5, -i, 5, -i);
            yTick->setPen(axisPen);
            yTick->setZValue(1001);
            m_pScene->addItem(yTick);

            QGraphicsSimpleTextItem *yLabel = new QGraphicsSimpleTextItem(QString::number(i));
            yLabel->setPos(10, -i - 10);
            yLabel->setZValue(1001);
            m_pScene->addItem(yLabel);
        }
    }
}

void WaferGraphicsView::SetWaferRadius(double radius)
{
    // 半径由外部(晶圆尺寸/边缘长度)计算并设置；仅在 DisplayChipLayout 重建场景时生效
    m_waferRadius = radius;
}

void WaferGraphicsView::SetWaferNotchAngle(int angle)
{
    // 切边角度；仅在 DisplayChipLayout 重建场景时生效
    m_waferNotchAngle = angle;
}

void WaferGraphicsView::DisplayChipLayout(const QList<QPointF> &chipPositions, double chipWidth, double chipHeight)
{
    // 一键绘制晶圆+芯片布局：清空后按名义坐标绘制芯片矩形；
    // 晶圆标记点由 DisplayWaferMarkPoints 按实际名义坐标另行绘制（不再固定画在每个芯片左上/右下角）
    ClearScene();
    drawAxis();
    // 记录芯片尺寸：标记点大小随芯片尺寸缩放
    m_chipWidth = chipWidth;
    m_chipHeight = chipHeight;
    QRectF bounds;
    // 晶圆外轮廓：以原点为圆心，无填充
    if (m_waferRadius > 0)
    {
        QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(
            -m_waferRadius, -m_waferRadius, m_waferRadius * 2, m_waferRadius * 2);
        circle->setPen(QPen(Qt::black, 1));
        circle->setBrush(Qt::NoBrush);
        circle->setZValue(-1);   // 置于芯片矩形之后
        m_pScene->addItem(circle);
        bounds = bounds.united(circle->sceneBoundingRect());

        // 切边方向小三角：位于晶圆外侧，尖端指向圆心（0°→-y 指+y，90°→+x 指-x，180°→+y 指-y，270°→-x 指+x）
        QPointF dirOut;
        switch (m_waferNotchAngle)
        {
            case 0:   dirOut = QPointF( 0,  1); break;
            case 90:  dirOut = QPointF( 1,  0); break;
            case 180: dirOut = QPointF( 0, -1); break;
            case 270: dirOut = QPointF(-1,  0); break;
            default:  dirOut = QPointF(); break;
        }
        if (!dirOut.isNull())
        {
            const double h = qMax(8.0, m_waferRadius * 0.06);   // 三角高度
            const double w = h * 0.6;                            // 半底宽
            const QPointF perp(-dirOut.y(), dirOut.x());
            const QPointF apex = dirOut * m_waferRadius;              // 尖端在圆边缘
            const QPointF baseMid = dirOut * (m_waferRadius + h);     // 底边在圆外侧
            QPolygonF tri;
            tri << apex << baseMid + perp * w << baseMid - perp * w;
            QGraphicsPolygonItem *triItem = new QGraphicsPolygonItem(tri);
            triItem->setPen(QPen(Qt::black, 1));
            triItem->setBrush(QBrush(Qt::white));
            triItem->setZValue(-1);
            m_pScene->addItem(triItem);
            bounds = bounds.united(triItem->sceneBoundingRect());
        }
    }
    for (const QPointF &pos : chipPositions)
    {
        // 名义坐标(x, y)映射到场景(x, -y)：QGraphicsView y 轴向下，需翻转 y 保持数学坐标系显示
        QGraphicsRectItem *item = new QGraphicsRectItem(pos.x() - chipWidth / 2, -pos.y() - chipHeight / 2, chipWidth, chipHeight);
        item->setZValue(0);
        item->setBrush(QBrush(Qt::yellow));
        m_pScene->addItem(item);
        bounds = bounds.united(item->sceneBoundingRect());
    }
    m_chipBounds = bounds;
    m_hasChipBounds = true;
    fitToChips(bounds);
    // 场景重建后恢复晶圆标记点（数据源 m_waferMarkPoints 不随 scene 清空而丢失）
    rebuildWaferMarkItems();
}

void WaferGraphicsView::DisplayWaferMarkPoints(const std::vector<ISTAR::BasicPlatform::BasicValue::S_Point2D> &markPositions)
{
    // 记录数据源（scene 清空后据此重建）并按名义坐标重建全部标记点图元
    m_waferMarkPoints = markPositions;
    rebuildWaferMarkItems();
}

void WaferGraphicsView::rebuildWaferMarkItems()
{
    // 释放旧图元：仍在场景中的先移除再删除（scene->clear() 已删除的图元不在此路径）
    for (LevelingMarkItem *item : m_lstWaferMarkItems)
    {
        if (item->scene() == m_pScene)
            m_pScene->removeItem(item);
        delete item;
    }
    m_lstWaferMarkItems.clear();
    m_lstWaferMarkItems.reserve(int(m_waferMarkPoints.size()));

    for (const auto &p : m_waferMarkPoints)
    {
        // 名义坐标(x, y)映射到场景(x, -y)：标记点绘制在传入的精确位置
        LevelingMarkItem *item = new LevelingMarkItem(QPointF(p.x_, -p.y_), m_chipWidth, m_chipHeight);
        item->setZValue(1500);   // 位于芯片矩形之上、坐标轴刻度之下
        connect(item, &LevelingMarkItem::sig_clicked, this, &WaferGraphicsView::onWaferMarkClicked);
        m_pScene->addItem(item);
        m_lstWaferMarkItems.append(item);
    }
    update();
}

void WaferGraphicsView::onWaferMarkClicked(const QPointF &scenePos)
{
    // 点击标记点：在图上该点旁显示其名义坐标（场景 y 翻转回数学坐标，保留 3 位小数）
    const QString text = QString("(%1, %2)")
        .arg(QString::number(scenePos.x(), 'f', 3),
             QString::number(-scenePos.y(), 'f', 3));

    if (!m_pCoordLabel)
    {
        m_pCoordLabel = new QGraphicsSimpleTextItem();
        m_pCoordLabel->setZValue(1600);   // 位于芯片矩形与标记点之上
        m_pCoordLabel->setBrush(QBrush(Qt::red));   // 坐标文字用红色
        QFont font = m_pCoordLabel->font();
        font.setBold(true);                          // 加粗
        m_pCoordLabel->setFont(font);
        m_pScene->addItem(m_pCoordLabel);
    }
    m_pCoordLabel->setText(text);
    // 文本水平居中于标记点正上方
    const QRectF br = m_pCoordLabel->boundingRect();
    m_pCoordLabel->setPos(scenePos.x() - br.width() / 2, scenePos.y() - br.height() - 4.0);
    update();
}

void WaferGraphicsView::fitToChips(const QRectF &bounds)
{
    // 按芯片包围盒放大视图：空数据时退回原点居中，否则留 30% 边距自适应
    if (bounds.isNull())
    {
        centerOn(0, 0);
        update();
        return;
    }

    double padX = bounds.width() * 0.3;
    double padY = bounds.height() * 0.3;
    fitInView(bounds.adjusted(-padX, -padY, padX, padY), Qt::KeepAspectRatio);
    update();
}

void WaferGraphicsView::wheelEvent(QWheelEvent *event)
{
    // 以鼠标位置为缩放中心进行缩放
    QPointF mousePos = mapToScene(event->posF().toPoint());
    double angle = event->orientation() == Qt::Vertical ? event->angleDelta().y() : event->angleDelta().x();
    double factor = qPow(1.005, angle);
    scale(factor, factor);
    m_bIsInitialFit = false;

    QPointF newMousePos = mapToScene(event->posF().toPoint());
    QPointF delta = newMousePos - mousePos;
    QPointF center = viewport()->rect().center();
    QPointF newCenter = mapToScene(center.toPoint()) - delta;
    centerOn(newCenter);
    event->accept();
}

void WaferGraphicsView::keyPressEvent(QKeyEvent *event)
{
    // 方向键平移视图
    int step = 20;
    switch (event->key())
    {
        case Qt::Key_Up:
            verticalScrollBar()->setValue(verticalScrollBar()->value() - step);
            break;
        case Qt::Key_Down:
            verticalScrollBar()->setValue(verticalScrollBar()->value() + step);
            break;
        case Qt::Key_Left:
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - step);
            break;
        case Qt::Key_Right:
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + step);
            break;
        default:
            QGraphicsView::keyPressEvent(event);
    }
}

void WaferGraphicsView::resizeEvent(QResizeEvent *event)
{
    // 首次显示时自适应缩放：优先按芯片包围盒放大，无数据时退回整个场景
    QGraphicsView::resizeEvent(event);
    if (m_bIsInitialFit)
    {
        if (m_hasChipBounds)
            fitToChips(m_chipBounds);
        else
            fitInView(QRectF(-200, -200, 400, 400), Qt::KeepAspectRatio);
    }
}
