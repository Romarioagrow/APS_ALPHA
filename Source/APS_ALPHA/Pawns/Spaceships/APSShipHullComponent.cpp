#include "APSShipHullComponent.h"

#include "Spaceship.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Chaos/ParticleHandle.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace APSShipHullLocal
{
	// Rio 09.10 (walking aboard the L flagship on the autopilot: game thread 15-17 ms against 5-9 seated). Default 0 until the
	// ap-walk A/B (run_0810_night.ps1 -Only ap-walk -HomeShipClass ...L_P1_08): the same frames, fewer SetBodyTransform.
	TAutoConsoleVariable<int32> CVarHullBodySyncOnce(
		TEXT("aps.Ship.HullBodySyncOnce"), 0,
		TEXT("Rio 09.10 (walking aboard a flying ship): 1: the plain moves of one ship tick (the autopilot's or the pilot's turn ")
		TEXT("and the step) reach the hull's own kinematic body once, with the last transform; UE 5.4 walks every collision ")
		TEXT("shape of the body per send. Only a kinematic, unwelded, not held hull body without overlap events, never a ground ")
		TEXT("vehicle; a teleport, a move without physics, a physics flip or a hold sends the held-back move first. The body ends ")
		TEXT("each tick where it did before. 0: one send per move, as before."));

	/** P.Chaos.SyncKinematicOnGameThread 1: a kinematic body's pose comes back from the simulation, not from its target. */
	bool IsKinematicSyncedFromSimulation()
	{
		static IConsoleVariable* const Sync = IConsoleManager::Get().FindConsoleVariable(TEXT("P.Chaos.SyncKinematicOnGameThread"));
		return Sync && Sync->GetInt() == 1;
	}
}

UAPSShipHullComponent::UAPSShipHullComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAPSShipHullComponent::SetBodyHeld(const bool bHold)
{
	// Rio 09.10 (aps.Ship.HullBodySyncOnce): a held-back move reaches the body before it is held, as it did before.
	FlushBodySync();
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

void UAPSShipHullComponent::SetSimulatePhysics(const bool bSimulate)
{
	// Rio 09.10 (aps.Ship.HullBodySyncOnce): a held-back move reaches the body before its physics flips, as it did before.
	FlushBodySync();
	Super::SetSimulatePhysics(bSimulate);
}

void UAPSShipHullComponent::BeginBodySync()
{
	if (BodySyncDepth++ == 0)
	{
		bBodySyncPending = false;
		BodySyncScale = GetComponentTransform().GetScale3D();
	}
}

void UAPSShipHullComponent::EndBodySync()
{
	if (BodySyncDepth <= 0 || --BodySyncDepth > 0)
	{
		return;
	}
	FlushBodySync();
	++BodySyncFrames;
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	if (BodySyncSince < 0.0 || Now < BodySyncSince)
	{
		BodySyncSince = Now;
	}
	if (Now - BodySyncSince >= 5.0)
	{
		if (BodySyncSaved > 0)
		{
			UBodySetup* Setup = GetBodySetup();
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s: hull body sync (aps.Ship.HullBodySyncOnce): %d ticks, %d plain moves, %d sends ")
				TEXT("to its %d collision shapes saved in %.1f s"), *GetNameSafe(GetOwner()), BodySyncFrames, BodySyncMoves, BodySyncSaved,
				Setup ? Setup->AggGeom.GetElementCount() : 0, Now - BodySyncSince);
		}
		BodySyncFrames = 0;
		BodySyncMoves = 0;
		BodySyncSaved = 0;
		BodySyncSince = Now;
	}
}

void UAPSShipHullComponent::FlushBodySync()
{
	if (!bBodySyncPending)
	{
		return;
	}
	bBodySyncPending = false;
	// Only to the body that would have had it: one rebuilt meanwhile was created where the hull is now.
	if (!bBodyHeld && IsPhysicsStateCreated() && BodyInstance.GetPhysicsActorHandle() == PendingBodyHandle)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_HullBodySync);
		// UPrimitiveComponent::SendPhysicsTransform, with the transform the hull had at that move.
		BodyInstance.SetBodyTransform(PendingBodyTransform, ETeleportType::None);
		BodyInstance.UpdateBodyScale(PendingBodyTransform.GetScale3D());
	}
}

FBox UAPSShipHullComponent::GetPendingBodyBounds() const
{
	const FPhysicsActorHandle& Handle = BodyInstance.GetPhysicsActorHandle();
	if (!bBodySyncPending || !FPhysicsInterface::IsValid(Handle) || Handle != PendingBodyHandle)
	{
		return BodyInstance.GetBodyBounds();
	}
	// FBodyInstance::GetBodyBounds after the send: FChaosEngineInterface::GetBounds_AssumesLocked over the body's X and R,
	// and SetKinematicTarget_AssumesLocked stores X as given and R as a float quaternion (Chaos FParticlePositionRotation).
	const FTransform Pose(FQuat(FQuat4f(PendingBodyTransform.GetRotation())), PendingBodyTransform.GetLocation());
	FBox Box(EForceInit::ForceInitToZero);
	FPhysicsCommand::ExecuteRead(Handle, [&Box, &Pose](const FPhysicsActorHandle& Actor)
	{
		Box = FPhysicsInterface::GetBounds_AssumesLocked(Actor, Pose);
	});
	return Box;
}

void UAPSShipHullComponent::OnUpdateTransform(const EUpdateTransformFlags UpdateTransformFlags, const ETeleportType Teleport)
{
	if (BodySyncDepth > 0 && !bBodyHeld)
	{
		// Rio 09.10 (aps.Ship.HullBodySyncOnce): a plain move that UPrimitiveComponent::OnUpdateTransform would send now is
		// held back (the next plain move overwrites the same kinematic target and pose); anything else goes after it.
		if (Teleport == ETeleportType::None && !(UpdateTransformFlags & EUpdateTransformFlags::SkipPhysicsUpdate)
			&& IsPhysicsStateCreated() && !IsWelded() && !IsSimulatingPhysics() && !GetGenerateOverlapEvents()
			&& GetComponentTransform().GetScale3D().Equals(BodySyncScale, 0.0))
		{
			BodySyncSaved += bBodySyncPending ? 1 : 0;
			++BodySyncMoves;
			bBodySyncPending = true;
			PendingBodyTransform = GetComponentTransform();
			PendingBodyHandle = BodyInstance.GetPhysicsActorHandle();
			Super::OnUpdateTransform(UpdateTransformFlags | EUpdateTransformFlags::SkipPhysicsUpdate, Teleport);
			return;
		}
		FlushBodySync();
	}
	// Rio 07.10 (aps.Ship.HullHold): the hull's own moves, its parent's motion and world shifts all arrive here; a held body
	// is not sent them (UPrimitiveComponent::OnUpdateTransform skips SendPhysicsTransform). The proxy boxes and the other
	// children keep theirs: UE 5.4 USceneComponent::PropagateTransformUpdate never passes SkipPhysicsUpdate on to children.
	Super::OnUpdateTransform(bBodyHeld ? UpdateTransformFlags | EUpdateTransformFlags::SkipPhysicsUpdate : UpdateTransformFlags,
		Teleport);
}

APSShipHull::FScopedBodySync::FScopedBodySync(ASpaceship& Ship)
{
	if (APSShipHullLocal::CVarHullBodySyncOnce.GetValueOnGameThread() == 0 || Ship.IsGroundVehicle())
	{
		return;
	}
	UAPSShipHullComponent* Root = Cast<UAPSShipHullComponent>(Ship.GetRootComponent());
	if (!Root || !Root->IsRegistered() || !Root->IsPhysicsStateCreated() || Root->IsBodyHeld() || Root->IsSimulatingPhysics()
		|| Root->GetGenerateOverlapEvents() || Root->IsWelded() || Root->BodyInstance.bUpdateKinematicFromSimulation
		|| !FPhysicsInterface::IsValid(Root->BodyInstance.GetPhysicsActorHandle())
		|| APSShipHullLocal::IsKinematicSyncedFromSimulation())
	{
		return;
	}
	// A child welded into the body moves with its sends and keeps its own overlap events: such a ship sends every move.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(&Ship);
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive != Root && Primitive->BodyInstance.WeldParent == &Root->BodyInstance)
		{
			return;
		}
	}
	Hull = Root;
	Root->BeginBodySync();
}

void APSShipHull::FScopedBodySync::End()
{
	if (UAPSShipHullComponent* Root = Hull.Get())
	{
		Root->EndBodySync();
	}
	Hull.Reset();
}

namespace APSShipHullLocal
{
	FString ChannelName(const ECollisionChannel Channel)
	{
		const FName Name = UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(static_cast<int32>(Channel));
		return Name.IsNone() ? FString::FromInt(static_cast<int32>(Channel)) : Name.ToString();
	}

	const TCHAR* ResponseName(const ECollisionResponse Response)
	{
		return Response == ECR_Block ? TEXT("block") : Response == ECR_Overlap ? TEXT("overlap") : TEXT("ignore");
	}

	/** Simple shapes of the body setup, and the shapes of the body in the scene (-1: no body). */
	FString DescribeShapes(UPrimitiveComponent& Primitive, int32& OutSetupShapes)
	{
		OutSetupShapes = 0;
		FString Text;
		if (const UBodySetup* Setup = Primitive.GetBodySetup())
		{
			const FKAggregateGeom& Geom = Setup->AggGeom;
			OutSetupShapes = Geom.GetElementCount();
			Text = FString::Printf(TEXT("%d simple (convex %d, box %d, sphere %d, capsule %d), trimesh %d"), OutSetupShapes,
				Geom.ConvexElems.Num(), Geom.BoxElems.Num(), Geom.SphereElems.Num(), Geom.SphylElems.Num(),
				Setup->TriMeshGeometries.Num());
		}
		const FPhysicsActorHandle& Handle = Primitive.BodyInstance.GetPhysicsActorHandle();
		const int32 InScene = Primitive.IsPhysicsStateCreated() && FPhysicsInterface::IsValid(Handle)
			? FPhysicsInterface::GetNumShapes(Handle) : -1;
		return Text + FString::Printf(TEXT(", in scene %d"), InScene);
	}

	void ReportShip(ASpaceship& Ship, const UCapsuleComponent* PlayerCapsule)
	{
		UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Ship.GetRootComponent());
		if (!Root)
		{
			return;
		}
		const UAPSShipHullComponent* Hull = Cast<UAPSShipHullComponent>(Root);
		int32 HullShapes = 0;
		const FString HullText = DescribeShapes(*Root, HullShapes);
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] collision report %s (%s): hull %s: %s; body %s, enabled %d, profile %s, object %s, ")
			TEXT("overlaps %d, simulating %d, welded %d; speed %.1f m/s; pilot %d"),
			*Ship.GetName(), *GetNameSafe(Ship.GetClass()), *GetNameSafe(Root->GetClass()), *HullText,
			Hull && Hull->IsBodyHeld() ? TEXT("held") : Root->IsPhysicsStateCreated() ? TEXT("in") : TEXT("OUT"),
			static_cast<int32>(Root->BodyInstance.GetCollisionEnabled(false)), *Root->GetCollisionProfileName().ToString(),
			*ChannelName(Root->GetCollisionObjectType()), Root->GetGenerateOverlapEvents() ? 1 : 0, Root->IsSimulatingPhysics() ? 1 : 0,
			Root->IsWelded() ? 1 : 0, Ship.GetKinematicVelocity().Size() / 100.0, Ship.HasPilot() ? 1 : 0);

		// Read-only probes of the two costs the hull's shape count sets: the pre-filter passes every send runs over the shapes
		// (FAccelerationStructureHandle; UpdateShapeBounds walks them once more), and single queries at the player's capsule.
		const FPhysicsActorHandle& HullHandle = Root->BodyInstance.GetPhysicsActorHandle();
		if (Root->IsPhysicsStateCreated() && FPhysicsInterface::IsValid(HullHandle))
		{
			const double FilterStart = FPlatformTime::Seconds();
			FPhysicsCommand::ExecuteRead(HullHandle, [](const FPhysicsActorHandle& Actor)
			{
				const Chaos::FAccelerationStructureHandle Probe(Actor->GetParticle_LowLevel());
				(void)Probe;
			});
			const double FilterMs = (FPlatformTime::Seconds() - FilterStart) * 1000.0;
			double OverlapMs = -1.0;
			double TraceMs = -1.0;
			if (UWorld* World = Ship.GetWorld(); World && PlayerCapsule)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipCollisionReport), false, PlayerCapsule->GetOwner());
				const FVector At = PlayerCapsule->GetComponentLocation();
				const double OverlapStart = FPlatformTime::Seconds();
				World->OverlapBlockingTestByChannel(At, PlayerCapsule->GetComponentQuat(), ECC_Pawn,
					FCollisionShape::MakeCapsule(PlayerCapsule->GetScaledCapsuleRadius(), PlayerCapsule->GetScaledCapsuleHalfHeight()),
					Params);
				OverlapMs = (FPlatformTime::Seconds() - OverlapStart) * 1000.0;
				FHitResult Hit;
				const double TraceStart = FPlatformTime::Seconds();
				World->LineTraceSingleByChannel(Hit, At, At - PlayerCapsule->GetUpVector() * 300.0, ECC_Visibility, Params);
				TraceMs = (FPlatformTime::Seconds() - TraceStart) * 1000.0;
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] collision report %s: probes: one send's two pre-filter passes over the hull's shapes ")
				TEXT("%.3f ms; at the player's capsule: one capsule overlap test %.3f ms, one 3 m floor trace %.3f ms (-1: no player capsule)"),
				*Ship.GetName(), FilterMs, OverlapMs, TraceMs);
		}

		// Every other body of the ship moves with each hull move (a send each); overlap generators query the scene each move.
		TInlineComponentArray<UPrimitiveComponent*> Primitives(&Ship);
		int32 Bodies = 0;
		int32 BodyShapes = 0;
		int32 InstanceBodies = 0;
		FString Heavy;
		FString Overlappers;
		int32 OverlapCount = 0;
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive || Primitive == Root)
			{
				continue;
			}
			if (Primitive->IsPhysicsStateCreated())
			{
				int32 Shapes = 0;
				const FString Text = DescribeShapes(*Primitive, Shapes);
				++Bodies;
				BodyShapes += Shapes;
				if (const UInstancedStaticMeshComponent* Instanced = Cast<UInstancedStaticMeshComponent>(Primitive))
				{
					InstanceBodies += Instanced->InstanceBodies.Num();
				}
				if (Shapes >= 64)
				{
					Heavy += FString::Printf(TEXT(" %s[%s]"), *Primitive->GetName(), *Text);
				}
			}
			if (Primitive->GetGenerateOverlapEvents() && Primitive->IsQueryCollisionEnabled())
			{
				++OverlapCount;
				if (OverlapCount <= 12)
				{
					Overlappers += FString::Printf(TEXT(" %s[%s, object %s, r %.0f m, WorldDynamic %s, Pawn %s]"), *Primitive->GetName(),
						*GetNameSafe(Primitive->GetClass()), *ChannelName(Primitive->GetCollisionObjectType()),
						Primitive->Bounds.SphereRadius / 100.0, ResponseName(Primitive->GetCollisionResponseToChannel(ECC_WorldDynamic)),
						ResponseName(Primitive->GetCollisionResponseToChannel(ECC_Pawn)));
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] collision report %s: %d other bodies (%d simple shapes, %d instance bodies)%s%s; ")
			TEXT("%d overlap generators (each queries the scene on every hull move):%s"), *Ship.GetName(), Bodies, BodyShapes,
			InstanceBodies, Heavy.IsEmpty() ? TEXT("") : TEXT(", heavy:"), *Heavy, OverlapCount, *Overlappers);

		TArray<AActor*> Riders;
		Ship.GetAttachedActors(Riders, true, true);
		FString RiderNames;
		for (const AActor* Rider : Riders)
		{
			RiderNames += FString::Printf(TEXT(" %s"), *GetNameSafe(Rider));
		}
		const UPrimitiveComponent* Zone = Ship.SphereCollisionComponent;
		const ECollisionChannel ZoneObject = Zone ? Zone->GetCollisionObjectType() : ECC_WorldDynamic;
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] collision report %s: riders %d:%s | gravity zone: enabled %d, object %s, overlaps %d; ")
			TEXT("the hull's response to that object %s, to GravityZone %s; the player's capsule (object %s): to that object %s, ")
			TEXT("to GravityZone %s"), *Ship.GetName(), Riders.Num(), *RiderNames,
			Zone ? static_cast<int32>(Zone->GetCollisionEnabled()) : -1, *ChannelName(ZoneObject),
			Zone && Zone->GetGenerateOverlapEvents() ? 1 : 0, ResponseName(Root->GetCollisionResponseToChannel(ZoneObject)),
			ResponseName(Root->GetCollisionResponseToChannel(ECC_GameTraceChannel2)),
			PlayerCapsule ? *ChannelName(PlayerCapsule->GetCollisionObjectType()) : TEXT("-"),
			PlayerCapsule ? ResponseName(PlayerCapsule->GetCollisionResponseToChannel(ZoneObject)) : TEXT("-"),
			PlayerCapsule ? ResponseName(PlayerCapsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2)) : TEXT("-"));
	}

	void CollisionReport(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		const bool bAll = Args.Contains(TEXT("all"));
		const APlayerController* Player = World->GetFirstPlayerController();
		const APawn* PlayerPawn = Player ? Player->GetPawn() : nullptr;
		const ACharacter* Walker = Cast<ACharacter>(PlayerPawn);
		const UCapsuleComponent* Capsule = Walker ? Walker->GetCapsuleComponent() : nullptr;
		int32 Reported = 0;
		for (TActorIterator<ASpaceship> It(World); It; ++It)
		{
			ASpaceship* Ship = *It;
			const UPrimitiveComponent* Root = Ship ? Cast<UPrimitiveComponent>(Ship->GetRootComponent()) : nullptr;
			if (!Root)
			{
				continue;
			}
			// Without "all": the ship the player flies or rides, and every ship whose hull is within 300 m of him.
			const bool bNear = PlayerPawn && (PlayerPawn == Ship || PlayerPawn->GetAttachParentActor() == Ship
				|| FVector::Dist(PlayerPawn->GetActorLocation(), Root->Bounds.Origin) <= Root->Bounds.SphereRadius + 30000.0);
			if (bAll || bNear)
			{
				ReportShip(*Ship, Capsule);
				++Reported;
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] collision report: %d ships (aps.Ship.CollisionReport all: every ship); ")
			TEXT("aps.Ship.HullBodySyncOnce %d"), Reported, CVarHullBodySyncOnce.GetValueOnGameThread());
	}

	FAutoConsoleCommandWithWorldAndArgs CollisionReportCommand(
		TEXT("aps.Ship.CollisionReport"),
		TEXT("Rio 09.10 (collision cost): logs what every hull move of the ship the player flies or rides (and of ships within ")
		TEXT("300 m; 'all' for every ship) costs the game thread: the hull body's shapes, the other bodies that move with it, ")
		TEXT("the overlap generators that query the scene on each move, the riders, the gravity zone's channels. Read only."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CollisionReport));
}
