#pragma once

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "APS_ALPHA/Gameplay/Gravity/GravitySource.h"
#include "OrbitalBody.generated.h"

UCLASS()
class APS_ALPHA_API AOrbitalBody : public ACelestialBody, public IGravitySource
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Planet")
	TArray<FVector> LagrangePoints;

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

public:
	void SetOrbitDistance(double OrbitDistance);
};
