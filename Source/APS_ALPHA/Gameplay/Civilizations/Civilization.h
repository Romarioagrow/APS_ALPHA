// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "UObject/NoExportTypes.h"
#include "Civilization.generated.h"

class USpawnParameters;

enum CivilizationType {
    Type1,
    Type2,
    //...
};

enum GovernmentType {
    Democracy,
    Monarchy,
    //...
};

enum EconomicSystem {
    Capitalism,
    Socialism,
    //...
};

enum SocietyType {
    TypeA,
    TypeB,
    //...
};

struct Infrastructure {
    int starOutposts;
    int planetOutposts;
    int orbitalOutposts;
    int groundOutposts;
    //...
};

// Базовый класс подразделения
class Division {
public:
	FName name;
	int level;
	//virtual void Update() = 0; // Обновить состояние подразделения. Это абстрактный метод, который будет реализован в каждом конкретном подразделении
};

// Подразделение планетарного обследования
class PlanetarySurveyDivision : public Division {
public:
	// Planetary Survey Division
	void	PerfomPlanetSurfaceSurvey();
	void	PerformGeologicalSurvey();
	void	PerformAtmosphericAnalysis();
	void	PerformBiologicalScan();
	void	EvaluateResourceDeposits();
	void	MapPlanetarySurface();
};

class AstronomicalExplorationDivision : public Division {
public:
	// Astronomical Exploration Division
	void	PerfomPlanetBodySurvey();
	void	PerfomStarSystemSurvey();
	void	PerfomStarClusterSurvey();
	void	PerfomGalaxySurvey();
	void	ScanStarSystem();
	void	SurveyPlanetaryBodies();
	void	DetectAnomalies();
	void	ChartHyperspaceRoutes();
	void	CatalogStellarObjects();
};

class IndustryProductionDivision : public Division {
public:
	// Industry Production Division
	void	ManufactureGoods();
	void	RefineRawMaterials();
	void	ImplementProductionLine();
	void	IncreaseOutputEfficiency();
	void	IntroduceNewProducts();
};

class IndustryMiningDivision : public Division {
public:
	// Industry Mining Division
	void	EstablishExtractionSite();
	void	ExcavateMinerals();
	void	ExpandMiningArea();
	void	ExtractRareElements();
	void	OptimizeResourceGathering();
};

class IndustryConstructionDivision : public Division {
public:
	// Industry Construction Division
	void	BuildIndustrialFacilities();
	void	ExpandInfrastructureNetwork();
	void	ErectResidentialBlocks();
	void	RenovateAgingStructures();
	void	ConstructInfrastructureInstallations();
};

class ShipyardDivision : public Division {
public:
	// Shipyard Division
	void	AssembleStarship();
	void	UpgradeVesselSystems();
	void	LaunchSpacecraft();
	void	ImplementShipDesigns();
	void	IncreaseShipProductionRate();
};

class ColonizationDivision : public Division {
public:

	// Colonization Division
	void	EstablishColony();
	void	EstablishOutpost();
	void	EstablishSettlement();
	void	EstablishFrontierColony();
	void	BuildSettlementInfrastructure();
	void	ExpandColonyBorders();
	void	SettleNewInhabitants();
	void	StrengthenColonyDefenses();
};

class ScienceDivision : public Division {
public:
	// Science& Exploring Division
	void	ResearchNewTechnologies();
	void	ExpediteScientificDiscoveries();
	void	InvestigateAnomalies();
	void	ConductScientificResearch();
	void	DevelopNewTechnologies();
	void	PublishResearchFindings();
	void	CollaborateWithOtherDivisions();
	void	AdvanceScientificKnowledge();
};

class TerraformingDivision : public Division {
public:
	// Terraforming Division
	void	InitiateTerraformingProtocol();
	void	AlterPlanetaryAtmosphere();
	void	EngineerBiosphere();
	void	MonitorClimateChanges();
	void	StabilizePlanetaryConditions();
};

class  CivilAffairsDivision : public Division {
public:
	// Civil Affairs Division
	void	ClaimStarSystem();
	void	ClaimStarCluster();
	void	ClaimGalaxy();
	void	RegulatePublicServices();
	void	EnforceLawAndOrder();
	void	ManagePopulationGrowth();
	void	ConductPublicElections();
	void	ResolveCivilDisputes();
};

class  ForeignAffairsDivision : public Division {
public:
	ForeignAffairsDivision();

	// Foreign Affairs Division
	void	EstablishCivilization();
	void	EstablishState();
	void	SignInterstellarTreaties();
	void	EstablishDiplomaticMissions();
	void	CoordinateInterstateRelations();
	void	ResolveIntergalacticConflicts();
	void	ExchangeCulturalIdeas();
};

// Подразделение военных операций
class MilitaryDivision : public Division {
public:
	MilitaryDivision();

	void MobilizeForces();
	void ExecuteDefenseProtocol();
	void ExecuteOffensiveStrategy();
	void TrainSoldiers();
	void DevelopMilitaryTechnology();
};

// Флотское подразделение
class FleetDivision : public Division {
public:
	FleetDivision();

	void DeployFleet();
	void ExecuteNavalStrategy();
	void RepairShips();
	void ConductNavalExercises();
	void UpgradeFleet();
};

USTRUCT(BlueprintType)
struct FAPSCivilizationInfrastructure
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StarOutposts{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlanetOutposts{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 OrbitalStations{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 GroundSettlements{0};
};

USTRUCT(BlueprintType)
struct FAPSCivilizationDivisions
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Exploration{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Industry{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Science{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CivilAffairs{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Military{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 FleetCommand{0};
};

/** Runtime civilization model created from the generation menu and kept by the game-instance subsystem. */
UCLASS(BlueprintType)
class APS_ALPHA_API UCivilization : public UObject
{
	GENERATED_BODY()

public:
	UCivilization();
	void InitializeFromSpawnParameters(const USpawnParameters* Parameters);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	FString Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	EAPSCivilizationArchetype Archetype{EAPSCivilizationArchetype::Balanced};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	EAPSGovernmentType Government{EAPSGovernmentType::Democracy};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	EAPSEconomicSystem Economy{EAPSEconomicSystem::Mixed};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	EAPSSocietyType Society{EAPSSocietyType::Cooperative};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	int32 Population{1};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	int64 Credits{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	int32 TechnologyLevel{1};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	int32 FleetSize{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	FAPSCivilizationInfrastructure Infrastructure;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	FAPSCivilizationDivisions Divisions;
};
