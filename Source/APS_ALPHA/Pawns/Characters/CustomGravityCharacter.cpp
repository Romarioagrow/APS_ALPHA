// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomGravityCharacter.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Gameplay/Construction/APSConstructionMode.h"
#include "APS_ALPHA/Pawns/Characters/GravityDetectorComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/UI/Hud/SAPSWalkerHud.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ObjectKey.h"

namespace APSCharacterQueryThrottle
{
	// Rio 06.10 (night perf R7, "walking aboard costs frames"): two of the walker's scene queries do not need every
	// frame. The take-control trace behind the F prompt runs at a fixed rate (the key press itself always traces fresh),
	// and a deck found under an airborne walker is trusted for a moment instead of sweeping 1 km down again each frame.
	// Both CVars at 0 restore the per-frame queries of 61532ed6.
	TAutoConsoleVariable<float> CVarInteractionTraceHz(
		TEXT("aps.Character.InteractionTraceHz"), 10.0f,
		TEXT("Rate of the walker's take-control trace (the F prompt), per second of real time: the prompt is never older ")
		TEXT("than 1/rate. Pressing F always traces fresh. <= 0: every frame (before 06.10)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarSurfaceSupportCacheS(
		TEXT("aps.Character.SurfaceSupportCacheS"), 0.1f,
		TEXT("Seconds a deck or station floor found under an airborne walker is trusted before the support sweep runs again. ")
		TEXT("Only a found floor is reused: a miss, zero-G, another gravity source, a turn of the down in the source's frame ")
		TEXT("or a 5 m teleport of the walker sweep fresh. Keep it below the 0.2 s support-loss grace. <= 0: every frame (before 06.10)."),
		ECVF_Default);
	// Rio 09.10 (playtest 08.10 item 3, "sometimes no gravity"): a pilot who sat weightless stood up into
	// UpdateGravityDirection's zero-G branch, which keeps zero-G at the first missed floor probe without the 0.2 s grace.
	// Rio 09.10 night (zero-G "small jerks", ACustomGravityCharacter::UpdateExactCameraLag).
	TAutoConsoleVariable<int32> CVarExactCameraLag(
		TEXT("aps.Character.ExactCameraLag"), 1,
		TEXT("1: the walker's camera lag is the exact first-order lag (the same trail behind a steadily moving character ")
		TEXT("whatever the frame times). 0: the spring arm's own lag with substepping (before 09.10), whose trail jumps by ")
		TEXT("~4 cm at 12 m/s on every 16.7 ms frame among 8.3 ms ones."),
		ECVF_Default);
	TAutoConsoleVariable<int32> CVarExitClearsZeroG(
		TEXT("aps.Character.ExitClearsZeroG"), 1,
		TEXT("Rio 09.10 (playtest 08.10 item 3): 1: getting up aboard a ship clears a zero-G the pilot carried from the seat (never ")
		TEXT("the G override), so the deck's 0.2 s support grace applies. 0: that zero-G stays until a floor is found, as before."),
		ECVF_Default);

	// The character header is a shared zone (06.10 night: one writer per file), so the throttle's timestamps live here,
	// one entry per character, dropped on EndPlay, UnPossessed and a vehicle exit. Move them into the class when the
	// header is next opened.
	struct FQueryTimes
	{
		double InteractionTraceRealSeconds = -1.0;
		double SupportFoundSeconds = -1.0;
		TWeakObjectPtr<const AActor> SupportSource;
		FVector SupportLocalStart = FVector::ZeroVector;
		FVector SupportLocalDirection = FVector::ZeroVector;
		/** Rio 09.10: the "[APS.Seat] stood up 1 s ago" line was written for the latest vehicle exit. */
		bool bStandUpAudited = false;
	};
	TMap<FObjectKey, FQueryTimes> QueryTimes;

	void Forget(const ACustomGravityCharacter& Character)
	{
		QueryTimes.Remove(FObjectKey(&Character));
	}

	void NoteInteractionTrace(const ACustomGravityCharacter& Character)
	{
		if (const UWorld* World = Character.GetWorld())
		{
			QueryTimes.FindOrAdd(FObjectKey(&Character)).InteractionTraceRealSeconds = World->GetRealTimeSeconds();
		}
	}

	// True when the prompt's trace is due this frame: throttle off, no trace yet, or the last one is 1/rate old.
	bool IsInteractionTraceDue(const ACustomGravityCharacter& Character)
	{
		const float Hz = CVarInteractionTraceHz.GetValueOnGameThread();
		const UWorld* World = Character.GetWorld();
		if (Hz <= 0.0f || !World)
		{
			return true;
		}
		const FQueryTimes* Times = QueryTimes.Find(FObjectKey(&Character));
		const double Age = Times ? World->GetRealTimeSeconds() - Times->InteractionTraceRealSeconds : -1.0;
		if (Times && Times->InteractionTraceRealSeconds >= 0.0 && Age >= 0.0 && Age < 1.0 / Hz)
		{
			return false;
		}
		NoteInteractionTrace(Character);
		return true;
	}
}

ACustomGravityCharacter::ACustomGravityCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate the character from controller input — we handle it ourselves
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure CharacterMovement for custom gravity
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	// Standard third-person surface locomotion: input is camera-relative and the
	// character turns toward the resulting movement vector.
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 500.f, 0.f);
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = SurfaceWalkSpeed;
	MoveComp->JumpZVelocity = 500.f;
	MoveComp->BrakingDecelerationWalking = 2000.f;
	MoveComp->MaxFlySpeed = ZeroGMaxSpeed;
	MoveComp->BrakingDecelerationFlying = ZeroGBrakingDeceleration;

	GravityDetector = CreateDefaultSubobject<UGravityDetectorComponent>(TEXT("GravityDetector"));

	// Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// The capsule is rotated by CharacterMovement to remain vertical relative to
	// custom gravity. Keep the view in world space so that rotation is not applied
	// a second time through the attachment hierarchy.
	CameraBoom->SetAbsolute(false, true, false);
	CameraBoom->TargetArmLength = CameraBoomLength;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;
	CameraBoom->bUseCameraLagSubstepping = true;
	CameraBoom->CameraLagMaxTimeStep = 1.0f / 120.0f;
	CameraBoom->bClampToMaxPhysicsDeltaTime = true;
	// Generated-civilization handoff can teleport the pawn from its temporary
	// station spawn to a full-scale planet in one frame. Never let camera lag
	// retain an unlimited, planet-sized separation from the character.
	CameraBoom->CameraLagMaxDistance = CameraBoomLength * 0.75f;

	// Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FClassFinder<UAnimInstance> SurfaceAnimationBlueprint(
		TEXT("/Game/APS/APS_ALPHA/Blueprints/APS_ABP_Manny"));
	if (SurfaceAnimationBlueprint.Succeeded())
	{
		SurfaceAnimationClass = SurfaceAnimationBlueprint.Class;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> ZeroGAnimationBlueprint(
		TEXT("/Game/APS/APS_ALPHA/Blueprints/ABP_ZeroGAnim"));
	if (ZeroGAnimationBlueprint.Succeeded())
	{
		ZeroGAnimationClass = ZeroGAnimationBlueprint.Class;
	}
}

void ACustomGravityCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint component defaults from older character revisions must not enable
	// a second, controller-driven camera/facing path at runtime.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetAbsolute(false, true, false);
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	// Old Blueprint defaults can override the native spring-arm settings. Force a
	// frame-rate-independent camera at runtime so custom-gravity movement cannot
	// appear to pulse while the capsule itself is moving smoothly.
	CameraBoom->bUseCameraLagSubstepping = true;
	CameraBoom->CameraLagMaxTimeStep = 1.0f / 120.0f;
	CameraBoom->bClampToMaxPhysicsDeltaTime = true;
	FollowCamera->bUsePawnControlRotation = false;
	NormalizeThirdPersonCameraRig();
	if (!SurfaceAnimationClass && GetMesh())
	{
		SurfaceAnimationClass = GetMesh()->GetAnimClass();
	}
	ApplyAnimationMode();

	bManualGravityOverride = IsValid(GravityTarget);
	if (GravityDetector)
	{
		GravityDetector->OnClosestGravityBodyChanged.AddDynamic(
			this, &ACustomGravityCharacter::HandleGravitySourceChanged);
	}

	// Add Input Mapping Context
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Initialize gravity
	UpdateGravityDirection(0.0f);
	CameraReferenceUp = GetGravityUpVector();
	CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
		GetActorForwardVector(), CameraReferenceUp).GetSafeNormal();
	if (CameraForwardOnGravityPlane.IsNearlyZero())
	{
		CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
			FVector::ForwardVector, CameraReferenceUp).GetSafeNormal();
	}
	CreateInteractionPrompt();
	CreateTraversalHud();
}

void ACustomGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// After leaving a vehicle: a character moving away from its planet faster than 10 m/s logs why, once.
	if (!bExitRiseLogged && GetWorld() && GetWorld()->GetTimeSeconds() - VehicleExitSeconds < 8.0
		&& CurrentGravityType == EGravityType::OnPlanet && GravityDetector && GravityDetector->GravityTargetActor)
	{
		const FVector Outward = (GetActorLocation() - GravityDetector->GravityTargetActor->GetActorLocation()).GetSafeNormal();
		const double Rise = FVector::DotProduct(GetVelocity(), Outward);
		if (Rise > 1000.0)
		{
			bExitRiseLogged = true;
			const UCharacterMovementComponent* Movement = GetCharacterMovement();
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Gravity] Rising after exit: %.1f m/s away from %s; gravityDir=%s outward=%s zeroG=%d manual=%d mode=%d base=%s handoffSuspended=%d"),
				Rise / 100.0, *GetNameSafe(GravityDetector->GravityTargetActor), *CurrentGravityDir.ToCompactString(),
				*Outward.ToCompactString(), bIsZeroG ? 1 : 0, bManualZeroGOverride ? 1 : 0,
				Movement ? static_cast<int32>(Movement->MovementMode.GetValue()) : -1,
				Movement && Movement->GetMovementBase() ? *GetNameSafe(Movement->GetMovementBase()->GetOwner()) : TEXT("-"),
				bSurfaceHandoffSuspended ? 1 : 0);
		}
	}
	// Rio 09.10 (playtest 08.10 items 1-3): one line a second after getting up says where the walker stands (the stand-up
	// checks' proof: on a deck of the ship it left, near the seat, under the ship's gravity).
	if (const double SinceExit = GetWorld() ? GetWorld()->GetTimeSeconds() - VehicleExitSeconds : -1.0;
		SinceExit >= 1.0 && SinceExit < 8.0)
	{
		APSCharacterQueryThrottle::FQueryTimes& Times = APSCharacterQueryThrottle::QueryTimes.FindOrAdd(FObjectKey(this));
		if (!Times.bStandUpAudited)
		{
			Times.bStandUpAudited = true;
			const ASpaceship* Ship = AboardShip.Get();
			const UCharacterMovementComponent* Movement = GetCharacterMovement();
			const FVector Local = Ship ? Ship->GetActorTransform().InverseTransformPosition(GetActorLocation()) : FVector::ZeroVector;
			const FVector SeatLocal = Ship && Ship->PilotChair
				? Ship->GetActorTransform().InverseTransformPosition(Ship->PilotChair->GetComponentLocation()) : FVector::ZeroVector;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Seat] stood up 1 s ago: ship=%s local=%s cm seatLocal=%s cm fromSeat=%.0f cm onGround=%d zeroG=%d gravity=%s source=%s attached=%d"),
				*GetNameSafe(Ship), *Local.ToCompactString(), *SeatLocal.ToCompactString(),
				Ship ? FVector::Dist(Local, SeatLocal) : -1.0, Movement && Movement->IsMovingOnGround() ? 1 : 0, bIsZeroG ? 1 : 0,
				*UEnum::GetValueAsString(CurrentGravityType),
				*GetNameSafe(GravityDetector ? GravityDetector->GravityTargetActor : nullptr),
				Ship && GetAttachParentActor() == Ship ? 1 : 0);
		}
	}

	UpdateBuildMode(DeltaTime);

	if (bSurfaceHandoffSuspended)
	{
		// WorldScape keys both visual and collision streaming to this pawn.  Even a
		// small gravity/input displacement while its async batch is cooking can snap
		// the two producers to different cells and leave a flat/stale collider under
		// the rendered terrain.  Keep the observer physically invariant until the
		// generator explicitly releases the handoff.
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			if (Movement->MovementMode != MOVE_None)
			{
				Movement->DisableMovement();
			}
		}
		UpdateCameraReferenceFrame();
		UpdateBuildCamera(DeltaTime);
		AlignCameraToGravity(DeltaTime);
		UpdateGravityAnimationParameters();
		if (!DefersExactCameraLag())
		{
			UpdateExactCameraLag(DeltaTime);
		}
		return;
	}

	UpdateShipPassenger();
	if (bUseCustomGravity)
	{
		UpdateGravityDirection(DeltaTime);
	}
	AdvanceGravityStrengthTransition(DeltaTime);
	UpdateMovementSpeed(DeltaTime);
	UpdateBoostJump(DeltaTime);

	UpdateCameraReferenceFrame();
	if (bIsZeroG)
	{
		SynchronizeCharacterToCamera(DeltaTime);
	}
	UpdateGravityAnimationParameters();
	UpdateBuildCamera(DeltaTime);
	AlignCameraToGravity(DeltaTime);
	// Rio 06.10 (night perf R7): the prompt's trace runs at aps.Character.InteractionTraceHz, not every frame. It still
	// runs aboard (a rover in the cargo bay is found by it, not by the ship fallback). Between traces only the cheap
	// check runs, so a craft someone else took or that is gone drops its prompt in the same frame as before.
	if (APSCharacterQueryThrottle::IsInteractionTraceDue(*this))
	{
		UpdateInteractionCandidate();
	}
	else if (AActor* Cached = CurrentInteractableActor.Get())
	{
		IVehicleControlling* CachedVehicle = Cast<IVehicleControlling>(Cached);
		if (!CachedVehicle || !CachedVehicle->CanRequestVehicleControl(this))
		{
			CurrentInteractableActor = nullptr;
		}
	}
	if (!InteractionPromptWidget.IsValid())
	{
		CreateInteractionPrompt();
	}
	if (!TraversalHudWidget.IsValid())
	{
		CreateTraversalHud();
	}
	if (!DefersExactCameraLag())
	{
		UpdateExactCameraLag(DeltaTime);
	}
}

void ACustomGravityCharacter::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);
	ExactLagPrevOrigin += InOffset;
}

float ACustomGravityCharacter::GetCameraLocationLagSpeed() const
{
	return CameraBoom && CameraBoom->CameraLagSpeed > 0.f ? CameraBoom->CameraLagSpeed : ExactLagSpeed;
}

void ACustomGravityCharacter::UpdateExactCameraLag(float DeltaTime)
{
	if (!CameraBoom)
	{
		return;
	}
	if (APSCharacterQueryThrottle::CVarExactCameraLag.GetValueOnGameThread() == 0)
	{
		// Hand the lag back to the arm exactly as it was.
		if (ExactLagSpeed > 0.f && CameraBoom->CameraLagSpeed <= 0.f)
		{
			CameraBoom->CameraLagSpeed = ExactLagSpeed;
		}
		if (bExactLagValid)
		{
			CameraBoom->TargetOffset = FVector::ZeroVector;
			bExactLagValid = false;
		}
		return;
	}
	if (CameraBoom->CameraLagSpeed > 0.f)
	{
		ExactLagSpeed = CameraBoom->CameraLagSpeed;
		// Speed 0: USpringArmComponent's VInterpTo returns the (offset) arm origin itself, no second lag.
		CameraBoom->CameraLagSpeed = 0.f;
	}
	const FVector Origin = CameraBoom->GetComponentLocation();
	if (!CameraBoom->bEnableCameraLag || !(ExactLagSpeed > 0.f))
	{
		// No lag wanted (aboard a ship): the arm starts at the character; lag starts fresh when it comes back.
		if (bExactLagValid || !CameraBoom->TargetOffset.IsZero())
		{
			CameraBoom->TargetOffset = FVector::ZeroVector;
		}
		bExactLagValid = false;
		return;
	}
	if (!bExactLagValid)
	{
		ExactLagTrail = FVector::ZeroVector;
		ExactLagPrevOrigin = Origin;
		bExactLagValid = true;
	}
	if (DeltaTime > UE_SMALL_NUMBER)
	{
		// x' = k (O(t) - x) with O moving linearly from the previous origin to this one through the frame:
		// trail(t + dt) = -u/k + (trail(t) + u/k) e^(-k dt), u = the origin's velocity over the frame.
		const double K = ExactLagSpeed;
		const FVector U = (Origin - ExactLagPrevOrigin) / DeltaTime;
		const FVector Lead = U / K;
		ExactLagTrail = -Lead + (ExactLagTrail + Lead) * FMath::Exp(-K * DeltaTime);
		ExactLagPrevOrigin = Origin;
	}
	// The rig's distance limit (teleports, first person) as the arm applies it.
	const float MaxDistance = CameraBoom->CameraLagMaxDistance;
	if (MaxDistance > 0.f && ExactLagTrail.SizeSquared() > FMath::Square(MaxDistance))
	{
		ExactLagTrail = ExactLagTrail.GetSafeNormal() * MaxDistance;
	}
	CameraBoom->TargetOffset = ExactLagTrail;
}

void ACustomGravityCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ConstructionMode.IsValid())
	{
		ExitBuildMode(true);
	}
	RemoveInteractionPrompt();
	RemoveTraversalHud();
	// Rio 06.10 (night perf R7): the query throttle's per-character timestamps go with the character.
	APSCharacterQueryThrottle::Forget(*this);
	Super::EndPlay(EndPlayReason);
}

void ACustomGravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACustomGravityCharacter::HandleMove);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACustomGravityCharacter::HandleLook);
		}
		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACustomGravityCharacter::HandleJumpStarted);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACustomGravityCharacter::HandleJumpCompleted);
		}
	}

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ACustomGravityCharacter::TryInteract);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Pressed, this, &ACustomGravityCharacter::HandleSprintStarted);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Released, this, &ACustomGravityCharacter::HandleSprintCompleted);
	PlayerInputComponent->BindAxis("MoveUp", this, &ACustomGravityCharacter::HandleZeroGVertical);
	PlayerInputComponent->BindAxis("RotateRoll", this, &ACustomGravityCharacter::HandleZeroGRoll);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &ACustomGravityCharacter::ToggleManualZeroGOverride);
	// Build mode (Rio 02.10). While it is on, its own input component above this one takes B to leave.
	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &ACustomGravityCharacter::ToggleBuildMode);
}

// ──────────────────────── Input Handlers ────────────────────────

void ACustomGravityCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	UpdateCameraReferenceFrame();
	if (bIsZeroG)
	{
		const FQuat ViewRotation = GetCameraViewRotation();
		AddMovementInput(ViewRotation.GetForwardVector(), MovementVector.Y);
		AddMovementInput(ViewRotation.GetRightVector(), MovementVector.X);
		return;
	}

	const FVector GravityUp = GetGravityUpVector();
	const FVector CamForward = GetCameraPlanarForward();
	const FVector CamRight = FVector::CrossProduct(GravityUp, CamForward).GetSafeNormal();

	// Movement and visible facing use exactly the same gravity-relative basis.
	AddMovementInput(CamForward, MovementVector.Y);
	AddMovementInput(CamRight, MovementVector.X);
}

void ACustomGravityCharacter::HandleLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (ShouldInvertLookPitch())
	{
		LookAxisVector.Y = -LookAxisVector.Y;
	}
	if (ConstructionMode.IsValid())
	{
		// Building: the cursor moves freely; a right or middle drag turns the camera about gravity and tilts its look
		// down within the build view's range.
		const APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if (!PlayerController || (!PlayerController->IsInputKeyDown(EKeys::RightMouseButton)
			&& !PlayerController->IsInputKeyDown(EKeys::MiddleMouseButton)))
		{
			return;
		}
		UpdateCameraReferenceFrame();
		const float BuildYawDelta = LookAxisVector.X * LookSensitivity;
		if (!FMath::IsNearlyZero(BuildYawDelta))
		{
			const FVector GravityUp = GetGravityUpVector();
			CameraForwardOnGravityPlane = FVector::VectorPlaneProject(FQuat(GravityUp, FMath::DegreesToRadians(BuildYawDelta))
				.RotateVector(CameraForwardOnGravityPlane), GravityUp).GetSafeNormal();
		}
		BuildCameraPitch = FMath::Clamp(BuildCameraPitch - LookAxisVector.Y * LookSensitivity, 20.f, 85.f);
		BuildRightDragAmount += FMath::Abs(LookAxisVector.X) + FMath::Abs(LookAxisVector.Y);
		return;
	}
	if (bIsZeroG)
	{
		if (!bZeroGViewRotationInitialized)
		{
			ZeroGViewRotation = CameraBoom
				? CameraBoom->GetComponentQuat()
				: GetActorQuat();
			bZeroGViewRotationInitialized = true;
		}

		if (!FMath::IsNearlyZero(LookAxisVector.X))
		{
			const FQuat YawRotation(
				ZeroGViewRotation.GetUpVector(),
				FMath::DegreesToRadians(LookAxisVector.X * LookSensitivity));
			ZeroGViewRotation = (YawRotation * ZeroGViewRotation).GetNormalized();
		}
		if (!FMath::IsNearlyZero(LookAxisVector.Y))
		{
			const FQuat PitchRotation(
				ZeroGViewRotation.GetRightVector(),
				FMath::DegreesToRadians(-LookAxisVector.Y * LookSensitivity));
			ZeroGViewRotation = (PitchRotation * ZeroGViewRotation).GetNormalized();
		}

		SynchronizeCharacterToCamera(0.0f);
		return;
	}

	UpdateCameraReferenceFrame();

	const float YawDelta = LookAxisVector.X * LookSensitivity;
	if (!FMath::IsNearlyZero(YawDelta))
	{
		const FVector GravityUp = GetGravityUpVector();
		CameraForwardOnGravityPlane = FQuat(
			GravityUp, FMath::DegreesToRadians(YawDelta))
			.RotateVector(CameraForwardOnGravityPlane);
		CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
			CameraForwardOnGravityPlane, GravityUp).GetSafeNormal();
	}
	CameraPitch = FMath::Clamp(CameraPitch - LookAxisVector.Y * LookSensitivity, -80.f, 80.f);
}

void ACustomGravityCharacter::HandleJumpStarted()
{
	if (bIsZeroG)
	{
		AddMovementInput(GetCameraViewRotation().GetUpVector(), 1.0f);
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bDoubleTap = LastJumpPressTime >= 0.0f &&
		Now - LastJumpPressTime <= DoubleTapJumpWindow;
	LastJumpPressTime = bDoubleTap ? -1.0f : Now;

	if (bDoubleTap)
	{
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (Movement)
		{
			const FVector GravityUp = GetGravityUpVector();
			const float CurrentUpSpeed = FVector::DotProduct(Movement->Velocity, GravityUp);
			Movement->Velocity += GravityUp *
				FMath::Max(0.0f, DoubleTapJumpVelocity - CurrentUpSpeed);
			Movement->SetMovementMode(MOVE_Falling);
			bBoostJumpHeld = true;
		}
		return;
	}

	Jump();
}

void ACustomGravityCharacter::HandleJumpCompleted()
{
	bBoostJumpHeld = false;
	if (!bIsZeroG)
	{
		StopJumping();
	}
}

void ACustomGravityCharacter::HandleZeroGVertical(float Value)
{
	if (bIsZeroG && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetCameraViewRotation().GetUpVector(), Value);
	}
}

void ACustomGravityCharacter::HandleZeroGRoll(float Value)
{
	if (!bIsZeroG || FMath::IsNearlyZero(Value) || !GetWorld())
	{
		return;
	}

	if (!bZeroGViewRotationInitialized)
	{
		ZeroGViewRotation = CameraBoom
			? CameraBoom->GetComponentQuat()
			: GetActorQuat();
		bZeroGViewRotationInitialized = true;
	}

	// Input mapping is Q=-1/E=+1. Quaternion positive rotation appears as a
	// left roll to the player, so negate it to keep Q=left and E=right.
	const float RollDelta = -Value * ZeroGRollSpeed * GetWorld()->GetDeltaSeconds();
	const FQuat RollRotation(
		ZeroGViewRotation.GetForwardVector(),
		FMath::DegreesToRadians(RollDelta));
	ZeroGViewRotation = (RollRotation * ZeroGViewRotation).GetNormalized();
}

void ACustomGravityCharacter::HandleSprintStarted()
{
	if (!bSprintHeld)
	{
		SprintHoldDuration = 0.0f;
	}
	bSprintHeld = true;
}

void ACustomGravityCharacter::HandleSprintCompleted()
{
	bSprintHeld = false;
	SprintHoldDuration = 0.0f;
}

void ACustomGravityCharacter::UpdateMovementSpeed(float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}
	if (bSprintHeld)
	{
		SprintHoldDuration += FMath::Max(DeltaTime, 0.0f);
	}
	if (bIsZeroG)
	{
		const float TargetSpeed = bSprintHeld
			? FMath::Min(
				ZeroGSprintSpeed + SprintHoldDuration * ZeroGSprintGrowthRate,
				ZeroGSprintMaxSpeed)
			: ZeroGMaxSpeed;
		Movement->MaxFlySpeed = FMath::FInterpConstantTo(
			Movement->MaxFlySpeed, TargetSpeed, DeltaTime, SprintSpeedChangeRate);
	}
	else
	{
		const float TargetSpeed = bSprintHeld
			? FMath::Min(
				SurfaceSprintSpeed + SprintHoldDuration * SurfaceSprintGrowthRate,
				SurfaceSprintMaxSpeed)
			: SurfaceWalkSpeed;
		Movement->MaxWalkSpeed = FMath::FInterpConstantTo(
			Movement->MaxWalkSpeed, TargetSpeed, DeltaTime, SprintSpeedChangeRate);
	}
}

void ACustomGravityCharacter::UpdateBoostJump(float DeltaTime)
{
	if (!bBoostJumpHeld || bIsZeroG || DeltaTime <= 0.0f)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || Movement->IsMovingOnGround())
	{
		return;
	}

	const FVector GravityUp = GetGravityUpVector();
	const float CurrentUpSpeed = FVector::DotProduct(Movement->Velocity, GravityUp);
	if (CurrentUpSpeed < BoostJumpMaxUpSpeed)
	{
		const float AddedSpeed = FMath::Min(
			BoostJumpAcceleration * DeltaTime,
			BoostJumpMaxUpSpeed - CurrentUpSpeed);
		Movement->Velocity += GravityUp * AddedSpeed;
	}
}

void ACustomGravityCharacter::TryInteract()
{
	// Rio 06.10 (night perf R7): with the prompt's trace throttled, the key press traces fresh, so F acts on the current
	// aim as it did with the per-frame trace (and the prompt shows the same craft from this frame on).
	if (APSCharacterQueryThrottle::CVarInteractionTraceHz.GetValueOnGameThread() > 0.0f)
	{
		UpdateInteractionCandidate();
		APSCharacterQueryThrottle::NoteInteractionTrace(*this);
	}
	AActor* Candidate = CurrentInteractableActor.Get();
	if (!Candidate)
	{
		Candidate = FindInteractionCandidate();
	}

	IVehicleControlling* Vehicle = Candidate ? Cast<IVehicleControlling>(Candidate) : nullptr;
	if (!Vehicle)
	{
		return;
	}

	if (Vehicle->CanRequestVehicleControl(this))
	{
		Vehicle->RequestVehicleControl(this);
	}
}

AActor* ACustomGravityCharacter::FindInteractionCandidate()
{
	AActor* Candidate = nullptr;
	if (UWorld* World = GetWorld())
	{
		const FVector Start = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
		const FVector Direction = FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector();
		// Ships answer with their InteractionBounds box, which overlaps Visibility rather than blocking it (Rio 02.10:
		// a blocking box swallowed the foot IK's traces aboard and sank the mesh), so the overlaps count too: the first
		// vehicle among them and the first block.
		TArray<FHitResult> Hits;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSCharacterInteraction), false, this);
		World->LineTraceMultiByChannel(Hits, Start, Start + Direction * InteractionDistance, ECC_Visibility, QueryParams);
		for (const FHitResult& Hit : Hits)
		{
			if (AActor* Vehicle = ResolveVehicleActor(Hit.GetActor()))
			{
				Candidate = Vehicle;
				break;
			}
		}
	}

	if (!Candidate && GravityDetector)
	{
		Candidate = ResolveVehicleActor(GravityDetector->CurrentSpaceship);
	}

	// Rio 04.10 ("bring back taking the controls from anywhere once I'm in; when I get up I appear at the seat"): aboard
	// any ship the controls are taken wherever the player stands (02.10 limited a modelled cabin to its seat); getting up
	// still puts the pilot behind the seat (ASpaceship::GetPilotExitTransform).
	return Candidate;
}

AActor* ACustomGravityCharacter::ResolveVehicleActor(AActor* Candidate)
{
	for (AActor* Current = Candidate; IsValid(Current); Current = Current->GetAttachParentActor())
	{
		if (Current->GetClass()->ImplementsInterface(UVehicleControlling::StaticClass()))
		{
			return Current;
		}
	}
	return nullptr;
}

void ACustomGravityCharacter::UpdateInteractionCandidate()
{
	AActor* Candidate = FindInteractionCandidate();
	if (IVehicleControlling* Vehicle = Candidate ? Cast<IVehicleControlling>(Candidate) : nullptr)
	{
		if (!Vehicle->CanRequestVehicleControl(this))
		{
			Candidate = nullptr;
		}
	}
	else
	{
		Candidate = nullptr;
	}
	// Rio 06.10 (night perf R7): one line per change of the prompt's craft, so offscreen runs show the F prompt still
	// appears (aboard, or at a rover) with the trace throttled.
	if (Candidate != CurrentInteractableActor.Get() && IsLocallyControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Interact] prompt %s -> %s (traceHz=%.1f)"),
			*GetNameSafe(CurrentInteractableActor.Get()), *GetNameSafe(Candidate),
			APSCharacterQueryThrottle::CVarInteractionTraceHz.GetValueOnGameThread());
	}
	CurrentInteractableActor = Candidate;
}

void ACustomGravityCharacter::CreateInteractionPrompt()
{
	if (InteractionPromptWidget.IsValid() || !IsLocallyControlled() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const TWeakObjectPtr<ACustomGravityCharacter> WeakThis(this);
	InteractionPromptWidget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 0.0f, 132.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				const AGravityPlayerController* Controller = Character ? Cast<AGravityPlayerController>(Character->GetController()) : nullptr;
				return Character && Character->IsLocallyControlled() && Character->CurrentInteractableActor.IsValid()
					&& !(Controller && Controller->IsStrategicMapOpen())
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			[
				// Rio 06.10 (Claude UI): a HUD card in the interface theme, naming the craft rather than its actor.
				SNew(SAPSTakeControlPrompt)
				.Subject_Lambda([WeakThis]()
				{
					const ACustomGravityCharacter* Character = WeakThis.Get();
					const AActor* Vehicle = Character ? Character->CurrentInteractableActor.Get() : nullptr;
					if (const ASpaceship* Craft = Cast<ASpaceship>(Vehicle))
					{
						return Craft->IsGroundVehicle()
							? NSLOCTEXT("APSInteraction", "GroundVehicle", "Ground vehicle")
							: FText::Format(NSLOCTEXT("APSInteraction", "ShipClass", "Ship, class {0}"), FText::FromString(Craft->GetSizeClassName()));
					}
					return Vehicle ? NSLOCTEXT("APSInteraction", "Vehicle", "Vehicle") : FText::GetEmpty();
				})
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(InteractionPromptWidget.ToSharedRef(), 50);
}

void ACustomGravityCharacter::RemoveInteractionPrompt()
{
	if (InteractionPromptWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(InteractionPromptWidget.ToSharedRef());
	}
	InteractionPromptWidget.Reset();
}

// ──────────────────────── Gravity ────────────────────────

void ACustomGravityCharacter::CreateTraversalHud()
{
	if (TraversalHudWidget.IsValid() || !IsLocallyControlled() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const TWeakObjectPtr<ACustomGravityCharacter> WeakThis(this);
	TraversalHudWidget =
		SNew(SOverlay)
		// The F10 map and the colony terminal leave gaps between their panels: the walker's HUD steps aside under them
		// instead of showing through (02.10 test shots).
		.Visibility_Lambda([WeakThis]()
		{
			const ACustomGravityCharacter* Character = WeakThis.Get();
			const AGravityPlayerController* Controller = Character ? Cast<AGravityPlayerController>(Character->GetController()) : nullptr;
			const UAPSColonyTerminalSubsystem* Terminal = Character && Character->GetWorld()
				? Character->GetWorld()->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
			// Rio 06.10: also while the pilot sits in a craft (not controlled): the ship's bar holds the bottom centre then.
			return !Character || !Character->IsLocallyControlled()
				|| (Controller && Controller->IsStrategicMapOpen()) || (Terminal && Terminal->IsTerminalOpen())
				? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible;
		})
		// Rio 06.10 (Claude UI): the HUD's instrument family (UI/Hud/SAPSWalkerHud) instead of the text panel.
		+ SOverlay::Slot()
		[
			SNew(SAPSWalkerHud)
			.StatusText_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				return Character ? Character->GetTraversalStatusText() : FText::GetEmpty();
			})
			.HintText_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				return Character ? Character->GetTraversalHintText() : FText::GetEmpty();
			})
			.BuildText_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				return Character ? Character->GetBuildHintText() : FText::GetEmpty();
			})
			.Alert_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				return Character && Character->bManualZeroGOverride;
			})
		];

	GEngine->GameViewport->AddViewportWidgetContent(TraversalHudWidget.ToSharedRef(), 40);
}

void ACustomGravityCharacter::RemoveTraversalHud()
{
	if (TraversalHudWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(TraversalHudWidget.ToSharedRef());
	}
	TraversalHudWidget.Reset();
}

FText ACustomGravityCharacter::GetTraversalStatusText() const
{
	FString Mode;
	if (bManualZeroGOverride)
	{
		Mode = TEXT("MANUAL ZERO-G");
	}
	else if (bIsZeroG)
	{
		Mode = TEXT("ZERO-G / FREE FLIGHT");
	}
	else
	{
		switch (CurrentGravityType)
		{
		case EGravityType::OnPlanet:
			Mode = TEXT("GRAVITY / PLANET");
			break;
		case EGravityType::OnShip:
			Mode = TEXT("GRAVITY / SHIP");
			break;
		case EGravityType::OnStation:
			Mode = TEXT("GRAVITY / STATION");
			break;
		default:
			Mode = TEXT("GRAVITY / SURFACE");
			break;
		}
	}

	const float SpeedMetersPerSecond = GetVelocity().Size() / 100.0f;
	const TCHAR* BoostStatus = bSprintHeld ? TEXT(" | BOOST") : TEXT("");
	return FText::FromString(FString::Printf(
		TEXT("%s | %.1f m/s%s"), *Mode, SpeedMetersPerSecond, BoostStatus));
}

FText ACustomGravityCharacter::GetTraversalHintText() const
{
	return bIsZeroG
		? FText::FromString(TEXT("WASD FLY   SPACE / ALT VERTICAL   Q / E ROLL   SHIFT BOOST   G GRAVITY"))
		: FText::FromString(TEXT("WASD MOVE   SHIFT SPRINT   SPACE JUMP / DOUBLE-TAP BOOST   G ZERO-G"));
}

void ACustomGravityCharacter::UpdateGravityDirection(float DeltaTime)
{
	// Rio 03.10 ("flew from the surface to the HQ station, got out of the ship: it centres on the planet again"): the
	// surface spawn pins gravity to its planet (SetGravityTarget) and the pin outlived the planet, so aboard a ship or on
	// a station the character still fell toward the planet's centre. A ship or station the detector finds releases it.
	if (bManualGravityOverride && GravityTarget && GravityDetector && IsValid(GravityDetector->GravityTargetActor)
		&& GravityDetector->GravityTargetActor != GravityTarget
		&& (GravityDetector->CurrentGravityType == EGravityType::OnShip
			|| GravityDetector->CurrentGravityType == EGravityType::OnStation))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] spawn pin to %s released: %s is the local frame"),
			*GetNameSafe(GravityTarget), *GetNameSafe(GravityDetector->GravityTargetActor));
		bManualGravityOverride = false;
	}

	if (bManualZeroGOverride)
	{
		SetZeroGravityEnabled(true);
		return;
	}

	FVector NewGravityDir = FVector::ZeroVector;

	if (bManualGravityOverride && GravityTarget)
	{
		// Radial gravity: direction from character toward the gravity center
		NewGravityDir = (GravityTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		CurrentGravityType = EGravityType::OnPlanet;
	}
	else if (bManualGravityOverride)
	{
		NewGravityDir = DefaultGravityDirection.GetSafeNormal();
	}
	else if (GravityDetector)
	{
		GravityTarget = GravityDetector->GravityTargetActor;
		CurrentGravityType = GravityDetector->CurrentGravityType;
		NewGravityDir = GravityDetector->GetGravityDirectionAtLocation(GetActorLocation());
	}
	else if (!bUseZeroGWhenNoSource)
	{
		NewGravityDir = DefaultGravityDirection.GetSafeNormal();
	}

	if (NewGravityDir.IsNearlyZero())
	{
		if (bUseZeroGWhenNoSource)
		{
			SetZeroGravityEnabled(true);
			return;
		}

		NewGravityDir = FVector(0.f, 0.f, -1.f);
	}

	DesiredGravityDir = NewGravityDir.GetSafeNormal();

	const bool bRequiresNearbySurface =
		CurrentGravityType == EGravityType::OnStation ||
		CurrentGravityType == EGravityType::OnShip;
	if (bRequiresNearbySurface)
	{
		if (HasSurfaceGravitySupport(DesiredGravityDir))
		{
			SurfaceSupportLossElapsed = 0.0f;
		}
		else if (bIsZeroG)
		{
			SetZeroGravityEnabled(true);
			return;
		}
		else
		{
			SurfaceSupportLossElapsed += FMath::Max(DeltaTime, 0.0f);
			if (SurfaceSupportLossElapsed >= SurfaceSupportLossGracePeriod)
			{
				SetZeroGravityEnabled(true);
				return;
			}
		}
	}
	else
	{
		SurfaceSupportLossElapsed = 0.0f;
	}

	const bool bWasZeroG = bIsZeroG;
	SetZeroGravityEnabled(false);

	if (!bGravityDirectionInitialized)
	{
		CurrentGravityDir = DesiredGravityDir;
		GravityTransitionStartDir = DesiredGravityDir;
		GravityTransitionTargetDir = DesiredGravityDir;
		GravityTransitionElapsed = GravityTransitionDuration;
		bGravityDirectionInitialized = true;
	}
	else
	{
		const double TargetDot = FVector::DotProduct(
			GravityTransitionTargetDir.GetSafeNormal(), DesiredGravityDir);
		const bool bDirectionChanged = TargetDot <
			FMath::Cos(FMath::DegreesToRadians(1.0));
		if (bWasZeroG)
		{
			StartGravityDirectionTransition(DesiredGravityDir, true);
		}
		else if (GravityTransitionElapsed < GravityTransitionDuration)
		{
			// A radial source changes direction continuously while the character
			// travels. Retarget the active blend without restarting its timer every
			// frame; repeated restarts were perceived as gravity/camera clipping.
			GravityTransitionTargetDir = DesiredGravityDir;
		}
		else if (bDirectionChanged)
		{
			StartGravityDirectionTransition(DesiredGravityDir, false);
		}
		else
		{
			CurrentGravityDir = DesiredGravityDir;
			GravityTransitionTargetDir = DesiredGravityDir;
		}
	}

	AdvanceGravityDirectionTransition(DeltaTime);
	// Aboard a turning ship its down turns every frame: once the boarding transition is over, gravity follows the deck
	// at once instead of a world-space blend that lagged the floor.
	if (AboardShip.IsValid() && CurrentGravityType == EGravityType::OnShip && !bIsZeroG
		&& GravityTransitionElapsed >= GravityTransitionDuration)
	{
		CurrentGravityDir = DesiredGravityDir;
		GravityTransitionTargetDir = DesiredGravityDir;
	}

	// UE 5.4 aligns the capsule and movement simulation to this direction. Feeding
	// it the interpolated vector keeps physics, camera and the visible body in sync.
	GetCharacterMovement()->SetGravityDirection(CurrentGravityDir);
}

void ACustomGravityCharacter::StartGravityDirectionTransition(
	const FVector& TargetGravityDirection, bool bStartFromActorUp)
{
	const FVector SafeTarget = TargetGravityDirection.GetSafeNormal();
	if (SafeTarget.IsNearlyZero())
	{
		return;
	}

	if (bStartFromActorUp)
	{
		CurrentGravityDir = -GetActorUpVector();
	}
	CurrentGravityDir = CurrentGravityDir.GetSafeNormal();
	GravityTransitionStartDir = CurrentGravityDir;
	GravityTransitionTargetDir = SafeTarget;
	GravityTransitionElapsed = 0.0f;

	if (GravityTransitionDuration <= UE_SMALL_NUMBER ||
		GravityTransitionStartDir.Equals(GravityTransitionTargetDir, KINDA_SMALL_NUMBER))
	{
		CurrentGravityDir = GravityTransitionTargetDir;
		GravityTransitionElapsed = GravityTransitionDuration;
	}
}

void ACustomGravityCharacter::AdvanceGravityDirectionTransition(float DeltaTime)
{
	if (!bGravityDirectionInitialized ||
		GravityTransitionElapsed >= GravityTransitionDuration)
	{
		return;
	}

	if (GravityTransitionDuration <= UE_SMALL_NUMBER)
	{
		CurrentGravityDir = GravityTransitionTargetDir;
		GravityTransitionElapsed = GravityTransitionDuration;
		return;
	}

	GravityTransitionElapsed = FMath::Min(
		GravityTransitionElapsed + FMath::Max(DeltaTime, 0.0f),
		GravityTransitionDuration);
	const float LinearAlpha = GravityTransitionElapsed / GravityTransitionDuration;
	// Quintic smootherstep keeps both angular velocity and acceleration continuous
	// at the ends, which avoids a visible catch when the new gravity frame settles.
	const float SmoothAlpha = LinearAlpha * LinearAlpha * LinearAlpha *
		(LinearAlpha * (LinearAlpha * 6.0f - 15.0f) + 10.0f);
	const FQuat FullRotation = FQuat::FindBetweenNormals(
		GravityTransitionStartDir, GravityTransitionTargetDir);
	CurrentGravityDir = FQuat::Slerp(FQuat::Identity, FullRotation, SmoothAlpha)
		.RotateVector(GravityTransitionStartDir).GetSafeNormal();

	if (GravityTransitionElapsed >= GravityTransitionDuration)
	{
		CurrentGravityDir = GravityTransitionTargetDir;
	}
}

void ACustomGravityCharacter::AdvanceGravityStrengthTransition(float DeltaTime)
{
	if (!bGravityCaptureBlendActive || bIsZeroG)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		bGravityCaptureBlendActive = false;
		return;
	}

	if (GravityCaptureBlendDuration <= UE_SMALL_NUMBER)
	{
		Movement->GravityScale = 1.0f;
		bGravityCaptureBlendActive = false;
		return;
	}

	GravityCaptureBlendElapsed = FMath::Min(
		GravityCaptureBlendElapsed + FMath::Max(DeltaTime, 0.0f),
		GravityCaptureBlendDuration);
	const float LinearAlpha = GravityCaptureBlendElapsed / GravityCaptureBlendDuration;
	const float SmoothAlpha = LinearAlpha * LinearAlpha * LinearAlpha *
		(LinearAlpha * (LinearAlpha * 6.0f - 15.0f) + 10.0f);
	Movement->GravityScale = SmoothAlpha;

	if (GravityCaptureBlendElapsed >= GravityCaptureBlendDuration)
	{
		Movement->GravityScale = 1.0f;
		bGravityCaptureBlendActive = false;
	}
}

bool ACustomGravityCharacter::HasSurfaceGravitySupport(const FVector& GravityDirection)
{
	if (!GravityTarget || !GetWorld() || GravityDirection.IsNearlyZero())
	{
		return false;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement && Movement->IsMovingOnGround())
	{
		return true;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float CapsuleHalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.f;
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 42.f;
	const float ProbeRadius = FMath::Min(SurfaceGravityProbeRadius, CapsuleRadius * 0.8f);

	const FVector Start = GetActorLocation();

	// Rio 06.10 (night perf R7): an airborne walker aboard (a jump, the fall after getting up) swept 1 km down every
	// frame. A floor found under it is trusted for aps.Character.SurfaceSupportCacheS (0.1 s, below the 0.2 s loss
	// grace), measured in the gravity source's own frame so a moving or turning ship keeps it. Only a found floor is
	// reused: a miss sweeps again next frame (a gap crossed in a jump resets the grace as before), zero-G sweeps every
	// frame (gravity comes back at once), and another source, a turn of the down or a 5 m teleport of the walker sweep
	// fresh. At worst a walker stepping off a deck into space turns zero-G 0.1 s later.
	const float SupportCacheSeconds = APSCharacterQueryThrottle::CVarSurfaceSupportCacheS.GetValueOnGameThread();
	APSCharacterQueryThrottle::FQueryTimes* SupportTimes = SupportCacheSeconds > 0.0f
		? &APSCharacterQueryThrottle::QueryTimes.FindOrAdd(FObjectKey(this)) : nullptr;
	const FTransform SourceTransform = GravityTarget->GetActorTransform();
	const FVector LocalStart = SourceTransform.InverseTransformPositionNoScale(Start);
	const FVector LocalDirection = SourceTransform.InverseTransformVectorNoScale(GravityDirection.GetSafeNormal());
	const double Now = GetWorld()->GetTimeSeconds();
	if (SupportTimes && !bIsZeroG && SupportTimes->SupportFoundSeconds >= 0.0
		&& Now - SupportTimes->SupportFoundSeconds >= 0.0 && Now - SupportTimes->SupportFoundSeconds < SupportCacheSeconds
		&& SupportTimes->SupportSource.Get() == GravityTarget
		&& FVector::DistSquared(LocalStart, SupportTimes->SupportLocalStart) < FMath::Square(500.0)
		&& FVector::DotProduct(LocalDirection, SupportTimes->SupportLocalDirection) > 0.996)
	{
		return true;
	}

	const FVector End = Start + GravityDirection.GetSafeNormal() *
		(CapsuleHalfHeight + SurfaceGravityAcquisitionDistance);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSSurfaceGravityProbe), false, this);
	// A shape the probe starts inside is no floor (an interaction or trigger volume round the pilot).
	QueryParams.bFindInitialOverlaps = false;
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(ProbeRadius), QueryParams);

	const FVector GravityUp = -GravityDirection.GetSafeNormal();
	const float WalkableFloorZ = Movement ? Movement->GetWalkableFloorZ() : 0.7f;
	const bool bSupported = bHit && FVector::DotProduct(Hit.ImpactNormal.GetSafeNormal(), GravityUp) >= WalkableFloorZ;
	if (SupportTimes)
	{
		SupportTimes->SupportFoundSeconds = bSupported ? Now : -1.0;
		SupportTimes->SupportSource = GravityTarget;
		SupportTimes->SupportLocalStart = LocalStart;
		SupportTimes->SupportLocalDirection = LocalDirection;
	}
	return bSupported;
}

void ACustomGravityCharacter::UpdateCameraReferenceFrame()
{
	const FVector GravityUp = GetGravityUpVector();

	if (!CameraReferenceUp.Equals(GravityUp, KINDA_SMALL_NUMBER))
	{
		CameraForwardOnGravityPlane = FQuat::FindBetweenNormals(
			CameraReferenceUp, GravityUp).RotateVector(CameraForwardOnGravityPlane);
		CameraReferenceUp = GravityUp;
	}

	CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
		CameraForwardOnGravityPlane, GravityUp).GetSafeNormal();
	if (CameraForwardOnGravityPlane.IsNearlyZero())
	{
		CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
			GetActorForwardVector(), GravityUp).GetSafeNormal();
	}
	if (CameraForwardOnGravityPlane.IsNearlyZero())
	{
		CameraForwardOnGravityPlane = FVector::VectorPlaneProject(
			FVector::ForwardVector, GravityUp).GetSafeNormal();
	}
}

FVector ACustomGravityCharacter::GetCameraPlanarForward() const
{
	const FVector GravityUp = GetGravityUpVector();
	FVector Forward = FVector::VectorPlaneProject(
		CameraForwardOnGravityPlane, GravityUp).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::VectorPlaneProject(
			GetActorForwardVector(), GravityUp).GetSafeNormal();
	}
	return Forward;
}

FQuat ACustomGravityCharacter::GetCameraViewRotation() const
{
	if (bIsZeroG && bZeroGViewRotationInitialized)
	{
		return ZeroGViewRotation;
	}

	const FVector GravityUp = GetGravityUpVector();
	const FVector PlanarForward = GetCameraPlanarForward();
	const FVector CameraRight = FVector::CrossProduct(
		GravityUp, PlanarForward).GetSafeNormal();
	const FQuat PitchQuat(CameraRight, FMath::DegreesToRadians(CameraPitch));
	const FVector ViewForward = PitchQuat.RotateVector(PlanarForward).GetSafeNormal();
	return FRotationMatrix::MakeFromXZ(ViewForward, GravityUp).ToQuat();
}

void ACustomGravityCharacter::SynchronizeCharacterToCamera(float DeltaTime)
{
	const FQuat TargetRotation = bIsZeroG
		? GetCameraViewRotation()
		: FRotationMatrix::MakeFromXZ(
			GetCameraPlanarForward(), GetGravityUpVector()).ToQuat();

	if (ZeroGOrientationTransitionRemaining > UE_SMALL_NUMBER && DeltaTime > 0.0f)
	{
		const float Alpha = FMath::Clamp(
			DeltaTime / ZeroGOrientationTransitionRemaining, 0.0f, 1.0f);
		SetActorRotation(FQuat::Slerp(GetActorQuat(), TargetRotation, Alpha));
		ZeroGOrientationTransitionRemaining = FMath::Max(
			0.0f, ZeroGOrientationTransitionRemaining - DeltaTime);
	}
	else if (ZeroGOrientationTransitionRemaining <= UE_SMALL_NUMBER)
	{
		SetActorRotation(TargetRotation);
	}
}

void ACustomGravityCharacter::AlignCameraToGravity(float DeltaTime)
{
	(void)DeltaTime;
	UpdateCameraReferenceFrame();
	const FQuat TargetRotation = GetCameraViewRotation();

	// Camera and character share one heading; no spring-arm yaw lag can make W
	// disagree with what the player sees.
	CameraBoom->SetWorldRotation(TargetRotation);
}

// ──────────────────────── Public API ────────────────────────

void ACustomGravityCharacter::NormalizeThirdPersonCameraRig()
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float CharacterHalfHeight = Capsule
		? FMath::Max(Capsule->GetUnscaledCapsuleHalfHeight(), 40.0f)
		: 88.0f;
	const float MinimumArmLength = CharacterHalfHeight * 2.5f;
	const float MaximumArmLength = CharacterHalfHeight * 5.0f;
	const float RequestedArmLength = FMath::IsFinite(CameraBoomLength)
		? CameraBoomLength : CharacterHalfHeight * 3.25f;
	const float NormalizedArmLength = FMath::Clamp(
		RequestedArmLength, MinimumArmLength, MaximumArmLength);

	// Inherited Blueprint component defaults predate the full-scale handoff and
	// may contain stale scale/offset/arm values. Re-establish a character-sized
	// third-person rig after Blueprint BeginPlay, while retaining the absolute
	// rotation used by the custom-gravity reference frame.
	CameraBoomLength = NormalizedArmLength;
	CameraBoom->SetRelativeScale3D(FVector::OneVector);
	CameraBoom->SetRelativeLocation(FVector(0.0, 0.0, CharacterHalfHeight * 0.65f));
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FVector::ZeroVector;
	CameraBoom->TargetArmLength = NormalizedArmLength;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetAbsolute(false, true, false);
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagMaxDistance = FMath::Max(
		CharacterHalfHeight, NormalizedArmLength * 0.75f);

	FollowCamera->SetRelativeLocationAndRotation(
		FVector::ZeroVector, FRotator::ZeroRotator);
	FollowCamera->SetRelativeScale3D(FVector::OneVector);
	FollowCamera->bUsePawnControlRotation = false;

	UE_LOG(LogTemp, Display,
		TEXT("[APS.CameraRig] character=%s capsuleHalfHeight=%.2fcm arm=%.2fcm maxLag=%.2fcm boomScale=%s cameraScale=%s"),
		*GetName(), CharacterHalfHeight, CameraBoom->TargetArmLength,
		CameraBoom->CameraLagMaxDistance,
		*CameraBoom->GetRelativeScale3D().ToCompactString(),
		*FollowCamera->GetRelativeScale3D().ToCompactString());
}

void ACustomGravityCharacter::SetGravityTarget(AActor* NewTarget)
{
	GravityTarget = NewTarget;
	bManualGravityOverride = IsValid(NewTarget);
	UpdateGravityDirection(0.0f);
}

void ACustomGravityCharacter::SetSurfaceHandoffSuspended(const bool bSuspended)
{
	if (bSurfaceHandoffSuspended == bSuspended)
	{
		if (bSuspended)
		{
			if (UCharacterMovementComponent* Movement = GetCharacterMovement())
			{
				Movement->StopMovementImmediately();
				Movement->DisableMovement();
			}
		}
		return;
	}

	bSurfaceHandoffSuspended = bSuspended;
	if (AController* OwningController = GetController())
	{
		OwningController->SetIgnoreMoveInput(bSuspended);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		if (bSuspended)
		{
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetMovementMode(bIsZeroG ? MOVE_Flying : MOVE_Falling);
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("[APS.Civilization.SurfaceSpawn] character streaming hold=%s character=%s location=%s"),
		bSuspended ? TEXT("ON") : TEXT("OFF"), *GetName(),
		*GetActorLocation().ToCompactString());
}

void ACustomGravityCharacter::SetViewDirection(const FVector& Forward, const float PitchUpDegrees)
{
	const FVector GravityUp = GetGravityUpVector();
	const FVector Planar = FVector::VectorPlaneProject(Forward, GravityUp).GetSafeNormal();
	if (Planar.IsNearlyZero())
	{
		return;
	}
	// The camera keeps its own heading on the gravity plane and the character follows it every tick
	// (SynchronizeCharacterToCamera); a positive CameraPitch looks down.
	CameraForwardOnGravityPlane = Planar;
	CameraReferenceUp = GravityUp;
	CameraPitch = FMath::Clamp(-PitchUpDegrees, -80.f, 80.f);
	SetActorRotation(FRotationMatrix::MakeFromXZ(Planar, GravityUp).ToQuat(), ETeleportType::TeleportPhysics);
	if (CameraBoom)
	{
		CameraBoom->SetWorldRotation(GetCameraViewRotation());
	}
}

void ACustomGravityCharacter::SetCustomGravityDirection(const FVector& NewDirection)
{
	bUseCustomGravity = true;
	bManualGravityOverride = true;
	GravityTarget = nullptr;
	DefaultGravityDirection = NewDirection.GetSafeNormal();
	UpdateGravityDirection(0.0f);
}

void ACustomGravityCharacter::SetZeroGravityEnabled(bool bEnabled)
{
	const bool bStateChanged = bIsZeroG != bEnabled;
	const FQuat PreviousViewRotation = GetCameraViewRotation();
	bIsZeroG = bEnabled;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}
	if (!bStateChanged)
	{
		return;
	}

	if (bIsZeroG)
	{
		ZeroGViewRotation = PreviousViewRotation.GetNormalized();
		bZeroGViewRotationInitialized = true;
	}
	else if (bZeroGViewRotationInitialized)
	{
		const FVector TargetGravityUp = DesiredGravityDir.IsNearlyZero()
			? GetGravityUpVector()
			: -DesiredGravityDir.GetSafeNormal();
		const FVector ViewForward = PreviousViewRotation.GetForwardVector().GetSafeNormal();
		FVector PlanarForward = FVector::VectorPlaneProject(
			ViewForward, TargetGravityUp).GetSafeNormal();
		if (PlanarForward.IsNearlyZero())
		{
			PlanarForward = FVector::VectorPlaneProject(
				PreviousViewRotation.GetUpVector(), TargetGravityUp).GetSafeNormal();
		}
		if (!PlanarForward.IsNearlyZero())
		{
			CameraForwardOnGravityPlane = PlanarForward;
			CameraReferenceUp = TargetGravityUp;
			const float VerticalViewAmount = FMath::Clamp(
				FVector::DotProduct(ViewForward, TargetGravityUp), -1.0f, 1.0f);
			CameraPitch = FMath::Clamp(
				-FMath::RadiansToDegrees(FMath::Asin(VerticalViewAmount)),
				-80.0f, 80.0f);
		}
		bZeroGViewRotationInitialized = false;
	}

	if (bIsZeroG)
	{
		bGravityCaptureBlendActive = false;
		GravityCaptureBlendElapsed = 0.0f;
		if (!GravityDetector || !IsValid(GravityDetector->GravityTargetActor))
		{
			CurrentGravityType = EGravityType::ZeroG;
		}
		Movement->GravityScale = 0.0f;
		Movement->bOrientRotationToMovement = false;
		Movement->MaxFlySpeed = ZeroGMaxSpeed;
		Movement->MaxAcceleration = ZeroGAcceleration;
		Movement->BrakingDecelerationFlying = ZeroGBrakingDeceleration;
		Movement->SetMovementMode(MOVE_Flying);
	}
	else
	{
		if (bStateChanged)
		{
			GravityCaptureBlendElapsed = 0.0f;
			bGravityCaptureBlendActive = GravityCaptureBlendDuration > UE_SMALL_NUMBER;
			Movement->GravityScale = bGravityCaptureBlendActive ? 0.0f : 1.0f;
		}
		else if (!bGravityCaptureBlendActive)
		{
			Movement->GravityScale = 1.0f;
		}
		Movement->bOrientRotationToMovement = true;
		Movement->MaxAcceleration = 2048.0f;
		if (Movement->MovementMode == MOVE_Flying)
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
	}

	if (bStateChanged)
	{
		ZeroGOrientationTransitionRemaining = bEnabled
			? GravityTransitionDuration
			: 0.0f;
		ApplyAnimationMode();
		UpdateGravityAnimationParameters();
	}
}

void ACustomGravityCharacter::SetManualZeroGOverride(bool bEnabled)
{
	if (bManualZeroGOverride == bEnabled)
	{
		return;
	}

	bManualZeroGOverride = bEnabled;
	if (bManualZeroGOverride)
	{
		SetZeroGravityEnabled(true);
	}
	else
	{
		if (GravityDetector)
		{
			GravityDetector->RunGravityCheck(this);
		}
		UpdateGravityDirection(0.0f);
	}

	UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] ManualZeroG=%s character=%s source=%s"),
		bManualZeroGOverride ? TEXT("ON") : TEXT("OFF"),
		*GetName(), *GetNameSafe(GravityDetector ? GravityDetector->GravityTargetActor : nullptr));
}

void ACustomGravityCharacter::ToggleManualZeroGOverride()
{
	// G is the ship's engine key: a press carried over from the cockpit must not switch gravity off on the ground.
	if (!bManualZeroGOverride && GetWorld() && GetWorld()->GetTimeSeconds() - VehicleExitSeconds < 3.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] ManualZeroG ignored: %.1f s after leaving a vehicle"),
			GetWorld()->GetTimeSeconds() - VehicleExitSeconds);
		return;
	}
	SetManualZeroGOverride(!bManualZeroGOverride);
}

void ACustomGravityCharacter::ApplyAnimationMode()
{
	if (!GetMesh())
	{
		return;
	}

	UClass* DesiredAnimationClass = bIsZeroG
		? ZeroGAnimationClass.Get()
		: SurfaceAnimationClass.Get();
	if (!DesiredAnimationClass || GetMesh()->GetAnimClass() == DesiredAnimationClass)
	{
		return;
	}

	GetMesh()->SetAnimInstanceClass(DesiredAnimationClass);
	GetMesh()->GlobalAnimRateScale = 1.0f;
	CachedAnimationInstance.Reset();
	AnimationPropertyCache.Reset();
	UE_LOG(LogTemp, Warning, TEXT("[APS.Animation] character=%s mode=%s animClass=%s"),
		*GetName(), bIsZeroG ? TEXT("ZeroG") : TEXT("Surface"),
		*GetNameSafe(DesiredAnimationClass));
}

void ACustomGravityCharacter::UpdateGravityAnimationParameters()
{
	if (!GetMesh())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}
	if (CachedAnimationInstance.Get() != AnimInstance)
	{
		CachedAnimationInstance = AnimInstance;
		AnimationPropertyCache.Reset();
	}

	const auto FindAnimationProperty = [this, AnimInstance](const FName PropertyName) -> FProperty*
	{
		if (FProperty** CachedProperty = AnimationPropertyCache.Find(PropertyName))
		{
			return *CachedProperty;
		}

		FProperty* Property = FindFProperty<FProperty>(AnimInstance->GetClass(), PropertyName);
		AnimationPropertyCache.Add(PropertyName, Property);
		return Property;
	};

	const FVector LocalVelocity = GetActorQuat().UnrotateVector(GetVelocity());
	const FVector LocalAcceleration = GetCharacterMovement()
		? GetActorQuat().UnrotateVector(GetCharacterMovement()->GetCurrentAcceleration())
		: FVector::ZeroVector;
	const float GroundSpeed = FVector(LocalVelocity.X, LocalVelocity.Y, 0.0).Size();
	const float MovementDirection = FMath::RadiansToDegrees(
		FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	const auto SetNumericProperty = [AnimInstance, &FindAnimationProperty](const FName PropertyName, const double Value)
	{
		if (FNumericProperty* Property = CastField<FNumericProperty>(
			FindAnimationProperty(PropertyName)))
		{
			void* ValueAddress = Property->ContainerPtrToValuePtr<void>(AnimInstance);
			if (Property->IsFloatingPoint())
			{
				Property->SetFloatingPointPropertyValue(ValueAddress, Value);
			}
			else
			{
				Property->SetIntPropertyValue(ValueAddress, static_cast<int64>(Value));
			}
		}
	};
	const auto SetBoolProperty = [AnimInstance, &FindAnimationProperty](const FName PropertyName, const bool Value)
	{
		if (FBoolProperty* Property = CastField<FBoolProperty>(
			FindAnimationProperty(PropertyName)))
		{
			Property->SetPropertyValue_InContainer(AnimInstance, Value);
		}
	};
	const auto SetVectorProperty = [AnimInstance, &FindAnimationProperty](const FName PropertyName, const FVector& Value)
	{
		if (FStructProperty* Property = CastField<FStructProperty>(
			FindAnimationProperty(PropertyName));
			Property && Property->Struct == TBaseStructure<FVector>::Get())
		{
			*Property->ContainerPtrToValuePtr<FVector>(AnimInstance) = Value;
		}
	};
	const EGravityType AnimationGravityType = bIsZeroG
		? EGravityType::ZeroG
		: CurrentGravityType;
	const auto SetGravityTypeProperty = [AnimInstance, AnimationGravityType, &FindAnimationProperty](const FName PropertyName)
	{
		const int64 GravityTypeValue = static_cast<int64>(AnimationGravityType);
		if (FEnumProperty* Property = CastField<FEnumProperty>(
			FindAnimationProperty(PropertyName)))
		{
			void* ValueAddress = Property->ContainerPtrToValuePtr<void>(AnimInstance);
			Property->GetUnderlyingProperty()->SetIntPropertyValue(ValueAddress, GravityTypeValue);
		}
		else if (FByteProperty* ByteProperty = CastField<FByteProperty>(
			FindAnimationProperty(PropertyName)))
		{
			ByteProperty->SetPropertyValue_InContainer(
				AnimInstance, static_cast<uint8>(AnimationGravityType));
		}
	};

	SetNumericProperty(TEXT("ForwardSpeed"), LocalVelocity.X);
	SetNumericProperty(TEXT("RightSpeed"), LocalVelocity.Y);
	SetNumericProperty(TEXT("UpSpeed"), LocalVelocity.Z);
	// The existing Zero-G BlendSpace uses Sides on X and Forward on Y, with
	// authored samples at +/-2500 cm/s. Keep raw local speeds so lateral flight
	// reaches the left/right poses instead of collapsing into the forward pose.
	SetNumericProperty(TEXT("Sides"), LocalVelocity.Y);
	SetNumericProperty(TEXT("Forward"), LocalVelocity.X);
	SetNumericProperty(TEXT("Speed"), LocalVelocity.Size());
	SetNumericProperty(TEXT("GroundSpeed"), GroundSpeed);
	SetNumericProperty(TEXT("Direction"), MovementDirection);
	SetNumericProperty(TEXT("MovementDirection"), MovementDirection);
	SetNumericProperty(TEXT("Velocity_X"), LocalVelocity.X);
	SetNumericProperty(TEXT("Velocity_Y"), LocalVelocity.Y);
	SetNumericProperty(TEXT("Velocity_Z"), LocalVelocity.Z);
	SetVectorProperty(TEXT("Velocity"), LocalVelocity);
	SetVectorProperty(TEXT("Acceleration"), LocalAcceleration);
	SetVectorProperty(TEXT("CurrentAcceleration"), LocalAcceleration);
	SetBoolProperty(TEXT("ZeroG"), bIsZeroG);
	SetBoolProperty(TEXT("bIsZeroG"), bIsZeroG);
	SetBoolProperty(TEXT("ShouldMove"), GroundSpeed > 3.0f);
	SetBoolProperty(TEXT("Moving"), GroundSpeed > 3.0f);
	SetBoolProperty(TEXT("Sprinting"), bSprintHeld);
	SetBoolProperty(TEXT("bIsSprinting"), bSprintHeld);
	const bool bIsFalling = GetCharacterMovement() && GetCharacterMovement()->IsFalling();
	SetBoolProperty(TEXT("Falling"), bIsFalling);
	SetBoolProperty(TEXT("IsFalling"), bIsFalling);
	SetGravityTypeProperty(TEXT("GravityType"));
	SetGravityTypeProperty(TEXT("CurrentGravityType"));

	float TargetPlayRate = 1.0f;
	if (bSprintHeld)
	{
		if (bIsZeroG && ZeroGSprintSpeed > UE_SMALL_NUMBER)
		{
			const float SpeedRatio = LocalVelocity.Size() / ZeroGSprintSpeed;
			TargetPlayRate = FMath::Clamp(
				1.0f + (SpeedRatio - 1.0f) * 0.35f, 1.0f, 1.5f);
		}
		else if (!bIsZeroG && SurfaceWalkSpeed > UE_SMALL_NUMBER)
		{
			const float SpeedRatio = GroundSpeed / SurfaceWalkSpeed;
			TargetPlayRate = FMath::Clamp(
				1.0f + (SpeedRatio - 1.0f) * 0.55f, 1.0f, 1.85f);
		}
	}
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	GetMesh()->GlobalAnimRateScale = FMath::FInterpTo(
		GetMesh()->GlobalAnimRateScale, TargetPlayRate, DeltaTime, 7.0f);
}

void ACustomGravityCharacter::SetBase(UPrimitiveComponent* NewBase, const FName BoneName, bool bNotifyActor)
{
	const ASpaceship* Ship = AboardShip.Get();
	if (Ship && NewBase && NewBase->GetOwner() == Ship)
	{
		NewBase = nullptr;
	}
	Super::SetBase(NewBase, BoneName, bNotifyActor);
}

void ACustomGravityCharacter::UpdateShipPassenger()
{
	ASpaceship* Ship = GravityDetector && GravityDetector->CurrentGravityType == EGravityType::OnShip
		? Cast<ASpaceship>(GravityDetector->GravityTargetActor) : nullptr;
	if (AboardShip.IsValid() && AboardShip.Get() != Ship)
	{
		LeaveShip();
	}
	// Also after the seat: leaving the controls detaches the pilot, and aboard it is attached again.
	if (Ship && (AboardShip.Get() != Ship || GetAttachParentActor() != Ship))
	{
		BoardShip(*Ship);
	}
	if (const ASpaceship* Current = AboardShip.Get())
	{
		const FQuat ShipQuat = Current->GetActorQuat();
		const FQuat Delta = ShipQuat * AboardShipLastQuat.Inverse();
		AboardShipLastQuat = ShipQuat;
		if (!Delta.Equals(FQuat::Identity, 1.0e-9))
		{
			// The view turns with the deck: its heading, the up it was measured against and the zero-G view.
			CameraForwardOnGravityPlane = Delta.RotateVector(CameraForwardOnGravityPlane);
			CameraReferenceUp = Delta.RotateVector(CameraReferenceUp);
			ZeroGViewRotation = Delta * ZeroGViewRotation;
		}
	}
}

void ACustomGravityCharacter::BoardShip(ASpaceship& Ship)
{
	const bool bAlreadyAboard = AboardShip.Get() == &Ship;
	AboardShip = &Ship;
	AboardShipLastQuat = Ship.GetActorQuat();
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// From now on the deck's motion comes with the attachment: a walking speed is kept, a flight speed (getting up
		// from the seat in flight) is not.
		if (Movement->Velocity.Size() > Movement->MaxWalkSpeed * 1.5f)
		{
			Movement->Velocity = FVector::ZeroVector;
		}
		SetBase(nullptr);
	}
	AttachToComponent(Ship.GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	if (UPrimitiveComponent* Hull = Cast<UPrimitiveComponent>(Ship.GetRootComponent()))
	{
		Hull->IgnoreActorWhenMoving(this, true);
	}
	if (CameraBoom && !bAlreadyAboard)
	{
		// World-space lag would leave the camera behind a moving deck.
		bAboardSavedCameraLag = CameraBoom->bEnableCameraLag;
		bAboardSavedCameraRotationLag = CameraBoom->bEnableCameraRotationLag;
		CameraBoom->bEnableCameraLag = false;
		CameraBoom->bEnableCameraRotationLag = false;
	}
	if (!bAlreadyAboard)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Gravity] aboard %s: attached, walking in the ship's frame"), *Ship.GetName());
	}
}

void ACustomGravityCharacter::LeaveShip()
{
	// Rio 06.10 (still ship): a ship owing its travel pays it while the walker is still aboard.
	if (AboardShip.IsValid())
	{
		if (UAPSWorldOriginSubsystem* Origin = GetWorld() ? GetWorld()->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr)
		{
			Origin->SettleDeferredTravel(TEXT("a walker leaves the ship"));
		}
	}
	ASpaceship* Ship = AboardShip.Get();
	AboardShip.Reset();
	if (!Ship)
	{
		return;
	}
	if (UPrimitiveComponent* Hull = Cast<UPrimitiveComponent>(Ship->GetRootComponent()))
	{
		Hull->IgnoreActorWhenMoving(this, false);
	}
	if (GetAttachParentActor() == Ship)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (CameraBoom)
	{
		CameraBoom->bEnableCameraLag = bAboardSavedCameraLag;
		CameraBoom->bEnableCameraRotationLag = bAboardSavedCameraRotationLag;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Gravity] left %s"), *Ship->GetName());
}

void ACustomGravityCharacter::SettleAfterVehicleExit(const FVector& Facing, AActor* LeftVehicle)
{
	// Rio 06.10 (night perf R7): the walker stands up somewhere new: its first support sweep and prompt trace run fresh.
	APSCharacterQueryThrottle::Forget(*this);
	// F3 (Rio, 02.10: "disembarking on a planet ignores its gravity"): a zero-G toggled with G before boarding must
	// not outlive the flight. The place of exit decides (A3); in empty space that is zero-G anyway.
	if (bManualZeroGOverride)
	{
		bManualZeroGOverride = false;
		UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] ManualZeroG=OFF on vehicle exit character=%s"), *GetName());
	}
	// Rio 03.10: the surface spawn's pin to its planet (SetGravityTarget) ends with the first ride: where the character
	// gets out (a station, a ship's deck, another world) the detector decides, not the planet it started on.
	if (bManualGravityOverride && GravityTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] spawn pin to %s released on vehicle exit"), *GetNameSafe(GravityTarget));
		bManualGravityOverride = false;
	}
	// Rio 05.10 (REAL SCALE: getting up at billions of c, "source=None" and the pilot was left light years behind): a
	// seated pilot's cached overlaps are empty at such speeds, so the detector found no deck to stand on and the next
	// world shift carried the free character away with the rest of the world. The ship just left is known: inside its
	// gravity sphere the character goes aboard now (attached, it rides every shift with the ship); the detector below
	// keeps it there by the same sphere, or lets it go for a better source as before.
	ASpaceship* LeftShip = Cast<ASpaceship>(LeftVehicle);
	if (LeftShip && LeftShip->ProvidesShipGravity() && LeftShip->SphereCollisionComponent
		&& FVector::DistSquared(GetActorLocation(), LeftShip->SphereCollisionComponent->GetComponentLocation())
			<= FMath::Square(LeftShip->SphereCollisionComponent->GetScaledSphereRadius()))
	{
		BoardShip(*LeftShip);
	}
	// Rio 09.10 (playtest 08.10 item 3, aps.Character.ExitClearsZeroG): a pilot who sat weightless (it floated into the seat, or
	// its detector lost every source while it sat) stands up on the deck with the normal 0.2 s support grace instead of
	// UpdateGravityDirection's at-once zero-G; the G override is never touched.
	if (AboardShip.IsValid() && bIsZeroG && !bManualZeroGOverride
		&& APSCharacterQueryThrottle::CVarExitClearsZeroG.GetValueOnGameThread() != 0)
	{
		SetZeroGravityEnabled(false);
	}
	// While seated the character did not tick: its gravity frame is the one it boarded in, possibly another body.
	if (GravityDetector)
	{
		GravityDetector->RunGravityCheckForActor(this);
	}
	// Rio 04.10 evening ("on autopilot I got up a second time and the ship flew off; the character was left in space"):
	// at cruise a ship covers hundreds of thousands of km a frame, and boarding waited for the character's next tick,
	// after the ship had moved on: the pilot was attached that far behind it and its gravity let go. A deck that holds
	// the pilot now takes it along from the moment it stands up, at rest on the deck (the attachment brings the ship's
	// own motion).
	UpdateShipPassenger();
	if (AboardShip.IsValid())
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
	}
	bGravityDirectionInitialized = false;
	UpdateGravityDirection(0.0f);
	VehicleExitSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -100.0;
	bExitRiseLogged = false;
	{
		const AActor* Source = GravityDetector ? GravityDetector->GravityTargetActor : nullptr;
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Gravity] Exit settle character=%s source=%s type=%d dir=%s toSource=%s velocity=%s movementMode=%d"),
			*GetName(), *GetNameSafe(Source), static_cast<int32>(CurrentGravityType), *CurrentGravityDir.ToCompactString(),
			Source ? *(Source->GetActorLocation() - GetActorLocation()).GetSafeNormal().ToCompactString() : TEXT("-"),
			*GetVelocity().ToCompactString(),
			GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode.GetValue()) : -1);
	}
	if (!bIsZeroG && !CurrentGravityDir.IsNearlyZero())
	{
		const FVector Up = -CurrentGravityDir.GetSafeNormal();
		FVector Forward = FVector::VectorPlaneProject(Facing, Up).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			Forward = FVector::VectorPlaneProject(GetActorForwardVector(), Up).GetSafeNormal();
		}
		if (!Forward.IsNearlyZero())
		{
			SetActorRotation(FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat(), ETeleportType::TeleportPhysics);
		}
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(bIsZeroG ? MOVE_Flying : MOVE_Falling);
	}
}

void ACustomGravityCharacter::HandleGravitySourceChanged(AActor* NewSource)
{
	if (bManualGravityOverride)
	{
		return;
	}

	GravityTarget = NewSource;
	UpdateGravityDirection(0.0f);
}

FVector ACustomGravityCharacter::GetCurrentGravityDirection() const
{
	return CurrentGravityDir;
}

FVector ACustomGravityCharacter::GetGravityUpVector() const
{
	return -CurrentGravityDir;
}

// ──────────────────────── Build mode ────────────────────────

namespace APSCharacterBuildLocal
{
	constexpr float RotateStepDegrees = 15.f;
	constexpr float FineRotateStepDegrees = 5.f;
	constexpr float CameraBlendSeconds = 0.45f;
	/** Mouse travel during a right press below which it is a click (drop the selection), not a camera drag. */
	constexpr float RightClickDragLimit = 6.f;
	constexpr float MinBuildArmLength = 800.f;
	constexpr float MaxBuildArmLength = 4500.f;
	constexpr float ArmLengthStep = 250.f;

	/** 1-9, then 0 for the tenth card; INDEX_NONE for other keys. */
	int32 SlotOfKey(const FKey& Key)
	{
		const FKey SlotKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven,
			EKeys::Eight, EKeys::Nine, EKeys::Zero};
		for (int32 Slot = 0; Slot < static_cast<int32>(UE_ARRAY_COUNT(SlotKeys)); ++Slot)
		{
			if (SlotKeys[Slot] == Key)
			{
				return Slot;
			}
		}
		return INDEX_NONE;
	}

	APSConstruction::FFrame MakeFrame(const ACustomGravityCharacter& Character, const UGravityDetectorComponent* Detector,
		const float RadiusCm)
	{
		APSConstruction::FFrame Frame;
		Frame.Site = Detector ? Cast<APlanetaryBody>(Detector->GravityTargetActor) : nullptr;
		Frame.BuilderLocation = Character.GetActorLocation();
		Frame.BuilderUp = Character.GetGravityUpVector();
		const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
		Frame.BuilderFeetCm = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0;
		Frame.RadiusCm = RadiusCm;
		Frame.Placement = APSConstruction::EPlacement::Surface;
		return Frame;
	}

	/** The colony terminal or the strategic map holds the input with its own widget. */
	bool IsOtherScreenOpen(const UWorld* World, const APlayerController* PlayerController)
	{
		const UAPSColonyTerminalSubsystem* Terminal = World ? World->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		const AGravityPlayerController* GravityController = Cast<AGravityPlayerController>(PlayerController);
		return (Terminal && Terminal->IsTerminalOpen()) || (GravityController && GravityController->IsStrategicMapOpen());
	}

	void ShowBuildCursor(APlayerController& PlayerController)
	{
		// Game and UI: the palette takes its clicks, the viewport keeps the keyboard (WASD walks on) and the world clicks.
		PlayerController.SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController.SetInputMode(InputMode);
	}
}

bool ACustomGravityCharacter::IsInBuildMode() const
{
	return ConstructionMode.IsValid();
}

bool ACustomGravityCharacter::CanBuildHere() const
{
	return !bSurfaceHandoffSuspended && !bIsZeroG && CurrentGravityType == EGravityType::OnPlanet && GravityDetector
		&& Cast<APlanetaryBody>(GravityDetector->GravityTargetActor) != nullptr;
}

void ACustomGravityCharacter::ToggleBuildMode()
{
	if (ConstructionMode.IsValid())
	{
		ExitBuildMode(true);
		return;
	}
	EnterBuildMode();
}

void ACustomGravityCharacter::EnterBuildMode()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	UWorld* LiveWorld = GetWorld();
	if (ConstructionMode.IsValid() || !PlayerController || !LiveWorld || !IsLocallyControlled())
	{
		return;
	}
	// The terminal or the strategic map holds the input with its own widget: B is not for building then.
	if (APSCharacterBuildLocal::IsOtherScreenOpen(LiveWorld, PlayerController))
	{
		return;
	}
	if (!CanBuildHere())
	{
		BuildRefusalText = bSurfaceHandoffSuspended
			? NSLOCTEXT("APSConstruction", "BuildWaitGround", "BUILD MODE: the ground is still loading")
			: NSLOCTEXT("APSConstruction", "BuildNeedsGround", "BUILD MODE: stand on a planet or a moon");
		BuildRefusalUntilSeconds = LiveWorld->GetTimeSeconds() + 3.0;
		return;
	}
	BuildRefusalUntilSeconds = 0.0;

	ConstructionMode = MakeShared<FAPSConstructionMode>(this);
	ConstructionMode->Begin(APSCharacterBuildLocal::MakeFrame(*this, GravityDetector, BuildZoneRadiusCm));

	// Its own keys, on a component the controller puts above the character's: they take the pace keys 1-3 and the
	// zero-G roll on Q/E while building. It is the character's component, so it lives exactly as long as build mode.
	if (UInputComponent* BuildInput = NewObject<UInputComponent>(this))
	{
		const FKey PressedKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six,
			EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero, EKeys::Q, EKeys::E, EKeys::C, EKeys::X, EKeys::Delete,
			EKeys::B, EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::MouseScrollUp, EKeys::MouseScrollDown};
		for (const FKey& PressedKey : PressedKeys)
		{
			BuildInput->BindKey(PressedKey, IE_Pressed, this, &ACustomGravityCharacter::HandleBuildKey);
		}
		BuildInput->BindKey(EKeys::Q, IE_Repeat, this, &ACustomGravityCharacter::HandleBuildKey);
		BuildInput->BindKey(EKeys::E, IE_Repeat, this, &ACustomGravityCharacter::HandleBuildKey);
		BuildInput->BindKey(EKeys::RightMouseButton, IE_Released, this, &ACustomGravityCharacter::HandleBuildKeyReleased);
		BuildInput->RegisterComponent();
		BuildInputComponent = BuildInput;
	}

	if (BuildCameraBlend <= 0.f)
	{
		BuildSavedCameraPitch = CameraPitch;
	}
	bBuildRightHeld = false;
	BuildViewTarget = PlayerController->GetViewTarget();
	APSCharacterBuildLocal::ShowBuildCursor(*PlayerController);
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] %s enters build mode"), *GetName());
}

void ACustomGravityCharacter::ExitBuildMode(const bool bRestoreInput)
{
	const TSharedPtr<FAPSConstructionMode> EndingMode = MoveTemp(ConstructionMode);
	ConstructionMode.Reset();
	if (EndingMode.IsValid())
	{
		EndingMode->End();
	}
	if (UInputComponent* BuildInput = BuildInputComponent.Get())
	{
		// Unbound at once (this may run inside one of its own key events, whose dispatch holds copies); then gone.
		BuildInput->KeyBindings.Reset();
		BuildInput->DestroyComponent();
	}
	BuildInputComponent.Reset();
	BuildViewTarget.Reset();
	bBuildRightHeld = false;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller); bRestoreInput && PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void ACustomGravityCharacter::UpdateBuildMode(const float DeltaTime)
{
	if (!ConstructionMode.IsValid())
	{
		return;
	}
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	// Another screen took the input (the strategic map and its camera, the terminal): leave quietly, that screen gives
	// the game its input back when it closes. A view arriving at the character (a blend from the ship) is not one.
	const AActor* ViewTarget = PlayerController ? PlayerController->GetViewTarget() : nullptr;
	const bool bOtherScreen = PlayerController && ((ViewTarget != this && ViewTarget != BuildViewTarget.Get())
		|| APSCharacterBuildLocal::IsOtherScreenOpen(GetWorld(), PlayerController));
	if (!PlayerController || !IsLocallyControlled() || bOtherScreen || !CanBuildHere() || ConstructionMode->IsExitRequested())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Construction] build mode ends: %s"), !PlayerController ? TEXT("no controller")
			: bOtherScreen ? TEXT("another screen took the input") : !CanBuildHere() ? TEXT("not on a body's ground")
			: TEXT("asked"));
		ExitBuildMode(!bOtherScreen);
		return;
	}
	if (!PlayerController->ShouldShowMouseCursor())
	{
		// Something else hid the cursor (a screen closing in the same frame): build mode needs it.
		APSCharacterBuildLocal::ShowBuildCursor(*PlayerController);
	}
	ConstructionMode->Tick(DeltaTime, APSCharacterBuildLocal::MakeFrame(*this, GravityDetector, BuildZoneRadiusCm));
}

void ACustomGravityCharacter::UpdateBuildCamera(const float DeltaTime)
{
	const bool bBuilding = ConstructionMode.IsValid();
	if ((!bBuilding && BuildCameraBlend <= 0.f) || !CameraBoom)
	{
		return;
	}
	// Back and up over the shoulder to a look down on the zone, and back after; the pitch and length stay the
	// player's while building (right or middle drag, the wheel with nothing picked).
	BuildCameraBlend = FMath::FInterpConstantTo(BuildCameraBlend, bBuilding ? 1.f : 0.f, DeltaTime,
		1.f / APSCharacterBuildLocal::CameraBlendSeconds);
	const float Alpha = BuildCameraBlend * BuildCameraBlend * (3.f - 2.f * BuildCameraBlend);
	CameraBoom->TargetArmLength = FMath::Lerp(CameraBoomLength, BuildArmLength, Alpha);
	CameraPitch = FMath::Lerp(BuildSavedCameraPitch, BuildCameraPitch, Alpha);
}

FText ACustomGravityCharacter::GetBuildHintText() const
{
	if (ConstructionMode.IsValid() || !IsLocallyControlled())
	{
		return FText::GetEmpty();
	}
	const UWorld* LiveWorld = GetWorld();
	if (LiveWorld && LiveWorld->GetTimeSeconds() < BuildRefusalUntilSeconds)
	{
		return BuildRefusalText;
	}
	return CanBuildHere() ? NSLOCTEXT("APSConstruction", "BuildHint", "B  BUILD MODE") : FText::GetEmpty();
}

void ACustomGravityCharacter::HandleBuildKey(const FKey Key)
{
	using namespace APSCharacterBuildLocal;
	// Held for the call: a key may end build mode.
	const TSharedPtr<FAPSConstructionMode> Building = ConstructionMode;
	if (!Building.IsValid())
	{
		return;
	}
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	const bool bFine = PlayerController
		&& (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift));
	const float Step = bFine ? FineRotateStepDegrees : RotateStepDegrees;
	if (const int32 Slot = SlotOfKey(Key); Slot != INDEX_NONE)
	{
		Building->SelectSlot(Slot);
	}
	else if (Key == EKeys::Q)
	{
		Building->Rotate(-Step);
	}
	else if (Key == EKeys::E)
	{
		Building->Rotate(Step);
	}
	else if (Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown)
	{
		const float Sign = Key == EKeys::MouseScrollUp ? 1.f : -1.f;
		if (Building->HasSelection())
		{
			Building->Rotate(Sign * Step);
		}
		else
		{
			BuildArmLength = FMath::Clamp(BuildArmLength - Sign * ArmLengthStep, MinBuildArmLength, MaxBuildArmLength);
		}
	}
	else if (Key == EKeys::LeftMouseButton)
	{
		Building->Place();
	}
	else if (Key == EKeys::RightMouseButton)
	{
		bBuildRightHeld = true;
		BuildRightDragAmount = 0.f;
	}
	else if (Key == EKeys::C)
	{
		Building->ToggleSection();
	}
	else if (Key == EKeys::X || Key == EKeys::Delete)
	{
		Building->RemoveHovered();
	}
	else if (Key == EKeys::B)
	{
		Building->RequestExit();
	}
}

void ACustomGravityCharacter::HandleBuildKeyReleased(const FKey Key)
{
	if (Key != EKeys::RightMouseButton)
	{
		return;
	}
	// A click, not a drag of the camera: drops the selection.
	if (ConstructionMode.IsValid() && bBuildRightHeld && BuildRightDragAmount < APSCharacterBuildLocal::RightClickDragLimit)
	{
		ConstructionMode->CancelSelection();
	}
	bBuildRightHeld = false;
}

void ACustomGravityCharacter::UnPossessed()
{
	if (ConstructionMode.IsValid())
	{
		ExitBuildMode(true);
	}
	// Rio 06.10 (night perf R7): whoever possesses the character next starts with a fresh prompt trace.
	APSCharacterQueryThrottle::Forget(*this);
	Super::UnPossessed();
}
