#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSInteractionTypes.h"
#include "APSInteractionSubsystem.generated.h"

/** Deterministic, presentation-neutral ordering key for one eligible focus candidate. */
struct APS_ALPHA_API FAPSInteractionFocusScore
{
	int32 Priority{0};
	double FocusAlignment{-1.0};
	double DistanceCm{TNumericLimits<double>::Max()};
	FGuid TargetStableId;

	bool IsStructurallyValid() const
	{
		return TargetStableId.IsValid() && FMath::IsFinite(FocusAlignment)
			&& FocusAlignment >= -1.0 && FocusAlignment <= 1.0
			&& FMath::IsFinite(DistanceCm) && DistanceCm >= 0.0;
	}
};

/** Tickless arbiter for range, authoritative-ID and stale-revision interaction gates. */
UCLASS()
class APS_ALPHA_API UAPSInteractionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="APS|Interaction")
	bool QueryActor(AActor* Candidate, const FAPSInteractionContext& Context,
		FAPSInteractionPromptDescriptor& OutPrompt, FString& OutFailure) const;

	/** Selects one canonical candidate without actor-name, pointer, or input-order fallback. */
	UFUNCTION(BlueprintCallable, Category="APS|Interaction")
	bool ResolveFocus(const TArray<AActor*>& Candidates,
		const FAPSInteractionContext& Context, AActor*& OutFocusedActor,
		FAPSInteractionPromptDescriptor& OutPrompt, FString& OutFailure) const;

	UFUNCTION(BlueprintCallable, Category="APS|Interaction")
	FAPSInteractionExecutionResult ExecuteActor(AActor* Candidate,
		const FAPSInteractionContext& Context,
		const FAPSInteractionExecutionRequest& Request) const;

	static double DistanceToActorBoundsCm(const AActor* Candidate, const FVector& Origin);
	static bool IsFocusScorePreferred(const FAPSInteractionFocusScore& Candidate,
		const FAPSInteractionFocusScore& Current);
};
