// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CustomGravityCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

/**
 * Custom Gravity Character using UE 5.4 SetGravityDirection().
 * Supports planet-like radial gravity, directional gravity zones,
 * and smooth camera alignment to arbitrary gravity directions.
 */
UCLASS()
class APS_ALPHA_API ACustomGravityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACustomGravityCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ──────────────────────── Components ────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// ──────────────────────── Enhanced Input ────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	// ──────────────────────── Gravity Settings ────────────────────────

	/** Enable custom gravity (planet-style radial gravity toward GravityTarget) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	bool bUseCustomGravity = true;

	/** Actor to use as gravity center (e.g. planet). If null, uses DefaultGravityDirection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	AActor* GravityTarget;

	/** Fallback gravity direction when no GravityTarget is set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	FVector DefaultGravityDirection = FVector(0.f, 0.f, -1.f);

	/** How fast the character rotation aligns to gravity direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float GravityAlignmentSpeed = 10.f;

	/** How fast the camera springarm aligns to gravity up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float CameraAlignmentSpeed = 8.f;

	// ──────────────────────── Camera Settings ────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraBoomLength = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

	// ──────────────────────── Functions ────────────────────────

	/** Set a new gravity target actor (planet, station, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravityTarget(AActor* NewTarget);

	/** Set a custom gravity direction directly */
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetCustomGravityDirection(const FVector& NewDirection);

	/** Get the current gravity direction (normalized) */
	UFUNCTION(BlueprintPure, Category = "Gravity")
	FVector GetCurrentGravityDirection() const;

	/** Get the "up" vector relative to current gravity */
	UFUNCTION(BlueprintPure, Category = "Gravity")
	FVector GetGravityUpVector() const;

protected:
	// Input handlers
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();

	// Gravity
	void UpdateGravityDirection();
	void AlignCameraToGravity(float DeltaTime);

private:
	/** Accumulated yaw/pitch for camera control */
	float CameraYaw = 0.f;
	float CameraPitch = 0.f;

	/** Current gravity direction (cached) */
	FVector CurrentGravityDir = FVector(0.f, 0.f, -1.f);
};
