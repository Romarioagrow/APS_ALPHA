﻿#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSave.generated.h"

class APlanetaryBody;
class UGeneratedWorld;
struct FPlanetData;
struct FGeneratedWorldData;
struct FActorSaveData;

UCLASS()
class APS_ALPHA_API UGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	FString WorldName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = "State")
	TArray<FActorSaveData> ActorSaveDataArray;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<FGeneratedWorldData> GeneratedWorldsDataArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<FPlanetData> InhabitedPlanetsDataArray;
	
	UFUNCTION(BlueprintCallable)
	FGeneratedWorldData GetGeneratedWorld();
};