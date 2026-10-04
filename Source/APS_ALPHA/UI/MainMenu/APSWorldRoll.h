#pragma once

#include "CoreMinimal.h"

class UGeneratedWorld;

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
		PlanetOnly
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

	/** Rolls a new world into World from RollSeed. The caller has already cleared the explicit edit maps. */
	FResult Apply(UGeneratedWorld& World, int32 RollSeed, EScope Scope);

	/** A person drives the menu: no -unattended, commandlet, automation test, -ExecCmds script, APSBench, APSDiagnostic
	 * or APSProbe switch, nor the -APSNoWorldRoll opt-out. */
	bool IsInteractiveSession();
}
