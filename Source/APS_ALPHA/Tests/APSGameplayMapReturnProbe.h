#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "GameFramework/PawnMovementComponent.h"
#include "EngineUtils.h"
#include "HAL/PlatformMemory.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

// Opt-in GENERATED TEST WORLD ONLY. Exercise the actual F10 widget/controller
// and production streamer with a scripted, real controlled pawn. Do not force
// surface readiness, profile, material, root lifetime or streaming selection.
// This is a rendering/lifecycle diagnostic, NOT a ship handling benchmark.
namespace APSGameplayMapReturnProbe
{
    inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeMapReturn")); }
    enum class EResult : uint8 { Pending, Done, Failed };
    enum class EPhase : uint8 { Baseline, NearMap, NearReturn, Outbound, RemoteMap, Inbound, Returned, Done };

    inline double RouteRadius(double Near, double Far, double Alpha, bool Returning)
    {
        const double T = FMath::Clamp(Alpha, 0.0, 1.0);
        const double A = Returning ? 1.0 - T : T;
        return FMath::Exp(FMath::Lerp(FMath::Loge(Near), FMath::Loge(Far), A));
    }

    class FProbe
    {
        TWeakObjectPtr<AAstroGenerator> Generator;
        TWeakObjectPtr<APlanet> Home;
        TWeakObjectPtr<AGravityPlayerController> Controller;
        TWeakObjectPtr<APawn> Pawn;
        TWeakObjectPtr<UPawnMovementComponent> Movement;
        TWeakObjectPtr<AWorldScapeRoot> MapRoot;
        FVector OriginalOffset = FVector::ZeroVector;
        FQuat OriginalRotation = FQuat::Identity;
        FRotator ControlRotation = FRotator::ZeroRotator;
        double Started = 0, PhaseStarted = 0, FirstSelected = -1, NextFlightFrame = 0;
        double NearRadius = 0, FarRadius = 0;
        int32 Resolution = 0, MapRoots = 0, MapFocusCount = 0, Trip = 0, StableFrames = 0, FlightFrame = 0;
        bool bActive = false, bMapOpen = false, bMovementWasActive = false;
        EPhase Phase = EPhase::Baseline;
        FString Csv = TEXT("elapsed_s,phase,trip,altitude_km,dt_ms,home_state,ready,roots,workers,ram_mib\n");

        AWorldScapeRoot* Root() const
        {
            auto* Surface = Home.IsValid() ? Home->PlanetaryEnvironmentGenerator : nullptr;
            return IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
        }
        int32 OwnedRoots() const
        {
            int32 Count = 0;
            if (Home.IsValid()) for (TActorIterator<AWorldScapeRoot> It(Home->GetWorld()); It; ++It)
                if (It->GetOwner() == Home.Get()) ++Count;
            return Count;
        }
        void Enter(EPhase Next, double Now)
        {
            Phase = Next; PhaseStarted = Now; StableFrames = 0;
            UE_LOG(LogTemp, Display, TEXT("[APS.MapReturn] phase=%d trip=%d elapsed=%.3f roots=%d RAMMiB=%.1f"),
                int32(Phase), Trip, Now-Started, OwnedRoots(), FPlatformMemory::GetStats().UsedPhysical/1048576.);
        }
        void Place(double Radius)
        {
            // Relative to the CURRENT body centre: origin rebasing is real and
            // must not teleport the observer back to a stale absolute position.
            Pawn->SetActorLocationAndRotation(Home->GetActorLocation() + OriginalOffset.GetSafeNormal()*Radius,
                OriginalRotation, false, nullptr, ETeleportType::TeleportPhysics);
            if (Movement.IsValid()) Movement->StopMovementImmediately();
        }
        void OpenMap(double Now, EPhase MapPhase)
        {
            MapRoot = Root(); MapRoots = OwnedRoots(); MapFocusCount = 0;
            Controller->ToggleStrategicMap(); bMapOpen = true;
            Enter(MapPhase, Now);
        }
        bool MapInvariant(FString& Error) const
        {
            if (!Home->bStreamWorldScapeSurface || Generator->GetActivePreviewWorldScapeBody()
                || Root() != MapRoot.Get() || OwnedRoots() != MapRoots
                || (IsValid(Root()) && Root()->LodResolution != Resolution))
            {
                Error = TEXT("F10 changed gameplay root/stream flag/resolution or allocated preview terrain");
                return false;
            }
            return true;
        }
    public:
        ~FProbe() { Restore(); }
        bool Begin(AAstroGenerator* InGenerator, double Now, FString& Error)
        {
            if (!Requested() || !IsValid(InGenerator) || InGenerator->UsesContinuousPreviewFrame()
                || InGenerator->ActorHasTag(TEXT("WorldGenerationPreview")))
            { Error = TEXT("Map return requires an explicitly requested live gameplay generator"); return false; }
            Generator = InGenerator; Home = InGenerator->HomePlanet;
            Controller = Cast<AGravityPlayerController>(InGenerator->GetWorld()->GetFirstPlayerController());
            Pawn = Controller.IsValid() ? Controller->GetPawn() : nullptr;
            if (!Home.IsValid() || !Pawn.IsValid() || !IsValid(Root()) || !Home->bStreamWorldScapeSurface)
            { Error = TEXT("Map return requires a controlled, ready streaming home"); return false; }
            OriginalOffset = Pawn->GetActorLocation() - Home->GetActorLocation();
            OriginalRotation = Pawn->GetActorQuat(); ControlRotation = Controller->GetControlRotation();
            NearRadius = OriginalOffset.Size();
            FarRadius = FMath::Max(Home->GetWorldScapeDeactivationRadiusCm()*1.5, NearRadius*160.0);
            if (!FMath::IsFinite(FarRadius) || NearRadius <= 0 || FarRadius <= NearRadius)
            { Error = TEXT("Map return route radii are invalid"); return false; }
            Resolution = Root()->LodResolution;
            Movement = Pawn->GetMovementComponent();
            bMovementWasActive = Movement.IsValid() && Movement->IsActive();
            if (Movement.IsValid()) { Movement->StopMovementImmediately(); Movement->Deactivate(); }
            bActive = true; Started = Now; Enter(EPhase::Baseline, Now);
            return true;
        }
        void Restore()
        {
            if (!bActive) return;
            // Retain partial timing evidence on failure/cancellation as well.
            const FString EvidenceDir = FPaths::ProjectSavedDir()/TEXT("Diagnostics");
            IFileManager::Get().MakeDirectory(*EvidenceDir, true);
            FFileHelper::SaveStringToFile(Csv, *(EvidenceDir/TEXT("MapReturn.csv")));
            if (bMapOpen && Controller.IsValid()) Controller->ToggleStrategicMap();
            bMapOpen = false;
            if (Pawn.IsValid() && Home.IsValid()) Place(NearRadius);
            if (Controller.IsValid()) Controller->SetControlRotation(ControlRotation);
            if (bMovementWasActive && Movement.IsValid()) Movement->Activate();
            bActive = false;
        }

        EResult Tick(double Now, TFunctionRef<bool(AWorldScapeRoot*, APawn*, FString&)> RenderReady,
            TFunctionRef<bool(const FString&, FString&)> Capture, FString& Error)
        {
            if (!bActive || !Home.IsValid() || !Controller.IsValid() || !Pawn.IsValid() || !Generator.IsValid())
            { Error = TEXT("Map return lost its live test-world lease"); return EResult::Failed; }
            if (Now-Started > 240.0)
            { Error = TEXT("Map return exceeded its bounded 240s route"); return EResult::Failed; }
            auto* Streaming = Home->GetWorld()->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>();
            if (!Streaming || !Home->bStreamWorldScapeSurface || Generator->GetActivePreviewWorldScapeBody())
            { Error = TEXT("Map return lost production streaming ownership"); return EResult::Failed; }
            auto* R = Root();
            const double Radius = FVector::Distance(Pawn->GetActorLocation(), Home->GetActorLocation());
            Csv += FString::Printf(TEXT("%.6f,%d,%d,%.6f,%.6f,%d,%d,%d,%d,%.3f\n"), Now-Started,
                int32(Phase), Trip, (Radius-Home->RadiusKM*100000.)/100000., Home->GetWorld()->GetDeltaSeconds()*1000.,
                int32(Home->GetWorldScapeStreamingState()), Home->bWorldScapeSurfaceReady,
                OwnedRoots(), IsValid(R)?R->WorldScapeLodInGeneration.Num():0, FPlatformMemory::GetStats().UsedPhysical/1048576.);

            const double Elapsed = Now-PhaseStarted;
            if (Phase == EPhase::NearMap || Phase == EPhase::RemoteMap)
            {
                Place(Phase == EPhase::NearMap ? NearRadius : FarRadius);
                if (!MapInvariant(Error)) return EResult::Failed;
                if (Elapsed < 0.8*(MapFocusCount+1)) return EResult::Pending;
                if (MapFocusCount < 3)
                {
                    Generator->FocusPreviewTarget(EAstroPreviewFocus::HomePlanet, Controller.Get());
                    ++MapFocusCount;
                    return MapInvariant(Error) ? EResult::Pending : EResult::Failed;
                }
                if (Controller->GetViewTarget() != Generator.Get())
                { Error = TEXT("F10 did not actually show the live generator camera"); return EResult::Failed; }
                Controller->ToggleStrategicMap(); bMapOpen = false;
                if (Phase == EPhase::NearMap) Enter(EPhase::NearReturn, Now);
                else { FirstSelected = -1; NextFlightFrame = Now; FlightFrame = 0; Enter(EPhase::Inbound, Now); }
                return EResult::Pending;
            }
            if (Phase == EPhase::Outbound || Phase == EPhase::Inbound)
            {
                const bool Returning = Phase == EPhase::Inbound;
                Place(RouteRadius(NearRadius, FarRadius, Elapsed/12.0, Returning));
                if (Returning && FirstSelected < 0 && Streaming->GetActiveBody() == Home.Get()) FirstSelected = Now;
                // Sparse frames record the MOVING transition too; do not wait for
                // readiness or hide transient fallback before taking these frames.
                if (Now >= NextFlightFrame)
                {
                    const FString Label = FString::Printf(TEXT("Trip%d_%s_%02d"), Trip, Returning?TEXT("In"):TEXT("Out"), FlightFrame++);
                    if (!Capture(Label, Error) && !Error.IsEmpty()) return EResult::Failed;
                    NextFlightFrame = Now + 2.0;
                }
                if (Elapsed < 12.0) return EResult::Pending;
                if (Returning) { Enter(EPhase::Returned, Now); return EResult::Pending; }
                if (Home->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded
                    || Streaming->GetResidentFamily() == Home.Get() || IsValid(Root()))
                {
                    if (Elapsed < 30) return EResult::Pending;
                    Error = TEXT("Distant flight failed to unload home; no always-resident workaround accepted"); return EResult::Failed;
                }
                OpenMap(Now, EPhase::RemoteMap); return EResult::Pending;
            }

            Place(NearRadius);
            FString Pending;
            const bool Ready = IsValid(R) && Streaming->GetActiveBody() == Home.Get() && Home->bWorldScapeSurfaceReady
                && !R->IsHidden() && R->LodResolution == Resolution && RenderReady(R, Pawn.Get(), Pending);
            StableFrames = Ready ? StableFrames+1 : 0;
            if (StableFrames < 8 || Elapsed < 1.0 || Controller->GetViewTarget() != Pawn.Get())
            {
                if (Elapsed < 45.0) return EResult::Pending;
                Error = TEXT("Returned ground did not regain complete visible pawn-centred payload: ") + Pending;
                return EResult::Failed;
            }
            const FString Label = Phase == EPhase::Baseline ? TEXT("BeforeF10") : Phase == EPhase::NearReturn
                ? TEXT("AfterF10") : FString::Printf(TEXT("Trip%d_ReturnedGround"), Trip);
            if (!Capture(Label, Error)) return Error.IsEmpty() ? EResult::Pending : EResult::Failed;
            UE_LOG(LogTemp, Display, TEXT("[APS.MapReturn] frame=%s renderReady=1 selectedToSettledSeconds=%.3f resolution=%d root=%s"),
                *Label, FirstSelected>=0?Now-FirstSelected:0., Resolution, *GetNameSafe(R));
            if (Phase == EPhase::Baseline) OpenMap(Now, EPhase::NearMap);
            else if (Phase == EPhase::NearReturn || ++Trip < 2)
            { NextFlightFrame = Now; FlightFrame = 0; Enter(EPhase::Outbound, Now); }
            else
            {
                const FString Dir = FPaths::ProjectSavedDir()/TEXT("Diagnostics");
                IFileManager::Get().MakeDirectory(*Dir, true);
                if (!FFileHelper::SaveStringToFile(Csv, *(Dir/TEXT("MapReturn.csv"))))
                { Error = TEXT("Could not save map-return timing/memory evidence"); return EResult::Failed; }
                Enter(EPhase::Done, Now); Restore(); return EResult::Done;
            }
            return EResult::Pending;
        }
    };
}
#endif
