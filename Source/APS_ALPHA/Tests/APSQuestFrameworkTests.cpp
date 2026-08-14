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
	TestTrue(TEXT("Definition registers"), Runtime.RegisterDefinition(Definition, Reason));
	TestTrue(TEXT("Quest starts"), Runtime.StartQuest(QuestId, Guid(1000), Reason));
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
	TestFalse(TEXT("Unknown out-of-order event is rejected"), Runtime.SubmitEvent(OutOfOrder, Reason));
	TestTrue(TEXT("Ordering diagnostic is explicit"), Reason.Contains(TEXT("Out-of-order")));
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

	FAPSQuestSaveData Unsupported = SaveData;
	Unsupported.SchemaVersion = FAPSQuestSaveData::LatestSchemaVersion + 1;
	TestFalse(TEXT("Newer schema is rejected safely"), Restored.RestoreSaveData(
		Unsupported, Reason));
	TestNotNull(TEXT("Existing runtime remains after rejection"), Restored.FindInstance(QuestId));

#if !UE_BUILD_SHIPPING
	TestTrue(TEXT("Debug skip is available only in non-shipping test build"),
		Restored.RecoverQuest(QuestId, BaseNodeId, EAPSQuestRecoveryPolicy::DebugSkip, Reason));
#endif
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
