// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TechActor.h"
#include "Headquarters.generated.h"

class UCivilization;
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
