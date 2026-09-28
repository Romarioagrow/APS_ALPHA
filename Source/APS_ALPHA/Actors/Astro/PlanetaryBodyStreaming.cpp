#include "PlanetaryBody.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeReadinessPolicy.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"

namespace
{
	constexpr double WorldScapeRenderAnchorToleranceCm = 2.0;
	constexpr double WorldScapeRenderRadiusToleranceCm = 10.0;
	constexpr int32 MaximumInitialWorldScapeRenderSamples = 128;

	const UWorldScapeLod* FindUniqueTerrainRenderLod0(const AWorldScapeRoot* Root)
	{
		const UWorldScapeLod* Result = nullptr;
		if (!IsValid(Root))
		{
			return nullptr;
		}

		for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
		{
			if (!IsValid(Lod) || Lod->WaterBody || Lod->Lod != 0)
			{
				continue;
			}
			if (Result)
			{
				return nullptr;
			}
			Result = Lod;
		}
		return Result;
	}

	bool HasInitialFullScaleGameplayRenderContract(AWorldScapeRoot* Root)
	{
		if (!IsValid(Root) || Root->WorldScapeLodInGeneration.Num() != 0
			|| !IsValid(Root->TransformKeeper)
			|| !Root->GetActorScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER))
		{
			return false;
		}

		const FVector ObserverWorldPosition = Root->bOverridePlayerPosition
			? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
		if (ObserverWorldPosition.ContainsNaN())
		{
			return false;
		}

		const FVector ObserverEcef = Root->WorldToECEF(ObserverWorldPosition).ToFVector();
		const FVector ObserverNormal = ObserverEcef.GetSafeNormal();
		const UWorldScapeLod* RenderLod0 = FindUniqueTerrainRenderLod0(Root);
		if (ObserverNormal.IsNearlyZero() || !IsValid(RenderLod0)
			|| !APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
				RenderLod0, ObserverNormal, true)
			|| !IsValid(RenderLod0->Mesh) || !RenderLod0->Mesh->IsRegistered()
			|| !RenderLod0->Mesh->IsVisible() || RenderLod0->Mesh->bHiddenInGame
			|| RenderLod0->Mesh->GetNumSections() < 1
			|| !RenderLod0->Mesh->IsMeshSectionVisible(0)
			|| RenderLod0->Mesh->GetAttachParent() != Root->TransformKeeper
			|| !RenderLod0->Mesh->GetRelativeScale3D().Equals(
				FVector::OneVector, KINDA_SMALL_NUMBER)
			|| !FMath::IsFinite(RenderLod0->LodSize) || RenderLod0->LodSize <= 0.0)
		{
			return false;
		}

		// WorldScape stores generated vertices as local offsets around RelativePosition,
		// while the mesh itself is attached directly to TransformKeeper at that ECEF
		// anchor. Both comparisons are required: worker payload completion alone can be
		// true one frame before a recycled mesh transform represents the same patch.
		const FVector LodAnchorEcef = RenderLod0->RelativePosition.ToFVector();
		const FVector ExpectedAnchorWorld = Root->ECEFToWorld(LodAnchorEcef).ToFVector();
		const double RelativeAnchorErrorCm = FVector::Distance(
			RenderLod0->Mesh->GetRelativeLocation(), LodAnchorEcef);
		const double WorldAnchorErrorCm = FVector::Distance(
			RenderLod0->Mesh->GetComponentLocation(), ExpectedAnchorWorld);
		const double AnchorRadiusErrorCm = FMath::Abs(
			LodAnchorEcef.Size() - Root->PlanetScaleCode);
		if (!FMath::IsFinite(RelativeAnchorErrorCm) || !FMath::IsFinite(WorldAnchorErrorCm)
			|| !FMath::IsFinite(AnchorRadiusErrorCm)
			|| RelativeAnchorErrorCm > WorldScapeRenderAnchorToleranceCm
			|| WorldAnchorErrorCm > WorldScapeRenderAnchorToleranceCm
			|| AnchorRadiusErrorCm > WorldScapeRenderRadiusToleranceCm)
		{
			return false;
		}

		// Initial readiness is the atomic hand-off from the smooth body placeholder to
		// WorldScape. A correctly centred but undisplaced base sphere is therefore not a
		// valid first frame. Inspect a bounded, evenly distributed sample of the actual
		// visible LOD0 vertices and make sure that their displacement/relief agrees
		// with the current analytic field, including legitimately flat basins.
		// This intentionally runs only before the body's
		// readiness latch; later incremental WorldScape workers do not revoke readiness.
		const FWorldScapeMeshSection* VisibleRenderSection =
			RenderLod0->Mesh->GetProcMeshSection(0);
		if (!VisibleRenderSection
			|| VisibleRenderSection->PlanetVertexBuffer.Num() < 3
			|| VisibleRenderSection->PlanetIndexBuffer.Num() < 3
			|| VisibleRenderSection->PlanetVertexBuffer.Num() != RenderLod0->Vertices.Num()
			|| VisibleRenderSection->PlanetIndexBuffer.Num() != RenderLod0->Triangles.Num())
		{
			return false;
		}
		const FTransform RenderMeshTransform = RenderLod0->Mesh->GetComponentTransform();
		const FVector SurfaceCenter = Root->GetActorLocation();
		const int32 RenderSampleStride = FMath::Max(
			1, FMath::CeilToInt(static_cast<double>(
				VisibleRenderSection->PlanetVertexBuffer.Num())
				/ static_cast<double>(MaximumInitialWorldScapeRenderSamples)));
		APSWorldScapeReadinessPolicy::FHeightAgreement HeightAgreement;
		for (int32 VertexIndex = 0;
			VertexIndex < VisibleRenderSection->PlanetVertexBuffer.Num();
			VertexIndex += RenderSampleStride)
		{
			const FVector WorldVertex = RenderMeshTransform.TransformPosition(
				VisibleRenderSection->PlanetVertexBuffer[VertexIndex].Position);
			const double RenderHeightCm = FVector::Distance(WorldVertex, SurfaceCenter)
				- Root->PlanetScaleCode;
			const double ExpectedHeightCm = Root->GetGroundHeight(WorldVertex, false);
			if (WorldVertex.ContainsNaN() || !FMath::IsFinite(RenderHeightCm)
				|| !FMath::IsFinite(ExpectedHeightCm))
			{
				return false;
			}
			HeightAgreement.Add(RenderHeightCm, ExpectedHeightCm);
		}
		// Include the final corner even when the stride does not land on it. It is both
		// a useful relief extremum and a guard against a partially published final row.
		const int32 LastVertexIndex =
			VisibleRenderSection->PlanetVertexBuffer.Num() - 1;
		if (LastVertexIndex % RenderSampleStride != 0)
		{
			const FVector WorldVertex = RenderMeshTransform.TransformPosition(
				VisibleRenderSection->PlanetVertexBuffer[LastVertexIndex].Position);
			const double RenderHeightCm = FVector::Distance(WorldVertex, SurfaceCenter)
				- Root->PlanetScaleCode;
			const double ExpectedHeightCm = Root->GetGroundHeight(WorldVertex, false);
			if (WorldVertex.ContainsNaN() || !FMath::IsFinite(RenderHeightCm)
				|| !FMath::IsFinite(ExpectedHeightCm))
			{
				return false;
			}
			HeightAgreement.Add(RenderHeightCm, ExpectedHeightCm);
		}
		if (!HeightAgreement.IsReady())
		{
			return false;
		}

		// LOD0 is a square tangent patch. A point inside its half diagonal is covered;
		// anything farther away is a valid payload for an old observer and must not be
		// exposed as the first gameplay frame.
		const FVector ObserverShellEcef = ObserverNormal * Root->PlanetScaleCode;
		const double ObserverToPatchCenterCm = FVector::Distance(
			ObserverShellEcef, LodAnchorEcef);
		const double MaximumCoveredCenterOffsetCm = FMath::Max(
			RenderLod0->LodSize * 0.75, static_cast<double>(Root->TriangleSize) * 4.0);
		return FMath::IsFinite(ObserverToPatchCenterCm)
			&& ObserverToPatchCenterCm <= MaximumCoveredCenterOffsetCm;
	}
}

void APlanetaryBody::SetWorldScapeStreamingState(EWorldScapeSurfaceState NewState)
{
	if (!bStreamWorldScapeSurface && !bGenerateByDefault)
	{
		return;
	}

	auto SetPlaceholderVisible = [this](bool bVisible)
	{
		if (APlanet* Planet = Cast<APlanet>(this))
		{
			bVisible ? Planet->EnableSphereMesh() : Planet->DisableSphereMesh();
		}
		else if (AMoon* Moon = Cast<AMoon>(this))
		{
			bVisible ? Moon->EnableSphereMesh() : Moon->DisableSphereMesh();
		}
	};

	if (NewState == EWorldScapeSurfaceState::Unloaded)
	{
		if (IsValid(PlanetaryEnvironmentGenerator))
		{
			PlanetaryEnvironmentGenerator->UnloadWorldScapeRoot();
		}
		SetPlaceholderVisible(true);
		bEnvironmentSpawned = false;
		bWorldScapeSurfaceReady = false;
		WorldScapeSurfaceState = EWorldScapeSurfaceState::Unloaded;
		return;
	}
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(PlanetType))
	{
		if (IsValid(PlanetaryEnvironmentGenerator))
		{
			PlanetaryEnvironmentGenerator->UnloadWorldScapeRoot();
		}
		SetPlaceholderVisible(true);
		bEnvironmentSpawned = false;
		bWorldScapeSurfaceReady = false;
		WorldScapeSurfaceState = EWorldScapeSurfaceState::Unloaded;
		return;
	}

	APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator();
	if (!Generator)
	{
		return;
	}

	auto PrepareSurface = [this, Generator]() -> bool
	{
		if (!EnsureWorldScapeSurface())
		{
			return false;
		}
		const APlanet* Planet = Cast<APlanet>(this);
		Generator->bOwnsWorldScapeRootInstance = !Planet || !Planet->IsManual;
		if (Generator->bOwnsWorldScapeRootInstance)
		{
			Generator->WorldScapeRootInstance->SetOwner(this);
			Generator->WorldScapeRootInstance->SetFlags(RF_Transient);
		}
		if ((!Planet || !Planet->IsManual) && !Generator->IsSurfaceProfileCurrent(this))
		{
			// Planet type/radius edits in the generation menu reuse the selected
			// body whenever possible. Reapply the preset whenever its source data
			// changed instead of leaving the previous world's terrain/materials.
			Generator->ApplySurfaceProfile(this);
		}
		return true;
	};

	if (NewState == EWorldScapeSurfaceState::Preloaded)
	{
		if (PrepareSurface())
		{
			// Allocate/profile a hidden, collision-free sibling. The streaming owner
			// may then warm one real payload before publishing it as FrozenVisible.
			Generator->PreloadWorldScapeRoot();
			SetPlaceholderVisible(true);
			bEnvironmentSpawned = false;
			bWorldScapeSurfaceReady = false;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::Preloaded;
		}
		return;
	}

	switch (NewState)
	{
	case EWorldScapeSurfaceState::Active:
	{
		AWorldScapeRoot* PreviousRoot = Generator->WorldScapeRootInstance;
		const uint32 PreviousProfile = Generator->AppliedSurfaceProfileSignature;
		const bool bHadPublishedSurface = bWorldScapeSurfaceReady
			&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
				|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible);
		if (PrepareSurface())
		{
			const APlanet* Planet = Cast<APlanet>(this);
			const bool bResumePublishedSurface = bHadPublishedSurface
				&& bWorldScapeSurfaceReady
				&& IsValid(PreviousRoot) && PreviousRoot == Generator->WorldScapeRootInstance
				&& PreviousProfile == Generator->AppliedSurfaceProfileSignature
				&& !Generator->IsSurfaceProfileApplyPending()
				&& ((Planet && Planet->IsManual) || Generator->IsSurfaceProfileCurrent(this));
			// A planet -> moon -> planet handoff resumes an already published root.
			// Do not revoke its readiness or cover it with the placeholder while its
			// LOD workers catch up to the returning observer. First activation, new
			// roots and changed/pending profiles still require the complete initial
			// render contract. Never inspect worker-owned vertex buffers here.
			SetPlaceholderVisible(!bResumePublishedSurface);
			bWorldScapeSurfaceReady = bResumePublishedSurface;
			Generator->SpawnWorldScapeRoot();
			Generator->WorldScapeRootInstance->SetActorHiddenInGame(!bResumePublishedSurface);
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::Active;
			RefreshWorldScapeSurfaceVisibility();
			if (bResumePublishedSurface)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldScape] Resumed published surface body=%s root=%s workers=%d"),
					*GetName(), *GetNameSafe(PreviousRoot), PreviousRoot->WorldScapeLodInGeneration.Num());
			}
		}
		break;
	}

	case EWorldScapeSurfaceState::FrozenVisible:
		if (IsValid(Generator->WorldScapeRootInstance)
			&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
				|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible
				|| (WorldScapeSurfaceState == EWorldScapeSurfaceState::Preloaded && bWorldScapeSurfaceReady)
				|| IsWorldScapeStreamingActive()))
		{
			Generator->FreezeWorldScapeRoot();
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::FrozenVisible;
			// Establish the display state before refreshing: a just-published standby
			// is no longer "building" once frozen, but its readiness must survive.
			RefreshWorldScapeSurfaceVisibility();
		}
		else
		{
			SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
		}
		break;

	case EWorldScapeSurfaceState::Preloaded:
	case EWorldScapeSurfaceState::Unloaded:
	default:
		break;
	}
}

bool APlanetaryBody::RefreshWorldScapeSurfaceVisibility()
{
	auto SetPlaceholderVisible = [this](bool bVisible)
	{
		if (APlanet* Planet = Cast<APlanet>(this))
		{
			bVisible ? Planet->EnableSphereMesh() : Planet->DisableSphereMesh();
		}
		else if (AMoon* Moon = Cast<AMoon>(this))
		{
			bVisible ? Moon->EnableSphereMesh() : Moon->DisableSphereMesh();
		}
	};

	AWorldScapeRoot* Root = IsValid(PlanetaryEnvironmentGenerator)
		? PlanetaryEnvironmentGenerator->WorldScapeRootInstance : nullptr;
	bool bHasStableTerrainCoverage = false;
	const bool bWasSurfaceReady = bWorldScapeSurfaceReady;
	const bool bBuildingStandby = IsValid(Root)
		&& WorldScapeSurfaceState == EWorldScapeSurfaceState::Preloaded
		&& Root->bGenerateWorldScape && !Root->bFreezeGeneration;
	if (IsValid(Root)
		&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
			|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible || bBuildingStandby))
	{
		// The ocean is a visual liquid shell, never the physical planetary surface.
		// SpawnWorldScapeRoot has to enable collision on the root so the generated
		// terrain CollisionLods can become walkable; WorldScape may also create or
		// recycle ocean LOD components after that call and inherit a collidable
		// state. That produced the reported "double floor": the visible displaced
		// terrain could be crossed and the pawn settled on a smooth inner ocean
		// sphere. Reassert this for every streaming refresh so newly pooled ocean
		// components cannot become gameplay ground between LOD updates.
		for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
		{
			if (IsValid(OceanLod) && IsValid(OceanLod->Mesh))
			{
				OceanLod->Mesh->SetGenerateOverlapEvents(false);
				OceanLod->Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
				OceanLod->Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				// Ocean sections are a level colour/depth shell.  Letting the independently
				// stitched clipmap strips cast/occlude projects their square topology onto
				// the scene and makes flat water look physically displaced.
				OceanLod->Mesh->SetCastShadow(false);
				OceanLod->Mesh->bCastContactShadow = false;
				OceanLod->Mesh->bCastStaticShadow = false;
				OceanLod->Mesh->bCastDynamicShadow = false;
				OceanLod->Mesh->bCastFarShadow = false;
				OceanLod->Mesh->bCastShadowAsTwoSided = false;
				OceanLod->Mesh->bUseAsOccluder = false;
				OceanLod->Mesh->bTreatAsBackgroundForOcclusion = false;
			}
		}

		const bool bWorkersInFlight = Root->WorldScapeLodInGeneration.Num() > 0;
		if (APSWorldScapeReadinessPolicy::NeedsInitialPayloadValidation(bWasSurfaceReady, bWorkersInFlight))
		{
			const FVector ObserverWorldPosition = Root->bOverridePlayerPosition
				? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
			const FVector DesiredSurfaceNormal = Root->WorldToECEF(
				ObserverWorldPosition).ToFVector().GetSafeNormal();
			int32 ReadyTerrainLods = 0;
			for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
			{
				if (APSWorldScapePayloadValidation::HasCompletePayload(Lod, true))
				{
					++ReadyTerrainLods;
				}
			}

			const int32 RequiredTerrainLods = Root->WorldScapeLod.Num();
			const bool bTerrainReady = RequiredTerrainLods >= Root->MaxLod
				&& ReadyTerrainLods == RequiredTerrainLods;
			const bool bOceanReady = !Root->bOcean
				|| APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(
					Root->WorldScapeLodOcean, Root->OceanMaxLod, true,
					DesiredSurfaceNormal, false);
			bHasStableTerrainCoverage = bTerrainReady && bOceanReady;

			const bool bRequiresStrictInitialGameplayRender = !bWasSurfaceReady
				&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active || bBuildingStandby)
				&& FMath::IsNearlyEqual(WorldScapePresentationScale, 1.0)
				&& IsValid(GetWorld()) && GetWorld()->IsGameWorld();
			if (bHasStableTerrainCoverage && bRequiresStrictInitialGameplayRender)
			{
				bHasStableTerrainCoverage = HasInitialFullScaleGameplayRenderContract(Root);
			}
		}
	}
	if (bHasStableTerrainCoverage && !bWasSurfaceReady
		&& !PlanetaryEnvironmentGenerator->FinalizeStableWaterMaterial())
	{
		// Keep the placeholder for one more refresh rather than exposing a partially
		// published or pre-stream material proxy as the first gameplay frame.
		bHasStableTerrainCoverage = false;
	}

	if (bHasStableTerrainCoverage && !bWasSurfaceReady)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldScape] Surface ready body=%s lods=%d oceanLods=%d"),
			*GetName(), Root->WorldScapeLod.Num(), Root->WorldScapeLodOcean.Num());
	}
	// Readiness is an activation latch, not a statement that no incremental worker
	// happens to be active on this exact frame. Once a complete payload has replaced
	// the fallback globe, WorldScape continuously streams new LOD patches as the pawn
	// moves. Revoking readiness for every such batch hid the real root and put the
	// smooth placeholder back on top, which looked like disappearing terrain, half a
	// planet or a flat texture while rotating/moving. Explicit root/profile/state
	// invalidation already clears bWorldScapeSurfaceReady before reaching this path.
	const bool bCanDisplaySurface = IsValid(Root)
		&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
			|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible || bBuildingStandby);
	bWorldScapeSurfaceReady = bCanDisplaySurface
		&& (bWasSurfaceReady || bHasStableTerrainCoverage);
	if (IsValid(Root))
	{
		Root->SetActorHiddenInGame(!bWorldScapeSurfaceReady);
	}
	SetPlaceholderVisible(!bWorldScapeSurfaceReady);
	if (bWorldScapeSurfaceReady && IsValid(PlanetaryEnvironmentGenerator))
	{
		PlanetaryEnvironmentGenerator->UpdateOrbitalWaterAppearance();
	}
	return bWorldScapeSurfaceReady;
}

double APlanetaryBody::GetWorldScapePreloadRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double BodyRadiusCm = ActivationRadius / FMath::Max(WorldScapeActivationRadiusMultiplier, 96.0);
	return FMath::Max(GetWorldScapeDeactivationRadiusCm(),
		BodyRadiusCm * FMath::Max(WorldScapePreloadRadiusMultiplier, 144.0));
}

double APlanetaryBody::GetWorldScapeUnloadRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double BodyRadiusCm = ActivationRadius / FMath::Max(WorldScapeActivationRadiusMultiplier, 96.0);
	return FMath::Max(GetWorldScapePreloadRadiusCm() * 1.2,
		BodyRadiusCm * FMath::Max(WorldScapeUnloadRadiusMultiplier, 192.0));
}
