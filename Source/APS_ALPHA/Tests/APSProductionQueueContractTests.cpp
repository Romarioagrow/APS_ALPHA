#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Gameplay/Production/APSProductionEventSubsystem.h"
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

	FAPSProductionDefinition StagedRecipe()
	{
		FAPSProductionDefinition Result;
		Result.DefinitionId = FPrimaryAssetId(TEXT("Recipe"),
			TEXT("Test.StagedPersistence"));
		Result.Domain = EAPSProductionDomain::Crafting;
		Result.Category = TEXT("Persistence");
		Result.DisplayName = Text(TEXT("StagedPersistenceRecipe"),
			TEXT("Staged Persistence Recipe"));
		Result.DurationSeconds = 10.0;
		return Result;
	}

	FAPSProductionContextRegistration ActorContext(const FGuid& ContextId,
		const FGuid& OwnerId, AActor* Actor)
	{
		FAPSProductionContextRegistration Result;
		Result.ContextStableId = ContextId;
		Result.OwnerStableId = OwnerId;
		Result.Domain = EAPSProductionDomain::Crafting;
		Result.AccessMode = EAPSProductionAccessMode::ActorGated;
		Result.ContextActor = Actor;
		Result.QueueCapacity = 4;
		Result.MaximumConcurrentJobs = 1;
		return Result;
	}

	FAPSProductionPersistenceState StagedState(const FGuid& ContextId,
		const FGuid& OwnerId, const FAPSProductionDefinition& Definition,
		const FPrimaryAssetId& ItemId)
	{
		FAPSProductionPersistenceState Result;
		Result.StateId = FGuid::NewGuid();
		Result.EventStream.StreamId = FGuid::NewGuid();
		Result.EventStream.LastSequence = 1;

		FAPSProductionInventoryRecord& Inventory =
			Result.Inventories.AddDefaulted_GetRef();
		Inventory.OwnerStableId = OwnerId;
		Inventory.ItemId = ItemId;
		Inventory.Quantity = 7;

		FAPSProductionJobRecord& Job = Result.Jobs.AddDefaulted_GetRef();
		Job.JobId = FGuid::NewGuid();
		Job.CorrelationId = FGuid::NewGuid();
		Job.SubjectStableId = FGuid::NewGuid();
		Job.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
		Job.ContextStableId = ContextId;
		Job.OwnerStableId = OwnerId;
		Job.DefinitionId = Definition.DefinitionId;
		Job.DefinitionSchemaVersion = Definition.DefinitionSchemaVersion;
		Job.Domain = Definition.Domain;
		Job.Quantity = 1;
		Job.State = EAPSProductionJobState::Queued;
		Job.DurationSeconds = Definition.DurationSeconds;
		Job.QueueOrdinal = 1;

		FAPSProductionEventCorrelationRecord& Correlation =
			Result.EventStream.Correlations.AddDefaulted_GetRef();
		Correlation.CorrelationId = Job.CorrelationId;
		Correlation.Verb = TEXT("APS.Crafting.Craft");
		Correlation.SubjectStableId = Job.SubjectStableId;
		Correlation.SubjectIdentityDomain = Job.SubjectIdentityDomain;
		Correlation.TargetStableId = Job.JobId;
		Correlation.DefinitionId = Job.DefinitionId;
		Correlation.DefinitionSchemaVersion = Job.DefinitionSchemaVersion;
		Correlation.Quantity = Job.Quantity;
		Correlation.Result = EAPSProductionEventResult::Requested;
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
	Command.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
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
	Command.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSDebugProductionLauncherContractTest,
	"APS.Gameplay.Production.DebugLauncherContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSDebugProductionLauncherContractTest::RunTest(const FString& Parameters)
{
	using namespace APSProductionQueueTest;
	UAPSProductionSubsystem* Production = NewObject<UAPSProductionSubsystem>();
	if (!Production)
	{
		return false;
	}
	FAPSProductionDefinition Recipe;
	Recipe.DefinitionId = FPrimaryAssetId(TEXT("Recipe"), TEXT("Debug.FreeRecipe"));
	Recipe.Domain = EAPSProductionDomain::Crafting;
	Recipe.Category = TEXT("Debug");
	Recipe.DisplayName = Text(TEXT("DebugRecipe"), TEXT("Debug Recipe"));
	Recipe.MaximumBatchSize = 1;
	FString Failure;
	TestTrue(TEXT("Debug recipe registers through the same catalog contract"),
		Production->RegisterDefinition(Recipe, Failure));

	FAPSProductionContextRegistration DebugContext;
	DebugContext.ContextStableId = FGuid::NewGuid();
	DebugContext.OwnerStableId = FGuid::NewGuid();
	DebugContext.Domain = EAPSProductionDomain::Crafting;
	DebugContext.AccessMode = EAPSProductionAccessMode::DebugLauncher;
	DebugContext.ContextActor = nullptr;
	TestTrue(TEXT("Explicit debug-global context registers without an actor"),
		Production->RegisterContext(DebugContext, Failure));

	int32 PanelRequestCount = 0;
	FAPSProductionSnapshot RequestedSnapshot;
	Production->OnPanelRequested().AddLambda(
		[&PanelRequestCount, &RequestedSnapshot](const FAPSProductionSnapshot& Snapshot)
		{
			++PanelRequestCount;
			RequestedSnapshot = Snapshot;
		});
	TestTrue(TEXT("Debug launcher requests the controlled presenter boundary"),
		Production->RequestPanel(DebugContext.ContextStableId, nullptr,
			EAPSProductionAccessMode::DebugLauncher, Failure));
	TestEqual(TEXT("Panel request broadcasts exactly once"), PanelRequestCount, 1);
	TestTrue(TEXT("Debug snapshot is visibly classified"),
		RequestedSnapshot.AccessMode == EAPSProductionAccessMode::DebugLauncher);

	AActor* MasqueradingActor = NewObject<AActor>();
	FAPSProductionSnapshot Snapshot;
	TestFalse(TEXT("Debug context rejects actor-gated masquerading"),
		Production->QuerySnapshot(DebugContext.ContextStableId, MasqueradingActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));

	FAPSProductionCommand Command;
	Command.ActionId = TEXT("APS.Production.Enqueue");
	Command.ExpectedRevision = RequestedSnapshot.Revision;
	Command.ContextStableId = DebugContext.ContextStableId;
	Command.DefinitionId = Recipe.DefinitionId;
	Command.AccessMode = EAPSProductionAccessMode::DebugLauncher;
	const FAPSProductionCommandResult Enqueued = Production->ExecuteCommand(Command);
	TestTrue(TEXT("Debug-only enqueue may omit canonical actor subject identity"),
		Enqueued.Status == EAPSProductionCommandStatus::Accepted);
	Production->AdvanceProduction(0.0);

	FAPSProductionPersistenceState Persisted;
	Production->ExportPersistenceState(Persisted);
	TestEqual(TEXT("Debug jobs never enter production persistence"), Persisted.Jobs.Num(), 0);
	TestEqual(TEXT("Debug inventories never enter production persistence"),
		Persisted.Inventories.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionActorRebindContractTest,
	"APS.Gameplay.Production.ActorRebindContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionActorRebindContractTest::RunTest(const FString& Parameters)
{
	UAPSProductionSubsystem* Production = NewObject<UAPSProductionSubsystem>();
	AActor* FirstActor = NewObject<AActor>();
	AActor* ReplacementActor = NewObject<AActor>();
	if (!Production || !FirstActor || !ReplacementActor)
	{
		return false;
	}
	FAPSProductionContextRegistration Context;
	Context.ContextStableId = FGuid::NewGuid();
	Context.OwnerStableId = FGuid::NewGuid();
	Context.Domain = EAPSProductionDomain::Building;
	Context.ContextActor = FirstActor;
	FString Failure;
	TestTrue(TEXT("First canonical actor binds production context"),
		Production->RegisterContext(Context, Failure));
	TestTrue(TEXT("Empty actor context detaches without deleting persisted state"),
		Production->UnregisterContext(Context.ContextStableId, FirstActor,
			EAPSProductionAccessMode::ActorGated, Failure));

	FAPSProductionSnapshot Snapshot;
	TestFalse(TEXT("Detached actor can no longer query context"),
		Production->QuerySnapshot(Context.ContextStableId, FirstActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	FAPSProductionContextRegistration WrongOwner = Context;
	WrongOwner.ContextActor = ReplacementActor;
	WrongOwner.OwnerStableId = FGuid::NewGuid();
	TestFalse(TEXT("Rebind rejects ownership drift"),
		Production->RegisterContext(WrongOwner, Failure));
	Context.ContextActor = ReplacementActor;
	TestTrue(TEXT("Compatible replacement actor rebinds same canonical context"),
		Production->RegisterContext(Context, Failure));
	TestTrue(TEXT("Replacement actor queries the preserved context"),
		Production->QuerySnapshot(Context.ContextStableId, ReplacementActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionStagedPersistenceContractTest,
	"APS.Gameplay.Production.StagedPersistenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionStagedPersistenceContractTest::RunTest(const FString& Parameters)
{
	using namespace APSProductionQueueTest;
	const FAPSProductionDefinition Definition = StagedRecipe();
	const FPrimaryAssetId ItemId(TEXT("Item"), TEXT("Test.StagedInventory"));
	FString Failure;

	// Context-first path: readiness survives while its definition is unavailable,
	// and old runtime data cannot service gameplay during the load quarantine.
	UAPSProductionSubsystem* ContextFirst = NewObject<UAPSProductionSubsystem>();
	AActor* OldActor = NewObject<AActor>();
	AActor* ReadyActor = NewObject<AActor>();
	if (!ContextFirst || !OldActor || !ReadyActor)
	{
		return false;
	}
	const FGuid OwnerId = FGuid::NewGuid();
	const FGuid ContextId = FGuid::NewGuid();
	FAPSProductionContextRegistration Context = ActorContext(ContextId,
		OwnerId, OldActor);
	TestTrue(TEXT("Old actor context starts bound"),
		ContextFirst->RegisterContext(Context, Failure));
	FAPSProductionInventoryRecord OldInventory;
	OldInventory.OwnerStableId = OwnerId;
	OldInventory.ItemId = ItemId;
	OldInventory.Quantity = 99;
	TestTrue(TEXT("Old runtime inventory starts populated"),
		ContextFirst->SetInventoryAmount(OldInventory, Failure));

	const FAPSProductionPersistenceState State = StagedState(ContextId,
		OwnerId, Definition, ItemId);
	const FGuid LoadGenerationId = FGuid::NewGuid();
	TestTrue(TEXT("Valid projection stages structurally"),
		ContextFirst->StagePersistenceState(State, LoadGenerationId, Failure));
	TestTrue(TEXT("Valid staging enters mutation quarantine"),
		ContextFirst->IsPersistenceLoadQuarantined());
	FGuid PendingGeneration;
	TestTrue(TEXT("Pending generation is observable to actor-ready adapters"),
		ContextFirst->GetPendingLoadGenerationId(PendingGeneration));
	TestEqual(TEXT("Pending generation is exact"), PendingGeneration,
		LoadGenerationId);
	TestTrue(TEXT("Identical same-generation restaging is idempotent"),
		ContextFirst->StagePersistenceState(State, LoadGenerationId, Failure));

	FAPSProductionSnapshot Snapshot;
	TestFalse(TEXT("Quarantine rejects old actor snapshot access"),
		ContextFirst->QuerySnapshot(ContextId, OldActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	TestEqual(TEXT("Snapshot rejection identifies load quarantine"), Failure,
		FString(TEXT("APS.Production.PersistenceLoadInProgress")));
	TestEqual(TEXT("Rollback inventory is not visible during quarantine"),
		ContextFirst->GetInventoryQuantity(OwnerId, ItemId), int64{0});
	TestFalse(TEXT("Quarantine rejects direct inventory replacement"),
		ContextFirst->SetInventoryAmount(OldInventory, Failure));
	TestEqual(TEXT("Inventory rejection identifies load quarantine"), Failure,
		FString(TEXT("APS.Production.PersistenceLoadInProgress")));
	const FAPSProductionAmount Delta = Amount(TEXT("Item"),
		TEXT("Test.StagedInventory"), 1);
	TestFalse(TEXT("Quarantine rejects direct inventory delta"),
		ContextFirst->ApplyInventoryDelta(OwnerId, Delta, true, Failure));
	TestFalse(TEXT("Quarantine rejects materialization mutation"),
		ContextFirst->ResolveMaterialization(ContextId, FGuid::NewGuid(),
			FGuid::NewGuid(), FSoftClassPath{}, true, NAME_None, Failure));
	FAPSProductionCommand Command;
	Command.ActionId = TEXT("APS.Production.Enqueue");
	Command.ContextStableId = ContextId;
	Command.ContextActor = OldActor;
	Command.AccessMode = EAPSProductionAccessMode::ActorGated;
	const FAPSProductionCommandResult CommandResult =
		ContextFirst->ExecuteCommand(Command);
	TestEqual(TEXT("Quarantine rejects gameplay command semantically"),
		CommandResult.FailureCode,
		FName(TEXT("APS.Production.PersistenceLoadInProgress")));
	ContextFirst->AdvanceProduction(100.0);
	FAPSProductionPersistenceState RejectedExport;
	TestFalse(TEXT("Coherent export rejects pending load projection"),
		ContextFirst->TryExportPersistenceState(RejectedExport, Failure));
	TestEqual(TEXT("Export rejection identifies load quarantine"), Failure,
		FString(TEXT("APS.Production.PersistenceLoadInProgress")));

	Context.ContextActor = ReadyActor;
	TestTrue(TEXT("Same-generation actor-ready context registers before definition"),
		ContextFirst->RegisterContextForLoad(Context, LoadGenerationId, Failure));
	TestTrue(TEXT("Missing definition keeps runtime quarantined"),
		ContextFirst->IsPersistenceLoadQuarantined());
	TestTrue(TEXT("Explicit apply reports dependency wait without polling"),
		ContextFirst->TryApplyStagedPersistenceState(Failure)
			== EAPSProductionPersistenceApplyStatus::WaitingForDependencies);
	TestTrue(TEXT("Later definition completes remembered generation atomically"),
		ContextFirst->RegisterDefinition(Definition, Failure));
	TestFalse(TEXT("Successful atomic apply exits quarantine"),
		ContextFirst->IsPersistenceLoadQuarantined());
	TestFalse(TEXT("Applied generation is no longer pending"),
		ContextFirst->GetPendingLoadGenerationId(PendingGeneration));
	TestTrue(TEXT("Applied context snapshot becomes available exactly after apply"),
		ContextFirst->QuerySnapshot(ContextId, ReadyActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));
	TestEqual(TEXT("Atomic apply exposes exactly one staged job"),
		Snapshot.Jobs.Num(), 1);
	TestTrue(TEXT("Quarantined tick did not advance the staged job"),
		Snapshot.Jobs[0].State == EAPSProductionJobState::Queued
			&& Snapshot.Jobs[0].ProgressNormalized == 0.0);
	TestEqual(TEXT("Atomic apply replaces old inventory as one projection"),
		ContextFirst->GetInventoryQuantity(OwnerId, ItemId), int64{7});

	// Definition-first path cannot unlock without an exact actor-ready token;
	// a newer generation replaces pending readiness without accepting stale actors.
	UAPSProductionSubsystem* DefinitionFirst = NewObject<UAPSProductionSubsystem>();
	AActor* DefinitionFirstActor = NewObject<AActor>();
	if (!DefinitionFirst || !DefinitionFirstActor)
	{
		return false;
	}
	const FGuid OwnerId2 = FGuid::NewGuid();
	const FGuid ContextId2 = FGuid::NewGuid();
	const FGuid FirstGeneration = FGuid::NewGuid();
	const FGuid SecondGeneration = FGuid::NewGuid();
	const FAPSProductionPersistenceState FirstState = StagedState(ContextId2,
		OwnerId2, Definition, ItemId);
	FAPSProductionPersistenceState SecondState = FirstState;
	SecondState.StateId = FGuid::NewGuid();
	SecondState.EventStream.StreamId = FGuid::NewGuid();
	TestTrue(TEXT("First load generation stages"),
		DefinitionFirst->StagePersistenceState(FirstState,
			FirstGeneration, Failure));
	TestTrue(TEXT("New generation explicitly replaces pending projection"),
		DefinitionFirst->StagePersistenceState(SecondState,
			SecondGeneration, Failure));
	TestTrue(TEXT("Replacement generation is now authoritative"),
		DefinitionFirst->GetPendingLoadGenerationId(PendingGeneration));
	TestEqual(TEXT("Replacement generation remains exact"), PendingGeneration,
		SecondGeneration);
	FAPSProductionContextRegistration Context2 = ActorContext(ContextId2,
		OwnerId2, DefinitionFirstActor);
	TestFalse(TEXT("Stale generation actor-ready registration is rejected"),
		DefinitionFirst->RegisterContextForLoad(Context2,
			FirstGeneration, Failure));
	TestTrue(TEXT("Definition registration alone succeeds"),
		DefinitionFirst->RegisterDefinition(Definition, Failure));
	TestTrue(TEXT("Definition-only path remains quarantined"),
		DefinitionFirst->IsPersistenceLoadQuarantined());
	TestTrue(TEXT("Exact replacement-generation context unlocks apply"),
		DefinitionFirst->RegisterContextForLoad(Context2,
			SecondGeneration, Failure));
	TestFalse(TEXT("Definition-first atomic apply exits quarantine"),
		DefinitionFirst->IsPersistenceLoadQuarantined());

	// Same-generation projection conflict rejects the load and sanitizes old
	// actor-gated bindings/data instead of reactivating the previous world.
	UAPSProductionSubsystem* ConflictRuntime = NewObject<UAPSProductionSubsystem>();
	AActor* ConflictActor = NewObject<AActor>();
	if (!ConflictRuntime || !ConflictActor)
	{
		return false;
	}
	const FGuid OwnerId3 = FGuid::NewGuid();
	const FGuid ContextId3 = FGuid::NewGuid();
	FAPSProductionContextRegistration Context3 = ActorContext(ContextId3,
		OwnerId3, ConflictActor);
	TestTrue(TEXT("Conflict fixture binds old actor"),
		ConflictRuntime->RegisterContext(Context3, Failure));
	OldInventory.OwnerStableId = OwnerId3;
	TestTrue(TEXT("Conflict fixture seeds old inventory"),
		ConflictRuntime->SetInventoryAmount(OldInventory, Failure));
	const FGuid ConflictGeneration = FGuid::NewGuid();
	const FAPSProductionPersistenceState ConflictState = StagedState(ContextId3,
		OwnerId3, Definition, ItemId);
	TestTrue(TEXT("Conflict fixture stages first projection"),
		ConflictRuntime->StagePersistenceState(ConflictState,
			ConflictGeneration, Failure));
	FAPSProductionPersistenceState DriftedState = ConflictState;
	DriftedState.StateId = FGuid::NewGuid();
	DriftedState.Inventories[0].Quantity = 8;
	TestFalse(TEXT("Same-generation different projection conflicts"),
		ConflictRuntime->StagePersistenceState(DriftedState,
			ConflictGeneration, Failure));
	TestEqual(TEXT("Conflict is semantic"), Failure,
		FString(TEXT("APS.Production.PersistenceProjectionConflict")));
	TestFalse(TEXT("Rejected generation leaves no pending projection"),
		ConflictRuntime->GetPendingLoadGenerationId(PendingGeneration));
	TestFalse(TEXT("Rejected generation does not retain quarantine forever"),
		ConflictRuntime->IsPersistenceLoadQuarantined());
	TestEqual(TEXT("Rejected generation sanitizes old inventory"),
		ConflictRuntime->GetInventoryQuantity(OwnerId3, ItemId), int64{0});
	TestFalse(TEXT("Rejected generation never reactivates old actor binding"),
		ConflictRuntime->QuerySnapshot(ContextId3, ConflictActor,
			EAPSProductionAccessMode::ActorGated, Snapshot, Failure));

	// Event stream replacement is atomic and never replays historical lifecycle events.
	UAPSProductionEventSubsystem* Events =
		NewObject<UAPSProductionEventSubsystem>();
	if (!Events)
	{
		return false;
	}
	int32 ReplayCount = 0;
	Events->OnEventPublished().AddLambda(
		[&ReplayCount](const FAPSProductionEvent&)
		{
			++ReplayCount;
		});
	TestTrue(TEXT("Validated stream state replaces without replay"),
		Events->ReplaceStreamStateForLoad(State.EventStream, Failure));
	TestEqual(TEXT("Load replacement emits no lifecycle event"), ReplayCount, 0);
	TestEqual(TEXT("Load replacement preserves persisted StreamId"),
		Events->GetStreamId(), State.EventStream.StreamId);
	TestEqual(TEXT("Load replacement preserves sequence"),
		Events->GetLastSequence(), State.EventStream.LastSequence);
	return true;
}

#endif
