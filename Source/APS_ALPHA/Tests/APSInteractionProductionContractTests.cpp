#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Gameplay/Interaction/APSInteractionSubsystem.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractionTypes.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEventSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSInteractionDescriptorContractTest,
	"APS.Gameplay.Interaction.DescriptorContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSInteractionDescriptorContractTest::RunTest(const FString& Parameters)
{
	FAPSInteractionActionDescriptor Action;
	Action.ActionId = TEXT("OpenConsole");
	Action.InputActionName = TEXT("Interact");
	Action.InputMode = EAPSInteractionInputMode::Press;

	FString Reason;
	TestTrue(TEXT("Enabled press action is structurally valid"),
		Action.IsStructurallyValid(&Reason));

	Action.InputMode = EAPSInteractionInputMode::Hold;
	Action.HoldDurationSeconds = 0.0;
	TestFalse(TEXT("Hold action requires positive duration"),
		Action.IsStructurallyValid(&Reason));
	Action.HoldDurationSeconds = 0.75;
	TestTrue(TEXT("Configured hold action is structurally valid"),
		Action.IsStructurallyValid(&Reason));

	FAPSInteractionPromptDescriptor Prompt;
	Prompt.PromptId = FGuid::NewGuid();
	Prompt.Revision = 3;
	Prompt.TargetStableId = FGuid::NewGuid();
	Prompt.ContextStableId = Prompt.TargetStableId;
	Prompt.DisplayName.Namespace = TEXT("APSInteraction");
	Prompt.DisplayName.Key = TEXT("HeadquartersConsole");
	Prompt.DistanceCm = 120.0;
	Prompt.RangeCm = 600.0;
	Prompt.Availability = EAPSInteractionAvailability::Available;
	Prompt.Actions.Add(Action);
	TestTrue(TEXT("Production prompt with canonical ID and action is valid"),
		Prompt.IsStructurallyValid(true, &Reason));

	Prompt.TargetStableId.Invalidate();
	TestFalse(TEXT("Production prompt rejects missing canonical TargetStableId"),
		Prompt.IsStructurallyValid(true, &Reason));
	TestTrue(TEXT("Explicit debug prompt may omit canonical TargetStableId"),
		Prompt.IsStructurallyValid(false, &Reason));

	FAPSInteractionExecutionRequest Request;
	Request.CorrelationId = FGuid::NewGuid();
	Request.ActionId = Action.ActionId;
	Request.ExpectedRevision = Prompt.Revision;
	Request.SubjectStableId = FGuid::NewGuid();
	Request.TargetStableId = FGuid::NewGuid();
	Request.Quantity = 1;
	TestTrue(TEXT("Production request with canonical subject/target IDs is valid"),
		Request.IsStructurallyValid(true, &Reason));
	Request.Quantity = 0;
	TestFalse(TEXT("Interaction request rejects zero quantity"),
		Request.IsStructurallyValid(true, &Reason));

	FAPSInteractionFocusScore Baseline;
	Baseline.Priority = 10;
	Baseline.FocusAlignment = 0.90;
	Baseline.DistanceCm = 200.0;
	Baseline.TargetStableId = FGuid(2, 0, 0, 0);
	FAPSInteractionFocusScore Candidate = Baseline;
	Candidate.Priority = 11;
	TestTrue(TEXT("Focus ordering prefers semantic priority first"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.FocusAlignment = 0.95;
	TestTrue(TEXT("Focus ordering prefers alignment second"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.DistanceCm = 150.0;
	TestTrue(TEXT("Focus ordering prefers distance third"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.TargetStableId = FGuid(1, 0, 0, 0);
	TestTrue(TEXT("Focus ordering uses canonical binary StableId last"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.TargetStableId.Invalidate();
	TestFalse(TEXT("Focus ordering rejects unresolved canonical StableId"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	FAPSInteractionFocusScore EmptyScore;
	TestTrue(TEXT("First valid focus candidate replaces empty score"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Baseline, EmptyScore));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionEventLifecycleContractTest,
	"APS.Gameplay.Production.EventLifecycleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionEventLifecycleContractTest::RunTest(const FString& Parameters)
{
	UAPSProductionEventSubsystem* Events = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Production event subsystem can be created for contract test"), Events);
	if (!Events)
	{
		return false;
	}

	int32 BroadcastCount = 0;
	Events->OnEventPublished().AddLambda(
		[&BroadcastCount](const FAPSProductionEvent&)
		{
			++BroadcastCount;
		});

	FAPSProductionEventPublishPolicy Policy;
	FAPSProductionEvent Requested;
	Requested.Verb = TEXT("APS.Shipyard.Enqueue");
	Requested.SubjectStableId = FGuid::NewGuid();
	Requested.TargetStableId = FGuid::NewGuid();
	Requested.DefinitionId = FPrimaryAssetId(TEXT("Ship"), TEXT("APS.TestShip"));
	Requested.DefinitionSchemaVersion = 1;
	Requested.Quantity = 2;
	Requested.Result = EAPSProductionEventResult::Requested;

	FString Failure;
	TestTrue(TEXT("Requested event publishes"),
		Events->PublishEvent(Requested, Policy, Failure));
	TestTrue(TEXT("Publisher assigns EventId"), Requested.EventId.IsValid());
	TestTrue(TEXT("Publisher assigns CorrelationId"), Requested.CorrelationId.IsValid());
	TestEqual(TEXT("First event sequence is one"), Requested.Sequence, int64{1});

	FAPSProductionEvent Started = Requested;
	Started.EventId.Invalidate();
	Started.Result = EAPSProductionEventResult::Started;
	Started.Sequence = 0;
	TestTrue(TEXT("Requested may transition to Started"),
		Events->PublishEvent(Started, Policy, Failure));

	FAPSProductionEvent Drifted = Started;
	Drifted.EventId.Invalidate();
	Drifted.TargetStableId = FGuid::NewGuid();
	Drifted.Result = EAPSProductionEventResult::Succeeded;
	Drifted.Sequence = 0;
	TestFalse(TEXT("Correlation rejects target payload drift"),
		Events->PublishEvent(Drifted, Policy, Failure));

	FAPSProductionEvent Succeeded = Started;
	Succeeded.EventId.Invalidate();
	Succeeded.Result = EAPSProductionEventResult::Succeeded;
	Succeeded.ResultCode = TEXT("APS.Shipyard.JobCompleted");
	Succeeded.Sequence = 0;
	TestTrue(TEXT("Started may transition to terminal success"),
		Events->PublishEvent(Succeeded, Policy, Failure));

	FAPSProductionEvent DuplicateTerminal = Succeeded;
	DuplicateTerminal.EventId.Invalidate();
	DuplicateTerminal.Sequence = 0;
	TestFalse(TEXT("Correlation accepts only one terminal result"),
		Events->PublishEvent(DuplicateTerminal, Policy, Failure));
	TestEqual(TEXT("Only accepted lifecycle events broadcast"), BroadcastCount, 3);
	TestEqual(TEXT("Rejected events do not consume sequence"),
		Events->GetLastSequence(), int64{3});

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionEventPersistenceContractTest,
	"APS.Gameplay.Production.EventPersistenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionEventPersistenceContractTest::RunTest(const FString& Parameters)
{
	UAPSProductionEventSubsystem* Source = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Source event subsystem exists"), Source);
	if (!Source)
	{
		return false;
	}

	FAPSProductionEventPublishPolicy ProductionPolicy;
	FAPSProductionEvent Requested;
	Requested.Verb = TEXT("APS.Building.Build");
	Requested.SubjectStableId = FGuid::NewGuid();
	Requested.TargetStableId = FGuid::NewGuid();
	Requested.DefinitionId = FPrimaryAssetId(TEXT("Buildable"), TEXT("APS.TestHQ"));
	Requested.DefinitionSchemaVersion = 2;
	Requested.Quantity = 1;
	Requested.Result = EAPSProductionEventResult::Requested;
	FString Failure;
	TestTrue(TEXT("Production request publishes before save"),
		Source->PublishEvent(Requested, ProductionPolicy, Failure));

	FAPSProductionEvent Started = Requested;
	Started.EventId.Invalidate();
	Started.Sequence = 0;
	Started.Result = EAPSProductionEventResult::Started;
	TestTrue(TEXT("Production lifecycle reaches Started before save"),
		Source->PublishEvent(Started, ProductionPolicy, Failure));

	FAPSProductionEventPublishPolicy DebugPolicy;
	DebugPolicy.bDebugOnly = true;
	FAPSProductionEvent DebugRequested;
	DebugRequested.Verb = TEXT("APS.Crafting.Craft");
	DebugRequested.Result = EAPSProductionEventResult::Requested;
	TestTrue(TEXT("Explicit debug event publishes without canonical actor IDs"),
		Source->PublishEvent(DebugRequested, DebugPolicy, Failure));

	FAPSProductionEventStreamState Persisted;
	Source->ExportStreamState(Persisted);
	TestEqual(TEXT("Global sequence includes accepted debug events"),
		Persisted.LastSequence, int64{3});
	TestEqual(TEXT("Debug-only correlation is excluded from persistence"),
		Persisted.Correlations.Num(), 1);
	TestEqual(TEXT("Persisted correlation is the production lifecycle"),
		Persisted.Correlations[0].CorrelationId, Requested.CorrelationId);

	UAPSProductionEventSubsystem* Restored = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Restored event subsystem exists"), Restored);
	if (!Restored)
	{
		return false;
	}
	int32 RestoreBroadcastCount = 0;
	Restored->OnEventPublished().AddLambda(
		[&RestoreBroadcastCount](const FAPSProductionEvent&)
		{
			++RestoreBroadcastCount;
		});
	TestTrue(TEXT("Persisted event stream restores"),
		Restored->RestoreStreamState(Persisted, Failure));
	TestEqual(TEXT("Restore never rebroadcasts historical events"),
		RestoreBroadcastCount, 0);

	FAPSProductionEvent Succeeded = Started;
	Succeeded.EventId.Invalidate();
	Succeeded.Sequence = 0;
	Succeeded.Result = EAPSProductionEventResult::Succeeded;
	Succeeded.ResultCode = TEXT("APS.Building.JobCompleted");
	TestTrue(TEXT("Restored Started correlation may complete"),
		Restored->PublishEvent(Succeeded, ProductionPolicy, Failure));
	TestEqual(TEXT("Sequence continues after restored stream"),
		Succeeded.Sequence, int64{4});

	FAPSProductionEvent DuplicateTerminal = Succeeded;
	DuplicateTerminal.EventId.Invalidate();
	DuplicateTerminal.Sequence = 0;
	TestFalse(TEXT("Restored stream still rejects duplicate terminal result"),
		Restored->PublishEvent(DuplicateTerminal, ProductionPolicy, Failure));
	return true;
}

#endif
