// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CustomGravityPlayerController.generated.h"

class ACustomGravityCharacter;

/**
 * Player Controller designed for custom gravity characters (UE 5.4+).
 * Handles control rotation relative to the character's current gravity direction,
 * preventing gimbal lock and camera inversion at extreme gravity orientations.
 */
UCLASS()
class APS_ALPHA_API ACustomGravityPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACustomGravityPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void UpdateRotation(float DeltaTime) override;

public:
	/**
	 * Override to transform control rotation into gravity-relative space.
	 * This ensures AddMovementInput works correctly with custom gravity.
	 */
	virtual FRotator GetDesiredRotation() const override;

	/** Speed at which the controller rotation aligns to new gravity direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float ControlRotationAlignSpeed = 10.f;

private:
	/** Cached reference to the gravity character */
	UPROPERTY()
	TWeakObjectPtr<ACustomGravityCharacter> GravityCharacter;

	/** Previous gravity up vector for detecting gravity changes */
	FVector PreviousGravityUp = FVector::UpVector;
};
