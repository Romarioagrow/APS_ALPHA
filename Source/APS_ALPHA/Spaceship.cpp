// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "WorldActor.h"
#include "Kismet/GameplayStatics.h"
#include "CelestialBody.h"
#include "Star.h"


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

// TO COMP
struct ZoneData
{
	AWorldActor* Actor;
	double Distance;

	ZoneData(AWorldActor* InActor, double InDistance)
		: Actor(InActor)
		, Distance(InDistance)
	{}
};

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

void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	uint64 StartCycles = FPlatformTime::Cycles();

	if (Pilot)
	{
		// Telemetry
		FVector ActorLocation = GetActorLocation();
		FVector ActorVelocity = GetVelocity();
		double ActorSpeed = ActorVelocity.Size();

		PrintOnboardComputerBasicIformation();

		// Displaying them on screen
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Location: %s"), *ActorLocation.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Velocity: %f"), ActorSpeed));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Thrust Force: %f"), OnboardComputer->GetEngineThrustForce()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Hull Angular Damping: %f"), SpaceshipHull->GetAngularDamping()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Hull Linear Damping: %f"), SpaceshipHull->GetLinearDamping()));

		if (bEngineRunning)
		{
			FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
			SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);

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

			CurrentZones.Empty();
			for (AWorldActor* WorldActor : WorldActors)
			{
				if (WorldActor)
				{
					FVector ShipLocation = GetActorLocation();
					double InfluenceRadius = WorldActor->AffectionRadiusKM * 100000; // Конвертировать километры в юниты Unreal (1 unit = 1 cm)
					double DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), ShipLocation) - FMath::Square(InfluenceRadius);

					FVector Origin, BoxExtent;
					WorldActor->GetActorBounds(true, Origin, BoxExtent);
					InfluenceRadius = BoxExtent.Size() / 2;

					if (DistanceSquared <= FMath::Square(InfluenceRadius))
					{
						// Добавить в список текущих зон
						CurrentZones.Add(FActorDistance(WorldActor, DistanceSquared));

						// Вывести сообщение на экран
						double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
						FString Message = FString::Printf(TEXT("Inside the zone of influence of actor %s at a distance of %f kilometers."), *WorldActor->GetName(), DistanceInKm);
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, Message);
					}
				}
			}

			// Сортировать массив по расстоянию до актора
			CurrentZones.Sort([](const FActorDistance& A, const FActorDistance& B)
				{
					return A.Distance < B.Distance;
				});

			if (CurrentZones.Num() > 0)
			{
				const FActorDistance& AffectedActorDistance = CurrentZones[0];

				AWorldActor* AffectedActor = AffectedActorDistance.Actor;
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Affect Object: ") + AffectedActor->GetName());

				if (OnboardComputer->FlightSystem.CurrentFlightSafeMode != EFlightSafeMode::Unsafe)
				{
					OnboardComputer->ComputeFlightStatus(AffectedActor);
				}

				ACelestialBody* CelestialBody = Cast<ACelestialBody>(AffectedActor);
				if (CelestialBody)
				{
					TArray<AActor*> ChildActors;
					GetAttachedActorsRecursively(CelestialBody, ChildActors);
					for (AActor* ChildActor : ChildActors)
					{
						AWorldActor* WorldActorChild = Cast<AWorldActor>(ChildActor);
						if (WorldActorChild && (WorldActorChild->IsA(AOrbitalBody::StaticClass()) || WorldActorChild->IsA(ATechActor::StaticClass())))
						{
							CalculateDistanceAndAddToZones(WorldActorChild);
						}
					}
					if (ChildActors.Num() > 0)
					{
						AActor* ClosestObject = ChildActors[0];
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Closest Object: %s"), *ClosestObject->GetName()));
					}
				}

			}
		}
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
}

void ASpaceship::SwitchEngines()
{
	bEngineRunning = !bEngineRunning;
	SpaceshipHull->SetSimulatePhysics(bEngineRunning);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Engine running: %s"), bEngineRunning ? TEXT("true") : TEXT("false")));
}

void ASpaceship::ThrustForward(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ForwardVector->GetForwardVector();
	const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustSide(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	const FVector Direction = ForwardVector->GetRightVector();
	const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustVertical(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	const FVector Direction = ForwardVector->GetUpVector();
	const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustYaw(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;  
	FVector TorqueVector = GetActorUpVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustPitch(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorRightVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustRoll(float Value)
{
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorForwardVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}



