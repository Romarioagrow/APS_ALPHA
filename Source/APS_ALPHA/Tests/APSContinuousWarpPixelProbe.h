#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APSContinuousWarpPixelAssets.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeCore/Public/WorldScapeMeshComponent.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace APSContinuousWarpPixelProbe
{
    inline constexpr const TCHAR* MasterPath = APSContinuousWarpPixelAssets::MasterPath;
    inline constexpr const TCHAR* TemplatePath = APSContinuousWarpPixelAssets::TemplatePath;
    inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeContinuousWarpPixel")); }
    enum class EReadiness : uint8 { Ready, Pending, Failed };
    struct FPreparationBudget
    {
        static constexpr double TimeoutSeconds = 120.0;
        void Begin(double Now) { if (!bStarted) { StartSeconds = Now; bStarted = true; } }
        bool Started() const { return bStarted; }
        double Elapsed(double Now) const { return bStarted ? FMath::Max(0.0, Now - StartSeconds) : 0.0; }
        bool Expired(double Now) const { return bStarted && Elapsed(Now) >= TimeoutSeconds; }
    private:
        bool bStarted = false;
        double StartSeconds = 0.0;
    };
    inline constexpr double PreparationAllowance(bool bExplicitWarpPixel)
    { return bExplicitWarpPixel ? FPreparationBudget::TimeoutSeconds : 0.0; }
    inline bool IsCandidate(const UMaterialInstanceDynamic* Material)
    {
        return IsValid(Material) && Material->Parent && Material->Parent->GetPathName() == TemplatePath
            && Material->GetMaterial() && Material->GetMaterial()->GetPathName() == MasterPath;
    }
    inline bool WriteFrame(UMaterialInstanceDynamic* Material, const USceneComponent* Root, double PresentationScale)
    {
        // Diagnostic exact-master guard, never a production IsSharedStack opt-in.
        return IsCandidate(Material) && APSSharedTerrainMaterial::WritePhysicalFrame(Material, Root, PresentationScale);
    }

    // One explicitly selected candidate for a whole live descent. No freeze,
    // mesh edits, per-LOD materials, default routing or parameter-mode changes.
    class FLease
    {
        TStrongObjectPtr<UMaterialInstance> Template{nullptr};
        TStrongObjectPtr<UMaterialInstanceDynamic> Native{nullptr}, Candidate{nullptr};
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<USceneComponent> FrameRoot;
        TWeakObjectPtr<APlanetaryBody> Body;
        FWSMaterialLodArray Original;
        double PresentationScale = 0.0;
        bool bSourcesVerified = false, bCompileRequested = false, bPrepared = false;
        FPreparationBudget PreparationBudget;
        double LastPreparationProgressSeconds = -1.0;

        void LogPreparation(const TCHAR* Phase, double Now, FMaterialResource* Resource) const
        {
            int32 GlobalRemainingJobs = -1;
#if WITH_EDITOR
            if (GShaderCompilingManager) GlobalRemainingJobs = GShaderCompilingManager->GetNumRemainingJobs();
#endif
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Prepare] phase=%s elapsed=%.3fs cap=%.0fs template=%s resource=%d complete=%d errors=%d LocalVF=%d compileRequested=%d globalRemainingJobs=%d; nonblocking test-only wait"),
                Phase, PreparationBudget.Elapsed(Now), FPreparationBudget::TimeoutSeconds, TemplatePath, Resource != nullptr,
                Resource && Resource->IsGameThreadShaderMapComplete(), Resource ? Resource->GetCompileErrors().Num() : -1,
                Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType), bCompileRequested, GlobalRemainingJobs);
        }

        bool UniformsMatch(FString& Error) const
        {
            TArray<FMaterialParameterInfo> Infos;
            TArray<FGuid> Ids;
            Native->GetAllScalarParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                float A = 0, B = 0;
                if (!Native->GetScalarParameterValue(Info, A) || !Candidate->GetScalarParameterValue(Info, B) || A != B)
                { Error = TEXT("Warp-pixel changed scalar: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Native->GetAllVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                FLinearColor A, B;
                if (!Native->GetVectorParameterValue(Info, A) || !Candidate->GetVectorParameterValue(Info, B) || A != B)
                { Error = TEXT("Warp-pixel changed vector: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Native->GetAllDoubleVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                FVector4 A, B;
                if (!Native->GetDoubleVectorParameterValue(Info, A) || !Candidate->GetDoubleVectorParameterValue(Info, B) || A != B)
                { Error = TEXT("Warp-pixel physical frame differs from native: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Native->GetAllTextureParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                UTexture* A = nullptr; UTexture* B = nullptr;
                if (!Native->GetTextureParameterValue(Info, A) || !Candidate->GetTextureParameterValue(Info, B) || A != B)
                { Error = TEXT("Warp-pixel changed texture: ") + Info.Name.ToString(); return false; }
            }
            return true;
        }
    public:
        ~FLease() { Restore(); }
        bool Active() const { return Root.IsValid() && Candidate.IsValid(); }
        UMaterialInstanceDynamic* Material() const { return Candidate.Get(); }

        EReadiness Prepare(UWorld* World, FString& Error)
        {
            if (!Requested() || !IsValid(World))
            { Error = TEXT("Warp-pixel probe requires explicit flag and world"); return EReadiness::Failed; }
            const double StartNow = FPlatformTime::Seconds();
            const bool bFirstPreparation = !PreparationBudget.Started();
            PreparationBudget.Begin(StartNow); // First Prepare, not the earlier flight step.
            if (bFirstPreparation) LogPreparation(TEXT("begin"), StartNow, nullptr);
            if (!bSourcesVerified)
            {
                if (!APSContinuousWarpPixelAssets::VerifySources(Error)) return EReadiness::Failed;
                bSourcesVerified = true;
            }
            if (!Template.IsValid()) Template.Reset(LoadObject<UMaterialInstance>(nullptr, TemplatePath));
            if (!Template.IsValid() || !Template->Parent || Template->Parent->GetPathName() != MasterPath)
            { Error = TEXT("Baked current-Continuous warp-pixel candidate missing/wrong parent"); return EReadiness::Failed; }
            auto* Resource = Template->GetMaterialResource(World->GetFeatureLevel());
            if (!Resource || Resource->GetCompileErrors().Num())
            { Error = TEXT("Warp-pixel candidate has no resource or shader errors"); return EReadiness::Failed; }
            const double PollNow = FPlatformTime::Seconds(); // Includes synchronous source/asset loading above.
            if (PreparationBudget.Expired(PollNow))
            {
                LogPreparation(TEXT("timeout"), PollNow, Resource);
                Error = FString::Printf(TEXT("Warp-pixel candidate preparation timed out after %.3fs (cap %.0fs from first Prepare)"),
                    PreparationBudget.Elapsed(PollNow), FPreparationBudget::TimeoutSeconds);
                return EReadiness::Failed;
            }
            if (!Resource->IsGameThreadShaderMapComplete())
            {
#if WITH_EDITOR
                if (!bCompileRequested) Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                bCompileRequested = true;
                if (LastPreparationProgressSeconds < 0.0 || PollNow - LastPreparationProgressSeconds >= 5.0)
                {
                    LastPreparationProgressSeconds = PollNow;
                    LogPreparation(TEXT("progress"), PollNow, Resource);
                }
                Error = FString::Printf(TEXT("Warp-pixel candidate shader preparation pending (%.3fs / %.0fs)"),
                    PreparationBudget.Elapsed(PollNow), FPreparationBudget::TimeoutSeconds);
                return EReadiness::Pending;
            }
            const auto* Map = Resource->GetGameThreadShaderMap();
            if (!Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            { Error = TEXT("Warp-pixel candidate lacks LocalVF"); return EReadiness::Failed; }
            if (!bPrepared) LogPreparation(TEXT("ready"), PollNow, Resource);
            bPrepared = true;
            Error.Reset();
            return EReadiness::Ready;
        }

        bool Begin(APlanetarySurfaceGenerator* S, FString& Error)
        {
            auto* R = IsValid(S) ? S->WorldScapeRootInstance : nullptr;
            auto* B = IsValid(S) ? S->PlanetaryBody : nullptr;
            auto* N = IsValid(S) ? S->ResolvedTerrainMaterialInstance : nullptr;
            if (!Requested() || Active() || !bPrepared || !IsValid(R) || !IsValid(B) || !IsValid(N)
                || !IsValid(R->GetRootComponent()) || S->ResolvedSurfaceProfile.PlanetType != EPlanetType::Frozen
                || B->PlanetType != EPlanetType::Frozen || B->WorldScapePresentationScale != 1.0
                || !N->Parent || N->Parent->GetPathName() != APSTerrainContinuityMaterial::TemplatePath
                || !APSSharedTerrainMaterial::IsSharedStack(N) || R->TerrainMaterial.DefaultMaterial != N
                || R->Terrain_MakeMaterialInstance || !R->TerrainMaterial.MaterialsLod.IsEmpty()
                || R->bFreezeGeneration || !R->IsActorTickEnabled())
            { Error = TEXT("Warp-pixel lease requires prepared candidate and live Frozen single-MID Continuous root"); return false; }
            Candidate.Reset(UMaterialInstanceDynamic::Create(Template.Get(), R));
            if (!Candidate.IsValid()) { Error = TEXT("Warp-pixel MID allocation failed"); return false; }
            Native.Reset(N); Original = R->TerrainMaterial;
            Surface = S; Root = R; FrameRoot = R->GetRootComponent(); Body = B;
            PresentationScale = B->WorldScapePresentationScale;
            Candidate->CopyMaterialUniformParameters(N); // No mode/palette/texture/threshold edits.
            if (!WriteFrame(Candidate.Get(), FrameRoot.Get(), PresentationScale))
            { Error = TEXT("Warp-pixel initial physical frame failed"); Restore(); return false; }
            const TWeakObjectPtr<UMaterialInstanceDynamic> WeakMaterial(Candidate.Get());
            const TWeakObjectPtr<USceneComponent> WeakRoot(FrameRoot.Get());
            const double Scale = PresentationScale;
            FrameRoot->TransformUpdated.AddWeakLambda(Candidate.Get(),
                [WeakMaterial, Scale](USceneComponent* Updated, EUpdateTransformFlags, ETeleportType)
                { WriteFrame(WeakMaterial.Get(), Updated, Scale); });
            APSWorldShiftEvents::BindPostShift(Candidate.Get(),
                [WeakMaterial, WeakRoot, Scale](UWorld* World)
                {
                    auto* Component = WeakRoot.Get();
                    if (IsValid(Component) && Component->GetWorld() == World)
                        WriteFrame(WeakMaterial.Get(), Component, Scale);
                });
            // Restore preserves this exact pointer; production IsSharedStack is
            // deliberately not extended to recognize the diagnostic candidate.
            S->ResolvedTerrainMaterialInstance = Candidate.Get();
            FWSMaterialLodArray Replacement = Original;
            Replacement.DefaultMaterial = Candidate.Get();
            R->UpdateTerrainMaterial(Replacement);
            if (!Validate(S, Error)) { Restore(); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.ContinuousWarpPixel.Probe] begin parent=%s master=%s masterWarpVIs=5; exact native uniforms/textures, live LOD, own post-shift binding; diagnostic replacement, not production default"), TemplatePath, MasterPath);
            return true;
        }

        bool Validate(APlanetarySurfaceGenerator* S, FString& Error) const
        {
            auto* R = Root.Get();
            auto* Component = FrameRoot.Get();
            if (!Active() || S != Surface.Get() || !IsValid(S) || !IsValid(Component) || !Body.IsValid()
                || !Native.IsValid() || !IsCandidate(Candidate.Get()) || S->WorldScapeRootInstance != R
                || S->ResolvedTerrainMaterialInstance != Candidate.Get() || R->TerrainMaterial.DefaultMaterial != Candidate.Get()
                || R->GetRootComponent() != Component || R->Terrain_MakeMaterialInstance || !R->TerrainMaterial.MaterialsLod.IsEmpty()
                || R->bFreezeGeneration || !R->IsActorTickEnabled() || Body->WorldScapePresentationScale != PresentationScale)
            { Error = TEXT("Warp-pixel lease binding/root/generation changed"); return false; }
            // Observe only. Never repair coordinates here and hide a missed event.
            FVector4 Center;
            const FVector Expected = Component->GetComponentLocation();
            if (!Candidate->GetDoubleVectorParameterValue(FMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), Center)
                || FVector(Center.X, Center.Y, Center.Z) != Expected)
            { Error = TEXT("Warp-pixel candidate missed physical center event"); return false; }
            if (!UniformsMatch(Error)) return false;
            for (const auto* Lod : R->WorldScapeLod)
                if (IsValid(Lod) && IsValid(Lod->Mesh))
                    for (int32 Index = 0; Index < Lod->Mesh->GetNumSections(); ++Index)
                        if (Lod->Mesh->GetMaterial(Index) != Candidate.Get())
                        { Error = TEXT("Warp-pixel candidate lost on a published/regenerated section"); return false; }
            Error.Reset();
            return true;
        }

        void Restore()
        {
            if (Candidate.IsValid())
            {
                if (auto* Component = FrameRoot.Get()) Component->TransformUpdated.RemoveAll(Candidate.Get());
                APSWorldShiftEvents::OnPostDoubleShift().RemoveAll(Candidate.Get());
                FCoreDelegates::PostWorldOriginOffset.RemoveAll(Candidate.Get());
                // The original MID retains its original callbacks. Refresh from
                // the actual component, never copy diagnostic parameters back.
                if (Native.IsValid() && FrameRoot.IsValid())
                    APSSharedTerrainMaterial::WriteFrame(Native.Get(), FrameRoot.Get(), PresentationScale);
                if (auto* S = Surface.Get(); S && S->ResolvedTerrainMaterialInstance == Candidate.Get())
                    S->ResolvedTerrainMaterialInstance = Native.Get();
                if (auto* R = Root.Get(); R && R->TerrainMaterial.DefaultMaterial == Candidate.Get())
                    R->UpdateTerrainMaterial(Original);
            }
            Candidate.Reset(); Native.Reset(); Surface.Reset(); Root.Reset(); FrameRoot.Reset(); Body.Reset();
            PresentationScale = 0.0;
        }
    };
}
#endif
