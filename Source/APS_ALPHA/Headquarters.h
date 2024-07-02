// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Civilization.h"

#include "CoreMinimal.h"
#include "TechActor.h"
#include "Headquarters.generated.h"

/**
 * 
 */
UCLASS(ABSTRACT)
class APS_ALPHA_API AHeadquarters : public ATechActor
{
	GENERATED_BODY()

public:

	/*
	All Gameplay Logic 
	*/

	UPROPERTY()
		UCivilization* Civilization;

	

	
};
