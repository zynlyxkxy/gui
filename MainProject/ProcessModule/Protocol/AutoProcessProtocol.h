#ifndef AUTOPROCESSPROTOCOL_H
#define AUTOPROCESSPROTOCOL_H

#include "istar_serializer.hpp"

/// 自动连续键合流程命令（流程 → GUI，主动上报）
enum AutoProcessCMD
{
    AutoProcessCMD_None = 0,
    AutoProcessCMD_CurProcess,
    AutoProcessCMD_Finished,   // 自动流程完成（载荷 S_AutoContinueBondFinishedData）
};

/// 自动流程完成载荷（AutoProcessCMD_Finished 的 JSON 数据）

struct S_AutoProcessCMDFinishedData
{
    int status = 0;           // 0=正常完成 1=异常失败
    double elapsedMs = 0.0;   // 整场任务总耗时（ms，Start 到 Finish）
    YLT_REFL(S_AutoProcessCMDFinishedData, status, elapsedMs);
};

struct S_AutoProcessCurProcessData
{
    int processId;
    double posX = 0.0;
    double posY = 0.0;      // 操作中芯片坐标
    double elapsedMs = 0;      // 运行耗时（ms）
    YLT_REFL(S_AutoProcessCurProcessData, processId, posX, posY, elapsedMs);
};

#endif // AUTOPROCESSPROTOCOL_H
