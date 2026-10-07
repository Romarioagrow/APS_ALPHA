#pragma once
#include "APSPlanetSurfaceProfile.h"

namespace APSPlanetScatterMaterial
{
inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/ScatterMaterial20260930V1");
inline constexpr const TCHAR* Shapes[] = {TEXT("Pebble"), TEXT("RockA"), TEXT("RockB"), TEXT("SlabA"), TEXT("SlabB")};
inline FString Path(int32 Index)
{
    const FString Name = FString(TEXT("MI_APS_Scatter_")) + Shapes[Index];
    return FString(Folder) / Name + TEXT(".") + Name;
}
inline FLinearColor Tint(EPlanetType Type)
{
    // Low-chroma exposed minerals, not an artificial snow layer or emissive ore.
    switch (Type)
    {
    case EPlanetType::Ice: case EPlanetType::Frozen: case EPlanetType::Rogue:
    case EPlanetType::Nordic: case EPlanetType::Tundra: return FLinearColor(.16f,.18f,.20f);
    case EPlanetType::Desert: case EPlanetType::Sand: return FLinearColor(.19f,.13f,.08f);
    case EPlanetType::Basalt: case EPlanetType::Volcanic: case EPlanetType::Lava:
    case EPlanetType::Melted: return FLinearColor(.035f,.04f,.044f);
    default: return FLinearColor(.095f,.082f,.070f);
    }
}
}
