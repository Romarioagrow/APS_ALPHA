#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "APSCanonicalReliefChart.h"
#include "APS_ALPHA/Generation/APSNativeGlobeSnapshot.h"
#include "APS_ALPHA/Core/Planetary/APSTerrainContinuityMaterial.h"
#include "APSPublishedTerrainDescentAudit.h"
#include "Async/Async.h"
#include "Camera/CameraTypes.h"
#include "EngineGlobals.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RenderCommandFence.h"
#include "UObject/StrongObjectPtr.h"

// Test-only static Lidim body fixture. The caller retains its existing camera/
// pawn lease, places both at 9.1 km using the existing 55-degree view, and never
// starts the descent clock. No alternate MID, material, geometry or lighting.
namespace APSCanonicalReliefChartAB
{
    inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeCanonicalChartAB")); }
    constexpr double PreparationSeconds = 90.0, WholeTestAllowanceSeconds = 120.0;
    enum class EResult : uint8 { Pending, Complete, Failed };

    class FLease
    {
        struct FBuildResult { APSCanonicalReliefChart::FData Data; FString Error; bool Valid = false; };
        TFuture<FBuildResult> Future;
        TStrongObjectPtr<UMaterialInstanceDynamic> Material{nullptr};
        TStrongObjectPtr<UTexture> OldTexture{nullptr};
        TStrongObjectPtr<UTexture2D> Texture{nullptr};
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<UMaterialInterface> Parent;
        FRenderCommandFence Fence;
        APSPublishedTerrainDescentAudit::FAudit Audit;
        FLinearColor OldVectors[4], BoundVectors[4];
        float OldAvailable = 0;
        FVector CameraLocal = FVector::ZeroVector;
        FQuat CameraLocalRotation = FQuat::Identity;
        float Fov = 0, Aspect = 0;
        uint32 ProfileIdentity = 0, GeometrySignature = 0;
        double BeginTime = 0, PhaseTime = -1, NextCapture = 0, LastProgress = -10;
        uint64 LastCaptureFrame = MAX_uint64;
        int32 PhaseIndex = -1, Captures = 0, TotalCaptures = 0;
        bool bStarted = false, bLeased = false, bComplete = false, bReleased = false;
        FString Error;

        static FName TextureName() { return TEXT("APS_CanonicalChartTexture"); }
        static FName AvailableName() { return TEXT("APS_CanonicalChartAvailable"); }
        static FName VectorName(int32 I)
        {
            static const FName Names[] = {TEXT("APS_CanonicalChartCenter"), TEXT("APS_CanonicalChartU"),
                TEXT("APS_CanonicalChartV"), TEXT("APS_CanonicalChartMetrics")};
            return Names[I];
        }
        EResult Fail(const FString& Why) { Error = Why; return EResult::Failed; }

        // Bounded GT-published sample signature, not a GPU fence or a full mesh
        // equality proof. Never read worker-owned LOD staging arrays.
        bool ReadGeometry(AWorldScapeRoot* R, uint32& Hash) const
        {
            if (!R->WorldScapeLodInGeneration.IsEmpty() || R->IsHidden()) return false;
            Hash = GetTypeHash(R->WorldScapeLod.Num());
            const FTransform Frame = R->GetActorTransform();
            for (const auto* Lod : R->WorldScapeLod)
            {
                auto* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                if (!IsValid(Mesh) || !Mesh->IsVisible() || Mesh->bHiddenInGame || Mesh->GetNumSections() != 3) return false;
                Hash = HashCombine(Hash, GetTypeHash(Mesh->GetUniqueID()));
                bool HasGeometry = false;
                for (int32 SectionIndex = 0; SectionIndex < 3; ++SectionIndex)
                {
                    const auto* S = Mesh->GetProcMeshSection(SectionIndex);
                    if (!S || Mesh->GetMaterial(SectionIndex) != Material.Get()) return false;
                    Hash = HashCombine(Hash, GetTypeHash(S->PlanetVertexBuffer.Num()));
                    Hash = HashCombine(Hash, GetTypeHash(S->PlanetIndexBuffer.Num()));
                    Hash = HashCombine(Hash, GetTypeHash(S->bSectionVisible));
                    if (S->PlanetVertexBuffer.IsEmpty() && S->PlanetIndexBuffer.IsEmpty()) continue;
                    if (S->PlanetVertexBuffer.IsEmpty() || S->PlanetIndexBuffer.Num() < 3 || !S->bSectionVisible) return false;
                    HasGeometry = true;
                    for (int32 Sample = 0; Sample < 32; ++Sample)
                    {
                        const auto& V = S->PlanetVertexBuffer[(int64(Sample) * (S->PlanetVertexBuffer.Num() - 1)) / 31];
                        const FVector P = Frame.InverseTransformPosition(Mesh->GetComponentTransform().TransformPosition(V.Position));
                        if (P.ContainsNaN() || V.Normal.ContainsNaN()) return false;
                        // Rebase roundoff below one millimetre is not a new pose.
                        for (int32 Axis = 0; Axis < 3; ++Axis)
                            Hash = HashCombine(Hash, GetTypeHash(FMath::RoundToInt64(P[Axis] * 10.0)));
                        Hash = HashCombine(Hash, GetTypeHash(V.Normal));
                        Hash = HashCombine(Hash, GetTypeHash(V.Color));
                        Hash = HashCombine(Hash, GetTypeHash(V.UV0));
                        Hash = HashCombine(Hash, GetTypeHash(V.UV1));
                    }
                }
                if (!HasGeometry) return false;
            }
            return !R->WorldScapeLod.IsEmpty();
        }

        bool Validate(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View)
        {
            auto* R = Root.Get();
            if (!IsValid(S) || S != Surface.Get() || !IsValid(R) || S->WorldScapeRootInstance != R
                || S->ResolvedTerrainMaterialInstance != Material.Get() || R->TerrainMaterial.DefaultMaterial != Material.Get()
                || Material->Parent.Get() != Parent.Get() || !IsValid(S->PlanetaryBody)
                || !S->PlanetaryBody->bWorldScapeSurfaceReady || R->bFreezeGeneration || !R->IsActorTickEnabled()
                || APSNativeGlobeSnapshot::Identity(S->PlanetaryBody, S->SurfaceProfileCatalog) != ProfileIdentity)
            { Error = TEXT("Static chart lost original body/root/profile/MID binding"); return false; }
            const FTransform Frame = R->GetActorTransform();
            if (View.Location.ContainsNaN() || View.Rotation.ContainsNaN()
                || !Frame.InverseTransformPosition(View.Location).Equals(CameraLocal, .1)
                || !(Frame.GetRotation().Inverse() * View.Rotation.Quaternion()).Equals(CameraLocalRotation, 1.e-6)
                || View.FOV != Fov || View.AspectRatio != Aspect)
            { Error = TEXT("Static chart camera/optics changed; comparison invalid"); return false; }
            const auto* Resource = Material->GetMaterialResource(R->GetWorld()->GetFeatureLevel());
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            { Error = TEXT("Static chart shader is not complete with LocalVF"); return false; }
#if WITH_EDITOR
            if (!Resource->GetCompileErrors().IsEmpty())
            { Error = TEXT("Static chart shader has compile errors"); return false; }
#endif
            if (bLeased)
            {
                float Available = -1; UTexture* Bound = nullptr;
                if (!Material->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), Available)
                    || Available != (PhaseIndex == 1 ? 1.0f : 0.0f)
                    || !Material->GetTextureParameterValue(FMaterialParameterInfo(TextureName()), Bound) || Bound != Texture.Get())
                { Error = TEXT("Static chart owned texture/availability overwritten"); return false; }
                for (int32 I = 0; I < 4; ++I)
                {
                    FLinearColor Value;
                    if (!Material->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), Value) || Value != BoundVectors[I])
                    { Error = TEXT("Static chart coordinates overwritten"); return false; }
            }
            }
            return true;
        }

        bool Begin(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, double Now)
        {
            if (!WITH_EDITOR || !Requested() || !FApp::IsUnattended() || !IsInGameThread() || !IsValid(S)
                || !IsValid(S->PlanetaryBody) || !IsValid(S->WorldScapeRootInstance))
            { Error = TEXT("Static chart requires explicit unattended live fixture"); return false; }
            APSClosedGlobeMesh::FSamplingFrame Frame; APSClosedGlobeMesh::FBuildOptions Options;
            if (!APSNativeGlobeSnapshot::Capture(S->PlanetaryBody, S, Frame, Options, ProfileIdentity, Error)) return false;
            auto* M = S->ResolvedTerrainMaterialInstance; auto* R = S->WorldScapeRootInstance;
            if (Frame.Profile.PlanetType != EPlanetType::Frozen || R->Seed != 257455
                || !IsValid(M) || !M->GetMaterial() || M->GetMaterial()->GetPathName() != APSTerrainContinuityMaterial::MasterPath
                || !M->Parent || M->Parent->GetPathName() != APSTerrainContinuityMaterial::TemplatePath)
            { Error = TEXT("Static chart is restricted to the actual Lidim fixture and same canonical master/MIC"); return false; }
            UTexture* Previous = nullptr;
            if (!M->GetTextureParameterValue(FMaterialParameterInfo(TextureName()), Previous) || !IsValid(Previous)
                || !M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), OldAvailable) || OldAvailable != 0.0f)
            { Error = TEXT("Static chart requires offline installed, default-disabled parameters"); return false; }
            for (int32 I = 0; I < 4; ++I)
                if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), OldVectors[I]))
                { Error = TEXT("Static chart parameter missing"); return false; }
            Root = R; Surface = S; Parent = M->Parent; Material.Reset(M); OldTexture.Reset(Previous);
            CameraLocal = R->GetActorTransform().InverseTransformPosition(View.Location);
            CameraLocalRotation = R->GetActorQuat().Inverse() * View.Rotation.Quaternion();
            Fov = View.FOV; Aspect = View.AspectRatio;
            if (!Validate(S, View)) return false;
            const FVector3d Ray = CameraLocalRotation.GetForwardVector();
            const double B = FVector3d::DotProduct(CameraLocal, Ray);
            const double Discriminant = B * B - (CameraLocal.SizeSquared() - Frame.Radius * Frame.Radius);
            if (!FMath::IsFinite(Discriminant) || Discriminant <= 0)
            { Error = TEXT("Static chart central camera ray misses physical sphere"); return false; }
            const double Distance = -B - FMath::Sqrt(Discriminant);
            if (!FMath::IsFinite(Distance) || Distance <= 0)
            { Error = TEXT("Static chart sphere intersection is behind camera"); return false; }
            const FVector3d C = (CameraLocal + Ray * Distance).GetSafeNormal();
            const FVector3d U = FVector3d::VectorPlaneProject(CameraLocalRotation.GetRightVector(), C).GetSafeNormal();
            const FVector3d V = FVector3d::CrossProduct(C, U);
            if (U.IsNearlyZero() || !APSCanonicalReliefChart::OrientationContractsPass())
            { Error = TEXT("Static chart orientation contract failed"); return false; }
            BoundVectors[0] = FLinearColor(C.X,C.Y,C.Z,0); BoundVectors[1] = FLinearColor(U.X,U.Y,U.Z,0);
            BoundVectors[2] = FLinearColor(V.X,V.Y,V.Z,0);
            BoundVectors[3] = FLinearColor(Frame.Radius / (1024.0 * 1500.0), .5f / 1024.0f, 1024.0f * 1500.0f, 0);
            bStarted = true; BeginTime = Now;
            // Worker captures only values. Timeout/Release never waits or touches
            // its data; a late result is discarded without a UObject callback.
            Future = Async(EAsyncExecution::ThreadPool, [Frame, C, U, V]() mutable
            {
                FBuildResult Result;
                Result.Valid = APSCanonicalReliefChart::Build(Frame,C,U,V,1024,1500.0,1500.0,Result.Data,Result.Error);
                return Result;
            });
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] PREPARE 1024x1024 spacing=15m halfStep=15m cap=90s center=%s U=%s V=%s cameraLocal=%s; same MID, no geometry/light changes, fixture NOT saved camera"),
                *C.ToString(),*U.ToString(),*V.ToString(),*CameraLocal.ToString());
            return true;
        }

    public:
        ~FLease() { Release(); }
        bool Started() const { return bStarted; }
        bool Complete() const { return bComplete; }
        const FString& GetError() const { return Error; }
        const TCHAR* Phase() const { return PhaseIndex == 0 ? TEXT("A0") : PhaseIndex == 1 ? TEXT("B1") : PhaseIndex == 2 ? TEXT("A0-return") : TEXT("prepare"); }

        // Capture(Phase, IndexWithinPhase, Error) must save PNG successfully.
        // ReadPixels is unfenced to exact GPU pose; the command fence plus 2s
        // and distinct GFrameCounters provide settled observed captures only.
        template<typename TCapture>
        EResult Tick(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, double Now, TCapture Capture)
        {
            if (bReleased || !Error.IsEmpty()) return Fail(Error.IsEmpty() ? TEXT("Static chart already released") : Error);
            if (bComplete) return EResult::Complete;
            if (!bStarted && !Begin(S, View, Now)) return EResult::Failed;
            if (!Validate(S, View)) return EResult::Failed;
            if (PhaseIndex < 0)
            {
                if (Now - BeginTime >= PreparationSeconds) return Fail(TEXT("Static chart asynchronous preparation exceeded 90 seconds"));
                if (Future.IsValid())
                {
                    if (!Future.IsReady())
                    {
                        if (Now - LastProgress >= 5) { LastProgress = Now; UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] preparing elapsed=%.3fs"),Now-BeginTime); }
                        return EResult::Pending;
                    }
                    const auto& Result = Future.Get(); // Ready above: no blocking GT wait.
                    if (!Result.Valid) return Fail(Result.Error);
                    FString UploadError; Texture.Reset(APSCanonicalReliefChart::CreateTransient(Result.Data,UploadError));
                    if (!Texture.IsValid()) return Fail(UploadError);
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] CPU-ready seconds=%.3f heightCalls=%lld bytes=%lld; not gameplay performance"),
                        Result.Data.Metadata.PreparationWallSeconds,Result.Data.Metadata.ActualHeightSamples,Result.Data.ByteCount);
                    Future.Reset(); Fence.BeginFence(); return EResult::Pending;
                }
                if (!Fence.IsFenceComplete()) return EResult::Pending;
                if (!Texture.IsValid() || !Texture->GetResource() || !Texture->GetResource()->IsInitialized())
                    return Fail(TEXT("Static chart texture resource missing after upload fence"));
                if (!ReadGeometry(Root.Get(),GeometrySignature)) return EResult::Pending;
                Material->SetTextureParameterValue(TextureName(),Texture.Get());
                for (int32 I=0; I<4; ++I) Material->SetVectorParameterValue(VectorName(I),BoundVectors[I]);
                Material->SetScalarParameterValue(AvailableName(),0);
                bLeased=true; PhaseIndex=0; PhaseTime=-1; Fence.BeginFence(); return EResult::Pending;
            }
            uint32 CurrentGeometry = 0;
            if (!ReadGeometry(Root.Get(),CurrentGeometry) || CurrentGeometry != GeometrySignature)
                return Fail(TEXT("Static chart observed geometry/publication changed during A/B; no valid comparison"));
            if (PhaseTime < 0)
            {
                if (Now - BeginTime > PreparationSeconds + 24) return Fail(TEXT("Static chart overall phase/fence deadline exceeded"));
                if (!Fence.IsFenceComplete()) return EResult::Pending;
                PhaseTime=Now; NextCapture=Now+2; Captures=0;
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] PHASE %s available=%d GTframe=%llu sampledGeometry=%08x; final lighting normal only"),
                    Phase(),PhaseIndex==1,GFrameCounter,GeometrySignature);
            }
            if (Now - PhaseTime > 8) return Fail(TEXT("Static chart phase did not capture two settled frames within 8 seconds"));
            if (Captures < 2 && Now >= NextCapture && LastCaptureFrame != GFrameCounter)
            {
                if (!Audit.Tick(Root.Get(),View.Location,Now-BeginTime,Phase())) return Fail(Audit.GetError());
                FString CaptureError;
                if (!Capture(Phase(),Captures,CaptureError)) return CaptureError.IsEmpty() ? EResult::Pending : Fail(CaptureError);
                LastCaptureFrame=GFrameCounter; ++Captures; ++TotalCaptures; NextCapture=Now+.25;
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] CAPTURE phase=%s count=%d GTframe=%llu held=%.3fs camera=%s rotation=%s fov=%.6f aspect=%.6f observedUnfenced=1"),
                    Phase(),Captures,GFrameCounter,Now-PhaseTime,*View.Location.ToString(),*View.Rotation.ToString(),View.FOV,View.AspectRatio);
            }
            if (Captures==2 && Now-PhaseTime>=3)
            {
                if (PhaseIndex==2)
                {
                    bComplete=TotalCaptures==6;
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] COMPLETE captures=%d phases=0-1-0 samePose=1 sampledGeometryStable=1; evidence only, NOT visual acceptance"),TotalCaptures);
                    return bComplete ? EResult::Complete : Fail(TEXT("Static chart capture contract incomplete"));
                }
                ++PhaseIndex; Material->SetScalarParameterValue(AvailableName(),PhaseIndex==1?1.0f:0.0f);
                PhaseTime=-1; Fence.BeginFence();
            }
            return EResult::Pending;
        }

        void Release()
        {
            if (bReleased) return;
            bReleased=true;
            if (bLeased && IsInGameThread() && Material.IsValid())
            {
                Material->SetScalarParameterValue(AvailableName(),OldAvailable);
                Material->SetTextureParameterValue(TextureName(),OldTexture.Get());
                for (int32 I=0; I<4; ++I) Material->SetVectorParameterValue(VectorName(I),OldVectors[I]);
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart.AB] restored six effective chart values on original MID; may retain equivalent explicit overrides, no broad clear, other params untouched"));
            }
            // Reset is nonblocking; the CPU task retains no UObject or this.
            Future.Reset(); Texture.Reset(); OldTexture.Reset(); Material.Reset();
        }
    };
}
#endif
