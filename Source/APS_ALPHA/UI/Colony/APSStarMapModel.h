#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"

class AActor;
class ASpaceship;
class FAPSStarSystems;
class UWorld;

/**
 * Rio 05.10 (star map): the STAR level of the in-game maps as data, without Slate. The approved layout is "rings by
 * rank": the centre system (home, or the system the pilot is in) in the middle and every other shown system in its real
 * direction projected into the home system's orbital plane, but at the radius of its rank by distance: the 6 nearest on
 * the first ring, the next 10 on the second, the next 9 on the third, and the farther ones the map keeps (known, in use
 * by the fleet, picked) on a dashed outer ring. Stars spread along their ring keeping their order. The drawing reads only
 * the order of the distances and the directions, so it is the same at compressed (AU) and real (light-year) distances;
 * only the rings' captions carry the scale. SAPSStarScheme draws it; the tests check the layout without a world.
 */
namespace APSStarMap
{
	/** Rank rings: the 6 nearest, the next 10, the next 9 (25); everything else shown is on the outer ring. */
	inline constexpr int32 RankRings = 3;
	inline constexpr int32 RingSizes[RankRings] = {6, 10, 9};
	inline constexpr int32 NearestCount = 25;
	inline constexpr int32 OuterRing = 3;
	/** At most this many systems besides the centre: the 25 nearest, every known one, those in use, the picked one. */
	inline constexpr int32 MaxShown = 60;
	/** Ring radii in layout units: the approved mock's 126, 210, 284 and 332 px over 126 (the widget fits its own scale). */
	inline constexpr double RingRadius[4] = {1.0, 210.0 / 126.0, 284.0 / 126.0, 332.0 / 126.0};
	/** Inside a rank ring a nearer member sits a little inside it, a farther one outside (the mock's 20 px), order kept. */
	inline constexpr double RingSpread = 20.0 / 126.0;
	/** Above or below the plane by more than this sine (53 degrees) a system is marked with an up or down triangle. */
	inline constexpr double ElevationMark = 0.8;

	enum class EFilter : uint8
	{
		All,
		Known,
		Claimed,
		Uncharted
	};

	struct FLayoutItem
	{
		/** From the centre system, in any unit: only the directions and the order of the distances are read. */
		FVector Offset{FVector::ZeroVector};
		/** 0..2 the rank rings, 3 the outer ring. */
		int32 Ring{0};
	};

	struct FLayoutSlot
	{
		/** Layout units: the first ring is 1. */
		double Radius{0.0};
		/** Radians counter-clockwise from the plane's first axis after the spreading; drawn at (cos, -sin), y down. */
		double Angle{0.0};
		/** The real direction before the spreading. */
		double Bearing{0.0};
		/** Sine of the angle above (+) or below (-) the plane. */
		double Elevation{0.0};
		FVector2D Position{FVector2D::ZeroVector};
	};

	/**
	 * Rank rings for Count systems, nearest first, the first NearCount of them the near ones: by rank, except that a system
	 * keeps the ring it had (PreviousRings, INDEX_NONE when new) while its rank stays within one place of that ring, so one
	 * newcomer at a boundary does not move a row from ring to ring. The rest go to the outer ring.
	 */
	APS_ALPHA_API void AssignRings(int32 Count, int32 NearCount, const TArray<int32>& PreviousRings, TArray<int32>& OutRings);

	/** Lays the items out as the namespace's comment says; deterministic, the same for offsets scaled by any factor. */
	APS_ALPHA_API void Layout(const TArray<FLayoutItem>& Items, const FVector& PlaneU, const FVector& PlaneV,
		TArray<FLayoutSlot>& OutSlots);
	/** The band at a ring's top kept free of stars for its caption: half its angle, radians. */
	APS_ALPHA_API double CaptionBand(int32 Ring);
	/** The least angle between neighbours on a ring of Count members: the mock's arc, less when many share the ring. */
	APS_ALPHA_API double RingSeparation(int32 Ring, int32 Count);

	/** A system the map may show and why: 0 the pick, 1 in use by the fleet, 2 claimed, 3 known, 4 one of the nearest. */
	struct FCandidate
	{
		int32 Index{INDEX_NONE};
		double DistanceCm{0.0};
		uint8 Priority{4};
	};

	/**
	 * The shown set, nearest first: the near candidates (the 25 nearest) and then the extras, at most MaxCount. Over the
	 * limit the extras go by priority, the farthest first (the pick and the fleet's never go). OutNearCount: how many of
	 * OutShown are near ones (the rank rings); the rest are the outer ring's.
	 */
	APS_ALPHA_API void SelectShown(const TArray<FCandidate>& Near, const TArray<FCandidate>& Extras, int32 MaxCount,
		TArray<FCandidate>& OutShown, int32& OutNearCount);

	/** One shown system: what the map draws and the panel lists. */
	struct FSystem
	{
		/** In FAPSStarSystems. */
		int32 Index{INDEX_NONE};
		FGuid Id;
		FString Name;
		/** The primary's class as the catalogue writes it ("K4V", "DA"), and its kind ("MAIN SEQUENCE"). */
		FString Spectral;
		FText Kind;
		/** "A", "AB", "ABC": one letter a star, as the designations go. */
		FString Designation;
		int32 StarCount{1};
		FLinearColor Colour{FLinearColor::White};
		/** Disc radius by class, px, on the map and in the fleet's picker; giants glow. */
		float DiscRadius{5.0f};
		float PickerRadius{4.0f};
		bool bGiant{false};
		APSStars::EKnowledge Knowledge{APSStars::EKnowledge::Catalogued};
		bool bClaimed{false};
		bool bHome{false};
		bool bCentre{false};
		bool bInReach{false};
		/** Its scheme can be drawn now: the home system, or the one that stands materialized. */
		bool bMaterialized{false};
		/** The worlds as far as they are known: -1 unknown; the count (the catalogue's "up to" until it stands); their colours
		 * once their types are known (the system stands and is surveyed). */
		int32 Worlds{-1};
		bool bWorldsExact{false};
		TArray<FLinearColor> WorldColours;
		TArray<FText> WorldTypes;
		/** 0 none known, 1 detected, 2 located, 3 investigated; the kind for its name. */
		uint8 Anomaly{0};
		int32 AnomalyKind{INDEX_NONE};
		int32 Outposts{0};
		bool bColony{false};
		bool bHeadquarters{false};
		/** Ships of the civilization here and not under way, and the division most of them serve. */
		int32 Ships{0};
		APSFleet::EDivision ShipsDivision{APSFleet::EDivision::MainFleet};
		/** A fleet order's target or a ship's place: kept on the map, labelled with the claimed ones. */
		bool bInUse{false};
		/** From the centre system, cm. */
		double DistanceCm{0.0};
		/** Rank by distance among the shown (1 the nearest; the centre 0); the ring (0..2 rank rings, 3 the outer one). */
		int32 Rank{0};
		int32 Ring{0};
		FLayoutSlot Slot;
	};

	struct FRoute
	{
		FString CallSign;
		APSFleet::EDivision Division{APSFleet::EDivision::MainFleet};
		APSFleet::EOrder Order{APSFleet::EOrder::None};
		APSFleet::EPhase Phase{APSFleet::EPhase::Idle};
		TWeakObjectPtr<ASpaceship> Ship;
		/** Snapshot systems; From == To: work in a system (a progress arc round it). */
		int32 From{INDEX_NONE};
		int32 To{INDEX_NONE};
		/** The part of the way flown, 0..1, and the work done there, 0..1. */
		float Share{0.0f};
		float Progress{0.0f};
		/** Seconds left to arrive (negative: unknown). */
		double EtaSeconds{-1.0};
		/** "E-03  PROBE  0:18", "C-02  BUILDING  40%". */
		FText Chip;
	};

	struct FLink
	{
		int32 A{INDEX_NONE};
		int32 B{INDEX_NONE};
	};

	struct FRing
	{
		int32 Count{0};
		double NearCm{0.0};
		double FarCm{0.0};
		/** "6 NEAREST  /  UP TO 2.400 AU": the map's only scale, in the world's own unit. */
		FText Caption;
	};

	struct FSnapshot
	{
		bool bReady{false};
		/** Why nothing is drawn yet. */
		FText Status;
		FGuid CentreId;
		/** Systems[0] is the centre; the rest nearest first. */
		TArray<FSystem> Systems;
		TArray<FRoute> Routes;
		TArray<FLink> Links;
		FRing Rings[4];
		/** The catalogue (galaxy systems near the pilot included) and what the civilization knows of it, the centre aside. */
		int32 CatalogueCount{0};
		int32 KnownCount{0};
		/** Shown, the centre aside: all, known, claimed, uncharted (the filters' counts). */
		int32 FilterCounts[4]{0, 0, 0, 0};
		/** The galaxy's core, as a direction in the plane. */
		bool bCore{false};
		double CoreAngle{0.0};
		/** The pilot: in a shown system, else between them (by distance and direction, as the systems are). */
		bool bPilot{false};
		int32 PilotSystem{INDEX_NONE};
		FVector2D PilotPosition{FVector2D::ZeroVector};
		/** The map's plane and the centre's place in the world, for the live parts. */
		FVector PlaneU{FVector::ForwardVector};
		FVector PlaneV{FVector::RightVector};
		FVector CentreLocation{FVector::ZeroVector};
	};

	/** HOME, CLAIMED, CATALOGUED, SCANNED or SURVEYED. */
	APS_ALPHA_API FText StateName(const FSystem& System);
	APS_ALPHA_API FLinearColor StateColour(const FSystem& System);
	/** The plate's first line: "M5V  /  SCANNED", "DA  /  SCANNED  /  NO WORLDS", "G2V  /  HOME". */
	APS_ALPHA_API FText TypeLine(const FSystem& System);
	/** "0:38", "1:02:05"; empty when unknown (negative). */
	APS_ALPHA_API FString FormatEta(double Seconds);
	APS_ALPHA_API bool PassesFilter(const FSystem& System, EFilter Filter);
	/** "E-03  PROBE": a unit's order in a word or two. */
	APS_ALPHA_API FText OrderWord(APSFleet::EOrder Order);
	/**
	 * The ship a system order goes to, as the object page's actions pick it (APSObjectActions): a probe from exploration
	 * first, a survey of the system or an expedition from science first, a move from the main fleet; the nearest idle or
	 * holding one that can take it. Null with the first refusal when none can.
	 */
	APS_ALPHA_API ASpaceship* PickShip(const FAPSFleetCommand& Fleet, APSFleet::EOrder Order, const AActor* Target,
		FText& OutRefusal);
	/** The star actor standing for a system now: the home star, or the materialized system's; null when it is not real. */
	APS_ALPHA_API AActor* FindStarActor(UWorld* World, const FAPSStarSystems& Stars, int32 Index);
}

/**
 * The star map's data for one widget: read from the world when a revision (stars, fleet, infrastructure) or the query
 * changes, the fleet's progress twice a second. Plain C++ (SAPSStarScheme owns one).
 */
class APS_ALPHA_API FAPSStarMapModel
{
public:
	struct FQuery
	{
		/** The system in the centre; invalid: the home system. */
		FGuid Centre;
		/** Always shown, wherever they are (the pick, a planned order's target). */
		TArray<FGuid> Pinned;

		bool operator==(const FQuery& Other) const { return Centre == Other.Centre && Pinned == Other.Pinned; }
		bool operator!=(const FQuery& Other) const { return !(*this == Other); }
	};

	/** Re-reads the world when a revision or the query changed (true: rebuilt), then the live parts. */
	bool Update(UWorld* World, const FQuery& Query, bool bForce = false);
	const APSStarMap::FSnapshot& Get() const { return Snapshot; }
	/** Changes only when what is drawn where changes (not with the ships' progress), for the labels' cache. */
	uint32 GetLayoutVersion() const { return LayoutVersion; }
	/** The snapshot index of a system, or INDEX_NONE when it is not shown. */
	int32 IndexOf(const FGuid& Id) const;
	/**
	 * Where a ship is on the map: in a shown system (OutSystem), else between them by its distance from the centre and its
	 * direction (OutPosition, layout units). False without the catalogue or the ship.
	 */
	bool LocateShip(UWorld* World, const AActor* Ship, int32& OutSystem, FVector2D& OutPosition) const;

private:
	void Rebuild(UWorld* World, const FQuery& Query);
	void UpdateLive(UWorld* World);
	/** Layout radius for a distance from the centre, between the shown systems' radii (monotone in the distance). */
	double RadiusAt(double DistanceCm) const;
	FVector2D PlaceAt(const FVector& WorldLocation) const;

	APSStarMap::FSnapshot Snapshot;
	FQuery SeenQuery;
	uint32 SeenStars{0};
	uint32 SeenFleet{0};
	uint32 SeenInfrastructure{0};
	int32 SeenMaterialized{INDEX_NONE};
	bool bBuilt{false};
	uint32 LayoutVersion{0};
	uint32 LayoutHash{0};
	TMap<FGuid, int32> SnapshotIndex;
	TMap<FGuid, int32> PreviousRings;
	/** Where each fleet order's flight began ("CALLSIGN|order|target" -> catalogue index), seen when it began. */
	TMap<FString, int32> RouteOrigins;
	/** The shown systems' distances from the centre, nearest first, and their layout radii. */
	TArray<TPair<double, double>> RadiusByDistance;
};
