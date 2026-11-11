/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: InitProtocol.h
* @Description: 初始化流程协议（SingleProc_Init）：本流程无数据上报，
*               只记录启动参数。枚举 InitTarget 见 ProcessManagerProtocol.h
*               （GUI Start 的 json 载荷：{"target":"acs"|"plc"}）。
*               本文件自包含，可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef __INIT_PROTOCOL_H__
#define __INIT_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include "ProcessManagerProtocol.h"   // InitTarget（初始化对象）

// ============================================================================
// 初始化流程启动协议（SingleProc_Init）：无流程数据上报，启动参数仅指示初始化哪一部分。
// 枚举在本文件（GUI Start 的 json 载荷 {"target":"all"|"acs"|"plc"}）；
// 参数记录结构体在 InitProtocol.h（S_InitData）
// ============================================================================

/// 初始化流程的json对象（GUI 指定初始化哪一部分；枚举值名即参数键字符串）
enum class InitTarget
{
    all,    // 全部初始化
    acs,    // 初始化 ACS（运动控制器）
    plc,    // 初始化 PLC
};

/// <summary>初始化流程执行参数（调度中心解析 Start json 后写入数据中心，
/// 流程运行初始化时读取，按 target 执行对应初始化动作）</summary>
struct S_InitData
{
    InitTarget target;   // 本次要初始化的对象（all/acs/plc）
    YLT_REFL(S_InitData, target);
};

#endif // __INIT_PROTOCOL_H__
