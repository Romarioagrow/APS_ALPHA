#include "APSInteractionContractTestActor.h"

AAPSInteractionContractTestActor::AAPSInteractionContractTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PromptId = FGuid::NewGuid();
}

FGuid AAPSInteractionContractTestActor::GetInteractionTargetStableId_Implementation() const
{
	return TargetStableId;
}

bool AAPSInteractionContractTestActor::QueryInteraction_Implementation(
	const FAPSInteractionContext& Context,
	FAPSInteractionPromptDescriptor& OutPrompt) const
{
	OutPrompt = FAPSInteractionPromptDescriptor{};
	OutPrompt.PromptId = PromptId;
	OutPrompt.Revision = Revision;
	OutPrompt.PromptKind = EAPSInteractionPromptKind::OpenPanel;
	OutPrompt.ContextKind = EAPSInteractionContextKind::HeadquartersConsole;
	OutPrompt.TargetStableId = TargetStableId;
	OutPrompt.TargetIdentityDomain = TargetIdentityDomain;
	OutPrompt.ContextStableId = TargetStableId;
	OutPrompt.DisplayName.Namespace = TEXT("APSInteractionTests");
	OutPrompt.DisplayName.Key = TEXT("TestConsole");
	OutPrompt.DisplayName.DefaultText = TEXT("Test Console");
	OutPrompt.RangeCm = 600.0;
	OutPrompt.Availability = EAPSInteractionAvailability::Available;
	FAPSInteractionActionDescriptor& Action = OutPrompt.Actions.AddDefaulted_GetRef();
	Action.ActionId = ActionId;
	Action.ActionKind = EAPSInteractionActionKind::Open;
	Action.InputActionName = TEXT("Interact");
	return true;
}

FAPSInteractionExecutionResult
AAPSInteractionContractTestActor::ExecuteInteraction_Implementation(
	const FAPSInteractionExecutionRequest& Request)
{
	++ExecutionCount;
	FAPSInteractionExecutionResult Result;
	Result.CorrelationId = Request.CorrelationId;
	Result.Status = ExecutionStatus;
	Result.Verb = Verb;
	Result.ResultCode = ResultCode;
	Result.FailureCode = FailureCode;
	Result.Quantity = Request.Quantity;
	return Result;
}
