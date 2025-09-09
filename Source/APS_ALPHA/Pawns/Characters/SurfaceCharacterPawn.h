#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/ArrowComponent.h"
#include "SurfaceCharacterPawn.generated.h"

UCLASS()
class APS_ALPHA_API ASurfaceCharacterPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ASurfaceCharacterPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Компоненты
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* PlayerCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* DirectionArrow;

	// Настройки движения
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float JumpVelocity = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxStepHeight = 50.0f;

	// Настройки поверхности
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float SurfaceDetectionDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float SurfaceOrientationSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	bool bEnableSurfaceMovement = true;

	// Входные функции
	UFUNCTION()
	void MoveForward(float Value);

	UFUNCTION()
	void MoveRight(float Value);

	UFUNCTION()
	void Turn(float Value);

	UFUNCTION()
	void LookUp(float Value);

	UFUNCTION()
	void StartJump();

	UFUNCTION()
	void StopJump();

	// Система движения по поверхностям
	void UpdateSurfaceOrientation();
	void ApplySurfaceGravity();
	FVector GetSurfaceNormal();
	bool IsOnSurface();

private:
	// Переменные для отслеживания поверхности
	FVector CurrentSurfaceNormal;
	bool bIsOnSurface;
	float SurfaceAngle;
};
