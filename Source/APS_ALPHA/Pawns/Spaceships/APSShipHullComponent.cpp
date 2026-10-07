#include "APSShipHullComponent.h"

UAPSShipHullComponent::UAPSShipHullComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAPSShipHullComponent::SetBodyHeld(const bool bHold)
{
	if (bBodyHeld == bHold)
	{
		return;
	}
	bBodyHeld = bHold;
	if (!bHold && IsPhysicsStateCreated())
	{
		// Rio 07.10 (aps.Ship.HullHold): one teleport to where the hull is now, a walk over the shapes' bounds, no build.
		SendPhysicsTransform(ETeleportType::TeleportPhysics);
	}
}

void UAPSShipHullComponent::OnUpdateTransform(const EUpdateTransformFlags UpdateTransformFlags, const ETeleportType Teleport)
{
	// Rio 07.10 (aps.Ship.HullHold): the hull's own moves, its parent's motion and world shifts all arrive here; a held body
	// is not sent them (UPrimitiveComponent::OnUpdateTransform skips SendPhysicsTransform). The proxy boxes and the other
	// children keep theirs: UE 5.4 USceneComponent::PropagateTransformUpdate never passes SkipPhysicsUpdate on to children.
	Super::OnUpdateTransform(bBodyHeld ? UpdateTransformFlags | EUpdateTransformFlags::SkipPhysicsUpdate : UpdateTransformFlags,
		Teleport);
}
