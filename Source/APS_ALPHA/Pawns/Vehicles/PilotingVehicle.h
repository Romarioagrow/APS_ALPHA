// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "PilotingVehicle.generated.h"

class AGravityCharacterPawn;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API APilotingVehicle : public AControlledPawn, public IVehicleControlling
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piloting")
	AGravityCharacterPawn* Pilot{};

	void TakeControl(APawn* Pawn);

	void ReleaseControl();
	//void TakeControl(APawn* Pawn) final;
};
