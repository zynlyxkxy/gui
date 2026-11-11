#ifndef LEVELINGMARKITEM_H
#define LEVELINGMARKITEM_H

#include <QGraphicsObject>
#include <QPointF>

// 晶圆标记点图元（小圆点，只读，不可拖动）：
// 在 WaferGraphicsView（gv_downWaferMarkPosition）上按名义坐标显示一个小圆点，
// 标记点位于传入的精确位置；点击时发出 sig_clicked(scenePos)，
// 由视图在图上该点旁绘制并显示其名义坐标（保留 3 位小数）。
// 标记大小随芯片尺寸(chipWidth/chipHeight)缩放；未提供时退回默认大小。
class LevelingMarkItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit LevelingMarkItem(const QPointF &sceneCenter, double chipWidth = 0.0,
                              double chipHeight = 0.0, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    // 标记点被点击时发出（scenePos 为标记点在场景中的位置，供视图显示坐标文本）
    void sig_clicked(const QPointF &scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    qreal calcHalfSize(double chipWidth, double chipHeight) const; // 由芯片尺寸计算标记半边长

    qreal m_half = 7.0; // 标记半边长（随芯片尺寸缩放，默认 7.0）
};

#endif // LEVELINGMARKITEM_H
