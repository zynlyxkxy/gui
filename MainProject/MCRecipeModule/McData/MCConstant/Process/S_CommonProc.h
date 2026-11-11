/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* @FileName: S_CommonProc.h
* @Description: 流程公共机器常数结构体：各业务流程共用的机械基准数据
*               （吸盘中心/最佳焦面/最佳焦面对应共焦值），装机标定得到，
*               由 S_Process 统一组织，流程在 setMCandRE 中取 mc.process.commonProc。
*
* 物理布局：上方键合头（RxRyZ），中间对准台（XYZ，上下各一个相机 + 各一个光谱共焦），
*           下方硅片台（XYRz）。
* 对焦约定（XY 由上往下配，焦距由能动的台子配）：
*   - 键合头 XY 无法移动 → 对准台 XY 匹配上吸盘中心（upChuckCenterAlignTableXY）；
*   - 硅片台 Z 无法调整   → 对准台 Z 匹配下吸盘/硅片台焦距（downChuckBestFocusAlignTableZ）；
*   - 键合头 RxRyZ 由自身调整匹配上吸盘最佳焦面（upChuckBestFocusBondHeadRxRyZ）；
*   - 硅片台中心对准台下相机中心（downChuckCenterWaferTableXYRz），
*     用于硅片台把中心送到对准台下相机视场；
*   - 两个共焦值为对应相机最佳焦面高度下，把该共焦传感器对准吸盘中心测得的读数
*     （下共焦对下吸盘中心、上共焦对上吸盘中心），作为流程 Z 补偿收敛基准。
*********************************************************************************/
#ifndef S_COMMON_PROC_H
#define S_COMMON_PROC_H

#include "istar_ylt.hpp"
#include "istar_basic_value_point.h"

using namespace ISTAR::BasicPlatform::BasicValue;

struct S_CommonProc
{
    double downCamBestFocusCfValue = 0.0;           // 对准台下相机最佳焦面高度下，下光谱共焦拍下吸盘中心时的读数
    double upCamBestFocusCfValue = 0.0;             // 对准台上相机最佳焦面高度下，上光谱共焦拍上吸盘中心时的读数
    S_Point2D upChuckCenterAlignTableXY;            // 上吸盘中心位置时，对准台XY
    double downChuckBestFocusAlignTableZ = 0.0;     // 下吸盘最佳焦面时，对准台Z向位置
    S_RxRyZ upChuckBestFocusBondHeadRxRyZ;          // 上吸盘最佳焦面位置时，键合头RxRyZ
    S_XYRz downChuckCenterWaferTableXYRz;           // 硅片台中心对准台下相机中心时，硅片台XYRz
    YLT_REFL(S_CommonProc,
             downCamBestFocusCfValue, upCamBestFocusCfValue,
             upChuckCenterAlignTableXY, downChuckBestFocusAlignTableZ,
             upChuckBestFocusBondHeadRxRyZ, downChuckCenterWaferTableXYRz);
};

#endif // S_COMMON_PROC_H
