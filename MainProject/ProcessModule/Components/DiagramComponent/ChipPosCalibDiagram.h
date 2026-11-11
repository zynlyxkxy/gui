#ifndef CHIPPOSCALIBDIAGRAM_H
#define CHIPPOSCALIBDIAGRAM_H

#include "ProcessModule/Components/DiagramComponent/Base/DiagramBase.h"

#include <QColor>
#include <QPointF>
#include <vector>

/**
 * 芯片位置标定图：基于 DiagramBase 的具体图。
 * 坐标约定：入参为处方名义坐标（相对晶圆中心）；本类一次换算成理论坐标
 * （理论 = 名义 + 晶圆中心），此后画点位置、标红、点击标注全部用理论坐标，
 * 不再以晶圆中心为 (0, 0) 呈现名义值。
 * 名义阶段：芯片黄、全部标定点黑（画在理论坐标），setRedPoints 把需标定的子集增量标红
 * （不重画、不清旧红），此时点不可点击；
 * 真实阶段：setRealPoints 绑定全部标定点的真实坐标（与标定点同序同长；点位不动、不清图），
 * 此后点可点击，标注两行：真实坐标 + 理论坐标（同一坐标系，差值为标定偏差）。
 */
class ChipPosCalibDiagram : public DiagramBase
{
    Q_OBJECT

public:
    explicit ChipPosCalibDiagram(QWidget *parent = nullptr);

    // ---- 名义阶段：芯片/标定点名义坐标（相对晶圆中心）+ 晶圆中心（绝对坐标，硅片台系）
    //      + 晶圆直径（单位 inch，0 = 不画大圆）----
    // 重调 = 整图重置：旧底图/芯片/点/红点/真实绑定全清
    void setNominal(const std::vector<QPointF> &chipNominalCenters,
                    const std::vector<QPointF> &markPointNominals,
                    double waferCx, double waferCy, double chipW, double chipH,
                    double waferDiameterInch = 0.0);

    // ---- 增量标红：传名义坐标（setNominal 点集的子集，基类按最近点匹配）；不重画、不清旧红 ----
    void setRedPoints(const std::vector<QPointF> &markPointNominals);

    // ---- 真实阶段：绑定全部标定点的真实坐标（与标定点同序同长），点位不动、不清图；
    //      false = 数量不符未生效 ----
    bool setRealPoints(const std::vector<QPointF> &markPointReals);

private:
    void onPointClicked(const QPointF &pos, const QColor &color, Qt::MouseButton btn);
    int indexOfDrawn(const QPointF &drawnPos) const;   // 点击回传的画点位 → 点下标（位级一致）

    QPointF m_center;                          // 晶圆中心（绝对坐标）
    std::vector<QPointF> m_pointTheoreticals;  // 标定点理论坐标（= 处方名义 + 中心；画点即此值）
    std::vector<QPointF> m_pointReals;         // 绑定的真实坐标（同序）；空 = 名义阶段（不可点击）
};

#endif // CHIPPOSCALIBDIAGRAM_H
