// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AstonomicalSystem.h"
#include "CelestialBodyInterface.h"
#include "CoreMinimal.h"
#include "AstroActor.h"
#include "CelestialSystem.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ACelestialSystem : public AAstroActor, public IAstonomicalSystem
{
	GENERATED_BODY()
};
