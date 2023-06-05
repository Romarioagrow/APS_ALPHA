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


// Структура для хранения диапазонов значений
USTRUCT(BlueprintType)
struct FRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
        float LowerBound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
        float UpperBound;
};

// Структура для хранения параметров скопления
USTRUCT(BlueprintType)
struct FClusterParameters
{
    GENERATED_BODY()

        // Количество звезд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FRange StarCountRange;

    // Плотность звезд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FRange StarDensityRange;

    // Вероятности спектральных классов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<ESpectralClass, FRange> SpectralClassProbabilitiesRange;

    // Вероятности стелларных классов
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
    // Конструктор
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

    // Тип скопления
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        EStarClusterType ClusterType;

    // Количество звезд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        int StarCount;

    // Плотность звезд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        double StarDensity;

    // Вероятности спектральных классов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<ESpectralClass, float> SpectralClassProbabilities;

    // Вероятности стелларных классов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        TMap<EStellarClass, float> StellarClassProbabilities;

    //// Модель звезды для создания инстансов
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
    //    UStaticMesh* StarModel;
    // HISM звезды для создания инстансов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UHierarchicalInstancedStaticMeshComponent* StarMeshInstances;

    // Границы звездного скопления
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FVector ClusterBounds;

    // Материал звезд
    /*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UMaterialInterface* StarMaterial;*/

    // Функция генерации скопления
    UFUNCTION(BlueprintCallable, Category = "Star Cluster")
        void GenerateCluster();
	
};
