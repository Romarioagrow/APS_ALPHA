#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSFleetCommandSubsystem.generated.h"

class FAPSFleetCommand;

/**
 * Owns the world's fleet command (APSFleetCommand.h: units, divisions, orders, the autopilot, surveys, outposts) and
 * ticks it with the world, so it pauses with the game. The menu and the console reach it through APSFleetFind(World).
 */
UCLASS()
class APS_ALPHA_API UAPSFleetCommandSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	TSharedPtr<FAPSFleetCommand> Fleet;
};
