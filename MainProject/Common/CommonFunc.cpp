#include "CommonFunc.h"

// S_Point2D 列表 → QPointF 列表
std::vector<QPointF> S_Point2DtoQPoints(const std::vector<S_Point2D> &pts)
{
    std::vector<QPointF> out;
    out.reserve(pts.size());
    for (const S_Point2D &p : pts)
    {
        out.emplace_back(p.x_, p.y_);
    }
    return out;
}

// QPointF 列表 → S_Point2D 列表
std::vector<S_Point2D> QPointstoS_Point2D(const std::vector<QPointF> &pts)
{
    std::vector<S_Point2D> out;
    out.reserve(pts.size());
    for (const QPointF &p : pts)
    {
        out.emplace_back(p.x(), p.y());
    }
    return out;
}
