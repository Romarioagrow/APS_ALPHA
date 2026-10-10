#pragma once

#include "CoreMinimal.h"

class AActor;
class ASpaceship;
class AStar;
class UWorld;

/**
 * Rio 09.10 (GAME_CONCEPT_ORIGIN section 2.2, section 7 items 16-19; ORIGIN_TASK_CARDS T-20): SPACE TRIPS, the third mode.
 * A ship, a pilot and the galaxy: excursion routes to the beautiful places, a guide that flies the ship from one to the
 * next, a guidebook in the journal, a photo mode. No goals, no economy, no grind: the world's rules say Goals = None
 * (APSWorldRules::IsTrip) and this runs; every other world leaves it asleep, with the console still able to call it.
 *
 * Routes come from the generated world itself: the home system's worlds, the nearest suns, red and cold stars, blue and
 * white ones, doubles and triples, the dark remnants, the crowded skies. A route is a list of stops; a stop is a star
 * system of the catalogue (FAPSStarSystems), reached with the star drive, then its worlds one by one with the ship's own
 * autopilot (UAPSShipFlightModel::EngageAutopilot), a hold at each for the view. Nothing in Pawns/Spaceships is changed:
 * the guide turns the nose with the star drive on (as the autopilot does with it off) and otherwise uses the public helm.
 *
 * Plain C++, one object per world (APSSpaceTrips::Find), ticked by the fleet command subsystem like the ground vehicles.
 */
namespace APSSpaceTrips
{
	/** One place a route visits: a star system of the catalogue; its worlds are found when the system stands. */
	struct FStop
	{
		int32 SystemIndex{INDEX_NONE};
		/** The system's name as the catalogue has it (for the guide's lines before it stands). */
		FString Name;
		/** How many of its worlds to show (0: all). */
		int32 WorldsToShow{0};
	};

	struct FRoute
	{
		FString Id;
		FText Name;
		FText Subtitle;
		TArray<FStop> Stops;
	};

	enum class EPhase : uint8
	{
		Idle,
		/** Waiting for the pilot aboard a ship with the engine on. */
		Waiting,
		/** Star drive toward the stop's star; the guide holds the nose on it. */
		StarLeg,
		/** Arrived at the system: waiting for it to stand (the materializer) and listing its worlds. */
		Arriving,
		/** The ship's autopilot flying to a world. */
		WorldLeg,
		/** Standing near a world for the view. */
		Hold,
		/** The pilot took the helm: the route waits for aps.Trip.Resume. */
		Paused,
		Done
	};

	class APS_ALPHA_API FTrip
	{
	public:
		explicit FTrip(UWorld* InWorld);

		void Tick(float DeltaSeconds);

		/** The routes this world offers now (built from the catalogue; most need the catalogue read first). */
		void BuildRoutes(TArray<FRoute>& OutRoutes) const;
		/** Starts a route by id or by its number in BuildRoutes' list (0-based); false and a journal line when it cannot. */
		bool Start(const FString& IdOrIndex);
		/** Skips to the next world, or the next stop when the worlds are done. */
		void Next();
		void Pause(const FText& Why);
		void Resume();
		void Stop(const FText& Why);
		/** A still of the view without the HUD into Saved/Screenshots/Trips (the journal names the file). */
		void Photo(const FString& Name);

		EPhase GetPhase() const { return Phase; }
		const FRoute& GetRoute() const { return Route; }
		int32 GetStopIndex() const { return StopIndex; }
		/** One line for the log: the route, the stop, what the guide is doing. */
		FString Describe() const;

	private:
		ASpaceship* PilotShip() const;
		void BeginStop(ASpaceship& Ship);
		void BeginStarLeg(ASpaceship& Ship);
		void TickStarLeg(ASpaceship& Ship, float DeltaSeconds);
		void TickArriving(ASpaceship& Ship);
		void ListWorlds(ASpaceship& Ship);
		void BeginWorldLeg(ASpaceship& Ship);
		void TickWorldLeg(ASpaceship& Ship);
		void NextWorld(ASpaceship& Ship);
		void Guide(const FText& Text) const;
		/** The guidebook's lines for a star and for a world. */
		FText DescribeStar(const AStar* Star, const FStop& Stop) const;
		FText DescribeWorld(const ASpaceship& Ship, const AActor* Body) const;
		FText NameOf(const ASpaceship& Ship, const AActor* Body) const;

		TWeakObjectPtr<UWorld> World;
		FRoute Route;
		EPhase Phase{EPhase::Idle};
		EPhase PausedFrom{EPhase::Idle};
		int32 StopIndex{INDEX_NONE};
		TArray<TWeakObjectPtr<AActor>> Worlds;
		int32 WorldIndex{INDEX_NONE};
		TWeakObjectPtr<AActor> Target;
		float PhaseSeconds{0.0f};
		float HoldSeconds{0.0f};
		/** StarLeg: the drive was on at least once (its own arrival switches it off), and how often it was asked for. */
		bool bDriveWasOn{false};
		int32 EngageAttempts{0};
		float NextEngageSeconds{0.0f};
		/** StarLeg: the rotation the guide set last tick (a larger turn since is the pilot's hand). */
		FQuat LastSetRotation{FQuat::Identity};
		bool bRotated{false};
		bool bToldToBoard{false};
		bool bAutoStarted{false};
		bool bWelcomed{false};
		int32 PhotoSerial{0};
	};

	/** The world's trip object (made on first use when bCreate; forgotten with the world). */
	APS_ALPHA_API FTrip* Find(const UWorld* World, bool bCreate = false);
	/** Once a frame from the fleet command subsystem: runs the trip of a SPACE TRIPS world (and any started by console). */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);
}
