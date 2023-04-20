// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include <GameFramework/SpringArmComponent.h>

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GravityCharacterPawn.generated.h"

UCLASS()
class APS_ALPHA_API AGravityCharacterPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGravityCharacterPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


/**
 * @brief Character Basic Components
*/
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UCapsuleComponent* CapsuleComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UArrowComponent* ArrowForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USkeletalMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UCameraComponent* PlayerCamera;

/**
 * @brief General Movements, Rotations and Camera Control
*/
public:
	UFUNCTION()
		void Turn(float Value);
	UFUNCTION()
		void LookUp(float Value);

	UFUNCTION()
		void MoveForward(float Value);
	UFUNCTION()
		void MoveRight(float Value);
	UFUNCTION()
		void MoveUp(float Value);

	UFUNCTION()
		void RotatePitch(float Value);
	UFUNCTION()
		void RotateRoll(float Value);
	UFUNCTION()
		void RotateYaw(float Value);

private:
	float CameraYawScale{ 1.0f };	
	float CameraPitchScale{ 1.0f };	
	float CharacterRotationScale{ 1.25f };	
	float CharacterMovementScale{ 25.f };	

};
