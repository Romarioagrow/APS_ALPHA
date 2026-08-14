#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APSQuestCivilizationAdapterSubsystem.generated.h"

class UAPSCivilizationMaterializationSubsystem;

/**
 * Tickless binding from the Civilization-owned actor-ready delegate to Quest runtime.
 * It subscribes at world BeginPlay, performs one gated late-read, and never materializes actors.
 */
UCLASS()
class APS_ALPHA_API UAPSQuestCivilizationAdapterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void Subscribe(UWorld& InWorld);
	void HandleMaterializationStateChanged(
		const FAPSCivilizationRuntimeManifest& Manifest,
		EAPSCivilizationMaterializationState Previous,
		EAPSCivilizationMaterializationState Current);
	bool TryConsumeReadyManifest(const FAPSCivilizationRuntimeManifest& Manifest);
	void Unsubscribe();

	TWeakObjectPtr<UAPSCivilizationMaterializationSubsystem> CivilizationSubsystem;
	FDelegateHandle MaterializationStateChangedHandle;
};
