/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_Process.h
* @Description: 流程 MC 常数总结构体：统一组织 Process 目录下各流程的
*               机器常数结构体，业务侧只需 include 本文件即可。
*********************************************************************************/
#ifndef S_PROCESS_H
#define S_PROCESS_H

#include "istar_ylt.hpp"

#include "Process/S_CommonProc.h"
#include "Process/S_ChipPosCalibProc.h"
#include "Process/S_ChuckLevelProc.h"
#include "Process/S_FineLevelProc.h"
#include "Process/S_BondProc.h"
#include "Process/S_WaferLoadUnloadProc.h"

struct S_Process
{
    S_CommonProc commonProc;
    S_ChipPosCalibProc chipPosCalibProc;
    S_ChuckLevelProc chuckLevelProc;
    S_FineLevelProc fineLevelProc;
    S_BondProc bondProc;
    S_WaferLoadUnloadProc waferLoadUnloadProc;   // 晶圆/蓝膜上下片
    YLT_REFL(S_Process,
             commonProc, chipPosCalibProc, chuckLevelProc, fineLevelProc, bondProc,
             waferLoadUnloadProc);
};

#endif // S_PROCESS_H
