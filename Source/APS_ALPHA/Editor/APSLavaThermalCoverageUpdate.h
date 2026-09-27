#pragma once
#if WITH_EDITOR
#include "APSLavaAntiGridUpdate.h"
#include "APSLavaCrustReflectanceUpdate.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"

// Offline shared-shader trial. Default strength is ZERO until rendered approval.
// One emission multiplier; native colours, roughness, geometry and coverage stay
// intact. No new texture fetches. The same physical filtering applies in both
// gameplay and PLANET, never a material substitution by observer distance.
namespace APSLavaThermalCoverageUpdate
{
    struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
    inline constexpr FExpected Expected[] = {
        {TEXT("M_APS_SharedLava"), TEXT("2DB3405BECA5CE8B178CE728F731AB76F24AEFF3")},
        {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
        {TEXT("MI_APS_SharedLava"), TEXT("9E5D5EF9183A800DA87D88F42B4CE47ECFDE6342")},
        {TEXT("MF_APS_LavaWAT20kmAntiGrid_v1"), TEXT("27640676C83642BF2A50C432F1D605C2025FA306")}
    };

    inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B)
    {
        return A.Expression == B.Expression && A.OutputIndex == B.OutputIndex
            && A.Mask == B.Mask && A.MaskR == B.MaskR && A.MaskG == B.MaskG
            && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
    }

    inline bool Run(IAssetTools& Tools)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        using APSLavaAntiGridUpdate::Filename;
        using APSLavaAntiGridUpdate::Hash;
        FCore Core(Tools, APSSharedLavaMaterialBuilder::Destination);
        const auto Refuse = [](const FString& Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[APS.LavaThermalCoverage] Refused: %s"), *Why);
            return false;
        };
        if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
        for (const FExpected& E : Expected)
            if (Hash(Filename(E.Name)) != E.SHA1)
                return Refuse(TEXT("Accepted asset changed: ") + Filename(E.Name));
        const FString Root(APSSharedLavaMaterialBuilder::Destination);
        auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
        auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
        auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
        if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
            || Native->Parent.Get() != Master || APS->Parent.Get() != Native
            || Master->GetBlendMode() != BLEND_Masked || FCore::Expressions(Master).Num() != 119)
            return Refuse(TEXT("Unexpected graph/parent/blend/unsaved state"));
        FExpressionInput* Emissive = Master->GetExpressionInputForProperty(MP_EmissiveColor);
        auto* FineOffset = FindObject<UMaterialExpressionAdd>(Master, TEXT("MaterialExpressionAdd_26"));
        auto* Fine = FineOffset ? Cast<UMaterialExpressionMaterialFunctionCall>(FineOffset->A.Expression) : nullptr;
        UMaterialExpressionDoubleVectorParameter* InverseScale = nullptr;
        for (UMaterialExpression* E : FCore::Expressions(Master))
            if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E))
                if (P->ParameterName == TEXT("APS_SharedInverseScale")) InverseScale = P;
        if (!Emissive || !Emissive->Expression
            || Emissive->Expression->GetName() != TEXT("MaterialExpressionMultiply_27")
            || Emissive->OutputIndex != 0 || Emissive->Mask || !Fine || !InverseScale
            || Fine->GetName() != TEXT("MaterialExpressionMaterialFunctionCall_14")
            || FineOffset->A.OutputIndex != 2 || FineOffset->A.Mask || FineOffset->ConstB != 0.3f)
            return Refuse(TEXT("Audited emission/fine field/physical frame changed"));
        const FExpressionInput OriginalEmission = *Emissive;
        TMap<int32, FExpressionInput> OriginalProperties;
        for (int32 P = 0; P < MP_MAX; ++P)
            if (const FExpressionInput* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P)))
                OriginalProperties.Add(P, *Input);
        float Brightness = -1.0f;
        FLinearColor Emission;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 1.5f
            || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), Emission)
            || !Emission.Equals(FLinearColor(0.42f, 0.018f, 0.001f, 1.0f), 1.e-6f))
            return Refuse(TEXT("Saved emission authority changed"));
        const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
            / TEXT("SharedLavaThermalCoverageBackup_20260927"));
        if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup already exists"));
        if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
        for (const FExpected& E : Expected)
        {
            const FString Source = Filename(E.Name), Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
            if (Hash(Source) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Source, false, false) != COPY_OK
                || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup mismatch before edit"));
        }

        // Use the native 400 m triplanar red field BEFORE its positive offset.
        // The material previously emitted across the entire liquid sheet.
        // Keep the native hot colour response; only cooled coverage suppresses it.
        // Differentiate rendered positions natively before Custom float demotion.
        auto* Position = Core.Add<UMaterialExpressionWorldPosition>(Master);
        Position->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
        auto* DX = Core.Add<UMaterialExpressionDDX>(Master);
        auto* DY = Core.Add<UMaterialExpressionDDY>(Master);
        DX->Value.Connect(0, Position); DY->Value.Connect(0, Position);
        auto* PhysicalDX = Core.Add<UMaterialExpressionMultiply>(Master);
        auto* PhysicalDY = Core.Add<UMaterialExpressionMultiply>(Master);
        PhysicalDX->A.Connect(0, DX); PhysicalDY->A.Connect(0, DY);
        for (auto* D : {PhysicalDX, PhysicalDY})
        {
            D->B.Connect(0, InverseScale);
            D->B.Mask = D->B.MaskR = 1;
            D->B.MaskG = D->B.MaskB = D->B.MaskA = 0;
        }
        auto* Coverage = Core.Add<UMaterialExpressionCustom>(Master);
        Coverage->Description = TEXT("APS filtered native lava thermal coverage v1");
        Coverage->OutputType = CMOT_Float1;
        Coverage->Inputs.Empty();
        auto AddInput = [Coverage](const TCHAR* Name, UMaterialExpression* E, int32 Output = 0)
        {
            FCustomInput In; In.InputName = Name; In.Input.Connect(Output, E);
            Coverage->Inputs.Add(In);
        };
        AddInput(TEXT("Fine"), Fine, 2);
        AddInput(TEXT("PhysicalDX"), PhysicalDX); AddInput(TEXT("PhysicalDY"), PhysicalDY);
        Coverage->Code = TEXT(
            "float footprint = max(length(PhysicalDX), length(PhysicalDY));\n"
            "float width = max(0.07, 0.5 * (abs(ddx(Fine.r)) + abs(ddy(Fine.r))));\n"
            "float resolved = smoothstep(0.35 - width, 0.35 + width, Fine.r);\n"
            "// Subpixel crust integrates to a provisional coverage, not all cold/hot.\n"
            "// Continuous physical footprint, independent of preview actor scale.\n"
            "float unresolved = smoothstep(3200.0, 12800.0, footprint);\n"
            "return lerp(resolved, 0.35, unresolved);");
        auto* Strength = Core.Add<UMaterialExpressionScalarParameter>(Master);
        Strength->ParameterName = TEXT("APS_LavaThermalCoverage");
        Strength->DefaultValue = 0.0f; // opt-in pending same-shore rendered comparison
        Strength->SliderMin = 0.0f; Strength->SliderMax = 1.0f;
        Strength->Group = TEXT("APS Lava Surface"); Strength->UpdateParameterGuid(true, true);
        auto* StrengthBound = Core.Add<UMaterialExpressionClamp>(Master);
        StrengthBound->Input.Connect(0, Strength);
        StrengthBound->MinDefault = 0.0f; StrengthBound->MaxDefault = 1.0f;
        auto* Gate = Core.Add<UMaterialExpressionLinearInterpolate>(Master);
        Gate->ConstA = 1.0f; Gate->B.Connect(0, Coverage); Gate->Alpha.Connect(0, StrengthBound);
        auto* Radiance = Core.Add<UMaterialExpressionMultiply>(Master);
        Radiance->A = OriginalEmission; Radiance->B.Connect(0, Gate);
        Emissive->Connect(0, Radiance);
        if (FCore::Expressions(Master).Num() != 129 || !SameInput(Radiance->A, OriginalEmission))
            return Refuse(TEXT("Unexpected thermal graph rewrite"));
        for (const auto& Pair : OriginalProperties)
        {
            if (Pair.Key == MP_EmissiveColor) continue;
            const FExpressionInput* Now = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
            if (!Now || !SameInput(*Now, Pair.Value))
                return Refuse(TEXT("A non-emissive material output changed"));
        }
        Master->PostEditChange();
        UMaterialInterface* Materials[] = {Master, Native, APS};
        for (UMaterialInterface* Material : Materials)
            Material->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        for (UMaterialInterface* Material : Materials)
        {
            FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
                || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
                return Refuse(TEXT("Shader/LocalVF not ready; no asset saved"));
        }
        float ActualReflectance = -1.0f, ActualBrightness = -1.0f, ActualThermal = -1.0f;
        FLinearColor ActualEmission;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), ActualThermal)
            || ActualThermal != 0.0f
            || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaCrustReflectance")), ActualReflectance)
            || ActualReflectance != 0.08f
            || !APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), ActualBrightness)
            || ActualBrightness != Brightness
            || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), ActualEmission)
            || ActualEmission != Emission) return Refuse(TEXT("Inherited parameter contract changed"));
        for (const FExpected& E : Expected)
            if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
        UPackage* Package = Master->GetOutermost();
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Master, *Filename(Expected[0].Name), Args))
            return Refuse(TEXT("Master save failed; retain verified backup"));
        for (int32 I = 1; I < UE_ARRAY_COUNT(Expected); ++I)
            if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1)
                return Refuse(TEXT("Protected MIC/function changed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaThermalCoverage] SAVED masterOnly=1 thermalDefault=0 nativeColourBrightnessUnchanged=1 coordinatesTexturesMaskUnchanged=1 newTextureSamples=0 backup=%s renderedAcceptancePending=1"), *Backup);
        return true;
    }
}
#endif
