#include "APSInteractionTypes.h"

namespace
{
	bool Reject(FString* OutReason, const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	}
}

bool FAPSInteractionActionDescriptor::IsStructurallyValid(FString* OutReason) const
{
	if (ActionId.IsNone())
	{
		return Reject(OutReason, TEXT("Interaction action has no stable ActionId."));
	}
	if (InputActionName.IsNone())
	{
		return Reject(OutReason, TEXT("Interaction action has no Enhanced Input action name."));
	}
	if (!bEnabled && !UnavailableReason.IsSet())
	{
		return Reject(OutReason, TEXT("Disabled interaction action has no unavailable reason."));
	}
	if (InputMode == EAPSInteractionInputMode::Hold && HoldDurationSeconds <= 0.0)
	{
		return Reject(OutReason, TEXT("Hold interaction action has a non-positive duration."));
	}
	if (!FMath::IsFinite(HoldProgress) || HoldProgress < 0.0 || HoldProgress > 1.0)
	{
		return Reject(OutReason, TEXT("Interaction hold progress is outside [0,1]."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSInteractionPromptDescriptor::IsStructurallyValid(
	const bool bRequireAuthoritativeTargetId, FString* OutReason) const
{
	if (!PromptId.IsValid())
	{
		return Reject(OutReason, TEXT("Interaction prompt has no stable PromptId."));
	}
	if (Revision < 0)
	{
		return Reject(OutReason, TEXT("Interaction prompt revision is negative."));
	}
	if (bRequireAuthoritativeTargetId && !TargetStableId.IsValid())
	{
		return Reject(OutReason, TEXT("Production interaction prompt has no canonical TargetStableId."));
	}
	if (bRequireAuthoritativeTargetId && !ContextStableId.IsValid())
	{
		return Reject(OutReason,
			TEXT("Production interaction prompt has no canonical ContextStableId."));
	}
	if (!DisplayName.IsSet())
	{
		return Reject(OutReason, TEXT("Interaction prompt has no display-name localization descriptor."));
	}
	if (!FMath::IsFinite(DistanceCm) || !FMath::IsFinite(RangeCm)
		|| DistanceCm < 0.0 || RangeCm <= 0.0)
	{
		return Reject(OutReason, TEXT("Interaction prompt range values are invalid."));
	}
	if (Availability != EAPSInteractionAvailability::Hidden && Actions.IsEmpty())
	{
		return Reject(OutReason, TEXT("Visible interaction prompt has no actions."));
	}
	TSet<FName> UniqueActionIds;
	for (const FAPSInteractionActionDescriptor& Action : Actions)
	{
		FString ActionReason;
		if (!Action.IsStructurallyValid(&ActionReason))
		{
			if (OutReason)
			{
				*OutReason = MoveTemp(ActionReason);
			}
			return false;
		}
		if (UniqueActionIds.Contains(Action.ActionId))
		{
			return Reject(OutReason, TEXT("Interaction prompt contains duplicate ActionId values."));
		}
		UniqueActionIds.Add(Action.ActionId);
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSInteractionExecutionRequest::IsStructurallyValid(
	const bool bRequireAuthoritativeIds, FString* OutReason) const
{
	if (!CorrelationId.IsValid())
	{
		return Reject(OutReason, TEXT("Interaction request has no CorrelationId."));
	}
	if (ActionId.IsNone())
	{
		return Reject(OutReason, TEXT("Interaction request has no ActionId."));
	}
	if (ExpectedRevision < 0)
	{
		return Reject(OutReason, TEXT("Interaction request revision is negative."));
	}
	if (Quantity < 1)
	{
		return Reject(OutReason, TEXT("Interaction request quantity must be at least one."));
	}
	if (bRequireAuthoritativeIds && (!SubjectStableId.IsValid() || !TargetStableId.IsValid()))
	{
		return Reject(OutReason, TEXT("Production interaction request is missing canonical subject/target IDs."));
	}
	if (bRequireAuthoritativeIds
		&& SubjectIdentityDomain == EAPSSubjectIdentityDomain::None)
	{
		return Reject(OutReason,
			TEXT("Production interaction request has no authoritative subject identity domain."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}
