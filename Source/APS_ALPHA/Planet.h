// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Planet.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API APlanet : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()
	
};
