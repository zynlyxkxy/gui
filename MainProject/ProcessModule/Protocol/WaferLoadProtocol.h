#ifndef WAFERLOADPROTOCOL_H
#define WAFERLOADPROTOCOL_H

enum WaferLoadCMD
{
    WaferLoadCMD_None = 0,
    // ---- 请求（GUI → 流程）----
    WaferLoadCMD_ManualLoadFinish,       // 人工上片完成
    // ---- 响应（流程 → GUI）----
    WaferLoadCMD_MoveToLoadingPosFinish

};

#endif // WAFERLOADPROTOCOL_H
