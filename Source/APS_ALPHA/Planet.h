// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
//#include "Moon.h"
#include <Components/SphereComponent.h>
#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"

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
	TArray<AMoon*> Moons;

public:
	void AddMoon(AMoon* Moon);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

};
