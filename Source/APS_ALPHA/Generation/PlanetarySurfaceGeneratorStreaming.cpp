#include "PlanetarySurfaceGenerator.h"
#include "APSNativeGlobeSnapshot.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetReliefRuntime.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceRadius.h"
#include "APS_ALPHA/Core/Planetary/APSNativeTerrainMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedGeneratedLiquidMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeLiquidLattice.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaSurface.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaMaterialPreparation.h"
#include "APS_ALPHA/Core/Planetary/APSOrbitalWaterAppearance.h"
#include "APSWorldScapePlanetNoise.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "LocalVertexFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MaterialShared.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace APSWorldScapeProfiles
{
    TAutoConsoleVariable<int32> CVarShoreWater(
        TEXT("aps.Surface.ShoreWater"), 0,
        TEXT("DEFAULT-OFF shore transmission factory trial for existing CoastalWater families. ")
        TEXT("Same saved template in PLANET/gameplay; requires supported single-layer-water render settings. ")
        TEXT("Recreate profile after changing; no geometry or other-chemistry changes."), ECVF_Default);
    TAutoConsoleVariable<int32> CVarCoastalWater(
        TEXT("aps.Surface.CoastalWater"), 1,
        TEXT("Versioned dark Water with footprint-filtered anchored ripples and native bathymetry. ")
        TEXT("Generated Water/Terrestrial/Oasis only; recreate profile after change. 0 restores SharedLiquid. ")
        TEXT("Enabled for validated release-path families; other liquids and authored instances are unchanged."), ECVF_Default);
    TAutoConsoleVariable<int32> CVarTerrestrialPalette(
        TEXT("aps.Surface.TerrestrialPalette"), 1,
        TEXT("Material-only Earth-like palette: lower biome chroma, preserve luminance. ")
        TEXT("Generated Shared/Continuous Terra only; no exposure/geometry changes. Read on profile recreation."), ECVF_Default);
    TAutoConsoleVariable<int32> CVarTerrainContinuity(
        TEXT("aps.Surface.TerrainContinuity"), 1,
        TEXT("Read-only compatibility marker. Generated nonmagmatic terrain always uses the original continuous material; no rollback parent."), ECVF_ReadOnly);
    TAutoConsoleVariable<float> CVarLivingPaletteDetail(
        TEXT("aps.Surface.LivingPaletteDetail"), 1.0f,
        TEXT("0..1 restores the distinct cool dryland palette endpoint on generated SharedTerra water worlds. ")
        TEXT("0 restores identical Color4/Color5. No climate/height/coverage change. Read on profile creation."), ECVF_Default);
    TAutoConsoleVariable<int32> CVarCoastResolution(
        TEXT("aps.Surface.CoastResolution"), 0,
        TEXT("Matched terrain/liquid ring resolution for generated full-scale wet planets, capped at 256. ")
        TEXT("0 keeps aps.Surface.MeshResolution (default 256). Read at profile creation; restart PIE. ")
        TEXT("No new surface, collision change or material bake. Higher vertex cost; FPS needs validation."), ECVF_Default);
    TAutoConsoleVariable<float> CVarOrbitalWaterContrast(
        TEXT("aps.Surface.OrbitalWaterContrast"), 0.0f,
        TEXT("Read-only compatibility marker; ignored by rendering. Water retains its saved ")
        TEXT("roughness/specular at every camera height. Lava and Ammonia are unchanged."), ECVF_ReadOnly);
    TAutoConsoleVariable<int32> CVarUnifiedLavaSurface(
        TEXT("aps.Surface.UnifiedLavaSurface"), 1,
        TEXT("Read-only compatibility marker. Eligible generated lava retains its original unified surface; no split-material rollback."), ECVF_ReadOnly);
    TAutoConsoleVariable<int32> CVarSurfaceMeshResolution(
        TEXT("aps.Surface.MeshResolution"), APSWorldScapeLiquidLattice::DefaultTerrainResolution,
        TEXT("Generated full-scale WorldScape ring resolution, 96..256 in multiples of four. ")
        TEXT("Default 256; 192 restores the previous geometry budget. FPS needs runtime validation. ")
        TEXT("Read only when creating a surface profile: stop PIE before changing. ")
        TEXT("Terrain and liquid stay on matching lattices; collision and PLANET previews are unchanged."),
        ECVF_Default);
    // 08.10: registered with the module (it was a function static, created on the first surface profile, so a startup
    // -ExecCmds value never reached it and the A/B arm "0" silently ran as 1).
    TAutoConsoleVariable<int32> CVarPreparedAllFamilies(
        TEXT("aps.Surface.PreparedAllFamilies"), 1,
        TEXT("1: worker-prepared WorldScape publication for every planet family (08.10). ")
        TEXT("0: only Terrestrial, Frozen and Oasis (30.09). Read when a surface profile is created."),
        ECVF_Default);

	struct FSurfaceProfile
	{
		UWorldScapeNoiseClass* Noise{nullptr};
		UMaterialInstance* TerrainMaterial{nullptr};
		UMaterialInstance* OceanMaterial{nullptr};
		float NoiseScale{800.0f};
		float NoiseIntensity{1200000.0f};
		float OceanHeight{0.0f};
		bool bOcean{false};
	};
}

uint32 APlanetarySurfaceGenerator::BuildSurfaceProfileSignature(const APlanetaryBody* Body) const
{
	if (!IsValid(Body))
	{
		return 0;
	}

	uint32 Signature = GetTypeHash(Body->WorldScapeSeed);
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt64(
		APSPlanetSurfaceRadius::Kilometres(Body->RadiusKM, Body->PlanetRadiusKM) * 1000.0)));
	Signature = HashCombine(Signature,
		GetTypeHash(FMath::RoundToInt64(Body->WorldScapePresentationScale * 1.0e9)));
	Signature = HashCombine(Signature, GetTypeHash(static_cast<uint8>(Body->PlanetType)));
	const FAPSResolvedPlanetSurfaceProfile Resolved =
		UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body, SurfaceProfileCatalog);
	Signature = HashCombine(Signature,
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Resolved));
	if (const AMoon* Moon = Cast<AMoon>(Body))
	{
		Signature = HashCombine(Signature, GetTypeHash(static_cast<uint8>(Moon->MoonType)));
		Signature = HashCombine(Signature, 0x4D4F4F4Eu); // MOON
	}
	else
	{
		Signature = HashCombine(Signature, 0x504C414Eu); // PLAN
	}

	// Zero remains the explicit "no profile" state.
	return Signature == 0 ? 1u : Signature;
}

bool APlanetarySurfaceGenerator::IsSurfaceProfileCurrent(const APlanetaryBody* Body) const
{
	return bSurfaceProfileApplied
		&& AppliedSurfaceProfileSignature != 0
		&& AppliedSurfaceProfileSignature == BuildSurfaceProfileSignature(Body);
}

void APlanetarySurfaceGenerator::UpdateOrbitalWaterAppearance()
{
	AWorldScapeRoot* Root = WorldScapeRootInstance;
	UMaterialInstanceDynamic* Material = ResolvedOceanMaterialInstance;
	const APlanetaryBody* Body = PlanetaryBody;
	const APlanet* Planet = Cast<APlanet>(Body);
	if (!IsValid(Root) || !Root->bOcean || !IsValid(Body)
		|| !APSOrbitalWaterAppearance::IsEligible(bOwnsWorldScapeRootInstance, Planet && Planet->IsManual,
			ResolvedSurfaceProfile.LiquidType, Body->WorldScapePresentationScale, Root->GetActorScale3D())
		|| !APSSharedGeneratedLiquidMaterial::IsFamilyInstance(Material, EAPSPlanetLiquidType::Water)) return;
	// Native rings and the body-owned closed globe must inherit the same saved
	// response. Do not reintroduce a camera-dependent override via stale config.
	APSOrbitalWaterAppearance::RestoreAuthoredResponse(Material);
}

bool APlanetarySurfaceGenerator::FinalizeStableWaterMaterial()
{
	AWorldScapeRoot* Root = WorldScapeRootInstance;
	if (!IsValid(Root) || !bSurfaceProfileApplied
		|| AppliedSurfaceProfileSignature == 0
		|| Root->WorldScapeLodInGeneration.Num() > 0)
	{
		return false;
	}

	// A dry profile has no streamed liquid slots to publish. All actual oceans
	// must pass readiness, not just Water; retaining a family is not proof that
	// its shader or the worker-created render proxies are ready.
	if (!Root->bOcean)
	{
		return true;
	}

	auto HasExactWaterSlots = [Root](UMaterialInterface* ExpectedMaterial)
	{
		if (!IsValid(ExpectedMaterial)
			|| Root->WorldScapeLodOcean.Num() != Root->OceanMaxLod)
		{
			return false;
		}

		int32 MatchingSlotCount = 0;
		for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
		{
			if (!IsValid(OceanLod) || !IsValid(OceanLod->Mesh)
				|| OceanLod->Mesh->GetNumSections() != 3)
			{
				return false;
			}
			for (int32 MaterialIndex = 0; MaterialIndex < 3; ++MaterialIndex)
			{
				if (OceanLod->Mesh->GetMaterial(MaterialIndex) != ExpectedMaterial)
				{
					return false;
				}
				++MatchingSlotCount;
			}
		}
		return MatchingSlotCount == Root->OceanMaxLod * 3;
	};

	if (FinalizedWaterMaterialRoot.Get() == Root
		&& FinalizedWaterMaterialProfileSignature == AppliedSurfaceProfileSignature
		&& Root->OceanMaterial.DefaultMaterial == ResolvedOceanMaterialInstance)
	{
		return HasExactWaterSlots(ResolvedOceanMaterialInstance);
	}

	const APlanet* LiquidPlanet = Cast<APlanet>(PlanetaryBody);
	const bool bSharedLiquid = APSSharedGeneratedLiquidMaterial::IsFamilyInstance(
		ResolvedOceanMaterialInstance, ResolvedSurfaceProfile.LiquidType)
		&& APSSharedGeneratedLiquidMaterial::ShouldMigrate(
			APSSharedGeneratedLiquidMaterial::ResolveSource(ResolvedSurfaceProfile, SurfaceProfileCatalog),
			ResolvedSurfaceProfile, LiquidPlanet && LiquidPlanet->IsManual);
	if (ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water || bSharedLiquid)
	{
		UMaterialInstanceDynamic* PreviousMID = ResolvedOceanMaterialInstance;
		UWorld* World = Root->GetWorld();
		if (!IsValid(World) || !IsValid(PreviousMID)
			|| Root->OceanMaterial.DefaultMaterial != PreviousMID
			|| !IsValid(PreviousMID->Parent) || !HasExactWaterSlots(PreviousMID))
		{
			return false;
		}
		// The job was requested at profile creation, alongside LOD generation.
		// Only poll here: no synchronous compilation stall during landing.
		FMaterialResource* Resource = PreviousMID->GetMaterialResource(World->GetFeatureLevel());
		FMaterialShaderMap* ShaderMap = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
		if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !ShaderMap
			|| !ShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
		{
			return false;
		}
		if (bSharedLiquid && !APSSharedGeneratedLiquidMaterial::IsRenderReady(
			PreviousMID, ResolvedSurfaceProfile.LiquidType, World)) return false;

		// Preserve the authored MIC chain (including lava textures/static switches).
		// Flattening to GetMaterial() would lose the inherited overrides.
		UMaterialInstanceDynamic* StableMID =
			UMaterialInstanceDynamic::Create(PreviousMID->Parent, Root);
		if (!IsValid(StableMID)) return false;
		StableMID->CopyInterpParameters(PreviousMID);
		if (bSharedLiquid)
		{
			// A fresh MID has no transform/rebase delegates. Rebind before publishing
			// all WorldScape sections; gameplay Hole alpha is never a shoreline mask.
			if (!IsValid(PlanetaryBody)) return false;
			StableMID->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), 0.0f);
			if (!APSSharedGeneratedLiquidMaterial::BindFrame(StableMID, ResolvedSurfaceProfile.LiquidType, Root->GetRootComponent(),
				PlanetaryBody->WorldScapePresentationScale)
				|| !APSSharedGeneratedLiquidMaterial::IsRenderReady(StableMID, ResolvedSurfaceProfile.LiquidType, World)) return false;
		}
		else
		{
			APSNativeTerrainMaterial::BindNewInstanceCenter(StableMID, Root->GetRootComponent());
		}
		Root->OceanMaterial.DefaultMaterial = StableMID;
		Root->UpdateOceanMaterial(Root->OceanMaterial);
		if (!HasExactWaterSlots(StableMID))
		{
			Root->OceanMaterial.DefaultMaterial = PreviousMID;
			Root->UpdateOceanMaterial(Root->OceanMaterial);
			return false;
		}
		for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
		{
			OceanLod->Mesh->MarkRenderStateDirty();
		}
		ResolvedOceanMaterialInstance = StableMID;
		FinalizedWaterMaterialRoot = Root;
		FinalizedWaterMaterialProfileSignature = AppliedSurfaceProfileSignature;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldScape.Liquid] Published type=%s parent=%s complete=1 localVF=1 slots=%d nativeCenter=%d sharedLiquid=%d context=%d"),
			*UEnum::GetValueAsString(ResolvedSurfaceProfile.LiquidType),
			*GetPathNameSafe(StableMID->Parent), Root->OceanMaxLod * 3,
			APSNativeTerrainMaterial::UsesNativePlanetCenter(StableMID) ? 1 : 0,
			bSharedLiquid ? 1 : 0, bSharedLiquid ? 0 : -1);
		return true;
	}

	UMaterialInstanceDynamic* PreviousWaterMID = ResolvedOceanMaterialInstance;
	if (!IsValid(PreviousWaterMID)
		|| Root->OceanMaterial.DefaultMaterial != PreviousWaterMID
		|| !HasExactWaterSlots(PreviousWaterMID))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Stable material finalization lost the exact configured MID set root=%s resolved=%s rootDefault=%s"),
			*GetNameSafe(Root), *GetNameSafe(PreviousWaterMID),
			*GetNameSafe(Root->OceanMaterial.DefaultMaterial));
		return false;
	}

	UMaterial* WaterMaster = PreviousWaterMID->GetMaterial();
	UWorld* World = Root->GetWorld();
	if (!IsValid(WaterMaster) || !IsValid(World))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Missing Water master/world root=%s master=%s"),
			*GetNameSafe(Root), *GetNameSafe(WaterMaster));
		return false;
	}

	// UE 5.4's public synchronous material barrier. In editor builds it submits
	// incomplete render feature-level jobs with ForceLocal priority and waits for
	// completion; in cooked builds the validation below still rejects a missing map.
	PreviousWaterMID->EnsureIsComplete();
	FMaterialResource* WaterResource = PreviousWaterMID->GetMaterialResource(
		World->GetFeatureLevel());
	FMaterialShaderMap* WaterShaderMap = WaterResource
		? WaterResource->GetGameThreadShaderMap()
		: nullptr;
	const bool bShaderMapComplete = WaterResource
		&& WaterResource->IsGameThreadShaderMapComplete();
	const bool bLocalVertexFactoryReady = WaterShaderMap
		&& WaterShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType) != nullptr;
	if (!bShaderMapComplete || !bLocalVertexFactoryReady)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Water shader map not render-ready root=%s master=%s featureLevel=%d complete=%d localVF=%d"),
			*GetNameSafe(Root), *GetNameSafe(WaterMaster),
			static_cast<int32>(World->GetFeatureLevel()),
			bShaderMapComplete ? 1 : 0, bLocalVertexFactoryReady ? 1 : 0);
		return false;
	}

	UMaterialInstanceDynamic* StableWaterMID =
		UMaterialInstanceDynamic::Create(PreviousWaterMID->Parent, Root);
	if (!IsValid(StableWaterMID))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Could not create stable Water MID root=%s master=%s"),
			*GetNameSafe(Root), *GetNameSafe(WaterMaster));
		return false;
	}

	// Keep the exact saved MIC chain, static permutation and all dynamic
	// overrides through the worker-batch handoff, just as for other liquid types.
	StableWaterMID->CopyInterpParameters(PreviousWaterMID);
	auto CopyWaterColor = [PreviousWaterMID, StableWaterMID](const FName ParameterName)
	{
		FLinearColor Value = FLinearColor::Black;
		if (!PreviousWaterMID->GetVectorParameterValue(
			FHashedMaterialParameterInfo(ParameterName), Value))
		{
			return false;
		}
		StableWaterMID->SetVectorParameterValue(ParameterName, Value);
		return true;
	};
	if (!CopyWaterColor(TEXT("WaterDeepColor"))
		|| !CopyWaterColor(TEXT("WaterShallowColor"))
		|| !CopyWaterColor(TEXT("WaterRadianceFloor")))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Stable Water MID could not copy the authored marine palette root=%s source=%s"),
			*GetNameSafe(Root), *GetNameSafe(PreviousWaterMID));
		return false;
	}
	// Publish only after the complete worker batch exists. UpdateOceanMaterial is
	// synchronous on the game thread; validation below makes the pointer swap atomic
	// from the readiness hand-off's perspective.
	Root->OceanMaterial.DefaultMaterial = StableWaterMID;
	Root->UpdateOceanMaterial(Root->OceanMaterial);
	for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
	{
		if (IsValid(OceanLod) && IsValid(OceanLod->Mesh))
		{
			OceanLod->Mesh->MarkRenderStateDirty();
		}
	}
	if (!HasExactWaterSlots(StableWaterMID))
	{
		// Never expose a partially swapped material set. Restore the original root
		// contract and let the next readiness refresh retry finalization.
		Root->OceanMaterial.DefaultMaterial = PreviousWaterMID;
		Root->UpdateOceanMaterial(Root->OceanMaterial);
		for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
		{
			if (IsValid(OceanLod) && IsValid(OceanLod->Mesh))
			{
				OceanLod->Mesh->MarkRenderStateDirty();
			}
		}
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Water] Stable Water MID did not reach the exact ocean slot set root=%s lods=%d expected=%d"),
			*GetNameSafe(Root), Root->WorldScapeLodOcean.Num(), Root->OceanMaxLod);
		return false;
	}

	ResolvedOceanMaterialInstance = StableWaterMID;
	FinalizedWaterMaterialRoot = Root;
	FinalizedWaterMaterialProfileSignature = AppliedSurfaceProfileSignature;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldScape.Water] Stable Water MID published root=%s signature=%u master=%s slots=%d"),
		*GetNameSafe(Root), AppliedSurfaceProfileSignature, *GetNameSafe(WaterMaster),
		Root->OceanMaxLod * 3);
	return true;
}

bool APlanetarySurfaceGenerator::CreateRuntimeWorldScapeRoot(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !GetWorld())
	{
		return false;
	}
	if (IsValid(WorldScapeRootInstance))
	{
		PlanetaryBody = Body;
		return true;
	}

	PlanetaryBody = Body;
	const FTransform RootTransform(Body->GetActorQuat(), Body->GetActorLocation(), FVector::OneVector);
	WorldScapeRootInstance = GetWorld()->SpawnActorDeferred<AWorldScapeRoot>(
		AWorldScapeRoot::StaticClass(), RootTransform, Body, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(WorldScapeRootInstance))
	{
		return false;
	}

	WorldScapeRootInstance->SetFlags(RF_Transient);
	WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
	WorldScapeRootInstance->bGenerateWorldScape = false;
	WorldScapeRootInstance->bFreezeGeneration = true;
	// WorldScape 5.4 defaults foliage to true in its constructor. APS owns the
	// opposite invariant and may opt a fresh full-scale root in only after its
	// resolved profile has been budgeted.
	WorldScapeRootInstance->bGenerateFoliages = false;
	UGameplayStatics::FinishSpawningActor(WorldScapeRootInstance, RootTransform);
	bOwnsWorldScapeRootInstance = true;
	CancelPendingSurfaceProfileApply();
	CancelLavaMaterialPreparation();
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	WorldScapeRootInstance->SetActorScale3D(FVector::OneVector);
	WorldScapeRootInstance->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorEnableCollision(false);
	FinalizedWaterMaterialRoot.Reset();
	FinalizedWaterMaterialProfileSignature = 0;
	return true;
}

bool APlanetarySurfaceGenerator::ReplaceDrainedRuntimeWorldScapeRoot(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !GetWorld())
	{
		return false;
	}

	AWorldScapeRoot* DrainedRootToReplace = WorldScapeRootInstance;
	if (IsValid(DrainedRootToReplace))
	{
		// Stop the producer before inspecting its worker map. A non-empty map means
		// LodGenerationThread can still write into one of this root's UWorldScapeLod
		// buffers, so neither its profile references nor the actor may be retired yet.
		DrainedRootToReplace->bGenerateWorldScape = true;
		DrainedRootToReplace->bFreezeGeneration = true;
		DrainedRootToReplace->SetActorTickEnabled(false);
		DrainedRootToReplace->SetActorHiddenInGame(true);
		DrainedRootToReplace->SetActorEnableCollision(false);
		if (FAPSWorldScapeFoliagePolicy::HasPendingNativeWorker(DrainedRootToReplace))
		{
			// A native EndPlay would otherwise EnsureCompletion on the game thread.
			return false;
		}
		if (DrainedRootToReplace->WorldScapeLodInGeneration.Num() > 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.WorldScape] Refused runtime root replacement body=%s workers=%d"),
				*GetNameSafe(Body),
				DrainedRootToReplace->WorldScapeLodInGeneration.Num());
			return false;
		}

		DrainedRootToReplace->bGenerateWorldScape = false;
		// AActor::Destroy returns true only after the world accepted destruction and
		// marked the actor pending kill. Do not clear the generator pointer/profile
		// state first: a rejected destroy must not create a hidden orphan root.
		if (!DrainedRootToReplace->Destroy())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldScape] Failed to destroy drained runtime root body=%s root=%s"),
				*GetNameSafe(Body), *GetNameSafe(DrainedRootToReplace));
			return false;
		}
	}

	WorldScapeRootInstance = nullptr;
	bOwnsWorldScapeRootInstance = false;
	CancelPendingSurfaceProfileApply();
	CancelLavaMaterialPreparation();
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	SetActorTickEnabled(false);

	// The resolved noise and material instances are outered to the retired root.
	// Release every cached per-root reference before constructing its replacement.
	bSurfaceProfileApplied = false;
	AppliedSurfaceProfileSignature = 0;
	ResolvedSurfaceProfile = FAPSResolvedPlanetSurfaceProfile{};
	ResolvedNoiseInstance = nullptr;
	ResolvedTerrainMaterialInstance = nullptr;
	ResolvedOceanMaterialInstance = nullptr;
	FinalizedWaterMaterialRoot.Reset();
	FinalizedWaterMaterialProfileSignature = 0;
	PlanetaryBody = nullptr;
	Body->bWorldScapeSurfaceReady = false;

	if (!CreateRuntimeWorldScapeRoot(Body))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape] Failed to create replacement runtime root body=%s"),
			*GetNameSafe(Body));
		return false;
	}
	return true;
}


void APlanetarySurfaceGenerator::StopLavaMaterialPolling()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(LavaMaterialPollTimer);
	bPendingLavaMaterial = false;
}

void APlanetarySurfaceGenerator::CancelLavaMaterialPreparation()
{
	StopLavaMaterialPolling();
	LavaMaterialPreparation.Reset();
	LavaMaterialBody.Reset();
	LavaMaterialRoot.Reset();
	bLavaMaterialTimedOut = false;
	bLavaMaterialFailureLogged = false;
	LavaMaterialRequestTime = 0.0;
	LavaMaterialResumeState = EDeferredWorldScapeRootState::Preloaded;
}

bool APlanetarySurfaceGenerator::DeferProfileForLavaMaterial(APlanetaryBody* Body)
{
	using namespace APSUnifiedLavaSurface;
	const APlanet* Planet = Cast<APlanet>(Body);
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance) || !GetWorld()
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		CancelLavaMaterialPreparation();
		return false;
	}
	if (!IsValid(SurfaceProfileCatalog))
		SurfaceProfileCatalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
			TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	const FAPSResolvedPlanetSurfaceProfile Requested =
		UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body, SurfaceProfileCatalog);
	if (!APSWorldScapeSurfaceEnvelope::Eligible(Requested.LiquidType == EAPSPlanetLiquidType::Lava,
		FMath::IsFinite(Requested.LandCoverage) && Requested.LandCoverage < 0.995f,
		Planet && Planet->IsManual, Body->WorldScapePresentationScale))
	{
		CancelLavaMaterialPreparation();
		return false;
	}
	// Custom authored stacks cannot be converted by the factory. Do not delay
	// them for an unrelated candidate; the existing selection remains authoritative.
	if (IsValid(SurfaceProfileCatalog))
		if (const auto* Definition = SurfaceProfileCatalog->Archetypes.Find(Requested.Archetype))
			if (UMaterialInstance* Terrain = Definition->TerrainMaterial.LoadSynchronous())
				if (!APSSharedTerrainMaterial::IsStockGeneratedTemplate(Terrain))
				{
					CancelLavaMaterialPreparation();
					return false;
				}
	UMaterialInstance* Liquid = APSSharedGeneratedLiquidMaterial::ResolveSource(Requested, SurfaceProfileCatalog);
	if (!APSSharedGeneratedLiquidMaterial::ShouldMigrate(Liquid, Requested, false)
		&& !APSSharedLavaMaterial::IsSharedStack(Liquid))
	{
		CancelLavaMaterialPreparation();
		return false;
	}
	if (!LavaMaterialPreparation || LavaMaterialBody.Get() != Body
		|| LavaMaterialRoot.Get() != WorldScapeRootInstance)
	{
		CancelLavaMaterialPreparation();
		LavaMaterialPreparation = MakeShared<FMaterialPreparation>();
		LavaMaterialBody = Body;
		LavaMaterialRoot = WorldScapeRootInstance;
		LavaMaterialRequestTime = FPlatformTime::Seconds();
		LavaMaterialResumeState = bPendingSurfaceProfileApply ? DeferredWorldScapeRootState
			: WorldScapeRootInstance->bGenerateWorldScape && !WorldScapeRootInstance->bFreezeGeneration
				? EDeferredWorldScapeRootState::Active
				: WorldScapeRootInstance->bGenerateWorldScape && !WorldScapeRootInstance->IsHidden()
					? EDeferredWorldScapeRootState::FrozenVisible : EDeferredWorldScapeRootState::Preloaded;
	}
	// A terminal failure is a publication block, not permission to select another
	// terrain/ocean pair. Keep the existing pending guard visible to lifecycle
	// callers, but never restart a timer or shader poll for this body/root attempt.
	if (bLavaMaterialFailureLogged) return true;
	EPreparationState State = bLavaMaterialTimedOut ? EPreparationState::Failed
		: LavaMaterialPreparation->Poll(GetWorld()->GetFeatureLevel());
	if (State == EPreparationState::Pending && FPlatformTime::Seconds() - LavaMaterialRequestTime > 180.0)
	{
		// Never wait forever or cancel shader jobs shared by another body.
		bLavaMaterialTimedOut = true;
		State = EPreparationState::Failed;
	}
	if (State != EPreparationState::Pending)
	{
		const bool bWasPending = bPendingLavaMaterial;
		StopLavaMaterialPolling();
		if (State == EPreparationState::Failed)
		{
			bPendingLavaMaterial = true;
			bLavaMaterialFailureLogged = true;
			UE_LOG(LogTemp, Error, TEXT("[APS.UnifiedLava.Prepare] body=%s failed=%s; publication blocked, existing profile retained, no material substitution or automatic retry"),
				*GetNameSafe(Body), bLavaMaterialTimedOut ? TEXT("Timeout180s") : LavaMaterialPreparation->GetFailureReason());
			return true;
		}
		else if (bWasPending && State == EPreparationState::Ready)
			UE_LOG(LogTemp, Display, TEXT("[APS.UnifiedLava.Prepare] body=%s ready elapsed=%.3fs (shader readiness, not visual acceptance)"),
				*GetNameSafe(Body), FPlatformTime::Seconds() - LavaMaterialRequestTime);
		return false;
	}
	if (!bPendingLavaMaterial)
	{
		bPendingLavaMaterial = true;
		GetWorld()->GetTimerManager().SetTimer(LavaMaterialPollTimer, this,
			&APlanetarySurfaceGenerator::TryFinalizeLavaMaterial, 0.1f, true);
		UE_LOG(LogTemp, Display, TEXT("[APS.UnifiedLava.Prepare] body=%s pending; existing profile/visibility/collision unchanged"), *GetNameSafe(Body));
	}
	return true;
}

void APlanetarySurfaceGenerator::TryFinalizeLavaMaterial()
{
	if (!bPendingLavaMaterial) return;
	APlanetaryBody* Body = LavaMaterialBody.Get();
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance)
		|| PlanetaryBody != Body || LavaMaterialRoot.Get() != WorldScapeRootInstance)
	{
		CancelLavaMaterialPreparation();
		return;
	}
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		UnloadWorldScapeRoot();
		return;
	}
	// Preloading may still be draining an older batch. Explicit unload cancels
	// this timer outright; a preload drain retains the root and resumes here later.
	if (bPendingWorldScapeUnload) return;
	const EDeferredWorldScapeRootState ResumeState = LavaMaterialResumeState;
	ApplySurfaceProfile(Body); // Re-resolve the latest type/seed/radius before publication.
	if (bPendingLavaMaterial) return;
	if (bPendingSurfaceProfileApply)
	{
		DeferredWorldScapeRootState = ResumeState;
		return;
	}
	if (!IsSurfaceProfileCurrent(Body)) return;
	// Shader readiness permits configuration, not immediate display of the old
	// mesh with a changed profile. The existing geometry visibility gate owns it.
	Body->bWorldScapeSurfaceReady = false;
	if (ResumeState == EDeferredWorldScapeRootState::Active)
	{
		SpawnWorldScapeRoot();
		WorldScapeRootInstance->SetActorHiddenInGame(true);
	}
	else PreloadWorldScapeRoot(); // Never expose old geometry with the new profile.
}

void APlanetarySurfaceGenerator::ApplySurfaceProfile(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance))
	{
		CancelLavaMaterialPreparation();
		return;
	}
	if (bSurfaceProfileApplied)
	{
		const FAPSResolvedPlanetSurfaceProfile RequestedProfile =
			UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body, SurfaceProfileCatalog);
		const bool bRootHasActivatedFoliage = WorldScapeRootInstance->bGenerateFoliages
			&& !WorldScapeRootInstance->Foliages.IsEmpty();
		// WorldScape 5.4 has no public foliage-worker drain. Once either the applied
		// or requested profile opts into foliage, mutating noise, seeds, materials or
		// root-owned transient collections in place is unsafe. A fresh root is the
		// only supported transition boundary for foliage profiles.
		if (FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			ResolvedSurfaceProfile, RequestedProfile, bRootHasActivatedFoliage))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldScape.Foliage] Rejected live profile apply body=%s "
					"appliedSignature=%u requestedSignature=%u; recreate a fresh runtime root"),
				*GetNameSafe(Body), AppliedSurfaceProfileSignature,
				UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(RequestedProfile));
			CancelLavaMaterialPreparation();
			return;
		}
	}

	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		// Gas giants are rendered by their astronomical mesh/material pipeline. Never
		// let a previously selected solid planet leave its WorldScape profile active.
		// Owned roots use the same two-phase worker drain as normal streaming unload;
		// clearing transient noise/material references while SetData is still running
		// would recreate the reported WorldScape access violation.
		UnloadWorldScapeRoot();
		return;
	}

	// Request shaders before stopping any existing geometry producer. A fresh
	// root stays disabled; a published root retains its exact old state.
	if (DeferProfileForLavaMaterial(Body)) return;

	// WorldScape 5.4 workers read the root profile throughout DoWork and write the
	// associated UWorldScapeLod buffers at the end. Changing any regeneration
	// property while that batch is alive makes the next root tick call
	// GenerateBaseMesh -> CleanComponents, invalidating the exact Lod that SetData
	// still owns. Keep the complete old profile immutable and queue only the body
	// model until CheckForLodGeneration has joined the current batch on the game
	// thread. Repeated slider edits merely replace the pending model state.
	if (bPendingSurfaceProfileApply
		|| WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
	{
		QueueSurfaceProfileApply(Body);
		return;
	}

	// A body can return during the final frame of a deferred unload. A worker-free
	// root is safe to reclaim immediately; the caller's streaming-state operation
	// will select its final active/preloaded presentation below this call.
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	ApplySurfaceProfileNow(Body);
}

void APlanetarySurfaceGenerator::ApplySurfaceProfileNow(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance)
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		return;
	}

	if (Body->WorldScapeSeed == 0)
	{
		const uint32 IdentityHash = HashCombine(GetTypeHash(Body->GetFName()), GetTypeHash(Body->GetActorLocation()));
		Body->WorldScapeSeed = 10 + static_cast<int32>(IdentityHash % 999983u);
	}
	if (!IsValid(SurfaceProfileCatalog))
	{
		SurfaceProfileCatalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
			TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	}

	using namespace APSWorldScapeProfiles;
	FSurfaceProfile Profile;
	// PLANET UI exposes one EPlanetType surface pipeline for planets and moons.
	// ResolveForBody is the sole owner of noise/material selection for both.
	const FAPSResolvedPlanetSurfaceProfile RequestedProfile =
		UAPSPlanetSurfaceProfileResolver::ResolveForBody(
		Body, SurfaceProfileCatalog);
	const bool bRootHasActivatedFoliage = WorldScapeRootInstance->bGenerateFoliages
		&& !WorldScapeRootInstance->Foliages.IsEmpty();
	if (bSurfaceProfileApplied
		&& FAPSWorldScapeFoliagePolicy::RequiresFreshRootForProfileTransition(
			ResolvedSurfaceProfile, RequestedProfile, bRootHasActivatedFoliage))
	{
		// Defense in depth for deferred/internal callers that bypass ApplySurfaceProfile.
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape.Foliage] Rejected internal live profile mutation body=%s"),
				*GetNameSafe(Body));
		return;
	}
	if (DeferProfileForLavaMaterial(Body)) return;
	// Validate/create the authoritative material before changing any Resolved
	// member. Even a failure after preparation reported Ready must retain the
	// published profile, rather than commit SharedMagma plus a separate lava mesh.
	TStrongObjectPtr<UMaterialInstanceDynamic> PreparedUnified;
	if (LavaMaterialPreparation && LavaMaterialBody.Get() == Body
		&& LavaMaterialRoot.Get() == WorldScapeRootInstance)
	{
		const double Radius = APSPlanetSurfaceRadius::Kilometres(Body->RadiusKM, Body->PlanetRadiusKM) * 100000.0;
		APSUnifiedLavaSurface::ECreateFailure Failure = APSUnifiedLavaSurface::ECreateFailure::None;
		PreparedUnified.Reset(APSUnifiedLavaSurface::Create(WorldScapeRootInstance,
			WorldScapeRootInstance->GetRootComponent(), RequestedProfile, Radius,
			GetWorld()->GetFeatureLevel(), &Failure));
		if (!PreparedUnified.IsValid())
		{
			StopLavaMaterialPolling();
			bPendingLavaMaterial = true;
			bLavaMaterialFailureLogged = true;
			UE_LOG(LogTemp, Error,
				TEXT("[APS.UnifiedLava.Prepare] body=%s failed=Create:%s; publication blocked before profile mutation, existing profile retained, no material substitution or automatic retry"),
				*GetNameSafe(Body), APSUnifiedLavaSurface::FailureName(Failure));
			return;
		}
	}
	FinalizedWaterMaterialRoot.Reset();
	FinalizedWaterMaterialProfileSignature = 0;
	ResolvedSurfaceProfile = RequestedProfile;
	{
		UMaterialInstance* BaseTerrainMaterial = nullptr;
		if (IsValid(SurfaceProfileCatalog))
		{
			if (const FAPSPlanetSurfaceArchetypeDefinition* AuthoredDefinition =
				SurfaceProfileCatalog->Archetypes.Find(ResolvedSurfaceProfile.Archetype))
			{
				if (UMaterialInstance* AuthoredMaterial = AuthoredDefinition->TerrainMaterial.LoadSynchronous())
				{
					BaseTerrainMaterial = AuthoredMaterial;
				}
			}
		}
		if (!IsValid(BaseTerrainMaterial))
		{
			const TCHAR* FallbackTerrainPath =
				ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Temperate
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Temperate.MI_APS_WS_Temperate")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Oceanic
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Oceanic.MI_APS_WS_Oceanic")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Biosphere
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Biosphere.MI_APS_WS_Biosphere")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Desert
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Desert.MI_APS_WS_Desert")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Cryogenic
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Cryogenic.MI_APS_WS_Cryogenic")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Magmatic
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Magmatic.MI_APS_WS_Magmatic")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::Metallic
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Metallic.MI_APS_WS_Metallic")
				: ResolvedSurfaceProfile.Archetype == EAPSPlanetSurfaceArchetype::ExoticChemical
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_ExoticChemical.MI_APS_WS_ExoticChemical")
					: TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Rocky.MI_APS_WS_Rocky");
			BaseTerrainMaterial = LoadObject<UMaterialInstance>(nullptr, FallbackTerrainPath);
		}

		const APlanet* Planet = Cast<APlanet>(Body);
		if (!(Planet && Planet->IsManual)
			&& APSSharedTerrainMaterial::IsStockGeneratedTemplate(BaseTerrainMaterial))
		{
			const TCHAR* SharedPath = APSSharedTerrainMaterial::TemplatePath(ResolvedSurfaceProfile);
			UMaterialInstance* SharedTemplate = LoadObject<UMaterialInstance>(nullptr, SharedPath);
			if (APSSharedTerrainMaterial::IsExactTemplate(SharedTemplate, ResolvedSurfaceProfile))
			{
				BaseTerrainMaterial = SharedTemplate;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.SharedTerrain] Profile parent selected body=%s template=%s presentationScale=%.9g (binding only, not visual acceptance)"),
					*GetNameSafe(Body), SharedPath, Body->WorldScapePresentationScale);
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.SharedTerrain] Required template missing/invalid body=%s path=%s; publication blocked, no material substitution"),
					*GetNameSafe(Body), SharedPath);
				return;
			}
		}

		ResolvedNoiseInstance = NewObject<UAPSWorldScapePlanetNoise>(
			WorldScapeRootInstance, NAME_None, RF_Transient);
		if (IsValid(ResolvedNoiseInstance))
		{
			bool bCoastalCandidate = false;
#if WITH_DEV_AUTOMATION_TESTS
			// Opt-in isolated evidence run only. No production or saved-profile default.
			bCoastalCandidate = FParse::Param(FCommandLine::Get(), TEXT("APSProbeCoastalReliefV1"));
#endif
			ResolvedNoiseInstance->Configure(ResolvedSurfaceProfile, false, bCoastalCandidate);
			Profile.Noise = ResolvedNoiseInstance;
		}

		ResolvedTerrainMaterialInstance = IsValid(BaseTerrainMaterial)
			? UMaterialInstanceDynamic::Create(BaseTerrainMaterial, WorldScapeRootInstance)
			: nullptr;
		if (IsValid(ResolvedTerrainMaterialInstance))
		{
			UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
				ResolvedTerrainMaterialInstance, ResolvedSurfaceProfile);
			if (APSSharedTerrainMaterial::IsSharedStack(ResolvedTerrainMaterialInstance))
			{
				if (!APSSharedTerrainMaterial::BindNewInstanceFrame(ResolvedTerrainMaterialInstance,
					WorldScapeRootInstance->GetRootComponent(), Body->WorldScapePresentationScale))
				{
					UE_LOG(LogTemp, Error,
						TEXT("[APS.SharedTerrain] Invalid physical frame body=%s; surface profile publication withheld"), *GetNameSafe(Body));
					return;
				}
			}
			else
			{
				APSNativeTerrainMaterial::BindNewInstanceCenter(ResolvedTerrainMaterialInstance,
					WorldScapeRootInstance->GetRootComponent());
			}
			Profile.TerrainMaterial = ResolvedTerrainMaterialInstance;
		}

		UMaterialInstance* BaseOceanMaterial = nullptr;
		// The catalog is keyed by broad archetype, while a concrete preset may
		// deliberately remove that archetype's liquid (Exoplanet is the important
		// case). Do not carry an unused ammonia/water material into a dry body's root.
		if (ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			&& IsValid(SurfaceProfileCatalog))
		{
			if (const FAPSPlanetSurfaceArchetypeDefinition* AuthoredDefinition =
				SurfaceProfileCatalog->Archetypes.Find(ResolvedSurfaceProfile.Archetype))
			{
				// Subtype/modifier resolution may change the broad archetype's liquid
				// (for example an Oasis modifier on an otherwise dry family). Only use
				// the catalog material when its authored liquid still matches the body.
				if (AuthoredDefinition->LiquidType == ResolvedSurfaceProfile.LiquidType)
				{
					if (UMaterialInstance* AuthoredOcean =
						AuthoredDefinition->OceanMaterial.LoadSynchronous())
					{
						BaseOceanMaterial = AuthoredOcean;
					}
				}
			}
		}
		if (!IsValid(BaseOceanMaterial) && ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None)
		{
			const TCHAR* FallbackOceanPath = ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Lava
				? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava")
				: ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Ammonia
					? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia")
					: TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water.MI_APS_WS_Water");
			BaseOceanMaterial = LoadObject<UMaterialInstance>(nullptr, FallbackOceanPath);
		}
		const bool bUseSharedLiquid = APSSharedGeneratedLiquidMaterial::ShouldMigrate(
			BaseOceanMaterial, ResolvedSurfaceProfile, Planet && Planet->IsManual);
		ResolvedOceanMaterialInstance = bUseSharedLiquid
			? APSSharedGeneratedLiquidMaterial::Create(WorldScapeRootInstance, WorldScapeRootInstance->GetRootComponent(),
				Body->WorldScapePresentationScale, false, false, ResolvedSurfaceProfile, BaseOceanMaterial)
			: IsValid(BaseOceanMaterial)
				? UMaterialInstanceDynamic::Create(BaseOceanMaterial, WorldScapeRootInstance) : nullptr;
		if (bUseSharedLiquid && !IsValid(ResolvedOceanMaterialInstance))
		{
			// Do not silently commit a dry planet or a different liquid parent when the
			// protected shared asset/frame is unavailable. Existing proxies stay owned
			// by their last complete root/closed-globe commit while readiness is false.
			bSurfaceProfileApplied = false;
			AppliedSurfaceProfileSignature = 0;
			UE_LOG(LogTemp, Error,
				TEXT("[APS.SharedLiquid] Invalid saved asset/frame body=%s type=%s; surface profile publication withheld"),
				*GetNameSafe(Body), *UEnum::GetValueAsString(ResolvedSurfaceProfile.LiquidType));
			return;
		}
		if (IsValid(ResolvedOceanMaterialInstance))
		{
			// Liquid style is authored once in the project MIC / WorldScape template
			// chain generated by APSPlanetSurfaceAssetCommandlet. Runtime must not add
			// project-only controls to that plugin graph (or multiply lava emission).
			Profile.OceanMaterial = ResolvedOceanMaterialInstance;
			if (!bUseSharedLiquid)
				APSNativeTerrainMaterial::BindNewInstanceCenter(ResolvedOceanMaterialInstance,
					WorldScapeRootInstance->GetRootComponent());
			if ((bUseSharedLiquid || (ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water
				&& Body->WorldScapePresentationScale >= 0.999)) && GetWorld()->IsGameWorld())
			{
				FMaterialResource* Resource = ResolvedOceanMaterialInstance->GetMaterialResource(
					GetWorld()->GetFeatureLevel());
#if WITH_EDITOR
				if (Resource && !Resource->IsGameThreadShaderMapComplete())
				{
					Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
				}
#endif
				if (bUseSharedLiquid)
				{
					FPSOPrecacheParams Params;
					Params.bStaticLighting = false;
					Params.bCastShadow = true;
					Params.SetMobility(EComponentMobility::Movable);
					static_cast<UMaterialInterface*>(ResolvedOceanMaterialInstance)->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
				}
				UE_LOG(LogTemp, Log,
					TEXT("[APS.WorldScape.Liquid] Preparing type=%s parent=%s resource=%d complete=%d nativeCenter=%d sharedLiquid=%d context=%d"),
					*UEnum::GetValueAsString(ResolvedSurfaceProfile.LiquidType),
					*GetPathNameSafe(ResolvedOceanMaterialInstance->Parent), Resource ? 1 : 0,
					Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
					APSNativeTerrainMaterial::UsesNativePlanetCenter(ResolvedOceanMaterialInstance) ? 1 : 0,
					bUseSharedLiquid ? 1 : 0, bUseSharedLiquid ? 0 : -1);
			}
		}

		Profile.NoiseScale = ResolvedSurfaceProfile.NoiseScale;
		Profile.NoiseIntensity = ResolvedSurfaceProfile.NoiseIntensity;
		Profile.bOcean = ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			&& ResolvedSurfaceProfile.LandCoverage < 0.995f
			&& IsValid(Profile.OceanMaterial);
		Profile.OceanHeight = ResolvedSurfaceProfile.OceanLevel * ResolvedSurfaceProfile.NoiseIntensity;
		if (PreparedUnified.IsValid() && IsValid(ResolvedNoiseInstance))
		{
			UMaterialInstanceDynamic* Unified = PreparedUnified.Get();
			if (IsValid(Unified))
			{
				// Publish all three parts together, before any new worker starts. A
				// missing material must NEVER flatten terrain or remove the old lava.
				ResolvedNoiseInstance->Configure(ResolvedSurfaceProfile, true);
				ResolvedTerrainMaterialInstance = Unified;
				Profile.TerrainMaterial = Unified;
				Profile.bOcean = false;
				Profile.OceanMaterial = nullptr;
				ResolvedOceanMaterialInstance = nullptr;
			}
			UE_LOG(LogTemp, Warning, TEXT("[APS.UnifiedLava] body=%s installed=%d ocean=%d (candidate; visual validation required)"),
				*GetNameSafe(Body), IsValid(Unified) ? 1 : 0, Profile.bOcean ? 1 : 0);
		}
	}
	if (!IsValid(Profile.Noise) || !IsValid(Profile.TerrainMaterial))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldScape] Surface profile is incomplete for %s: noise=%s terrain=%s"),
			*GetNameSafe(Body), *GetNameSafe(Profile.Noise), *GetNameSafe(Profile.TerrainMaterial));
		WorldScapeRootInstance->bGenerateWorldScape = false;
		WorldScapeRootInstance->bFreezeGeneration = true;
		WorldScapeRootInstance->SetActorHiddenInGame(true);
		WorldScapeRootInstance->SetActorEnableCollision(false);
		bSurfaceProfileApplied = false;
		AppliedSurfaceProfileSignature = 0;
		return;
	}

	const double PresentationScale = FMath::Clamp(Body->WorldScapePresentationScale, 1.0e-9, 1.0);
	const double BodyRadiusCm = APSPlanetSurfaceRadius::Kilometres(Body->RadiusKM, Body->PlanetRadiusKM)
		* 100000.0 * PresentationScale;
	WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
	WorldScapeRootInstance->PlanetScale = FMath::Max(BodyRadiusCm, 100000.0);
	WorldScapeRootInstance->DistanceToFreezeGeneration = FMath::Max(
		Body->GetWorldScapeActivationRadiusCm() - WorldScapeRootInstance->PlanetScale,
		WorldScapeRootInstance->PlanetScale * 2.0);
	WorldScapeRootInstance->WorldScapeNoise = Profile.Noise;
	WorldScapeRootInstance->TerrainMaterial.DefaultMaterial = Profile.TerrainMaterial;
	WorldScapeRootInstance->OceanMaterial.DefaultMaterial = Profile.OceanMaterial;
	WorldScapeRootInstance->bOcean = Profile.bOcean && IsValid(Profile.OceanMaterial);
	WorldScapeRootInstance->OceanHeight = Profile.OceanHeight * PresentationScale;
	// WorldScape 5.4 stores both previous values as int32 even though the public
	// properties are float. Any fractional value compares unequal on every tick,
	// calls GenerateBaseMesh again, and continuously destroys/recreates the LODs.
	// That was the direct cause of the preview placeholder, 30 FPS entry hitch and
	// the SetData crash window. Keep both values integral at the plugin boundary.
	WorldScapeRootInstance->NoiseScale = FMath::Max(
		1.0f, static_cast<float>(FMath::RoundToInt(Profile.NoiseScale)));
	WorldScapeRootInstance->NoiseIntensity = FMath::Max(
		1.0f, static_cast<float>(FMath::RoundToInt(Profile.NoiseIntensity * PresentationScale)));
	WorldScapeRootInstance->Seed = Body->WorldScapeSeed;
	// WorldScape 5.4's CustomNoise constructor compares its requested seed against
	// an uninitialised private Seed member before building the permutation tables.
	// Force one different value first, then the authoritative body seed. This runs
	// on the game thread before any LOD workers are launched and guarantees that a
	// preset switch cannot accidentally retain an all-zero/stale noise table.
	const int32 SeedPrimer = Body->WorldScapeSeed == MAX_int32
		? Body->WorldScapeSeed - 1 : Body->WorldScapeSeed + 1;
	WorldScapeRootInstance->PlanetNoise.SetSeed(SeedPrimer);
	WorldScapeRootInstance->PlanetNoise.SetSeed(Body->WorldScapeSeed);
	// WorldScape's default eight LODs cover only a small square around the pawn on
	// a full-scale planet. From low orbit that square has visible straight edges.
	// Extra coarse rings extend the mesh beyond the geometric horizon while the
	// single-active-body streaming budget keeps the cost bounded.
	// Start with a bounded near-body budget. The former 13x160 terrain plus 13x96
	// ocean request saturated the WorldScape worker on first focus and reproduced
	// the reported 8-30 FPS stall before even one usable patch could be shown. The
	// normalized menu globe is identifiable by its presentation scale and uses the
	// same lighter budget that its preview owner confirms after profile application.
	const bool bScaledOrbitalPreview = PresentationScale < 0.999;
	// Configure only a newly-created root. WorldScape 5.4 exposes no public drain
	// for its foliage worker, so hot-swapping collections on a previously active
	// root is intentionally outside this foundation. The sparse default is limited
	// to validated generated planets; manual/moon and scaled roots are vetoed.
	if (bOwnsWorldScapeRootInstance && !bSurfaceProfileApplied)
	{
		const APlanet* FoliagePlanet = Cast<APlanet>(Body);
		FAPSWorldScapeFoliagePolicy::ApplyToFreshOwnedRuntimeRoot(
			WorldScapeRootInstance, ResolvedSurfaceProfile, bScaledOrbitalPreview, !FoliagePlanet || FoliagePlanet->IsManual);
	}
	if (IsValid(ResolvedTerrainMaterialInstance) && IsValid(WorldScapeRootInstance))
	{
		const double SeedPhase = static_cast<double>(ResolvedSurfaceProfile.TerrainSeed % 104729)
			* 0.000137;
		ResolvedTerrainMaterialInstance->SetVectorParameterValue(
			TEXT("OrbitalSeedOffset"),
			FLinearColor(
				static_cast<float>(3.0 + FMath::Sin(SeedPhase * 17.0 + 0.31) * 7.0),
				static_cast<float>(9.0 + FMath::Cos(SeedPhase * 11.0 + 1.27) * 7.0),
				static_cast<float>(15.0 + FMath::Sin(SeedPhase * 7.0 + 2.13) * 7.0),
				0.0f));
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(
			TEXT("OrbitalFeatureScale"),
			FMath::Clamp(2.45f * ResolvedSurfaceProfile.ContinentalFrequencyMultiplier,
				1.35f, 6.0f));
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(
			TEXT("OrbitalDetailScale"),
			FMath::Clamp(7.0f * ResolvedSurfaceProfile.RegionalFrequencyMultiplier,
				3.5f, 18.0f));
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(
			TEXT("OrbitalPresentationBlend"), bScaledOrbitalPreview ? 1.0f : 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(
			TEXT("OrbitalNormalBlend"), bScaledOrbitalPreview ? 1.0f : 0.0f);
	}
	// The canonical terrain graph contains centimetre-scale detail for walkable
	// full-scale gameplay. On the normalized menu globe the complete planet is only
	// a few kilometres wide, so those same bands undersample against WorldScape's
	// coarse orbital LODs. Keep the authoritative mesh and macro/meso palette intact,
	// but disable only terrain near-field shading bands for scaled presentation. The
	// physical liquid remains the independently-authored WorldScape plugin material.
	if (bScaledOrbitalPreview && IsValid(ResolvedTerrainMaterialInstance))
	{
		// The WorldScape geometry remains authoritative.  Only its shading normal is
		// represented as one continuous sphere from orbit, preventing independently
		// streamed LOD patches from reading as a dark square grid.  Full-scale roots
		// keep OrbitalNormalBlend=0 and use their physical terrain normals unchanged.
		// The displaced WorldScape geometry and silhouette remain real; only its orbital
		// shading normal is unified so independently-normalised patch seams cannot flash.
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("OrbitalNormalBlend"), 1.0f);
		// Temperature/humidity are vertex payloads too.  A tiny amount preserves broad
		// climate identity without outlining the coarse orbital mesh topology.
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("ClimateBlend"), 0.018f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("NearColorStrength"), 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("MacroColorStrength"), 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("MesoColorStrength"), 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("DetailNormalStrength"), 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("DetailRoughnessStrength"), 0.0f);
		ResolvedTerrainMaterialInstance->SetScalarParameterValue(TEXT("MesoRoughnessStrength"), 0.0f);
	}
	WorldScapeRootInstance->MaxLod = bScaledOrbitalPreview ? 6 : 10;
	WorldScapeRootInstance->LodResolution = APSWorldScapeLiquidLattice::TerrainResolution(
		bOwnsWorldScapeRootInstance, bScaledOrbitalPreview,
		APSWorldScapeProfiles::CVarSurfaceMeshResolution.GetValueOnGameThread());
	const APlanet* CoastPlanet = Cast<APlanet>(Body);
	WorldScapeRootInstance->LodResolution = APSWorldScapeLiquidLattice::CoastResolution(
		WorldScapeRootInstance->LodResolution, bOwnsWorldScapeRootInstance && !(CoastPlanet && CoastPlanet->IsManual),
		bScaledOrbitalPreview, ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			&& ResolvedSurfaceProfile.LandCoverage < 0.995f,
		APSWorldScapeProfiles::CVarCoastResolution.GetValueOnGameThread());
	WorldScapeRootInstance->TriangleSize = bScaledOrbitalPreview ? 450.0f : 120.0f;
	WorldScapeRootInstance->OceanMaxLod = bScaledOrbitalPreview ? 6 : 9;
	WorldScapeRootInstance->OceanLodResolution = bScaledOrbitalPreview ? 32 : 64;
	WorldScapeRootInstance->OceanTriangleSize = bScaledOrbitalPreview ? 650.0f : 200.0f;
	// Lava and exotic oceans need the same lattice guarantee as water: material
	// colour/emission cannot repair intersections between unequal spherical chords.
	// Matching the ocean does not alter collision, authored roots or scaled previews.
	APSWorldScapeLiquidLattice::MatchTerrain(*WorldScapeRootInstance,
		bOwnsWorldScapeRootInstance, bScaledOrbitalPreview);
	// Keep the gameplay collision sample spacing identical to terrain LOD0 so the
	// pawn does not walk on the visibly smoother 2 m default collision sheet. A
	// 64x64 padded patch gives roughly 74 m of full-scale coverage around each
	// collision dependant actor without multiplying every visual LOD's vertex cost.
	// Orbital preview roots never need collision and retain the inexpensive plugin
	// defaults until AstroGenerator applies its presentation-only budget.
	WorldScapeRootInstance->bGenerateCollision = !bScaledOrbitalPreview;
	WorldScapeRootInstance->bPaddedCollision = true;
	WorldScapeRootInstance->CollisionResolution = bScaledOrbitalPreview ? 16 : 64;
	WorldScapeRootInstance->CollisionTriangleSize = bScaledOrbitalPreview
		? 200.0f : WorldScapeRootInstance->TriangleSize;
	// These properties are copied by WorldScape 5.4 into every generated terrain
	// mesh. Contact and dynamic shadows are the near-field relief cues required by
	// gameplay; static/far shadows keep the same surface coherent at wider views.
	// Two-sided shadows stay disabled because the terrain is a closed outward-facing
	// planet shell. Accurate tangents remain at the plugin default: its API documents
	// that option as greatly slowing generation, so it is not safe for this fix.
	WorldScapeRootInstance->TerrainContactShadow = !bScaledOrbitalPreview;
	WorldScapeRootInstance->TerrainCastStaticShadow = !bScaledOrbitalPreview;
	WorldScapeRootInstance->TerrainCastDynamicShadow = !bScaledOrbitalPreview;
	WorldScapeRootInstance->TerrainFarShadow = !bScaledOrbitalPreview;
	WorldScapeRootInstance->TerrainTowSideShadow = false;
	// Sewing duplicates physical vertices across three sections. Opt only owned,
	// spherical, non-tangent generated ground into the native area-normal weld.
	// Authored roots and ocean sections keep their original shading contract.
	const FName CoincidentNormalTag(TEXT("APS.GeneratedTerrain.WeldCoincidentNormals"));
	if (bOwnsWorldScapeRootInstance && !bScaledOrbitalPreview
		&& WorldScapeRootInstance->GenerationType == EWorldScapeType::Planet
		&& !WorldScapeRootInstance->bGenerateTangents)
		WorldScapeRootInstance->Tags.AddUnique(CoincidentNormalTag);
	else
		WorldScapeRootInstance->Tags.Remove(CoincidentNormalTag);
	// The native sampler is immutable while this root generates; its existing LOD
	// workers already share it. Allow bounded parallel collision samples too, without
	// coarsening physics or moving component publication off the game thread. The
	// plugin falls back to serial for volumes/heightmaps and accepts a 1-task rollback.
	const FName ParallelCollisionTag(TEXT("APS.Collision.ParallelSamples"));
	if (bOwnsWorldScapeRootInstance && !bScaledOrbitalPreview
		&& IsValid(ResolvedNoiseInstance) && Profile.Noise == ResolvedNoiseInstance
		&& ResolvedNoiseInstance->GetClass() == UAPSWorldScapePlanetNoise::StaticClass())
		WorldScapeRootInstance->Tags.AddUnique(ParallelCollisionTag);
	else
		WorldScapeRootInstance->Tags.Remove(ParallelCollisionTag);
	// Bounded rollout of worker-prepared visual publication. The native default
	// (-1) requires this explicit owner tag; 0 restores the legacy publication.
	// Keep other presets, authored roots and scaled previews on their old path.
	const FName PreparedPublicationTag(TEXT("APS.Mesh.PreparedPublication"));
	// Rio 08.10 night (0.6.1: "the ice planet: zero freezes; other planets freeze"): every family. Packaged A/B descents,
	// frames >= 50 ms without -> with: ocean 8/8/5 -> 0/4/0, forest 9 -> 1, near-surface frames equal (n-fam-*,
	// night_0810). Rio 08.10 afternoon: no exceptions, the lava stack (Volcanic / Lava / Melted) too, every family A/B'd.
	// aps.Surface.PreparedAllFamilies 0 restores the 30.09 list.
	const EPlanetType PreparedType = ResolvedSurfaceProfile.PlanetType;
	const bool bPreparedPublicationType = PreparedType == EPlanetType::Terrestrial
		|| PreparedType == EPlanetType::Frozen
		|| PreparedType == EPlanetType::Oasis
		|| APSWorldScapeProfiles::CVarPreparedAllFamilies.GetValueOnGameThread() != 0;
	if (bPreparedPublicationType && WorldScapeRootInstance->ActorHasTag(ParallelCollisionTag)
		&& !(CoastPlanet && CoastPlanet->IsManual))
		WorldScapeRootInstance->Tags.AddUnique(PreparedPublicationTag);
	else
		WorldScapeRootInstance->Tags.Remove(PreparedPublicationTag);
	UE_LOG(LogTemp, Log, TEXT("[APS.Surface] publication body=%s type=%s prepared=%d"), *GetNameSafe(Body),
		*UEnum::GetValueAsString(PreparedType), WorldScapeRootInstance->ActorHasTag(PreparedPublicationTag) ? 1 : 0);
	// Depth accompanies the versioned material; diagnostics can still opt in.
	// The native LOD worker owns UV1; no per-frame GT bathymetry or mesh rewrite.
	// Do not enable for authored/compressed roots or other liquid families.
	static const bool bRequestWaterDepth = FParse::Param(FCommandLine::Get(), TEXT("APSWaterDepthPayload"));
	const FName WaterDepthTag(TEXT("APS.GeneratedOcean.BathymetryUV1"));
	const bool bWaterDepth = (bRequestWaterDepth || APSCoastalWaterMaterial::IsInstance(ResolvedOceanMaterialInstance))
		&& bOwnsWorldScapeRootInstance
		&& !bScaledOrbitalPreview && !(CoastPlanet && CoastPlanet->IsManual)
		&& FMath::IsNearlyEqual(PresentationScale, 1.0, 1.e-12)
		&& WorldScapeRootInstance->GenerationType == EWorldScapeType::Planet
		&& WorldScapeRootInstance->bOcean && IsValid(ResolvedNoiseInstance)
		&& Profile.Noise == ResolvedNoiseInstance
		&& ResolvedNoiseInstance->GetClass() == UAPSWorldScapePlanetNoise::StaticClass()
		&& APSSharedGeneratedLiquidMaterial::IsFamilyInstance(
			ResolvedOceanMaterialInstance, EAPSPlanetLiquidType::Water);
	if (bWaterDepth) WorldScapeRootInstance->Tags.AddUnique(WaterDepthTag);
	else WorldScapeRootInstance->Tags.Remove(WaterDepthTag);
	// A streamed ocean is a colour/depth presentation shell, never a shadow caster
	// or an occlusion source.  WorldScape's defaults otherwise let independently
	// stitched clipmap sections cast their rectangular boundaries onto the terrain,
	// reproducing the reported raised/grid-like "water" despite a perfectly constant
	// ocean radius.  The ocean remains opaque/depth-writing, but cannot project its
	// section topology into the scene lighting or HZB.
	WorldScapeRootInstance->OceanMeshIsOccluder = false;
	WorldScapeRootInstance->OceanMeshTreatAsBackGroundForOcclusion = false;
	// Match the saved manual root's native UE5.4 HeightAnchor (100 m).
	// WorldScape grows triangle spacing by 2^round(log2(height / anchor)).
	// The radius-based 0.5-2.5 km override kept a small, dense square under the
	// observer at altitude. Restore earlier expansion without adding vertices,
	// LOD rings or collision work. Ground-level 1.2 m spacing stays unchanged.
	WorldScapeRootInstance->HeightAnchor = bScaledOrbitalPreview
		? FMath::Clamp(static_cast<float>(WorldScapeRootInstance->PlanetScale * 0.00025),
			50000.0f, 250000.0f)
		: 10000.0f;
	bSurfaceProfileApplied = true;
	AppliedSurfaceProfileSignature = BuildSurfaceProfileSignature(Body);

	// Candidate owner survives root retirement. No material is selected/replaced;
	// the disabled-by-default path subscribes this already-created native MID.
	if (APSPlanetReliefRuntime::IsEnabled())
	{
		APSClosedGlobeMesh::FSamplingFrame ReliefFrame;
		APSClosedGlobeMesh::FBuildOptions ReliefOptions;
		uint32 ReliefIdentity=0; FString ReliefError;
		if (APSNativeGlobeSnapshot::Capture(Body,this,ReliefFrame,ReliefOptions,ReliefIdentity,ReliefError))
			APSPlanetReliefRuntime::Register(Body,ReliefFrame,ReliefIdentity,ResolvedTerrainMaterialInstance);
	}

	const FString SurfaceSubtype = UEnum::GetValueAsString(Body->PlanetType);
	const FString SurfaceArchetype = UEnum::GetValueAsString(ResolvedSurfaceProfile.Archetype);
	// One-shot profile diagnostic. This runs only when a body profile is applied,
	// never from the per-frame surface readiness/streaming paths.
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldScape] Profile body=%s subtype=%s archetype=%s ocean=%s seed=%d "
			"noiseScale=%.0f noiseIntensity=%.0f presentationScale=%.6e planetScale=%.6e "
			"mode=%s terrainLod=%dx%d@%.0f oceanLod=%dx%d@%.0f "
			"collision=%s/%dx%d@%.0f padded=%s tangents=%s heightAnchor=%.0f "
			"shadows=contact:%s/static:%s/dynamic:%s/far:%s/twoSided:%s noise=%s terrain=%s"),
		*Body->GetName(), *SurfaceSubtype, *SurfaceArchetype,
		WorldScapeRootInstance->bOcean ? TEXT("true") : TEXT("false"), Body->WorldScapeSeed,
		WorldScapeRootInstance->NoiseScale, WorldScapeRootInstance->NoiseIntensity,
		PresentationScale, WorldScapeRootInstance->PlanetScale,
		bScaledOrbitalPreview ? TEXT("scaled-preview") : TEXT("full-scale-gameplay"),
		WorldScapeRootInstance->MaxLod, WorldScapeRootInstance->LodResolution,
		WorldScapeRootInstance->TriangleSize,
		WorldScapeRootInstance->OceanMaxLod, WorldScapeRootInstance->OceanLodResolution,
		WorldScapeRootInstance->OceanTriangleSize,
		WorldScapeRootInstance->bGenerateCollision ? TEXT("runtime") : TEXT("disabled"),
		WorldScapeRootInstance->CollisionResolution, WorldScapeRootInstance->CollisionResolution,
		WorldScapeRootInstance->CollisionTriangleSize,
		WorldScapeRootInstance->bPaddedCollision ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->bGenerateTangents ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->HeightAnchor,
		WorldScapeRootInstance->TerrainContactShadow ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->TerrainCastStaticShadow ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->TerrainCastDynamicShadow ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->TerrainFarShadow ? TEXT("true") : TEXT("false"),
		WorldScapeRootInstance->TerrainTowSideShadow ? TEXT("true") : TEXT("false"),
		*GetNameSafe(Profile.Noise), *GetNameSafe(Profile.TerrainMaterial));
}

void APlanetarySurfaceGenerator::PreloadWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
	}
	if (bPendingLavaMaterial)
	{
		LavaMaterialResumeState = EDeferredWorldScapeRootState::Preloaded;
		if (!bSurfaceProfileApplied) return;
	}
	if (bPendingSurfaceProfileApply)
	{
		DeferredWorldScapeRootState = EDeferredWorldScapeRootState::Preloaded;
		HoldWorldScapeRootForProfileDrain();
		return;
	}
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorEnableCollision(false);
	if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
	{
		// Preloading can be requested while an earlier active state is still finishing.
		// Freeze the producer and let this generator poll CheckForLodGeneration. This
		// consumes the current batch without allowing UpdatePosition to enqueue another.
		bPendingWorldScapeUnload = true;
		bDestroyWorldScapeRootAfterDrain = false;
		WorldScapeRootInstance->bGenerateWorldScape = true;
		WorldScapeRootInstance->bFreezeGeneration = true;
		WorldScapeRootInstance->SetActorTickEnabled(false);
		SetActorTickEnabled(true);
		return;
	}
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	SetActorTickEnabled(false);
	WorldScapeRootInstance->bGenerateWorldScape = false;
	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->SetActorTickEnabled(false);
}

void APlanetarySurfaceGenerator::FreezeWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
	}
	if (bPendingLavaMaterial)
	{
		LavaMaterialResumeState = EDeferredWorldScapeRootState::FrozenVisible;
		if (!bSurfaceProfileApplied) return;
	}
	if (bPendingSurfaceProfileApply)
	{
		DeferredWorldScapeRootState = EDeferredWorldScapeRootState::FrozenVisible;
		HoldWorldScapeRootForProfileDrain();
		return;
	}
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	SetActorTickEnabled(false);
	WorldScapeRootInstance->bGenerateWorldScape = true;
	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->SetActorHiddenInGame(false);
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorEnableCollision(false);
}

void APlanetarySurfaceGenerator::UnloadWorldScapeRoot()
{
	CancelLavaMaterialPreparation();
	// Unload has stronger lifetime semantics than a queued edit. Cancel the edit
	// request but retain its old profile references until the common worker drain
	// below is complete.
	CancelPendingSurfaceProfileApply();
	if (!IsValid(WorldScapeRootInstance))
	{
		WorldScapeRootInstance = nullptr;
		bOwnsWorldScapeRootInstance = false;
		bSurfaceProfileApplied = false;
		AppliedSurfaceProfileSignature = 0;
		ResolvedNoiseInstance = nullptr;
		ResolvedTerrainMaterialInstance = nullptr;
		ResolvedOceanMaterialInstance = nullptr;
		FinalizedWaterMaterialRoot.Reset();
		FinalizedWaterMaterialProfileSignature = 0;
		bPendingWorldScapeUnload = false;
		bDestroyWorldScapeRootAfterDrain = false;
		SetActorTickEnabled(false);
		return;
	}
	// A borrowed/manual root cannot be destroyed here, but it is just as unsafe to
	// disable it while a worker owns one of its LOD components. Both ownership modes
	// therefore share the drain; finalization decides whether to destroy or retain.
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorEnableCollision(false);
	bPendingWorldScapeUnload = true;
	bDestroyWorldScapeRootAfterDrain = bOwnsWorldScapeRootInstance;
	if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0
		|| FAPSWorldScapeFoliagePolicy::HasPendingNativeWorker(WorldScapeRootInstance))
	{
		// Do not use the WorldScape actor tick for draining. Its tick runs
		// UpdatePosition before CheckForLodGeneration and can start a fresh batch in
		// the frame after the old one completes. Freeze it and consume only results.
		WorldScapeRootInstance->bGenerateWorldScape = true;
		WorldScapeRootInstance->bFreezeGeneration = true;
		WorldScapeRootInstance->SetActorTickEnabled(false);
		SetActorTickEnabled(true);
		return;
	}
	TryFinalizeWorldScapeUnload();
}

void APlanetarySurfaceGenerator::QueueSurfaceProfileApply(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance))
	{
		return;
	}

	const bool bWasAlreadyPending = bPendingSurfaceProfileApply;
	if (!bWasAlreadyPending)
	{
		if (WorldScapeRootInstance->bGenerateWorldScape
			&& !WorldScapeRootInstance->bFreezeGeneration)
		{
			DeferredWorldScapeRootState = EDeferredWorldScapeRootState::Active;
		}
		else if (WorldScapeRootInstance->bGenerateWorldScape
			&& !WorldScapeRootInstance->IsHidden())
		{
			DeferredWorldScapeRootState = EDeferredWorldScapeRootState::FrozenVisible;
		}
		else
		{
			DeferredWorldScapeRootState = EDeferredWorldScapeRootState::Preloaded;
		}
	}

	PendingSurfaceProfileBody = Body;
	bPendingSurfaceProfileApply = true;
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	Body->bWorldScapeSurfaceReady = false;
	HoldWorldScapeRootForProfileDrain();

	if (!bWasAlreadyPending)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldScape] Deferred profile body=%s workers=%d signature=%u"),
			*GetNameSafe(Body), WorldScapeRootInstance->WorldScapeLodInGeneration.Num(),
			BuildSurfaceProfileSignature(Body));
	}
}

void APlanetarySurfaceGenerator::HoldWorldScapeRootForProfileDrain()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
	}

	// bGenerateWorldScape must remain true: if an external tick slips through with
	// it false, WorldScape immediately calls CleanComponents. bFreezeGeneration
	// suppresses UpdatePosition/new jobs while this actor alone consumes results.
	WorldScapeRootInstance->bGenerateWorldScape = true;
	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorEnableCollision(false);
	SetActorTickEnabled(true);
}

void APlanetarySurfaceGenerator::TryFinalizeSurfaceProfileApply()
{
	if (!bPendingSurfaceProfileApply)
	{
		return;
	}
	if (!IsValid(WorldScapeRootInstance))
	{
		CancelPendingSurfaceProfileApply();
		SetActorTickEnabled(false);
		return;
	}

	HoldWorldScapeRootForProfileDrain();
	if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
	{
		WorldScapeRootInstance->CheckForLodGeneration();
		if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
		{
			return;
		}
	}

	APlanetaryBody* Body = PendingSurfaceProfileBody.Get();
	const EDeferredWorldScapeRootState ResumeState = DeferredWorldScapeRootState;
	CancelPendingSurfaceProfileApply();
	if (!IsValid(Body)
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		UnloadWorldScapeRoot();
		return;
	}

	const uint32 PreviousSignature = AppliedSurfaceProfileSignature;
	ApplySurfaceProfileNow(Body);
	if (bPendingLavaMaterial)
	{
		LavaMaterialResumeState = ResumeState;
		return;
	}
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldScape] Applied deferred profile body=%s previous=%u current=%u"),
		*GetNameSafe(Body), PreviousSignature, AppliedSurfaceProfileSignature);

	if (!bSurfaceProfileApplied)
	{
		PreloadWorldScapeRoot();
		return;
	}

	switch (ResumeState)
	{
	case EDeferredWorldScapeRootState::Active:
		SpawnWorldScapeRoot();
		break;
	case EDeferredWorldScapeRootState::FrozenVisible:
		// The retained mesh belongs to the previous profile. Keep it hidden and
		// configured; the next Active transition regenerates it atomically instead
		// of presenting old geometry with the new material/profile signature.
		PreloadWorldScapeRoot();
		break;
	case EDeferredWorldScapeRootState::Preloaded:
	default:
		PreloadWorldScapeRoot();
		break;
	}
}

void APlanetarySurfaceGenerator::CancelPendingSurfaceProfileApply()
{
	bPendingSurfaceProfileApply = false;
	PendingSurfaceProfileBody.Reset();
	DeferredWorldScapeRootState = EDeferredWorldScapeRootState::Preloaded;
}

void APlanetarySurfaceGenerator::TryFinalizeWorldScapeUnload()
{
	if (!bPendingWorldScapeUnload)
	{
		return;
	}
	if (!IsValid(WorldScapeRootInstance))
	{
		WorldScapeRootInstance = nullptr;
		FinalizedWaterMaterialRoot.Reset();
		FinalizedWaterMaterialProfileSignature = 0;
		bPendingWorldScapeUnload = false;
		bDestroyWorldScapeRootAfterDrain = false;
		SetActorTickEnabled(false);
		return;
	}
	// Unload holds the producer tick stopped. Keep its references and poll, not
	// EnsureCompletion: an in-flight foliage worker must finish before EndPlay.
	if (FAPSWorldScapeFoliagePolicy::HasPendingNativeWorker(WorldScapeRootInstance))
	{
		return;
	}
	if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
	{
		WorldScapeRootInstance->CheckForLodGeneration();
		if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
		{
			return;
		}
	}

	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->bGenerateWorldScape = false;
	WorldScapeRootInstance->SetActorTickEnabled(false);
	if (bDestroyWorldScapeRootAfterDrain)
	{
		WorldScapeRootInstance->Destroy();
		WorldScapeRootInstance = nullptr;
		bOwnsWorldScapeRootInstance = false;
	}
	bSurfaceProfileApplied = false;
	AppliedSurfaceProfileSignature = 0;
	ResolvedNoiseInstance = nullptr;
	ResolvedTerrainMaterialInstance = nullptr;
	ResolvedOceanMaterialInstance = nullptr;
	FinalizedWaterMaterialRoot.Reset();
	FinalizedWaterMaterialProfileSignature = 0;
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	MoonLikeNoise = nullptr;
	LavaWorldNoise = nullptr;
	SelenaeNoise = nullptr;
	SelenaeMetalNoise = nullptr;
	EarthLikeNoise = nullptr;
	EarthNoise = nullptr;
	TerraNoise = nullptr;
	IceWorldNoise = nullptr;
	TerraDesert = nullptr;
	TerraForestNoise = nullptr;
	MI_Terra = nullptr;
	MI_Selenae = nullptr;
	MI_Magma = nullptr;
	MI_Planetary_Ocean = nullptr;
	MI_Lava_Ocean = nullptr;
	SetActorTickEnabled(false);
}
