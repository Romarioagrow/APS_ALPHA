#include "APSProductionEvent.h"

namespace
{
	bool RejectProductionEvent(FString* OutReason, const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	}
}

bool FAPSProductionEvent::IsTerminal() const
{
	return Result == EAPSProductionEventResult::Succeeded
		|| Result == EAPSProductionEventResult::Failed
		|| Result == EAPSProductionEventResult::Cancelled;
}

bool FAPSProductionEvent::IsStructurallyValid(
	const bool bRequireSubjectId, const bool bRequireTargetId, FString* OutReason) const
{
	if (!EventId.IsValid())
	{
		return RejectProductionEvent(OutReason, TEXT("Production event has no EventId."));
	}
	if (!CorrelationId.IsValid())
	{
		return RejectProductionEvent(OutReason, TEXT("Production event has no CorrelationId."));
	}
	if (Verb.IsNone())
	{
		return RejectProductionEvent(OutReason, TEXT("Production event has no Verb."));
	}
	if (Quantity < 1)
	{
		return RejectProductionEvent(OutReason, TEXT("Production event quantity must be at least one."));
	}
	if (DefinitionId.IsValid() && DefinitionSchemaVersion < 1)
	{
		return RejectProductionEvent(OutReason, TEXT("Production event definition schema version is invalid."));
	}
	if (bRequireSubjectId && !SubjectStableId.IsValid())
	{
		return RejectProductionEvent(OutReason, TEXT("Production event has no canonical SubjectStableId."));
	}
	if (bRequireTargetId && !TargetStableId.IsValid())
	{
		return RejectProductionEvent(OutReason, TEXT("Production event has no canonical TargetStableId."));
	}
	if (Result == EAPSProductionEventResult::Failed && FailureCode.IsNone())
	{
		return RejectProductionEvent(OutReason, TEXT("Failed production event has no FailureCode."));
	}
	if (Result != EAPSProductionEventResult::Failed && !FailureCode.IsNone())
	{
		return RejectProductionEvent(OutReason, TEXT("Non-failed production event carries a FailureCode."));
	}
	if (Sequence < 1)
	{
		return RejectProductionEvent(OutReason, TEXT("Production event sequence is not assigned."));
	}
	if (!FMath::IsFinite(TimestampWorldSeconds) || TimestampWorldSeconds < 0.0)
	{
		return RejectProductionEvent(OutReason, TEXT("Production event timestamp is invalid."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}
