// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseProceduralGenerator.h"
#include "StarClusterGenerator.generated.h"

class AStarCluster;
struct FStarModel;
struct FStarClusterModel;
enum class EStarClusterType : uint8;
enum class EStarClusterSize : uint8;

UCLASS()
class APS_ALPHA_API UStarClusterGenerator : public UBaseProceduralGenerator
{
	GENERATED_BODY()

public:
	UStarClusterGenerator();

	FVector CalculateStarPosition(int StarIndex, AStarCluster* StarCluster, const TSharedPtr<FStarModel> StarModel);

	int GetStarsAmountByRange(EStarClusterSize StarClusterSize);

	double GetStarClusterDensityByRange();

	FVector GetStarClusterBoundsByRange(EStarClusterType ClusterType);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	TSubclassOf<class AStarCluster> BP_StarClusterClass;

	EStarClusterType GetRandomClusterType();

	TUniquePtr<FStarClusterModel>
	GenerateRandomStarClusterModelByParams(TUniquePtr<FStarClusterModel> StarClusterModel);

	void GetRandomStarClusterModel(TSharedPtr<FStarClusterModel> StarClusterModel);
};
