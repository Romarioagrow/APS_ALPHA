// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "WorldActor.h"
#include "Kismet/GameplayStatics.h"
#include "CelestialBody.h"
#include "Star.h"
#include "StarCluster.h"
#include "AstroGenerator.h"
#include "NavigatableBody.h"


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
}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();

	 // Находим AAstroGenerator (например, ищем первый найденный AAstroGenerator в мире)
	GeneratedWorld = Cast<AAstroGenerator>(UGameplayStatics::GetActorOfClass(GetWorld(), AAstroGenerator::StaticClass()));
	GeneratedStarCluster = Cast<AStarCluster>(UGameplayStatics::GetActorOfClass(GetWorld(), AStarCluster::StaticClass()));

	//FActorSpawnParameters SpawnParams;
	//SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//FVector SpawnLocation = PilotChair->GetComponentLocation(); // Место спавна на кресле пилота
	//FRotator SpawnRotation = PilotChair->GetComponentRotation(); // Ориентация спавна в соответствии с креслом пилота
	//Pilot = GetWorld()->SpawnActor<AGravityCharacterPawn>(AGravityCharacterPawn::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);

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



		{
			FVector ShipLocation = this->GetActorLocation();

			AWorldActor* ClosestActor = nullptr;
			double ClosestDistance = DBL_MAX;
			//TArray<AWorldActor*> CurrentZones;
			CurrentZones.Empty();
			for (AWorldActor* Actor : WorldNavigatableActors)
			{
				FVector ActorLocation = Actor->GetActorLocation();
				double AffectionRadius = Actor->AffectionRadiusKM * 100000; // переводим в сантиметры
				double SurfaceRadius = Actor->RadiusKM * 100000; // переводим в сантиметры
				// Вычисляем расстояние от корабля до границы радиуса воздействия актёра
				double DistanceToActor = (ActorLocation - ShipLocation).Size() - SurfaceRadius;
				double DistanceToAffectionZone = (ActorLocation - ShipLocation).Size();// -AffectionRadius;

				// Выводим расстояние в километрах
				FString DistanceMessage = FString::Printf(TEXT("Distance to actor %s is %f km"), *Actor->GetName(), DistanceToActor / 100000);
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, DistanceMessage);

				// Если корабль находится в зоне действия актёра (т.е. расстояние меньше или равно нулю)
				if (DistanceToAffectionZone <= AffectionRadius)
				{
					// Присваиваем AffactedActor ссылку на этого актёра
					AffectedActor = Actor;
					//UE_LOG(LogTemp, Warning, TEXT("Ship is within the action radius of %s"), *Actor->GetName());
					// Выводим сообщение на экран
					FString RadiusMessage = FString::Printf(TEXT("Ship is within the action radius of %s"), *Actor->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, RadiusMessage);

					// Добавляем актора в CurrentZones
					CurrentZonesInfluence.Add(Actor);
				}
				else
				{
					FString RadiusMessage = FString::Printf(TEXT("Dist for Affection zone of %s is %f km, rad: %f"), *Actor->GetName(), DistanceToAffectionZone, AffectionRadius);
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, RadiusMessage);

				}

				// Находим ближайшего актёра
				if (DistanceToActor < ClosestDistance)
				{
					ClosestDistance = DistanceToActor;
					ClosestActor = Actor;
				}
			}

			if (ClosestActor != nullptr)
			{
				FString ClosestActorMessage = FString::Printf(TEXT("Closest actor is %s"), *ClosestActor->GetName());
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, ClosestActorMessage);
				OnboardComputer->ComputeFlightStatus(ClosestActor);
			}

			// Выводим CurrentZones на экран
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("CurrentZonesInfluence: %f"), CurrentZonesInfluence.Num()));
			for (AWorldActor* Actor : CurrentZonesInfluence)
			{
				//FString ZoneMessage = FString::Printf(TEXT("Actor %s is in the current zone"), *Actor->GetName());
				//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, TEXT("Actor %s is in the current zone"), *Actor->GetName());
				//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor %s is in the current zone"), *Actor->GetName()));

			}

		}



		//CurrentZones.Empty();
		//for (AWorldActor* WorldActor : WorldNavigatableActors)
		//{
		//	if (WorldActor)
		//	{
		//		FVector ShipLocation = GetActorLocation();
		//		double InfluenceRadius = WorldActor->AffectionRadiusKM * 100000; // Конвертировать километры в юниты Unreal (1 unit = 1 cm)
		//		double DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), ShipLocation) - FMath::Square(InfluenceRadius);

		//		FVector Origin, BoxExtent;
		//		WorldActor->GetActorBounds(true, Origin, BoxExtent);
		//		InfluenceRadius = BoxExtent.Size() / 2;

		//		if (DistanceSquared <= FMath::Square(InfluenceRadius))
		//		{
		//			// Добавить в список текущих зон
		//			CurrentZones.Add(FActorDistance(WorldActor, DistanceSquared));

		//			// Вывести сообщение на экран
		//			double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
		//			FString Message = FString::Printf(TEXT("Inside the zone of influence of actor %s at a distance of %f kilometers."), *WorldActor->GetName(), DistanceInKm);
		//			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, Message);
		//		}
		//	}
		//}

		//// Сортировать массив по расстоянию до актора
		//CurrentZones.Sort([](const FActorDistance& A, const FActorDistance& B)
		//	{
		//		return A.Distance < B.Distance;
		//	});

		//if (CurrentZones.Num() > 0)
		//{
		//	const FActorDistance& AffectedActorDistance = CurrentZones[0];

		//	AWorldActor* AffectedActor = AffectedActorDistance.Actor;
		//	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Affect Object: ") + AffectedActor->GetName());

		//	//if (OnboardComputer->FlightSystem.CurrentFlightSafeMode != EFlightSafeMode::Unsafe)

		//	OnboardComputer->ComputeFlightStatus(AffectedActor);

		//	if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Interstellar && !bIsScaled)
		//	{
		//		//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::FTL;
		//		//SwitchEngineMode(EEngineMode::Offset);
		//		//Owner->
		//		//ToggleScale();
		//		bIsScaled = true;
		//	}
		//	else if (OnboardComputer->FlightSystem.CurrentFlightMode != EFlightMode::Interstellar && bIsScaled)
		//	{
		//		//Owner->
		//		//ToggleScale();
		//		bIsScaled = false;
		//	}

		//	ACelestialBody* CelestialBody = Cast<ACelestialBody>(AffectedActor);
		//	if (CelestialBody)
		//	{
		//		TArray<AActor*> ChildActors;
		//		GetAttachedActorsRecursively(CelestialBody, ChildActors);
		//		for (AActor* ChildActor : ChildActors)
		//		{
		//			AWorldActor* WorldActorChild = Cast<AWorldActor>(ChildActor);
		//			if (WorldActorChild && (WorldActorChild->IsA(AOrbitalBody::StaticClass()) || WorldActorChild->IsA(ATechActor::StaticClass())))
		//			{
		//				CalculateDistanceAndAddToZones(WorldActorChild);
		//			}
		//		}
		//		if (ChildActors.Num() > 0)
		//		{
		//			AActor* ClosestObject = ChildActors[0];
		//			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Closest Object: %s"), *ClosestObject->GetName()));
		//		}
		//	}
		//}

		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Interstellar)
		{
			if (GeneratedStarCluster)
			{
				TSharedPtr<FStarModel> NearestStar = FindNearestStar(GeneratedStarCluster->StarsModel, GetActorLocation());
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Nearest Star: %f"), NearestStar->Radius));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("NO GeneratedStarCluster")));
			}
		}
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
			this->SetActorLocation(this->GetActorLocation() / 1000000000.0);
		}
		else
		{
			GeneratedWorld->SetActorScale3D(FVector(1000000000.0, 1000000000.0, 1000000000.0));
			this->SetActorLocation(this->GetActorLocation() * 1000000000.0);
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
		// Получаем вектор вперед корабля
		//const FVector Direction = SpaceshipHull->GetForwardVector();

		// Сдвигаем StarSystem
		OffsetSystem->AddActorLocalOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce()); /// CRASHED PIE!
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddWorldOffset(Offset, true);
	}

	/*else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Impulse = Direction * Value * OnboardComputer->GetEngineThrustForce();
		SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
	}*/

	
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
//void ASpaceship::ThrustYaw(float Value)
//{
//	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;
//
//	float Torque = Value * 0.5;  
//	FVector TorqueVector = GetActorUpVector() * Torque;
//	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
//}
//
//void ASpaceship::ThrustPitch(float Value)
//{
//	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;
//
//	float Torque = Value * 0.5;
//	FVector TorqueVector = GetActorRightVector() * Torque;
//	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
//}
//
//void ASpaceship::ThrustRoll(float Value)
//{
//	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;
//
//	float Torque = Value * 0.5;
//	FVector TorqueVector = GetActorForwardVector() * Torque;
//
//	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
//}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}

UStaticMeshComponent* ASpaceship::GetSpaceshipHull()
{
	return SpaceshipHull;
}

