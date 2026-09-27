#pragma once

#include "WorldScapeRoot.h"

namespace APSWorldScapeLiquidLattice
{
    // A liquid's chemistry must not select a different spherical tessellation.
    // All three values participate in WorldScape's independent altitude/snap
    // calculation; matching only resolution still leaves different chord sag.
    inline bool MatchTerrain(AWorldScapeRoot& Root, bool bOwned, bool bScaledPreview)
    {
        if (!bOwned || bScaledPreview || !Root.bOcean) return false;
        Root.OceanMaxLod = Root.MaxLod;
        Root.OceanLodResolution = Root.LodResolution;
        Root.OceanTriangleSize = Root.TriangleSize;
        return true;
    }
}
