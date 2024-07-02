// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AstroActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
/*#include "StarGenerationModel.h"
#include "StarSpectralClass.h"
#include "StarClusterType.h"
#include "StarType.h"
#include "StarClusterPopulation.h"
#include "StarClusterComposition.h"
#include "StarClusterSize.h"
#include "NavigatableBody.h"*/
#include "Star.h"

#include "CoreMinimal.h"
#include "StarCluster.generated.h"

enum class EStellarType : uint8;
enum class ESpectralClass : uint8;
struct FStarModel;
enum class EStarClusterSize : uint8;
enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;

// ��������� ��� �������� ���������� ��������
USTRUCT(BlueprintType)
struct FRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float LowerBound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float UpperBound;
};

// ��������� ��� �������� ���������� ���������
USTRUCT(BlueprintType)
struct FStarClusterModel
{
	GENERATED_BODY()

	// ���������� �����
	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	FRange StarCountRange;

	// ��������� �����
	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	FRange StarDensityRange;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterType StarClusterType;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterSize StarClusterSize;
};

UCLASS()
class APS_ALPHA_API AStarCluster : public AAstroActor /*, public INavigatableBody*/
{
	GENERATED_BODY()

public:
	AStarCluster();

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	EStarClusterSize StarClusterSize;

	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	TArray<AStar*> Stars;

	TMap<FVector, TSharedPtr<FStarModel>> StarsModel;

	void AddStarToCluster(AStar* Star);

	void AddStarToClusterModel(FVector StarLocation, TSharedPtr<FStarModel> StarModel);

	// ��� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	EStarClusterType ClusterType;

	// ���������� �����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	int StarAmount;

	// ��������� �����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	double StarDensity;

	// ����������� ������������ �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	TMap<ESpectralClass, float> SpectralClassProbabilities;

	// ����������� ���������� �������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	TMap<EStellarType, float> StellarClassProbabilities;

	// HISM ������ ��� �������� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	UHierarchicalInstancedStaticMeshComponent* StarMeshInstances;

	// ������� ��������� ���������
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	FVector ClusterBounds;

	// �������� �����
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	    UMaterialInterface* StarMaterial;*/

	// ������� ��������� ���������
	UFUNCTION(BlueprintCallable, Category = "Star Cluster")
	void GenerateCluster();
};
