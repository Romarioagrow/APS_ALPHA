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

	// �������� Arrow ���������
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(CapsuleComponent);
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


	// Compute and display Character Velocity
	const FVector CurrentVelocity = CapsuleComponent->GetPhysicsLinearVelocity();
	ForwardSpeed = FVector::DotProduct(CurrentVelocity, GetActorForwardVector());
	RightSpeed = FVector::DotProduct(CurrentVelocity, GetActorRightVector());
	UpSpeed = FVector::DotProduct(CurrentVelocity, GetActorUpVector());
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("ForwardSpeed: %f"), ForwardSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("RightSpeed: %f"), RightSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("UpSpeed: %f"), UpSpeed));
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

void AGravityCharacterPawn::UpdateGravity()
{
	switch (CurrentGravityType)
	{
	case EGravityType::ZeroG:
		UpdateZeroGGravity();
		break;
	case EGravityType::OnStation:
		UpdateStationGravity();
		break;
	case EGravityType::OnPlanet:
		UpdatePlanetGravity();
		break;
	case EGravityType::OnShip:
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

	// ��������� ������� ������ �� ������� ����
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

	// switch gravity param

}

void AGravityCharacterPawn::SwitchGravityToZeroG(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToZERO-G")));
	CurrentGravityType = EGravityType::ZeroG;
	GravityTargetActor = nullptr;
}

void AGravityCharacterPawn::UpdateAnimationState()
{
	// �������� ��������� ������������� �������� ��������� ����������
	FString AnimationStateString = StaticEnum<EAnimationState>()->GetNameStringByValue(static_cast<int32>(CurrentAnimationState));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("AnimationStateString: %s"), *AnimationStateString));
	
	// �������� ������ �� ���
	UWorld* World = GetWorld();

	if (World)
	{
		// ����������, �� ������� ������ ���� ������, ����� ��������� ��� ������ ���������
		const float GroundDistanceThreshold = 10.0f;
		const float JumpDistanceThreshold = 250.0f;
		
		
		// ��������� ������� �����������
		FVector StartLocation = GetActorLocation();

		// �������� ������� �����������
		FVector EndLocation = StartLocation - (GetActorUpVector() * JumpDistanceThreshold);

		// ��������� ����������� ��� ��������
		FHitResult AnimHitResult;

		// ��������� ��������
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		// ���������� Line Trace ��� ����������� ��������� ��������
		bool bIsGrounded = World->LineTraceSingleByChannel(AnimHitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

		// ����� �������� AnimHitResult.Distance �� �����
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("AnimHitResult.Distance: %f"), AnimHitResult.Distance));


		//c ����������� ��������� ��������
		if (bIsGrounded && AnimHitResult.Distance < CapsuleComponent->GetScaledCapsuleHalfHeight() + 2.5f)
		{
			// �������� ����� �� �����������
			CurrentAnimationState = EAnimationState::OnGround;
		}
		else if (bIsGrounded && AnimHitResult.Distance > GroundDistanceThreshold)
		{
			// �������� �������
			CurrentAnimationState = EAnimationState::Jumping;
		}
		else
		{
			// �������� ������ (������ �� ������)
			CurrentAnimationState = EAnimationState::Falling;
		}
	}
}

void AGravityCharacterPawn::UpdateGravityState()
{
	// �������� ��������� ������������� �������� ��������� ����������
	FString GravityStateString = StaticEnum<EGravityState>()->GetNameStringByValue(static_cast<int32>(CurrentGravityState));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("GravityStateString: %s"), *GravityStateString));

	// �������� ������ �� ���
	UWorld* World = GetWorld();

	if (World)
	{
		const float GravityDistanceThreshold = 5000.0f;

		// ��������� ������� �����������
		FVector StartLocation = GetActorLocation();

		// �������� ������� �����������
		FVector EndLocation = StartLocation - (GetActorUpVector() * GravityDistanceThreshold);

		// ��������� �����������
		FHitResult HitResult;

		// ��������� ��������
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		// ���������� Line Trace By Channel
		bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

		// ����������� ���������� ����� �����������
		FColor DebugLineColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(World, StartLocation, EndLocation, DebugLineColor, false, 2.0f, 0, 2.0f);

		if (bHit)
		{
			CurrentGravityState = EGravityState::Attracting;
			//// ���� ���-�� ���� ������, ����������� ���������
			//AActor* HitActor = HitResult.GetActor();
			//if (HitActor)
			//{
			//	// ������� ���-�� � �������, �������� �� ������
			//}
		}
		else
		{
			CurrentGravityState = EGravityState::LowG;
		}
	}
}

void AGravityCharacterPawn::UpdateZeroGGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ZeroG Gravity")));
}

void AGravityCharacterPawn::UpdateStationRotation()
{
	// Rotation Z and X
	const FVector GravityRotZ = GravityTargetActor->GetActorUpVector();
	const FVector GravityRotX = CapsuleComponent->GetForwardVector();

	// Make Rot from ZX
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityRotZ, GravityRotX);
	const FRotator Rotation = RotationMatrix.Rotator();

	// Interp to Rot
	const FRotator ActorRotation = GetActorRotation();
	FRotator Result = FMath::RInterpTo(ActorRotation, Rotation, GetWorld()->GetDeltaSeconds(), 5.f);

	SetActorRotation(Result);
}

void AGravityCharacterPawn::UpdateStationGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Station Gravity")));

	UpdateStationRotation();

	UpdateGravityState();

	UpdateAnimationState();


	/// CHECK GRAVITY FORCE / EFFECT
	if (CurrentGravityState != EGravityState::LowG)
	{
		// ���������� �������������� ���� � ���������
		const float GravityStrength = -980.0f; // ��������, ���� ���������� �����
		FVector GravityForce = GravityTargetActor->GetActorUpVector() * GravityStrength;
		CapsuleComponent->AddForce(GravityForce, "none", true);
	}

	// Set SpringCameraArm Relative Roll always 0
	FRotator CamRot = CameraSpringArm->GetRelativeRotation();
	CameraSpringArm->SetRelativeRotation(FRotator(CamRot.Pitch, CamRot.Yaw, 0.0f));

	// �������� ������� �������� CameraSpringArm
	FRotator CameraSpringArmRotation = CameraSpringArm->GetRelativeRotation();

	// ���������� ����� �������� Realtive Yaw ��� Arrow Component from CameraSpringArmRotation
	FRotator NewArrowRotation(0.0f, CameraSpringArmRotation.Yaw, 0.0f);
	ArrowComponent->SetRelativeRotation(NewArrowRotation);
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
	const FRotator ResultRotation = FMath::RInterpTo(ActorRotation, RotationMatrix.Rotator(), GetWorld()->GetDeltaSeconds(), 8.f);
	
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

void AGravityCharacterPawn::AlignCharacterToCameraZeroG()
{
	// �������� ������� ���������� �������� ������
	const FQuat CameraQuat = CameraSpringArm->GetComponentQuat();

	// ��������� ����� ���������� �������� ���������, ��������� ���������� �������� ������
	FQuat NewCharacterQuat = FQuat(CameraQuat.X, CameraQuat.Y, CameraQuat.Z, CameraQuat.W);

	// ������������� �������� CapsuleComponent � ��������� � �������� ������
	FQuat InterpolatedQuat = FMath::QInterpTo(GetActorQuat(), NewCharacterQuat, GetWorld()->GetDeltaSeconds(), CameraInterpolationSpeed);
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// �������� �������� CameraSpringArm
	CameraSpringArm->SetWorldRotation(FRotator(CameraQuat.Rotator().Pitch, NewCharacterQuat.Rotator().Yaw, CameraQuat.Rotator().Roll));
}

void AGravityCharacterPawn::AlignCharacterToCameraOnStation()
{
	// �������� ������� ���������� �������� �������
	const FQuat CapsuleQuat = CapsuleComponent->GetComponentQuat();
	// �������� ������� ���������� �������� SpringArm
	const FQuat SpringArmQuat = ArrowComponent->GetComponentQuat();

	// ������������� �������� ������� � �������� �������� SpringArm
	FQuat InterpolatedQuat = FMath::QInterpTo(CapsuleQuat, SpringArmQuat, GetWorld()->GetDeltaSeconds(), 5.0f);
	// ��������� ������� ����� �������� � ����������������� ���������
	FQuat DifferenceQuat = CapsuleQuat.Inverse() * InterpolatedQuat;

	// ������������� ����� �������� ��� ������ � �������
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// ��������� �������� ������������ ��� SpringArm
	FQuat NewSpringArmQuat = CameraSpringArm->GetComponentQuat() * DifferenceQuat.Inverse();
	CameraSpringArm->SetWorldRotation(NewSpringArmQuat);
}

void AGravityCharacterPawn::MoveForward(const float Value)
{
	if (Value != 0)
	{
		switch (CurrentGravityType) 
		{
			case EGravityType::OnStation:
				MoveForwardOnStation(Value);
				break;
			case EGravityType::OnPlanet:
				MoveForwardOnPlanet(Value);
				break;
			case EGravityType::OnShip:
				MoveForwardOnShip(Value);
				break;
			case EGravityType::ZeroG:
				MoveForwardZeroG(Value);
				break;
		}
	}
}

void AGravityCharacterPawn::MoveRight(const float Value)
{
	if (Value != 0)
	{
		switch (CurrentGravityType)
		{
		case EGravityType::OnStation:
			MoveRightOnStation(Value);
			break;
		case EGravityType::OnPlanet:
			MoveRightOnPlanet(Value);
			break;
		case EGravityType::OnShip:
			MoveRightOnShip(Value);
			break;
		case EGravityType::ZeroG:
			MoveRightZeroG(Value);
			break;
		}
	}
}

void AGravityCharacterPawn::MoveForwardOnStation(const float Value)
{
	/*switch (CurrentGravityState)
	{
	case EGravityState::LowG:
		break;
	case EGravityState::Attracting:
		break;
	case EGravityState::Jumping:
		break;
	case EGravityState::Falling:
		break;
	case EGravityState::Attracted:
		break;
	default:
		break;
	}*/

	AlignCharacterToCameraOnStation();

	// Add movement force to capsule 
	FVector ArrowForwardVector = ArrowComponent->GetForwardVector();
	CapsuleComponent->AddForce(ArrowForwardVector * (Value * CharacterMovementScale * 100), "None", true);
}
void AGravityCharacterPawn::MoveRightOnStation(const float Value)
{
	AlignCharacterToCameraOnStation();

	FVector ArrowRightVector = ArrowComponent->GetRightVector();
	CapsuleComponent->AddForce(ArrowRightVector * (Value * CharacterMovementScale * 100), "None", true);
}


void AGravityCharacterPawn::MoveForwardOnPlanet(const float Value)
{

}

void AGravityCharacterPawn::MoveForwardOnShip(const float Value)
{

}

void AGravityCharacterPawn::MoveForwardZeroG(const float Value)
{
	AlignCharacterToCameraZeroG();
	CapsuleComponent->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementScale), "None", true);
}


void AGravityCharacterPawn::MoveRightOnPlanet(const float Value)
{
	
}
void AGravityCharacterPawn::MoveRightOnShip(const float Value)
{
	
}
void AGravityCharacterPawn::MoveRightZeroG(const float Value)
{
	AlignCharacterToCameraZeroG();
	CapsuleComponent->AddImpulse(GetActorRightVector() * (Value * CharacterMovementScale), "None", true);
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
	if (Value != 0)
	{
		//CurrentGravityState, CurrentGravityType
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);

		switch (CurrentGravityType)
		{
		case EGravityType::ZeroG:
			RotationToAdd.Pitch = Value * CharacterRotationScale;
			AddActorLocalRotation(RotationToAdd);
			break;
		case EGravityType::OnStation:
			if (CurrentGravityState == EGravityState::LowG)
			{
				RotationToAdd.Pitch = Value * CharacterRotationScale;
				AddActorLocalRotation(RotationToAdd);
			}
			break;
		case EGravityType::OnPlanet:
			break;
		case EGravityType::OnShip:
			break;
		default:
			break;
		}
	}
}

void AGravityCharacterPawn::RotateRoll(const float Value)
{
	if (Value != 0)
	{
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);

		switch (CurrentGravityType)
		{
		case EGravityType::ZeroG:
			RotationToAdd.Roll = Value * CharacterRotationScale;
			AddActorLocalRotation(RotationToAdd);
			break;
		case EGravityType::OnStation:
			if (CurrentGravityState == EGravityState::LowG)
			{
				RotationToAdd.Roll = Value * CharacterRotationScale;
				AddActorLocalRotation(RotationToAdd);
			}
			break;
		case EGravityType::OnPlanet:
			break;
		case EGravityType::OnShip:
			break;
		default:
			break;
		}
	}
}

void AGravityCharacterPawn::RotateYaw(const float Value)
{
	if (Value != 0)
	{
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
		RotationToAdd.Yaw = Value * CharacterRotationScale;
		AddActorLocalRotation(RotationToAdd);
		/*switch (CurrentGravityType)
		{
		case EGravityType::ZeroG:
			RotationToAdd.Yaw = Value * CharacterRotationScale;
			AddActorLocalRotation(RotationToAdd);
			break;
		case EGravityType::OnStation:
			if (CurrentGravityState == EGravityState::LowG)
			{
				RotationToAdd.Yaw = Value * CharacterRotationScale;
				AddActorLocalRotation(RotationToAdd);
			}
			break;
		case EGravityType::OnPlanet:
			break;
		case EGravityType::OnShip:
			break;
		default:
			break;
		}*/
	}
}
