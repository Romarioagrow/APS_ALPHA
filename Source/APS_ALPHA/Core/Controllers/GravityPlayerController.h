// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GravityPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AGravityPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

public:
	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	void SaveGame();

	/*UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	void LoadGame();*/
};
