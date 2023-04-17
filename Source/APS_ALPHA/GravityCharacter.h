// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GravityTypeEnum.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GravityCharacter.generated.h"

UCLASS()
class APS_ALPHA_API AGravityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGravityCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

/**
 * @Gravity Params
*/
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
		EGravityType CurrentGravityType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
		FVector GravityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
		float GravityStrength;

public:
	UFUNCTION(BlueprintCallable, Category = "Gravity")
		void SetGravityType(EGravityType NewGravityType);

protected:
	UFUNCTION(BlueprintCallable, Category = "Gravity")
		void UpdateGravity();

	UFUNCTION(BlueprintCallable, Category = "GravityCamera")
		void UpdateCameraOrientation();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		USpringArmComponent* CameraSpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		UCameraComponent* PlayerCamera;

private:
	void UpdateZeroGGravity();
	void UpdateZeroGCamera();

	void UpdateStationGravity();
	void UpdateStationCamera();

	void UpdatePlanetGravity();
	void UpdatePlanetCamera();

	void UpdateShipGravity();
	void UpdateShipCamera();
};
