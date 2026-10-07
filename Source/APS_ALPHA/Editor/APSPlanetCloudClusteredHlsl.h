#pragma once
#include "APSPlanetCloudRefinedHlsl.h"

// V32 is an isolated shape experiment built on V31. V27, V30 and V31 keep
// their existing fields. This constructor does not select a runtime material.
namespace APSPlanetCloudClusteredHlsl
{
inline FString Code()
{
    FString Shader = APSPlanetCloudRefinedHlsl::Code();
    if (Shader.IsEmpty()) return FString();
    const auto ReplaceOne = [&Shader](const TCHAR* Before, const TCHAR* After)
    {
        return Shader.ReplaceInline(Before, After, ESearchCase::CaseSensitive) == 1;
    };
    if (!ReplaceOne(
        TEXT("float Meso=F.noise(F.rotateDomain(WeatherN*(32./max(WeatherScale,.25)))+SeedOffset);"),
        TEXT(R"HLSL(
// Reuse the existing meso evaluation to organize the unchanged kilometre
// billows into groups. The lattice spacing is 24 km at WeatherScale=1,
// independent of planet radius; the existing seed and wind frame are retained.
float MesoFrequency=BillowFrequencyScale/24.;
float MesoResolved=1.-smoothstep(.25,.5,Footprint*MesoFrequency);
float Meso=.5;
if(MesoResolved>.001)
    Meso+=MesoResolved*(F.noise(F.rotateDomain(
        WeatherN*(max(Radius,1.)*MesoFrequency))+SeedOffset)-.5);
)HLSL"))) return FString();
    if (!ReplaceOne(
        TEXT("float WeatherBody=saturate(Shape+.22*(Meso-.5));"),
        TEXT(R"HLSL(
// Center the cluster modulation around the existing weather envelope. This
// changes the spatial field and opacity; centering does NOT preserve mean
// thresholded density. Fully unresolved meso converges to the central envelope,
// not an integral over its lost variance. Both light taps keep the existing
// frozen local weather envelope, an approximation across an 8 km light path.
float WeatherBody=saturate(Shape+.65*(smoothstep(.32,.68,Meso)-.5));
)HLSL"))) return FString();
    return Shader;
}
}
