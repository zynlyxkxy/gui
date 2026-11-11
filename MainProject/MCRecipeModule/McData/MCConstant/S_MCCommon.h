/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_MCCommon.h
* @Description: MC 常数公共定义：被 MCConstant 目录下各结构体头共享引用的
*               基础常数结构体（当前为轴公共常数 S_MCCommon_Axis）。
*********************************************************************************/
#ifndef S_MCCOMMON_H
#define S_MCCOMMON_H

#include "istar_ylt.hpp"

struct S_MCCommon_Axis {
    double vel;                  // 速度
    double accVel;               // 加速度
    double tolerance;            // 误差
    double softLimitPositive;    // 正向软限位
    double softLimitNegative;    // 负向软限位
    int settlingTime;            // 整定时间ms
    int moveTimeOut;             // 移动超时
    YLT_REFL(S_MCCommon_Axis, vel, accVel, moveTimeOut, softLimitPositive,
             softLimitNegative, settlingTime, tolerance);
};

#endif // S_MCCOMMON_H
