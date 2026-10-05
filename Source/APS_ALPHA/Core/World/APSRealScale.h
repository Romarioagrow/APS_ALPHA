// Rio 05.10 (real scale, stage 2): the REAL SCALE gameplay switch. Every real-scale hook asks APSRealScale::IsActive and
// keeps its legacy path when it is false: in the menu, in every legacy world and before the world's generator is final.
#pragma once

#include "CoreMinimal.h"

class AAstroGenerator;
class UWorld;

namespace APSRealScale
{
	/** The generator a game world's stellar view draws (never the menu preview), or null until it is final. Cached. */
	APS_ALPHA_API AAstroGenerator* FindGameplayGenerator(const UWorld* World);

	/** True only in a game world whose generator built a REAL SCALE world (AAstroGenerator::UsesRealScale). */
	APS_ALPHA_API bool IsActive(const UWorld* World);}
