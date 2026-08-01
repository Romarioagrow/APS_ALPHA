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
#include "InputCoreTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
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
	CreateTraversalHud();
}

void ACustomGravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	AlignCameraToGravity(DeltaTime);
	UpdateInteractionCandidate();
	if (!InteractionPromptWidget.IsValid())
	{
		CreateInteractionPrompt();
	}
	if (!TraversalHudWidget.IsValid())
	{
		CreateTraversalHud();
	}
}

void ACustomGravityCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInteractionPrompt();
	RemoveTraversalHud();
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

void ACustomGravityCharacter::CreateTraversalHud()
{
	if (TraversalHudWidget.IsValid() || !IsLocallyControlled() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	const TWeakObjectPtr<ACustomGravityCharacter> WeakThis(this);
	TraversalHudWidget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(28.0f, 0.0f, 0.0f, 28.0f)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(TOptional<int32>(8))
			.BlurStrength(6.0f)
			.bApplyAlphaToBlur(true)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.004f, 0.012f, 0.024f, 0.94f))
				.Padding(FMargin(16.0f, 11.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const ACustomGravityCharacter* Character = WeakThis.Get();
							return Character ? Character->GetTraversalStatusText() : FText::GetEmpty();
						})
						.ColorAndOpacity_Lambda([WeakThis]()
						{
							const ACustomGravityCharacter* Character = WeakThis.Get();
							return Character && Character->bManualZeroGOverride
								? FSlateColor(FLinearColor(1.0f, 0.55f, 0.18f, 1.0f))
								: FSlateColor(FLinearColor(0.2f, 0.82f, 1.0f, 1.0f));
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const ACustomGravityCharacter* Character = WeakThis.Get();
							return Character ? Character->GetTraversalHintText() : FText::GetEmpty();
						})
						.ColorAndOpacity(FLinearColor(0.82f, 0.87f, 0.92f, 1.0f))
					]
				]
			]
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
