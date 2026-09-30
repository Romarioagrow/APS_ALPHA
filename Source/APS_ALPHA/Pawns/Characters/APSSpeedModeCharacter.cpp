#include "APSSpeedModeCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "UObject/UnrealType.h"

AAPSSpeedModeCharacter::AAPSSpeedModeCharacter()
{
	// 1: a walk, Shift is a slow run.
	WalkMode.WalkSpeed = 170.f;
	WalkMode.SprintSpeed = 380.f;
	WalkMode.SprintMaxSpeed = 380.f;
	WalkMode.SprintGrowthRate = 0.f;
	WalkMode.SpeedChangeRate = 700.f;
	WalkMode.MaxAcceleration = 1200.f;
	WalkMode.BrakingDeceleration = 1400.f;
	WalkMode.JumpZVelocity = 420.f;
	WalkMode.bBoostJumps = false;
	WalkMode.AirControl = 0.25f;

	// 2: about the stock pace for corridors and decks, Shift runs.
	IndoorMode.WalkSpeed = 500.f;
	IndoorMode.SprintSpeed = 800.f;
	IndoorMode.SprintMaxSpeed = 800.f;
	IndoorMode.SprintGrowthRate = 0.f;
	IndoorMode.SpeedChangeRate = 900.f;
	IndoorMode.MaxAcceleration = 2048.f;
	IndoorMode.BrakingDeceleration = 2000.f;
	IndoorMode.JumpZVelocity = 500.f;
	IndoorMode.bBoostJumps = false;
	IndoorMode.AirControl = 0.35f;

	// 3: the base character's pace and sprint build-up with a far higher ceiling (1500 -> 4000 cm/s),
	// stronger acceleration and boost jumps.
	OpenMode.WalkSpeed = 600.f;
	OpenMode.SprintSpeed = 900.f;
	OpenMode.SprintMaxSpeed = 4000.f;
	OpenMode.SprintGrowthRate = 450.f;
	OpenMode.SpeedChangeRate = 2400.f;
	OpenMode.MaxAcceleration = 4096.f;
	OpenMode.BrakingDeceleration = 2500.f;
	OpenMode.JumpZVelocity = 650.f;
	OpenMode.bBoostJumps = true;
	OpenMode.BoostJumpVelocity = 1100.f;
	OpenMode.BoostJumpAcceleration = 1800.f;
	OpenMode.BoostJumpMaxUpSpeed = 2600.f;
	OpenMode.AirControl = 0.5f;
}

void AAPSSpeedModeCharacter::BeginPlay()
{
	Super::BeginPlay();
	// NormalizeThirdPersonCameraRig (in Super::BeginPlay) has set the rig's safety limit for teleports.
	BaseCameraLagMaxDistance = CameraBoom ? CameraBoom->CameraLagMaxDistance : 0.f;
	SpeedMode = DefaultSpeedMode;
	ApplySpeedMode();
	UE_LOG(LogTemp, Log, TEXT("[APS.SpeedMode] character=%s mode=%d"), *GetName(), static_cast<int32>(SpeedMode) + 1);
}

void AAPSSpeedModeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Modes 1-2 jump plainly; the base's double-tap boost jump is reserved for mode 3 (see HandleModeJumpStarted).
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent); EnhancedInput && JumpAction)
	{
		TArray<uint32> BaseJumpHandles;
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : EnhancedInput->GetActionEventBindings())
		{
			if (Binding && Binding->GetAction() == JumpAction && Binding->GetTriggerEvent() == ETriggerEvent::Started)
			{
				BaseJumpHandles.Add(Binding->GetHandle());
			}
		}
		for (const uint32 Handle : BaseJumpHandles)
		{
			EnhancedInput->RemoveBindingByHandle(Handle);
		}
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AAPSSpeedModeCharacter::HandleModeJumpStarted);
	}

	// Free on foot: ships bind 1-3 only while piloted.
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AAPSSpeedModeCharacter::SelectWalkMode);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AAPSSpeedModeCharacter::SelectIndoorMode);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AAPSSpeedModeCharacter::SelectOpenMode);
}

void AAPSSpeedModeCharacter::Tick(float DeltaTime)
{
	// The base re-applies its own acceleration on every gravity/zero-G transition; the mode wins again before
	// the base reads the pace this frame.
	ApplySpeedMode();
	Super::Tick(DeltaTime);
	if (IsSurfaceHandoffSuspended() || DeltaTime <= 0.f)
	{
		return;
	}
	UpdateTurnRate(DeltaTime);
	UpdateCameraLag(DeltaTime);
	UpdateLocomotionAnimation(DeltaTime);
}

void AAPSSpeedModeCharacter::SetSpeedMode(EAPSSpeedMode NewMode)
{
	if (SpeedMode == NewMode)
	{
		return;
	}
	SpeedMode = NewMode;
	ApplySpeedMode();
	UE_LOG(LogTemp, Log, TEXT("[APS.SpeedMode] character=%s mode=%d walk=%.0f sprint=%.0f..%.0f cm/s"), *GetName(),
		static_cast<int32>(SpeedMode) + 1, SurfaceWalkSpeed, SurfaceSprintSpeed, SurfaceSprintMaxSpeed);
}

void AAPSSpeedModeCharacter::SelectWalkMode()
{
	SetSpeedMode(EAPSSpeedMode::Walk);
}

void AAPSSpeedModeCharacter::SelectIndoorMode()
{
	SetSpeedMode(EAPSSpeedMode::Indoor);
}

void AAPSSpeedModeCharacter::SelectOpenMode()
{
	SetSpeedMode(EAPSSpeedMode::Open);
}

const FAPSSpeedModeSettings& AAPSSpeedModeCharacter::GetModeSettings() const
{
	switch (SpeedMode)
	{
	case EAPSSpeedMode::Walk: return WalkMode;
	case EAPSSpeedMode::Open: return OpenMode;
	case EAPSSpeedMode::Indoor:
	default: return IndoorMode;
	}
}

void AAPSSpeedModeCharacter::ApplySpeedMode()
{
	// The base character's own sprint and jump logic runs on these values.
	const FAPSSpeedModeSettings& Mode = GetModeSettings();
	SurfaceWalkSpeed = Mode.WalkSpeed;
	SurfaceSprintSpeed = FMath::Max(Mode.SprintSpeed, Mode.WalkSpeed);
	SurfaceSprintMaxSpeed = FMath::Max(Mode.SprintMaxSpeed, SurfaceSprintSpeed);
	SurfaceSprintGrowthRate = Mode.SprintGrowthRate;
	SprintSpeedChangeRate = Mode.SpeedChangeRate;
	DoubleTapJumpVelocity = Mode.BoostJumpVelocity;
	BoostJumpAcceleration = Mode.BoostJumpAcceleration;
	BoostJumpMaxUpSpeed = Mode.BoostJumpMaxUpSpeed;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->JumpZVelocity = Mode.JumpZVelocity;
		Movement->BrakingDecelerationWalking = Mode.BrakingDeceleration;
		Movement->AirControl = Mode.AirControl;
		if (!bIsZeroG)
		{
			// Zero-G keeps the base character's flight acceleration.
			Movement->MaxAcceleration = Mode.MaxAcceleration;
		}
	}
}

void AAPSSpeedModeCharacter::HandleModeJumpStarted()
{
	// Zero-G thrust and the boost jump are the base character's; modes without boost jumps jump plainly.
	if (bIsZeroG || GetModeSettings().bBoostJumps)
	{
		HandleJumpStarted();
		return;
	}
	Jump();
}

void AAPSSpeedModeCharacter::UpdateTurnRate(float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || bIsZeroG || !Movement->bOrientRotationToMovement)
	{
		TurnRateDegrees = 0.f;
		return;
	}
	// The movement component turns towards the input at RotationRate.Yaw per second (a constant 500 in the base
	// character: full rate at once and a dead stop). Here the rate follows the remaining angle, so a turn eases
	// out, and the rate itself changes gradually, so it eases in. The component applies it on the next step.
	const FVector Up = GetGravityUpVector();
	const FVector Desired = FVector::VectorPlaneProject(Movement->GetCurrentAcceleration(), Up).GetSafeNormal();
	float TargetRate = 0.f;
	if (!Desired.IsNearlyZero())
	{
		const FVector Facing = FVector::VectorPlaneProject(GetActorForwardVector(), Up).GetSafeNormal();
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(
			FMath::Clamp(static_cast<float>(FVector::DotProduct(Facing, Desired)), -1.f, 1.f)));
		const float GroundSpeed = FVector::VectorPlaneProject(GetVelocity(), Up).Size();
		const float FastAlpha = FMath::Clamp(GroundSpeed / FMath::Max(FastTurnSpeed, 1.f), 0.f, 1.f);
		const float Response = FMath::Lerp(TurnResponse, TurnResponseFast, FastAlpha);
		const float RateCap = FMath::Lerp(MaxTurnRate, MaxTurnRateFast, FastAlpha);
		TargetRate = FMath::Min(AngleDegrees * Response, RateCap);
	}
	TurnRateDegrees = FMath::FInterpConstantTo(TurnRateDegrees, TargetRate, DeltaTime, TurnRateAcceleration);
	// A negative rate means "instant" to the movement component; keep a small positive floor.
	Movement->RotationRate = FRotator(0.f, FMath::Max(TurnRateDegrees, 1.f), 0.f);
}

void AAPSSpeedModeCharacter::UpdateCameraLag(float DeltaTime)
{
	if (!CameraBoom || !CameraBoom->bEnableCameraLag || BaseCameraLagMaxDistance <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Spring-arm lag trails a steadily moving target by Speed / CameraLagSpeed. Past about 22 m/s that reaches the
	// rig's CameraLagMaxDistance and the clamp kinks the camera on every turn. Keep the soft lag and let the limit
	// grow with the pace instead: at once when speeding up, gently when slowing (the trail shrinks first). At rest
	// the rig's own limit still bounds teleports.
	const float TargetLimit = FMath::Max(BaseCameraLagMaxDistance,
		GetVelocity().Size() / FMath::Max(CameraBoom->CameraLagSpeed, 1.f) / CameraLagShare);
	CameraBoom->CameraLagMaxDistance = TargetLimit >= CameraBoom->CameraLagMaxDistance
		? TargetLimit : FMath::FInterpTo(CameraBoom->CameraLagMaxDistance, TargetLimit, DeltaTime, 2.f);
}

void AAPSSpeedModeCharacter::UpdateLocomotionAnimation(float DeltaTime)
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	UAnimInstance* Anim = MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
	if (!Anim || bIsZeroG)
	{
		// The zero-G animation keeps the base character's raw values and play rate.
		return;
	}
	if (AnimationInstance.Get() != Anim)
	{
		AnimationInstance = Anim;
		AnimationProperties.Reset();
	}
	const auto FindProperty = [this, Anim](const FName Name) -> FProperty*
	{
		if (FProperty** Cached = AnimationProperties.Find(Name))
		{
			return *Cached;
		}
		FProperty* Property = FindFProperty<FProperty>(Anim->GetClass(), Name);
		AnimationProperties.Add(Name, Property);
		return Property;
	};
	const auto SetNumber = [Anim, &FindProperty](const FName Name, const double Value)
	{
		if (FNumericProperty* Property = CastField<FNumericProperty>(FindProperty(Name)))
		{
			void* Address = Property->ContainerPtrToValuePtr<void>(Anim);
			if (Property->IsFloatingPoint())
			{
				Property->SetFloatingPointPropertyValue(Address, Value);
			}
			else
			{
				Property->SetIntPropertyValue(Address, static_cast<int64>(Value));
			}
		}
	};
	const auto SetBool = [Anim, &FindProperty](const FName Name, const bool bValue)
	{
		if (FBoolProperty* Property = CastField<FBoolProperty>(FindProperty(Name)))
		{
			Property->SetPropertyValue_InContainer(Anim, bValue);
		}
	};

	// The base character has just written the raw values of this frame. A sharp turn or reversal briefly dips the
	// ground speed, which swings the walk/run blend and the play rate and flips ShouldMove through Idle; smoothed
	// values keep the stride continuous.
	const FVector LocalVelocity = GetActorQuat().UnrotateVector(GetVelocity());
	const float GroundSpeed = FVector(LocalVelocity.X, LocalVelocity.Y, 0.0).Size();
	const float Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	const float Alpha = FMath::Clamp(DeltaTime * AnimationSmoothing, 0.f, 1.f);
	SmoothedGroundSpeed = FMath::Lerp(SmoothedGroundSpeed, GroundSpeed, Alpha);
	SmoothedSpeed = FMath::Lerp(SmoothedSpeed, static_cast<float>(LocalVelocity.Size()), Alpha);
	SmoothedDirection = FRotator::NormalizeAxis(SmoothedDirection
		+ FMath::FindDeltaAngleDegrees(SmoothedDirection, Direction) * (GroundSpeed > 3.f ? Alpha : 0.f));
	SetNumber(TEXT("GroundSpeed"), SmoothedGroundSpeed);
	SetNumber(TEXT("Speed"), SmoothedSpeed);
	SetNumber(TEXT("Direction"), SmoothedDirection);
	SetNumber(TEXT("MovementDirection"), SmoothedDirection);
	const bool bMoving = SmoothedGroundSpeed > 3.f;
	SetBool(TEXT("ShouldMove"), bMoving);
	SetBool(TEXT("Moving"), bMoving);

	// Above the pace the run animation was authored for, step faster (sub-linearly, like a real stride) instead of
	// only while Shift is held; the base's rate assumes its own 600 cm/s walk.
	const float PaceRatio = SmoothedGroundSpeed / FMath::Max(AnimationRunSpeed, 1.f);
	const float TargetPlayRate = FMath::Clamp(1.f + (PaceRatio - 1.f) * 0.6f, 1.f, MaxAnimationPlayRate);
	MeshComponent->GlobalAnimRateScale = FMath::FInterpTo(MeshComponent->GlobalAnimRateScale, TargetPlayRate, DeltaTime, 4.f);
}

FText AAPSSpeedModeCharacter::GetTraversalStatusText() const
{
	const FText Base = Super::GetTraversalStatusText();
	if (bIsZeroG)
	{
		return Base;
	}
	static const TCHAR* const ModeNames[] = {TEXT("1 WALK"), TEXT("2 INDOOR"), TEXT("3 OPEN")};
	return FText::FromString(FString::Printf(TEXT("%s | PACE %s"), *Base.ToString(),
		ModeNames[FMath::Clamp(static_cast<int32>(SpeedMode), 0, 2)]));
}

FText AAPSSpeedModeCharacter::GetTraversalHintText() const
{
	if (bIsZeroG)
	{
		return Super::GetTraversalHintText();
	}
	return FText::FromString(GetModeSettings().bBoostJumps
		? TEXT("WASD MOVE   SHIFT SPRINT   SPACE JUMP / DOUBLE-TAP BOOST   1 2 3 PACE   G ZERO-G")
		: TEXT("WASD MOVE   SHIFT RUN   SPACE JUMP   1 2 3 PACE   G ZERO-G"));
}
