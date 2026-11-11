#ifndef LEVELINGDIAGRAM_H
#define LEVELINGDIAGRAM_H

#include <QGraphicsView>
#include <QList>
#include <QPointF>

class QGraphicsScene;

class LevelingDiagram : public QGraphicsView
{
    Q_OBJECT

public:
    enum ShapeType
    {
        GreenSquare,
        BlueCircle
    };

    explicit LevelingDiagram(QWidget *parent = nullptr);

    void setShapeType(ShapeType type);
    void setPoints(const QList<QPointF> &points);

    // 方形尺寸：芯片宽度/高度（单位 mm），按两个参数区分重载
    void setShapeSize(double chipWidth, double chipHeight);
    // 圆形尺寸：晶圆尺寸（单位 inch），绘制前换算为 mm，按单个参数区分重载
    void setShapeSize(double waferSizeInch);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuild();
    void drawAxes();
    void drawShape();
    void drawPoints();
    // 按当前缩放把坐标标注的屏幕像素偏移换算为场景偏移（保持文字紧贴点位）
    void adjustLabelOffsets();
    // 按当前缩放把坐标轴的场景线宽换算为屏幕像素宽（小图形放大时轴不变粗）
    void adjustAxisWidths();

private:
    QGraphicsScene *m_pScene;
    ShapeType m_shape = GreenSquare;
    QList<QPointF> m_points;
    double m_chipWidth = 160.0;    // 芯片宽度（mm），方形大小
    double m_chipHeight = 160.0;   // 芯片高度（mm），方形大小
    double m_waferSizeInch = 6.0;  // 晶圆尺寸（inch），圆形大小
};

#endif // LEVELINGDIAGRAM_H
