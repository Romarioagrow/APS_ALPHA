#pragma once

#include "CoreMinimal.h"

namespace APSPlanetSurfaceRadius
{
    // RadiusKM is the physical model; PlanetRadiusKM is its legacy whole-km
    // cache. Taking max() lets a later floor -> round cache refresh move terrain
    // by up to 500 m and invalidate an otherwise unchanged live profile.
    inline double Kilometres(double ModelRadiusKm, int32 LegacyRadiusKm)
    {
        return FMath::IsFinite(ModelRadiusKm) && ModelRadiusKm > 0.0
            ? ModelRadiusKm : FMath::Max(0.0, static_cast<double>(LegacyRadiusKm));
    }
}
