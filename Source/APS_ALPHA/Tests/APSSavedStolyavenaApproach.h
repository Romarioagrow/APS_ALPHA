#pragma once

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "WorldScapeRoot.h"

// Caller supplies immutable save/model/private-process guards. Ordinary VisitForTest
// and streaming create the real body. Only an owned observer is moved; no profile,
// material, lighting, root, origin, saved-pawn transform or save is written.
// This is NOT the incident camera, native ship physics, or visual acceptance.
// The observer radial is fixed in body coordinates. Its35-degree oblique view
// does NOT track one ground landmark across heights.
namespace APSSavedStolyavenaApproach
{
    inline constexpr double DurationSeconds = 80.0;
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/M_APS_ContinuousTerrain.M_APS_ContinuousTerrain");

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
        const TCHAR* Phase() const { return CurrentPhase; }
        double PhaseAgeSeconds() const { return bReady ? CurrentPhaseAge : 0.0; }
        APlanet* GetBody() const { return Body.Get(); }
        APawn* GetObserver() const { return Observer.Get(); }
        AWorldScapeRoot* GetRoot() const
        {
            APlanet* P = Body.Get();
            APlanetarySurfaceGenerator* S = P ? P->PlanetaryEnvironmentGenerator : nullptr;
            return IsValid(S) ? S->WorldScapeRootInstance : nullptr;
        }

        bool Tick(APlayerController* PC, double Elapsed)
        {
            if (!Error.IsEmpty()) return false;
            if (!IsInGameThread() || bReleased || !FMath::IsFinite(Elapsed) || Elapsed < LastElapsed
                || !IsValid(PC) || !IsValid(PC->PlayerCameraManager)) return Fail(TEXT("invalid observer controller/time/lifecycle"));
            LastElapsed = Elapsed;
            UWorld* World = PC->GetWorld();
            UAPSWorldOriginSubsystem* Origin = IsValid(World) ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
            if (!Origin || !Finite(Origin->GetOriginOffset())) return Fail(TEXT("generation frame unavailable"));
            if (!bLeased && !Lease(PC, Origin)) return false;
            if (Controller.Get() != PC || SavedWorld.Get() != World || OriginSystem.Get() != Origin
                || !Observer.IsValid() || PC->GetPawn() != Observer.Get() || PC->GetViewTarget() != Observer.Get()
                || !OriginalPawn.IsValid()) return Fail(TEXT("observer/original pawn/possession/view ownership lost"));

            FAPSStarSystems* Systems = APSStarSystemsFind(World);
            FAPSSystemMaterializer* Materializer = Systems ? Systems->GetMaterializer() : nullptr;
            if (!Systems || !Systems->IsReady() || !Materializer)
                return Elapsed > 60.0 ? Fail(TEXT("60s catalogue/materializer deadline")) : true;
            if (!bVisited)
            {
                int32 Count = 0;
                for (int32 I = 0; I < Systems->Num(); ++I)
                    if (const FAPSStarSystemInfo* Info = Systems->Get(I); Info && Info->Name.Equals(TEXT("VULEX"), ESearchCase::IgnoreCase))
                    { SystemIndex = I; SystemId = Info->Id; ++Count; }
                if (Count > 1) return Fail(TEXT("VULEX catalogue identity is ambiguous"));
                if (Count == 0)
                {
                    // The cluster is ready before the asynchronously indexed
                    // galaxy restores its saved systems. Let ordinary streaming
                    // restore them; do not synthesize/register another target.
                    CurrentPhase = TEXT("saved-galaxy-catalogue-wait");
                    if (Elapsed >= NextLog)
                    {
                        NextLog = Elapsed + 1.0;
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] CATALOGUE_WAIT t=%.3f systems=%d galaxySystems=%d exactVulexMatches=0 deadline=60"),
                            Elapsed, Systems->Num(), Systems->NumGalaxySystems());
                    }
                    return Elapsed > 60.0 ? Fail(TEXT("VULEX absent after60s ordinary saved-galaxy restoration")) : true;
                }
                const FAPSStarSystemInfo* Info = Systems->Get(SystemIndex);
                if (!Info || !SystemId.IsValid() || Info->bHome || Info->bInsideHome || !Finite(Info->Location)
                    || !FMath::IsFinite(Info->RoomCm) || Info->RoomCm <= 0.0)
                    return Fail(TEXT("VULEX is not a valid ordinary non-home materialization target"));
                // False is essential: the planet shortcut silently picks the first world.
                if (!Materializer->VisitForTest(*Systems, SystemIndex, false)) return Fail(TEXT("ordinary VULEX VisitForTest failed"));
                bVisited = true;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] VISIT system=VULEX id=%s index=%d planetShortcut=0 observer=%s"),
                    *SystemId.ToString(), SystemIndex, *Observer->GetPathName());
            }
            const FAPSStarSystemInfo* Info = Systems->Get(SystemIndex);
            if (!Info || Info->Id != SystemId || !Info->Name.Equals(TEXT("VULEX"), ESearchCase::IgnoreCase))
                return Fail(TEXT("catalogue target changed"));
            if (!Body.IsValid())
            {
                CurrentPhase = TEXT("ordinary-materialization");
                if (Materializer->GetActiveIndex() == SystemIndex)
                {
                    TArray<APlanet*> Planets;
                    Materializer->GetPlanets(Planets);
                    APlanet* Found = nullptr;
                    for (APlanet* P : Planets)
                        if (IsValid(P) && P->AstroName == FName(TEXT("Stolyavena")))
                        { if (Found) return Fail(TEXT("duplicate Stolyavena in VULEX")); Found = P; }
                    if (Found)
                    {
                        if (!BodyIdentity(Found) || !IsValid(Found->ParentStar) || Found->ParentStar->GetWorld() != World)
                            return Fail(TEXT("materialized Stolyavena body identity/parent star mismatch"));
                        Body = Found; ParentStar = Found->ParentStar; BodyArrived = Elapsed;
                        LocalSun = Found->GetActorQuat().UnrotateVector((ParentStar->GetActorLocation() - Found->GetActorLocation()).GetSafeNormal());
                        if (!Finite(LocalSun) || LocalSun.IsNearlyZero()) return Fail(TEXT("invalid actual parent-star direction"));
                        LocalSide = FVector::CrossProduct(LocalSun, FMath::Abs(LocalSun.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
                        LocalRay = (LocalSun + LocalSide).GetSafeNormal(); // Oblique daylight, not a forced light.
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] BODY body=%s star=%s seed=%d radiusKm=%.9f initialLocalRay=%s"),
                            *Found->GetPathName(), *ParentStar->GetPathName(), Found->WorldScapeSeed, Found->RadiusKM, *LocalRay.ToString());
                    }
                }
                if (!Body.IsValid()) return Elapsed > 60.0 ? Fail(TEXT("60s real Stolyavena materialization deadline")) : true;
            }
            APlanet* P = Body.Get();
            if (Materializer->GetActiveIndex() != SystemIndex || !BodyIdentity(P) || P->ParentStar != ParentStar.Get()
                || P->GetWorld() != World || P->GetActorTransform().ContainsNaN())
                return Fail(TEXT("actual body/system/frame identity changed"));
            AWorldScapeRoot* Root = GetRoot();
            APlanetarySurfaceGenerator* Surface = P->PlanetaryEnvironmentGenerator;
            if (!bReady)
            {
                CurrentPhase = TEXT("orbit-native-warmup");
                if (!Place(PC, Origin, 1000.0)) return false;
                // Initial profile application precedes the first root tick. Wait for
                // the body's existing readiness contract before strict numeric checks.
                if (IsValid(Root) && IsValid(Surface) && Surface->bSurfaceProfileApplied
                    && Surface->IsSurfaceProfileCurrent(P) && !Surface->IsSurfaceProfileApplyPending()
                    && P->bWorldScapeSurfaceReady)
                {
                    if (!ProfileIdentity(P, Surface, Root)) return false;
                    if (!bSiteSelected)
                    {
                        if (!SelectLand(Root)) return false;
                        bSiteSelected = true;
                        if (!Place(PC, Origin, 1000.0)) return false;
                    }
                    if (PublishedReady(P, Root))
                    {
                        if (StableSince < 0.0) StableSince = Elapsed;
                        if (Elapsed - StableSince >= 3.0)
                        {
                            Terrain = Root; OriginalMaterial = Root->TerrainMaterial.DefaultMaterial;
                            bReady = true; RouteStarted = Elapsed;
                            UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] READY t=%.3f root=%s material=%s groundCm=%.6f oceanCm=%.6f localRay=%s duration=80 clearanceKm=1000,20,2,0.034,2,20,1000 optics=unchanged"),
                                Elapsed, *Root->GetPathName(), *GetPathNameSafe(OriginalMaterial.Get()), GroundCm, Root->OceanHeight, *LocalRay.ToString());
                        }
                    }
                    else StableSince = -1.0;
                }
                else StableSince = -1.0;
                if (!bReady && Elapsed - BodyArrived > 30.0) return Fail(TEXT("30s native Stolyavena warmup deadline"));
            }
            if (bReady)
            {
                if (!IsValid(Root) || !IsValid(Surface) || !Surface->bSurfaceProfileApplied
                    || !Surface->IsSurfaceProfileCurrent(P) || Surface->IsSurfaceProfileApplyPending()
                    || P->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
                    || !Root->bGenerateWorldScape || Root->bFreezeGeneration || !Root->IsActorTickEnabled()
                    || !ProfileIdentity(P, Surface, Root))
                    return Fail(TEXT("route lost the native canonical material/profile"));
                if (Root != Terrain.Get() || Root->TerrainMaterial.DefaultMaterial != OriginalMaterial.Get())
                {
                    UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] NATIVE_IDENTITY_TRANSITION oldRoot=%s newRoot=%s oldMID=%s newMID=%s canonicalParentVerified=1 no-forced-replacement"),
                        *GetPathNameSafe(Terrain.Get()), *Root->GetPathName(), *GetPathNameSafe(OriginalMaterial.Get()), *GetPathNameSafe(Root->TerrainMaterial.DefaultMaterial));
                    Terrain = Root; OriginalMaterial = Root->TerrainMaterial.DefaultMaterial;
                }
                const double T = FMath::Clamp(Elapsed - RouteStarted, 0.0, DurationSeconds);
                static const double Times[] = {0,3,15,18,26,29,37,43,51,54,62,65,77,80};
                static const double Heights[] = {1000,1000,20,20,2,2,0.034,0.034,2,2,20,20,1000,1000};
                static const TCHAR* Phases[] = {TEXT("orbit-hold"),TEXT("orbit-to20km"),TEXT("20km-down-hold"),TEXT("20km-to2km"),
                    TEXT("2km-down-hold"),TEXT("2km-to34m"),TEXT("ground34m-hold"),TEXT("34m-to2km"),TEXT("2km-up-hold"),
                    TEXT("2km-to20km"),TEXT("20km-up-hold"),TEXT("20km-to-orbit"),TEXT("orbit-return-hold")};
                int32 Leg = 0;
                while (Leg < 12 && T >= Times[Leg + 1]) ++Leg;
                const double U = FMath::Clamp((T - Times[Leg]) / (Times[Leg + 1] - Times[Leg]), 0.0, 1.0);
                const double S = U * U * (3.0 - 2.0 * U);
                ClearanceKm = FMath::Exp(FMath::Lerp(FMath::Loge(Heights[Leg]), FMath::Loge(Heights[Leg + 1]), S));
                CurrentPhase = Phases[Leg];
                CurrentPhaseAge = T - Times[Leg];
                if (!Place(PC, Origin, ClearanceKm)) return false;
                if (T >= DurationSeconds && !bComplete)
                {
                    bComplete = true; CurrentPhase = TEXT("complete");
                    // Caller verifies every mandatory hold's successful PNG and
                    // cleanup before emitting the final COMPLETE marker.
                }
            }
            if (Elapsed >= NextLog || PreviousPhase != CurrentPhase)
            {
                NextLog = Elapsed + 1.0; PreviousPhase = CurrentPhase;
                UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] POSE t=%.3f routeT=%.3f phase=%s ready=%d bodyGeneration=%s observerGeneration=%s localRay=%s clearanceKm=%.9f groundCm=%.6f originOffset=%s root=%s rootHidden=%d bodyReady=%d"),
                    Elapsed, bReady ? Elapsed - RouteStarted : -1.0, CurrentPhase, bReady,
                    *Origin->ToGenerationFrame(P->GetActorLocation()).ToString(), *Origin->ToGenerationFrame(Observer->GetActorLocation()).ToString(),
                    *LocalRay.ToString(), ClearanceKm, GroundCm, *Origin->GetOriginOffset().ToString(), *GetPathNameSafe(Root),
                    Root ? Root->IsHidden() : -1, P->bWorldScapeSurfaceReady);
            }
            return true;
        }

        void Release()
        {
            if (!IsInGameThread() || bReleased) return;
            bReleased = true;
            APlayerController* PC = Controller.Get();
            APawn* Owned = Observer.Get();
            if (bLeased && IsValid(PC) && PC->GetWorld() == SavedWorld.Get() && PC->GetPawn() == Owned && OriginalPawn.IsValid())
            {
                // Normal possession callbacks only. Never board, move, reconfigure or
                // restore a guessed velocity to the original character/ship.
                PC->Possess(OriginalPawn.Get());
                PC->SetControlRotation(OriginalControl);
                if (OriginalTarget.IsValid() && OriginalTarget->GetWorld() == SavedWorld.Get()) PC->SetViewTarget(OriginalTarget.Get());
                if ((PC->GetPawn() != OriginalPawn.Get() || !OriginalTarget.IsValid()
                    || PC->GetViewTarget() != OriginalTarget.Get() || !PC->GetControlRotation().Equals(OriginalControl, 1.e-3)) && Error.IsEmpty())
                    Error = TEXT("original possession/view target/control rotation restoration readback failed");
            }
            else if (bLeased && Error.IsEmpty()) Error = TEXT("ownership lost; original possession cannot be safely restored");
            if (IsValid(Owned))
            {
                if (IsValid(PC) && PC->GetPawn() == Owned) PC->UnPossess();
                Owned->Destroy();
            }
            Observer.Reset();
            if (!Error.IsEmpty()) UE_LOG(LogTemp, Error, TEXT("[APS.SavedStolyavena] RELEASE %s"), *Error);
        }

    private:
        static bool Finite(const FVector& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }
        static bool BodyIdentity(const APlanet* P)
        {
            return IsValid(P) && P->AstroName == FName(TEXT("Stolyavena")) && P->PlanetType == EPlanetType::Terrestrial
                && P->WorldScapeSeed == 738810 && FMath::IsFinite(P->RadiusKM) && FMath::Abs(P->RadiusKM - 5970.156) <= 0.01
                && FMath::IsFinite(P->WorldScapePresentationScale) && FMath::Abs(P->WorldScapePresentationScale - 1.0) <= 1.e-9;
        }
        bool Lease(APlayerController* PC, UAPSWorldOriginSubsystem* Origin)
        {
            const FMinimalViewInfo Natural = PC->PlayerCameraManager->GetCameraCacheView();
            if (!IsValid(PC->GetPawn()) || !IsValid(PC->GetViewTarget()) || PC->GetControlRotation().ContainsNaN()
                || !PC->IsLocalController() || !FMath::IsFinite(Natural.FOV) || Natural.FOV <= 0.f || Natural.FOV >= 180.f
                || !FMath::IsFinite(Natural.AspectRatio) || Natural.AspectRatio <= 0.f || !FMath::IsFinite(Natural.PostProcessBlendWeight)
                || Natural.ProjectionMode != ECameraProjectionMode::Perspective || !Natural.OffCenterProjectionOffset.IsNearlyZero()
                || !Finite(Natural.Location) || Natural.Rotation.ContainsNaN()) return Fail(TEXT("unsupported saved pawn/local camera optics"));
            Controller = PC; SavedWorld = PC->GetWorld(); OriginSystem = Origin;
            OriginalPawn = PC->GetPawn(); OriginalTarget = PC->GetViewTarget(); OriginalControl = PC->GetControlRotation();
            FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            APawn* Owned = SavedWorld->SpawnActor<APawn>(APawn::StaticClass(), Natural.Location, Natural.Rotation, Params);
            if (!Owned) return Fail(TEXT("transient observer spawn failed"));
            Observer = Owned;
            UCameraComponent* Lens = NewObject<UCameraComponent>(Owned, NAME_None, RF_Transient);
            if (!Lens) { Owned->Destroy(); Observer.Reset(); return Fail(TEXT("transient observer lens failed")); }
            Owned->SetRootComponent(Lens); Owned->AddInstanceComponent(Lens); Lens->RegisterComponent();
            Lens->SetFieldOfView(Natural.FOV); Lens->SetAspectRatio(Natural.AspectRatio);
            Lens->SetConstraintAspectRatio(Natural.bConstrainAspectRatio); Lens->SetProjectionMode(Natural.ProjectionMode);
            Lens->SetUseFieldOfViewForLOD(Natural.bUseFieldOfViewForLOD);
            Lens->bOverrideAspectRatioAxisConstraint = Natural.AspectRatioAxisConstraint.IsSet();
            if (Natural.AspectRatioAxisConstraint.IsSet()) Lens->SetAspectRatioAxisConstraint(Natural.AspectRatioAxisConstraint.GetValue());
            Lens->PostProcessSettings = Natural.PostProcessSettings; Lens->PostProcessBlendWeight = Natural.PostProcessBlendWeight;
            Owned->SetActorEnableCollision(false); Owned->SetActorTickEnabled(false);
            Owned->SetActorLocationAndRotation(Natural.Location, Natural.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
            bLeased = true; PC->Possess(Owned); PC->SetViewTarget(Owned);
            if (PC->GetPawn() != Owned || PC->GetViewTarget() != Owned) return Fail(TEXT("transient observer possession failed"));
            UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] LEASE originalPawn=%s originalTarget=%s observer=%s fov=%.6f aspect=%.6f originalPawnNotRelocated=1 ordinary-possession-callbacks=1 controlled-route-not-native-flight=1"),
                *OriginalPawn->GetPathName(), *GetPathNameSafe(OriginalTarget.Get()), *Owned->GetPathName(), Natural.FOV, Natural.AspectRatio);
            return true;
        }
        bool ProfileIdentity(APlanet* P, APlanetarySurfaceGenerator* S, AWorldScapeRoot* R)
        {
            const auto& Profile = S->ResolvedSurfaceProfile;
            UMaterialInterface* Material = R->TerrainMaterial.DefaultMaterial;
            const UMaterialInstance* MI = Cast<UMaterialInstance>(Material);
            if (Profile.PlanetType != EPlanetType::Terrestrial || Profile.TerrainSeed != 738810 || R->Seed != 738810
                || !FMath::IsFinite(Profile.NoiseScale) || FMath::Abs(Profile.NoiseScale - 655.0f) > 0.51f
                || !FMath::IsFinite(Profile.NoiseIntensity) || FMath::Abs(Profile.NoiseIntensity - 860637.0f) > 0.51f
                || !FMath::IsFinite(R->NoiseScale) || !FMath::IsFinite(R->NoiseIntensity)
                || FMath::Abs(R->NoiseScale - Profile.NoiseScale) > 0.51f
                || FMath::Abs(R->NoiseIntensity - Profile.NoiseIntensity) > 0.51f || R->bFlatWorld
                || !FMath::IsFinite(R->PlanetScaleCode) || FMath::Abs(R->PlanetScaleCode / 100000.0 - 5970.156) > 0.01
                || !R->GetActorScale3D().Equals(FVector::OneVector, 1.e-6) || R->GetActorTransform().ContainsNaN()
                || FVector::Distance(P->GetActorLocation(), R->GetActorLocation()) > 100.0
                || !MI || !MI->Parent || MI->Parent->GetPathName() != TemplatePath || !MI->GetMaterial()
                || MI->GetMaterial()->GetPathName() != MasterPath || Material != S->ResolvedTerrainMaterialInstance)
                return Fail(*FString::Printf(TEXT("actual Stolyavena profile/root/material mismatch: type=%d terrainSeed=%d rootSeed=%d noise=%.6f/%.6f rootNoise=%.6f/%.6f radiusKm=%.9f material=%s parent=%s"),
                    int32(Profile.PlanetType), Profile.TerrainSeed, R->Seed, Profile.NoiseScale, Profile.NoiseIntensity,
                    R->NoiseScale, R->NoiseIntensity, R->PlanetScaleCode / 100000.0, *GetPathNameSafe(Material), *GetPathNameSafe(MI ? MI->Parent.Get() : nullptr)));
            for (UWorldScapeLod* Lod : R->WorldScapeLod)
                if (IsValid(Lod) && IsValid(Lod->Mesh))
                    for (int32 I = 0; I < Lod->Mesh->GetNumSections(); ++I)
                        if (Lod->Mesh->GetMaterial(I) != Material) return Fail(TEXT("native terrain LOD material identity differs"));
            return true;
        }
        static bool PublishedReady(APlanet* P, AWorldScapeRoot* R)
        {
            if (P->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active || !R->bGenerateWorldScape || R->bFreezeGeneration
                || !R->IsActorTickEnabled() || R->IsHidden() || R->WorldScapeLod.Num() < 3 || R->WorldScapeLodInGeneration.Num() != 0) return false;
            for (int32 L = 0; L < 3; ++L)
            {
                UWorldScapeLod* Lod = R->WorldScapeLod[L];
                UWorldScapeMeshComponent* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                if (!IsValid(Mesh) || !Mesh->IsRegistered() || !Mesh->IsVisible() || Mesh->bHiddenInGame || Mesh->GetNumSections() < 3) return false;
                for (int32 I = 0; I < 3; ++I)
                {
                    const FWorldScapeMeshSection* Section = Mesh->GetProcMeshSection(I);
                    if (!Section || !Section->bSectionVisible || Section->PlanetVertexBuffer.IsEmpty() || Section->PlanetIndexBuffer.IsEmpty()) return false;
                }
            }
            return true;
        }
        bool SelectLand(AWorldScapeRoot* R)
        {
            const FVector Other = FVector::CrossProduct(LocalSun, LocalSide).GetSafeNormal();
            const double Angles[] = {45.0,30.0,60.0,15.0,0.0};
            int32 SampleCount = 0;
            for (double Angle : Angles)
                for (int32 Azimuth = 0; Azimuth < (Angle == 0.0 ? 1 : 8); ++Azimuth)
                {
                    const double A = FMath::DegreesToRadians(Angle), B = Azimuth * PI / 4.0;
                    const FVector Ray = (LocalSun * FMath::Cos(A) + (LocalSide * FMath::Cos(B) + Other * FMath::Sin(B)) * FMath::Sin(A)).GetSafeNormal();
                    const FVector Up = Body->GetActorQuat().RotateVector(Ray);
                    const double H = R->GetGroundHeight(R->GetActorLocation() + Up * R->PlanetScaleCode, false);
                    ++SampleCount;
                    if (!FMath::IsFinite(H) || !FMath::IsFinite(R->OceanHeight)) return Fail(TEXT("non-finite native ground/ocean sample"));
                    if (!R->bOcean || H > R->OceanHeight + 10000.0)
                    {
                        LocalRay = Ray; GroundCm = H;
                        LocalTangent = FVector::VectorPlaneProject(LocalSide, LocalRay).GetSafeNormal();
                        if (LocalTangent.IsNearlyZero()) LocalTangent = FVector::VectorPlaneProject(Other, LocalRay).GetSafeNormal();
                        UE_LOG(LogTemp, Display, TEXT("[APS.SavedStolyavena] SITE samples=%d sunAngleDeg=%.1f groundCm=%.6f seaCm=%.6f localRay=%s readonlyNativeHeight=1 collisionProof=0"),
                            SampleCount, Angle, H, R->OceanHeight, *LocalRay.ToString());
                        return true;
                    }
                }
            return Fail(TEXT("no dry daytime site in33 bounded native height samples; geography unchanged"));
        }
        bool Place(APlayerController* PC, UAPSWorldOriginSubsystem* Origin, double HeightKm)
        {
            APlanet* P = Body.Get();
            const FVector Up = P->GetActorQuat().RotateVector(LocalRay).GetSafeNormal();
            FVector Tangent = P->GetActorQuat().RotateVector(bSiteSelected ? LocalTangent : LocalSide);
            Tangent = FVector::VectorPlaneProject(Tangent, Up).GetSafeNormal();
            const FVector Forward = (Tangent * FMath::Cos(FMath::DegreesToRadians(35.0)) - Up * FMath::Sin(FMath::DegreesToRadians(35.0))).GetSafeNormal();
            const FRotator Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
            const FVector Generation = Origin->ToGenerationFrame(P->GetActorLocation()) + Up * (P->GetWorldScapeBodyRadiusCm() + GroundCm + HeightKm * 100000.0);
            const FVector Position = Origin->FromGenerationFrame(Generation);
            if (!Finite(Position) || Up.IsNearlyZero() || Tangent.IsNearlyZero() || Rotation.ContainsNaN()) return Fail(TEXT("non-finite or degenerate observer pose"));
            Observer->SetActorLocationAndRotation(Position, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
            PC->SetControlRotation(Rotation);
            if (!Observer->GetActorLocation().Equals(Position, 1.0)) return Fail(TEXT("observer placement readback mismatch"));
            return true;
        }
        bool Fail(const TCHAR* Reason) { if (Error.IsEmpty()) Error = Reason; CurrentPhase = TEXT("failed"); Release(); return false; }

        TWeakObjectPtr<APawn> OriginalPawn, Observer;
        TWeakObjectPtr<AActor> OriginalTarget;
        TWeakObjectPtr<APlayerController> Controller;
        TWeakObjectPtr<UWorld> SavedWorld;
        TWeakObjectPtr<UAPSWorldOriginSubsystem> OriginSystem;
        TWeakObjectPtr<APlanet> Body;
        TWeakObjectPtr<AStar> ParentStar;
        TWeakObjectPtr<AWorldScapeRoot> Terrain;
        TWeakObjectPtr<UMaterialInterface> OriginalMaterial;
        FRotator OriginalControl = FRotator::ZeroRotator;
        FGuid SystemId;
        FVector LocalSun = FVector::ZeroVector, LocalSide = FVector::ZeroVector, LocalRay = FVector::ZeroVector, LocalTangent = FVector::ZeroVector;
        double LastElapsed = 0.0, BodyArrived = 0.0, StableSince = -1.0, RouteStarted = 0.0, NextLog = 0.0, GroundCm = 0.0, ClearanceKm = 1000.0;
        double CurrentPhaseAge = 0.0;
        int32 SystemIndex = INDEX_NONE;
        bool bLeased = false, bVisited = false, bSiteSelected = false, bReady = false, bComplete = false, bReleased = false;
        const TCHAR* CurrentPhase = TEXT("catalogue");
        FString Error, PreviousPhase;
    };
}
#endif
