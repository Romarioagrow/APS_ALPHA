// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "GameFramework/SpringArmComponent.h"

// Sets default values
ASpaceCharacter::ASpaceCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	RootComponent = SkeletalMesh;

	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	CameraSpringArm->SetupAttachment(SkeletalMesh);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraSpringArm);

	MovementSpeed = 1000.0f;
	RotationSpeed = 100.0f;
}

void ASpaceCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASpaceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpaceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASpaceCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASpaceCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ASpaceCharacter::MoveUp);

	PlayerInputComponent->BindAxis("Turn", this, &ASpaceCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASpaceCharacter::LookUp);
	PlayerInputComponent->BindAxis("Roll", this, &ASpaceCharacter::Roll);

	// Привязка вращения
	PlayerInputComponent->BindAxis("RotatePitch", this, &ASpaceCharacter::RotatePitch);
	PlayerInputComponent->BindAxis("RotateYaw", this, &ASpaceCharacter::RotateYaw);
	PlayerInputComponent->BindAxis("RotateRoll", this, &ASpaceCharacter::RotateRoll);
}

void ASpaceCharacter::MoveForward(float Value)
{
	FVector Direction = GetActorForwardVector();
	AddActorWorldOffset(Direction * Value * MovementSpeed * GetWorld()->GetDeltaSeconds(), true);
}

void ASpaceCharacter::MoveRight(float Value)
{
	FVector Direction = GetActorRightVector();
	AddActorWorldOffset(Direction * Value * MovementSpeed * GetWorld()->GetDeltaSeconds(), true);
}

void ASpaceCharacter::MoveUp(float Value)
{
	FVector Direction = GetActorUpVector();
	AddActorWorldOffset(Direction * Value * MovementSpeed * GetWorld()->GetDeltaSeconds(), true);
}

void ASpaceCharacter::Turn(float Value)
{
	AddActorWorldRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, Value * RotationSpeed * GetWorld()->GetDeltaSeconds())));
}

void ASpaceCharacter::LookUp(float Value)
{
	CameraSpringArm->AddLocalRotation(FRotator(Value * RotationSpeed * GetWorld()->GetDeltaSeconds(), 0.0f, 0.0f));
}

void ASpaceCharacter::Roll(float Value)
{
	AddActorWorldRotation(FQuat::MakeFromEuler(FVector(Value * RotationSpeed * GetWorld()->GetDeltaSeconds(), 0.0f, 0.0f)));
}

void ASpaceCharacter::RotatePitch(float Value)
{
	FRotator NewRotation = GetActorRotation();
	NewRotation.Pitch += Value;
	SetActorRotation(NewRotation);
	UpdateCameraOrientation();
}

void ASpaceCharacter::RotateYaw(float Value)
{
	FRotator NewRotation = GetActorRotation();
	NewRotation.Yaw += Value;
	SetActorRotation(NewRotation);
	UpdateCameraOrientation();
}

void ASpaceCharacter::RotateRoll(float Value)
{
	FRotator NewRotation = GetActorRotation();
	NewRotation.Roll += Value;
	SetActorRotation(NewRotation);
	UpdateCameraOrientation();
}

void ASpaceCharacter::UpdateCameraOrientation()
{
	FRotator NewCameraRotation = GetActorRotation();
	CameraSpringArm->SetWorldRotation(NewCameraRotation);
}