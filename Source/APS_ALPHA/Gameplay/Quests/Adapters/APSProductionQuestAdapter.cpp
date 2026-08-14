#include "APSProductionQuestAdapter.h"

bool FAPSProductionQuestAdapter::Normalize(const FAPSProductionEvent& Source,
	FAPSQuestEvent& OutEvent, FString& OutReason)
{
	OutReason.Reset();
	if (!Source.StreamId.IsValid())
	{
		OutReason = TEXT("Production Quest adapter requires the authoritative Production StreamId");
		return false;
	}
	if (!Source.IsStructurallyValid(false, false, &OutReason))
	{
		return false;
	}

	if (Source.ContextTags.Contains(FName(TEXT("APS.DebugOnly"))))
	{
		OutReason = TEXT("Debug-only production events cannot advance Quest state");
		return false;
	}
	if (!Source.SubjectStableId.IsValid())
	{
		OutReason = TEXT("Production Quest adapter requires a canonical SubjectStableId");
		return false;
	}
	EAPSQuestEntityKind SubjectKind = EAPSQuestEntityKind::None;
	if (!MapSubjectKind(Source.SubjectIdentityDomain, SubjectKind, OutReason))
	{
		return false;
	}

	FAPSQuestEvent Normalized;
	Normalized.StreamId = Source.StreamId;
	Normalized.EventId = Source.EventId;
	Normalized.CorrelationId = Source.CorrelationId;
	Normalized.Sequence = Source.Sequence;
	Normalized.Verb = Source.Verb;
	Normalized.DefinitionId = Source.DefinitionId;
	Normalized.DefinitionSchemaVersion = Source.DefinitionSchemaVersion;
	Normalized.Result = MapResult(Source.Result);
	Normalized.Quantity = Source.Quantity;
	Normalized.FailureCode = Source.FailureCode;
	Normalized.ContextLabels = Source.ContextTags;
	if (!Source.ResultCode.IsNone())
	{
		Normalized.ContextLabels.AddUnique(Source.ResultCode);
	}
	if (Source.SubjectStableId.IsValid())
	{
		Normalized.Subject.Kind = SubjectKind;
		Normalized.Subject.Guid = Source.SubjectStableId;
	}
	if (Source.TargetStableId.IsValid())
	{
		Normalized.Target.Kind = EAPSQuestEntityKind::GameplayEntity;
		Normalized.Target.Guid = Source.TargetStableId;
	}
	if (!Normalized.IsStructurallyValid(&OutReason))
	{
		return false;
	}

	OutEvent = MoveTemp(Normalized);
	return true;
}

bool FAPSProductionQuestAdapter::MapSubjectKind(
	const EAPSSubjectIdentityDomain Domain, EAPSQuestEntityKind& OutKind,
	FString& OutReason)
{
	switch (Domain)
	{
	case EAPSSubjectIdentityDomain::GameplayEntity:
		OutKind = EAPSQuestEntityKind::GameplayEntity;
		return true;
	case EAPSSubjectIdentityDomain::Player:
		OutKind = EAPSQuestEntityKind::Player;
		return true;
	default:
		OutReason = TEXT("Production Quest adapter rejects a missing subject identity domain");
		return false;
	}
}

EAPSQuestEventResult FAPSProductionQuestAdapter::MapResult(EAPSProductionEventResult Result)
{
	switch (Result)
	{
	case EAPSProductionEventResult::Requested:
		return EAPSQuestEventResult::Requested;
	case EAPSProductionEventResult::Started:
		return EAPSQuestEventResult::Started;
	case EAPSProductionEventResult::Succeeded:
		return EAPSQuestEventResult::Succeeded;
	case EAPSProductionEventResult::Failed:
		return EAPSQuestEventResult::Failed;
	case EAPSProductionEventResult::Cancelled:
		return EAPSQuestEventResult::Cancelled;
	default:
		return EAPSQuestEventResult::Failed;
	}
}
