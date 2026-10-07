#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "MaterialShared.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderingThread.h"
#include "WorldScapeRoot.h"

// Opt-in actual-save diagnostic. Caller owns save/hash/command-line guards.
// Controlled kinematic CAMERA/PAWN trajectory, NOT native flight or exact user view.
// A/B are generation-frame lens anchors; the real saved pawn stays10m behind the
// lens and drives ordinary streaming. No root/profile/material/origin/save writes.
namespace APSSavedLidimApproach
{
    inline FVector AnchorA() { return FVector(-9292423239211.45, -61775603144007.87, -1898397529769.63); }
    inline FVector AnchorB() { return FVector(-9292427771778.64, -61775602211342.40, -1898399424007.86); }
    inline constexpr double DurationSeconds = 50.67;

    class FRoute
    {
    public:
        FRoute() = default;
        FRoute(const FRoute&) = delete;
        FRoute& operator=(const FRoute&) = delete;
        ~FRoute() { Release(); }
        bool Started() const { return bLeased; }
        bool Ready() const { return bReady; }
        bool IsComplete() const { return bComplete; }
        const FString& GetError() const { return Error; }
        const TCHAR* Phase() const { return Error.IsEmpty() ? CurrentPhase : TEXT("failed"); }

        // Every-frame caller-owned tick. true=valid waiting/progress/complete;
        // false=latched failure+cleanup. Warmup captures must be labeled warmup.
        // Completion retains the final pose until caller Release AFTER capture.
        bool Tick(ACustomGravityCharacter* Character, APlanetaryBody* Lidim,
            AWorldScapeRoot* Root, double Elapsed)
        {
            if (!Error.IsEmpty()) return false;
            if (!IsInGameThread()) return Fail(TEXT("Lidim approach requires game thread"));
            if (bReleased) return Fail(TEXT("Lidim approach already released"));
            if (!FMath::IsFinite(Elapsed) || Elapsed < 0.0 || !IsValid(Character) || !IsValid(Lidim) || !IsValid(Root))
                return Fail(TEXT("invalid time or missing saved pawn/Lidim/standby root"));
            UWorld* World = Character->GetWorld();
            APlayerController* PC = Cast<APlayerController>(Character->GetController());
            UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
            UAPSWorldOriginSubsystem* Origin = IsValid(World) ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
            APlanetarySurfaceGenerator* Surface = Lidim->PlanetaryEnvironmentGenerator;
            if (!IsValid(World) || !IsValid(PC) || !IsValid(PC->PlayerCameraManager) || !IsValid(Movement) || !IsValid(Origin)
                || PC->GetPawn() != Character || PC->GetWorld() != World || Lidim->GetWorld() != World || Root->GetWorld() != World
                || !IsValid(Surface) || Surface->WorldScapeRootInstance != Root)
                return Fail(TEXT("saved world/possession/body-surface-root association invalid"));
            if (Character->GetActorTransform().ContainsNaN() || Lidim->GetActorTransform().ContainsNaN()
                || Root->GetActorTransform().ContainsNaN() || !Finite(Character->GetVelocity())
                || !Finite(Origin->GetOriginOffset()) || !FMath::IsFinite(Root->PlanetScaleCode)
                || !FMath::IsFinite(Lidim->RadiusKM) || !FMath::IsFinite(Lidim->WorldScapePresentationScale)
                || Lidim->AstroName != FName(TEXT("Lidim")) || Lidim->PlanetType != EPlanetType::Frozen
                || Lidim->WorldScapeSeed != 257455 || Root->Seed != 257455 || Root->bFlatWorld || Root->bOcean
                || FMath::Abs(Lidim->RadiusKM - 1280.896) > 0.01
                || FMath::Abs(Root->PlanetScaleCode / 100000.0 - Lidim->RadiusKM) > 0.001
                || FMath::Abs(Lidim->WorldScapePresentationScale - 1.0) > 1.e-9
                || !Root->GetActorScale3D().Equals(FVector::OneVector, 1.e-6)
                || FVector::Distance(Lidim->GetActorLocation(), Root->GetActorLocation()) > 100.0)
                return Fail(TEXT("actual saved Lidim type/seed/radius/concentric unit-scale frame mismatch"));
            const FVector CenterGeneration = Origin->ToGenerationFrame(Root->GetActorLocation());
            if (!bLeased)
            {
                if (Character->IsSurfaceHandoffSuspended() || Movement->MovementMode == MOVE_None)
                    return Fail(TEXT("cannot lease already suspended/disabled saved character"));
                const double HeightA = (FVector::Distance(AnchorA(), CenterGeneration) - Root->PlanetScaleCode) / 100000.0;
                const double HeightB = (FVector::Distance(AnchorB(), CenterGeneration) - Root->PlanetScaleCode) / 100000.0;
                if (!Finite(CenterGeneration) || HeightA < 20.0 || HeightA > 32.0 || HeightB < 2.0 || HeightB > 6.0)
                    return Fail(TEXT("fixed anchors mismatch actual Lidim: A must be20..32km, B2..6km"));
                const FMinimalViewInfo Natural = PC->PlayerCameraManager->GetCameraCacheView();
                if (!FMath::IsFinite(Natural.FOV) || Natural.FOV <= 0.f || Natural.FOV >= 180.f
                    || !FMath::IsFinite(Natural.AspectRatio) || Natural.AspectRatio <= 0.f
                    || !FMath::IsFinite(Natural.PostProcessBlendWeight) || Natural.ProjectionMode != ECameraProjectionMode::Perspective
                    || !Natural.OffCenterProjectionOffset.IsNearlyZero())
                    return Fail(TEXT("unsupported or invalid natural camera optics"));
                Pawn = Character; Body = Lidim; Terrain = Root; Controller = PC; SavedWorld = World;
                OriginSystem = Origin; OriginalTarget = PC->GetViewTarget(); Parent = Character->GetAttachParentActor();
                OriginalTransform = Character->GetActorTransform();
                OriginalTransform.SetLocation(Origin->ToGenerationFrame(OriginalTransform.GetLocation()));
                OriginalMode = Movement->MovementMode; OriginalCustomMode = Movement->CustomMovementMode;
                Center = CenterGeneration; RootRotation = Root->GetActorQuat(); RadiusCm = Root->PlanetScaleCode;
                StartElapsed = LastElapsed = Elapsed;
                FActorSpawnParameters Params;
                Params.ObjectFlags |= RF_Transient;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                ACameraActor* NewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Natural.Location, Natural.Rotation, Params);
                if (!IsValid(NewCamera) || !IsValid(NewCamera->GetCameraComponent()))
                {
                    if (IsValid(NewCamera)) NewCamera->Destroy();
                    return Fail(TEXT("could not create transient diagnostic camera"));
                }
                Camera = NewCamera;
                UCameraComponent* Lens = NewCamera->GetCameraComponent();
                Lens->SetFieldOfView(Natural.FOV); Lens->SetAspectRatio(Natural.AspectRatio);
                Lens->SetConstraintAspectRatio(Natural.bConstrainAspectRatio); Lens->SetProjectionMode(Natural.ProjectionMode);
                Lens->SetUseFieldOfViewForLOD(Natural.bUseFieldOfViewForLOD);
                Lens->bOverrideAspectRatioAxisConstraint = Natural.AspectRatioAxisConstraint.IsSet();
                if (Natural.AspectRatioAxisConstraint.IsSet()) Lens->SetAspectRatioAxisConstraint(Natural.AspectRatioAxisConstraint.GetValue());
                Lens->PostProcessSettings = Natural.PostProcessSettings; Lens->PostProcessBlendWeight = Natural.PostProcessBlendWeight;
                bLeased = true;
                Movement->DisableMovement(); // Native mode change, no direct velocity assignment.
                PC->SetViewTarget(NewCamera);
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedLidimApproach] BEGIN scope=controlled-kinematic-not-native-flight-not-exact-user-camera initial-placement-explicit root=%s centerGeneration=%s rootQuat=%s radiusKm=%.9f anchorA=%s heightAkm=%.9f anchorB=%s heightBkm=%.9f fov=%.6f aspect=%.6f PPWeight=%.6f movementModeBefore=%d original-velocity-not-restored"),
                    *Root->GetPathName(), *Center.ToString(), *RootRotation.ToString(), RadiusCm / 100000.0,
                    *AnchorA().ToString(), HeightA, *AnchorB().ToString(), HeightB, Natural.FOV, Natural.AspectRatio,
                    Natural.PostProcessBlendWeight, static_cast<int32>(OriginalMode));
            }
            if (Pawn.Get() != Character || Body.Get() != Lidim || Terrain.Get() != Root || Controller.Get() != PC
                || SavedWorld.Get() != World || OriginSystem.Get() != Origin || Parent.Get() != Character->GetAttachParentActor()
                || !Camera.IsValid() || PC->GetViewTarget() != Camera.Get() || Root->PlanetScaleCode != RadiusCm
                || FVector::Distance(CenterGeneration, Center) > 100.0 || RootRotation.AngularDistance(Root->GetActorQuat()) > 1.e-6)
                return Fail(TEXT("leased pawn/body/root/controller/view target or generation frame changed"));
            if (Elapsed < LastElapsed || Elapsed - StartElapsed > 90.0) return Fail(TEXT("non-monotonic time or90s approach timeout"));
            LastElapsed = Elapsed;
            if (Movement->MovementMode != MOVE_None) Movement->DisableMovement();
            FVector LensGeneration = AnchorA();
            if (!bReady)
            {
                CurrentPhase = TEXT("warmup-A");
                if (!Place(Character, Origin, LensGeneration)) return false;
                if (PublishedReady(Lidim, Surface, Root) && !Character->IsSurfaceHandoffSuspended())
                {
                    if (StableSince < 0.0) StableSince = Elapsed;
                    ++StableTicks;
                    if (Elapsed - StableSince >= 3.0 && StableTicks >= 12)
                    {
                        const FVector UpB = (AnchorB() - Center).GetSafeNormal();
                        const double GroundCm = Root->GetGroundHeight(Root->GetActorLocation() + UpB * RadiusCm, false);
                        if (!FMath::IsFinite(GroundCm)) return Fail(TEXT("native B ground sample is non-finite"));
                        GroundAnchor = Center + UpB * (RadiusCm + GroundCm + 3400.0);
                        if (!Finite(GroundAnchor) || FVector::DotProduct(AnchorB() - GroundAnchor, UpB) <= 0.0)
                            return Fail(TEXT("native ground+34m is not below anchor B"));
                        bReady = true; RouteStart = Elapsed;
                        // Read the actual published terrain resource once, not just
                        // the MID's name or a commandlet's different shader cache.
                        if (UMaterialInterface* M = Root->TerrainMaterial.DefaultMaterial)
                        {
                            const auto Feature = Root->GetWorld()->GetFeatureLevel();
                            FMaterialResource* R = M->GetMaterialResource(Feature);
                            UE_LOG(LogTemp, Display, TEXT("[APS.SavedLidimApproach.Shader] GT material=%s feature=%d quality=%d complete=%d errors=%d finished=%d"),
                                *M->GetPathName(), int32(Feature), R ? int32(R->GetQualityLevel()) : -1,
                                R && R->IsGameThreadShaderMapComplete(), R ? R->GetCompileErrors().Num() : -1, R && R->IsCompilationFinished());
                            const FMaterialRenderProxy* Proxy = M->GetRenderProxy();
                            ENQUEUE_RENDER_COMMAND(APSSavedLidimActualShader)([Proxy,Feature](FRHICommandListImmediate&)
                            {
                                const FMaterial* Actual = Proxy->GetMaterialNoFallback(Feature);
                                UE_LOG(LogTemp, Display, TEXT("[APS.SavedLidimApproach.Shader] RT actual=%s quality=%d complete=%d"),
                                    Actual ? *Actual->GetFriendlyName() : TEXT("null"), Actual ? int32(Actual->GetQualityLevel()) : -1,
                                    Actual && Actual->IsRenderingThreadShaderMapComplete());
                            });
                        }
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedLidimApproach] READY t=%.3f rootCenterGeneration=%s groundNoiseCm=%.9f groundLensGeneration=%s lensClearanceM=34 pawnBehindLensM=10 durationSeconds=50.67 no-forced-root-update"),
                            Elapsed - StartElapsed, *CenterGeneration.ToString(), GroundCm, *GroundAnchor.ToString());
                    }
                }
                else { StableSince = -1.0; StableTicks = 0; }
                if (!bReady && Elapsed - StartElapsed > 30.0) return Fail(TEXT("30s initial A production-streaming readiness timeout"));
            }
            else if (Character->IsSurfaceHandoffSuspended() || !LiveGeneration(Lidim, Root) || !Surface->bSurfaceProfileApplied
                || !Surface->IsSurfaceProfileCurrent(Lidim) || Surface->IsSurfaceProfileApplyPending())
                return Fail(TEXT("route lost active native generation/profile or acquired a surface handoff suspension"));
            if (bReady)
            {
                const double T = FMath::Clamp(Elapsed - RouteStart, 0.0, DurationSeconds);
                if (T < 3.0) { LensGeneration = AnchorA(); CurrentPhase = TEXT("A-hold"); }
                else if (T < 13.835) { LensGeneration = FMath::Lerp(AnchorA(), AnchorB(), (T - 3.0) / 10.835); CurrentPhase = TEXT("A-to-B-chord"); }
                else if (T < 16.835) { LensGeneration = AnchorB(); CurrentPhase = TEXT("B-hold-down"); }
                else if (T < 22.835) { LensGeneration = FMath::Lerp(AnchorB(), GroundAnchor, (T - 16.835) / 6.0); CurrentPhase = TEXT("B-to-ground34m"); }
                else if (T < 27.835) { LensGeneration = GroundAnchor; CurrentPhase = TEXT("ground34m-hold"); }
                else if (T < 33.835) { LensGeneration = FMath::Lerp(GroundAnchor, AnchorB(), (T - 27.835) / 6.0); CurrentPhase = TEXT("ground34m-to-B"); }
                else if (T < 36.835) { LensGeneration = AnchorB(); CurrentPhase = TEXT("B-hold-up"); }
                else if (T < 47.67) { LensGeneration = FMath::Lerp(AnchorB(), AnchorA(), (T - 36.835) / 10.835); CurrentPhase = TEXT("B-to-A-chord"); }
                else { LensGeneration = AnchorA(); CurrentPhase = TEXT("A-final-hold"); }
                if (!Place(Character, Origin, LensGeneration)) return false;
                bComplete = T >= DurationSeconds;
                if (bComplete) CurrentPhase = TEXT("complete");
            }
            if (PreviousPhase != CurrentPhase || Elapsed >= NextHeartbeat)
            {
                PreviousPhase = CurrentPhase; NextHeartbeat = Elapsed + 1.0;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedLidimApproach] t=%.3f routeT=%.3f phase=%s ready=%d cameraGeneration=%s pawnGeneration=%s rootGeneration=%s originOffset=%s cameraHeightKm=%.9f incomplete=%d bodyReady=%d rootHidden=%d"),
                    Elapsed - StartElapsed, bReady ? Elapsed - RouteStart : -1.0, Phase(), bReady,
                    *Origin->ToGenerationFrame(Camera->GetActorLocation()).ToString(),
                    *Origin->ToGenerationFrame(Character->GetActorLocation()).ToString(), *CenterGeneration.ToString(),
                    *Origin->GetOriginOffset().ToString(), (FVector::Distance(LensGeneration, Center) - RadiusCm) / 100000.0,
                    Root->WorldScapeLodInGeneration.Num(), Lidim->bWorldScapeSurfaceReady, Root->IsHidden());
            }
            return true;
        }

        // Restores only the leased pawn/world/controller and owned camera target.
        void Release()
        {
            if (!IsInGameThread() || bReleased) return;
            bReleased = true;
            ACustomGravityCharacter* Character = Pawn.Get();
            APlayerController* PC = Controller.Get();
            UAPSWorldOriginSubsystem* Origin = OriginSystem.Get();
            ACameraActor* OwnedCamera = Camera.Get();
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
                if (!Character->GetActorLocation().Equals(Restore.GetLocation(), 1.0))
                {
                    if (Error.IsEmpty()) Error = TEXT("could not restore original generation-frame pawn position");
                    UE_LOG(LogTemp, Error, TEXT("[APS.SavedLidimApproach] RESTORE position verification failed"));
                }
                if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
                    if (Movement->MovementMode == MOVE_None) Movement->SetMovementMode(OriginalMode, OriginalCustomMode);
            }
            else if (bLeased)
            {
                if (Error.IsEmpty()) Error = TEXT("lost pawn ownership: original transform/mode cannot be safely restored");
                UE_LOG(LogTemp, Error, TEXT("[APS.SavedLidimApproach] RESTORE skipped: original pawn/world/possession lost"));
            }
            if (IsValid(OwnedCamera)) OwnedCamera->Destroy();
            Camera.Reset();
        }

    private:
        static bool Finite(const FVector& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }
        static bool LiveGeneration(const APlanetaryBody* Lidim, const AWorldScapeRoot* Root)
        {
            return Lidim->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Active
                && Root->bGenerateWorldScape && !Root->bFreezeGeneration && Root->IsActorTickEnabled();
        }
        static bool PublishedReady(APlanetaryBody* Lidim, APlanetarySurfaceGenerator* Surface, AWorldScapeRoot* Root)
        {
            if (!LiveGeneration(Lidim, Root) || !Lidim->bWorldScapeSurfaceReady || !Surface->bSurfaceProfileApplied || !Surface->IsSurfaceProfileCurrent(Lidim)
                || Surface->IsSurfaceProfileApplyPending() || Root->IsHidden() || Root->WorldScapeLod.Num() < 3
                || Root->WorldScapeLodInGeneration.Num() != 0) return false;
            for (int32 I = 0; I < 3; ++I)
            {
                const UWorldScapeLod* Lod = Root->WorldScapeLod[I];
                UWorldScapeMeshComponent* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                if (!IsValid(Mesh) || !Mesh->IsRegistered() || !Mesh->IsVisible() || Mesh->bHiddenInGame || Mesh->GetNumSections() < 3) return false;
                for (int32 S = 0; S < 3; ++S)
                {
                    const FWorldScapeMeshSection* Section = Mesh->GetProcMeshSection(S);
                    if (!Section || !Section->bSectionVisible || Section->PlanetVertexBuffer.IsEmpty()
                        || Section->PlanetIndexBuffer.IsEmpty() || !IsValid(Mesh->GetMaterial(S))) return false;
                }
            }
            return true;
        }
        bool Place(ACustomGravityCharacter* Character, UAPSWorldOriginSubsystem* Origin, const FVector& LensGeneration)
        {
            const FVector Up = (LensGeneration - Center).GetSafeNormal();
            const FVector Tangent = FVector::VectorPlaneProject(AnchorB() - AnchorA(), Up).GetSafeNormal();
            if (!Finite(LensGeneration) || Up.IsNearlyZero() || Tangent.IsNearlyZero()) return Fail(TEXT("degenerate route camera frame"));
            const double Angle = FMath::DegreesToRadians(25.0);
            const FVector Forward = (Tangent * FMath::Cos(Angle) - Up * FMath::Sin(Angle)).GetSafeNormal();
            const FRotator Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
            const FVector LensWorld = Origin->FromGenerationFrame(LensGeneration);
            const FVector PawnWorld = Origin->FromGenerationFrame(LensGeneration - Forward * 1000.0);
            if (!Finite(LensWorld) || !Finite(PawnWorld) || Rotation.ContainsNaN()) return Fail(TEXT("invalid converted route pose"));
            // Explicit nonphysical observer placement, no sweep or direct velocity
            // assignment/worker/root tick. Ground sampling is NOT collision proof.
            Character->SetActorLocation(PawnWorld, false, nullptr, ETeleportType::TeleportPhysics);
            Camera->SetActorLocationAndRotation(LensWorld, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
            if (!Character->GetActorLocation().Equals(PawnWorld, 1.0) || !Camera->GetActorLocation().Equals(LensWorld, 1.0))
                return Fail(TEXT("kinematic camera/pawn placement readback mismatch"));
            return true;
        }
        bool Fail(const TCHAR* Reason) { if (Error.IsEmpty()) Error = Reason; Release(); return false; }

        TWeakObjectPtr<ACustomGravityCharacter> Pawn;
        TWeakObjectPtr<APlanetaryBody> Body;
        TWeakObjectPtr<AWorldScapeRoot> Terrain;
        TWeakObjectPtr<APlayerController> Controller;
        TWeakObjectPtr<UWorld> SavedWorld;
        TWeakObjectPtr<UAPSWorldOriginSubsystem> OriginSystem;
        TWeakObjectPtr<AActor> OriginalTarget, Parent;
        TWeakObjectPtr<ACameraActor> Camera;
        FTransform OriginalTransform;
        FQuat RootRotation = FQuat::Identity;
        FVector Center = FVector::ZeroVector, GroundAnchor = FVector::ZeroVector;
        EMovementMode OriginalMode = MOVE_None;
        uint8 OriginalCustomMode = 0;
        double RadiusCm = 0.0, StartElapsed = 0.0, LastElapsed = 0.0, RouteStart = 0.0;
        double StableSince = -1.0, NextHeartbeat = 0.0;
        int32 StableTicks = 0;
        bool bLeased = false, bReady = false, bComplete = false, bReleased = false;
        const TCHAR* CurrentPhase = TEXT("uninitialized");
        FString PreviousPhase, Error;
    };
}

#endif
