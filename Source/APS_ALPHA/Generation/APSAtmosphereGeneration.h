#pragma once

#include "PlanetaryAtmosphere.h"

namespace APSAtmosphereGeneration
{
	inline float GeneratedAirGlow(float PressureResponse, float HumidityResponse,
		float DustResponse, float SeededResponse)
	{
		// Airglow is an emissive night-limb contribution, not the daylight haze.
		// Keep it subordinate to scattering instead of drawing a neon outer shell.
		return FMath::Clamp(0.002f + PressureResponse * 0.003f
			+ HumidityResponse * 0.001f + (1.0f - DustResponse) * 0.0005f
			+ SeededResponse * 0.0002f, 0.0018f, 0.007f);
	}

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
