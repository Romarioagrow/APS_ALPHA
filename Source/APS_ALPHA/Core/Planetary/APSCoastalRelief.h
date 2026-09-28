#pragma once
#include "APSPlanetSurfaceProfile.h"

// Candidate only: no second surface, LOD-dependent heights or mutable worker CVar.
// Retain broad coastal bays, but prevent metre/kilometre relief from punching a
// dense archipelago through an otherwise continuous continental shoreline.
namespace APSCoastalRelief
{
    inline bool Allows(const FAPSResolvedPlanetSurfaceProfile& P)
    {
        return P.LiquidType == EAPSPlanetLiquidType::Water
            && P.LandCoverage > 0.005f && P.LandCoverage < 0.995f
            && (P.PlanetType == EPlanetType::Terrestrial
                || P.PlanetType == EPlanetType::Water || P.PlanetType == EPlanetType::Oasis);
    }

    inline double Height(double Macro, double RegionalDelta, double Detailed, double Sea, double SignedLand)
    {
        const double Distance = FMath::Abs(SignedLand);
        if (Distance >= 0.12) return Detailed; // bit-exact interior/deep-ocean path
        const double Base = Macro + RegionalDelta * 0.35;
        const double Delta = Detailed - Base;
        const double Budget = FMath::Abs(Base - Sea) * 0.65;
        // Smooth saturation (no clipped terraces). At the broad shoreline the
        // detail vanishes continuously; on either side it cannot reverse the sign.
        const double Denominator = FMath::Sqrt(Budget * Budget + Delta * Delta);
        const double Limited = Denominator > 0.0 ? Budget * Delta / Denominator : 0.0;
        const double Coast = Base + Limited;
        if (Distance <= 0.025) return Coast;
        return FMath::Lerp(Coast, Detailed, FMath::SmoothStep(0.025, 0.12, Distance));
    }
}
