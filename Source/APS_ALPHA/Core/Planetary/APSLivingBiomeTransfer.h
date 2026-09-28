#pragma once
#include "APSLivingTerrainPalette.h"

// Semantic vertex channels only. Do not change Data.Height, coast geometry,
// collision, seed, or the shared world-to-planet coordinate frame.
namespace APSLivingBiomeTransfer
{
    // Native MF_LayerAdjustment: saturate((1+2*c)*(R-shift-variation)-c)^2.
    // The inherited shift -.509154 made R=.08 (sea datum) already full inland.
    // Keep coast sand through the datum, then blend inland across a narrow band.
    inline constexpr float CoastShift = -0.40f;
    inline constexpr float CoastContrast = 12.0f;
    inline constexpr float HumidLandShift = -0.18f;

    inline double Height(double AboveSea, double LandSpan)
    {
        const double Ratio = FMath::Max(0.0, AboveSea) / FMath::Max(LandSpan, 0.001);
        // The old r/(1+r) shoulder put the whole normal land span below .47;
        // its upper layers could never see the intended .08.. .86 range.
        return 0.08 + 0.78 * FMath::Clamp(Ratio, 0.0, 1.0);
    }

    inline double Humidity(double Original, double RegionalQuantile, double Latitude,
        double OceanInfluence)
    {
        // Reuse the existing seeded humidity sample, remapped to an area quantile.
        // Small centred fBm values previously supplied only a few percent of
        // variation: wet grass won virtually everywhere on a temperate continent.
        const double AbsLatitude = FMath::Abs(FMath::Clamp(Latitude, -1.0, 1.0));
        const double Subtropical = FMath::SmoothStep(0.15, 0.35, AbsLatitude)
            * (1.0 - FMath::SmoothStep(0.55, 0.75, AbsLatitude));
        const double Inland = 1.0 - FMath::Clamp(OceanInfluence, 0.0, 1.0);
        return FMath::Clamp(Original + (FMath::Clamp(RegionalQuantile, 0.0, 1.0) - 0.5)
            * 0.80 - Subtropical * Inland * 0.18, 0.0, 1.0);
    }
}
