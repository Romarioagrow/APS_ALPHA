#pragma once

#include <cmath>

// A single opaque volcanic surface, not a liquid-physics implementation. Kept
// independent of Unreal so the geometry contract can also be checked without
// launching the editor used by another task.
namespace APSWorldScapeSurfaceEnvelope
{
    inline bool Eligible(bool IsLava, bool HasOcean, bool IsManual, double PresentationScale)
    {
        return IsLava && HasOcean && !IsManual
            && std::isfinite(PresentationScale) && std::abs(PresentationScale - 1.0) <= 1.0e-9;
    }

    inline double Height(double RockHeight, double SeaHeight, bool Enabled)
    {
        // This is the visible upper envelope of the old rock + opaque lava pair.
        // It is applied by the SAME noise instance to render and collision jobs.
        if (!Enabled || !std::isfinite(RockHeight) || !std::isfinite(SeaHeight))
            return RockHeight;
        return RockHeight < SeaHeight ? SeaHeight : RockHeight;
    }
}
