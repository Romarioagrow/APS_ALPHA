#pragma once

#include "APSPlanetSurfaceProfile.h"
#include "APSUnifiedLavaAssets.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

// Shared routing/parameter policy, not a declaration of visual acceptance.
// Terrain always uses its original graph. The legacy flag below is retained only
// for separate water-policy diagnostics; it must not select a terrain parent.
namespace APSPlanetSurfaceMaterialPolicy
{
    struct FAcceptedPaths
    {
        EPlanetType Type;
        bool bContinuousTerrain;
        bool bCoastalWater;
    };

    // Pre-unification baseline: 14 terrain types and 3 water types, retained for
    // controlled comparisons. Chemistry and geometry remain profile-owned.
    inline constexpr FAcceptedPaths AcceptedPaths[] = {
        {EPlanetType::Rocky, true, false},
        {EPlanetType::Terrestrial, true, true},
        {EPlanetType::Water, false, true},
        {EPlanetType::Desert, true, false},
        {EPlanetType::Forest, true, false},
        {EPlanetType::Ice, true, false},
        {EPlanetType::Frozen, true, false},
        {EPlanetType::SuperEarth, true, false},
        {EPlanetType::Nordic, true, false},
        {EPlanetType::Tundra, true, false},
        {EPlanetType::HighMountain, true, false},
        {EPlanetType::Sand, true, false},
        {EPlanetType::Oasis, true, true},
        {EPlanetType::Pangea, true, false},
        {EPlanetType::Savanna, true, false}
    };

    inline FAcceptedPaths AcceptedFor(EPlanetType Type)
    {
        for (const FAcceptedPaths& Paths : AcceptedPaths)
            if (Paths.Type == Type) return Paths;
        return {Type, false, false};
    }

    inline bool UnifiedRoutesEnabled()
    {
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
        // Explicit legacy control wins even if an old candidate flag is present.
        return !FParse::Param(FCommandLine::Get(), TEXT("APSLegacySurfacePipelineControl"));
#else
        return true;
#endif
    }

    inline bool IsConcreteSolidType(EPlanetType Type)
    {
        // Legacy Exoplanet remains resolvable. Unknown is a placeholder, not a
        // concrete preset; future/invalid IDs must not enter an untested graph.
        return static_cast<uint8>(Type) <= APSPlanetTypes::LastValue
            && Type != EPlanetType::Unknown
            && UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type);
    }

    inline bool AllowsContinuousTerrain(EPlanetType Type, bool bUnified)
    {
        return AcceptedFor(Type).bContinuousTerrain
            || (bUnified && IsConcreteSolidType(Type)
                && UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(Type) != EAPSPlanetSurfaceArchetype::Magmatic);
    }

    inline bool AllowsCoastalWater(const FAPSResolvedPlanetSurfaceProfile& Profile, bool bUnified)
    {
        return Profile.LiquidType == EAPSPlanetLiquidType::Water
            && FMath::IsFinite(Profile.LandCoverage) && Profile.LandCoverage >= 0.0f && Profile.LandCoverage < .995f
            && (AcceptedFor(Profile.PlanetType).bCoastalWater
                || (bUnified && IsConcreteSolidType(Profile.PlanetType)));
    }

    inline constexpr const TCHAR* ContinuousMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");
    inline constexpr const TCHAR* ContinuousTemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra");
    inline constexpr const TCHAR* SharedMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/M_APS_SharedWorldScapeTerrain.M_APS_SharedWorldScapeTerrain");
    inline constexpr const TCHAR* UnifiedLavaMasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");

    inline const TCHAR* SelectedContinuousTemplatePath(EPlanetType Type)
    {
        return ContinuousTemplatePath;
    }

    enum class ETerrainGraph : uint8 { Other, Shared, Continuous, UnifiedLava };

    inline ETerrainGraph TerrainGraph(UMaterialInterface* Material)
    {
        const UMaterial* Master = IsValid(Material) ? Material->GetMaterial() : nullptr;
        if (!IsValid(Master)) return ETerrainGraph::Other;
        const FString Path = Master->GetPathName();
        if (Path == SharedMasterPath) return ETerrainGraph::Shared;
        if (Path == ContinuousMasterPath) return ETerrainGraph::Continuous;
        if (Path == UnifiedLavaMasterPath || (APSUnifiedLavaAssets::DetailCandidate()
            && Path == APSUnifiedLavaAssets::MasterPath())) return ETerrainGraph::UnifiedLava;
        return ETerrainGraph::Other;
    }

    inline constexpr float FarNormalStartCm = 200000.0f;
    inline constexpr float FarNormalEndCm = 2000000.0f;

    inline bool ApplyFarNormalPolicy(UMaterialInstanceDynamic* Material)
    {
        if (TerrainGraph(Material) == ETerrainGraph::Other) return false;
        // Only the two already accepted rock-normal uniforms are shared. In
        // particular, never introduce orbital/warp overrides into UnifiedLava.
        Material->SetScalarParameterValue(TEXT("APS_FarNormalStartCm"), FarNormalStartCm);
        Material->SetScalarParameterValue(TEXT("APS_FarNormalEndCm"), FarNormalEndCm);
        return true;
    }
}
