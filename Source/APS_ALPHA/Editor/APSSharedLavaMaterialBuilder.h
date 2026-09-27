#pragma once

#if WITH_EDITOR

// Requires only the small shared-core-hooks.patch and the parent's audited
// function-pin-preserving core. No independent copy of the spatial walker.
#include "APSSharedTerrainMaterialBuilder.h"
#include "APSLavaAntiGrid.h"
#include "LocalVertexFactory.h"
#include "StaticParameterSet.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"

namespace APSSharedLavaMaterialBuilder
{
    inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid");
    inline constexpr const TCHAR* NativePath = TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape");
    inline constexpr const TCHAR* NativeTemplatePath = TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_LavaOcean.MI_LavaOcean");
    inline constexpr const TCHAR* APSTemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava");

    inline bool SameTypeState(UMaterialInstanceConstant* Source, UMaterialInstanceConstant* Copy)
    {
        if (!Source || !Copy || !Source->GetStaticParameters().Equivalent(Copy->GetStaticParameters())) return false;
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        Source->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            float A = 0, B = 0;
            if (!Source->GetScalarParameterValue(Info, A) || !Copy->GetScalarParameterValue(Info, B) || A != B) return false;
        }
        Infos.Reset(); Ids.Reset();
        Source->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            FLinearColor A, B;
            if (!Source->GetVectorParameterValue(Info, A) || !Copy->GetVectorParameterValue(Info, B) || A != B) return false;
        }
        Infos.Reset(); Ids.Reset();
        Source->GetAllDoubleVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            FVector4 A, B;
            if (!Source->GetDoubleVectorParameterValue(Info, A) || !Copy->GetDoubleVectorParameterValue(Info, B) || A != B) return false;
        }
        Infos.Reset(); Ids.Reset();
        Source->GetAllTextureParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UTexture* A = nullptr;
            UTexture* B = nullptr;
            if (!Source->GetTextureParameterValue(Info, A) || !Copy->GetTextureParameterValue(Info, B) || A != B) return false;
        }
        return true;
    }

    inline bool Build(IAssetTools& AssetTools)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        FCore Core(AssetTools, Destination);
        const auto Fail = [&Core](const FString& Reason)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.SharedLava] Refused before binding: %s %s"), *Reason, *Core.Error);
            return false;
        };
        UMaterial* Native = LoadObject<UMaterial>(nullptr, NativePath);
        auto* NativeTemplate = LoadObject<UMaterialInstanceConstant>(nullptr, NativeTemplatePath);
        auto* APSTemplate = LoadObject<UMaterialInstanceConstant>(nullptr, APSTemplatePath);
        if (!Native || !NativeTemplate || !APSTemplate
            || NativeTemplate->Parent.Get() != Native || APSTemplate->Parent.Get() != NativeTemplate
            || Native->GetBlendMode() != BLEND_Opaque
            || !Native->GetShadingModels().HasShadingModel(MSM_DefaultLit))
            return Fail(TEXT("Native lava graph or exact APS parent chain changed"));
        for (EMaterialProperty P : {MP_Normal, MP_WorldPositionOffset, MP_PixelDepthOffset, MP_MaterialAttributes, MP_OpacityMask})
            if (const FExpressionInput* I = Native->GetExpressionInputForProperty(P); I && I->Expression)
                return Fail(TEXT("Unaudited native normal/displacement/mask output"));
        int32 Transforms = 0, Interpolators = 0, WorldAlignedCalls = 0;
        for (UMaterialExpression* E : FCore::Expressions(Native))
        {
            if (const auto* T = Cast<UMaterialExpressionTransform>(E))
            {
                if (T->TransformSourceType != TRANSFORMSOURCE_World || T->TransformType != TRANSFORM_Local)
                    return Fail(TEXT("Native lava transform differs from exported World-to-Local contract"));
                ++Transforms;
            }
            if (Cast<UMaterialExpressionVertexInterpolator>(E)) ++Interpolators;
            if (auto* C = Cast<UMaterialExpressionMaterialFunctionCall>(E))
                if (C->MaterialFunction && C->MaterialFunction->GetPathName() ==
                    TEXT("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture")) ++WorldAlignedCalls;
        }
        if (Transforms != 5 || Interpolators != 4 || WorldAlignedCalls != 3)
            return Fail(TEXT("Native lava spatial inventory changed; refresh the export audit"));

        UMaterial* Master = Cast<UMaterial>(Core.Duplicate(Native, TEXT("M_APS_SharedLava")));
        if (!Master || !Core.Patch(Master)) return Fail(TEXT("Spatial graph adaptation"));
        if (!APSLavaAntiGrid::Apply(Core, Master)) return Fail(TEXT("Private 20km lava anti-grid adaptation"));

        // Preserve the closed PLANET shoreline without importing WorldScape's
        // incompatible Hole alpha into gameplay. A=0 still renders physical lava.
        // This new masked permutation needs rendered acceptance before binding.
        auto* Context = Core.Add<UMaterialExpressionScalarParameter>(Master);
        Context->ParameterName = TEXT("APS_UsePresentationWaterMask");
        Context->DefaultValue = 0.0f;
        Context->UpdateParameterGuid(true, true);
        auto* Color = Core.Add<UMaterialExpressionVertexColor>(Master);
        auto* Shore = Core.Add<UMaterialExpressionSmoothStep>(Master);
        Shore->ConstMin = 0.08f;
        Shore->ConstMax = 0.72f;
        Shore->Value.Connect(4, Color);
        auto* Coverage = Core.Add<UMaterialExpressionLinearInterpolate>(Master);
        Coverage->ConstA = 1.0f;
        Coverage->B.Expression = Shore;
        Coverage->Alpha.Expression = Context;
        Master->BlendMode = BLEND_Masked;
        Master->OpacityMaskClipValue = 0.3333f;
        // Preserve native TwoSided/shading/normal settings, all colour/roughness
        // connections, volume sampling stages and all four VertexInterpolators.
        Master->GetExpressionInputForProperty(MP_OpacityMask)->Expression = Coverage;

        auto* NativeCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(NativeTemplate, TEXT("MI_APS_SharedLavaNative")));
        auto* APSCopy = Cast<UMaterialInstanceConstant>(Core.Duplicate(APSTemplate, TEXT("MI_APS_SharedLava")));
        if (!NativeCopy || !APSCopy) return Fail(TEXT("Clone exact MIC chain"));
        NativeCopy->SetParentEditorOnly(Master, false);
        APSCopy->SetParentEditorOnly(NativeCopy, false);
        Master->PostEditChange();
        NativeCopy->PostEditChange();
        APSCopy->PostEditChange();
        if (!SameTypeState(NativeTemplate, NativeCopy) || !SameTypeState(APSTemplate, APSCopy)
            || APSCopy->GetMaterial() != Master || APSCopy->GetBlendMode() != BLEND_Masked)
            return Fail(TEXT("Evaluated scalar/vector/double-vector/texture/static state changed"));

        for (UObject* Output : Core.Outputs)
            if (auto* Material = Cast<UMaterialInterface>(Output))
                Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UObject* Output : Core.Outputs)
        {
            auto* Material = Cast<UMaterialInterface>(Output);
            if (!Material) continue;
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            const bool Ready = Resource && Resource->IsGameThreadShaderMapComplete()
                && Resource->GetCompileErrors().Num() == 0 && Map
                && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
            UE_LOG(LogTemp, Display, TEXT("[APS.SharedLava] Ready=%d path=%s feature=%d"), Ready, *Material->GetPathName(), int32(GMaxRHIFeatureLevel));
            if (!Ready)
            {
                if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                    UE_LOG(LogTemp, Error, TEXT("[APS.SharedLava] %s"), *Error);
                return Fail(TEXT("Exact material permutation incomplete, errors, or missing LocalVF"));
            }
        }
        // Only newly owned outputs. Existing names were refused by Core.Duplicate.
        // This does not select or bind the material to any planet/catalog/level.
        for (UObject* Output : Core.Outputs)
        {
            UPackage* Package = Output->GetOutermost();
            if (!Package->GetName().StartsWith(FString(Destination) + TEXT("/")))
                return Fail(TEXT("Output escaped shared-liquid package directory"));
            Package->MarkPackageDirty();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone;
            Args.SaveFlags = SAVE_NoError;
            if (!UPackage::SavePackage(Package, Output, *File, Args)) return Fail(TEXT("Save newly owned liquid output"));
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.SharedLava] Candidate saved, NOT runtime bound: outputs=%d parent=%s spatialInputs=%d"), Core.Outputs.Num(), *APSCopy->GetPathName(), Core.SpatialInputs);
        return true;
    }
}
#endif
