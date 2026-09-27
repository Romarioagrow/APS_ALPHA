#pragma once
#if WITH_EDITOR
#include "APSLavaCrustReflectanceUpdate.h"
#include "APSLavaFinePeriodUpdate.h"
#include "APSLavaFineStochasticUpdate.h"
#include "APSLavaCrustNormalUpdate.h"
#include "APSLavaCrustFieldUpdate.h"
#include "APSLavaRadianceIntegrationUpdate.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"

// Pre-integrate coverage BEFORE mip filtering. A project-owned copy keeps the
// native RGB source bytes; its linear alpha stores the derived thermal field.
// One existing triplanar read exports RGBA. No observer-specific shader/model.
namespace APSLavaThermalMipUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
using APSLavaCrustReflectanceUpdate::SameInput;
inline constexpr const TCHAR* TextureName = TEXT("T_APS_LavaThermal400m_RGBA_v1");
struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
inline constexpr FExpected Expected[] = {
    {TEXT("M_APS_SharedLava"), TEXT("2DB3405BECA5CE8B178CE728F731AB76F24AEFF3")},
    {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
    {TEXT("MI_APS_SharedLava"), TEXT("9E5D5EF9183A800DA87D88F42B4CE47ECFDE6342")},
    {TEXT("MF_APS_LavaWAT20kmAntiGrid_v1"), TEXT("27640676C83642BF2A50C432F1D605C2025FA306")},
    {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("92D0F30659A78568DC2510E4D3CC25FD17150C83")}
};

inline bool TailAlpha(UTexture2D* Texture, double& Alpha, FString& Format)
{
    auto* Data = Texture ? Texture->GetPlatformData() : nullptr;
    if (!Data || Data->Mips.IsEmpty()) return false;
    auto& Tail = Data->Mips.Last();
    Format = GPixelFormats[Data->PixelFormat].Name;
    if (Tail.SizeX != 1 || Tail.SizeY != 1) return false;
    const int64 Bytes = Tail.BulkData.GetBulkDataSize();
    const uint8* P = static_cast<const uint8*>(Tail.BulkData.LockReadOnly());
    bool Ok = false;
    if (P && Data->PixelFormat == PF_DXT5 && Bytes >= 16)
    {
        const int32 I = P[2] & 7; // first texel in the BC3 alpha block
        double Table[8] = {double(P[0]), double(P[1])};
        if (P[0] > P[1])
            for (int32 J = 2; J < 8; ++J) Table[J] = ((8-J)*P[0] + (J-1)*P[1]) / 7.0;
        else
        {
            for (int32 J = 2; J < 6; ++J) Table[J] = ((6-J)*P[0] + (J-1)*P[1]) / 5.0;
            Table[6] = 0; Table[7] = 255;
        }
        Alpha = Table[I] / 255.0; Ok = true;
    }
    else if (P && Data->PixelFormat == PF_B8G8R8A8 && Bytes >= 4)
    {
        Alpha = P[3] / 255.0; Ok = true;
    }
    Tail.BulkData.Unlock();
    return Ok;
}

inline bool Run(IAssetTools& Tools)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("InspectSharedLavaRadiance")))
        return APSLavaRadianceIntegrationUpdate::Inspect();
    if (FParse::Param(FCommandLine::Get(), TEXT("ParameterizeSharedLavaRadianceIntegration")))
        return APSLavaRadianceIntegrationUpdate::Run(Tools);
    if (FParse::Param(FCommandLine::Get(), TEXT("ParameterizeSharedLavaCrustField")))
        return APSLavaCrustFieldUpdate::Run(Tools);
    if (FParse::Param(FCommandLine::Get(), TEXT("ParameterizeSharedLavaCrustNormal")))
        return APSLavaCrustNormalUpdate::Run(Tools);
    if (FParse::Param(FCommandLine::Get(), TEXT("ParameterizeSharedLavaFineStochastic")))
        return APSLavaFineStochasticUpdate::Run(Tools);
    if (FParse::Param(FCommandLine::Get(), TEXT("ParameterizeSharedLavaFinePeriod")))
        return APSLavaFinePeriodUpdate::Run(Tools);
    FCore Core(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse = [&Core](const FString& Why)
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaThermalMips] Refused: %s %s"), *Why, *Core.Error);
        return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    for (const FExpected& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(FString(TEXT("Saved asset drift: ")) + E.Name);
    const FString EngineSource = FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir()
        / TEXT("EngineMaterials/T_Default_MacroVariation.uasset"));
    const FString EngineHash = TEXT("133AA153CA022BC6E3BEE7F6747FBA4A11E5108A");
    if (Hash(EngineSource) != EngineHash) return Refuse(TEXT("Engine source texture drift"));
    if (FPackageName::DoesPackageExist(FString(APSSharedLavaMaterialBuilder::Destination) / TextureName))
        return Refuse(TEXT("Private texture already exists; inspect prior attempt"));
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
        || Native->Parent.Get() != Master || APS->Parent.Get() != Native
        || Master->GetBlendMode() != BLEND_Masked || FCore::Expressions(Master).Num() != 119)
        return Refuse(TEXT("Unexpected saved graph/chain/blend"));
    auto* FineOffset = FindObject<UMaterialExpressionAdd>(Master, TEXT("MaterialExpressionAdd_26"));
    auto* Fine = FineOffset ? Cast<UMaterialExpressionMaterialFunctionCall>(FineOffset->A.Expression) : nullptr;
    auto* Emission = Master->GetExpressionInputForProperty(MP_EmissiveColor);
    if (!Fine || Fine->GetName() != TEXT("MaterialExpressionMaterialFunctionCall_14")
        || FineOffset->ConstB != .3f || FineOffset->A.OutputIndex != 2 || FineOffset->A.Mask
        || !Fine->MaterialFunction || Fine->MaterialFunction->GetName() != TEXT("MF_APS_WorldAlignedTexture_a83aa78c")
        || !Emission || !Emission->Expression || Emission->OutputIndex != 0 || Emission->Mask
        || Emission->Expression->GetName() != TEXT("MaterialExpressionMultiply_27"))
        return Refuse(TEXT("Audited fine/emission roots changed"));
    int32 Consumers = 0;
    for (auto* E : FCore::Expressions(Master))
        for (auto* In : E->GetInputsView())
            if (In && In->Expression == Fine)
            {
                if (In != &FineOffset->A) return Refuse(TEXT("Additional fine read consumer"));
                ++Consumers;
            }
    if (Consumers != 1) return Refuse(TEXT("Fine consumer inventory changed"));
    auto* TexPin = Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P)
        { return P.Input.InputName == TEXT("TextureObject"); });
    auto* ExportPin = Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P)
        { return P.Input.InputName == TEXT("Export Float 4"); });
    auto* TexObject = TexPin ? Cast<UMaterialExpressionTextureObject>(TexPin->Input.Expression) : nullptr;
    auto* Source = TexObject ? Cast<UTexture2D>(TexObject->Texture) : nullptr;
    if (!Source || Source->GetPathName() != TEXT("/Engine/EngineMaterials/T_Default_MacroVariation.T_Default_MacroVariation")
        || !ExportPin || ExportPin->Input.Expression || Source->VirtualTextureStreaming || !Source->SRGB
        || Source->CompressionSettings != TC_Default || Source->Source.GetFormat() != TSF_BGRA8
        || Source->Source.GetNumLayers() != 1 || Source->Source.GetNumSlices() != 1
        || Source->bDoScaleMipsForAlphaCoverage || Source->bPreserveBorder
        || (Source->MipGenSettings != TMGS_FromTextureGroup && Source->MipGenSettings != TMGS_SimpleAverage))
        return Refuse(FString::Printf(TEXT("Unsupported texture/pin state format=%d"), Source ? int32(Source->Source.GetFormat()) : -1));
    const int64 W = Source->Source.GetSizeX(), H = Source->Source.GetSizeY();
    TArray64<uint8> Bytes;
    if (W < 1 || H < 1 || W > 4096 || H > 4096 || !FMath::IsPowerOfTwo(uint32(W)) || !FMath::IsPowerOfTwo(uint32(H))
        || !Source->Source.GetMipData(Bytes, 0) || Bytes.Num() != W*H*4)
        return Refuse(TEXT("Unsupported/bounded texture source layout"));
    const TArray64<uint8> Original = Bytes;
    auto* Pixels = reinterpret_cast<FColor*>(Bytes.GetData());
    double Mean = 0;
    for (int64 I = 0; I < W*H; ++I)
    {
        // FLinearColor(FColor) decodes sRGB RGB exactly once; alpha stays linear.
        const float R = FLinearColor(Pixels[I]).R;
        const float T = FMath::Clamp((R - .55f) / .20f, 0.0f, 1.0f);
        Pixels[I].A = uint8(FMath::Clamp(FMath::RoundToInt(255.0f*T*T*(3.0f-2.0f*T)), 0, 255));
        Mean += Pixels[I].A / 255.0;
    }
    Mean /= W*H;
    for (int64 I = 0; I < Bytes.Num(); I += 4)
        if (Bytes[I] != Original[I] || Bytes[I+1] != Original[I+1] || Bytes[I+2] != Original[I+2])
            return Refuse(TEXT("Source RGB mutation"));
    float Brightness = -1; FLinearColor Colour;
    if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Brightness")), Brightness) || Brightness != 1.5f
        || !APS->GetVectorParameterValue(FMaterialParameterInfo(TEXT("EmissiveColor")), Colour)
        || !Colour.Equals(FLinearColor(.42f, .018f, .001f, 1), 1.e-6f))
        return Refuse(TEXT("Saved palette/brightness authority changed"));
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaThermalMipBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup exists"));
    if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
    for (const auto& E : Expected)
    {
        const FString Dest = Backup / (FString(E.Name) + TEXT(".uasset"));
        if (Hash(Filename(E.Name)) != E.SHA1 || IFileManager::Get().Copy(*Dest, *Filename(E.Name), false, false) != COPY_OK
            || Hash(Dest) != E.SHA1) return Refuse(TEXT("Backup mismatch"));
    }
    auto* Packed = Cast<UTexture2D>(Core.Duplicate(Source, TextureName));
    if (!Packed) return Refuse(TEXT("Texture duplication failed"));
    Packed->PreEditChange(nullptr);
    Packed->Source.Init(int32(W), int32(H), 1, 1, TSF_BGRA8, Bytes.GetData());
    Packed->SRGB = true;
    Packed->CompressionNoAlpha = false; Packed->CompressionForceAlpha = true;
    // Average alpha as fractional thermal area, NOT thresholded cutout coverage.
    Packed->MipGenSettings = TMGS_SimpleAverage;
    Packed->bDoScaleMipsForAlphaCoverage = false;
    Packed->PostEditChange();
    FAssetCompilingManager::Get().FinishAllCompilation();
    double Tail = -1; FString Format;
    if (!TailAlpha(Packed, Tail, Format) || FMath::Abs(Tail-Mean) > .025)
        return Refuse(FString::Printf(TEXT("Filtered coverage mean invalid: source=%.6f tail=%.6f format=%s"), Mean, Tail, *Format));
    TArray64<uint8> Check;
    if (!Packed->Source.GetMipData(Check, 0) || Check != Bytes) return Refuse(TEXT("Packed source readback mismatch"));
    TMap<int32, FExpressionInput> Properties;
    for (int32 P = 0; P < MP_MAX; ++P)
        if (const auto* In = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P)))
            Properties.Add(P, *In);
    const FExpressionInput OriginalEmission = *Emission;
    auto* Object = Core.Add<UMaterialExpressionTextureObject>(Master);
    Object->Texture = Packed; Object->SamplerType = SAMPLERTYPE_Color;
    TexPin->Input.Connect(0, Object);
    auto* RGBA = Core.Add<UMaterialExpressionStaticBool>(Master);
    RGBA->Value = true; ExportPin->Input.Connect(0, RGBA);
    // Keep native RGB algebra at exactly three components after exporting RGBA.
    FineOffset->A.Mask = FineOffset->A.MaskR = FineOffset->A.MaskG = FineOffset->A.MaskB = 1;
    FineOffset->A.MaskA = 0;
    auto* Coverage = Core.Add<UMaterialExpressionComponentMask>(Master);
    Coverage->Input.Connect(2, Fine);
    Coverage->R = Coverage->G = Coverage->B = false; Coverage->A = true;
    Coverage->Desc = TEXT("Pre-integrated lava coverage: linear filtered alpha, no post-mip threshold");
    auto* Strength = Core.Add<UMaterialExpressionScalarParameter>(Master);
    Strength->ParameterName = TEXT("APS_LavaThermalCoverage"); Strength->DefaultValue = 0;
    Strength->SliderMin = 0; Strength->SliderMax = 1;
    Strength->Group = TEXT("APS Lava Surface"); Strength->UpdateParameterGuid(true, true);
    auto* Bound = Core.Add<UMaterialExpressionClamp>(Master);
    Bound->Input.Connect(0, Strength); Bound->MinDefault = 0; Bound->MaxDefault = 1;
    auto* Gate = Core.Add<UMaterialExpressionLinearInterpolate>(Master);
    Gate->ConstA = 1; Gate->B.Connect(0, Coverage); Gate->Alpha.Connect(0, Bound);
    auto* Radiance = Core.Add<UMaterialExpressionMultiply>(Master);
    Radiance->A = OriginalEmission; Radiance->B.Connect(0, Gate);
    Emission->Connect(0, Radiance);
    if (FCore::Expressions(Master).Num() != 126) return Refuse(TEXT("Unexpected node count"));
    for (const auto& Pair : Properties)
    {
        if (Pair.Key == MP_EmissiveColor) continue;
        const auto* Now = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
        if (!Now || !SameInput(*Now, Pair.Value)) return Refuse(TEXT("Non-emissive output edge changed"));
    }
    Master->PostEditChange();
    UMaterialInterface* Materials[] = {Master, Native, APS};
    for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M : Materials)
    {
        FMaterialResource* R = M->GetMaterialResource(GMaxRHIFeatureLevel);
        const FMaterialShaderMap* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            return Refuse(TEXT("Complete error-free LocalVF required before save"));
    }
    float Effective = -1;
    if (!APS->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaThermalCoverage")), Effective)
        || Effective != 0) return Refuse(TEXT("Trial must remain disabled pending rendered approval"));
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
    if (Hash(EngineSource) != EngineHash) return Refuse(TEXT("Concurrent engine texture edit"));
    for (UObject* Output : {static_cast<UObject*>(Packed), static_cast<UObject*>(Master)})
    {
        UPackage* Package = Output->GetOutermost(); Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Output,
            *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args))
            return Refuse(TEXT("Save failed; retain backups/partial output for inspection"));
    }
    for (int32 I = 1; I < UE_ARRAY_COUNT(Expected); ++I)
        if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1) return Refuse(TEXT("Protected MIC/function changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaThermalMips] SAVED disabledDefault=1 packed=%s sourceRGBExact=1 sourceSize=%lldx%lld sourceMean=%.6f compressedTail=%.6f pixelFormat=%s nativeSamplesReused=3 newTextureFetches=0 mipAlphaAverage=1 arbitraryDistanceMean=0 originalPalette=1 backup=%s renderedAcceptancePending=1"),
        *Packed->GetPathName(), W, H, Mean, Tail, *Format, *Backup);
    return true;
}
}
#endif
