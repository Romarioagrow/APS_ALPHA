// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "GravityCharacterPawn.h"
#include "SpaceshipOnboardComputer.h"
#include "StarCluster.h"

#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "Spaceship.generated.h"

struct FActorDistance
{
	AWorldActor* Actor;

	double Distance;

	FActorDistance(AWorldActor* InActor, double InDistance) : Actor(InActor), Distance(InDistance) {}
};

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



UCLASS()
class APS_ALPHA_API ASpaceship : public ASpacecraft, public IGravitySource
{
	GENERATED_BODY()

public:
	// ќпределение типа делегата
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlightModeChangedDelegate);

	// ќпределение экземпл€ров делегата
	FOnFlightModeChangedDelegate OnInterstellarMode;
	FOnFlightModeChangedDelegate OnStellarMode;
	FOnFlightModeChangedDelegate OnInterplanetaryMode;
	
	// ќбъ€вление функций
	void UpdateNavigatableActorsForInterstellar();
	void UpdateNavigatableActorsForStellar();
	void UpdateNavigatableActorsForInterplanetary();

public:
	ASpaceship();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight Mode")
		EFlightMode LastFlightMode = EFlightMode::Basic;  // или любое начальное значение


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* SpaceshipHull;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* PilotChair;

	UPROPERTY(VisibleAnywhere, Category = "Astro Actor")
		AStarCluster* GeneratedStarCluster;

	UPROPERTY(VisibleAnywhere, Category = "Astro Actor")
		AActor* GeneratedWorld;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
		USpaceshipOnboardComputer* OnboardComputer;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
		AStarSystem* OffsetSystem;

	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
		AAstroActor* OffsetGalaxy;

	TArray<AWorldActor*> WorldNavigatableActors{};

	TArray<AWorldActor*> CurrentZonesInfluence{};

	TArray<FActorDistance> Distances;

	TArray<FActorDistance> CurrentZones;

	AWorldActor* AffectedActor{ nullptr };

	AWorldActor* ClosestActor{ nullptr };


	void ToggleScale();

	bool bIsScaledUp{ true };

	bool bIsScaled{ false };

	bool bEngineRunning{ false };

	bool bIsAccelerating{ false };

	bool bIsDecelerating{ false };

public:	

	void ComputeProximity();

	void UpdateNavigatableActors();

	void CheckFlightModeChange();

	UStaticMeshComponent* GetSpaceshipHull();

	void PrintOnboardComputerBasicIformation();

	void SwitchCamera();

	void SwitchEngines();

	void ThrustForward(float Value);

	void ThrustSide(float Value);

	void ThrustVertical(float Value);

	void ThrustYaw(float Value);

	void ThrustPitch(float Value);

	void ThrustRoll(float Value);

	void SetPilot(AGravityCharacterPawn* NewPilot);

	void CalculateDistanceAndAddToZones(AWorldActor* WorldActor);

	void StartAccelerationBoost();

	void StopAccelerationBoost();

	void StartDecelerationBoost();

	void StopDecelerationBoost();

	void HandleAccelerationBoost(float Value);

	void HandleDecelerationBoost(float Value);

	void IncreaseFlightMode();

	void DecreaseFlightMode();
};


