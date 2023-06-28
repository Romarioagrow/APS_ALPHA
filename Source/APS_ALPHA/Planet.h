// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <Components/SphereComponent.h>
#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "PlanetType.h"
#include "MoonGenerationModel.h"
#include "PlanetGenerationModel.h"
#include "PlanetaryZoneType.h"
#include "NavigatableBody.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Planet.generated.h"

class AMoon;
class AStar;

UCLASS()
class APS_ALPHA_API APlanet : public AOrbitalBody, public IPlanetaryEnvironment, public INavigatableBody
{
	GENERATED_BODY()

public:
	APlanet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
		USphereComponent* GravityCollisionZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		TArray<FOrbitInfo> Orbits;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		USphereComponent* PlanetaryZone;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		int PlanetRadiusKM;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		AStar* ParnetStar;
	
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetType PlanetType;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetaryZoneType PlanetZone;

	// ����� ���������
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		int32 AmountOfMoons { 0 };

	// ����� ���������
	UPROPERTY(VisibleAnywhere,Category = "Planet")
		int32 Temperature { 0 };


	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetDensity{ 0 };

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetGravityStrength{ 0 };

		TArray<TSharedPtr<FMoonData>> MoonsList;

private:
	TArray<AMoon*> Moons;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

public:
	void AddMoon(AMoon* Moon);

	void SetPlanetType(EPlanetType PlanetType);

	void SetPlanetZone(EPlanetaryZoneType PlanetZone);

	void SetPlanetDensity(double PlanetDensity);

	void SetPlanetGravityStrength(double PlanetGravityStrength);

	void SetTemperature(double Temperature);

	void SetAmountOfMoons(int AmountOfMoons);

	void SetMoonsList(TArray<TSharedPtr<FMoonData>> NewAmountOfMoons);

	void SetParentStar(AStar* Star);
};
