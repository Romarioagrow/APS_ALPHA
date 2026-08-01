// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "PilotingVehicle.generated.h"

class AGravityCharacterPawn;
class AController;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API APilotingVehicle : public AControlledPawn, public IVehicleControlling
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Piloting")
	TObjectPtr<APawn> Pilot{};

	void TakeControl(APawn* Pawn);

	void ReleaseControl();

	virtual bool CanRequestVehicleControl(APawn* RequestingPawn) const override;
	virtual bool RequestVehicleControl(APawn* RequestingPawn) override;
	virtual bool RequestReleaseVehicleControl() override;

	UFUNCTION(BlueprintPure, Category = "Piloting")
	bool HasPilot() const { return IsValid(Pilot); }

	/** Functional default: no ship Blueprint needs a seated-character animation to be usable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Piloting")
	bool bHidePilotDuringControl{true};

	UFUNCTION(BlueprintImplementableEvent, Category = "Piloting")
	void OnPilotControlStarted(APawn* NewPilot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Piloting")
	void OnPilotControlEnded(APawn* PreviousPilot);

protected:
	virtual USceneComponent* GetPilotSeatComponent() const;
	virtual FTransform GetPilotExitTransform() const;

private:
	bool BeginVehicleControl(APawn* RequestingPawn);
	bool EndVehicleControl();

	UPROPERTY(Transient)
	TObjectPtr<AController> PilotController{};

	bool bPilotCollisionWasEnabled{true};
	bool bPilotTickWasEnabled{true};
	bool bPilotWasHiddenInGame{false};
	bool bPilotRootWasSimulatingPhysics{false};
	uint8 PilotMovementMode{0};
	TArray<TWeakObjectPtr<USkeletalMeshComponent>> PilotSkeletalComponents;
	TArray<bool> PilotSkeletalTickStates;
};
