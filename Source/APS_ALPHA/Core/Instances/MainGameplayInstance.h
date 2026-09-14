#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MainGameplayInstance.generated.h"

class USpawnParameters;
class UGeneratedWorld;
class UCivilization;

UCLASS()
class UMainGameplayInstance : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "World Generation")
	UGeneratedWorld* NewGeneratedWorld;

	UPROPERTY(BlueprintReadWrite, Category = "World Generation")
	USpawnParameters* SpawnParameters;

	/** Runtime civilization selected in Slate and consumed by generated gameplay. */
	UPROPERTY(BlueprintReadOnly, Category = "Civilization")
	TObjectPtr<UCivilization> CurrentCivilization;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loading")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loading")
	bool bIsLoadingMode{false};

	/** True from save selection until the reconstructed hierarchy has been overlaid. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loading")
	bool bPendingSavedWorldReplay{false};

	/** Prevents the legacy level Blueprint from restoring actors before generation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loading")
	bool bSavedWorldHierarchyReady{false};

	/**
	 * The Start Single Game route opens the authored showcase map exactly as it
	 * is saved. Placed AstroGenerator actors must not reinterpret that map as a
	 * procedural-generation request during BeginPlay.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	bool bUseAuthoredSinglePlayWorld{false};

	/**
	 * True only for a committed Generate Civilization handoff. Generate Space
	 * and Create Planet share the astronomical model but must not inherit the
	 * generator Blueprint's default headquarters/station/character classes.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	bool bSpawnGeneratedCivilization{false};
};
