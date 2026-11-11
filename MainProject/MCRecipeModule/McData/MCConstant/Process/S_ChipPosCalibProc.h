/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_ChipPosCalibProc.h
* @Description: 芯片位置标定流程 MC 常数结构体。
*********************************************************************************/
#ifndef S_CHIP_POS_CALIB_PROC_H
#define S_CHIP_POS_CALIB_PROC_H

#include "istar_ylt.hpp"

struct S_ChipPosCalibProc
{
    int maxZCompensateTimes = 3;       // 对准台Z补偿最大尝试次数（防止不收敛死循环）
    double zCompensatePrecision = 0.0; // 对准台Z补偿收敛精度（mm，下共焦测高结果判断收敛用）
    YLT_REFL(S_ChipPosCalibProc, maxZCompensateTimes, zCompensatePrecision);
};

#endif // S_CHIP_POS_CALIB_PROC_H
