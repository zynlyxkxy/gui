/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_FineLevelProc.h
* @Description: 精调平流程（精度模式/产率模式合并）MC 常数结构体。
*********************************************************************************/
#ifndef S_FINE_LEVEL_PROC_H
#define S_FINE_LEVEL_PROC_H

#include "istar_ylt.hpp"

struct S_FineLevelProc
{
    int maxCompensateTimes = 3;      // 补偿循环最大次数（防止不收敛死循环）
    YLT_REFL(S_FineLevelProc, maxCompensateTimes);
};

#endif // S_FINE_LEVEL_PROC_H
