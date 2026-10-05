#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"

// Staged rollout of the rendered upper-density correction, not a terrain route.
// Preserve the accepted Volcanic/Theon and other unvalidated atmosphere parents.
namespace APSAtmosphereTailMaterial
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Atmosphere");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Atmosphere/M_APS_AtmosphereTail.M_APS_AtmosphereTail");
    inline constexpr const TCHAR* TailParameter = TEXT("APS_AtmosphereTail");
    inline constexpr bool EnabledFor(EPlanetType Type) { return Type == EPlanetType::Frozen; }
    inline constexpr bool EnabledFor(EMoonType Type) { return Type == EMoonType::Icy; }
}
