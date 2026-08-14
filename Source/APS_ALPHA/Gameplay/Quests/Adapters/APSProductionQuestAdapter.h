#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEvent.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestTypes.h"

/** Pure owner-boundary adapter for the factual APS-81 production event contract. */
class APS_ALPHA_API FAPSProductionQuestAdapter
{
public:
	/**
	 * Normalizes a committed production event without publishing or executing gameplay.
	 * StreamId is supplied by the save/world integration owner; telemetry time is ignored.
	 */
	static bool Normalize(const FAPSProductionEvent& Source, const FGuid& StreamId,
		FAPSQuestEvent& OutEvent, FString& OutReason);

private:
	static EAPSQuestEventResult MapResult(EAPSProductionEventResult Result);
};
