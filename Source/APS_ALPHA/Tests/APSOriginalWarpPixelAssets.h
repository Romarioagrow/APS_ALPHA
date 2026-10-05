#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Exact immutable diagnostic output, shared with the offline original-colour
// warp-pixel bake. No production asset is changed or reparented at runtime.
namespace APSOriginalWarpPixelAssets
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003/M_APS_ContinuousWarpPixel.M_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ContinuousOriginalWarpPixel20261003/MI_APS_ContinuousWarpPixel.MI_APS_ContinuousWarpPixel");
    inline constexpr const TCHAR* SourceMasterSHA1 = TEXT("9541C8FB506D3F95E277D97C2FB1E7D626A420B0");

    inline bool Requested()
    {
        const TCHAR* Cmd = FCommandLine::Get();
        // Process-stable selection before LoadWorldSlot, not a live MID swap.
        // The existing replay and runner own private Saved/SHA/readiness guards.
        return FApp::IsUnattended() && !IsRunningCommandlet()
            && FParse::Param(Cmd, TEXT("unattended"))
            && !FParse::Param(Cmd, TEXT("NullRHI"))
            && FParse::Param(Cmd, TEXT("APSSavedPlanetReplay"))
            && FParse::Param(Cmd, TEXT("APSSavedPlanetNativeGlobeRoundTrip"))
            && FParse::Param(Cmd, TEXT("APSOriginalWarpPixelGlobe"));
    }
}
#endif
