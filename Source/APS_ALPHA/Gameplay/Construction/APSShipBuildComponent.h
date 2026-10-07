#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "APSShipBuildComponent.generated.h"

class APlanetaryBody;
class ASpaceship;
class FAPSConstructionMode;
class UInputComponent;

/**
 * Orbital build mode (Rio 02.10, C14: "build mode on the surface and in orbit ... in orbit it can be rotated freely"): a
 * ship in orbit of a planet or moon hosts FAPSConstructionMode with the orbit placement, as the walker hosts the ground one.
 * B starts and ends it. The ghost floats on the cursor's ray (the wheel sets how far), Q/E turn it, R tilts it (Shift the
 * other way), and the zone is a sphere around the ship. The ship holds its turn while building (the mouse is the cursor's);
 * W/S still move it. Placed props and structures belong to the world below, as the ground's do.
 */
UCLASS()
class APS_ALPHA_API UAPSShipBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPSShipBuildComponent();

	/** B: on, or off when on. */
	void Toggle();
	bool IsBuilding() const { return Mode.IsValid(); }
	/** The running build mode (test hooks), or null. */
	FAPSConstructionMode* GetMode() const { return Mode.Get(); }
	/** The world this ship builds at: the nearest planet or moon within OrbitReachRadii of its surface, outside the air. */
	APlanetaryBody* FindOrbitSite(FText* OutWhyNot = nullptr) const;
	/** The flight hint's part: "B ORBITAL BUILD" where it can start, why it could not for a moment after B, else empty. */
	FString GetHintText() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Enter();
	void Exit(bool bRestoreInput);
	void HandleKey(FKey Key);
	void IgnoreAxis(float Value) {}
	ASpaceship* GetShip() const;

	TSharedPtr<FAPSConstructionMode> Mode;
	TWeakObjectPtr<UInputComponent> BuildInput;
	TWeakObjectPtr<APlanetaryBody> Site;
	FText RefusalText;
	double RefusalUntilSeconds{0.0};
};
