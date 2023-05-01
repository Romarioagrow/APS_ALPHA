// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
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

public:
	APlanet();

	/*virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;*/

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

};
