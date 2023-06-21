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
