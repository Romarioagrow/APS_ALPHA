#pragma once

#include "APSClosedGlobeMesh.h"
#include "APSWorldScapePlanetNoise.h"
#include "PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaSurface.h"
#include "Materials/MaterialInstanceConstant.h"

// Capture the already applied ground factory, never select a second globe
// material/profile. Only FSamplingFrame/FBuildOptions cross to mesh workers.
namespace APSNativeGlobeSnapshot
{
    // Positive evidence of an authored override only. Missing assets, invalid
    // native state and shader/profile preparation are NOT permission to fall back.
    // Call on the game thread before taking visibility/initial-coverage ownership.
    inline bool IsExplicitAuthoredOverride(const APlanetaryBody* Body)
    {
        if (!IsInGameThread() || !IsValid(Body)) return false;
        if (const auto* Planet = Cast<APlanet>(Body); Planet && Planet->IsManual) return true;
        const auto* Generator = Body->PlanetaryEnvironmentGenerator;
        const auto* Root = IsValid(Generator) ? Generator->WorldScapeRootInstance : nullptr;
        if (IsValid(Root) && !Generator->bOwnsWorldScapeRootInstance) return true;
        const auto KnownTerrain = [](UMaterialInterface* Material)
        {
            return APSSharedTerrainMaterial::IsGeneratedCatalogStack(Material)
                || APSSharedTerrainMaterial::IsSharedStack(Material)
                || APSNativeTerrainMaterial::IsNativeStack(Material);
        };
        const auto KnownLiquid = [](UMaterialInterface* Material, EAPSPlanetLiquidType Type)
        {
            if (!IsValid(Material)) return false;
            const auto* MID = Cast<UMaterialInstanceDynamic>(Material);
            const UMaterialInterface* Saved = MID ? MID->Parent.Get() : Material;
            if (!IsValid(Saved)) return false;
            const TCHAR* Source = APSSharedGeneratedLiquidMaterial::SourcePath(Type);
            const TCHAR* Shared = APSSharedGeneratedLiquidMaterial::TemplatePath(Type);
            const FString Path = Saved->GetPathName();
            // Identity only: a broken known APS MIC must remain an error, not custom.
            return (Source && Path == Source) || (Shared && Path == Shared)
                || (Type == EAPSPlanetLiquidType::Water
                    && (Path == APSCoastalWaterMaterial::TemplatePath
                        || Path == APSShoreWaterMaterial::TemplatePath));
        };
        // Hot visibility path: never load assets or resolve a fresh profile here.
        // Until the native factory supplies loaded authoring data, stay managed.
        const UAPSPlanetSurfaceCatalog* Catalog = IsValid(Generator) ? Generator->SurfaceProfileCatalog : nullptr;
        if (IsValid(Catalog))
            if (const auto* Definition = Catalog->Archetypes.Find(
                UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(Body->PlanetType)))
            {
                auto* Terrain = Definition->TerrainMaterial.Get();
                if (IsValid(Terrain) && IsValid(Terrain->GetMaterial()) && !KnownTerrain(Terrain)) return true;
                if (IsValid(Generator) && Generator->bSurfaceProfileApplied
                    && Generator->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
                    && Generator->ResolvedSurfaceProfile.LandCoverage < .995f
                    && Definition->LiquidType == Generator->ResolvedSurfaceProfile.LiquidType)
                {
                    auto* Liquid = Definition->OceanMaterial.Get();
                    if (IsValid(Liquid) && IsValid(Liquid->GetMaterial())
                        && !KnownLiquid(Liquid, Generator->ResolvedSurfaceProfile.LiquidType)) return true;
                }
            }
        if (!IsValid(Root)) return false;
        // An unconfigured/deferred root still contains plugin defaults, not an
        // authored override. Only the completed native factory makes these valid.
        if (!Generator->bSurfaceProfileApplied || Generator->IsSurfaceProfileApplyPending()
            || Generator->IsLavaMaterialPreparationPending()) return false;
        if (Root->bUsePlanetaryHeightMap || Root->NoiseOffset != FVector::ZeroVector
            || !Root->HeightMapVolumeList.IsEmpty() || !Root->NoiseVolumeList.IsEmpty()
            || !Root->TerrainHoleList.IsEmpty() || !Root->HeightMapVolumeDataList.IsEmpty()
            || !Root->NoiseVolumeDataList.IsEmpty() || !Root->TerrainHoleDataList.IsEmpty()
            || Root->Terrain_MakeMaterialInstance || !Root->TerrainMaterial.MaterialsLod.IsEmpty()
            || !Root->OceanMaterial.MaterialsLod.IsEmpty()) return true;
        if (IsValid(Root->WorldScapeNoise)
            && Root->WorldScapeNoise->GetClass() != UAPSWorldScapePlanetNoise::StaticClass()) return true;
        auto* Terrain = Root->TerrainMaterial.DefaultMaterial;
        if (IsValid(Terrain) && IsValid(Terrain->GetMaterial()) && !KnownTerrain(Terrain)) return true;
        auto* Liquid = Root->bOcean ? Root->OceanMaterial.DefaultMaterial : nullptr;
        return IsValid(Liquid) && IsValid(Liquid->GetMaterial())
            && Generator->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
            && !KnownLiquid(Liquid, Generator->ResolvedSurfaceProfile.LiquidType);
    }

    inline uint32 Identity(const APlanetaryBody* Body, const UAPSPlanetSurfaceCatalog* Catalog)
    {
        if (!IsInGameThread() || !IsValid(Body)
            || !FMath::IsFinite(Body->GetWorldScapeBodyRadiusCm())) return 0;
        const auto Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body, Catalog);
        uint32 Result = HashCombineFast(UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Profile),
            GetTypeHash(Body->GetWorldScapeBodyRadiusCm()));
        Result = HashCombineFast(Result, GetTypeHash(Body->WorldScapeSeed));
        return Result ? Result : 1u;
    }

    inline bool Ready(const APlanetaryBody* Body, const APlanetarySurfaceGenerator* Generator, FString& Error)
    {
        const auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
        if (!IsInGameThread()) return Fail(TEXT("Native globe capture requires the game thread"));
        if (!IsValid(Body) || !IsValid(Generator) || Generator->PlanetaryBody != Body)
            return Fail(TEXT("Native globe has no matching body/factory"));
        if (!IsValid(Generator->GetWorld()) || Body->GetWorld() != Generator->GetWorld())
            return Fail(TEXT("Native globe body/factory world is invalid"));
        const auto* Planet = Cast<APlanet>(Body);
        if ((Planet && Planet->IsManual) || !Generator->bOwnsWorldScapeRootInstance)
            return Fail(TEXT("Native globe does not support authored/manual roots"));
        if (!FMath::IsFinite(Body->WorldScapePresentationScale) || Body->WorldScapePresentationScale != 1.0)
            return Fail(TEXT("Native globe requires full-scale body presentation"));
        if (Generator->IsSurfaceProfileApplyPending() || !Generator->IsSurfaceProfileCurrent(Body))
            return Fail(TEXT("Native globe profile is not fully applied yet"));
        const auto* Root = Generator->WorldScapeRootInstance;
        const auto* Noise = Generator->ResolvedNoiseInstance;
        if (!IsValid(Root) || !IsValid(Noise) || Root->GetOwner() != Body
            || Noise->GetClass() != UAPSWorldScapePlanetNoise::StaticClass() || Root->WorldScapeNoise != Noise)
            return Fail(TEXT("Native globe requires the exact configured APS noise/root"));
        if (Root->GenerationType != EWorldScapeType::Planet || Root->bFlatWorld
            || !Root->GetActorScale3D().Equals(FVector::OneVector, 1.e-9)
            || Root->GetActorLocation().ContainsNaN() || Root->GetActorQuat().ContainsNaN())
            return Fail(TEXT("Native globe requires a finite unit-scale spherical root"));
        if (Root->bUsePlanetaryHeightMap || Root->NoiseOffset != FVector::ZeroVector
            || !Root->HeightMapVolumeList.IsEmpty() || !Root->NoiseVolumeList.IsEmpty()
            || !Root->TerrainHoleList.IsEmpty() || !Root->HeightMapVolumeDataList.IsEmpty()
            || !Root->NoiseVolumeDataList.IsEmpty() || !Root->TerrainHoleDataList.IsEmpty())
            return Fail(TEXT("Native globe cannot reproduce heightmap/noise/hole volumes or noise offsets"));
        if (Root->Seed != Body->WorldScapeSeed
            || double(Root->PlanetScale) != double(float(Body->GetWorldScapeBodyRadiusCm()))
            || !FMath::IsFinite(Root->NoiseScale) || Root->NoiseScale < 1.0f
            || !FMath::IsFinite(Root->NoiseIntensity) || Root->NoiseIntensity < 1.0f
            || !FMath::IsFinite(Root->OceanHeight))
            return Fail(TEXT("Native globe root seed/radius/noise settings differ from applied body"));
        if (UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Noise->SurfaceProfile)
            != UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Generator->ResolvedSurfaceProfile))
            return Fail(TEXT("Native globe configured noise profile differs from factory profile"));
        auto* Terrain = Generator->ResolvedTerrainMaterialInstance;
        if (!IsValid(Terrain) || !IsValid(Terrain->Parent.Get())
            || Root->TerrainMaterial.DefaultMaterial != Terrain || Root->Terrain_MakeMaterialInstance
            || !Root->TerrainMaterial.MaterialsLod.IsEmpty()
            || (!APSSharedTerrainMaterial::IsSharedStack(Terrain)
                && !APSNativeTerrainMaterial::IsNativeStack(Terrain)))
            return Fail(TEXT("Native globe does not support custom/per-LOD terrain material routing"));
        const auto& Profile = Generator->ResolvedSurfaceProfile;
        const bool Unified = Terrain->GetMaterial()->GetPathName() == APSUnifiedLavaAssets::MasterPath();
        if (Unified && (Profile.LiquidType != EAPSPlanetLiquidType::Lava || Root->bOcean
            || IsValid(Generator->ResolvedOceanMaterialInstance)))
            return Fail(TEXT("Native globe UnifiedLava material/envelope/ocean decision is inconsistent"));
        const bool WantsLiquid = Profile.LiquidType != EAPSPlanetLiquidType::None && Profile.LandCoverage < .995f;
        if (WantsLiquid && !Root->bOcean && !Unified)
            return Fail(TEXT("Native globe liquid profile has no applied ocean or unified envelope"));
        if (Root->bOcean && (!IsValid(Generator->ResolvedOceanMaterialInstance)
            || Root->OceanMaterial.DefaultMaterial != Generator->ResolvedOceanMaterialInstance
            || !Root->OceanMaterial.MaterialsLod.IsEmpty()
            || !APSSharedGeneratedLiquidMaterial::HasSavedParameterAuthority(
                Generator->ResolvedOceanMaterialInstance, Profile.LiquidType)))
            return Fail(TEXT("Native globe does not support custom/incomplete ocean material routing"));
        return true;
    }

    inline bool Capture(const APlanetaryBody* Body, const APlanetarySurfaceGenerator* Generator,
        APSClosedGlobeMesh::FSamplingFrame& Frame, APSClosedGlobeMesh::FBuildOptions& Options,
        uint32& Signature, FString& Error)
    {
        Error.Reset(); Signature = 0;
        if (!Ready(Body, Generator, Error)) return false;
        const auto* Root = Generator->WorldScapeRootInstance;
        const auto* Noise = Generator->ResolvedNoiseInstance;
        APSClosedGlobeMesh::FSamplingFrame Snapshot;
        Snapshot.Profile = Generator->ResolvedSurfaceProfile;
        Snapshot.SeededNoise = Root->PlanetNoise; // Already primed/seeded by the native factory.
        // PlanetScaleCode is stale before the first native tick; PlanetScale is
        // the applied float that InitializeValues will copy verbatim into it.
        Snapshot.Radius = double(Root->PlanetScale);
        Snapshot.NoiseScale = double(Root->NoiseScale);
        Snapshot.NoiseIntensity = double(Root->NoiseIntensity);
        Snapshot.PresentationScale = 1.0;
        Snapshot.OceanHeight = FMath::Max(double(Root->OceanHeight),
            double(Snapshot.Profile.OceanLevel) * Snapshot.NoiseIntensity);
        Snapshot.bCoastalReliefCandidate = Noise->UsesCoastalReliefCandidate();
        Snapshot.bApplyNativeLavaEnvelope = Generator->ResolvedTerrainMaterialInstance->GetMaterial()->GetPathName()
            == APSUnifiedLavaAssets::MasterPath();
        Options.bBuildOcean = Root->bOcean;
        Options.bWaterDepth = Root->bOcean && Snapshot.Profile.LiquidType == EAPSPlanetLiquidType::Water
            && APSCoastalWaterMaterial::IsInstance(Generator->ResolvedOceanMaterialInstance);
        Options.bRefineCoast = Options.bWaterDepth;
        Options.NormalReliefExaggeration = 1.0;
        Options.NormalPolicy = APSClosedGlobeMesh::ENormalPolicy::PhysicalTriangles;
        // Native SetData encodes A=Data.Hole ? 1 : 0, not visibility. Unsupported
        // hole-volume paths were rejected above. Ocean has separate WaterMask A.
        Options.bNativeTerrainHoleAlpha = true;
        Frame = MoveTemp(Snapshot);
        Signature = Identity(Body, Generator->SurfaceProfileCatalog);
        if (!Signature) { Error = TEXT("Native globe body identity is invalid"); return false; }
        return true;
    }

    inline bool CloneMaterials(UObject* Owner, USceneComponent* UnitFrame,
        const APlanetarySurfaceGenerator* Generator, UMaterialInstanceDynamic*& Terrain,
        UMaterialInstanceDynamic*& Liquid, FString& Error)
    {
        Terrain = nullptr; Liquid = nullptr; Error.Reset();
        const auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
        if (!IsValid(Generator) || !Ready(Generator->PlanetaryBody, Generator, Error)) return false;
        const auto* Root = Generator->WorldScapeRootInstance;
        if (!IsValid(Owner) || !IsValid(UnitFrame) || UnitFrame->GetWorld() != Generator->GetWorld()
            || !UnitFrame->GetComponentScale().Equals(FVector::OneVector, 1.e-9)
            || !UnitFrame->GetComponentLocation().Equals(Root->GetActorLocation(), .01)
            || !UnitFrame->GetComponentQuat().Equals(Root->GetActorQuat(), 1.e-9))
            return Fail(TEXT("Native globe material frame does not match the unit-scale native root"));
        auto* SourceTerrain = Generator->ResolvedTerrainMaterialInstance;
        auto* SourceLiquid = Root->bOcean ? Generator->ResolvedOceanMaterialInstance : nullptr;
        for (auto* Source : {SourceTerrain, SourceLiquid})
        {
            if (!Source) continue;
            if (!IsValid(Source->Parent.Get()) || !Source->Parent->IsA<UMaterialInstanceConstant>())
                return Fail(TEXT("Native globe requires a saved MIC parent, not a root-owned MID chain"));
            if (!Source->RuntimeVirtualTextureParameterValues.IsEmpty() || !Source->SparseVolumeTextureParameterValues.IsEmpty())
                return Fail(TEXT("Native globe cannot copy unhandled virtual/sparse texture overrides"));
            auto* Resource = Source->GetMaterialResource(Generator->GetWorld()->GetFeatureLevel());
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
#if WITH_EDITOR
            if (Resource && !Resource->GetCompileErrors().IsEmpty())
                return Fail(TEXT("Native globe source material has shader compile errors"));
            // Editor/game PostLoad may leave an on-demand partial shader map even
            // though the visible native root can already draw it. Unlike a bake,
            // checking completeness alone does not schedule the missing jobs.
            // UE deduplicates submissions per map/priority. Never block the game
            // thread, recompile a complete map, or publish a fallback material.
            if (Resource && Map && !Resource->IsGameThreadShaderMapComplete())
                Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
#endif
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !Map
                || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            {
                Error = FString::Printf(TEXT("Native globe source shader pending: parent=%s resource=%d map=%d complete=%d localVF=%d"),
                    *GetPathNameSafe(Source->Parent.Get()), int32(Resource != nullptr), int32(Map != nullptr),
                    int32(Resource && Resource->IsGameThreadShaderMapComplete()),
                    int32(Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)));
                return false;
            }
        }
        // Same saved MIC parent preserves static switches and inherited values.
        // Do not parent a globe MID to the retiring root's MID. CopyParameterOverrides
        // includes DoubleVector and full ParameterInfo; root delegates are NOT copied.
        auto* NewTerrain = UMaterialInstanceDynamic::Create(SourceTerrain->Parent, Owner);
        if (!NewTerrain) return Fail(TEXT("Native globe terrain MID creation failed"));
        NewTerrain->CopyParameterOverrides(SourceTerrain);
        if (APSSharedTerrainMaterial::IsSharedStack(NewTerrain))
        {
            if (!APSSharedTerrainMaterial::BindNewInstanceFrame(NewTerrain, UnitFrame, 1.0))
                return Fail(TEXT("Native globe terrain frame binding failed"));
        }
        else APSNativeTerrainMaterial::BindNewInstanceCenter(NewTerrain, UnitFrame);
        UMaterialInstanceDynamic* NewLiquid = nullptr;
        if (SourceLiquid)
        {
            NewLiquid = UMaterialInstanceDynamic::Create(SourceLiquid->Parent, Owner);
            if (!NewLiquid) return Fail(TEXT("Native globe liquid MID creation failed"));
            NewLiquid->CopyParameterOverrides(SourceLiquid);
            NewLiquid->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), 1.0f);
            if (!APSSharedGeneratedLiquidMaterial::BindFrame(NewLiquid,
                Generator->ResolvedSurfaceProfile.LiquidType, UnitFrame, 1.0))
                return Fail(TEXT("Native globe liquid frame binding failed"));
        }
        // Caller must hold these in UPROPERTY/strong references before root unload;
        // UObject Outer alone is not a garbage-collection ownership reference.
        Terrain = NewTerrain; Liquid = NewLiquid;
        return true;
    }
}
