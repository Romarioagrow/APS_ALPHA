#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Gameplay/Production/APSProductionSubsystem.h"
#include "GameFramework/Actor.h"

namespace APSProductionQueueTest
{
	FAPSLocalizedTextDescriptor Text(const TCHAR* Key, const TCHAR* DefaultText)
	{
		FAPSLocalizedTextDescriptor Result;
		Result.Namespace = TEXT("APSProductionTests");
		Result.Key = Key;
		Result.DefaultText = DefaultText;
		return Result;
	}

	FAPSProductionAmount Amount(const TCHAR* Type, const TCHAR* Name,
		const int64 Quantity)
	{
		FAPSProductionAmount Result;
		Result.ItemId = FPrimaryAssetId(Type, Name);
		Result.Quantity = Quantity;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCraftingQueueContractTest,
	"APS.Gameplay.Production.CraftingQueueContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCraftingQueueContractTest::RunTest(const FString& Parameters)
{
	using namespace APSProductionQueueTest;
	UAPSProductionSubsystem* Production = NewObject<UAPSProductionSubsystem>();
	AActor* Station = NewObject<AActor>();
	AActor* Instigator = NewObject<AActor>();
	TestNotNull(TEXT("Production subsystem exists"), Production);
	TestNotNull(TEXT("Crafting station exists"), Station);
	if (!Production || !Station || !Instigator)
	{
		return false;
	}

	const FGuid OwnerId = FGuid::NewGuid();
	const FGuid ContextId = FGuid::NewGuid();
	const FGuid SubjectId = FGuid::NewGuid();
	const FPrimaryAssetId OreId(TEXT("Item"), TEXT("Test.Ore"));
	const FPrimaryAssetId PlateId(TEXT("Item"), TEXT("Test.Plate"));

	FAPSProductionDefinition Recipe;
	Recipe.DefinitionId = FPrimaryAssetId(TEXT("Recipe"), TEXT("Test.Plate"));
	Recipe.Domain = EAPSProductionDomain::Crafting;
	Recipe.Category = TEXT("Materials");
	Recipe.DisplayName = Text(TEXT("PlateRecipe"), TEXT("Plate"));
	Recipe.Costs.Add(Amount(TEXT("Item"), TEXT("Test.Ore"), 2));
	Recipe.Outputs.Add(Amount(TEXT("Item"), TEXT("Test.Plate"), 1));
	Recipe.DurationSeconds = 5.0;
	Recipe.MaximumBatchSize = 3;
	FString Failure;
	TestTrue(TEXT("Recipe registers"), Production->RegisterDefinition(Recipe, Failure));

	FAPSProductionContextRegistration Context;
	Context.ContextStableId = ContextId;
	Context.OwnerStableId = OwnerId;
	Context.Domain = EAPSProductionDomain::Crafting;
	Context.ContextActor = Station;
	Context.QueueCapacity = 2;
	TestTrue(TEXT("Actor-gated crafting context registers"),
		Production->RegisterContext(Context, Failure));

	FAPSProductionInventoryRecord Ore;
	Ore.OwnerStableId = OwnerId;
	Ore.ItemId = OreId;
	Ore.Quantity = 10;
	TestTrue(TEXT("Inventory seed imports through gameplay seam"),
		Production->SetInventoryAmount(Ore, Failure));

	FAPSProductionSnapshot Snapshot;
	TestFalse(TEXT("Debug access cannot alias actor-gated context"),
		Production->QuerySnapshot(ContextId, nullptr,
			EAPSProductionAccessMode::DebugLauncher, Snapshot, Failure));
	TestTrue(TEXT("Registered actor can query crafting snapshot"),
		Production->QuerySnapshot(ContextId, Station,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	TestEqual(TEXT("Snapshot exposes one recipe"), Snapshot.Definitions.Num(), 1);
	const int64 EnqueueRevision = Snapshot.Revision;

	FAPSProductionCommand Command;
	Command.ActionId = TEXT("APS.Production.Enqueue");
	Command.ExpectedRevision = EnqueueRevision;
	Command.SubjectStableId = SubjectId;
	Command.ContextStableId = ContextId;
	Command.DefinitionId = Recipe.DefinitionId;
	Command.Quantity = 2;
	Command.InstigatorActor = Instigator;
	Command.ContextActor = Station;
	const FAPSProductionCommandResult Enqueued = Production->ExecuteCommand(Command);
	TestTrue(TEXT("Revision-safe enqueue is accepted"),
		Enqueued.Status == EAPSProductionCommandStatus::Accepted);
	TestTrue(TEXT("Queue mints a generic persisted JobId"), Enqueued.JobId.IsValid());
	TestEqual(TEXT("Recipe inputs reserve atomically"),
		Production->GetInventoryQuantity(OwnerId, OreId), int64{6});

	const FAPSProductionCommandResult Stale = Production->ExecuteCommand(Command);
	TestTrue(TEXT("Repeated stale command is rejected"),
		Stale.Status == EAPSProductionCommandStatus::Rejected);
	TestEqual(TEXT("Stale rejection is semantic"), Stale.FailureCode,
		FName(TEXT("APS.Production.StaleRevision")));

	Production->AdvanceProduction(0.0);
	Production->AdvanceProduction(10.0);
	TestTrue(TEXT("Completed snapshot queries"),
		Production->QuerySnapshot(ContextId, Station,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	TestEqual(TEXT("Queue retains one inspectable job"), Snapshot.Jobs.Num(), 1);
	TestTrue(TEXT("Crafting job reaches terminal success"),
		Snapshot.Jobs[0].State == EAPSProductionJobState::Succeeded);
	TestEqual(TEXT("Crafting output credits owner inventory"),
		Production->GetInventoryQuantity(OwnerId, PlateId), int64{2});

	FAPSProductionPersistenceState Persisted;
	Production->ExportPersistenceState(Persisted);
	TestEqual(TEXT("Actor-gated job exports"), Persisted.Jobs.Num(), 1);
	TestTrue(TEXT("Authoritative recipe ID persists"),
		Persisted.Jobs[0].DefinitionId == Recipe.DefinitionId);

	UAPSProductionSubsystem* Restored = NewObject<UAPSProductionSubsystem>();
	AActor* RestoredStation = NewObject<AActor>();
	Context.ContextActor = RestoredStation;
	TestTrue(TEXT("Definition registers before restore"),
		Restored->RegisterDefinition(Recipe, Failure));
	TestTrue(TEXT("Actor-ready context registers before restore"),
		Restored->RegisterContext(Context, Failure));
	TestTrue(TEXT("Persistence projection restores"),
		Restored->RestorePersistenceState(Persisted, Failure));
	TestEqual(TEXT("Restored output inventory is stable"),
		Restored->GetInventoryQuantity(OwnerId, PlateId), int64{2});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShipyardMaterializationContractTest,
	"APS.Gameplay.Production.ShipyardMaterializationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSShipyardMaterializationContractTest::RunTest(const FString& Parameters)
{
	using namespace APSProductionQueueTest;
	UAPSProductionSubsystem* Production = NewObject<UAPSProductionSubsystem>();
	AActor* Shipyard = NewObject<AActor>();
	AActor* Instigator = NewObject<AActor>();
	if (!Production || !Shipyard || !Instigator)
	{
		return false;
	}

	FAPSProductionDefinition Ship;
	Ship.DefinitionId = FPrimaryAssetId(TEXT("Ship"), TEXT("AI.P1.01"));
	Ship.Domain = EAPSProductionDomain::Shipyard;
	Ship.Category = TEXT("P1");
	Ship.DisplayName = Text(TEXT("TestShip"), TEXT("Test Ship"));
	Ship.DurationSeconds = 1.0;
	FString Failure;
	TestTrue(TEXT("Ship catalog identity registers"),
		Production->RegisterDefinition(Ship, Failure));

	FAPSProductionContextRegistration Context;
	Context.ContextStableId = FGuid::NewGuid();
	Context.OwnerStableId = FGuid::NewGuid();
	Context.Domain = EAPSProductionDomain::Shipyard;
	Context.ContextActor = Shipyard;
	TestFalse(TEXT("Production shipyard rejects missing canonical spawn pad"),
		Production->RegisterContext(Context, Failure));
	Context.SpawnPadStableId = FGuid::NewGuid();
	TestTrue(TEXT("Shipyard registers with separate job/pad/owner identities"),
		Production->RegisterContext(Context, Failure));

	FAPSProductionSnapshot Snapshot;
	TestTrue(TEXT("Shipyard snapshot queries"),
		Production->QuerySnapshot(Context.ContextStableId, Shipyard,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	FAPSProductionCommand Command;
	Command.ActionId = TEXT("APS.Production.Enqueue");
	Command.ExpectedRevision = Snapshot.Revision;
	Command.SubjectStableId = FGuid::NewGuid();
	Command.ContextStableId = Context.ContextStableId;
	Command.DefinitionId = Ship.DefinitionId;
	Command.InstigatorActor = Instigator;
	Command.ContextActor = Shipyard;
	const FAPSProductionCommandResult Enqueued = Production->ExecuteCommand(Command);
	TestTrue(TEXT("Ship job enqueues by FPrimaryAssetId"),
		Enqueued.Status == EAPSProductionCommandStatus::Accepted);

	Production->AdvanceProduction(0.0);
	Production->AdvanceProduction(1.0);
	TestTrue(TEXT("Finished build time waits for Ships materialization adapter"),
		Production->QuerySnapshot(Context.ContextStableId, Shipyard,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure)
		&& Snapshot.Jobs.Num() == 1
		&& Snapshot.Jobs[0].State == EAPSProductionJobState::AwaitingMaterialization);
	TestFalse(TEXT("Success rejects missing canonical spawned actor identity"),
		Production->ResolveMaterialization(Context.ContextStableId, Enqueued.JobId,
			FGuid{}, FSoftClassPath{}, true, NAME_None, Failure));
	const FGuid ShipInstanceId = FGuid::NewGuid();
	const FSoftClassPath DiagnosticClass(TEXT("/Game/Test/BP_TestShip.BP_TestShip_C"));
	TestTrue(TEXT("Ships adapter resolves awaiting job exactly once"),
		Production->ResolveMaterialization(Context.ContextStableId, Enqueued.JobId,
			ShipInstanceId, DiagnosticClass, true, NAME_None, Failure));
	TestFalse(TEXT("Terminal ship job cannot resolve twice"),
		Production->ResolveMaterialization(Context.ContextStableId, Enqueued.JobId,
			ShipInstanceId, DiagnosticClass, true, NAME_None, Failure));

	FAPSProductionPersistenceState Persisted;
	Production->ExportPersistenceState(Persisted);
	TestEqual(TEXT("Ship job persists"), Persisted.Jobs.Num(), 1);
	TestTrue(TEXT("Ship type identity remains authoritative"),
		Persisted.Jobs[0].DefinitionId == Ship.DefinitionId);
	TestTrue(TEXT("Spawn pad identity remains separate"),
		Persisted.Jobs[0].SpawnPadStableId == Context.SpawnPadStableId);
	TestTrue(TEXT("Spawned instance identity remains separate"),
		Persisted.Jobs[0].MaterializedActorStableId == ShipInstanceId);
	TestTrue(TEXT("Resolved class snapshot is diagnostic only"),
		Persisted.Jobs[0].ResolvedClassSnapshot == DiagnosticClass);
	return true;
}

#endif
