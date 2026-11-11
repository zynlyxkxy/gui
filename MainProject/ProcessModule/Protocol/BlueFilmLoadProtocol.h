#ifndef BLUEFILMLOADPROTOCOL_H
#define BLUEFILMLOADPROTOCOL_H

enum BlueFilmLoadCMD
{
    BlueFilmLoadCMD_None = 0,
    // ---- 请求（GUI → 流程）----

    // ---- 响应（流程 → GUI）----
    BlueFilmLoadCMD_MoveToLoadingPosFinish

};
#endif // BLUEFILMLOADPROTOCOL_H
