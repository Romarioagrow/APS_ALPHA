// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/*#include "MoonType.h"
#include "PlanetaryEnvironment.h"
#include "OrbitalBody.h"
#include "NavigatableBody.h"*/
#include "PlanetaryBody.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "Moon.generated.h"

enum class EMoonType : uint8;
class APlanet;

UCLASS()
class APS_ALPHA_API AMoon : public APlanetaryBody, public INavigatableBody
{
	GENERATED_BODY()

public:
	// Добавляем якорь орбиты
	/*UPROPERTY(VisibleAnywhere, Category = "Orbit")
		USceneComponent* OrbitAnchor;*/

	AMoon();

	//private:
public:
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
	
	void DisableSphereMesh();
	
	void EnableSphereMesh();
};
