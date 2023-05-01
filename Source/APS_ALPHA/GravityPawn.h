// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GravitySource.h"
#include "GravityActorInterface.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GravityPawn.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGravityPawn : public UGravityActorInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IGravityPawn
{
	GENERATED_BODY()

public:
	virtual void UpdateGravity() = 0;

	virtual void UpdateGravityType() = 0;

	virtual void SwitchGravityType(AActor* GravitySourceActor) = 0;
};
