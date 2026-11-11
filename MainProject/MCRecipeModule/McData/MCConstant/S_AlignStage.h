#ifndef S_ALIGNSTAGE_H
#define S_ALIGNSTAGE_H

#include "istar_ylt.hpp"
#include "S_MCCommon.h"
#include "istar_basic_value_point.h"

using namespace ISTAR::BasicPlatform::BasicValue;

struct S_ChromaticConfocal{
    int cfId = -1;               // 共焦传感器通道号（-1 = 未配置/无效，勿删默认值）
    int cfTimeOutMs = 30000;         // 共焦无结果超时（ms，MC 常量必填）
    S_Point2D cfToCamOffsetXY;       // 对准台光谱共焦相对于相机XY偏移
    YLT_REFL(S_ChromaticConfocal, cfId, cfTimeOutMs, cfToCamOffsetXY);
};

struct S_FineAlignCamera{
    int cameraId = -1;              // 相机 id（-1 = 未配置/无效，勿删默认值：子流程靠它判断相机是否启用）
    int visionTimeOutMs = 30000;    // 视觉识别无结果超时（ms，MC 常量）
    // 像素→光栅换算已改走 iStar 标定库（VisionManager::PixelToRuler），
    // 分辨率/像元/倍率三件套随之移除，标定数据见 CameraConfig
    YLT_REFL(S_FineAlignCamera, cameraId, visionTimeOutMs);
};

struct S_AlignStage {
    S_MCCommon_Axis alignStageX;
    S_MCCommon_Axis alignStageY;
    S_MCCommon_Axis alignStageZ;

    S_FineAlignCamera upFineAlignCamera;
    S_FineAlignCamera downFineAlignCamera;
    S_ChromaticConfocal upChromaticConfocal;
    S_ChromaticConfocal downChromaticConfocal;

    YLT_REFL(S_AlignStage, alignStageX, alignStageY, alignStageZ,
             upFineAlignCamera, downFineAlignCamera,
             upChromaticConfocal, downChromaticConfocal);
};


#endif // S_ALIGNSTAGE_H
