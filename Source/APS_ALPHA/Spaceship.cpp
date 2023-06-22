// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"

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

//FString GetEnumValueAsString(const TCHAR* Enum, int32 EnumValue)
//{
//	UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
//	return EnumPtr ? EnumPtr->GetNameByValue(EnumValue).ToString() : "";
//}

// и теперь вы можете просто вызывать её
//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Flight Type: %s"), *GetEnumValueAsString(TEXT("EFlightType"), (int32)OnboardComputer->FlightSystemInstance.CurrentFlightType)));
// Аналогично для остальных типов...


void ASpaceship::PrintOnboardComputerBasicIformation() 
{
	EFlightType FlightType = OnboardComputer->FlightSystem.CurrentFlightType;
	FString String = UEnum::GetValueAsString(FlightType);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("CurrentFlightType: %s"), *String));

	//FString CurrentFlightType = OnboardComputer->GetCurrentFlightType();
	//UE_LOG(LogTemp, Warning, TEXT("CurrentFlightType: %s"), CurrentFlightType);

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Current Flight Type: %s"), CurrentFlightType));


	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("CurrentFlightType: %s"), OnboardComputer->FlightSystemInstance.CurrentFlightType));

	/*FString GravityStateString = StaticEnum<USpaceshipOnboardComputer::EFlightType>()->GetNameStringByValue(static_cast<int32>(OnboardComputer->FlightSystemInstance.CurrentFlightType));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("GravityStateString: %s"), *GravityStateString));*/

	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type: %s"), *GetEnumValueAsString(TEXT("EFlightType"), (int32)OnboardComputer->FlightSystemInstance.CurrentFlightType)));
	////GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode: %s"), *GetEnumValueAsString(TEXT("EFlightMode"), (int32)OnboardComputer->FlightSystemInstance.CurrentFlightMode)));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"), *GetEnumValueAsString(TEXT("EFlightStatus"), (int32)OnboardComputer->FlightSystemInstance.CurrentFlightStatus)));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State: %s"), *GetEnumValueAsString(TEXT("EEngineState"), (int32)OnboardComputer->EngineSystemInstance.CurrentEngineState)));
	//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type: %s"), *GetEnumValueAsString(TEXT("EEngineType"), (int32)OnboardComputer->EngineSystemInstance.CurrentEngineType)));



	//UE_LOG(LogTemp, Warning, TEXT("Current Target: %s"), *OnboardComputer->TargetSystemInstance.CurrentTarget);
	//UE_LOG(LogTemp, Warning, TEXT("Current Astro Target: %s"), *OnboardComputer->AstroNavigationSystemInstance.CurrentAstroTarget);
	/*GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type: %d"), OnboardComputer->FlightSystemInstance.CurrentFlightType));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode: %d"), OnboardComputer->FlightSystemInstance.CurrentFlightMode));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %d"), OnboardComputer->FlightSystemInstance.CurrentFlightStatus));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State: %d"), OnboardComputer->EngineSystemInstance.CurrentEngineState));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type: %d"), OnboardComputer->EngineSystemInstance.CurrentEngineType));*/
	/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Flight Type: %s"), *GET_ENUM_NAME(EFlightType, OnboardComputer->FlightSystemInstance.CurrentFlightType)));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode: %s"), *GET_ENUM_NAME(EFlightMode, OnboardComputer->FlightSystemInstance.CurrentFlightMode)));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"), *GET_ENUM_NAME(EFlightStatus, OnboardComputer->FlightSystemInstance.CurrentFlightStatus)));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Engine State: %s"), *GET_ENUM_NAME(EEngineState, OnboardComputer->EngineSystemInstance.CurrentEngineState)));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Current Engine Type: %s"), *GET_ENUM_NAME(EEngineType, OnboardComputer->EngineSystemInstance.CurrentEngineType)));*/

	//UE_LOG(LogTemp, Warning, TEXT("Current Thrust Mode: %d"), OnboardComputer->EngineSystemInstance.CurrentThrustMode);
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
	//const FVector Direction = GetActorForwardVector();
	const FVector Direction = ForwardVector->GetForwardVector();// GetActorForwardVector();

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
	const FVector Direction = ForwardVector->GetRightVector();// GetActorForwardVector();

	// Умножаем вектор вперед на значение оси и на силу двигателя.
	const FVector Impulse = Direction * Value * ThrustForce;
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustVertical(float Value)
{
	// Если Value равно нулю, никакого ввода не было, поэтому ничего не делаем.
	if (FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	// Получаем вектор вперед корабля.
	//const FVector Direction = GetActorForwardVector();
	const FVector Direction = ForwardVector->GetUpVector();// GetActorForwardVector();

	// Умножаем вектор вперед на значение оси и на силу двигателя.
	const FVector Impulse = Direction * Value * ThrustForce;
	SpaceshipHull->AddImpulse(Impulse, NAME_None, true);
}

void ASpaceship::ThrustYaw(float Value)
{
	float Torque = Value * 0.5;  // Вы можете настроить YawTorqueFactor для контроля скорости вращения
	FVector TorqueVector = GetActorUpVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustPitch(float Value)
{
	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorRightVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::ThrustRoll(float Value)
{
	float Torque = Value * 0.5;
	FVector TorqueVector = GetActorForwardVector() * Torque;
	SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();

}

void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Assuming 'Pilot' is already assigned and valid
	if (Pilot)
	{
		// Telemetry
		// Get actor's world location
		FVector ActorLocation = GetActorLocation();
		// Get actor's forward, right and up vectors
		FVector ActorForward = GetActorForwardVector();
		FVector ActorRight = GetActorRightVector();
		FVector ActorUp = GetActorUpVector();
		// Get actor's linear speed
		FVector ActorVelocity = GetVelocity();
		double ActorSpeed = ActorVelocity.Size();

		// Displaying them on screen
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorLocation: %s"), *ActorLocation.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorForward: %s"), *ActorForward.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorRight: %s"), *ActorRight.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorUp: %s"), *ActorUp.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("ActorSpeed: %f"), ActorSpeed));


			PrintOnboardComputerBasicIformation();
		}

	
		FString PilotName = Pilot->GetName();

		// Display pilot's actor name on screen
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Pilot Actor Name: %s"), *PilotName));
		}

		FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
		SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);
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
