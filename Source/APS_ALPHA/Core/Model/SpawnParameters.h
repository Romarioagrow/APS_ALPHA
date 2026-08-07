#pragma once
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
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

UCLASS()
class USpawnParameters : public UObject
{
	GENERATED_BODY()
	
public:
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "1"))
	int32 FoundingPopulation{12000};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "0"))
	int64 StartingCredits{1250000};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "1", ClampMax = "10"))
	int32 TechnologyLevel{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization", meta = (ClampMin = "0"))
	int32 StartingFleetSize{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0"))
	int32 StarOutposts{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0"))
	int32 PlanetOutposts{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0"))
	int32 OrbitalOutposts{1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Civilization|Infrastructure", meta = (ClampMin = "0"))
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
};
