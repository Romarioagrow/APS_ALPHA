#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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

#endif
