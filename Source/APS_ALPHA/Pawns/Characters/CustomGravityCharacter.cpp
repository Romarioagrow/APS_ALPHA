// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomGravityCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "APS_ALPHA/Core/Interfaces/VehicleControlling.h"
#include "APS_ALPHA/Pawns/Characters/GravityDetectorComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

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
	MoveComp->MaxWalkSpeed = 600.f;
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
	FollowCamera->bUsePawnControlRotation = false;
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
}

void ACustomGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUseCustomGravity)
	{
		UpdateGravityDirection(DeltaTime);
	}
	AdvanceGravityStrengthTransition(DeltaTime);

	UpdateCameraReferenceFrame();
	if (bIsZeroG)
	{
		SynchronizeCharacterToCamera(DeltaTime);
	}
	UpdateGravityAnimationParameters();
	AlignCameraToGravity(DeltaTime);
	UpdateInteractionCandidate();
	if (!InteractionPromptWidget.IsValid())
	{
		CreateInteractionPrompt();
	}
}

void ACustomGravityCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInteractionPrompt();
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
	PlayerInputComponent->BindAxis("MoveUp", this, &ACustomGravityCharacter::HandleZeroGVertical);
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
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
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
	if (bIsZeroG)
	{
		SynchronizeCharacterToCamera(0.0f);
	}
}

void ACustomGravityCharacter::HandleJumpStarted()
{
	if (bIsZeroG)
	{
		AddMovementInput(GetCameraViewRotation().GetUpVector(), 1.0f);
		return;
	}

	Jump();
}

void ACustomGravityCharacter::HandleJumpCompleted()
{
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

void ACustomGravityCharacter::TryInteract()
{
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
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSCharacterInteraction), false, this);
		if (World->LineTraceSingleByChannel(Hit, Start, Start + Direction * InteractionDistance,
			ECC_Visibility, QueryParams))
		{
			Candidate = ResolveVehicleActor(Hit.GetActor());
		}
	}

	if (!Candidate && GravityDetector)
	{
		Candidate = ResolveVehicleActor(GravityDetector->CurrentSpaceship);
	}
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
		.Padding(0.0f, 0.0f, 0.0f, 72.0f)
		[
			SNew(SBorder)
			.Visibility_Lambda([WeakThis]()
			{
				const ACustomGravityCharacter* Character = WeakThis.Get();
				return Character && Character->IsLocallyControlled() && Character->CurrentInteractableActor.IsValid()
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			.BorderBackgroundColor(FLinearColor(0.01f, 0.025f, 0.045f, 0.9f))
			.Padding(FMargin(18.0f, 9.0f))
			[
				SNew(STextBlock)
				.Text_Lambda([WeakThis]()
				{
					const ACustomGravityCharacter* Character = WeakThis.Get();
					const AActor* Vehicle = Character ? Character->CurrentInteractableActor.Get() : nullptr;
					return Vehicle
						? FText::Format(NSLOCTEXT("APSInteraction", "TakeControl", "F  TAKE CONTROL  /  {0}"), FText::FromString(Vehicle->GetName()))
						: FText::GetEmpty();
				})
				.ColorAndOpacity(FLinearColor(0.2f, 0.82f, 1.0f, 1.0f))
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

void ACustomGravityCharacter::UpdateGravityDirection(float DeltaTime)
{
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
		if (bWasZeroG || bDirectionChanged)
		{
			StartGravityDirectionTransition(DesiredGravityDir, bWasZeroG);
		}
		else if (GravityTransitionElapsed >= GravityTransitionDuration)
		{
			CurrentGravityDir = DesiredGravityDir;
			GravityTransitionTargetDir = DesiredGravityDir;
		}
		else
		{
			GravityTransitionTargetDir = DesiredGravityDir;
		}
	}

	AdvanceGravityDirectionTransition(DeltaTime);

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
	const FVector End = Start + GravityDirection.GetSafeNormal() *
		(CapsuleHalfHeight + SurfaceGravityAcquisitionDistance);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSSurfaceGravityProbe), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(ProbeRadius), QueryParams);
	if (!bHit)
	{
		return false;
	}

	const FVector GravityUp = -GravityDirection.GetSafeNormal();
	const float WalkableFloorZ = Movement ? Movement->GetWalkableFloorZ() : 0.7f;
	if (FVector::DotProduct(Hit.ImpactNormal.GetSafeNormal(), GravityUp) < WalkableFloorZ)
	{
		return false;
	}

	return true;
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

void ACustomGravityCharacter::SetGravityTarget(AActor* NewTarget)
{
	GravityTarget = NewTarget;
	bManualGravityOverride = IsValid(NewTarget);
	UpdateGravityDirection(0.0f);
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
	bIsZeroG = bEnabled;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
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

	const FVector LocalVelocity = GetActorQuat().UnrotateVector(GetVelocity());
	const FVector LocalAcceleration = GetCharacterMovement()
		? GetActorQuat().UnrotateVector(GetCharacterMovement()->GetCurrentAcceleration())
		: FVector::ZeroVector;
	const float GroundSpeed = FVector(LocalVelocity.X, LocalVelocity.Y, 0.0).Size();
	const float MovementDirection = FMath::RadiansToDegrees(
		FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	const auto SetNumericProperty = [AnimInstance](const FName PropertyName, const double Value)
	{
		if (FNumericProperty* Property = FindFProperty<FNumericProperty>(
			AnimInstance->GetClass(), PropertyName))
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
	const auto SetBoolProperty = [AnimInstance](const FName PropertyName, const bool Value)
	{
		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(
			AnimInstance->GetClass(), PropertyName))
		{
			Property->SetPropertyValue_InContainer(AnimInstance, Value);
		}
	};
	const auto SetVectorProperty = [AnimInstance](const FName PropertyName, const FVector& Value)
	{
		if (FStructProperty* Property = FindFProperty<FStructProperty>(
			AnimInstance->GetClass(), PropertyName);
			Property && Property->Struct == TBaseStructure<FVector>::Get())
		{
			*Property->ContainerPtrToValuePtr<FVector>(AnimInstance) = Value;
		}
	};
	const EGravityType AnimationGravityType = bIsZeroG
		? EGravityType::ZeroG
		: CurrentGravityType;
	const auto SetGravityTypeProperty = [AnimInstance, AnimationGravityType](const FName PropertyName)
	{
		const int64 GravityTypeValue = static_cast<int64>(AnimationGravityType);
		if (FEnumProperty* Property = FindFProperty<FEnumProperty>(
			AnimInstance->GetClass(), PropertyName))
		{
			void* ValueAddress = Property->ContainerPtrToValuePtr<void>(AnimInstance);
			Property->GetUnderlyingProperty()->SetIntPropertyValue(ValueAddress, GravityTypeValue);
		}
		else if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(
			AnimInstance->GetClass(), PropertyName))
		{
			ByteProperty->SetPropertyValue_InContainer(
				AnimInstance, static_cast<uint8>(AnimationGravityType));
		}
	};

	SetNumericProperty(TEXT("ForwardSpeed"), LocalVelocity.X);
	SetNumericProperty(TEXT("RightSpeed"), LocalVelocity.Y);
	SetNumericProperty(TEXT("UpSpeed"), LocalVelocity.Z);
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
	const bool bIsFalling = GetCharacterMovement() && GetCharacterMovement()->IsFalling();
	SetBoolProperty(TEXT("Falling"), bIsFalling);
	SetBoolProperty(TEXT("IsFalling"), bIsFalling);
	SetGravityTypeProperty(TEXT("GravityType"));
	SetGravityTypeProperty(TEXT("CurrentGravityType"));
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
