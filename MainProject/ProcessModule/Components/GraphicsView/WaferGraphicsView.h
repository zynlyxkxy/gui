#ifndef WAFERGRAPHICSVIEW_H
#define WAFERGRAPHICSVIEW_H

#include "ProcessModule/Components/GraphicsView/LevelingMarkItem.h"
#include "istar_basic_value_point.h"

#include <QGraphicsView>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <vector>

class QGraphicsSimpleTextItem;   // 仅前向声明：点击标记后显示的坐标文本图元在 .cpp 中创建

// 晶圆标记点图形视图（ProcessPage.ui 中 gv_downWaferMarkPosition 提升为本类）：
// 已按实际使用精简 —— 保留的功能：
//   1) 按输入参数画标记点：DisplayChipLayout 绘制晶圆+芯片布局，DisplayWaferMarkPoints
//      按各标记点名义坐标(vector<S_Point2D>)绘制小圆点（取代旧版每个芯片左上/右下固定角点画法）；
//   2) 点击标记点在图上该点旁显示其坐标（保留 3 位小数）；
//   3) 绘制坐标轴；以及滚轮缩放 / 方向键平移 / 首次显示自适应缩放等视图交互。
class WaferGraphicsView : public QGraphicsView
{
public:
    explicit WaferGraphicsView(QWidget *parent = nullptr);

    // 一键绘制晶圆+芯片布局：按晶圆直径(半径=直径/2)与切边角绘制外轮廓，并按名义坐标绘制芯片矩形（黄色）
    void DisplayChipLayout(const QList<QPointF> &chipPositions, double chipWidth, double chipHeight);
    // 设置晶圆半径：DisplayChipLayout 重建场景时以原点为圆心绘制晶圆外轮廓（无填充）
    void SetWaferRadius(double radius);
    // 设置切边角度(0/90/180/270)：在晶圆外侧画指示切边位置的小三角，尖端指向圆心
    void SetWaferNotchAngle(int angle);
    // 按各标记点的实际名义坐标在芯片/晶圆布局上绘制可点击的小圆点标记：
    // markPositions 提供每个标记点的具体位置（取代固定画在每个芯片左上/右下角）；
    // 点击标记时在图上该点旁显示其名义坐标（保留 3 位小数）。
    // 场景清空重建（如再次 DisplayChipLayout）后自动按最近一次传入的数据源恢复标记点。
    void DisplayWaferMarkPoints(const std::vector<ISTAR::BasicPlatform::BasicValue::S_Point2D> &markPositions);

protected:
    // 滚轮缩放（以鼠标位置为缩放中心）
    void wheelEvent(QWheelEvent *event) override;
    // 方向键平移视图
    void keyPressEvent(QKeyEvent *event) override;
    // 首次显示时按场景范围自适应缩放
    void resizeEvent(QResizeEvent *event) override;

private:
    // 清空场景中的内容（坐标系由 drawAxis 重建，标记点数据源保留）
    void ClearScene();
    // 绘制坐标轴、原点与刻度
    void drawAxis();
    // 按 m_waferMarkPoints 数据源重建全部标记点图元（scene 清空后用于恢复标记）
    void rebuildWaferMarkItems();
    // 标记点点击：在图上该点旁绘制/移动坐标文本（名义坐标，保留 3 位小数）
    void onWaferMarkClicked(const QPointF &scenePos);
    // 按包围盒自适应放大视图（留 30% 边距；空数据时退回原点居中）
    void fitToChips(const QRectF &bounds);

private:
    QGraphicsScene *m_pScene = nullptr;        // 图形场景
    QRectF m_chipBounds;                       // 最近一次绘制的芯片包围盒
    bool m_hasChipBounds = false;              // 是否已有芯片包围盒数据
    bool m_bIsInitialFit = true;               // 是否处于首次布局
    double m_waferRadius = 0.0;                // 晶圆半径(>0 时绘制外轮廓圆)
    int m_waferNotchAngle = 0;                 // 切边角度(0/90/180/270)
    double m_chipWidth = 0.0;                  // 芯片宽（标记尺寸基准，DisplayChipLayout 时记录）
    double m_chipHeight = 0.0;                 // 芯片高（标记尺寸基准，DisplayChipLayout 时记录）
    // 晶圆标记点数据源（名义坐标）+ 图元；scene 清空后据此重建
    std::vector<ISTAR::BasicPlatform::BasicValue::S_Point2D> m_waferMarkPoints;
    QList<LevelingMarkItem *> m_lstWaferMarkItems;
    QGraphicsSimpleTextItem *m_pCoordLabel = nullptr;   // 点击标记后显示的坐标文本（位于该点旁）
};

#endif // WAFERGRAPHICSVIEW_H
