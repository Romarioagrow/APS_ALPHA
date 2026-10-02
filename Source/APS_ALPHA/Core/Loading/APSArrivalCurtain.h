#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 02.10 ("on the planet it spawns twice: it appears, then appears somewhere else and the objects spawn ... a loading
 * screen: until everything has appeared and spawned and the system says all is ready, only then the screen goes off, and
 * it is beautiful at once, without jerks"). A full-screen curtain over the generated world from its first frame until the
 * start has settled: the starter set in the world, on a surface start the pilot placed for good (the terrain handoff's
 * FINAL), the colony on the ground and its vehicles parked, then the pilot still and the frames smooth for a moment. It
 * fades out then. A deadline lifts it anyway and logs what was still missing. aps.Loading.Curtain 0 turns it off.
 * Plain C++: hooked to every game world as it begins play, ticked on its own.
 */
namespace APSArrivalCurtain
{
	/** True while the curtain covers this world. */
	APS_ALPHA_API bool IsUp(const UWorld* World);
}
