// Fill out your copyright notice in the Description page of Project Settings.


#include "APS_GravityCharacter.h"

#include "APS_ALPHA/Actors/Astro/OrbitalBody.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/GravityState.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AAPS_GravityCharacter::AAPS_GravityCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create and set up SpringArmComponent
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(GetCapsuleComponent());
	CameraSpringArm->TargetArmLength = 300.0f;
	CameraSpringArm->bUsePawnControlRotation = false;

	// Create and set up CameraComponent
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);
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

	if (!bIsManualZeroG && CurrentGravityType == EGravityType::OnStation)
	{
		UpdateGravityState();
	}
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

	PlayerInputComponent->BindAxis("Turn", this, &AAPS_GravityCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AAPS_GravityCharacter::LookUp);
}


void AAPS_GravityCharacter::EnableZeroG()
{

	//OnEnableZeroG();
	
	if (!bIsZeroG)
	{
		bIsZeroG = true;
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("EnableZeroG")));

		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Flying);
			//MovementComponent->GravityScale = 0.f;
			//MovementComponent->bOrientRotationToMovement = false;
			//MovementComponent->BrakingFriction = 0.f;
			//MovementComponent->BrakingDecelerationFlying = 0.f;
		}

		/*bUseControllerRotationYaw   = false;
		bUseControllerRotationPitch = false;
		bUseControllerRotationRoll  = false;*/

		/*if (USpringArmComponent* Boom = FindComponentByClass<USpringArmComponent>())
		{
			Boom->bUsePawnControlRotation = true;
			Boom->bEnableCameraLag = false;
			Boom->bEnableCameraRotationLag = false;
		}*/
	}
}


void AAPS_GravityCharacter::DisableZeroG(float InGravityScale)
{
	//OnDisableZeroG();
	
	if (bIsZeroG)
	{
		bIsZeroG = false;
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("DisableZeroG")));


		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
			//MovementComponent->GravityScale = InGravityScale;

			// ДЕФОЛТ: поворачиваемся по направлению движения (нормальная ходьба вбок работает)
			//MovementComponent->bOrientRotationToMovement = true;

			// Вернуть стандартные тормоза (если менял)
			//MovementComponent->BrakingFriction = 2.f;
			//MovementComponent->BrakingDecelerationWalking = 2048.f;
		}

		// ДЕФОЛТ: актёр не крутится от контроллера (камера — отдельно)
		/*bUseControllerRotationYaw   = false;
		bUseControllerRotationPitch = false;
		bUseControllerRotationRoll  = false;*/

		// Камера продолжает крутиться мышью
		/*if (USpringArmComponent* Boom = FindComponentByClass<USpringArmComponent>())
		{
			Boom->bUsePawnControlRotation = true;
		}*/

		// На всякий: убрать возможный крен камеры после Zero-G
		/*if (AController* PC = Controller)
		{
			const FRotator R = PC->GetControlRotation();
			PC->SetControlRotation(FRotator(R.Pitch, R.Yaw, 0.f));
		}*/
	}
}


void AAPS_GravityCharacter::UpdateGravityState()
{
	/*FString GravityStateString = StaticEnum<EGravityState>()->GetNameStringByValue(
		static_cast<int32>(CurrentGravityState));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
									 FString::Printf(TEXT("GravityStateString: %s"), *GravityStateString));*/

	if (UWorld* World = GetWorld())
	{
		const float GravityDistanceThreshold = 15000.0f; // TODO: Distance to gravity

		FVector StartLocation = GetActorLocation();
		FVector EndLocation = StartLocation - (GetActorUpVector() * GravityDistanceThreshold);
		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility,
		                                            CollisionParams);

		FColor DebugLineColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(World, StartLocation, EndLocation, DebugLineColor, false, 2.0f, 0, 2.0f);

		if (bHit)
		{
			/*CurrentGravityState = EGravityState::Attracting;
			UpdateGravityPhysicParams();*/

			float DistanceToGround = (HitResult.ImpactPoint - StartLocation).Size();
			HeightAboveGround = DistanceToGround - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow,
			                                 FString::Printf(TEXT("HeightAboveGround: %f"), HeightAboveGround));

			//bIsZeroG = false;
			//DisableZeroG(1);

			if (CurrentGravityState != EGravityState::Attracted)
			{
				CurrentGravityState = EGravityState::Attracting;
			}
			

		}
		else
		{
			CurrentGravityState = EGravityState::LowG;

			/*CurrentGravityState = EGravityState::LowG;
			UpdateGravityPhysicParams();*/
			//bIsZeroG = true;
			//EnableZeroG();
		}
	}
}

void AAPS_GravityCharacter::RotateYaw(float Value)
{
	if (Value != 0)
	{
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
		RotationToAdd.Yaw = Value * CharacterRotationScale;
		AddActorLocalRotation(RotationToAdd);
	}
	/*if (FMath::IsNearlyZero(Value)) return;

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
	}*/
}

void AAPS_GravityCharacter::RotatePitch(float Value)
{
	if (Value != 0)
	{
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);

		RotationToAdd.Pitch = Value * CharacterRotationScale;
		AddActorLocalRotation(RotationToAdd);
	}


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
	if (Value != 0)
	{
		FRotator RotationToAdd(0.0f, 0.0f, 0.0f);
		RotationToAdd.Roll = Value * CharacterRotationScale;
		AddActorLocalRotation(RotationToAdd);
	}


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

void AAPS_GravityCharacter::AlignCharacterToCameraZeroG()
{
	const FQuat CameraQuat = CameraSpringArm->GetComponentQuat();
	FQuat NewCharacterQuat = FQuat(CameraQuat.X, CameraQuat.Y, CameraQuat.Z, CameraQuat.W);
	FQuat InterpolatedQuat = FMath::QInterpTo(GetActorQuat(), NewCharacterQuat, GetWorld()->GetDeltaSeconds(),
	                                          CameraInterpolationSpeed);
	GetCapsuleComponent()->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	CameraSpringArm->SetWorldRotation(FRotator(CameraQuat.Rotator().Pitch, NewCharacterQuat.Rotator().Yaw,
	                                           CameraQuat.Rotator().Roll));
}

void AAPS_GravityCharacter::Turn(const float Value)
{
	if (CurrentGravityState != EGravityState::Attracted)
	{
		FRotator TargetRotation = CameraSpringArm->GetRelativeRotation();
		TargetRotation.Yaw += Value * CharacterRotationScale;
		CameraSpringArm->SetRelativeRotation(TargetRotation);
	}
}

void AAPS_GravityCharacter::LookUp(const float Value)
{
	if (CurrentGravityState != EGravityState::Attracted)
	{
		FRotator TargetRotation = CameraSpringArm->GetRelativeRotation();
		TargetRotation.Pitch += Value * CharacterRotationScale;
		CameraSpringArm->SetRelativeRotation(TargetRotation);
	}
}

void AAPS_GravityCharacter::MoveForward(float Value)
{
	if (CurrentGravityState != EGravityState::Attracted)
	{
		if (FMath::IsNearlyZero(Value)) return;


		AlignCharacterToCameraZeroG();
		GetCapsuleComponent()->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementForce), "None", true);
	}


	/*if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		const FRotator Cam = (Controller ? Controller->GetControlRotation() : GetActorRotation());
		const FVector Fwd = FRotationMatrix(Cam).GetUnitAxis(EAxis::X);
		AddMovementInput(Fwd, Value);
		// мгновенно выровнять по камере по yaw+pitch (roll не трогаем)
		const FRotator Target(Cam.Pitch, Cam.Yaw, GetActorRotation().Roll);
		Controller->SetControlRotation(Target);
		SetActorRotation(Target);
	}*/
}


void AAPS_GravityCharacter::MoveRight(float Value)
{
	
	if (CurrentGravityState != EGravityState::Attracted)
	{
		if (FMath::IsNearlyZero(Value)) return;
		AlignCharacterToCameraZeroG();
		GetCapsuleComponent()->AddImpulse(GetActorRightVector() * (Value * CharacterMovementForce), "None", true);
	}

	/*if (FMath::IsNearlyZero(Value)) return;


	if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		const FRotator Cam = (Controller ? Controller->GetControlRotation() : GetActorRotation());
		const FVector Right = FRotationMatrix(Cam).GetUnitAxis(EAxis::Y);
		AddMovementInput(Right, Value);
	}*/
}

void AAPS_GravityCharacter::MoveUp(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	if (GetCharacterMovement()->MovementMode == MOVE_Flying) // ZERO-G
	{
		GetCapsuleComponent()->AddImpulse(GetActorUpVector() * (Value * CharacterJumpForce), "None", true);

		/*const FRotator Rotation = (Controller ? Controller->GetControlRotation() : GetActorRotation());
		const FVector VectorUp = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Z);
		AddMovementInput(VectorUp, Value * 10);*/
	}
}

void AAPS_GravityCharacter::RunGravityCheck(ACharacter* Self)
{
	if (!Self) return;

	{
		//FName TagToCheck = "GravitySource";
		TArray<AActor*> GravitySources;
		TArray<AWorldActor*> WorldNavigatableActors;
		UGameplayStatics::GetAllActorsOfClass(Self->GetWorld(), AWorldActor::StaticClass(), GravitySources);
		TMap<AWorldActor*, double> ActorDistances;

		for (AActor* Actor : GravitySources)
		{
			if (Actor && Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
			{
				AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
				WorldNavigatableActors.Add(WorldNavigatableActor);

				double Distance = (FVector::Distance(Self->GetActorLocation(),
				                                     WorldNavigatableActor->GetActorLocation()) / 100000.0) -
					WorldNavigatableActor->RadiusKM;
				ActorDistances.Add(WorldNavigatableActor, Distance);
			}
		}

		WorldNavigatableActors.Sort([&](const AWorldActor& A, const AWorldActor& B)
		{
			return ActorDistances[&A] < ActorDistances[&B];
		});

		if (WorldNavigatableActors.Num() > 0)
		{
			AWorldActor* ClosestActor = WorldNavigatableActors[0];

			FString DebugMessageClosest = FString::Printf(
				TEXT("Closest Actor: %s \nDistance to surface: %f km \nAffectionRadiusKM: %f"),
				*ClosestActor->GetFName().ToString(), ActorDistances[ClosestActor],
				ClosestActor->AffectionRadiusKM);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, DebugMessageClosest);

			if (ActorDistances[ClosestActor] <= ClosestActor->AffectionRadiusKM)
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green,
					                                 FString::Printf(
						                                 TEXT("Affected Actor: %s"),
						                                 *ClosestActor->GetFName().ToString()));
				SwitchGravityType(ClosestActor);
			}
			else
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
					                                 TEXT("No Actor within AffectionRadiusKM"));
				CurrentGravityType = EGravityType::ZeroG;
				//UpdateGravityPhysicParams();
			}
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("No Closest Gravity Actor"));
			CurrentGravityType = EGravityType::ZeroG;
			//Self->UpdateGravityPhysicParams();
		}
	}
}

void AAPS_GravityCharacter::SwitchGravityType(AActor* GravitySourceActor)
{
	GravityTargetActor = GravitySourceActor;

	//ClosestBody = NewClosest;
	OnClosestGravityBodyChanged.Broadcast(GravitySourceActor);

	if (GravitySourceActor->IsA(ASpaceStation::StaticClass()) || GravitySourceActor->IsA(
		ASpaceHeadquarters::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnStation;
		//OnGravityPhysicsParamChanged.Broadcast();
	}
	else if (GravitySourceActor->IsA(AOrbitalBody::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnPlanet;
		//OnGravityPhysicsParamChanged.Broadcast();
	}
	else if (GravitySourceActor->IsA(ASpaceship::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnShip;
		ASpaceship* Spaceship = Cast<ASpaceship>(GravitySourceActor);
		if (Spaceship != nullptr)
		{
			CurrentSpaceship = Spaceship;
			//OnGravityPhysicsParamChanged.Broadcast();
		}
	}
	// Remove ship
	if (CurrentGravityType != EGravityType::OnShip)
	{
		CurrentSpaceship = nullptr;
	}

	// switch gravity param
	//UpdateGravityPhysicParams();
}
