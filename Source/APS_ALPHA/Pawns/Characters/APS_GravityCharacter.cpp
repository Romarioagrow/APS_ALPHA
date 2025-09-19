// Fill out your copyright notice in the Description page of Project Settings.


#include "APS_GravityCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

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

}


void AAPS_GravityCharacter::EnableZeroG(ACharacter* C, float MaxSpeedZeroG = 600.f)
{
	if (!C) return;
	auto* Move = C->GetCharacterMovement();
	if (!Move) return;

	// Полная невесомость
	Move->GravityScale = 0.0f;               // отключает падение
	Move->SetMovementMode(MOVE_Flying);      // свободный полёт
	Move->MaxFlySpeed = MaxSpeedZeroG;

	// Без авто-торможения
	Move->BrakingFriction = 0.f;
	Move->BrakingFrictionFactor = 0.f;
	Move->BrakingDecelerationFlying = 0.f;
	Move->AirControl = 0.f;
}


void AAPS_GravityCharacter::DisableZeroG(ACharacter* C, const FVector& GravityDir, float GravityScale = 1.f)
{
	if (!C) return;
	auto* Move = C->GetCharacterMovement();
	if (!Move) return;

	Move->SetMovementMode(MOVE_Walking);                 // или MOVE_Falling
	Move->GravityScale = GravityScale;                   // вернули силу g
	Move->SetGravityDirection(GravityDir.GetSafeNormal()); // направление g (UE5.4 фича)

	// Вернём стандартные тормоза (по желанию)
	Move->BrakingFriction = 2.f;
	Move->BrakingFrictionFactor = 1.f;
	Move->BrakingDecelerationFlying = 2048.f;
	Move->AirControl = 0.35f;
}