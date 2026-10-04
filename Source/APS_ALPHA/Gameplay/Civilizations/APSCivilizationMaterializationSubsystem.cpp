#include "APSCivilizationMaterializationSubsystem.h"

#include "APSCivilizationIdentityComponent.h"
#include "APSCivilizationJournalSubsystem.h"
#include "APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float RetryIntervalSeconds = 0.25f;
	constexpr double MinimumPadDiameterCm = 9000.0;
	/** A pad on stilts beside the base, not an airfield: a bigger ship does not land at the colony (01.10, 952 m). */
	constexpr double MaximumPadDiameterCm = 16000.0;
	constexpr double PadShipClearanceCm = 1500.0;
	constexpr double BasePadRouteClearanceCm = 3000.0;
	constexpr double SupportClearanceCm = 25.0;
	/** The pad's deck stands on stilts at least this high above the highest ground under it. */
	constexpr double PadDeckClearanceCm = 150.0;
	/** Surface start: the base stands this far ahead of the landed pilot, turned this far to the right of the view. */
	constexpr double PilotSiteDistanceCm = 26000.0;
	constexpr double PilotSiteBearingDegrees = 35.0;
	/** The pilot counts as landed after staying within PilotSettleDriftCm this long, this close to the surface. */
	constexpr double PilotSettleSeconds = 1.5;
	constexpr double PilotSettleDriftCm = 100000.0;
	constexpr double PilotSurfaceReachCm = 2000000.0;
	/** A pilot who has not landed by then gets the seed site. */
	constexpr double PilotLandTimeoutSeconds = 45.0;
	/** Colony arrival (Rio 02.10, "in front of the ramp, the ship and the star in view"): the stand point lies on the
	 * pad's base side, this far past the deck edge (half the route clearance, so the base stays clear) and this far to
	 * the side of the base-pad line (beside the 18 m access ramp); the pilot hovers this high over it until the terrain
	 * collision exists, and goes back to the landing site after the timeout. */
	constexpr double PilotArrivalBeyondDeckCm = BasePadRouteClearanceCm * 0.5;
	constexpr double PilotArrivalStandOffCm = 1300.0;
	constexpr double PilotArrivalHoverCm = 200.0;
	/** The colony gets this long to stand (and the ground under the stand point to exist); the way back as long again. */
	constexpr double PilotArrivalTimeoutSeconds = 30.0;
	/** A pilot who has walked this far from the landing site keeps walking; no arrival. */
	constexpr double PilotArrivalLeashCm = 100000.0;

	/**
	 * The ship's landing footprint: the horizontal radius of its visible hull around the pivot, for any heading. The
	 * whole component box also holds invisible volumes (interaction and gravity spheres) and made a 170 m pad for an
	 * M3 that then hung metres above the slope (30.09).
	 */
	double ShipHullRadiusCm(const AActor* Ship)
	{
		const FVector Scale = Ship->GetActorScale3D().GetAbs();
		const FBox Hull = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Ship);
		if (!Hull.IsValid)
		{
			return (Ship->CalculateComponentsBoundingBoxInLocalSpace(true).GetExtent() * Scale).Size();
		}
		const FVector Center = Hull.GetCenter() * Scale;
		const FVector Extent = Hull.GetExtent() * Scale;
		return FVector2D(FMath::Abs(Center.X) + Extent.X, FMath::Abs(Center.Y) + Extent.Y).Size();
	}

	/** The lowest and highest points of an actor along Direction, relative to Point, from its own oriented bounds. */
	FVector2D OrientedExtremes(const AActor* Actor, const FVector& Point, const FVector& Direction)
	{
		// Visible meshes only: a ship's invisible gravity and interaction spheres held it far above the deck (01.10).
		FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Actor);
		if (!Local.IsValid)
		{
			Local = Actor->CalculateComponentsBoundingBoxInLocalSpace(true);
		}
		const double Pivot = FVector::DotProduct(Actor->GetActorLocation() - Point, Direction);
		if (!Local.IsValid)
		{
			return FVector2D(Pivot, Pivot);
		}
		FVector Corners[8];
		Local.GetVertices(Corners);
		const FTransform Transform = Actor->GetActorTransform();
		FVector2D Extremes(TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
		for (const FVector& Corner : Corners)
		{
			const double Along = FVector::DotProduct(Transform.TransformPosition(Corner) - Point, Direction);
			Extremes.X = FMath::Min(Extremes.X, Along);
			Extremes.Y = FMath::Max(Extremes.Y, Along);
		}
		return Extremes;
	}

	/** The resolver checks a few points of a footprint and the ground between them can rise higher: lifts the
	 * structure so that its underside clears the highest ground beneath it (stilts or a plinth fill below). */
	void LiftClearOfGround(AActor* Structure, APlanetaryBody* Body, const double ClearanceCm)
	{
		const UWorld* World = IsValid(Structure) ? Structure->GetWorld() : nullptr;
		const UAPSSpawnPlacementSubsystem* Spawner = World ? World->GetSubsystem<UAPSSpawnPlacementSubsystem>() : nullptr;
		const double Clearance = Spawner ? Spawner->MeasureGroundClearance(Structure, Body)
			: -TNumericLimits<double>::Max();
		if (Clearance > -TNumericLimits<double>::Max() && Clearance < ClearanceCm)
		{
			Structure->AddActorWorldOffset(Structure->GetActorUpVector() * (ClearanceCm - Clearance), false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
}

void UAPSCivilizationMaterializationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAPSCivilizationMaterializationSubsystem::Deinitialize()
{
	ResetPlacementSearch();
	RuntimeManifest = FAPSCivilizationRuntimeManifest{};
	LastPlacementResult = FAPSCivilizationFootprintResult{};
	MaterializedBase = nullptr;
	MaterializedPad = nullptr;
	MaterializedShip = nullptr;
	MaterializedHomeBody.Reset();
	bManifestInitialized = false;
	bManifestRestoredFromSave = false;
	bMaterializationComplete = false;
	bPilotSiteResolved = false;
	PilotSettleStartSeconds = PilotWaitStartSeconds = -1.0;
	Super::Deinitialize();
}

void UAPSCivilizationMaterializationSubsystem::ResetPlacementSearch()
{
	UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(
		PlacementHomeBody.Get(), LastPlacementResult.PlacementKey);
	PlacementSearch.Reset();
	PlacementHomeBody.Reset();
	PlacementEnvelopeShip.Reset();
	PlacementEnvelopeScale = FVector::ZeroVector;
	PlacementShipEnvelopeDiameterCm = 0.0;
	LastPlacementResult = FAPSCivilizationFootprintResult{};
	RetryAccumulator = 0.0f;
}

void UAPSCivilizationMaterializationSubsystem::Tick(const float DeltaTime)
{
	if (bPilotArrivalPending)
	{
		TickPilotArrival();
	}
	if (bMaterializationComplete || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	RetryAccumulator += DeltaTime;
	// Search slices run every frame; waiting for the canonical profile/collision
	// still polls at 4 Hz. Never repeat the entire terrain search on a retry.
	if (!PlacementSearch.IsSearching() && RetryAccumulator < RetryIntervalSeconds)
	{
		return;
	}
	RetryAccumulator = 0.0f;

	AAstroGenerator* Generator = nullptr;
	APlanetaryBody* HomeBody = nullptr;
	if (!TryInitializeManifest(Generator, HomeBody))
	{
		return;
	}
	if (!TryResolveSafeSite(HomeBody, LastPlacementResult))
	{
		// The site is known but its collision is not built yet: WorldScape builds collision around the pilot, so the
		// pilot goes there first and the colony appears beside them (a remote site may never get collision otherwise).
		if (LastPlacementResult.bTerrainResolved && !LastPlacementResult.bCollisionReady && !bPilotArrivalBegun)
		{
			MaterializedHomeBody = HomeBody;
			BeginPilotArrival(HomeBody, LastPlacementResult.BaseTransform.GetLocation(),
				LastPlacementResult.PadTransform.GetLocation());
		}
		return;
	}
	if (!TryMaterializeEntities(Generator, HomeBody, LastPlacementResult))
	{
		return;
	}
	FString AcceptanceFailure;
	if (!ValidateMaterializedStarterSet(HomeBody, LastPlacementResult, AcceptanceFailure))
	{
		UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(
			HomeBody, LastPlacementResult.PlacementKey);
		TransitionMaterializationState(EAPSCivilizationMaterializationState::Blocked);
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization.Materialization] actor-ready validation blocked: %s"),
			*AcceptanceFailure);
		return;
	}

	UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(
		HomeBody, LastPlacementResult.PlacementKey);

	PlacementSearch.Reset();
	bMaterializationComplete = true;
	TransitionMaterializationState(bManifestRestoredFromSave
		? EAPSCivilizationMaterializationState::LoadedFromSave
		: EAPSCivilizationMaterializationState::Materialized);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Civilization.Materialization] complete manifest=%s civilization=%s home=%s candidate=%d base=%s pad=%s ship=%s loaded=%d"),
		*RuntimeManifest.ManifestId.ToString(EGuidFormats::DigitsWithHyphens),
		*RuntimeManifest.CivilizationId.ToString(EGuidFormats::DigitsWithHyphens),
		*RuntimeManifest.HomeBodyKey, LastPlacementResult.CandidateOrdinal,
		*GetNameSafe(MaterializedBase), *GetNameSafe(MaterializedPad),
		*GetNameSafe(MaterializedShip), bManifestRestoredFromSave ? 1 : 0);
	if (!bPilotArrivalBegun && IsValid(MaterializedBase) && IsValid(MaterializedPad))
	{
		BeginPilotArrival(HomeBody, MaterializedBase->GetActorLocation(), MaterializedPad->GetActorLocation());
	}
}

TStatId UAPSCivilizationMaterializationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSCivilizationMaterializationSubsystem, STATGROUP_Tickables);
}

bool UAPSCivilizationMaterializationSubsystem::IsTickable() const
{
	return !IsTemplate() && (bPilotArrivalPending || (!bMaterializationComplete
		&& RuntimeManifest.MaterializationState != EAPSCivilizationMaterializationState::Blocked));
}

bool UAPSCivilizationMaterializationSubsystem::DoesSupportWorldType(
	const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSCivilizationMaterializationSubsystem::TransitionMaterializationState(
	const EAPSCivilizationMaterializationState NewState)
{
	const EAPSCivilizationMaterializationState Previous =
		RuntimeManifest.MaterializationState;
	if (Previous == NewState)
	{
		return;
	}
	RuntimeManifest.MaterializationState = NewState;
	MaterializationStateChanged.Broadcast(RuntimeManifest, Previous, NewState);
}

bool UAPSCivilizationMaterializationSubsystem::RestoreRuntimeManifest(
	const FAPSCivilizationRuntimeManifest& SavedManifest)
{
	FAPSCivilizationRuntimeManifest Migrated = SavedManifest;
	FString ValidationReason;
	if (!Migrated.MigrateToLatest(&ValidationReason))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] saved manifest rejected: %s"),
			*ValidationReason);
		return false;
	}
	ResetPlacementSearch();
	bPilotSiteResolved = false;
	PilotSettleStartSeconds = PilotWaitStartSeconds = -1.0;
	RuntimeManifest = MoveTemp(Migrated);
	RuntimeManifest.MaterializationState = EAPSCivilizationMaterializationState::PendingPlacement;
	bManifestInitialized = true;
	bManifestRestoredFromSave = true;
	bMaterializationComplete = false;
	return true;
}

bool UAPSCivilizationMaterializationSubsystem::TryInitializeManifest(
	AAstroGenerator*& OutGenerator, APlanetaryBody*& OutHomeBody)
{
	OutGenerator = nullptr;
	OutHomeBody = nullptr;
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (!GameplayState)
	{
		return false;
	}
	if (!bManifestRestoredFromSave
		&& (!GameplayState->bSpawnGeneratedCivilization || GameplayState->bIsLoadingMode
			|| !IsValid(GameplayState->NewGeneratedWorld)
			|| !IsValid(GameplayState->SpawnParameters)
			|| !IsValid(GameplayState->CurrentCivilization.Get())))
	{
		return false;
	}

	for (TActorIterator<AAstroGenerator> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && IsValid(It->HomePlanet))
		{
			OutGenerator = *It;
			OutHomeBody = It->HomePlanet;
			break;
		}
	}
	if (!OutGenerator || !OutHomeBody)
	{
		return false;
	}

	// Rio 03.10: the generator's own home system first. The system nearest the home planet stood in for it, and with
	// catalogue neighbours 1-11 AU away a materialized neighbour won (06:58, ZAKONARA at 10.9 AU): the colony then
	// waited for "the saved home system" for good (24,207 log lines in 5 minutes, no colony).
	AStarSystem* HomeSystem = OutGenerator->GetPreviewHomeSystem();
	if (!IsValid(HomeSystem) || !HomeSystem->StableSystemId.IsValid())
	{
		HomeSystem = nullptr;
		double BestDistanceSq = TNumericLimits<double>::Max();
		for (TActorIterator<AStarSystem> It(GetWorld()); It; ++It)
		{
			if (!IsValid(*It) || !It->StableSystemId.IsValid())
			{
				continue;
			}
			const double DistanceSq = FVector::DistSquared(It->GetActorLocation(),
				OutHomeBody->GetActorLocation());
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				HomeSystem = *It;
			}
		}
	}
	if (!bManifestInitialized)
	{
		if (!HomeSystem || !IsValid(GameplayState->NewGeneratedWorld)
			|| !IsValid(GameplayState->SpawnParameters))
		{
			return false;
		}
		const UGeneratedWorld* WorldModel = GameplayState->NewGeneratedWorld;
		const FString BodyKey = FString::Printf(TEXT("SYS0/S0/P%d"),
			FMath::Max(0, WorldModel->StartPlanetIndex - 1));
		RuntimeManifest = FAPSCivilizationRuntimeManifestFactory::Build(
			WorldModel->GenerationSeed, HomeSystem->StableSystemId, BodyKey,
			GameplayState->SpawnParameters,
			FSoftClassPath(AColony::StaticClass()),
			FSoftClassPath(AAPSCivilizationLandingPad::StaticClass()));
		RuntimeManifest.bUsesPlaceholderAssets = true;
		RuntimeManifest.FallbackReason =
			TEXT("No authored starting-base/pad catalog entry is committed; using legacy AColony and native modular landing-pad placeholders.");
		FString ValidationReason;
		if (!RuntimeManifest.IsStructurallyValid(&ValidationReason))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Civilization.Materialization] rejected manifest: %s"),
				*ValidationReason);
			return false;
		}
		bManifestInitialized = true;
	}
	else if (HomeSystem && RuntimeManifest.HomeSystemId != HomeSystem->StableSystemId)
	{
		// Every 5 s, not every frame.
		static double LastWaitLogSeconds = -100.0;
		if (const double Now = FPlatformTime::Seconds(); Now - LastWaitLogSeconds >= 5.0)
		{
			LastWaitLogSeconds = Now;
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Civilization.Materialization] waiting for saved home system id=%s current=%s"),
				*RuntimeManifest.HomeSystemId.ToString(EGuidFormats::DigitsWithHyphens),
				*HomeSystem->StableSystemId.ToString(EGuidFormats::DigitsWithHyphens));
		}
		return false;
	}
	return true;
}

bool UAPSCivilizationMaterializationSubsystem::TryResolveSafeSite(
	APlanetaryBody* HomeBody, FAPSCivilizationFootprintResult& OutResult)
{
	if (PlacementHomeBody.Get() != HomeBody)
	{
		ResetPlacementSearch();
		PlacementHomeBody = HomeBody;
	}
	FAPSCivilizationManifestEntity* ShipEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::SelectedShip);
	ASpaceship* SelectedShip = ShipEntity ? FindSelectedStarterShip(*ShipEntity) : nullptr;
	if (!ShipEntity || !IsValid(SelectedShip))
	{
		return false;
	}

	// A moving ship's world AABB changes with every rotation and would continually
	// invalidate the pending site. A local bounding sphere is rotation-independent
	// and conservatively fits the ship for any landing orientation.
	// Capture the selected starter asset once per actor/scale. Repeated inverse
	// world transforms also introduce tiny floating-point changes during flight.
	const FVector ShipScale = SelectedShip->GetActorScale3D().GetAbs();
	if (PlacementEnvelopeShip.Get() != SelectedShip || PlacementEnvelopeScale != ShipScale)
	{
		PlacementShipEnvelopeDiameterCm = FMath::Min(2.0 * ShipHullRadiusCm(SelectedShip) + 2.0 * PadShipClearanceCm,
			MaximumPadDiameterCm);
		PlacementEnvelopeShip = SelectedShip;
		PlacementEnvelopeScale = ShipScale;
	}

	FVector PreferredUp = FVector::ZeroVector;
	if (!ResolvePilotSite(HomeBody, PreferredUp))
	{
		return false;
	}

	FAPSCivilizationFootprintRequest Request;
	Request.ManifestSeed = static_cast<int32>(GetTypeHash(RuntimeManifest.ManifestId));
	Request.PreferredUp = PreferredUp;
	Request.PadDiameterCm = FMath::Max(MinimumPadDiameterCm, PlacementShipEnvelopeDiameterCm);
	Request.SeparationCm = Request.BaseSizeCm.X * 0.5
		+ Request.PadDiameterCm * 0.5 + BasePadRouteClearanceCm;
	Request.SurfaceClearanceCm = SupportClearanceCm;
	if (bRelaxedFootprint)
	{
		// Rough worlds (30.09, a frozen planet: 628 candidates, none within 7 degrees for the 80 x 55 m base) get a
		// smaller, steeper foundation before the colony gives up.
		Request.BaseSizeCm = FVector2D(5000.0, 3500.0);
		Request.MaximumStructureSlope = 0.25;
		Request.MaximumRouteSlope = 0.40;
		Request.SeparationCm = Request.BaseSizeCm.X * 0.5 + Request.PadDiameterCm * 0.5 + BasePadRouteClearanceCm;
	}
	const bool bPreviouslyResolved = OutResult.bTerrainResolved;
	const int64 PreviousKey = OutResult.PlacementKey;
	const bool bReady = UAPSPlanetSurfacePlacementResolver::AdvanceCivilizationFootprint(
		HomeBody, Request, PlacementSearch, OutResult);
	if (const double Now = GetWorld()->GetTimeSeconds(); !bReady && Now - LastPlacementLogSeconds >= 5.0)
	{
		// Where the search stands while the colony waits (Rio, 30.09: no colony appeared near a surface start).
		LastPlacementLogSeconds = Now;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Civilization.Materialization] site search: candidates=%d searching=%d terrain=%d dry=%d slope=%d route=%d lod0=%d collision=%d reason='%s'"),
			PlacementSearch.GetEvaluatedCandidateCount(), PlacementSearch.IsSearching() ? 1 : 0,
			OutResult.bTerrainResolved ? 1 : 0, OutResult.bDry ? 1 : 0, OutResult.bSlopeValid ? 1 : 0,
			OutResult.bWalkableRoute ? 1 : 0, OutResult.bLod0Ready ? 1 : 0, OutResult.bCollisionReady ? 1 : 0,
			*OutResult.FailureReason);
	}
	if (!bReady && !bRelaxedFootprint && !PlacementSearch.IsSearching() && !OutResult.bTerrainResolved
		&& OutResult.FailureReason.StartsWith(TEXT("No deterministic")))
	{
		bRelaxedFootprint = true;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Civilization.Materialization] no site within the strict slopes after %d candidates; trying a 50 x 35 m foundation up to 14 degrees"),
			PlacementSearch.GetEvaluatedCandidateCount());
	}
	if (bPreviouslyResolved && (!OutResult.bTerrainResolved || PreviousKey != OutResult.PlacementKey))
	{
		UAPSPlanetSurfacePlacementResolver::ReleasePlacementAnchors(HomeBody, PreviousKey);
	}
	if (!bReady && OutResult.bTerrainResolved)
	{
		UAPSPlanetSurfacePlacementResolver::RequestPlacementAnchors(
			HomeBody, OutResult);
	}
	return bReady;
}

bool UAPSCivilizationMaterializationSubsystem::ResolvePilotSite(APlanetaryBody* HomeBody, FVector& OutPreferredUp)
{
	OutPreferredUp = FVector::ZeroVector;
	UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const USpawnParameters* Spawn = GameplayState ? GameplayState->SpawnParameters : nullptr;
	// Saved actors keep their planet-relative transforms; orbital starts keep the seed site.
	if (bManifestRestoredFromSave || !IsValid(HomeBody) || !Spawn
		|| Spawn->CharacterSpawnPlace != ECharSpawnPlace::PlanetSurface)
	{
		return true;
	}
	if (bPilotSiteResolved)
	{
		OutPreferredUp = HomeBody->GetActorTransform().TransformVectorNoScale(PilotSiteLocal);
		return true;
	}

	const double Now = World->GetTimeSeconds();
	PilotWaitStartSeconds = PilotWaitStartSeconds < 0.0 ? Now : PilotWaitStartSeconds;
	const APawn* Pilot = UGameplayStatics::GetPlayerPawn(World, 0);
	const ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pilot);
	const double RadiusCm = HomeBody->GetWorldScapeBodyRadiusCm();
	const FVector FromCentre = Pilot ? Pilot->GetActorLocation() - HomeBody->GetActorLocation() : FVector::ZeroVector;
	// The surface spawn teleports the pilot once more when the terrain collision lands; wait until it holds still.
	const bool bOnSurface = IsValid(Pilot) && RadiusCm > 0.0 && FromCentre.Size() < RadiusCm + PilotSurfaceReachCm
		&& !(Character && Character->IsSurfaceHandoffSuspended());
	if (!bOnSurface)
	{
		PilotSettleStartSeconds = -1.0;
	}
	else if (PilotSettleStartSeconds < 0.0
		|| FVector::Dist(Pilot->GetActorLocation(), PilotSettleLocation) > PilotSettleDriftCm)
	{
		// First landed sample, or the spawn moved the pilot again (a world-origin rebase too): settle from here.
		PilotSettleStartSeconds = Now;
		PilotSettleLocation = Pilot->GetActorLocation();
		PilotSettleLocal = HomeBody->GetActorTransform().InverseTransformPosition(PilotSettleLocation);
	}
	if (PilotSettleStartSeconds < 0.0 || Now - PilotSettleStartSeconds < PilotSettleSeconds)
	{
		if (Now - PilotWaitStartSeconds < PilotLandTimeoutSeconds)
		{
			return false;
		}
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] the surface-start pilot did not land within %.0f s; seed site"),
			PilotLandTimeoutSeconds);
		bPilotSiteResolved = true;
		PilotSiteLocal = FVector::ZeroVector;
		return true;
	}

	// Ahead and a little to the right of the first view: the base, the pad and the landed ship stand in the frame
	// next to the star the spawn faces.
	const FVector Outward = FromCentre.GetSafeNormal();
	FVector View = FVector::VectorPlaneProject(Pilot->GetControlRotation().Vector(), Outward).GetSafeNormal();
	if (View.IsNearlyZero())
	{
		View = FVector::VectorPlaneProject(Pilot->GetActorForwardVector(), Outward).GetSafeNormal();
	}
	if (View.IsNearlyZero())
	{
		FVector Unused;
		Outward.FindBestAxisVectors(View, Unused);
	}
	const FVector Toward = View.RotateAngleAxis(PilotSiteBearingDegrees, Outward);
	const double Angle = PilotSiteDistanceCm / RadiusCm;
	const FVector Site = (Outward * FMath::Cos(Angle) + Toward * FMath::Sin(Angle)).GetSafeNormal();
	PilotSiteLocal = HomeBody->GetActorTransform().InverseTransformVectorNoScale(Site);
	bPilotSiteResolved = true;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Civilization.Materialization] surface start: colony site %.0f m from the pilot, %.0f deg right of the view (waited %.1f s)"),
		PilotSiteDistanceCm / 100.0, PilotSiteBearingDegrees, Now - PilotWaitStartSeconds);
	OutPreferredUp = Site;
	return true;
}

void UAPSCivilizationMaterializationSubsystem::BeginPilotArrival(APlanetaryBody* HomeBody, const FVector& BaseLocation,
	const FVector& PadLocation)
{
	UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const USpawnParameters* Spawn = GameplayState ? GameplayState->SpawnParameters : nullptr;
	ACustomGravityCharacter* Pilot = Cast<ACustomGravityCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	APlanetarySurfaceGenerator* Surface = IsValid(HomeBody) ? HomeBody->PlanetaryEnvironmentGenerator : nullptr;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	// Only a new surface start, and only while the pilot still stands where the spawn put them.
	if (bPilotArrivalBegun || bManifestRestoredFromSave || !Spawn
		|| Spawn->CharacterSpawnPlace != ECharSpawnPlace::PlanetSurface
		|| !IsValid(Pilot) || Pilot->IsSurfaceHandoffSuspended() || !IsValid(Root) || Root->PlanetScale <= 0.0
		|| PilotSettleStartSeconds < 0.0
		|| FVector::Dist(Pilot->GetActorLocation(), HomeBody->GetActorTransform().TransformPosition(PilotSettleLocal))
			> PilotArrivalLeashCm)
	{
		return;
	}
	const FVector RootCenter = Root->GetActorLocation();
	const FVector Middle = (BaseLocation + PadLocation) * 0.5;
	const FVector Up = (Middle - RootCenter).GetSafeNormal();
	const FVector Along = FVector::VectorPlaneProject(PadLocation - BaseLocation, Up).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(Up, Along).GetSafeNormal();
	if (Up.IsNearlyZero() || Side.IsNearlyZero())
	{
		return;
	}
	// Of the two sides of the base-pad line, the one from which the view to the colony also faces the sun.
	FVector SunAzimuth = FVector::ZeroVector;
	if (const APlanet* Planet = Cast<APlanet>(HomeBody); Planet && IsValid(Planet->ParentStar))
	{
		SunAzimuth = FVector::VectorPlaneProject(Planet->ParentStar->GetActorLocation() - Middle, Up).GetSafeNormal();
	}
	const FVector StandSide = FVector::DotProduct(-Side, SunAzimuth) >= FVector::DotProduct(Side, SunAzimuth)
		? Side : -Side;
	// The deck grows with the ship (up to 160 m): the old stand beside the middle of the base-pad line lay under it.
	const double DeckRadiusCm = IsValid(MaterializedPad)
		? 0.5 * MinimumPadDiameterCm * MaterializedPad->GetActorScale3D().GetAbs().X
		: 0.5 * FMath::Max(MinimumPadDiameterCm, PlacementShipEnvelopeDiameterCm);
	const FVector StandPoint = PadLocation - Along * (DeckRadiusCm + PilotArrivalBeyondDeckCm)
		+ StandSide * PilotArrivalStandOffCm;
	const FVector StandDirection = (StandPoint - RootCenter).GetSafeNormal();
	const double HeightCm = Root->GetGroundHeight(RootCenter + StandDirection * Root->PlanetScale, false);
	// The terrain's own relief bounds a ground height; anything beyond it is a broken sample, not a place for the pilot.
	if (!FMath::IsFinite(HeightCm) || FMath::Abs(HeightCm) > FMath::Max(2.0 * Root->NoiseIntensity, 100000.0))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] no colony arrival: ground height %.0f m at the stand point"),
			HeightCm / 100.0);
		return;
	}
	const FVector Arrival = RootCenter + StandDirection * (Root->PlanetScale + HeightCm);
	// Face the landing pad and the ship on it; TickPilotArrival turns the camera toward the sun when both fit the frame.
	const FVector View = FVector::VectorPlaneProject(PadLocation - Arrival, StandDirection).GetSafeNormal();
	const FTransform BodyTransform = HomeBody->GetActorTransform();
	PilotArrivalLocal = BodyTransform.InverseTransformPosition(Arrival);
	PilotArrivalViewLocal = BodyTransform.InverseTransformVectorNoScale(View);
	PilotArrivalReturnLocal = BodyTransform.InverseTransformPosition(Pilot->GetActorLocation());
	PilotArrivalDeadlineSeconds = World->GetTimeSeconds() + PilotArrivalTimeoutSeconds;
	bPilotArrivalReturning = false;
	bPilotArrivalPending = true;
	bPilotArrivalBegun = true;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Civilization.Materialization] pilot heads to the colony: %.0f m away, stand %.0f m past the %.0f m deck's edge, %.0f m beside the ramp line"),
		FVector::Dist(Pilot->GetActorLocation(), Arrival) / 100.0, PilotArrivalBeyondDeckCm / 100.0,
		DeckRadiusCm / 50.0, PilotArrivalStandOffCm / 100.0);
	Pilot->SetSurfaceHandoffSuspended(true);
	if (bMaterializationComplete && PlacePilotInHeadquarters(Pilot))
	{
		return;
	}
	// Upright over the stand point until the colony stands: WorldScape builds the collision around the pilot.
	Pilot->SetActorLocationAndRotation(Arrival + StandDirection * PilotArrivalHoverCm,
		FRotationMatrix::MakeFromXZ(View.IsNearlyZero() ? Along : View, StandDirection).Rotator(), false, nullptr,
		ETeleportType::TeleportPhysics);
}

void UAPSCivilizationMaterializationSubsystem::TickPilotArrival()
{
	UWorld* World = GetWorld();
	ACustomGravityCharacter* Pilot = World
		? Cast<ACustomGravityCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	APlanetaryBody* HomeBody = MaterializedHomeBody.Get();
	APlanetarySurfaceGenerator* Surface = IsValid(HomeBody) ? HomeBody->PlanetaryEnvironmentGenerator : nullptr;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!IsValid(Pilot) || !IsValid(Root))
	{
		bPilotArrivalPending = false;
		if (IsValid(Pilot))
		{
			Pilot->SetSurfaceHandoffSuspended(false);
		}
		return;
	}
	// Rio 03.10: a surface start begins inside the new headquarters, as soon as the colony stands.
	if (bMaterializationComplete && PlacePilotInHeadquarters(Pilot))
	{
		return;
	}
	const FTransform BodyTransform = HomeBody->GetActorTransform();
	const FVector Arrival = BodyTransform.TransformPosition(PilotArrivalLocal);
	const FVector ArrivalView = BodyTransform.TransformVectorNoScale(PilotArrivalViewLocal);
	const FVector RootCenter = Root->GetActorLocation();
	const FVector Up = (Arrival - RootCenter).GetSafeNormal();
	// Only the current WorldScape collision counts: not the ocean, the colony or an authored sphere.
	TSet<const UPrimitiveComponent*> TerrainCollision;
	for (const UWorldScapeLod* Lod : Root->CollisionLods)
	{
		if (IsValid(Lod) && IsValid(Lod->Mesh) && Lod->Mesh->IsRegistered())
		{
			TerrainCollision.Add(Lod->Mesh);
		}
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSColonyArrival), true, Pilot);
	for (AActor* ColonyActor : {MaterializedBase.Get(), MaterializedPad.Get(), static_cast<AActor*>(MaterializedShip.Get())})
	{
		if (IsValid(ColonyActor))
		{
			Params.AddIgnoredActor(ColonyActor);
		}
	}
	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, Arrival + Up * 30000.0, Arrival - Up * 30000.0, ECC_Visibility, Params);
	const double ExpectedRadius = FVector::Distance(Arrival, RootCenter);
	// The collision mesh is coarser than the height samples: the placement resolver's own tolerance.
	const double ToleranceCm = FMath::Max(250.0, static_cast<double>(Root->CollisionTriangleSize) * 2.0);
	const FHitResult* Ground = Hits.FindByPredicate([&](const FHitResult& Hit)
	{
		return Hit.bBlockingHit && TerrainCollision.Contains(Hit.GetComponent())
			&& FMath::Abs(FVector::Distance(Hit.ImpactPoint, RootCenter) - ExpectedRadius) < ToleranceCm;
	});
	const double Now = World->GetTimeSeconds();
	const bool bDeadline = Now > PilotArrivalDeadlineSeconds;
	// Without the headquarters (aps.Colony.HQ 0, the relaxed 50 x 35 m base) the pilot stands by the pad once the colony
	// stands or has given up; on the way back, as soon as the landing site's ground holds.
	const bool bColonyDone = bMaterializationComplete
		|| RuntimeManifest.MaterializationState == EAPSCivilizationMaterializationState::Blocked;
	if (Ground && (bColonyDone || bDeadline || bPilotArrivalReturning))
	{
		const FVector PilotArrivalView = ArrivalView.IsNearlyZero()
			? FVector::VectorPlaneProject(Pilot->GetActorForwardVector(), Up).GetSafeNormal() : ArrivalView;
		const UCapsuleComponent* Capsule = Pilot->GetCapsuleComponent();
		const double HalfHeightCm = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0;
		Pilot->SetActorLocation(Ground->ImpactPoint + Up * (HalfHeightCm + 2.0), false, nullptr,
			ETeleportType::TeleportPhysics);
		Pilot->SetActorRotation(FRotationMatrix::MakeFromXZ(PilotArrivalView, Up).Rotator(), ETeleportType::TeleportPhysics);
		if (APlayerController* Controller = Cast<APlayerController>(Pilot->GetController()))
		{
			// Rio 02.10: the ship and the star in the first frame. The pilot faces the ship; the camera turns toward the
			// star as far as the ship stays in view (up to 30 degrees, for a star within 80 degrees of the ship's
			// bearing) and tilts up to a higher star; otherwise it tilts up a little over the ship.
			FVector CameraView = PilotArrivalView;
			double PitchDegrees = 12.0;
			const APlanet* Planet = Cast<APlanet>(HomeBody);
			if (Planet && IsValid(Planet->ParentStar))
			{
				const FVector ToStar = (Planet->ParentStar->GetActorLocation() - Pilot->GetActorLocation()).GetSafeNormal();
				const FVector StarAzimuth = FVector::VectorPlaneProject(ToStar, Up).GetSafeNormal();
				const double StarElevation = FMath::RadiansToDegrees(
					FMath::Asin(FMath::Clamp(FVector::DotProduct(ToStar, Up), -1.0, 1.0)));
				const double Bearing = FMath::RadiansToDegrees(FMath::Atan2(
					FVector::DotProduct(FVector::CrossProduct(PilotArrivalView, StarAzimuth), Up),
					FVector::DotProduct(PilotArrivalView, StarAzimuth)));
				if (!StarAzimuth.IsNearlyZero() && StarElevation > -2.0 && FMath::Abs(Bearing) <= 80.0)
				{
					CameraView = PilotArrivalView.RotateAngleAxis(
						FMath::Sign(Bearing) * FMath::Clamp(FMath::Abs(Bearing) - 20.0, 0.0, 30.0), Up);
					PitchDegrees = FMath::Clamp(StarElevation - 18.0, 12.0, 22.0);
				}
			}
			Controller->SetControlRotation(FRotationMatrix::MakeFromXZ((CameraView
				+ Up * FMath::Tan(FMath::DegreesToRadians(PitchDegrees))).GetSafeNormal(), Up).Rotator());
			Pilot->SetViewDirection(CameraView, static_cast<float>(PitchDegrees));
		}
		FinishPilotArrival(Pilot, bPilotArrivalReturning ? TEXT("back at the landing site") : TEXT("at the colony"));
		return;
	}
	if (!bDeadline)
	{
		return;
	}
	if (!bPilotArrivalReturning)
	{
		// No colony and no ground under the stand point in time: back over the landing site the spawn proved, held until
		// its ground collision is there again (the old release there dropped the pilot into the terrain, Rio 03.10).
		bPilotArrivalReturning = true;
		PilotArrivalLocal = PilotArrivalReturnLocal;
		PilotArrivalDeadlineSeconds = Now + PilotArrivalTimeoutSeconds;
		const FVector Return = BodyTransform.TransformPosition(PilotArrivalReturnLocal);
		Pilot->SetActorLocation(Return + (Return - RootCenter).GetSafeNormal() * PilotArrivalHoverCm, false, nullptr,
			ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] no colony or terrain collision at the stand point within %.0f s; the pilot goes back over the landing site"),
			PilotArrivalTimeoutSeconds);
		return;
	}
	// Not even there yet: keep holding (WorldScape builds the collision around the pilot; a release would fall through).
	PilotArrivalDeadlineSeconds = Now + PilotArrivalTimeoutSeconds;
	UE_LOG(LogTemp, Error,
		TEXT("[APS.Civilization.Materialization] still no terrain collision under the landing site; the pilot keeps hovering"));
}

bool UAPSCivilizationMaterializationSubsystem::PlacePilotInHeadquarters(ACustomGravityCharacter* Pilot)
{
	const AColony* Colony = Cast<AColony>(MaterializedBase);
	FTransform Spot;
	if (!IsValid(Pilot) || !IsValid(Colony) || !Colony->GetHeadquartersArrival(Spot))
	{
		return false;
	}
	// On the hall's floor (the building's own collision; no terrain needed), facing into the hall.
	const FVector Up = Spot.GetUnitAxis(EAxis::Z);
	const FRotator Facing = FRotationMatrix::MakeFromXZ(Spot.GetUnitAxis(EAxis::X), Up).Rotator();
	const UCapsuleComponent* Capsule = Pilot->GetCapsuleComponent();
	const double HalfHeightCm = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0;
	Pilot->SetActorLocationAndRotation(Spot.GetLocation() + Up * (HalfHeightCm + 2.0), Facing, false, nullptr,
		ETeleportType::TeleportPhysics);
	// A little down the hall, the holotable and the console row ahead.
	Pilot->SetViewDirection(Spot.GetUnitAxis(EAxis::X), -5.0f);
	if (APlayerController* Controller = Cast<APlayerController>(Pilot->GetController()))
	{
		Controller->SetControlRotation(Facing);
	}
	FinishPilotArrival(Pilot, TEXT("inside the headquarters"));
	return true;
}

void UAPSCivilizationMaterializationSubsystem::FinishPilotArrival(ACustomGravityCharacter* Pilot, const TCHAR* Where)
{
	const bool bAtColony = !bPilotArrivalReturning;
	Pilot->SetSurfaceHandoffSuspended(false);
	bPilotArrivalPending = false;
	bPilotArrivalReturning = false;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Civilization.Materialization] pilot arrived %s: base %.0f m, pad %.0f m away"), Where,
		IsValid(MaterializedBase) ? FVector::Dist(Pilot->GetActorLocation(), MaterializedBase->GetActorLocation()) / 100.0 : -1.0,
		IsValid(MaterializedPad) ? FVector::Dist(Pilot->GetActorLocation(), MaterializedPad->GetActorLocation()) / 100.0 : -1.0);
	if (UAPSWorldOriginSubsystem* WorldOrigin = GetWorld() ? GetWorld()->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr)
	{
		WorldOrigin->RebaseOnto(Pilot->GetActorLocation(), TEXT("colony arrival"));
	}
	if (bAtColony)
	{
		UAPSCivilizationJournalSubsystem::Post(this, TEXT("Colony"), NSLOCTEXT("APSCivilizationJournal", "PilotArrived",
			"The pilot reached the colony site."));
	}
}

AActor* UAPSCivilizationMaterializationSubsystem::FindMaterializedActor(
	const FAPSCivilizationManifestEntity& Entity) const
{
	AActor* NamedCandidate = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		if (const UAPSCivilizationIdentityComponent* Identity =
			Candidate->FindComponentByClass<UAPSCivilizationIdentityComponent>())
		{
			if (Identity->StableEntityId == Entity.StableId)
			{
				return Candidate;
			}
		}
		if (Candidate->GetFName() == Entity.MakeDeterministicActorName())
		{
			NamedCandidate = Candidate;
		}
	}
	return NamedCandidate;
}

ASpaceship* UAPSCivilizationMaterializationSubsystem::FindSelectedStarterShip(
	const FAPSCivilizationManifestEntity& Entity) const
{
	if (AActor* Existing = FindMaterializedActor(Entity))
	{
		return Cast<ASpaceship>(Existing);
	}
	UClass* SelectedClass = Entity.ActorClass.ResolveClass();
	if (!SelectedClass)
	{
		SelectedClass = Entity.ActorClass.TryLoadClass<ASpaceship>();
	}
	ASpaceship* LexicalFallback = nullptr;
	FString FallbackName;
	for (TActorIterator<ASpaceship> It(GetWorld()); It; ++It)
	{
		ASpaceship* Candidate = *It;
		if (!IsValid(Candidate) || !SelectedClass || !Candidate->IsA(SelectedClass))
		{
			continue;
		}
		if (Candidate->ActorHasTag(TEXT("APS.Fleet.HomeShip")))
		{
			return Candidate;
		}
		const FString CandidateName = Candidate->GetName();
		if (!LexicalFallback || CandidateName < FallbackName)
		{
			LexicalFallback = Candidate;
			FallbackName = CandidateName;
		}
	}
	return LexicalFallback;
}

void UAPSCivilizationMaterializationSubsystem::BindIdentity(
	AActor* Actor, const FAPSCivilizationManifestEntity& Entity) const
{
	if (!IsValid(Actor))
	{
		return;
	}
	UAPSCivilizationIdentityComponent* Identity =
		Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
	if (!Identity)
	{
		Identity = NewObject<UAPSCivilizationIdentityComponent>(Actor,
			TEXT("CivilizationIdentity"), RF_Transactional);
		Actor->AddInstanceComponent(Identity);
		Identity->RegisterComponent();
	}
	Identity->InitializeFromManifest(RuntimeManifest, Entity);
	Actor->Tags.AddUnique(TEXT("APS.Civilization.Materialized"));
	if (Actor->GetFName() != Entity.MakeDeterministicActorName())
	{
		Actor->Rename(*Entity.MakeDeterministicActorName().ToString(),
			Actor->GetOuter(), REN_DontCreateRedirectors | REN_NonTransactional);
	}
}

void UAPSCivilizationMaterializationSubsystem::PersistRelativeTransform(
	AActor* Actor, APlanetaryBody* HomeBody, FAPSCivilizationManifestEntity& Entity)
{
	if (!IsValid(Actor) || !IsValid(HomeBody))
	{
		return;
	}
	Entity.PlanetRelativeTransform = Actor->GetActorTransform().GetRelativeTransform(
		HomeBody->GetActorTransform());
	Entity.bHasPersistedTransform = true;
}

void UAPSCivilizationMaterializationSubsystem::PlaceBoundsOnSupportPlane(
	AActor* Actor, const FTransform& SupportTransform, const double ExtraClearanceCm)
{
	if (!IsValid(Actor))
	{
		return;
	}
	Actor->SetActorTransform(SupportTransform, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector Outward = SupportTransform.GetUnitAxis(EAxis::Z);
	// The bottom along the support normal from the actor's own oriented bounds. The world-axis box of a wide flat
	// part on a tilted plane (any site away from the world axes) reached tens of metres below it: the 108 m pad stood
	// 64 m above the ground, 217 m in an earlier game (Rio, 01.10: the pad hangs and cannot be reached).
	const double PivotToBottom = OrientedExtremes(Actor, Actor->GetActorLocation(), Outward).X;
	Actor->SetActorLocation(SupportTransform.GetLocation()
		+ Outward * (ExtraClearanceCm - PivotToBottom), false, nullptr,
		ETeleportType::TeleportPhysics);
}

bool UAPSCivilizationMaterializationSubsystem::TryMaterializeEntities(
	AAstroGenerator* Generator, APlanetaryBody* HomeBody,
	const FAPSCivilizationFootprintResult& Placement)
{
	if (!IsValid(Generator) || !IsValid(HomeBody) || !Placement.bReadyForMaterialization)
	{
		return false;
	}
	FAPSCivilizationManifestEntity* BaseEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::BaseModule);
	FAPSCivilizationManifestEntity* PadEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::LandingPad);
	FAPSCivilizationManifestEntity* ShipEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::SelectedShip);
	if (!BaseEntity || !PadEntity || !ShipEntity)
	{
		return false;
	}

	auto ResolveOrSpawn = [this](FAPSCivilizationManifestEntity& Entity,
		const FTransform& SpawnTransform) -> AActor*
	{
		if (AActor* Existing = FindMaterializedActor(Entity))
		{
			return Existing;
		}
		UClass* ActorClass = Entity.ActorClass.ResolveClass();
		if (!ActorClass)
		{
			ActorClass = Entity.ActorClass.TryLoadClass<AActor>();
		}
		if (!ActorClass || !ActorClass->IsChildOf(AActor::StaticClass()))
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.Name = Entity.MakeDeterministicActorName();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return GetWorld()->SpawnActor<AActor>(ActorClass, SpawnTransform, Params);
	};

	MaterializedBase = ResolveOrSpawn(*BaseEntity, Placement.BaseTransform);
	MaterializedPad = ResolveOrSpawn(*PadEntity, Placement.PadTransform);
	MaterializedShip = FindSelectedStarterShip(*ShipEntity);
	if (!IsValid(MaterializedBase) || !IsValid(MaterializedPad) || !IsValid(MaterializedShip))
	{
		return false;
	}

	// Rio 03.10: the home colony's main building is the new headquarters (BP_ColonyHQ on its 80 x 55 m foundation), and
	// a surface start begins inside it. A rough world's relaxed 50 x 35 m site gets it too: the lift below raises the
	// foundation clear of the highest ground under all of it, and the construction's plinth fills beneath.
	AColony* HomeColony = Cast<AColony>(MaterializedBase);
	if (HomeColony)
	{
		HomeColony->UseHeadquartersLook();
	}
	if (BaseEntity->bHasPersistedTransform)
	{
		MaterializedBase->SetActorTransform(BaseEntity->PlanetRelativeTransform
			* HomeBody->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
		// A save sited before the headquarters (or on a relaxed site): if the 80 x 55 m foundation would sink into
		// the ground there, the compact base stays where the save put it.
		const UAPSSpawnPlacementSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UAPSSpawnPlacementSubsystem>() : nullptr;
		if (HomeColony && HomeColony->HasHeadquartersLook() && Spawner)
		{
			const double Clearance = Spawner->MeasureGroundClearance(MaterializedBase, HomeBody);
			if (Clearance > -TNumericLimits<double>::Max() && Clearance < -100.0)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Civilization.Materialization] saved site buries the headquarters by %.0f cm: compact base"),
					-Clearance);
				HomeColony->UseCompactLook();
			}
		}
	}
	else
	{
		PlaceBoundsOnSupportPlane(MaterializedBase, Placement.BaseTransform, SupportClearanceCm);
		LiftClearOfGround(MaterializedBase, HomeBody, SupportClearanceCm);
	}
	MaterializedBase->Tags.AddUnique(TEXT("APS.Placeholder.LegacyColony"));
	BindIdentity(MaterializedBase, *BaseEntity);

	// The pad the resolver checked: the ship's hull footprint plus clearance, never less than the standard deck.
	const double RequiredPadDiameter = FMath::Clamp(PlacementShipEnvelopeDiameterCm > 0.0
		? PlacementShipEnvelopeDiameterCm
		: 2.0 * ShipHullRadiusCm(MaterializedShip) + 2.0 * PadShipClearanceCm, MinimumPadDiameterCm, MaximumPadDiameterCm);
	const double PadScale = RequiredPadDiameter / MinimumPadDiameterCm;
	if (PadEntity->bHasPersistedTransform)
	{
		MaterializedPad->SetActorTransform(PadEntity->PlanetRelativeTransform
			* HomeBody->GetActorTransform(), false, nullptr,
			ETeleportType::TeleportPhysics);
	}
	else
	{
		FTransform PadSupportTransform = Placement.PadTransform;
		PadSupportTransform.SetScale3D(FVector(PadScale, PadScale, 1.0));
		PlaceBoundsOnSupportPlane(MaterializedPad, PadSupportTransform,
			SupportClearanceCm);
		LiftClearOfGround(MaterializedPad, HomeBody, PadDeckClearanceCm);
	}
	BindIdentity(MaterializedPad, *PadEntity);

	// The ship lands on the pad for a surface start (or where a save parked it). An orbital or in-ship start keeps it
	// docked at the headquarters or under its pilot: moving it would carry the pilot or empty the hangar.
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UMainGameplayInstance* GameplayState = GameInstance
		? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const bool bSurfaceStart = GameplayState && GameplayState->SpawnParameters
		&& GameplayState->SpawnParameters->CharacterSpawnPlace == ECharSpawnPlace::PlanetSurface;
	bShipParkedAtColony = !MaterializedShip->HasPilot()
		&& (ShipEntity->bHasPersistedTransform || (!bManifestRestoredFromSave && bSurfaceStart));
	if (bShipParkedAtColony)
	{
		MaterializedShip->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (ShipEntity->bHasPersistedTransform)
		{
			MaterializedShip->SetActorTransform(ShipEntity->PlanetRelativeTransform
				* HomeBody->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			// On the deck: the pad's top along its normal is the ship's support plane.
			FTransform DeckTransform = Placement.PadTransform;
			const FVector PadUp = Placement.PadTransform.GetUnitAxis(EAxis::Z);
			DeckTransform.SetLocation(Placement.PadTransform.GetLocation() + PadUp
				* OrientedExtremes(MaterializedPad, Placement.PadTransform.GetLocation(), PadUp).Y);
			PlaceBoundsOnSupportPlane(MaterializedShip, DeckTransform, 50.0);
		}
		MaterializedShip->bProvidesArtificialGravity = false;
		MaterializedShip->bApplyExternalGravity = true;
		MaterializedShip->Tags.AddUnique(TEXT("APS.Civilization.SurfaceParked"));
	}
	else
	{
		MaterializedShip->Tags.AddUnique(TEXT("APS.Civilization.InService"));
	}
	BindIdentity(MaterializedShip, *ShipEntity);

	MaterializedBase->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	MaterializedPad->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	PersistRelativeTransform(MaterializedBase, HomeBody, *BaseEntity);
	PersistRelativeTransform(MaterializedPad, HomeBody, *PadEntity);
	if (bShipParkedAtColony)
	{
		MaterializedShip->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
		PersistRelativeTransform(MaterializedShip, HomeBody, *ShipEntity);
	}
	MaterializedHomeBody = HomeBody;
	return true;
}
