#pragma once

#include "CoreMinimal.h"

namespace APSPlanetSurfaceFill
{
    constexpr double FadeStartCm = 1000000.0; // Preserve the accepted ground/low-flight light below 10 km.
    constexpr double MaximumAltitudeCm = 5000000.0;

    inline bool IsEligible(bool bNativeSurfaceReady, bool bManagedSolidSurface)
    {
        // Illumination belongs to the body, not to its current LOD representation.
        // Keep authored ready surfaces, and light managed solids while streaming.
        return bNativeSurfaceReady || bManagedSolidSurface;
    }

    inline float Weight(double AltitudeCm)
    {
        if (!FMath::IsFinite(AltitudeCm)) return 0.0f;
        const double T = FMath::Clamp((AltitudeCm - FadeStartCm)
            / (MaximumAltitudeCm - FadeStartCm), 0.0, 1.0);
        return static_cast<float>(1.0 - T * T * (3.0 - 2.0 * T));
    }
}
