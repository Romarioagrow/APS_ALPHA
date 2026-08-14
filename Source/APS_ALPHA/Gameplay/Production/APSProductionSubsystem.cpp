#include "APSProductionSubsystem.h"

#include "APSProductionEventSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSProduction, Log, All);

namespace APSProductionRuntime
{
	const FName SelectAction(TEXT("APS.Production.Select"));
	const FName EnqueueAction(TEXT("APS.Production.Enqueue"));
	const FName CancelAction(TEXT("APS.Production.Cancel"));

	const FName AcceptedCode(TEXT("APS.Production.Accepted"));
	const FName SelectionChangedCode(TEXT("APS.Production.SelectionChanged"));
	const FName JobQueuedCode(TEXT("APS.Production.JobQueued"));
	const FName JobStartedCode(TEXT("APS.Production.JobStarted"));
	const FName AwaitingMaterializationCode(TEXT("APS.Production.AwaitingMaterialization"));
	const FName JobCompletedCode(TEXT("APS.Production.JobCompleted"));
	const FName JobCancelledCode(TEXT("APS.Production.JobCancelled"));

	const FName InvalidContext(TEXT("APS.Production.InvalidContext"));
	const FName InvalidAccess(TEXT("APS.Production.InvalidAccess"));
	const FName InvalidCommand(TEXT("APS.Production.InvalidCommand"));
	const FName StaleRevision(TEXT("APS.Production.StaleRevision"));
	const FName UnknownDefinition(TEXT("APS.Production.UnknownDefinition"));
	const FName DomainMismatch(TEXT("APS.Production.DomainMismatch"));
	const FName QueueFull(TEXT("APS.Production.QueueFull"));
	const FName InsufficientResources(TEXT("APS.Production.InsufficientResources"));
	const FName InvalidJob(TEXT("APS.Production.InvalidJob"));
	const FName InvalidTransition(TEXT("APS.Production.InvalidTransition"));
	const FName EventRejected(TEXT("APS.Production.EventRejected"));
	const FName OutputOverflow(TEXT("APS.Production.OutputOverflow"));

	FAPSLocalizedTextDescriptor ReasonDescriptor(const FName Key,
		const TCHAR* DefaultText)
	{
		FAPSLocalizedTextDescriptor Descriptor;
		Descriptor.Namespace = TEXT("APSProduction");
		Descriptor.Key = Key;
		Descriptor.DefaultText = DefaultText;
		return Descriptor;
	}
}

bool UAPSProductionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game
		|| World->WorldType == EWorldType::PIE);
}

void UAPSProductionSubsystem::Deinitialize()
{
	Definitions.Reset();
	Contexts.Reset();
	Inventories.Reset();
	SnapshotInvalidated.Clear();
	PanelRequested.Clear();
	Super::Deinitialize();
}

void UAPSProductionSubsystem::Tick(const float DeltaTime)
{
	AdvanceProduction(static_cast<double>(DeltaTime));
}

TStatId UAPSProductionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSProductionSubsystem, STATGROUP_Tickables);
}

bool UAPSProductionSubsystem::RegisterDefinition(
	const FAPSProductionDefinition& Definition, FString& OutFailure)
{
	OutFailure.Reset();
	if (!Definition.IsStructurallyValid(&OutFailure))
	{
		return false;
	}
	if (Definitions.Contains(Definition.DefinitionId))
	{
		OutFailure = TEXT("APS.Production.DuplicateDefinition");
		return false;
	}
	Definitions.Add(Definition.DefinitionId, Definition);
	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		if (Pair.Value.Domain == Definition.Domain)
		{
			BumpRevision(Pair.Value);
		}
	}
	return true;
}

bool UAPSProductionSubsystem::RegisterContext(
	const FAPSProductionContextRegistration& Registration, FString& OutFailure)
{
	OutFailure.Reset();
	if (!Registration.IsStructurallyValid(&OutFailure))
	{
		return false;
	}
	if (FContextState* Existing = Contexts.Find(Registration.ContextStableId))
	{
		const bool bCompatibleRebind = Existing->AccessMode == EAPSProductionAccessMode::ActorGated
			&& Registration.AccessMode == EAPSProductionAccessMode::ActorGated
			&& !Existing->ContextActor.IsValid()
			&& Existing->OwnerStableId == Registration.OwnerStableId
			&& Existing->Domain == Registration.Domain
			&& Existing->QueueCapacity == Registration.QueueCapacity
			&& Existing->MaximumConcurrentJobs == Registration.MaximumConcurrentJobs
			&& Existing->SpawnPadStableId == Registration.SpawnPadStableId;
		if (bCompatibleRebind)
		{
			Existing->ContextActor = Registration.ContextActor;
			BumpRevision(*Existing);
			return true;
		}
		OutFailure = TEXT("APS.Production.DuplicateContext");
		return false;
	}

	FContextState Context;
	Context.ContextStableId = Registration.ContextStableId;
	Context.OwnerStableId = Registration.OwnerStableId;
	Context.Domain = Registration.Domain;
	Context.AccessMode = Registration.AccessMode;
	Context.ContextActor = Registration.ContextActor;
	Context.QueueCapacity = Registration.QueueCapacity;
	Context.MaximumConcurrentJobs = Registration.MaximumConcurrentJobs;
	Context.SpawnPadStableId = Registration.SpawnPadStableId;
	Context.SnapshotId = FGuid::NewGuid();
	Contexts.Add(Context.ContextStableId, MoveTemp(Context));
	SnapshotInvalidated.Broadcast(Registration.ContextStableId);
	return true;
}

bool UAPSProductionSubsystem::UnregisterContext(const FGuid ContextStableId,
	AActor* ContextActor, const EAPSProductionAccessMode AccessMode, FString& OutFailure)
{
	OutFailure.Reset();
	FContextState* Context = Contexts.Find(ContextStableId);
	if (!Context)
	{
		OutFailure = APSProductionRuntime::InvalidContext.ToString();
		return false;
	}
	if (!ValidateAccess(*Context, ContextActor, AccessMode, OutFailure))
	{
		return false;
	}
	if (CountActiveJobs(*Context) > 0)
	{
		OutFailure = TEXT("APS.Production.ContextHasActiveJobs");
		return false;
	}
	if (Context->AccessMode == EAPSProductionAccessMode::ActorGated)
	{
		// Keep persisted queue/history across actor streaming. A compatible canonical actor
		// may rebind this same context later; world teardown clears the subsystem.
		Context->ContextActor.Reset();
		BumpRevision(*Context);
	}
	else
	{
		Contexts.Remove(ContextStableId);
		SnapshotInvalidated.Broadcast(ContextStableId);
	}
	return true;
}

bool UAPSProductionSubsystem::ValidateAccess(const FContextState& Context,
	AActor* ContextActor, const EAPSProductionAccessMode AccessMode,
	FString& OutFailure) const
{
	if (Context.AccessMode != AccessMode)
	{
		OutFailure = APSProductionRuntime::InvalidAccess.ToString();
		return false;
	}
	if (AccessMode == EAPSProductionAccessMode::ActorGated)
	{
		if (!IsValid(ContextActor) || !Context.ContextActor.IsValid()
			|| Context.ContextActor.Get() != ContextActor)
		{
			OutFailure = TEXT("APS.Production.ContextActorMismatch");
			return false;
		}
	}
	else if (IsValid(ContextActor) || Context.ContextActor.IsValid())
	{
		OutFailure = TEXT("APS.Production.DebugContextActorRejected");
		return false;
	}
	return true;
}

bool UAPSProductionSubsystem::QuerySnapshot(const FGuid ContextStableId,
	AActor* ContextActor, const EAPSProductionAccessMode AccessMode,
	FAPSProductionSnapshot& OutSnapshot, FString& OutFailure) const
{
	using namespace APSProductionRuntime;
	OutSnapshot = FAPSProductionSnapshot{};
	OutFailure.Reset();
	const FContextState* Context = Contexts.Find(ContextStableId);
	if (!Context)
	{
		OutFailure = InvalidContext.ToString();
		return false;
	}
	if (!ValidateAccess(*Context, ContextActor, AccessMode, OutFailure))
	{
		return false;
	}

	OutSnapshot.SnapshotId = Context->SnapshotId;
	OutSnapshot.Revision = Context->Revision;
	OutSnapshot.ContextStableId = Context->ContextStableId;
	OutSnapshot.OwnerStableId = Context->OwnerStableId;
	OutSnapshot.Domain = Context->Domain;
	OutSnapshot.AccessMode = Context->AccessMode;
	OutSnapshot.CurrentSelectionId = Context->SelectedDefinitionId;
	OutSnapshot.QueueCapacity = Context->QueueCapacity;
	const bool bQueueFull = CountActiveJobs(*Context) >= Context->QueueCapacity;

	for (const TPair<FPrimaryAssetId, FAPSProductionDefinition>& Pair : Definitions)
	{
		const FAPSProductionDefinition& Definition = Pair.Value;
		if (Definition.Domain != Context->Domain)
		{
			continue;
		}
		FAPSProductionDefinitionSnapshot& Entry = OutSnapshot.Definitions.AddDefaulted_GetRef();
		Entry.DefinitionId = Definition.DefinitionId;
		Entry.DefinitionSchemaVersion = Definition.DefinitionSchemaVersion;
		Entry.Category = Definition.Category;
		Entry.DisplayName = Definition.DisplayName;
		Entry.Costs = Definition.Costs;
		Entry.Outputs = Definition.Outputs;
		Entry.MaximumBatchSize = Definition.MaximumBatchSize;
		Entry.ActionIds = {SelectAction, EnqueueAction};
		Entry.bAvailable = !bQueueFull && CanAfford(*Context, Definition, 1);
		if (bQueueFull)
		{
			Entry.UnavailableReason = ReasonDescriptor(QueueFull, TEXT("Production queue is full."));
		}
		else if (!Entry.bAvailable)
		{
			Entry.UnavailableReason = ReasonDescriptor(InsufficientResources,
				TEXT("Required resources are not available."));
		}
	}
	OutSnapshot.Definitions.Sort([](const FAPSProductionDefinitionSnapshot& Left,
		const FAPSProductionDefinitionSnapshot& Right)
		{
			return Left.DefinitionId.ToString() < Right.DefinitionId.ToString();
		});

	TArray<const FAPSProductionJobRecord*> OrderedJobs;
	OrderedJobs.Reserve(Context->Jobs.Num());
	for (const FAPSProductionJobRecord& Job : Context->Jobs)
	{
		OrderedJobs.Add(&Job);
	}
	OrderedJobs.Sort([](const FAPSProductionJobRecord& Left,
		const FAPSProductionJobRecord& Right)
		{
			return Left.QueueOrdinal < Right.QueueOrdinal;
		});
	int32 ActivePosition = 0;
	for (const FAPSProductionJobRecord* Job : OrderedJobs)
	{
		FAPSProductionJobSnapshot& Entry = OutSnapshot.Jobs.AddDefaulted_GetRef();
		Entry.JobId = Job->JobId;
		Entry.DefinitionId = Job->DefinitionId;
		Entry.Quantity = Job->Quantity;
		Entry.State = Job->State;
		if (!Job->IsTerminal())
		{
			Entry.QueuePosition = ActivePosition++;
			Entry.ActionIds.Add(CancelAction);
		}
		Entry.ProgressNormalized = Job->DurationSeconds <= 0.0
			? (Job->State == EAPSProductionJobState::Queued ? 0.0 : 1.0)
			: FMath::Clamp(Job->ProgressSeconds / Job->DurationSeconds, 0.0, 1.0);
		if (Job->State == EAPSProductionJobState::AwaitingMaterialization)
		{
			Entry.StatusReason = ReasonDescriptor(AwaitingMaterializationCode,
				TEXT("Awaiting world materialization."));
		}
		else if (Job->State == EAPSProductionJobState::Failed)
		{
			Entry.StatusReason = ReasonDescriptor(Job->FailureCode,
				TEXT("Production job failed."));
		}
	}
	return true;
}

bool UAPSProductionSubsystem::RequestPanel(const FGuid ContextStableId,
	AActor* ContextActor, const EAPSProductionAccessMode AccessMode,
	FString& OutFailure)
{
	FAPSProductionSnapshot Snapshot;
	if (!QuerySnapshot(ContextStableId, ContextActor, AccessMode, Snapshot, OutFailure))
	{
		return false;
	}
	PanelRequested.Broadcast(Snapshot);
	return true;
}

bool UAPSProductionSubsystem::ValidateCommand(const FAPSProductionCommand& Command,
	const FContextState& Context, FString& OutFailure) const
{
	using namespace APSProductionRuntime;
	if (Command.ContextStableId != Context.ContextStableId || Command.Quantity < 1)
	{
		OutFailure = InvalidCommand.ToString();
		return false;
	}
	if (Command.ExpectedRevision != Context.Revision)
	{
		OutFailure = StaleRevision.ToString();
		return false;
	}
	if (Context.AccessMode == EAPSProductionAccessMode::ActorGated
		&& (!Command.SubjectStableId.IsValid() || !IsValid(Command.InstigatorActor)))
	{
		OutFailure = TEXT("APS.Production.MissingCanonicalSubject");
		return false;
	}
	if (Command.ActionId != SelectAction && Command.ActionId != EnqueueAction
		&& Command.ActionId != CancelAction)
	{
		OutFailure = TEXT("APS.Production.UnknownAction");
		return false;
	}
	if ((Command.ActionId == SelectAction || Command.ActionId == EnqueueAction)
		&& !Command.DefinitionId.IsValid())
	{
		OutFailure = UnknownDefinition.ToString();
		return false;
	}
	if (Command.ActionId == CancelAction && !Command.JobId.IsValid())
	{
		OutFailure = InvalidJob.ToString();
		return false;
	}
	return true;
}

FAPSProductionCommandResult UAPSProductionSubsystem::ExecuteCommand(
	const FAPSProductionCommand& Command)
{
	using namespace APSProductionRuntime;
	FAPSProductionCommandResult Result;
	Result.CorrelationId = Command.CorrelationId;
	FContextState* Context = Contexts.Find(Command.ContextStableId);
	if (!Context)
	{
		Result.FailureCode = InvalidContext;
		return Result;
	}
	Result.Revision = Context->Revision;
	FString Failure;
	if (!ValidateAccess(*Context, Command.ContextActor, Command.AccessMode, Failure))
	{
		Result.FailureCode = FName(*Failure);
		return Result;
	}
	if (!ValidateCommand(Command, *Context, Failure))
	{
		Result.FailureCode = FName(*Failure);
		return Result;
	}

	if (Command.ActionId == SelectAction)
	{
		const FAPSProductionDefinition* Definition = Definitions.Find(Command.DefinitionId);
		if (!Definition)
		{
			Result.FailureCode = UnknownDefinition;
			return Result;
		}
		if (Definition->Domain != Context->Domain)
		{
			Result.FailureCode = DomainMismatch;
			return Result;
		}
		Context->SelectedDefinitionId = Definition->DefinitionId;
		BumpRevision(*Context);
		Result.Status = EAPSProductionCommandStatus::Accepted;
		Result.ResultCode = SelectionChangedCode;
		Result.Revision = Context->Revision;
		return Result;
	}

	if (Command.ActionId == CancelAction)
	{
		FAPSProductionJobRecord* Job = Context->Jobs.FindByPredicate(
			[&Command](const FAPSProductionJobRecord& Candidate)
			{
				return Candidate.JobId == Command.JobId;
			});
		if (!Job)
		{
			Result.FailureCode = InvalidJob;
			return Result;
		}
		if (Job->IsTerminal())
		{
			Result.FailureCode = InvalidTransition;
			return Result;
		}
		const FAPSProductionDefinition* Definition = Definitions.Find(Job->DefinitionId);
		if (!Definition)
		{
			Result.FailureCode = UnknownDefinition;
			return Result;
		}
		if (!PublishJobEvent(*Context, *Job, EAPSProductionEventResult::Cancelled,
			JobCancelledCode, NAME_None, Failure))
		{
			Result.FailureCode = EventRejected;
			return Result;
		}
		if (Job->bInputsReserved)
		{
			RefundCosts(*Context, *Definition, Job->Quantity);
			Job->bInputsReserved = false;
		}
		Job->State = EAPSProductionJobState::Cancelled;
		BumpRevision(*Context);
		Result.CorrelationId = Job->CorrelationId;
		Result.JobId = Job->JobId;
		Result.Status = EAPSProductionCommandStatus::Accepted;
		Result.ResultCode = JobCancelledCode;
		Result.Revision = Context->Revision;
		return Result;
	}

	const FAPSProductionDefinition* Definition = Definitions.Find(Command.DefinitionId);
	if (!Definition)
	{
		Result.FailureCode = UnknownDefinition;
		return Result;
	}
	if (Definition->Domain != Context->Domain)
	{
		Result.FailureCode = DomainMismatch;
		return Result;
	}
	if (Command.Quantity > Definition->MaximumBatchSize)
	{
		Result.FailureCode = TEXT("APS.Production.BatchTooLarge");
		return Result;
	}
	if (CountActiveJobs(*Context) >= Context->QueueCapacity)
	{
		Result.FailureCode = QueueFull;
		return Result;
	}
	if (!CanAfford(*Context, *Definition, Command.Quantity))
	{
		Result.FailureCode = InsufficientResources;
		return Result;
	}

	FAPSProductionJobRecord Job;
	Job.JobId = FGuid::NewGuid();
	Job.CorrelationId = Command.CorrelationId.IsValid()
		? Command.CorrelationId : FGuid::NewGuid();
	Job.SubjectStableId = Command.SubjectStableId;
	Job.ContextStableId = Context->ContextStableId;
	Job.OwnerStableId = Context->OwnerStableId;
	Job.DefinitionId = Definition->DefinitionId;
	Job.DefinitionSchemaVersion = Definition->DefinitionSchemaVersion;
	Job.Domain = Definition->Domain;
	Job.Quantity = Command.Quantity;
	Job.State = EAPSProductionJobState::Queued;
	Job.DurationSeconds = Definition->DurationSeconds * Command.Quantity;
	Job.QueueOrdinal = Context->NextQueueOrdinal++;
	Job.SpawnPadStableId = Context->SpawnPadStableId;
	Job.bDebugOnly = Context->AccessMode == EAPSProductionAccessMode::DebugLauncher;
	if (!FMath::IsFinite(Job.DurationSeconds))
	{
		Result.FailureCode = TEXT("APS.Production.DurationOverflow");
		return Result;
	}
	if (!PublishJobEvent(*Context, Job, EAPSProductionEventResult::Requested,
		JobQueuedCode, NAME_None, Failure))
	{
		Result.FailureCode = EventRejected;
		return Result;
	}
	if (!ReserveCosts(*Context, *Definition, Command.Quantity, Failure))
	{
		Result.FailureCode = FName(*Failure);
		return Result;
	}
	Job.bInputsReserved = !Definition->Costs.IsEmpty();
	Context->Jobs.Add(Job);
	BumpRevision(*Context);
	Result.CorrelationId = Job.CorrelationId;
	Result.JobId = Job.JobId;
	Result.Status = EAPSProductionCommandStatus::Accepted;
	Result.ResultCode = JobQueuedCode;
	Result.Revision = Context->Revision;
	return Result;
}

bool UAPSProductionSubsystem::CheckedScaleQuantity(const int64 UnitQuantity,
	const int32 Multiplier, int64& OutQuantity)
{
	if (UnitQuantity < 0 || Multiplier < 0
		|| (Multiplier > 0 && UnitQuantity > MAX_int64 / Multiplier))
	{
		return false;
	}
	OutQuantity = UnitQuantity * static_cast<int64>(Multiplier);
	return true;
}

int64 UAPSProductionSubsystem::GetInventoryQuantity(const FGuid& OwnerStableId,
	const FPrimaryAssetId& ItemId) const
{
	const TMap<FPrimaryAssetId, FInventoryEntry>* Inventory = Inventories.Find(OwnerStableId);
	const FInventoryEntry* Entry = Inventory ? Inventory->Find(ItemId) : nullptr;
	return Entry ? Entry->Quantity : 0;
}

bool UAPSProductionSubsystem::CanAfford(const FContextState& Context,
	const FAPSProductionDefinition& Definition, const int32 Quantity) const
{
	for (const FAPSProductionAmount& Cost : Definition.Costs)
	{
		int64 Required = 0;
		if (!CheckedScaleQuantity(Cost.Quantity, Quantity, Required)
			|| GetInventoryQuantity(Context.OwnerStableId, Cost.ItemId) < Required)
		{
			return false;
		}
	}
	return true;
}

bool UAPSProductionSubsystem::ReserveCosts(const FContextState& Context,
	const FAPSProductionDefinition& Definition, const int32 Quantity,
	FString& OutFailure)
{
	if (!CanAfford(Context, Definition, Quantity))
	{
		OutFailure = APSProductionRuntime::InsufficientResources.ToString();
		return false;
	}
	TMap<FPrimaryAssetId, FInventoryEntry>& Inventory = Inventories.FindOrAdd(Context.OwnerStableId);
	for (const FAPSProductionAmount& Cost : Definition.Costs)
	{
		int64 Required = 0;
		CheckedScaleQuantity(Cost.Quantity, Quantity, Required);
		FInventoryEntry& Entry = Inventory.FindOrAdd(Cost.ItemId);
		Entry.SchemaVersion = Cost.ItemSchemaVersion;
		Entry.Quantity -= Required;
	}
	return true;
}

void UAPSProductionSubsystem::RefundCosts(const FContextState& Context,
	const FAPSProductionDefinition& Definition, const int32 Quantity)
{
	TMap<FPrimaryAssetId, FInventoryEntry>& Inventory = Inventories.FindOrAdd(Context.OwnerStableId);
	for (const FAPSProductionAmount& Cost : Definition.Costs)
	{
		int64 Refund = 0;
		if (!CheckedScaleQuantity(Cost.Quantity, Quantity, Refund))
		{
			continue;
		}
		FInventoryEntry& Entry = Inventory.FindOrAdd(Cost.ItemId);
		Entry.SchemaVersion = Cost.ItemSchemaVersion;
		if (Entry.Quantity > MAX_int64 - Refund)
		{
			UE_LOG(LogAPSProduction, Error,
				TEXT("[APS.Production] Inventory refund overflow owner=%s item=%s"),
				*Context.OwnerStableId.ToString(), *Cost.ItemId.ToString());
			Entry.Quantity = MAX_int64;
		}
		else
		{
			Entry.Quantity += Refund;
		}
	}
}

bool UAPSProductionSubsystem::ApplyOutputs(const FContextState& Context,
	const FAPSProductionDefinition& Definition, const int32 Quantity,
	FString& OutFailure)
{
	TMap<FPrimaryAssetId, FInventoryEntry>& Inventory = Inventories.FindOrAdd(Context.OwnerStableId);
	for (const FAPSProductionAmount& Output : Definition.Outputs)
	{
		int64 Produced = 0;
		const int64 Existing = GetInventoryQuantity(Context.OwnerStableId, Output.ItemId);
		if (!CheckedScaleQuantity(Output.Quantity, Quantity, Produced)
			|| Existing > MAX_int64 - Produced)
		{
			OutFailure = APSProductionRuntime::OutputOverflow.ToString();
			return false;
		}
	}
	for (const FAPSProductionAmount& Output : Definition.Outputs)
	{
		int64 Produced = 0;
		CheckedScaleQuantity(Output.Quantity, Quantity, Produced);
		FInventoryEntry& Entry = Inventory.FindOrAdd(Output.ItemId);
		Entry.SchemaVersion = Output.ItemSchemaVersion;
		Entry.Quantity += Produced;
	}
	return true;
}

bool UAPSProductionSubsystem::SetInventoryAmount(
	const FAPSProductionInventoryRecord& Record, FString& OutFailure)
{
	OutFailure.Reset();
	if (!Record.OwnerStableId.IsValid() || !Record.ItemId.IsValid()
		|| Record.ItemSchemaVersion < 1 || Record.Quantity < 0)
	{
		OutFailure = TEXT("APS.Production.InvalidInventoryRecord");
		return false;
	}
	FInventoryEntry& Entry = Inventories.FindOrAdd(Record.OwnerStableId)
		.FindOrAdd(Record.ItemId);
	Entry.SchemaVersion = Record.ItemSchemaVersion;
	Entry.Quantity = Record.Quantity;
	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		if (Pair.Value.OwnerStableId == Record.OwnerStableId)
		{
			BumpRevision(Pair.Value);
		}
	}
	return true;
}

bool UAPSProductionSubsystem::ApplyInventoryDelta(const FGuid& OwnerStableId,
	const FAPSProductionAmount& Amount, const bool bAdd, FString& OutFailure)
{
	OutFailure.Reset();
	if (!OwnerStableId.IsValid() || !Amount.IsStructurallyValid(&OutFailure))
	{
		return false;
	}
	FInventoryEntry& Entry = Inventories.FindOrAdd(OwnerStableId).FindOrAdd(Amount.ItemId);
	if (bAdd && Entry.Quantity > MAX_int64 - Amount.Quantity)
	{
		OutFailure = TEXT("APS.Production.InventoryOverflow");
		return false;
	}
	if (!bAdd && Entry.Quantity < Amount.Quantity)
	{
		OutFailure = APSProductionRuntime::InsufficientResources.ToString();
		return false;
	}
	Entry.SchemaVersion = Amount.ItemSchemaVersion;
	Entry.Quantity += bAdd ? Amount.Quantity : -Amount.Quantity;
	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		if (Pair.Value.OwnerStableId == OwnerStableId)
		{
			BumpRevision(Pair.Value);
		}
	}
	return true;
}

FName UAPSProductionSubsystem::VerbForDomain(const EAPSProductionDomain Domain)
{
	switch (Domain)
	{
	case EAPSProductionDomain::Crafting: return TEXT("APS.Crafting.Craft");
	case EAPSProductionDomain::Building: return TEXT("APS.Building.Build");
	case EAPSProductionDomain::Shipyard: return TEXT("APS.Shipyard.BuildShip");
	default: return NAME_None;
	}
}

bool UAPSProductionSubsystem::PublishJobEvent(const FContextState& Context,
	const FAPSProductionJobRecord& Job, const EAPSProductionEventResult EventResult,
	const FName ResultCode, const FName FailureCode, FString& OutFailure) const
{
	UWorld* World = GetWorld();
	UAPSProductionEventSubsystem* Events = World
		? World->GetSubsystem<UAPSProductionEventSubsystem>() : nullptr;
	if (!Events)
	{
		// Enables deterministic NewObject contract tests. Runtime worlds always own the event subsystem.
		return true;
	}
	FAPSProductionEvent Event;
	Event.CorrelationId = Job.CorrelationId;
	Event.Verb = VerbForDomain(Job.Domain);
	Event.SubjectStableId = Job.SubjectStableId;
	Event.TargetStableId = Job.JobId;
	Event.DefinitionId = Job.DefinitionId;
	Event.DefinitionSchemaVersion = Job.DefinitionSchemaVersion;
	Event.Quantity = Job.Quantity;
	Event.Result = EventResult;
	Event.ResultCode = ResultCode;
	Event.FailureCode = FailureCode;
	Event.ContextTags.Add(Context.Domain == EAPSProductionDomain::Crafting
		? FName(TEXT("APS.Crafting"))
		: Context.Domain == EAPSProductionDomain::Building
			? FName(TEXT("APS.Building")) : FName(TEXT("APS.Shipyard")));
	FAPSProductionEventPublishPolicy Policy;
	Policy.bDebugOnly = Context.AccessMode == EAPSProductionAccessMode::DebugLauncher;
	return Events->PublishEvent(Event, Policy, OutFailure);
}

int32 UAPSProductionSubsystem::CountActiveJobs(const FContextState& Context) const
{
	int32 Count = 0;
	for (const FAPSProductionJobRecord& Job : Context.Jobs)
	{
		if (!Job.IsTerminal())
		{
			++Count;
		}
	}
	return Count;
}

int32 UAPSProductionSubsystem::CountInProgressJobs(const FContextState& Context) const
{
	int32 Count = 0;
	for (const FAPSProductionJobRecord& Job : Context.Jobs)
	{
		if (Job.State == EAPSProductionJobState::InProgress
			|| Job.State == EAPSProductionJobState::AwaitingMaterialization)
		{
			++Count;
		}
	}
	return Count;
}

void UAPSProductionSubsystem::StartQueuedJobs(FContextState& Context)
{
	int32 Running = CountInProgressJobs(Context);
	if (Running >= Context.MaximumConcurrentJobs)
	{
		return;
	}
	Context.Jobs.Sort([](const FAPSProductionJobRecord& Left,
		const FAPSProductionJobRecord& Right)
		{
			return Left.QueueOrdinal < Right.QueueOrdinal;
		});
	for (FAPSProductionJobRecord& Job : Context.Jobs)
	{
		if (Job.State != EAPSProductionJobState::Queued)
		{
			continue;
		}
		FString Failure;
		if (!PublishJobEvent(Context, Job, EAPSProductionEventResult::Started,
			APSProductionRuntime::JobStartedCode, NAME_None, Failure))
		{
			Job.State = EAPSProductionJobState::Failed;
			Job.FailureCode = APSProductionRuntime::EventRejected;
			const FAPSProductionDefinition* Definition = Definitions.Find(Job.DefinitionId);
			if (Definition && Job.bInputsReserved)
			{
				RefundCosts(Context, *Definition, Job.Quantity);
				Job.bInputsReserved = false;
			}
		}
		else
		{
			Job.State = EAPSProductionJobState::InProgress;
			++Running;
		}
		BumpRevision(Context);
		if (Running >= Context.MaximumConcurrentJobs)
		{
			break;
		}
	}
}

void UAPSProductionSubsystem::CompleteJob(FContextState& Context,
	FAPSProductionJobRecord& Job)
{
	const FAPSProductionDefinition* Definition = Definitions.Find(Job.DefinitionId);
	if (!Definition)
	{
		Job.State = EAPSProductionJobState::Failed;
		Job.FailureCode = APSProductionRuntime::UnknownDefinition;
		BumpRevision(Context);
		return;
	}
	if (Job.Domain != EAPSProductionDomain::Crafting)
	{
		Job.State = EAPSProductionJobState::AwaitingMaterialization;
		BumpRevision(Context);
		return;
	}

	FString Failure;
	if (!ApplyOutputs(Context, *Definition, Job.Quantity, Failure))
	{
		Job.State = EAPSProductionJobState::Failed;
		Job.FailureCode = FName(*Failure);
		if (Job.bInputsReserved)
		{
			RefundCosts(Context, *Definition, Job.Quantity);
			Job.bInputsReserved = false;
		}
		PublishJobEvent(Context, Job, EAPSProductionEventResult::Failed,
			NAME_None, Job.FailureCode, Failure);
		BumpRevision(Context);
		return;
	}
	if (!PublishJobEvent(Context, Job, EAPSProductionEventResult::Succeeded,
		APSProductionRuntime::JobCompletedCode, NAME_None, Failure))
	{
		Job.State = EAPSProductionJobState::Failed;
		Job.FailureCode = APSProductionRuntime::EventRejected;
		BumpRevision(Context);
		return;
	}
	Job.State = EAPSProductionJobState::Succeeded;
	Job.bInputsReserved = false;
	BumpRevision(Context);
}

void UAPSProductionSubsystem::AdvanceProduction(const double DeltaSeconds)
{
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0)
	{
		return;
	}
	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		FContextState& Context = Pair.Value;
		StartQueuedJobs(Context);
		for (FAPSProductionJobRecord& Job : Context.Jobs)
		{
			if (Job.State != EAPSProductionJobState::InProgress)
			{
				continue;
			}
			const double PreviousProgress = Job.ProgressSeconds;
			Job.ProgressSeconds = FMath::Min(Job.DurationSeconds,
				Job.ProgressSeconds + DeltaSeconds);
			if (Job.ProgressSeconds >= Job.DurationSeconds)
			{
				CompleteJob(Context, Job);
			}
			else if (Job.ProgressSeconds != PreviousProgress)
			{
				BumpRevision(Context);
			}
		}
	}
}

bool UAPSProductionSubsystem::ResolveMaterialization(const FGuid& ContextStableId,
	const FGuid& JobId, const FGuid& MaterializedActorStableId,
	const FSoftClassPath& ResolvedClassSnapshot, const bool bSucceeded,
	const FName FailureCode, FString& OutFailure)
{
	using namespace APSProductionRuntime;
	OutFailure.Reset();
	FContextState* Context = Contexts.Find(ContextStableId);
	FAPSProductionJobRecord* Job = Context
		? Context->Jobs.FindByPredicate([&JobId](const FAPSProductionJobRecord& Candidate)
			{
				return Candidate.JobId == JobId;
			}) : nullptr;
	if (!Context || !Job)
	{
		OutFailure = InvalidJob.ToString();
		return false;
	}
	if (Job->State != EAPSProductionJobState::AwaitingMaterialization)
	{
		OutFailure = InvalidTransition.ToString();
		return false;
	}
	if (bSucceeded && !MaterializedActorStableId.IsValid())
	{
		OutFailure = TEXT("APS.Production.MissingMaterializedActorId");
		return false;
	}
	if (!bSucceeded && FailureCode.IsNone())
	{
		OutFailure = TEXT("APS.Production.MissingMaterializationFailure");
		return false;
	}

	if (!PublishJobEvent(*Context, *Job,
		bSucceeded ? EAPSProductionEventResult::Succeeded : EAPSProductionEventResult::Failed,
		bSucceeded ? JobCompletedCode : NAME_None,
		bSucceeded ? NAME_None : FailureCode, OutFailure))
	{
		return false;
	}
	Job->ResolvedClassSnapshot = ResolvedClassSnapshot;
	Job->MaterializedActorStableId = MaterializedActorStableId;
	Job->State = bSucceeded ? EAPSProductionJobState::Succeeded
		: EAPSProductionJobState::Failed;
	Job->FailureCode = bSucceeded ? NAME_None : FailureCode;
	if (!bSucceeded && Job->bInputsReserved)
	{
		if (const FAPSProductionDefinition* Definition = Definitions.Find(Job->DefinitionId))
		{
			RefundCosts(*Context, *Definition, Job->Quantity);
		}
	}
	Job->bInputsReserved = false;
	BumpRevision(*Context);
	return true;
}

void UAPSProductionSubsystem::BumpRevision(FContextState& Context)
{
	if (Context.Revision == MAX_int64)
	{
		Context.SnapshotId = FGuid::NewGuid();
		Context.Revision = 1;
	}
	else
	{
		++Context.Revision;
	}
	SnapshotInvalidated.Broadcast(Context.ContextStableId);
}

void UAPSProductionSubsystem::ExportPersistenceState(
	FAPSProductionPersistenceState& OutState) const
{
	OutState = FAPSProductionPersistenceState{};
	TSet<FGuid> PersistedOwners;
	for (const TPair<FGuid, FContextState>& Pair : Contexts)
	{
		const FContextState& Context = Pair.Value;
		if (Context.AccessMode != EAPSProductionAccessMode::ActorGated)
		{
			continue;
		}
		PersistedOwners.Add(Context.OwnerStableId);
		for (const FAPSProductionJobRecord& Job : Context.Jobs)
		{
			if (!Job.bDebugOnly)
			{
				OutState.Jobs.Add(Job);
			}
		}
	}
	for (const FGuid& OwnerStableId : PersistedOwners)
	{
		const TMap<FPrimaryAssetId, FInventoryEntry>* Inventory = Inventories.Find(OwnerStableId);
		if (!Inventory)
		{
			continue;
		}
		for (const TPair<FPrimaryAssetId, FInventoryEntry>& Pair : *Inventory)
		{
			FAPSProductionInventoryRecord& Record = OutState.Inventories.AddDefaulted_GetRef();
			Record.OwnerStableId = OwnerStableId;
			Record.ItemId = Pair.Key;
			Record.ItemSchemaVersion = Pair.Value.SchemaVersion;
			Record.Quantity = Pair.Value.Quantity;
		}
	}
	OutState.Jobs.Sort([](const FAPSProductionJobRecord& Left,
		const FAPSProductionJobRecord& Right)
		{
			return Left.QueueOrdinal < Right.QueueOrdinal;
		});
}

bool UAPSProductionSubsystem::RestorePersistenceState(
	const FAPSProductionPersistenceState& State, FString& OutFailure)
{
	OutFailure.Reset();
	if (State.SchemaVersion != 1)
	{
		OutFailure = TEXT("APS.Production.UnsupportedPersistenceSchema");
		return false;
	}
	TSet<FGuid> JobIds;
	TSet<FGuid> CorrelationIds;
	for (const FAPSProductionJobRecord& Job : State.Jobs)
	{
		const FContextState* Context = Contexts.Find(Job.ContextStableId);
		const FAPSProductionDefinition* Definition = Definitions.Find(Job.DefinitionId);
		if (!Job.IsStructurallyValid(&OutFailure) || Job.bDebugOnly || !Context
			|| Context->AccessMode != EAPSProductionAccessMode::ActorGated
			|| Context->OwnerStableId != Job.OwnerStableId || Context->Domain != Job.Domain
			|| !Definition || Definition->DefinitionSchemaVersion != Job.DefinitionSchemaVersion
			|| JobIds.Contains(Job.JobId) || CorrelationIds.Contains(Job.CorrelationId))
		{
			if (OutFailure.IsEmpty())
			{
				OutFailure = TEXT("APS.Production.InvalidPersistedJob");
			}
			return false;
		}
		JobIds.Add(Job.JobId);
		CorrelationIds.Add(Job.CorrelationId);
	}
	for (const FAPSProductionInventoryRecord& Record : State.Inventories)
	{
		if (!Record.OwnerStableId.IsValid() || !Record.ItemId.IsValid()
			|| Record.ItemSchemaVersion < 1 || Record.Quantity < 0)
		{
			OutFailure = TEXT("APS.Production.InvalidPersistedInventory");
			return false;
		}
	}

	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		if (Pair.Value.AccessMode == EAPSProductionAccessMode::ActorGated)
		{
			Pair.Value.Jobs.Reset();
			Pair.Value.NextQueueOrdinal = 1;
		}
	}
	Inventories.Reset();
	for (const FAPSProductionInventoryRecord& Record : State.Inventories)
	{
		FInventoryEntry& Entry = Inventories.FindOrAdd(Record.OwnerStableId)
			.FindOrAdd(Record.ItemId);
		Entry.SchemaVersion = Record.ItemSchemaVersion;
		Entry.Quantity = Record.Quantity;
	}
	for (const FAPSProductionJobRecord& Job : State.Jobs)
	{
		FContextState& Context = Contexts.FindChecked(Job.ContextStableId);
		Context.Jobs.Add(Job);
		Context.NextQueueOrdinal = FMath::Max(Context.NextQueueOrdinal,
			Job.QueueOrdinal + 1);
	}
	for (TPair<FGuid, FContextState>& Pair : Contexts)
	{
		if (Pair.Value.AccessMode == EAPSProductionAccessMode::ActorGated)
		{
			BumpRevision(Pair.Value);
		}
	}
	return true;
}
