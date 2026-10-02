#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 02.10 (department missions in play): the tracked mission in the top-left corner of the game view.
 * It shows the department, the title, the objective with its progress, and the reward.
 * Hidden without an active tracked mission; the terminal and the maps draw over it. Kept by the fleet command subsystem.
 */
namespace APSMissionTracker
{
	/** Puts the tracker into the world's game view once a mission is tracked (cheap when it is there already). */
	APS_ALPHA_API void Tick(UWorld* World);
	/** Takes it out of the view at the world's end. */
	APS_ALPHA_API void Remove(UWorld* World);
}
