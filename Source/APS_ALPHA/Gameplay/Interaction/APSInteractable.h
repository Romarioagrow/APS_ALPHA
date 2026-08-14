#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "APSInteractionTypes.h"
#include "APSInteractable.generated.h"

UINTERFACE(BlueprintType)
class APS_ALPHA_API UAPSInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Actor-owned interaction policy. UI never calls this interface directly. */
class APS_ALPHA_API IAPSInteractable
{
	GENERATED_BODY()

public:
	/** Returns a canonical ID already owned by Civilization/Dev2/catalog state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="APS|Interaction")
	FGuid GetInteractionTargetStableId() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="APS|Interaction")
	bool QueryInteraction(const FAPSInteractionContext& Context,
		FAPSInteractionPromptDescriptor& OutPrompt) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="APS|Interaction")
	FAPSInteractionExecutionResult ExecuteInteraction(
		const FAPSInteractionExecutionRequest& Request);
};
