#pragma once

#include "Star.h"
#include "PlanetOrbit.h" 
#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "PlanetarySystem.generated.h"

enum class EOrbitDistributionType : uint8;
enum class EPlanetarySystemType : uint8;

UCLASS()
class APS_ALPHA_API APlanetarySystem : public ACelestialSystem
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	AStar* Star;

	UPROPERTY(VisibleAnywhere)
	int AmountOfPlanets;

	UPROPERTY(VisibleAnywhere)
	EPlanetarySystemType PlanetarySystemType;

	UPROPERTY(VisibleAnywhere)
	EOrbitDistributionType OrbitDistributionType;

	UPROPERTY(VisibleAnywhere)
	FName FullSpectralName;

	/// TODO: ZONES, TO SINGLE STRUCT
	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius HabitableZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius GasGiantsZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius ColdZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius IceZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius WarmZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius HotZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius InnerPlanetZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius OuterPlanetZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	TMap<int32, FZoneRadius> AsteroidBeltZoneRadius;

	UPROPERTY(VisibleAnywhere, Category = "Zones")
	FZoneRadius KuiperBeltZoneRadius;

	double HotZoneOuter;
	
	double WarmZoneOuter;
	
	double ColdZoneOuter;
	
	double IceZoneOuter;
	
	double GasGiantsZoneOuter;
	
	double KuiperBeltZoneOuter;
	
	double InnerZoneOuter;
	
	double OuterZoneOuter;
	
	double HabitableZoneOuter;
	
	double StarDeadZoneOuter;

	TArray<TSharedPtr<FPlanetData>> PlanetsList;

	UPROPERTY(VisibleAnywhere, Category = "Planets")
	TArray<APlanet*> PlanetsActorsList;

	UPROPERTY(VisibleAnywhere, Category = "Planets")
	TArray<APlanetOrbit*> PlanetOrbitsList;

	void SetStar(AStar* Star);

	void SetAmountOfPlanets(int AmountOfPlanets);

	void SetPlanetarySystemType(EPlanetarySystemType PlanetarySystemType);

	void SetOrbitDistributionType(EOrbitDistributionType OrbitDistributionType);

	void SetStarFullSpectralName(FName NewStarFullSpectralName);

	void SetPlanetsList(TArray<TSharedPtr<FPlanetData>> NewPlanetsList);
};
