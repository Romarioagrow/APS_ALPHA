#pragma once

#include "CoreMinimal.h"
#include "SavedActorData.generated.h"

USTRUCT(BlueprintType)
struct FSavedActorData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "State")
	FString ActorClassName;

	UPROPERTY(VisibleAnywhere, Category = "State")
	FTransform Transform;

	// Add other properties you want to save here
};