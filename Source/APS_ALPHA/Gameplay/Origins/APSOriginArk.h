#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
class APlanetaryBody;
class ASpaceship;
class UGeneratedWorld;
class USpawnParameters;
class UWorld;

/**
 * Rio 18.10 (0.6.6, the origins branch: "the ARK first; the temporary ark is an L ship with an interior; SANDBOX stays as
 * it is"): the ARK origin's honest start and its ark.
 *  - The start kit: an ORIGIN world takes its own start, not the menu's last numbers (a CORE preset gave ARK ten ships and
 *    five stations): one hull, no escorts, outposts, settlements or parked vehicles; ARK's hull is an L ship (aps.Origin.ArkShip).
 *  - The ark: no headquarters, station or orbital yard are built (AAstroGenerator); the colony lands the hull on the ground
 *    beside an invisible site anchor for the modules, the pilot starts aboard in the dark, the drive is dead for good (no
 *    seat, no fleet orders), and F at the ark powers it: the ship's own lights come on in a wave from the pilot outward.
 *  - The story's camera moments (the walker's view turns to a target and holds), and the monument's ring around the ark.
 * Everything here runs only in an ARK world (IsArkWorld); SANDBOX, SPACE TRIPS and the other origins never see it.
 */
namespace APSOriginArk
{
	/** The ark's tag (the home ship of an ARK world). */
	APS_ALPHA_API FName Tag();
	/** The mission board's token for the powered ark (saved with the board). */
	APS_ALPHA_API FName PowerToken();

	/** An ORIGIN world with the honest start on (aps.Origin.HonestStart): the ladder applies and the start is the origin's. */
	APS_ALPHA_API bool IsHonestStart(const UWorld* World);
	/** The running world plays the ARK origin's honest start. */
	APS_ALPHA_API bool IsArkWorld(const UWorld* World);
	/** Rio 19.10 (SPACE TRIPS, "a clean sky"): a trip world founded with the trip's kit (the ship alone, no colony in sight). */
	APS_ALPHA_API bool IsTripKitWorld(const UWorld* World);
	/** The start builds no home complex: the ARK (the ark lands on the ground) or a trip (the ship waits in orbit). */
	APS_ALPHA_API bool IsHullOnlyStart(const UWorld* World);
	/** Before travel (the menu's commit, on the game's copy): the origin's start kit; SANDBOX and trips untouched. */
	APS_ALPHA_API void ApplyStartKit(const UGeneratedWorld& World, USpawnParameters& Spawn);
	/** Back in the menu: the kit's numbers go back to the defaults (the next world is not founded with them). */
	APS_ALPHA_API void ClearStartKit(USpawnParameters& Spawn);

	APS_ALPHA_API bool IsArk(const AActor* Actor);
	APS_ALPHA_API ASpaceship* FindArk(const UWorld* World);
	APS_ALPHA_API bool IsPowered(const UWorld* World);

	/** The colony parked the home ship (a new start or a load): it becomes the ark, dark until powered. */
	APS_ALPHA_API void AdoptArk(ASpaceship& Ship);
	/** Places the ark's ramp foot on the ground (the ramp deploys only on ground within 45 cm), moving the hull along its up. */
	APS_ALPHA_API void SettleRampOnGround(ASpaceship& Ship, const APlanetaryBody& Body);
	/** Where a new ARK start puts the pilot: aboard, on a deck of the ark; false while no deck answers yet. */
	APS_ALPHA_API bool PlacePilotAboard(ASpaceship& Ship, APawn& Pilot);
	/** The pawn stands outside the ark's hull box by MarginCm or more (on foot). */
	APS_ALPHA_API bool IsOutside(const UWorld* World, const APawn* Pawn, double MarginCm);

	/**
	 * F at the ark: the action words for the prompt; empty when the ark offers nothing here. Dark: POWER THE ARK (aboard);
	 * powered: the ark's lift, down to the ground beside its exit (aboard) or up to the bridge (outside, at the exit).
	 */
	APS_ALPHA_API FText ActionFor(const APawn* Pawn, const AActor* Candidate);
	/** The prompt's second line for the ark ("main bus down", "lift"). */
	APS_ALPHA_API FText SubjectFor(const UWorld* World);
	/** The ark when the walker stands outside at its exit (the lift's door), for the F prompt; null otherwise. */
	APS_ALPHA_API AActor* ExitCandidate(const APawn* Pawn);
	/** F at the ark: true when the ark took the key (powered up). */
	APS_ALPHA_API bool TryInteract(APawn* Pawn, AActor* Candidate);

	/** A camera moment: the walker's view turns to Target over TurnSeconds, holds HoldSeconds, then is the player's again. */
	APS_ALPHA_API void LookAt(UWorld* World, const FVector& Target, float TurnSeconds = 1.2f, float HoldSeconds = 1.6f);
	/** The home monument of the Builders in an ARK world (null until it stands). */
	APS_ALPHA_API AActor* FindMonument(const UWorld* World);

	/**
	 * The monument's ring in an ARK world: true with the ark's direction in the body's own frame and the ring's distances
	 * once the ark stands on Body; Wait true while the ark is not down yet (the monument waits for it).
	 */
	APS_ALPHA_API bool MonumentRing(const UWorld* World, const APlanetaryBody* Body, FVector& OutLocalCentre,
		FVector& OutLocalToward, double& OutMinCm, double& OutMaxCm, bool& bOutWait);
	/** A candidate on that ring (body-local unit direction) within the sector around Toward the ark's exit faces (the hull hides
	 * the rest from beside it), seeded: the same world puts it at the same place. */
	APS_ALPHA_API FVector RingDirection(const FVector& LocalCentre, const FVector& LocalToward, double BodyRadiusCm, double MinCm,
		double MaxCm, uint32 Seed, int32 Index);

	/** Once a frame (the fleet command subsystem): the light wave, the camera moments. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);
}
