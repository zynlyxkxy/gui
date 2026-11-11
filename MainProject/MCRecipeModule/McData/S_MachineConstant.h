/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_MachineConstant.h
* @Description: 机器常数唯一载体：装机标定得到的硬件相对位置/偏置
*               （台子偏移/高度等机械数据）。原 S_MCConstant 的成员已并入
*               本结构，S_MCConstant 已废弃删除。
*               点数据使用平台 BasicValue 类型（自带 YLT_REFL，可被
*               Serializer::from_rawJson 直接解析）。
*********************************************************************************/
#ifndef S_MACHINE_CONSTANT_H
#define S_MACHINE_CONSTANT_H

#include "istar_ylt.hpp"

#include "MCConstant/S_SiliconStage.h"
#include "MCConstant/S_BondArm.h"
#include "MCConstant/S_AlignStage.h"
#include "MCConstant/S_ChipTransfer.h"
#include "MCConstant/S_Process.h"

struct S_MachineConstant {
    S_SiliconStage siliconStage;
    S_BondArm bondArm;
    S_AlignStage alignStage;
    S_ChipTransfer chipTransfer;
    S_Process process;   // 流程 MC 常数（各子流程结构体）
    YLT_REFL(S_MachineConstant,
             siliconStage, bondArm, alignStage, chipTransfer, process);
};

#endif // S_MACHINE_CONSTANT_H
