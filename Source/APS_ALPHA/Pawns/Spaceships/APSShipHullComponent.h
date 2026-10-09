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

	/**
	 * Rio 09.10 (aps.Ship.HullBodySyncOnce; walking aboard the L flagship on the autopilot, game thread 15-17 ms): the
	 * autopilot's turn and the step of the same frame each sent the kinematic hull body a target, and UE 5.4 walks every
	 * shape of it per send (FChaosEngineInterface::SetKinematicTarget_AssumesLocked: UpdateShapeBounds, then the
	 * acceleration structure and its two pre-filter passes over the shapes). Inside a sync scope a plain move
	 * (ETeleportType::None) is held back and only the last one is sent, with the transform it had; anything else that
	 * reaches the body (a teleport, a move without physics, a physics flip, a hold) sends the held-back one first, so the
	 * body gets the same calls in the same order, minus the targets the next plain move overwrote anyway.
	 */
	void BeginBodySync();
	void EndBodySync();
	/** A plain move of the open scope has not reached the body yet. */
	bool HasPendingBodySync() const { return bBodySyncPending; }
	/** The bounds FBodyInstance::GetBodyBounds returns once the held-back move is sent (Chaos keeps R in float). */
	FBox GetPendingBodyBounds() const;

	virtual void SetSimulatePhysics(bool bSimulate) override;

protected:
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

private:
	/** Sends the held-back plain move, if the body that would have got it is still there. */
	void FlushBodySync();

	bool bBodyHeld = false;
	int32 BodySyncDepth = 0;
	bool bBodySyncPending = false;
	FTransform PendingBodyTransform;
	FPhysicsActorHandle PendingBodyHandle = nullptr;
	FVector BodySyncScale = FVector::OneVector;
	/** Per-ship counts for the summary line (aps.Ship.HullBodySyncOnce). */
	int32 BodySyncFrames = 0;
	int32 BodySyncMoves = 0;
	int32 BodySyncSaved = 0;
	double BodySyncSince = -1.0;
};

class ASpaceship;

namespace APSShipHull
{
	/** Emergency compile-time switch, valid ONLY while no ship Blueprint or map has been saved with this class: false makes
	 * ASpaceship create a plain UStaticMeshComponent again. The normal rollback is aps.Ship.HullHold 0 (nothing is held, a
	 * pure pass-through). Once an asset was saved with it, removing the class also needs [CoreRedirects]
	 * +ClassRedirects=(OldName="/Script/APS_ALPHA.APSShipHullComponent",NewName="/Script/Engine.StaticMeshComponent"). */
	inline constexpr bool bSubclass = true;

	/**
	 * Rio 09.10 (aps.Ship.HullBodySyncOnce): opens a body sync scope on the ship's root hull for the moves of one tick (its
	 * step and its turn) when that is safe: a kinematic, unwelded, not held hull body in the scene, without overlap events
	 * (nobody keeps overlaps with it, so its pose inside the scope is read by no overlap set) and not a ground vehicle.
	 * End() (or the destructor) sends the last move.
	 */
	class FScopedBodySync
	{
	public:
		explicit FScopedBodySync(ASpaceship& Ship);
		~FScopedBodySync() { End(); }
		void End();
		FScopedBodySync(const FScopedBodySync&) = delete;
		FScopedBodySync& operator=(const FScopedBodySync&) = delete;

	private:
		TWeakObjectPtr<UAPSShipHullComponent> Hull;
	};
}
