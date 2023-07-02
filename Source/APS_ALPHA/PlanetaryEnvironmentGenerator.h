// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "WorldScapeCore/Public/WorldScapeRoot.h"
//#include "Planet.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetaryEnvironmentGenerator.generated.h"

//class APlanet;
class APlanet;  // Forward declaration
class AMoon;  // Forward declaration

UCLASS()
class APS_ALPHA_API APlanetaryEnvironmentGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlanetaryEnvironmentGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, Category = "World Scape")
		AWorldScapeRoot* WorldScapeRootInstance;
	
	UPROPERTY(VisibleAnywhere, Category = "World Scape")
		double RadiusKM;



	void InitWorldScape(UWorld* World);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon);

	//void SetupDefaultGenerator();

};
