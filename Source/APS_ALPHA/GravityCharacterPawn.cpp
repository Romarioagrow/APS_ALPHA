// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityCharacterPawn.h"
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>
#include "SpaceshipGravityActor.h"
#include "StationGravityActor.h"

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
	//ArrowForwardVector = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowForwardVector"));
	//ArrowForwardVector->SetupAttachment(CapsuleComponent);

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
	/*CameraSpringArm->bEnableCameraLag = true;
	CameraSpringArm->CameraLagSpeed = 3.0f;*/

	// Create and set up CameraComponent
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);

	/*bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = true;
	CameraSpringArm->bUsePawnControlRotation = true;*/

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
void AGravityCharacterPawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	// update velocity for animations
	//const FVector Velocity = GetVelocity();
	//FVector Velocity = UPrimitiveComponent::GetPhysicsLinearVelocity();
	//const FVector Forward = GetActorForwardVector();
	//const FVector Right = GetActorRightVector();


	/*FRotator CurrentRotation = GetActorRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, RotationInterpolationSpeed);
	SetActorRotation(NewRotation);*/

	const FVector CurrentVelocity = CapsuleComponent->GetPhysicsLinearVelocity();
	//float Speed = CurrentVelocity.Length();

	ForwardSpeed = FVector::DotProduct(CurrentVelocity, GetActorForwardVector());
	RightSpeed = FVector::DotProduct(CurrentVelocity, GetActorRightVector());

	ForwardSpeed /= 10.0f;
	RightSpeed /= 10.0f;

	//float ForwardSpeed = FVector::DotProduct(CurrentVelocity, GetActorForwardVector());
	//float RightSpeed = FVector::DotProduct(CurrentVelocity, GetActorRightVector());


	//ForwardSpeed = FVector::DotProduct(Velocity, Forward);
	//RightSpeed = FVector::DotProduct(Velocity, Right);

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
		const UClass* FirstGravityActor = TaggedActors[0]->GetClass();
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
	const FRotator Result = FMath::RInterpTo(ActorRotation, RotationMatrix.Rotator(), GetWorld()->GetDeltaSeconds(), 2.f);

	SetActorRotation(Result);
}

void AGravityCharacterPawn::UpdateShipGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Ship Gravity")));
}


/**
 * @brief  Movements, Rotations, Camera
*/

void AGravityCharacterPawn::Turn(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Yaw = Value * CharacterRotationScale;

	CameraSpringArm->AddLocalRotation(RotationToAdd, false);



	/*FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Yaw = Value * CharacterRotationScale;
	CameraSpringArm->AddLocalRotation(RotationToAdd, false);*/

	//AddControllerYawInput(Value);
	/*FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Roll = Value * CharacterRotationScale;
	CameraSpringArm->AddLocalRotation(RotationToAdd, false);*/

	if (Value != 0)
	{

		/*FRotator ForwardRotator = ArrowForwardVector->GetRelativeRotation();
		const float NewCameraYaw = (ForwardRotator.Yaw + Value) * CameraYawScale;
		ArrowForwardVector->SetRelativeRotation(FRotator(ForwardRotator.Pitch, NewCameraYaw, ForwardRotator.Roll));*/
	}
}

void AGravityCharacterPawn::LookUp(const float Value)
{

	//AddControllerPitchInput(Value);

	/*FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Pitch = Value * CharacterRotationScale;
	CameraSpringArm->AddLocalRotation(RotationToAdd, false);*/


	if (Value != 0)
	{

		/*FRotator ForwardRotator = ArrowForwardVector->GetRelativeRotation();
		const float NewCameraPitch = (ForwardRotator.Pitch + Value) * CameraPitchScale;
		ArrowForwardVector->SetRelativeRotation(FRotator(NewCameraPitch, ForwardRotator.Yaw, ForwardRotator.Roll));*/
	}
}

void AGravityCharacterPawn::MoveForward(const float Value)
{
	if (Value != 0)
	{
		// rotate character
		/*FRotator RelativeForwardRotator = ArrowForwardVector->GetRelativeRotation();
		AddActorLocalRotation(RelativeForwardRotator);
		ArrowForwardVector->AddLocalRotation(RelativeForwardRotator.GetInverse());*/

		const FVector CameraVector = PlayerCamera->GetForwardVector();
		//AddActorLocalRotation(CameraVector.Rotation());

		if (GetActorRightVector() != CameraVector)
		{
			//SetActorRelativeRotation(CameraVector.Rotation());
			//SetActorRotation(CameraVector.Rotation());
		}



		// add impulse
		CapsuleComponent->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementScale), "None", true);
	}
}

void AGravityCharacterPawn::MoveRight(const float Value)
{
	if (Value != 0)
	{
		// rotate character
		/*FRotator RelativeForwardRotator = ArrowForwardVector->GetRelativeRotation();
		AddActorLocalRotation(RelativeForwardRotator);
		ArrowForwardVector->AddLocalRotation(RelativeForwardRotator.GetInverse());*/

		const FVector CameraVector = PlayerCamera->GetRightVector();
		//AddActorLocalRotation(CameraVector.Rotation());

		if (GetActorRightVector() != CameraVector)
		{
			//SetActorRelativeRotation(CameraVector.Rotation());
			//SetActorRotation(CameraVector.Rotation());

		}

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
	//CameraSpringArm->AddLocalRotation(RotationToAdd, false);
	AddActorLocalRotation(RotationToAdd);

}

void AGravityCharacterPawn::RotateRoll(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Roll = Value * CharacterRotationScale;
	//CameraSpringArm->AddLocalRotation(RotationToAdd, false);
	AddActorLocalRotation(RotationToAdd);
	//GetActorRelativeRotation();

}

void AGravityCharacterPawn::RotateYaw(const float Value)
{
	FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
	RotationToAdd.Yaw = Value * CharacterRotationScale;
	//CameraSpringArm->AddLocalRotation(RotationToAdd, false);
	AddActorLocalRotation(RotationToAdd);
}

//void AGravityCharacterPawn::RotatePitch(const float Value)
//{
//	//AddActorLocalRotation(FQuat(FRotator(Value * CharacterRotationScale, 0.0f, 0.0f)));
//	//CameraSpringArm->AddLocalRotation(FRotator(0.0f, 0.0f, Value * CharacterRotationScale));
//	//AddControllerPitchInput(Value * CharacterRotationScale);
//
//	DesiredRotation.Pitch += Value * CharacterRotationScale;
//}
//
//void AGravityCharacterPawn::RotateRoll(const float Value)
//{
//	//AddControllerRollInput(Value * CharacterRotationScale);
//	DesiredRotation.Roll += Value * CharacterRotationScale;
//
//	//AddActorLocalRotation(FQuat(FRotator(0.0f, 0.0f, Value * CharacterRotationScale)));
//	//CameraSpringArm->AddLocalRotation(FRotator(0.0f, 0.0f, Value * CharacterRotationScale));
//
//}
//
//void AGravityCharacterPawn::RotateYaw(const float Value)
//{
//	//AddActorLocalRotation(FQuat(FRotator(0.0f, Value * CharacterRotationScale, 0.0f)));
//	//CameraSpringArm->AddLocalRotation(FRotator(0.0f, 0.0f, Value * CharacterRotationScale));
//	//AddControllerYawInput(Value * CharacterRotationScale);
//
//	DesiredRotation.Yaw += Value * CharacterRotationScale;
//}


