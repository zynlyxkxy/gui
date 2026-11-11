#ifndef S_CHIPTRANSFER_H
#define S_CHIPTRANSFER_H

#include "istar_ylt.hpp"
#include "S_MCCommon.h"

struct S_ChipTransfer {
    S_MCCommon_Axis punchPinZ;
    S_MCCommon_Axis filpModuleX;
    S_MCCommon_Axis filpModuleZ;
    S_MCCommon_Axis filpModuleRx;
    S_MCCommon_Axis filpModuleRz;

    YLT_REFL(S_ChipTransfer, punchPinZ, filpModuleX, filpModuleZ,
             filpModuleRx, filpModuleRz);
};

#endif // S_CHIPTRANSFER_H
