// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomGravityPlayerController.h"

#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

ACustomGravityPlayerController::ACustomGravityPlayerController()
{
	// Allow the controller to tick
	PrimaryActorTick.bCanEverTick = true;
}

void ACustomGravityPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ACustomGravityPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	GravityCharacter = Cast<ACustomGravityCharacter>(InPawn);
	if (GravityCharacter.IsValid())
	{
		PreviousGravityUp = GravityCharacter->GetGravityUpVector();
	}
}

void ACustomGravityPlayerController::UpdateRotation(float DeltaTime)
{
	if (!GravityCharacter.IsValid())
	{
		Super::UpdateRotation(DeltaTime);
		return;
	}

	const FVector GravityUp = GravityCharacter->GetGravityUpVector();

	// Detect gravity direction change and re-orient control rotation
	if (!GravityUp.Equals(PreviousGravityUp, 0.001f))
	{
		// Calculate the rotation delta from old gravity up to new gravity up
		const FQuat DeltaQuat = FQuat::FindBetweenNormals(PreviousGravityUp, GravityUp);

		// Rotate the current control rotation by this delta
		FRotator CurrentControlRot = GetControlRotation();
		FQuat ControlQuat = CurrentControlRot.Quaternion();
		ControlQuat = DeltaQuat * ControlQuat;
		SetControlRotation(ControlQuat.Rotator());

		PreviousGravityUp = GravityUp;
	}

	// Process standard mouse/gamepad input
	Super::UpdateRotation(DeltaTime);
}

FRotator ACustomGravityPlayerController::GetDesiredRotation() const
{
	if (GravityCharacter.IsValid())
	{
		return GravityCharacter->GetActorRotation();
	}
	return Super::GetDesiredRotation();
}
