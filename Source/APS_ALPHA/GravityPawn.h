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

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	//virtual void UpdateGravity();

	//virtual void UpdateGravityStatus(AActor* OtherActor);

	//virtual FVector GetGravityDirection() const;

	virtual void UpdateGravity() = 0;

	virtual void UpdateGravityType() = 0;

	virtual void SwitchGravityType(AActor* GravitySourceActor) = 0;

protected:
	//EGravityType CurrentGravityType{
	//	EGravityType::ZeroG
	//};

	//TScriptInterface<IGravitySource> GravityTargetActor;
	////TScriptInterface<IGravityObject>

	//FVector GravityDirection{
	//	0.0f, 0.0f, 0.0f
	//};

	

private:
	/*virtual void UpdateZeroGGravity();

	virtual void UpdateStationGravity();

	virtual void UpdatePlanetGravity();

	virtual void UpdateShipGravity();*/
};
