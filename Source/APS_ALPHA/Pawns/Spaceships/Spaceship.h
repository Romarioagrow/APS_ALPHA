// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
/*#include "GravitySource.h"
#include "GravityCharacterPawn.h"
#include "StarCluster.h"*/
#include "SpaceshipOnboardComputer.h"

#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "APS_ALPHA/Gameplay/Gravity/GravitySource.h"
#include "Spaceship.generated.h"

class APlanet;
class AStarSystem;
class AStarCluster;
class UCameraComponent;
class USpringArmComponent;
class AWorldActor;

struct FActorDistance
{
	AWorldActor* Actor;

	double Distance;

	FActorDistance(AWorldActor* InActor, double InDistance) : Actor(InActor), Distance(InDistance)
	{
	}
};

// TO COMP
struct ZoneData
{
	AWorldActor* Actor;

	double Distance;

	ZoneData(AWorldActor* InActor, double InDistance)
		: Actor(InActor)
		  , Distance(InDistance)
	{
	}
};


UCLASS()
class APS_ALPHA_API ASpaceship : public ASpacecraft, public IGravitySource
{
	GENERATED_BODY()

public:
	// Определение типа делегата
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFlightModeChangedDelegate);

	// Определение экземпляров делегата
	FOnFlightModeChangedDelegate OnInterstellarMode;
	FOnFlightModeChangedDelegate OnStellarMode;
	FOnFlightModeChangedDelegate OnInterplanetaryMode;

	// Объявление функций
	UFUNCTION()
	void UpdateNavigatableActorsForInterstellar();
	UFUNCTION()
	void UpdateNavigatableActorsForStellar();
	UFUNCTION()
	void UpdateNavigatableActorsForInterplanetary();

public:
	ASpaceship();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* SmallScaleHullMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* LargeScaleHullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flight Mode")
	EFlightMode LastFlightMode = EFlightMode::Basic;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USphereComponent* SphereCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* SpaceshipHull;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
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

	APlanet* AffectedPlanet{nullptr};

	AWorldActor* AffectedActor{nullptr};

	AWorldActor* ClosestActor{nullptr};

	UStaticMeshComponent* GetSpaceshipHull();

public:
	void ToggleScale();

	bool bIsScaledUp{true};

	bool bIsScaled{false};

	UPROPERTY(BlueprintReadWrite, Category = "Engine Settings")
	bool bEngineRunning{false};

	UFUNCTION(BlueprintPure, Category="Engine Settings")
	bool GetEngineRunning() const { return bEngineRunning; }

	bool bIsAccelerating{false};

	bool bIsDecelerating{false};

	void ComputeProximity();

	void UpdateNavigatableActors();

	void CheckFlightModeChange();

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
