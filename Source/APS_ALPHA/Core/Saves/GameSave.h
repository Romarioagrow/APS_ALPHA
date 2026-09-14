#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "GameSave.generated.h"

class APlanetaryBody;
class UGeneratedWorld;
struct FPlanetData;
struct FGeneratedWorldData;
struct FActorSaveData;

UCLASS()
class APS_ALPHA_API UGameSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Versioned contract for model snapshots and player-state persistence. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	int32 SaveFormatVersion{2};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Basic")
	FString WorldName;

	UPROPERTY(VisibleAnywhere, Category = "Basic")
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = "State")
	TArray<FActorSaveData> ActorSaveDataArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	bool bHasCivilizationManifest{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Civilization")
	FAPSCivilizationRuntimeManifest CivilizationManifest;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<FGeneratedWorldData> GeneratedWorldsDataArray;

	/**
	 * Tagged UObject snapshot of the actor-free generation model.  The legacy
	 * summary above remains browser- and migration-friendly; this payload retains
	 * the canonical stellar catalogue, generation seed and every authored edit.
	 */
	UPROPERTY(VisibleAnywhere, Category = "World")
	TArray<uint8> GeneratedWorldModelData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<FPlanetData> InhabitedPlanetsDataArray;

	/** The possessed pawn is not an ABaseActor and therefore needs its own record. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	bool bHasPlayerPawnState{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	FString PlayerPawnClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	FTransform PlayerPawnTransform{FTransform::Identity};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	FRotator PlayerControlRotation{FRotator::ZeroRotator};
	
	UFUNCTION(BlueprintCallable)
	FGeneratedWorldData GetGeneratedWorld();
};
