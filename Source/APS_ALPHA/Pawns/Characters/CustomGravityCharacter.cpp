// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomGravityCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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
	UpdateGravityDirection();
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
		UpdateGravityDirection();
	}

	UpdateCameraReferenceFrame();
	if (bIsZeroG)
	{
		SynchronizeCharacterToCamera();
	}
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
		SynchronizeCharacterToCamera();
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

void ACustomGravityCharacter::UpdateGravityDirection()
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

	CurrentGravityDir = NewGravityDir;

	// UE 5.4: Set custom gravity direction on CharacterMovementComponent
	GetCharacterMovement()->SetGravityDirection(CurrentGravityDir);

	const bool bRequiresNearbySurface =
		CurrentGravityType == EGravityType::OnStation ||
		CurrentGravityType == EGravityType::OnShip;
	if (bRequiresNearbySurface && !HasSurfaceGravitySupport(CurrentGravityDir))
	{
		SetZeroGravityEnabled(true);
		return;
	}

	SetZeroGravityEnabled(false);
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

void ACustomGravityCharacter::SynchronizeCharacterToCamera()
{
	const FQuat TargetRotation = bIsZeroG
		? GetCameraViewRotation()
		: FRotationMatrix::MakeFromXZ(
			GetCameraPlanarForward(), GetGravityUpVector()).ToQuat();
	SetActorRotation(TargetRotation);
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
	UpdateGravityDirection();
}

void ACustomGravityCharacter::SetCustomGravityDirection(const FVector& NewDirection)
{
	bUseCustomGravity = true;
	bManualGravityOverride = true;
	GravityTarget = nullptr;
	DefaultGravityDirection = NewDirection.GetSafeNormal();
	UpdateGravityDirection();
}

void ACustomGravityCharacter::SetZeroGravityEnabled(bool bEnabled)
{
	bIsZeroG = bEnabled;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (bIsZeroG)
	{
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
		Movement->GravityScale = 1.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->MaxAcceleration = 2048.0f;
		if (Movement->MovementMode == MOVE_Flying)
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
}

void ACustomGravityCharacter::HandleGravitySourceChanged(AActor* NewSource)
{
	if (bManualGravityOverride)
	{
		return;
	}

	GravityTarget = NewSource;
	UpdateGravityDirection();
}

FVector ACustomGravityCharacter::GetCurrentGravityDirection() const
{
	return CurrentGravityDir;
}

FVector ACustomGravityCharacter::GetGravityUpVector() const
{
	return -CurrentGravityDir;
}
