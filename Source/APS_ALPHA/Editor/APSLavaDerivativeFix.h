#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionTextureSample.h"

// Reusable clean fix, independent of diagnostic gates. Only a newly owned
// master's private WAT copy is changed; vendor/previous shared assets are read-only.
namespace APSLavaDerivativeFix
{
    inline bool Apply(APSSharedTerrainMaterialBuilder::FBuild& Core, UMaterial* Master)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        auto Fail = [&Core](const TCHAR* Why) { Core.Error = Why; return false; };
        const TCHAR* ExpectedWAT = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c");
        TArray<UMaterialExpressionMaterialFunctionCall*> Calls;
        UMaterialFunction* Source = nullptr;
        for (const TCHAR* Name : {TEXT("MaterialExpressionMaterialFunctionCall_14"), TEXT("MaterialExpressionMaterialFunctionCall_16"), TEXT("MaterialExpressionMaterialFunctionCall_17")})
        {
            auto* Call = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, Name);
            auto* Function = Call ? Cast<UMaterialFunction>(Call->MaterialFunction) : nullptr;
            if (!Function || Function->GetPathName() != ExpectedWAT || (Source && Source != Function))
                return Fail(TEXT("Derivative fix requires all three audited shared WAT calls"));
            Source = Function; Calls.Add(Call);
        }
        if (!Source->bAllExpressionsLoadedCorrectly) return Fail(TEXT("Source WAT expressions are incomplete"));
        auto* Copy = Cast<UMaterialFunction>(Core.Duplicate(Source, TEXT("MF_APS_LavaWorldAlignedTextureGrad")));
        if (!Copy) return false;
        const TArray<UMaterialExpression*> Original = FCore::Expressions(Copy);
        int32 Samples = 0;
        for (UMaterialExpression* E : Original)
        {
            auto* Sample = Cast<UMaterialExpressionTextureSample>(E);
            if (!Sample) continue;
            const FString Name = Sample->GetName();
            if ((Name != TEXT("MaterialExpressionTextureSample_0") && Name != TEXT("MaterialExpressionTextureSample_3") && Name != TEXT("MaterialExpressionTextureSample_5"))
                || Sample->MipValueMode != TMVM_None || Sample->SamplerSource != SSM_Wrap_WorldGroupSettings
                || !Sample->Coordinates.Expression || !Cast<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression)
                || !Sample->TextureObject.Expression || Sample->TextureObject.Expression->GetName() != TEXT("MaterialExpressionFunctionInput_0")
                || Sample->CoordinatesDX.Expression || Sample->CoordinatesDY.Expression || Sample->MipValue.Expression)
                return Fail(TEXT("WAT sampling topology/settings differ from audited implicit wrapped UVs"));
            auto* UV = CastChecked<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression);
            if (!UV->Input.Expression || UV->Input.Expression->GetName() != TEXT("MaterialExpressionDivide_0"))
                return Fail(TEXT("WAT UVs are not unwrapped physical texture-size coordinates"));
            auto* DX = Core.Add<UMaterialExpressionDDX>(Copy);
            auto* DY = Core.Add<UMaterialExpressionDDY>(Copy);
            DX->Value = Sample->Coordinates;
            DY->Value = Sample->Coordinates;
            // UE5.4 DDX/Y compiles LWC inputs to WSDdx/yDemote, BEFORE address
            // wrapping. Implicit sampling otherwise sees WSApplyAddressMode's
            // discontinuity (HLSLMaterialTranslator.cpp 6914, explicit path off).
            Sample->MipValueMode = TMVM_Derivative;
            Sample->CoordinatesDX.Connect(0, DX);
            Sample->CoordinatesDY.Connect(0, DY);
            ++Samples;
        }
        if (Samples != 3 || FCore::Expressions(Copy).Num() != Original.Num() + 6)
            return Fail(TEXT("Expected exactly three texture samples and six derivative nodes"));
        Copy->PostEditChange();
        for (auto* Call : Calls)
            if (!Core.ReconnectFunctionById(Call, Source, Copy)) return false;
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaDerivativeFix] privateFunction=%s samples=3 newDerivativeNodes=6 calls=3 textureScaleBlendPaletteUnchanged=1"), *Copy->GetPathName());
        return true;
    }
}
#endif
