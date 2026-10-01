#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"

class AActor;
class APlanetaryBody;
class ASpaceShipyard;
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
		Return
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

	/** The flagship leads the main fleet; the rest by size: XXS/XS scouts, S science, M construction, L and up the line. */
	APS_ALPHA_API EDivision DefaultDivision(ESpaceshipSizeClass SizeClass, bool bFlagship);
	/** Move and Return suit every division; a survey needs exploration or science, an outpost construction. */
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
	/** Seconds of work at the target, shortened by the division's level (each level 20%). */
	APS_ALPHA_API double WorkSeconds(EOrder Order, EDivision Division, int32 DivisionLevel);
	/** Orbit slot distance from a body's centre for a body radius (cm). */
	APS_ALPHA_API double SlotRadius(double BodyRadiusCm);
	/**
	 * A detour point when the straight way from Start to End passes through a sphere (Centre, Radius): the closest point
	 * of the segment pushed out to 1.35 radii. False when the segment clears 1.1 radii, comes closest only at its end
	 * (an approach from above, even to a berth just over the surface) or ends inside the body.
	 */
	APS_ALPHA_API bool Detour(const FVector& Start, const FVector& End, const FVector& Centre, double Radius, FVector& OutPoint);
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

	friend FArchive& operator<<(FArchive& Ar, FAPSFleetSaveData& Data);
};

/** A ship the shipyard can build: an entry of the civilization's ship catalogue. */
struct APS_ALPHA_API FAPSShipyardOption
{
	TSubclassOf<ASpaceship> ShipClass;
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};
	FText Name;
	float BuildSeconds{0.0f};
};

/** A ship on the slipway or waiting for it. */
struct APS_ALPHA_API FAPSShipyardJob
{
	TSubclassOf<ASpaceship> ShipClass;
	ESpaceshipSizeClass SizeClass{ESpaceshipSizeClass::M};
	FText Name;
	float Length{1.0f};
	float Progress{0.0f};
};

/** A planet or moon the civilization has surveyed or built at. */
struct APS_ALPHA_API FAPSFleetBodyRecord
{
	TWeakObjectPtr<APlanetaryBody> Body;
	APSFleet::ESurvey Survey{APSFleet::ESurvey::Unknown};
	/** Lines revealed so far (survey, then study), for the map and the journal. */
	TArray<FText> Findings;
	TArray<TWeakObjectPtr<AActor>> Outposts;
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
	FText CheckOrder(const ASpaceship* Ship, APSFleet::EOrder Order, const AActor* Target) const;
	/** Gives the order to every ship that can take it; returns how many did and the first refusal. */
	int32 IssueOrder(const TArray<ASpaceship*>& Ships, APSFleet::EOrder Order, AActor* Target, FText& OutRefusal);
	void CancelOrder(const ASpaceship* Ship);

	APSFleet::ESurvey GetSurvey(const AActor* Body) const;
	const FAPSFleetBodyRecord* FindBody(const AActor* Body) const;
	int32 CountOutposts(const AActor* Body) const;
	const TArray<FAPSFleetBodyRecord>& GetBodies() const { return Bodies; }

	/** Changes on every unit, order, survey or outpost change, so the menu rebuilds only then. */
	uint32 GetRevision() const { return Revision; }
	/** One line for the list: the order, its phase and progress or distance. */
	FText DescribeState(const FAPSFleetUnit& Unit) const;

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
	 * Shipyard (Rio, 01.10: "ships cannot be built in the game yet"): the catalogue's ships, built one after another at
	 * the civilization's shipyard; each launches above it and joins the fleet as a new unit. The time falls with the
	 * Industry level.
	 */
	void GetShipyardOptions(TArray<FAPSShipyardOption>& OutOptions) const;
	/** Queues one ship; the refusal when it cannot (no shipyard, a full slipway). */
	FText OrderShip(const FAPSShipyardOption& Option);
	const TArray<FAPSShipyardJob>& GetShipyardQueue() const { return ShipyardQueue; }
	int32 GetLaunchedCount() const { return LaunchedCount; }
	ASpaceShipyard* FindShipyard() const;
	static constexpr int32 ShipyardQueueLimit = 6;

private:
	void TickShipyard(float DeltaSeconds);
	ASpaceship* LaunchShip(TSubclassOf<ASpaceship> ShipClass, const FTransform& Transform);
	void ApplyPendingRestore();
	AActor* FindByKey(const FString& Key) const;
	class AAutonomousOutpost* SpawnOutpost(APlanetaryBody* Body, const FVector& Location, const FQuat& Rotation,
		const FText& Name);
	FAPSFleetUnit* FindUnitMutable(const ASpaceship* Ship);
	void RefreshUnits();
	void TickUnit(FAPSFleetUnit& Unit, float DeltaSeconds);
	/** One autopilot step toward Goal; BeyondGoalCm is the way left after it (a detour point is not the end). */
	void Fly(FAPSFleetUnit& Unit, ASpaceship* Ship, const FVector& Goal, double BeyondGoalCm, float DeltaSeconds,
		double& OutRemainingCm);
	void Arrive(FAPSFleetUnit& Unit, ASpaceship* Ship);
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
	/** Ships launched in this world, for their slots above the shipyard. */
	int32 LaunchedCount{0};
	TOptional<FAPSFleetSaveData> PendingRestore;
	double PendingRestoreSince{-1.0};
};

/** The world's fleet command, or null outside a generated game (UAPSFleetCommandSubsystem registers it). */
APS_ALPHA_API FAPSFleetCommand* APSFleetFind(const UWorld* World);
/** The owning subsystem registers its fleet command for its world, and null when it goes. */
APS_ALPHA_API void APSFleetRegister(const UWorld* World, FAPSFleetCommand* Fleet);
