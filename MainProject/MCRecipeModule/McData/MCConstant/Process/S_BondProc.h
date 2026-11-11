/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_BondProc.h
* @Description: 键合流程 MC 常数结构体（对准台让开位/搜位参数等；对照 1920：搜位力/
*               速度原为 IOController AO 设定（范围 0-10N / 0-3mm/s），超时原为流程内
*               写死 300s，此处统一收进机器常数）。
*********************************************************************************/
#ifndef S_BOND_PROC_H
#define S_BOND_PROC_H

#include "istar_ylt.hpp"
#include "istar_basic_value_point.h"

using namespace ISTAR::BasicPlatform::BasicValue;

struct S_BondProc
{
    double searchForce = 0.0;        // 搜位力 N（1920 SetSearchForce，范围 0-10）
    double searchSpeed = 0.0;        // 搜位速度 mm/s（1920 SetSearchPosSpeed，范围 0-3）
    int searchPosTimeoutMs = 300000; // 搜位超时 ms（1920 m_nSearchBondPosTimeout 默认 300s）
    S_Point2D alignStageSafePosXY;   // 对准台安全位（XY向，装机标定）：进 PreBond 先把对准台移到这里让开
    YLT_REFL(S_BondProc, searchForce, searchSpeed, searchPosTimeoutMs, alignStageSafePosXY);
};

#endif // S_BOND_PROC_H
