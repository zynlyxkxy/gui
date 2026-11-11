#ifndef S_SILICONSTAGE_H
#define S_SILICONSTAGE_H

#include "istar_ylt.hpp"
#include "S_MCCommon.h"

struct S_SiliconStage {
    S_MCCommon_Axis siliconStageX;
    S_MCCommon_Axis siliconStageY;
    S_MCCommon_Axis siliconStageRz;
    S_MCCommon_Axis epinZ;

    double chuckDiameter = 0.0;    // 下吸盘直径
    YLT_REFL(S_SiliconStage, siliconStageX, siliconStageY, siliconStageRz, epinZ,
             chuckDiameter);
};

#endif // S_SILICONSTAGE_H
