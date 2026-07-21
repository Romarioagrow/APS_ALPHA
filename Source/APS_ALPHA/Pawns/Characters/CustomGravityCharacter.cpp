// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomGravityCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

ACustomGravityCharacter::ACustomGravityCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate the character from controller input — we handle it ourselves
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure CharacterMovement for custom gravity
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 500.f, 0.f);
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = 600.f;
	MoveComp->JumpZVelocity = 500.f;
	MoveComp->BrakingDecelerationWalking = 2000.f;

	// Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
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
}

void ACustomGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUseCustomGravity)
	{
		UpdateGravityDirection();
	}

	AlignCameraToGravity(DeltaTime);
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
}

// ──────────────────────── Input Handlers ────────────────────────

void ACustomGravityCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// Get the camera's forward and right vectors, projected onto the gravity plane
	const FVector GravityUp = GetGravityUpVector();

	const FRotator CameraWorldRot = CameraBoom->GetComponentRotation();
	FVector CamForward = FRotationMatrix(CameraWorldRot).GetUnitAxis(EAxis::X);
	FVector CamRight = FRotationMatrix(CameraWorldRot).GetUnitAxis(EAxis::Y);

	// Project onto gravity plane (remove the gravity-up component)
	CamForward = FVector::VectorPlaneProject(CamForward, GravityUp).GetSafeNormal();
	CamRight = FVector::VectorPlaneProject(CamRight, GravityUp).GetSafeNormal();

	// Apply movement input
	AddMovementInput(CamForward, MovementVector.Y);
	AddMovementInput(CamRight, MovementVector.X);
}

void ACustomGravityCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	CameraYaw += LookAxisVector.X * LookSensitivity;
	CameraPitch = FMath::Clamp(CameraPitch - LookAxisVector.Y * LookSensitivity, -80.f, 80.f);
}

void ACustomGravityCharacter::HandleJumpStarted()
{
	Jump();
}

void ACustomGravityCharacter::HandleJumpCompleted()
{
	StopJumping();
}

// ──────────────────────── Gravity ────────────────────────

void ACustomGravityCharacter::UpdateGravityDirection()
{
	FVector NewGravityDir;

	if (GravityTarget)
	{
		// Radial gravity: direction from character toward the gravity center
		NewGravityDir = (GravityTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}
	else
	{
		NewGravityDir = DefaultGravityDirection.GetSafeNormal();
	}

	if (NewGravityDir.IsNearlyZero())
	{
		NewGravityDir = FVector(0.f, 0.f, -1.f);
	}

	CurrentGravityDir = NewGravityDir;

	// UE 5.4: Set custom gravity direction on CharacterMovementComponent
	GetCharacterMovement()->SetGravityDirection(CurrentGravityDir);
}

void ACustomGravityCharacter::AlignCameraToGravity(float DeltaTime)
{
	const FVector GravityUp = GetGravityUpVector();

	// Build camera rotation:
	// Yaw rotates around gravity-up axis
	// Pitch tilts around the local right axis
	const FQuat YawQuat = FQuat(GravityUp, FMath::DegreesToRadians(CameraYaw));

	// Start with a base forward direction on the gravity plane
	// Use the character's forward projected onto gravity plane as reference
	FVector CharForward = GetActorForwardVector();
	CharForward = FVector::VectorPlaneProject(CharForward, GravityUp).GetSafeNormal();
	if (CharForward.IsNearlyZero())
	{
		CharForward = FVector::VectorPlaneProject(FVector::ForwardVector, GravityUp).GetSafeNormal();
	}

	// Camera forward after yaw
	const FVector YawedForward = YawQuat.RotateVector(FVector::ForwardVector);
	const FVector YawedRight = YawQuat.RotateVector(FVector::RightVector);

	// Build rotation from yaw (around gravity-up) + pitch (around right)
	const FQuat PitchQuat = FQuat(YawedRight, FMath::DegreesToRadians(CameraPitch));

	// Final camera forward
	const FVector FinalForward = PitchQuat.RotateVector(YawedForward);

	// Build the full rotation matrix with gravity-up as the "up" reference
	const FMatrix CameraMatrix = FRotationMatrix::MakeFromXZ(FinalForward, GravityUp);
	const FQuat TargetRotation = CameraMatrix.ToQuat();

	// Smoothly interpolate the camera boom rotation
	const FQuat CurrentRot = CameraBoom->GetComponentQuat();
	const FQuat InterpolatedRot = FQuat::Slerp(CurrentRot, TargetRotation, FMath::Clamp(CameraAlignmentSpeed * DeltaTime, 0.f, 1.f));

	CameraBoom->SetWorldRotation(InterpolatedRot);
}

// ──────────────────────── Public API ────────────────────────

void ACustomGravityCharacter::SetGravityTarget(AActor* NewTarget)
{
	GravityTarget = NewTarget;
	UpdateGravityDirection();
}

void ACustomGravityCharacter::SetCustomGravityDirection(const FVector& NewDirection)
{
	bUseCustomGravity = true;
	GravityTarget = nullptr;
	DefaultGravityDirection = NewDirection.GetSafeNormal();
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
