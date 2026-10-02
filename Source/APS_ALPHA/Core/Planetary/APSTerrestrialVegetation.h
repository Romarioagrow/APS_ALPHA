#pragma once
#include "APSPlanetSurfaceProfile.h"

// Presentation-only vegetation budget. Keep both body AND resolved gameplay
// biomass unchanged: fleet scans/anomalies consume the latter.
namespace APSTerrestrialVegetation
{
inline constexpr float DefaultVisualDensity = .55f;
inline bool Apply(FAPSResolvedPlanetSurfaceProfile& P, bool GeneratedPlanet, int32 Mode)
{
    if (Mode != 1 || !GeneratedPlanet || P.PlanetType != EPlanetType::Terrestrial
        || P.Foliage.bEnabled || !P.Foliage.Collections.IsEmpty()
        || !FMath::IsFinite(P.Biomass) || !FMath::IsFinite(P.Biodiversity)
        || !FMath::IsFinite(P.VisualFoliageDensity)
        || P.Biomass != 0.0f || P.Biodiversity != 0.0f || P.VisualFoliageDensity != 0.0f) return false;
    // Apply after all terrain/material modifiers. Only the existing foliage
    // mask consumes this value in the noise sampler; dry/cold/water stay empty.
    P.VisualFoliageDensity = DefaultVisualDensity;
    return true;
}
}
