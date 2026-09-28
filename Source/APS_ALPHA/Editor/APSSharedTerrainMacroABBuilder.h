#pragma once

#if WITH_EDITOR
class IAssetTools;
namespace APSSharedTerrainMacroABBuilder
{
    // Creates isolated diagnostic assets. Does not change runtime selectors.
    bool Build(IAssetTools& Tools);
    // Offline, hash-guarded installation of the rendered V2 function only.
    bool Promote(IAssetTools& Tools);
}
#endif
