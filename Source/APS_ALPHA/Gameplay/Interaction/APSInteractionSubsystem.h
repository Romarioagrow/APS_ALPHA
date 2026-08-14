#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSInteractionTypes.h"
#include "APSInteractionSubsystem.generated.h"

/** Tickless arbiter for range, authoritative-ID and stale-revision interaction gates. */
UCLASS()
class APS_ALPHA_API UAPSInteractionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="APS|Interaction")
	bool QueryActor(AActor* Candidate, const FAPSInteractionContext& Context,
		FAPSInteractionPromptDescriptor& OutPrompt, FString& OutFailure) const;

	UFUNCTION(BlueprintCallable, Category="APS|Interaction")
	FAPSInteractionExecutionResult ExecuteActor(AActor* Candidate,
		const FAPSInteractionExecutionRequest& Request,
		bool bDebugIdentityOverride = false) const;

	static double DistanceToActorBoundsCm(const AActor* Candidate, const FVector& Origin);
};
