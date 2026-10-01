#pragma once

#include "CoreMinimal.h"

class APlanetaryBody;

/**
 * How much air a planet or moon has (Rio, 01.10: "some planets have a weak atmosphere; that must be in the model and
 * count for the atmosphere and for the stars"). One number per body: 0 none (airless: stars by day, a black sky),
 * 1 Earth-like, up to 3 crushing. It follows the body's generated type, varies a little with its surface seed, and is
 * zero when the body has no atmosphere shell (AtmosphereHeight 0). Deterministic: the same body gives the same air.
 *
 * Read by the stars' daylight fade (a thin sky hides fewer stars) and by surveys; the sky's own brightness is next.
 */
namespace APSAtmosphereModel
{
	APS_ALPHA_API float Density(const APlanetaryBody* Body);
	/** How strongly a lit sky of this density masks the stars: 0 none (airless), 1 an Earth-like day or thicker. */
	APS_ALPHA_API float DaySkyMasking(float Density);
	/** NONE, THIN, EARTH-LIKE, DENSE or CRUSHING. */
	APS_ALPHA_API FText Describe(float Density);
}
