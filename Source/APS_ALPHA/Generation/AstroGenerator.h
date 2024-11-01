#pragma once

#include "StarSystemGenerator.h"
#include "StarClusterGenerator.h"
#include "GalaxyGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/BaseActor.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "GameFramework/Actor.h"
#include "AstroGenerator.generated.h"

class USpawnParameters;
class AControlledPawn;
class AAstroAnchor;
class AMoon;
class APlanetOrbit;
class UGeneratedWorld;
class APlanet;
class ASpaceShipyard;
class ASpaceship;
class ASpaceStation;
class ASpaceHeadquarters;
enum class ECharSpawnPlace : uint8;
enum class EHomeSystemPosition : uint8;
enum class EOrbitHeight : uint8;
struct FPlanetModel;
struct FPlanetData;

UCLASS()
class APS_ALPHA_API AAstroGenerator : public ABaseActor
{
	GENERATED_BODY()

public:
	void SpawnStartInteractiveActors(TSharedPtr<FPlanetModel> StartPlanetModel);

	void ComputeStarAmount(TSharedPtr<FStarSystemModel>& StarSystemModel, int& AmountOfStars);

	TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel;

	void SpawnPlanetMoons(const TSharedPtr<FPlanetModel>& PlanetModel);

	void ResolveSpawnLocation(const ASpaceship* NewHomeSpaceship, FVector& CharSpawnLocation);

	void SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bAutoGeneration{false};

	UFUNCTION()
	void DisplayNewGeneratedWorld();

	UFUNCTION()
	void ApplyWorldModel();

	UFUNCTION()
	void GenerateWorldByModel();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	UGeneratedWorld* GeneratedWorldModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	USpawnParameters* SpawnParameters;

	void SetAutoStarSystemModel();

	void GenerateRandomStarSystemModel();

	void GenerateHomeStarSystem();

	void SetMoonRotation(APlanetOrbit* NewMoonOrbit);

	virtual void BeginPlay() override;

	void Test_GenerateFullscaled();

	void InitGenerationLevel();

	void GenerateGalaxiesCluster();

	void GenerateGalaxy();

	void GeneratePlanetSystem();

	void GenerateSinglePlanet();

	void GenerateRandomWorld();

	void InitAstroGenerators();
	
	void ApplySpawnParameters();

	bool CheckGeneratorsFails();

	void GenerateStarSystemByModel();

	void ShowPlanetsList(TArray<TSharedPtr<FPlanetData>> PlanetDataMap);

	void SpawnMoons(UWorld* World, APlanet* Planet, int32 NumberOfMoons);

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AGalaxy* GeneratedGalaxy;

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AStarCluster* GeneratedStarCluster;

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AStarSystem* GeneratedHomeStarSystem;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceHeadquarters* HomeSpaceHeadquarters;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceStation* HomeSpaceStation;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceship* HomeSpaceship;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceShipyard* HomeSpaceShipyard;

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AActor* GeneratedWorld;

	double FullScaleValue{1000000000.0};

public:
	TMap<int32, TSharedPtr<FStarModel>> StarIndexModelMap;

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateFullScaledWorld{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateHomeSystem{false};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{false};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bSpawnStarterLocation{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bSpawnStarterPlanet{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{EAstroGenerationLevel::StarCluster};

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	AWorldScapeRoot* StartHomePlanet;

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APlanetarySystem* HomePlanetarySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AStar* HomeStar;

	// STAR SYSTEM GENERATOR
	UPROPERTY(VisibleAnywhere, Category = "Home System")
	double StarSystemDeadZone{1};

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	bool bFullScaledHomeSystem{false};

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	FVector HomeSystemRadius{0};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{false};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar;

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bNeedOrbitRotation{true};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bOrbitRotationCheck{true};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{1};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{true};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomStartPlanetNumber"))
	int StartPlanetNumber{1};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarType HomeSystemStarType{EStarType::SingleStar};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType HomeStarStellarType{EStellarType::MainSequence};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass HomeStarSpectralClass{ESpectralClass::G};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType HomeSystemPlanetaryType{EPlanetarySystemType::MultiPlanetSystem};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType HomeSystemOrbitDistributionType{EOrbitDistributionType::Uniform};

	// STAR CLUSTER GENERATOR
	UPROPERTY(EditAnywhere, Category = "Galaxy Cluster")
	bool bGenerateRandomGalaxyCluster{false};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	bool bGenerateRandomGalaxy{false};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, Category = "Star Cluster")
	bool bGenerateRandomCluster{false};

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{250};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{100000};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{10.0};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawn{true};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawnAtRandomPlanet{true};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	ECharSpawnPlace CharSpawnPlace;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EOrbitHeight HomeSpaceStationOrbitHeight;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<AControlledPawn> BP_CharacterClass;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceStation> BP_HomeSpaceStation;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceship> BP_HomeSpaceship;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceShipyard> BP_HomeSpaceShipyard;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceHeadquarters> BP_HomeSpaceHeadquarters;

	UPROPERTY()
	TArray<AStarSystem*> GeneratedStarSystems;

	UPROPERTY()
	UGalaxyGenerator* GalaxyGenerator;

	UPROPERTY()
	UStarClusterGenerator* StarClusterGenerator;

	UPROPERTY()
	UStarSystemGenerator* StarSystemGenerator;

	UPROPERTY()
	UPlanetarySystemGenerator* PlanetarySystemGenerator;

	UPROPERTY()
	UStarGenerator* StarGenerator;

	UPROPERTY()
	UPlanetGenerator* PlanetGenerator;

	UPROPERTY()
	UMoonGenerator* MoonGenerator;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AGalaxy> BP_GalaxyClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStarCluster> BP_StarClusterClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanetarySystem> BP_PlanetarySystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStarSystem> BP_StarSystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStar> BP_StarClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanet> BP_PlanetClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanetOrbit> BP_PlanetOrbitClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AMoon> BP_MoonClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AAstroAnchor> BP_AstroAnchorClass;

	int GetRandomValueFromStarAmountRange(EStarClusterType ClusterType);

	void GenerateStarCluster();
	
	void AddGeneratedWorldModelData();

	UFUNCTION()
	FVector DetermineHomeSystemSpawnLocation();

	UFUNCTION()
	FVector GetHomeSystemSpawnLocationForGalaxy(TArray<AActor*> AttachedActors, int32 RandomIndex);

	UFUNCTION()
	FVector GetHomeSystemSpawnLocationForStarCluster(TArray<AActor*> AttachedActors, int32 RandomIndex);

	void GenerateStarSystem(AStarSystem* NewStarSystem, TSharedPtr<FStarSystemModel> StarSystemModel);
	
	void RotatePlanetOrbits(APlanetarySystem* NewPlanetarySystem);
	
	void ComputeHomeSystemPosition(FTransform& HomeSystemTransform, FVector& HomeSystemSpawnLocation);
};
