// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityCharacterPawn.h"

// Sets default values
AGravityCharacterPawn::AGravityCharacterPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create CapsuleComponent
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleMesh"));
	// Set properties
	CapsuleComponent->InitCapsuleSize(42.f, 96.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	// Attach to root component
	//CapsuleMeshComponent->SetupAttachment(RootComponent);
	SetRootComponent(CapsuleComponent);

	// Create and attach arrow component
	ArrowForwardVector = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowForwardVector"));
	ArrowForwardVector->SetupAttachment(CapsuleComponent);

	// Create and attach skeletal mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0));

	// Create and set up SpringArmComponent
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(ArrowForwardVector);
	CameraSpringArm->TargetArmLength = 300.0f;
	CameraSpringArm->bUsePawnControlRotation = false;

	// Create and set up CameraComponent
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);



}

// Called when the game starts or when spawned
void AGravityCharacterPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGravityCharacterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AGravityCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Настройка параметров перемещения персонажа
	PlayerInputComponent->BindAxis("MoveForward", this, &AGravityCharacterPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravityCharacterPawn::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AGravityCharacterPawn::MoveUp);

	// Настройка параметров поворота камеры
	PlayerInputComponent->BindAxis("Turn", this, &AGravityCharacterPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravityCharacterPawn::LookUp);

	PlayerInputComponent->BindAxis("RotatePitch", this, &AGravityCharacterPawn::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AGravityCharacterPawn::RotateRoll);
	PlayerInputComponent->BindAxis("RotateYaw", this, &AGravityCharacterPawn::RotateYaw);
}

void AGravityCharacterPawn::Turn(float Value)
{
	if (Value != 0)
	{
		FRotator ForwardRotator = ArrowForwardVector->GetRelativeRotation();
		float NewCameraYaw = (ForwardRotator.Yaw + Value) * CameraYawScale;
		ArrowForwardVector->SetRelativeRotation(FRotator(ForwardRotator.Pitch, NewCameraYaw, ForwardRotator.Roll));
	}
}

void AGravityCharacterPawn::LookUp(float Value)
{
	if (Value != 0)
	{
		FRotator ForwardRotator = ArrowForwardVector->GetRelativeRotation();
		float NewCameraPitch = (ForwardRotator.Pitch + Value) * CameraPitchScale;
		ArrowForwardVector->SetRelativeRotation(FRotator(NewCameraPitch, ForwardRotator.Yaw, ForwardRotator.Roll));
	}
}

void AGravityCharacterPawn::MoveForward(float Value)
{
	if (Value != 0)
	{
		// rotate character
		FRotator RelativeForwardRotator = ArrowForwardVector->GetRelativeRotation();
		AddActorLocalRotation(RelativeForwardRotator);
		ArrowForwardVector->AddLocalRotation(RelativeForwardRotator.GetInverse());

		// add impulse
		CapsuleComponent->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::MoveRight(float Value)
{
	if (Value != 0)
	{
		// rotate character
		FRotator RelativeForwardRotator = ArrowForwardVector->GetRelativeRotation();
		AddActorLocalRotation(RelativeForwardRotator);
		ArrowForwardVector->AddLocalRotation(RelativeForwardRotator.GetInverse());

		// add impulse
		CapsuleComponent->AddImpulse(GetActorRightVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::MoveUp(float Value)
{
	if (Value != 0)
	{
		// add impulse
		CapsuleComponent->AddImpulse(GetActorUpVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::RotatePitch(float Value)
{
	AddActorLocalRotation(FQuat(FRotator(Value * CharacterRotationScale, 0.0f, 0.0f)));
}

void AGravityCharacterPawn::RotateRoll(float Value)
{
	AddActorLocalRotation(FQuat(FRotator(0.0f, 0.0f, Value * CharacterRotationScale)));
}

void AGravityCharacterPawn::RotateYaw(float Value)
{
	AddActorLocalRotation(FQuat(FRotator(0.0f, Value * CharacterRotationScale, 0.0f)));
}
