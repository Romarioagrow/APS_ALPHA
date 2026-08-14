#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSQuestPresentationSessionSubsystem.generated.h"

/** Advances the presentation epoch once per game world without owning any UI objects. */
UCLASS()
class APS_ALPHA_API UAPSQuestPresentationSessionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	FGuid SessionEpoch;
};
