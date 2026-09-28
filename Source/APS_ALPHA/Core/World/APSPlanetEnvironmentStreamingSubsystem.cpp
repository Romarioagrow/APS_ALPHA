#include "APSPlanetEnvironmentStreamingSubsystem.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeStreamingPolicy.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSWorldScapeStreaming, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarMaxStandbyRoots(
		TEXT("aps.Surface.MaxStandbyRoots"), 4,
		TEXT("Maximum hidden, unpublished sibling roots prepared ahead of travel (0..32). ")
		TEXT("Never evicts the selected body or an already published surface. Not a total GPU memory cap."),
		ECVF_Default);

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
	WarmingBody.Reset();
	PrewarmedBodies.Reset();
	VisibleLiquidBodies.Reset();
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
	if (UpdateElapsed >= UpdateInterval)
	{
		UpdateElapsed = 0.0f;
		UpdateActiveEnvironment();
	}
	RefreshVisibleLiquidAppearance();
}

void UAPSPlanetEnvironmentStreamingSubsystem::RefreshVisibleLiquidAppearance()
{
	for (const TWeakObjectPtr<APlanetaryBody>& WeakBody : VisibleLiquidBodies)
	{
		APlanetaryBody* Body = WeakBody.Get();
		if (IsValid(Body) && Body->bWorldScapeSurfaceReady
			&& (Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Active
				|| Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::FrozenVisible)
			&& IsValid(Body->PlanetaryEnvironmentGenerator))
		{
			Body->PlanetaryEnvironmentGenerator->UpdateOrbitalWaterAppearance();
		}
	}
}

TStatId UAPSPlanetEnvironmentStreamingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSPlanetEnvironmentStreamingSubsystem, STATGROUP_Tickables);
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateStandbyWarmup(
	APlanetaryBody* Candidate, APawn* Observer)
{
	if (APlanetaryBody* Previous = WarmingBody.Get(); Previous && Previous != Candidate)
	{
		// An approach takes ownership of the same root; do not pause that new active
		// producer. A cancelled speculative build drains through the existing path.
		if (Previous->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded
			&& IsValid(Previous->PlanetaryEnvironmentGenerator))
			Previous->PlanetaryEnvironmentGenerator->PreloadWorldScapeRoot();
	}
	const bool bStarting = WarmingBody.Get() != Candidate;
	WarmingBody = Candidate;
	if (!IsValid(Candidate) || !IsValid(Observer)
		|| !IsValid(Candidate->PlanetaryEnvironmentGenerator)) return;
	auto* Generator = Candidate->PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = Generator->WorldScapeRootInstance;
	if (!IsValid(Root) || Generator->IsSurfaceProfileApplyPending())
	{
		WarmingBody.Reset(); // Retry starting after the deferred profile drain.
		return;
	}
	if (bStarting)
	{
		// Allocation alone was not preload: the moon remained a white placeholder
		// until the player approached it. Build one real, collision-free payload now.
		// Keep its observer fixed until publication so travel cannot starve readiness.
		Root->bOverridePlayerPosition = true;
		Root->OverridedPlayerPosition = Observer->GetActorLocation();
		Generator->SpawnWorldScapeRoot();
		Root->DistanceToFreezeGeneration = 0.0f;
		Root->SetActorHiddenInGame(true);
		Root->SetActorEnableCollision(false);
		Root->bGenerateCollision = false;
#if WITH_EDITOR
		Root->bGenerateCollisionInEditor = false;
#endif
	}
	if (Root->WorldScapeLodInGeneration.Num() > 0)
		Root->CheckForLodGeneration(); // Non-blocking IsDone fence, never wait/join.
	if (Candidate->RefreshWorldScapeSurfaceVisibility())
	{
		Candidate->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
		PrewarmedBodies.AddUnique(Candidate);
		WarmingBody.Reset();
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Published standby terrain: %s"),
			*Candidate->GetPathName());
	}
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
	VisibleLiquidBodies.Reset();
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
	if (ObserverLocation.ContainsNaN()) return;
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
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
		{
			// Gas MOONS obey the same rule as planets. A live solid->gas edit must
			// also release its old root, not leave it updating outside the candidate set.
			if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded)
				Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
			continue;
		}
		StreamedBodies.Add(Body);
		if (APlanet* Family = ResolveFamilyPlanet(Body); IsValid(Family))
		{
			Families.FindOrAdd(Family).Add(Body);
		}
	}

	APlanet* BestFamily = nullptr;
	double BestFamilyScore = TNumericLimits<double>::Max();
	FString BestFamilyKey;
	for (const TPair<APlanet*, TArray<APlanetaryBody*>>& Pair : Families)
	{
		const bool bResident = ResidentFamily.Get() == Pair.Key;
		double FamilyScore = TNumericLimits<double>::Max();
		for (APlanetaryBody* Body : Pair.Value)
		{
			const double Limit = bResident
				? Body->GetWorldScapeUnloadRadiusCm()
				: Body->GetWorldScapePreloadRadiusCm();
			const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
			FamilyScore = FMath::Min(FamilyScore, APSWorldScapeStreamingPolicy::Score(
				Distance, Body->GetWorldScapeBodyRadiusCm(), Limit, bResident, 0.72));
		}
		const FString Key = Pair.Key->GetPathName();
		if (APSWorldScapeStreamingPolicy::Prefer(FamilyScore, bResident, Key,
			BestFamilyScore, BestFamily && BestFamily == ResidentFamily.Get(), BestFamilyKey))
		{
			BestFamilyScore = FamilyScore;
			BestFamily = Pair.Key;
			BestFamilyKey = Key;
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
		UpdateStandbyWarmup(nullptr, Observer);
		PrewarmedBodies.Reset();
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
	FString BestBodyKey;
	for (APlanetaryBody* Body : FamilyBodies)
	{
		const bool bCurrentBody = ActiveBody.Get() == Body;
		const double Limit = bCurrentBody
			? Body->GetWorldScapeDeactivationRadiusCm()
			: Body->GetWorldScapeActivationRadiusCm();
		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		const double BodyScore = APSWorldScapeStreamingPolicy::Score(
			Distance, Body->GetWorldScapeBodyRadiusCm(), Limit, bCurrentBody, 0.8);
		const FString Key = Body->GetPathName();
		if (APSWorldScapeStreamingPolicy::Prefer(BodyScore, bCurrentBody, Key,
			BestBodyScore, BestBody && BestBody == ActiveBody.Get(), BestBodyKey))
		{
			BestBodyScore = BodyScore;
			BestBody = Body;
			BestBodyKey = Key;
		}
	}

	// The selected body is never delayed by the speculative sibling preload budget.
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

	// Bound speculative allocations only. Frozen *published* siblings still render
	// their real geometry: replacing them with the placeholder is not safe eviction.
	// Existing warm roots get a small bias to avoid churning near equal distances.
	struct FStandbyCandidate
	{
		APlanetaryBody* Body;
		double Score;
		FString Key;
	};
	TArray<FStandbyCandidate> Standby;
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (Body == BestBody || Body->bWorldScapeSurfaceReady) continue;
		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		const bool bWarm = Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded;
		Standby.Add({Body, APSWorldScapeStreamingPolicy::Score(Distance,
			Body->GetWorldScapeBodyRadiusCm(), TNumericLimits<double>::Max(), bWarm, 0.8),
			Body->GetPathName()});
	}
	Standby.Sort([](const FStandbyCandidate& A, const FStandbyCandidate& B)
	{
		return A.Score != B.Score ? A.Score < B.Score
			: A.Key.Compare(B.Key, ESearchCase::CaseSensitive) < 0;
	});
	const int32 MaxStandby = FMath::Clamp(CVarMaxStandbyRoots.GetValueOnGameThread(), 0, 32);
	constexpr int32 MaxSiblingPreloadsPerUpdate = 2;
	int32 SiblingPreloads = 0;
	for (int32 Index = 0; Index < Standby.Num(); ++Index)
	{
		APlanetaryBody* Body = Standby[Index].Body;
		if (Index >= MaxStandby || Standby[Index].Score == TNumericLimits<double>::Max())
		{
			if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded)
				Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
		}
		else if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Preloaded
			&& SiblingPreloads < MaxSiblingPreloadsPerUpdate)
		{
			// This can also reclaim a never-published frozen attempt. The generator
			// drains workers before cleanup; no worker-owned arrays are freed here.
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
			++SiblingPreloads;
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
	// Warm sequentially, and only after the foreground body has actually published.
	// Count completed speculative surfaces too: otherwise each published moon frees
	// a hidden slot and eventually every moon gets a high-resolution allocation.
	PrewarmedBodies.RemoveAll([this, BestBody, BestFamily](const TWeakObjectPtr<APlanetaryBody>& WeakBody)
	{
		const APlanetaryBody* Body = WeakBody.Get();
		return !IsValid(Body) || Body == BestBody || !Body->bWorldScapeSurfaceReady
			|| ResolveFamilyPlanet(WeakBody.Get()) != BestFamily;
	});
	APlanetaryBody* WarmCandidate = nullptr;
	if (BestBody && BestBody->bWorldScapeSurfaceReady && PrewarmedBodies.Num() < MaxStandby)
	{
		for (int32 Index = 0; Index < FMath::Min(MaxStandby, Standby.Num()); ++Index)
		{
			APlanetaryBody* Body = Standby[Index].Body;
			if (Body->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded
				&& !Body->bWorldScapeSurfaceReady)
			{
				if (!WarmCandidate || Body == WarmingBody.Get()) WarmCandidate = Body;
			}
		}
	}
	UpdateStandbyWarmup(WarmCandidate, Observer);
	for (APlanetaryBody* Body : FamilyBodies)
	{
		if (Body->bWorldScapeSurfaceReady && IsValid(Body->PlanetaryEnvironmentGenerator)
			&& Body->PlanetaryEnvironmentGenerator->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Water)
			VisibleLiquidBodies.Add(Body);
	}
}
