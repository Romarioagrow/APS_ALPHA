// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ControlledPawn.h"
#include "GravityCharacterPawn.h"

#include "VehicleControlling.h"
#include "CoreMinimal.h"
#include "PilotingVehicle.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APilotingVehicle : public AControlledPawn, public IVehicleControlling
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piloting")
		AGravityCharacterPawn* Pilot;

	void TakeControl(APawn* Pawn);

	void ReleaseControl();
	//void TakeControl(APawn* Pawn) final;
};
