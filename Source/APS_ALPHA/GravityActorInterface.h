// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GravityTypeEnum.h"
#include "GravityActor.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GravityActorInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGravityActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IGravityActorInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual void UpdateGravity();

	virtual void UpdateGravityStatus(AActor* OtherActor);

	virtual void UpdateGravityStatus();

	virtual FVector GetGravityDirection() const;

	virtual void SwitchGravityType(AGravityActor* GravityActorTarget);

protected:
	EGravityType CurrentGravityType {
		EGravityType::ZeroG
	};

	AGravityActor* GravityActorTarget;

	FVector GravityDirection {
		0.0f, 0.0f, 0.0f
	};

private:
	virtual void UpdateZeroGGravity();

	virtual void UpdateStationGravity();

	virtual void UpdatePlanetGravity();

	virtual void UpdateShipGravity();
};
