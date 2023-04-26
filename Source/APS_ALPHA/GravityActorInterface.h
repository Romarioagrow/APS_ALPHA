// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GravityActorInterface.generated.h"
#include "GravityTypeEnum.h"
#include "GravityActor.h"

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


	void UpdateGravity();

	void UpdateGravityStatus(AActor* OtherActor);

	void UpdateGravityStatus();

	FVector GetGravityDirection() const;

	void SwitchGravityType(AGravityActor* GravityTargetActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
		EGravityType CurrentGravityType {
		EGravityType::ZeroG
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
		AGravityActor* GravityTargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
		FVector GravityDirection {
		0.0f, 0.0f, 0.0f
	};

private:
	void UpdateZeroGGravity();

	void UpdateStationGravity();

	void UpdatePlanetGravity();

	void UpdateShipGravity();
};
