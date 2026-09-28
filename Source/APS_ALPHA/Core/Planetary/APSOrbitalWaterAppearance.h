#pragma once

#include "CoreMinimal.h"
#include "APSPlanetSurfaceProfile.h"

namespace APSOrbitalWaterAppearance
{
    // Body-class independent: a generated water moon uses the same optical
    // contract as a planet. Borrowed/authored/scaled/custom surfaces stay intact.
    inline bool IsEligible(bool bOwnedRoot, bool bManual, EAPSPlanetLiquidType Liquid,
        double PresentationScale, const FVector& RootScale)
    {
        return bOwnedRoot && !bManual && Liquid == EAPSPlanetLiquidType::Water
            && FMath::IsFinite(PresentationScale) && FMath::IsNearlyEqual(PresentationScale, 1.0)
            && !RootScale.ContainsNaN() && RootScale.Equals(FVector::OneVector, KINDA_SMALL_NUMBER);
    }

    // This is an optical response only, not a different material/surface at
    // altitude. Below 20 km every saved near-water parameter remains exact.
    inline float Weight(double HeightCm, float Strength)
    {
        if (!FMath::IsFinite(HeightCm) || !FMath::IsFinite(Strength)) return 0.0f;
        const double T = FMath::Clamp((HeightCm - 2000000.0) / 18000000.0, 0.0, 1.0);
        return static_cast<float>(T * T * (3.0 - 2.0 * T)) * FMath::Clamp(Strength, 0.0f, 1.0f);
    }

    inline float Specular(float Saved, float Blend)
    {
        // UE's dielectric specular input .25 corresponds to normal-incidence
        // reflectance .02. Keep a deliberately dimmer authored value dimmer.
        return FMath::Lerp(Saved, FMath::Min(Saved, 0.25f), Blend);
    }

    inline float Roughness(float Saved, float Blend)
    {
        // With subpixel waves faded out, retain their broad specular response
        // instead of a perfectly smooth, bright orbital mirror. Never polish
        // an intentionally rougher saved ocean or alter its palette/normal.
        return FMath::Lerp(Saved, FMath::Max(Saved, 0.26f), Blend);
    }
}
