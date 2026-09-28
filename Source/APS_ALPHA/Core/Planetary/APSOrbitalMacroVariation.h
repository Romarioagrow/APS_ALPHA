#pragma once
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

namespace APSOrbitalMacroVariation
{
    // Expand only after rendered family coverage; never globally retune Magma,
    // frozen worlds or authored reference planets from a terrestrial comparison.
    inline bool Allows(EPlanetType Type)
    {
        return Type == EPlanetType::Terrestrial || Type == EPlanetType::Oasis;
    }
}
