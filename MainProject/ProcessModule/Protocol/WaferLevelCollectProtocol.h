/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: WaferLevelCollectProtocol.h
* @Description: 晶圆调平数据采集流程与 GUI 的协议定义。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef __WAFER_LEVEL_COLLECT_PROTOCOL_H__
#define __WAFER_LEVEL_COLLECT_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 本流程只有上报（流程 → GUI），无 GUI 下发命令
// ============================================================================

/// 晶圆调平流程上报命令
enum WaferLevelCollectCMD
{
    WaferLevelCollectCMD_None = 0,

    // ---- 上报（流程 → GUI）----
    /// 单点采集结果（过程中每测完一个点推一次）
    WaferLevelCollectCMD_MeasurePoint,
    /// 晶圆调平结果（结束前推一次）
    WaferLevelCollectCMD_Result,
};

/// 单点采集结果载荷（WaferLevelCollectCMD_MeasurePoint 的 JSON 数据）
struct S_WaferLevelPointData
{
    int index = 0;          // 点序号
    double nominalX = 0.0;  // 名义坐标 x
    double nominalY = 0.0;  // 名义坐标 y
    double waferX = 0.0;    // 硅片台坐标 x（采集时）
    double waferY = 0.0;    // 硅片台坐标 y
    double downCf = 0.0;    // 下光谱共焦测量值
    YLT_REFL(S_WaferLevelPointData, index, nominalX, nominalY, waferX, waferY, downCf);
};

/// 晶圆调平结果载荷（WaferLevelCollectCMD_Result 的 JSON 数据）
struct S_WaferLevelResultData
{
    double alignStageZWithWafer = 0.0;   // 有晶圆情况下的最佳对准台高度
    double waferRx = 0.0;   // 下晶圆测量 Rx
    double waferRy = 0.0;   // 下晶圆测量 Ry
    double startTime = 0.0; // 运行启动时间（ms）
    double endTime = 0.0;   // 运行结束时间（ms）
    double elapsedMs = 0.0; // 运行耗时（ms）
    YLT_REFL(S_WaferLevelResultData, alignStageZWithWafer, waferRx, waferRy, startTime, endTime, elapsedMs);
};

struct S_WaferLevelData
{
    bool valid = false;                              // 数据是否有效（本流程已执行并写入）
    std::vector<S_WaferLevelPointData> pointsData;   // 晶圆调平点集
    S_WaferLevelResultData resultData;
    YLT_REFL(S_WaferLevelData, valid, pointsData, resultData);
};

#endif // __WAFER_LEVEL_COLLECT_PROTOCOL_H__
