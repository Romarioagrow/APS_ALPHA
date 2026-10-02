#pragma once
#include "CoreMinimal.h"

// Water-only candidate constants; no production selector is installed here.
namespace APSWaterSurfaceFilter
{
    inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930/M_APS_WaterSurface.M_APS_WaterSurface");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930/MI_APS_WaterSurface.MI_APS_WaterSurface");
    inline constexpr const TCHAR* PassFolder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930");
    inline constexpr const TCHAR* PassMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930/M_APS_WaterSurface.M_APS_WaterSurface");
    inline constexpr const TCHAR* PassTemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePass20260930/MI_APS_WaterSurface.MI_APS_WaterSurface");
    inline constexpr float PrimaryScaleCm = 1200.0f;
    inline constexpr const TCHAR* PreciseFolder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930");
    inline constexpr const TCHAR* PreciseMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930/M_APS_WaterSurface.M_APS_WaterSurface");
    inline constexpr const TCHAR* PreciseTemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfacePrecise20260930/MI_APS_WaterSurface.MI_APS_WaterSurface");
    inline constexpr float SecondaryScaleCm = 410.0f;
    inline constexpr const TCHAR* RelativeFolder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfaceRelative20260930");
    inline constexpr const TCHAR* RelativeMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfaceRelative20260930/M_APS_WaterSurface.M_APS_WaterSurface");
    inline constexpr const TCHAR* RelativeTemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurfaceRelative20260930/MI_APS_WaterSurface.MI_APS_WaterSurface");
    // UE's simplex gradient requires periods divisible by three. The two bands
    // have different physical periods (~12km / 6km), well outside their resolved
    // close-up footprint. Folding precedes VectorNoise's float cast. Precision
    // of the saved upstream physical frame still needs actual GPU verification.
    inline constexpr int32 PrimaryPeriod = 1023;
    inline constexpr int32 SecondaryPeriod = 1533;

    inline float Visibility(float CellsPerPixel)
    {
        if (!FMath::IsFinite(CellsPerPixel) || CellsPerPixel < 0.0f) return 0.0f;
        return 1.0f - FMath::SmoothStep(0.25f, 1.0f, CellsPerPixel);
    }
    inline const TCHAR* FilterHlsl()
    {
        // Derivatives of the UNWRAPPED LWC domain are supplied by compiler nodes.
        // Taking ddx(frac(domain)) instead would draw lines at each fold boundary.
        return TEXT("float footprint = max(length(DX), length(DY));\n")
            TEXT("float visibility = isfinite(footprint) ? 1.0 - smoothstep(0.25, 1.0, footprint) : 0.0;\n")
            TEXT("return Noise * visibility;");
    }
}
