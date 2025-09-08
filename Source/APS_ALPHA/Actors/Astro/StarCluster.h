#pragma once

#include "AstroActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Star.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "StarCluster.generated.h"

struct FStarModel;
enum class EStellarType : uint8;
enum class ESpectralClass : uint8;
enum class EStarClusterSize : uint8;
enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;

// Структура для хранения диапазона значений
USTRUCT(BlueprintType)
struct FRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float LowerBound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float UpperBound;

	// Конструктор по умолчанию для инициализации всех свойств
	FRange()
		: LowerBound(0.0f)
		, UpperBound(0.0f)
	{
	}
};

//     
USTRUCT(BlueprintType)
struct FStarClusterModel
{
	GENERATED_BODY()

	// Конструктор по умолчанию для инициализации всех свойств
	FStarClusterModel()
		: StarClusterType(EStarClusterType::Unknown)
		, StarClusterPopulation(EStarClusterPopulation::Unknown)
		, StarClusterComposition(EStarClusterComposition::Unknown)
		, StarClusterSize(EStarClusterSize::Unknown)
	{
	}

	//  
	UPROPERTY(VisibleAnywhere, Category = "Star Cluster")
	FRange StarCountRange;

	//  
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

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	EStarClusterType ClusterType;

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	int StarAmount;

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	double StarDensity;

	//   
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	TMap<ESpectralClass, float> SpectralClassProbabilities;

	//   
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	TMap<EStellarType, float> StellarClassProbabilities;

	// HISM    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	UHierarchicalInstancedStaticMeshComponent* StarMeshInstances;

	//   
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	FVector ClusterBounds;

	//   
	UFUNCTION(BlueprintCallable, Category = "Star Cluster")
	void GenerateCluster();
};
