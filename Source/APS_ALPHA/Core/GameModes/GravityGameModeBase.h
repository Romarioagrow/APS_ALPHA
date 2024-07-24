// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
};
