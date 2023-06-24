// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "GravityCharacterPawn.h"
#include "SpaceshipOnboardComputer.h"

#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "Spaceship.generated.h"

// Определите эту структуру где-то в начале файла
struct FActorDistance
{
	AWorldActor* Actor;
	double Distance;

	FActorDistance(AWorldActor* InActor, double InDistance) : Actor(InActor), Distance(InDistance) {}
};


/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceship : public ASpacecraft, public IGravitySource
{
	GENERATED_BODY()
	
public:
	ASpaceship();

	void PrintOnboardComputerBasicIformation();

private:
	UPROPERTY(VisibleAnywhere, Category = "Onboard Computer")
	USpaceshipOnboardComputer* OnboardComputer;

	TArray<AWorldActor*> WorldActors{};
	TArray<AWorldActor*> CurrentZonesInfluence{};
	TArray<FActorDistance> Distances;
	TArray<FActorDistance> CurrentZones;

	

public:

	void SwitchCamera();

	bool bEngineRunning {false };

	void SwitchEngines();

	void ThrustForward(float Value);

	void ThrustSide(float Value);

	void ThrustVertical(float Value);

	void ThrustYaw(float Value);

	void ThrustPitch(float Value);

	void ThrustRoll(float Value);

	void SetPilot(AGravityCharacterPawn* NewPilot);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* SpaceshipHull;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* PilotChair;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	void CalculateDistanceAndAddToZones(AWorldActor* WorldActor);
	virtual void Tick(float DeltaTime) override;


	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};


