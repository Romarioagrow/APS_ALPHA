// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StarGenerationModel.h"
#include "StarCluster.h"

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarClusterGenerator.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UStarClusterGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarClusterGenerator();

	FVector CalculateStarPosition(int StarIndex, AStarCluster* StarCluster, const FStarModel& StarModel);

	int GetStarsAmountByRange(EStarClusterSize StarClusterSize);
	
	double GetStarClusterDensityByRange();
	
	FVector GetStarClusterBoundsByRange(EStarClusterType ClusterType);

public:
	void GenerateRandomStarCluster(UWorld* World);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
		TSubclassOf<class AStarCluster> BP_StarClusterClass;

	EStarClusterType GetRandomClusterType();

	FStarClusterModel GenerateRandomStarClusterModelByParams(FStarClusterModel& StarClusterModel);

	FStarClusterModel GetRandomStarClusterModel();
	
};
