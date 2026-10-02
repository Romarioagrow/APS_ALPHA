#pragma once

#include "APSPlanetSurfaceProfile.h"

// Shared by the offline palette bake and the explicit fresh-root prototype.
// No UPROPERTY/save-schema change; the actual profile remains untouched.
namespace APSPlanetFoliagePrototype
{
    inline constexpr const TCHAR* Root = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/FoliagePrototype20260929V1");
    enum class EMesh : uint8 { SmallRock, Rock, TemperateGrass, ColdGrass, DryGrass, Tree, Count };
    struct FRecipe
    {
        EMesh Mesh = EMesh::SmallRock;
        float HeightCm = 70;
        float Attempts = 4;
        bool bBiological = false;
    };

    inline FRecipe Recipe(EPlanetType Type)
    {
        switch (Type)
        {
        case EPlanetType::Terrestrial: case EPlanetType::Pangea: case EPlanetType::SuperEarth:
            return {EMesh::TemperateGrass, 35, 12, true};
        case EPlanetType::Nordic: case EPlanetType::Tundra:
            return {EMesh::ColdGrass, 25, 8, true};
        case EPlanetType::Forest:
            return {EMesh::Tree, 450, 2, true};
        case EPlanetType::Savanna: case EPlanetType::Oasis:
            return {EMesh::DryGrass, 40, 8, true};
        case EPlanetType::Ocean: case EPlanetType::Water: case EPlanetType::Archipelago:
            return {EMesh::SmallRock, 45, 4, false}; // Exposed shore only, never ocean-floor grass.
        case EPlanetType::HighMountain: case EPlanetType::Volcanic: case EPlanetType::Melted:
        case EPlanetType::Lava: case EPlanetType::Basalt:
            return {EMesh::Rock, 130, 3, false};
        case EPlanetType::Ice: case EPlanetType::Frozen: case EPlanetType::Rogue:
            return {EMesh::Rock, 100, 3, false}; // Temporary silhouette, not a finished ice asset.
        case EPlanetType::Crystal:
            return {EMesh::Rock, 180, 2, false}; // Temporary outcrop until crystal art replaces it.
        default:
            return {EMesh::SmallRock, 60, 4, false};
        }
    }

    inline FAPSPlanetFoliageProfile Settings(EPlanetType Type)
    {
        FAPSPlanetFoliageProfile Result;
        if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) return Result;
        const UEnum* Types = StaticEnum<EPlanetType>();
        if (!Types) return Result;
        const FString Name = TEXT("FC_APS_Proto_") + Types->GetNameStringByValue(uint8(Type));
        Result.bEnabled = true;
        Result.Collections.Add(TSoftObjectPtr<UWorldScapeFoliagesCollection>(
            FSoftObjectPath(FString(Root) / Name + TEXT(".") + Name)));
        Result.MaxCollections = 1;
        Result.MaxTypesPerCollection = 1;
        Result.MaxInstancesPerSectorPerCollection = 16;
        Result.MaxClusterMeshesPerType = 1;
        Result.MinSectorSizeCm = 20000;
        Result.MaxCullDistanceMultiplier = 0.5f;
        Result.bUseNoiseMask = Recipe(Type).bBiological;
        Result.bCastShadows = false;
        return Result;
    }
}
