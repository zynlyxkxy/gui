/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: ChuckLevelProtocol.h
* @Description: 上下吸盘调平流程与 GUI 的协议定义。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef CHUCKLEVELPROTOCOL_H
#define CHUCKLEVELPROTOCOL_H

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 本流程只有上报（流程 → GUI），无 GUI 下发命令
// ============================================================================

/// 上下吸盘调平流程上报命令
enum ChuckLevelCMD
{
    ChuckLevelCMD_None = 0,

    // ---- 上报（流程 → GUI）----
    /// 上吸盘调平点采集结果（过程中每测完一个上点推一次）
    ChuckLevelCMD_UpMeasurePoint,
    /// 下吸盘调平点采集结果（过程中每测完一个下点推一次）
    ChuckLevelCMD_DownMeasurePoint,
    /// 上下吸盘调平结果（结束前推一次）
    ChuckLevelCMD_Result,
};

/// 单点采集结果载荷（ChuckLevelCMD_UpMeasurePoint / DownMeasurePoint 的 JSON 数据）
/// 注：上吸盘调平点采集时移动对准台（posX/posY = 对准台XY）；下吸盘调平点采集时移动硅片台（posX/posY = 硅片台XY）
struct S_ChuckLevelPointData
{
    int index = 0;          // 点序号
    double nominalX = 0.0;  // 名义坐标 x（处方原值）
    double nominalY = 0.0;  // 名义坐标 y
    double posX = 0.0;      // 采集时台子坐标 x（上点=对准台XY；下点=硅片台XY）
    double posY = 0.0;      // 采集时台子坐标 y
    double cfValue = 0.0;   // 共焦测量值（上吸盘调平点=上共焦值；下吸盘调平点=下共焦值）
    YLT_REFL(S_ChuckLevelPointData, index, nominalX, nominalY, posX, posY, cfValue);
};

/// 上下吸盘调平结果载荷（ChuckLevelCMD_Result 的 JSON 数据）
struct S_ChuckLevelResultData
{
    double rxTolerance = 0.0;    // Rx 调平阈值
    double ryTolerance = 0.0;    // Ry 调平阈值
    double upChuckRx = 0.0;      // 上吸盘测量 Rx
    double upChuckRy = 0.0;      // 上吸盘测量 Ry
    double downChuckRx = 0.0;    // 下吸盘测量 Rx
    double downChuckRy = 0.0;    // 下吸盘测量 Ry
    double bondHeadRxComp = 0.0; // 键合头补偿 Rx
    double bondHeadRyComp = 0.0; // 键合头补偿 Ry
    double bondHeadRxPos = 0.0;  // 最终补偿后的键合头 Rx 绝对位置（键合等后续流程取用）
    double bondHeadRyPos = 0.0;  // 最终补偿后的键合头 Ry 绝对位置（同上）
    double startTime = 0.0;      // 运行启动时间（ms）
    double endTime = 0.0;        // 运行结束时间（ms）
    double elapsedMs = 0.0;      // 运行耗时（ms）
    YLT_REFL(S_ChuckLevelResultData, rxTolerance, ryTolerance,
             upChuckRx, upChuckRy, downChuckRx, downChuckRy,
             bondHeadRxComp, bondHeadRyComp,
             bondHeadRxPos, bondHeadRyPos, startTime, endTime, elapsedMs);
};

struct S_ChuckLevelData
{
    bool valid = false;                                     // 数据是否有效（本流程已执行并写入）
    std::vector<S_ChuckLevelPointData> downPointsData;      // 下吸盘调平点集（仅一轮采集）
    std::vector<S_ChuckLevelPointData> upPointsData;        // 上吸盘调平点集（最后一轮，报告用）
    std::vector<S_ChuckLevelPointData> upAllPointsData;     // 上吸盘调平点集（全部轮次，补偿循环各轮都保留）
    S_ChuckLevelResultData resultData;
    YLT_REFL(S_ChuckLevelData, valid, downPointsData,
             upPointsData, upAllPointsData, resultData);
};

#endif // CHUCKLEVELPROTOCOL_H
