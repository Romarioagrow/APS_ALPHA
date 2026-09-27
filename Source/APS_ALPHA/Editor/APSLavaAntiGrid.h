#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSquareRoot.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureSample.h"

// Private 20-km stochastic WAT, preserving the native close sample and mip-tail LOD.
// The Custom node emits coordinates/weights ONLY; all reads remain native Color
// TextureSamples, with explicit gradients of the original unwrapped LWC UVs.
// No palette, brightness, coverage, histogram, contrast or debug-gate edits.
namespace APSLavaAntiGrid
{
    inline constexpr const TCHAR* PrivateName = TEXT("MF_APS_LavaWAT20kmAntiGrid_v1");
    inline bool Apply(APSSharedTerrainMaterialBuilder::FBuild& Core, UMaterial* Master)
    {
        using FCore = APSSharedTerrainMaterialBuilder::FBuild;
        const auto Fail = [&Core](const TCHAR* Why) { Core.Error = Why; return false; };
        const FString OwnedRoot = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid");
        if (!Master || Core.DestinationRoot != OwnedRoot
            || Master->GetPathName() != OwnedRoot + TEXT("/M_APS_SharedLava.M_APS_SharedLava"))
            return Fail(TEXT("Anti-grid requires the exact APS shared-liquid owner"));
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
        auto* Copy = Cast<UMaterialFunction>(Core.Duplicate(Source, PrivateName));
        if (!Copy) return false;
        const TArray<UMaterialExpression*> Original = FCore::Expressions(Copy);
        const TCHAR* Samples[] = {TEXT("MaterialExpressionTextureSample_0"), TEXT("MaterialExpressionTextureSample_3"), TEXT("MaterialExpressionTextureSample_5")};
        const TCHAR* Projections[] = {TEXT("MaterialExpressionComponentMask_2"), TEXT("MaterialExpressionComponentMask_4"), TEXT("MaterialExpressionComponentMask_0")};
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
                || UV->GetName() != Projections[I] || UV->R != (I != 0) || UV->G != (I != 1) || UV->B != (I != 2) || UV->A
                || Sample->Coordinates.OutputIndex != 0 || Sample->Coordinates.Mask || UV->Input.OutputIndex != 0 || UV->Input.Mask
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
            // The native derivatives must precede ANY frac/skew/hash operation.
            // Native Floor/Frac preserve LWC before Custom inputs demote to float.
            auto* U = Core.Add<UMaterialExpressionComponentMask>(Copy);
            auto* V = Core.Add<UMaterialExpressionComponentMask>(Copy);
            U->Input = Sample->Coordinates; U->R = true; U->G = U->B = U->A = false;
            V->Input = Sample->Coordinates; V->G = true; V->R = V->B = V->A = false;
            auto* SkewV = Core.Add<UMaterialExpressionMultiply>(Copy);
            SkewV->A.Connect(0, V); SkewV->ConstB = 0.5773502691896258f;
            auto* SkewU = Core.Add<UMaterialExpressionSubtract>(Copy);
            SkewU->A.Connect(0, U); SkewU->B.Connect(0, SkewV);
            auto* SkewY = Core.Add<UMaterialExpressionMultiply>(Copy);
            SkewY->A.Connect(0, V); SkewY->ConstB = 1.1547005383792515f;
            auto* Lattice = Core.Add<UMaterialExpressionAppendVector>(Copy);
            Lattice->A.Connect(0, SkewU); Lattice->B.Connect(0, SkewY);
            auto* TexturePhase = Core.Add<UMaterialExpressionFrac>(Copy);
            TexturePhase->Input = Sample->Coordinates;
            auto* CellFraction = Core.Add<UMaterialExpressionFrac>(Copy);
            auto* CellFloor = Core.Add<UMaterialExpressionFloor>(Copy);
            CellFraction->Input.Connect(0, Lattice); CellFloor->Input.Connect(0, Lattice);
            // Bounded positive modulo AFTER native floor avoids float-demoting
            // large world cell IDs. 65536 is exact; neighbours wrap identically.
            auto* CellDivisor = Core.Add<UMaterialExpressionMultiply>(Copy);
            CellDivisor->A.Connect(0, CellFloor); CellDivisor->ConstB = 1.0f / 65536.0f;
            auto* CellWrapped = Core.Add<UMaterialExpressionFrac>(Copy);
            CellWrapped->Input.Connect(0, CellDivisor);
            auto* CellID = Core.Add<UMaterialExpressionMultiply>(Copy);
            CellID->A.Connect(0, CellWrapped); CellID->ConstB = 65536.0f;
            auto* Coordinates = Core.Add<UMaterialExpressionCustom>(Copy);
            Coordinates->Description = TEXT("Stochastic triangle coordinates and barycentrics; no texture fetch");
            Coordinates->OutputType = CMOT_Float2;
            Coordinates->Inputs.Reset();
            FCustomInput PhaseInput; PhaseInput.InputName = TEXT("TexturePhase"); PhaseInput.Input.Connect(0, TexturePhase);
            FCustomInput FractionInput; FractionInput.InputName = TEXT("CellFraction"); FractionInput.Input.Connect(0, CellFraction);
            FCustomInput IDInput; IDInput.InputName = TEXT("CellID"); IDInput.Input.Connect(0, CellID);
            Coordinates->Inputs.Add(PhaseInput); Coordinates->Inputs.Add(FractionInput); Coordinates->Inputs.Add(IDInput);
            Coordinates->AdditionalOutputs.Reset();
            FCustomOutput UV1; UV1.OutputName = TEXT("UV1"); UV1.OutputType = CMOT_Float2;
            FCustomOutput UV2; UV2.OutputName = TEXT("UV2"); UV2.OutputType = CMOT_Float2;
            FCustomOutput Weights; Weights.OutputName = TEXT("Weights"); Weights.OutputType = CMOT_Float3;
            Coordinates->AdditionalOutputs.Add(UV1); Coordinates->AdditionalOutputs.Add(UV2); Coordinates->AdditionalOutputs.Add(Weights);
            // A vertex ID, not a triangle ID, determines the translation. At
            // every shared edge the absent third weight is zero and the two
            // remaining samples/weights match, including cell wrap boundaries.
            Coordinates->Code = FString::Printf(TEXT(
                "uint2 cell = ((uint2)floor(CellID + 0.5)) & 65535u;\n"
                "float2 f = saturate(CellFraction);\n"
                "uint2 v0, v1, v2;\n"
                "if (f.x + f.y <= 1.0) {\n"
                " v0 = cell; v1 = cell + uint2(1,0); v2 = cell + uint2(0,1);\n"
                " Weights = float3(1.0-f.x-f.y, f.x, f.y);\n"
                "} else {\n"
                " v0 = cell + uint2(1,1); v1 = cell + uint2(0,1); v2 = cell + uint2(1,0);\n"
                " Weights = float3(f.x+f.y-1.0, 1.0-f.x, 1.0-f.y);\n"
                "}\n"
                "v0 &= 65535u; v1 &= 65535u; v2 &= 65535u;\n"
                "uint3 h = uint3(v0.x ^ (v0.y << 16), v1.x ^ (v1.y << 16), v2.x ^ (v2.y << 16)) ^ %uu;\n"
                "h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;\n"
                "float3 ox = float3(h & 65535u) * (1.0/65536.0);\n"
                "float3 oy = float3(h >> 16) * (1.0/65536.0);\n"
                "UV1 = frac(TexturePhase + float2(ox.y, oy.y));\n"
                "UV2 = frac(TexturePhase + float2(ox.z, oy.z));\n"
                "return frac(TexturePhase + float2(ox.x, oy.x));\n"), 0x2c9277b5u + uint32(I) * 0x9e3779b9u);
            // RebuildOutputs is not ENGINE_API on UE5.4's MinimalAPI class.
            // Populate its public output array using the same audited layout.
            Coordinates->Outputs.Reset();
            Coordinates->Outputs.Add(FExpressionOutput(TEXT("return")));
            for (const FCustomOutput& Output : Coordinates->AdditionalOutputs)
                Coordinates->Outputs.Add(FExpressionOutput(Output.OutputName));
            // Leave the native sample, UVs and implicit mip path untouched.
            // The anti-grid branch needs three independent translated taps.
            UMaterialExpressionTextureSample* Taps[3] = {};
            for (int32 T = 0; T < 3; ++T)
            {
                Taps[T] = Core.Add<UMaterialExpressionTextureSample>(Copy);
                Taps[T]->Texture = Sample->Texture;
                Taps[T]->TextureObject = Sample->TextureObject;
                Taps[T]->SamplerType = Sample->SamplerType;
                Taps[T]->SamplerSource = Sample->SamplerSource;
                Taps[T]->AutomaticViewMipBias = Sample->AutomaticViewMipBias;
                Taps[T]->AutomaticViewMipBiasValue = Sample->AutomaticViewMipBiasValue;
                Taps[T]->Coordinates.Connect(T, Coordinates);
                Taps[T]->MipValueMode = TMVM_Derivative;
                Taps[T]->CoordinatesDX.Connect(0, DX); Taps[T]->CoordinatesDY.Connect(0, DY);
            }
            UMaterialExpressionMultiply* WeightedRGB[3];
            UMaterialExpressionMultiply* WeightedAlpha[3];
            for (int32 T = 0; T < 3; ++T)
            {
                auto* Weight = Core.Add<UMaterialExpressionComponentMask>(Copy);
                Weight->Input.Connect(3, Coordinates);
                Weight->R = T == 0; Weight->G = T == 1; Weight->B = T == 2; Weight->A = false;
                WeightedRGB[T] = Core.Add<UMaterialExpressionMultiply>(Copy);
                WeightedAlpha[T] = Core.Add<UMaterialExpressionMultiply>(Copy);
                WeightedRGB[T]->A.Connect(0, Taps[T]); WeightedRGB[T]->B.Connect(0, Weight);
                WeightedAlpha[T]->A.Connect(4, Taps[T]); WeightedAlpha[T]->B.Connect(0, Weight);
            }
            auto* RGB01 = Core.Add<UMaterialExpressionAdd>(Copy);
            auto* Alpha01 = Core.Add<UMaterialExpressionAdd>(Copy);
            auto* StochasticRGB = Core.Add<UMaterialExpressionAdd>(Copy);
            auto* StochasticAlpha = Core.Add<UMaterialExpressionAdd>(Copy);
            RGB01->A.Connect(0, WeightedRGB[0]); RGB01->B.Connect(0, WeightedRGB[1]);
            Alpha01->A.Connect(0, WeightedAlpha[0]); Alpha01->B.Connect(0, WeightedAlpha[1]);
            StochasticRGB->A.Connect(0, RGB01); StochasticRGB->B.Connect(0, WeightedRGB[2]);
            StochasticAlpha->A.Connect(0, Alpha01); StochasticAlpha->B.Connect(0, WeightedAlpha[2]);
            // SAME bandwidth guardband and SAME texture mip tail as bandwidthLOD.
            auto* XX = Core.Add<UMaterialExpressionDotProduct>(Copy);
            auto* YY = Core.Add<UMaterialExpressionDotProduct>(Copy);
            XX->A.Connect(0, DX); XX->B.Connect(0, DX);
            YY->A.Connect(0, DY); YY->B.Connect(0, DY);
            auto* Sum = Core.Add<UMaterialExpressionAdd>(Copy);
            Sum->A.Connect(0, XX); Sum->B.Connect(0, YY);
            auto* Footprint = Core.Add<UMaterialExpressionSquareRoot>(Copy);
            Footprint->Input.Connect(0, Sum);
            // Physical projection footprint, shared by orbit and gameplay.
            // Keep exact native colour/alpha below 20 m per pixel; smoothly
            // reach stochastic at 80 m per pixel for this 20-km texture only.
            // This is a visual fade, not a dynamic-branch performance claim.
            auto* AntiGridFade = Core.Add<UMaterialExpressionSmoothStep>(Copy);
            AntiGridFade->ConstMin = 0.001f; AntiGridFade->ConstMax = 0.004f;
            AntiGridFade->Value.Connect(0, Footprint);
            auto* NativeToStochasticRGB = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            auto* NativeToStochasticAlpha = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            NativeToStochasticRGB->A.Connect(0, Sample);
            NativeToStochasticRGB->B.Connect(0, StochasticRGB);
            NativeToStochasticRGB->Alpha.Connect(0, AntiGridFade);
            NativeToStochasticAlpha->A.Connect(4, Sample);
            NativeToStochasticAlpha->B.Connect(0, StochasticAlpha);
            NativeToStochasticAlpha->Alpha.Connect(0, AntiGridFade);
            auto* Fade = Core.Add<UMaterialExpressionSmoothStep>(Copy);
            Fade->ConstMin = 0.125f; Fade->ConstMax = 0.5f;
            Fade->Value.Connect(0, Footprint);
            auto* Detail = Core.Add<UMaterialExpressionOneMinus>(Copy);
            Detail->Input.Connect(0, Fade);
            auto* FilterRGB = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            auto* FilterAlpha = Core.Add<UMaterialExpressionLinearInterpolate>(Copy);
            FilterRGB->A.Connect(0, Mean); FilterRGB->B.Connect(0, NativeToStochasticRGB); FilterRGB->Alpha.Connect(0, Detail);
            FilterAlpha->A.Connect(4, Mean); FilterAlpha->B.Connect(0, NativeToStochasticAlpha); FilterAlpha->Alpha.Connect(0, Detail);
            // Lerp's only output is already typed RGB/scalar; do not keep the
            // TextureSample alpha output4/maskA on the new scalar Lerp output.
            for (FExpressionInput* Input : RGB) Input->Connect(0, FilterRGB);
            for (FExpressionInput* Input : Alpha) Input->Connect(0, FilterAlpha);
            ChangedEdges += RGB.Num() + Alpha.Num();
            if (Sample->MipValueMode != TMVM_None || Sample->Coordinates.Expression != UV
                || Sample->CoordinatesDX.Expression || Sample->CoordinatesDY.Expression
                || DX->Value.Expression != UV || DY->Value.Expression != UV
                || FilterRGB->B.Expression != NativeToStochasticRGB || FilterAlpha->B.Expression != NativeToStochasticAlpha
                || Coordinates->Inputs.Num() != 3 || Coordinates->AdditionalOutputs.Num() != 3)
                return Fail(TEXT("Stochastic coordinates/gradients/blend graph changed"));
            for (int32 T = 0; T < 3; ++T)
                if (Taps[T]->SamplerType != SAMPLERTYPE_Color || Taps[T]->SamplerSource != Mean->SamplerSource
                    || Taps[T]->TextureObject.Expression != Mean->TextureObject.Expression
                    || Taps[T]->Coordinates.Expression != Coordinates || Taps[T]->Coordinates.OutputIndex != T
                    || Taps[T]->CoordinatesDX.Expression != DX || Taps[T]->CoordinatesDY.Expression != DY)
                    return Fail(TEXT("Native colour sampler or explicit gradient mismatch"));
        }
        int32 FinalSamples = 0, CoordinateKernels = 0;
        for (UMaterialExpression* E : FCore::Expressions(Copy))
        {
            if (Cast<UMaterialExpressionTextureSample>(E)) ++FinalSamples;
            if (auto* C = Cast<UMaterialExpressionCustom>(E))
                if (!Original.Contains(C)) ++CoordinateKernels;
        }
        if (!Mean || ChangedEdges != 9 || FCore::Expressions(Copy).Num() != Original.Num() + 128
            || FinalSamples != 13 || CoordinateKernels != 3)
            return Fail(TEXT("Unexpected stochastic bandwidth graph inventory"));
        Copy->PostEditChange();
        if (!Core.ReconnectFunctionById(Calls[2], Source, Copy)) return false;
        if (Calls[0]->MaterialFunction != Source || Calls[1]->MaterialFunction != Source || Calls[2]->MaterialFunction != Copy)
            return Fail(TEXT("20km-only private function selection failed"));
        UE_LOG(LogTemp, Display, TEXT("[APS.LavaAntiGrid] function=%s projections=3 filteredEdges=9 newNodes=128 affectedCalls=1 untouched400m5431m=1 nativeClose=1 stochasticFade=0.001..0.004 guardband=0.125..0.5 meanTexture=%s mipLevel=16 sRGB=1 nativeColorSampling=1 explicitUnwrappedGradients=1 stochasticTaps=9 addedDetailTaps=9 histogramCompensation=0"),
            *Copy->GetPathName(), *AuditedTexture->GetPathName());
        return true;
    }
}
#endif
