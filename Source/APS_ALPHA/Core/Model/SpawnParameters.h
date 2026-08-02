#pragma once
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "SpawnParameters.generated.h"

class ASpaceHeadquarters;
class ASpaceShipyard;
class ASpaceship;
class ASpaceStation;
class AControlledPawn;

UCLASS()
class USpawnParameters : public UObject
{
	GENERATED_BODY()
	
public:
	/** Where the playable character enters the generated home system. */
	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	ECharSpawnPlace CharacterSpawnPlace{ECharSpawnPlace::PlanetOrbit};

	/** Shared orbital preset for the initial station/headquarters group. */
	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EOrbitHeight HomeStationOrbitHeight{EOrbitHeight::LowOrbit};

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
};
