#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 02.10 (department missions in play): the tracked mission in the top-left corner of the game view.
 * It shows the department, the title, the objective with its progress, and the reward.
 * Hidden without an active tracked mission and under the terminal and the maps. Kept by the fleet command subsystem.
 *
 * Rio 03.10 ("the journal runs over the objective; combine them in one style"): one chamfered card with the onboarding
 * objective, in the objective panel's style. The objective heads it, the mission follows under a thin divider; either
 * one alone fills the card, so the two never overlap whatever their texts.
 */
namespace APSMissionTracker
{
	/** Puts the card into the world's game view once a mission is tracked or the objective is set (cheap when it is there). */
	APS_ALPHA_API void Tick(UWorld* World);
	/** Takes it out of the view at the world's end. */
	APS_ALPHA_API void Remove(UWorld* World);
	/**
	 * The colony terminal subsystem's onboarding objective for the card's top section, read every frame: false hides the
	 * section, true shows OutTitle over OutBody. An empty function takes it out again (and never adds the card).
	 */
	APS_ALPHA_API void SetObjective(UWorld* World, TFunction<bool(FText& OutTitle, FText& OutBody)> Objective);
}
