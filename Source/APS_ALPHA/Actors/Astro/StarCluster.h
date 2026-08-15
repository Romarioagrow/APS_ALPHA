#pragma once

#include "AstroActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Star.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "StarCluster.generated.h"

class AStarSystem;
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

/**
 * Actor-free identity and generation data for one HISM star.
 * The full actor hierarchy is materialized only when gameplay actually needs it.
 */
USTRUCT(BlueprintType)
struct FClusterStarSystemRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	FGuid StableId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	int32 InstanceIndex{INDEX_NONE};

	/** Canonical cluster-catalog location. It is never overwritten with a render proxy coordinate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	FVector ClusterLocalLocation{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	FStarSystemModel SystemModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	FStarModel PrimaryStarModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	bool bMaterialized{false};

	TWeakObjectPtr<AStarSystem> MaterializedSystem;
};

UCLASS()
class APS_ALPHA_API AStarCluster : public AAstroActor /*, public INavigatableBody*/
{
	GENERATED_BODY()

public:
	AStarCluster();
	bool EnsureCanonicalStellarMaterial();

protected:
	virtual void PostInitializeComponents() override;

public:

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

	/** Seed for stable system identities. Zero means "choose once during generation". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster|Generation")
	int32 GenerationSeed{0};

	/** Ordered one-to-one with StarMeshInstances. No star/system actors are allocated here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster|Generation")
	TArray<FClusterStarSystemRecord> PotentialStarSystems;
	/** One immutable projected transform per canonical record, before view-only LOD changes. */
	TArray<FTransform> SystemProxyBaseTransforms;
	FAPSCanonicalStellarProjectionFrame CanonicalProjectionFrame;

	FGuid MakeStableSystemId(int32 InstanceIndex) const;
	/** Compatibility overload for isolated model tests and legacy callers. */
	void RegisterPotentialSystem(int32 InstanceIndex, const FVector& ClusterLocalLocation,
		const FStarModel& PrimaryStarModel, const FStarSystemModel& SystemModel);
	void RegisterPotentialSystem(int32 InstanceIndex, const FVector& ClusterLocalLocation,
		const FTransform& ProxyBaseTransform, const FStarModel& PrimaryStarModel,
		const FStarSystemModel& SystemModel);
	const FClusterStarSystemRecord* FindPotentialSystem(int32 InstanceIndex) const;
	FClusterStarSystemRecord* FindPotentialSystemMutable(int32 InstanceIndex);
	/** Immutable base-projection address for model/materialization consumers. */
	FVector GetPotentialSystemWorldLocation(const FClusterStarSystemRecord& Record) const;
	/** Current view-only HISM address for picking and presentation diagnostics. */
	FVector GetPotentialSystemPresentedWorldLocation(const FClusterStarSystemRecord& Record) const;
	/** Canonical root address; not a render-world coordinate. */
	bool GetPotentialSystemCanonicalRootLocationCm(
		const FClusterStarSystemRecord& Record, FVector& OutCanonicalRootCm) const;
	bool GetPotentialSystemBaseProxyTransform(int32 InstanceIndex, FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Star Cluster|Generation")
	bool GetPotentialSystemRecord(int32 InstanceIndex, FClusterStarSystemRecord& OutRecord) const;

	/** Performs the one render-state update/tree build after all stars and custom data are present. */
	void FinalizeGeneratedInstances();

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	EStarClusterType ClusterType;

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star Cluster")
	int StarAmount;

	/** Complete logical population before the preview/runtime HISM budget is applied. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster|Generation")
	int32 ModeledStarAmount{0};

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
