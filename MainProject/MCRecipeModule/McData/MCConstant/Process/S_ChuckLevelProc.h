/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_ChuckLevelProc.h
* @Description: 上下吸盘调平流程 MC 常数结构体。
*********************************************************************************/
#ifndef S_CHUCK_LEVEL_PROC_H
#define S_CHUCK_LEVEL_PROC_H

#include "istar_ylt.hpp"

struct S_ChuckLevelProc
{
    int maxCompensateTimes = 3;      // 补偿循环最大次数（防止不收敛死循环）
    YLT_REFL(S_ChuckLevelProc, maxCompensateTimes);
};

#endif // S_CHUCK_LEVEL_PROC_H
