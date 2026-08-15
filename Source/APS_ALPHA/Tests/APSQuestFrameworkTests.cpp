#include "APS_ALPHA/Gameplay/Quests/APSQuestDefinition.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestRuntime.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace APSQuestTests
{
	const FName QuestId(TEXT("APS.Test.Onboarding"));
	const FName BaseNodeId(TEXT("InspectBase"));
	const FName ShipNodeId(TEXT("TakeShip"));
	const FName BaseBinding(TEXT("Base"));
	const FName ShipBinding(TEXT("SelectedShip"));

	FGuid Guid(uint32 Seed)
	{
		return FGuid(Seed, Seed + 1, Seed + 2, Seed + 3);
	}

	FAPSQuestEntityRef CivilizationEntity(uint32 Seed)
	{
		FAPSQuestEntityRef Entity;
		Entity.Kind = EAPSQuestEntityKind::CivilizationEntity;
		Entity.Guid = Guid(Seed);
		return Entity;
	}

	UAPSQuestDefinition* MakeTwoStepDefinition()
	{
		UAPSQuestDefinition* Definition = NewObject<UAPSQuestDefinition>(GetTransientPackage());
		Definition->QuestId = QuestId;
		Definition->DefinitionVersion = 1;
		Definition->EntryNodeId = BaseNodeId;

		FAPSQuestBindingDefinition& BaseSchema =
			Definition->BindingDefinitions.AddDefaulted_GetRef();
		BaseSchema.BindingName = BaseBinding;
		BaseSchema.ExpectedKind = EAPSQuestEntityKind::CivilizationEntity;
		FAPSQuestBindingDefinition& ShipSchema =
			Definition->BindingDefinitions.AddDefaulted_GetRef();
		ShipSchema.BindingName = ShipBinding;
		ShipSchema.ExpectedKind = EAPSQuestEntityKind::CivilizationEntity;

		FAPSQuestObjectiveNodeDefinition& Base = Definition->Nodes.AddDefaulted_GetRef();
		Base.NodeId = BaseNodeId;
		Base.Trigger.Verb = TEXT("APS.Interaction.Inspect");
		Base.Trigger.TargetMatch = EAPSQuestBindingMatch::NamedBinding;
		Base.Trigger.TargetBinding = BaseBinding;
		Base.Prompt.PromptId = TEXT("APS.Prompt.InspectBase");

		FAPSQuestObjectiveNodeDefinition& Ship = Definition->Nodes.AddDefaulted_GetRef();
		Ship.NodeId = ShipNodeId;
		Ship.Trigger.Verb = TEXT("APS.Interaction.TakeControl");
		Ship.Trigger.TargetMatch = EAPSQuestBindingMatch::NamedBinding;
		Ship.Trigger.TargetBinding = ShipBinding;
		Ship.Prompt.PromptId = TEXT("APS.Prompt.TakeShip");
		FAPSQuestRewardDefinition& Reward = Ship.Rewards.AddDefaulted_GetRef();
		Reward.RewardId = TEXT("UnlockNavigation");
		Reward.RewardType = TEXT("APS.Progression.Unlock");
		Reward.TargetBinding = ShipBinding;

		FAPSQuestEdgeDefinition& Edge = Definition->SuccessEdges.AddDefaulted_GetRef();
		Edge.FromNodeId = BaseNodeId;
		Edge.ToNodeId = ShipNodeId;
		return Definition;
	}

	FAPSQuestEvent MakeEvent(FGuid StreamId, int64 Sequence, FName Verb,
		const FAPSQuestEntityRef& Target, uint32 IdentitySeed,
		EAPSQuestEventResult Result = EAPSQuestEventResult::Succeeded)
	{
		FAPSQuestEvent Event;
		Event.StreamId = StreamId;
		Event.Sequence = Sequence;
		Event.EventId = Guid(IdentitySeed);
		Event.CorrelationId = Guid(IdentitySeed + 100);
		Event.Verb = Verb;
		Event.Result = Result;
		Event.Target = Target;
		if (Result == EAPSQuestEventResult::Failed)
		{
			Event.FailureCode = TEXT("APS.Interaction.OutOfRange");
		}
		return Event;
	}

	const FAPSQuestNodeRuntimeState* FindNode(const FAPSQuestInstanceSaveData* Instance,
		FName NodeId)
	{
		return Instance ? Instance->Nodes.FindByPredicate(
			[NodeId](const FAPSQuestNodeRuntimeState& Node)
			{
				return Node.NodeId == NodeId;
			}) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestDefinitionValidationTest,
	"APS.Quest.Definition.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestDefinitionValidationTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	UAPSQuestDefinition* Definition = MakeTwoStepDefinition();
	TArray<FString> Errors;
	TestTrue(TEXT("Valid two-step graph passes"), Definition->ValidateDefinition(Errors));
	TestEqual(TEXT("Valid graph has no diagnostics"), Errors.Num(), 0);

	FAPSQuestEdgeDefinition& Cycle = Definition->SuccessEdges.AddDefaulted_GetRef();
	Cycle.FromNodeId = ShipNodeId;
	Cycle.ToNodeId = BaseNodeId;
	TestFalse(TEXT("Cycle is rejected"), Definition->ValidateDefinition(Errors));
	TestTrue(TEXT("Cycle diagnostic is present"), Errors.ContainsByPredicate(
		[](const FString& Error) { return Error.Contains(TEXT("cycle")); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestDeterministicFlowTest,
	"APS.Quest.Runtime.DeterministicFlowAndExactlyOnceReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestDeterministicFlowTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	FAPSQuestRuntime Runtime;
	UAPSQuestDefinition* Definition = MakeTwoStepDefinition();
	FString Reason;
	const FGuid PromptEpoch = Guid(900);
	Runtime.BeginPromptSession(PromptEpoch);
	FAPSQuestPromptSnapshot CurrentPrompt;
	TestTrue(TEXT("Presentation session exposes an initial clear tombstone"),
		Runtime.TryGetCurrentPromptSnapshot(CurrentPrompt));
	TestEqual(TEXT("Initial prompt state is cleared"), CurrentPrompt.State,
		EAPSQuestPromptState::Cleared);
	TestEqual(TEXT("Prompt epoch is explicit"), CurrentPrompt.SessionEpoch, PromptEpoch);
	TestTrue(TEXT("Definition registers"), Runtime.RegisterDefinition(Definition, Reason));
	TestTrue(TEXT("Quest starts"), Runtime.StartQuest(QuestId, Guid(1000), Reason));
	TestTrue(TEXT("Current prompt is queryable after bootstrap"),
		Runtime.TryGetCurrentPromptSnapshot(CurrentPrompt));
	TestEqual(TEXT("Bootstrap prompt is active"), CurrentPrompt.State,
		EAPSQuestPromptState::Active);
	TestEqual(TEXT("Bootstrap prompt has semantic id"), CurrentPrompt.PromptId,
		FName(TEXT("APS.Prompt.InspectBase")));
	TestEqual(TEXT("Prompt context is the quest instance"), CurrentPrompt.ContextStableId,
		Guid(1000));
	TestTrue(TEXT("Prompt stable id is deterministic and valid"),
		CurrentPrompt.PromptStableId.IsValid());
	const int64 BootstrapRevision = CurrentPrompt.Revision;
	TestTrue(TEXT("Matching quest bootstrap is idempotent"),
		Runtime.StartQuest(QuestId, Guid(1000), Reason));
	Runtime.TryGetCurrentPromptSnapshot(CurrentPrompt);
	TestEqual(TEXT("Idempotent bootstrap does not republish prompt"),
		CurrentPrompt.Revision, BootstrapRevision);
	TestFalse(TEXT("Quest identity cannot silently retarget"),
		Runtime.StartQuest(QuestId, Guid(1001), Reason));
	TestTrue(TEXT("Identity conflict diagnostic is explicit"),
		Reason.Contains(TEXT("different identity")));
	const FAPSQuestEntityRef Base = CivilizationEntity(10);
	const FAPSQuestEntityRef Ship = CivilizationEntity(20);
	TestTrue(TEXT("Base binds"), Runtime.BindEntity(QuestId, BaseBinding, Base, Reason));
	TestTrue(TEXT("Ship binds"), Runtime.BindEntity(QuestId, ShipBinding, Ship, Reason));

	int32 RewardRequestCount = 0;
	FAPSQuestRewardCommand LastReward;
	Runtime.OnRewardRequested().AddLambda(
		[&RewardRequestCount, &LastReward](const FAPSQuestRewardCommand& Command)
		{
			++RewardRequestCount;
			LastReward = Command;
		});

	const FGuid StreamId = Guid(2000);
	const FAPSQuestEvent Inspect = MakeEvent(StreamId, 1,
		TEXT("APS.Interaction.Inspect"), Base, 3000);
	TestTrue(TEXT("Inspect event accepted"), Runtime.SubmitEvent(Inspect, Reason));
	const FAPSQuestInstanceSaveData* Instance = Runtime.FindInstance(QuestId);
	TestEqual(TEXT("Base completes"), FindNode(Instance, BaseNodeId)->State,
		EAPSQuestNodeState::Completed);
	TestEqual(TEXT("Ship becomes active"), FindNode(Instance, ShipNodeId)->State,
		EAPSQuestNodeState::Active);
	Runtime.TryGetCurrentPromptSnapshot(CurrentPrompt);
	TestEqual(TEXT("Successor prompt is current"), CurrentPrompt.PromptId,
		FName(TEXT("APS.Prompt.TakeShip")));
	TestTrue(TEXT("Prompt revision advances monotonically"),
		CurrentPrompt.Revision > BootstrapRevision);
	const int64 SuccessorRevision = CurrentPrompt.Revision;

	TestTrue(TEXT("Exact duplicate is idempotently accepted"), Runtime.SubmitEvent(Inspect, Reason));
	TestEqual(TEXT("Duplicate did not request reward"), RewardRequestCount, 0);

	const FAPSQuestEvent TakeShip = MakeEvent(StreamId, 2,
		TEXT("APS.Interaction.TakeControl"), Ship, 4000);
	TestTrue(TEXT("Take-control accepted"), Runtime.SubmitEvent(TakeShip, Reason));
	Instance = Runtime.FindInstance(QuestId);
	TestEqual(TEXT("Quest completes"), Instance->State, EAPSQuestInstanceState::Completed);
	TestEqual(TEXT("Reward requested exactly once"), RewardRequestCount, 1);
	TestTrue(TEXT("Reward transaction is stable"), LastReward.TransactionId.IsValid());
	TestTrue(TEXT("Reward target resolves selected ship"), LastReward.Target.Matches(Ship));
	Runtime.TryGetCurrentPromptSnapshot(CurrentPrompt);
	TestEqual(TEXT("Completion publishes a clear tombstone"), CurrentPrompt.State,
		EAPSQuestPromptState::Cleared);
	TestTrue(TEXT("Clear tombstone has a newer revision"),
		CurrentPrompt.Revision > SuccessorRevision);

	TestTrue(TEXT("Terminal duplicate stays idempotent"), Runtime.SubmitEvent(TakeShip, Reason));
	TestEqual(TEXT("Reward remains exactly once"), RewardRequestCount, 1);
	TestTrue(TEXT("Reward applies"), Runtime.AcknowledgeReward(
		QuestId, LastReward.TransactionId, true, NAME_None, Reason));
	TestTrue(TEXT("Repeated success acknowledgement is idempotent"), Runtime.AcknowledgeReward(
		QuestId, LastReward.TransactionId, true, NAME_None, Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestIdentityAndOrderingTest,
	"APS.Quest.Runtime.IdentityFailureAndOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestIdentityAndOrderingTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	FAPSQuestRuntime Runtime;
	UAPSQuestDefinition* Definition = MakeTwoStepDefinition();
	FString Reason;
	Runtime.RegisterDefinition(Definition, Reason);
	Runtime.StartQuest(QuestId, Guid(5000), Reason);
	const FAPSQuestEntityRef Base = CivilizationEntity(30);
	const FAPSQuestEntityRef OtherBase = CivilizationEntity(31);
	FAPSQuestEntityRef WrongDomain;
	WrongDomain.Kind = EAPSQuestEntityKind::CelestialBody;
	WrongDomain.StableKey = TEXT("SYS0/S0/P0");
	TestFalse(TEXT("Binding rejects a different canonical identity domain"),
		Runtime.BindEntity(QuestId, BaseBinding, WrongDomain, Reason));
	TestTrue(TEXT("Wrong-domain diagnostic is explicit"), Reason.Contains(TEXT("identity kind")));
	TestTrue(TEXT("Initial binding succeeds"), Runtime.BindEntity(
		QuestId, BaseBinding, Base, Reason));
	TestFalse(TEXT("Binding cannot silently retarget"), Runtime.BindEntity(
		QuestId, BaseBinding, OtherBase, Reason));
	TestTrue(TEXT("Retarget diagnostic is explicit"), Reason.Contains(TEXT("retarget")));

	const FGuid StreamId = Guid(6000);
	const FAPSQuestEvent Failed = MakeEvent(StreamId, 2,
		TEXT("APS.Interaction.Inspect"), Base, 6100, EAPSQuestEventResult::Failed);
	TestTrue(TEXT("Failed committed action is consumed"), Runtime.SubmitEvent(Failed, Reason));
	const FAPSQuestNodeRuntimeState* BaseNode = FindNode(Runtime.FindInstance(QuestId), BaseNodeId);
	TestEqual(TEXT("Failed action keeps objective active"), BaseNode->State,
		EAPSQuestNodeState::Active);
	TestEqual(TEXT("Stable failure code is recorded"), BaseNode->LastFailureCode,
		FName(TEXT("APS.Interaction.OutOfRange")));

	const FAPSQuestEvent OutOfOrder = MakeEvent(StreamId, 1,
		TEXT("APS.Interaction.Inspect"), Base, 6200);
	TestFalse(TEXT("Relevant out-of-order event is rejected"),
		Runtime.SubmitEvent(OutOfOrder, Reason));
	TestTrue(TEXT("Ordering diagnostic is explicit"), Reason.Contains(TEXT("Out-of-order")));
	TestFalse(TEXT("Rejected event does not enter dedupe"),
		Runtime.FindInstance(QuestId)->ConsumedEventIds.Contains(OutOfOrder.EventId));

	const FAPSQuestEvent IndependentStream = MakeEvent(Guid(6001), 1,
		TEXT("APS.Interaction.Inspect"), OtherBase, 6150);
	TestFalse(TEXT("Unmatched owner event remains a transient no-op"),
		Runtime.SubmitEvent(IndependentStream, Reason));
	TestEqual(TEXT("Unmatched event does not create a stream cursor"),
		Runtime.FindInstance(QuestId)->EventStreams.Num(), 1);
	TestFalse(TEXT("Unmatched event does not enter dedupe"),
		Runtime.FindInstance(QuestId)->ConsumedEventIds.Contains(IndependentStream.EventId));

	const FAPSQuestEvent MatchingIndependentStream = MakeEvent(Guid(6001), 1,
		TEXT("APS.Interaction.Inspect"), Base, 6151);
	TestTrue(TEXT("Later relevant fact can consume the untouched stream sequence"),
		Runtime.SubmitEvent(MatchingIndependentStream, Reason));
	TestEqual(TEXT("Relevant independent stream retains its own cursor"),
		Runtime.FindInstance(QuestId)->EventStreams.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestMultiInstanceFanoutTest,
	"APS.Quest.Runtime.MultiInstanceFanoutAndRelevance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestMultiInstanceFanoutTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	const FName SecondaryQuestId(TEXT("APS.Test.Onboarding.Secondary"));
	FAPSQuestRuntime Runtime;
	UAPSQuestDefinition* Primary = MakeTwoStepDefinition();
	FString Reason;
	TestTrue(TEXT("Primary definition registers"), Runtime.RegisterDefinition(Primary, Reason));
	TestTrue(TEXT("Primary quest starts"), Runtime.StartQuest(QuestId, Guid(8000), Reason));
	const FAPSQuestEntityRef Base = CivilizationEntity(50);
	TestTrue(TEXT("Primary target binds"), Runtime.BindEntity(
		QuestId, BaseBinding, Base, Reason));

	const FGuid SharedStreamId = Guid(8100);
	const FAPSQuestEvent PrimaryAdvance = MakeEvent(SharedStreamId, 2,
		TEXT("APS.Interaction.Inspect"), Base, 8200, EAPSQuestEventResult::Failed);
	TestTrue(TEXT("Primary quest advances its owner cursor"),
		Runtime.SubmitEvent(PrimaryAdvance, Reason));

	UAPSQuestDefinition* Secondary = MakeTwoStepDefinition();
	Secondary->QuestId = SecondaryQuestId;
	TestTrue(TEXT("Secondary definition registers"), Runtime.RegisterDefinition(Secondary, Reason));
	TestTrue(TEXT("Secondary quest starts"), Runtime.StartQuest(
		SecondaryQuestId, Guid(8300), Reason));
	TestTrue(TEXT("Secondary target binds"), Runtime.BindEntity(
		SecondaryQuestId, BaseBinding, Base, Reason));

	const FAPSQuestEvent FanoutEvent = MakeEvent(SharedStreamId, 1,
		TEXT("APS.Interaction.Inspect"), Base, 8400);
	TestTrue(TEXT("Out-of-order rejection in one quest does not block another"),
		Runtime.SubmitEvent(FanoutEvent, Reason));
	TestTrue(TEXT("Accepted fanout has no global ordering error"), Reason.IsEmpty());
	TestEqual(TEXT("Primary objective remains active"),
		FindNode(Runtime.FindInstance(QuestId), BaseNodeId)->State,
		EAPSQuestNodeState::Active);
	TestEqual(TEXT("Secondary objective consumes the same owner fact"),
		FindNode(Runtime.FindInstance(SecondaryQuestId), BaseNodeId)->State,
		EAPSQuestNodeState::Completed);
	TestFalse(TEXT("Primary dedupe excludes its rejected event"),
		Runtime.FindInstance(QuestId)->ConsumedEventIds.Contains(FanoutEvent.EventId));
	TestTrue(TEXT("Secondary dedupe records its accepted event"),
		Runtime.FindInstance(SecondaryQuestId)->ConsumedEventIds.Contains(FanoutEvent.EventId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestAuthoritativeIngressTest,
	"APS.Quest.Runtime.AuthoritativeIngressGuardrails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestAuthoritativeIngressTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	FAPSQuestRuntime Runtime;
	UAPSQuestDefinition* Definition = MakeTwoStepDefinition();
	FString Reason;
	TestTrue(TEXT("Definition registers"), Runtime.RegisterDefinition(Definition, Reason));
	TestTrue(TEXT("Quest starts"), Runtime.StartQuest(QuestId, Guid(8500), Reason));
	const FAPSQuestEntityRef Base = CivilizationEntity(60);
	const FAPSQuestEntityRef Ship = CivilizationEntity(61);
	TestTrue(TEXT("Base binds"), Runtime.BindEntity(QuestId, BaseBinding, Base, Reason));
	TestTrue(TEXT("Ship binds"), Runtime.BindEntity(QuestId, ShipBinding, Ship, Reason));

	int32 RewardRequestCount = 0;
	Runtime.OnRewardRequested().AddLambda(
		[&RewardRequestCount](const FAPSQuestRewardCommand&)
		{
			++RewardRequestCount;
		});

	const FAPSQuestEvent UnsequencedInspect = MakeEvent(FGuid(), 0,
		TEXT("APS.Interaction.Inspect"), Base, 8600);
	TestFalse(TEXT("Production ingress rejects an unsequenced event"),
		Runtime.SubmitEvent(UnsequencedInspect, Reason));
	TestTrue(TEXT("Authoritative ingress diagnostic is explicit"),
		Reason.Contains(TEXT("Authoritative")));
	const FAPSQuestInstanceSaveData* Instance = Runtime.FindInstance(QuestId);
	TestEqual(TEXT("Rejected event leaves objective active"),
		FindNode(Instance, BaseNodeId)->State, EAPSQuestNodeState::Active);
	TestFalse(TEXT("Rejected event does not enter dedupe"),
		Instance->ConsumedEventIds.Contains(UnsequencedInspect.EventId));
	TestEqual(TEXT("Rejected event creates no rewards"), Instance->RewardLedger.Num(), 0);

#if !UE_BUILD_SHIPPING
	const FAPSQuestEvent SequencedInspect = MakeEvent(Guid(8650), 1,
		TEXT("APS.Interaction.Inspect"), Base, 8651);
	TestFalse(TEXT("Debug ingress rejects an authoritative owner event"),
		Runtime.DebugInjectEvent(SequencedInspect, Reason));
	TestTrue(TEXT("Debug ingress diagnostic requires its zero-sequence domain"),
		Reason.Contains(TEXT("zero Sequence")));
	TestTrue(TEXT("Non-shipping debug ingress accepts only the zero-sequence event"),
		Runtime.DebugInjectEvent(UnsequencedInspect, Reason));
	Instance = Runtime.FindInstance(QuestId);
	TestEqual(TEXT("Debug ingress completes the first objective"),
		FindNode(Instance, BaseNodeId)->State, EAPSQuestNodeState::Completed);
	TestEqual(TEXT("Debug ingress activates the successor"),
		FindNode(Instance, ShipNodeId)->State, EAPSQuestNodeState::Active);

	const FAPSQuestEvent UnsequencedRewardNode = MakeEvent(FGuid(), 0,
		TEXT("APS.Interaction.TakeControl"), Ship, 8700);
	TestTrue(TEXT("Debug ingress can traverse the reward-bearing objective"),
		Runtime.DebugInjectEvent(UnsequencedRewardNode, Reason));
	Instance = Runtime.FindInstance(QuestId);
	TestEqual(TEXT("Debug ingress still completes the quest"), Instance->State,
		EAPSQuestInstanceState::Completed);
	TestEqual(TEXT("Debug ingress never publishes reward commands"), RewardRequestCount, 0);
	TestEqual(TEXT("Debug ingress never mutates the reward ledger"),
		Instance->RewardLedger.Num(), 0);
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestPersistenceRecoveryTest,
	"APS.Quest.Runtime.PersistenceAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestPersistenceRecoveryTest::RunTest(const FString& Parameters)
{
	using namespace APSQuestTests;
	FAPSQuestRuntime Source;
	UAPSQuestDefinition* Definition = MakeTwoStepDefinition();
	FString Reason;
	Source.RegisterDefinition(Definition, Reason);
	Source.StartQuest(QuestId, Guid(7000), Reason);
	const FAPSQuestEntityRef Base = CivilizationEntity(40);
	Source.BindEntity(QuestId, BaseBinding, Base, Reason);
	TestTrue(TEXT("Quest suspends with stable reason"), Source.SuspendQuest(
		QuestId, TEXT("Quest.Adapter.Unavailable"), Reason));

	const FAPSQuestSaveData SaveData = Source.ExportSaveData();
	FAPSQuestRuntime Restored;
	Restored.RegisterDefinition(Definition, Reason);
	TestTrue(TEXT("Supported save restores"), Restored.RestoreSaveData(SaveData, Reason));
	const FAPSQuestInstanceSaveData* Instance = Restored.FindInstance(QuestId);
	TestNotNull(TEXT("Restored instance exists"), Instance);
	TestEqual(TEXT("Suspension survives save"), Instance->State,
		EAPSQuestInstanceState::Suspended);
	TestTrue(TEXT("Same node resumes"), Restored.RecoverQuest(
		QuestId, BaseNodeId, EAPSQuestRecoveryPolicy::ResumeSameIdentity, Reason));
	Instance = Restored.FindInstance(QuestId);
	TestEqual(TEXT("Recovered quest is running"), Instance->State,
		EAPSQuestInstanceState::Running);
	TestEqual(TEXT("Bound identity survives save"), Instance->Bindings[0].Entity.Guid, Base.Guid);

	FAPSQuestSaveData Legacy = SaveData;
	Legacy.SchemaVersion = 1;
	Legacy.Instances[0].EventStreams.Reset();
	Legacy.Instances[0].EventStreamId = Guid(7050);
	Legacy.Instances[0].LastConsumedSequence = 7;
	FAPSQuestRuntime Migrated;
	Migrated.RegisterDefinition(Definition, Reason);
	TestTrue(TEXT("V1 single-stream save migrates"), Migrated.RestoreSaveData(Legacy, Reason));
	const FAPSQuestInstanceSaveData* MigratedInstance = Migrated.FindInstance(QuestId);
	TestEqual(TEXT("Migrated save has one stream cursor"), MigratedInstance->EventStreams.Num(), 1);
	TestEqual(TEXT("Migrated stream sequence is preserved"),
		MigratedInstance->EventStreams[0].LastConsumedSequence, static_cast<int64>(7));

	FAPSQuestSaveData Unsupported = SaveData;
	Unsupported.SchemaVersion = FAPSQuestSaveData::LatestSchemaVersion + 1;
	TestFalse(TEXT("Newer schema is rejected safely"), Restored.RestoreSaveData(
		Unsupported, Reason));
	TestNotNull(TEXT("Existing runtime remains after rejection"), Restored.FindInstance(QuestId));

#if !UE_BUILD_SHIPPING
	int32 DebugRewardRequestCount = 0;
	Restored.OnRewardRequested().AddLambda(
		[&DebugRewardRequestCount](const FAPSQuestRewardCommand&)
		{
			++DebugRewardRequestCount;
		});
	TestTrue(TEXT("Debug skip is available only in non-shipping test build"),
		Restored.RecoverQuest(QuestId, BaseNodeId, EAPSQuestRecoveryPolicy::DebugSkip, Reason));
	TestTrue(TEXT("Debug skip can traverse a reward-bearing node"),
		Restored.RecoverQuest(QuestId, ShipNodeId, EAPSQuestRecoveryPolicy::DebugSkip, Reason));
	Instance = Restored.FindInstance(QuestId);
	TestEqual(TEXT("Debug skip never publishes production reward commands"),
		DebugRewardRequestCount, 0);
	TestEqual(TEXT("Debug skip never mutates the production reward ledger"),
		Instance->RewardLedger.Num(), 0);
	TestEqual(TEXT("Debug traversal still completes the quest"), Instance->State,
		EAPSQuestInstanceState::Completed);
#endif
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
