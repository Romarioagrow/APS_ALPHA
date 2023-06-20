// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Civilization.generated.h"

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

// Ѕазовый класс подразделени€
class Division {
public:
	FName name;
	int level;
	//virtual void Update() = 0; // ќбновить состо€ние подразделени€. Ёто абстрактный метод, который будет реализован в каждом конкретном подразделении
};

// ѕодразделение планетарного обследовани€
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

// ѕодразделение военных операций
class MilitaryDivision : public Division {
public:
	MilitaryDivision();

	void MobilizeForces();
	void ExecuteDefenseProtocol();
	void ExecuteOffensiveStrategy();
	void TrainSoldiers();
	void DevelopMilitaryTechnology();
};

// ‘лотское подразделение
class FleetDivision : public Division {
public:
	FleetDivision();

	void DeployFleet();
	void ExecuteNavalStrategy();
	void RepairShips();
	void ConductNavalExercises();
	void UpgradeFleet();
};

/**
 * 
 */
UCLASS()
class APS_ALPHA_API UCivilization : public UObject
{
	GENERATED_BODY()

public:
	UCivilization();  // Constructor

public:
    FName name;
    CivilizationType civilizationType;
    GovernmentType governmentType;
    EconomicSystem economicSystem;
    SocietyType societyType;
    int population;
    int presenceInSystems;
    int presenceInPlanets;
    Infrastructure infrastructure;
    TArray<Division> divisions;





};
