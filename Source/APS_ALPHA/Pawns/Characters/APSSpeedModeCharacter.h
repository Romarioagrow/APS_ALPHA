#pragma once

#include "CoreMinimal.h"
#include "CustomGravityCharacter.h"
#include "APSSpeedModeCharacter.generated.h"

class UAnimInstance;
class FProperty;

/** Keys 1-3 on foot. While piloting, the same keys select a ship's engine. */
UENUM(BlueprintType)
enum class EAPSSpeedMode : uint8
{
	/** 1: walk; Shift is a slow run. Plain jumps. */
	Walk,
	/** 2: about the stock pace, for interiors; Shift runs. Plain jumps. */
	Indoor,
	/** 3: open ground: a long sprint build-up and the double-tap boost jump. */
	Open
};

USTRUCT(BlueprintType)
struct FAPSSpeedModeSettings
{
	GENERATED_BODY()

	/** Pace without Shift, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0"))
	float WalkSpeed = 600.f;

	/** Shift pace when it is pressed, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0"))
	float SprintSpeed = 900.f;

	/** Shift pace after holding it long enough, cm/s. Equal to SprintSpeed for a steady run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0"))
	float SprintMaxSpeed = 1500.f;

	/** Growth of the Shift pace per second of holding, cm/s². */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "0.0"))
	float SprintGrowthRate = 120.f;

	/** How quickly the speed cap follows a change of pace, cm/s². */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0"))
	float SpeedChangeRate = 900.f;

	/** Ground acceleration, cm/s². */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0"))
	float MaxAcceleration = 2048.f;

	/** Ground braking without input, cm/s². */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "0.0"))
	float BrakingDeceleration = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "0.0"))
	float JumpZVelocity = 500.f;

	/** A second Space tap within the double-tap window boosts upwards while held (the base character's jump). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode")
	bool bBoostJumps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0", EditCondition = "bBoostJumps"))
	float BoostJumpVelocity = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "0.0", EditCondition = "bBoostJumps"))
	float BoostJumpAcceleration = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "1.0", EditCondition = "bBoostJumps"))
	float BoostJumpMaxUpSpeed = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Mode", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirControl = 0.35f;
};

/**
 * The production pilot with three speed modes (keys 1-3), a smoothed turn towards the movement direction,
 * smoothed locomotion values for the animation blueprint and camera lag that keeps up with fast runs.
 * Everything else — gravity, zero-G, vehicles, interaction, camera frame — is ACustomGravityCharacter's.
 */
UCLASS()
class APS_ALPHA_API AAPSSpeedModeCharacter : public ACustomGravityCharacter
{
	GENERATED_BODY()

public:
	AAPSSpeedModeCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed Mode")
	void SetSpeedMode(EAPSSpeedMode NewMode);

	UFUNCTION(BlueprintPure, Category = "Movement|Speed Mode")
	EAPSSpeedMode GetSpeedMode() const { return SpeedMode; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed Mode")
	EAPSSpeedMode DefaultSpeedMode = EAPSSpeedMode::Indoor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed Mode")
	FAPSSpeedModeSettings WalkMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed Mode")
	FAPSSpeedModeSettings IndoorMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed Mode")
	FAPSSpeedModeSettings OpenMode;

	/** Turn speed per degree still to turn, 1/s, at a walk: the turn eases out instead of stopping dead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "0.5"))
	float TurnResponse = 12.f;

	/** The same at FastTurnSpeed and above: a fast runner turns in wider, gentler arcs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "0.5"))
	float TurnResponseFast = 6.f;

	/** Turn rate cap at a walk, deg/s. The base character turns at a constant 500 deg/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "1.0"))
	float MaxTurnRate = 720.f;

	/** Turn rate cap at FastTurnSpeed and above, deg/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "1.0"))
	float MaxTurnRateFast = 360.f;

	/** Ground speed from which the fast turn values apply, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "1.0"))
	float FastTurnSpeed = 2000.f;

	/** How quickly the turn rate itself changes, deg/s²: a turn starts smoothly instead of at full rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Turning", meta = (ClampMin = "1.0"))
	float TurnRateAcceleration = 4000.f;

	/** Pace at which the run animation's steps match the ground (top sample of the walk/run blend space), cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "1.0"))
	float AnimationRunSpeed = 500.f;

	/** Faster paces play the run animation faster, up to this rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "1.0"))
	float MaxAnimationPlayRate = 2.2f;

	/** Smoothing of the speed and direction values the animation blueprint reads, 1/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "1.0"))
	float AnimationSmoothing = 10.f;

	/** The camera's lag distance limit grows with the pace so the trail stays at this share of it: past ~22 m/s the base
	 * rig's limit clamps the lag and kinks the camera on every turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "0.95"))
	float CameraLagShare = 0.75f;

protected:
	virtual void BeginPlay() override;
	virtual FText GetTraversalStatusText() const override;
	virtual FText GetTraversalHintText() const override;

private:
	const FAPSSpeedModeSettings& GetModeSettings() const;
	void ApplySpeedMode();
	void HandleModeJumpStarted();
	void SelectWalkMode();
	void SelectIndoorMode();
	void SelectOpenMode();
	void UpdateTurnRate(float DeltaTime);
	void UpdateCameraLag(float DeltaTime);
	void UpdateLocomotionAnimation(float DeltaTime);

	EAPSSpeedMode SpeedMode = EAPSSpeedMode::Indoor;
	float TurnRateDegrees = 0.f;
	float SmoothedGroundSpeed = 0.f;
	float SmoothedSpeed = 0.f;
	float SmoothedDirection = 0.f;
	float BaseCameraLagMaxDistance = 0.f;
	TWeakObjectPtr<UAnimInstance> AnimationInstance;
	TMap<FName, FProperty*> AnimationProperties;
};
