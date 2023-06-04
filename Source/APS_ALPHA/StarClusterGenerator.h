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

	//FVector CalculateStarPosition(int StarIndex, const AStarCluster* StarCluster, const FStarModel& StarModel);
	FVector CalculateStarPosition(int StarIndex, const AStarCluster* StarCluster, const FStarModel& StarModel);

public:
	void GenerateRandomStarCluster(UWorld* World);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
		TSubclassOf<class AStarCluster> BP_StarClusterClass;
};
