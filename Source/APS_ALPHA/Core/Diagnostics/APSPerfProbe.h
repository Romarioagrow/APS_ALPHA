#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 02.10 ("FPS drops, it was not like this before, check"): every few seconds of play one line with the frame,
 * game-thread, render-thread and GPU times, average and worst ([APS.Perf], aps.Perf.LogSeconds), plus a line for each
 * hitch over aps.Perf.HitchMs. A slow stretch in any play log then names the thread that holds the frame.
 */
namespace APSPerfProbe
{
	/** Called every frame by a game-world subsystem; logs when the interval has passed. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);

	/** Rio 06.10 (audit: a dump outlived its world): called by the same subsystem as the world ends; stops the probe's
	 * own 'stat dumphitches' if it was running for this world (or its world is already gone). */
	APS_ALPHA_API void WorldEnded(UWorld* World);
}
