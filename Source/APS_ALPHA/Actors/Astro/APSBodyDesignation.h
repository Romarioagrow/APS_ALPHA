#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * Short catalogue designations (Rio, 02.10): star A, planet A1 (first planet of star A), moon A5.04 (fourth moon of
 * planet A5). Every star of a multiple system has its own letter. Empty for an actor not linked into a system.
 */
namespace APSBodyDesignation
{
	APS_ALPHA_API FString Of(const AActor* Actor);
}
