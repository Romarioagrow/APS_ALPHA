#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

namespace APSAtmosphereControlBounds
{
    inline bool IsGasGiant(const EPlanetType Type)
    {
        return Type == EPlanetType::GasGiant || Type == EPlanetType::HotGiant
            || Type == EPlanetType::IceGiant;
    }

    inline double HeightMaximum(const EPlanetType Type)
    {
        // Generated height is radius / 30: the supported 200000 km body needs
        // 6666.67 km of atmosphere, including float conversion in AtmoScape.
        return IsGasGiant(Type) ? 7000.0 : 2000.0;
    }

    inline double RayleighMaximum(const EPlanetType Type)
    {
        return IsGasGiant(Type) ? 80.0 : 64.0;
    }

    inline double Clamp(const double Value, const EPlanetType Type, const double Maximum)
    {
        // Keep the existing non-gas clamp behavior, including legacy inputs.
        const double SafeValue = IsGasGiant(Type) && !FMath::IsFinite(Value) ? 0.0 : Value;
        return FMath::Clamp(SafeValue, 0.0, Maximum);
    }

    inline double Height(const double Value, const EPlanetType Type)
    {
        return Clamp(Value, Type, HeightMaximum(Type));
    }

    inline double Rayleigh(const double Value, const EPlanetType Type)
    {
        return Clamp(Value, Type, RayleighMaximum(Type));
    }
}
