// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SpaceCharacter.generated.h"

UCLASS()
class APS_ALPHA_API ASpaceCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASpaceCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		class UStaticMeshComponent* StaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		class USpringArmComponent* CameraSpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float MovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		float RotationSpeed;

	UFUNCTION(BlueprintCallable, Category = "Rotation")
		void RotatePitch(float Value);

	UFUNCTION(BlueprintCallable, Category = "Rotation")
		void RotateYaw(float Value);

	UFUNCTION(BlueprintCallable, Category = "Rotation")
		void RotateRoll(float Value);

	void UpdateCameraOrientation();

protected:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);

	void Turn(float Value);
	void LookUp(float Value);
	void Roll(float Value);
};
