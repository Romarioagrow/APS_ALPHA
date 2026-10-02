#pragma once
#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// An explicit editor automation experiment, never a new shipping/default path.
// Preparation, creation and the observer must resolve the same exact pair.
namespace APSUnifiedLavaAssets
{
inline bool DetailCandidate()
{
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
    return FParse::Param(FCommandLine::Get(), TEXT("APSUnifiedLavaDetailCandidate"));
#else
    return false;
#endif
}
inline const TCHAR* TemplatePath()
{
    return DetailCandidate()
        ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface")
        : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
}
inline const TCHAR* MasterPath()
{
    return DetailCandidate()
        ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface")
        : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");
}
inline float DetailStrength()
{
    float Value = 1.0f;
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
    if (DetailCandidate()) FParse::Value(FCommandLine::Get(), TEXT("APSUnifiedLavaDetailStrength="), Value);
#endif
    return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, 1.0f) : 1.0f;
}
}
