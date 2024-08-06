// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuController.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AMainMenuController : public APlayerController
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void LoadGame();
	void OnLevelLoaded(UWorld* LoadedWorld);
};
