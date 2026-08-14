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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="APS|Production Event")
	bool PublishEvent(UPARAM(ref) FAPSProductionEvent& Event,
		const FAPSProductionEventPublishPolicy& Policy, FString& OutFailure);

	FOnAPSProductionEventPublished& OnEventPublished() { return EventPublished; }

	UFUNCTION(BlueprintPure, Category="APS|Production Event")
	int64 GetLastSequence() const { return LastSequence; }

	UFUNCTION(BlueprintPure, Category="APS|Production Event")
	FGuid GetStreamId() const { return StreamId; }

	/** Save integration restores the greatest sequence already committed to this stream. */
	void RestoreLastSequence(int64 PersistedLastSequence);

	void ExportStreamState(FAPSProductionEventStreamState& OutState) const;
	bool RestoreStreamState(const FAPSProductionEventStreamState& State,
		FString& OutFailure);

	/** Atomically replaces a validated persisted stream during a staged world load. */
	bool ReplaceStreamStateForLoad(const FAPSProductionEventStreamState& State,
		FString& OutFailure);

private:
	struct FCorrelationState
	{
		EAPSProductionEventResult Result{EAPSProductionEventResult::Requested};
		FName Verb;
		FGuid SubjectStableId;
		EAPSSubjectIdentityDomain SubjectIdentityDomain{EAPSSubjectIdentityDomain::GameplayEntity};
		FGuid TargetStableId;
		FPrimaryAssetId DefinitionId;
		int32 DefinitionSchemaVersion{1};
		int32 Quantity{1};
		bool bDebugOnly{false};
	};

	bool ValidateTransition(const FAPSProductionEvent& Event, bool bDebugOnly,
		FString& OutFailure) const;

	bool ReplaceValidatedStreamState(const FAPSProductionEventStreamState& State,
		FString& OutFailure);

	void EnsureStreamId() const;
	FOnAPSProductionEventPublished EventPublished;
	TMap<FGuid, FCorrelationState> CorrelationStates;
	TSet<FGuid> PublishedEventIds;
	mutable FGuid StreamId;
	int64 LastSequence{0};
};
