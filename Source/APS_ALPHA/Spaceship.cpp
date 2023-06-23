// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "WorldActor.h"
#include "Kismet/GameplayStatics.h"


ASpaceship::ASpaceship()
{
	// Создание компонента SpaceshipHull
	SpaceshipHull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipHull"));
	RootComponent = SpaceshipHull; // делаем его корневым компонентом

	// Создание компонента SphereCollisionComponent и прикрепление его к SpaceshipHull
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	SphereCollisionComponent->SetupAttachment(SpaceshipHull);

	// Создание компонента PilotChair и прикрепление его к SpaceshipHull
	PilotChair = CreateDefaultSubobject<USceneComponent>(TEXT("PilotChair"));
	PilotChair->SetupAttachment(SpaceshipHull);

	ForwardVector = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardVector"));
	ForwardVector->SetupAttachment(SpaceshipHull);

	OnboardComputer = CreateDefaultSubobject<USpaceshipOnboardComputer>(TEXT("OnboardComputer"));
}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> TempActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldActor::StaticClass(), TempActors);
	// Приведем тип AActor к AWorldActor и добавим в массив
	for (AActor* Actor : TempActors)
	{
		AWorldActor* WorldActor = Cast<AWorldActor>(Actor);
		if (WorldActor)
		{
			WorldActors.Add(WorldActor);
		}
	}
}

void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Pilot)
	{
		// Telemetry
		FVector ActorLocation = GetActorLocation();
		FVector ActorVelocity = GetVelocity();
		double ActorSpeed = ActorVelocity.Size();

		PrintOnboardComputerBasicIformation();

		// Displaying them on screen
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorLocation: %s"), *ActorLocation.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorSpeed: %f"), ActorSpeed));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Pilot Actor Name: %s"), *Pilot->GetName()));

		if (bEngineRunning)
		{
			FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
			SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);

			CurrentZones.Empty();

			// Проверить расстояния до всех акторов AWorldActor
			for (AWorldActor* WorldActor : WorldActors)
			{
				if (WorldActor)
				{
					double DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), GetActorLocation());

					FVector Origin, BoxExtent;
					WorldActor->GetActorBounds(true, Origin, BoxExtent);
					double InfluenceRadius = BoxExtent.Size() / 2;

					if (DistanceSquared <= FMath::Square(InfluenceRadius))
					{
						CurrentZones.Add(WorldActor); // Добавить в список текущих зон

						// Вывести сообщение на экран
						// TO PROXIMITY SYSTEM
						//FString Message = FString::Printf(TEXT("Inside the zone of influence of actor %s at a distance of %f units."), *WorldActor->GetName(), FMath::Sqrt(DistanceSquared));
						double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
						FString Message = FString::Printf(TEXT("Inside the zone of influence of actor %s at a distance of %f kilometers."), *WorldActor->GetName(), DistanceInKm);

						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, Message);
					}
				}
			}

			// После первого цикла...
			for (AWorldActor* WorldActor : CurrentZones)
			{
				FString Message = FString::Printf(TEXT("Currently inside the zone of influence of actor %s."), *WorldActor->GetName());
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, Message);
			}
		}
	}
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
}


void ASpaceship::PrintOnboardComputerBasicIformation() 
{
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

	/*GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type: %s"), *OnboardComputer->GetFlightTypeAsString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode: %s"), *OnboardComputer->GetFlightModeAsString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"), *OnboardComputer->GetFlightStatusAsString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State: %s"), *OnboardComputer->GetEngineStateAsString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type: %s"), *OnboardComputer->GetEngineTypeAsString()));*/
}

void ASpaceship::SwitchEngines()
{
	bEngineRunning = !bEngineRunning;
	SpaceshipHull->SetSimulatePhysics(bEngineRunning);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Engine running: %s"), bEngineRunning ? TEXT("true") : TEXT("false")));
}

void ASpaceship::ThrustForward(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	const FVector Direction = ForwardVector->GetForwardVector();

	// Умножаем вектор вперед на значение оси и на силу двигателя.
	const FVector Impulse = Direction * Value * ThrustForce;
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustSide(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	//const FVector Direction = GetActorForwardVector();
	const FVector Direction = ForwardVector->GetRightVector();

	// Умножаем вектор вперед на значение оси и на силу двигателя.
	const FVector Impulse = Direction * Value * ThrustForce;
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustVertical(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	const FVector Direction = ForwardVector->GetUpVector();

	// Умножаем вектор вперед на значение оси и на силу двигателя.
	const FVector Impulse = Direction * Value * ThrustForce;
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustYaw(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;  // Вы можете настроить YawTorqueFactor для контроля скорости вращения
	FVector TorqueVector = GetActorUpVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustPitch(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorRightVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustRoll(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorForwardVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}



