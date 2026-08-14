#include "APSProductionEventSubsystem.h"

#include "Engine/World.h"

void UAPSProductionEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureStreamId();
}

void UAPSProductionEventSubsystem::RestoreLastSequence(const int64 PersistedLastSequence)
{
	LastSequence = FMath::Max(LastSequence, PersistedLastSequence);
}

void UAPSProductionEventSubsystem::EnsureStreamId() const
{
	if (!StreamId.IsValid())
	{
		StreamId = FGuid::NewGuid();
	}
}

void UAPSProductionEventSubsystem::ExportStreamState(
	FAPSProductionEventStreamState& OutState) const
{
	EnsureStreamId();
	OutState = FAPSProductionEventStreamState{};
	OutState.StreamId = StreamId;
	OutState.LastSequence = LastSequence;
	for (const TPair<FGuid, FCorrelationState>& Pair : CorrelationStates)
	{
		if (Pair.Value.bDebugOnly)
		{
			continue;
		}
		FAPSProductionEventCorrelationRecord& Record =
			OutState.Correlations.AddDefaulted_GetRef();
		Record.CorrelationId = Pair.Key;
		Record.Verb = Pair.Value.Verb;
		Record.SubjectStableId = Pair.Value.SubjectStableId;
		Record.SubjectIdentityDomain = Pair.Value.SubjectIdentityDomain;
		Record.TargetStableId = Pair.Value.TargetStableId;
		Record.DefinitionId = Pair.Value.DefinitionId;
		Record.DefinitionSchemaVersion = Pair.Value.DefinitionSchemaVersion;
		Record.Quantity = Pair.Value.Quantity;
		Record.Result = Pair.Value.Result;
	}
	OutState.Correlations.Sort([](
		const FAPSProductionEventCorrelationRecord& Left,
		const FAPSProductionEventCorrelationRecord& Right)
		{
			if (Left.CorrelationId.A != Right.CorrelationId.A)
			{
				return Left.CorrelationId.A < Right.CorrelationId.A;
			}
			if (Left.CorrelationId.B != Right.CorrelationId.B)
			{
				return Left.CorrelationId.B < Right.CorrelationId.B;
			}
			if (Left.CorrelationId.C != Right.CorrelationId.C)
			{
				return Left.CorrelationId.C < Right.CorrelationId.C;
			}
			return Left.CorrelationId.D < Right.CorrelationId.D;
		});
}

bool UAPSProductionEventSubsystem::RestoreStreamState(
	const FAPSProductionEventStreamState& State, FString& OutFailure)
{
	OutFailure.Reset();
	if (!State.IsStructurallyValid(&OutFailure))
	{
		return false;
	}
	if (LastSequence != 0 || !CorrelationStates.IsEmpty()
		|| !PublishedEventIds.IsEmpty())
	{
		OutFailure = TEXT("APS.Production.EventStreamAlreadyInitialized");
		return false;
	}
	return ReplaceValidatedStreamState(State, OutFailure);
}

bool UAPSProductionEventSubsystem::ReplaceStreamStateForLoad(
	const FAPSProductionEventStreamState& State, FString& OutFailure)
{
	OutFailure.Reset();
	if (!State.IsStructurallyValid(&OutFailure))
	{
		return false;
	}
	return ReplaceValidatedStreamState(State, OutFailure);
}

bool UAPSProductionEventSubsystem::ReplaceValidatedStreamState(
	const FAPSProductionEventStreamState& State, FString& OutFailure)
{
	TMap<FGuid, FCorrelationState> ReplacementCorrelations;
	for (const FAPSProductionEventCorrelationRecord& Record : State.Correlations)
	{
		FCorrelationState& Restored = ReplacementCorrelations.Add(Record.CorrelationId);
		Restored.Result = Record.Result;
		Restored.Verb = Record.Verb;
		Restored.SubjectStableId = Record.SubjectStableId;
		Restored.SubjectIdentityDomain = Record.SubjectIdentityDomain;
		Restored.TargetStableId = Record.TargetStableId;
		Restored.DefinitionId = Record.DefinitionId;
		Restored.DefinitionSchemaVersion = Record.DefinitionSchemaVersion;
		Restored.Quantity = Record.Quantity;
		Restored.bDebugOnly = false;
	}

	CorrelationStates = MoveTemp(ReplacementCorrelations);
	PublishedEventIds.Reset();
	StreamId = State.SchemaVersion >= 2 && State.StreamId.IsValid()
		? State.StreamId : FGuid::NewGuid();
	LastSequence = State.LastSequence;
	OutFailure.Reset();
	return true;
}

bool UAPSProductionEventSubsystem::ValidateTransition(
	const FAPSProductionEvent& Event, const bool bDebugOnly,
	FString& OutFailure) const
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
	if (Previous->bDebugOnly != bDebugOnly)
	{
		OutFailure = TEXT("APS.Production.CorrelationAccessModeMismatch");
		return false;
	}
	if (Previous->Verb != Event.Verb
		|| Previous->SubjectStableId != Event.SubjectStableId
		|| Previous->SubjectIdentityDomain != Event.SubjectIdentityDomain
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
	EnsureStreamId();
	if (Event.StreamId.IsValid() && Event.StreamId != StreamId)
	{
		OutFailure = TEXT("APS.Production.EventStreamMismatch");
		return false;
	}
	Event.StreamId = StreamId;
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
	if (!ValidateTransition(Event, Policy.bDebugOnly, OutFailure))
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
	State.SubjectIdentityDomain = Event.SubjectIdentityDomain;
	State.TargetStableId = Event.TargetStableId;
	State.DefinitionId = Event.DefinitionId;
	State.DefinitionSchemaVersion = Event.DefinitionSchemaVersion;
	State.Quantity = Event.Quantity;
	State.bDebugOnly = Policy.bDebugOnly;
	EventPublished.Broadcast(Event);
	return true;
}
