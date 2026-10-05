#pragma once
#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Preparation, creation and observers always resolve the accepted original pair.
// Obsolete diagnostic flags must not substitute a different terrain graph.
namespace APSUnifiedLavaAssets
{
inline bool DetailCandidate()
{
    return false;
}
inline const TCHAR* TemplatePath()
{
    return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
}
inline const TCHAR* MasterPath()
{
    return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");
}
inline float DetailStrength()
{
    return 1.0f;
}
}
