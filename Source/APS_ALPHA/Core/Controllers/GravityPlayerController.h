// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
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
	
	FString GetCurrentSaveSlotName() const;

public:
	FString CurrentSaveSlotName;

	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	void SaveNewWorld(const EAstroGenerationLevel AstroGenerationLevel, UGeneratedWorld* GeneratedWorldModel);

	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	void LoadWorld();
	
	FString GenerateUniqueSaveSlotName(EAstroGenerationLevel AstroGenerationLevel) const;

	UFUNCTION(BlueprintPure, Category = "SaveLoad")
	bool GetLoadingMode();

	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	void SetLoadingModeFalse();

	static FName GenerateUniqueName(const FString& ObjectType);
};
