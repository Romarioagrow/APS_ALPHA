#pragma once

#include "CoreMinimal.h"
#include "APSUnifiedLavaAssets.h"
#include "HAL/PlatformProperties.h"
#include "LocalVertexFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"
#include "Misc/App.h"
#include "RHIGlobals.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

namespace APSUnifiedLavaSurface
{
    enum class EPreparationState : uint8
    {
        Unrequested, Pending, Ready, Failed
    };

    // Owned by the caller, which keeps its existing surface until Ready. Reset
    // releases our reference; it does not cancel shared engine compilation jobs.
    struct FMaterialPreparation
    {
        EPreparationState Poll(ERHIFeatureLevel::Type InFeatureLevel)
        {
            check(IsInGameThread());
            if (State == EPreparationState::Failed) return State;
            if (static_cast<uint32>(InFeatureLevel) >= static_cast<uint32>(ERHIFeatureLevel::Num))
                return Fail(TEXT("InvalidFeatureLevel"));
            if (State != EPreparationState::Unrequested && FeatureLevel != InFeatureLevel)
                return Fail(TEXT("FeatureLevelChangedRequiresReset"));
            if (GUsingNullRHI) return Fail(TEXT("NullRHI"));
            if (!FApp::CanEverRender()) return Fail(TEXT("RenderingUnavailable"));

            const TCHAR* TemplatePath = APSUnifiedLavaAssets::TemplatePath();
            const TCHAR* MasterPath = APSUnifiedLavaAssets::MasterPath();
            if (State == EPreparationState::Unrequested)
            {
                FeatureLevel = InFeatureLevel;
                Material.Reset(LoadObject<UMaterialInstance>(nullptr, TemplatePath));
                State = EPreparationState::Pending;
            }
            if (!IsValid(Material.Get())) return Fail(TEXT("MissingTemplate"));
            if (!Material->IsA<UMaterialInstanceConstant>() || Material->GetPathName() != TemplatePath)
                return Fail(TEXT("UnexpectedTemplate"));
            const UMaterial* Master = Material->GetMaterial();
            if (!IsValid(Master) || Master->GetPathName() != MasterPath)
                return Fail(TEXT("UnexpectedMaster"));
            if (Material->GetBlendMode() != BLEND_Opaque) return Fail(TEXT("NonOpaque"));

            // Never replace the MIC resource with its master's permutation.
            FMaterialResource* Resource = Material->GetMaterialResource(FeatureLevel);
            EPreparationState Observed = Inspect(Resource);
            if (Observed != EPreparationState::Pending) return State = Observed;

#if WITH_EDITOR
            if (FPlatformProperties::RequiresCookedData()) return Fail(NotReadyReason(Resource));
            if (!bCompileRequested)
            {
                // UE may turn Default into a synchronous request when background
                // compilation is disabled. Reject that mode before requesting it.
                if (!GShaderCompilingManager || !GShaderCompilingManager->AllowAsynchronousShaderCompiling()
                    || GShaderCompilingManager->IsShaderCompilationSkipped())
                    return Fail(TEXT("AsyncCompilationUnavailable"));
                if (Resource && Resource->RequiresSynchronousCompilation())
                    return Fail(TEXT("SynchronousCompilationRequired"));
                // This public query consumes only an already-ready cache result.
                // Re-read the MIC resource/readiness in case that publishes a map.
                const bool bCompilationPending = Resource && !Resource->IsCompilationFinished();
                Resource = Material->GetMaterialResource(FeatureLevel);
                Observed = Inspect(Resource);
                if (Observed != EPreparationState::Pending) return State = Observed;
                if (bCompilationPending && (!Resource || !Resource->GetGameThreadShaderMap()))
                    return State = EPreparationState::Pending;
                bCompileRequested = true;
                if (bCompilationPending)
                {
                    // Another owner already has jobs for this permutation. The
                    // engine deduplicates requests at the same/lower priority.
                    Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::Normal);
                }
                else
                {
                    // Lazy PostLoad(None) can have an empty map with no submitted
                    // jobs. Default requests them on the same saved MIC.
                    Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
                }
                Resource = Material->GetMaterialResource(FeatureLevel);
                Observed = Inspect(Resource);
                if (Observed != EPreparationState::Pending) return State = Observed;
            }
            if (!Resource) return Fail(TEXT("MissingResource"));
            const bool bCompilationFinished = Resource->IsCompilationFinished();
            Resource = Material->GetMaterialResource(FeatureLevel);
            Observed = Inspect(Resource);
            if (Observed != EPreparationState::Pending) return State = Observed;
            if (!Resource || bCompilationFinished) return Fail(NotReadyReason(Resource));
            return State = EPreparationState::Pending;
#else
            // Cooked runtimes cannot repair missing shader permutations.
            return Fail(NotReadyReason(Resource));
#endif
        }

        void Reset()
        {
            check(IsInGameThread());
            Material.Reset();
            FeatureLevel = ERHIFeatureLevel::Num;
            bCompileRequested = false;
            FailureReason = TEXT("None");
            State = EPreparationState::Unrequested;
        }

        UMaterialInstance* GetMaterial() const { return Material.Get(); }
        const TCHAR* GetFailureReason() const { return FailureReason; }

    private:
        EPreparationState Inspect(FMaterialResource* Resource)
        {
#if WITH_EDITOR
            if (Resource && Resource->GetCompileErrors().Num() > 0)
                return Fail(TEXT("ShaderCompileErrors"));
#endif
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Map || !Resource->IsGameThreadShaderMapComplete())
                return EPreparationState::Pending;
            if (!Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Fail(TEXT("MissingLocalVertexFactory"));
            return EPreparationState::Ready;
        }

        const TCHAR* NotReadyReason(FMaterialResource* Resource) const
        {
            if (!Resource) return TEXT("MissingResource");
            if (!Resource->GetGameThreadShaderMap()) return TEXT("MissingShaderMap");
            return TEXT("IncompleteShaderMap");
        }

        EPreparationState Fail(const TCHAR* Reason)
        {
            FailureReason = Reason;
            return State = EPreparationState::Failed;
        }

        TStrongObjectPtr<UMaterialInstance> Material;
        ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::Num;
        bool bCompileRequested = false;
        const TCHAR* FailureReason = TEXT("None");
        EPreparationState State = EPreparationState::Unrequested;
    };
}
