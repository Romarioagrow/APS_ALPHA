#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.h"
#include "WorldScapeRoot.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace APSFoliageFlightProbe
{
inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageFlight")); }
inline bool Performance() { return Requested() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageFlightPerf")); }
inline bool Off() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageFlightOff")); }
inline bool Reentry() { return Requested() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageFlightReentry")); }
inline bool SurfaceScatter() { return Requested() && FParse::Param(FCommandLine::Get(), TEXT("APSProbeSurfaceScatterFlight")); }
inline int32 ExpectedCollision() { int32 Mode = -1; FParse::Value(FCommandLine::Get(), TEXT("APSProbeWalkingCollisionExpected="), Mode); return Mode; }
inline int32 ExpectedExclusion() { int32 Mode = -1; FParse::Value(FCommandLine::Get(), TEXT("APSProbeStructureExclusionExpected="), Mode); return Mode; }
struct FProbe
{
    struct FFrame { double T, HeightKm, Dt, Game, Render, GPU, RAM; int32 Instances, Components, Workers, ActiveProxies, AllocatedProxies; double PawnSpeed; bool Grounded; int32 Excluded, Volumes, PendingExclusion; };
    TArray<FFrame> Frames;
    double NextMemorySample = 0, RAM = 0;
    bool SawNearInstances = false, SawRetired = false, SawReturnInstances = false, SawReturnRetired = false, Unsafe = false;
    bool SawNearCollision = false, SawReturnCollision = false, UnexpectedCollision = false;
    bool SawNearExclusion = false, SawReturnExclusion = false, UnexpectedExclusion = false;
    bool Tick(AWorldScapeRoot* Root, double T, double HeightKm, FString& Error)
    {
        if (!Root || Frames.Num() >= 32768) { Error = TEXT("Foliage flight lost root or exceeded frame bound"); return false; }
        int32 Instances = 0, Components = 0, Workers = 0, ActiveProxies = 0, AllocatedProxies = 0;
        int32 Excluded = 0, Volumes = 0, PendingExclusion = 0;
        const auto* PC = Root->GetWorld()->GetFirstPlayerController();
        const auto* Pawn = PC ? PC->GetPawn() : nullptr;
        const auto* Walker = Cast<ACharacter>(Pawn);
        const bool Grounded = Walker && Walker->GetCharacterMovement() && Walker->GetCharacterMovement()->IsMovingOnGround();
        const double PawnSpeed = Pawn ? Pawn->GetVelocity().Size() : -1.;
        for (TActorIterator<AWorldScapeRoot> It(Root->GetWorld()); It; ++It)
        {
            auto* R = *It;
            if (R->GetOwner() != Root->GetOwner()) continue;
            Workers += FAPSWorldScapeFoliagePolicy::HasPendingNativeWorker(R);
            if (auto* Exclusion = R->FindComponentByClass<UAPSFoliageExclusionComponent>())
            {
                Excluded += Exclusion->GetSuppressedCount(); Volumes += Exclusion->GetVolumeCount();
                PendingExclusion += Exclusion->GetPendingCount();
            }
            if(auto* Collision=R->FindComponentByClass<UAPSFoliageCollisionComponent>())
            {
                ActiveProxies+=Collision->GetActiveProxyCount();
                AllocatedProxies+=Collision->GetAllocatedProxyCount();
                Unsafe|=Collision->GetActiveProxyCount()>UAPSFoliageCollisionComponent::MaximumProxies;
            }
            TArray<UInstancedStaticMeshComponent*> Meshes; R->GetComponents(Meshes);
            for (auto* M : Meshes)
            {
                if (!IsValid(M) || !M->IsRegistered()) continue;
                ++Components; Instances += M->GetInstanceCount();
                Unsafe |= M->GetCollisionEnabled() != ECollisionEnabled::NoCollision
                    || M->CastShadow || M->bAffectDistanceFieldLighting || M->bNeverDistanceCull;
            }
        }
        if (T >= NextMemorySample)
        {
            RAM = FPlatformMemory::GetStats().UsedPhysical / 1048576.; NextMemorySample = T + 1.;
            UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight] t=%.3f heightKm=%.5f instances=%d components=%d workers=%d processRAMMiB=%.1f off=%d"),
                T, HeightKm, Instances, Components, Workers, RAM, Off());
            UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight.Collision] active=%d allocated=%d pawnSpeedCm=%.3f grounded=%d rootTick=%d rootCollision=%d hidden=%d; teleported route is NOT walking"),
                ActiveProxies, AllocatedProxies, PawnSpeed, Grounded, Root->IsActorTickEnabled(), Root->GetActorEnableCollision(), Root->IsHidden());
        }
        SawNearInstances |= T >= 8. && T <= 12. && HeightKm < .1 && Instances > 0;
        SawNearExclusion |= T >= 8. && T <= 12. && HeightKm < .1 && Excluded > 0 && PendingExclusion == 0;
        SawReturnExclusion |= T >= 29. && T <= 33. && HeightKm < .1 && Excluded > 0 && PendingExclusion == 0;
        UnexpectedExclusion |= ExpectedExclusion() == 0 && Excluded != 0;
        Unsafe |= T >= 20. && HeightKm > 50. && Excluded != 0;
        SawNearCollision |= T >= 8. && T <= 12. && HeightKm < .1 && ActiveProxies > 0 && Grounded;
        SawReturnCollision |= T >= 29. && T <= 33. && HeightKm < .1 && ActiveProxies > 0 && Grounded;
        UnexpectedCollision |= ExpectedCollision() == 0 && (ActiveProxies > 0 || AllocatedProxies > 0);
        SawRetired |= T >= 20. && HeightKm > 50. && Instances == 0 && Components == 0 && Workers == 0 && AllocatedProxies == 0;
        SawReturnInstances |= T >= 29. && T <= 33. && HeightKm < .1 && Instances > 0;
        SawReturnRetired |= T >= 41. && HeightKm > 50. && Instances == 0 && Components == 0 && Workers == 0 && AllocatedProxies == 0;
        Unsafe |= T >= 20. && HeightKm > 50. && AllocatedProxies != 0;
        Frames.Add({T, HeightKm, FApp::GetDeltaTime()*1000., FPlatformTime::ToMilliseconds(GGameThreadTime),
            FPlatformTime::ToMilliseconds(GRenderThreadTime), FPlatformTime::ToMilliseconds(GGPUFrameTime), RAM, Instances, Components, Workers, ActiveProxies, AllocatedProxies, PawnSpeed, Grounded, Excluded, Volumes, PendingExclusion});
        return true;
    }
    bool Finish(FString& Error)
    {
        const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"));
        const FString Path = Directory / TEXT("FoliageFlight.csv");
        FString Csv = TEXT("t,height_km,dt_ms,game_ms,render_ms,gpu_ms,process_ram_mib_1hz,instances,components,workers,active_proxies,allocated_proxies,pawn_speed_cm_s,grounded,excluded,exclusion_volumes,pending_exclusion\n");
        bool OffHadInstances = false;
        for (const auto& F : Frames)
        {
            Csv += FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.3f,%d,%d,%d,%d,%d,%.3f,%d,%d,%d,%d\n"),
                F.T,F.HeightKm,F.Dt,F.Game,F.Render,F.GPU,F.RAM,F.Instances,F.Components,F.Workers,F.ActiveProxies,F.AllocatedProxies,F.PawnSpeed,F.Grounded,F.Excluded,F.Volumes,F.PendingExclusion);
            OffHadInstances |= F.Instances != 0 || F.Components != 0 || F.AllocatedProxies != 0;
        }
        if (IFileManager::Get().FileExists(*Path) || !IFileManager::Get().MakeDirectory(*Directory,true)
            || !FFileHelper::SaveStringToFile(Csv,*Path)) { Error = TEXT("Foliage flight evidence write failed/existed"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight] frames=%d sawNear=%d retiredAt100km=%d unsafe=%d off=%d csv=%s; process RAM sampled1Hz, no VRAM/perf acceptance without paired run; coordinate route NOT ship dynamics"),
            Frames.Num(), SawNearInstances, SawRetired, Unsafe, Off(), *Path);
        if (Frames.Num() < 100 || Unsafe || (Off() ? OffHadInstances : !SawNearInstances || !SawRetired))
        { Error = TEXT("Foliage flight failed spawn/retire/safety contract; CSV retained"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight.Return] requested=%d repopulated=%d retiredAgain=%d"), Reentry(), SawReturnInstances, SawReturnRetired);
        if (Reentry() && !Off() && (!SawReturnInstances || !SawReturnRetired))
        { Error = TEXT("Foliage reentry failed repopulate/second-retirement contract; CSV retained"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight.CollisionResult] expected=%d near=%d return=%d unexpected=%d"),
            ExpectedCollision(), SawNearCollision, SawReturnCollision, UnexpectedCollision);
        if (UnexpectedCollision || (ExpectedCollision() == 1 && (!SawNearCollision || (Reentry() && !SawReturnCollision))))
        { Error = TEXT("Collision ON/OFF route did not exercise the expected near/return physics; ordinary foliage PASS is insufficient"); return false; }
        UE_LOG(LogTemp, Display, TEXT("[APS.FoliageFlight.ExclusionResult] expected=%d near=%d return=%d unexpected=%d"), ExpectedExclusion(), SawNearExclusion, SawReturnExclusion, UnexpectedExclusion);
        if (UnexpectedExclusion || (ExpectedExclusion() == 1 && (!SawNearExclusion || (Reentry() && !SawReturnExclusion))))
        { Error = TEXT("Structure exclusion route did not exercise natural filtered instances on both approaches"); return false; }
        return true;
    }
};
}
#endif
