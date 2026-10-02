#pragma once

#include "CoreMinimal.h"
#include "RHI.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaAssets.h"
#include "LocalVertexFactory.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"
#include "ShaderCompiler.h"
#include "UObject/UObjectGlobals.h"

namespace APSUnifiedLavaPendingDiagnostics
{
    // Caller owns the five-second throttle. Never load, request or pump compilation.
    inline void Log(ERHIFeatureLevel::Type FeatureLevel, double Elapsed)
    {
        check(IsInGameThread());
        const TCHAR* Path = APSUnifiedLavaAssets::TemplatePath();
        UMaterialInstanceConstant* MIC = FindObject<UMaterialInstanceConstant>(nullptr, Path);
        const bool bValidFeature = static_cast<uint32>(FeatureLevel) < static_cast<uint32>(ERHIFeatureLevel::Num);
        FMaterialResource* Resource = IsValid(MIC) && bValidFeature ? MIC->GetMaterialResource(FeatureLevel) : nullptr;
        const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        const bool bContent = Map && Map->GetContent();
        int32 Shaders = -1, Pipelines = -1;
        if (bContent) Map->CountNumShaders(Shaders, Pipelines);
        const int32 Errors = Resource ? Resource->GetCompileErrors().Num() : -1;
        FString FirstError = Errors > 0 ? Resource->GetCompileErrors()[0].Left(512) : TEXT("none");
        FirstError.ReplaceInline(TEXT("\r"), TEXT(" "));
        FirstError.ReplaceInline(TEXT("\n"), TEXT(" "));
        const uint32 MapId = Map ? Map->GetCompilingId() : 0;
        FShaderCompilingManager* Manager = GShaderCompilingManager;
        // Finalized clones can have ID zero while the resource still has work.
        const TCHAR* MapJobs = MapId && Manager
            ? (Manager->IsCompilingShaderMap(MapId) ? TEXT("active") : TEXT("inactive")) : TEXT("unknown");
        UE_LOG(LogTemp, Display,
            TEXT("[APS][UnifiedLavaPending] elapsed=%.2fs template=%s found=%d feature=%d resource=%d owner=%s map=%d content=%d complete=%d localVF=%d shaders=%d pipelines=%d finalized=%d success=%d errors=%d firstError=\"%s\" mapId=%u mapJobs=%s ddc=opaque"),
            Elapsed, Path, IsValid(MIC) ? 1 : 0, static_cast<int32>(FeatureLevel), Resource ? 1 : 0,
            *GetPathNameSafe(Resource ? Resource->GetMaterialInterface() : nullptr), Map ? 1 : 0, bContent ? 1 : 0,
            Resource ? static_cast<int32>(Resource->IsGameThreadShaderMapComplete()) : -1,
            bContent ? (Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0) : -1,
            Shaders, Pipelines, Map ? static_cast<int32>(Map->IsCompilationFinalized()) : -1,
            Map ? static_cast<int32>(Map->CompiledSuccessfully()) : -1, Errors, *FirstError, MapId, MapJobs);
        // These counters cover ALL materials, not necessarily the MIC above.
        UE_LOG(LogTemp, Display,
            TEXT("[APS][UnifiedLavaPending] elapsed=%.2fs scope=global manager=%d pendingJobs=%d outstandingJobs=%d remainingJobs=%d hasJobs=%d localWorkers=%d async=%d skipped=%d"),
            Elapsed, Manager ? 1 : 0, Manager ? Manager->GetNumPendingJobs() : -1,
            Manager ? Manager->GetNumOutstandingJobs() : -1, Manager ? Manager->GetNumRemainingJobs() : -1,
            Manager ? static_cast<int32>(Manager->HasShaderJobs()) : -1, Manager ? Manager->GetNumLocalWorkers() : -1,
            Manager ? static_cast<int32>(Manager->AllowAsynchronousShaderCompiling()) : -1,
            Manager ? static_cast<int32>(Manager->IsShaderCompilationSkipped()) : -1);
    }
}
#endif
