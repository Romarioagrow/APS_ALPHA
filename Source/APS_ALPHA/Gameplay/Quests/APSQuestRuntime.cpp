#include "APSQuestRuntime.h"

#include "Misc/SecureHash.h"

namespace
{
	const FName MissingDefinitionCode(TEXT("Quest.Definition.Missing"));
	const FName VersionMismatchCode(TEXT("Quest.Definition.VersionMismatch"));
	const FName CancelledCode(TEXT("Quest.Action.Cancelled"));

	FName EffectiveFailureCode(const FAPSQuestEvent& Event)
	{
		if (!Event.FailureCode.IsNone())
		{
			return Event.FailureCode;
		}
		return Event.Result == EAPSQuestEventResult::Cancelled ? CancelledCode : NAME_None;
	}
}

FAPSQuestRuntime::FAPSQuestRuntime()
{
	BeginPromptSession(FGuid::NewGuid());
}

bool FAPSQuestRuntime::RegisterDefinition(const UAPSQuestDefinition* Definition,
	FString& OutReason)
{
	OutReason.Reset();
	if (!IsValid(Definition))
	{
		OutReason = TEXT("Quest definition is invalid");
		return false;
	}
	TArray<FString> Errors;
	if (!Definition->ValidateDefinition(Errors))
	{
		OutReason = FString::Join(Errors, TEXT("; "));
		return false;
	}
	if (const UAPSQuestDefinition* Existing = FindDefinition(Definition->QuestId))
	{
		if (Existing != Definition || Existing->DefinitionVersion != Definition->DefinitionVersion)
		{
			OutReason = FString::Printf(TEXT("Quest %s is already registered"),
				*Definition->QuestId.ToString());
			return false;
		}
		return true;
	}
	Definitions.Add(Definition->QuestId, Definition);
	return true;
}

bool FAPSQuestRuntime::StartQuest(FName QuestId, const FGuid& InstanceId,
	FString& OutReason)
{
	OutReason.Reset();
	const UAPSQuestDefinition* Definition = FindDefinition(QuestId);
	if (!Definition)
	{
		OutReason = TEXT("Quest definition is not registered");
		return false;
	}
	if (!InstanceId.IsValid())
	{
		OutReason = TEXT("Quest instance requires a stable InstanceId");
		return false;
	}
	if (const FAPSQuestInstanceSaveData* Existing = Instances.Find(QuestId))
	{
		if (Existing->InstanceId == InstanceId
			&& Existing->DefinitionVersion == Definition->DefinitionVersion)
		{
			// World adapters may receive both their native delegate and the required
			// late-read for one canonical fact. Matching bootstrap is a safe no-op.
			return true;
		}
		OutReason = TEXT("Quest instance already exists with a different identity or version");
		return false;
	}

	FAPSQuestInstanceSaveData Instance;
	Instance.QuestId = QuestId;
	Instance.DefinitionVersion = Definition->DefinitionVersion;
	Instance.InstanceId = InstanceId;
	Instance.State = EAPSQuestInstanceState::Running;
	for (const FAPSQuestObjectiveNodeDefinition& Node : Definition->Nodes)
	{
		FAPSQuestNodeRuntimeState& RuntimeNode = Instance.Nodes.AddDefaulted_GetRef();
		RuntimeNode.NodeId = Node.NodeId;
		RuntimeNode.State = Node.NodeId == Definition->EntryNodeId
			? EAPSQuestNodeState::Active : EAPSQuestNodeState::Dormant;
	}

	FAPSQuestInstanceSaveData& Stored = Instances.Add(QuestId, MoveTemp(Instance));
	InstanceChanged.Broadcast(QuestId, Stored);
	if (const FAPSQuestObjectiveNodeDefinition* Entry = Definition->FindNode(Definition->EntryNodeId))
	{
		PublishPrompt(Stored, *Entry);
	}
	return true;
}

bool FAPSQuestRuntime::BindEntity(FName QuestId, FName BindingName,
	const FAPSQuestEntityRef& Entity, FString& OutReason)
{
	OutReason.Reset();
	FAPSQuestInstanceSaveData* Instance = FindMutableInstance(QuestId);
	if (!Instance || BindingName.IsNone() || !Entity.IsValid())
	{
		OutReason = TEXT("Binding requires a quest instance, name and canonical entity");
		return false;
	}
	const UAPSQuestDefinition* Definition = FindDefinition(QuestId);
	if (!Definition || Definition->DefinitionVersion != Instance->DefinitionVersion)
	{
		OutReason = TEXT("Binding requires the registered matching quest definition");
		return false;
	}
	const FAPSQuestBindingDefinition* BindingDefinition =
		Definition->FindBindingDefinition(BindingName);
	if (!BindingDefinition)
	{
		OutReason = FString::Printf(TEXT("Binding %s is not declared by quest %s"),
			*BindingName.ToString(), *QuestId.ToString());
		return false;
	}
	if (BindingDefinition->ExpectedKind != Entity.Kind)
	{
		OutReason = FString::Printf(TEXT("Binding %s requires identity kind %d but received %d"),
			*BindingName.ToString(), static_cast<int32>(BindingDefinition->ExpectedKind),
			static_cast<int32>(Entity.Kind));
		return false;
	}
	if (FAPSQuestNamedEntityBinding* Existing = Instance->Bindings.FindByPredicate(
		[BindingName](const FAPSQuestNamedEntityBinding& Binding)
		{
			return Binding.BindingName == BindingName;
		}))
	{
		if (!Existing->Entity.Matches(Entity))
		{
			OutReason = FString::Printf(TEXT("Binding %s cannot silently retarget from %s to %s"),
				*BindingName.ToString(), *Existing->Entity.ToDebugString(), *Entity.ToDebugString());
			return false;
		}
		return true;
	}

	FAPSQuestNamedEntityBinding& Added = Instance->Bindings.AddDefaulted_GetRef();
	Added.BindingName = BindingName;
	Added.Entity = Entity;
	InstanceChanged.Broadcast(QuestId, *Instance);
	return true;
}

bool FAPSQuestRuntime::BeginEventStream(FName QuestId, const FGuid& StreamId,
	FString& OutReason)
{
	OutReason.Reset();
	FAPSQuestInstanceSaveData* Instance = FindMutableInstance(QuestId);
	if (!Instance || !StreamId.IsValid())
	{
		OutReason = TEXT("Event stream requires a quest instance and valid StreamId");
		return false;
	}
	if (FindMutableStreamCursor(*Instance, StreamId))
	{
		return true;
	}
	FAPSQuestEventStreamCursor& Cursor = Instance->EventStreams.AddDefaulted_GetRef();
	Cursor.StreamId = StreamId;
	InstanceChanged.Broadcast(QuestId, *Instance);
	return true;
}

bool FAPSQuestRuntime::SubmitEvent(const FAPSQuestEvent& Event, FString& OutReason)
{
	OutReason.Reset();
	if (!Event.StreamId.IsValid() || Event.Sequence <= 0)
	{
		OutReason = TEXT("Authoritative quest event requires StreamId and positive Sequence");
		return false;
	}
	return SubmitEventInternal(Event, true, OutReason);
}

bool FAPSQuestRuntime::DebugInjectEvent(const FAPSQuestEvent& Event, FString& OutReason)
{
	OutReason.Reset();
#if UE_BUILD_SHIPPING
	(void)Event;
	OutReason = TEXT("Quest debug event injection is unavailable in Shipping");
	return false;
#else
	if (Event.StreamId.IsValid() || Event.Sequence != 0)
	{
		OutReason = TEXT("Quest debug event requires an empty StreamId and zero Sequence");
		return false;
	}
	return SubmitEventInternal(Event, false, OutReason);
#endif
}

bool FAPSQuestRuntime::SubmitEventInternal(const FAPSQuestEvent& Event,
	const bool bRequestRewards, FString& OutReason)
{
	OutReason.Reset();
	if (!Event.IsStructurallyValid(&OutReason))
	{
		return false;
	}
	if (Event.Sequence > 0 && !Event.StreamId.IsValid())
	{
		OutReason = TEXT("Sequenced quest event requires StreamId");
		return false;
	}

	bool bAcceptedByAnyQuest = false;
	bool bRejectedByOrdering = false;
	for (TPair<FName, FAPSQuestInstanceSaveData>& Pair : Instances)
	{
		FAPSQuestInstanceSaveData& Instance = Pair.Value;
		if (Instance.ConsumedEventIds.Contains(Event.EventId)
			|| (Event.IsTerminal() && Instance.ConsumedTerminalCorrelations.Contains(Event.CorrelationId)))
		{
			// At-least-once delivery remains a successful no-op even after completion.
			bAcceptedByAnyQuest = true;
			continue;
		}

		if (Instance.State != EAPSQuestInstanceState::Running)
		{
			continue;
		}
		const UAPSQuestDefinition* Definition = FindDefinition(Instance.QuestId);
		if (!Definition || Definition->DefinitionVersion != Instance.DefinitionVersion)
		{
			Instance.State = EAPSQuestInstanceState::Suspended;
			Instance.LastFailureCode = Definition ? VersionMismatchCode : MissingDefinitionCode;
			InstanceChanged.Broadcast(Instance.QuestId, Instance);
			continue;
		}

		// Snapshot active nodes so one event cannot cascade into a successor that was
		// activated by that same event.
		TArray<FName> ActiveNodeIds;
		for (const FAPSQuestNodeRuntimeState& Candidate : Instance.Nodes)
		{
			if (Candidate.State == EAPSQuestNodeState::Active)
			{
				ActiveNodeIds.Add(Candidate.NodeId);
			}
		}

		// Owner events are transient until an active predicate recognizes them. An
		// unrelated live event must not consume dedupe identity or advance a stream
		// cursor that a later active objective still needs.
		TArray<FName> RelevantNodeIds;
		const bool bTerminalFailure = Event.Result == EAPSQuestEventResult::Failed
			|| Event.Result == EAPSQuestEventResult::Cancelled;
		for (FName ActiveNodeId : ActiveNodeIds)
		{
			const FAPSQuestNodeRuntimeState* NodeState = FindMutableNode(Instance, ActiveNodeId);
			if (!NodeState || NodeState->State != EAPSQuestNodeState::Active)
			{
				continue;
			}
			const FAPSQuestObjectiveNodeDefinition* Node = Definition->FindNode(NodeState->NodeId);
			if (!Node)
			{
				continue;
			}

			FString MatchReason;
			if ((bTerminalFailure
					&& MatchesPredicate(Instance, Node->Trigger, Event, false, MatchReason))
				|| MatchesPredicate(Instance, Node->Trigger, Event, true, MatchReason))
			{
				RelevantNodeIds.Add(ActiveNodeId);
			}
		}
		if (RelevantNodeIds.IsEmpty())
		{
			continue;
		}

		FAPSQuestEventStreamCursor* EventStreamCursor = nullptr;
		if (Event.Sequence > 0)
		{
			EventStreamCursor = FindMutableStreamCursor(Instance, Event.StreamId);
			if (EventStreamCursor
				&& Event.Sequence <= EventStreamCursor->LastConsumedSequence)
			{
				bRejectedByOrdering = true;
				continue;
			}
			if (!EventStreamCursor)
			{
				EventStreamCursor = &Instance.EventStreams.AddDefaulted_GetRef();
				EventStreamCursor->StreamId = Event.StreamId;
			}
		}

		bool bStateChanged = false;
		for (FName RelevantNodeId : RelevantNodeIds)
		{
			FAPSQuestNodeRuntimeState* NodeState = FindMutableNode(Instance, RelevantNodeId);
			if (!NodeState || NodeState->State != EAPSQuestNodeState::Active)
			{
				continue;
			}
			const FAPSQuestObjectiveNodeDefinition* Node = Definition->FindNode(NodeState->NodeId);
			if (!Node)
			{
				continue;
			}
			if (bTerminalFailure)
			{
				NodeState->LastFailureCode = EffectiveFailureCode(Event);
				bStateChanged = true;
				continue;
			}
			NodeState->Progress = FMath::Min(Node->RequiredProgress,
				NodeState->Progress + FMath::Max(1, Event.Quantity));
			NodeState->LastFailureCode = NAME_None;
			bStateChanged = true;
			if (NodeState->Progress >= Node->RequiredProgress)
			{
				CompleteNode(Instance, *Definition, *NodeState, bRequestRewards);
			}
		}

		AppendBoundedGuid(Instance.ConsumedEventIds, Event.EventId);
		if (Event.IsTerminal())
		{
			AppendBoundedGuid(Instance.ConsumedTerminalCorrelations, Event.CorrelationId);
		}
		if (EventStreamCursor)
		{
			EventStreamCursor->LastConsumedSequence = Event.Sequence;
		}
		RefreshInstanceCompletion(Instance, *Definition);
		if (bStateChanged)
		{
			InstanceChanged.Broadcast(Instance.QuestId, Instance);
		}
		bAcceptedByAnyQuest = true;
	}

	if (!bAcceptedByAnyQuest)
	{
		OutReason = bRejectedByOrdering
			? TEXT("Out-of-order quest event rejected for its stream")
			: TEXT("No running quest accepted the event");
	}
	return bAcceptedByAnyQuest;
}

bool FAPSQuestRuntime::AcknowledgeReward(FName QuestId, const FGuid& TransactionId,
	bool bApplied, FName FailureCode, FString& OutReason)
{
	OutReason.Reset();
	FAPSQuestInstanceSaveData* Instance = FindMutableInstance(QuestId);
	if (!Instance || !TransactionId.IsValid())
	{
		OutReason = TEXT("Reward acknowledgement requires quest and transaction");
		return false;
	}
	FAPSQuestRewardLedgerEntry* Entry = Instance->RewardLedger.FindByPredicate(
		[TransactionId](const FAPSQuestRewardLedgerEntry& Candidate)
		{
			return Candidate.TransactionId == TransactionId;
		});
	if (!Entry)
	{
		OutReason = TEXT("Reward transaction is unknown");
		return false;
	}
	const EAPSQuestRewardState RequestedState = bApplied
		? EAPSQuestRewardState::Applied : EAPSQuestRewardState::Failed;
	if (Entry->State != EAPSQuestRewardState::Requested && Entry->State != RequestedState)
	{
		OutReason = TEXT("Reward transaction cannot change terminal state");
		return false;
	}
	Entry->State = RequestedState;
	Entry->FailureCode = bApplied ? NAME_None : FailureCode;
	InstanceChanged.Broadcast(QuestId, *Instance);
	return true;
}

bool FAPSQuestRuntime::SuspendQuest(FName QuestId, FName FailureCode,
	FString& OutReason)
{
	OutReason.Reset();
	FAPSQuestInstanceSaveData* Instance = FindMutableInstance(QuestId);
	if (!Instance || FailureCode.IsNone())
	{
		OutReason = TEXT("Suspension requires quest and stable failure code");
		return false;
	}
	Instance->State = EAPSQuestInstanceState::Suspended;
	Instance->LastFailureCode = FailureCode;
	for (FAPSQuestNodeRuntimeState& Node : Instance->Nodes)
	{
		if (Node.State == EAPSQuestNodeState::Active)
		{
			Node.State = EAPSQuestNodeState::Suspended;
			Node.LastFailureCode = FailureCode;
		}
	}
	RefreshCurrentPromptFromState(true);
	InstanceChanged.Broadcast(QuestId, *Instance);
	return true;
}

bool FAPSQuestRuntime::RecoverQuest(FName QuestId, FName NodeId,
	EAPSQuestRecoveryPolicy Policy, FString& OutReason)
{
	OutReason.Reset();
	FAPSQuestInstanceSaveData* Instance = FindMutableInstance(QuestId);
	const UAPSQuestDefinition* Definition = FindDefinition(QuestId);
	if (!Instance || !Definition)
	{
		OutReason = TEXT("Recovery requires quest instance and definition");
		return false;
	}
	FAPSQuestNodeRuntimeState* Node = FindMutableNode(*Instance, NodeId);
	if (!Node)
	{
		OutReason = TEXT("Recovery node does not exist");
		return false;
	}

	if (Policy == EAPSQuestRecoveryPolicy::DebugSkip)
	{
#if UE_BUILD_SHIPPING
		OutReason = TEXT("Debug skip is disabled in Shipping");
		return false;
#else
		if (Node->State == EAPSQuestNodeState::Completed)
		{
			return true;
		}
		// Debug traversal is intentionally side-effect free for production reward owners.
		CompleteNode(*Instance, *Definition, *Node, false);
		RefreshInstanceCompletion(*Instance, *Definition);
		InstanceChanged.Broadcast(QuestId, *Instance);
		return true;
#endif
	}

	if (Node->State != EAPSQuestNodeState::Suspended
		&& Node->State != EAPSQuestNodeState::Failed
		&& Instance->State != EAPSQuestInstanceState::Suspended)
	{
		OutReason = TEXT("Quest node is not recoverable in its current state");
		return false;
	}
	if (Policy == EAPSQuestRecoveryPolicy::RestartNode)
	{
		Node->Progress = 0;
	}
	Node->State = EAPSQuestNodeState::Active;
	Node->LastFailureCode = NAME_None;
	Instance->State = EAPSQuestInstanceState::Running;
	Instance->LastFailureCode = NAME_None;
	InstanceChanged.Broadcast(QuestId, *Instance);
	if (const FAPSQuestObjectiveNodeDefinition* DefinitionNode = Definition->FindNode(NodeId))
	{
		PublishPrompt(*Instance, *DefinitionNode);
	}
	return true;
}

FAPSQuestSaveData FAPSQuestRuntime::ExportSaveData() const
{
	FAPSQuestSaveData SaveData;
	Instances.GenerateValueArray(SaveData.Instances);
	SaveData.Instances.Sort([](const FAPSQuestInstanceSaveData& Left,
		const FAPSQuestInstanceSaveData& Right)
	{
		return Left.QuestId.LexicalLess(Right.QuestId);
	});
	return SaveData;
}

bool FAPSQuestRuntime::RestoreSaveData(const FAPSQuestSaveData& SaveData,
	FString& OutReason)
{
	OutReason.Reset();
	if (SaveData.SchemaVersion <= 0 || SaveData.SchemaVersion > FAPSQuestSaveData::LatestSchemaVersion)
	{
		OutReason = TEXT("Unsupported Quest save schema; existing runtime was preserved");
		return false;
	}

	TMap<FName, FAPSQuestInstanceSaveData> Restored;
	for (FAPSQuestInstanceSaveData Instance : SaveData.Instances)
	{
		if (Instance.QuestId.IsNone() || !Instance.InstanceId.IsValid()
			|| Restored.Contains(Instance.QuestId))
		{
			OutReason = TEXT("Quest save contains invalid or duplicate instance identity");
			return false;
		}
		if (SaveData.SchemaVersion == 1 && Instance.EventStreamId.IsValid())
		{
			FAPSQuestEventStreamCursor& Migrated = Instance.EventStreams.AddDefaulted_GetRef();
			Migrated.StreamId = Instance.EventStreamId;
			Migrated.LastConsumedSequence = Instance.LastConsumedSequence;
			Instance.EventStreamId.Invalidate();
			Instance.LastConsumedSequence = 0;
		}
		TSet<FGuid> RestoredStreamIds;
		for (const FAPSQuestEventStreamCursor& Cursor : Instance.EventStreams)
		{
			if (!Cursor.StreamId.IsValid() || Cursor.LastConsumedSequence < 0
				|| RestoredStreamIds.Contains(Cursor.StreamId))
			{
				OutReason = TEXT("Quest save contains invalid or duplicate event stream cursor");
				return false;
			}
			RestoredStreamIds.Add(Cursor.StreamId);
		}
		const UAPSQuestDefinition* Definition = FindDefinition(Instance.QuestId);
		if (!Definition)
		{
			Instance.State = EAPSQuestInstanceState::Suspended;
			Instance.LastFailureCode = MissingDefinitionCode;
		}
		else if (Definition->DefinitionVersion != Instance.DefinitionVersion)
		{
			Instance.State = EAPSQuestInstanceState::Suspended;
			Instance.LastFailureCode = VersionMismatchCode;
		}
		else
		{
			for (const FAPSQuestNodeRuntimeState& Node : Instance.Nodes)
			{
				if (!Definition->FindNode(Node.NodeId))
				{
					OutReason = FString::Printf(TEXT("Quest %s save references missing node %s"),
						*Instance.QuestId.ToString(), *Node.NodeId.ToString());
					return false;
				}
			}
		}
		TSet<FName> RestoredBindingNames;
		for (const FAPSQuestNamedEntityBinding& Binding : Instance.Bindings)
		{
			if (Binding.BindingName.IsNone() || !Binding.Entity.IsValid()
				|| RestoredBindingNames.Contains(Binding.BindingName))
			{
				OutReason = TEXT("Quest save contains invalid or duplicate entity binding");
				return false;
			}
			RestoredBindingNames.Add(Binding.BindingName);
			if (Definition && Definition->DefinitionVersion == Instance.DefinitionVersion)
			{
				const FAPSQuestBindingDefinition* BindingDefinition =
					Definition->FindBindingDefinition(Binding.BindingName);
				if (!BindingDefinition || BindingDefinition->ExpectedKind != Binding.Entity.Kind)
				{
					OutReason = TEXT("Quest save contains undeclared or wrong-domain entity binding");
					return false;
				}
			}
		}
		Restored.Add(Instance.QuestId, MoveTemp(Instance));
	}

	Instances = MoveTemp(Restored);
	for (const TPair<FName, FAPSQuestInstanceSaveData>& Pair : Instances)
	{
		InstanceChanged.Broadcast(Pair.Key, Pair.Value);
	}
	RefreshCurrentPromptFromState(true);
	return true;
}

const FAPSQuestInstanceSaveData* FAPSQuestRuntime::FindInstance(FName QuestId) const
{
	return Instances.Find(QuestId);
}

void FAPSQuestRuntime::BeginPromptSession(const FGuid& SessionEpoch)
{
	PromptSessionEpoch = SessionEpoch.IsValid() ? SessionEpoch : FGuid::NewGuid();
	PromptRevision = 0;
	CurrentPrompt.Reset();
	RefreshCurrentPromptFromState(false);
}

void FAPSQuestRuntime::EndPromptSession(const FGuid& SessionEpoch)
{
	if (SessionEpoch == PromptSessionEpoch)
	{
		SetPromptCleared(true);
	}
}

bool FAPSQuestRuntime::TryGetCurrentPromptSnapshot(
	FAPSQuestPromptSnapshot& OutSnapshot) const
{
	if (!CurrentPrompt.IsSet())
	{
		return false;
	}
	OutSnapshot = CurrentPrompt.GetValue();
	return true;
}

FString FAPSQuestRuntime::DumpQuest(FName QuestId) const
{
	const FAPSQuestInstanceSaveData* Instance = FindInstance(QuestId);
	if (!Instance)
	{
		return FString::Printf(TEXT("Quest %s: missing"), *QuestId.ToString());
	}
	FString Result = FString::Printf(TEXT("Quest %s v%d instance=%s state=%d streams=%d"),
		*Instance->QuestId.ToString(), Instance->DefinitionVersion,
		*Instance->InstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
		static_cast<int32>(Instance->State), Instance->EventStreams.Num());
	for (const FAPSQuestEventStreamCursor& Cursor : Instance->EventStreams)
	{
		Result += FString::Printf(TEXT("\n  stream %s seq=%lld"),
			*Cursor.StreamId.ToString(EGuidFormats::DigitsWithHyphensLower),
			Cursor.LastConsumedSequence);
	}
	for (const FAPSQuestNodeRuntimeState& Node : Instance->Nodes)
	{
		Result += FString::Printf(TEXT("\n  %s state=%d progress=%d failure=%s"),
			*Node.NodeId.ToString(), static_cast<int32>(Node.State), Node.Progress,
			*Node.LastFailureCode.ToString());
	}
	for (const FAPSQuestNamedEntityBinding& Binding : Instance->Bindings)
	{
		Result += FString::Printf(TEXT("\n  bind %s=%s"), *Binding.BindingName.ToString(),
			*Binding.Entity.ToDebugString());
	}
	return Result;
}

const UAPSQuestDefinition* FAPSQuestRuntime::FindDefinition(FName QuestId) const
{
	const UAPSQuestDefinition* const* Found = Definitions.Find(QuestId);
	return Found ? *Found : nullptr;
}

FAPSQuestInstanceSaveData* FAPSQuestRuntime::FindMutableInstance(FName QuestId)
{
	return Instances.Find(QuestId);
}

FAPSQuestNodeRuntimeState* FAPSQuestRuntime::FindMutableNode(
	FAPSQuestInstanceSaveData& Instance, FName NodeId) const
{
	return Instance.Nodes.FindByPredicate([NodeId](const FAPSQuestNodeRuntimeState& Node)
	{
		return Node.NodeId == NodeId;
	});
}

FAPSQuestEventStreamCursor* FAPSQuestRuntime::FindMutableStreamCursor(
	FAPSQuestInstanceSaveData& Instance, const FGuid& StreamId) const
{
	return Instance.EventStreams.FindByPredicate(
		[&StreamId](const FAPSQuestEventStreamCursor& Cursor)
		{
			return Cursor.StreamId == StreamId;
		});
}

const FAPSQuestNamedEntityBinding* FAPSQuestRuntime::FindBinding(
	const FAPSQuestInstanceSaveData& Instance, FName BindingName) const
{
	return Instance.Bindings.FindByPredicate([BindingName](const FAPSQuestNamedEntityBinding& Binding)
	{
		return Binding.BindingName == BindingName;
	});
}

bool FAPSQuestRuntime::MatchesEntity(const FAPSQuestInstanceSaveData& Instance,
	EAPSQuestBindingMatch Match, const FAPSQuestEntityRef& Exact, FName Binding,
	const FAPSQuestEntityRef& Actual, FString& OutReason) const
{
	switch (Match)
	{
	case EAPSQuestBindingMatch::Any:
		return true;
	case EAPSQuestBindingMatch::Exact:
		return Exact.Matches(Actual);
	case EAPSQuestBindingMatch::NamedBinding:
		if (const FAPSQuestNamedEntityBinding* Resolved = FindBinding(Instance, Binding))
		{
			return Resolved->Entity.Matches(Actual);
		}
		OutReason = FString::Printf(TEXT("Required binding %s is unresolved"), *Binding.ToString());
		return false;
	default:
		return false;
	}
}

bool FAPSQuestRuntime::MatchesPredicate(const FAPSQuestInstanceSaveData& Instance,
	const FAPSQuestEventPredicate& Predicate, const FAPSQuestEvent& Event,
	bool bRequireResult, FString& OutReason) const
{
	OutReason.Reset();
	if (Event.Verb != Predicate.Verb || Event.Quantity < Predicate.MinimumQuantity
		|| (bRequireResult && Event.Result != Predicate.RequiredResult)
		|| (Predicate.RequiredDefinitionId.IsValid()
			&& Event.DefinitionId != Predicate.RequiredDefinitionId)
		|| Event.DefinitionSchemaVersion < Predicate.MinimumDefinitionSchemaVersion)
	{
		return false;
	}
	return MatchesEntity(Instance, Predicate.SubjectMatch, Predicate.ExactSubject,
			Predicate.SubjectBinding, Event.Subject, OutReason)
		&& MatchesEntity(Instance, Predicate.TargetMatch, Predicate.ExactTarget,
			Predicate.TargetBinding, Event.Target, OutReason);
}

void FAPSQuestRuntime::PublishPrompt(const FAPSQuestInstanceSaveData& Instance,
	const FAPSQuestObjectiveNodeDefinition& Node)
{
	if (Node.Prompt.PromptId.IsNone())
	{
		return;
	}
	SetCurrentPrompt(BuildPromptSnapshot(Instance, Node), true);
}

FAPSQuestPromptSnapshot FAPSQuestRuntime::BuildPromptSnapshot(
	const FAPSQuestInstanceSaveData& Instance,
	const FAPSQuestObjectiveNodeDefinition& Node) const
{
	FAPSQuestPromptSnapshot Snapshot;
	Snapshot.PromptStableId = MakePromptStableId(
		Instance, Node.NodeId, Node.Prompt.PromptId);
	Snapshot.ContextStableId = Instance.InstanceId;
	Snapshot.State = EAPSQuestPromptState::Active;
	Snapshot.PromptId = Node.Prompt.PromptId;
	Snapshot.QuestId = Instance.QuestId;
	Snapshot.NodeId = Node.NodeId;
	Snapshot.Title = Node.Prompt.Title;
	Snapshot.Body = Node.Prompt.Body;
	Snapshot.Actions = Node.Prompt.Actions;
	Snapshot.Priority = Node.Prompt.Priority;
	Snapshot.bModal = Node.Prompt.bModal;
	Snapshot.bDismissible = Node.Prompt.bDismissible;
	return Snapshot;
}

void FAPSQuestRuntime::SetCurrentPrompt(FAPSQuestPromptSnapshot Snapshot,
	const bool bBroadcast)
{
	if (!PromptSessionEpoch.IsValid())
	{
		PromptSessionEpoch = FGuid::NewGuid();
		PromptRevision = 0;
	}
	Snapshot.SessionEpoch = PromptSessionEpoch;
	Snapshot.Revision = ++PromptRevision;
	CurrentPrompt = MoveTemp(Snapshot);
	if (bBroadcast)
	{
		PromptPublished.Broadcast(CurrentPrompt.GetValue());
	}
}

void FAPSQuestRuntime::SetPromptCleared(const bool bBroadcast)
{
	FAPSQuestPromptSnapshot Tombstone;
	Tombstone.State = EAPSQuestPromptState::Cleared;
	if (CurrentPrompt.IsSet())
	{
		const FAPSQuestPromptSnapshot& Existing = CurrentPrompt.GetValue();
		Tombstone.PromptStableId = Existing.PromptStableId;
		Tombstone.ContextStableId = Existing.ContextStableId;
		Tombstone.PromptId = Existing.PromptId;
		Tombstone.QuestId = Existing.QuestId;
		Tombstone.NodeId = Existing.NodeId;
	}
	SetCurrentPrompt(MoveTemp(Tombstone), bBroadcast);
}

void FAPSQuestRuntime::RefreshCurrentPromptFromState(const bool bBroadcast)
{
	const FAPSQuestInstanceSaveData* BestInstance = nullptr;
	const FAPSQuestObjectiveNodeDefinition* BestNode = nullptr;
	for (const TPair<FName, FAPSQuestInstanceSaveData>& Pair : Instances)
	{
		const FAPSQuestInstanceSaveData& Instance = Pair.Value;
		if (Instance.State != EAPSQuestInstanceState::Running)
		{
			continue;
		}
		const UAPSQuestDefinition* Definition = FindDefinition(Instance.QuestId);
		if (!Definition || Definition->DefinitionVersion != Instance.DefinitionVersion)
		{
			continue;
		}
		for (const FAPSQuestNodeRuntimeState& RuntimeNode : Instance.Nodes)
		{
			if (RuntimeNode.State != EAPSQuestNodeState::Active)
			{
				continue;
			}
			const FAPSQuestObjectiveNodeDefinition* Candidate =
				Definition->FindNode(RuntimeNode.NodeId);
			if (!Candidate || Candidate->Prompt.PromptId.IsNone())
			{
				continue;
			}
			const bool bPreferred = !BestNode
				|| Candidate->Prompt.Priority > BestNode->Prompt.Priority
				|| (Candidate->Prompt.Priority == BestNode->Prompt.Priority
					&& (Instance.QuestId.LexicalLess(BestInstance->QuestId)
						|| (Instance.QuestId == BestInstance->QuestId
							&& Candidate->NodeId.LexicalLess(BestNode->NodeId))));
			if (bPreferred)
			{
				BestInstance = &Instance;
				BestNode = Candidate;
			}
		}
	}
	if (BestInstance && BestNode)
	{
		SetCurrentPrompt(BuildPromptSnapshot(*BestInstance, *BestNode), bBroadcast);
	}
	else
	{
		SetPromptCleared(bBroadcast);
	}
}

void FAPSQuestRuntime::RequestRewards(FAPSQuestInstanceSaveData& Instance,
	const FAPSQuestObjectiveNodeDefinition& Node)
{
	for (const FAPSQuestRewardDefinition& Reward : Node.Rewards)
	{
		const FGuid TransactionId = MakeRewardTransactionId(Instance, Node.NodeId, Reward.RewardId);
		if (Instance.RewardLedger.ContainsByPredicate(
			[TransactionId](const FAPSQuestRewardLedgerEntry& Entry)
			{
				return Entry.TransactionId == TransactionId;
			}))
		{
			continue;
		}

		FAPSQuestRewardLedgerEntry& Entry = Instance.RewardLedger.AddDefaulted_GetRef();
		Entry.TransactionId = TransactionId;
		Entry.NodeId = Node.NodeId;
		Entry.RewardId = Reward.RewardId;
		Entry.State = EAPSQuestRewardState::Requested;

		FAPSQuestRewardCommand Command;
		Command.TransactionId = TransactionId;
		Command.QuestId = Instance.QuestId;
		Command.NodeId = Node.NodeId;
		Command.RewardId = Reward.RewardId;
		Command.RewardType = Reward.RewardType;
		Command.Quantity = Reward.Quantity;
		Command.Parameters = Reward.Parameters;
		if (!Reward.TargetBinding.IsNone())
		{
			if (const FAPSQuestNamedEntityBinding* Binding = FindBinding(Instance, Reward.TargetBinding))
			{
				Command.Target = Binding->Entity;
			}
		}
		RewardRequested.Broadcast(Command);
	}
}

void FAPSQuestRuntime::CompleteNode(FAPSQuestInstanceSaveData& Instance,
	const UAPSQuestDefinition& Definition, FAPSQuestNodeRuntimeState& NodeState,
	const bool bRequestRewards)
{
	if (NodeState.State == EAPSQuestNodeState::Completed)
	{
		return;
	}
	const FAPSQuestObjectiveNodeDefinition* Node = Definition.FindNode(NodeState.NodeId);
	if (!Node)
	{
		Instance.State = EAPSQuestInstanceState::Suspended;
		Instance.LastFailureCode = MissingDefinitionCode;
		return;
	}
	NodeState.State = EAPSQuestNodeState::Satisfied;
	if (bRequestRewards)
	{
		RequestRewards(Instance, *Node);
	}
	NodeState.State = EAPSQuestNodeState::Completed;

	TArray<FName> Successors;
	Definition.GetSuccessors(NodeState.NodeId, Successors);
	for (FName Successor : Successors)
	{
		if (FAPSQuestNodeRuntimeState* SuccessorState = FindMutableNode(Instance, Successor))
		{
			if (SuccessorState->State == EAPSQuestNodeState::Dormant)
			{
				SuccessorState->State = EAPSQuestNodeState::Active;
				if (const FAPSQuestObjectiveNodeDefinition* SuccessorDefinition =
					Definition.FindNode(Successor))
				{
					PublishPrompt(Instance, *SuccessorDefinition);
				}
			}
		}
	}
}

void FAPSQuestRuntime::RefreshInstanceCompletion(FAPSQuestInstanceSaveData& Instance,
	const UAPSQuestDefinition& Definition)
{
	if (Instance.State != EAPSQuestInstanceState::Running)
	{
		return;
	}
	bool bHasActiveNode = false;
	bool bAllRequiredComplete = true;
	for (const FAPSQuestObjectiveNodeDefinition& DefinitionNode : Definition.Nodes)
	{
		const FAPSQuestNodeRuntimeState* RuntimeNode = Instance.Nodes.FindByPredicate(
			[&DefinitionNode](const FAPSQuestNodeRuntimeState& Node)
			{
				return Node.NodeId == DefinitionNode.NodeId;
			});
		if (!RuntimeNode)
		{
			bAllRequiredComplete = false;
			continue;
		}
		bHasActiveNode |= RuntimeNode->State == EAPSQuestNodeState::Active;
		if (DefinitionNode.bRequired && RuntimeNode->State != EAPSQuestNodeState::Completed)
		{
			bAllRequiredComplete = false;
		}
	}
	if (!bHasActiveNode && bAllRequiredComplete)
	{
		Instance.State = EAPSQuestInstanceState::Completed;
		Instance.LastFailureCode = NAME_None;
		RefreshCurrentPromptFromState(true);
	}
}

FGuid FAPSQuestRuntime::MakeRewardTransactionId(const FAPSQuestInstanceSaveData& Instance,
	FName NodeId, FName RewardId)
{
	const FString Canonical = FString::Printf(TEXT("APS.Quest.Reward|%s|%s|%s|%s"),
		*Instance.QuestId.ToString(),
		*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
		*NodeId.ToString(), *RewardId.ToString());
	FTCHARToUTF8 Utf8(*Canonical);
	FMD5 Md5;
	Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	uint8 Digest[16];
	Md5.Final(Digest);
	auto ReadWord = [&Digest](int32 Offset)
	{
		return static_cast<uint32>(Digest[Offset]) << 24
			| static_cast<uint32>(Digest[Offset + 1]) << 16
			| static_cast<uint32>(Digest[Offset + 2]) << 8
			| static_cast<uint32>(Digest[Offset + 3]);
	};
	return FGuid(ReadWord(0), ReadWord(4), ReadWord(8), ReadWord(12));
}

FGuid FAPSQuestRuntime::MakePromptStableId(const FAPSQuestInstanceSaveData& Instance,
	FName NodeId, FName PromptId)
{
	const FString Canonical = FString::Printf(TEXT("APS.Quest.Prompt|%s|%s|%s|%s"),
		*Instance.QuestId.ToString(),
		*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
		*NodeId.ToString(), *PromptId.ToString());
	FTCHARToUTF8 Utf8(*Canonical);
	FMD5 Md5;
	Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	uint8 Digest[16];
	Md5.Final(Digest);
	auto ReadWord = [&Digest](const int32 Offset)
	{
		return static_cast<uint32>(Digest[Offset]) << 24
			| static_cast<uint32>(Digest[Offset + 1]) << 16
			| static_cast<uint32>(Digest[Offset + 2]) << 8
			| static_cast<uint32>(Digest[Offset + 3]);
	};
	return FGuid(ReadWord(0), ReadWord(4), ReadWord(8), ReadWord(12));
}

void FAPSQuestRuntime::AppendBoundedGuid(TArray<FGuid>& Values, const FGuid& Value)
{
	if (!Value.IsValid() || Values.Contains(Value))
	{
		return;
	}
	Values.Add(Value);
	if (Values.Num() > MaximumDedupeEntries)
	{
		Values.RemoveAt(0, Values.Num() - MaximumDedupeEntries, EAllowShrinking::No);
	}
}
