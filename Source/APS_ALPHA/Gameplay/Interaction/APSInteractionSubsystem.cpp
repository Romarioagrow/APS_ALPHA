#include "APSInteractionSubsystem.h"

#include "APSInteractable.h"
#include "GameFramework/Actor.h"

namespace APSInteractionFailures
{
	const FName InvalidCandidate(TEXT("APS.Interaction.InvalidCandidate"));
	const FName NotInteractable(TEXT("APS.Interaction.NotInteractable"));
	const FName MissingIdentity(TEXT("APS.Interaction.MissingCanonicalIdentity"));
	const FName InvalidRequest(TEXT("APS.Interaction.InvalidRequest"));
}

double UAPSInteractionSubsystem::DistanceToActorBoundsCm(
	const AActor* Candidate, const FVector& Origin)
{
	if (!IsValid(Candidate))
	{
		return TNumericLimits<double>::Max();
	}
	FVector BoundsOrigin;
	FVector BoundsExtent;
	Candidate->GetActorBounds(true, BoundsOrigin, BoundsExtent);
	const FBox Bounds(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
	return FMath::Sqrt(Bounds.ComputeSquaredDistanceToPoint(Origin));
}

bool UAPSInteractionSubsystem::QueryActor(
	AActor* Candidate, const FAPSInteractionContext& Context,
	FAPSInteractionPromptDescriptor& OutPrompt, FString& OutFailure) const
{
	OutPrompt = FAPSInteractionPromptDescriptor{};
	OutFailure.Reset();
	if (!IsValid(Candidate))
	{
		OutFailure = APSInteractionFailures::InvalidCandidate.ToString();
		return false;
	}
	if (!Candidate->GetClass()->ImplementsInterface(UAPSInteractable::StaticClass()))
	{
		OutFailure = APSInteractionFailures::NotInteractable.ToString();
		return false;
	}
	if (!Context.bDebugIdentityOverride && !Context.SubjectStableId.IsValid())
	{
		OutFailure = APSInteractionFailures::MissingIdentity.ToString();
		return false;
	}

	const double DistanceCm = DistanceToActorBoundsCm(Candidate, Context.ViewOrigin);
	if (!FMath::IsFinite(DistanceCm) || Context.MaximumRangeCm <= 0.0
		|| DistanceCm > Context.MaximumRangeCm)
	{
		OutFailure = TEXT("APS.Interaction.OutOfRange");
		return false;
	}
	if (!IAPSInteractable::Execute_QueryInteraction(Candidate, Context, OutPrompt))
	{
		OutFailure = TEXT("APS.Interaction.QueryRejected");
		return false;
	}
	if (!OutPrompt.TargetStableId.IsValid())
	{
		OutPrompt.TargetStableId = IAPSInteractable::Execute_GetInteractionTargetStableId(Candidate);
	}
	OutPrompt.DistanceCm = DistanceCm;
	if (OutPrompt.RangeCm <= 0.0)
	{
		OutPrompt.RangeCm = Context.MaximumRangeCm;
	}
	if (DistanceCm > OutPrompt.RangeCm)
	{
		OutFailure = TEXT("APS.Interaction.ActorRangeRejected");
		return false;
	}
	if (!OutPrompt.IsStructurallyValid(!Context.bDebugIdentityOverride, &OutFailure))
	{
		return false;
	}
	return true;
}

FAPSInteractionExecutionResult UAPSInteractionSubsystem::ExecuteActor(
	AActor* Candidate, const FAPSInteractionExecutionRequest& Request,
	const bool bDebugIdentityOverride) const
{
	FAPSInteractionExecutionResult Result;
	Result.CorrelationId = Request.CorrelationId;
	Result.Quantity = Request.Quantity;
	if (!IsValid(Candidate))
	{
		Result.FailureCode = APSInteractionFailures::InvalidCandidate;
		return Result;
	}
	if (!Candidate->GetClass()->ImplementsInterface(UAPSInteractable::StaticClass()))
	{
		Result.FailureCode = APSInteractionFailures::NotInteractable;
		return Result;
	}
	FString ValidationReason;
	if (!Request.IsStructurallyValid(!bDebugIdentityOverride, &ValidationReason))
	{
		Result.FailureCode = APSInteractionFailures::InvalidRequest;
		return Result;
	}
	const FGuid TargetId = IAPSInteractable::Execute_GetInteractionTargetStableId(Candidate);
	if (!bDebugIdentityOverride && (!TargetId.IsValid() || TargetId != Request.TargetStableId))
	{
		Result.FailureCode = APSInteractionFailures::MissingIdentity;
		return Result;
	}
	Result = IAPSInteractable::Execute_ExecuteInteraction(Candidate, Request);
	if (!Result.CorrelationId.IsValid())
	{
		Result.CorrelationId = Request.CorrelationId;
	}
	return Result;
}
