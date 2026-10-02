#pragma once
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Enums/StartStation.h"
#include "SpawnParameters.generated.h"

class ASpaceHeadquarters;
class ASpaceShipyard;
class ASpaceship;
class ASpaceStation;
class APawn;

UENUM(BlueprintType)
enum class EAPSCivilizationArchetype : uint8
{
	Balanced      UMETA(DisplayName = "Balanced"),
	Explorers     UMETA(DisplayName = "Explorers"),
	Industrial    UMETA(DisplayName = "Industrial"),
	Scientific    UMETA(DisplayName = "Scientific"),
	Diplomatic    UMETA(DisplayName = "Diplomatic"),
	Military      UMETA(DisplayName = "Military")
};

UENUM(BlueprintType)
enum class EAPSGovernmentType : uint8
{
	Democracy     UMETA(DisplayName = "Democracy"),
	Council       UMETA(DisplayName = "Council"),
	Technocracy   UMETA(DisplayName = "Technocracy"),
	Monarchy      UMETA(DisplayName = "Monarchy"),
	Corporate     UMETA(DisplayName = "Corporate"),
	Collective    UMETA(DisplayName = "Collective")
};

UENUM(BlueprintType)
enum class EAPSEconomicSystem : uint8
{
	Mixed         UMETA(DisplayName = "Mixed Economy"),
	Capitalism    UMETA(DisplayName = "Capitalism"),
	Planned       UMETA(DisplayName = "Planned Economy"),
	ResourceBased UMETA(DisplayName = "Resource Based")
};

UENUM(BlueprintType)
enum class EAPSSocietyType : uint8
{
	Cooperative   UMETA(DisplayName = "Cooperative"),
	Individualist UMETA(DisplayName = "Individualist"),
	Expansionist  UMETA(DisplayName = "Expansionist"),
	Scientific    UMETA(DisplayName = "Scientific"),
	Traditional   UMETA(DisplayName = "Traditional")
};

/** C19 (Rio 02.10, "for the ground start choose the base colony, the ground vehicles, the launch pad"): what the colony
 * on the home world orders with the founding, built over the first minutes. */
UENUM(BlueprintType)
enum class EAPSColonyStartPackage : uint8
{
	Outpost     UMETA(DisplayName = "Outpost"),
	Standard    UMETA(DisplayName = "Standard"),
	Settlement  UMETA(DisplayName = "Settlement")
};

/** C19: what the colony's landing pad stands on. */
UENUM(BlueprintType)
enum class EAPSLaunchPadStart : uint8
{
	Stilts  UMETA(DisplayName = "On stilts"),
	Plinth  UMETA(DisplayName = "On a plinth")
};

UCLASS()
class APS_ALPHA_API USpawnParameters : public UObject
{
	GENERATED_BODY()
	
public:
	static constexpr int32 MaxStartingFleetSize = 24;
	static constexpr int32 MaxInfrastructurePerCategory = 16;

	/** Normalizes menu/save input before it is frozen into a generated session. */
	UFUNCTION(BlueprintCallable, Category = "Civilization")
	void SanitizeForGeneration();

	/** Number of physical civilization actors requested by the current manifest. */
	UFUNCTION(BlueprintPure, Category = "Civilization")
	int32 GetPlannedPhysicalActorCount() const;

	UFUNCTION(BlueprintPure, Category = "Civilization")
	int32 GetPlannedInfrastructureActorCount() const;

	/** Controllable units the start creates: the fleet's ships and the colony's ground vehicles (menu manifest). */
	int32 GetPlannedUnitCount() const;

	/** Structures the start creates: HQ, shipyard, stations and outposts, and the colony's founding modules. */
	int32 GetPlannedStructureCount() const;

	/** Persistent civilization setup shared by generation UI and generated gameplay level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization")
	FString CivilizationName{TEXT("APOSFERA CIVILIZATION")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization")
	EAPSCivilizationArchetype CivilizationArchetype{EAPSCivilizationArchetype::Balanced};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization")
	EAPSGovernmentType GovernmentType{EAPSGovernmentType::Democracy};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization")
	EAPSEconomicSystem EconomicSystem{EAPSEconomicSystem::Mixed};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization")
	EAPSSocietyType SocietyType{EAPSSocietyType::Cooperative};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "1", ClampMax = "100000000"))
	int32 FoundingPopulation{12000};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "0", ClampMax = "2000000000"))
	int64 StartingCredits{1250000};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "1", ClampMax = "10"))
	int32 TechnologyLevel{1};

	/** Total starting ships, including the home ship required by the starter hierarchy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "1", ClampMax = "24"))
	int32 StartingFleetSize{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0", ClampMax = "16"))
	int32 StarOutposts{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0", ClampMax = "16"))
	int32 PlanetOutposts{1};

	/** Total orbital stations, including the mandatory home station. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "1", ClampMax = "16"))
	int32 OrbitalOutposts{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0", ClampMax = "16"))
	int32 GroundOutposts{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 ExplorationDivisionLevel{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 IndustryDivisionLevel{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 ScienceDivisionLevel{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 CivilAffairsDivisionLevel{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 MilitaryDivisionLevel{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Divisions", meta = (ClampMin = "0", ClampMax = "20"))
	int32 FleetDivisionLevel{1};

	/** Where the playable character enters the generated home system. */
	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	ECharSpawnPlace CharacterSpawnPlace{ECharSpawnPlace::PlanetOrbit};

	/** Shared orbital preset for the initial station/headquarters group. */
	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EOrbitHeight HomeStationOrbitHeight{EOrbitHeight::LowOrbit};

	/** For orbital starts: which home-complex station the pilot starts in (the headquarters by default, Rio 29.09). */
	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EAPSStartStation StartStation{EAPSStartStation::Headquarters};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<APawn> BP_CharacterClass;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceStation> BP_HomeSpaceStation;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceship> BP_HomeSpaceship;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceShipyard> BP_HomeSpaceShipyard;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceHeadquarters> BP_HomeSpaceHeadquarters;

	/** C19: the colony's founding package (Outpost: the base alone, as before). */
	UPROPERTY(EditAnywhere, Category = "Player Spawn|Ground Start")
	EAPSColonyStartPackage ColonyStartPackage{EAPSColonyStartPackage::Outpost};

	UPROPERTY(EditAnywhere, Category = "Player Spawn|Ground Start")
	EAPSLaunchPadStart LaunchPadStart{EAPSLaunchPadStart::Stilts};

	/** C19: the ground vehicles parked at the colony, as bits: 1 rover, 2 hover, 4 drone (all three by default). */
	UPROPERTY(EditAnywhere, Category = "Player Spawn|Ground Start", meta = (ClampMin = "0", ClampMax = "7"))
	int32 GroundVehicleMask{7};

	/** The colony modules the package orders, in order (catalogue ids, APSColonyModuleCatalogue). */
	static void GetColonyStartModules(EAPSColonyStartPackage Package, TArray<FName>& OutModules);
};
