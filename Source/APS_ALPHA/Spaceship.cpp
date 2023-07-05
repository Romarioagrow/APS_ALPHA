// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "WorldActor.h"
#include "Kismet/GameplayStatics.h"
#include "CelestialBody.h"
#include "Star.h"
#include "StarCluster.h"
#include "AstroGenerator.h"
#include "NavigatableBody.h"

void ASpaceship::UpdateNavigatableActorsForInterstellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterstellar"));

	WorldNavigatableActors.Empty();
	if (LastFlightMode == EFlightMode::Stellar)
	{
		ToggleScale();
		bIsScaled = true;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Downscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForStellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("UpdateNavigatableActorsForStellar"));

	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldActor::StaticClass(), WorldActors);
	for (AActor* Actor : WorldActors)
	{
		if (Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
		{
			AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
			WorldNavigatableActors.Add(WorldNavigatableActor);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("WorldActor: %s"), *WorldNavigatableActor->GetName()));
		}
	}

	if (LastFlightMode == EFlightMode::Interstellar)
	{
		ToggleScale();
		bIsScaled = false;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Upscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForInterplanetary()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterplanetary"));
}

ASpaceship::ASpaceship()
{
	// Создание компонента SpaceshipHull
	SpaceshipHull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipHull"));
	RootComponent = SpaceshipHull; 

	// Создание компонента SphereCollisionComponent и прикрепление его к SpaceshipHull
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	SphereCollisionComponent->SetupAttachment(SpaceshipHull);

	// Создание компонента PilotChair и прикрепление его к SpaceshipHull
	PilotChair = CreateDefaultSubobject<USceneComponent>(TEXT("PilotChair"));
	PilotChair->SetupAttachment(SpaceshipHull);

	ForwardVector = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardVector"));
	ForwardVector->SetupAttachment(SpaceshipHull);

	// Привязка делегатов к функциям
	OnInterstellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterstellar);
	OnStellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForStellar);
	OnInterplanetaryMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterplanetary);


	// Создайте и настройте компонент SpringArm
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 1000.0;
	SpringArmComponent->SetWorldLocation(FVector(2340.0, 0.0, 1280.0));
	//SpringArmComponent->SetWorldRotation(FRotator(0.0, -180.0, 0.0 ));
	//SpringArmComponent->bUsePawnControlRotation = true;

	// Создайте и настройте компонент камеры
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);


}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();

	GeneratedWorld = Cast<AAstroGenerator>(UGameplayStatics::GetActorOfClass(GetWorld(), AAstroGenerator::StaticClass()));
	GeneratedStarCluster = Cast<AStarCluster>(UGameplayStatics::GetActorOfClass(GetWorld(), AStarCluster::StaticClass()));

	OnboardComputer = NewObject<USpaceshipOnboardComputer>(this, TEXT("OnboardComputer"));
	if (OnboardComputer)
	{
		OnboardComputer->SpaceshipHull = SpaceshipHull;
		OnboardComputer->OffsetSystem = OffsetSystem;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OnboardComputer is nullptr!"));
	}

	if (!OffsetSystem)
	{
		OffsetSystem = Cast<AStarSystem>(UGameplayStatics::GetActorOfClass(GetWorld(), AStarSystem::StaticClass()));
		if (OffsetSystem)  
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("OffsetSystem successfully obtained!"));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to obtain OffsetSystem."));
		}
	}

	// UpdateNavigatableActors();	
	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldActor::StaticClass(), WorldActors);
	for (AActor* Actor : WorldActors)
	{
		if (Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
		{
			AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
			WorldNavigatableActors.Add(WorldNavigatableActor);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("WorldActor: %s"), *WorldNavigatableActor->GetName()));
		}
	}

	ComputeProximity();

}

void GetAttachedActorsRecursively(AActor* ParentActor, TArray<AActor*>& OutActors)
{
	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);
	for (AActor* ChildActor : AttachedActors)
	{
		OutActors.Add(ChildActor);
		GetAttachedActorsRecursively(ChildActor, OutActors);
	}
}

void ASpaceship::CalculateDistanceAndAddToZones(AWorldActor* WorldActor)
{
	if (WorldActor)
	{
		double DistanceSquared = 0;
		FVector ShipLocation = GetActorLocation();

		double InfluenceRadius = WorldActor->AffectionRadiusKM * 100000; // Конвертировать километры в юниты Unreal (1 unit = 1 cm)
		DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), ShipLocation) - FMath::Square(InfluenceRadius);

		// Получить родительского актора и вывести его на экран
		AActor* ParentActor = WorldActor->GetRootComponent()->GetAttachParent()->GetOwner();
		FString ParentName = ParentActor ? *ParentActor->GetName() : FString("No Parent");

		// Вывести сообщение на экран
		double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
		FString Message = FString::Printf(TEXT("Distance to %s (orbiting %s): %f kilometers."), *WorldActor->GetName(), *ParentName, DistanceInKm);

		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, Message);
	}
}


TSharedPtr<FStarModel> FindNearestStar(TMap<FVector, TSharedPtr<FStarModel>>& Stars, const FVector& ReferencePoint)
{
	TSharedPtr<FStarModel> NearestStar = nullptr;
	float NearestDistanceSquared = FLT_MAX;

	for (const auto& KeyValuePair : Stars)
	{
		float TotalDistSquared = FVector::DistSquared(ReferencePoint, KeyValuePair.Key);
		float RadiusSquared = FMath::Square(KeyValuePair.Value->Radius);
		float DistanceSquared = TotalDistSquared > RadiusSquared ? TotalDistSquared - RadiusSquared : 0.0f;

		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestStar = KeyValuePair.Value;
		}
	}

	return NearestStar;
}
void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	uint64 StartCycles = FPlatformTime::Cycles();

	/*
		switch affection zone
		cazlulate distances and zones in current zone
		switch zones by affection type
	*/

	if (bEngineRunning)
	{
		PrintOnboardComputerBasicIformation();

		if (SpaceshipHull->IsSimulatingPhysics())
		{
			FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
			SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);
		}
		if (bIsAccelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Accelerating")));
			OnboardComputer->AccelerateBoost(DeltaTime);
		}
		if (bIsDecelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Decelerating")));
			OnboardComputer->DecelerateBoost(DeltaTime);
		}
		if (ClosestActor != nullptr)
		{
			FString ClosestActorMessage = FString::Printf(TEXT("Closest actor is %s"), *ClosestActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestActorMessage);
		}
		if (AffectedActor != nullptr)
		{
			FString ClosestAffectedActorMessage = FString::Printf(TEXT("Affection zone is %s"), *AffectedActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestAffectedActorMessage);
		}
		UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EFlightMode"), true);
		if (!EnumPtr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, TEXT("EFlightMode enum not found"));
		}
		else
		{
			FString EnumValueString = EnumPtr->GetNameByValue((int64)LastFlightMode).ToString();
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, EnumValueString);
		}

		ComputeProximity();

		// Если мы в режиме Interstellar, проверить ближайшую звезду
		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Interstellar)
		{
			//ComputeClosestStar(); // Обновить OffsetSystem

			double Distance = FVector::Dist(this->GetActorLocation(), OffsetSystem->GetActorLocation());
			double AffectionRadiusUnits = OffsetSystem->AffectionRadiusKM * 100000 / 1000000000.0; // Переводим км в юниты и учитываем масштабирование
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("Distance: %f, Affection Radius: %f"), Distance, AffectionRadiusUnits));

			// Если мы в пределах зоны влияния, переключиться на Stellar
			if (Distance < AffectionRadiusUnits && LastFlightMode != EFlightMode::Stellar)
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Stellar;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);

				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, TEXT("AffectionRadiusUnits!!!"));
			}
		}
		else 
		{
			OnboardComputer->ComputeFlightStatus(AffectedActor);
		}

		if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
		{
			switch (OnboardComputer->FlightSystem.CurrentFlightMode)
			{
			case EFlightMode::Interstellar:
				OnInterstellarMode.Broadcast();
				break;

			case EFlightMode::Stellar:
				OnStellarMode.Broadcast();
				break;

			case EFlightMode::Interplanetary:
				OnInterplanetaryMode.Broadcast();
				break;

			default:
				break;
			}

			UpdateNavigatableActors();
			LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
		}

		OnboardComputer->ComputeFlightParams();
	}
	if (Pilot)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Pilot Actor Name: %s"), *Pilot->GetName()));
	}
	uint64 EndCycles = FPlatformTime::Cycles();
	double ElapsedTime = FPlatformTime::ToSeconds(EndCycles - StartCycles);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Time elapsed: %f seconds"), ElapsedTime));
}

void ASpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ReleaseControl", IE_Pressed, this, &ASpaceship::ReleaseControl);
	PlayerInputComponent->BindAction("SwitchEngines", IE_Pressed, this, &ASpaceship::SwitchEngines);
	PlayerInputComponent->BindAxis("ThrustForward", this, &ASpaceship::ThrustForward);
	PlayerInputComponent->BindAxis("ThrustSide", this, &ASpaceship::ThrustSide);
	PlayerInputComponent->BindAxis("ThrustVertical", this, &ASpaceship::ThrustVertical);
	PlayerInputComponent->BindAxis("ThrustYaw", this, &ASpaceship::ThrustYaw);
	PlayerInputComponent->BindAxis("ThrustPitch", this, &ASpaceship::ThrustPitch);
	PlayerInputComponent->BindAxis("ThrustRoll", this, &ASpaceship::ThrustRoll);

	PlayerInputComponent->BindAction("ToggleScale", IE_Pressed, this, &ASpaceship::ToggleScale);

	PlayerInputComponent->BindAction("IncreaseFlightMode", IE_Pressed, this, &ASpaceship::IncreaseFlightMode);
	PlayerInputComponent->BindAction("DecreaseFlightMode", IE_Pressed, this, &ASpaceship::DecreaseFlightMode);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Pressed, this, &ASpaceship::StartAccelerationBoost);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Released, this, &ASpaceship::StopAccelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Pressed, this, &ASpaceship::StartDecelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Released, this, &ASpaceship::StopDecelerationBoost);
}

void ASpaceship::StartAccelerationBoost()
{
	bIsAccelerating = true;
	OnboardComputer->IsBoosting = true;
}

void ASpaceship::StopAccelerationBoost()
{
	bIsAccelerating = false;
	OnboardComputer->IsBoosting = false;
}

void ASpaceship::StartDecelerationBoost()
{
	bIsDecelerating = true;
	OnboardComputer->IsBoosting = true;
}

void ASpaceship::StopDecelerationBoost()
{
	bIsDecelerating = false;
	OnboardComputer->IsBoosting = false;
}

void ASpaceship::HandleAccelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->AccelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::HandleDecelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->DecelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::IncreaseFlightMode()
{
	if (OnboardComputer != nullptr)
	{
		OnboardComputer->IncreaseFlightMode();
	}
}

void ASpaceship::DecreaseFlightMode()
{
	if (OnboardComputer != nullptr)
	{
		OnboardComputer->DecreaseFlightMode();
	}
}


void ASpaceship::ToggleScale()
{
	if (GeneratedWorld)
	{
		if (bIsScaledUp)
		{
			GeneratedWorld->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() / 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() / 1000000000.0);
			this->SetActorScale3D(FVector(0.01, 0.01, 0.01));
			/// this->SetSmallHullMesh
			//this->SpaceshipHull = SmallScaleHullMesh;
			//this->SpaceshipHull->SetStaticMesh(SmallScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 35;
			//this->CameraComponent->FieldOfView = 45;
			//this->CameraComponent->FOV

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1")));
		}
		else
		{
			GeneratedWorld->SetActorScale3D(FVector(1000000000.0, 1000000000.0, 1000000000.0));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() * 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() * 1000000000.0);
			this->SetActorScale3D(FVector(1, 1, 1));
			/// this->SetBigHullMesh
			//this->SpaceshipHull->SetStaticMesh(LargeScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 1000;
			//this->CameraComponent->FieldOfView = 90;

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1000000000")));
		}

		// Переключаем состояние масштабирования
		bIsScaledUp = !bIsScaledUp;
	}
}


void ASpaceship::PrintOnboardComputerBasicIformation() 
{
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Safe Mode: %s"),
		*OnboardComputer->GetEnumValueAsString(TEXT("EFlightSafeMode"), (int32)OnboardComputer->FlightSystem.CurrentFlightSafeMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Range: %s"),
		*OnboardComputer->GetEnumValueAsString(TEXT("EFlightRange"), (int32)OnboardComputer->FlightSystem.CurrentFlightRange)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"),
		*OnboardComputer->GetEnumValueAsString(TEXT("EFlightStatus"), (int32)OnboardComputer->FlightSystem.CurrentFlightStatus)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State:	%s"),
		*OnboardComputer->GetEnumValueAsString(TEXT("EEngineState"), (int32)OnboardComputer->EngineSystem.CurrentEngineState)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type:	%s"), 
		*OnboardComputer->GetEnumValueAsString(TEXT("EEngineMode"), (int32)OnboardComputer->EngineSystem.CurrentEngineMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type:	%s"), 
		*OnboardComputer->GetEnumValueAsString(TEXT("EFlightType"), (int32)OnboardComputer->FlightSystem.CurrentFlightType)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode:	%s"), 
		*OnboardComputer->GetEnumValueAsString(TEXT("EFlightMode"), (int32)OnboardComputer->FlightSystem.CurrentFlightMode)));

	// Telemetry
	FVector ActorLocation = GetActorLocation();
	FVector ActorVelocity = GetVelocity();
	double ActorSpeed = ActorVelocity.Size();

	// Displaying them on screen
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Location: %s"), *ActorLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Velocity: %f"), ActorSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Thrust Force: %f"), OnboardComputer->GetEngineThrustForce()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Hull Angular Damping: %f"), SpaceshipHull->GetAngularDamping()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Hull Linear Damping: %f"), SpaceshipHull->GetLinearDamping()));

}

void ASpaceship::SwitchEngines()
{
	bEngineRunning = !bEngineRunning;
	//SpaceshipHull->SetSimulatePhysics(bEngineRunning);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Engine running: %s"), bEngineRunning ? TEXT("true") : TEXT("false")));
}

void ASpaceship::ThrustForward(float Value)
{

	double EngineThrustForce = OnboardComputer->GetEngineThrustForce();

	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	/*if (OnboardComputer->CurrentMovementStrategy)
	{
		OnboardComputer->CurrentMovementStrategy->ThrustForward(Value);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("NO CurrentMovementStrategy")));

	}*/
	const FVector Direction = ForwardVector->GetForwardVector();

	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		if (EngineThrustForce < 100)
		{

			OffsetSystem->AddActorLocalOffset(-Direction * Value * EngineThrustForce); /// CRASHED PIE!
		}
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		const FVector Impulse = Direction * Value * EngineThrustForce;
		SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * EngineThrustForce;
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
}

void ASpaceship::ThrustSide(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	/*if (OnboardComputer->CurrentMovementStrategy)
	{
		OnboardComputer->CurrentMovementStrategy->ThrustSide(Value);
	}*/
	const FVector Direction = ForwardVector->GetRightVector();

	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// Получаем вектор вперед корабля
		// Получаем вектор в сторону корабля
		//const FVector Direction = SpaceshipHull->GetRightVector();

		// Сдвигаем StarSystem
		OffsetSystem->AddActorLocalOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce());
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		// Получаем вектор вперед корабля.
		const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
}

void ASpaceship::ThrustVertical(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	/*if (OnboardComputer->CurrentMovementStrategy)
	{
		OnboardComputer->CurrentMovementStrategy->ThrustVertical(Value);
	}*/
	const FVector Direction = ForwardVector->GetUpVector();

	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// Получаем вектор вперед корабля
		// Получаем вектор в сторону корабля
		//const FVector Direction = SpaceshipHull->GetRightVector();

		// Сдвигаем StarSystem
		OffsetSystem->AddActorLocalOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce());
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		// Получаем вектор вперед корабля.
		const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
}

void ASpaceship::ThrustYaw(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float RotationAmount = Value * 0.5;  // Вы можете изменить эту переменную для управления скоростью вращения

	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, RotationAmount, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = GetActorUpVector() * RotationAmount;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
}

void ASpaceship::ThrustPitch(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float RotationAmount = Value * 0.5;

	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(RotationAmount, 0, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = GetActorRightVector() * RotationAmount;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
}

void ASpaceship::ThrustRoll(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float RotationAmount = Value * 0.5;

	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, 0, RotationAmount);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = GetActorForwardVector() * RotationAmount;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}

void ASpaceship::ComputeProximity()
{
	FVector ShipLocation = this->GetActorLocation();
	double ClosestDistance = DBL_MAX;
	double ClosestAffectionDistance = DBL_MAX;
	CurrentZonesInfluence.Empty();

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Navigatable proximity: %d"), WorldNavigatableActors.Num()));

	for (AWorldActor* Actor : WorldNavigatableActors)
	{
		FVector ActorLocation = Actor->GetActorLocation();
		double AffectionRadius = Actor->AffectionRadiusKM * 100000;
		double SurfaceRadius = Actor->RadiusKM * 100000;
		double DistanceToActor = (ActorLocation - ShipLocation).Size() - SurfaceRadius;
		double DistanceToAffectionZone = (ActorLocation - ShipLocation).Size();

		// Выводим расстояние в километрах
		//FString DistanceMessage = FString::Printf(TEXT("Distance to actor %s is %f km"), *Actor->GetName(), DistanceToActor / 100000);
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, DistanceMessage);

		// Если корабль находится в зоне действия актёра (т.е. расстояние меньше или равно нулю)
		if (DistanceToAffectionZone <= AffectionRadius)
		{
			CurrentZonesInfluence.Add(Actor);

			if (DistanceToAffectionZone < ClosestAffectionDistance)
			{
				ClosestAffectionDistance = DistanceToAffectionZone;
				AffectedActor = Actor;
			}
		}

		if (DistanceToActor < ClosestDistance)
		{
			ClosestDistance = DistanceToActor;
			ClosestActor = Actor;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("CurrentZonesInfluence: %d"), CurrentZonesInfluence.Num()));
	if (CurrentZonesInfluence.Num() > 0)
	{
		for (AWorldActor* Actor : CurrentZonesInfluence)
		{
			FString RadiusMessage = FString::Printf(TEXT("Ship is affected by %s"), *Actor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, RadiusMessage);
		}
		//OnboardComputer->ComputeFlightStatus(AffectedActor);
	}
	else
	{
		AffectedActor = nullptr;
	}
}

void ASpaceship::UpdateNavigatableActors()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("UpdateNavigatableActors!!!")));

}

void ASpaceship::CheckFlightModeChange()
{
	// Если режим полета изменился
	if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
	{
		// Обновляем список акторов
		UpdateNavigatableActors();

		// Обновляем LastFlightMode
		LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
	}
}

UStaticMeshComponent* ASpaceship::GetSpaceshipHull()
{
	return SpaceshipHull;
}

