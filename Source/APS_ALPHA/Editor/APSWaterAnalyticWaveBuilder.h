#pragma once
#if WITH_EDITOR
#include "APSWaterSurfaceFilterBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h"

namespace APSWaterAnalyticWaveBuilder
{
inline bool Build(IAssetTools& AssetTools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    const bool bAnchored = FParse::Param(FCommandLine::Get(), TEXT("APSWaterAnchorSplit"));
    const bool bNoise = FParse::Param(FCommandLine::Get(), TEXT("APSWaterAnchorNoise"));
    if (bNoise && !bAnchored) return false;
    const TCHAR* Folder = bNoise ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchoredNoise20260930") : bAnchored ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnchored20260930") : APSWaterAnalyticWaves::Folder;
    FCore Core(AssetTools, Folder);
    auto Fail = [&Core](const TCHAR* Why)
    { UE_LOG(LogTemp, Error, TEXT("[APS.WaterAnalytic] %s %s"), Why, *Core.Error); return false; };
    auto* Source = LoadObject<UMaterial>(nullptr, APSWaterSurfaceFilter::RelativeMasterPath);
    auto* SourceMI = LoadObject<UMaterialInstanceConstant>(nullptr, APSWaterSurfaceFilter::RelativeTemplatePath);
    const auto Unchanged = [&]()
    { return SavedSourceMatches(Source, TEXT("1EE4B8928AEC1B5564768E920F986BCB33FE9CFE"))
        && SavedSourceMatches(SourceMI, TEXT("D07046FC4FD669F02DB7F3F9F35662B2BB102A20")); };
    if (!Unchanged() || SourceMI->Parent.Get() != Source || Source->bTangentSpaceNormal
        || Source->GetBlendMode() != BLEND_Masked || !Source->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit))
        return Fail(TEXT("Camera-relative source contract drift"));
    auto* M = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_WaterAnalytic")));
    if (!M) return Fail(TEXT("New output only"));
    const auto Originals = FCore::Expressions(M);
    int32 Bands = 0;
    for (auto* E : Originals)
    {
        auto* Noise = Cast<UMaterialExpressionVectorNoise>(E);
        if (!Noise) continue;
        if (Noise->NoiseFunction != VNF_GradientALU || !Noise->bTiling
            || !Cast<UMaterialExpressionCustom>(Noise->Position.Expression))
            return Fail(TEXT("Expected precise folded two-band source"));
        // The four-sine variant localizes coordinate errors but is too regular
        // for water art. Keep the source gradient-noise kernel in this candidate.
        if (bNoise) { ++Bands; continue; }
        auto* Wave = Core.Add<UMaterialExpressionCustom>(M);
        Wave->Inputs.Empty(); Wave->OutputType = CMOT_Float4;
        Wave->Description = TEXT("Analytic four-direction water gradient; kernel-only candidate");
        FCustomInput P; P.InputName = TEXT("P"); P.Input = Noise->Position; Wave->Inputs.Add(P);
        Wave->Code = APSWaterAnalyticWaves::Hlsl();
        int32 Consumers = 0;
        for (auto* Original : Originals)
            for (FExpressionInput* Input : Original->GetInputsView())
                if (Input && Input->Expression == Noise) { Input->Expression = Wave; ++Consumers; }
        if (Consumers != 1) return Fail(TEXT("Expected exactly one footprint-filter consumer per band"));
        ++Bands;
    }
    if (Bands != 2) return Fail(TEXT("Expected two wave bands"));
    if (bAnchored)
    {
        int32 Domains = 0;
        for (auto* E : Originals)
        {
            auto* D = Cast<UMaterialExpressionCustom>(E);
            if (!D || D->AdditionalOutputs.Num() != 2 || !D->Code.Contains(TEXT("DFFracDemote"))) continue;
            if (D->Inputs.Num() != 9 || !D->Code.Contains(TEXT("PreViewTranslation"))) return Fail(TEXT("Anchor domain drift"));
            const bool bPrimary = D->Code.Contains(TEXT("1023.0f"));
            if (!bPrimary && !D->Code.Contains(TEXT("1533.0f"))) return Fail(TEXT("Unknown fold period"));
            const int32 Period = bPrimary ? 1023 : 1533;
            // Fold the view origin FIRST. Per-pixel centimetres never pass
            // through an enormous absolute coordinate or its DF high part.
            D->Code = TEXT("FDFVector3 A = DFSubtract(DFNegate(ResolvedView.PreViewTranslation), WSToDF(LWCCenter));\n")
                TEXT("FDFVector3 AX = DFMultiply(A, MakeDFVector3(XHigh,XLow));\n")
                TEXT("FDFVector3 AY = DFMultiply(A, MakeDFVector3(YHigh,YLow));\n")
                TEXT("FDFVector3 AZ = DFMultiply(A, MakeDFVector3(ZHigh,ZLow));\n")
                TEXT("FDFScalar X = DFAdd(DFAdd(DFGetX(AX),DFGetY(AX)),DFGetZ(AX));\n")
                TEXT("FDFScalar Y = DFAdd(DFAdd(DFGetX(AY),DFGetY(AY)),DFGetZ(AY));\n")
                TEXT("FDFScalar Z = DFAdd(DFAdd(DFGetX(AZ),DFGetY(AZ)),DFGetZ(AZ));\n")
                TEXT("float s = max(Size,1.0);\nFDFVector3 U = DFDivide(MakeDFVector(X,Y,Z),s);\n")
                + FString::Printf(TEXT("float3 origin = DFFracDemote(DFDivide(U,%d.0))*%d.0;\n"),Period,Period)
                + TEXT("float3 local = float3(dot(Raw,XHigh)+dot(Raw,XLow),dot(Raw,YHigh)+dot(Raw,YLow),dot(Raw,ZHigh)+dot(Raw,ZLow))/s;\n")
                TEXT("DomainDX=ddx(local); DomainDY=ddy(local);\n")
                + FString::Printf(TEXT("return frac((origin+local)/%d.0)*%d.0;\n"),Period,Period);
            D->Description = TEXT("View-origin folded separately from small per-pixel wave delta; same planet-fixed phase");
            ++Domains;
        }
        if (Domains != 2) return Fail(TEXT("Expected exactly two anchored domains"));
    }
    for (EMaterialProperty Property : {MP_BaseColor, MP_Normal, MP_Roughness, MP_Metallic,
        MP_Specular, MP_EmissiveColor, MP_OpacityMask, MP_WorldPositionOffset, MP_PixelDepthOffset})
        if (!SameInput(*Source->GetExpressionInputForProperty(Property), *M->GetExpressionInputForProperty(Property), true))
            return Fail(TEXT("Output wiring drift"));
    auto* MI = Cast<UMaterialInstanceConstant>(Core.Duplicate(SourceMI, TEXT("MI_APS_WaterAnalytic")));
    if (!MI) return Fail(TEXT("New template only"));
    MI->SetParentEditorOnly(M, false); M->PostEditChange(); MI->PostEditChange();
    for (auto* Output : Core.Outputs)
        CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* Output : Core.Outputs)
    {
        auto* Resource = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Map || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Incomplete shaders/LocalVF"));
    }
    if (!Unchanged() || Core.Outputs.Num() != 2) return Fail(TEXT("Input/output contract drift"));
    for (auto* Output : Core.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(Folder) + TEXT("/"))) return Fail(TEXT("Output escaped folder"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Output, *Filename, Args)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterAnalytic] saved=2 bound=0 bands=2 anchorSplit=%d originalNoise=%d geometryDepthFrameLighting=unchanged; visual/cost validation pending"), int(bAnchored), int(bNoise));
    return true;
}
}
#endif
