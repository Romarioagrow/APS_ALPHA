#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestTypes.h"

/** Value-only snapshot copied from the Civilization-owned actor-ready manifest seam. */
struct APS_ALPHA_API FAPSCivilizationQuestReadySnapshot
{
	FGuid ManifestId;
	EAPSCivilizationMaterializationState Provenance{
		EAPSCivilizationMaterializationState::PendingPlacement};
	FAPSQuestEntityRef Base;
	FAPSQuestEntityRef LandingPad;
	FAPSQuestEntityRef SelectedShip;
	FAPSQuestEntityRef HomeSystem;
	FAPSQuestEvent ReadyEvent;
};

/** Pure Quest-owned adapter; never retains a manifest reference or scans world actors. */
class APS_ALPHA_API FAPSCivilizationQuestAdapter
{
public:
	static const FName ReadyVerb;

	/**
	 * Copies, migrates and validates one canonical actor-ready manifest. Materialized and
	 * LoadedFromSave intentionally normalize to the same deterministic semantic fact.
	 */
	static bool NormalizeReadyManifest(
		const FAPSCivilizationRuntimeManifest& Source,
		FAPSCivilizationQuestReadySnapshot& OutSnapshot,
		FString& OutReason);

private:
	static FGuid MakeReadyFactId(const FGuid& ManifestId);
};
