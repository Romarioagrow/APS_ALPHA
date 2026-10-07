#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Rio 03.10 ("every star must be reachable; it should look the same; load only the nearest ones"). The galaxy catalogue
 * stars drawn as GPU points have no mesh: one a few AU away stayed a single bright pixel, and a flight to it ended in a
 * sudden full-size star. The drawn GPU-only stars nearest to the camera that are at least aps.Stars.ResolvePixels in
 * radius become crisp photospheres here (one instanced mesh, the star material's instance path, like the catalogue's
 * resolved stars), so a point grows into its star; the point at the centre is hidden by the sphere's depth.
 */
namespace APSGalaxyNearStars
{
	/** Each frame from the gameplay stellar view (queries the nearest-star index at most ten times a second). */
	void Update(UWorld* World, const FVector& Camera, double PixelTangent, bool bDaylightHidden);
	/** Removes a world's photospheres (the stellar view resets). */
	void Reset(const UWorld* World);
	/** A catalogue star that stands materialized as a real system is drawn by its actor, not here. */
	void SetMaterialized(const UWorld* World, int64 CatalogIndex, bool bMaterialized);
}
