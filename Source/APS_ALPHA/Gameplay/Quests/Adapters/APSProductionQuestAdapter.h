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
	 * Production owns StreamId and subject identity domain; telemetry time is ignored.
	 */
	static bool Normalize(const FAPSProductionEvent& Source, FAPSQuestEvent& OutEvent,
		FString& OutReason);

private:
	static bool MapSubjectKind(EAPSSubjectIdentityDomain Domain,
		EAPSQuestEntityKind& OutKind, FString& OutReason);
	static EAPSQuestEventResult MapResult(EAPSProductionEventResult Result);
};
