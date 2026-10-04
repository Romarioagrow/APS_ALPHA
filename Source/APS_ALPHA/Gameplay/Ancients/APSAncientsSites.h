#pragma once

#include "CoreMinimal.h"
#include "APSAncientsTypes.h"

class AActor;
class AAstroGenerator;
class APlanet;
class APlanetaryBody;
class FAPSStarSystems;
class UWorld;

/**
 * Where the Builders' sites are, as data (Docs/Design/ANCIENT_STRUCTURES.md, "Placement"). Every roll hashes the world or
 * cluster seed with stable keys, so the same world always has the same sites, kinds and shapes; nothing here looks at
 * the player, the clock or the global random stream.
 */
namespace APSAncientsSites
{
	/** The generated world's seed (the committed model's, else the generator's); 0 when neither carries one. */
	int32 WorldSeedOf(const UWorld* World, const AAstroGenerator* Generator);

	/**
	 * The start system's guaranteed sites: the monument on the home planet, the small circle on a home moon with a surface
	 * (the home planet when it has none) and the derelict in the home planet's high orbit. OutBodies maps each id to its
	 * body. Needs the home planet's moons to exist (they are spawned with it).
	 */
	void BuildHomeSpecs(int32 WorldSeed, APlanet* HomePlanet, TArray<APSAncients::FSiteSpec>& OutSpecs,
		TMap<FString, TWeakObjectPtr<APlanetaryBody>>& OutBodies);

	/**
	 * The chance site of another world of the start system (about one world in five with a surface): false when the world
	 * hides none. Rolled per world from its own key, so the order worlds are met in does not matter.
	 */
	bool RollLostWorks(int32 WorldSeed, const APlanetaryBody* Body, APSAncients::FSiteSpec& OutSpec);

	/** Chance sites in the systems nearest home, at least one among the three nearest; nearest first. */
	void BuildNearbySpecs(const FAPSStarSystems& Stars, TArray<APSAncients::FSiteSpec>& OutSpecs);

	/** The Index-th candidate place of a surface site: a body-local direction away from the poles, the same every session. */
	FVector CandidateDirection(uint32 Seed, int32 Index, double MaxLatitudeDegrees = 55.0);
	/** The site's turn about its up, radians. */
	double Yaw(uint32 Seed);
	/** An orbital site: body-local direction and distance from the body's centre in body radii. */
	void OrbitOf(uint32 Seed, FVector& OutLocalDirection, double& OutRadii);
	/** The orbital site's own turn (its hull axis and roll), relative to the body. */
	FQuat OrbitalTurn(uint32 Seed);
	/** A body's fleet key (FAPSFleetCommand::KeyOf) and its catalogue name for the texts. */
	FString KeyOf(const AActor* Body);
	FText BodyName(const AActor* Body);
}
