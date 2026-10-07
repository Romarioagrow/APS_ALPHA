#pragma once

#include "CoreMinimal.h"

class AAstroGenerator;

/**
 * Rio 03.10 ("the points in the centre stick onto each other: let them be close to each other, but not on top of each
 * other"): the menu's live cluster sample can be spaced apart in dense cores, for presentation only. Points, visited
 * (materialized) systems, labels and picking all read the same offset, so nothing jumps when a system is visited.
 * Catalogue records, saves and gameplay positions never change, and the home system never moves.
 * aps.Preview.ClusterSpacingPixels (0 = off) sets the spacing; a new preview applies a change.
 */
namespace APSPreviewClusterSpacing
{
	/** Presentation offset of one live cluster system (physical cm, preview root frame); zero when not spaced. */
	FVector GetOffsetCm(const AAstroGenerator* Generator, int32 InstanceIndex);
}
