// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
//#include "Planet.h"

#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Moon.generated.h"

// Forward declaration of APlanet
class APlanet;

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AMoon : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
		APlanet* ParentPlanet;

public:
	void SetParentPlanet(APlanet* Planet);
};
