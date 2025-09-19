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

	PlayerInputComponent->BindAxis("RotatePitch", this, &AAPS_GravityCharacter::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AAPS_GravityCharacter::RotateRoll);
	PlayerInputComponent->BindAxis("RotateYaw", this, &AAPS_GravityCharacter::RotateYaw);

}


void AAPS_GravityCharacter::EnableZeroG(ACharacter* Character)
{
	auto* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(MOVE_Flying);
	Move->GravityScale = 0.f;
	Move->bOrientRotationToMovement = false;        // не крутить от движения

	// Поворачивать актёра по контроллеру
	Character->bUseControllerRotationYaw   = true;
	Character->bUseControllerRotationPitch = true;
	Character->bUseControllerRotationRoll  = true;

	// Камера-бум должен брать поворот из контроллера
	if (USpringArmComponent* Boom = Character->FindComponentByClass<USpringArmComponent>())
		Boom->bUsePawnControlRotation = true;
}


void AAPS_GravityCharacter::DisableZeroG(ACharacter* Character, float InGravityScale)
{
	if (!Character) return;

	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
		Move->GravityScale = InGravityScale;
		Move->bOrientRotationToMovement = true;   // персонаж поворачивается по движению
	}

	// персонаж больше НЕ крутится от контроллера
	Character->bUseControllerRotationYaw   = false;
	Character->bUseControllerRotationPitch = false;
	Character->bUseControllerRotationRoll  = false;

	// камера продолжает крутиться от контроллера (как в стандартном Third Person)
	if (USpringArmComponent* Boom = Character->FindComponentByClass<USpringArmComponent>())
	{
		Boom->bUsePawnControlRotation = true;     // ← ключевая правка
	}
}


void AAPS_GravityCharacter::MoveUp(const float Value)
{
	if (Value != 0 && bIsZeroG)
	{
		GetCapsuleComponent()->AddImpulse(GetActorUpVector() * (Value * CharacterJumpForce), "None", true);
	}
}

void AAPS_GravityCharacter::RotatePitch(float Value)
{
	if (bIsZeroG && !FMath::IsNearlyZero(Value))
		AddControllerPitchInput(Value * CharacterRotationScale);
}

void AAPS_GravityCharacter::RotateYaw(float Value)
{
	if (bIsZeroG && !FMath::IsNearlyZero(Value))
		AddControllerYawInput(Value * CharacterRotationScale);
}

// У PlayerController нет готового AddControllerRollInput, поэтому делаем вручную:
void AAPS_GravityCharacter::RotateRoll(float Value)
{
	if (!(bIsZeroG && !FMath::IsNearlyZero(Value))) return;

	if (AController* PC = Controller)
	{
		const FRotator Ctrl = PC->GetControlRotation();
		const FRotator New  = FRotator(Ctrl.Pitch, Ctrl.Yaw, Ctrl.Roll + Value * CharacterRotationScale);
		PC->SetControlRotation(New);
	}
}