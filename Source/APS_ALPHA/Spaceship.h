// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "GravityCharacterPawn.h"
#include "SpaceshipOnboardComputer.h"

#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "Spaceship.generated.h"


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


public:
	void SwitchCamera();

	double ThrustForce { 100 };

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
	virtual void Tick(float DeltaTime) override;


	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};


