// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <Components/SphereComponent.h>
#include "OrbitalBody.h"
//#include "PlanetType.h"
#include "MoonGenerationModel.h"
#include "PlanetGenerationModel.h"
#include "PlanetaryZoneType.h"
#include "NavigatableBody.h"
#include "PlanetaryBody.h"
#include "PlanetOrbit.h" 

#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Planet.generated.h"

class AMoon;
class AStar;
//class APlanetEnvironmentGenerator;

UCLASS()
class APS_ALPHA_API APlanet : public APlanetaryBody, public INavigatableBody
{
	GENERATED_BODY()


protected:
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void HandleOnStellarMode();

	void CheckPlayerPawn();

public:
	APlanet();

	bool IsNotGasGiant();

	bool IsComputingWSCProximity{ true };

	FTimerHandle PlayerPawnTimerHandle;

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Environment")
		bool bGenerateByDefault{ false };
	*/
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Environment")
		double PlanetRadiusKM{ 0 };*/

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Environment")
		APlanetEnvironmentGenerator* PlanetEnvironmentGenerator;*/
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
		USphereComponent* GravityCollisionZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
		TArray<FOrbitInfo> Orbits;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		USphereComponent* PlanetaryZone;

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		int PlanetRadiusKM;
	
//private:
public:
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		AStar* ParnetStar;
	
	/*UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetType PlanetType;*/

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		EPlanetaryZoneType PlanetZone;

	// Число спутников
	UPROPERTY(VisibleAnywhere, Category = "Planet")
		int32 AmountOfMoons { 0 };

	// Число спутников
	UPROPERTY(VisibleAnywhere,Category = "Planet")
		int32 Temperature { 0 };


	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetDensity{ 0 };

	UPROPERTY(VisibleAnywhere, Category = "Planet")
		double PlanetGravityStrength{ 0 };

		TArray<TSharedPtr<FMoonData>> MoonsList;

//private:
public:
	UPROPERTY(VisibleAnywhere, Category = "Moons")
		TArray<AMoon*> Moons;

	UPROPERTY(VisibleAnywhere, Category = "Moons")
		TArray<APlanetOrbit*> MoonOrbitsList;

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
