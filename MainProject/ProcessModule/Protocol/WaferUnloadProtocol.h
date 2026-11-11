#ifndef WAFERUNLOADPROTOCOL_H
#define WAFERUNLOADPROTOCOL_H

enum WaferUnLoadCMD
{
    WaferUnLoadCMD_None = 0,
    // ---- 请求（GUI → 流程）----
    WaferUnLoadCMD_ManualUnLoadFinish,       // 人工下片完成
    // ---- 响应（流程 → GUI）----
    WaferUnLoadCMD_MoveToUnLoadingPosFinish

};

#endif // WAFERUNLOADPROTOCOL_H
