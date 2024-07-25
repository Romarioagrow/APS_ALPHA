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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astro Object BP")
	TSubclassOf<AAstroGenerator> BP_AstroGeneratorClass;
};
