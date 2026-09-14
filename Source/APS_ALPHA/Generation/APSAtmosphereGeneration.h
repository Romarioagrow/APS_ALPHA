#pragma once

#include "PlanetaryAtmosphere.h"

namespace APSAtmosphereGeneration
{
	/** Call only after a complete authored/generated km-parameter batch, not every tick.
	 * The batch already supplies the new radius AND scattering heights. Adopt it as
	 * the relative-scale baseline, so UpdateScale does not apply the radius change
	 * twice. Subsequent radius/height edits still use AtmoScape's relative scaling.
	 */
	inline void CommitAuthoredDimensions(AAtmoScape& Atmosphere)
	{
		Atmosphere.bKeepRelativeScale = true;
		Atmosphere.LastPlanetRadius = Atmosphere.PlanetRadius;
		Atmosphere.LastAtmosphereHeight = Atmosphere.AtmosphereHeight;
	}
}
