#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Rendering/APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "BufferVisualizationData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/Texture.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"
#include "ShowFlags.h"
#include "ShaderCore.h"
#include "WorldScapeRoot.h"
#include "WorldScapeMeshComponent.h"
#include "UObject/UObjectIterator.h"

// Private exact-save observer diagnostic, not native flight acceptance. The
// caller owns isolated SavedDir/save-SHA guards and capture. Only pawn placement,
// movement lease and an owned camera are changed; streaming stays authoritative.
namespace APSSavedNativeGlobeRoundTrip
{
    inline FVector AnchorA() { return FVector(-9292423239211.45, -61775603144007.87, -1898397529769.63); }

    class FRoute
    {
    public:
        FRoute() = default;
        FRoute(const FRoute&) = delete;
        FRoute& operator=(const FRoute&) = delete;
        ~FRoute() { Release(); }
        bool Started() const { return bLeased; }
        bool Ready() const { return bSnapshot; }
        bool IsComplete() const { return Stage == 3; }
        const FString& GetError() const { return Error; }
        const TCHAR* Phase() const
        {
            if (!Error.IsEmpty()) return TEXT("failed");
            return Stage == 0 ? TEXT("anchor-A-active") : Stage == 1 ? TEXT("outside-all-unload-spheres")
                : Stage == 2 ? TEXT("anchor-A-native-return") : TEXT("complete");
        }

        bool Tick(ACustomGravityCharacter* Character, APlanetaryBody* Lidim, double Elapsed)
        {
            if (!Error.IsEmpty()) return false;
            if (!IsInGameThread()) return Fail(TEXT("native globe route requires game thread"));
            if (bReleased || !FMath::IsFinite(Elapsed) || Elapsed < 0.0 || !IsValid(Character) || !IsValid(Lidim))
                return Fail(TEXT("released route, invalid time or missing saved pawn/body"));
            UWorld* World = Character->GetWorld();
            auto* PC = Cast<APlayerController>(Character->GetController());
            auto* Move = Character->GetCharacterMovement();
            auto* Origin = IsValid(World) ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
            if (!IsValid(World) || !World->IsGameWorld() || !IsValid(PC) || !IsValid(PC->PlayerCameraManager)
                || !IsValid(Move) || !IsValid(Origin) || PC->GetPawn() != Character || Lidim->GetWorld() != World
                || !FMath::IsFinite(Lidim->RadiusKM) || Lidim->AstroName != FName(TEXT("Lidim"))
                || Lidim->PlanetType != EPlanetType::Frozen || Lidim->WorldScapeSeed != 257455
                || FMath::Abs(Lidim->RadiusKM - 1280.896) > .01 || Lidim->WorldScapePresentationScale != 1.0)
                return Fail(TEXT("saved Lidim/possession/full-scale world contract mismatch"));
            auto* Generator = Lidim->PlanetaryEnvironmentGenerator;
            if (!bLeased)
            {
                if (IsRunningCommandlet() || !FParse::Param(FCommandLine::Get(), TEXT("APSSavedPlanetNativeGlobeRoundTrip")))
                    return Fail(TEXT("explicit APSSavedPlanetNativeGlobeRoundTrip opt-in required"));
                const bool WantWireframe = FParse::Param(FCommandLine::Get(), TEXT("APSSavedPlanetGlobeWireframe"));
                const bool WantBaseColor = FParse::Param(FCommandLine::Get(), TEXT("APSSavedPlanetGlobeBaseColor"));
                const bool WantWorldNormal = FParse::Param(FCommandLine::Get(), TEXT("APSSavedPlanetGlobeWorldNormal"));
                const bool WantBuffer = WantBaseColor || WantWorldNormal;
                const TCHAR* BufferName = WantWorldNormal ? TEXT("WorldNormal") : TEXT("BaseColor");
                if ((int32(WantWireframe) + int32(WantBaseColor) + int32(WantWorldNormal) > 1) || ((WantWireframe || WantBuffer)
                    && FParse::Param(FCommandLine::Get(), TEXT("APSOriginalWarpPixelGlobe"))))
                    return Fail(TEXT("wireframe, BaseColor, WorldNormal and original-warp candidate are separate diagnostics"));
                auto* Viewport = World->GetGameViewport();
                if ((WantWireframe || WantBuffer) && (!FApp::IsUnattended() || !IsValid(Viewport)
                    || Viewport->GetWorld() != World || !Viewport->Viewport))
                    return Fail(TEXT("globe view-mode diagnostic requires unattended lifecycle route and this world's live game viewport"));
                const auto* DebugModes = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ForceDebugViewModes"));
                if ((WantWireframe || WantBuffer) && (!DebugModes || DebugModes->GetInt() != 1 || !AllowDebugViewmodes()))
                    return Fail(TEXT("globe view-mode diagnostic requires process-start r.ForceDebugViewModes=1 and AllowDebugViewmodes=true"));
                const auto* BufferTarget = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
                if (WantBuffer && (!BufferTarget || BufferTarget->GetString() != BufferName
                    || !GetBufferVisualizationData().GetMaterial(FName(BufferName))))
                    return Fail(TEXT("buffer visualization requires the registered engine material and matching startup target"));
                if (Character->IsSurfaceHandoffSuspended() || Move->MovementMode == MOVE_None
                    || Character->GetActorTransform().ContainsNaN() || !Finite(Move->Velocity))
                    return Fail(TEXT("cannot lease suspended/disabled/non-finite saved character"));
                Center = Origin->ToGenerationFrame(Lidim->GetActorLocation());
                RadiusCm = Lidim->GetWorldScapeBodyRadiusCm();
                const double Height = (FVector::Distance(AnchorA(), Center) - RadiusCm) / 100000.0;
                if (!Finite(Center) || !FMath::IsFinite(Height) || Height < 20.0 || Height > 32.0)
                    return Fail(TEXT("actual Lidim center does not match generation-frame anchor A"));
                const FMinimalViewInfo View = PC->PlayerCameraManager->GetCameraCacheView();
                if (!FMath::IsFinite(View.FOV) || View.FOV <= 0.f || View.FOV >= 180.f
                    || !FMath::IsFinite(View.AspectRatio) || View.AspectRatio <= 0.f
                    || !FMath::IsFinite(View.PostProcessBlendWeight) || View.ProjectionMode != ECameraProjectionMode::Perspective
                    || !View.OffCenterProjectionOffset.IsNearlyZero())
                    return Fail(TEXT("unsupported natural camera optics"));
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                auto* NewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), View.Location, View.Rotation, Params);
                if (!IsValid(NewCamera) || !IsValid(NewCamera->GetCameraComponent()))
                {
                    if (IsValid(NewCamera)) NewCamera->Destroy();
                    return Fail(TEXT("diagnostic camera creation failed"));
                }
                Pawn = Character; Body = Lidim; Player = PC; SavedWorld = World; OriginSystem = Origin;
                Parent = Character->GetAttachParentActor(); OriginalTarget = PC->GetViewTarget(); Camera = NewCamera;
                OriginalTransform = Character->GetActorTransform();
                OriginalTransform.SetLocation(Origin->ToGenerationFrame(OriginalTransform.GetLocation()));
                OriginalVelocity = Move->Velocity; OriginalMode = Move->MovementMode; OriginalCustom = Move->CustomMovementMode;
                auto* Lens = NewCamera->GetCameraComponent();
                Lens->SetFieldOfView(View.FOV); Lens->SetAspectRatio(View.AspectRatio);
                Lens->SetConstraintAspectRatio(View.bConstrainAspectRatio); Lens->SetProjectionMode(View.ProjectionMode);
                Lens->SetUseFieldOfViewForLOD(View.bUseFieldOfViewForLOD);
                Lens->bOverrideAspectRatioAxisConstraint = View.AspectRatioAxisConstraint.IsSet();
                if (View.AspectRatioAxisConstraint.IsSet()) Lens->SetAspectRatioAxisConstraint(View.AspectRatioAxisConstraint.GetValue());
                Lens->PostProcessSettings = View.PostProcessSettings; Lens->PostProcessBlendWeight = View.PostProcessBlendWeight;
                StartElapsed = StageStarted = LastElapsed = Elapsed; bLeased = true;
                if (WantWireframe || WantBuffer)
                {
                    DiagnosticViewport = Viewport;
                    OriginalViewMode = Viewport->ViewModeIndex;
                    OriginalShowFlags = Viewport->EngineShowFlags;
                    const EViewModeIndex DiagnosticMode = WantBuffer ? VMI_VisualizeBuffer : VMI_Wireframe;
                    Viewport->ViewModeIndex = DiagnosticMode;
                    ApplyViewMode(DiagnosticMode, true, Viewport->EngineShowFlags);
                    if (WantWireframe) Viewport->EngineShowFlags.SetMaterials(false);
                    bViewModeLease = true; BufferViewTarget = WantBuffer ? BufferName : TEXT("");
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] VIEWMODE kind=%s viewport=%s previousMode=%d previousWireframe=%d previousMaterials=%d previousVisualizeBuffer=%d diagnostic-only-not-lit-acceptance"),
                        WantBuffer ? BufferName : TEXT("Wireframe"), *Viewport->GetPathName(), OriginalViewMode,
                        int32(OriginalShowFlags.Wireframe), int32(OriginalShowFlags.Materials), int32(OriginalShowFlags.VisualizeBuffer));
                }
                Move->DisableMovement(); PC->SetViewTarget(NewCamera);
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] BEGIN controlled-relocation-not-native-flight same-observer-camera same-optics no-root-state-material-save-writes fov=%.6f aspect=%.6f centerGeneration=%s"),
                    View.FOV, View.AspectRatio, *Center.ToString());
            }
            if (Pawn.Get() != Character || Body.Get() != Lidim || Player.Get() != PC || SavedWorld.Get() != World
                || OriginSystem.Get() != Origin || Parent.Get() != Character->GetAttachParentActor()
                || !Camera.IsValid() || PC->GetViewTarget() != Camera.Get()
                || Lidim->GetWorldScapeBodyRadiusCm() != RadiusCm
                || !Origin->ToGenerationFrame(Lidim->GetActorLocation()).Equals(Center, 100.0))
                return Fail(TEXT("leased ownership, camera, body radius or generation center changed"));
            if (Elapsed < LastElapsed || Elapsed - StartElapsed > 90.0 || Elapsed - StageStarted > 30.0)
                return Fail(TEXT("non-monotonic time or bounded 30s-stage/90s-total timeout"));
            LastElapsed = Elapsed;
            if (IsComplete()) return true;
            if (Move->MovementMode != MOVE_None) Move->DisableMovement();
            if (bSnapshot && (!IsValid(Generator) || ProfileSignature(Generator) != ProfileHash))
                return Fail(TEXT("saved native surface profile changed during round trip"));
            if (!Place(Character, Origin, Stage == 1 ? FarGeneration : AnchorA())) return false;
            if (bViewModeLease && Elapsed >= NextViewModeAudit)
            {
                NextViewModeAudit = Elapsed + .25;
                if (!AuditViewMode(Elapsed)) return false;
            }
            auto* Root = IsValid(Generator) ? Generator->WorldScapeRootInstance : nullptr;
            bool StageReady = false;
            if (Stage == 0 || Stage == 2)
            {
                if (Stage == 2 && !bReturnBeforeAudited)
                {
                    bReturnBeforeAudited = true;
                    AuditRepresentatives(Lidim, Root, Origin, Elapsed, TEXT("return-before-ready"));
                }
                StageReady = NativeReady(Lidim, Generator, Root) && !Character->IsSurfaceHandoffSuspended();
                if (StageReady)
                {
                    auto* MID = Cast<UMaterialInstanceDynamic>(Root->TerrainMaterial.DefaultMaterial);
                    if (!IsValid(MID) || !IsValid(MID->Parent.Get()) || Root->Seed != 257455 || Root->bOcean
                        || double(Root->PlanetScale) != double(float(RadiusCm))
                        || !Origin->ToGenerationFrame(Root->GetActorLocation()).Equals(Center, 100.0))
                        return Fail(TEXT("published native root material/seed/radius/ocean mismatch"));
                    if (!bSnapshot)
                    {
                        MaterialParent = MID->Parent->GetPathName(); ProfileHash = ProfileSignature(Generator);
                        RootRadius = Root->PlanetScale; NoiseScale = Root->NoiseScale; NoiseIntensity = Root->NoiseIntensity;
                        FirstRootID = Root->GetUniqueID(); bSnapshot = true;
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] SNAPSHOT root=%s id=%u seed=%d radius=%.9f profile=%u parent=%s noise=%.9f/%.9f"),
                            *Root->GetPathName(), FirstRootID, Root->Seed, RootRadius, ProfileHash, *MaterialParent, NoiseScale, NoiseIntensity);
                        AuditRepresentatives(Lidim, Root, Origin, Elapsed, TEXT("initial-native-ready"));
                    }
                    if (MID->Parent->GetPathName() != MaterialParent || Root->PlanetScale != RootRadius
                        || Root->NoiseScale != NoiseScale || Root->NoiseIntensity != NoiseIntensity
                        || ProfileSignature(Generator) != ProfileHash)
                        return Fail(TEXT("native return changed saved parent/profile/radius/noise"));
                    if (Stage == 2 && Root->GetUniqueID() == FirstRootID)
                        return Fail(TEXT("return reused original root despite observed root-absent unload"));
                    if (Stage == 2 && !bReturnReadyAudited)
                    {
                        bReturnReadyAudited = true;
                        AuditRepresentatives(Lidim, Root, Origin, Elapsed, TEXT("return-native-ready"));
                    }
                }
            }
            else
            {
                if (!OutsideAllBodies(World, Origin, FarGeneration)) return Fail(TEXT("far observer is no longer outside every generated body's unload radius"));
                StageReady = Lidim->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
                    && !IsValid(Root) && !Lidim->bWorldScapeSurfaceReady && ClosedGlobeReady(Lidim);
                if (!Error.IsEmpty()) return false;
            }
            if (StageReady)
            {
                if (StableSince < 0.0) StableSince = Elapsed;
                if (Elapsed - StableSince >= 2.0)
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] STAGE_READY phase=%s t=%.3f state=%d root=%s parent=%s"),
                        Phase(), Elapsed - StartElapsed, int32(Lidim->GetWorldScapeStreamingState()), *GetPathNameSafe(Root), *MaterialParent);
                    if (Stage == 0 && !ChooseFar(World, Origin)) return false;
                    ++Stage; StageStarted = Elapsed; StableSince = -1.0;
                    if (Stage < 3 && !Place(Character, Origin, Stage == 1 ? FarGeneration : AnchorA())) return false;
                    StageReady = Stage == 3;
                }
            }
            else StableSince = -1.0;
            if (Stage != LastPMCAuditStage || (Stage == 2 && Elapsed >= NextPMCAudit))
            {
                const bool bPhaseChanged = Stage != LastPMCAuditStage;
                LastPMCAuditStage = Stage; NextPMCAudit = Elapsed + .25;
                AuditClosedPMCs(Lidim, Origin, Elapsed);
                AuditLightFrame(Origin, Elapsed, bPhaseChanged);
            }
            if (Elapsed >= NextLog)
            {
                NextLog = Elapsed + 1.0;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] t=%.3f phase=%s ready=%d state=%d bodyReady=%d root=%s observerGeneration=%s cameraGeneration=%s profile=%u"),
                    Elapsed - StartElapsed, Phase(), StageReady, int32(Lidim->GetWorldScapeStreamingState()), Lidim->bWorldScapeSurfaceReady,
                    *GetPathNameSafe(Root), *Origin->ToGenerationFrame(Character->GetActorLocation()).ToString(),
                    *Origin->ToGenerationFrame(Camera->GetActorLocation()).ToString(), ProfileHash);
            }
            return true;
        }

        void Release()
        {
            if (!IsInGameThread() || bReleased) return;
            bReleased = true;
            if (bViewModeLease)
            {
                auto* Viewport = DiagnosticViewport.Get();
                if (IsValid(Viewport) && Viewport->GetWorld() == SavedWorld.Get())
                {
                    Viewport->ViewModeIndex = OriginalViewMode;
                    Viewport->EngineShowFlags = OriginalShowFlags;
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] VIEWMODE restored kind=%s mode=%d all-original-showflags wireframe=%d materials=%d visualizeBuffer=%d"),
                        BufferViewTarget.IsEmpty() ? TEXT("Wireframe") : *BufferViewTarget, OriginalViewMode,
                        int32(OriginalShowFlags.Wireframe), int32(OriginalShowFlags.Materials), int32(OriginalShowFlags.VisualizeBuffer));
                }
                else RestoreError(TEXT("diagnostic viewport/world ownership lost; flag restoration refused"));
                bViewModeLease = false;
            }
            auto* Character = Pawn.Get(); auto* PC = Player.Get(); auto* Origin = OriginSystem.Get();
            auto* OwnedCamera = Camera.Get();
            if (IsValid(PC) && IsValid(OwnedCamera) && PC->GetViewTarget() == OwnedCamera)
            {
                AActor* Target = OriginalTarget.Get();
                if (!IsValid(Target)) Target = Character;
                if (IsValid(Target) && Target->GetWorld() == SavedWorld.Get()) PC->SetViewTarget(Target);
            }
            if (bLeased && IsValid(Character) && IsValid(PC) && IsValid(Origin)
                && Character->GetWorld() == SavedWorld.Get() && Character->GetController() == PC && PC->GetPawn() == Character
                && Parent.Get() == Character->GetAttachParentActor())
            {
                FTransform Restore = OriginalTransform;
                Restore.SetLocation(Origin->FromGenerationFrame(Restore.GetLocation()));
                Character->SetActorTransform(Restore, false, nullptr, ETeleportType::TeleportPhysics);
                if (!Character->GetActorTransform().Equals(Restore, .01))
                    RestoreError(TEXT("original generation-frame pawn transform restoration failed"));
                if (auto* Move = Character->GetCharacterMovement())
                {
                    if (Move->MovementMode == MOVE_None && !Character->IsSurfaceHandoffSuspended())
                    {
                        Move->SetMovementMode(OriginalMode, OriginalCustom);
                        Move->Velocity = OriginalVelocity; // Restore only the value owned by this lease.
                    }
                    else RestoreError(TEXT("native movement ownership changed; mode/velocity restoration refused"));
                }
            }
            else if (bLeased) RestoreError(TEXT("original pawn/world/possession lost; restoration refused"));
            if (IsValid(OwnedCamera)) OwnedCamera->Destroy();
            Camera.Reset();
        }

    private:
        bool AuditViewMode(double Elapsed)
        {
            auto* Viewport = DiagnosticViewport.Get();
            if (!IsValid(Viewport) || Viewport->GetWorld() != SavedWorld.Get())
                return Fail(TEXT("diagnostic viewport/world ownership changed"));
            const auto* DebugModes = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ForceDebugViewModes"));
            const bool Allowed = AllowDebugViewmodes();
            // Reproduce Draw's override on a copy only. This is GT configuration
            // evidence, not proof that a rendered frame contains wire edges.
            FEngineShowFlags DrawFlags = Viewport->EngineShowFlags;
            EngineShowFlagOverride(ESFIM_Game, EViewModeIndex(Viewport->ViewModeIndex), DrawFlags, false);
            if (!BufferViewTarget.IsEmpty())
            {
                const auto* Target = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
                const FString TargetName = Target ? Target->GetString() : TEXT("<missing>");
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.%s] t=%.3f phase=%s mode=%d expectedMode=%d viewportVisualizeBuffer=%d drawDerivedVisualizeBuffer=%d drawDerivedMaterials=%d drawDerivedWire=%d drawDerivedPostProcessing=%d target=%s forceDebug=%d allowDebug=%d GT-config-only-not-rendered-proof"),
                    *BufferViewTarget, Elapsed - StartElapsed, Phase(), Viewport->ViewModeIndex, int32(VMI_VisualizeBuffer),
                    int32(Viewport->EngineShowFlags.VisualizeBuffer), int32(DrawFlags.VisualizeBuffer),
                    int32(DrawFlags.Materials), int32(DrawFlags.Wireframe), int32(DrawFlags.PostProcessing), *TargetName,
                    DebugModes ? DebugModes->GetInt() : -1, Allowed);
                if (Viewport->ViewModeIndex != VMI_VisualizeBuffer || !DrawFlags.VisualizeBuffer
                    || !DrawFlags.Materials || DrawFlags.Wireframe || !DrawFlags.PostProcessing || !Allowed || !DebugModes || DebugModes->GetInt() != 1
                    || TargetName != BufferViewTarget)
                    return Fail(TEXT("buffer mode/Draw flags/target/debug permission lost; buffer diagnostic invalid"));
                return true;
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Wireframe] t=%.3f phase=%s mode=%d expectedMode=%d viewportWire=%d viewportMaterials=%d drawDerivedWire=%d drawDerivedMaterials=%d forceDebug=%d allowDebug=%d GT-config-only-not-rendered-proof"),
                Elapsed - StartElapsed, Phase(), Viewport->ViewModeIndex, int32(VMI_Wireframe),
                int32(Viewport->EngineShowFlags.Wireframe), int32(Viewport->EngineShowFlags.Materials),
                int32(DrawFlags.Wireframe), int32(DrawFlags.Materials), DebugModes ? DebugModes->GetInt() : -1, Allowed);
            if (Viewport->ViewModeIndex != VMI_Wireframe || !DrawFlags.Wireframe || DrawFlags.Materials || !Allowed)
                return Fail(TEXT("wireframe mode/Draw flags/debug permission lost; geometry diagnostic invalid"));
            return true;
        }
        static void AuditOverrides(const TCHAR* Event, const TCHAR* Role, UMaterialInterface* Material)
        {
            const auto* MID = Cast<UMaterialInstanceDynamic>(Material);
            if (!IsValid(MID))
            {
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Parameters] event=%s role=%s material=%s MID=0"), Event, Role, *GetPathNameSafe(Material));
                return;
            }
            const auto Key = [](const FMaterialParameterInfo& I)
            { return FString::Printf(TEXT("%s[%d:%d]"), *I.Name.ToString(), int32(I.Association), I.Index); };
            FString Scalars, Vectors, Doubles, Textures;
            for (const auto& V : MID->ScalarParameterValues) Scalars += FString::Printf(TEXT(" %s=%.9g"), *Key(V.ParameterInfo), double(V.ParameterValue));
            for (const auto& V : MID->VectorParameterValues) Vectors += FString::Printf(TEXT(" %s=(%.9g,%.9g,%.9g,%.9g)"),
                *Key(V.ParameterInfo), double(V.ParameterValue.R), double(V.ParameterValue.G), double(V.ParameterValue.B), double(V.ParameterValue.A));
            for (const auto& V : MID->DoubleVectorParameterValues) Doubles += FString::Printf(TEXT(" %s=(%.17g,%.17g,%.17g,%.17g)"),
                *Key(V.ParameterInfo), V.ParameterValue.X, V.ParameterValue.Y, V.ParameterValue.Z, V.ParameterValue.W);
            for (const auto& V : MID->TextureParameterValues) Textures += FString::Printf(TEXT(" %s=%s"), *Key(V.ParameterInfo), *GetPathNameSafe(V.ParameterValue.Get()));
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Parameters] event=%s role=%s MID=%s parent=%s scalarCount=%d vectorCount=%d doubleCount=%d textureCount=%d fontCount=%d RVTCount=%d sparseCount=%d overrides-only textures-names-only"),
                Event, Role, *MID->GetPathName(), *GetPathNameSafe(MID->Parent.Get()), MID->ScalarParameterValues.Num(), MID->VectorParameterValues.Num(),
                MID->DoubleVectorParameterValues.Num(), MID->TextureParameterValues.Num(), MID->FontParameterValues.Num(),
                MID->RuntimeVirtualTextureParameterValues.Num(), MID->SparseVolumeTextureParameterValues.Num());
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Parameters] event=%s role=%s scalars:%s"), Event, Role, *Scalars);
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Parameters] event=%s role=%s vectors:%s doubles:%s textures:%s"), Event, Role, *Vectors, *Doubles, *Textures);
        }
        // Three bounded snapshots, never a recurring full-vertex scan. Native
        // data comes only from GT-published Mesh sections, not Lod worker arrays.
        void AuditRepresentatives(APlanetaryBody* B, AWorldScapeRoot* R, UAPSWorldOriginSubsystem* Origin,
            double Elapsed, const TCHAR* Event) const
        {
            const FVector Query = Camera->GetActorLocation();
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Sample] event=%s t=%.3f cameraWorld=%s cameraGeneration=%s anchorAWorld=%s root=%s"),
                Event, Elapsed - StartElapsed, *Query.ToString(), *Origin->ToGenerationFrame(Query).ToString(),
                *Origin->FromGenerationFrame(AnchorA()).ToString(), *GetPathNameSafe(R));
            struct FNearest
            {
                double DistanceSq = TNumericLimits<double>::Max();
                FVector Point = FVector::ZeroVector, Normal = FVector::ZeroVector;
                FColor Color = FColor::Black;
                int32 Section = -1, Index = -1;
            };
            const auto Scan = [&Query](const auto& Vertices, int32 Section, const FTransform& Transform, FNearest& N)
            {
                const FVector Scale = Transform.GetScale3D();
                if (Scale.ContainsNaN() || FMath::Abs(Scale.X) < UE_DOUBLE_SMALL_NUMBER
                    || FMath::Abs(Scale.Y) < UE_DOUBLE_SMALL_NUMBER || FMath::Abs(Scale.Z) < UE_DOUBLE_SMALL_NUMBER) return;
                const FVector InverseScale(1.0 / Scale.X, 1.0 / Scale.Y, 1.0 / Scale.Z);
                for (int32 I = 0; I < Vertices.Num(); ++I)
                {
                    const auto& V = Vertices[I];
                    const FVector Point = Transform.TransformPosition(V.Position);
                    const double DistanceSq = FVector::DistSquared(Query, Point);
                    if (!FMath::IsFinite(DistanceSq) || DistanceSq >= N.DistanceSq) continue;
                    N.DistanceSq = DistanceSq; N.Point = Point;
                    N.Normal = Transform.GetRotation().RotateVector(V.Normal * InverseScale).GetSafeNormal();
                    N.Color = V.Color; N.Section = Section; N.Index = I;
                }
            };
            const auto LogNearest = [&](const TCHAR* Role, UMeshComponent* Mesh, const FNearest& N)
            {
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Sample] event=%s role=%s mesh=%s available=%d registered=%d visible=%d hidden=%d section=%d vertex=%d distanceCm=%.9g world=%s normalWorld=%s rgba8=(%u,%u,%u,%u) radialDot=%.9g"),
                    Event, Role, *GetPathNameSafe(Mesh), N.Index >= 0, IsValid(Mesh) && Mesh->IsRegistered(), IsValid(Mesh) && Mesh->IsVisible(),
                    IsValid(Mesh) && Mesh->bHiddenInGame, N.Section, N.Index, N.Index >= 0 ? FMath::Sqrt(N.DistanceSq) : -1.0,
                    *N.Point.ToString(), *N.Normal.ToString(), uint32(N.Color.R), uint32(N.Color.G), uint32(N.Color.B), uint32(N.Color.A),
                    N.Index >= 0 ? FVector::DotProduct(N.Normal, (N.Point - B->GetActorLocation()).GetSafeNormal()) : -2.0);
            };
            int32 ClosedCount = 0;
            TInlineComponentArray<UProceduralMeshComponent*> Closed(B);
            for (auto* M : Closed) if (IsValid(M) && M->ComponentHasTag(TEXT("APS.NativeClosedTerrain")))
            {
                ++ClosedCount;
                AuditOverrides(Event, *M->GetName(), M->GetMaterial(0));
                FNearest N;
                for (int32 S = 0; S < M->GetNumSections(); ++S)
                    if (const auto* Section = M->GetProcMeshSection(S)) Scan(Section->ProcVertexBuffer, S, M->GetComponentTransform(), N);
                LogNearest(TEXT("closed-terrain"), M, N);
            }
            if (ClosedCount == 0) UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.Sample] event=%s closedTerrainCount=0"), Event);
            AuditOverrides(Event, TEXT("native-root"), IsValid(R) ? R->TerrainMaterial.DefaultMaterial : nullptr);
            UWorldScapeMeshComponent* NativeMesh = IsValid(R) && !R->WorldScapeLod.IsEmpty() && IsValid(R->WorldScapeLod[0])
                ? R->WorldScapeLod[0]->Mesh : nullptr;
            FNearest Native;
            if (IsValid(NativeMesh))
                for (int32 S = 0; S < NativeMesh->GetNumSections(); ++S)
                    if (const auto* Section = NativeMesh->GetProcMeshSection(S)) Scan(Section->PlanetVertexBuffer, S, NativeMesh->GetComponentTransform(), Native);
            LogNearest(TEXT("native-Lod0"), NativeMesh, Native);
        }
        // Read the selected cache and the exact actor identity without selecting or
        // refreshing anything. Directional-light forward is the star-to-observer ray.
        static FString OwnerTags(const AActor* Owner)
        {
            FString Result;
            if (IsValid(Owner))
                for (const FName Tag : Owner->Tags)
                    Result += (Result.IsEmpty() ? TEXT("") : TEXT(",")) + Tag.ToString();
            return Result;
        }

        void LogShadowPrimitive(const TCHAR* Role, const UPrimitiveComponent* Mesh,
            const FVector& Observer, const FVector& TowardStar, double Elapsed) const
        {
            if (!IsValid(Mesh)) return;
            const AActor* Owner = Mesh->GetOwner();
            const FVector Delta = Mesh->Bounds.Origin - Observer;
            const bool bRay = !TowardStar.IsNearlyZero() && Finite(Delta);
            const double Along = bRay ? FVector::DotProduct(Delta, TowardStar) : 0.0;
            const double Gap = bRay ? (Delta - TowardStar * Along).Size() : -1.0;
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.ShadowPrimitive] t=%.3f phase=%s role=%s component=%s owner=%s tags=%s registered=%d renderState=%d visible=%d hidden=%d ownerHidden=%d castShadow=%d dynamicShadow=%d staticShadow=%d hiddenShadow=%d boundsCenter=%s boundsDistanceCm=%.9g radiusCm=%.9g towardStarCm=%.9g rayGapCm=%.9g rayValid=%d GT-candidate-not-proven-occluder"),
                Elapsed - StartElapsed, Phase(), Role, *Mesh->GetPathName(), *GetPathNameSafe(Owner), *OwnerTags(Owner),
                int32(Mesh->IsRegistered()), int32(Mesh->IsRenderStateCreated()), int32(Mesh->IsVisible()),
                int32(Mesh->bHiddenInGame), IsValid(Owner) ? int32(Owner->IsHidden()) : -1,
                int32(Mesh->CastShadow), int32(Mesh->bCastDynamicShadow), int32(Mesh->bCastStaticShadow), int32(Mesh->bCastHiddenShadow),
                *Mesh->Bounds.Origin.ToString(), Delta.Size(), double(Mesh->Bounds.SphereRadius), Along, Gap, int32(bRay));
        }

        // One read-only enumeration per route phase. The geometric filter is a
        // conservative sphere/ray candidate list, not renderer shadow evidence.
        void AuditShadowCasters(UWorld* World, AStar* Star, const FVector& Observer,
            const FVector& ActualRay, double Elapsed) const
        {
            const FVector TowardStar = -ActualRay;
            if (IsValid(Star))
            {
                LogShadowPrimitive(TEXT("selected-star-photosphere"), Star->StarMesh, Observer, TowardStar, Elapsed);
                LogShadowPrimitive(TEXT("selected-star-corona"), Star->CoronaMesh, Observer, TowardStar, Elapsed);
            }
            for (TActorIterator<AAstroGenerator> It(World); It; ++It)
                if (IsValid(*It))
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.ShadowGenerator] t=%.3f phase=%s generator=%s tags=%s fullScale=%d consumed=%d integrateStart=%d homeStar=%s selectedStar=%s homeIsSelected=%d"),
                        Elapsed - StartElapsed, Phase(), *It->GetPathName(), *OwnerTags(*It), int32(It->bGenerateFullScaledWorld),
                        int32(It->GetCanonicalStellarProjectionDescriptor().bConsumedFinalizedDataset), int32(It->bIntegrateStartPlanet),
                        *GetPathNameSafe(It->HomeStar), *GetPathNameSafe(Star), int32(IsValid(Star) && It->HomeStar == Star));
            // Deliberately independent of visibility, CastShadow and the ray:
            // hidden BP-authored spheres may serialize bCastHiddenShadow=true.
            int32 BodyMeshCount = 0;
            for (TActorIterator<APlanetaryBody> It(World); It; ++It)
            {
                if (!IsValid(*It)) continue;
                TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
                for (auto* Mesh : Meshes)
                    if (IsValid(Mesh))
                    {
                        ++BodyMeshCount;
                        LogShadowPrimitive(TEXT("body-static-all"), Mesh, Observer, TowardStar, Elapsed);
                    }
            }
            struct FCandidate { UPrimitiveComponent* Mesh; double Projected; };
            TArray<FCandidate> Closest;
            Closest.Reserve(33);
            int32 Eligible = 0;
            if (!TowardStar.IsNearlyZero())
                for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
                {
                    UPrimitiveComponent* Mesh = *It;
                    if (!IsValid(Mesh) || Mesh->GetWorld() != World || Mesh->IsTemplate()
                        || !Mesh->IsRegistered() || !Mesh->CastShadow) continue;
                    const AActor* Owner = Mesh->GetOwner();
                    const bool bVisible = Mesh->IsVisible() && !Mesh->bHiddenInGame
                        && (!IsValid(Owner) || !Owner->IsHidden());
                    if (!bVisible && !Mesh->bCastHiddenShadow) continue;
                    const FVector Delta = Mesh->Bounds.Origin - Observer;
                    const double Radius = Mesh->Bounds.SphereRadius;
                    if (!Finite(Delta) || !FMath::IsFinite(Radius) || Radius < 0.0) continue;
                    const double Along = FVector::DotProduct(Delta, TowardStar);
                    const double Gap = (Delta - TowardStar * Along).Size();
                    if (Along + Radius <= 0.0 || Gap > Radius + 1000000.0) continue;
                    ++Eligible;
                    Closest.Add({Mesh, FMath::Max(0.0, Along - Radius)});
                    Closest.Sort([](const FCandidate& A, const FCandidate& B)
                    {
                        return A.Projected == B.Projected ? A.Mesh->GetPathName() < B.Mesh->GetPathName() : A.Projected < B.Projected;
                    });
                    if (Closest.Num() > 32) Closest.RemoveAt(32, 1, EAllowShrinking::No);
                }
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.ShadowCasters] t=%.3f phase=%s rayValid=%d towardStar=%s bodyStaticMeshes=%d eligible=%d emitted=%d truncated=%d marginCm=1000000 sort=forward-sphere-entry"),
                Elapsed - StartElapsed, Phase(), int32(!TowardStar.IsNearlyZero()), *TowardStar.ToString(), BodyMeshCount,
                Eligible, Closest.Num(), int32(Eligible > Closest.Num()));
            for (const FCandidate& Entry : Closest)
                LogShadowPrimitive(TEXT("ray-candidate"), Entry.Mesh, Observer, TowardStar, Elapsed);
        }

        void AuditLightFrame(UAPSWorldOriginSubsystem* Origin, double Elapsed, bool bPhaseChanged) const
        {
            UWorld* World = SavedWorld.Get();
            if (!IsValid(World) || !IsValid(Origin) || !Pawn.IsValid()) return;
            const FVector Observer = Pawn->GetActorLocation();
            auto* Stellar = World->GetSubsystem<UAPSStellarVisualSubsystem>();
            FVector CachedWorld = FVector::ZeroVector;
            FString Identity;
            const bool bCached = Stellar && Stellar->GetActiveStellarTarget(CachedWorld, Identity) && Finite(CachedWorld);
            AStar* ActualStar = nullptr;
            if (bCached)
                for (TActorIterator<AStar> It(World); It; ++It)
                    if (It->GetPathName() == Identity) { ActualStar = *It; break; }
            const FVector ActualWorld = IsValid(ActualStar) ? ActualStar->GetActorLocation() : FVector::ZeroVector;
            const bool bActual = IsValid(ActualStar) && Finite(ActualWorld);
            const FVector CachedRay = bCached ? (Observer - CachedWorld).GetSafeNormal() : FVector::ZeroVector;
            const FVector ActualRay = bActual ? (Observer - ActualWorld).GetSafeNormal() : FVector::ZeroVector;
            const auto Angle = [](const FVector& A, const FVector& B)
            {
                return A.IsNearlyZero() || B.IsNearlyZero() ? -1.0
                    : FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B), -1.0, 1.0)));
            };
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.StellarFrame] t=%.3f phase=%s identity=%s cachedValid=%d actualFound=%d cachedWorld=%s actualWorld=%s cacheActualDistanceKm=%.9f cacheActualRayAngleDeg=%.9f observerWorld=%s origin=%s engineOrigin=%s read-only-GT"),
                Elapsed - StartElapsed, Phase(), *Identity, int32(bCached), int32(bActual), *CachedWorld.ToString(),
                *ActualWorld.ToString(), bCached && bActual ? FVector::Distance(CachedWorld, ActualWorld) / 100000.0 : -1.0,
                Angle(CachedRay, ActualRay), *Observer.ToString(), *Origin->GetOriginOffset().ToString(), *World->OriginLocation.ToString());
            int32 Count = 0;
            for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
            {
                auto* Light = *It;
                if (!IsValid(Light) || Light->GetWorld() != World || Light->IsTemplate()) continue;
                ++Count;
                const FVector Forward = Light->GetForwardVector().GetSafeNormal();
                const AActor* Owner = Light->GetOwner();
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.DirectionalLight] t=%.3f phase=%s component=%s owner=%s forward=%s angleToCachedRayDeg=%.9f angleToActualRayDeg=%.9f intensity=%.9g registered=%d visible=%d hiddenInGame=%d ownerHidden=%d affectsWorld=%d castShadows=%d dynamicShadows=%d staticShadows=%d ownerTags=%s origin=%s"),
                    Elapsed - StartElapsed, Phase(), *Light->GetPathName(), *GetPathNameSafe(Owner), *Forward.ToString(),
                    Angle(Forward, CachedRay), Angle(Forward, ActualRay), double(Light->Intensity), int32(Light->IsRegistered()),
                    int32(Light->IsVisible()), int32(Light->bHiddenInGame), IsValid(Owner) ? int32(Owner->IsHidden()) : -1,
                    int32(Light->bAffectsWorld), int32(Light->CastShadows), int32(Light->CastDynamicShadows),
                    int32(Light->CastStaticShadows), *OwnerTags(Owner), *Origin->GetOriginOffset().ToString());
            }
            if (Count == 0) UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.DirectionalLight] t=%.3f phase=%s count=0"), Elapsed - StartElapsed, Phase());
            if (bPhaseChanged) AuditShadowCasters(World, ActualStar, Observer, ActualRay, Elapsed);
        }

        // GT readback only: stage boundaries plus 4Hz during native return.
        // Includes hidden/pending closed meshes; no refresh, bounds update or RT flush.
        void AuditClosedPMCs(APlanetaryBody* B, UAPSWorldOriginSubsystem* Origin, double Elapsed) const
        {
            auto* G = B->PlanetaryEnvironmentGenerator;
            auto* R = IsValid(G) ? G->WorldScapeRootInstance : nullptr;
            auto* BodyRoot = B->GetRootComponent();
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.PMC] t=%.3f phase=%s body=%s bodyTransform=%s bodyGeneration=%s bodyHidden=%d bodyRootVisible=%d bodyRootHidden=%d state=%d bodyReady=%d root=%s rootHidden=%d origin=%s"),
                Elapsed - StartElapsed, Phase(), *B->GetPathName(), *B->GetActorTransform().ToString(),
                *Origin->ToGenerationFrame(B->GetActorLocation()).ToString(), B->IsHidden(),
                IsValid(BodyRoot) && BodyRoot->IsVisible(), IsValid(BodyRoot) && BodyRoot->bHiddenInGame,
                int32(B->GetWorldScapeStreamingState()), B->bWorldScapeSurfaceReady,
                *GetPathNameSafe(R), IsValid(R) ? int32(R->IsHidden()) : -1, *Origin->GetOriginOffset().ToString());
            int32 Count = 0;
            TInlineComponentArray<UProceduralMeshComponent*> Meshes(B);
            for (auto* M : Meshes)
            {
                if (!IsValid(M) || (!M->ComponentHasTag(TEXT("APS.NativeClosedTerrain"))
                    && !M->ComponentHasTag(TEXT("APS.NativeClosedOcean")))) continue;
                ++Count;
                auto* Frame = M->GetAttachParent();
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.PMC] t=%.3f phase=%s mesh=%s registered=%d visible=%d hidden=%d transform=%s generation=%s worldBounds=%s sections=%d frame=%s frameRegistered=%d frameVisible=%d frameHidden=%d frameTransform=%s"),
                    Elapsed - StartElapsed, Phase(), *M->GetPathName(), M->IsRegistered(), M->IsVisible(), M->bHiddenInGame,
                    *M->GetComponentTransform().ToString(), *Origin->ToGenerationFrame(M->GetComponentLocation()).ToString(),
                    *M->Bounds.GetBox().ToString(), M->GetNumSections(), *GetPathNameSafe(Frame),
                    IsValid(Frame) && Frame->IsRegistered(), IsValid(Frame) && Frame->IsVisible(), IsValid(Frame) && Frame->bHiddenInGame,
                    IsValid(Frame) ? *Frame->GetComponentTransform().ToString() : TEXT("none"));
                for (int32 Index = 0; Index < M->GetNumSections(); ++Index)
                {
                    const auto* Section = M->GetProcMeshSection(Index);
                    auto* Material = M->GetMaterial(Index);
                    auto* MID = Cast<UMaterialInstanceDynamic>(Material);
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.PMC] t=%.3f phase=%s mesh=%s section=%d present=%d visible=%d vertices=%d indices=%d localBounds=%s material=%s parent=%s capturedParentMatch=%d"),
                        Elapsed - StartElapsed, Phase(), *M->GetPathName(), Index, Section != nullptr,
                        Section && Section->bSectionVisible, Section ? Section->ProcVertexBuffer.Num() : 0,
                        Section ? Section->ProcIndexBuffer.Num() : 0, Section ? *Section->SectionLocalBox.ToString() : TEXT("none"),
                        *GetPathNameSafe(Material), IsValid(MID) ? *GetPathNameSafe(MID->Parent.Get()) : TEXT("none"),
                        IsValid(MID) && IsValid(MID->Parent.Get()) && MID->Parent->GetPathName() == MaterialParent);
                }
            }
            if (Count == 0) UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe.PMC] t=%.3f phase=%s nativeClosedPMCCount=0"), Elapsed - StartElapsed, Phase());
        }
        static bool Finite(const FVector& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }
        static bool GeneratedBody(const APlanetaryBody* B)
        {
            if (!IsValid(B) || !B->bStreamWorldScapeSurface || B->WorldScapePresentationScale != 1.0) return false;
            for (const AActor* A = B; IsValid(A); A = A->GetAttachParentActor())
                if (A->ActorHasTag(TEXT("WorldGenerationPreview"))) return false;
            return true;
        }
        static uint32 ProfileSignature(const APlanetarySurfaceGenerator* G)
        {
            return UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(
                UAPSPlanetSurfaceProfileResolver::ResolveForBody(G->PlanetaryBody, G->SurfaceProfileCatalog));
        }
        static bool NativeReady(APlanetaryBody* B, APlanetarySurfaceGenerator* G, AWorldScapeRoot* R)
        {
            return IsValid(G) && IsValid(R) && G->PlanetaryBody == B && G->bOwnsWorldScapeRootInstance
                && B->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Active && B->bWorldScapeSurfaceReady
                && G->IsSurfaceProfileCurrent(B) && !G->IsSurfaceProfileApplyPending() && !R->IsHidden()
                && UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(G->ResolvedSurfaceProfile) == ProfileSignature(G)
                && R->bGenerateWorldScape && !R->bFreezeGeneration && R->IsActorTickEnabled()
                && R->TerrainMaterial.DefaultMaterial == G->ResolvedTerrainMaterialInstance;
        }
        bool OutsideAllBodies(UWorld* W, UAPSWorldOriginSubsystem* O, const FVector& Position) const
        {
            int32 Count = 0;
            for (TActorIterator<APlanetaryBody> It(W); It; ++It) if (GeneratedBody(*It))
            {
                const double Limit = It->GetWorldScapeUnloadRadiusCm();
                const FVector C = O->ToGenerationFrame(It->GetActorLocation());
                if (!Finite(C) || !FMath::IsFinite(Limit) || Limit <= 0.0 || FVector::Distance(Position, C) <= Limit) return false;
                ++Count;
            }
            return Count > 0 && Finite(Position);
        }
        bool ChooseFar(UWorld* W, UAPSWorldOriginSubsystem* O)
        {
            double Distance = 0.0;
            for (TActorIterator<APlanetaryBody> It(W); It; ++It) if (GeneratedBody(*It))
            {
                const double Limit = It->GetWorldScapeUnloadRadiusCm();
                const FVector C = O->ToGenerationFrame(It->GetActorLocation());
                if (!Finite(C) || !FMath::IsFinite(Limit) || Limit <= 0.0) return Fail(TEXT("invalid generated-body unload sphere"));
                Distance = FMath::Max(Distance, FVector::Distance(Center, C) + Limit);
            }
            FarGeneration = Center + (AnchorA() - Center).GetSafeNormal() * (Distance * 1.1 + 100000000.0);
            if (!OutsideAllBodies(W, O, FarGeneration)) return Fail(TEXT("could not find finite observer outside all unload spheres"));
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedNativeGlobe] FAR generation=%s distanceKm=%.9f same-lens tiny-angular-body-expected"),
                *FarGeneration.ToString(), FVector::Distance(FarGeneration, Center) / 100000.0);
            return true;
        }
        bool ClosedGlobeReady(APlanetaryBody* B)
        {
            if (B->IsHidden()) return false;
            TInlineComponentArray<UStaticMeshComponent*> Authored(B);
            for (auto* M : Authored) if (IsValid(M) && (M->IsVisible() || !M->bHiddenInGame)) return false;
            int32 Visible = 0;
            TInlineComponentArray<UProceduralMeshComponent*> Meshes(B);
            for (auto* M : Meshes) if (IsValid(M) && M->ComponentHasTag(TEXT("APS.NativeClosedTerrain"))
                && M->IsRegistered() && M->IsVisible() && !M->bHiddenInGame)
            {
                const auto* Section = M->GetProcMeshSection(0);
                auto* MID = Cast<UMaterialInstanceDynamic>(M->GetMaterial(0));
                if (!Section || !Section->bSectionVisible || Section->ProcVertexBuffer.IsEmpty() || Section->ProcIndexBuffer.IsEmpty()) return false;
                if (!IsValid(MID) || !IsValid(MID->Parent.Get()) || MID->Parent->GetPathName() != MaterialParent)
                    return Fail(TEXT("visible closed globe does not use the captured native material parent"));
                ++Visible;
            }
            if (Visible > 1) return Fail(TEXT("multiple visible closed terrain globes"));
            return Visible == 1;
        }
        bool Place(ACustomGravityCharacter* Character, UAPSWorldOriginSubsystem* Origin, const FVector& Generation)
        {
            const FVector WorldPosition = Origin->FromGenerationFrame(Generation);
            const FRotator Look = (Center - Generation).Rotation();
            if (!Finite(WorldPosition) || Look.ContainsNaN()) return Fail(TEXT("non-finite controlled observer pose"));
            Character->SetActorLocation(WorldPosition, false, nullptr, ETeleportType::TeleportPhysics);
            Camera->SetActorLocationAndRotation(WorldPosition, Look, false, nullptr, ETeleportType::TeleportPhysics);
            if (!Character->GetActorLocation().Equals(WorldPosition, 1.0) || !Camera->GetActorLocation().Equals(WorldPosition, 1.0))
                return Fail(TEXT("observer/camera placement readback mismatch"));
            return true;
        }
        void RestoreError(const TCHAR* Reason)
        {
            if (Error.IsEmpty()) Error = Reason;
            UE_LOG(LogTemp, Error, TEXT("[APS.SavedNativeGlobe] RESTORE %s"), Reason);
        }
        bool Fail(const TCHAR* Reason) { if (Error.IsEmpty()) Error = Reason; Release(); return false; }
        TWeakObjectPtr<ACustomGravityCharacter> Pawn;
        TWeakObjectPtr<APlanetaryBody> Body;
        TWeakObjectPtr<APlayerController> Player;
        TWeakObjectPtr<UWorld> SavedWorld;
        TWeakObjectPtr<UAPSWorldOriginSubsystem> OriginSystem;
        TWeakObjectPtr<AActor> Parent, OriginalTarget;
        TWeakObjectPtr<ACameraActor> Camera;
        TWeakObjectPtr<UGameViewportClient> DiagnosticViewport;
        FTransform OriginalTransform;
        FVector OriginalVelocity = FVector::ZeroVector, Center = FVector::ZeroVector, FarGeneration = FVector::ZeroVector;
        EMovementMode OriginalMode = MOVE_None;
        uint8 OriginalCustom = 0;
        int32 Stage = 0;
        int32 LastPMCAuditStage = -1;
        uint32 ProfileHash = 0, FirstRootID = 0;
        double RadiusCm = 0.0, RootRadius = 0.0, NoiseScale = 0.0, NoiseIntensity = 0.0;
        double StartElapsed = 0.0, LastElapsed = 0.0, StageStarted = 0.0, StableSince = -1.0, NextLog = 0.0;
        double NextPMCAudit = 0.0;
        bool bLeased = false, bReleased = false, bSnapshot = false;
        bool bReturnBeforeAudited = false, bReturnReadyAudited = false;
        bool bViewModeLease = false;
        FString BufferViewTarget;
        int32 OriginalViewMode = VMI_Lit;
        FEngineShowFlags OriginalShowFlags{ESFIM_Game};
        double NextViewModeAudit = 0.0;
        FString MaterialParent, Error;
    };
}

#endif
