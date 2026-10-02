#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 02.10 ("no planet surfaces, a grey ball: critical, blocker"): what stands for a planet or moon on screen, written
 * to the log ([APS.SurfaceDiag]): the body's own components (the placeholder sphere among them) with visibility and
 * material, its WorldScape root (hidden, initialised, LOD meshes with their materials, the observer it builds for), the
 * atmosphere and attached actors. Logged once when the view comes within three radii of a body and again 15 s later,
 * and on demand with aps.Surface.Diag.
 */
namespace APSSurfaceDiag
{
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);
	APS_ALPHA_API void DumpNearest(UWorld* World, int32 Count, const TCHAR* Reason);
}
