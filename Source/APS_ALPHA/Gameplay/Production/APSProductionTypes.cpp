#include "APSProductionTypes.h"

#include "GameFramework/Actor.h"

namespace
{
	bool RejectAPSProductionType(FString* OutReason, const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	}

	bool HasDuplicateAPSProductionAmounts(const TArray<FAPSProductionAmount>& Amounts)
	{
		TSet<FPrimaryAssetId> Seen;
		for (const FAPSProductionAmount& Amount : Amounts)
		{
			if (Seen.Contains(Amount.ItemId))
			{
				return true;
			}
			Seen.Add(Amount.ItemId);
		}
		return false;
	}
}

bool FAPSProductionAmount::IsStructurallyValid(FString* OutReason) const
{
	if (!ItemId.IsValid())
	{
		return RejectAPSProductionType(OutReason, TEXT("Production amount has no ItemId."));
	}
	if (ItemSchemaVersion < 1)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production amount schema version is invalid."));
	}
	if (Quantity < 1)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production amount quantity must be positive."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSProductionDefinition::IsStructurallyValid(FString* OutReason) const
{
	if (!DefinitionId.IsValid())
	{
		return RejectAPSProductionType(OutReason, TEXT("Production definition has no DefinitionId."));
	}
	if (DefinitionSchemaVersion < 1)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production definition schema version is invalid."));
	}
	const FPrimaryAssetType ExpectedType = Domain == EAPSProductionDomain::Crafting
		? FPrimaryAssetType(TEXT("Recipe"))
		: Domain == EAPSProductionDomain::Building
			? FPrimaryAssetType(TEXT("Buildable"))
			: FPrimaryAssetType(TEXT("Ship"));
	if (DefinitionId.PrimaryAssetType != ExpectedType)
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production definition primary asset type does not match its domain."));
	}
	if (Category.IsNone() || !DisplayName.IsSet())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production definition requires category and display localization."));
	}
	if (!FMath::IsFinite(DurationSeconds) || DurationSeconds < 0.0)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production duration is invalid."));
	}
	if (MaximumBatchSize < 1)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production maximum batch size is invalid."));
	}
	for (const FAPSProductionAmount& Cost : Costs)
	{
		if (!Cost.IsStructurallyValid(OutReason))
		{
			return false;
		}
	}
	for (const FAPSProductionAmount& Output : Outputs)
	{
		if (!Output.IsStructurallyValid(OutReason))
		{
			return false;
		}
	}
	if (HasDuplicateAPSProductionAmounts(Costs)
		|| HasDuplicateAPSProductionAmounts(Outputs))
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production definition contains duplicate item amounts."));
	}
	if (Domain != EAPSProductionDomain::Crafting && !Outputs.IsEmpty())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Building and Shipyard definitions materialize actors and cannot carry inventory outputs."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSProductionContextRegistration::IsStructurallyValid(FString* OutReason) const
{
	if (!ContextStableId.IsValid() || !OwnerStableId.IsValid())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production context requires stable context and owner IDs."));
	}
	if (QueueCapacity < 1 || MaximumConcurrentJobs < 1
		|| MaximumConcurrentJobs > QueueCapacity)
	{
		return RejectAPSProductionType(OutReason, TEXT("Production queue capacity is invalid."));
	}
	if (AccessMode == EAPSProductionAccessMode::ActorGated && !IsValid(ContextActor))
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Actor-gated production context has no live context actor."));
	}
	if (AccessMode == EAPSProductionAccessMode::DebugLauncher && IsValid(ContextActor))
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Debug launcher context must not masquerade as an actor-gated context."));
	}
	if (AccessMode == EAPSProductionAccessMode::ActorGated
		&& Domain == EAPSProductionDomain::Shipyard && !SpawnPadStableId.IsValid())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Actor-gated Shipyard context has no canonical spawn pad ID."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

bool FAPSProductionJobRecord::IsTerminal() const
{
	return State == EAPSProductionJobState::Succeeded
		|| State == EAPSProductionJobState::Failed
		|| State == EAPSProductionJobState::Cancelled;
}

bool FAPSProductionJobRecord::IsStructurallyValid(FString* OutReason) const
{
	if (!JobId.IsValid() || !CorrelationId.IsValid()
		|| (!bDebugOnly && !SubjectStableId.IsValid())
		|| !ContextStableId.IsValid() || !OwnerStableId.IsValid())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production job requires stable job, correlation, subject, context and owner IDs."));
	}
	if (!DefinitionId.IsValid() || DefinitionSchemaVersion < 1 || Quantity < 1)
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production job definition identity or quantity is invalid."));
	}
	if (!FMath::IsFinite(ProgressSeconds) || ProgressSeconds < 0.0
		|| !FMath::IsFinite(DurationSeconds) || DurationSeconds < 0.0
		|| QueueOrdinal < 1)
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Production job progress, duration or ordinal is invalid."));
	}
	if (State == EAPSProductionJobState::Failed && FailureCode.IsNone())
	{
		return RejectAPSProductionType(OutReason, TEXT("Failed production job has no failure code."));
	}
	if (State != EAPSProductionJobState::Failed && !FailureCode.IsNone())
	{
		return RejectAPSProductionType(OutReason,
			TEXT("Non-failed production job carries a failure code."));
	}
	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}
