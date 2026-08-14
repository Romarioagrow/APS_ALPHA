#include "APSProductionEventSubsystem.h"

#include "Engine/World.h"

void UAPSProductionEventSubsystem::RestoreLastSequence(const int64 PersistedLastSequence)
{
	LastSequence = FMath::Max(LastSequence, PersistedLastSequence);
}

bool UAPSProductionEventSubsystem::ValidateTransition(
	const FAPSProductionEvent& Event, FString& OutFailure) const
{
	const FCorrelationState* Previous = CorrelationStates.Find(Event.CorrelationId);
	if (Event.Result == EAPSProductionEventResult::Requested)
	{
		if (Previous)
		{
			OutFailure = TEXT("APS.Production.DuplicateRequest");
			return false;
		}
		return true;
	}
	if (!Previous)
	{
		OutFailure = TEXT("APS.Production.MissingRequestedEvent");
		return false;
	}
	if (Previous->Verb != Event.Verb
		|| Previous->SubjectStableId != Event.SubjectStableId
		|| Previous->TargetStableId != Event.TargetStableId
		|| Previous->DefinitionId != Event.DefinitionId
		|| Previous->DefinitionSchemaVersion != Event.DefinitionSchemaVersion
		|| Previous->Quantity != Event.Quantity)
	{
		OutFailure = TEXT("APS.Production.CorrelationPayloadMismatch");
		return false;
	}
	if (Previous->Result == EAPSProductionEventResult::Succeeded
		|| Previous->Result == EAPSProductionEventResult::Failed
		|| Previous->Result == EAPSProductionEventResult::Cancelled)
	{
		OutFailure = TEXT("APS.Production.CorrelationAlreadyTerminal");
		return false;
	}
	if (Event.Result == EAPSProductionEventResult::Started
		&& Previous->Result != EAPSProductionEventResult::Requested)
	{
		OutFailure = TEXT("APS.Production.InvalidStartedTransition");
		return false;
	}
	return true;
}

bool UAPSProductionEventSubsystem::PublishEvent(
	FAPSProductionEvent& Event, const FAPSProductionEventPublishPolicy& Policy,
	FString& OutFailure)
{
	OutFailure.Reset();
	if (Policy.bDebugOnly)
	{
		// Debug launcher callers may omit canonical actor IDs, but the event remains
		// explicitly marked and cannot silently enter the production path.
		Event.ContextTags.AddUnique(FName(TEXT("APS.DebugOnly")));
	}
	if (!Event.EventId.IsValid())
	{
		Event.EventId = FGuid::NewGuid();
	}
	if (Event.Result == EAPSProductionEventResult::Requested
		&& !Event.CorrelationId.IsValid())
	{
		Event.CorrelationId = FGuid::NewGuid();
	}
	if (PublishedEventIds.Contains(Event.EventId))
	{
		OutFailure = TEXT("APS.Production.DuplicateEventId");
		return false;
	}
	if (!ValidateTransition(Event, OutFailure))
	{
		return false;
	}

	Event.Sequence = LastSequence + 1;
	Event.TimestampWorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bRequireSubject = Policy.bRequireSubjectStableId && !Policy.bDebugOnly;
	const bool bRequireTarget = Policy.bRequireTargetStableId && !Policy.bDebugOnly;
	if (!Event.IsStructurallyValid(bRequireSubject, bRequireTarget, &OutFailure))
	{
		Event.Sequence = 0;
		return false;
	}

	LastSequence = Event.Sequence;
	PublishedEventIds.Add(Event.EventId);
	FCorrelationState& State = CorrelationStates.FindOrAdd(Event.CorrelationId);
	State.Result = Event.Result;
	State.Verb = Event.Verb;
	State.SubjectStableId = Event.SubjectStableId;
	State.TargetStableId = Event.TargetStableId;
	State.DefinitionId = Event.DefinitionId;
	State.DefinitionSchemaVersion = Event.DefinitionSchemaVersion;
	State.Quantity = Event.Quantity;
	EventPublished.Broadcast(Event);
	return true;
}
