#pragma once

#include "StarSystemGenerator.h"
#include "StarClusterGenerator.h"
#include "GalaxyGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "GameFramework/Actor.h"
#include "AstroGenerator.generated.h"

class UGeneratedWorld;
class APlanet;
class ASpaceShipyard;
class ASpaceship;
class ASpaceStation;
class ASpaceHeadquarters;
enum class ECharSpawnPlace : uint8;
enum class EHomeSystemPosition : uint8;
enum class EOrbitHeight : uint8;


// TODO: FormStarSystemsInCluster();
UCLASS()
class APS_ALPHA_API AAstroGenerator : public AActor
{
	GENERATED_BODY()

public:
	AAstroGenerator();

	void SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bAutoGeneration{ false };

	UFUNCTION()
	void DisplayNewGeneratedWorld();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	UGeneratedWorld* NewGeneratedWorld;

	void GenerateHomeStarSystem();

	virtual void BeginPlay() override;

	void GenerateTEST_FULLSCALED();

	void InitGenerationLevel();

	void GenerateGalaxiesCluster();

	void GenerateGalaxy();

	void GenerateStarSystem();

	void GeneratePlanetSystem();

	void GenerateSinglePlanet();

	void GenerateRandomWorld();

	void InitAstroGenerators();

	void GenerateCustomHomeSystem();

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

public:

	TMap<int32, TSharedPtr<FStarModel>> StarIndexModelMap;

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateFullScaledWorld{ true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateHomeSystem{ false };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{ false };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{ EAstroGenerationLevel::StarCluster };

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	AWorldScapeRoot* StartHomePlanet;

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;

	UPROPERTY()
	APlanetarySystem* HomePlanetarySystem;

	// STAR SYSTEM GENERATOR
	UPROPERTY(VisibleAnywhere, Category = "Home System")
	double StarSystemDeadZone{ 1 };

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	bool bFullScaledHomeSystem{ false };

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	FVector HomeSystemRadius{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{ true };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar;

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bNeedOrbitRotation{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bOrbitRotationCheck{ false };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{ 1 };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{ true };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomStartPlanetNumber"))
	int StartPlanetNumber{ 1 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarType HomeSystemStarType{ EStarType::SingleStar };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType HomeStarStellarType{ EStellarType::MainSequence };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass HomeStarSpectralClass{ ESpectralClass::G };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType HomeSystemPlanetaryType{ EPlanetarySystemType::MultiPlanetSystem };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType HomeSystemOrbitDistributionType{ EOrbitDistributionType::Uniform };

	// STAR CLUSTER GENERATOR
	UPROPERTY(EditAnywhere, Category = "Galaxy Cluster")
	bool bGenerateRandomGalaxyCluster{ false };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	bool bGenerateRandomGalaxy{ false };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, Category = "Star Cluster")
	bool bGenerateRandomCluster{ false };

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{ 250 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{ 100000 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{ 10.0 };

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawn{ true };

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawnAtRandomPlanet{ true };

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	ECharSpawnPlace CharSpawnPlace;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EOrbitHeight HomeSpaceStationOrbitHeight;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<class AControlledPawn> BP_CharacterClass;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<class ASpaceStation> BP_HomeSpaceStation;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<class ASpaceship> BP_HomeSpaceship;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<class ASpaceShipyard> BP_HomeSpaceShipyard;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<class ASpaceHeadquarters> BP_HomeSpaceHeadquarters;

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
	TSubclassOf<class AGalaxy> BP_GalaxyClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class AStarCluster> BP_StarClusterClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class APlanetarySystem> BP_PlanetarySystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class AStarSystem> BP_StarSystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class AStar> BP_StarClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class APlanet> BP_PlanetClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class APlanetOrbit> BP_PlanetOrbit;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class AMoon> BP_MoonClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<class AAstroAnchor> BP_AstroAnchor;

	EStarClusterType GetRandomClusterType();

	int GetRandomValueFromStarAmountRange(EStarClusterType ClusterType);

	void GenerateStarCluster();
};
