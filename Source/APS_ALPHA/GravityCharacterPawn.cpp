// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityCharacterPawn.h"
#include "Spaceship.h"
#include "SpaceStation.h"
#include "OrbitalBody.h"
#include "GravityCharacterPawn.h"
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>

// Sets default values
AGravityCharacterPawn::AGravityCharacterPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create CapsuleComponent
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleMesh"));
	CapsuleComponent->InitCapsuleSize(42.f, 96.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(CapsuleComponent);

	// Create and attach skeletal mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0));

	// Create and set up SpringArmComponent
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(CapsuleComponent);
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

	// Bind overlaps
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AGravityCharacterPawn::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &AGravityCharacterPawn::OnEndOverlap);

	UpdateGravityType();
}

// Called every frame
void AGravityCharacterPawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update gravity
	UpdateGravity();

	const FVector CurrentVelocity = CapsuleComponent->GetPhysicsLinearVelocity();
	ForwardSpeed = FVector::DotProduct(CurrentVelocity, GetActorForwardVector());
	RightSpeed = FVector::DotProduct(CurrentVelocity, GetActorRightVector());
	ForwardSpeed /= 10.0f;
	RightSpeed /= 10.0f;

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("ForwardSpeed: %f"), ForwardSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("RightSpeed: %f"), RightSpeed));
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

	// Настройка параметров поворота персонажа
	PlayerInputComponent->BindAxis("RotatePitch", this, &AGravityCharacterPawn::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AGravityCharacterPawn::RotateRoll);
	PlayerInputComponent->BindAxis("RotateYaw", this, &AGravityCharacterPawn::RotateYaw);
}

void AGravityCharacterPawn::UpdateGravity()
{
	switch (CurrentGravityType)
	{
	case EGravityType::ZeroG:
		// Логика обновления гравитации в режиме невесомости
		UpdateZeroGGravity();
		break;
	case EGravityType::OnStation:
		// Логика обновления гравитации на станции
		UpdateStationGravity();
		break;
	case EGravityType::OnPlanet:
		// Логика обновления гравитации на планете
		UpdatePlanetGravity();
		break;
	case EGravityType::OnShip:
		// Логика обновления гравитации на корабле
		UpdateShipGravity();
		break;
	}
}


/**
 * @brief Overlaps
*/

void AGravityCharacterPawn::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("BeginOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("BeginOverlap with: %s"), *OtherActor->GetName()));

	// if gravity object
	if (OtherActor != nullptr)
	{
		if (OtherActor->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
		{			
			SwitchGravityType(OtherActor);
		}
	}
}

void AGravityCharacterPawn::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("EndOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("EndOverlap with: %s"), *OtherActor->GetName()));

	UpdateGravityType();
}

/**
 * @brief Gravity Actions
*/

void AGravityCharacterPawn::UpdateGravityType()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("UpdateGravityType")));

	FName TagToCheck = "GravitySource";
	TArray<AActor*> OverlappingActorsWithTag;
	TArray<AActor*> OverlappingActors;
	CapsuleComponent->GetOverlappingActors(OverlappingActors);

	// Проверить каждого актера на наличие тега
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor != nullptr && Actor->ActorHasTag(TagToCheck))
		{
			OverlappingActorsWithTag.Add(Actor);
		}
	}

	if (OverlappingActorsWithTag.Num() > 0)
	{
		AActor* FirstGravityActor = OverlappingActorsWithTag[0];
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("FirstGravityActor : %s"), *FirstGravityActor->GetName()));

		// switch gravity to first 
		SwitchGravityType(FirstGravityActor);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("FirstGravityActor 0")));
		CurrentGravityType = EGravityType::ZeroG;
	}
}

void AGravityCharacterPawn::SwitchGravityType(AActor* GravitySourceActor)
{
	GravityTargetActor = GravitySourceActor;
	
	if (GravitySourceActor->IsA(ASpaceStation::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnStation;
	}
	else if (GravitySourceActor->IsA(AOrbitalBody::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnPlanet;
	}
	else if (GravitySourceActor->IsA(ASpaceship::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnShip;
	}
}

void AGravityCharacterPawn::SwitchGravityToZeroG(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToZERO-G")));
	CurrentGravityType = EGravityType::ZeroG;
	GravityTargetActor = nullptr;
}

void AGravityCharacterPawn::UpdateZeroGGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ZeroG Gravity")));
}

void AGravityCharacterPawn::UpdateStationGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Station Gravity")));

	const FVector GravityRotZ = GravityTargetActor->GetActorUpVector();
	const FVector GravityRotX = CapsuleComponent->GetForwardVector();
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityRotZ, GravityRotX);
	const FRotator Rotation = RotationMatrix.Rotator();
	const FRotator ActorRotation = GetActorRotation();
	const FRotator Result = FMath::RInterpTo(ActorRotation, Rotation, GetWorld()->GetDeltaSeconds(), 2.f);

	SetActorRotation(Result);
}

void AGravityCharacterPawn::UpdatePlanetGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Planet Gravity")));

	const FVector GravityTargetLocation = GravityTargetActor->GetActorLocation();
	const FVector ActorLocation = GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GravityTargetLocation, ActorLocation);
	const FVector CapsuleForwardVector = CapsuleComponent->GetForwardVector();
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(LookAtRotation.Vector(), CapsuleForwardVector);
	const FRotator ActorRotation = GetActorRotation();
	const FRotator ResultRotation = FMath::RInterpTo(ActorRotation, RotationMatrix.Rotator(), GetWorld()->GetDeltaSeconds(), 2.f);
	
	SetActorRotation(ResultRotation);
}

void AGravityCharacterPawn::UpdateShipGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Ship Gravity")));

	const FVector GravityRotZ = GravityTargetActor->GetActorUpVector();
	const FVector GravityRotX = CapsuleComponent->GetForwardVector();
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityRotZ, GravityRotX);
	const FRotator Rotation = RotationMatrix.Rotator();
	const FRotator ActorRotation = GetActorRotation();
	const FRotator Result = FMath::RInterpTo(ActorRotation, Rotation, GetWorld()->GetDeltaSeconds(), 2.f);

	SetActorRotation(Result);
}

/**
 * @brief  Movements, Rotations, Camera
*/

void AGravityCharacterPawn::Turn(const float Value)
{
	FRotator TargetRotation = CameraSpringArm->GetRelativeRotation(); 
	TargetRotation.Yaw += Value * CharacterRotationScale;
	CameraSpringArm->SetRelativeRotation(TargetRotation);

}

void AGravityCharacterPawn::LookUp(const float Value)
{
	FRotator TargetRotation = CameraSpringArm->GetRelativeRotation(); 
	TargetRotation.Pitch += Value * CharacterRotationScale;
	CameraSpringArm->SetRelativeRotation(TargetRotation);
}

void AGravityCharacterPawn::AlignCharacterToCamera()
{
	// Получаем текущий кватернион вращения камеры
	const FQuat CameraQuat = CameraSpringArm->GetComponentQuat();

	// Вычисляем новый кватернион вращения персонажа, используя кватернион вращения камеры
	FQuat NewCharacterQuat = FQuat(CameraQuat.X, CameraQuat.Y, CameraQuat.Z, CameraQuat.W);

	// Интерполируем вращение CapsuleComponent и персонажа к вращению камеры
	FQuat InterpolatedQuat = FMath::QInterpTo(GetActorQuat(), NewCharacterQuat, GetWorld()->GetDeltaSeconds(), CameraInterpolationSpeed);
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// Обнуляем вращение CameraSpringArm
	CameraSpringArm->SetWorldRotation(FRotator(CameraQuat.Rotator().Pitch, NewCharacterQuat.Rotator().Yaw, CameraQuat.Rotator().Roll));
}

void AGravityCharacterPawn::MoveForward(const float Value)
{
	if (Value != 0)
	{
		AlignCharacterToCamera();

		// add impulse
		CapsuleComponent->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::MoveRight(const float Value)
{
	if (Value != 0)
	{
		AlignCharacterToCamera();

		// add impulse
		CapsuleComponent->AddImpulse(GetActorRightVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::MoveUp(const float Value)
{
	if (Value != 0)
	{
		// add impulse
		CapsuleComponent->AddImpulse(GetActorUpVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::RotatePitch(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Pitch = Value * CharacterRotationScale;
	AddActorLocalRotation(RotationToAdd);
}

void AGravityCharacterPawn::RotateRoll(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Roll = Value * CharacterRotationScale;
	AddActorLocalRotation(RotationToAdd);
}

void AGravityCharacterPawn::RotateYaw(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Yaw = Value * CharacterRotationScale;
	AddActorLocalRotation(RotationToAdd);
}
