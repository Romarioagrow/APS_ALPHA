#pragma once

#include "CoreMinimal.h"

class UGeneratedWorld;
enum class EPlanetaryZoneType : uint8;

/**
 * REGENERATE world roll (Rio 03.10: "every press unique by count, by type, by moons ... adequate, never nonsense, yet
 * sometimes something really interesting"). One fresh seed drives every choice deterministically: the home system's
 * stars, planet count, orbit layout and tilt, the start world with its moons, and now and then a hand-made archetype.
 * Everything is written into the menu model where the left panel reads and edits it (base home fields, the SYS0
 * system edit, a star edit for every sun, archetype body and orbit edits, the start-world editor buffer), so a rolled
 * world saves, loads and reaches gameplay exactly like an edited one. Plain C++: no reflection is added.
 * The System scope also rolls the galaxy (type, per-type subclass, size, density) and the home cluster (formation,
 * size Small..Giant, population, composition) from a stream of their own, now and then as a paired archetype.
 */
namespace APSWorldRoll
{
	enum class EScope : uint8
	{
		/** The complete home system: stars, planets and the start world. */
		System,
		/** The PLANET route builds one body: only the world itself is rolled. */
		PlanetOnly,
		/** Rio 04.10 evening ("on every level REGENERATE rolls only its own; on OVERVIEW everything, as now"): the scopes
		 * below keep the world seed, so the galaxy, the cluster, the home record and every body outside them stay. */
		GalaxyOnly,
		ClusterOnly,
		/** The home recipe (stars, planets, orbits, the start world) in its place in the cluster; the sky stays. */
		HomeSystemOnly,
		/** One star of the home system; its planets stay. */
		StarOnly,
		/** The selected body: the panel's editor buffer, which the view model then saves onto that body. */
		BodyOnly
	};

	/** What a scoped roll aims at: StarOnly the home system's star (0 = the primary), BodyOnly the orbit zone of the
	 * selected body (unset: rolled as on the PLANET route). */
	struct FScopeTarget
	{
		int32 StarIndex{0};
		TOptional<EPlanetaryZoneType> Zone;
	};

	struct FResult
	{
		int32 RollSeed{0};
		int32 WorldSeed{0};
		FString Archetype;
		/** "seed=... world=... star=... planets=... moons=... archetype=..." for the log line. */
		FString Summary;
	};

	/** A seed nobody rolled before: wall clock, CPU cycles, a press counter and the global stream, mixed. */
	int32 MakeFreshSeed();

	/** Rolls a new world into World from RollSeed. For System and HomeSystemOnly the caller has already cleared the explicit
	 * edit maps; the other scoped rolls overwrite only their own fields. */
	FResult Apply(UGeneratedWorld& World, int32 RollSeed, EScope Scope, const FScopeTarget& Target = FScopeTarget());

	/** A person drives the menu: no -unattended, commandlet, automation test, -ExecCmds script, APSBench, APSDiagnostic
	 * or APSProbe switch, nor the -APSNoWorldRoll opt-out. */
	bool IsInteractiveSession();
}
