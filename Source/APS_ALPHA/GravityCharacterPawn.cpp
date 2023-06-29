// Fill out your copyright notice in the Description page of Project Settings.
#include "GravityCharacterPawn.h"

#include "GravityMovementStruct.h"
#include "GravityParamStruct.h"
#include "GravityCharacterPawn.h"
#include "Spaceship.h"
#include "SpaceStation.h"
#include "OrbitalBody.h"
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>

// Sets default values
AGravityCharacterPawn::AGravityCharacterPawn()
{
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

	// Создайте Arrow компонент
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(CapsuleComponent);

	// Set Gravity Collision Profile
	CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_EngineTraceChannel3 , ECollisionResponse::ECR_Ignore);
}


// Called when the game starts or when spawned
void AGravityCharacterPawn::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlaps
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AGravityCharacterPawn::OnBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &AGravityCharacterPawn::OnEndOverlap);

	// Set initial gravity type
	UpdateGravityType();
}

// Called every frame
void AGravityCharacterPawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apply Gravity by GravityType
	UpdateGravity();

	// Compute and display Character Velocity
	const FVector CurrentVelocity = CapsuleComponent->GetPhysicsLinearVelocity();
	ForwardSpeed = FVector::DotProduct(CurrentVelocity, GetActorForwardVector());
	RightSpeed = FVector::DotProduct(CurrentVelocity, GetActorRightVector());
	UpSpeed = FVector::DotProduct(CurrentVelocity, GetActorUpVector());
	float LinearDamping = CapsuleComponent->GetLinearDamping();
	float AngularDamping = CapsuleComponent->GetAngularDamping();
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("ForwardSpeed: %f"), ForwardSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("RightSpeed: %f"), RightSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("UpSpeed: %f"), UpSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("AngularDamping: %f"), AngularDamping));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("LinearDamping: %f"), LinearDamping));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("CharacterJumpForce: %f"), CharacterJumpForce));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("CharacterMovementForce: %f"), CharacterMovementForce));

	// Char Location
	FString LocationString = GetActorLocation().ToString();
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("PlayerCharacter Location: %s"), *LocationString));

	// Current Ship
	if (CurrentSpaceship) {
		FString SpaceshipName = CurrentSpaceship->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, FString::Printf(TEXT("Current Spaceship: %s"), *SpaceshipName));
	}
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

	PlayerInputComponent->BindAction("CharacterAction", IE_Pressed, this, &AGravityCharacterPawn::CharacterAction);

}

void AGravityCharacterPawn::CharacterAction()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("CharacterAction!")));

	/*TArray<AActor*> overlappingActors;
	GetOverlappingActors(overlappingActors, TSubclassOf<AActor>()); */ // get all types of actors


	if (CurrentGravityType == EGravityType::OnShip)
	{
		if (CurrentSpaceship && isAllowedToControlSpaceship) 
		{
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				CurrentSpaceship->TakeControl(this);
				// Possess the spaceship
				PlayerController->Possess(CurrentSpaceship);

				// Attach the pawn to the PilotChair of the spaceship
				FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
				AttachToComponent(CurrentSpaceship->PilotChair, AttachmentRules);

				// Disable pawn's input and movement
				//DisableInput(PlayerController);
				SetActorEnableCollision(false);
				SetActorTickEnabled(false);
				CapsuleComponent->SetSimulatePhysics(false);
				AddActorLocalRotation(FRotator(0.0, 180.0, 0.0));

			}
		}
	}
	//for (AActor* actor : overlappingActors)
	//{
	//	if (actor->GetClass()->ImplementsInterface(UIVehicleControlling::StaticClass()))
	//	{
	//		// This actor implements the IVehicleControlling interface.
	//		// You can cast it to the interface type and call methods on it, like so:

	//		IIVehicleControlling* vehicleController = Cast<IIVehicleControlling>(actor);
	//		if (vehicleController)
	//		{
	//			// call methods on vehicleController here
	//		}
	//	}
	//}

}

void AGravityCharacterPawn::ReleaseControl(APilotingVehicle* PilotingVehicle)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Orange, FString::Printf(TEXT("AGravityCharacterPawn ReleaseControl")));
	CurrentGravityType = EGravityType::OnShip;

	// Cast the PilotingVehicle to ASpaceship
	ASpaceship* Spaceship = Cast<ASpaceship>(PilotingVehicle);

	if (Spaceship)
	{
		CurrentSpaceship = Spaceship;

		CapsuleComponent->SetSimulatePhysics(true);
		SetActorLocation(CurrentSpaceship->PilotChair->GetComponentLocation());
		AddActorLocalOffset(FVector(0.0, -160.0, 0.0));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast APilotingVehicle to ASpaceship"));
	}

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

void AGravityCharacterPawn::UpdateGravityPhysicParams()
{
	FGravityParamStruct GravityParams;
	FGravityMovementStruct GravityMovements;
	double AngularDamping {0}, LinearDamping{0};
	
	switch (CurrentGravityType)
	{
	case EGravityType::ZeroG:
		AngularDamping = GravityParams.AngularDampingZeroG;
		LinearDamping = GravityParams.LinearDampingZeroG;
		CharacterMovementForce = GravityMovements.MovementsForceSpeedZeroG;
		CharacterJumpForce = GravityMovements.JumpForceSpeedZeroG;
		break;

	case EGravityType::OnStation:
		if (CurrentGravityState == EGravityState::LowG)
		{
			AngularDamping = GravityParams.AngularDampingLowG;
			LinearDamping = GravityParams.LinearDampingLowG;
			CharacterMovementForce = GravityMovements.MovementsForceSpeedLowG;
			CharacterJumpForce = GravityMovements.JumpForceSpeedLowG;
		}
		else
		{
			AngularDamping = GravityParams.AngularDampingStation;
			LinearDamping = GravityParams.LinearDampingStation;
			CharacterMovementForce = GravityMovements.MovementsForceSpeedStation;
			CharacterJumpForce = GravityMovements.JumpForceSpeedStation;
		}
		break;

	case EGravityType::OnPlanet:
		AngularDamping = GravityParams.AngularDampingPlanet;
		LinearDamping = GravityParams.LinearDampingPlanet;
		CharacterMovementForce = GravityMovements.MovementsForceSpeedPlanet;
		CharacterJumpForce = GravityMovements.JumpForceSpeedPlanet;
		break;

	case EGravityType::OnShip:
		AngularDamping = GravityParams.AngularDampingShip;
		LinearDamping = GravityParams.LinearDampingShip;
		CharacterMovementForce = GravityMovements.MovementsForceSpeedShip;
		CharacterJumpForce = GravityMovements.JumpForceSpeedShip;
		break;
	}

	CapsuleComponent->SetAngularDamping(AngularDamping);
	CapsuleComponent->SetLinearDamping(LinearDamping);
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
		AActor* LastGravityActor = OverlappingActorsWithTag[OverlappingActorsWithTag.Num()-1];
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("LastGravityActor : %s"), *LastGravityActor->GetName()));

		// switch gravity to first 
		SwitchGravityType(LastGravityActor);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("LastGravityActor 0")));
		CurrentGravityType = EGravityType::ZeroG;
		UpdateGravityPhysicParams();
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
		ASpaceship* Spaceship = Cast<ASpaceship>(GravitySourceActor);
		if (Spaceship != nullptr)
		{
			CurrentSpaceship = Spaceship;
		}
	}
	// Remove ship
	if (CurrentGravityType != EGravityType::OnShip)
	{
		CurrentSpaceship = nullptr;
	}

	// switch gravity param
	UpdateGravityPhysicParams();
}

void AGravityCharacterPawn::SwitchGravityToZeroG(AActor* OtherActor)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, FString::Printf(TEXT("SwitchGravityToZERO-G")));
	CurrentGravityType = EGravityType::ZeroG;
	GravityTargetActor = nullptr;
}

void AGravityCharacterPawn::UpdateAnimationState()
{
	// Получить строковое представление текущего состояния гравитации
	FString AnimationStateString = StaticEnum<EAnimationState>()->GetNameStringByValue(static_cast<int32>(CurrentAnimationState));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("AnimationStateString: %s"), *AnimationStateString));
	
	// Получить ссылку на мир
	UWorld* World = GetWorld();

	if (World)
	{
		// Расстояние, на котором должен быть объект, чтобы считаться под ногами персонажа
		const float GroundDistanceThreshold = 10.0f;
		const float JumpDistanceThreshold = 250.0f;
		
		// Начальная позиция трассировки
		FVector StartLocation = GetActorLocation();

		// Конечная позиция трассировки
		FVector EndLocation = StartLocation - (GetActorUpVector() * JumpDistanceThreshold);

		// Результат трассировки для анимации
		FHitResult AnimHitResult;

		// Настройка коллизии
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		// Выполнение Line Trace для определения состояния анимации
		bool bIsGrounded = World->LineTraceSingleByChannel(AnimHitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

		// Вывод значения AnimHitResult.Distance на экран
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("AnimHitResult.Distance: %f"), AnimHitResult.Distance));

		float JumpDeadZone = 3.5f;

		//c Определение состояния анимации
		if (bIsGrounded && AnimHitResult.Distance < CapsuleComponent->GetScaledCapsuleHalfHeight() + JumpDeadZone)
		{
			// Персонаж стоит на поверхности
			CurrentAnimationState = EAnimationState::OnGround;
		}
		else if (bIsGrounded && AnimHitResult.Distance > GroundDistanceThreshold)
		{
			// Персонаж прыгает
			CurrentAnimationState = EAnimationState::Jumping;
		}
		else
		{
			// Персонаж падает (ничего не задето)
			CurrentAnimationState = EAnimationState::Falling;
		}
	}
}

void AGravityCharacterPawn::UpdateGravityState()
{
	// Получить строковое представление текущего состояния гравитации
	FString GravityStateString = StaticEnum<EGravityState>()->GetNameStringByValue(static_cast<int32>(CurrentGravityState));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("GravityStateString: %s"), *GravityStateString));

	// Получить ссылку на мир
	UWorld* World = GetWorld();

	if (World)
	{
		const float GravityDistanceThreshold = 15000.0f; // TODO: Distance to gravity

		// Начальная позиция трассировки
		FVector StartLocation = GetActorLocation();

		// Конечная позиция трассировки
		FVector EndLocation = StartLocation - (GetActorUpVector() * GravityDistanceThreshold);

		// Результат трассировки
		FHitResult HitResult;

		// Настройка коллизии
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(this);

		// Выполнение Line Trace By Channel
		bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

		// Отображение отладочной линии трассировки
		FColor DebugLineColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(World, StartLocation, EndLocation, DebugLineColor, false, 2.0f, 0, 2.0f);

		if (bHit)
		{
			CurrentGravityState = EGravityState::Attracting;
			UpdateGravityPhysicParams();

			// Получаем расстояние между начальной точкой и точкой столкновения
			float DistanceToGround = (HitResult.ImpactPoint - StartLocation).Size();

			// Вычитаем CapsuleHalfHeight из расстояния до земли
			HeightAboveGround = DistanceToGround - CapsuleComponent->GetScaledCapsuleHalfHeight();

			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("HeightAboveGround: %f"), HeightAboveGround));
		}
		else
		{
			CurrentGravityState = EGravityState::LowG;
			UpdateGravityPhysicParams();
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


	// CHECK GRAVITY FORCE / EFFECT
	if (CurrentGravityState != EGravityState::LowG)
	{
		// Добавление гравитационной силы к персонажу
		const float GravityStrength = -980.0f; // Например, сила гравитации Земли
		FVector GravityForce = GravityTargetActor->GetActorUpVector() * GravityStrength;
		CapsuleComponent->AddForce(GravityForce, "none", true);
	}

	// Set SpringCameraArm Relative Roll always 0
	FRotator CamRot = CameraSpringArm->GetRelativeRotation();
	CameraSpringArm->SetRelativeRotation(FRotator(CamRot.Pitch, CamRot.Yaw, 0.0f));

	// Получить текущее вращение CameraSpringArm
	FRotator CameraSpringArmRotation = CameraSpringArm->GetRelativeRotation();

	// Установить новое вращение Realtive Yaw для Arrow Component from CameraSpringArmRotation
	FRotator NewArrowRotation(0.0f, CameraSpringArmRotation.Yaw, 0.0f);
	ArrowComponent->SetRelativeRotation(NewArrowRotation);
}

void AGravityCharacterPawn::UpdatePlanetGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Planet Gravity")));

	// align rotation with planet
	const FVector GravityTargetLocation = GravityTargetActor->GetActorLocation();
	const FVector ActorLocation = GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GravityTargetLocation, ActorLocation);
	const FVector CapsuleForwardVector = CapsuleComponent->GetForwardVector();
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(LookAtRotation.Vector(), CapsuleForwardVector);
	const FRotator ActorRotation = GetActorRotation();
	const FRotator ResultRotation = FMath::RInterpTo(ActorRotation, RotationMatrix.Rotator(), GetWorld()->GetDeltaSeconds(), 5.f);
	SetActorRotation(ResultRotation);

	// Добавление гравитационной силы к персонажу
	const float GravityStrength = -980.0f; // Например, сила гравитации Земли
	GravityDirection = (GravityTargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector GravityForce = GravityDirection * GravityStrength * -1;
	CapsuleComponent->AddForce(GravityForce, "none", true);

	// 
	UpdateAnimationState();

	// Set SpringCameraArm Relative Roll always 0
	FRotator CamRot = CameraSpringArm->GetRelativeRotation();
	CameraSpringArm->SetRelativeRotation(FRotator(CamRot.Pitch, CamRot.Yaw, 0.0f));

	// Получить текущее вращение CameraSpringArm
	FRotator CameraSpringArmRotation = CameraSpringArm->GetRelativeRotation();

	// Установить новое вращение Realtive Yaw для Arrow Component from CameraSpringArmRotation
	FRotator NewArrowRotation(0.0f, CameraSpringArmRotation.Yaw, 0.0f);
	ArrowComponent->SetRelativeRotation(NewArrowRotation);
}

void AGravityCharacterPawn::UpdateShipGravity()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Ship Gravity")));

	const FVector GravityRotZ = GravityTargetActor->GetActorUpVector();
	const FVector GravityRotX = CapsuleComponent->GetForwardVector();
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(GravityRotZ, GravityRotX);
	const FRotator Rotation = RotationMatrix.Rotator();
	const FRotator ActorRotation = GetActorRotation();
	const FRotator Result = FMath::RInterpTo(ActorRotation, Rotation, GetWorld()->GetDeltaSeconds(), 5.f);
	SetActorRotation(Result);

	//
	UpdateGravityState();

	//
	UpdateAnimationState();

	// CHECK GRAVITY FORCE / EFFECT
	if (CurrentGravityState != EGravityState::LowG)
	{
		// Добавление гравитационной силы к персонажу
		const float GravityStrength = -980.0f; // Например, сила гравитации Земли
		FVector GravityForce = GravityTargetActor->GetActorUpVector() * GravityStrength;
		CapsuleComponent->AddForce(GravityForce, "none", true);
	}

	// Set SpringCameraArm Relative Roll always 0
	FRotator CamRot = CameraSpringArm->GetRelativeRotation();
	CameraSpringArm->SetRelativeRotation(FRotator(CamRot.Pitch, CamRot.Yaw, 0.0f));

	// Получить текущее вращение CameraSpringArm
	FRotator CameraSpringArmRotation = CameraSpringArm->GetRelativeRotation();

	// Установить новое вращение Realtive Yaw для Arrow Component from CameraSpringArmRotation
	FRotator NewArrowRotation(0.0f, CameraSpringArmRotation.Yaw, 0.0f);
	ArrowComponent->SetRelativeRotation(NewArrowRotation);

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

void AGravityCharacterPawn::AlignCharacterToCameraOnStation()
{
	// Получаем текущий кватернион вращения капсулы
	const FQuat CapsuleQuat = CapsuleComponent->GetComponentQuat();
	// Получаем текущий кватернион вращения SpringArm
	const FQuat ArrowForwardVector = ArrowComponent->GetComponentQuat();

	// Интерполируем вращение капсулы к целевому вращению SpringArm
	FQuat InterpolatedQuat = FMath::QInterpTo(CapsuleQuat, ArrowForwardVector, GetWorld()->GetDeltaSeconds(), 5.0f);
	// Вычисляем разницу между исходным и интерполированным вращением
	FQuat DifferenceQuat = CapsuleQuat.Inverse() * InterpolatedQuat;

	// Устанавливаем новое вращение для актора и капсулы
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// Применяем обратную интерполяцию для SpringArm
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
	AlignCharacterToCameraOnStation();

	FVector ArrowForwardVector = ArrowComponent->GetForwardVector();
	CapsuleComponent->AddForce(ArrowForwardVector * (Value * CharacterMovementForce), "None", true);
}
void AGravityCharacterPawn::MoveRightOnStation(const float Value)
{
	AlignCharacterToCameraOnStation();

	FVector ArrowRightVector = ArrowComponent->GetRightVector();
	CapsuleComponent->AddForce(ArrowRightVector * (Value * CharacterMovementForce), "None", true);
}

void AGravityCharacterPawn::MoveForwardOnPlanet(const float Value)
{
	// Получаем текущий кватернион вращения капсулы
	const FQuat CapsuleQuat = CapsuleComponent->GetComponentQuat();
	// Получаем текущий кватернион вращения SpringArm
	const FQuat ArrowForwardVector = ArrowComponent->GetComponentQuat();

	// Интерполируем вращение капсулы к целевому вращению SpringArm
	FQuat InterpolatedQuat = FMath::QInterpTo(CapsuleQuat, ArrowForwardVector, GetWorld()->GetDeltaSeconds(), 5.0f);
	// Вычисляем разницу между исходным и интерполированным вращением
	FQuat DifferenceQuat = CapsuleQuat.Inverse() * InterpolatedQuat;

	// Устанавливаем новое вращение для актора и капсулы
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// Применяем обратную интерполяцию для SpringArm
	FQuat NewSpringArmQuat = CameraSpringArm->GetComponentQuat() * DifferenceQuat.Inverse();
	CameraSpringArm->SetWorldRotation(NewSpringArmQuat);

	//
	FVector ArrowForwardVectorV = ArrowComponent->GetForwardVector();
	CapsuleComponent->AddForce(ArrowForwardVectorV * (Value * CharacterMovementForce), "None", true);
}

void AGravityCharacterPawn::MoveForwardOnShip(const float Value)
{
	//
	MoveForwardOnStation(Value);
}

void AGravityCharacterPawn::MoveForwardZeroG(const float Value)
{
	AlignCharacterToCameraZeroG();
	CapsuleComponent->AddImpulse(GetActorForwardVector() * (Value * CharacterMovementForce), "None", true);
}


void AGravityCharacterPawn::MoveRightOnPlanet(const float Value)
{
	// Получаем текущий кватернион вращения капсулы
	const FQuat CapsuleQuat = CapsuleComponent->GetComponentQuat();
	// Получаем текущий кватернион вращения SpringArm
	const FQuat ArrowForwardVector = ArrowComponent->GetComponentQuat();

	// Интерполируем вращение капсулы к целевому вращению SpringArm
	FQuat InterpolatedQuat = FMath::QInterpTo(CapsuleQuat, ArrowForwardVector, GetWorld()->GetDeltaSeconds(), 5.0f);
	// Вычисляем разницу между исходным и интерполированным вращением
	FQuat DifferenceQuat = CapsuleQuat.Inverse() * InterpolatedQuat;

	// Устанавливаем новое вращение для актора и капсулы
	CapsuleComponent->SetWorldRotation(InterpolatedQuat);
	SetActorRotation(InterpolatedQuat);

	// Применяем обратную интерполяцию для SpringArm
	FQuat NewSpringArmQuat = CameraSpringArm->GetComponentQuat() * DifferenceQuat.Inverse();
	CameraSpringArm->SetWorldRotation(NewSpringArmQuat);

	//
	FVector ArrowRightVector = ArrowComponent->GetRightVector();
	CapsuleComponent->AddForce(ArrowRightVector * (Value * CharacterMovementForce), "None", true);

}
void AGravityCharacterPawn::MoveRightOnShip(const float Value)
{
	//
	MoveRightOnStation(Value);
}
void AGravityCharacterPawn::MoveRightZeroG(const float Value)
{
	AlignCharacterToCameraZeroG();
	CapsuleComponent->AddImpulse(GetActorRightVector() * (Value * CharacterMovementForce), "None", true);
}

void AGravityCharacterPawn::MoveUp(const float Value)
{
	if (Value != 0)
	{		
		switch (CurrentGravityType)
		{
		case EGravityType::OnStation:
		{
			CapsuleComponent->AddImpulse(GetActorUpVector() * (Value * CharacterJumpForce), "None", true);
			break;
		}
		case EGravityType::OnPlanet:
		{

			// Получаем вектор гравитации (от персонажа к центру планеты)
			FVector JumpGravityDirection = (GravityTargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();

			// Добавляем силу в направлении, противоположном гравитации
			CapsuleComponent->AddForce(-JumpGravityDirection * CharacterJumpForce * Value, "none", true);
			break;
		}

		case EGravityType::OnShip:
			CapsuleComponent->AddImpulse(GetActorUpVector() * (Value * CharacterJumpForce), "None", true);
			break;
		case EGravityType::ZeroG:
			CapsuleComponent->AddImpulse(GetActorUpVector() * (Value * CharacterJumpForce), "None", true);
			break;
		}
	}
}

void AGravityCharacterPawn::RotatePitch(const float Value)
{
	if (Value != 0)
	{
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
	}
}
