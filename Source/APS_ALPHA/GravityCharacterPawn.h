// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include <GameFramework/SpringArmComponent.h>
#include "GravityTypeEnum.h"
#include "StationGravityActor.h"
#include "PlanetGravityActor.h"
#include "SpaceshipGravityActor.h"

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


protected:
	UFUNCTION()
		void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
			UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
			UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateGravityStatus();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
		EGravityType CurrentGravityType {
		EGravityType::ZeroG
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
		AGravityActor* GravityTargetActor;

	void SwitchGravityToStation(AActor* OtherActor);

	void SwitchGravityToPlanet(AActor* OtherActor);

	void SwitchGravityToSpaceship(AActor* OtherActor);

	void SwitchGravityToZeroG(AActor* OtherActor);

private:
	void UpdateZeroGGravity();

	void UpdateStationGravity();

	void UpdatePlanetGravity();

	void UpdateShipGravity();
};
