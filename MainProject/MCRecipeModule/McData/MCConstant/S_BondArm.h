#ifndef S_BONDARM_H
#define S_BONDARM_H

#include "istar_ylt.hpp"
#include "S_MCCommon.h"

struct S_BondArm {
    S_MCCommon_Axis bondHeadRx;
    S_MCCommon_Axis bondHeadRy;
    S_MCCommon_Axis bondHeadZ;

    double chuckLength = 0.0;    // 吸盘长（mm）
    double chuckWidth  = 0.0;    // 吸盘宽（mm）
    YLT_REFL(S_BondArm, bondHeadRx, bondHeadRy, bondHeadZ, chuckLength, chuckWidth);
};


#endif // S_BONDARM_H
