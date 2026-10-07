#pragma once

#include "CoreMinimal.h"

/**
 * The renderer's safety switches for jumps it cannot follow (APSRenderSafety.cpp). The virtual shadow map cache pans its
 * clipmaps by an int32 page offset: a camera jump of thousands of kilometres in one frame (the strategic map, a world
 * shift) overflows it and asserts. The guard turns panning off for such frames and a second after.
 */
namespace APSRenderSafety
{
	/** Panning off from this frame on, for the guard's hold (Rio 03.10: the floating origin shifts the world under the
	 * camera; the guard alone would only see the jump a frame late). */
	APS_ALPHA_API void MarkCameraJump(const TCHAR* Why);
}
