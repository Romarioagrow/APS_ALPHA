#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"

class AActor;
class APlanetaryBody;
class FAPSInfrastructure;
class UStaticMesh;
class UWorld;
struct FSoftObjectPath;

/**
 * Hubs and megastructures (Rio 03.10: "megastructures of more or less planetary scale, also system-scale like a Dyson
 * sphere; space hubs, huge, not the stations we usually build; build them in chains, a space elevator, then a space ring;
 * reuse what stands on the hand-made level"). The catalogue types (APSInfrastructureCatalog: SpaceHub, GrandHub,
 * SpaceElevator, OrbitalRing, DysonSwarm, DysonSphere) keep the infrastructure's own flow (orders, construction ships,
 * stocks, saves); this module gives them their place by the world's geometry, their look from the authored level's
 * meshes at the authored size, and the scaffold that grows while the construction ships work.
 *
 * Plain C++ (no reflection): the actors are ATechInfrastructure roots with runtime components, so ship navigation lists
 * them and fleet MOVE orders take them. Nothing new is saved: a built one is an FAPSBuiltStructure (its root's transform),
 * a scaffold is read from the fleet's working construction ships. Design: Docs/Design/MEGASTRUCTURES.md.
 */
namespace APSMegastructures
{
	/** The hand-made level's meshes (L_APS_SinglePlay_StartLocation, scanned 03.10) and their authored scale. */
	namespace Assets
	{
		/** The disc hub with the spindle hanging below (AI_Stations/01, 115 x 115 x 57 cm): 10000 in the level, 11.5 km. */
		constexpr const TCHAR* SpaceHub = TEXT("/Game/APS/APS_ALPHA/Assets/AI_Stations/01/73a7128ad60fd404a858c8197ba7d1c9.73a7128ad60fd404a858c8197ba7d1c9");
		constexpr double SpaceHubScale = 10000.0;
		/** The four-armed hub (Megastructure_P1_20, 67 x 67 x 59 cm): 100000 in the level, 67 km. */
		constexpr const TCHAR* GrandHub = TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Megastructure_P1_20/SM_Megastructure_P1_20.SM_Megastructure_P1_20");
		constexpr double GrandHubScale = 100000.0;
		/** The tower with the ring collar (AI_Buildings/02, 28 x 28 x 60 cm): 10000 on the home planet, 6 km tall. */
		constexpr const TCHAR* ElevatorTower = TEXT("/Game/APS/APS_ALPHA/Assets/AI_Buildings/02/88d53275bba1c5c4b2730187583c083a.88d53275bba1c5c4b2730187583c083a");
		constexpr double ElevatorTowerScale = 10000.0;
		/** The spline elevator merged mesh standing on the level's moon (10, 10, 13.5 there). A long thin mesh becomes the
		 * tether; a compact one stands at the base as the terminal and the tether is a cable. */
		constexpr const TCHAR* ElevatorTether = TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_BP_SplineElevator_Vert36_2.SM_MERGED_BP_SplineElevator_Vert36_2");
		constexpr double ElevatorTetherScale = 10.0;
		/** The spire hub with arms (Megastructure_P1_13, 41 x 66 x 60 cm): 10000 in the level, 6 km: the counterweight. */
		constexpr const TCHAR* Counterweight = TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Megastructure_P1_13/SM_Megastructure_P1_13.SM_Megastructure_P1_13");
		constexpr double CounterweightScale = 10000.0;
		/** The bronze ring (Megastructure_P1_23, a hoop 10 cm thick and 60 cm across, its axis X): around the level's moon
		 * at (400000, 4000000, 4000000), 1.884 of the moon's radius. */
		constexpr const TCHAR* Ring = TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Megastructure_P1_23/SM_Megastructure_P1_23.SM_Megastructure_P1_23");
		constexpr double RingThicknessRatio = 399993.446 / 4000000.119;
		constexpr double RingRadiusRatio = 1200.0 / 637.0;
		/** Engine shapes for the scaffolds, the cable and the collectors. */
		constexpr const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
		constexpr const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		constexpr const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	}

	enum class EKind : uint8
	{
		None,
		SpaceHub,
		GrandHub,
		SpaceElevator,
		OrbitalRing,
		DysonSwarm,
		DysonSphere
	};
	APS_ALPHA_API EKind KindOf(APSInfrastructure::EVisual Visual);

	// ---- Pure geometry (no world; APS.Megastructures.* tests) ----------------------------------------------------------

	/**
	 * The stationary orbit's radius in body radii: Kepler with M = rho 4/3 pi R^3, so r/R = (G rho T^2 / 3 pi)^(1/3) and the
	 * body's size drops out (Earth: 5.51 g/cm3, 23.93 h -> 6.62).
	 */
	APS_ALPHA_API double StationaryOrbitRatio(double DensityGramsPerCm3, double DayHours);

	/** Where a world megastructure's parts go, cm from the body's centre. */
	struct FWorldLayout
	{
		double BodyRadiusCm{0.0};
		/** Free room up to the nearest neighbour's surface (a moon, the parent planet), from the centre; 0: unlimited. */
		double ClearanceCm{0.0};
		double RingRadiusCm{0.0};
		double CounterweightRadiusCm{0.0};
		double HubRadiusCm{0.0};
		bool bRingFits{false};
		bool bElevatorFits{false};
		bool bHubFits{false};
	};
	/**
	 * The ring at the authored 1.884 radii (at least 1.3), the counterweight at the stationary orbit (2.2..9 radii, above
	 * the ring), hubs at 1.6 fleet slots out: everything at most 0.75..0.8 of the clearance, so no moon runs into them.
	 */
	APS_ALPHA_API FWorldLayout ComputeLayout(double BodyRadiusCm, double StationaryRatio, double ClearanceCm);

	/** A construction's visible step (0..Steps) for its progress 0..1; 0 until work starts, Steps at the end. */
	APS_ALPHA_API int32 StageOf(float Progress, int32 Steps);

	/**
	 * The rotation taking the mesh's axis MeshAxis (0 X, 1 Y, 2 Z) to WorldAxis and the next mesh axis ((MeshAxis + 1) % 3)
	 * toward Reference (made perpendicular); always a proper rotation.
	 */
	APS_ALPHA_API FQuat AxisRotation(int32 MeshAxis, const FVector& WorldAxis, const FVector& Reference);
	APS_ALPHA_API int32 LongestAxis(const FVector& Extent);
	APS_ALPHA_API int32 ShortestAxis(const FVector& Extent);
	/** A mesh whose bounds are Origin +- Extent, turned and scaled, with its mesh-space point Anchor at Point. */
	APS_ALPHA_API FTransform PlaceMesh(const FVector& Anchor, const FVector& Scale, const FQuat& Rotation, const FVector& Point);
	/**
	 * A hoop mesh (its thinnest axis is the hoop's axis) around Centre: its outer radius RadiusCm, its axis along Axis, its
	 * thickness ThicknessRatio of the plane scale (as authored).
	 */
	APS_ALPHA_API FTransform FitRing(const FVector& Origin, const FVector& Extent, const FVector& Centre, const FVector& Axis,
		const FVector& Reference, double RadiusCm, double ThicknessRatio);
	/** A mesh stretched along its longest axis from From to To, its other axes ThicknessCm across. */
	APS_ALPHA_API FTransform FitSpan(const FVector& Origin, const FVector& Extent, int32 MeshAxis, const FVector& From,
		const FVector& To, const FVector& Reference, double ThicknessCm);
	/** A mesh standing on Point with its Z along Up, scaled uniformly: the bottom of its bounds at Point (towers). */
	APS_ALPHA_API FTransform FitStanding(const FVector& Origin, const FVector& Extent, const FVector& Point, const FVector& Up,
		const FVector& Reference, double Scale);
	/** A mesh centred on Point with its Z along Axis, scaled uniformly (hubs). */
	APS_ALPHA_API FTransform FitCentred(const FVector& Origin, const FVector& Extent, const FVector& Point, const FVector& Axis,
		const FVector& Reference, double Scale);
	/** A direction in the equatorial plane of a body whose north is Up, Reference projected into it and turned by
	 * OffsetDegrees about Up; Fallback's projection when Reference lies along Up. */
	APS_ALPHA_API FVector EquatorDirection(const FVector& Up, const FVector& Reference, double OffsetDegrees, const FVector& Fallback);
	/** Points evenly round a circle (centre, axis, radius), starting at Start (projected) and going both ways from it, so a
	 * ring under construction grows from its first segment outward. */
	APS_ALPHA_API void RingPoints(const FVector& Centre, const FVector& Axis, const FVector& Start, double RadiusCm, int32 Count,
		TArray<FVector>& OutPoints);

	// ---- In the world ---------------------------------------------------------------------------------------------------

	/** The layout at a planet or moon: its radius, its neighbours, its density (by type when unknown) and its day. */
	APS_ALPHA_API bool LayoutAt(const AActor* Site, FWorldLayout& OutLayout);
	/** Why this hub or megastructure has no room at this place (empty when it has). */
	APS_ALPHA_API FText CheckRoom(const APSInfrastructure::FType& Type, const AActor* Site);
	/**
	 * Where a hub or megastructure's root stands at this place: an elevator on the equator a little east of the colony
	 * (else under the station, else by the world's name), a ring around the equator with its root where the elevator's
	 * tether crosses it, a hub in a high orbit off the ring's plane on a meridian of the world's own, a swarm or sphere
	 * around the star. The same from the order to the last stage, whoever builds it (Near, the builder, is not used: the
	 * ship holds by the scaffold, so a place taken from it would drift). False when the place does not suit it (OutRoot
	 * keeps the runtime's own spot).
	 */
	APS_ALPHA_API bool PlaceRoot(const APSInfrastructure::FType& Type, const AActor& Site, const FAPSInfrastructure& Infrastructure,
		const FVector* Near, FTransform& OutRoot);
	/**
	 * Where the construction ship holds while it raises one (Rio 03.10: by its scaffold, not at the world's generic slot):
	 * over a hub, beside the elevator's tether at the fleet's slot height, just outside the ring at its junction, over a
	 * swarm ring. False when the place does not suit the type.
	 */
	APS_ALPHA_API bool WorkSlot(const APSInfrastructure::FType& Type, const AActor& Site, const FAPSInfrastructure& Infrastructure,
		FVector& OutSlot);
	/**
	 * The built structure: an ATechInfrastructure root at Root, attached to the site, with the authored meshes fitted to the
	 * world (a missing asset falls back to engine shapes and says so in the log). Null only without a world.
	 */
	APS_ALPHA_API AActor* SpawnStructure(UWorld* World, const APSInfrastructure::FType& Type, AActor* Site, const FTransform& Root,
		const FString& ActorName, const FText& Name, const FAPSInfrastructure& Infrastructure);
	/** The meshes a type's look loads, to stream them in while the construction ships work. */
	APS_ALPHA_API void GetAssetPaths(const APSInfrastructure::FType& Type, TArray<FSoftObjectPath>& OutPaths);
}

/**
 * The construction sites of hubs and megastructures (Rio 03.10: "under construction: ring segments appear progressively,
 * the tether grows"): for every construction ship at work on one, a scaffold at the structure's own place, grown to the
 * work's stage; gone when the work ends (built or cancelled). Read twice a second from the fleet; a stage changes the
 * scaffold's instances only when it moves on, nothing runs per frame. Owned by FAPSInfrastructure.
 */
class FAPSMegastructureYard
{
public:
	explicit FAPSMegastructureYard(UWorld* InWorld);
	~FAPSMegastructureYard();
	FAPSMegastructureYard(const FAPSMegastructureYard&) = delete;
	FAPSMegastructureYard& operator=(const FAPSMegastructureYard&) = delete;

	void Tick(float DeltaSeconds, const FAPSInfrastructure& Infrastructure);
	/** The work at this place ended (the structure stands or the order went): its scaffold goes. */
	void Finish(const AActor* Site, FName Type);
	/** Console and tests: shows a type's scaffold at a place at this progress (negative: back to the fleet's own work). */
	void SetPreview(AActor* Site, FName Type, float Progress);
	int32 GetScaffoldCount() const { return Scaffolds.Num(); }

private:
	struct FScaffold;
	void Grow(FScaffold& Scaffold, float Progress);
	bool Create(FScaffold& Scaffold, const APSInfrastructure::FType& Type, AActor& Site, const FAPSInfrastructure& Infrastructure);
	void Destroy(FScaffold& Scaffold);

	TWeakObjectPtr<UWorld> World;
	TMap<FString, TUniquePtr<FScaffold>> Scaffolds;
	/** Console previews: key (site + type) and progress. */
	TMap<FString, TPair<TWeakObjectPtr<AActor>, float>> Previews;
	float Clock{0.0f};
};
