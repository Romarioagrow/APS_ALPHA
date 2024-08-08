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
	
public:
	UFUNCTION(BlueprintCallable, Category = "Saves")
	void SetSaveSlotName(FString OutSaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Saves")
	void SetLoadingModeTrue();
};
