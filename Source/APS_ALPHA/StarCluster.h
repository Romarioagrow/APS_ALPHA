// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AstroActor.h"
#include "StarSpectralClass.h"
#include "StarClusterType.h"
#include "StarType.h"
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

    // Модель звезды для создания инстансов
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UStaticMesh* StarModel;

    // Границы звездного скопления
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        FVector ClusterBounds;

    // Материал звезд
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
        UMaterialInterface* StarMaterial;

    // Функция генерации скопления
    UFUNCTION(BlueprintCallable, Category = "Star Cluster")
        void GenerateCluster();
	
};
