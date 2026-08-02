#pragma once

#include "Star.h"
#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "StarSystem.generated.h"

enum class EStarType : uint8;

UCLASS()
class APS_ALPHA_API AStarSystem : public ACelestialSystem, public INavigatableBody
{
	GENERATED_BODY()

public:
	AStarSystem();

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(VisibleAnywhere, Category = "Star System")
	AStar* MainStar;

	/** Stable identity shared with the lightweight cluster record that produced this actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System|Generation")
	FGuid StableSystemId;

	/** Deterministic generation seed retained when a HISM point becomes a playable system. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System|Generation")
	int32 GenerationSeed{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System|Generation")
	bool bMaterializedFromCluster{false};

	void SetStarsAmount(int StarsAmount);

	void AddNewStar(AStar* MewStar);

	void SetStarSystemType(EStarType Type);

	UPROPERTY(VisibleAnywhere, Category = "Star System")
	double StarSystemRadius;

	UPROPERTY(VisibleAnywhere, Category = "Star System")
	FVector StarSystemZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	USphereComponent* StarSystemZone;

private:
	UPROPERTY(VisibleAnywhere, Category = "Star System")
	int StarsAmount;

	UPROPERTY(VisibleAnywhere, Category = "Star System")
	EStarType StarSystemType;

	UPROPERTY()
	TArray<AStar*> Stars;
};
