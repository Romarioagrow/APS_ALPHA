// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StarSystemGenerator.h"
#include "PlanetaryProceduralGenerator.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeneratedWorld.generated.h"

UCLASS()
class APS_ALPHA_API AAstroGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AAstroGenerator();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, Category = "Generation Params")
		bool bNeedOrbitRotation { false };

	UPROPERTY()
		TArray<AStarSystem*> GeneratedStarSystems;

	UPROPERTY()
		UStarSystemGenerator* StarSystemGenerator;

	UPROPERTY()
		UPlanetarySystemGenerator* PlanetarySystemGenerator;

	UPROPERTY()
		UStarGenerator* StarGenerator;

	UPROPERTY()
		UPlanetGenerator* PlanetGenerator;

	UPROPERTY()
		UMoonGenerator* MoonGenerator;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class APlanetarySystem> BP_PlanetarySystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class AStarSystem> BP_StarSystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class AStar> BP_StarClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class APlanet> BP_PlanetClass;
	
	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class APlanetOrbit> BP_PlanetOrbit;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
		TSubclassOf<class AMoon> BP_MoonClass;
	
	// ������� ��������� ����� �������� �������
	void GenerateRandomStarSystem();

	void GenerateStarCluster();

};
