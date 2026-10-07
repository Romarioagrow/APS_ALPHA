#pragma once

#include "CoreMinimal.h"

class AStar;
enum class EStellarType : uint8;

/**
 * Rio 03.10 ("the black hole is ugly, make it beautiful"). V1 drew a black hole as the opaque photosphere sphere
 * with the master's archetype 6: an orange rim plus an equatorial stripe painted across the ball, a disc that
 * never left the silhouette. V2 keeps that sphere only as the envelope every preview scope frames, scales and
 * picks (bounds, collision and visibility state stay; no view draws it) and draws inside the same radius, with
 * existing materials:
 *  - a true black shadow (M_SpectralStarMat_SUN, ordinary archetype, black spectrum: zero emission);
 *  - a thin HDR photon ring: the archetype-6 shader on an inside-out sphere just outside the shadow, so it is
 *    camera-facing and carries the lensed disc edges with their Doppler asymmetry;
 *  - a tilted accretion disc (M_APS_PreviewGuide bands): hot white-blue inner edge to orange-red, ragged rim;
 *  - relativistic beaming: the receding half dimmer and redder, turned to the view every frame;
 *  - the lensed far side of the disc as soft arcs above and below the shadow (camera-facing).
 * No light: the hole does not light its system (the stellar subsystem's key is a separate hook).
 * aps.Stars.BlackHoleV2 0 restores V1 at once for built black holes.
 */
namespace APSBlackHoleVisual
{
	/** Builds, updates or removes the V2 presentation after the star's spectral material was applied. */
	void Configure(AStar* Star, EStellarType StellarType, float SurfaceSeed);

	/** aps.Stars.BlackHoleV2 != 0. */
	bool IsEnabled();

	/** Suggested key light while a black hole is the active star: a dim warm disc glow instead of a sun. */
	void GetDiskKeyLight(float& OutIntensity, float& OutTemperatureKelvin);

	/** Suggested catalogue point colour (custom data 0..2) for a black hole: a faint warm point instead of none. */
	FLinearColor GetCatalogueTint();
}
