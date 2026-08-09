// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CustomGravityCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UGravityDetectorComponent;
class UAnimInstance;
class SWidget;
class FProperty;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ──────────────────────── Components ────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
	UGravityDetectorComponent* GravityDetector;

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

	/** Falling outside every configured gravity field becomes true zero-G. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	bool bUseZeroGWhenNoSource = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool bIsZeroG = false;

	/** Player override: ignore all gravity fields until G is pressed again. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|ZeroG")
	bool bManualZeroGOverride = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
	EGravityType CurrentGravityType = EGravityType::ZeroG;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG")
	float ZeroGMaxSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG")
	float ZeroGAcceleration = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG")
	float ZeroGBrakingDeceleration = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG", meta = (ClampMin = "1.0"))
	float ZeroGSprintSpeed = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG", meta = (ClampMin = "1.0"))
	float ZeroGSprintMaxSpeed = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG", meta = (ClampMin = "0.0"))
	float ZeroGSprintGrowthRate = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|ZeroG", meta = (ClampMin = "1.0"))
	float ZeroGRollSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0"))
	float SurfaceWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0"))
	float SurfaceSprintSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0"))
	float SurfaceSprintMaxSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "0.0"))
	float SurfaceSprintGrowthRate = 120.f;

	/** How quickly the speed cap moves between normal and sprint values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (ClampMin = "1.0"))
	float SprintSpeedChangeRate = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (ClampMin = "0.1", ClampMax = "0.6"))
	float DoubleTapJumpWindow = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (ClampMin = "1.0"))
	float DoubleTapJumpVelocity = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (ClampMin = "0.0"))
	float BoostJumpAcceleration = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (ClampMin = "1.0"))
	float BoostJumpMaxUpSpeed = 1800.f;

	/** Maximum distance searched along local gravity for a deck. If no deck is
	 * directly below the character, station/ship movement remains true zero-G. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|Surface", meta = (ClampMin = "100.0"))
	float SurfaceGravityAcquisitionDistance = 100000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|Surface", meta = (ClampMin = "1.0"))
	float SurfaceGravityProbeRadius = 24.f;

	/** Prevents a one-frame floor-probe miss from repeatedly toggling Flying/Falling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|Surface", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SurfaceSupportLossGracePeriod = 0.2f;

	/** Duration of the visible orientation blend when gravity mode/direction changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|Transition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GravityTransitionDuration = 0.8f;

	/** Time used to ramp gravity strength from zero to full after entering a field. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity|Transition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GravityCaptureBlendDuration = 0.45f;

	// ──────────────────────── Camera Settings ────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraBoomLength = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionDistance = 600.f;

	// ──────────────────────── Functions ────────────────────────

	/** Set a new gravity target actor (planet, station, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravityTarget(AActor* NewTarget);

	/**
	 * Holds the character motionless while WorldScape publishes the render and
	 * collision patches for a generated surface start.  Gravity/input are resumed
	 * only after the handoff has validated that both patches describe the same
	 * physical height field.
	 */
	void SetSurfaceHandoffSuspended(bool bSuspended);
	bool IsSurfaceHandoffSuspended() const { return bSurfaceHandoffSuspended; }

	/** Set a custom gravity direction directly */
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetCustomGravityDirection(const FVector& NewDirection);

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetZeroGravityEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Gravity|ZeroG")
	void SetManualZeroGOverride(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Gravity|ZeroG")
	void ToggleManualZeroGOverride();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

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
	void HandleZeroGVertical(float Value);
	void HandleZeroGRoll(float Value);
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void UpdateMovementSpeed(float DeltaTime);
	void UpdateBoostJump(float DeltaTime);
	void UpdateInteractionCandidate();
	AActor* FindInteractionCandidate();
	static AActor* ResolveVehicleActor(AActor* Candidate);
	void CreateInteractionPrompt();
	void RemoveInteractionPrompt();
	void CreateTraversalHud();
	void RemoveTraversalHud();
	FText GetTraversalStatusText() const;
	FText GetTraversalHintText() const;

	// Gravity
	void UpdateGravityDirection(float DeltaTime);
	bool HasSurfaceGravitySupport(const FVector& GravityDirection);
	void UpdateCameraReferenceFrame();
	void AlignCameraToGravity(float DeltaTime);
	void NormalizeThirdPersonCameraRig();
	void SynchronizeCharacterToCamera(float DeltaTime);
	FVector GetCameraPlanarForward() const;
	FQuat GetCameraViewRotation() const;
	void StartGravityDirectionTransition(const FVector& TargetGravityDirection, bool bStartFromActorUp);
	void AdvanceGravityDirectionTransition(float DeltaTime);
	void AdvanceGravityStrengthTransition(float DeltaTime);
	void ApplyAnimationMode();
	void UpdateGravityAnimationParameters();

	UFUNCTION()
	void HandleGravitySourceChanged(AActor* NewSource);

private:
	/** Stable camera heading on the current gravity plane. It must not be rebuilt
	 * from ActorForward every frame because movement also rotates the actor. */
	FVector CameraForwardOnGravityPlane = FVector::ForwardVector;
	FVector CameraReferenceUp = FVector::UpVector;

	/** Accumulated pitch for camera control. */
	float CameraPitch = 0.f;
	FQuat ZeroGViewRotation = FQuat::Identity;
	bool bZeroGViewRotationInitialized = false;

	float LastJumpPressTime = -1.f;
	float SprintHoldDuration = 0.f;
	bool bBoostJumpHeld = false;
	bool bSprintHeld = false;

	/** Current gravity direction (cached) */
	FVector CurrentGravityDir = FVector(0.f, 0.f, -1.f);
	FVector DesiredGravityDir = FVector(0.f, 0.f, -1.f);
	FVector GravityTransitionStartDir = FVector(0.f, 0.f, -1.f);
	FVector GravityTransitionTargetDir = FVector(0.f, 0.f, -1.f);
	float GravityTransitionElapsed = 0.f;
	float ZeroGOrientationTransitionRemaining = 0.f;
	float GravityCaptureBlendElapsed = 0.f;
	float SurfaceSupportLossElapsed = 0.f;
	bool bGravityCaptureBlendActive = false;
	bool bGravityDirectionInitialized = false;

	bool bManualGravityOverride = false;
	bool bSurfaceHandoffSuspended = false;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> SurfaceAnimationClass;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> ZeroGAnimationClass;

	TWeakObjectPtr<UAnimInstance> CachedAnimationInstance;
	TMap<FName, FProperty*> AnimationPropertyCache;

	TWeakObjectPtr<AActor> CurrentInteractableActor;
	TSharedPtr<SWidget> InteractionPromptWidget;
	TSharedPtr<SWidget> TraversalHudWidget;
};
