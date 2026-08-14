#include "APS_ALPHA/Gameplay/Quests/APSEarlyAccessOnboardingDefinition.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestRuntime.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace APSQuestOnboardingRouteTests
{
	FGuid StableGuid(uint32 Seed)
	{
		return FGuid(Seed, Seed + 1, Seed + 2, Seed + 3);
	}

	FAPSQuestEntityRef GuidEntity(EAPSQuestEntityKind Kind, uint32 Seed)
	{
		FAPSQuestEntityRef Entity;
		Entity.Kind = Kind;
		Entity.Guid = StableGuid(Seed);
		return Entity;
	}

	FAPSQuestEntityRef KeyEntity(EAPSQuestEntityKind Kind, const TCHAR* StableKey)
	{
		FAPSQuestEntityRef Entity;
		Entity.Kind = Kind;
		Entity.StableKey = StableKey;
		return Entity;
	}

	const FAPSQuestNodeRuntimeState* FindNode(
		const FAPSQuestInstanceSaveData* Instance, FName NodeId)
	{
		return Instance ? Instance->Nodes.FindByPredicate(
			[NodeId](const FAPSQuestNodeRuntimeState& Node)
			{
				return Node.NodeId == NodeId;
			}) : nullptr;
	}

	FAPSQuestEvent MakeEvent(const FAPSQuestObjectiveNodeDefinition& Node,
		const TMap<FName, FAPSQuestEntityRef>& Bindings, const FGuid& StreamId,
		int64 Sequence, uint32 IdentitySeed)
	{
		FAPSQuestEvent Event;
		Event.StreamId = StreamId;
		Event.Sequence = Sequence;
		Event.EventId = StableGuid(IdentitySeed);
		Event.CorrelationId = StableGuid(IdentitySeed + 1000);
		Event.Verb = Node.Trigger.Verb;
		Event.Result = EAPSQuestEventResult::Succeeded;
		Event.DefinitionId = Node.Trigger.RequiredDefinitionId;
		Event.DefinitionSchemaVersion = Node.Trigger.MinimumDefinitionSchemaVersion;
		if (Node.Trigger.SubjectMatch == EAPSQuestBindingMatch::NamedBinding)
		{
			Event.Subject = Bindings.FindChecked(Node.Trigger.SubjectBinding);
		}
		if (Node.Trigger.TargetMatch == EAPSQuestBindingMatch::NamedBinding)
		{
			Event.Target = Bindings.FindChecked(Node.Trigger.TargetBinding);
		}
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestFirstOnboardingRouteTest,
	"APS.Quest.Onboarding.FirstRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestFirstOnboardingRouteTest::RunTest(const FString& Parameters)
{
	using Contract = FAPSEarlyAccessOnboardingContract;
	using namespace APSQuestOnboardingRouteTests;

	UAPSEarlyAccessOnboardingDefinition* Definition =
		NewObject<UAPSEarlyAccessOnboardingDefinition>(GetTransientPackage());
	TestNotNull(TEXT("Native first-route definition exists"), Definition);
	TArray<FString> ValidationErrors;
	TestTrue(TEXT("Native first-route graph validates"),
		Definition->ValidateDefinition(ValidationErrors));
	TestEqual(TEXT("First route has fifteen committed objectives"), Definition->Nodes.Num(), 15);
	TestEqual(TEXT("Linear route has fourteen edges"), Definition->SuccessEdges.Num(), 14);
	TestEqual(TEXT("Route declares seven typed canonical bindings"),
		Definition->BindingDefinitions.Num(), 7);
	for (const FAPSQuestObjectiveNodeDefinition& Node : Definition->Nodes)
	{
		TestFalse(*FString::Printf(TEXT("%s has an explicit title"), *Node.NodeId.ToString()),
			Node.Prompt.Title.IsEmpty());
		TestFalse(*FString::Printf(TEXT("%s has explicit body text"), *Node.NodeId.ToString()),
			Node.Prompt.Body.IsEmpty());
		TestTrue(*FString::Printf(TEXT("%s exposes FocusObjective"), *Node.NodeId.ToString()),
			Node.Prompt.Actions.ContainsByPredicate(
				[](const FAPSQuestPromptAction& Action)
				{
					return Action.ActionId == TEXT("FocusObjective") && !Action.Label.IsEmpty();
				}));
	}

	FAPSQuestObjectiveNodeDefinition* BuildNode = Definition->Nodes.FindByPredicate(
		[](const FAPSQuestObjectiveNodeDefinition& Node)
		{
			return Node.NodeId == Contract::StructurePlacedNode;
		});
	TestNotNull(TEXT("Build objective exists"), BuildNode);
	const FPrimaryAssetId FoundationDefinition(
		FPrimaryAssetType(TEXT("APSBuildable")), FName(TEXT("Foundation")));
	BuildNode->Trigger.RequiredDefinitionId = FoundationDefinition;
	BuildNode->Trigger.MinimumDefinitionSchemaVersion = 2;
	TestTrue(TEXT("Typed build objective remains valid"),
		Definition->ValidateDefinition(ValidationErrors));

	TMap<FName, FAPSQuestEntityRef> Bindings;
	Bindings.Add(Contract::BaseBinding,
		GuidEntity(EAPSQuestEntityKind::CivilizationEntity, 10));
	Bindings.Add(Contract::LandingPadBinding,
		GuidEntity(EAPSQuestEntityKind::CivilizationEntity, 20));
	Bindings.Add(Contract::SelectedShipBinding,
		GuidEntity(EAPSQuestEntityKind::CivilizationEntity, 30));
	Bindings.Add(Contract::PlayerCharacterBinding,
		GuidEntity(EAPSQuestEntityKind::Player, 40));
	Bindings.Add(Contract::DestinationBodyBinding,
		KeyEntity(EAPSQuestEntityKind::CelestialBody, TEXT("SYS0/S0/P0")));
	Bindings.Add(Contract::LandingContextBinding,
		KeyEntity(EAPSQuestEntityKind::LandingContext, TEXT("SYS0/S0/P0/EA-LANDING-0")));
	Bindings.Add(Contract::HomeSystemBinding,
		GuidEntity(EAPSQuestEntityKind::StarSystem, 50));

	TUniquePtr<FAPSQuestRuntime> Runtime = MakeUnique<FAPSQuestRuntime>();
	FString Reason;
	TestTrue(TEXT("Route definition registers"), Runtime->RegisterDefinition(Definition, Reason));
	TestTrue(TEXT("Route starts"), Runtime->StartQuest(Contract::QuestId, StableGuid(100), Reason));
	for (const TPair<FName, FAPSQuestEntityRef>& Binding : Bindings)
	{
		TestTrue(*FString::Printf(TEXT("Binding %s is accepted"), *Binding.Key.ToString()),
			Runtime->BindEntity(Contract::QuestId, Binding.Key, Binding.Value, Reason));
	}
	const FGuid StreamId = StableGuid(200);
	TestTrue(TEXT("Event stream begins"),
		Runtime->BeginEventStream(Contract::QuestId, StreamId, Reason));

	int32 RewardRequestCount = 0;
	FAPSQuestRewardCommand LastReward;
	auto BindRewardListener = [&RewardRequestCount, &LastReward](FAPSQuestRuntime& Target)
	{
		Target.OnRewardRequested().AddLambda(
			[&RewardRequestCount, &LastReward](const FAPSQuestRewardCommand& Command)
			{
				++RewardRequestCount;
				LastReward = Command;
			});
	};
	BindRewardListener(*Runtime);

	auto RestoreCheckpoint = [this, &Runtime, Definition, &Reason, &BindRewardListener]()
	{
		const FAPSQuestSaveData SaveData = Runtime->ExportSaveData();
		TUniquePtr<FAPSQuestRuntime> Restored = MakeUnique<FAPSQuestRuntime>();
		TestTrue(TEXT("Checkpoint definition registers"),
			Restored->RegisterDefinition(Definition, Reason));
		TestTrue(TEXT("Checkpoint restores"), Restored->RestoreSaveData(SaveData, Reason));
		Runtime = MoveTemp(Restored);
		BindRewardListener(*Runtime);
	};

	int64 Sequence = 1;
	uint32 EventSeed = 1000;
	FAPSQuestEvent LastCommittedEvent;
	for (const FAPSQuestObjectiveNodeDefinition& Node : Definition->Nodes)
	{
		if (Node.NodeId == Contract::StructurePlacedNode)
		{
			FAPSQuestEvent WrongDefinition = MakeEvent(
				Node, Bindings, StreamId, Sequence++, EventSeed++);
			WrongDefinition.DefinitionId = FPrimaryAssetId(
				FPrimaryAssetType(TEXT("APSBuildable")), FName(TEXT("WrongType")));
			TestTrue(TEXT("Unrelated typed success is safely consumed"),
				Runtime->SubmitEvent(WrongDefinition, Reason));
			TestEqual(TEXT("Wrong definition does not advance build"),
				FindNode(Runtime->FindInstance(Contract::QuestId), Node.NodeId)->State,
				EAPSQuestNodeState::Active);

			FAPSQuestEvent FailedBuild = MakeEvent(
				Node, Bindings, StreamId, Sequence++, EventSeed++);
			FailedBuild.Result = EAPSQuestEventResult::Failed;
			FailedBuild.FailureCode = TEXT("APS.Build.PlacementBlocked");
			TestTrue(TEXT("Failed build fact is consumed"),
				Runtime->SubmitEvent(FailedBuild, Reason));
			const FAPSQuestNodeRuntimeState* FailedState =
				FindNode(Runtime->FindInstance(Contract::QuestId), Node.NodeId);
			TestEqual(TEXT("Failed build remains retryable"), FailedState->State,
				EAPSQuestNodeState::Active);
			TestEqual(TEXT("Failed build reason is persisted"), FailedState->LastFailureCode,
				FName(TEXT("APS.Build.PlacementBlocked")));
			RestoreCheckpoint(); // pre-build checkpoint after a representative recoverable failure
		}

		LastCommittedEvent = MakeEvent(Node, Bindings, StreamId, Sequence++, EventSeed++);
		TestTrue(*FString::Printf(TEXT("Objective %s accepts committed fact"),
			*Node.NodeId.ToString()), Runtime->SubmitEvent(LastCommittedEvent, Reason));
		TestEqual(*FString::Printf(TEXT("Objective %s completes"), *Node.NodeId.ToString()),
			FindNode(Runtime->FindInstance(Contract::QuestId), Node.NodeId)->State,
			EAPSQuestNodeState::Completed);

		if (Node.NodeId == Contract::TakeoffCommittedNode
			|| Node.NodeId == Contract::SurfaceEnteredNode)
		{
			RestoreCheckpoint(); // in-flight and landed checkpoints
		}
	}

	const FAPSQuestInstanceSaveData* Completed = Runtime->FindInstance(Contract::QuestId);
	TestEqual(TEXT("First onboarding route completes"), Completed->State,
		EAPSQuestInstanceState::Completed);
	TestEqual(TEXT("Completion reward is requested once"), RewardRequestCount, 1);
	TestEqual(TEXT("Completion reward targets the bound player"), LastReward.Target.Kind,
		EAPSQuestEntityKind::Player);
	TestTrue(TEXT("Completion reward target identity is exact"),
		LastReward.Target.Matches(Bindings.FindChecked(Contract::PlayerCharacterBinding)));

	RestoreCheckpoint(); // completed checkpoint
	TestEqual(TEXT("Completed state survives restore"),
		Runtime->FindInstance(Contract::QuestId)->State, EAPSQuestInstanceState::Completed);
	TestTrue(TEXT("Terminal duplicate after restore is idempotent"),
		Runtime->SubmitEvent(LastCommittedEvent, Reason));
	TestEqual(TEXT("Duplicate after restore does not re-emit reward"), RewardRequestCount, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
