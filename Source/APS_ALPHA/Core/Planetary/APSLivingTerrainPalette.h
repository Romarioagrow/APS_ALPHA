#pragma once
#include "APSPlanetSurfaceProfile.h"

namespace APSLivingTerrainPalette
{
    inline bool IsFiniteColor(const FLinearColor& Color)
    {
        return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G)
            && FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A);
    }

    inline bool Allows(const FAPSResolvedPlanetSurfaceProfile& Profile)
    {
        return Profile.LiquidType == EAPSPlanetLiquidType::Water
            && (Profile.Archetype == EAPSPlanetSurfaceArchetype::Temperate
                || Profile.Archetype == EAPSPlanetSurfaceArchetype::Oceanic
                || Profile.Archetype == EAPSPlanetSurfaceArchetype::Biosphere);
    }

    // Native MF_MidLayerBlock interpolates Color4/Color5 with its existing
    // temperature field. Binding both to Dryland removed that variation.
    // Restore a restrained cooler dryland endpoint using only authored colours;
    // keep hot dryland, snow, wet greens and every coverage/height mask unchanged.
    inline FLinearColor CoolDryland(const FAPSResolvedPlanetSurfaceProfile& Profile, float Strength)
    {
        const auto& P = Profile.Palette;
        if (!Allows(Profile) || !FMath::IsFinite(Strength) || !IsFiniteColor(P.Highland)
            || !IsFiniteColor(P.Dryland) || Strength <= 0.0f) return P.Dryland;
        const float Blend = 0.5f * FMath::Clamp(Strength, 0.0f, 1.0f);
        FLinearColor Result = FMath::Lerp(P.Dryland, P.Highland, Blend);
        Result.A = P.Dryland.A;
        return Result;
    }

    // The dry rocky branch and exposed top-layer rock must not reuse the grassier
    // highland tint verbatim. Stay inside the profile's authored stone palette;
    // preserve vegetation Color3 and the separate Peak/snow endpoint.
    inline FLinearColor ExposedRock(const FAPSResolvedPlanetSurfaceProfile& Profile)
    {
        const auto& P = Profile.Palette;
        if (!Allows(Profile) || !IsFiniteColor(P.Highland) || !IsFiniteColor(P.Slope)) return P.Highland;
        FLinearColor Result = FMath::Lerp(P.Highland, P.Slope, 0.7f);
        Result.A = P.Highland.A;
        return Result;
    }
}
