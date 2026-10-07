#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "APSShipHullComponent.generated.h"

/**
 * Rio 07.10 ("turn the fleet proxies on", without the 0.3-0.7 s freeze when a hull's body came back): a ship's root hull
 * mesh (ASpaceship::SpaceshipHull) that can hold its physics body. UE 5.4 builds a body in one synchronous InitBody over all
 * its shapes (~22 us a shape: 297-675 ms for the M02's 13.5k before Codex's phase 1), so a ship flying on its proxy boxes
 * no longer destroys its body: ASpaceship makes it inert (PhysicsOnly, every channel ignored: out of queries and of the
 * simulation's pairs, the physics bit kept so nothing is rebuilt) and holds it here, so the hull's own moves, its parent's
 * motion and world shifts are not sent to physics either. Releasing it sends one teleport to where the hull is now.
 * A UStaticMeshComponent in every other respect: while nothing holds it (aps.Ship.HullHold 0) it is a pure pass-through.
 */
UCLASS(ClassGroup = (APS))
class APS_ALPHA_API UAPSShipHullComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UAPSShipHullComponent(const FObjectInitializer& ObjectInitializer);

	bool IsBodyHeld() const { return bBodyHeld; }
	/** Holds the body where it is (its transform is no longer sent to physics), or sends it to the hull with one teleport. */
	void SetBodyHeld(bool bHold);

protected:
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

private:
	bool bBodyHeld = false;
};

namespace APSShipHull
{
	/** Emergency compile-time switch, valid ONLY while no ship Blueprint or map has been saved with this class: false makes
	 * ASpaceship create a plain UStaticMeshComponent again. The normal rollback is aps.Ship.HullHold 0 (nothing is held, a
	 * pure pass-through). Once an asset was saved with it, removing the class also needs [CoreRedirects]
	 * +ClassRedirects=(OldName="/Script/APS_ALPHA.APSShipHullComponent",NewName="/Script/Engine.StaticMeshComponent"). */
	inline constexpr bool bSubclass = true;
}
