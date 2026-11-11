#ifndef BLUEFILMUNLOADPROTOCOL_H
#define BLUEFILMUNLOADPROTOCOL_H

enum BlueFilmUnLoadCMD
{
    BlueFilmUnLoadCMD_None = 0,
    // ---- 请求（GUI → 流程）----

    // ---- 响应（流程 → GUI）----
    BlueFilmUnLoadCMD_MoveToUnLoadingPosFinish

};

#endif // BLUEFILMUNLOADPROTOCOL_H
