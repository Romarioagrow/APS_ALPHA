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

    // Rio 06.10 (Krathys: "the atmosphere is cut off with a hard outline"): a thin moon shell (H = R/30, Hr 4.1-8 km)
    // still holds a tenth of its density at the shell top, where the plugin's math stops dead. Every moon but the
    // accepted Volcanic ones takes the tail master; in gameplay the tail's weight follows ThinShellWeight (0 there is
    // the native math exactly), and the menu preview keeps 0. Planets are unchanged.
    inline constexpr bool ManagedFor(EPlanetType Type) { return EnabledFor(Type); }
    inline constexpr bool ManagedFor(EMoonType Type) { return Type != EMoonType::Volcanic; }
    /** 1 on a thin shell (height under 8 Rayleigh heights), 0 from 12 up (exactly the native look). */
    inline float ThinShellWeight(float HeightKm, float RayleighHeightKm)
    {
        return 1.0f - FMath::SmoothStep(8.0f, 12.0f, HeightKm / FMath::Max(RayleighHeightKm, 0.001f));
    }
}
