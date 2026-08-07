#include "PlanetarySurfaceGenerator.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APSWorldScapePlanetNoise.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectGlobals.h"

namespace APSWorldScapeProfiles
{
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
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt64(Body->RadiusKM * 1000.0)));
	Signature = HashCombine(Signature, GetTypeHash(static_cast<int64>(Body->PlanetRadiusKM)));
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
	UGameplayStatics::FinishSpawningActor(WorldScapeRootInstance, RootTransform);
	bOwnsWorldScapeRootInstance = true;
	CancelPendingSurfaceProfileApply();
	bPendingWorldScapeUnload = false;
	bDestroyWorldScapeRootAfterDrain = false;
	WorldScapeRootInstance->SetActorScale3D(FVector::OneVector);
	WorldScapeRootInstance->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorEnableCollision(false);
	return true;
}


void APlanetarySurfaceGenerator::ApplySurfaceProfile(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance))
	{
		return;
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
	ResolvedSurfaceProfile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(
		Body, SurfaceProfileCatalog);
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

		ResolvedNoiseInstance = NewObject<UAPSWorldScapePlanetNoise>(
			WorldScapeRootInstance, NAME_None, RF_Transient);
		if (IsValid(ResolvedNoiseInstance))
		{
			ResolvedNoiseInstance->Configure(ResolvedSurfaceProfile);
			Profile.Noise = ResolvedNoiseInstance;
		}

		ResolvedTerrainMaterialInstance = IsValid(BaseTerrainMaterial)
			? UMaterialInstanceDynamic::Create(BaseTerrainMaterial, WorldScapeRootInstance)
			: nullptr;
		if (IsValid(ResolvedTerrainMaterialInstance))
		{
			UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
				ResolvedTerrainMaterialInstance, ResolvedSurfaceProfile);
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
		ResolvedOceanMaterialInstance = IsValid(BaseOceanMaterial)
			? UMaterialInstanceDynamic::Create(BaseOceanMaterial, WorldScapeRootInstance)
			: nullptr;
		if (IsValid(ResolvedOceanMaterialInstance))
		{
			// Liquid style is authored once in the canonical MIC generated by
			// APSPlanetSurfaceAssetCommandlet. Runtime must not repeat style overrides
			// (or multiply the already emissive lava graph a second time). Per-body
			// terrain emission remains part of the resolved terrain palette; the liquid
			// shell deliberately retains the bounded authored MIC values.
			Profile.OceanMaterial = ResolvedOceanMaterialInstance;
		}

		Profile.NoiseScale = ResolvedSurfaceProfile.NoiseScale;
		Profile.NoiseIntensity = ResolvedSurfaceProfile.NoiseIntensity;
		Profile.bOcean = ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			&& ResolvedSurfaceProfile.LandCoverage < 0.995f
			&& IsValid(Profile.OceanMaterial);
		Profile.OceanHeight = ResolvedSurfaceProfile.OceanLevel * ResolvedSurfaceProfile.NoiseIntensity;
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
	const double BodyRadiusCm = FMath::Max(Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM))
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
	WorldScapeRootInstance->MaxLod = bScaledOrbitalPreview ? 6 : 10;
	WorldScapeRootInstance->LodResolution = bScaledOrbitalPreview ? 48 : 96;
	WorldScapeRootInstance->TriangleSize = bScaledOrbitalPreview ? 450.0f : 180.0f;
	WorldScapeRootInstance->OceanMaxLod = bScaledOrbitalPreview ? 6 : 9;
	WorldScapeRootInstance->OceanLodResolution = bScaledOrbitalPreview ? 32 : 64;
	WorldScapeRootInstance->OceanTriangleSize = bScaledOrbitalPreview ? 650.0f : 260.0f;
	WorldScapeRootInstance->HeightAnchor = FMath::Clamp(
		static_cast<float>(WorldScapeRootInstance->PlanetScale * 0.00025), 50000.0f, 250000.0f);
	bSurfaceProfileApplied = true;
	AppliedSurfaceProfileSignature = BuildSurfaceProfileSignature(Body);

	const FString SurfaceSubtype = UEnum::GetValueAsString(Body->PlanetType);
	const FString SurfaceArchetype = UEnum::GetValueAsString(ResolvedSurfaceProfile.Archetype);
	// One-shot profile diagnostic. This runs only when a body profile is applied,
	// never from the per-frame surface readiness/streaming paths.
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldScape] Profile body=%s subtype=%s archetype=%s ocean=%s seed=%d "
			"noiseScale=%.0f noiseIntensity=%.0f presentationScale=%.6e planetScale=%.6e "
			"terrainLod=%dx%d@%.0f oceanLod=%dx%d@%.0f noise=%s terrain=%s"),
		*Body->GetName(), *SurfaceSubtype, *SurfaceArchetype,
		WorldScapeRootInstance->bOcean ? TEXT("true") : TEXT("false"), Body->WorldScapeSeed,
		WorldScapeRootInstance->NoiseScale, WorldScapeRootInstance->NoiseIntensity,
		PresentationScale, WorldScapeRootInstance->PlanetScale,
		WorldScapeRootInstance->MaxLod, WorldScapeRootInstance->LodResolution,
		WorldScapeRootInstance->TriangleSize,
		WorldScapeRootInstance->OceanMaxLod, WorldScapeRootInstance->OceanLodResolution,
		WorldScapeRootInstance->OceanTriangleSize,
		*GetNameSafe(Profile.Noise), *GetNameSafe(Profile.TerrainMaterial));
}

void APlanetarySurfaceGenerator::PreloadWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
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
	if (WorldScapeRootInstance->WorldScapeLodInGeneration.Num() > 0)
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
		bPendingWorldScapeUnload = false;
		bDestroyWorldScapeRootAfterDrain = false;
		SetActorTickEnabled(false);
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
