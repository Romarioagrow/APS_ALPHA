// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AstroActor.h"
#include "PlanetOrbit.generated.h"

class APlanet;
/**
 * 
 */
UCLASS()
class APS_ALPHA_API APlanetOrbit : public AAstroActor
{
	GENERATED_BODY()

public:
	APlanet* Planet;
};
