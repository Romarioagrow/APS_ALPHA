// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "GameFramework/GameModeBase.h"
#include "GravityGameModeBase.generated.h"

/**
 *
 */
UCLASS()
class APS_ALPHA_API AGravityGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGravityGameModeBase();

	virtual void BeginPlay() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AAstroGenerator> BP_AstroGeneratorClass;

private:
	/** Clears one-shot PrintString output left by the legacy level graph. */
	void ClearLegacyLevelScreenMessages();
};
