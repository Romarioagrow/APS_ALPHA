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
	constexpr double PadShipClearanceCm = 1500.0;
	constexpr double BasePadRouteClearanceCm = 3000.0;
	constexpr double SupportClearanceCm = 25.0;
	/** Surface start: the base stands this far ahead of the landed pilot, turned this far to the right of the view. */
	constexpr double PilotSiteDistanceCm = 26000.0;
	constexpr double PilotSiteBearingDegrees = 35.0;
	/** The pilot counts as landed after staying within PilotSettleDriftCm this long, this close to the surface. */
	constexpr double PilotSettleSeconds = 1.5;
	constexpr double PilotSettleDriftCm = 100000.0;
	constexpr double PilotSurfaceReachCm = 2000000.0;
	/** A pilot who has not landed by then gets the seed site. */
	constexpr double PilotLandTimeoutSeconds = 45.0;
	/** Colony arrival: the stand point lies on the route between base and pad, this far to its side, so both stay
	 * inside the collision WorldScape builds around the pilot (about +-64 m); the pilot hovers this high over it until
	 * the terrain collision exists, and goes back to the landing site after the timeout. */
	constexpr double PilotArrivalStandOffCm = 1000.0;
	constexpr double PilotArrivalHoverCm = 200.0;
	constexpr double PilotArrivalTimeoutSeconds = 15.0;
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

	double ProjectedExtent(const FVector& Extent, const FVector& Direction)
	{
		return FMath::Abs(Direction.X) * Extent.X
			+ FMath::Abs(Direction.Y) * Extent.Y
			+ FMath::Abs(Direction.Z) * Extent.Z;
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

	AStarSystem* HomeSystem = nullptr;
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
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] waiting for saved home system id=%s current=%s"),
			*RuntimeManifest.HomeSystemId.ToString(EGuidFormats::DigitsWithHyphens),
			*HomeSystem->StableSystemId.ToString(EGuidFormats::DigitsWithHyphens));
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
		PlacementShipEnvelopeDiameterCm = 2.0 * ShipHullRadiusCm(SelectedShip) + 2.0 * PadShipClearanceCm;
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
		|| FVector::Dist(Pilot->GetActorLocation(), PilotSettleLocation) > PilotArrivalLeashCm)
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
	const FVector StandDirection = ((Middle - RootCenter).GetSafeNormal() * Root->PlanetScale
		+ StandSide * PilotArrivalStandOffCm).GetSafeNormal();
	const double HeightCm = Root->GetGroundHeight(RootCenter + StandDirection * Root->PlanetScale, false);
	if (!FMath::IsFinite(HeightCm))
	{
		return;
	}
	PilotArrivalLocation = RootCenter + StandDirection * (Root->PlanetScale + HeightCm);
	// Face along the route: toward the landing pad and the ship when they lie sunward, else toward the base, so the
	// first view holds the colony and, if it can, the sun.
	const FVector ToPad = FVector::VectorPlaneProject(PadLocation - Middle, Up).GetSafeNormal();
	const FVector FacingTarget = SunAzimuth.IsNearlyZero() || FVector::DotProduct(ToPad, SunAzimuth) >= 0.0
		? PadLocation : BaseLocation;
	PilotArrivalView = FVector::VectorPlaneProject(FacingTarget - PilotArrivalLocation, StandDirection).GetSafeNormal();
	PilotArrivalReturn = Pilot->GetActorLocation();
	PilotArrivalDeadlineSeconds = World->GetTimeSeconds() + PilotArrivalTimeoutSeconds;
	Pilot->SetSurfaceHandoffSuspended(true);
	Pilot->SetActorLocation(PilotArrivalLocation + StandDirection * PilotArrivalHoverCm, false, nullptr,
		ETeleportType::TeleportPhysics);
	bPilotArrivalPending = true;
	bPilotArrivalBegun = true;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Civilization.Materialization] pilot heads to the colony: %.0f m away, stand %.0f m beside the base-pad line"),
		FVector::Dist(PilotArrivalReturn, PilotArrivalLocation) / 100.0, PilotArrivalStandOffCm / 100.0);
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
	const FVector RootCenter = Root->GetActorLocation();
	const FVector Up = (PilotArrivalLocation - RootCenter).GetSafeNormal();
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
	World->LineTraceMultiByChannel(Hits, PilotArrivalLocation + Up * 30000.0, PilotArrivalLocation - Up * 30000.0,
		ECC_Visibility, Params);
	const double ExpectedRadius = FVector::Distance(PilotArrivalLocation, RootCenter);
	const FHitResult* Ground = Hits.FindByPredicate([&](const FHitResult& Hit)
	{
		return Hit.bBlockingHit && TerrainCollision.Contains(Hit.GetComponent())
			&& FMath::Abs(FVector::Distance(Hit.ImpactPoint, RootCenter) - ExpectedRadius) < 250.0;
	});
	if (Ground)
	{
		const UCapsuleComponent* Capsule = Pilot->GetCapsuleComponent();
		const double HalfHeightCm = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0;
		Pilot->SetActorLocation(Ground->ImpactPoint + Up * (HalfHeightCm + 2.0), false, nullptr,
			ETeleportType::TeleportPhysics);
		Pilot->SetActorRotation(FRotationMatrix::MakeFromXZ(PilotArrivalView, Up).Rotator(), ETeleportType::TeleportPhysics);
		if (APlayerController* Controller = Cast<APlayerController>(Pilot->GetController()))
		{
			// The camera tilts up a little so the sun stands in the frame beside the colony.
			Controller->SetControlRotation(FRotationMatrix::MakeFromXZ((PilotArrivalView
				+ Up * FMath::Tan(FMath::DegreesToRadians(12.0))).GetSafeNormal(), Up).Rotator());
		}
		Pilot->SetSurfaceHandoffSuspended(false);
		bPilotArrivalPending = false;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Civilization.Materialization] pilot arrived at the colony: base %.0f m, pad %.0f m away"),
			IsValid(MaterializedBase) ? FVector::Dist(Pilot->GetActorLocation(), MaterializedBase->GetActorLocation()) / 100.0 : -1.0,
			IsValid(MaterializedPad) ? FVector::Dist(Pilot->GetActorLocation(), MaterializedPad->GetActorLocation()) / 100.0 : -1.0);
		if (UAPSWorldOriginSubsystem* WorldOrigin = World->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			WorldOrigin->RebaseOnto(Pilot->GetActorLocation(), TEXT("colony arrival"));
		}
		UAPSCivilizationJournalSubsystem::Post(this, TEXT("Colony"), NSLOCTEXT("APSCivilizationJournal", "PilotArrived",
			"The pilot reached the colony site."));
		return;
	}
	if (World->GetTimeSeconds() > PilotArrivalDeadlineSeconds)
	{
		// No ground under the stand point in time: back to the landing site the spawn proved.
		Pilot->SetActorLocation(PilotArrivalReturn, false, nullptr, ETeleportType::TeleportPhysics);
		Pilot->SetSurfaceHandoffSuspended(false);
		bPilotArrivalPending = false;
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Civilization.Materialization] no terrain collision at the colony within %.0f s; the pilot stays at the landing site"),
			PilotArrivalTimeoutSeconds);
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
	FVector BoundsOrigin;
	FVector BoundsExtent;
	Actor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	const FVector Outward = SupportTransform.GetUnitAxis(EAxis::Z);
	const double PivotToBottom = FVector::DotProduct(
		BoundsOrigin - Actor->GetActorLocation(), Outward)
		- ProjectedExtent(BoundsExtent, Outward);
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

	if (BaseEntity->bHasPersistedTransform)
	{
		MaterializedBase->SetActorTransform(BaseEntity->PlanetRelativeTransform
			* HomeBody->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		PlaceBoundsOnSupportPlane(MaterializedBase, Placement.BaseTransform, SupportClearanceCm);
	}
	MaterializedBase->Tags.AddUnique(TEXT("APS.Placeholder.LegacyColony"));
	BindIdentity(MaterializedBase, *BaseEntity);

	// The pad the resolver checked: the ship's hull footprint plus clearance, never less than the standard deck.
	const double RequiredPadDiameter = FMath::Max(MinimumPadDiameterCm, PlacementShipEnvelopeDiameterCm > 0.0
		? PlacementShipEnvelopeDiameterCm
		: 2.0 * ShipHullRadiusCm(MaterializedShip) + 2.0 * PadShipClearanceCm);
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
			PlaceBoundsOnSupportPlane(MaterializedShip, Placement.PadTransform, 150.0);
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
