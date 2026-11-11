#ifndef COMMONFUNC_H
#define COMMONFUNC_H

#include "istar_basic_value_point.h"   // S_Point2D

#include <QPointF>
#include <vector>

using ISTAR::BasicPlatform::BasicValue::S_Point2D;

/**
 * 通用函数集合（多处都要用的才往这里放）：
 * 谁用谁 include "Common/CommonFunc.h"，不要在各自文件里再写一份局部实现，
 * 避免同名逻辑多处漂移。
 */

// S_Point2D 列表 → QPointF 列表
std::vector<QPointF> S_Point2DtoQPoints(const std::vector<S_Point2D> &pts);

// QPointF 列表 → S_Point2D 列表
std::vector<S_Point2D> QPointstoS_Point2D(const std::vector<QPointF> &pts);

#endif // COMMONFUNC_H
