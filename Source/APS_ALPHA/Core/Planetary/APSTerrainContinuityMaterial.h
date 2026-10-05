#pragma once

#include "APSPlanetSurfaceMaterialPolicy.h"
// The original terrain is mandatory, not a switchable near/orbit presentation.
// Broad routing does not by itself prove visual acceptance of every family.
namespace APSTerrainContinuityMaterial
{
    inline constexpr const TCHAR* MasterPath = APSPlanetSurfaceMaterialPolicy::ContinuousMasterPath;
    inline constexpr const TCHAR* TemplatePath = APSPlanetSurfaceMaterialPolicy::ContinuousTemplatePath;
    inline bool Allows(EPlanetType Type)
    {
        return APSPlanetSurfaceMaterialPolicy::AllowsContinuousTerrain(Type, true);
    }
    inline bool Enabled()
    {
        return true;
    }
}
