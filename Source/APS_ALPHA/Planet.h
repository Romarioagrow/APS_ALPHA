// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
//#include "Moon.h"
#include <Components/SphereComponent.h>
#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "PlanetType.h"
#include "PlanetaryZoneType.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Planet.generated.h"

// Forward declaration of AMoon
class AMoon;

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APlanet : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

public:
	APlanet();

private:
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetType PlanetType;

	// Число спутников
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		int32 AmountOfMoons { 0 };

	// Число спутников
	UPROPERTY(VisibleAnywhere,Category = "Planet")
		int32 Temperature { 0 };

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetaryZoneType PlanetZone;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetDensity{ 0 };

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetGravityStrength{ 0 };

private:
	TArray<AMoon*> Moons;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

public:
	void AddMoon(AMoon* Moon);

	void SetPlanetType(EPlanetType PlanetType);
	void SetPlanetZone(EPlanetaryZoneType PlanetZone);
	void SetPlanetDensity(double PlanetDensity);
	void SetPlanetGravityStrength(double PlanetGravityStrength);
	void SetTemperature(double Temperature);
	void SetAmountOfMoons(int AmountOfMoons);
};
