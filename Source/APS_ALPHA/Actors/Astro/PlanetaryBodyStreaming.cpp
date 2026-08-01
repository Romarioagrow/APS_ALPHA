#include "PlanetaryBody.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"

void APlanetaryBody::SetWorldScapeStreamingState(EWorldScapeSurfaceState NewState)
{
	if (!bStreamWorldScapeSurface && !bGenerateByDefault)
	{
		return;
	}

	APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator();
	if (!Generator)
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
		if ((!Planet || !Planet->IsManual) && !Generator->bSurfaceProfileApplied)
		{
			Generator->ApplySurfaceProfile(this);
		}
		return true;
	};

	switch (NewState)
	{
	case EWorldScapeSurfaceState::Active:
		if (PrepareSurface())
		{
			Generator->SpawnWorldScapeRoot();
			SetPlaceholderVisible(false);
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::Active;
		}
		break;

	case EWorldScapeSurfaceState::FrozenVisible:
		if (IsValid(Generator->WorldScapeRootInstance)
			&& (WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
				|| WorldScapeSurfaceState == EWorldScapeSurfaceState::FrozenVisible
				|| IsWorldScapeStreamingActive()))
		{
			Generator->FreezeWorldScapeRoot();
			SetPlaceholderVisible(false);
			bEnvironmentSpawned = true;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::FrozenVisible;
		}
		else
		{
			SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
		}
		break;

	case EWorldScapeSurfaceState::Preloaded:
		if (PrepareSurface())
		{
			Generator->PreloadWorldScapeRoot();
			SetPlaceholderVisible(true);
			bEnvironmentSpawned = false;
			WorldScapeSurfaceState = EWorldScapeSurfaceState::Preloaded;
		}
		break;

	case EWorldScapeSurfaceState::Unloaded:
	default:
		Generator->UnloadWorldScapeRoot();
		SetPlaceholderVisible(true);
		bEnvironmentSpawned = false;
		WorldScapeSurfaceState = EWorldScapeSurfaceState::Unloaded;
		break;
	}
}

double APlanetaryBody::GetWorldScapePreloadRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double BodyRadiusCm = ActivationRadius / FMath::Max(WorldScapeActivationRadiusMultiplier, 1.0);
	return FMath::Max(GetWorldScapeDeactivationRadiusCm(), BodyRadiusCm * WorldScapePreloadRadiusMultiplier);
}

double APlanetaryBody::GetWorldScapeUnloadRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double BodyRadiusCm = ActivationRadius / FMath::Max(WorldScapeActivationRadiusMultiplier, 1.0);
	return FMath::Max(GetWorldScapePreloadRadiusCm() * 1.2, BodyRadiusCm * WorldScapeUnloadRadiusMultiplier);
}
