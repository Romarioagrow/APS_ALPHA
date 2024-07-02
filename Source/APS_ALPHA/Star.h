// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Planet.h"
#include "StarType.h"
#include "StarSpectralClass.h"
#include "SpectralType.h"
#include "NavigatableBody.h"

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Star.generated.h"

class APlanetarySystem;

UCLASS()
class APS_ALPHA_API AStar : public ACelestialBody, public INavigatableBody
{
	GENERATED_BODY()

public:
	AStar();

protected:
	virtual void BeginPlay() override;

public:
	void SetStarProperties(FLinearColor Color, float Multiplier);

	UPROPERTY(VisibleAnywhere, Category = "Star")
		UStaticMeshComponent* StarMesh;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		UMaterialInstanceDynamic* StarDynamicMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		APlanetarySystem* PlanetarySystem;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		USphereComponent* PlanetarySystemZone;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		int StarRadiusKM;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
		double StarAffectionZoneRadius;
	double MinOrbit;
	double MaxOrbit;
private:
	UPROPERTY(VisibleAnywhere, Category = "Star")
		float Luminosity;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		int SurfaceTemperature;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
		EStellarType StellarClass;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
		ESpectralClass SpectralClass;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
		ESpectralType SpectralType;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
		FName FullSpectralClass;

	UPROPERTY(VisibleAnywhere, Category = "Star")
		FName FullSpectralName;
	
	UPROPERTY(VisibleAnywhere, Category = "Star")
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

	void SetStarType(EStellarType StarType);

	void SetStarSpectralClass(ESpectralClass StarSpectralClass);

	void SetStarSpectralType(ESpectralType StarSpectralType);

	void SetFullSpectralClass(FName FullSpectralClass);

	void SetFullSpectralName(FName FullSpectralName);

	void SetSpectralSubclass(int SpectralSubclass);
};
