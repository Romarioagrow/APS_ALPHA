#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSave.generated.h"

class UGeneratedWorld;
struct FActorSaveData;

UCLASS()
class APS_ALPHA_API UGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = "State")
	TArray<FActorSaveData> ActorSaveDataArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	UGeneratedWorld* GeneratedWorld;

	UFUNCTION(BlueprintCallable)
	UGeneratedWorld* GetGeneratedWorld();

	UFUNCTION(BlueprintCallable)
	FString GetSaveSlotName();
	
};