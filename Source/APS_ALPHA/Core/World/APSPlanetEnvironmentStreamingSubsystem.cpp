#include "APSPlanetEnvironmentStreamingSubsystem.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSWorldScapeStreaming, Log, All);

namespace
{
	bool IsExplicitMenuPreviewBody(const AActor* Actor)
	{
		for (const AActor* Parent = Actor; IsValid(Parent); Parent = Parent->GetAttachParentActor())
		{
			if (Parent->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return true;
			}
		}
		return false;
	}
}

bool UAPSPlanetEnvironmentStreamingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSPlanetEnvironmentStreamingSubsystem::Deinitialize()
{
	ClearGameplayCollisionAnchor();
	Super::Deinitialize();
}

void UAPSPlanetEnvironmentStreamingSubsystem::Tick(float DeltaTime)
{
	// The active body/family search is intentionally amortized below, but WorldScape's
	// visual producer must follow the possessed pawn every frame. Leaving this position
	// on the half-second cadence lets a fast manual approach outrun the generated patch,
	// while the visual chunk producer continues to target the pawn's previous location.
	RefreshGameplayObserverPosition();

	UpdateElapsed += DeltaTime;
	if (UpdateElapsed < UpdateInterval)
	{
		return;
	}
	UpdateElapsed = 0.0f;
	UpdateActiveEnvironment();
}

TStatId UAPSPlanetEnvironmentStreamingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSPlanetEnvironmentStreamingSubsystem, STATGROUP_Tickables);
}

APlanet* UAPSPlanetEnvironmentStreamingSubsystem::ResolveFamilyPlanet(APlanetaryBody* Body) const
{
	if (APlanet* Planet = Cast<APlanet>(Body))
	{
		return Planet;
	}
	if (const AMoon* Moon = Cast<AMoon>(Body))
	{
		return Moon->ParentPlanet;
	}
	return nullptr;
}

void UAPSPlanetEnvironmentStreamingSubsystem::ClearGameplayCollisionAnchor()
{
	AWorldScapeRoot* Root = AnchoredWorldScapeRoot.Get();
	APawn* Pawn = CollisionAnchorPawn.Get();
	if (IsValid(Root) && IsValid(Pawn))
	{
		Root->CollisionDependantActor.Remove(Pawn);
	}
	AnchoredWorldScapeRoot.Reset();
	CollisionAnchorPawn.Reset();
}

void UAPSPlanetEnvironmentStreamingSubsystem::RefreshGameplayObserverPosition()
{
	AWorldScapeRoot* Root = AnchoredWorldScapeRoot.Get();
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!IsValid(Root) || !IsValid(Observer))
	{
		return;
	}
	if (CollisionAnchorPawn.Get() != Observer)
	{
		// Possession changes are cheap observer-contract changes, not a reason to run
		// the global body/family search early. Rebind this already-active root now so
		// neither its visual nor collision producer follows the previous pawn.
		ApplyGameplayObserverContract(Root, Observer);
		return;
	}

	Root->bOverridePlayerPosition = true;
	Root->OverridedPlayerPosition = Observer->GetActorLocation();
}

void UAPSPlanetEnvironmentStreamingSubsystem::ApplyGameplayObserverContract(
	AWorldScapeRoot* Root, APawn* Observer)
{
	if (!IsValid(Root) || !IsValid(Observer))
	{
		return;
	}

	const bool bAnchorChanged = AnchoredWorldScapeRoot.Get() != Root
		|| CollisionAnchorPawn.Get() != Observer;
	if (bAnchorChanged)
	{
		ClearGameplayCollisionAnchor();
		AnchoredWorldScapeRoot = Root;
		CollisionAnchorPawn = Observer;
	}

	// WorldScape uses OverridedPlayerPosition only for its visual LOD producer.
	// CollisionLodHandler independently rebuilds a list from controllers/editor
	// viewport state. During possession/handoff the controller entry can be transient
	// while a detached editor camera remains present. Register the possessed pawn
	// explicitly in that second contract as well. AddUnique also repairs a root that
	// internally cleared the invoker list during regeneration without accumulating
	// duplicate anchors on the subsystem's half-second refresh.
	Root->CollisionDependantActor.AddUnique(Observer);
	Root->bOverridePlayerPosition = true;
	Root->OverridedPlayerPosition = Observer->GetActorLocation();
	Root->DistanceToFreezeGeneration = 0.0f;
	Root->bGenerateCollision = true;
	Root->bGenerateCollisionForAllPlayer = true;
#if WITH_EDITOR
	Root->bGenerateCollisionInEditor = true;
	Root->bStaticCollisionInEditor = false;
#endif

	if (bAnchorChanged)
	{
		UE_LOG(LogAPSWorldScapeStreaming, Log,
			TEXT("Bound WorldScape gameplay anchor: root=%s pawn=%s location=%s"),
			*Root->GetPathName(), *Observer->GetPathName(),
			*Observer->GetActorLocation().ToCompactString());
	}
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateActiveEnvironment()
{
	UWorld* World = GetWorld();
	// Do not disable streaming for the complete authored SinglePlay map. Its
	// integrated home planet opts out explicitly (bStreamWorldScapeSurface=false),
	// while the other generated planets and moons still need the same distant
	// family preload and nearest-body activation used by generated gameplay.
	// A map-wide early return made every remote atmosphere permanently empty.

	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Observer)
	{
		return;
	}

	const FVector ObserverLocation = Observer->GetActorLocation();
	auto GetSurfaceDistance = [&ObserverLocation](const APlanetaryBody* Body)
	{
		return FMath::Max(0.0,
			FVector::Distance(ObserverLocation, Body->GetActorLocation()) - Body->GetWorldScapeBodyRadiusCm());
	};
	TArray<APlanetaryBody*> StreamedBodies;
	TMap<APlanet*, TArray<APlanetaryBody*>> Families;
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		APlanetaryBody* Body = *It;
		if (!IsValid(Body) || !Body->bStreamWorldScapeSurface
			|| IsExplicitMenuPreviewBody(Body))
		{
			continue;
		}
		if (const APlanet* Planet = Cast<APlanet>(Body); Planet && !Planet->IsNotGasGiant())
		{
			// Gas giants keep their lightweight volumetric/sphere renderer. Their
			// solid moons still join the family through ParentPlanet below.
			continue;
		}
		StreamedBodies.Add(Body);
		if (APlanet* Family = ResolveFamilyPlanet(Body))
		{
			Families.FindOrAdd(Family).Add(Body);
		}
	}

	APlanet* BestFamily = nullptr;
	double BestFamilyScore = TNumericLimits<double>::Max();
	if (APlanet* CurrentFamily = ResidentFamily.Get())
	{
		if (const TArray<APlanetaryBody*>* CurrentFamilyBodies = Families.Find(CurrentFamily))
		{
			// While the observer remains inside the resident family's normal preload
			// zone, keep the whole family stable. Without this lock an overlapping
			// neighbour could win one update and unload every planet/moon root even
			// though the player had not actually left the current planetary system.
			const bool bInsideResidentCore = CurrentFamilyBodies->ContainsByPredicate(
				[&ObserverLocation](const APlanetaryBody* Body)
				{
					return FVector::Distance(ObserverLocation, Body->GetActorLocation())
						<= Body->GetWorldScapePreloadRadiusCm();
				});
			if (bInsideResidentCore)
			{
				BestFamily = CurrentFamily;
				BestFamilyScore = 0.0;
			}
		}
	}
	for (const TPair<APlanet*, TArray<APlanetaryBody*>>& Pair : Families)
	{
		if (BestFamily)
		{
			break;
		}
		const bool bResident = ResidentFamily.Get() == Pair.Key;
		double FamilyScore = TNumericLimits<double>::Max();
		for (APlanetaryBody* Body : Pair.Value)
		{
			const double Limit = bResident
				? Body->GetWorldScapeUnloadRadiusCm()
				: Body->GetWorldScapePreloadRadiusCm();
			const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
			if (Limit > UE_DOUBLE_SMALL_NUMBER && Distance <= Limit)
			{
				const double ResidentBias = bResident ? 0.72 : 1.0;
				FamilyScore = FMath::Min(FamilyScore, GetSurfaceDistance(Body) * ResidentBias);
			}
		}
		if (FamilyScore < BestFamilyScore)
		{
			BestFamilyScore = FamilyScore;
			BestFamily = Pair.Key;
		}
	}

	const bool bFamilyChanged = BestFamily != ResidentFamily.Get();
	for (APlanetaryBody* Body : StreamedBodies)
	{
		if (ResolveFamilyPlanet(Body) != BestFamily
			&& (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded
				|| Body->IsWorldScapeStreamingActive()))
		{
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
		}
	}

	if (!BestFamily)
	{
		if (ResidentFamily.IsValid())
		{
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Unloaded WorldScape family: %s"),
				*GetNameSafe(ResidentFamily.Get()));
		}
		ClearGameplayCollisionAnchor();
		ActiveBody.Reset();
		ResidentFamily.Reset();
		return;
	}

	ResidentFamily = BestFamily;
	TArray<APlanetaryBody*>& FamilyBodies = Families.FindChecked(BestFamily);
	if (bFamilyChanged)
	{
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Selected WorldScape family: %s (%d bodies)"),
			*BestFamily->GetName(), FamilyBodies.Num());
	}

	APlanetaryBody* BestBody = nullptr;
	double BestBodyScore = TNumericLimits<double>::Max();
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (APlanet* Planet = Cast<APlanet>(Body); Planet && !Planet->IsNotGasGiant())
		{
			continue;
		}
		const bool bCurrentBody = ActiveBody.Get() == Body;
		const double Limit = bCurrentBody
			? Body->GetWorldScapeDeactivationRadiusCm()
			: Body->GetWorldScapeActivationRadiusCm();
		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		if (Limit <= UE_DOUBLE_SMALL_NUMBER || Distance > Limit)
		{
			continue;
		}
		const double BodyScore = GetSurfaceDistance(Body) * (bCurrentBody ? 0.8 : 1.0);
		if (BodyScore < BestBodyScore)
		{
			BestBodyScore = BodyScore;
			BestBody = Body;
		}
	}

	// Preparing every planet and moon in one subsystem update synchronously loaded
	// dozens of WorldScape profiles and spawned dozens of roots.  Keep the family
	// resident contract, but activate the nearest body immediately and amortize its
	// siblings across later updates. At the 0.5 s cadence the whole family is still
	// warm well before normal inter-body travel can reach it.
	if (BestBody && BestBody->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded)
	{
		BestBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	}
	if (BestBody && IsValid(BestBody->PlanetaryEnvironmentGenerator))
	{
		if (AWorldScapeRoot* ActiveRoot =
			BestBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
		{
			// WorldScape's WITH_EDITOR path prefers the editor viewport camera even
			// during PIE. Always supply the actual gameplay observer so chunks are
			// generated under the player rather than elsewhere on the planet. The
			// subsystem owns freezing explicitly, so disable the plugin's second,
			// editor-camera-based distance freeze as well.
			ApplyGameplayObserverContract(ActiveRoot, Observer);
		}
	}
	constexpr int32 MaxSiblingPreloadsPerUpdate = 2;
	int32 SiblingPreloads = 0;
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (Body != BestBody
			&& Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
			&& SiblingPreloads < MaxSiblingPreloadsPerUpdate)
		{
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
			++SiblingPreloads;
		}
	}

	if (APlanetaryBody* PreviousBody = ActiveBody.Get(); PreviousBody && PreviousBody != BestBody)
	{
		if (ResolveFamilyPlanet(PreviousBody) == BestFamily)
		{
			// Keep already generated siblings visible and collision-free. Only the
			// nearest body updates its chunks, so approaching a moon never erases the
			// parent planet and returning does not rebuild it from scratch.
			PreviousBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Froze resident WorldScape surface: %s"),
				*PreviousBody->GetPathName());
		}
	}

	ActiveBody = BestBody;
	if (BestBody)
	{
		if (BestBody->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
			|| !BestBody->IsWorldScapeStreamingActive())
		{
			BestBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Activated WorldScape surface: %s"),
				*BestBody->GetPathName());
		}
		if (IsValid(BestBody->PlanetaryEnvironmentGenerator))
		{
			if (AWorldScapeRoot* ActiveRoot =
				BestBody->PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
			{
				// SetWorldScapeStreamingState(Active) can create/replace the root. Reapply
				// the observer and collision contract to that final active instance in the
				// same subsystem update rather than waiting another half second.
				ApplyGameplayObserverContract(ActiveRoot, Observer);
			}
		}
		BestBody->RefreshWorldScapeSurfaceVisibility();
	}
	else
	{
		ClearGameplayCollisionAnchor();
	}
}
