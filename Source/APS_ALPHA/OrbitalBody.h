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
	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double OrbitDistance;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		float RotationSpeed;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		float OrbitalPeriod;
	
	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		float OrbitalSpeed;
	
	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		float AxialTilt;

public:
	void SetOrbitDistance(double OrbitDistance);

};
