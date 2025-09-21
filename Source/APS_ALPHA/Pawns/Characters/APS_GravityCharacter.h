// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "GameFramework/Character.h"
#include "APS_GravityCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASpaceship;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClosestGravityBody, AActor*, NewBody);

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
	void EnableZeroG();

	UFUNCTION(BlueprintCallable)
	void DisableZeroG(float InGravityScale);


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector GravityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsZeroG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsManualZeroG;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float HeightAboveGround;

	UFUNCTION()
	void MoveUp(float Value);

	UFUNCTION()
	void RotatePitch(float Value);
	
	UFUNCTION()
	void RotateRoll(float Value);
	
	UFUNCTION()
	void MoveForward(float Value);

	UFUNCTION()
	void MoveRight(float Value);

	UFUNCTION()
	void RotateYaw(float Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* PlayerCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	AActor* GravityTargetActor;

	/*UPROPERTY(BlueprintAssignable, Category="Gravity|Events")
	FOnGravityPhysicsParamChanged OnGravityPhysicsParamChanged;*/
	
	UPROPERTY(BlueprintAssignable, Category="Gravity|Events")
	FOnClosestGravityBody OnClosestGravityBodyChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	EGravityType CurrentGravityType{
		EGravityType::ZeroG
	};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	ASpaceship* CurrentSpaceship;

	UFUNCTION(BlueprintCallable, Category="Gravity")
	void RunGravityCheck(ACharacter* Character);

	void SwitchGravityType(AActor* GravitySourceActor);

private:
	double CameraYawScale{1.0};
	
	double CameraPitchScale{1.0};
	
	double CharacterRotationScale{1.25};
	
	double CharacterMovementForce{25};
	
	double CharacterJumpForce{25.0};

	void UpdateGravityState();
};
