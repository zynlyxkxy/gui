/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: CoarseLevelProtocol.h
* @Description: 粗调平流程与 GUI 的协议定义。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef __COARSE_LEVEL_PROTOCOL_H__
#define __COARSE_LEVEL_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 本流程只有上报（流程 → GUI），无 GUI 下发命令
// ============================================================================

/// 粗调平流程上报命令
enum CoarseLevelCMD
{
    CoarseLevelCMD_None = 0,

    // ---- 上报（流程 → GUI）----
    /// 单点采集结果（过程中每测完一个点推一次）
    CoarseLevelCMD_MeasurePoint,
    /// 粗调平结果（结束前推一次）
    CoarseLevelCMD_Result,
};

/// 单点采集结果载荷（CoarseLevelCMD_MeasurePoint 的 JSON 数据）
struct S_CoarseLevelPointData
{
    int index = 0;          // 点序号
    double nominalX = 0.0;  // 名义坐标 x
    double nominalY = 0.0;  // 名义坐标 y
    double alignX = 0.0;    // 对准台坐标 x（采集时）
    double alignY = 0.0;    // 对准台坐标 y
    double upCf = 0.0;      // 上共焦测量值
    double downCf = 0.0;    // 下共焦测量值
    YLT_REFL(S_CoarseLevelPointData, index, nominalX, nominalY, alignX, alignY, upCf, downCf);
};

/// 粗调平结果载荷（CoarseLevelCMD_Result 的 JSON 数据）
struct S_CoarseLevelResultData
{
    double rxTolerance = 0.0;    // Rx 调平阈值
    double ryTolerance = 0.0;    // Ry 调平阈值
    double upChuckRx = 0.0;      // 上吸盘测量 Rx
    double upChuckRy = 0.0;      // 上吸盘测量 Ry
    double downChuckRx = 0.0;    // 下吸盘测量 Rx
    double downChuckRy = 0.0;    // 下吸盘测量 Ry
    double alignZComp = 0.0;     // 对准台补偿 Z
    double bondHeadRxComp = 0.0; // 键合头补偿 Rx
    double bondHeadRyComp = 0.0; // 键合头补偿 Ry
    double bondHeadZComp = 0.0;  // 键合头补偿 Z
    double alignStageZPos = 0.0; // 最终补偿后的对准台 Z 绝对位置（键合等后续流程取用）
    double bondHeadRxPos = 0.0;  // 最终补偿后的键合头 Rx 绝对位置（同上）
    double bondHeadRyPos = 0.0;  // 最终补偿后的键合头 Ry 绝对位置（同上）
    double bondHeadZPos = 0.0;   // 最终补偿后的键合头 Z 绝对位置（同上，避免相对移动叠加误差）
    double startTime = 0.0;      // 运行启动时间（ms）
    double endTime = 0.0;        // 运行结束时间（ms）
    double elapsedMs = 0.0;      // 运行耗时（ms）
    YLT_REFL(S_CoarseLevelResultData, rxTolerance, ryTolerance,
             upChuckRx, upChuckRy, downChuckRx, downChuckRy,
             alignZComp, bondHeadRxComp, bondHeadRyComp, bondHeadZComp,
             alignStageZPos, bondHeadRxPos, bondHeadRyPos, bondHeadZPos,
             startTime, endTime, elapsedMs);
};

struct S_CoarseLevelData
{
    bool valid = false;                               // 数据是否有效（本流程已执行并写入）
    std::vector<S_CoarseLevelPointData> pointsData;   // 粗调平点集
    S_CoarseLevelResultData resultData;
    YLT_REFL(S_CoarseLevelData, valid, pointsData, resultData);
};

#endif // __COARSE_LEVEL_PROTOCOL_H__
