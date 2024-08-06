#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSave.generated.h"

UCLASS()
class APS_ALPHA_API UGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	UGameSave();

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = "State")
	TArray<FTransform> ActorTransforms;

	UPROPERTY(VisibleAnywhere, Category = "State")
	TArray<FString> ActorNames;
};