#pragma once

#include "CoreMinimal.h"

class AActor;
class AAstroGenerator;
class APlanet;
class APlanetarySystem;
class AStar;
class AStarSystem;
class FAPSStarSystems;
class UWorld;
struct FPlanetarySystemModel;

/**
 * D.8 (Rio 01.10-02.10: "colonize the cluster's other systems", "dense systems for the cluster's stars, planets only where
 * there is room, the systems' spheres never overlap"): the cluster system the pilot comes to becomes real. Its star
 * (AAstroGenerator::MaterializeClusterStarSystem) and its planets and moons are spawned as the home system's are, on
 * orbits drawn in to fit its room among the neighbours (half the distance to the nearest star); a planet that does not
 * fit is left out. Surfaces stream as the home system's do. Once the pilot is well away, the surfaces are released and
 * the system goes back to its catalogue point. One system at a time besides the home; a system always comes back the
 * same, seeded by its catalogue record.
 */
class APS_ALPHA_API FAPSSystemMaterializer
{
public:
	explicit FAPSSystemMaterializer(UWorld* InWorld);

	/** Four times a second from FAPSStarSystems::Tick, once the catalogue is read. */
	void Update(FAPSStarSystems& Systems, float DeltaSeconds);

	/** The catalogue index of the system that stands materialized (or is being built or released), or INDEX_NONE. */
	int32 GetActiveIndex() const { return ActiveIndex; }
	/** Its planets, once spawned. */
	void GetPlanets(TArray<APlanet*>& OutPlanets) const;

	/** Test runs: moves the piloted ship to a system's edge facing its star, or near its first planet once spawned. */
	bool VisitForTest(const FAPSStarSystems& Systems, int32 Index, bool bNearPlanet);

	void LogState(const FAPSStarSystems& Systems) const;

private:
	enum class EStage : uint8
	{
		Idle,
		Spawning,
		Ready,
		Draining
	};

	bool Begin(FAPSStarSystems& Systems, int32 Index);
	/** Spawns the next planet with its moons; false once all are out. */
	bool SpawnNextPlanet();
	void BeginDrain(const TCHAR* Reason);
	/** True once no surface of the system is in use or draining (or after a time-out). */
	bool IsDrained(float DeltaSeconds);
	void Finish();

	TWeakObjectPtr<UWorld> World;
	EStage Stage{EStage::Idle};
	int32 ActiveIndex{INDEX_NONE};
	int32 ActiveInstance{INDEX_NONE};
	FString ActiveName;
	TWeakObjectPtr<AAstroGenerator> Generator;
	TWeakObjectPtr<AStarSystem> System;
	TWeakObjectPtr<AStar> Star;
	TWeakObjectPtr<APlanetarySystem> PlanetarySystem;
	TSharedPtr<FPlanetarySystemModel> Model;
	/** The model's planets that fit, inner first: their model index, orbit radius (cm) and orbit plane. */
	struct FPlannedPlanet
	{
		int32 ModelIndex{INDEX_NONE};
		double OrbitCm{0.0};
		FRotator Orbit{FRotator::ZeroRotator};
	};
	TArray<FPlannedPlanet> Plan;
	int32 NextPlanet{0};
	TArray<TWeakObjectPtr<APlanet>> Planets;
	int32 SystemSeed{0};
	int32 WorldSeed{0};
	/** "C<record id>": the generation address of the system's bodies, for their names and surface seeds. */
	FString Address;
	float AwaySeconds{0.0f};
	float DrainSeconds{0.0f};
	/** A system that would not materialize is not tried again for a while. */
	int32 FailedIndex{INDEX_NONE};
	double FailedUntilSeconds{0.0};
};
