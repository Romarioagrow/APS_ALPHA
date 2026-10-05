#pragma once
#include "APSPlanetCloudHlsl.h"

// V31 is a NEW single-layer diagnostic package. Keep the V27 source/asset and
// the separate V30 layered candidate intact for same-field comparisons.
namespace APSPlanetCloudRefinedHlsl
{
inline FString Code()
{
    FString Shader = APSPlanetCloudHlsl::Code();
    const auto ReplaceOne = [&Shader](const TCHAR* Before, const TCHAR* After)
    {
        // Fail closed if the shared shader changes; never bake a partly patched
        // candidate under a new version name and mistake it for this experiment.
        return Shader.ReplaceInline(Before, After, ESearchCase::CaseSensitive) == 1;
    };
    if (!ReplaceOne(
        TEXT("float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));"),
        TEXT(R"HLSL(
float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));
// Reduce white screen-space integration grain ONLY after both physical noise
// octaves are fully pixel-filtered. Use the start of the whole shell interval,
// not the jittered sample, so every step retains the same continuous phase.
// .32 is the coarsest octave: full filtering starts at .5/.32 = 1.5625 km
// in its scaled domain. Leave a margin and blend rather than switching at a LOD.
float PixelFootprintAtEntry=RayFootprint*Start*BillowFrequencyScale;
float FarPhaseBlend=smoothstep(1.6,3.2,PixelFootprintAtEntry);
float StablePhase=lerp(SamplePhase,.5+.2*(SamplePhase-.5),FarPhaseBlend);
)HLSL"))) return FString();
    if (!ReplaceOne(TEXT("float SampleDistance=NextStart+SamplePhase*Step;"),
        TEXT("float SampleDistance=NextStart+StablePhase*Step;"))) return FString();
    if (!ReplaceOne(
        TEXT("float LD=F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,Footprint,LT,BillowFrequencyScale);"),
        TEXT(R"HLSL(
// Two sunlight samples represent half the light segment each. Filter that
// footprint too, not just the much smaller nearby camera pixel. Same field,
// same two light taps; no additional noise octaves or view-march samples.
float LightFootprint=max(Footprint,LightLength*.25);
float LD=F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,LightFootprint,LT,BillowFrequencyScale);
)HLSL"))) return FString();
    return Shader;
}
}
