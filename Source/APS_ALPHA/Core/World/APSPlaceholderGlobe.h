#pragma once
#include "CoreMinimal.h"

class APlanetaryBody;
class UWorld;

// Closed physical geometry for bodies without resident WorldScape LODs.
// Material, profile and planet-fixed coordinates are never substituted or baked.
namespace APSPlaceholderGlobe
{
	APS_ALPHA_API bool Handles(const APlanetaryBody* Body);
	APS_ALPHA_API void Apply(APlanetaryBody* Body);
	// True means this path owns visibility; do not enable the old authored spheres.
	APS_ALPHA_API bool SetVisible(APlanetaryBody* Body, bool bVisible);
	// Keep a published native root until its same-profile closed geometry is ready.
	APS_ALPHA_API bool PrepareForUnload(APlanetaryBody* Body);
	APS_ALPHA_API void Tick(UWorld* World);
	APS_ALPHA_API void Release(UWorld* World);
	// Initial loading curtain only; no new curtain during travel.
	APS_ALPHA_API bool HasInitialCoverage(UWorld* World);
}
