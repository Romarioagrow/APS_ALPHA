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

	UPROPERTY(VisibleAnywhere, Category = "Star System")
	AStar* MainStar;

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
