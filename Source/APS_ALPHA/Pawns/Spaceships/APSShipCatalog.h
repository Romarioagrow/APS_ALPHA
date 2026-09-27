#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Spaceship.h"
#include "APSShipCatalog.generated.h"

/** One ship Blueprint that procedural generation may spawn. */
USTRUCT(BlueprintType)
struct FAPSShipCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	TSubclassOf<ASpaceship> ShipClass;

	/** Stored with the entry so filtering does not need to load the Blueprint class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship", meta = (ClampMin = "0.0"))
	float Weight{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	bool bAllowInStartingFleet{true};
};

/** Ship Blueprints available to procedural generation, for example the starting fleet escorts. */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSShipCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ships")
	TArray<FAPSShipCatalogEntry> Ships;

	/** Smallest size class used for starting-fleet escorts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Fleet")
	ESpaceshipSizeClass StartingFleetMinSizeClass{ESpaceshipSizeClass::XXS};

	/** Largest size class used for starting-fleet escorts, so they fit the shipyard spacing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Starting Fleet")
	ESpaceshipSizeClass StartingFleetMaxSizeClass{ESpaceshipSizeClass::M};

	/** Weighted pick among starting-fleet entries. Returns INDEX_NONE when nothing matches. */
	int32 PickStartingFleetIndex(FRandomStream& Stream) const;

	/** Weighted pick among starting-fleet entries. Returns null when nothing matches. */
	TSubclassOf<ASpaceship> PickStartingFleetShip(FRandomStream& Stream) const;

private:
	bool IsStartingFleetCandidate(const FAPSShipCatalogEntry& Entry) const;
};
