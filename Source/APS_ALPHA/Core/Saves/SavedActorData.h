#pragma once

#include "CoreMinimal.h"
#include "SavedActorData.generated.h"

USTRUCT(BlueprintType)
struct FActorSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FString ActorName;

	UPROPERTY()
	FTransform ActorTransform;

	/** Stable civilization identity; invalid for legacy/non-civilization actors. */
	UPROPERTY()
	FGuid StableEntityId;

	UPROPERTY()
	FString ActorClass;

	UPROPERTY()
	TArray<uint8> ActorData;

	UPROPERTY()
	FString ParentActorName;
};