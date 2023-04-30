// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GravitySource.h"
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
	
};
