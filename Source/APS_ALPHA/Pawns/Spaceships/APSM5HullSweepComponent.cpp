#include "APSM5HullSweepComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsObjectExternalInterface.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "Physics/GenericPhysicsInterface.h"
#include "Chaos/ImplicitObject.h"
#include "ChaosInterfaceWrapperCore.h"
#include "HAL/IConsoleManager.h"

namespace
{
    TAutoConsoleVariable<int32> CVarM5SpatialSweep(TEXT("aps.M5.SpatialSweep"), 1,
        TEXT("Exact conservative hull sweep for opt-in M5 components; 0 uses engine sweep."));
    // Rio 06.10 (perf R2): A/B switch for the deduped ignore list in TryMoveHull; 0 = old per-shape add.
    TAutoConsoleVariable<int32> CVarM5SweepUniqueIgnore(TEXT("aps.M5.SweepUniqueIgnore"), 1,
        TEXT("1 adds each shape query-filter actor id to the M5 hull sweep ignore list once; 0 adds one entry per shape (old)."));

    struct FNode
    {
        FBox Bounds = FBox(ForceInit);
        int32 Begin = 0, End = 0, Left = INDEX_NONE, Right = INDEX_NONE;
    };
}

struct FAPSM5HullSweepTree
{
    TWeakObjectPtr<UStaticMesh> Mesh;
    const void* PhysicsObject = nullptr;
    const void* FirstShape = nullptr;
    FVector Scale = FVector::ZeroVector;
    int32 SourceShapeCount = 0;
    TArray<int32> Indices;
    TArray<FBox> ShapeBounds;
    TArray<FNode> Nodes;

    int32 BuildNode(int32 Begin, int32 End)
    {
        FNode Node;
        Node.Begin = Begin; Node.End = End;
        for (int32 I = Begin; I < End; ++I) Node.Bounds += ShapeBounds[Indices[I]];
        const int32 Index = Nodes.Add(Node);
        if (End - Begin > 8)
        {
            const FVector Extent = Node.Bounds.GetExtent();
            const int32 Axis = Extent.X >= Extent.Y && Extent.X >= Extent.Z ? 0 : Extent.Y >= Extent.Z ? 1 : 2;
            MakeArrayView(Indices.GetData() + Begin, End - Begin).Sort([this, Axis](int32 A, int32 B)
                { return ShapeBounds[A].GetCenter()[Axis] < ShapeBounds[B].GetCenter()[Axis]; });
            const int32 Mid = (Begin + End) / 2;
            const int32 Left = BuildNode(Begin, Mid), Right = BuildNode(Mid, End);
            Nodes[Index].Left = Left; Nodes[Index].Right = Right;
        }
        return Index;
    }
};

UAPSM5HullSweepComponent::UAPSM5HullSweepComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UAPSM5HullSweepComponent::~UAPSM5HullSweepComponent() = default;

bool UAPSM5HullSweepComponent::TestMoveHull(const FVector& Delta, FHitResult& OutHit)
{
    ASpaceship* Ship = Cast<ASpaceship>(GetOwner());
    return Ship && TryMoveHull(Ship->GetSpaceshipHull(), Delta, OutHit);
}

FString UAPSM5HullSweepComponent::GetSweepDiagnostics() const
{
    return FString::Printf(TEXT("shapes=%d broad=%d exact=%d milliseconds=%.4f"),
        LastShapeCount, LastBroadPhaseQueries, LastExactShapeQueries, LastMilliseconds);
}

bool UAPSM5HullSweepComponent::TryMoveHull(UStaticMeshComponent* Hull, const FVector& Delta, FHitResult& OutHit)
{
    // Event-generating or simulated bodies keep the engine's complete event/physics semantics.
    ASpaceship* Ship = Cast<ASpaceship>(GetOwner());
    UWorld* World = GetWorld();
    if (!CVarM5SpatialSweep.GetValueOnGameThread() || !Ship || !Hull || !World
        || Ship->GetRootComponent() != Hull || Hull->IsSimulatingPhysics()
        || Hull->GetGenerateOverlapEvents() || !Hull->IsQueryCollisionEnabled() || Delta.IsNearlyZero()) return false;

    const double Started = FPlatformTime::Seconds();
    LastBroadPhaseQueries = LastExactShapeQueries = 0;
    const FTransform Pose = Hull->GetComponentTransform();
    const FVector Start = Pose.GetLocation();
    const FQuat Rotation = Pose.GetRotation();
    TArray<Chaos::FPhysicsObject*> Objects = Hull->GetAllPhysicsObjects();
    if (Objects.Num() != 1) return false; // Never simplify welded/multi-body assemblies.
    FLockedReadPhysicsObjectExternalInterface Interface = FPhysicsObjectExternalInterface::LockRead(Objects);
    TArray<Chaos::FShapeInstanceProxy*> Shapes = Interface->GetAllThreadShapes(Objects);
    if (Shapes.IsEmpty() || Interface->AreAllDisabled(Objects)) return false;

    if (!Tree || Tree->Mesh != Hull->GetStaticMesh() || Tree->PhysicsObject != Objects[0]
        || Tree->FirstShape != Shapes[0] || Tree->SourceShapeCount != Shapes.Num()
        || !Tree->Scale.Equals(Pose.GetScale3D(), 1.e-7))
    {
        Tree = MakeShared<FAPSM5HullSweepTree>();
        Tree->Mesh = Hull->GetStaticMesh(); Tree->PhysicsObject = Objects[0];
        Tree->FirstShape = Shapes[0]; Tree->SourceShapeCount = Shapes.Num();
        Tree->Scale = Pose.GetScale3D(); Tree->ShapeBounds.SetNum(Shapes.Num());
        for (int32 I = 0; I < Shapes.Num(); ++I)
        {
            FPhysicsShapeHandle Handle{Shapes[I], nullptr};
            const auto& Geometry = Handle.GetGeometry();
            if (!Geometry.IsConvex()) continue; // Same exclusion as UWorld::ComponentSweepMulti.
            if (!Geometry.HasBoundingBox()) { Tree.Reset(); return false; }
            const auto Box = Geometry.BoundingBox();
            Tree->ShapeBounds[I] = FBox(FVector(Box.Min()), FVector(Box.Max())).ExpandBy(0.1);
            Tree->Indices.Add(I);
        }
        if (Tree->Indices.IsEmpty()) { Tree.Reset(); return false; }
        Tree->BuildNode(0, Tree->Indices.Num());
    }
    LastShapeCount = Tree->Indices.Num();

    FCollisionQueryParams Params(SCENE_QUERY_STAT(M5SpatialHullSweep), Hull->bTraceComplexOnMove, Ship);
    Params.AddIgnoredActors(Hull->GetMoveIgnoreActors());
    Params.AddIgnoredComponents(Hull->GetMoveIgnoreComponents());
    Params.bIgnoreTouches = true;
    // Match engine exclusion of all actor IDs carried by welded shape query filters.
    // Rio 06.10 (perf R2): on a non-welded hull every shape carries the ship's own id, so the
    // per-shape add pushed ~13.5k duplicates and the scene-query PreFilter scanned that list
    // linearly for every foreign candidate shape (terrain, HQ, pads). Add each id once; the
    // filter only tests membership, so hits are identical. aps.M5.SweepUniqueIgnore 0 = old loop.
    const bool bUniqueIgnore = CVarM5SweepUniqueIgnore.GetValueOnGameThread() != 0;
    uint32 LastIgnoredId = Ship->GetUniqueID(); // Already in the list via Params(..., Ship).
    for (Chaos::FShapeInstanceProxy* Shape : Shapes)
    {
        const uint32 Id = ChaosInterface::GetQueryFilterData(*Shape).Word0;
        if (!bUniqueIgnore) { Params.AddIgnoredActor(Id); continue; }
        if (Id == LastIgnoredId) continue;
        LastIgnoredId = Id;
        if (!Params.GetIgnoredActors().Contains(Id)) Params.AddIgnoredActor(Id);
    }
    const ECollisionChannel Channel = Hull->GetCollisionObjectType();
    const FCollisionResponseParams Responses(Hull->GetCollisionResponseToChannels());
    TArray<int32, TInlineAllocator<64>> Stack;
    Stack.Add(0);
    TArray<FHitResult> Hits;
    OutHit = FHitResult();
    float MostOpposed = UE_BIG_NUMBER;
    const FVector Direction = Delta.GetSafeNormal();
    const auto* DistanceCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("p.HitDistanceTolerance"));
    const auto* DotCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("p.InitialOverlapTolerance"));
    const float DistanceTolerance = DistanceCVar ? DistanceCVar->GetFloat() : 0.f;
    const float DotTolerance = DotCVar ? DotCVar->GetFloat() : 0.f;
    while (!Stack.IsEmpty())
    {
        const FNode& Node = Tree->Nodes[Stack.Pop(EAllowShrinking::No)];
        // Geometry bounds already contain Chaos per-shape transforms and body scale.
        const FVector Center = Start + Rotation.RotateVector(Node.Bounds.GetCenter());
        ++LastBroadPhaseQueries;
        if (!World->SweepTestByChannel(Center, Center + Delta, Rotation, Channel,
            FCollisionShape::MakeBox(Node.Bounds.GetExtent()), Params, Responses)) continue;
        if (Node.Left != INDEX_NONE)
        {
            Stack.Add(Node.Right); Stack.Add(Node.Left); continue;
        }
        for (int32 I = Node.Begin; I < Node.End; ++I)
        {
            FPhysicsShapeHandle Handle{Shapes[Tree->Indices[I]], nullptr};
            auto Geometry = FPhysicsInterface::GetGeometryCollection(Handle);
            Hits.Reset(); ++LastExactShapeQueries;
            FPhysicsInterface::GeomSweepMulti(World, Geometry, Rotation, Hits, Start, Start + Delta,
                Channel, Params, Responses);
            for (const FHitResult& Hit : Hits)
            {
                if (!Hit.bBlockingHit) continue;
                if ((Hit.bStartPenetrating || Hit.Distance < DistanceTolerance)
                    && FVector::DotProduct(Hit.ImpactNormal, Direction) > DotTolerance) continue;
                if (Hit.bStartPenetrating)
                {
                    const float Dot = FVector::DotProduct(Hit.ImpactNormal, Delta);
                    if (!OutHit.bStartPenetrating || Dot < MostOpposed)
                        { OutHit = Hit; MostOpposed = Dot; }
                }
                else if (!OutHit.bStartPenetrating && (!OutHit.bBlockingHit || Hit.Time < OutHit.Time)) OutHit = Hit;
            }
        }
    }
    Interface.Release();
    float Fraction = 1.f;
    if (OutHit.bBlockingHit)
    {
        const float Distance = Delta.Size();
        const float PullBack = FMath::Clamp(0.1f, 0.1f / Distance, 1.f / Distance) + 0.001f;
        OutHit.Time = FMath::Clamp(OutHit.Time - PullBack, 0.f, 1.f);
        Fraction = OutHit.Time;
    }
    Ship->AddActorWorldOffset(Delta * Fraction, false, nullptr, ETeleportType::None);
    if (OutHit.bBlockingHit) Hull->DispatchBlockingHit(*Ship, OutHit);
    LastMilliseconds = (FPlatformTime::Seconds() - Started) * 1000.;
    return true;
}
