#include "APSCivilizationMaterializationSubsystem.h"

#include "APSCivilizationIdentityComponent.h"
#include "APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "EngineUtils.h"

namespace
{
	constexpr float RetryIntervalSeconds = 0.25f;
	constexpr double MinimumPadDiameterCm = 9000.0;
	constexpr double PadShipClearanceCm = 1500.0;
	constexpr double BasePadRouteClearanceCm = 3000.0;
	constexpr double SupportClearanceCm = 25.0;

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
	RuntimeManifest = FAPSCivilizationRuntimeManifest{};
	LastPlacementResult = FAPSCivilizationFootprintResult{};
	MaterializedBase = nullptr;
	MaterializedPad = nullptr;
	MaterializedShip = nullptr;
	MaterializedHomeBody.Reset();
	bManifestInitialized = false;
	bManifestRestoredFromSave = false;
	bMaterializationComplete = false;
	Super::Deinitialize();
}

void UAPSCivilizationMaterializationSubsystem::Tick(const float DeltaTime)
{
	if (bMaterializationComplete || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	RetryAccumulator += DeltaTime;
	if (RetryAccumulator < RetryIntervalSeconds)
	{
		return;
	}
	RetryAccumulator = 0.0f;

	AAstroGenerator* Generator = nullptr;
	APlanetaryBody* HomeBody = nullptr;
	if (!TryInitializeManifest(Generator, HomeBody)
		|| !TryResolveSafeSite(HomeBody, LastPlacementResult))
	{
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
}

TStatId UAPSCivilizationMaterializationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSCivilizationMaterializationSubsystem, STATGROUP_Tickables);
}

bool UAPSCivilizationMaterializationSubsystem::IsTickable() const
{
	return !IsTemplate() && !bMaterializationComplete
		&& RuntimeManifest.MaterializationState != EAPSCivilizationMaterializationState::Blocked;
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
	FAPSCivilizationManifestEntity* ShipEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::SelectedShip);
	ASpaceship* SelectedShip = ShipEntity ? FindSelectedStarterShip(*ShipEntity) : nullptr;
	if (!ShipEntity || !IsValid(SelectedShip))
	{
		return false;
	}

	FVector BoundsOrigin;
	FVector BoundsExtent;
	SelectedShip->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	const double ShipEnvelopeDiameter = 2.0 * FMath::Max3(
		static_cast<double>(BoundsExtent.X), static_cast<double>(BoundsExtent.Y),
		static_cast<double>(BoundsExtent.Z)) + 2.0 * PadShipClearanceCm;

	FAPSCivilizationFootprintRequest Request;
	Request.ManifestSeed = static_cast<int32>(GetTypeHash(RuntimeManifest.ManifestId));
	Request.PadDiameterCm = FMath::Max(MinimumPadDiameterCm, ShipEnvelopeDiameter);
	Request.SeparationCm = Request.BaseSizeCm.X * 0.5
		+ Request.PadDiameterCm * 0.5 + BasePadRouteClearanceCm;
	Request.SurfaceClearanceCm = SupportClearanceCm;
	const bool bReady = UAPSPlanetSurfacePlacementResolver::TryResolveCivilizationFootprint(
		HomeBody, Request, OutResult);
	if (!bReady && OutResult.bTerrainResolved)
	{
		UAPSPlanetSurfacePlacementResolver::RequestPlacementAnchors(
			HomeBody, OutResult);
	}
	return bReady;
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

	FVector ShipBoundsOrigin;
	FVector ShipBoundsExtent;
	MaterializedShip->GetActorBounds(false, ShipBoundsOrigin, ShipBoundsExtent);
	const double RequiredPadDiameter = FMath::Max(MinimumPadDiameterCm,
		2.0 * FMath::Max3(
			static_cast<double>(ShipBoundsExtent.X),
			static_cast<double>(ShipBoundsExtent.Y),
			static_cast<double>(ShipBoundsExtent.Z))
		+ 2.0 * PadShipClearanceCm);
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
	BindIdentity(MaterializedShip, *ShipEntity);

	MaterializedBase->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	MaterializedPad->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	MaterializedShip->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	PersistRelativeTransform(MaterializedBase, HomeBody, *BaseEntity);
	PersistRelativeTransform(MaterializedPad, HomeBody, *PadEntity);
	PersistRelativeTransform(MaterializedShip, HomeBody, *ShipEntity);
	MaterializedHomeBody = HomeBody;
	return true;
}
