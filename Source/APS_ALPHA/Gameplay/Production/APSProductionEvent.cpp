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
	if ((bRequireSubjectId || bRequireTargetId) && !StreamId.IsValid())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Production event has no authoritative StreamId."));
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
	if (bRequireSubjectId
		&& SubjectIdentityDomain == EAPSSubjectIdentityDomain::None)
	{
		return RejectProductionEvent(OutReason,
			TEXT("Production event has no authoritative subject identity domain."));
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

bool FAPSProductionEventCorrelationRecord::IsStructurallyValid(
	FString* OutReason) const
{
	if (!CorrelationId.IsValid())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation has no CorrelationId."));
	}
	if (Verb.IsNone())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation has no Verb."));
	}
	if (!SubjectStableId.IsValid() || !TargetStableId.IsValid())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation has no canonical subject or target ID."));
	}
	if (SubjectIdentityDomain == EAPSSubjectIdentityDomain::None)
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation has no subject identity domain."));
	}
	if (DefinitionId.IsValid() && DefinitionSchemaVersion < 1)
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation definition schema is invalid."));
	}
	if (Quantity < 1)
	{
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation quantity must be at least one."));
	}
	switch (Result)
	{
	case EAPSProductionEventResult::Requested:
	case EAPSProductionEventResult::Started:
	case EAPSProductionEventResult::Succeeded:
	case EAPSProductionEventResult::Failed:
	case EAPSProductionEventResult::Cancelled:
		break;
	default:
		return RejectProductionEvent(OutReason,
			TEXT("Persisted production correlation result is invalid."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSProductionEventStreamState::IsStructurallyValid(FString* OutReason) const
{
	if (SchemaVersion < 1 || SchemaVersion > LatestSchemaVersion)
	{
		return RejectProductionEvent(OutReason,
			TEXT("Unsupported production event stream schema."));
	}
	if (SchemaVersion >= 2 && !StreamId.IsValid())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Production event stream has no StreamId."));
	}
	if (LastSequence < 0 || LastSequence < Correlations.Num())
	{
		return RejectProductionEvent(OutReason,
			TEXT("Production event stream sequence is invalid."));
	}
	TSet<FGuid> CorrelationIds;
	for (const FAPSProductionEventCorrelationRecord& Record : Correlations)
	{
		if (!Record.IsStructurallyValid(OutReason)
			|| CorrelationIds.Contains(Record.CorrelationId))
		{
			if (OutReason && OutReason->IsEmpty())
			{
				*OutReason = TEXT("Production event stream has a duplicate correlation.");
			}
			return false;
		}
		CorrelationIds.Add(Record.CorrelationId);
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}
