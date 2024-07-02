// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseInterface.h"
#include "UObject/Interface.h"
#include "VehicleControlling.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UVehicleControlling : public UBaseInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class APS_ALPHA_API IVehicleControlling
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.

public:
	//virtual void TakeControl(APawn* Pawn);
};
