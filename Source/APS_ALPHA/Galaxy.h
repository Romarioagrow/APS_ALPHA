// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GalaxyType.h"
#include "GalaxyClass.h"

#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "Galaxy.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AGalaxy : public ACelestialSystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Home System")
		EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Home System")
		EGalaxyClass GalaxyGlass;
	
};
