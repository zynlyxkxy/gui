#include "ChipPosCalibDiagram.h"

ChipPosCalibDiagram::ChipPosCalibDiagram(QWidget *parent)
    : DiagramBase(parent)
{
    connect(this, &DiagramBase::pointClicked, this, &ChipPosCalibDiagram::onPointClicked);
}

void ChipPosCalibDiagram::setNominal(const std::vector<QPointF> &chipNominalCenters,
                                     const std::vector<QPointF> &markPointNominals,
                                     double waferCx, double waferCy, double chipW, double chipH,
                                     double waferDiameterInch)
{
    m_center = QPointF(waferCx, waferCy);
    setCenter(waferCx, waferCy);            // 基类存一份，外部换算用
    m_pointReals.clear();                   // 回到名义阶段（不可点击）

    // 名义坐标 → 理论坐标（= 名义 + 晶圆中心）：入口一次换算，之后本类内部只用理论坐标
    //（画点位置、标红、点击标注的名义值都是理论坐标，不再以中心为 0,0 呈现）
    std::vector<QPointF> chipTheoreticals;
    chipTheoreticals.reserve(chipNominalCenters.size());
    for (const QPointF &n : chipNominalCenters)
    {
        chipTheoreticals.push_back(n + m_center);
    }

    m_pointTheoreticals.clear();
    m_pointTheoreticals.reserve(markPointNominals.size());
    for (const QPointF &n : markPointNominals)
    {
        m_pointTheoreticals.push_back(n + m_center);
    }

    // 整图重置（处方更新 = 重调本接口）：旧底图、旧点、旧红、旧标注全清
    clearBackdrop();
    if (waferDiameterInch > 0.0)
    {
        constexpr double kInchToMm = 25.4;  // 1 inch = 25.4 mm（精确值）
        drawCircle(waferCx, waferCy, waferDiameterInch * kInchToMm);
    }
    clearPoints();

    // 芯片 / 标定点都画在理论坐标处（场景坐标 = 理论坐标）
    setDies(chipTheoreticals, chipW, chipH, QColor(Qt::yellow));
    addPoints(m_pointTheoreticals, QColor(Qt::black));
}

void ChipPosCalibDiagram::setRedPoints(const std::vector<QPointF> &markPointNominals)
{
    // 增量标红：入参是名义坐标，换算成理论坐标后改色（基类按最近点匹配），不重画
    for (const QPointF &n : markPointNominals)
    {
        setPointColor(n + m_center, QColor(Qt::red));
    }
}

bool ChipPosCalibDiagram::setRealPoints(const std::vector<QPointF> &markPointReals)
{
    if (markPointReals.size() != m_pointTheoreticals.size())
    {
        m_pointReals.clear();               // 长度不符：一个都不绑（同时清掉旧绑定）
        return false;
    }
    m_pointReals = markPointReals;          // 同序同长：全部绑定；点位不动、不清图
    return true;
}

void ChipPosCalibDiagram::onPointClicked(const QPointF &pos, const QColor &color,
                                         Qt::MouseButton btn)
{
    Q_UNUSED(color);
    if (m_pointReals.empty())
    {
        return;                             // 名义阶段（未绑定真实坐标）不可点击
    }

    const int i = indexOfDrawn(pos);
    if (i < 0)
    {
        return;
    }
    const QPointF &real = m_pointReals[static_cast<size_t>(i)];
    const QPointF &theory = m_pointTheoreticals[static_cast<size_t>(i)];
    const QString text = QString("实际坐标(%1, %2)\n理论坐标(%3, %4)")   // 绑定的真实坐标 + 理论坐标两行
                             .arg(real.x(), 0, 'f', 3).arg(real.y(), 0, 'f', 3)
                             .arg(theory.x(), 0, 'f', 3).arg(theory.y(), 0, 'f', 3);

    if (btn == Qt::LeftButton)
    {
        const bool wasMarked = pointMarked(pos);
        clearPointMarks();
        if (!wasMarked)
        {
            markPointText(pos, text.toStdString());   // 清其他显当前；点已显的 → 全清
        }
    }
    else if (btn == Qt::RightButton)
    {
        if (pointMarked(pos))
        {
            unmarkPoint(pos);        // 已标注 → 清掉这条
        }
        else
        {
            markPointText(pos, text.toStdString());   // 未标注 → 追加（不影响其他）
        }
    }
}

int ChipPosCalibDiagram::indexOfDrawn(const QPointF &drawnPos) const
{
    // 点击回传的就是画点时传入的理论坐标，同操作数直接比位级
    for (size_t i = 0; i < m_pointTheoreticals.size(); ++i)
    {
        if (m_pointTheoreticals[i] == drawnPos)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}
