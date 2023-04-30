// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Moon.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AMoon : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()
	
};
