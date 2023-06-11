// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StarSystemGenerator.h"
#include "StarClusterGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "AstroGenerationLevel.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstroGenerator.generated.h"

UCLASS()
class APS_ALPHA_API AAstroGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AAstroGenerator();

protected:
	virtual void BeginPlay() override;

public:
	
	/// BASE ASTRO GENERATOR
	UPROPERTY(EditAnywhere, Category = "Generation Params")
		bool bGenerateHomeSystem { false };
	
	UPROPERTY(EditAnywhere, Category = "Generation Params")
		bool bGenerateStarCluster { true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
		EAstroGenerationLevel AstroGenerationLevel { EAstroGenerationLevel::PlanetSystem };
	
	/// STAR SYSTEM GENERATOR
	UPROPERTY(EditAnywhere, Category = "Home System")
		bool bFullScaledHomeSystem{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
		bool bNeedOrbitRotation { false };

	UPROPERTY(EditAnywhere, Category = "Home System")
		FVector HomeSystemRadius { 0 };
	
	UPROPERTY(EditAnywhere, Category = "Home System")
		bool bRandomHomeSystem { false };

	UPROPERTY(EditAnywhere, Category = "Home System")
		EStarSystemType HomeSystemStarType { EStarSystemType::SingleStar };

	UPROPERTY(EditAnywhere, Category = "Home System")
		bool bRandomHomeStar;

	UPROPERTY(EditAnywhere, Category = "Home System")
		EStellarType HomeStarStellarType { EStellarType::MainSequence };

	UPROPERTY(EditAnywhere, Category = "Home System")
		ESpectralClass HomeStarSpectralClass { ESpectralClass::G };

	UPROPERTY(EditAnywhere, Category = "Home System")
		bool bRandomHomeSystemType;

	UPROPERTY(EditAnywhere, Category = "Home System")
		EPlanetarySystemType HomeSystemPlanetaryType { EPlanetarySystemType::MultiPlanetSystem };
	
	UPROPERTY(EditAnywhere, Category = "Home System")
		EOrbitDistributionType HomeSystemOrbitDistributionType { EOrbitDistributionType::Dense };

	/// STAR CLUSTER GENERATOR
	UPROPERTY(EditAnywhere, Category = "Star Cluster")
		bool bGenerateRandomCluster { true };

	UPROPERTY(EditAnywhere, Category = "Star Cluster")
		bool bGenerateFullScaledStarCluster { false };

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
		EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
		EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
		EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
		EStarClusterComposition StarClusterComposition;
	
	UPROPERTY(EditAnywhere, Category = "Galaxy")
		int GalaxySize{ 1 };

	UPROPERTY(EditAnywhere, Category = "Character Spawn")
		bool bCharacterSpawn{ true };

	UPROPERTY()
		TArray<AStarSystem*> GeneratedStarSystems;

	UPROPERTY()
		UStarClusterGenerator* StarClusterGenerator;

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
		TSubclassOf<class AStarCluster> BP_StarClusterClass;

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

	EStarClusterType GetRandomClusterType();

	int GetRandomValueFromStarAmountRange(EStarClusterType ClusterType);

	void GenerateStarCluster();

	
	

};
