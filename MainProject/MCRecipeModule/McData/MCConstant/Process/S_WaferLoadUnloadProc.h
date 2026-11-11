/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_WaferLoadUnloadProc.h
* @Description: 晶圆/蓝膜上下片流程 MC 常数结构体：上下片时的 epin 升降高度、
*               硅片台上料位、上料吸附气压判定阈值、对准台让开位（装机标定得到），
*               由 S_Process 统一组织，流程在 setMCandRE 中取 mc.process.waferLoadUnloadProc。
*
* 用途约定：
*   - epin 高度：上片时把晶圆顶到交接高度 / 降到最低让位（Z 向，平台坐标）；
*   - 硅片台上料位：硅片台接片时停的位置（XY 向）；
*   - 气压阈值：硅片台吸附到位判定门限（负压，kPa，越小吸附越紧）；
*   - 对准台让开位：上下片期间对准台退到的、不妨碍上片的位置（XY 向）。
*********************************************************************************/
#ifndef S_WAFER_LOAD_UNLOAD_PROC_H
#define S_WAFER_LOAD_UNLOAD_PROC_H

#include "istar_ylt.hpp"
#include "istar_basic_value_point.h"

using namespace ISTAR::BasicPlatform::BasicValue;

struct S_WaferLoadUnloadProc
{
    double epinLoadHeight = 0.0;                    // epin 上料高度（Z向）
    double epinLowestHeight = 0.0;                  // epin 最低高度（Z向）
    S_Point2D siliconStageLoadPosXY;                // 硅片台上料位置（XY向）
    double siliconStageAirPressureThreshold = 0.0;  // 硅片台气压阈值（kPa）
    S_Point2D alignStageLoadClearPosXY;             // 对准台不妨碍上片位置（XY向）
    YLT_REFL(S_WaferLoadUnloadProc,
             epinLoadHeight, epinLowestHeight,
             siliconStageLoadPosXY, siliconStageAirPressureThreshold,
             alignStageLoadClearPosXY);
};

#endif // S_WAFER_LOAD_UNLOAD_PROC_H
