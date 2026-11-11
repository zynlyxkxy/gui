#ifndef BLUEFILMCHIPHANDOVERPROTOCOL_H
#define BLUEFILMCHIPHANDOVERPROTOCOL_H

#include <vector>
#include "istar_serializer.hpp"

enum BlueFilmChipHandoverCMD
{
    BlueFilmChipHandoverCMD_None = 0,
    BlueFilmChipHandoverCMD_GetAllChips,  // GUI请求
    BlueFilmChipHandoverCMD_AllChipsRes,  // 响应
    BlueFilmChipHandoverCMD_CmdRejected,   // 命令被拒
};

/// 命令被拒载荷
struct S_BlueFilmChipHandoverCMD_CmdRejected
{
    int command = 0;           // 被拒绝的命令值
    std::string reason;        // 拒绝原因
    YLT_REFL(S_BlueFilmChipHandoverCMD_CmdRejected, command, reason);
};

struct ChipPosition
{
    double posX = 0.0;
    double posY = 0.0;
    int  status = 0;
    YLT_REFL(ChipPosition, posX, posY, status);
};

/// 蓝膜芯片数据状态：随 AllChipsRes 下发，GUI 据此区分"还没有数据"与真错误。
/// NotReady = 本机还没有任何快照（蓝膜台从未上报 map_data/pick_data）—— 正常状态，不该报错；
/// Ready    = 快照已就绪（chipPos 可能为空，表示真的一颗都没有）。
enum BlueFilmChipDataState
{
    BlueFilmChipDataState_NotReady = 0,
    BlueFilmChipDataState_Ready = 1,
};

struct S_BlueFilmChipPositionData
{
    std::vector<ChipPosition> chipPos;
    // 缺省 Ready：老端不下发本字段时按"有数据"处理，不会把真实点表误判成未就绪而清掉
    int state = static_cast<int>(BlueFilmChipDataState_Ready);
    YLT_REFL(S_BlueFilmChipPositionData, chipPos, state);
};

struct S_BlueFilmChipHandoverCMDFinishedData
{
    int status = 0;           // 0=正常完成 1=异常失败
    double startTime = 0.0;      // 运行启动时间（ms）
    double endTime = 0.0;        // 运行结束时间（ms）
    double elapsedMs = 0.0;   // 整场任务总耗时（ms，Start 到 Finish）
    YLT_REFL(S_BlueFilmChipHandoverCMDFinishedData, status, elapsedMs);
};

struct S_BlueFilmChipHandoverData
{
    bool valid = false;
    S_BlueFilmChipPositionData posData;
    S_BlueFilmChipHandoverCMDFinishedData res;
    YLT_REFL(S_BlueFilmChipHandoverData, valid, posData, res);
};

#endif // BLUEFILMCHIPHANDOVERPROTOCOL_H
