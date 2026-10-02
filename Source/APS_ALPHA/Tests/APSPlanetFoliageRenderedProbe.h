#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageLeafMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/PlatformMemory.h"
#include "HAL/IConsoleManager.h"
#include "MaterialShared.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StrongObjectPtr.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "StaticMeshResources.h"
#include "APSFoliageWalkingRenderedProbe.h"

// Explicit ground-placement diagnostic. Only diagnostic shader/PSO warmup;
// never changes habitat, instances, pawn placement or production defaults.
namespace APSPlanetFoliageRendered
{
    inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageGround")); }
    inline bool Published() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageGroundPublished")); }
    // Read-only distribution evidence; does not alter the native sampler or
    // waive missing-species acceptance. Sampling is bounded and diagnostic-only.
    inline void LogHabitat(AWorldScapeRoot* Root)
    {
        if (!Root || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageHabitatAudit"))) return;
        if (const auto* Noise = Cast<UAPSWorldScapePlanetNoise>(Root->WorldScapeNoise))
            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_VISUAL_ECOLOGY biomass=%g biodiversity=%g visualDensity=%g; gameplay biology unchanged"),
                Noise->SurfaceProfile.Biomass, Noise->SurfaceProfile.Biodiversity, Noise->SurfaceProfile.VisualFoliageDensity);
        auto* PC = Root->GetWorld()->GetFirstPlayerController();
        auto* Pawn = PC ? PC->GetPawn() : nullptr;
        if (!Pawn) return;
        TMap<UStaticMesh*, int32> Counts;
        TArray<UInstancedStaticMeshComponent*> Meshes; Root->GetComponents(Meshes);
        for (auto* Mesh : Meshes)
            if (IsValid(Mesh) && Mesh->GetStaticMesh()) Counts.FindOrAdd(Mesh->GetStaticMesh()) += Mesh->GetInstanceCount();
        for (auto* Collection : Root->Foliages)
            if (Collection) for (auto* Item : Collection->FoliageList)
                if (auto* Asset = Cast<UWorldScapeFoliagesAsset>(Item))
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_HABITAT_SPECIES mesh=%s instances=%d attempts=%g sectorCm=%g masked=%d overrideConstraints=%d"),
                        *GetPathNameSafe(Asset->StaticMesh), Counts.FindRef(Asset->StaticMesh), Asset->FoliagesCount,
                        Asset->FoliageSectorSize, Asset->bUseFoliageNoiseMask, Asset->bOverrideCollectionConstraint);
        const FVector Origin = Pawn->GetActorLocation();
        const FVector Up = (Origin - Root->GetActorLocation()).GetSafeNormal();
        FVector X, Y; Up.FindBestAxisVectors(X, Y);
        double Sum = 0, Min = 1, Max = 0, MinT = 1, MaxT = 0, MinH = 1, MaxH = 0;
        int32 Dry = 0;
        for (int32 I = -4; I <= 4; ++I) for (int32 J = -4; J <= 4; ++J)
        {
            const FVector WorldPoint = Origin + X * (I * 8000.0) + Y * (J * 8000.0);
            // Same transform as AWorldScapeRoot::GetGroundHeight, not world
            // coordinates passed directly into its ECEF noise sampler.
            const auto N = Root->GetGroundNoise(Root->WorldToECEF(DVector(WorldPoint)).ToFVector(), false);
            Sum += N.FoliageMask; Min = FMath::Min(Min, double(N.FoliageMask)); Max = FMath::Max(Max, double(N.FoliageMask));
            MinT = FMath::Min(MinT, double(N.Temperature)); MaxT = FMath::Max(MaxT, double(N.Temperature));
            MinH = FMath::Min(MinH, double(N.Humidity)); MaxH = FMath::Max(MaxH, double(N.Humidity));
            Dry += !N.Hole && N.Height > Root->GetGroundHeight(WorldPoint, true) && N.WaterMask < .85;
        }
        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_HABITAT_GRID samples=81 spacingCm=8000 extentCm=32000 dry=%d maskMin=%g maskMean=%g maskMax=%g tempMin=%g tempMax=%g humidityMin=%g humidityMax=%g; bounded local read-only sample, not native rejection counters or global habitat proof"),
            Dry, Min, Sum / 81., Max, MinT, MaxT, MinH, MaxH);
    }
    // After the three unchanged natural-pawn frames, inspect an EXISTING nearby
    // instance. Never move/spawn foliage just to obtain a good diagnostic image.
    struct FDetailView
    {
        TWeakObjectPtr<APlayerController> Controller;
        TWeakObjectPtr<AActor> OriginalTarget;
        TWeakObjectPtr<ACameraActor> Camera;
        TWeakObjectPtr<UInstancedStaticMeshComponent> InspectedMesh;
        int32 SavedForcedLOD = 0;
        bool bLOD0Inspection = false;
        int32 LeafFadePhase = 0;
        TWeakObjectPtr<UMaterialInterface> SavedLeafMaterial;
        FVector DetailTarget = FVector::ZeroVector;
        FVector SavedCameraPosition = FVector::ZeroVector;
        TWeakObjectPtr<AWorldScapeRoot> ScatterRoot;
        TSet<TWeakObjectPtr<UStaticMesh>> InspectedShapes;
        int32 ScatterDetailIndex() const { return ScatterRoot.IsValid() ? InspectedShapes.Num() : 0; }
        ~FDetailView() { Restore(); }
        void Restore()
        {
            if (SavedLeafMaterial.IsValid() && InspectedMesh.IsValid())
                InspectedMesh->SetMaterial(1, SavedLeafMaterial.Get());
            SavedLeafMaterial.Reset(); LeafFadePhase = 0;
            if (bLOD0Inspection && InspectedMesh.IsValid()) InspectedMesh->SetForcedLodModel(SavedForcedLOD);
            InspectedMesh.Reset(); bLOD0Inspection = false;
            if (Controller.IsValid() && Camera.IsValid() && Controller->GetViewTarget() == Camera.Get()
                && OriginalTarget.IsValid()) Controller->SetViewTarget(OriginalTarget.Get());
            if (Camera.IsValid()) Camera->Destroy();
            Camera.Reset(); Controller.Reset(); OriginalTarget.Reset();
        }
        bool Begin(UWorld* World, AWorldScapeRoot* Root, FString& Error)
        {
            auto* PC = World ? World->GetFirstPlayerController() : nullptr;
            auto* Pawn = PC ? PC->GetPawn() : nullptr;
            if (!Root || !Pawn) { Error = TEXT("Foliage detail needs real pawn/root"); return false; }
            if (Root->ActorHasTag(TEXT("APS.SurfaceScatter.V2"))) ScatterRoot = Root;
            const FBox PawnBounds = Pawn->GetComponentsBoundingBox(true);
            TArray<UInstancedStaticMeshComponent*> Meshes; Root->GetComponents(Meshes);
            double BestDistance = TNumericLimits<double>::Max();
            FTransform BestTransform; UStaticMesh* BestMesh = nullptr; int32 Sampled = 0;
            UInstancedStaticMeshComponent* BestComponent = nullptr;
            for (auto* Mesh : Meshes)
            {
                if (!IsValid(Mesh) || !Mesh->GetStaticMesh()) continue;
                if (ScatterRoot.IsValid() && InspectedShapes.Contains(Mesh->GetStaticMesh())) continue;
                for (int32 I = 0; I < Mesh->GetInstanceCount() && Sampled < 4096; ++I, ++Sampled)
                {
                    FTransform Transform;
                    if (!Mesh->GetInstanceTransform(I, Transform, true)) continue;
                    // A pebble under the character has no unobstructed ray.
                    // Inspect another existing instance; do not hide/move the pawn.
                    if (PawnBounds.IsValid && PawnBounds.IsInsideOrOn(
                        Transform.TransformPosition(Mesh->GetStaticMesh()->GetBounds().Origin))) continue;
                    const double Distance = FVector::Distance(Transform.GetLocation(), Pawn->GetActorLocation());
                    if (Distance < BestDistance)
                    { BestDistance = Distance; BestTransform = Transform; BestMesh = Mesh->GetStaticMesh(); BestComponent = Mesh; }
                }
            }
            if (!BestMesh || !FMath::IsFinite(BestDistance) || BestDistance > 100000)
            { Error = TEXT("No generated foliage instance within 1km for detail inspection"); return false; }
            const FVector Anchor = BestTransform.GetLocation();
            const FVector Up = (Anchor - Root->GetActorLocation()).GetSafeNormal();
            const auto Bounds = BestMesh->GetBounds();
            const FVector Target = BestTransform.TransformPosition(Bounds.Origin);
            FVector ToPawn = FVector::VectorPlaneProject(Pawn->GetActorLocation() - Anchor, Up).GetSafeNormal();
            if (ToPawn.IsNearlyZero()) ToPawn = FVector::CrossProduct(Up, FVector::RightVector).GetSafeNormal();
            const double Radius = Bounds.SphereRadius * BestTransform.GetScale3D().GetAbsMax();
            FVector View = Target + ToPawn * FMath::Max(400.0, Radius * 3.5) + Up * FMath::Max(120.0, Radius * 0.6);
            const FVector ViewUp = (View - Root->GetActorLocation()).GetSafeNormal();
            // WorldScape returns height above its base sphere, not a radius.
            const double GroundRadius = Root->PlanetScale + Root->GetGroundHeight(View, false);
            const double ViewRadius = (View - Root->GetActorLocation()).Size();
            View += ViewUp * FMath::Max(0.0, GroundRadius + 150.0 - ViewRadius);
            const bool bDistanceAB = FParse::Param(FCommandLine::Get(), TEXT("APSProbeLeafDistanceAB"));
            if (bDistanceAB || ScatterRoot.IsValid())
            {
                // A nearby colony can occlude the entire retreat. Choose ONE
                // clear ray for all distances; never hide/move scene objects.
                const double NearDistance = FMath::Max(400.0, Radius * 3.55);
                TArray<double> SightDistances{NearDistance};
                if (bDistanceAB) SightDistances.Append({8000.0, 8750.0, 9500.0, 11000.0});
                FCollisionQueryParams Query(SCENE_QUERY_STAT(APSFoliageDistanceSight), true);
                // The inspected object's own blocking proxy is the ray target,
                // not an occluder. Never ignore other rocks, terrain or colony.
                // Walking collision is now a project default, also outside the
                // contact diagnostic. Ignore ONLY the matched subject proxy.
                if (auto* TargetProxy = APSFoliageWalkingRendered::MatchingProxy(Root, BestMesh, BestTransform))
                    Query.AddIgnoredComponent(TargetProxy);
                bool bClearRoute = false; int32 Attempts = 0;
                for (double Elevation : {0.20, 0.45, 0.75})
                {
                    for (int32 Azimuth = 0; Azimuth < 24 && !bClearRoute; ++Azimuth)
                    {
                        ++Attempts;
                        const FVector Horizontal = (bDistanceAB ? -ToPawn : ToPawn).RotateAngleAxis(Azimuth * 15.0, Up);
                        const FVector Direction = (Horizontal + Up * Elevation).GetSafeNormal();
                        const FVector Right = FVector::CrossProduct(Up, Direction).GetSafeNormal();
                        bool bClear = true;
                        for (double Distance : SightDistances)
                        {
                            const FVector Eye = Target + Direction * Distance;
                            if ((Eye - Root->GetActorLocation()).Size() < Root->PlanetScale
                                + Root->GetGroundHeight(Eye, false) + 150.0) { bClear = false; break; }
                            // Some character collision presets ignore Visibility.
                            // Never accept a detail shot through the visible pawn.
                            if (PawnBounds.IsValid && FMath::LineBoxIntersection(PawnBounds, Eye, Target, Target - Eye))
                            { bClear = false; break; }
                            for (double Offset : {-0.35, 0.0, 0.35})
                            {
                                FHitResult Hit;
                                if (World->LineTraceSingleByChannel(Hit, Eye, Target + Right * Radius * Offset, ECC_Visibility, Query))
                                { bClear = false; break; }
                            }
                            if (!bClear) break;
                        }
                        if (bClear) { View = Target + Direction * NearDistance; bClearRoute = true; }
                    }
                    if (bClearRoute) break;
                }
                if (!bClearRoute) { Error = TEXT("No unobstructed foliage distance ray; no scene objects changed"); return false; }
                if (bDistanceAB)
                {
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DISTANCE_SIGHT attempts=%d allFiveDistancesClear=1; collision sight test only, rendered visibility still required"), Attempts);
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DETAIL_SIGHT attempts=%d existingShape=1 pawnOcclusionRejected=1; camera-only clear ray, rendered visibility still required"), Attempts);
                }
            }
            const double AnchorError = (Anchor - Root->GetActorLocation()).Size()
                - Root->PlanetScale - Root->GetGroundHeight(Anchor, false);
            if (!FMath::IsFinite(AnchorError) || View.ContainsNaN())
            { Error = TEXT("Invalid foliage world transform/ground oracle"); return false; }
            FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient;
            auto* C = World->SpawnActor<ACameraActor>(View, FRotationMatrix::MakeFromXZ(Target - View, Up).Rotator(), Spawn);
            if (!C) { Error = TEXT("Could not create transient foliage detail camera"); return false; }
            C->SetActorEnableCollision(false); C->GetCameraComponent()->FieldOfView = 60;
            C->GetCameraComponent()->PostProcessBlendWeight = 0;
            Controller = PC; OriginalTarget = PC->GetViewTarget(); Camera = C; PC->SetViewTarget(C);
            DetailTarget = Target; SavedCameraPosition = View;
            InspectedMesh = BestComponent; SavedForcedLOD = BestComponent->ForcedLodModel;
            if (ScatterRoot.IsValid()) InspectedShapes.Add(BestMesh);
            if (const auto* Data = BestMesh->GetRenderData())
                for (int32 LOD = 0; LOD < Data->LODResources.Num(); ++LOD)
                {
                    const auto& Level = Data->LODResources[LOD];
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DETAIL_LOD lod=%d triangles=%u screenSize=%g sections=%d"),
                        LOD, Level.GetNumTriangles(), Data->ScreenSize[LOD].GetValue(), Level.Sections.Num());
                    for (const auto& Section : Level.Sections)
                        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DETAIL_SECTION lod=%d slot=%d triangles=%u material=%s"),
                            LOD, Section.MaterialIndex, Section.NumTriangles, *GetPathNameSafe(BestComponent->GetMaterial(Section.MaterialIndex)));
                }
            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DETAIL mesh=%s sampled=%d originalPawnDistanceCm=%.3f anchorRadialErrorCm=%.3f radiusCm=%.3f anchor=%s camera=%s; existing instance, camera-only diagnostic, not natural player framing"),
                *BestMesh->GetPathName(), Sampled, BestDistance, AnchorError, Radius,
                *Anchor.ToCompactString(), *View.ToCompactString());
            return true;
        }
        bool AdvanceScatter(FString& Error)
        {
            if (!ScatterRoot.IsValid() || InspectedShapes.Num() >= 5) return false;
            AWorldScapeRoot* Root = ScatterRoot.Get();
            Restore();
            // Inspect the next naturally generated shape, never add instances
            // or defeat habitat/spacing gates to make an empty species pass.
            return Begin(Root->GetWorld(), Root, Error);
        }
        bool ForceLOD0(FString& Error)
        {
            if (!InspectedMesh.IsValid() || bLOD0Inspection)
            { Error = TEXT("Foliage LOD0 diagnostic lost its existing component"); return false; }
            // Diagnostic second frame only. The natural LOD image is captured
            // first and the original setting is restored on every cleanup path.
            InspectedMesh->SetForcedLodModel(1); bLOD0Inspection = true;
            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_DETAIL_LOD0 oneExistingComponent=1 previousForcedLOD=%d; not a production setting"), SavedForcedLOD);
            return true;
        }
        const TCHAR* CaptureLabel() const
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeLeafDistanceAB")) && LeafFadePhase > 0)
            {
                const TCHAR* Labels[] = {TEXT("Unused"), TEXT("OwnedLeafNear"), TEXT("OwnedLeaf80m"),
                    TEXT("OwnedLeaf87m"), TEXT("OwnedLeaf95m"), TEXT("OwnedLeaf110m"),
                    TEXT("OwnedLeafNear_RETURN"), TEXT("VendorLeaf_RETURN")};
                return Labels[FMath::Clamp(LeafFadePhase, 1, 7)];
            }
            if (LeafFadePhase == 1) return TEXT("LeafFade20_DIAGNOSTIC");
            if (LeafFadePhase == 2) return TEXT("LeafFadeOriginal_RETURN");
            return bLOD0Inspection ? TEXT("ExistingInstanceLOD0_DIAGNOSTIC") : TEXT("ExistingInstanceDetail");
        }
        bool AdvanceLeafFade(FString& Error)
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeLeafDistanceAB"))) return AdvanceLeafDistance(Error);
            if (!FParse::Param(FCommandLine::Get(), TEXT("APSProbeLeafFadeAB")) || LeafFadePhase == 2) return false;
            auto* Mesh = InspectedMesh.Get();
            if (!Mesh || !bLOD0Inspection)
            { Error = TEXT("Leaf fade A/B needs the existing forced-LOD component"); return false; }
            if (LeafFadePhase == 1)
            {
                if (!SavedLeafMaterial.IsValid()) { Error = TEXT("Lost original leaf material"); return false; }
                Mesh->SetMaterial(1, SavedLeafMaterial.Get()); LeafFadePhase = 2; return true;
            }
            auto* Leaf = Mesh->GetMaterial(1);
            if (!Leaf || Leaf->GetPathName() != TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf"))
            { Error = TEXT("Leaf fade A/B source drift; no material replaced"); return false; }
            float Original = 0;
            if (!Leaf->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FadeDistance")), Original))
            { Error = TEXT("Leaf FadeDistance parameter missing"); return false; }
            auto* MID = UMaterialInstanceDynamic::Create(Leaf, Mesh);
            if (!MID) { Error = TEXT("Leaf diagnostic MID allocation failed"); return false; }
            SavedLeafMaterial = Leaf; MID->SetScalarParameterValue(TEXT("FadeDistance"), 20.f);
            Mesh->SetMaterial(1, MID); LeafFadePhase = 1;
            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_LEAF_FADE_AB original=%g test=20 slot=1 instanceEndCullCm=%d; transient-only, camera/mesh/LOD unchanged"),
                Original, Mesh->InstanceEndCullDistance);
            return true;
        }
        bool AdvanceLeafDistance(FString& Error)
        {
            if (LeafFadePhase == 7) return false;
            auto* Mesh = InspectedMesh.Get();
            if (!Mesh || !Camera.IsValid() || !bLOD0Inspection)
            { Error = TEXT("Owned leaf distance probe lost component/camera"); return false; }
            if (LeafFadePhase == 0)
            {
                auto* Original = Mesh->GetMaterial(1);
                auto* Candidate = LoadObject<UMaterialInterface>(nullptr, APSFoliageLeafMaterial::TemplatePath);
                auto* Resource = Candidate ? Candidate->GetMaterialResource(Mesh->GetWorld()->GetFeatureLevel()) : nullptr;
                if (!Original || Original->GetPathName() != TEXT("/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf")
                    || !Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num())
                { Error = TEXT("Owned leaf candidate/source/shader not ready; bake first"); return false; }
                SavedLeafMaterial = Original; Mesh->SetMaterial(1, Candidate); LeafFadePhase = 1;
            }
            else
            {
                ++LeafFadePhase;
                if (LeafFadePhase <= 5)
                {
                    const double Distances[] = {0, 0, 8000, 8750, 9500, 11000};
                    Camera->SetActorLocation(DetailTarget + (SavedCameraPosition - DetailTarget).GetSafeNormal() * Distances[LeafFadePhase]);
                }
                else
                {
                    Camera->SetActorLocation(SavedCameraPosition);
                    if (LeafFadePhase == 7)
                    {
                        if (!SavedLeafMaterial.IsValid()) { Error = TEXT("Lost vendor leaf return material"); return false; }
                        Mesh->SetMaterial(1, SavedLeafMaterial.Get());
                    }
                }
            }
            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_LEAF_DISTANCE phase=%s distanceCm=%.3f nativeCullCm=%d material=%s; same instance and LOD, camera-only distance trial"),
                CaptureLabel(), FVector::Distance(Camera->GetActorLocation(), DetailTarget), Mesh->InstanceEndCullDistance,
                *GetPathNameSafe(Mesh->GetMaterial(1)));
            return true;
        }
    };
    enum class EReadiness : uint8 { Pending, Ready, Failed };
    struct FReadiness
    {
        TStrongObjectPtr<UMaterialInterface> LeafCandidate;
        bool bLeafWarmupRequested = false;
        TSet<TWeakObjectPtr<UMaterialInterface>> RequestedMaterials;
        TSet<TWeakObjectPtr<UInstancedStaticMeshComponent>> RequestedMeshes;

        EReadiness Poll(UWorld* World, AWorldScapeRoot* Subject, FString& Error)
        {
            if (!Requested()) return EReadiness::Ready;
            if (!World || !IsValid(Subject)) return EReadiness::Pending;
            bool bPending = false;
            if (FParse::Param(FCommandLine::Get(), TEXT("APSProbeLeafDistanceAB")))
            {
                if (!LeafCandidate.IsValid()) LeafCandidate.Reset(LoadObject<UMaterialInterface>(nullptr, APSFoliageLeafMaterial::TemplatePath));
                if (!LeafCandidate.IsValid()) { Error = TEXT("Owned leaf candidate absent; bake first"); return EReadiness::Failed; }
                auto* Resource = LeafCandidate->GetMaterialResource(World->GetFeatureLevel());
                if (Resource && Resource->GetCompileErrors().Num())
                { Error = TEXT("Owned leaf candidate shader compile failed"); return EReadiness::Failed; }
                if (Resource && !bLeafWarmupRequested)
                {
                    bLeafWarmupRequested = true;
#if WITH_EDITOR
                    if (!Resource->IsGameThreadShaderMapComplete()) Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_SHADER_WARMUP candidate=%s complete=%d"),
                        *LeafCandidate->GetPathName(), Resource->IsGameThreadShaderMapComplete());
                }
                bPending |= !Resource || !Resource->IsGameThreadShaderMapComplete();
            }
            int32 SubjectInstances = 0;
            for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
            {
                TArray<UInstancedStaticMeshComponent*> Meshes;
                It->GetComponents(Meshes);
                for (auto* Mesh : Meshes)
                {
                    if (!IsValid(Mesh)) continue;
                    if (*It == Subject) SubjectInstances += Mesh->GetInstanceCount();
                    for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
                    {
                        auto* Material = Mesh->GetMaterial(Slot);
                        auto* Resource = Material ? Material->GetMaterialResource(World->GetFeatureLevel()) : nullptr;
                        if (!Resource) { bPending = true; continue; }
                        if (Resource->GetCompileErrors().Num())
                        { Error = TEXT("Foliage material compilation failed: ") + GetPathNameSafe(Material); return EReadiness::Failed; }
                        if (!RequestedMaterials.Contains(Material))
                        {
                            if (RequestedMaterials.Num() >= 32)
                            { Error = TEXT("Foliage diagnostic exceeded its 32-material warmup limit"); return EReadiness::Failed; }
                            RequestedMaterials.Add(Material);
#if WITH_EDITOR
                            // A deferred uncooked map can stay partial forever without a
                            // request. Submit once; never FinishAllCompilation on the GT.
                            if (!Resource->IsGameThreadShaderMapComplete())
                                Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                            UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_SHADER_WARMUP material=%s complete=%d"),
                                *GetPathNameSafe(Material), Resource->IsGameThreadShaderMapComplete());
                        }
                        bPending |= !Resource->IsGameThreadShaderMapComplete();
                    }
                    if (!RequestedMeshes.Contains(Mesh))
                    {
                        RequestedMeshes.Add(Mesh);
                        // Let the actual HISM component choose its vertex factory/PSOs.
                        Mesh->PrecachePSOs();
                    }
                    bPending |= Mesh->IsPSOPrecaching();
                }
            }
            return bPending || SubjectInstances == 0 ? EReadiness::Pending : EReadiness::Ready;
        }
    };
    inline bool Snapshot(UWorld* World, AWorldScapeRoot* Subject,
        const FAPSResolvedPlanetSurfaceProfile& Profile, double PresentationScale,
        const TCHAR* Phase, bool bFinal, FString& Error)
    {
        if (!Requested()) return true;
        if (bFinal) LogHabitat(Subject);
        if (!APSFoliageWalkingRendered::Snapshot(Subject, Phase, bFinal, Error)) return false;
        const bool bOptIn = FAPSWorldScapeFoliagePolicy::IsRuntimeOptInEnabled();
        const auto ReadCVar = [](const TCHAR* Name) { const auto* V = IConsoleManager::Get().FindConsoleVariable(Name); return V ? V->GetInt() : 0; };
        const auto Plan = FAPSWorldScapeFoliagePolicy::BuildRuntimeActivationPlan(Profile,
            ReadCVar(TEXT("aps.WorldScapeFoliage.Enable")), PresentationScale < 0.999,
            ReadCVar(TEXT("aps.WorldScapeFoliage.Prototype")) == 1, false,
            APSPlanetSurfaceScatter::UsesScatter(Profile, ReadCVar(TEXT("aps.WorldScapeFoliage.SurfaceScatter"))));
        if (Published() && (!Plan.bSurfaceScatterPalette || !Subject
            || !Subject->ActorHasTag(TEXT("APS.SurfaceScatter.V2"))))
        { Error = TEXT("Published foliage proof did not select and bind the reviewed V2 scatter palette"); return false; }
        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_RENDER_PROFILE phase=%s type=%d biomass=%g biodiversity=%g humidity=%g authoredEnabled=%d authoredCollections=%d presentationScale=%.17g optIn=%d planEnabled=%d density=%g prototype=%d"),
            Phase, int32(Profile.PlanetType), Profile.Biomass, Profile.Biodiversity, Profile.Humidity,
            Profile.Foliage.bEnabled, Profile.Foliage.Collections.Num(), PresentationScale, bOptIn,
            Plan.bEnabled, Plan.HabitatDensityScale, Plan.bPrototypePalette);
        int32 Instances = 0, Components = 0, SubjectInstances = 0, SubjectComponents = 0;
        int32 Sectors = 0, QueueTypes = 0, PendingShaders = 0;
        TSet<FString> Bindings;
        bool bUnsafe = false;
        for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
        {
            for (const auto& Type : It->FoliageDataList) Sectors += Type.ActiveFoliageSector.Num();
            QueueTypes += It->FoliageDataToSpawn.Num(); // not queue length; never dequeue worker output
            TArray<UInstancedStaticMeshComponent*> Meshes;
            It->GetComponents(Meshes);
            for (auto* Mesh : Meshes)
            {
                if (!IsValid(Mesh)) continue;
                ++Components; Instances += Mesh->GetInstanceCount();
                if (*It == Subject) { ++SubjectComponents; SubjectInstances += Mesh->GetInstanceCount(); }
                bUnsafe |= Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision || Mesh->CastShadow
                    || Mesh->bAffectDistanceFieldLighting || Mesh->bNeverDistanceCull;
                const FString Identity = GetPathNameSafe(Mesh->GetStaticMesh());
                if (!Bindings.Contains(Identity))
                {
                    Bindings.Add(Identity);
                    UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_RENDER_MESH phase=%s mesh=%s cullEnd=%d instances=%d registered=%d hidden=%d collision=%d shadow=%d distanceField=%d neverCull=%d"),
                        Phase, *Identity, Mesh->InstanceEndCullDistance, Mesh->GetInstanceCount(), Mesh->IsRegistered(), Mesh->bHiddenInGame,
                        int32(Mesh->GetCollisionEnabled()), Mesh->CastShadow, Mesh->bAffectDistanceFieldLighting, Mesh->bNeverDistanceCull);
                    for (int32 Slot=0; Slot<Mesh->GetNumMaterials(); ++Slot)
                    {
                        auto* Material = Mesh->GetMaterial(Slot);
                        auto* Resource = Material ? Material->GetMaterialResource(World->GetFeatureLevel()) : nullptr;
                        // First-use instancing can compile asynchronously. Preserve the first
                        // frame evidence, but require complete shaders in the final snapshot.
                        if (Resource && !Resource->IsGameThreadShaderMapComplete()) ++PendingShaders;
                        bUnsafe |= !Resource || Resource->GetCompileErrors().Num() > 0;
                        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_RENDER_MATERIAL phase=%s slot=%d material=%s shaderComplete=%d compileErrors=%d"),
                            Phase, Slot, *GetPathNameSafe(Material), Resource && Resource->IsGameThreadShaderMapComplete(),
                            Resource ? Resource->GetCompileErrors().Num() : -1);
                    }
                }
            }
        }
        UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_RENDER_STATE phase=%s enabled=%d collections=%d subjectComponents=%d subjectInstances=%d worldComponents=%d worldInstances=%d retainedSectors=%d queueTypes=%d reservedRoots=%d processRAMMiB=%.2f unsafe=%d; snapshot only, not peak or VRAM measurement"),
            Phase, Subject && Subject->bGenerateFoliages, Subject ? Subject->Foliages.Num() : 0,
            SubjectComponents, SubjectInstances, Components, Instances, Sectors, QueueTypes,
            FAPSWorldScapeFoliagePolicy::GetReservedRootCount(World), FPlatformMemory::GetStats().UsedPhysical / 1048576.0, bUnsafe);
        if (bUnsafe || Instances > FAPSWorldScapeFoliagePolicy::MaximumPeakInstancesPerWorld
            || Components > FAPSWorldScapeFoliagePolicy::MaximumPeakMeshComponentsPerWorld)
        { Error = TEXT("Foliage diagnostic found unsafe component/material state or exceeded world admission envelope"); return false; }
        if (bFinal && (!Subject || !Subject->bGenerateFoliages || SubjectInstances == 0))
        { Error = TEXT("Foliage ground trial produced no subject instances; empty scenery is not placement evidence"); return false; }
        if (bFinal && PendingShaders > 0)
        { Error = TEXT("Foliage ground trial ended with incomplete shaders; placement alone is not material acceptance"); return false; }
        if (bFinal && Plan.bSurfaceScatterPalette)
        {
            TSet<UStaticMesh*> PaletteMeshes;
            for (auto* Collection : Subject->Foliages)
                if (Collection) for (auto* Item : Collection->FoliageList)
                    if (const auto* Asset = Cast<UWorldScapeFoliagesAsset>(Item)) PaletteMeshes.Add(Asset->StaticMesh);
            if (PaletteMeshes.Num() != 5)
            { Error = TEXT("SurfaceScatter must bind five different meshes; five scale variants are insufficient"); return false; }
            UE_LOG(LogTemp, Display, TEXT("APS_SCATTER_BINDINGS uniqueMeshes=5; observed instances and visible coverage require frame review"));
        }
        return true;
    }
}
#endif
