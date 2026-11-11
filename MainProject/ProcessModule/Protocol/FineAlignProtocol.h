/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: FineAlignProtocol.h
* @Description: 精对准流程与 GUI 的协议定义。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef __FINE_ALIGN_PROTOCOL_H__
#define __FINE_ALIGN_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 本流程只有上报（流程 → GUI），无 GUI 下发命令
// ============================================================================

/// 精对准流程上报命令
enum FineAlignCMD
{
    FineAlignCMD_None = 0,

    // ---- 上报（流程 → GUI）----
    /// 上标记定位结果（过程中每定位完一个上标记推一次）
    FineAlignCMD_UpMarkLocated,
    /// 下标记定位结果（过程中每定位完一个下标记推一次）
    FineAlignCMD_DownMarkLocated,
    /// 精对准结果（结束前推一次）
    FineAlignCMD_Result,
};

/// 单点标记定位结果载荷（FineAlignCMD_UpMarkLocated / DownMarkLocated 的 JSON 数据）
struct S_FineAlignMarkData
{
    int index = 0;            // 点序号
    bool ok = false;          // 是否识别到标记
    double nominalX = 0.0;    // 名义坐标 x（处方原值，芯片局部坐标系）
    double nominalY = 0.0;    // 名义坐标 y
    double actualX = 0.0;     // 实际坐标 x（对准台，像素修正后的准确位置，机器坐标 mm）
    double actualY = 0.0;     // 实际坐标 y
    double stageX = 0.0;      // 找到标记时对准台坐标 x（绕圈搜点命中的位置）
    double stageY = 0.0;      // 找到标记时对准台坐标 y
    double pixelX = 0.0;      // 识别像素坐标 x
    double pixelY = 0.0;      // 识别像素坐标 y
    double angle = 0.0;       // 识别旋转角
    double score = 0.0;       // 匹配分数
    YLT_REFL(S_FineAlignMarkData, index, ok,
             nominalX, nominalY, actualX, actualY,
             stageX, stageY, pixelX, pixelY, angle, score);
};

/// 精对准结果载荷（FineAlignCMD_Result 的 JSON 数据）
struct S_FineAlignResultData
{
    double xTolerance = 0.0;   // X 对准阈值
    double yTolerance = 0.0;   // Y 对准阈值
    double upChipX = 0.0;      // 上芯片位置 x
    double upChipY = 0.0;      // 上芯片位置 y
    double upChipRz = 0.0;     // 上芯片位置 rz
    double downChipX = 0.0;    // 下芯片位置 x
    double downChipY = 0.0;    // 下芯片位置 y
    double downChipRz = 0.0;   // 下芯片位置 rz
    double compX = 0.0;        // 计算结果补偿值 x（硅片台）
    double compY = 0.0;        // 计算结果补偿值 y
    double compRz = 0.0;       // 计算结果补偿值 rz
    double waferStageXPos = 0.0;   // 最终补偿后的硅片台 X 绝对位置（键合等后续流程取用）
    double waferStageYPos = 0.0;   // 最终补偿后的硅片台 Y 绝对位置（同上）
    double waferStageRzPos = 0.0;  // 最终补偿后的硅片台 Rz 绝对位置（同上）
    double startTime = 0.0;    // 运行启动时间（ms）
    double endTime = 0.0;      // 运行结束时间（ms）
    double elapsedMs = 0.0;    // 运行耗时（ms）
    YLT_REFL(S_FineAlignResultData, xTolerance, yTolerance,
             upChipX, upChipY, upChipRz, downChipX, downChipY, downChipRz,
             compX, compY, compRz,
             waferStageXPos, waferStageYPos, waferStageRzPos,
             startTime, endTime, elapsedMs);
};

struct S_FineAlignData
{
    bool valid = false;                                    // 数据是否有效（本流程已执行并写入）
    std::vector<S_FineAlignMarkData> downPointsData;       // 下标记定位点集（最后一轮，报告用）
    std::vector<S_FineAlignMarkData> downAllPointsData;    // 下标记定位点集（全部轮次，补偿循环各轮都保留）
    std::vector<S_FineAlignMarkData> upPointsData;         // 上标记定位点集（仅一轮采集）
    S_FineAlignResultData resultData;
    YLT_REFL(S_FineAlignData, valid,
             downPointsData, downAllPointsData, upPointsData, resultData);
};

#endif // __FINE_ALIGN_PROTOCOL_H__
