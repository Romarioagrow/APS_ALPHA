#pragma once

#include "WorldScapeRoot.h"

namespace APSWorldScapeLiquidLattice
{
    inline constexpr int32 DefaultTerrainResolution = 256;

    // Extend detailed rings; never change the seed, height coefficients,
    // finest 1.2 m ground spacing or collision density. WorldScape rings
    // require a multiple of four. The explicit ceiling bounds generation cost.
    inline int32 TerrainResolution(bool bOwned, bool bScaledPreview, int32 Requested)
    {
        if (bScaledPreview) return 48;
        if (!bOwned) return 96;
        return (FMath::Clamp(Requested, 96, 256) + 3) & ~3;
    }

    // Refine both sides of the shore together. Do not change the noise field,
    // altitude anchor, finest sample spacing, collision mesh or active root's
    // topology in flight. 256 costs ~1.78x the vertices of 192, not 4x (384).
    // Keep dry/authored/preview worlds on their established budget.
    inline int32 CoastResolution(int32 Base, bool bOwned, bool bScaledPreview,
        bool bHasLiquid, int32 Requested)
    {
        if (!bOwned || bScaledPreview || !bHasLiquid || Requested <= 0) return Base;
        const int32 Bounded = (FMath::Clamp(Requested, 96, 256) + 3) & ~3;
        return FMath::Max(Base, Bounded);
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
