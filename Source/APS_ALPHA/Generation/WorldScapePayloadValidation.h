#pragma once

#include "CoreMinimal.h"

class UWorldScapeLod;

namespace APSWorldScapePayloadValidation
{
	/**
	 * WorldScape allocates three mesh sections synchronously, then fills their main,
	 * vertical-strip and horizontal-strip payloads on a worker. Call this only after
	 * WorldScapeLodInGeneration is empty; it deliberately validates worker-owned arrays.
	 */
	APS_ALPHA_API bool HasCompletePayload(
		const UWorldScapeLod* Lod, bool bRequireProfileColor);

	APS_ALPHA_API bool HasCompleteCenteredPayload(const UWorldScapeLod* Lod,
		const FVector& DesiredSurfaceNormal, bool bRequireProfileColor);
}
