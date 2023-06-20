// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MoonType.h"
#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Moon.generated.h"

// Forward declaration of APlanet
class APlanet;

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AMoon : public AOrbitalBody, public IPlanetaryEnvironment
{
	GENERATED_BODY()

public:
		// ��������� ����� ������
	/*UPROPERTY(VisibleAnywhere, Category = "Orbit")
		USceneComponent* OrbitAnchor;*/

	AMoon();

private:
	UPROPERTY(VisibleAnywhere, Category = "Moon Model")
		APlanet* ParentPlanet;

	UPROPERTY(VisibleAnywhere, Category = "Moon Model")
		EMoonType MoonType;

	UPROPERTY(VisibleAnywhere, Category = "Moon Model")
		double MoonDensity;

	UPROPERTY(VisibleAnywhere, Category = "Moon Model")
		double MoonGravity;

public:
	void SetParentPlanet(APlanet* Planet);

	void SetMoonType(EMoonType Type);

	void SetMoonDensity(double MoonDensity);

	void SetMoonGravity(double MoonGravity);

};
