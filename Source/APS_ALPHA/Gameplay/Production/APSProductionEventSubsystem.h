#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSProductionEvent.h"
#include "APSProductionEventSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAPSProductionEventPublished,
	const FAPSProductionEvent&);

/** Validates lifecycle transitions, assigns event IDs/sequence, and broadcasts once. */
UCLASS()
class APS_ALPHA_API UAPSProductionEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="APS|Production Event")
	bool PublishEvent(UPARAM(ref) FAPSProductionEvent& Event,
		const FAPSProductionEventPublishPolicy& Policy, FString& OutFailure);

	FOnAPSProductionEventPublished& OnEventPublished() { return EventPublished; }

	UFUNCTION(BlueprintPure, Category="APS|Production Event")
	int64 GetLastSequence() const { return LastSequence; }

	/** Save integration restores the greatest sequence already committed to this stream. */
	void RestoreLastSequence(int64 PersistedLastSequence);

private:
	struct FCorrelationState
	{
		EAPSProductionEventResult Result{EAPSProductionEventResult::Requested};
		FName Verb;
		FGuid SubjectStableId;
		FGuid TargetStableId;
		int32 Quantity{1};
	};

	bool ValidateTransition(const FAPSProductionEvent& Event, FString& OutFailure) const;

	FOnAPSProductionEventPublished EventPublished;
	TMap<FGuid, FCorrelationState> CorrelationStates;
	TSet<FGuid> PublishedEventIds;
	int64 LastSequence{0};
};
