/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: UpLevelSelectProtocol.h
* @Description: 上调平点位选择流程与 GUI 的协议定义。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef UPLEVELSELECTPROTOCOL_H
#define UPLEVELSELECTPROTOCOL_H

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 本流程只有上报（流程 → GUI），无 GUI 下发命令
// ============================================================================

/// 上调平点位选择流程上报命令
enum UpLevelSelectCMD
{
    UpLevelSelectCMD_None = 0,

    // ---- 上报（流程 → GUI）----
    /// 单点标记定位结果（过程中每定位完一个点推一次）
    UpLevelSelectCMD_MarkLocated,
    /// 上调平点位选择结果（结束前推一次）
    UpLevelSelectCMD_Result,
};

/// 单点标记定位结果载荷（UpLevelSelectCMD_MarkLocated 的 JSON 数据）
struct S_UpLevelSelectMarkData
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
    YLT_REFL(S_UpLevelSelectMarkData, index, ok,
             nominalX, nominalY, actualX, actualY,
             stageX, stageY, pixelX, pixelY, angle, score);
};

/// 上调平点位选择结果载荷（UpLevelSelectCMD_Result 的 JSON 数据）
struct S_UpLevelSelectResultData
{
    double startTime = 0.0;   // 运行启动时间（ms）
    double endTime = 0.0;     // 运行结束时间（ms）
    double elapsedMs = 0.0;   // 运行耗时（ms）
    YLT_REFL(S_UpLevelSelectResultData, startTime, endTime, elapsedMs);
};

struct S_UpLevelSelectData
{
    bool valid = false;                                       // 数据是否有效（本流程已执行并写入）
    std::vector<S_UpLevelSelectMarkData> pointsData;          // 上调平标记定位点集
    S_UpLevelSelectResultData resultData;
    YLT_REFL(S_UpLevelSelectData, valid, pointsData, resultData);
};

#endif // UPLEVELSELECTPROTOCOL_H
