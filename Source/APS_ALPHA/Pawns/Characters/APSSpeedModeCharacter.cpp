#include "APSSpeedModeCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "UObject/UnrealType.h"

namespace APSSpeedModeFeel
{
	// Rio 09.10 (playtest: "the walker is cardboard: instant speed and direction changes, no weight; zero-G has no
	// paces and no C view"). Every CVar at 0 restores the behaviour before 09.10.
	TAutoConsoleVariable<int32> CVarWeight(
		TEXT("aps.Character.Weight"), 1,
		TEXT("Rio 09.10: 1: on foot, start, stop and turn ease in over aps.Character.WeightEaseS (acceleration, ground ")
		TEXT("friction and braking build up gradually; top speeds per pace unchanged); in zero-G the thrust eases in. ")
		TEXT("0: the pace's constant acceleration and braking, as before."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightEaseS(
		TEXT("aps.Character.WeightEaseS"), 0.3f,
		TEXT("Seconds over which the push (after pressing a direction or turning sharply) and the braking (after letting go) ")
		TEXT("build from 30% to full."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightAccelScale(
		TEXT("aps.Character.WeightAccelScale"), 0.6f,
		TEXT("Share of the pace's ground acceleration at full push while aps.Character.Weight is 1."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightFrictionScale(
		TEXT("aps.Character.WeightFrictionScale"), 0.4f,
		TEXT("Share of the movement component's ground friction while aps.Character.Weight is 1: lower turns the run ")
		TEXT("into the new direction more gradually and lets a stop glide a little."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightBrakeScale(
		TEXT("aps.Character.WeightBrakeScale"), 0.5f,
		TEXT("Share of the pace's braking deceleration at full braking while aps.Character.Weight is 1."),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarZeroGPace(
		TEXT("aps.Character.ZeroGPace"), 1,
		TEXT("Rio 09.10: 1: keys 1-3 set the zero-G pace too: 1 slow (0.35x speed, 0.6x thrust), 2 as before, 3 fast ")
		TEXT("(1.6x speed, 1.4x thrust) on the character's zero-G values. 0: one zero-G pace, as before."),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarFirstPerson(
		TEXT("aps.Character.FirstPerson"), 1,
		TEXT("Rio 09.10: 1: C toggles a first-person view on foot and in zero-G (smooth move of the camera to eye height, ")
		TEXT("own body hidden from the view but casting its shadow; not in build mode). 0: no first-person view."),
		ECVF_Default);

	constexpr float ZeroGSpeedScale[] = {0.35f, 1.f, 1.6f};
	constexpr float ZeroGThrustScale[] = {0.6f, 1.f, 1.4f};
	constexpr float FirstPersonBlendSeconds = 0.35f;
	constexpr float FirstPersonEyeShare = 0.8f;   // eye height above the capsule centre, share of the half height
}

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
	// The walker's own input only: a piloted ship is possessed and keeps its own C (mouse look); build mode's pushed
	// input consumes C first.
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAPSSpeedModeCharacter::ToggleFirstPersonView);
}

void AAPSSpeedModeCharacter::Tick(float DeltaTime)
{
	// The base re-applies its own acceleration on every gravity/zero-G transition; the mode wins again before
	// the base reads the pace this frame.
	ApplySpeedMode();
	UpdateMovementWeight(DeltaTime);
	Super::Tick(DeltaTime);
	if (IsSurfaceHandoffSuspended() || DeltaTime <= 0.f)
	{
		return;
	}
	UpdateTurnRate(DeltaTime);
	UpdateCameraLag(DeltaTime);
	UpdateFirstPersonCamera(DeltaTime);
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
	CacheBaseMovementValues();
	const FAPSSpeedModeSettings& Mode = GetModeSettings();
	// Zero-G paces scale the character's own zero-G values (Blueprint defaults included); pace 2 keeps them as they are.
	const bool bZeroGPace = APSSpeedModeFeel::CVarZeroGPace.GetValueOnGameThread() != 0;
	if (bZeroGPace || bZeroGPaceApplied)
	{
		const int32 Index = bZeroGPace ? FMath::Clamp(static_cast<int32>(SpeedMode), 0, 2) : 1;
		const float SpeedScale = APSSpeedModeFeel::ZeroGSpeedScale[Index];
		ZeroGMaxSpeed = BaseZeroGMaxSpeed * SpeedScale;
		ZeroGSprintSpeed = BaseZeroGSprintSpeed * SpeedScale;
		ZeroGSprintMaxSpeed = BaseZeroGSprintMaxSpeed * SpeedScale;
		ZeroGSprintGrowthRate = BaseZeroGSprintGrowthRate * SpeedScale;
		ZeroGAcceleration = BaseZeroGAcceleration * APSSpeedModeFeel::ZeroGThrustScale[Index];
		bZeroGPaceApplied = bZeroGPace;
	}
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

void AAPSSpeedModeCharacter::CacheBaseMovementValues()
{
	if (bBaseMovementCached)
	{
		return;
	}
	bBaseMovementCached = true;
	BaseZeroGMaxSpeed = ZeroGMaxSpeed;
	BaseZeroGAcceleration = ZeroGAcceleration;
	BaseZeroGSprintSpeed = ZeroGSprintSpeed;
	BaseZeroGSprintMaxSpeed = ZeroGSprintMaxSpeed;
	BaseZeroGSprintGrowthRate = ZeroGSprintGrowthRate;
	if (const UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		BaseGroundFriction = Movement->GroundFriction;
	}
}

void AAPSSpeedModeCharacter::UpdateMovementWeight(float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || DeltaTime <= 0.f)
	{
		return;
	}
	const bool bWeight = APSSpeedModeFeel::CVarWeight.GetValueOnGameThread() != 0;
	const bool bZeroGPace = APSSpeedModeFeel::CVarZeroGPace.GetValueOnGameThread() != 0;

	// This frame's input while the movement component has not consumed it yet, otherwise the last step's.
	FVector InputDirection = GetPendingMovementInputVector();
	if (InputDirection.IsNearlyZero())
	{
		InputDirection = Movement->GetCurrentAcceleration();
	}
	InputDirection = InputDirection.GetSafeNormal();
	if (InputDirection.IsZero())
	{
		IdleSeconds += DeltaTime;
		DriveSeconds = 0.f;
	}
	else
	{
		// A sharp change of direction (W to S, W to D) builds the push up again, like a body that has to turn first.
		if (!LastInputDirection.IsZero() && FVector::DotProduct(InputDirection, LastInputDirection) < 0.5f)
		{
			DriveSeconds = 0.f;
		}
		DriveSeconds += DeltaTime;
		IdleSeconds = 0.f;
	}
	LastInputDirection = InputDirection;
	const float Ease = FMath::Max(APSSpeedModeFeel::CVarWeightEaseS.GetValueOnGameThread(), 0.01f);
	const float DriveAlpha = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(DriveSeconds / Ease, 0.f, 1.f));
	const float BrakeAlpha = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(IdleSeconds / Ease, 0.f, 1.f));

	if (bIsZeroG)
	{
		// The base sets the zero-G thrust once on entering zero-G; a pace change or an eased push needs it every frame.
		if (bWeight || bZeroGPace || bZeroGAccelerationWritten)
		{
			Movement->MaxAcceleration = ZeroGAcceleration * (bWeight ? FMath::Lerp(0.25f, 1.f, DriveAlpha) : 1.f);
			bZeroGAccelerationWritten = bWeight || bZeroGPace;
		}
		return;
	}
	bZeroGAccelerationWritten = false;
	if (!bWeight)
	{
		// ApplySpeedMode has already set the pace's own acceleration and braking for this frame.
		if (bGroundWeightApplied)
		{
			Movement->GroundFriction = BaseGroundFriction;
			bGroundWeightApplied = false;
		}
		return;
	}
	if (!Movement->IsMovingOnGround())
	{
		// Air control keeps the pace's acceleration (ApplySpeedMode); ground friction does not act in the air.
		return;
	}
	// Ground walking (UE CharacterMovement): GroundFriction turns the velocity towards the input and, doubled by the
	// braking friction factor, slows a run without input together with BrakingDecelerationWalking. At the stock 8 a
	// stop or a turn is over in about a tenth of a second; here both start at 30% and build up over the ease time.
	const FAPSSpeedModeSettings& Mode = GetModeSettings();
	const float Friction = BaseGroundFriction
		* FMath::Clamp(APSSpeedModeFeel::CVarWeightFrictionScale.GetValueOnGameThread(), 0.05f, 1.f);
	Movement->MaxAcceleration = Mode.MaxAcceleration
		* FMath::Max(APSSpeedModeFeel::CVarWeightAccelScale.GetValueOnGameThread(), 0.05f)
		* FMath::Lerp(0.3f, 1.f, DriveAlpha);
	Movement->GroundFriction = InputDirection.IsZero() ? Friction * FMath::Lerp(0.3f, 1.f, BrakeAlpha) : Friction;
	Movement->BrakingDecelerationWalking = Mode.BrakingDeceleration
		* FMath::Max(APSSpeedModeFeel::CVarWeightBrakeScale.GetValueOnGameThread(), 0.f)
		* FMath::Lerp(0.3f, 1.f, BrakeAlpha);
	bGroundWeightApplied = true;
}

void AAPSSpeedModeCharacter::ToggleFirstPersonView()
{
	if (APSSpeedModeFeel::CVarFirstPerson.GetValueOnGameThread() == 0 || IsInBuildMode())
	{
		return;
	}
	bFirstPersonRequested = !bFirstPersonRequested;
	UE_LOG(LogTemp, Log, TEXT("[APS.Camera] character=%s first person %s"), *GetName(),
		bFirstPersonRequested ? TEXT("on") : TEXT("off"));
}

void AAPSSpeedModeCharacter::UpdateFirstPersonCamera(float DeltaTime)
{
	if (!CameraBoom)
	{
		return;
	}
	const bool bBuildMode = IsInBuildMode();
	if (bBuildMode || APSSpeedModeFeel::CVarFirstPerson.GetValueOnGameThread() == 0)
	{
		bFirstPersonRequested = false;
	}
	if (FirstPersonAlpha <= 0.f && !bFirstPersonRequested)
	{
		// Third person: the rig is the base character's, untouched.
		return;
	}
	if (FirstPersonAlpha <= 0.f)
	{
		ThirdPersonBoomLocation = CameraBoom->GetRelativeLocation();
	}
	const float Step = DeltaTime / APSSpeedModeFeel::FirstPersonBlendSeconds;
	// Build mode drives the arm itself: hand the rig back at once.
	FirstPersonAlpha = bBuildMode ? 0.f
		: FMath::Clamp(FirstPersonAlpha + (bFirstPersonRequested ? Step : -Step), 0.f, 1.f);
	const float Alpha = FMath::SmoothStep(0.f, 1.f, FirstPersonAlpha);
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float HalfHeight = Capsule ? FMath::Max(Capsule->GetUnscaledCapsuleHalfHeight(), 40.f) : 88.f;
	const FVector EyeLocation(0.0, 0.0, HalfHeight * APSSpeedModeFeel::FirstPersonEyeShare);
	CameraBoom->SetRelativeLocation(FMath::Lerp(ThirdPersonBoomLocation, EyeLocation, Alpha));
	if (!bBuildMode)
	{
		CameraBoom->TargetArmLength = FMath::Lerp(CameraBoomLength, 0.f, Alpha);
	}
	if (Alpha > 0.f && CameraBoom->bEnableCameraLag)
	{
		// The eye stays in the head: the positional lag shrinks to a centimetre (0 would mean "no limit").
		CameraBoom->CameraLagMaxDistance = FMath::Lerp(CameraBoom->CameraLagMaxDistance, 1.f, Alpha);
	}
	SetFirstPersonBodyHidden(Alpha > 0.6f);
}

void AAPSSpeedModeCharacter::SetFirstPersonBodyHidden(bool bHide)
{
	if (bHide == bFirstPersonBodyHidden)
	{
		return;
	}
	bFirstPersonBodyHidden = bHide;
	if (!bHide)
	{
		for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, uint8>& Part : FirstPersonHiddenParts)
		{
			if (UPrimitiveComponent* Primitive = Part.Key.Get())
			{
				Primitive->SetOwnerNoSee((Part.Value & 1) != 0);
				Primitive->SetCastHiddenShadow((Part.Value & 2) != 0);
			}
		}
		FirstPersonHiddenParts.Reset();
		return;
	}
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}
	TArray<USceneComponent*> Parts;
	MeshComponent->GetChildrenComponents(true, Parts);
	Parts.Add(MeshComponent);
	for (USceneComponent* Part : Parts)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Part))
		{
			FirstPersonHiddenParts.Emplace(Primitive,
				static_cast<uint8>((Primitive->bOwnerNoSee ? 1 : 0) | (Primitive->bCastHiddenShadow ? 2 : 0)));
			// Hidden from the own view only; the body keeps its shadow on the ground.
			Primitive->SetOwnerNoSee(true);
			Primitive->SetCastHiddenShadow(true);
		}
	}
}

FText AAPSSpeedModeCharacter::GetTraversalStatusText() const
{
	const FText Base = Super::GetTraversalStatusText();
	if (bIsZeroG && APSSpeedModeFeel::CVarZeroGPace.GetValueOnGameThread() == 0)
	{
		return Base;
	}
	static const TCHAR* const ModeNames[] = {TEXT("1 WALK"), TEXT("2 INDOOR"), TEXT("3 OPEN")};
	static const TCHAR* const ZeroGModeNames[] = {TEXT("1 SLOW"), TEXT("2 NORMAL"), TEXT("3 FAST")};
	return FText::FromString(FString::Printf(TEXT("%s | PACE %s"), *Base.ToString(),
		(bIsZeroG ? ZeroGModeNames : ModeNames)[FMath::Clamp(static_cast<int32>(SpeedMode), 0, 2)]));
}

FText AAPSSpeedModeCharacter::GetTraversalHintText() const
{
	const TCHAR* const ViewHint = APSSpeedModeFeel::CVarFirstPerson.GetValueOnGameThread() != 0 ? TEXT("   C VIEW") : TEXT("");
	if (bIsZeroG)
	{
		const FText Base = Super::GetTraversalHintText();
		if (APSSpeedModeFeel::CVarZeroGPace.GetValueOnGameThread() == 0 && !*ViewHint)
		{
			return Base;
		}
		return FText::FromString(FString::Printf(TEXT("%s%s%s"), *Base.ToString(),
			APSSpeedModeFeel::CVarZeroGPace.GetValueOnGameThread() != 0 ? TEXT("   1 2 3 PACE") : TEXT(""), ViewHint));
	}
	return FText::FromString(FString::Printf(TEXT("%s%s"), GetModeSettings().bBoostJumps
		? TEXT("WASD MOVE   SHIFT SPRINT   SPACE JUMP / DOUBLE-TAP BOOST   1 2 3 PACE   G ZERO-G")
		: TEXT("WASD MOVE   SHIFT RUN   SPACE JUMP   1 2 3 PACE   G ZERO-G"), ViewHint));
}
