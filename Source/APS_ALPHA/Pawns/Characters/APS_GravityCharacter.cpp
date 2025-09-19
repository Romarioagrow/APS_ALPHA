// Fill out your copyright notice in the Description page of Project Settings.


#include "APS_GravityCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
AAPS_GravityCharacter::AAPS_GravityCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAPS_GravityCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AAPS_GravityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AAPS_GravityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAPS_GravityCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAPS_GravityCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AAPS_GravityCharacter::MoveUp);

	PlayerInputComponent->BindAxis("RotatePitch", this, &AAPS_GravityCharacter::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AAPS_GravityCharacter::RotateRoll);
	PlayerInputComponent->BindAxis("RotateYaw", this, &AAPS_GravityCharacter::RotateYaw);
}


void AAPS_GravityCharacter::EnableZeroG(ACharacter* Character)
{
	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		M->SetMovementMode(MOVE_Flying);
		M->GravityScale = 0.f;
		M->bOrientRotationToMovement = false;
		M->BrakingFriction = 0.f;
		M->BrakingDecelerationFlying = 0.f;
	}

	/*bUseControllerRotationYaw   = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll  = false;*/

	if (USpringArmComponent* Boom = FindComponentByClass<USpringArmComponent>())
	{
		Boom->bUsePawnControlRotation = true;
		Boom->bEnableCameraLag = false;
		Boom->bEnableCameraRotationLag = false;
	}
}


void AAPS_GravityCharacter::DisableZeroG(ACharacter* Character, float InGravityScale)
{
	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		M->SetMovementMode(MOVE_Walking);
		M->GravityScale = InGravityScale;

		// ДЕФОЛТ: поворачиваемся по направлению движения (нормальная ходьба вбок работает)
		M->bOrientRotationToMovement = true;

		// Вернуть стандартные тормоза (если менял)
		M->BrakingFriction = 2.f;
		M->BrakingDecelerationWalking = 2048.f;
	}

	// ДЕФОЛТ: актёр не крутится от контроллера (камера — отдельно)
	/*bUseControllerRotationYaw   = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll  = false;*/

	// Камера продолжает крутиться мышью
	if (USpringArmComponent* Boom = FindComponentByClass<USpringArmComponent>())
	{
		Boom->bUsePawnControlRotation = true;
	}

	// На всякий: убрать возможный крен камеры после Zero-G
	if (AController* PC = Controller)
	{
		const FRotator R = PC->GetControlRotation();
		PC->SetControlRotation(FRotator(R.Pitch, R.Yaw, 0.f));
	}
}


void AAPS_GravityCharacter::RotateYaw(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	if (bIsZeroG)
	{
		const float Delta = Value * CharacterRotationScale;

		// 1) крутим актёра (локальный yaw)
		AddActorLocalRotation(FRotator(0.f, Delta, 0.f));

		// 2) крутим камеру (контроллер) тем же дельта-углом
		if (AController* PC = Controller)
		{
			FRotator Ctrl = PC->GetControlRotation();
			Ctrl.Yaw = FRotator::NormalizeAxis(Ctrl.Yaw + Delta);
			PC->SetControlRotation(Ctrl);
		}
	}
}

void AAPS_GravityCharacter::RotatePitch(float Value)
{
	/*if (FMath::IsNearlyZero(Value)) return;

	const float Delta = Value * CharacterRotationScale;

	// 1) крутим актёра (локальный yaw)
	AddActorLocalRotation(FRotator(0.f, Delta, 0.f));

	// 2) крутим камеру (контроллер) тем же дельта-углом
	if (AController* PC = Controller)
	{
		FRotator Ctrl = PC->GetControlRotation();
		Ctrl.Pitch = FRotator::NormalizeAxis(Ctrl.Pitch + Delta);
		PC->SetControlRotation(Ctrl);
	}*/
}

void AAPS_GravityCharacter::RotateRoll(float Value)
{
	/*if (FMath::IsNearlyZero(Value)) return;

	const float Delta = Value * CharacterRotationScale;

	// 1) крутим актёра (локальный yaw)
	AddActorLocalRotation(FRotator(0.f, Delta, 0.f));

	// 2) крутим камеру (контроллер) тем же дельта-углом
	if (AController* PC = Controller)
	{
		FRotator Ctrl = PC->GetControlRotation();
		Ctrl.Roll = FRotator::NormalizeAxis(Ctrl.Roll + Delta);
		PC->SetControlRotation(Ctrl);
	}*/
}

void AAPS_GravityCharacter::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	const FRotator Cam = (Controller ? Controller->GetControlRotation() : GetActorRotation());

	if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		const FVector Fwd = FRotationMatrix(Cam).GetUnitAxis(EAxis::X);
		AddMovementInput(Fwd, Value);
		// мгновенно выровнять по камере по yaw+pitch (roll не трогаем)
		const FRotator Target(Cam.Pitch, Cam.Yaw, GetActorRotation().Roll);
		Controller->SetControlRotation(Target);
		SetActorRotation(Target);
	}
}

void AAPS_GravityCharacter::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	const FRotator Cam = (Controller ? Controller->GetControlRotation() : GetActorRotation());

	if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		const FVector Right = FRotationMatrix(Cam).GetUnitAxis(EAxis::Y);
		AddMovementInput(Right, Value);
	}
}

void AAPS_GravityCharacter::MoveUp(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		const FRotator Cam = (Controller ? Controller->GetControlRotation() : GetActorRotation());
		const FVector Up = FRotationMatrix(Cam).GetUnitAxis(EAxis::Z);
		AddMovementInput(Up, Value);
	}
}
