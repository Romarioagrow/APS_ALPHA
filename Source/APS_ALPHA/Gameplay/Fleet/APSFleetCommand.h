#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"

class AActor;
class APlanetaryBody;
class ASpaceShipyard;
class FAPSStarSystems;
class UCivilization;
class UWorld;

/**
 * Fleet command (Rio, 01.10: "open K and see every ship, on the map and schematically, and give the real ships orders by
 * their type and by what we know: survey a planet, build an outpost"). The civilization's ships are units in four
 * divisions. An order flies the real ship actor on its own autopilot (nobody on board), works at the target and reports
 * to the journal: surveys reveal the body's generated surface profile, outposts need a survey first.
 *
 * Plain C++ owned by UAPSFleetCommandSubsystem, so the rules and the flight stay testable without a world. Nothing here
 * is saved yet: orders, surveys and outposts wait for the colony save handoff (CLAUDE_HANDOFF_COLONY_SAVES_2026-09-30).
 */
namespace APSFleet
{
	enum class EDivision : uint8
	{
		MainFleet,
		Exploration,
		Science,
		Construction,
		Count
	};

	enum class EOrder : uint8
	{
		None,
		Move,
		Survey,
		BuildOutpost,
		Return,
		// Construction after an outpost (Rio, 01.10). Appended: saved orders keep their numbers.
		BuildStation,
		BuildShipyard,
		BuildHeadquarters,
		/** To a located anomaly: the crew goes down and investigates it (exploration or science ships). */
		Expedition,
		/**
		 * The expansion (Rio 02.10: "colonize the other systems: a scan, a probe, a visit or a unit, then a station or an
		 * outpost"). A probe from an exploration or science ship scans a star system from its edge; a survey of the system
		 * charts it; BuildStructure raises an infrastructure catalogue type (APSInfrastructureCatalog) at a world or in a
		 * star system. Targets in other systems are the systems' anchors (FAPSStarSystems::GetAnchor).
		 */
		Probe,
		SurveySystem,
		BuildStructure
	};
	constexpr EOrder LastOrder = EOrder::BuildStructure;

	/**
	 * Anomalies (Rio, 01.10: "ships find anomalies on planets; to study one you land or send an expedition"). About two
	 * worlds in five hide one, the same world always the same one. A survey detects it, a study locates its site, then an
	 * expedition or the pilot on foot at the site investigates it.
	 */
	enum class EAnomalyState : uint8
	{
		Hidden,
		Detected,
		Located,
		Investigated
	};

	/** What shapes a world's anomaly: its generated surface profile and its air. */
	struct FAnomalyTraits
	{
		float Temperature{0.5f};
		float Biomass{0.0f};
		float Metallic{0.5f};
		float Seismic{0.3f};
		bool bAirless{false};
	};
	constexpr int32 AnomalyKindCount = 7;
	/** The anomaly the world with this key hides, or false; deterministic. OutDirection: its site, body-local. */
	APS_ALPHA_API bool RollAnomaly(const FString& WorldKey, const FAnomalyTraits& Traits, int32& OutKind, FVector& OutDirection);
	APS_ALPHA_API FText AnomalyName(int32 Kind);
	/** What the investigation finds. */
	APS_ALPHA_API FText AnomalyStory(int32 Kind);

	/**
	 * What the construction division raises in orbit once a world has an outpost: a station (work there goes faster),
	 * a shipyard (a slipway of its own in the SHIPYARD tab) and a sector headquarters (the fleet flies faster).
	 */
	enum class EStructure : uint8
	{
		Station,
		Shipyard,
		Headquarters,
		Count
	};

	/** Where an order stands: leaving the berth, under way, at work at the target, or done and holding there. */
	enum class EPhase : uint8
	{
		Idle,
		Departing,
		Transit,
		Working,
		Holding
	};

	/** What the civilization knows of a planet or moon: nothing, a survey (exploration), a study (science). */
	enum class ESurvey : uint8
	{
		Unknown,
		Surveyed,
		Studied
	};

	APS_ALPHA_API FText DivisionName(EDivision Division);
	APS_ALPHA_API FText DivisionRole(EDivision Division);
	APS_ALPHA_API FLinearColor DivisionColour(EDivision Division);
	APS_ALPHA_API FText OrderName(EOrder Order);
	APS_ALPHA_API FText SurveyName(ESurvey Survey);
	APS_ALPHA_API FText StructureName(EStructure Structure);
	/** The structure a construction order raises; false for every other order (an outpost included). */
	APS_ALPHA_API bool StructureOf(EOrder Order, EStructure& OutStructure);
	/** What the structure needs at the world first: an outpost for a station, a station for a shipyard or a HQ. */
	APS_ALPHA_API bool NeedsStation(EStructure Structure);

	/** The flagship leads the main fleet; the rest by size: XXS/XS scouts, S science, M construction, L and up the line. */
	APS_ALPHA_API EDivision DefaultDivision(ESpaceshipSizeClass SizeClass, bool bFlagship);
	/** Move and Return suit every division; a survey needs exploration or science, building construction. */
	APS_ALPHA_API bool DivisionCan(EDivision Division, EOrder Order);
	/** What a survey by this division leaves: exploration surveys, science studies (and surveys on the way). */
	APS_ALPHA_API ESurvey SurveyBy(EDivision Division);
	/** The next division when the player cycles a ship's assignment. */
	APS_ALPHA_API EDivision NextDivision(EDivision Division);

	/**
	 * Autopilot speed for one step (cm/s): an exponential approach to the target (0.8 of the remaining distance per
	 * second), no faster than the class allows or than one surface distance per second near a body (the ORBITAL band's
	 * rule), and gaining at most 2.5x per second (plus 50 m/s^2 from rest) so a departure never jumps.
	 */
	APS_ALPHA_API double StepSpeed(double CurrentSpeed, double DistanceCm, double SurfaceDistanceCm, double ClassCapCm,
		double DeltaSeconds);
	/** Class speed cap: ships without SpaceWrap (XXS, XS) stay at planetary speeds and only fly near their own planet. */
	APS_ALPHA_API double ClassCap(const FSpaceshipClassPreset& Preset);
	/** Seconds of work at the target, shortened by the division's level (each level 20%) and by a station there (25%). */
	APS_ALPHA_API double WorkSeconds(EOrder Order, EDivision Division, int32 DivisionLevel, bool bStationThere = false);
	/** Orbit slot distance from a body's centre for a body radius (cm). */
	APS_ALPHA_API double SlotRadius(double BodyRadiusCm);
	/**
	 * A detour point when the straight way from Start to End passes through a sphere (Centre, Radius): the closest point
	 * of the segment pushed out to 1.35 radii. False when the segment clears 1.1 radii, comes closest only at its end
	 * (an approach from above, even to a berth just over the surface) or ends inside the body.
	 */
	APS_ALPHA_API bool Detour(const FVector& Start, const FVector& End, const FVector& Centre, double Radius, FVector& OutPoint);
	/**
	 * The next point on the way round a body that lies across the way to Slot (01.10: ships bound for a moon behind the
	 * planet flew into it toward a detour point and crawled at the surface speed limit). Lower than 1.3 radii: straight
	 * out to 1.4. Higher: 30 degrees along the sphere toward the slot, at least 1.35 radii out, so the chord to it stays
	 * outside 1.3 radii.
	 */
	APS_ALPHA_API FVector RoundBody(const FVector& Ship, const FVector& Slot, const FVector& Centre, double Radius);
}

struct APS_ALPHA_API FAPSFleetUnit
{
	TWeakObjectPtr<ASpaceship> Ship;
	/** Short and stable for the session: FLAGSHIP, or the size class and the order of registration (M-04). */
	FString CallSign;
	APSFleet::EDivision Division{APSFleet::EDivision::MainFleet};
	bool bFlagship{false};

	APSFleet::EOrder Order{APSFleet::EOrder::None};
	APSFleet::EPhase Phase{APSFleet::EPhase::Idle};
	TWeakObjectPtr<AActor> Target;
	/** The slot at the target: a direction from its centre (body-relative, so it follows the body and origin shifts). */
	FVector SlotDirection{FVector::UpVector};
	/** The autopilot's heading, eased so a new waypoint turns the ship instead of snapping it. */
	FVector Heading{FVector::ZeroVector};
	/** Work progress, 0..1, and its length in seconds. */
	float Progress{0.0f};
	float WorkLength{0.0f};
	/** Where the ship came from (its berth), for Return: the parent it was attached to and the transform relative to it. */
	TWeakObjectPtr<AActor> Berth;
	FTransform BerthRelative{FTransform::Identity};
	bool bHasBerth{false};
	/** The berth was the colony's landing pad: the ship parks there again on return. */
	bool bBerthParked{false};
	/** Departure: the point to clear first, relative to the body it leaves. */
	TWeakObjectPtr<AActor> DepartFrom;
	FVector DepartOffset{FVector::ZeroVector};
	double Speed{0.0};
	/** Distance left to the target slot, for the list (cm). */
	double RemainingCm{0.0};
	/** Rio 04.10: the distance when the flight there began (this order's largest RemainingCm), for the screens' progress. */
	double TransitStartCm{0.0};
	/**
	 * Rio 04.10 ("whatever the units do, the pilot does from the bridge of a ship that can"): the order is the pilot's own:
	 * no autopilot, no slot; the work runs while the ship stays within reach of the target, then the unit is free again.
	 */
	bool bPilotWork{false};
	/** BuildStructure: the infrastructure catalogue type being raised (its cost was taken when the order was given). */
	FName StructureType;
};

/** A unit as saved (APSCivilizationSave), matched by its call sign on load. Actors are named by key (KeyOf). */
struct APS_ALPHA_API FAPSFleetUnitRecord
{
	FString CallSign;
	uint8 Division{0};
	uint8 Order{0};
	uint8 Phase{0};
	FString TargetKey;
	FVector SlotDirection{FVector::UpVector};
	float Progress{0.0f};
	float WorkLength{0.0f};
	bool bHasBerth{false};
	bool bBerthParked{false};
	FString BerthKey;
	FTransform BerthRelative{FTransform::Identity};
	/** Where the ship stood, relative to the actor ReferenceKey names (its target, or the nearest body). */
	FString ReferenceKey;
	FTransform RelativeTransform{FTransform::Identity};
	/** The pilot was aboard: the player's own record puts that ship back, not this one. */
	bool bPiloted{false};
	/** Built by the shipyard: its class, so a load spawns it again (the generator only respawns its own fleet). */
	FString SpawnClassPath;
};

struct APS_ALPHA_API FAPSFleetSaveData
{
	TArray<FAPSFleetUnitRecord> Units;
	/** Body key and survey level. */
	TArray<TPair<FString, uint8>> Surveys;
	/** Outposts the fleet built: body key, transform relative to the body, name. */
	struct FOutpost
	{
		FString BodyKey;
		FTransform RelativeTransform{FTransform::Identity};
		FString Name;
	};
	TArray<FOutpost> Outposts;

	/** Stations, shipyards and sector HQs the fleet built; respawned under their saved actor names (stable keys). */
	struct FStructure
	{
		uint8 Kind{0};
		FString BodyKey;
		FTransform RelativeTransform{FTransform::Identity};
		FString Name;
		FString ActorName;
	};
	TArray<FStructure> Structures;
	/** The slipways' queues, the ship on the slipway with its progress. */
	struct FShipyardJob
	{
		FString ClassPath;
		uint8 SizeClass{0};
		FString Name;
		float Length{1.0f};
		float Progress{0.0f};
		FString YardKey;
	};
	TArray<FShipyardJob> ShipyardJobs;
	/** Investigated anomalies: body key, 1 by an expedition, 2 by the pilot in person. */
	TArray<TPair<FString, uint8>> Investigations;
	/** BuildStructure orders under way: call sign and catalogue type (saved in the civilization's version 5 block). */
	TArray<TPair<FString, FString>> UnitStructureTypes;

	friend FArchive& operator<<(FArchive& Ar, FAPSFleetSaveData& Data);
	/** Structures and ShipyardJobs: civilization save version 4, appended after the older blocks so those still load. */
	static void SerializeExtras(FArchive& Ar, FAPSFleetSaveData& Data);
};

/** A ship the shipyard can build: an entry of the civilization's ship catalogue. */
struct APS_ALPHA_API FAPSShipyardOption
{
	TSubclassOf<ASpaceship> ShipClass;
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};
	FText Name;
	float BuildSeconds{0.0f};
	/** Rio 07-09.10, ORIGIN ladder (T-03): why it cannot be laid down now (empty: it can), and what it costs the stocks. */
	FText Refusal;
	FText CostText;
};

/** A ship on a shipyard's slipway or waiting for it. Every shipyard builds its first job at the same time. */
struct APS_ALPHA_API FAPSShipyardJob
{
	TSubclassOf<ASpaceship> ShipClass;
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};
	FText Name;
	float Length{1.0f};
	float Progress{0.0f};
	TWeakObjectPtr<ASpaceShipyard> Yard;
};

/** A station, shipyard or headquarters in orbit of a planet or moon, generated or built by the fleet. */
struct APS_ALPHA_API FAPSFleetStructure
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<APlanetaryBody> Body;
	APSFleet::EStructure Kind{APSFleet::EStructure::Station};
	/** Raised by the fleet's construction ships (saved and respawned), not part of the generated home complex. */
	bool bBuilt{false};
};

/** A planet or moon the civilization has surveyed or built at. */
struct APS_ALPHA_API FAPSFleetBodyRecord
{
	TWeakObjectPtr<APlanetaryBody> Body;
	APSFleet::ESurvey Survey{APSFleet::ESurvey::Unknown};
	/** Lines revealed so far (survey, then study), for the map and the journal. */
	TArray<FText> Findings;
	TArray<TWeakObjectPtr<AActor>> Outposts;
	/** The world's anomaly, if it hides one (APSFleet::RollAnomaly), and how far the civilization got with it. */
	bool bHasAnomaly{false};
	int32 AnomalyKind{0};
	FVector AnomalyDirection{FVector::UpVector};
	APSFleet::EAnomalyState Anomaly{APSFleet::EAnomalyState::Hidden};
	bool bAnomalyInPerson{false};
	/** A navigation beacon at a located site (the map lists it, the ship can set a course to it). */
	TWeakObjectPtr<AActor> AnomalyBeacon;
	bool bBeaconGrounded{false};
	/**
	 * Rio 07.10 (the worlds of other star systems were Unknown again after a revisit, with a second record each): set while
	 * the world is away (aps.Stars.HoldReleasedStructures): its fleet key, and the star system that released it (invalid
	 * for one a load could not find). Such a record is saved by its key and taken up again when a world of that key stands
	 * (of that system). Runtime only.
	 */
	FString Key;
	FGuid HostSystemId;
	/** Rio 07.10: kept from a save for a world away at load: its investigation (1 by an expedition, 2 in person). */
	uint8 HeldInvestigation{0};
};

class APS_ALPHA_API FAPSFleetCommand
{
public:
	explicit FAPSFleetCommand(UWorld* InWorld);
	~FAPSFleetCommand();

	void Tick(float DeltaSeconds);

	const TArray<FAPSFleetUnit>& GetUnits() const { return Units; }
	const FAPSFleetUnit* FindUnit(const ASpaceship* Ship) const;
	void SetDivision(const ASpaceship* Ship, APSFleet::EDivision Division);

	/** Why the ship cannot take this order at this target; empty when it can. */
	FText CheckOrder(const ASpaceship* Ship, APSFleet::EOrder Order, const AActor* Target, bool bByPilot = false) const;
	/**
	 * Gives the order to every ship that can take it; returns how many did and the first refusal. BuildStructure takes
	 * the catalogue type and goes to the first ship that can raise it (its cost is taken then).
	 */
	int32 IssueOrder(const TArray<ASpaceship*>& Ships, APSFleet::EOrder Order, AActor* Target, FText& OutRefusal,
		FName StructureType = NAME_None);
	/** Why this ship cannot raise this catalogue type at this place now; empty when it can. */
	FText CheckBuildOrder(const ASpaceship* Ship, const AActor* Target, FName StructureType, bool bByPilot = false) const;
	/**
	 * Rio 04.10: the piloted ship does an order itself (a survey, a study, a probe, a system survey, an outpost or any
	 * build its division can), by the same rules as a fleet order but flown by the pilot: it must be within reach of the
	 * target (PilotWorkRange) and the work goes on only while it stays there. Empty when given, else why not.
	 */
	FText IssuePilotOrder(ASpaceship* Ship, APSFleet::EOrder Order, AActor* Target, FName StructureType = NAME_None);
	/** Why the pilot cannot do it now (empty: can), without giving it. */
	FText CheckPilotOrder(const ASpaceship* Ship, APSFleet::EOrder Order, const AActor* Target, FName StructureType = NAME_None) const;
	/**
	 * How far the ship is from the target and how near the pilot's own work needs it, cm: a survey reads a world from a
	 * near approach, building and landings need a near orbit, a star system's orders its room among the neighbours.
	 */
	bool PilotWorkRange(const ASpaceship* Ship, APSFleet::EOrder Order, const AActor* Target, double& OutDistanceCm,
		double& OutRangeCm) const;
	/** The idle ship of the civilization nearest the target that can take the order (null: none can); the reason if none. */
	ASpaceship* PickShipFor(APSFleet::EOrder Order, const AActor* Target, FName StructureType, FText& OutRefusal) const;
	void CancelOrder(const ASpaceship* Ship);

	APSFleet::ESurvey GetSurvey(const AActor* Body) const;
	const FAPSFleetBodyRecord* FindBody(const AActor* Body) const;
	int32 CountOutposts(const AActor* Body) const;
	const TArray<FAPSFleetBodyRecord>& GetBodies() const { return Bodies; }

	/** Changes on every unit, order, survey or outpost change, so the menu rebuilds only then. */
	uint32 GetRevision() const { return Revision; }
	/** One line for the list: the order, its phase and progress or distance. */
	FText DescribeState(const FAPSFleetUnit& Unit) const;
	/**
	 * Rio 05.10 (star map): about how many seconds the ship's autopilot needs to the target's slot (the flight, not the work
	 * there), by StepSpeed's profile at its class cap: from its order's state when it already flies there, else from rest
	 * where it is. Negative when it cannot fly there (not a unit of the fleet; no SpaceWrap for another star or planet).
	 */
	double EstimateArrivalSeconds(const ASpaceship* Ship, const AActor* Target) const;

	/** Console: aps.Fleet.* (list, order, speed scale). */
	void LogUnits() const;
	int32 ConsoleOrder(const FString& Who, APSFleet::EOrder Order, const FString& TargetName, FText& OutRefusal);

	/** Saves: every unit (where it is, its division and order), the surveys and the outposts the fleet built. */
	void CaptureSave(FAPSFleetSaveData& OutData) const;
	/** Loads: applied once the saved units are back (the fleet respawns with the generated hierarchy), or after 20 s. */
	void SetPendingRestore(FAPSFleetSaveData&& Data);
	/** A stable name for an actor across sessions: a body's catalogue name, else the actor's name. */
	static FString KeyOf(const AActor* Actor);

	/**
	 * Shipyard (Rio, 01.10: "ships cannot be built in the game yet"): the catalogue's ships, built one after another on
	 * a shipyard's slipway; each launches above it and joins the fleet as a new unit. Every shipyard of the civilization
	 * (the home one and those its construction ships built) has its own slipway and builds at the same time as the
	 * others. The time falls with the Industry level.
	 */
	void GetShipyardOptions(TArray<FAPSShipyardOption>& OutOptions) const;
	/** Queues one ship at this shipyard (null: the home one); the refusal when it cannot (no shipyard, a full slipway). */
	FText OrderShip(const FAPSShipyardOption& Option, ASpaceShipyard* Yard = nullptr);
	const TArray<FAPSShipyardJob>& GetShipyardQueue() const { return ShipyardQueue; }
	int32 CountQueued(const ASpaceShipyard* Yard) const;
	int32 GetLaunchedCount() const { return LaunchedCount; }
	int32 GetLaunchedCount(const ASpaceShipyard* Yard) const;
	/** The civilization's home shipyard, else any. */
	ASpaceShipyard* FindShipyard() const;
	/** Every shipyard of the civilization, the home one first (any in the world when none is the civilization's). */
	void GetShipyards(TArray<ASpaceShipyard*>& OutYards) const;
	static constexpr int32 ShipyardQueueLimit = 6;

	/** Stations, shipyards and HQs in orbit, refreshed once a second and on every build. */
	const TArray<FAPSFleetStructure>& GetStructures() const { return Structures; }
	int32 CountStructures(const AActor* Body, APSFleet::EStructure Kind) const;
	/** Sector HQs the fleet built: each speeds the whole fleet up by 10%. */
	int32 CountBuiltHeadquarters() const;
	/** The planet or moon an orbital structure belongs to, through its attach parents. */
	static APlanetaryBody* OrbitedBody(const AActor* Actor);

	/**
	 * Rio 07.10 ("buildings on planets of other star systems disappear when that system goes away"): the materializer
	 * releases a foreign star system (FAPSSystemMaterializer::Finish, before its actors go). The fleet's ships at these
	 * worlds move to the system's beacon (their orders end as for any lost target, a build's cost returned); its stations,
	 * shipyards, HQs and outposts there and the worlds' records are held (saved, still counted) and stand again when the
	 * system does. aps.Stars.HoldReleasedStructures 0: nothing. Rio 07.10 (review: a ship at the star itself went with the
	 * tree): SystemRoot is the system's actor tree; every ship attached anywhere in it moves too, also when no world stood.
	 */
	void HoldOnRelease(const TArray<APlanetaryBody*>& ReleasedBodies, const FGuid& HostSystemId, FAPSStarSystems& Stars,
		const AActor* SystemRoot);

	/**
	 * Divisions grow with their work (Rio, 01.10: "the divisions should change the game, and the menu should say how"):
	 * exploration one level per three worlds surveyed, science one per two studied, industry (the construction ships)
	 * one per three outposts and structures built, fleet command one per four ships launched from the slipways; at most
	 * three each. Counted from the records a load restores, so nothing extra is saved.
	 */
	struct FWorkTally
	{
		int32 Surveyed{0};
		int32 Studied{0};
		int32 Built{0};
		int32 Launched{0};
		/** Anomalies investigated: an expedition counts once, the pilot in person twice (science grows with both). */
		int32 Investigated{0};
	};
	FWorkTally GetWorkTally() const;
	int32 GetEarnedLevel(APSFleet::EDivision Division) const;
	int32 GetEarnedFleetCommandLevel() const;
	/** The civilization's level plus what the work earned: what the rules use. */
	int32 GetDivisionLevel(APSFleet::EDivision Division) const;
	/** Every ship's autopilot speed factor (fleet command and sector HQs), and the main fleet's extra (military). */
	double GetSpeedScale() const { return SpeedScale(); }
	double GetDivisionSpeedFactor(APSFleet::EDivision Division) const;
	/** The shipyards' speed factor (industry). */
	float GetShipyardRate() const;
	/** A body's catalogue name, an actor's in-game name, else its class, upper case (as the journal names them). */
	static FText DisplayName(const AActor* Actor);
	/** The world record whose anomaly beacon this is, or null (the map names beacons by it). */
	const FAPSFleetBodyRecord* FindAnomalyByBeacon(const AActor* Beacon) const;
	/** One line on the world's anomaly for the order panel; empty when none is known. */
	FText DescribeAnomaly(const AActor* Body) const;
	/** Console and tests (aps.Fleet.Anomalies): every planet's and moon's anomaly, state and site in the log. */
	void LogAnomalies();

private:
	void TickShipyard(float DeltaSeconds);
	ASpaceship* LaunchShip(TSubclassOf<ASpaceship> ShipClass, const FTransform& Transform, ASpaceShipyard* Yard);
	/** bCountCivilization false (Rio 07.10): one held while its world was away stands again; the civilization counted it. */
	class ASpaceStation* SpawnStructure(APSFleet::EStructure Kind, APlanetaryBody* Body, const FTransform& Transform,
		const FText& Name, const FString& ActorName, bool bCountCivilization = true);
	void RefreshStructures();
	/**
	 * Rio 07.10: once a second, while anything is held: world records taken up again by the world of their key, held
	 * structures and outposts raised there (a few a time). A released system's only on its own bodies while it stands.
	 */
	void RaiseHeldWorlds();
	/** Rio 07.10: a world record whose world was away belongs to this body of its key again. */
	void RebindBody(FAPSFleetBodyRecord& Record, APlanetaryBody* Body);
	/** Anomalies (APSFleetAnomalies.cpp): rolled once per world record, revealed with the survey level, investigated by
	 * an expedition or the pilot on foot at the site. */
	void RollAnomaly(FAPSFleetBodyRecord& Record, const APlanetaryBody* Body) const;
	void RevealAnomaly(FAPSFleetBodyRecord& Record, APSFleet::ESurvey Level, const FText& By, bool bAnnounce);
	void InvestigateAnomaly(FAPSFleetBodyRecord& Record, const FText& By, bool bInPerson, bool bAnnounce);
	void SpawnAnomalyBeacon(FAPSFleetBodyRecord& Record);
	/** Once a second: beacons settle on the ground once the surface near them has collision; a pilot on foot at a
	 * located site investigates it. */
	void TickAnomalies();
	static FText AnomalySiteText(const FVector& Direction);
	void ApplyPendingRestore();
	AActor* FindByKey(const FString& Key) const;
	class AAutonomousOutpost* SpawnOutpost(APlanetaryBody* Body, const FVector& Location, const FQuat& Rotation,
		const FText& Name, bool bCountCivilization = true);
	FAPSFleetUnit* FindUnitMutable(const ASpaceship* Ship);
	void RefreshUnits();
	void TickUnit(FAPSFleetUnit& Unit, float DeltaSeconds);
	/** One autopilot step toward Goal; BeyondGoalCm is the way left after it (a detour point is not the end). */
	void Fly(FAPSFleetUnit& Unit, ASpaceship* Ship, const FVector& Goal, double BeyondGoalCm, float DeltaSeconds,
		double& OutRemainingCm);
	void Arrive(FAPSFleetUnit& Unit, ASpaceship* Ship);
	/** The work at the target begins (its length, bonuses and the journal line); Arrive's second half. */
	void BeginWork(FAPSFleetUnit& Unit);
	/** The pilot's own work: it advances while the ship stays within reach, ends when the pilot leaves or flies off. */
	void TickPilotWork(FAPSFleetUnit& Unit, float DeltaSeconds);
	void FinishWork(FAPSFleetUnit& Unit);
	FVector SlotLocation(const FAPSFleetUnit& Unit) const;
	/** The nearest planet, moon or star to a point and the distance to its surface (cm). */
	AActor* NearestBody(const FVector& Location, double& OutSurfaceDistanceCm, double& OutRadiusCm) const;
	/** The planet whose neighbourhood a point is in (moons and stations count for their planet). */
	APlanetaryBody* HomeBodyOf(const FVector& Location) const;
	FAPSFleetBodyRecord& BodyRecord(APlanetaryBody* Body);
	UCivilization* Civilization() const;
	int32 DivisionLevel(APSFleet::EDivision Division) const;
	double SpeedScale() const;
	void Post(const FText& Text) const;
	FText UnitName(const FAPSFleetUnit& Unit) const;

	TWeakObjectPtr<UWorld> World;
	TArray<FAPSFleetUnit> Units;
	TArray<FAPSFleetBodyRecord> Bodies;
	double RefreshSeconds{0.0};
	int32 RegisteredCount{0};
	uint32 Revision{1};
	bool bHomeKnown{false};
	TArray<FAPSShipyardJob> ShipyardQueue;
	/** Ships launched in this world, and per shipyard for their slots above it. */
	int32 LaunchedCount{0};
	TMap<TWeakObjectPtr<ASpaceShipyard>, int32> LaunchedAt;
	TArray<FAPSFleetStructure> Structures;
	/**
	 * Rio 07.10: the stations, shipyards, HQs and outposts the fleet built on worlds that are away, as a save writes them,
	 * with the star system that released them (invalid: held at load). Saved with the standing ones; counted for the
	 * divisions' levels and the HQs' speed.
	 */
	TArray<TPair<FGuid, FAPSFleetSaveData::FStructure>> HeldStructures;
	TArray<TPair<FGuid, FAPSFleetSaveData::FOutpost>> HeldOutposts;
	/** Numbers the fleet's structures' actor names, so a load gives them the same keys. */
	int32 StructureSerial{0};
	/** Earned levels last seen (exploration, science, industry, fleet command), for the journal's promotions; primed
	 * silently after a load. */
	int32 AnnouncedEarned[4]{0, 0, 0, 0};
	bool bEarnedPrimed{false};
	void AnnouncePromotions();
	TOptional<FAPSFleetSaveData> PendingRestore;
	double PendingRestoreSince{-1.0};
};

/** The world's fleet command, or null outside a generated game (UAPSFleetCommandSubsystem registers it). */
APS_ALPHA_API FAPSFleetCommand* APSFleetFind(const UWorld* World);
/** The owning subsystem registers its fleet command for its world, and null when it goes. */
APS_ALPHA_API void APSFleetRegister(const UWorld* World, FAPSFleetCommand* Fleet);
