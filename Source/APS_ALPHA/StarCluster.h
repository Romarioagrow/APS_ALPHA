// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
 
#include "AstroActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "StarGenerationModel.h"
#include "StarSpectralClass.h"
#include "StarClusterType.h"
#include "StarType.h"
#include "Star.h"
#include "StarClusterPopulation.h"
#include "StarClusterComposition.h"
#include "StarClusterSize.h"
#include "CoreMinimal.h"
#include "StarCluster.generated.h"


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
struct FClusterParameters
{
    GENERATED_BODY()

        // ���������� �����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FRange StarCountRange;

    // ��������� �����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FRange StarDensityRange;

    // ����������� ������������ �������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<ESpectralClass, FRange> SpectralClassProbabilitiesRange;

    // ����������� ���������� �������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<EStellarClass, FRange> StellarClassProbabilitiesRange;
};

/**
 * 
 */
UCLASS()
class APS_ALPHA_API AStarCluster : public AAstroActor
{
	GENERATED_BODY()



public:
    // �����������
    AStarCluster();
        
    UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        EStarClusterPopulation StarClusterPopulation;

    UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        EStarClusterComposition StarClusterComposition;
    
    UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        EStarClusterSize StarClusterSize;



    /*UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        EStarClusterPop



        UPROPERTY(VisibleAnywhere, Category = "Star Cluster")*/


    UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        TArray<AStar*> Stars;

    UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
        TMap<FVector, FStarModel> StarsModel;

    void AddStarToCluster(AStar* Star);

    void AddStarToClusterModel(FVector StarLocation, FStarModel StarModel);

    // ��� ���������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        EStarClusterType ClusterType;

    // ���������� �����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        int StarCount;

    // ��������� �����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        double StarDensity;

    // ����������� ������������ �������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<ESpectralClass, float> SpectralClassProbabilities;

    // ����������� ���������� �������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<EStellarClass, float> StellarClassProbabilities;

    //// ������ ������ ��� �������� ���������
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
    //    UStaticMesh* StarModel;
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
