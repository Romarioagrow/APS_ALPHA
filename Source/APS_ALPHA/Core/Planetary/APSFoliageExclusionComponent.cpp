#include "APSFoliageExclusionComponent.h"
#include "APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "WorldScapeRoot.h"

namespace
{
TAutoConsoleVariable<int32> ExclusionMode(TEXT("aps.WorldScapeFoliage.StructureExclusion"), 0,
    TEXT("APS scatter structure/access exclusion. 0 until rendered validation; 1 opt-in."));
bool BelongsTo(const AActor* Actor, const AActor* Body)
{
    for (int32 Depth = 0; Actor && Depth < 16; ++Depth, Actor = Actor->GetAttachParentActor())
        if (Actor == Body) return true;
    return false;
}
}

UAPSFoliageExclusionComponent::UAPSFoliageExclusionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    // No polling delay for newly published sectors: run after native root tick,
    // before walking proxies/render. Expensive footprint discovery is throttled.
}

bool UAPSFoliageExclusionComponent::Intersects(const FBox& MeshBox,
    const FTransform& InstanceToRoot, const FVolume& Volume)
{
    if (!MeshBox.IsValid || !Volume.LocalBox.IsValid || InstanceToRoot.ContainsNaN()
        || Volume.ToRoot.ContainsNaN() || Volume.ToRoot.GetScale3D().GetAbsMin() <= SMALL_NUMBER) return false;
    // Conservative oriented footprint test. Transform the eight corners exactly;
    // FTransform composition can lose shear with rotated nonuniform actor scale.
    FBox Local(ForceInit);
    for (int32 I = 0; I < 8; ++I)
    {
        const FVector Corner(I & 1 ? MeshBox.Max.X : MeshBox.Min.X,
            I & 2 ? MeshBox.Max.Y : MeshBox.Min.Y, I & 4 ? MeshBox.Max.Z : MeshBox.Min.Z);
        Local += Volume.ToRoot.InverseTransformPosition(InstanceToRoot.TransformPosition(Corner));
    }
    return Local.Intersect(Volume.LocalBox);
}

bool UAPSFoliageExclusionComponent::Excludes(const FBox& Box, const FTransform& ToWorld) const
{
    if (!GetOwner()) return false;
    const FTransform ToRoot = ToWorld.GetRelativeTransform(GetOwner()->GetActorTransform());
    for (const auto& V : Volumes) if (Intersects(Box, ToRoot, V)) return true;
    return false;
}

int32 UAPSFoliageExclusionComponent::GetSuppressedCount() const
{
    int32 Count = 0;
    for (const auto& Pair : Sources) if (Pair.Key.IsValid()) Count += Pair.Value.Removed.Num();
    return Count;
}

void UAPSFoliageExclusionComponent::RefreshFootprints()
{
    TArray<FVolume> Next;
    auto* Root = GetOwner();
    auto* Body = Root ? Root->GetAttachParentActor() : nullptr;
    TArray<AActor*> Bases, Pads;
    if (ExclusionMode.GetValueOnGameThread() == 1 && Body)
    {
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            auto* Actor = *It;
            const bool Base = Actor->ActorHasTag(TEXT("APS.Civilization.Base")) || Actor->IsA<AColony>();
            const bool Pad = Actor->ActorHasTag(TEXT("APS.Civilization.LandingPad"));
            if ((!Base && !Pad && !Actor->ActorHasTag(TEXT("APS.Colony.Module")))
                || Actor->IsHidden() || !BelongsTo(Actor, Body)) continue;
            FBox Box = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Actor);
            if (!Box.IsValid || Next.Num() >= MaxVolumes) continue;
            const FVector Scale = Actor->GetActorScale3D().GetAbs().ComponentMax(FVector(.001));
            // Two metres of access clearance; project five metres below the
            // visible foundation. Do not include gravity/interaction volumes.
            Box = Box.ExpandBy(FVector(200., 200., 200.) / Scale);
            Box.Min.Z -= 500. / Scale.Z;
            Next.Add({Actor->GetActorTransform().GetRelativeTransform(Root->GetActorTransform()), Box});
            if (Base) Bases.Add(Actor);
            if (Pad) Pads.Add(Actor);
        }
        // Same local base->landing-pad convention as placement RouteTargets.
        // One nearest base per pad, limited to 1 km; never connect two planets
        // or clear a long inter-colony strip. Footprints cover the actual ramp.
        for (auto* Pad : Pads)
        {
            AActor* Base = nullptr;
            double Best = FMath::Square(100000.);
            for (auto* Candidate : Bases)
            {
                const double D = FVector::DistSquared(Pad->GetActorLocation(), Candidate->GetActorLocation());
                if (D < Best) { Best = D; Base = Candidate; }
            }
            if (!Base || Best < 1. || Next.Num() >= MaxVolumes) continue;
            const FVector A = Base->GetActorLocation(), B = Pad->GetActorLocation();
            const FVector Up = Base->GetActorUpVector();
            const FTransform World(FRotationMatrix::MakeFromXZ((B - A).GetSafeNormal(), Up).ToQuat(), (A + B) * .5);
            const FVector Half(FMath::Sqrt(Best) * .5, 300., 1500.);
            Next.Add({World.GetRelativeTransform(Root->GetActorTransform()), FBox(-Half, Half)});
        }
    }
    bool Changed = Next.Num() != Volumes.Num();
    for (int32 I = 0; !Changed && I < Next.Num(); ++I)
        Changed = !Next[I].ToRoot.Equals(Volumes[I].ToRoot, .05)
            || !Next[I].LocalBox.Min.Equals(Volumes[I].LocalBox.Min, .05)
            || !Next[I].LocalBox.Max.Equals(Volumes[I].LocalBox.Max, .05);
    if (Changed) { Volumes = MoveTemp(Next); ++Revision; }
}

void UAPSFoliageExclusionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    auto* Root = Cast<AWorldScapeRoot>(GetOwner());
    if (!Root || !GetWorld()) return;
    TRACE_CPUPROFILER_EVENT_SCOPE(APSFoliageStructureExclusion);
    for (auto It = Sources.CreateIterator(); It; ++It)
        if (!It.Key().IsValid() || !It.Key()->IsRegistered()) It.RemoveCurrent();
    PendingCount = 0;
    if (Root->IsHidden() || !Root->IsActorTickEnabled() || !Root->bGenerateFoliages) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextFootprints) { RefreshFootprints(); NextFootprints = Now + .5; }
    if (Volumes.IsEmpty() && Sources.IsEmpty()) return;
    int32 Budget = MaxInstancesPerTick;
    TInlineComponentArray<UInstancedStaticMeshComponent*> Components(Root);
    for (auto* ISM : Components)
    {
        if (!IsValid(ISM) || !ISM->IsRegistered() || !ISM->GetStaticMesh()
            || !ISM->GetStaticMesh()->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root) + TEXT("/"))) continue;
        auto& State = Sources.FindOrAdd(ISM);
        const int32 Count = ISM->GetInstanceCount();
        // Native classic HISM sectors are immutable after AddInstances, then
        // ClearInstances+DestroyComponent on retirement. FCP is disabled by APS.
        // If another owner rewrites the component, discard saved removals rather
        // than resurrect stale indices/transforms into a different generation.
        if (State.Count != Count) { State.Removed.Reset(); State.Revision = 0; State.Count = Count; }
        if (State.Revision == Revision) continue;
        const int32 Work = Count + State.Removed.Num();
        if (Work > Budget) { ++PendingCount; continue; }
        Budget -= Work;
        const FBox Box = ISM->GetStaticMesh()->GetBoundingBox();
        const FTransform Component = ISM->GetComponentTransform();
        TArray<FTransform> Restored;
        for (int32 I = State.Removed.Num() - 1; I >= 0; --I)
            if (!Excludes(Box, State.Removed[I] * Component))
            { Restored.Add(State.Removed[I]); State.Removed.RemoveAtSwap(I, 1, EAllowShrinking::No); }
        TArray<int32> Remove;
        TArray<FTransform> Removed;
        for (int32 I = 0; I < Count; ++I)
        {
            FTransform Local;
            if (ISM->GetInstanceTransform(I, Local, false) && Excludes(Box, Local * Component))
            { Remove.Add(I); Removed.Add(Local); }
        }
        // One HISM tree invalidation per component, no per-instance physics,
        // no zero-scale placeholders, no native worker/queue writes.
        if (!Remove.IsEmpty() && ISM->RemoveInstances(Remove)) State.Removed.Append(Removed);
        if (!Restored.IsEmpty()) ISM->AddInstances(Restored, false);
        State.Count = ISM->GetInstanceCount(); State.Revision = Revision;
    }
}
