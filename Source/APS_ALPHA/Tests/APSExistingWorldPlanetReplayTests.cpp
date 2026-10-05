// Explicit opt-in observation of one hash-pinned saved world. CharacterRoundTrip
// uses native movement; LidimApproach is a separate controlled diagnostic camera
// route. Optional slope isolation changes one colour-mask expression in memory
// inside the guarded private process only; no saved assets or profiles change.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "APSPublishedTerrainDescentAudit.h"
#include "APSSavedCharacterRoundTrip.h"
#include "APSSavedLidimApproach.h"
#include "APSSavedStolyavenaApproach.h"
#include "APSSavedNativeGlobeRoundTrip.h"
#include "APSSavedSlopeIsolation.h"
#include "APSOriginalWarpPixelAssets.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstance.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "LocalVertexFactory.h"
#include "ShaderCompiler.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"

namespace APSExistingWorldPlanetReplayTests
{
    constexpr const TCHAR* Slot = TEXT("Jeqiwoga Cluster 1212");
    constexpr const TCHAR* StolyavenaSlot = TEXT("Puwivi GalaxiesCluster 364");
    constexpr const TCHAR* StolyavenaSaveSha1 = TEXT("8CF4085159D08EC7DDDD519A34170D58FC64F57F");
    constexpr const TCHAR* StolyavenaMetaSha1 = TEXT("E2D0AA8CC18349E8704263F55129242312100600");
    constexpr const TCHAR* RunPrefix = TEXT("F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/saved-world-replay-");
    constexpr double HierarchySeconds = 185.0, PawnSeconds = 30.0, ObserveSeconds = 20.0, TotalSeconds = 255.0;
    constexpr double CandidatePreparationSeconds = 120.0;

    static FString FullPath(FString Path)
    {
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeDirectoryName(Path);
        if (!FPaths::CollapseRelativeDirectories(Path)) return FString();
        return Path;
    }

    static bool FileSha1(const FString& Path, FString& Out)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.IsEmpty()) return false;
        FSHAHash Hash;
        FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
        Out = Hash.ToString();
        return true;
    }

    struct FGuard
    {
        FString SavedDir, SavePath, MetaPath, ExpectedSaveHash, MetaHash;
        bool bStolyavena = false;
        bool bCharacterRoundTrip = false;
        bool bLidimApproach = false;
        bool bNativeGlobeRoundTrip = false;
        int32 SlopeMode = -1; // Absent=-1; explicit recompiled control=0, native slope=1.

        const TCHAR* GetSlot() const { return bStolyavena ? StolyavenaSlot : Slot; }
        bool MatchesModel(const UGeneratedWorld* M) const
        {
            return IsValid(M) && (bStolyavena ? M->GenerationSeed == 2028432231
                : M->CanonicalStellarDataset.ClusterGenerationSeed == 121288183);
        }

        bool Initialize(FString& Error)
        {
            const TCHAR* Cmd = FCommandLine::Get();
            bStolyavena = FParse::Param(Cmd, TEXT("APSSavedPlanetStolyavena"));
            bCharacterRoundTrip = FParse::Param(Cmd, TEXT("APSSavedPlanetCharacterRoundTrip"));
            bLidimApproach = FParse::Param(Cmd, TEXT("APSSavedPlanetLidimApproach"));
            bNativeGlobeRoundTrip = FParse::Param(Cmd, TEXT("APSSavedPlanetNativeGlobeRoundTrip"));
            TArray<FString> CmdTokens;
            FString(Cmd).ParseIntoArrayWS(CmdTokens);
            for (const FString& Token : CmdTokens)
            {
                // This new mode has an allowlist, not a growing blacklist of
                // material/flight diagnostics. Old Khoax parsing is unchanged.
                if (bStolyavena && Token.StartsWith(TEXT("-APS"), ESearchCase::IgnoreCase)
                    && !Token.Equals(TEXT("-APSSavedPlanetReplay"), ESearchCase::IgnoreCase)
                    && !Token.Equals(TEXT("-APSSavedPlanetStolyavena"), ESearchCase::IgnoreCase)
                    && !Token.StartsWith(TEXT("-APSSavedPlanetReplaySlot="), ESearchCase::IgnoreCase)
                    && !Token.StartsWith(TEXT("-APSSavedPlanetExpectedSavedDir="), ESearchCase::IgnoreCase)
                    && !Token.StartsWith(TEXT("-APSSavedPlanetSaveSha1="), ESearchCase::IgnoreCase)
                    && !Token.StartsWith(TEXT("-APSSavedPlanetMetaSha1="), ESearchCase::IgnoreCase))
                { Error = TEXT("Stolyavena forbids all other APS route/material diagnostic flags"); return false; }
                if (!Token.StartsWith(TEXT("-APSSavedPlanetSlope"), ESearchCase::IgnoreCase)) continue;
                if (SlopeMode != -1 || (!Token.Equals(TEXT("-APSSavedPlanetSlope=0"), ESearchCase::IgnoreCase)
                    && !Token.Equals(TEXT("-APSSavedPlanetSlope=1"), ESearchCase::IgnoreCase)))
                { Error = TEXT("slope isolation requires exactly one explicit =0 or =1"); return false; }
                SlopeMode = Token.EndsWith(TEXT("=1")) ? 1 : 0;
            }
            if (SlopeMode >= 0 && (!bLidimApproach || bCharacterRoundTrip))
            { Error = TEXT("slope isolation is restricted to the separate controlled Lidim route"); return false; }
            if (bCharacterRoundTrip && bLidimApproach)
            { Error = TEXT("native character roundtrip and controlled Lidim route are separate diagnostics"); return false; }
            if (bNativeGlobeRoundTrip && (bCharacterRoundTrip || bLidimApproach || SlopeMode >= 0))
            { Error = TEXT("native globe lifecycle route must run separately"); return false; }
            FString RequestedSlot, RequestedSaved, UserDir;
            if (!FParse::Param(Cmd, TEXT("APSSavedPlanetReplay")) || !FApp::IsUnattended()
                || IsRunningCommandlet() || !GIsAutomationTesting || FParse::Param(Cmd, TEXT("NullRHI"))
                || !FParse::Value(Cmd, TEXT("APSSavedPlanetReplaySlot="), RequestedSlot)
                || !FParse::Value(Cmd, TEXT("APSSavedPlanetExpectedSavedDir="), RequestedSaved)
                || !FParse::Value(Cmd, TEXT("APSSavedPlanetSaveSha1="), ExpectedSaveHash)
                || !FParse::Value(Cmd, TEXT("UserDir="), UserDir))
            { Error = TEXT("requires explicit replay opt-in, unattended rendering, isolated UserDir/Saved and SHA1 guards"); return false; }
            const FString RunDir = FullPath(UserDir);
            const FString Label = RunDir.Mid(FCString::Strlen(RunPrefix));
            SavedDir = FullPath(FPaths::ProjectSavedDir());
            bool ValidLabel = !Label.IsEmpty() && Label.Len() <= 64;
            for (const TCHAR Ch : Label) ValidLabel &= (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9') || Ch == '-';
            bool ValidHash = ExpectedSaveHash.Len() == 40;
            for (const TCHAR Ch : ExpectedSaveHash) ValidHash &= FChar::IsHexDigit(Ch);
            if (RequestedSlot != GetSlot() || FPaths::GetCleanFilename(RequestedSlot) != RequestedSlot
                || !ExpectedSaveHash.Equals(bStolyavena ? StolyavenaSaveSha1 : TEXT("2F1A835EDFA93F8F01E291C64EF79104CA143CF4"), ESearchCase::IgnoreCase)
                || !RunDir.StartsWith(RunPrefix, ESearchCase::IgnoreCase) || !ValidLabel || !ValidHash
                || !SavedDir.Equals(FullPath(RequestedSaved), ESearchCase::IgnoreCase)
                || !SavedDir.Equals(FullPath(RunDir / TEXT("Saved")), ESearchCase::IgnoreCase)
                || SavedDir.Equals(FullPath(FPaths::ProjectDir() / TEXT("Saved")), ESearchCase::IgnoreCase)
                || FCString::Strifind(Cmd, TEXT("-APSProbe")) || FCString::Strifind(Cmd, TEXT("-APSDiagnosticPlanet")))
            { Error = TEXT("replay guard rejected slot/path/hash or conflicting diagnostic fixture flags"); return false; }
            SavePath = SavedDir / TEXT("SaveGames") / (FString(GetSlot()) + TEXT(".sav"));
            MetaPath = SavedDir / TEXT("SaveGames") / (FString(GetSlot()) + TEXT(".apsmeta"));
            if (!FileSha1(MetaPath, MetaHash)) { Error = TEXT("private sidecar missing/unreadable"); return false; }
            if (bStolyavena)
            {
                FString RequestedMeta;
                if (!FParse::Value(Cmd, TEXT("APSSavedPlanetMetaSha1="), RequestedMeta)
                    || !RequestedMeta.Equals(StolyavenaMetaSha1, ESearchCase::IgnoreCase)
                    || !MetaHash.Equals(StolyavenaMetaSha1, ESearchCase::IgnoreCase))
                { Error = TEXT("Stolyavena requires the pinned Puwivi metadata SHA1"); return false; }
            }
            return Check(Error);
        }

        bool Check(FString& Error) const
        {
            FString SaveHash, CurrentMetaHash;
            if (!FullPath(FPaths::ProjectSavedDir()).Equals(SavedDir, ESearchCase::IgnoreCase)
                || !FileSha1(SavePath, SaveHash) || !SaveHash.Equals(ExpectedSaveHash, ESearchCase::IgnoreCase)
                || !FileSha1(MetaPath, CurrentMetaHash) || CurrentMetaHash != MetaHash)
            { Error = TEXT("private SavedDir/save SHA1/sidecar changed or unreadable; originals were not accessed"); return false; }
            return true;
        }
    };

    class FReplayCommand final : public IAutomationLatentCommand
    {
    public:
        FReplayCommand(FAutomationTestBase* InTest, const FGuard& InGuard, double InStarted)
            : Test(InTest), Guard(InGuard), Started(InStarted) {}

        virtual bool Update() override
        {
            check(IsInGameThread());
            const double Now = FPlatformTime::Seconds();
            if (Now - Started > (Guard.bStolyavena ? 390.0 : Guard.bCharacterRoundTrip ? 1350.0 : Guard.bLidimApproach ? (Guard.SlopeMode >= 0 ? 540.0 : 360.0) : TotalSeconds)
                + (bPrepareOriginalWarpPixel ? CandidatePreparationSeconds : 0.0))
                return Finish(TEXT("bounded replay deadline exceeded"));
            // Preserve engine/automation errors as FAIL, but still collect passive evidence.
            // Only an explicit guard, replay, identity, readiness or observation failure stops this command.
            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
            UMainGameplayInstance* State = World && World->GetGameInstance()
                ? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
            if (!bLoadCalled)
            {
                AMainMenuController* Menu = Cast<AMainMenuController>(PC);
                if (!Menu || !State || !World->HasBegunPlay())
                    return Now - Started > 15.0 ? Finish(TEXT("main-menu PIE unavailable after 15 seconds")) : false;
                FString Error;
                // Guard.Initialize already succeeded before PIE. Prepare only the explicit
                // immutable candidate, before loading the save or starting any route clock.
                if (bPrepareOriginalWarpPixel && !PrepareOriginalWarpPixel(World, Error))
                    return Error.IsEmpty() ? false : Finish(Error);
                if (!Guard.Check(Error)) return Finish(Error);
                bLoadCalled = true; LoadStarted = bPrepareOriginalWarpPixel ? FPlatformTime::Seconds() : Now; // Exactly one normal UI load call, including on failure.
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] LOAD slot=%s savedDir=%s sha1=%s characterRoundTrip=%d scope=actual-save-replay not-exact-user-camera no-fixture no-save no-visual-success"),
                    Guard.GetSlot(), *Guard.SavedDir, *Guard.ExpectedSaveHash, Guard.bCharacterRoundTrip);
                Menu->LoadWorldSlot(Guard.GetSlot());
                Model = State->NewGeneratedWorld;
                if (!Model.IsValid() || !State->bPendingSavedWorldReplay || State->SaveSlotName != Guard.GetSlot()
                    || !Guard.MatchesModel(Model.Get()))
                    return Finish(Guard.bStolyavena ? TEXT("LoadWorldSlot did not prepare pinned Puwivi GenerationSeed2028432231")
                        : TEXT("LoadWorldSlot did not prepare the expected saved model/slot/cluster seed 121288183"));
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] MODEL generationSeed=%d clusterSeed=%d records=%d home=%s"),
                    Model->GenerationSeed, Model->CanonicalStellarDataset.ClusterGenerationSeed,
                    Model->CanonicalStellarDataset.ClusterRecords.Num(), *Model->HomePlanetName.ToString());
                return false;
            }
            if (!State || !PC || !World) return Now - LoadStarted > HierarchySeconds
                ? Finish(TEXT("world/controller missing at hierarchy deadline")) : false;
            if (!Model.IsValid() || State->NewGeneratedWorld != Model.Get() || State->SaveSlotName != Guard.GetSlot()
                || !Guard.MatchesModel(Model.Get()))
                return Finish(TEXT("saved model, cluster seed or slot replaced during replay"));
            if (ObserveStarted == 0.0 && Now >= NextProgress)
            {
                NextProgress = Now + 5.0;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] WAIT elapsed=%.3f pendingReplay=%d loading=%d pawn=%s"),
                    Now - LoadStarted, State->bPendingSavedWorldReplay, State->bIsLoadingMode, *GetPathNameSafe(PC->GetPawn()));
            }
            for (TActorIterator<AAstroGenerator> It(World); It; ++It)
                if (It->GetGeneratedWorldModel() == Model.Get() && It->HasGeneratedStarterCommitFailed())
                    return Finish(TEXT("normal saved-world starter hierarchy reported failure"));
            if (PawnStarted == 0.0)
            {
                if (Now - LoadStarted > HierarchySeconds) return Finish(TEXT("hierarchy/player overlay exceeded 185 seconds"));
                if (State->bPendingSavedWorldReplay || State->bIsLoadingMode || !Cast<AGravityPlayerController>(PC)) return false;
                PawnStarted = Now;
                if (Guard.bStolyavena)
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] HIERARCHY_OVERLAY_DONE t=%.3f savedPawn=%s; next ordinary VULEX visit with separate observer"),
                        Now - LoadStarted, *GetPathNameSafe(PC->GetPawn()));
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] HIERARCHY_OVERLAY_DONE t=%.3f; waiting for saved character and Jaim surface (saved CIVS pilot key is empty)"), Now - LoadStarted);
                }
            }
            if (Guard.bStolyavena) return UpdateStolyavena(World, PC, Now);
            APawn* Pawn = PC->GetPawn();
            APlanetaryBody* Jaim = nullptr;
            APlanetaryBody* Lidim = nullptr;
            for (TActorIterator<APlanetaryBody> It(World); It; ++It)
            {
                if (It->AstroName == FName(TEXT("Jaim"))) { if (Jaim) return Finish(TEXT("duplicate Jaim actor")); Jaim = *It; }
                if (It->AstroName == FName(TEXT("Lidim"))) { if (Lidim) return Finish(TEXT("duplicate Lidim actor")); Lidim = *It; }
            }
            if (Now >= NextRepresentation)
            {
                NextRepresentation = Now + 0.125;
                ObserveRepresentations(Jaim, 0, Now - LoadStarted);
                ObserveRepresentations(Lidim, 1, Now - LoadStarted);
            }
            if ((Jaim && Jaim->WorldScapeSeed != 597932) || (Lidim && Lidim->WorldScapeSeed != 257455))
                return Finish(TEXT("saved hierarchy body seeds differ: expected Jaim=597932, Lidim=257455"));
            if (Guard.bNativeGlobeRoundTrip) return UpdateNativeGlobe(World, PC, Pawn, Lidim, Now);
            if (Guard.bLidimApproach) return UpdateLidim(World, PC, Pawn, Lidim, Now);
            // Public, read-only association; never EnsurePlanetaryEnvironmentGenerator/EnsureWorldScapeSurface.
            APlanetarySurfaceGenerator* Surface = Jaim ? Jaim->PlanetaryEnvironmentGenerator : nullptr;
            AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
            if (!Pawn || !Jaim || !Lidim || !IsValid(Root)
                || !Surface->bSurfaceProfileApplied || !Surface->IsSurfaceProfileCurrent(Jaim)
                || Surface->IsSurfaceProfileApplyPending() || !Jaim->bWorldScapeSurfaceReady || !PC->PlayerCameraManager)
            {
                if (ObserveStarted != 0.0) return Finish(TEXT("restored pawn/body/root/camera lost during passive observation"));
                if (Now - PawnStarted <= PawnSeconds) return false;
                return Finish(FString::Printf(TEXT("30-second pawn/surface restoration deadline: pawn=%s Jaim=%s Lidim=%s root=%s"),
                    *GetPathNameSafe(Pawn), *GetPathNameSafe(Jaim), *GetPathNameSafe(Lidim), *GetPathNameSafe(Root)));
            }
            const auto& Profile = Surface->ResolvedSurfaceProfile;
            if (Jaim->PlanetType != EPlanetType::Frozen || Profile.PlanetType != EPlanetType::Frozen
                || !FMath::IsFinite(Jaim->RadiusKM) || FMath::Abs(Jaim->RadiusKM - 500.3573) > 0.001
                || !FMath::IsFinite(Jaim->WorldScapePresentationScale) || FMath::Abs(Jaim->WorldScapePresentationScale - 1.0) > 1.e-9
                || Root->Seed != 597932 || !FMath::IsFinite(Root->PlanetScaleCode)
                || FMath::Abs(Root->PlanetScaleCode / 100000.0 - 500.3573) > 0.001 || Root->bOcean
                || Profile.LiquidType != EAPSPlanetLiquidType::None || Root->GetWorld() != World)
                return Finish(TEXT("actual Jaim body/resolved profile/root mismatch (Frozen, seed597932, radius500.3573km +/-1m, full-scale, no ocean required)"));
            if (ObserveStarted == 0.0)
            {
                // Hash-pinned CIVS v5 has empty PilotedVehicleKey (length=0 at byte1562233).
                // Fleet.Unit.bPiloted=1 is not the normal restore seating instruction.
                // Observe the character the save actually restores; never force boarding.
                if (Pawn->GetClass()->GetPathName() != TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter_SpeedModes.BP_CustomGravityCharacter_SpeedModes_C"))
                    return Finish(TEXT("possessed pawn differs from the character restored by this save's empty pilot key"));
                // A distant Jaim actor alone is not proof that the saved pawn is at Jaim.
                APlanetaryBody* Nearest = nullptr; double NearestSurface = TNumericLimits<double>::Max();
                for (TActorIterator<APlanetaryBody> It(World); It; ++It)
                {
                    const double Distance = FMath::Abs(FVector::Distance(Pawn->GetActorLocation(), It->GetActorLocation()) - It->GetWorldScapeBodyRadiusCm());
                    if (Distance < NearestSurface) { NearestSurface = Distance; Nearest = *It; }
                }
                if (Nearest != Jaim) return Finish(TEXT("restored pawn's nearest planetary surface is not Jaim; no relocation attempted"));
                OutputDir = Guard.SavedDir / TEXT("Screenshots/Windows") / (TEXT("SavedWorldPlanetReplay_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
                if (IFileManager::Get().DirectoryExists(*OutputDir) || !IFileManager::Get().MakeDirectory(*OutputDir, true))
                    return Finish(TEXT("could not create unique private screenshot directory"));
                ObservedPawn = Pawn; ObservedRoot = Root; ObserveStarted = Now; NextAudit = NextCapture = Now;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] BEGIN pawn=%s savedPilotKey=empty body=BODY:Jaim radiusKm=%.9f seed=%d LidimSeed=%d root=%s profileTerrainSeed=%d noise=%.6f/%.6f output=%s characterRoundTrip=%d target-cadence-only synchronous-readback-not-benchmark not-exact-user-camera visualAcceptance=unassessed"),
                    *Pawn->GetPathName(), Jaim->RadiusKM, Jaim->WorldScapeSeed,
                    Lidim->WorldScapeSeed, *Root->GetPathName(), Profile.TerrainSeed, Profile.NoiseScale, Profile.NoiseIntensity, *OutputDir, Guard.bCharacterRoundTrip);
            }
            if (ObservedPawn.Get() != Pawn || ObservedRoot.Get() != Root) return Finish(TEXT("observed pawn/root replaced; no replacement accepted"));
            UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
            if (!Origin) return Finish(TEXT("generation-frame origin subsystem unavailable"));
            const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
            const double Elapsed = Now - ObserveStarted;
            if (Guard.bCharacterRoundTrip && !CharacterRoute.Tick(Cast<ACustomGravityCharacter>(Pawn), Root, Elapsed))
                return Finish(CharacterRoute.GetError());
            if (Now >= NextAudit)
            {
                NextAudit = Now + (Guard.bCharacterRoundTrip ? 0.25 : 0.125); ++AuditCount;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Pose] t=%.3f frame=%llu rootWorld=%s rootGeneration=%s bodyWorld=%s pawnWorld=%s pawnGeneration=%s cameraWorld=%s cameraGeneration=%s cameraECEF=%s cameraRot=%s fov=%.3f"),
                    Elapsed, static_cast<unsigned long long>(GFrameCounter), *Root->GetActorLocation().ToString(),
                    *Origin->ToGenerationFrame(Root->GetActorLocation()).ToString(), *Jaim->GetActorLocation().ToString(),
                    *Pawn->GetActorLocation().ToString(), *Origin->ToGenerationFrame(Pawn->GetActorLocation()).ToString(),
                    *Camera.ToString(), *Origin->ToGenerationFrame(Camera).ToString(),
                    *Root->WorldToECEF(Camera).ToFVector().ToString(), *PC->PlayerCameraManager->GetCameraRotation().ToString(),
                    PC->PlayerCameraManager->GetFOVAngle());
                if (!Audit.Tick(Root, Camera, Elapsed, Guard.bCharacterRoundTrip ? CharacterRoute.Phase() : TEXT("actual-save-passive")))
                    return Finish(Audit.GetError());
            }
            if (Now >= NextCapture)
            {
                NextCapture = Now + (Guard.bCharacterRoundTrip ? 0.5 : 0.25);
                FString Error;
                if (!Capture(World, Camera, Elapsed, Error)) return Finish(Error);
            }
            if (Guard.bCharacterRoundTrip) return CharacterRoute.IsComplete() ? Finish(FString()) : false;
            return Elapsed >= ObserveSeconds ? Finish(FString()) : false;
        }

    private:
        bool PrepareOriginalWarpPixel(UWorld* World, FString& Error)
        {
            if (bCandidatePrepared) return true;
            if (CandidatePreparationStarted == 0.0)
            {
                CandidatePreparationStarted = FPlatformTime::Seconds();
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.WarpPreparation] begin cap=120s template=%s beforeSaveLoad=1 routeClockStarted=0"),
                    APSOriginalWarpPixelAssets::TemplatePath);
                CandidateTemplate.Reset(LoadObject<UMaterialInstance>(nullptr, APSOriginalWarpPixelAssets::TemplatePath));
            }
            if (!CandidateTemplate.IsValid() || !CandidateTemplate->Parent
                || CandidateTemplate->Parent->GetPathName() != APSOriginalWarpPixelAssets::MasterPath)
            { Error = TEXT("original warp-pixel candidate missing or has unexpected parent"); return false; }
            auto* Resource = CandidateTemplate->GetMaterialResource(World->GetFeatureLevel());
            if (!Resource)
            { Error = TEXT("original warp-pixel candidate has no active-feature material resource"); return false; }
            if (!Resource->GetCompileErrors().IsEmpty())
            {
                for (const FString& CompileError : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.SavedPlanetReplay.WarpPreparation] %s"), *CompileError);
                Error = TEXT("original warp-pixel candidate has shader compile errors"); return false;
            }
            if (!Resource->IsGameThreadShaderMapComplete())
                Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
            const double PollNow = FPlatformTime::Seconds();
            const double Elapsed = PollNow - CandidatePreparationStarted;
            auto* Map = Resource->GetGameThreadShaderMap();
            const bool bComplete = Resource->IsGameThreadShaderMapComplete();
            const bool bLocalVF = Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            const bool bTimedOut = Elapsed >= CandidatePreparationSeconds;
            if (PollNow >= NextCandidateProgress || bTimedOut || (bComplete && bLocalVF))
            {
                NextCandidateProgress = PollNow + 5.0;
                const bool bCaching = Resource->IsCachingShaders();
                // Avoid the cache-completion path while it is pending. This public API
                // only checks outstanding compilation once caching has already finished.
                const int32 CompilationFinished = bCaching ? -1 : int32(Resource->IsCompilationFinished());
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.WarpPreparation] %s elapsed=%.3f feature=%d map=%d complete=%d localVF=%d isCaching=%d compilationFinished=%d remainingJobs=%d"),
                    bTimedOut ? TEXT("timeout") : bComplete && bLocalVF ? TEXT("ready") : TEXT("progress"),
                    Elapsed, int32(World->GetFeatureLevel()), int32(Map != nullptr), int32(bComplete), int32(bLocalVF),
                    int32(bCaching), CompilationFinished, GShaderCompilingManager ? GShaderCompilingManager->GetNumRemainingJobs() : -1);
            }
            if (bTimedOut)
            {
                Error = TEXT("original warp-pixel candidate preparation exceeded 120 seconds before save load"); return false;
            }
            if (!bComplete || !bLocalVF) return false;
            bCandidatePrepared = true;
            return true;
        }

        bool UpdateStolyavena(UWorld* World, APlayerController* PC, double Now)
        {
            if (!PC->GetPawn() || !PC->PlayerCameraManager)
                return Now - PawnStarted > PawnSeconds ? Finish(TEXT("Stolyavena route missing saved pawn/camera after30s")) : false;
            if (ObserveStarted == 0.0)
            {
                OutputDir = Guard.SavedDir / TEXT("Screenshots/Windows") / (TEXT("SavedStolyavenaApproach_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
                if (IFileManager::Get().DirectoryExists(*OutputDir) || !IFileManager::Get().MakeDirectory(*OutputDir, true))
                    return Finish(TEXT("could not create unique private Stolyavena screenshot directory"));
                ObserveStarted = Now; NextAudit = NextCapture = NextRepresentation = Now;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] BEGIN slot=%s generationSeed=%d savedPawn=%s output=%s scope=controlled-observer-on-actual-saved-body not-exact-user-camera nativeShipPhysics=0 no-shader-or-light-override"),
                    Guard.GetSlot(), Model->GenerationSeed, *GetPathNameSafe(PC->GetPawn()), *OutputDir);
            }
            const double Elapsed = Now - ObserveStarted;
            if (!StolyavenaRoute.Tick(PC, Elapsed)) return Finish(StolyavenaRoute.GetError());
            APlanet* Body = StolyavenaRoute.GetBody();
            AWorldScapeRoot* Root = StolyavenaRoute.GetRoot();
            UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
            if (!Origin) return Finish(TEXT("Stolyavena generation frame disappeared"));
            const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
            if (Now >= NextRepresentation)
            {
                NextRepresentation = Now + 0.125;
                ObserveRepresentations(Body, 0, Elapsed);
            }
            if (StolyavenaRoute.Ready() && Now >= NextAudit)
            {
                NextAudit = Now + 0.125;
                ++AuditCount;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena.Pose] t=%.3f phase=%s gtFrame=%llu bodyGeneration=%s rootGeneration=%s observerGeneration=%s cameraGeneration=%s cameraRot=%s fov=%.6f cameraHeightKm=%.9f plannedPoseNotCaptureProof=1"),
                    Elapsed, StolyavenaRoute.Phase(), static_cast<unsigned long long>(GFrameCounter),
                    *Origin->ToGenerationFrame(Body->GetActorLocation()).ToString(), *Origin->ToGenerationFrame(Root->GetActorLocation()).ToString(),
                    *Origin->ToGenerationFrame(StolyavenaRoute.GetObserver()->GetActorLocation()).ToString(),
                    *Origin->ToGenerationFrame(Camera).ToString(), *PC->PlayerCameraManager->GetCameraRotation().ToString(),
                    PC->PlayerCameraManager->GetFOVAngle(), (FVector::Distance(Camera, Root->GetActorLocation()) - Root->PlanetScaleCode) / 100000.0);
                if (!Audit.Tick(Root, Camera, Elapsed, StolyavenaRoute.Phase())) return Finish(Audit.GetError());
            }
            // Includes the real body's warmup; no forced camera-manager update or render fence.
            if (Body && StolyavenaRoute.Started() && Now >= NextCapture)
            {
                NextCapture = Now + 0.25;
                FString Error;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena.Capture] phase=%s index=%d"), StolyavenaRoute.Phase(), CaptureCount);
                if (!Capture(World, Camera, Elapsed, Error)) return Finish(Error);
                // Two successful readbacks on distinct GT frames after0.5s in
                // the hold exclude its first transition frame. Still unfenced:
                // observed-hold captures are NOT exact render-pose proof.
                static const TCHAR* RequiredHolds[] = {TEXT("orbit-hold"), TEXT("20km-down-hold"), TEXT("2km-down-hold"),
                    TEXT("ground34m-hold"), TEXT("2km-up-hold"), TEXT("20km-up-hold"), TEXT("orbit-return-hold")};
                for (int32 I = 0; I < UE_ARRAY_COUNT(RequiredHolds); ++I)
                    if (FCString::Strcmp(StolyavenaRoute.Phase(), RequiredHolds[I]) == 0
                        && StolyavenaRoute.PhaseAgeSeconds() >= 0.5 && !(StolyavenaCapturedHolds & (1u << I))
                        && (StolyavenaHoldCaptureCounts[I] == 0 || StolyavenaHoldCaptureFrames[I] != GFrameCounter))
                    {
                        StolyavenaHoldCaptureFrames[I] = GFrameCounter;
                        if (++StolyavenaHoldCaptureCounts[I] >= 2) StolyavenaCapturedHolds |= static_cast<uint8>(1u << I);
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] HOLD_CAPTURE phase=%s index=%d gtFrame=%llu holdAge=%.3f samples=%d mask=0x%02x viewportReadback=success scope=observed-hold-captures-unfenced not-render-pose-proof visualAcceptance=unassessed"),
                            RequiredHolds[I], CaptureCount - 1, static_cast<unsigned long long>(GFrameCounter), StolyavenaRoute.PhaseAgeSeconds(),
                            static_cast<int32>(StolyavenaHoldCaptureCounts[I]), static_cast<uint32>(StolyavenaCapturedHolds));
                    }
            }
            if (!StolyavenaRoute.IsComplete()) return false;
            if (StolyavenaCapturedHolds != 0x7f)
                return Finish(FString::Printf(TEXT("Stolyavena mandatory hold captures incomplete: mask=0x%02x expected=0x7f; route deadline is not extended"),
                    static_cast<uint32>(StolyavenaCapturedHolds)));
            return Finish(FString());
        }

        bool UpdateNativeGlobe(UWorld* World, APlayerController* PC, APawn* Pawn,
            APlanetaryBody* Body, double Now)
        {
            auto* Character = Cast<ACustomGravityCharacter>(Pawn);
            if (!Character || !IsValid(Body) || !PC->PlayerCameraManager)
                return Now - PawnStarted > PawnSeconds ? Finish(TEXT("native globe route missing restored character/body")) : false;
            if (ObserveStarted == 0.0)
            {
                OutputDir = Guard.SavedDir / TEXT("Screenshots/Windows") / (TEXT("NativeGlobeRoundTrip_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
                if (IFileManager::Get().DirectoryExists(*OutputDir) || !IFileManager::Get().MakeDirectory(*OutputDir, true))
                    return Finish(TEXT("could not create native globe screenshot directory"));
                ObservedPawn = Pawn; ObserveStarted = Now; NextCapture = Now;
            }
            if (ObservedPawn.Get() != Pawn) return Finish(TEXT("native globe route lost original pawn"));
            const double Elapsed = Now - ObserveStarted;
            if (!NativeGlobeRoute.Tick(Character, Body, Elapsed)) return Finish(NativeGlobeRoute.GetError());
            ++AuditCount;
            if (NativeGlobeRoute.Started() && Now >= NextCapture)
            {
                NextCapture = Now + 0.25;
                FString Error;
                UE_LOG(LogTemp, Display, TEXT("[APS.NativeGlobeReplay.Capture] phase=%s index=%d"), NativeGlobeRoute.Phase(), CaptureCount);
                if (!Capture(World, PC->PlayerCameraManager->GetCameraLocation(), Elapsed, Error)) return Finish(Error);
            }
            return NativeGlobeRoute.IsComplete() ? Finish(FString()) : false;
        }

        bool UpdateLidim(UWorld* World, APlayerController* PC, APawn* Pawn,
            APlanetaryBody* Body, double Now)
        {
            APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
            AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
            ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pawn);
            if (!Character || !IsValid(Root) || !Surface->bSurfaceProfileApplied
                || !Surface->IsSurfaceProfileCurrent(Body) || Surface->IsSurfaceProfileApplyPending()
                || (ObserveStarted == 0.0 && !Body->bWorldScapeSurfaceReady)
                || !PC->PlayerCameraManager)
            {
                if (ObserveStarted != 0.0) return Finish(TEXT("Lidim replay lost its restored character/body/profile/root/camera"));
                return Now - PawnStarted > PawnSeconds ? Finish(TEXT("Lidim standby root/profile unavailable after30s")) : false;
            }
            const auto& Profile = Surface->ResolvedSurfaceProfile;
            if (Body->PlanetType != EPlanetType::Frozen || Profile.PlanetType != EPlanetType::Frozen
                || Body->WorldScapeSeed != 257455 || Root->Seed != 257455
                || !FMath::IsFinite(Body->RadiusKM) || FMath::Abs(Body->RadiusKM - 1280.896) > 0.001
                || !FMath::IsFinite(Root->PlanetScaleCode) || FMath::Abs(Root->PlanetScaleCode / 100000.0 - 1280.896) > 0.001
                || !FMath::IsFinite(Body->WorldScapePresentationScale) || FMath::Abs(Body->WorldScapePresentationScale - 1.0) > 1.e-9
                || Root->bOcean || Profile.LiquidType != EAPSPlanetLiquidType::None || Root->GetWorld() != World)
                return Finish(FString::Printf(TEXT("actual saved Lidim profile mismatch: type=%d/%d seed=%d/%d radiusKm=%.12f rootKm=%.12f scale=%.12g ocean=%d liquid=%d sameWorld=%d; no replacement or repair attempted"),
                    static_cast<int32>(Body->PlanetType), static_cast<int32>(Profile.PlanetType), Body->WorldScapeSeed, Root->Seed,
                    Body->RadiusKM, Root->PlanetScaleCode / 100000.0, Body->WorldScapePresentationScale,
                    Root->bOcean, static_cast<int32>(Profile.LiquidType), Root->GetWorld() == World));
            if (Guard.SlopeMode >= 0)
            {
                FString SlopeError;
                if (Root->TerrainMaterial.DefaultMaterial != Surface->ResolvedTerrainMaterialInstance)
                    return Finish(TEXT("slope isolation requires the actual native single terrain MID"));
                if (ObserveStarted == 0.0)
                {
                    const auto Ready = SlopeIsolation.Prepare(World, Surface->ResolvedTerrainMaterialInstance,
                        Guard.SlopeMode == 1, SlopeError);
                    if (Ready == APSSavedSlopeIsolation::EReadiness::Failed) return Finish(SlopeError);
                    if (Ready == APSSavedSlopeIsolation::EReadiness::Pending) return false;
                    Now = FPlatformTime::Seconds(); // Compilation is not part of the camera route.
                }
                else if (!SlopeIsolation.Verify(SlopeError)) return Finish(SlopeError);
            }
            if (ObserveStarted == 0.0)
            {
                if (Pawn->GetClass()->GetPathName() != TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter_SpeedModes.BP_CustomGravityCharacter_SpeedModes_C"))
                    return Finish(TEXT("Lidim route requires the actual saved character class"));
                OutputDir = Guard.SavedDir / TEXT("Screenshots/Windows") / (TEXT("SavedLidimApproach_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
                if (IFileManager::Get().DirectoryExists(*OutputDir) || !IFileManager::Get().MakeDirectory(*OutputDir, true))
                    return Finish(TEXT("could not create unique Lidim screenshot directory"));
                ObservedPawn = Pawn; ObservedRoot = Root; ObserveStarted = Now; NextAudit = NextCapture = Now;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] BEGIN body=BODY:Lidim controlled-camera-pawn-route=1 nativeShipPhysics=0 exactUserCamera=0 root=%s radiusKm=%.9f seed=%d terrainSeed=%d noise=%.6f/%.6f output=%s slopeMode=%d no-disk-material-or-save-edits visualAcceptance=unassessed"),
                    *Root->GetPathName(), Body->RadiusKM, Body->WorldScapeSeed, Profile.TerrainSeed,
                    Profile.NoiseScale, Profile.NoiseIntensity, *OutputDir, Guard.SlopeMode);
            }
            if (ObservedPawn.Get() != Pawn || ObservedRoot.Get() != Root)
                return Finish(TEXT("Lidim route pawn/root identity changed"));
            const double Elapsed = Now - ObserveStarted;
            if (!LidimRoute.Tick(Character, Body, Root, Elapsed)) return Finish(LidimRoute.GetError());
            UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
            if (!Origin) return Finish(TEXT("Lidim route generation-frame subsystem unavailable"));
            const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
            if (Now >= NextAudit)
            {
                NextAudit = Now + 0.125;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Pose] t=%.3f phase=%s frame=%llu rootGeneration=%s rootQuat=%s pawnGeneration=%s cameraGeneration=%s cameraRot=%s fov=%.3f"),
                    Elapsed, LidimRoute.Phase(), static_cast<unsigned long long>(GFrameCounter),
                    *Origin->ToGenerationFrame(Root->GetActorLocation()).ToString(), *Root->GetActorQuat().ToString(),
                    *Origin->ToGenerationFrame(Pawn->GetActorLocation()).ToString(), *Origin->ToGenerationFrame(Camera).ToString(),
                    *PC->PlayerCameraManager->GetCameraRotation().ToString(), PC->PlayerCameraManager->GetFOVAngle());
                if (LidimRoute.Ready())
                {
                    ++AuditCount;
                    if (!Audit.Tick(Root, Camera, Elapsed, LidimRoute.Phase())) return Finish(Audit.GetError());
                }
            }
            if (Now >= NextCapture)
            {
                NextCapture = Now + 0.125;
                FString Error;
                if (!Capture(World, Camera, Elapsed, Error)) return Finish(Error);
            }
            return LidimRoute.IsComplete() ? Finish(FString()) : false;
        }

        // Shallow GT-published visibility/bindings only; no worker buffers, setters
        // or resource creation. Not proof of GPU culling or same-MID uniform parity.
        void ObserveRepresentations(APlanetaryBody* Body, int32 BodyIndex, double Elapsed)
        {
            TArray<FString> Lines;
            bool Truncated = false;
            const auto MaterialText = [](UMaterialInterface* Material)
            {
                const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
                return FString::Printf(TEXT("material=%s id=%u parent=%s"), *GetPathNameSafe(Material),
                    IsValid(Material) ? Material->GetUniqueID() : 0u,
                    *GetPathNameSafe(Instance ? Instance->Parent.Get() : nullptr));
            };
            Lines.Add(FString::Printf(TEXT("body=%s id=%u"), *GetPathNameSafe(Body), IsValid(Body) ? Body->GetUniqueID() : 0u));
            if (IsValid(Body))
            {
                APlanetarySurfaceGenerator* Surface = Body->PlanetaryEnvironmentGenerator;
                AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
                Lines.Add(FString::Printf(TEXT("state=%d ready=%d bodyHidden=%d surface=%s root=%s rootHidden=%d"),
                    static_cast<int32>(Body->GetWorldScapeStreamingState()), Body->bWorldScapeSurfaceReady,
                    Body->IsHidden(), *GetPathNameSafe(Surface), *GetPathNameSafe(Root), IsValid(Root) ? Root->IsHidden() : -1));
                TInlineComponentArray<UStaticMeshComponent*> Shells;
                Body->GetComponents(Shells);
                Lines.Add(FString::Printf(TEXT("shellCount=%d"), Shells.Num()));
                Truncated |= Shells.Num() > 32;
                for (int32 I = 0; I < FMath::Min(Shells.Num(), 32); ++I)
                {
                    UStaticMeshComponent* Mesh = Shells[I];
                    if (!IsValid(Mesh)) continue;
                    Lines.Add(FString::Printf(TEXT("shell=%s id=%u registered=%d visible=%d hiddenInGame=%d mesh=%s transform=%s slots=%d"),
                        *Mesh->GetPathName(), Mesh->GetUniqueID(), Mesh->IsRegistered(), Mesh->IsVisible(), Mesh->bHiddenInGame,
                        *GetPathNameSafe(Mesh->GetStaticMesh()), *Mesh->GetComponentTransform().ToString(), Mesh->GetNumMaterials()));
                    Truncated |= Mesh->GetNumMaterials() > 16;
                    for (int32 SlotIndex = 0; SlotIndex < FMath::Min(Mesh->GetNumMaterials(), 16); ++SlotIndex)
                        Lines.Add(FString::Printf(TEXT("shellIndex=%d slot=%d %s"), I, SlotIndex, *MaterialText(Mesh->GetMaterial(SlotIndex))));
                }
                if (IsValid(Root))
                {
                    Lines.Add(FString::Printf(TEXT("rootLods=%d"), Root->WorldScapeLod.Num()));
                    Truncated |= Root->WorldScapeLod.Num() > 32;
                    for (int32 LodIndex = 0; LodIndex < FMath::Min(Root->WorldScapeLod.Num(), 32); ++LodIndex)
                    {
                        UWorldScapeLod* Lod = Root->WorldScapeLod[LodIndex];
                        UWorldScapeMeshComponent* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                        if (!IsValid(Mesh)) { Lines.Add(FString::Printf(TEXT("lod=%d mesh=missing"), LodIndex)); continue; }
                        Lines.Add(FString::Printf(TEXT("lod=%d mesh=%s id=%u registered=%d visible=%d hiddenInGame=%d sections=%d"),
                            LodIndex, *Mesh->GetPathName(), Mesh->GetUniqueID(), Mesh->IsRegistered(), Mesh->IsVisible(),
                            Mesh->bHiddenInGame, Mesh->GetNumSections()));
                        Truncated |= Mesh->GetNumSections() > 16;
                        for (int32 SectionIndex = 0; SectionIndex < FMath::Min(Mesh->GetNumSections(), 16); ++SectionIndex)
                        {
                            const FWorldScapeMeshSection* Section = Mesh->GetProcMeshSection(SectionIndex);
                            Lines.Add(FString::Printf(TEXT("lod=%d section=%d present=%d visible=%d vertices=%d indices=%d %s"),
                                LodIndex, SectionIndex, Section != nullptr, Section ? Section->bSectionVisible : 0,
                                Section ? Section->PlanetVertexBuffer.Num() : 0, Section ? Section->PlanetIndexBuffer.Num() : 0,
                                *MaterialText(Mesh->GetMaterial(SectionIndex))));
                        }
                    }
                }
            }
            const FString Snapshot = FString::Join(Lines, TEXT("\n")) + (Truncated ? TEXT("\nTRUNCATED") : TEXT("\ncomplete-within-caps"));
            if (Snapshot != RepresentationSnapshots[BodyIndex])
            {
                RepresentationSnapshots[BodyIndex] = Snapshot;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Representations] BEGIN bodyIndex=%d t=%.3f gtFrame=%llu truncated=%d scope=GT-bindings-not-GPU-visibility-or-uniforms"),
                    BodyIndex, Elapsed, static_cast<unsigned long long>(GFrameCounter), Truncated);
                for (const FString& Line : Lines) UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Representations] %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Representations] END bodyIndex=%d"), BodyIndex);
            }
        }

        bool Capture(UWorld* World, const FVector& Camera, double Elapsed, FString& Error)
        {
            UGameViewportClient* Client = AutomationCommon::GetAnyGameViewportClient();
            FViewport* Viewport = Client ? Client->Viewport : nullptr;
            if (!Viewport || Client->GetWorld() != World) { Error = TEXT("actual gameplay viewport unavailable"); return false; }
            const FIntPoint Size = Viewport->GetSizeXY();
            TArray<FColor> Pixels;
            if (Size.X <= 0 || Size.Y <= 0 || Size.X > 4096 || Size.Y > 2160
                || !Viewport->ReadPixels(Pixels) || Pixels.Num() != static_cast<int64>(Size.X) * Size.Y)
            { Error = TEXT("game viewport size/readback invalid"); return false; }
            const FString Path = OutputDir / FString::Printf(TEXT("frame_%04d.png"), CaptureCount);
            TArray64<uint8> Png;
            FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
            if (Png.IsEmpty() || !FFileHelper::SaveArrayToFile(Png, *Path, &IFileManager::Get(), FILEWRITE_NoReplaceExisting))
            { Error = TEXT("PNG encode/write failed; existing captures never overwritten"); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay.Capture] index=%d t=%.3f gtFrame=%llu cameraWorld=%s path=%s renderPoseCorrespondence=unfenced"),
                CaptureCount++, Elapsed, static_cast<unsigned long long>(GFrameCounter), *Camera.ToString(), *Path);
            return true;
        }

        bool Finish(const FString& Error)
        {
            if (!SlopeIsolation.ReleaseAfterCapture())
                Test->AddError(TEXT("[APS.SavedPlanetReplay] slope shader restoration failed; saving stays disabled"));
            CharacterRoute.Release();
            LidimRoute.Release();
            NativeGlobeRoute.Release();
            StolyavenaRoute.Release();
            if (Guard.bStolyavena && Error.IsEmpty() && !StolyavenaRoute.GetError().IsEmpty())
                Test->AddError(TEXT("[APS.SavedStolyavena] ") + StolyavenaRoute.GetError());
            if (Error.IsEmpty() && !NativeGlobeRoute.GetError().IsEmpty())
                Test->AddError(TEXT("[APS.SavedPlanetReplay] ") + NativeGlobeRoute.GetError());
            if (!Error.IsEmpty()) Test->AddError(TEXT("[APS.SavedPlanetReplay] ") + Error);
            // Capture a terminating visibility/root transition even between 8-Hz samples.
            APlanetaryBody* FinalBodies[2] = {nullptr, nullptr};
            if (UWorld* World = AutomationCommon::GetAnyGameWorld())
                for (TActorIterator<APlanetaryBody> It(World); It; ++It)
                {
                    if (Guard.bStolyavena)
                    {
                        if (*It == StolyavenaRoute.GetBody()) FinalBodies[0] = *It;
                    }
                    else
                    {
                        if (It->AstroName == FName(TEXT("Jaim"))) FinalBodies[0] = *It;
                        if (It->AstroName == FName(TEXT("Lidim"))) FinalBodies[1] = *It;
                    }
                }
            const double Elapsed = FPlatformTime::Seconds() - (LoadStarted > 0.0 ? LoadStarted : Started);
            for (int32 I = 0; I < 2; ++I) ObserveRepresentations(FinalBodies[I], I, Elapsed);
            FString GuardError;
            if (!Guard.Check(GuardError)) Test->AddError(TEXT("[APS.SavedPlanetReplay] ") + GuardError);
            if (Error.IsEmpty() && (AuditCount == 0 || CaptureCount == 0)) Test->AddError(TEXT("[APS.SavedPlanetReplay] no observation evidence"));
            if (Guard.bStolyavena && StolyavenaRoute.IsComplete() && StolyavenaCapturedHolds == 0x7f && !Test->HasAnyErrors())
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] COMPLETE route=80s body=Stolyavena capturedHolds=7 restoredObserverLease=1 nativeShipPhysics=0 exactIncidentCamera=0 visualAcceptance=unassessed"));
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedPlanetReplay] END diagnostics=%s audits=%d captures=%d elapsed=%.3f output=%s visualAcceptance=unassessed exactIncidentCamera=false cleanup=automation-lifecycle-only saves-not-deleted"),
                Test->HasAnyErrors() ? TEXT("FAIL") : TEXT("COMPLETE"), AuditCount, CaptureCount, FPlatformTime::Seconds() - Started, *OutputDir);
            return true; // Only owned route controls/camera released; no save, map travel or forced end-PIE.
        }

        FAutomationTestBase* Test;
        FGuard Guard;
        TStrongObjectPtr<UMaterialInstance> CandidateTemplate;
        TWeakObjectPtr<UGeneratedWorld> Model;
        TWeakObjectPtr<APawn> ObservedPawn;
        TWeakObjectPtr<AWorldScapeRoot> ObservedRoot;
        APSPublishedTerrainDescentAudit::FAudit Audit;
        APSSavedCharacterRoundTrip::FRoute CharacterRoute;
        APSSavedLidimApproach::FRoute LidimRoute;
        APSSavedStolyavenaApproach::FRoute StolyavenaRoute;
        APSSavedNativeGlobeRoundTrip::FRoute NativeGlobeRoute;
        APSSavedSlopeIsolation::FScope SlopeIsolation;
        FString OutputDir;
        FString RepresentationSnapshots[2];
        double NextRepresentation = 0.0;
        double CandidatePreparationStarted = 0.0, NextCandidateProgress = 0.0;
        double Started, LoadStarted = 0.0, PawnStarted = 0.0, ObserveStarted = 0.0, NextAudit = 0.0, NextCapture = 0.0, NextProgress = 0.0;
        const bool bPrepareOriginalWarpPixel = APSOriginalWarpPixelAssets::Requested();
        bool bCandidatePrepared = false;
        bool bLoadCalled = false;
        uint8 StolyavenaCapturedHolds = 0;
        uint8 StolyavenaHoldCaptureCounts[7] = {};
        uint64 StolyavenaHoldCaptureFrames[7] = {};
        int32 AuditCount = 0, CaptureCount = 0;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSExistingWorldPlanetReplayTest,
    "APS.Rendered.Gameplay.ExistingWorldPlanetReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSExistingWorldPlanetReplayTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("APSSavedPlanetReplay")))
    {
        if (FCString::Strifind(FCommandLine::Get(), TEXT("-APSSavedPlanet")))
        {
            AddError(TEXT("[APS.SavedPlanetReplay] malformed/incomplete replay opt-in"));
            return false;
        }
        AddInfo(TEXT("[APS.SavedPlanetReplay] SKIP: explicit opt-in absent; no map/load/captures; not visual acceptance"));
        return true;
    }
    APSExistingWorldPlanetReplayTests::FGuard Guard;
    FString Error;
    if (!Guard.Initialize(Error)) { AddError(TEXT("[APS.SavedPlanetReplay] ") + Error); return false; }
    const double Started = FPlatformTime::Seconds();
    if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
    { AddError(TEXT("[APS.SavedPlanetReplay] could not open normal main-menu PIE")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(APSExistingWorldPlanetReplayTests::FReplayCommand(this, Guard, Started));
    return true;
}

#endif
