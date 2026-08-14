#include "APSInteractionSubsystem.h"

#include "APSInteractable.h"
#include "GameFramework/Actor.h"

namespace APSInteractionFailures
{
	const FName InvalidCandidate(TEXT("APS.Interaction.InvalidCandidate"));
	const FName NotInteractable(TEXT("APS.Interaction.NotInteractable"));
	const FName MissingIdentity(TEXT("APS.Interaction.MissingCanonicalIdentity"));
	const FName InvalidRequest(TEXT("APS.Interaction.InvalidRequest"));
	const FName SubjectMismatch(TEXT("APS.Interaction.SubjectMismatch"));
	const FName TargetMismatch(TEXT("APS.Interaction.TargetMismatch"));
	const FName StaleRevision(TEXT("APS.Interaction.StaleRevision"));
	const FName ActionUnavailable(TEXT("APS.Interaction.ActionUnavailable"));
	const FName InvalidExecutionResult(TEXT("APS.Interaction.InvalidExecutionResult"));
	const FName NoCandidates(TEXT("APS.Interaction.NoCandidates"));
	const FName AllCandidatesInvalid(TEXT("APS.Interaction.AllCandidatesInvalid"));
	const FName AllCandidatesOutOfRange(TEXT("APS.Interaction.AllCandidatesOutOfRange"));
	const FName AllCandidatesOutOfFocus(TEXT("APS.Interaction.AllCandidatesOutOfFocus"));
	const FName AllCandidatesGated(TEXT("APS.Interaction.AllCandidatesGated"));
	const FName NoEligibleCandidate(TEXT("APS.Interaction.NoEligibleCandidate"));
}

void UAPSInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureExecutionStreamId();
}

void UAPSInteractionSubsystem::Deinitialize()
{
	ExecutionPublished.Clear();
	ExecutionStreamId.Invalidate();
	LastExecutionSequence = 0;
	Super::Deinitialize();
}

double UAPSInteractionSubsystem::DistanceToActorBoundsCm(
	const AActor* Candidate, const FVector& Origin)
{
	if (!IsValid(Candidate))
	{
		return TNumericLimits<double>::Max();
	}
	const auto TryDistanceToBounds = [&Origin](const FBox& Bounds,
		double& OutDistanceCm)
	{
		if (!Bounds.IsValid || Bounds.ContainsNaN())
		{
			return false;
		}
		const double SquaredDistance = Bounds.ComputeSquaredDistanceToPoint(Origin);
		if (!FMath::IsFinite(SquaredDistance) || SquaredDistance < 0.0)
		{
			return false;
		}
		OutDistanceCm = FMath::Sqrt(SquaredDistance);
		return FMath::IsFinite(OutDistanceCm);
	};
	double DistanceCm = 0.0;
	if (TryDistanceToBounds(Candidate->GetComponentsBoundingBox(false), DistanceCm)
		|| TryDistanceToBounds(Candidate->GetComponentsBoundingBox(true), DistanceCm))
	{
		return DistanceCm;
	}
	const FVector ActorLocation = Candidate->GetActorLocation();
	if (ActorLocation.ContainsNaN())
	{
		return TNumericLimits<double>::Max();
	}
	const double ActorDistanceCm = FVector::Distance(ActorLocation, Origin);
	return FMath::IsFinite(ActorDistanceCm)
		? ActorDistanceCm : TNumericLimits<double>::Max();
}

bool UAPSInteractionSubsystem::IsFocusScorePreferred(
	const FAPSInteractionFocusScore& Candidate,
	const FAPSInteractionFocusScore& Current)
{
	if (!Candidate.IsStructurallyValid())
	{
		return false;
	}
	if (!Current.IsStructurallyValid())
	{
		return true;
	}
	if (Candidate.Priority != Current.Priority)
	{
		return Candidate.Priority > Current.Priority;
	}
	if (Candidate.FocusAlignment != Current.FocusAlignment)
	{
		return Candidate.FocusAlignment > Current.FocusAlignment;
	}
	if (Candidate.DistanceCm != Current.DistanceCm)
	{
		return Candidate.DistanceCm < Current.DistanceCm;
	}
	// FGuid's numeric fields are the canonical normalized order. No string case,
	// actor name, pointer, or candidate input order participates in the tie-break.
	if (Candidate.TargetStableId.A != Current.TargetStableId.A)
	{
		return Candidate.TargetStableId.A < Current.TargetStableId.A;
	}
	if (Candidate.TargetStableId.B != Current.TargetStableId.B)
	{
		return Candidate.TargetStableId.B < Current.TargetStableId.B;
	}
	if (Candidate.TargetStableId.C != Current.TargetStableId.C)
	{
		return Candidate.TargetStableId.C < Current.TargetStableId.C;
	}
	return Candidate.TargetStableId.D < Current.TargetStableId.D;
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
	if (!Context.bDebugIdentityOverride
		&& (!Context.SubjectStableId.IsValid()
			|| Context.SubjectIdentityDomain == EAPSSubjectIdentityDomain::None))
	{
		OutFailure = APSInteractionFailures::MissingIdentity.ToString();
		return false;
	}
	const FVector ViewDirection = Context.ViewDirection.GetSafeNormal();
	if (ViewDirection.IsNearlyZero() || !FMath::IsFinite(Context.MinimumFocusDot)
		|| Context.MinimumFocusDot < -1.0 || Context.MinimumFocusDot > 1.0)
	{
		OutFailure = TEXT("APS.Interaction.InvalidFocusContext");
		return false;
	}

	const double DistanceCm = DistanceToActorBoundsCm(Candidate, Context.ViewOrigin);
	if (!FMath::IsFinite(DistanceCm) || Context.MaximumRangeCm <= 0.0
		|| DistanceCm > Context.MaximumRangeCm)
	{
		OutFailure = TEXT("APS.Interaction.OutOfRange");
		return false;
	}
	FVector BoundsOrigin;
	FVector BoundsExtent;
	Candidate->GetActorBounds(true, BoundsOrigin, BoundsExtent);
	const FVector ToCandidate = (BoundsOrigin - Context.ViewOrigin).GetSafeNormal();
	if (!ToCandidate.IsNearlyZero()
		&& FVector::DotProduct(ViewDirection, ToCandidate) < Context.MinimumFocusDot)
	{
		OutFailure = TEXT("APS.Interaction.OutOfFocus");
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
	if (!OutPrompt.ContextStableId.IsValid())
	{
		OutPrompt.ContextStableId = OutPrompt.TargetStableId;
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

bool UAPSInteractionSubsystem::ResolveFocus(
	const TArray<AActor*>& Candidates, const FAPSInteractionContext& Context,
	AActor*& OutFocusedActor, FAPSInteractionPromptDescriptor& OutPrompt,
	FString& OutFailure) const
{
	OutFocusedActor = nullptr;
	OutPrompt = FAPSInteractionPromptDescriptor{};
	OutFailure.Reset();
	if (Candidates.IsEmpty())
	{
		OutFailure = APSInteractionFailures::NoCandidates.ToString();
		return false;
	}

	int32 QueryableCount = 0;
	int32 OutOfRangeCount = 0;
	int32 OutOfFocusCount = 0;
	int32 InvalidIdentityCount = 0;
	int32 GatedCount = 0;
	struct FEvaluatedFocusCandidate
	{
		AActor* Actor{nullptr};
		FAPSInteractionPromptDescriptor Prompt;
		FAPSInteractionFocusScore Score;
	};
	TArray<FEvaluatedFocusCandidate> Evaluated;
	TMap<FGuid, int32> TargetCounts;
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate)
			|| !Candidate->GetClass()->ImplementsInterface(UAPSInteractable::StaticClass()))
		{
			continue;
		}
		++QueryableCount;
		FAPSInteractionPromptDescriptor Prompt;
		FString QueryFailure;
		if (!QueryActor(Candidate, Context, Prompt, QueryFailure))
		{
			if (QueryFailure == TEXT("APS.Interaction.OutOfRange")
				|| QueryFailure == TEXT("APS.Interaction.ActorRangeRejected"))
			{
				++OutOfRangeCount;
			}
			else if (QueryFailure == TEXT("APS.Interaction.OutOfFocus"))
			{
				++OutOfFocusCount;
			}
			continue;
		}
		// Debug-only prompts without canonical IDs never enter actor focus. The
		// global debug launcher remains a separate, visibly classified path.
		if (!Prompt.TargetStableId.IsValid() || !Prompt.ContextStableId.IsValid())
		{
			++InvalidIdentityCount;
			continue;
		}
		if (Prompt.Availability == EAPSInteractionAvailability::Hidden)
		{
			++GatedCount;
			continue;
		}
		FVector BoundsOrigin;
		FVector BoundsExtent;
		Candidate->GetActorBounds(true, BoundsOrigin, BoundsExtent);
		const FVector ToCandidate = (BoundsOrigin - Context.ViewOrigin).GetSafeNormal();
		const double FocusAlignment = ToCandidate.IsNearlyZero() ? 1.0
			: FVector::DotProduct(Context.ViewDirection.GetSafeNormal(), ToCandidate);
		FEvaluatedFocusCandidate& Entry = Evaluated.AddDefaulted_GetRef();
		Entry.Actor = Candidate;
		Entry.Score.Priority = Prompt.Priority;
		Entry.Score.FocusAlignment = FocusAlignment;
		Entry.Score.DistanceCm = Prompt.DistanceCm;
		Entry.Score.TargetStableId = Prompt.TargetStableId;
		Entry.Prompt = MoveTemp(Prompt);
		++TargetCounts.FindOrAdd(Entry.Score.TargetStableId);
	}

	FAPSInteractionFocusScore BestScore;
	int32 UniqueCanonicalCandidateCount = 0;
	for (FEvaluatedFocusCandidate& Entry : Evaluated)
	{
		const int32* Count = TargetCounts.Find(Entry.Score.TargetStableId);
		if (!Count || *Count != 1)
		{
			// Duplicate canonical identities are all ineligible. Selecting either
			// actor would turn pointer/name/input order into hidden authority.
			continue;
		}
		++UniqueCanonicalCandidateCount;
		if (IsFocusScorePreferred(Entry.Score, BestScore))
		{
			BestScore = Entry.Score;
			OutFocusedActor = Entry.Actor;
			OutPrompt = MoveTemp(Entry.Prompt);
		}
	}
	if (OutFocusedActor)
	{
		return true;
	}
	if (QueryableCount == 0)
	{
		OutFailure = APSInteractionFailures::AllCandidatesInvalid.ToString();
	}
	else if (OutOfRangeCount == QueryableCount)
	{
		OutFailure = APSInteractionFailures::AllCandidatesOutOfRange.ToString();
	}
	else if (OutOfFocusCount == QueryableCount)
	{
		OutFailure = APSInteractionFailures::AllCandidatesOutOfFocus.ToString();
	}
	else if (InvalidIdentityCount == QueryableCount
		|| (!Evaluated.IsEmpty() && UniqueCanonicalCandidateCount == 0))
	{
		OutFailure = APSInteractionFailures::AllCandidatesInvalid.ToString();
	}
	else if (GatedCount == QueryableCount)
	{
		OutFailure = APSInteractionFailures::AllCandidatesGated.ToString();
	}
	else
	{
		OutFailure = APSInteractionFailures::NoEligibleCandidate.ToString();
	}
	return false;
}

FAPSInteractionExecutionResult UAPSInteractionSubsystem::ExecuteActor(
	AActor* Candidate, const FAPSInteractionContext& Context,
	const FAPSInteractionExecutionRequest& Request)
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
	if (!Request.IsStructurallyValid(!Context.bDebugIdentityOverride, &ValidationReason))
	{
		Result.FailureCode = APSInteractionFailures::InvalidRequest;
		return Result;
	}
	if (Context.SubjectStableId != Request.SubjectStableId
		|| Context.SubjectIdentityDomain != Request.SubjectIdentityDomain
		|| Context.InstigatorActor != Request.InstigatorActor)
	{
		Result.FailureCode = APSInteractionFailures::SubjectMismatch;
		return Result;
	}
	FAPSInteractionPromptDescriptor CurrentPrompt;
	if (!QueryActor(Candidate, Context, CurrentPrompt, ValidationReason))
	{
		Result.FailureCode = FName(*ValidationReason);
		return Result;
	}
	if (CurrentPrompt.TargetStableId != Request.TargetStableId
		|| CurrentPrompt.TargetIdentityDomain != Request.TargetIdentityDomain)
	{
		Result.FailureCode = APSInteractionFailures::TargetMismatch;
		return Result;
	}
	if (CurrentPrompt.Revision != Request.ExpectedRevision)
	{
		Result.FailureCode = APSInteractionFailures::StaleRevision;
		return Result;
	}
	const FAPSInteractionActionDescriptor* Action = CurrentPrompt.Actions.FindByPredicate(
		[&Request](const FAPSInteractionActionDescriptor& CandidateAction)
		{
			return CandidateAction.ActionId == Request.ActionId;
		});
	if (!Action || !Action->bEnabled
		|| CurrentPrompt.Availability != EAPSInteractionAvailability::Available)
	{
		Result.FailureCode = APSInteractionFailures::ActionUnavailable;
		return Result;
	}
	Result = IAPSInteractable::Execute_ExecuteInteraction(Candidate, Request);
	if (!Result.CorrelationId.IsValid())
	{
		Result.CorrelationId = Request.CorrelationId;
	}
	if (Result.CorrelationId != Request.CorrelationId
		|| Result.Quantity != Request.Quantity || Result.Verb.IsNone())
	{
		Result.Status = EAPSInteractionExecutionStatus::Failed;
		Result.Verb = NAME_None;
		Result.ResultCode = NAME_None;
		Result.FailureCode = APSInteractionFailures::InvalidExecutionResult;
		Result.CorrelationId = Request.CorrelationId;
		Result.Quantity = Request.Quantity;
		return Result;
	}
	if (!PublishExecutionEvent(Context, Request, CurrentPrompt, Result,
		ValidationReason))
	{
		Result.Status = EAPSInteractionExecutionStatus::Failed;
		Result.Verb = NAME_None;
		Result.ResultCode = NAME_None;
		Result.FailureCode = APSInteractionFailures::InvalidExecutionResult;
	}
	return Result;
}

void UAPSInteractionSubsystem::EnsureExecutionStreamId()
{
	if (!ExecutionStreamId.IsValid())
	{
		ExecutionStreamId = FGuid::NewGuid();
	}
}

bool UAPSInteractionSubsystem::PublishExecutionEvent(
	const FAPSInteractionContext& Context,
	const FAPSInteractionExecutionRequest& Request,
	const FAPSInteractionPromptDescriptor& Prompt,
	const FAPSInteractionExecutionResult& Result, FString& OutFailure)
{
	EnsureExecutionStreamId();
	FAPSInteractionExecutionEvent Event;
	Event.StreamId = ExecutionStreamId;
	Event.EventId = FGuid::NewGuid();
	Event.CorrelationId = Result.CorrelationId;
	Event.Sequence = LastExecutionSequence + 1;
	Event.Verb = Result.Verb;
	Event.ActionId = Request.ActionId;
	Event.SubjectStableId = Request.SubjectStableId;
	Event.SubjectIdentityDomain = Request.SubjectIdentityDomain;
	Event.TargetStableId = Prompt.TargetStableId;
	Event.TargetIdentityDomain = Prompt.TargetIdentityDomain;
	Event.Quantity = Result.Quantity;
	Event.Status = Result.Status;
	Event.ResultCode = Result.ResultCode;
	Event.FailureCode = Result.FailureCode;
	Event.bDebugOnly = Context.bDebugIdentityOverride;
	if (!Event.IsStructurallyValid(&OutFailure))
	{
		return false;
	}

	LastExecutionSequence = Event.Sequence;
	ExecutionPublished.Broadcast(Event);
	OutFailure.Reset();
	return true;
}
