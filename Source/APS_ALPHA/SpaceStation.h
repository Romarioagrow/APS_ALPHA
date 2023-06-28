// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "NavigatableBody.h"

#include "CoreMinimal.h"
#include "TechActor.h"
#include "SpaceStation.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceStation : /*public AGravityStation*/ public ATechActor, public IGravitySource, public INavigatableBody
{
	GENERATED_BODY()
	
public:
	ASpaceStation();

protected:
	

public:
	/// TO parent comp
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* SpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
		USphereComponent* GravityCollisionZone;
};
