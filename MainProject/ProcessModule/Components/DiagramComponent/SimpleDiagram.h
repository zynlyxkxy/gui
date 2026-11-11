#ifndef SIMPLEDIAGRAM_H
#define SIMPLEDIAGRAM_H

#include "istar_basic_value_point.h"   // S_Point2D

#include <QColor>
#include <QGraphicsView>
#include <vector>

class QGraphicsScene;

using ISTAR::BasicPlatform::BasicValue::S_Point2D;

enum class DiagramColor
{
    Green,
    Blue,
    Yellow,
    Red,
};

/**
 * 简单图组件：底图（方形 / 圆形）+ 点位，纯展示。
 * 尺寸单位 mm；底图定视野：画点不触发缩放，点超出底图由视口裁剪。
 * 点位不常显坐标，鼠标悬停约 150ms 显示 (x, y)（自带延时，比 Qt 原生 tooltip 快）。
 */
class SimpleDiagram : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SimpleDiagram(QWidget *parent = nullptr);

    // ---- 底图（互斥：后画的替换先画的）----
    void drawSquare(double widthMm, double heightMm, DiagramColor color);
    void drawCircle(double diameterMm, DiagramColor color);

    // ---- 点位 ----
    void drawPoints(const std::vector<S_Point2D> &points, DiagramColor color = DiagramColor::Yellow);   // 追加
    void clearPoints();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;   // 悬停点位：静止后弹坐标提示
    void leaveEvent(QEvent *event) override;            // 离开视口：收提示

private:
    void rebuild();
    QRectF viewBasis() const;   // 底图包围盒；无底图时按点位内容
    void drawAxes();
    void drawShape();
    void drawPoints();
    void adjustAxisWidths();    // 轴线宽按当前缩放保持像素恒定

    QGraphicsScene *m_pScene = nullptr;

    QTimer *m_tipTimer = nullptr;   // 悬停提示延时器（单次）
    QString m_tipText;              // 当前光标下的点位提示文本（空 = 没点在点上）
    QPoint m_tipPos;                // 提示位置（视口坐标）

    enum class Shape { None, Square, Circle };
    Shape m_shape = Shape::None;
    double m_w = 160.0;         // 方形宽 / 圆形直径（mm）
    double m_h = 160.0;         // 方形高（mm）
    QColor m_outlinePen;
    QColor m_outlineFill;

    struct PointRec
    {
        S_Point2D pos;
        DiagramColor color = DiagramColor::Yellow;
    };
    std::vector<PointRec> m_points;
};

#endif // SIMPLEDIAGRAM_H
