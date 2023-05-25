// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GravitySource.h"
#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "OrbitalBody.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AOrbitalBody : public ACelestialBody, public IGravitySource
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
		float RotationSpeed;

	UPROPERTY(VisibleAnywhere)
		float OrbitalPeriod;
	
	UPROPERTY(VisibleAnywhere)
		float OrbitalSpeed;
	
	UPROPERTY(VisibleAnywhere)
		double OrbitDistance;
	
	UPROPERTY(VisibleAnywhere)
		float AxialTilt;

public:
	void SetOrbitDistance(double OrbitDistance);

};
