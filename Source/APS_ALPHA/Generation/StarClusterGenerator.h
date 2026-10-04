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

	/**
	 * Rio 03.10: formations of the cluster types appended on 03.10 (Young Association, Moving
	 * Group, Super Star Cluster, Embedded, Double Cluster). A pure function of (seed, index, mass):
	 * no global RNG and no dependence on the render prefix, so every LOD shows the whole shape.
	 * OutPosition is in cluster-bounds units (CalculateStarPosition applies its x100) and stays
	 * inside +-Bounds/2. Returns false for the older types, which keep their historic code.
	 */
	static bool SampleSeededFormation(EStarClusterType ClusterType, int32 GenerationSeed, int32 StarIndex,
		const FVector& ClusterBounds, double StarMass, FVector& OutPosition);

	/**
	 * Rio 03.10 ("SIZE must show"): linear extent of a cluster size relative to Giant (1.0), applied to the type's
	 * bounds of new datasets. Historic datasets keep their stored table bounds.
	 */
	static double GetSizeExtentFactor(EStarClusterSize StarClusterSize);
	/** Live preview systems for a size: small clusters show every system, big ones a bounded dense sample. */
	static int32 GetPreviewFormationBudget(EStarClusterSize StarClusterSize);
	/** The canonical half extent GenerateStarCluster derives from bounds (globulars add their star envelope). */
	static double GetLogicalHalfExtent(const FVector& ClusterBounds, EStarClusterType ClusterType);

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
