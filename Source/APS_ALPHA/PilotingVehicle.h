// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VehicleControlling.h"
#include "CoreMinimal.h"
#include "ControlledPawn.h"
#include "PilotingVehicle.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APilotingVehicle : public AControlledPawn, public IVehicleControlling
{
	GENERATED_BODY()
	
};
