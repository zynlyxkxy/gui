/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: BondProtocol.h
* @Description: 键合流程（单步）私有协议：GUI 与 mctrl 两侧共用（自包含，可直接 cp 给 GUI）。
*               消息为 (process, command, json)，process = ProcessID::SingleProc_Bond，
*               command 从 1 起（0 = None）。
*               当前只有一条汇报：BondWork 分发工序前，把"即将执行第几条工艺"告知 GUI。
*********************************************************************************/
#ifndef BONDPROTOCOL_H
#define BONDPROTOCOL_H

#include "istar_serializer.hpp"

enum BondCMD
{
    BondCMD_None = 0,

    // ---- 汇报（流程 → GUI）----
    BondCMD_StepBegin,      // 即将执行第 N 条工艺（载荷 S_BondCMDStepBeginData；下发该工序前发出）

};

/// BondCMD_StepBegin 载荷
struct S_BondCMDStepBeginData
{
    int stepNo = 0;         // 即将执行的工艺编号（0 基，= 配方 lst_BondingSequence 的下标）
    YLT_REFL(S_BondCMDStepBeginData, stepNo);
};

#endif // BONDPROTOCOL_H
