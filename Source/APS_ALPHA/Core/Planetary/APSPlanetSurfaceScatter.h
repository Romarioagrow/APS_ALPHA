#pragma once
#include "APSPlanetSurfaceProfile.h"
#include "APSTerrestrialVegetation.h"

// Replaceable, five-silhouette palettes. No profile/save mutation. Automatic
// admission is limited to the exact material/habitat combinations reviewed.
namespace APSPlanetSurfaceScatter
{
inline constexpr const TCHAR* Root = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/SurfaceScatter20260930V2");
enum class EMesh : uint8 { Pebble, RockA, RockB, SlabA, SlabB, Grass, ColdGrass, DryGrass, TreeA, TreeB, Count };
struct FSpecies
{
    EMesh Mesh;
    float HeightCm;
    int32 Attempts;
    float SectorCm;
    bool Biological;
};
struct FPalette { FSpecies Species[5]; };

inline double GroundOffsetCm(double MeshBottomZ, double MeshHeight, double MinScale, double MaxScale, bool Biological)
{
    // WorldScape applies UWorldScapeFoliagesAsset::Offset in world centimetres,
    // WITHOUT the sampled mesh scale (its Blueprint branch differs). Choose
    // the lower pivot across the scale range so no variant floats above ground.
    return -FMath::Max(MeshBottomZ * MinScale, MeshBottomZ * MaxScale)
        - MeshHeight * MinScale * (Biological ? .01 : .08);
}

inline FPalette Recipe(EPlanetType Type)
{
    // Five DIFFERENT source geometries, not five scales/rotations of one rock.
    FPalette P{{{EMesh::Pebble, 22, 12, 8000, false},
        {EMesh::RockA, 55, 8, 8000, false}, {EMesh::RockB, 95, 5, 8000, false},
        {EMesh::SlabA, 140, 3, 12000, false}, {EMesh::SlabB, 190, 2, 16000, false}}};
    switch (Type)
    {
    case EPlanetType::Terrestrial: case EPlanetType::Pangea: case EPlanetType::SuperEarth:
        P.Species[3] = {EMesh::Grass, 30, 20, 8000, true};
        P.Species[4] = {EMesh::TreeA, 450, 4, 16000, true}; break;
    case EPlanetType::Forest:
        P.Species[2] = {EMesh::Grass, 40, 20, 8000, true};
        P.Species[3] = {EMesh::TreeA, 550, 6, 16000, true};
        P.Species[4] = {EMesh::TreeB, 700, 4, 18000, true}; break;
    case EPlanetType::Tundra: case EPlanetType::Nordic:
        P.Species[4] = {EMesh::ColdGrass, 22, 16, 8000, true}; break;
    case EPlanetType::Oasis: case EPlanetType::Savanna:
        P.Species[3] = {EMesh::DryGrass, 45, 20, 8000, true};
        P.Species[4] = {EMesh::TreeA, 350, 3, 18000, true}; break;
    case EPlanetType::Desert: case EPlanetType::Sand:
        P.Species[3].HeightCm = 85; P.Species[4].HeightCm = 135; break;
    case EPlanetType::Ice: case EPlanetType::Frozen: case EPlanetType::Rogue:
        // Exposed rock/debris, not a claim that the current assets are ice art.
        P.Species[0].HeightCm = 15; P.Species[3].HeightCm = 110; break;
    case EPlanetType::Ocean: case EPlanetType::Water: case EPlanetType::Archipelago:
        for (auto& S : P.Species) S.HeightCm *= .7f; break;
    case EPlanetType::HighMountain: case EPlanetType::Basalt: case EPlanetType::Volcanic:
    case EPlanetType::Lava: case EPlanetType::Melted:
        P.Species[3].HeightCm = 180; P.Species[4].HeightCm = 250; break;
    default: break; // Mineral-world placeholders; family-specific art remains replaceable.
    }
    return P;
}

inline bool HasHabitat(const FAPSResolvedPlanetSurfaceProfile& P)
{
    return FMath::IsFinite(P.Biomass) && FMath::IsFinite(P.Biodiversity)
        && FMath::IsFinite(P.VisualFoliageDensity)
        && FMath::Max3(P.Biomass, P.Biodiversity * .75f, P.VisualFoliageDensity) > .05f;
}

inline int32 BoundedOasisTreeAttempts(int32 Current, int32 OtherAttempts, int32 Requested, int32 Budget)
{
    // Spend only unused slots in the existing collection envelope. Climate,
    // water, slope, sector size and the other four species stay unchanged.
    if (Current < 1 || OtherAttempts < 0 || Budget < OtherAttempts + Current) return Current;
    return FMath::Clamp(Requested, Current, FMath::Max(Current, FMath::Min(16, Budget - OtherAttempts)));
}

inline bool IsPublishedProfile(const FAPSResolvedPlanetSurfaceProfile& P)
{
    // See 2026-09-29-foliage-prototype-palettes.md. A barren Tundra capture
    // does not validate ColdGrass. Oasis has five-shape and reentry evidence;
    // Metallic/Crystal mineral palettes have natural-ground and reentry evidence.
    // Terrestrial's reviewed visual-only preset keeps gameplay biology zero.
    // Explicit populated-biosphere Terrestrial profiles remain unreviewed.
    switch (P.PlanetType)
    {
    case EPlanetType::Frozen: case EPlanetType::Desert: case EPlanetType::Volcanic:
        return true; // These recipes contain minerals in either habitat state.
    case EPlanetType::Terrestrial:
        return !HasHabitat(P) || (P.Biomass == 0.f && P.Biodiversity == 0.f
            && P.VisualFoliageDensity == APSTerrestrialVegetation::DefaultVisualDensity);
    case EPlanetType::Tundra: case EPlanetType::Water:
    case EPlanetType::Metallic: case EPlanetType::Crystal:
        return !HasHabitat(P);
    case EPlanetType::Forest: case EPlanetType::Oasis:
        return HasHabitat(P);
    default: return false;
    }
}

inline bool UsesScatter(const FAPSResolvedPlanetSurfaceProfile& P, int32 Mode)
{
    return Mode == 1 || (Mode == 2 && IsPublishedProfile(P));
}

inline FString CollectionPath(EPlanetType Type, bool bHabitat)
{
    const UEnum* Types = StaticEnum<EPlanetType>();
    if (!Types || !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) return {};
    const FString Name = TEXT("FC_APS_Scatter_") + Types->GetNameStringByValue(uint8(Type))
        + (bHabitat ? TEXT("") : TEXT("_Mineral"));
    return FString(Root) / Name + TEXT(".") + Name;
}

inline FAPSPlanetFoliageProfile Settings(EPlanetType Type, bool bHabitat)
{
    FAPSPlanetFoliageProfile S;
    const FString Path = CollectionPath(Type, bHabitat);
    if (Path.IsEmpty()) return S;
    S.bEnabled = true;
    S.Collections.Add(TSoftObjectPtr<UWorldScapeFoliagesCollection>(FSoftObjectPath(Path)));
    S.MaxCollections = 1; S.MaxTypesPerCollection = 5;
    S.MaxInstancesPerSectorPerCollection = 64; S.MaxClusterMeshesPerType = 1;
    S.MinSectorSizeCm = 8000; S.MaxCullDistanceMultiplier = 1.25f;
    S.bUseNoiseMask = false; S.bCastShadows = false;
    return S;
}
}
