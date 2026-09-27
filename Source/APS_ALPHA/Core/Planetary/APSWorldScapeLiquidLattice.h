#pragma once

#include "WorldScapeRoot.h"

namespace APSWorldScapeLiquidLattice
{
    // Sample the same height field more densely; never change the seed, height
    // coefficients, 1.2 m ground spacing or collision density. WorldScape rings
    // require a multiple of four. The explicit ceiling bounds generation cost.
    inline int32 TerrainResolution(bool bOwned, bool bScaledPreview, int32 Requested)
    {
        if (bScaledPreview) return 48;
        if (!bOwned) return 96;
        return (FMath::Clamp(Requested, 96, 192) + 3) & ~3;
    }

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
