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

	// Preload is intentionally metadata-only. Creating one WorldScape actor for
	// every planet and moon in the resident family polluted the runtime Outliner
	// and spent memory on surfaces the player was not approaching. Only Active is
	// allowed to allocate a transient root; leaving Active releases that root.
	if (NewState == EWorldScapeSurfaceState::Preloaded
		|| NewState == EWorldScapeSurfaceState::Unloaded)
	{
		if (IsValid(PlanetaryEnvironmentGenerator))
		{
			if (NewState == EWorldScapeSurfaceState::Preloaded
				&& !PlanetaryEnvironmentGenerator->bOwnsWorldScapeRootInstance)
			{
				PlanetaryEnvironmentGenerator->PreloadWorldScapeRoot();
			}
			else
			{
				PlanetaryEnvironmentGenerator->UnloadWorldScapeRoot();
			}
		}
		SetPlaceholderVisible(true);
		bEnvironmentSpawned = false;
		WorldScapeSurfaceState = NewState;
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
	case EWorldScapeSurfaceState::Unloaded:
	default:
		break;
	}
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
