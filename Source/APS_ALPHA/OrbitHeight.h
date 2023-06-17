#pragma once
UENUM(BlueprintType)
enum class EOrbitHeight : uint8
{
    UpperAtmosphere     UMETA(DisplayName = "Upper Atmosphere"),
    LowOrbit            UMETA(DisplayName = "Low Orbit"),
    Geostationary       UMETA(DisplayName = "Geostationary Orbit"),
    HighOrbit           UMETA(DisplayName = "High Orbit"),
    VeryHighOrbit       UMETA(DisplayName = "Very High Orbit"),
};