#pragma once

#include "CoreMinimal.h"

class UAPSAudioBank;
struct FAPSAudioFootsteps;
struct FHitResult;

namespace APSAudioPlayback
{
	/** Resolve each contact from its supporting floor, independently of the gravity source. */
	const FAPSAudioFootsteps& ResolveFootsteps(const FHitResult& Floor, const UAPSAudioBank& Bank);
}
