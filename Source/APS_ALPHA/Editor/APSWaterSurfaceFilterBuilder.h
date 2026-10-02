#pragma once
#if WITH_EDITOR
#include "APSWaterDepthMaterialBuilder.h"
#include "APSWaterSurfacePassBuilder.h"
#include "APSWaterWavePrecisionBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h"
#include "Materials/MaterialExpressionFrac.h"

namespace APSWaterSurfaceFilterBuilder
{
inline bool Build(IAssetTools& AssetTools, bool bSurfacePass = false, bool bPrecise = false, bool bCameraRelative = false)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    if (bCameraRelative && !bPrecise) return false;
    if (bSurfacePass && bPrecise) return false; // One experimental axis at a time.
    const TCHAR* Folder = bCameraRelative ? APSWaterSurfaceFilter::RelativeFolder : bPrecise ? APSWaterSurfaceFilter::PreciseFolder : bSurfacePass ? APSWaterSurfaceFilter::PassFolder : APSWaterSurfaceFilter::Folder;
    FCore Core(AssetTools, Folder);
    auto Fail = [&Core](const TCHAR* Why)
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.WaterSurfaceFilter] %s %s"), Why, *Core.Error);
        return false;
    };
    auto* Source = LoadObject<UMaterial>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/M_APS_WaterDepth.M_APS_WaterDepth"));
    auto* Template = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260928/MI_APS_WaterDepth.MI_APS_WaterDepth"));
    if (!SavedSourceMatches(Source, TEXT("A711A09E59D490487A04CAC4E8BDB5621E8833D2"))
        || !SavedSourceMatches(Template, TEXT("C3451635932D6FDE94FF4D23E2EEF177265C352F"))
        || Template->Parent.Get() != Source || Source->bTangentSpaceNormal
        || Source->GetBlendMode() != BLEND_Masked)
        return Fail(TEXT("Filtered depth source contract changed"));
    auto* Master = Cast<UMaterial>(Core.Duplicate(Source, TEXT("M_APS_WaterSurface")));
    if (!Master) return Fail(TEXT("New output only"));
    const auto OriginalNodes = FCore::Expressions(Master);
    int32 Bands = 0;
    for (UMaterialExpression* Node : OriginalNodes)
    {
        auto* Noise = Cast<UMaterialExpressionVectorNoise>(Node);
        if (!Noise) continue;
        auto* Domain = Cast<UMaterialExpressionDivide>(Noise->Position.Expression);
        auto* Scale = Domain ? Cast<UMaterialExpressionScalarParameter>(Domain->B.Expression) : nullptr;
        if (Noise->NoiseFunction != VNF_GradientALU || Noise->bTiling
            || !Domain || !Scale || !Domain->A.Expression)
            return Fail(TEXT("Unsupported wave domain/noise node"));
        const bool bPrimary = Scale->ParameterName == TEXT("WaveScaleCm");
        if (!bPrimary && Scale->ParameterName != TEXT("PhysicalWaveDetailScaleCm"))
            return Fail(TEXT("Unknown wave band"));
        Bands |= bPrimary ? 1 : 2;
        const int32 Period = bPrimary ? APSWaterSurfaceFilter::PrimaryPeriod : APSWaterSurfaceFilter::SecondaryPeriod;
        // Keep the original upstream frame/scale graph. This bounds the noise
        // domain; it does NOT by itself prove precision of all upstream LWC ops.
        UMaterialExpressionCustom* Precise = bPrecise ? APSWaterWavePrecisionBuilder::Add(Core, Master, Domain, Scale, Period, bCameraRelative) : nullptr;
        if (bPrecise && !Precise) return Fail(TEXT("Compensated domain contract"));
        auto* TileDomain = Core.Add<UMaterialExpressionDivide>(Master);
        TileDomain->A.Expression = Domain; TileDomain->ConstB = float(Period);
        auto* Fraction = Core.Add<UMaterialExpressionFrac>(Master);
        Fraction->Input.Expression = TileDomain;
        auto* Folded = Core.Add<UMaterialExpressionMultiply>(Master);
        Folded->A.Expression = Fraction; Folded->ConstB = float(Period);
        Noise->Position.Expression = Precise ? static_cast<UMaterialExpression*>(Precise) : Folded;
        Noise->bTiling = true; Noise->TileSize = Period;
        auto* DX = Core.Add<UMaterialExpressionDDX>(Master);
        auto* DY = Core.Add<UMaterialExpressionDDY>(Master);
        DX->Value.Expression = Domain; DY->Value.Expression = Domain;
        auto* Filter = Core.Add<UMaterialExpressionCustom>(Master);
        Filter->Inputs.Empty(); // UE creates an unnamed default pin; indices below are Noise/DX/DY.
        Filter->OutputType = CMOT_Float4;
        Filter->Description = TEXT("Pixel-footprint wave filter; unfolded LWC derivatives, no distance switch");
        Filter->Code = APSWaterSurfaceFilter::FilterHlsl();
        for (auto Pair : {TPair<FName, UMaterialExpression*>(TEXT("Noise"), Noise),
                         TPair<FName, UMaterialExpression*>(TEXT("DX"), DX),
                         TPair<FName, UMaterialExpression*>(TEXT("DY"), DY)})
        {
            FCustomInput Input; Input.InputName = Pair.Key; Input.Input.Expression = Pair.Value;
            Filter->Inputs.Add(Input);
        }
        if (Precise)
        {
            Filter->Inputs[1].Input.Connect(1, Precise);
            Filter->Inputs[2].Input.Connect(2, Precise);
        }
        int32 Consumers = 0;
        for (UMaterialExpression* Original : OriginalNodes)
            for (FExpressionInput* Input : Original->GetInputsView())
                if (Input && Input->Expression == Noise)
                { Input->Expression = Filter; ++Consumers; }
        if (Consumers != 2) return Fail(TEXT("Expected gradient and scalar wave consumers"));
    }
    if (Bands != 3) return Fail(TEXT("Both physical wave bands required"));
    // No coverage, displacement, depth, lighting model or texture changes.
    for (EMaterialProperty Property : {MP_BaseColor, MP_Normal, MP_Roughness, MP_Metallic,
        MP_Specular, MP_EmissiveColor, MP_OpacityMask, MP_WorldPositionOffset, MP_PixelDepthOffset})
        if (!SameInput(*Source->GetExpressionInputForProperty(Property), *Master->GetExpressionInputForProperty(Property), true))
            return Fail(TEXT("Material output changed unexpectedly"));
    auto* Candidate = Cast<UMaterialInstanceConstant>(Core.Duplicate(Template, TEXT("MI_APS_WaterSurface")));
    if (!Candidate) return Fail(TEXT("New MIC only"));
    Candidate->SetParentEditorOnly(Master, false);
    Candidate->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("WaveScaleCm")), APSWaterSurfaceFilter::PrimaryScaleCm);
    Candidate->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("PhysicalWaveDetailScaleCm")), APSWaterSurfaceFilter::SecondaryScaleCm);
    // Keep noise in the normal: dye/roughness blotches are not water displacement.
    Candidate->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("WaveColorStrength")), 0.0f);
    Candidate->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("PhysicalWaveRoughnessStrength")), 0.0f);
    if (bSurfacePass && !APSWaterSurfacePassBuilder::Add(Core, Master))
        return Fail(TEXT("Water-only pass graph contract failed"));
    Master->PostEditChange(); Candidate->PostEditChange();
    const EMaterialShadingModel ExpectedModel = bSurfacePass ? MSM_SingleLayerWater : MSM_DefaultLit;
    if (!Master->GetShadingModels().HasOnlyShadingModel(ExpectedModel)
        || !Candidate->GetShadingModels().HasOnlyShadingModel(ExpectedModel))
        return Fail(TEXT("Water shading model did not round-trip"));
    for (UObject* Output : Core.Outputs)
        CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (UObject* Output : Core.Outputs)
    {
        auto* Resource = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        if (!Resource || !Map || !Resource->IsGameThreadShaderMapComplete()
            || Resource->GetCompileErrors().Num() || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
        {
            if (Resource) for (const FString& Error : Resource->GetCompileErrors())
                UE_LOG(LogTemp, Error, TEXT("[APS.WaterSurfaceFilter] %s"), *Error);
            return Fail(TEXT("Water surface shader/LocalVF incomplete"));
        }
    }
    if (!SavedSourceMatches(Source, TEXT("A711A09E59D490487A04CAC4E8BDB5621E8833D2"))
        || !SavedSourceMatches(Template, TEXT("C3451635932D6FDE94FF4D23E2EEF177265C352F")))
        return Fail(TEXT("Read-only inputs changed during bake"));
    if (Core.Outputs.Num() != 2) return Fail(TEXT("Expected exactly two new assets"));
    for (UObject* Output : Core.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(Folder) + TEXT("/")))
            return Fail(TEXT("Output escaped candidate folder"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Output, *Filename, Args)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterSurfaceFilter] Saved=2 bound=0 noiseBands=2 domainFold=1 footprintFilter=1 surfacePass=%d compensatedDomain=%d cameraRelative=%d geometry=unchanged; rendered acceptance pending"), int(bSurfacePass), int(bPrecise), int(bCameraRelative));
    return true;
}
}
#endif
