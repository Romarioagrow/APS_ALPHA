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
		double RotationSpeed;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double OrbitalPeriod;
	
	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double OrbitalSpeed;
	
	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double AxialTilt;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double SemiMajorAxis;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double TrueAnomaly;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double LongitudeOfAscendingNode;

	UPROPERTY(VisibleAnywhere, Category = "Orbital Body")
		double Inclination;


	/*SemiMajorAxis
		TrueAnomaly
		LongitudeOfAscendingNode
		Inclination*/



public:
	void SetOrbitDistance(double OrbitDistance);

};
