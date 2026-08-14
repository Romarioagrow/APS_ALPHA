#include "APSProductionQuestAdapter.h"

bool FAPSProductionQuestAdapter::Normalize(const FAPSProductionEvent& Source,
	const FGuid& StreamId, FAPSQuestEvent& OutEvent, FString& OutReason)
{
	OutReason.Reset();
	if (!StreamId.IsValid())
	{
		OutReason = TEXT("Production Quest adapter requires a configured save/world StreamId");
		return false;
	}
	if (!Source.IsStructurallyValid(false, false, &OutReason))
	{
		return false;
	}

	FAPSQuestEvent Normalized;
	Normalized.StreamId = StreamId;
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
		Normalized.Subject.Kind = EAPSQuestEntityKind::GameplayEntity;
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
