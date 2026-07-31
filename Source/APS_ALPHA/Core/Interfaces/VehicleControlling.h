// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleControlling.generated.h"

/**
 * Native vehicle-control contract. Blueprint vehicle classes inherit the shared
 * implementation from APilotingVehicle and can react through pilot lifecycle events.
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UVehicleControlling : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IVehicleControlling
{
	GENERATED_BODY()

public:
	/** Returns whether the pawn can start piloting this vehicle right now. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Interaction")
	virtual bool CanRequestVehicleControl(APawn* RequestingPawn) const = 0;

	/** Starts a vehicle-control session for the requesting pawn. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Interaction")
	virtual bool RequestVehicleControl(APawn* RequestingPawn) = 0;

	/** Ends the active vehicle-control session. */
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Interaction")
	virtual bool RequestReleaseVehicleControl() = 0;
};
