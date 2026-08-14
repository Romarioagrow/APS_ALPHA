#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCivilizationMaterializationAcceptanceTest,
	"APS.Civilization.Materialization.ActorReadyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCivilizationMaterializationAcceptanceTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(true)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
		nullptr, false, ERHIFeatureLevel::Num, &InitializationValues);
	if (!TestNotNull(TEXT("isolated game world"), World))
	{
		return false;
	}

	auto DestroyWorld = [&World]()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	};

	APlanetaryBody* HomeBody = World->SpawnActor<APlanetaryBody>();
	UAPSCivilizationMaterializationSubsystem* Subsystem =
		NewObject<UAPSCivilizationMaterializationSubsystem>(World);
	USpawnParameters* Spawn = NewObject<USpawnParameters>();
	if (!TestNotNull(TEXT("home body"), HomeBody)
		|| !TestNotNull(TEXT("materialization subsystem"), Subsystem)
		|| !TestNotNull(TEXT("spawn parameters"), Spawn))
	{
		DestroyWorld();
		return false;
	}

	Spawn->CivilizationName = TEXT("APS ACTOR READY TEST");
	Spawn->BP_HomeSpaceship = ASpaceship::StaticClass();
	Subsystem->RuntimeManifest = FAPSCivilizationRuntimeManifestFactory::Build(
		424242, FGuid(0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00),
		TEXT("SYS0/S0/P0"), Spawn,
		FSoftClassPath(AAPSCivilizationBaseModule::StaticClass()),
		FSoftClassPath(AAPSCivilizationLandingPad::StaticClass()));

	FAPSCivilizationManifestEntity* BaseEntity = Subsystem->RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::BaseModule);
	FAPSCivilizationManifestEntity* PadEntity = Subsystem->RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::LandingPad);
	FAPSCivilizationManifestEntity* ShipEntity = Subsystem->RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::SelectedShip);
	if (!TestNotNull(TEXT("base manifest entity"), BaseEntity)
		|| !TestNotNull(TEXT("pad manifest entity"), PadEntity)
		|| !TestNotNull(TEXT("ship manifest entity"), ShipEntity))
	{
		DestroyWorld();
		return false;
	}

	FAPSCivilizationFootprintResult Placement;
	Placement.BaseTransform = FTransform(FQuat::Identity, FVector(0.0, 0.0, 1000000.0));
	Placement.PadTransform = FTransform(FQuat::Identity, FVector(20000.0, 0.0, 1000000.0));
	Placement.Outward = FVector::UpVector;
	Placement.Forward = FVector::ForwardVector;
	Placement.Right = FVector::RightVector;
	Placement.CandidateOrdinal = 3;
	Placement.bTerrainResolved = true;
	Placement.bDry = true;
	Placement.bSlopeValid = true;
	Placement.bWalkableRoute = true;
	Placement.bLod0Ready = true;
	Placement.bCollisionReady = true;
	Placement.bReadyForMaterialization = true;

	AAPSCivilizationBaseModule* Base = World->SpawnActor<AAPSCivilizationBaseModule>();
	AAPSCivilizationLandingPad* Pad = World->SpawnActor<AAPSCivilizationLandingPad>();
	ASpaceship* Ship = World->SpawnActor<ASpaceship>();
	if (!TestNotNull(TEXT("base actor"), Base)
		|| !TestNotNull(TEXT("pad actor"), Pad)
		|| !TestNotNull(TEXT("selected ship actor"), Ship))
	{
		DestroyWorld();
		return false;
	}

	Subsystem->RuntimeManifest.MaterializationState =
		EAPSCivilizationMaterializationState::PendingPlacement;
	Subsystem->MaterializedBase = Base;
	Subsystem->MaterializedPad = Pad;
	Subsystem->MaterializedShip = Ship;
	Subsystem->MaterializedHomeBody = HomeBody;
	Subsystem->bManifestRestoredFromSave = false;
	Subsystem->PlaceBoundsOnSupportPlane(Base, Placement.BaseTransform, 25.0);
	Subsystem->PlaceBoundsOnSupportPlane(Pad, Placement.PadTransform, 25.0);
	Subsystem->PlaceBoundsOnSupportPlane(Ship, Placement.PadTransform, 150.0);
	Subsystem->BindIdentity(Base, *BaseEntity);
	Subsystem->BindIdentity(Pad, *PadEntity);
	Subsystem->BindIdentity(Ship, *ShipEntity);
	Ship->bProvidesArtificialGravity = false;
	Ship->bApplyExternalGravity = true;
	Base->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	Pad->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	Ship->AttachToActor(HomeBody, FAttachmentTransformRules::KeepWorldTransform);
	Subsystem->PersistRelativeTransform(Base, HomeBody, *BaseEntity);
	Subsystem->PersistRelativeTransform(Pad, HomeBody, *PadEntity);
	Subsystem->PersistRelativeTransform(Ship, HomeBody, *ShipEntity);

	FString FailureReason;
	TestTrue(TEXT("canonical starter set is actor-ready"),
		Subsystem->ValidateMaterializedStarterSet(HomeBody, Placement, FailureReason));
	TestTrue(TEXT("accepted starter set has no failure reason"), FailureReason.IsEmpty());

	Ship->bProvidesArtificialGravity = true;
	TestFalse(TEXT("surface ship cannot provide artificial gravity"),
		Subsystem->ValidateMaterializedStarterSet(HomeBody, Placement, FailureReason));
	TestTrue(TEXT("gravity rejection is explicit"), FailureReason.Contains(TEXT("gravity")));
	Ship->bProvidesArtificialGravity = false;

	AAPSCivilizationLandingPad* BlockingActor =
		World->SpawnActor<AAPSCivilizationLandingPad>(Base->GetActorTransform());
	TestNotNull(TEXT("blocking actor"), BlockingActor);
	if (BlockingActor)
	{
		TestFalse(TEXT("blocking actor overlap is rejected"),
			Subsystem->ValidateMaterializedStarterSet(HomeBody, Placement, FailureReason));
		TestTrue(TEXT("overlap rejection is explicit"), FailureReason.Contains(TEXT("overlaps")));
		BlockingActor->Destroy();
	}

	AAPSCivilizationLandingPad* DuplicateIdentity =
		World->SpawnActor<AAPSCivilizationLandingPad>(FVector(80000.0, 0.0, 1000000.0),
			FRotator::ZeroRotator);
	TestNotNull(TEXT("duplicate identity actor"), DuplicateIdentity);
	if (DuplicateIdentity)
	{
		DuplicateIdentity->CivilizationIdentity->InitializeFromManifest(
			Subsystem->RuntimeManifest, *PadEntity);
		TestFalse(TEXT("duplicate stable id is rejected"),
			Subsystem->ValidateMaterializedStarterSet(HomeBody, Placement, FailureReason));
		TestTrue(TEXT("duplicate rejection reports stable id cardinality"),
			FailureReason.Contains(TEXT("resolves to 2 actors")));
		DuplicateIdentity->Destroy();
	}

	int32 LifecycleCalls = 0;
	EAPSCivilizationMaterializationState ObservedPrevious =
		EAPSCivilizationMaterializationState::Blocked;
	EAPSCivilizationMaterializationState ObservedCurrent =
		EAPSCivilizationMaterializationState::Blocked;
	bool bCallbackSawComplete = false;
	const FDelegateHandle LifecycleHandle = Subsystem->OnMaterializationStateChanged().AddLambda(
		[&](const FAPSCivilizationRuntimeManifest& Manifest,
			const EAPSCivilizationMaterializationState Previous,
			const EAPSCivilizationMaterializationState Current)
		{
			++LifecycleCalls;
			ObservedPrevious = Previous;
			ObservedCurrent = Current;
			bCallbackSawComplete = Subsystem->IsMaterializationComplete();
			TestEqual(TEXT("callback manifest id is canonical"), Manifest.ManifestId,
				Subsystem->RuntimeManifest.ManifestId);
		});
	Subsystem->bMaterializationComplete = true;
	Subsystem->TransitionMaterializationState(
		EAPSCivilizationMaterializationState::Materialized);
	Subsystem->TransitionMaterializationState(
		EAPSCivilizationMaterializationState::Materialized);
	Subsystem->OnMaterializationStateChanged().Remove(LifecycleHandle);
	TestEqual(TEXT("lifecycle emits only for the actual enum transition"), LifecycleCalls, 1);
	TestEqual(TEXT("lifecycle previous state"), ObservedPrevious,
		EAPSCivilizationMaterializationState::PendingPlacement);
	TestEqual(TEXT("lifecycle actor-ready state"), ObservedCurrent,
		EAPSCivilizationMaterializationState::Materialized);
	TestTrue(TEXT("subscriber observes complete actor set"), bCallbackSawComplete);

	DestroyWorld();
	return true;
}

#endif
