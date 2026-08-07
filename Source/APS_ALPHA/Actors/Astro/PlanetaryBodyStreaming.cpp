#include "PlanetaryBody.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"

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
			// Allocate and profile every member of the resident family early, but
			// keep it hidden, frozen and collision-free until that body is selected.
			// This makes planet/moon handoff immediate without updating every surface.
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
		if (PrepareSurface())
		{
			// Do not expose an empty atmosphere while WorldScape is producing its
			// first chunks. The lightweight globe is replaced only after real mesh
			// geometry exists, which also makes slow machines transition cleanly.
			SetPlaceholderVisible(true);
			bWorldScapeSurfaceReady = false;
			Generator->SpawnWorldScapeRoot();
			// WorldScape creates its LOD meshes asynchronously. Keep the root itself
			// hidden until the complete terrain set is ready, otherwise partially
			// updated patches appear as the reported black/ripped planet.
			Generator->WorldScapeRootInstance->SetActorHiddenInGame(true);
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::Active;
			RefreshWorldScapeSurfaceVisibility();
		}
		break;

	case EWorldScapeSurfaceState::FrozenVisible:
		if (IsValid(Generator->WorldScapeRootInstance)
			&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
				|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible
				|| IsWorldScapeStreamingActive()))
		{
			Generator->FreezeWorldScapeRoot();
			RefreshWorldScapeSurfaceVisibility();
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::FrozenVisible;
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
	if (IsValid(Root)
		&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
			|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible))
	{
		const bool bWorkersInFlight = Root->WorldScapeLodInGeneration.Num() > 0;
		if (!bWorkersInFlight)
		{
			int32 ReadyTerrainLods = 0;
			for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
			{
				if (APSWorldScapePayloadValidation::HasCompletePayload(Lod, true))
				{
					++ReadyTerrainLods;
				}
			}

			int32 ReadyOceanLods = 0;
			for (const UWorldScapeLod* Lod : Root->WorldScapeLodOcean)
			{
				if (APSWorldScapePayloadValidation::HasCompletePayload(Lod, false))
				{
					++ReadyOceanLods;
				}
			}

			const int32 RequiredTerrainLods = Root->WorldScapeLod.Num();
			const bool bTerrainReady = RequiredTerrainLods >= Root->MaxLod
				&& ReadyTerrainLods == RequiredTerrainLods;
			const bool bOceanReady = !Root->bOcean
				|| (Root->WorldScapeLodOcean.Num() > 0
					&& ReadyOceanLods == Root->WorldScapeLodOcean.Num());
			bHasStableTerrainCoverage = bTerrainReady && bOceanReady;
		}
	}

	if (bHasStableTerrainCoverage && !bWasSurfaceReady)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldScape] Surface ready body=%s lods=%d oceanLods=%d"),
			*GetName(), Root->WorldScapeLod.Num(), Root->WorldScapeLodOcean.Num());
	}
	bWorldScapeSurfaceReady = bHasStableTerrainCoverage;
	if (IsValid(Root))
	{
		Root->SetActorHiddenInGame(!bWorldScapeSurfaceReady);
	}
	SetPlaceholderVisible(!bWorldScapeSurfaceReady);
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
