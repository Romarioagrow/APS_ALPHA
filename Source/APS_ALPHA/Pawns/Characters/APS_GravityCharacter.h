// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "APS_GravityCharacter.generated.h"

UCLASS()
class APS_ALPHA_API AAPS_GravityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAPS_GravityCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	void EnableZeroG(ACharacter* Character);

	UFUNCTION(BlueprintCallable)
	void DisableZeroG(ACharacter* Character, float InGravityScale);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector GravityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsZeroG;

	UFUNCTION()
	void MoveUp(float Value);

	UFUNCTION()
	void RotatePitch(float Value);
	
	UFUNCTION()
	void RotateRoll(float Value);
	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION()
	void RotateYaw(float Value);

private:
	double CameraYawScale{1.0};
	
	double CameraPitchScale{1.0};
	
	double CharacterRotationScale{1.25};
	
	double CharacterMovementForce{25};
	
	double CharacterJumpForce{25.0};
	
};
