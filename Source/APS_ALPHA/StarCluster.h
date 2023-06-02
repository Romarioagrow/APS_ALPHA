// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AstroActor.h"
#include "StarSpectralClass.h"
#include "StarClusterType.h"
#include "StarType.h"
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

    // ������ ������ ��� �������� ���������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UStaticMesh* StarModel;

    // ������� ��������� ���������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FVector ClusterBounds;

    // �������� �����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UMaterialInterface* StarMaterial;

    // ������� ��������� ���������
    UFUNCTION(BlueprintCallable, Category = "Star Cluster")
        void GenerateCluster();
	
};
