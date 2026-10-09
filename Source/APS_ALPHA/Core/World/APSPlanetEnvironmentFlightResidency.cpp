#include "APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFlightPolicy.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

// APSPlanetEnvironmentFirstBuild.cpp (aps.Surface.FirstWaveAckNoise): the hidden replacement is a fresh sliced root too.
namespace APSFirstBuildPrivate { void AcknowledgeFirstWaveNoise(AWorldScapeRoot* Root, const TCHAR* Path); }

namespace
{
    TAutoConsoleVariable<int32> CVarFlightResidency(TEXT("aps.Surface.FlightResidency"), 0,
        TEXT("Build an atomic low-density transit replacement, then retire near meshes. Diagnostic until rendered travel validation."));
    TAutoConsoleVariable<int32> CVarFlightBaseLodsPerFrame(TEXT("aps.Surface.FlightBaseLodsPerFrame"), 1,
        TEXT("Base components initialized per frame on the hidden flight replacement (1..4). 0 = legacy synchronous diagnostic control."));
    const FName TransitTag(TEXT("APS.Surface.Transit"));
}

void APlanetarySurfaceGenerator::ExchangeRuntimeSurface(APlanetarySurfaceGenerator& Other)
{
    check(IsInGameThread());
    check(PlanetaryBody == Other.PlanetaryBody && bOwnsWorldScapeRootInstance && Other.bOwnsWorldScapeRootInstance);
    Swap(WorldScapeRootInstance, Other.WorldScapeRootInstance);
    Swap(bOwnsWorldScapeRootInstance, Other.bOwnsWorldScapeRootInstance);
    Swap(bSurfaceProfileApplied, Other.bSurfaceProfileApplied);
    Swap(AppliedSurfaceProfileSignature, Other.AppliedSurfaceProfileSignature);
    Swap(ResolvedSurfaceProfile, Other.ResolvedSurfaceProfile);
    Swap(ResolvedNoiseInstance, Other.ResolvedNoiseInstance);
    Swap(ResolvedTerrainMaterialInstance, Other.ResolvedTerrainMaterialInstance);
    Swap(ResolvedOceanMaterialInstance, Other.ResolvedOceanMaterialInstance);
    Swap(FinalizedWaterMaterialRoot, Other.FinalizedWaterMaterialRoot);
    Swap(FinalizedWaterMaterialProfileSignature, Other.FinalizedWaterMaterialProfileSignature);
    // Legacy cached aliases must follow their root too, otherwise the old root's
    // materials/noise remain strongly reachable after the retirement fence.
    Swap(MoonLikeNoise, Other.MoonLikeNoise); Swap(LavaWorldNoise, Other.LavaWorldNoise);
    Swap(SelenaeNoise, Other.SelenaeNoise); Swap(SelenaeMetalNoise, Other.SelenaeMetalNoise);
    Swap(EarthLikeNoise, Other.EarthLikeNoise); Swap(EarthNoise, Other.EarthNoise);
    Swap(TerraNoise, Other.TerraNoise); Swap(IceWorldNoise, Other.IceWorldNoise);
    Swap(TerraDesert, Other.TerraDesert); Swap(TerraForestNoise, Other.TerraForestNoise);
    Swap(MI_Terra, Other.MI_Terra); Swap(MI_Selenae, Other.MI_Selenae);
    Swap(MI_Magma, Other.MI_Magma); Swap(MI_Planetary_Ocean, Other.MI_Planetary_Ocean);
    Swap(MI_Lava_Ocean, Other.MI_Lava_Ocean);
}

void UAPSPlanetEnvironmentStreamingSubsystem::CancelFlightReplacement()
{
    if (IsValid(FlightReplacement) && !bFlightReplacementRetiring)
    {
        FlightReplacement->UnloadWorldScapeRoot();
        bFlightReplacementRetiring = true;
    }
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateFlightResidency(float DeltaTime)
{
    // One extra producer globally. Cancellation and retirement drain the existing
    // worker batch without joining it or allocating another replacement alongside it.
    if (IsValid(FlightReplacement) && bFlightReplacementRetiring)
    {
        if (!IsValid(FlightReplacement->WorldScapeRootInstance))
        {
            UE_LOG(LogTemp, Display, TEXT("[APS.FlightResidency] retirement complete; old root destroyed through native worker drain"));
            FlightReplacement->Destroy();
            FlightReplacement = nullptr;
            FlightReplacementBody.Reset();
            bFlightReplacementRetiring = false;
        }
        else return;
    }
    APlanetaryBody* Body = ActiveBody.Get();
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* Observer = PC ? PC->GetPawn() : nullptr;
    APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
    AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
    if (CVarFlightResidency.GetValueOnGameThread() == 0 && !IsValid(FlightReplacement)
        && (!IsValid(Root) || !Root->ActorHasTag(TransitTag))) return;
    const APlanet* Planet = Cast<APlanet>(Body);
    const bool bEligible = IsValid(Root) && IsValid(Observer) && Surface->bOwnsWorldScapeRootInstance
        && Body->bWorldScapeSurfaceReady && Body->WorldScapePresentationScale == 1.0
        && !(Planet && Planet->IsManual) && !Surface->IsSurfaceProfileApplyPending()
        && !Surface->bPendingWorldScapeUnload
        && !Root->bGenerateFoliages && !Root->bFlatWorld;
    if (!bEligible)
    {
        CancelFlightReplacement(); MotionBody.Reset(); MotionObserver.Reset(); TransitDwell = 0; return;
    }
    const FVector Relative = Observer->GetActorLocation() - Body->GetActorLocation();
    FVector Velocity = FVector::ZeroVector;
    if (MotionBody.Get() == Body && MotionObserver.Get() == Observer && DeltaTime > 0 && DeltaTime < 2.0f)
        Velocity = (Relative - PreviousRelativePosition) / double(DeltaTime);
    else TransitDwell = 0;
    MotionBody = Body; MotionObserver = Observer; PreviousRelativePosition = Relative;
    const bool bTransit = Root->ActorHasTag(TransitTag);
    const bool bWantTransit = CVarFlightResidency.GetValueOnGameThread() != 0
        && APSWorldScapeFlightPolicy::WantsTransit(Relative, Velocity, Root->PlanetScale,
            bTransit || (IsValid(FlightReplacement) && bFlightReplacementTransit));
    TransitDwell = bWantTransit ? FMath::Min(TransitDwell + DeltaTime, 2.0f) : 0;

    if (IsValid(FlightReplacement))
    {
        AWorldScapeRoot* Candidate = FlightReplacement->WorldScapeRootInstance;
        // A near prefetch survives one noisy/long frame. Cancel it only after a
        // sustained new transit request; a far build cancels immediately on approach.
        const bool bReverse = bWantTransit != bFlightReplacementTransit
            && (bFlightReplacementTransit || TransitDwell >= 1.5f);
        if (FlightReplacementBody.Get() != Body || bReverse
            || !IsValid(Candidate) || FlightReplacement->AppliedSurfaceProfileSignature != Surface->AppliedSurfaceProfileSignature)
        { CancelFlightReplacement(); return; }
        Candidate->OverridedPlayerPosition = Observer->GetActorLocation();
        if (!Candidate->init && !Candidate->IsActorTickEnabled())
        {
            // No generation workers may observe a partial base set. Update the
            // normal first-Tick coordinate cache explicitly while Tick is held.
            Candidate->PlanetLocation = Candidate->GetActorLocation();
            Candidate->PlanetScaleCode = Candidate->PlanetScale;
            if (Candidate->AdvanceBaseMeshInitialization(FMath::Clamp(CVarFlightBaseLodsPerFrame.GetValueOnGameThread(), 1, 4)))
            {
                APSFirstBuildPrivate::AcknowledgeFirstWaveNoise(Candidate, TEXT("flight residency"));
                Candidate->SetActorTickEnabled(true);
            }
            return; // full payload/coverage still has to settle after native Tick.
        }
        if (!Candidate->WorldScapeLodInGeneration.IsEmpty()) Candidate->CheckForLodGeneration();
        if (!Body->HasWorldScapeReplacementCoverage(Candidate)
            || !FlightReplacement->FinalizeStableWaterMaterial()) return;
        if (!Surface->IsSurfaceProfileCurrent(Body)) { CancelFlightReplacement(); return; }
        // No placeholder frame and no partially built replacement. Visibility and
        // ownership change together; collision binding is repaired in this tick.
        // Placement anchors are independent actors, not children of a root.
        // Preserve their identity across the swap so refresh/release finds them
        // on the new owner instead of leaking duplicate base/pad/route actors.
        for (AActor* Anchor : Root->CollisionDependantActor)
            if (IsValid(Anchor)) Candidate->CollisionDependantActor.AddUnique(Anchor);
        ClearGameplayCollisionAnchor();
        Surface->ExchangeRuntimeSurface(*FlightReplacement);
        Candidate->SetActorHiddenInGame(false);
        Root->SetActorHiddenInGame(true);
        Root->SetActorEnableCollision(false);
        ApplyGameplayObserverContract(Candidate, Observer);
        Candidate->SetActorEnableCollision(!bFlightReplacementTransit);
        Body->RefreshWorldScapeSurfaceVisibility();
        UE_LOG(LogTemp, Display, TEXT("[APS.FlightResidency] commit body=%s transit=%d old=%s new=%s resolution=%d->%d heightKm=%.3f"),
            *Body->GetName(), bFlightReplacementTransit, *Root->GetName(), *Candidate->GetName(),
            Root->LodResolution, Candidate->LodResolution, (Relative.Size()-Candidate->PlanetScale)/100000.0);
        CancelFlightReplacement();
        return;
    }
    if (bTransit == bWantTransit || (bWantTransit && TransitDwell < 1.5f)) return;
    if (!Surface->IsSurfaceProfileCurrent(Body)) return;
    // Allocation/profile only, hidden. The new root owns independent immutable
    // noise/MIDs, so its worker never races the outgoing root's profile/lifetime.
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.Owner = Body;
    FlightReplacement = GetWorld()->SpawnActor<APlanetarySurfaceGenerator>(Params);
    if (!IsValid(FlightReplacement)) return;
    FlightReplacementBody = Body;
    bFlightReplacementTransit = bWantTransit;
    FlightReplacement->SurfaceProfileCatalog = Surface->SurfaceProfileCatalog;
    if (!FlightReplacement->CreateRuntimeWorldScapeRoot(Body))
    { CancelFlightReplacement(); return; }
    FlightReplacement->ApplySurfaceProfile(Body);
    AWorldScapeRoot* Candidate = FlightReplacement->WorldScapeRootInstance;
    if (!FlightReplacement->IsSurfaceProfileCurrent(Body)) { CancelFlightReplacement(); return; }
    if (bWantTransit)
    {
        // Keep each ring's span and the terrain/liquid spherical lattice identical.
        const int32 Resolution = FMath::Min(64, Candidate->LodResolution);
        Candidate->TriangleSize *= float(Candidate->LodResolution) / Resolution;
        Candidate->LodResolution = Resolution;
        Candidate->OceanLodResolution = Resolution;
        Candidate->OceanTriangleSize = Candidate->TriangleSize;
        Candidate->Tags.AddUnique(TransitTag);
    }
    Candidate->bGenerateFoliages = false;
    Candidate->bOverridePlayerPosition = true;
    Candidate->OverridedPlayerPosition = Observer->GetActorLocation();
    Candidate->DistanceToFreezeGeneration = 0;
    // Fresh roots have no geometry yet. Acknowledge the configured profile now:
    // the native first Tick builds its base mesh, then calls CheckForRegenerate.
    // Stale Prev_* values otherwise cause a second complete build in that tick.
    // This updates only the empty candidate's parameter/material cache; it does
    // not set init, generate geometry, or touch the outgoing root/workers.
    if (!Candidate->WorldScapeLod.IsEmpty() || !Candidate->WorldScapeLodOcean.IsEmpty()
        || !Candidate->WorldScapeLodInGeneration.IsEmpty())
    { CancelFlightReplacement(); return; }
    Candidate->CheckForRegenerate();
    FlightReplacement->SpawnWorldScapeRoot();
    Candidate->SetActorHiddenInGame(true);
    if (CVarFlightBaseLodsPerFrame.GetValueOnGameThread() > 0)
        Candidate->SetActorTickEnabled(false);
    Candidate->SetActorEnableCollision(false);
    Candidate->bGenerateCollision = false;
#if WITH_EDITOR
    Candidate->bGenerateCollisionInEditor = false;
#endif
    UE_LOG(LogTemp, Display, TEXT("[APS.FlightResidency] preparing body=%s transit=%d resolution=%d heightKm=%.3f oldStillVisible=1"),
        *Body->GetName(), bWantTransit, Candidate->LodResolution, (Relative.Size()-Candidate->PlanetScale)/100000.0);
}
