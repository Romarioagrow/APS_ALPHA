// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Planet.h"
#include "StarType.h"
#include "StarSpectralClass.h"
#include "SpectralType.h"
//#include "PlanetarySystem.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Star.generated.h"

class APlanetarySystem;

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AStar : public ACelestialBody
{
	GENERATED_BODY()

public:
	AStar();

protected:
	virtual void BeginPlay() override;

public:
	void SetStarProperties(FLinearColor Color, float Multiplier);

	UPROPERTY(VisibleAnywhere, Category = "Star Visual")
		UStaticMeshComponent* StarMesh;

	UPROPERTY(VisibleAnywhere, Category = "Star Visual")
		UMaterialInstanceDynamic* StarDynamicMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		APlanetarySystem* PlanetarySystem;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		USphereComponent* PlanetarySystemZone;

	UPROPERTY(VisibleAnywhere)
		int StarRadiusKM;

private:
	UPROPERTY(VisibleAnywhere)
		float Luminosity;

	UPROPERTY(VisibleAnywhere)
		int SurfaceTemperature;
	
	UPROPERTY(VisibleAnywhere)
		EStellarClass StellarClass;
	
	UPROPERTY(VisibleAnywhere)
		ESpectralClass SpectralClass;
	
	UPROPERTY(VisibleAnywhere)
		ESpectralType SpectralType;
	
	UPROPERTY(VisibleAnywhere)
		FName FullSpectralClass;

	UPROPERTY(VisibleAnywhere)
		FName FullSpectralName;
	
	UPROPERTY(VisibleAnywhere)
		int SpectralSubclass;

private:
	UPROPERTY()
		TArray<APlanet*> Planets;

	public:
		void AddPlanet(APlanet* Planet);

		void SetPlanetarySystem(APlanetarySystem* PlanetarySystem);

public:
	void SetLuminosity(float Luminosity);

	void SetSurfaceTemperature(int SurfaceTemperature);

	void SetStarType(EStellarClass StarType);

	void SetStarSpectralClass(ESpectralClass StarSpectralClass);

	void SetStarSpectralType(ESpectralType StarSpectralType);

	void SetFullSpectralClass(FName FullSpectralClass);

	void SetFullSpectralName(FName FullSpectralName);

	void SetSpectralSubclass(int SpectralSubclass);
};
