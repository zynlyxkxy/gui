/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: ChipPosCalibProtocol.h
* @Description: 芯片位置标定流程与 GUI 的协议定义（命令枚举 + 载荷结构体）。
*               本文件自包含（仅依赖 istar_serializer.hpp），可直接交给 GUI 侧解析。
*********************************************************************************/
#ifndef __CHIP_POS_CALIB_PROTOCOL_H__
#define __CHIP_POS_CALIB_PROTOCOL_H__

#include "istar_serializer.hpp"   // YLT_REFL（结构体 → JSON）

#include <string>
#include <vector>

// ============================================================================
// 与 GUI 的协议枚举（普通 enum，值带流程名前缀避免全局冲突）
// 请求（GUI→流程）与响应（流程→GUI）的 cmd 统一放在一起；
// GUI 侧字符串 ↔ 本枚举：ISTAREnumConverter 双向互转
// ============================================================================

/// 标定流程所有可能的 GUI cmd
enum ChipPosCalibCMD
{
    ChipPosCalibCMD_None = 0,
    // ---- 请求（GUI → 流程）----
    ChipPosCalibCMD_Move,         // 移动：指令
    ChipPosCalibCMD_Capture,      // 拍照：清缓存 → 拍照 → 结果+硅片台位置
    ChipPosCalibCMD_Accept,       // 确认：将所有结果传递给控制层
    ChipPosCalibCMD_GetLastResult,
    // ---- 响应（流程 → GUI）----
    ChipPosCalibCMD_EnterInteract, // 进入交互状态
    ChipPosCalibCMD_MarkResult,    // 拍照结果（含当时硅片台位置）
    ChipPosCalibCMD_CmdRejected,   // 命令被拒
    ChipPosCalibCMD_AllMarkTurePos,// 所有标记点的真实位置
};

struct S_ChipPosCalibMarkTurePos
{
    double nominalPosX = 0.0;       // 标记名义坐标 x
    double nominalPosY = 0.0;       // 标记名义坐标 y
    double truePosX = 0.0;          // 标记真实坐标 x
    double truePosY = 0.0;          // 标记真实坐标 y
    YLT_REFL(S_ChipPosCalibMarkTurePos, nominalPosX, nominalPosY, truePosX, truePosY);
};

/// 移动命令载荷（ChipPosCalibCMD_Move 的 JSON 数据）
/// 注意：结构体名不得与枚举值 ChipPosCalibCMD_Move 同名（同名作用域冲突编不过），故加 S_ 前缀
struct S_ChipPosCalibCMD_Move
{
    int moveMode = 0;           // 0：绝对移动，1：相对移动
    double x = 0.0;             // 要移动的x（绝对移动时，输入标记的名义坐标；相对移动的时候，填写相对当前位置要移动的距离）
    double y = 0.0;             // 要移动的y（绝对移动时，输入标记的名义坐标；相对移动的时候，填写相对当前位置要移动的距离）
    YLT_REFL(S_ChipPosCalibCMD_Move, moveMode, x, y);
};

/// 命令载荷（ChipPosCalibCMD_Accept 的 JSON 数据）
struct S_ChipPosCalibCMD_Accept
{
    std::vector<S_ChipPosCalibMarkTurePos> markPoints;  // 所有标记点的信息
    YLT_REFL(S_ChipPosCalibCMD_Accept, markPoints);
};

/// 命令载荷（ChipPosCalibCMD_MarkResult 的 JSON 数据）
struct S_ChipPosCalibCMD_MarkResult
{
    bool ok = false;            // 是否识别到标记
    double posX = 0.0;          // 识别像素坐标 x
    double posY = 0.0;          // 识别像素坐标 y
    double angle = 0.0;         // 旋转角
    double score = 0.0;         // 匹配分数
    double waferX = 0.0;        // 拍照时硅片台位置 x
    double waferY = 0.0;        // 拍照时硅片台位置 y
    double waferRz = 0.0;       // 拍照时硅片台位置 rz
    double markX = 0.0;         // 计算出来的标记理论坐标 x（硅片台坐标系）
    double markY = 0.0;         // 计算出来的标记理论坐标 y（硅片台坐标系）
    YLT_REFL(S_ChipPosCalibCMD_MarkResult, ok, posX, posY, angle, score, waferX, waferY, waferRz, markX, markY);
};

/// 命令被拒载荷（ChipPosCalibCMD_CmdRejected 的 JSON 数据）
struct S_ChipPosCalibCMD_CmdRejected
{
    int command = 0;           // 被拒绝的命令值（ChipPosCalibCMD）
    std::string reason;        // 拒绝原因
    YLT_REFL(S_ChipPosCalibCMD_CmdRejected, command, reason);
};

/// 所有标记点真实位置载荷（ChipPosCalibCMD_AllMarkTurePos 的 JSON 数据，流程→GUI）
struct S_ChipPosCalibCMD_AllMarkTurePos
{
    std::vector<double> matrix;                         // 名义坐标和实际坐标的转换矩阵
    std::vector<S_ChipPosCalibMarkTurePos> markPoints;  // 所有标记点的信息
    YLT_REFL(S_ChipPosCalibCMD_AllMarkTurePos, matrix, markPoints);
};


/// 芯片位置标定数据（两组标定标记；本流程含用户交互，不上报运行时间）
struct S_ChipPosCalibData
{
    bool valid = false;                                 // 数据是否有效（本流程已执行并写入）
    std::vector<S_ChipPosCalibMarkTurePos> pointsData;  // 标定标记（接收后保存）
    S_ChipPosCalibCMD_AllMarkTurePos allMarkTurePos;    // 所有标记点的信息
    YLT_REFL(S_ChipPosCalibData, valid, pointsData, allMarkTurePos);
};

#endif // __CHIP_POS_CALIB_PROTOCOL_H__
