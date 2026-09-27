#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSquareRoot.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureSample.h"

// Clean same-shader minification LOD. This helper changes only a new owner's
// private 20-km WAT function. It contains no debug gates, palette or emission edits.
namespace APSLavaBandwidthLOD
{
    inline bool Apply(APSSharedTerrainMaterialBuilder::FBuild& Core, UMaterial* Master)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        const auto Fail = [&Core](const TCHAR* Why) { Core.Error = Why; return false; };
        const TCHAR* ExpectedWAT = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MF_APS_WorldAlignedTexture_a83aa78c.MF_APS_WorldAlignedTexture_a83aa78c");
        const TCHAR* ExpectedTexture = TEXT("/Engine/EngineMaterials/T_Default_MacroVariation.T_Default_MacroVariation");
        auto* FineOffset = FindObject<UMaterialExpressionAdd>(Master, TEXT("MaterialExpressionAdd_26"));
        auto* CoarseProduct = FindObject<UMaterialExpressionMultiply>(Master, TEXT("MaterialExpressionMultiply_104"));
        if (!FineOffset || !CoarseProduct || FineOffset->ConstB != 0.3f)
            return Fail(TEXT("Audited WAT scale product changed"));
        FExpressionInput* Edges[] = {&FineOffset->A, &CoarseProduct->A, &CoarseProduct->B};
        const TCHAR* Names[] = {TEXT("MaterialExpressionMaterialFunctionCall_14"), TEXT("MaterialExpressionMaterialFunctionCall_16"), TEXT("MaterialExpressionMaterialFunctionCall_17")};
        const float Meters[] = {400.0f, 5431.0f, 20000.0f};
        TArray<UMaterialExpressionMaterialFunctionCall*> Calls;
        UMaterialFunction* Source = nullptr;
        UTexture2D* AuditedTexture = nullptr;
        for (int32 I = 0; I < 3; ++I)
        {
            auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Edges[I]->Expression);
            auto* Function = Call ? Cast<UMaterialFunction>(Call->MaterialFunction) : nullptr;
            if (!Call || Call->GetName() != Names[I] || Edges[I]->OutputIndex != 2 || Edges[I]->Mask
                || !Function || Function->GetPathName() != ExpectedWAT || (Source && Source != Function))
                return Fail(TEXT("Exact three shared WAT XYZ edges changed"));
            const FFunctionExpressionInput* SizePin = Call->FunctionInputs.FindByPredicate(
                [](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureSize"); });
            const FFunctionExpressionInput* TexturePin = Call->FunctionInputs.FindByPredicate(
                [](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureObject"); });
            auto* Size = SizePin ? Cast<UMaterialExpressionMultiply>(SizePin->Input.Expression) : nullptr;
            auto* Scale = Size ? Cast<UMaterialExpressionConstant>(Size->A.Expression) : nullptr;
            auto* Cm = Size ? Cast<UMaterialExpressionConstant>(Size->B.Expression) : nullptr;
            auto* Object = TexturePin ? Cast<UMaterialExpressionTextureObject>(TexturePin->Input.Expression) : nullptr;
            auto* Texture = Object ? Cast<UTexture2D>(Object->Texture) : nullptr;
            if (!Scale || Scale->R != Meters[I] || !Cm || Cm->R != 100.0f
                || !Texture || Texture->GetPathName() != ExpectedTexture || !Texture->SRGB
                || Texture->VirtualTextureStreaming || (AuditedTexture && AuditedTexture != Texture))
                return Fail(TEXT("Native physical scales or exact sRGB nonvirtual macro texture changed"));
            AuditedTexture = Texture; Source = Function; Calls.Add(Call);
        }
        // The tail is read via the *same texture sampler*, not a guessed colour.
        // Both audited chains (platform 2048/12 and streamed RHI 512/10) end at 1x1.
        const FTexturePlatformData* Data = AuditedTexture->GetPlatformData();
        if (!Data || Data->Mips.Num() < 1 || Data->Mips.Num() > 16
            || Data->Mips.Last().SizeX != 1 || Data->Mips.Last().SizeY != 1
            || !Source->bAllExpressionsLoadedCorrectly)
            return Fail(TEXT("Complete colour texture 1x1 mip tail/source function required"));
        auto* Copy = Cast<UMaterialFunction>(Core.Duplicate(Source, TEXT("MF_APS_LavaWAT20kmBandwidth")));
        if (!Copy) return false;
        const TArray<UMaterialExpression*> Original = FCore::Expressions(Copy);
        const TCHAR* Samples[] = {TEXT("MaterialExpressionTextureSample_0"), TEXT("MaterialExpressionTextureSample_3"), TEXT("MaterialExpressionTextureSample_5")};
        const TCHAR* Appends[] = {TEXT("MaterialExpressionAppendVector_9"), TEXT("MaterialExpressionAppendVector_11"), TEXT("MaterialExpressionAppendVector_13")};
        const TCHAR* Switches[] = {TEXT("MaterialExpressionStaticSwitch_0"), TEXT("MaterialExpressionStaticSwitch_2"), TEXT("MaterialExpressionStaticSwitch_4")};
        UMaterialExpressionTextureSample* Mean = nullptr;
        int32 ChangedEdges = 0;
        int32 SampleCount = 0;
        for (UMaterialExpression* E : Original) if (Cast<UMaterialExpressionTextureSample>(E)) ++SampleCount;
        if (SampleCount != 3) return Fail(TEXT("Expected exactly three native WAT projections"));
        for (int32 I = 0; I < 3; ++I)
        {
            auto* Sample = FindObject<UMaterialExpressionTextureSample>(Copy, Samples[I]);
            auto* UV = Sample ? Cast<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression) : nullptr;
            if (!Sample || !Original.Contains(Sample) || !UV || !UV->Input.Expression
                || UV->Input.Expression->GetName() != TEXT("MaterialExpressionDivide_0")
                || Sample->MipValueMode != TMVM_None || Sample->SamplerSource != SSM_Wrap_WorldGroupSettings
                || Sample->SamplerType != SAMPLERTYPE_Color || !Sample->TextureObject.Expression
                || Sample->TextureObject.Expression->GetName() != TEXT("MaterialExpressionFunctionInput_0")
                || Sample->CoordinatesDX.Expression || Sample->CoordinatesDY.Expression || Sample->MipValue.Expression)
                return Fail(TEXT("Original implicit colour sampling/unwrapped projection UVs changed"));
            // Capture only original edges BEFORE creating nodes; never rewrite
            // our native/mean inputs into themselves. Preserve RGB and alpha.
            TArray<FExpressionInput*> RGB, Alpha;
            for (UMaterialExpression* E : Original)
                for (FExpressionInput* Input : E->GetInputsView())
                {
                    if (!Input || Input->Expression != Sample) continue;
                    if (Input->OutputIndex == 0 && Input->Mask && Input->MaskR && Input->MaskG && Input->MaskB && !Input->MaskA
                        && (E->GetName() == Appends[I] || E->GetName() == Switches[I])) RGB.Add(Input);
                    else if (Input->OutputIndex == 4 && Input->Mask && !Input->MaskR && !Input->MaskG && !Input->MaskB && Input->MaskA
                        && E->GetName() == Appends[I]) Alpha.Add(Input);
                    else return Fail(TEXT("Unexpected native sample consumer/channel"));
                }
            if (RGB.Num() != 2 || Alpha.Num() != 1) return Fail(TEXT("Expected two RGB edges plus original alpha edge per projection"));
            if (!Mean)
            {
                auto* Center = Core.Add<UMaterialExpressionConstant2Vector>(Copy);
                Center->R = Center->G = 0.5f;
                Mean = Core.Add<UMaterialExpressionTextureSample>(Copy);
                Mean->Texture = Sample->Texture;
                Mean->TextureObject = Sample->TextureObject;
                Mean->SamplerType = Sample->SamplerType;
                Mean->SamplerSource = Sample->SamplerSource;
                Mean->Coordinates.Connect(0, Center);
                Mean->MipValueMode = TMVM_MipLevel;
                // D3D SampleLevel clamps beyond the SRV's last mip. Level16
                // reaches the same 1x1 tail regardless of streamed top-mip count.
                // Normal colour sampling performs sRGB->linear exactly once.
                Mean->ConstMipValue = 16;
                Mean->AutomaticViewMipBias = false;
            }
            else if (Mean->SamplerType != Sample->SamplerType || Mean->SamplerSource != Sample->SamplerSource
                || Mean->TextureObject.Expression != Sample->TextureObject.Expression)
                return Fail(TEXT("Projection colour samplers differ"));
            auto* DX = Core.Add<UMaterialExpressionDDX>(Copy);
            auto* DY = Core.Add<UMaterialExpressionDDY>(Copy);
            DX->Value = Sample->Coordinates; DY->Value = Sample->Coordinates;
            // LWC derivatives demote only after differencing the unwrapped UVs.
            auto* XX = Core.Add<UMaterialExpressionDotProduct>(Copy);
            auto* YY = Core.Add<UMaterialExpressionDotProduct>(Copy);
            XX->A.Connect(0, DX); XX->B.Connect(0, DX);
            YY->A.Connect(0, DY); YY->B.Connect(0, DY);
            auto* Sum = Core.Add<UMaterialExpressionAdd>(Copy);
            Sum->A.Connect(0, XX); Sum->B.Connect(0, YY);
            auto* Footprint = Core.Add<UMaterialExpressionSquareRoot>(Copy);
            Footprint->Input.Connect(0, Sum);
            auto* Fade = Core.Add<UMaterialExpressionSmoothStep>(Copy);
            Fade->ConstMin = 0.125f; Fade->ConstMax = 0.5f;
            Fade->Value.Connect(0, Footprint);
            auto* Detail = Core.Add<UMaterialExpressionOneMinus>(Copy);
            Detail->Input.Connect(0, Fade);
            auto* FilterRGB = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            auto* FilterAlpha = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            FilterRGB->A.Connect(0, Mean); FilterRGB->B.Connect(0, Sample); FilterRGB->Alpha.Connect(0, Detail);
            FilterAlpha->A.Connect(4, Mean); FilterAlpha->B.Connect(4, Sample); FilterAlpha->Alpha.Connect(0, Detail);
            // Lerp's only output is already typed RGB/scalar; do not keep the
            // TextureSample alpha output4/maskA on the new scalar Lerp output.
            for (FExpressionInput* Input : RGB) Input->Connect(0, FilterRGB);
            for (FExpressionInput* Input : Alpha) Input->Connect(0, FilterAlpha);
            ChangedEdges += RGB.Num() + Alpha.Num();
            if (Sample->MipValueMode != TMVM_None || FilterRGB->B.Expression != Sample || FilterAlpha->B.Expression != Sample)
                return Fail(TEXT("Original sample changed or filter self-cycle"));
        }
        if (!Mean || ChangedEdges != 9 || FCore::Expressions(Copy).Num() != Original.Num() + 32)
            return Fail(TEXT("Unexpected bandwidth graph inventory"));
        Copy->PostEditChange();
        if (!Core.ReconnectFunctionById(Calls[2], Source, Copy)) return false;
        if (Calls[0]->MaterialFunction != Source || Calls[1]->MaterialFunction != Source || Calls[2]->MaterialFunction != Copy)
            return Fail(TEXT("20km-only private function selection failed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaBandwidthLOD] function=%s projections=3 filteredEdges=9 newNodes=32 affectedCalls=1 untouched400m5431m=1 guardband=0.125..0.5 meanTexture=%s mipLevel=16 sRGB=1 nativeSamplingUnchanged=1"),
            *Copy->GetPathName(), *AuditedTexture->GetPathName());
        return true;
    }
}
#endif
