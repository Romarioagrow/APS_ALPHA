// Fill out your copyright notice in the Description page of Project Settings.


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

	// Bind overlaps
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AGravityCharacterPawn::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &AGravityCharacterPawn::OnEndOverlap);
}

// Called every frame
void AGravityCharacterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (CurrentGravityType)
	{
	case EGravityType::ZeroG:
		// ������ ���������� ���������� � ������ �����������
		UpdateZeroGGravity();
		break;
	case EGravityType::OnStation:
		// ������ ���������� ���������� �� �������
		UpdateStationGravity();
		break;
	case EGravityType::OnPlanet:
		// ������ ���������� ���������� �� �������
		UpdatePlanetGravity();
		break;
	case EGravityType::OnShip:
		// ������ ���������� ���������� �� �������
		UpdateShipGravity();
		break;
	}
}

// Called to bind functionality to input
void AGravityCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// ��������� ���������� ����������� ���������
	PlayerInputComponent->BindAxis("MoveForward", this, &AGravityCharacterPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGravityCharacterPawn::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AGravityCharacterPawn::MoveUp);

	// ��������� ���������� �������� ������
	PlayerInputComponent->BindAxis("Turn", this, &AGravityCharacterPawn::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AGravityCharacterPawn::LookUp);

	// ��������� ���������� �������� ���������
	PlayerInputComponent->BindAxis("RotatePitch", this, &AGravityCharacterPawn::RotatePitch);
	PlayerInputComponent->BindAxis("RotateRoll", this, &AGravityCharacterPawn::RotateRoll);
	PlayerInputComponent->BindAxis("RotateYaw", this, &AGravityCharacterPawn::RotateYaw);
}


/**
 * @brief Overlaps
*/

void AGravityCharacterPawn::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("BeginOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("BeginOverlap with: %s"), *OtherActor->GetName()));

	if (OtherActor->IsA(AStationGravityActor::StaticClass()))
	{
		SwitchGravityToStation(OtherActor);
	}
	else if (OtherActor->IsA(APlanetGravityActor::StaticClass()))
	{
		SwitchGravityToPlanet(OtherActor);
	}
	else if (OtherActor->IsA(ASpaceshipGravityActor::StaticClass()))
	{
		SwitchGravityToSpaceship(OtherActor);
	}
}

void AGravityCharacterPawn::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("EndOverlap with: %s"), *OtherActor->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("EndOverlap with: %s"), *OtherActor->GetName()));

	FVector GravityTargetLocation = OtherActor->GetActorLocation();

	UpdateGravityStatus();
}


/**
 * @brief Gravity Actions
*/

void AGravityCharacterPawn::UpdateGravityStatus()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("UpdateGravityStatus")));

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "EventManager", TaggedActors);

	if (TaggedActors.Num() > 0)
	{
		UClass* FirstGravityActor = TaggedActors[0]->GetClass();
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("FirstGravityActor : %s"), *FirstGravityActor->GetName()));

		// switch gravity to first 
		// ... 

	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("FirstGravityActor 0")));
		CurrentGravityType = EGravityType::ZeroG;
	}

}

void AGravityCharacterPawn::SwitchGravityToStation(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToStation")));

	CurrentGravityType = EGravityType::OnStation;
	GravityTargetActor = Cast<AStationGravityActor>(OtherActor);
}

void AGravityCharacterPawn::SwitchGravityToPlanet(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToSpaceship")));

	CurrentGravityType = EGravityType::OnPlanet;
	GravityTargetActor = Cast<APlanetGravityActor>(OtherActor);
}

void AGravityCharacterPawn::SwitchGravityToSpaceship(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToSpaceship")));
	CurrentGravityType = EGravityType::OnShip;
	GravityTargetActor = Cast<ASpaceshipGravityActor>(OtherActor);
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

	FVector GravityRotZ = GravityTargetActor->GetActorUpVector();
	FVector GravityRotX = CapsuleComponent->GetForwardVector();

	FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityRotZ, GravityRotX);
	FRotator Rotation = RotationMatrix.Rotator();
	FRotator ActorRotation = GetActorRotation();
	FRotator Result = FMath::RInterpTo(ActorRotation, Rotation, GetWorld()->GetDeltaSeconds(), 2.f);

	SetActorRotation(Result);
}

void AGravityCharacterPawn::UpdatePlanetGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Planet Gravity")));

	FVector GravityTargetLocation = GravityTargetActor->GetActorLocation();
	FVector ActorLocation = GetActorLocation();
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GravityTargetLocation, ActorLocation);
	FVector CapsuleForawardVector = CapsuleComponent->GetForwardVector();
	FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(LookAtRotation.Vector(), CapsuleForawardVector);

	FRotator ActorRotation = GetActorRotation();
	FRotator Result = FMath::RInterpTo(ActorRotation, RotationMatrix.Rotator(), GetWorld()->GetDeltaSeconds(), 2.f);

	SetActorRotation(Result);
}

void AGravityCharacterPawn::UpdateShipGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Ship Gravity")));
}


/**
 * @brief  Movements, Rotations, Camera
*/

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


