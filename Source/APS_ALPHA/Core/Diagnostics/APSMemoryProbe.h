#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * One memory line a minute in a game world ([APS.Mem], aps.Mem.LogSeconds): the process commit and working set, the
 * commit the system has left, UObjects, actors and WorldScape roots. The editor ran out of commit after an hour of play
 * (01.10: 52.7 GiB committed, 3.1 GiB resident), and the log had no trace of how it grew; with this every play log does.
 */
namespace APSMemoryProbe
{
	/** Called every frame by a game-world subsystem; logs when the interval has passed. */
	APS_ALPHA_API void Tick(UWorld* World);
	/** Logs one line now, with a reason (a map change, a test step). */
	APS_ALPHA_API void Log(UWorld* World, const TCHAR* Reason);
}
