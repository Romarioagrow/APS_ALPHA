// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StarSystemGenerator.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeneratedWorld.generated.h"

UCLASS()
class APS_ALPHA_API AGeneratedWorld : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGeneratedWorld();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY()
		UStarSystemGenerator* StarSystemGenerator;

	UPROPERTY()
		UStarGenerator* StarGenerator;

	UPROPERTY()
		UPlanetGenerator* PlanetGenerator;

	UPROPERTY()
		UMoonGenerator* MoonGenerator;

	// функция генерации стартовой системы
	//void GenerateHomeStarSystem(FStarSystemGenerationModel StarSystemModel, UWorld* World); //FStartSystemParams Params

	// функция генерации общей звездной системы
	void GenerateRandomStarSystem(); //FStartSystemParams Params // FStarSystemGenerationModel StarSystemModel, UWorld* World

	/*FStarSystemGenerationModel GenerateHomeStarSystemModel();

	FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModel();

	;*/
	FPlanetarySystemGenerationModel GenerateRandomPlanetraySystemModel();
//private:
	//void CreatePlanetarySystem(FPlanetarySystemGenerationModel PlanetarySystemModel); //FPlanetarySystemParams Params
};
