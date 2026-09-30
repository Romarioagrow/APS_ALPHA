#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSStartSequenceSubsystem.generated.h"

/**
 * Finishing touches of a new generated start that belong to no single actor. A "Space Ship" start seats the pilot in
 * the home ship instead of leaving them standing inside its hull (Rio, 29.09/30.09: every start mode must really work).
 */
UCLASS()
class APS_ALPHA_API UAPSStartSequenceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bDone; }
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}

private:
	/** When the pilot and the home ship were first both there; the generator finishes its start in the next frames. */
	double FirstReadySeconds{-1.0};
	bool bDone{false};
};
