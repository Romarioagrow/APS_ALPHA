// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
/*#include "AnimationState.h"
#include "GravityState.h"
#include "GravitySource.h"
#include "GravityObject.h"*/
#include "GravityPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
//#include "GravityTypeEnum.h"
//#include "PilotingVehicle.h"
#include <GameFramework/SpringArmComponent.h>

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Animations/AnimationState.h"
#include "APS_ALPHA/Core/Enums/GravityState.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "GameFramework/Pawn.h"
#include "GravityCharacterPawn.generated.h"

enum class EAnimationState : uint8;
enum class EGravityState : uint8;
enum class EGravityType : uint8;
class ASpaceship;
class APilotingVehicle;

UCLASS()
class APS_ALPHA_API AGravityCharacterPawn : public AControlledPawn, public IGravityPawn
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

	void CharacterAction();

	void ReleaseControl(APilotingVehicle* PilotingVehicle);

	/**
	 * @brief Character Basic Components
	*/
public:
	bool isAllowedToControlSpaceship{true}; /// false!

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ASpaceship* CurrentSpaceship;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* PlayerCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	float RotationInterpolationSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraInterpolationSpeed = 5.0f;

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
	void MoveForwardOnStation(const float Value);
	void MoveForwardOnPlanet(const float Value);
	void MoveForwardOnShip(const float Value);
	void MoveForwardZeroG(const float Value);
	void MoveRightOnStation(const float Value);
	void MoveRightOnPlanet(const float Value);
	void MoveRightOnShip(const float Value);
	void MoveRightZeroG(const float Value);
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

	UFUNCTION()
	void AlignCharacterToCameraZeroG();

	void AlignCharacterToCameraOnStation();

private:
	double CameraYawScale{1.0};
	double CameraPitchScale{1.0};
	double CharacterRotationScale{1.25};
	double CharacterMovementForce{25};
	double CharacterJumpForce{25.0};

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                    const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateGravityType() override;

	void SwitchGravityType(AActor* GravitySourceActor) override;

	void UpdateGravity() override;

	void UpdateGravityPhysicParams();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EGravityType CurrentGravityType{
		EGravityType::ZeroG
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EGravityState CurrentGravityState{
		EGravityState::LowG
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EAnimationState CurrentAnimationState{
		EAnimationState::Floating
	};

	FVector GravityDirection{
		0.0f, 0.0f, 0.0f
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	AActor* GravityTargetActor;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float ForwardSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float RightSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float UpSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float HeightAboveGround;

	void SwitchGravityToZeroG(AActor* OtherActor);

	void UpdateAnimationState();

	void UpdateGravityState();

	// Внутри определения класса:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* ArrowComponent;

private:
	void UpdateZeroGGravity();

	void UpdateStationRotation();

	void UpdateStationGravity();

	void UpdatePlanetGravity();

	void UpdateShipGravity();
};
