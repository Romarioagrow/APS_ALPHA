#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "UObject/UObjectHash.h"

// Offline, isolated experiment: two new assets, existing functions/textures read-only.
// No promotion, displacement, palette replacement or shore-mask change.
namespace APSUnifiedLavaDetailBuilder
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
inline constexpr const TCHAR* Destination = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/UnifiedLavaDetail20261002V1");
inline constexpr const TCHAR* SourceRoot = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava");
inline constexpr const TCHAR* MasterName = TEXT("M_APS_UnifiedLavaSurface");
inline constexpr const TCHAR* InstanceName = TEXT("MI_APS_UnifiedLavaSurface");
inline constexpr const TCHAR* FineName = TEXT("MF_APS_UnifiedLava_7f4dbc7d");

inline FString ObjectPath(const TCHAR* Root, const TCHAR* Name)
{
    return FString(Root) / Name + TEXT(".") + Name;
}
inline FString Filename(const FString& Package)
{
    return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension()));
}
inline FString Hash(const FString& File)
{
    TArray<uint8> Bytes;
    return FFileHelper::LoadFileToArray(Bytes, *File) ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString().ToUpper() : FString();
}
inline bool SameInput(const FExpressionInput& A, const FExpressionInput& B)
{
    return A.Expression == B.Expression && A.OutputIndex == B.OutputIndex && A.Mask == B.Mask
        && A.MaskR == B.MaskR && A.MaskG == B.MaskG && A.MaskB == B.MaskB && A.MaskA == B.MaskA;
}

inline bool Build(IAssetTools& Tools)
{
    FCore B(Tools, Destination);
    const auto Refuse = [&B](const FString& Why) {
        UE_LOG(LogTemp, Error, TEXT("[APS.UnifiedLavaDetail] Refused: %s %s"), *Why, *B.Error); return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline commandlet only"));
    for (const TCHAR* Name : {MasterName, InstanceName})
        if (FPackageName::DoesPackageExist(FString(Destination) / Name)
            || FindObject<UObject>(nullptr, *ObjectPath(Destination, Name))) return Refuse(TEXT("Candidate output already exists"));
    const TCHAR* Names[] = {MasterName, InstanceName, FineName};
    const TCHAR* Expected[] = {TEXT("62948CAB512650337405B653F0E380AC5F09C84F"),
        TEXT("67C94832663DBFBC6199401243C4531663E1CB92"), TEXT("F6D0326F0C4C45F548A583858D4BC1F391FB4D6C")};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Names); ++I)
        if (Hash(Filename(FString(SourceRoot) / Names[I])) != Expected[I]) return Refuse(TEXT("Audited source asset drift"));
    TArray<FString> ProtectedFiles;
    IFileManager::Get().FindFilesRecursive(ProtectedFiles, *FPaths::GetPath(Filename(FString(SourceRoot) / MasterName)), TEXT("*.uasset"), true, false);
    if (ProtectedFiles.Num() != 23) return Refuse(TEXT("Expected 23 protected unified assets"));
    TMap<FString, FString> ProtectedHashes;
    for (const FString& File : ProtectedFiles)
    {
        const FString Value = Hash(File);
        if (Value.IsEmpty()) return Refuse(TEXT("Cannot hash protected source"));
        ProtectedHashes.Add(File, Value);
    }
    const auto SourcesUnchanged = [&]() {
        for (const auto& Pair : ProtectedHashes) if (Hash(Pair.Key) != Pair.Value) return false;
        return true;
    };
    auto* Source = LoadObject<UMaterial>(nullptr, *ObjectPath(SourceRoot, MasterName));
    auto* SourceMIC = LoadObject<UMaterialInstanceConstant>(nullptr, *ObjectPath(SourceRoot, InstanceName));
    auto* Function = LoadObject<UMaterialFunction>(nullptr, *ObjectPath(SourceRoot, FineName));
    if (!Source || !SourceMIC || !Function || SourceMIC->Parent.Get() != Source
        || Source->GetOutermost()->IsDirty() || SourceMIC->GetOutermost()->IsDirty() || Function->GetOutermost()->IsDirty()
        || Source->GetBlendMode() != BLEND_Opaque || SourceMIC->GetBlendMode() != BLEND_Opaque
        || Source->bTangentSpaceNormal || Source->bUseMaterialAttributes
        || !Source->GetShadingModels().HasShadingModel(MSM_DefaultLit)) return Refuse(TEXT("Unexpected saved unified source contract"));

    const EMaterialProperty Properties[] = {MP_BaseColor, MP_EmissiveColor, MP_Roughness, MP_Metallic, MP_Specular, MP_AmbientOcclusion, MP_Normal};
    const auto Validate = [&](UMaterial* Material, TMap<EMaterialProperty, UMaterialExpressionLinearInterpolate*>& Blends) {
        UMaterialExpression* SharedAlpha = nullptr;
        for (EMaterialProperty P : Properties)
        {
            auto* Out = Material->GetExpressionInputForProperty(P);
            auto* Blend = Out ? Cast<UMaterialExpressionLinearInterpolate>(Out->Expression) : nullptr;
            if (!Blend || Out->OutputIndex != 0 || Out->Mask || !Blend->A.Expression || !Blend->B.Expression
                || !Blend->Alpha.Expression || Blend->Alpha.OutputIndex != 0 || Blend->Alpha.Mask) return false;
            if (P == MP_BaseColor || P == MP_EmissiveColor) { if (static_cast<FColorMaterialInput*>(Out)->UseConstant) return false; }
            else if (P == MP_Normal) { if (static_cast<FVectorMaterialInput*>(Out)->UseConstant) return false; }
            else if (static_cast<FScalarMaterialInput*>(Out)->UseConstant) return false;
            if (SharedAlpha && Blend->Alpha.Expression != SharedAlpha) return false;
            SharedAlpha = Blend->Alpha.Expression; Blends.Add(P, Blend);
        }
        auto* Weight = Cast<UMaterialExpressionOneMinus>(SharedAlpha);
        auto* Shore = Weight ? Cast<UMaterialExpressionSmoothStep>(Weight->Input.Expression) : nullptr;
        auto* Height = Shore ? Cast<UMaterialExpressionDivide>(Shore->Value.Expression) : nullptr;
        auto* Tolerance = Height ? Cast<UMaterialExpressionScalarParameter>(Height->B.Expression) : nullptr;
        if (!Shore || Shore->ConstMin != 1.0f || Shore->ConstMax != 3.0f || !Height
            || !Cast<UMaterialExpressionVertexInterpolator>(Height->A.Expression)
            || !Tolerance || Tolerance->ParameterName != TEXT("APS_UnifiedRadiusToleranceCm")) return false;
        for (EMaterialProperty P : {MP_WorldPositionOffset, MP_PixelDepthOffset, MP_OpacityMask})
            if (Material->GetExpressionInputForProperty(P)->Expression) return false;
        return true;
    };
    TMap<EMaterialProperty, UMaterialExpressionLinearInterpolate*> OriginalBlends;
    if (!Validate(Source, OriginalBlends)) return Refuse(TEXT("Expected seven original lerps and shared vertex-height shore alpha"));
    APSSharedTerrainNormalContinuity::TTransform<FCore> Reader(B);
    const auto FunctionGraph = Reader.Graph(Function);
    int32 PreciseKernels = 0, DerivativeSamples = 0;
    UMaterialExpressionTextureSample* SampleContract = nullptr;
    for (auto* E : FunctionGraph)
    {
        if (E->GetClass()->GetName() == TEXT("MaterialExpressionPixelNormalWS"))
            return Refuse(TEXT("Fine field cannot depend on the pixel normal it will drive"));
        if (auto* C = Cast<UMaterialExpressionCustom>(E); C
            && C->Description == TEXT("APS lava precise physical UV: compensated transform/division, wrap after unwrapped gradients, no warp"))
        {
            if (!C->Code.Contains(TEXT("DFSubtract(WSToDF(LWCRaw), WSToDF(LWCCenter))"))
                || !C->Code.Contains(TEXT("UVdx = DFDdxDemote(U); UVdy = DFDdyDemote(U);"))
                || !C->Code.Contains(TEXT("return DFFracDemote(U);"))) return Refuse(TEXT("Precision kernel changed"));
            ++PreciseKernels;
        }
        if (auto* T = Cast<UMaterialExpressionTextureSample>(E))
        {
            if (T->MipValueMode != TMVM_Derivative || T->SamplerType != SAMPLERTYPE_Color
                || T->SamplerSource != SSM_Wrap_WorldGroupSettings) return Refuse(TEXT("Precise sampling contract changed"));
            ++DerivativeSamples; SampleContract = T;
        }
    }
    if (!B.Error.IsEmpty() || PreciseKernels != 1 || DerivativeSamples != 3) return Refuse(TEXT("Expected one compensated kernel and three derivative samples"));
    auto* Master = Cast<UMaterial>(B.Duplicate(Source, MasterName));
    auto* MIC = Cast<UMaterialInstanceConstant>(B.Duplicate(SourceMIC, InstanceName));
    if (!Master || !MIC) return Refuse(TEXT("Candidate duplication failed"));
    TMap<EMaterialProperty, UMaterialExpressionLinearInterpolate*> Blends;
    if (!Validate(Master, Blends)) return Refuse(TEXT("Duplicate changed unified topology"));
    TMap<EMaterialProperty, FExpressionInput> OutputsBefore, RockBefore, AlphaBefore, LavaBefore;
    for (int32 P = 0; P < MP_MAX; ++P)
        if (auto* Input = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) OutputsBefore.Add(static_cast<EMaterialProperty>(P), *Input);
    for (EMaterialProperty P : Properties)
    {
        RockBefore.Add(P, Blends[P]->A); AlphaBefore.Add(P, Blends[P]->Alpha); LavaBefore.Add(P, Blends[P]->B);
    }
    UMaterialExpressionMaterialFunctionCall* Fine = nullptr;
    UMaterialExpressionDoubleVectorParameter* InverseScale = nullptr;
    for (auto* E : Reader.Graph(Master))
    {
        Reader.Register(Master, E);
        if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E); P && P->ParameterName == TEXT("APS_SharedInverseScale")) InverseScale = P;
        auto* Call = Cast<UMaterialExpressionMaterialFunctionCall>(E);
        if (!Call) continue;
        // Duplication drops transient function pin pointers. Rehydrate only the
        // copied call, against the SAME read-only function, preserving every wire.
        auto* SameFunction = Cast<UMaterialFunction>(Call->MaterialFunction);
        if (!SameFunction || !B.ReconnectFunctionById(Call, SameFunction, SameFunction)) return Refuse(TEXT("Copied function pin binding changed"));
        if (SameFunction != Function) continue;
        const auto* Size = Call->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureSize"); });
        auto* Product = Size ? Cast<UMaterialExpressionMultiply>(Size->Input.Expression) : nullptr;
        auto* A = Product ? Cast<UMaterialExpressionConstant>(Product->A.Expression) : nullptr;
        auto* C = Product ? Cast<UMaterialExpressionConstant>(Product->B.Expression) : nullptr;
        if (A && C && A->R == 400.0f && C->R == 100.0f)
        {
            if (Fine) return Refuse(TEXT("Ambiguous original fine texture call"));
            Fine = Call;
        }
    }
    if (!B.Error.IsEmpty() || !Fine || !InverseScale || !Cast<UMaterialExpressionVertexNormalWS>(Blends[MP_Normal]->B.Expression))
        return Refuse(TEXT("Expected original 400m fine call, inverse scale and world geometric lava normal"));
    const auto* TexturePin = Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("TextureObject"); });
    const auto* PositionPin = Fine->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P) { return P.Input.InputName == TEXT("WorldPosition"); });
    auto* Texture = TexturePin ? Cast<UMaterialExpressionTextureObject>(TexturePin->Input.Expression) : nullptr;
    if (!Texture || !Texture->Texture || !PositionPin || PositionPin->Input.Expression) return Refuse(TEXT("Fine texture/default physical-position contract changed"));
    const auto Scalar = [&](const TCHAR* Name, float Value, float Minimum, float Maximum) {
        auto* P = B.Add<UMaterialExpressionScalarParameter>(Master); P->ParameterName = Name; P->DefaultValue = Value;
        P->SliderMin = Minimum; P->SliderMax = Maximum; P->Group = TEXT("APS Unified Lava Detail Candidate"); P->UpdateParameterGuid(true, true); return P;
    };
    auto* Strength = Scalar(TEXT("APS_UnifiedDetailStrength"), 1.0f, 0.0f, 1.0f);
    auto* Period = Scalar(TEXT("APS_UnifiedDetailPeriodCm"), 200.0f, 100.0f, 1000.0f);
    auto* BoundPeriod = B.Add<UMaterialExpressionClamp>(Master);
    BoundPeriod->Input.Connect(0, Period); BoundPeriod->MinDefault = 100.0f; BoundPeriod->MaxDefault = 1000.0f;
    auto* Micro = B.Add<UMaterialExpressionMaterialFunctionCall>(Master);
    if (!Micro->SetMaterialFunction(Function)) return Refuse(TEXT("Cannot create candidate-only fine call"));
    for (auto& Pin : Micro->FunctionInputs)
    {
        const auto* Old = Fine->FunctionInputs.FindByPredicate([&](const FFunctionExpressionInput& P) { return P.ExpressionInputId == Pin.ExpressionInputId; });
        if (!Old) return Refuse(TEXT("New fine call input GUID mismatch"));
        Pin.Input = Old->Input;
        if (Pin.Input.InputName == TEXT("TextureSize")) Pin.Input.Connect(0, BoundPeriod);
    }
    const int32 XYZ = Micro->FunctionOutputs.IndexOfByPredicate([](const FFunctionExpressionOutput& O) { return O.Output.OutputName == TEXT("XYZ Texture"); });
    if (XYZ == INDEX_NONE) return Refuse(TEXT("Missing triplanar RGB output"));
    auto* Mean = B.Add<UMaterialExpressionTextureSample>(Master);
    Mean->Texture = Texture->Texture; Mean->TextureObject = TexturePin->Input;
    Mean->SamplerType = SampleContract->SamplerType; Mean->SamplerSource = SampleContract->SamplerSource;
    Mean->MipValueMode = TMVM_MipLevel; Mean->ConstMipValue = 16; Mean->AutomaticViewMipBias = false;
    auto* Center = B.Add<UMaterialExpressionConstant2Vector>(Master); Center->R = Center->G = 0.5f; Mean->Coordinates.Connect(0, Center);

    auto* Raw = B.Add<UMaterialExpressionWorldPosition>(Master); Raw->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
    auto* Camera = B.Add<UMaterialExpressionCameraPositionWS>(Master);
    auto* CameraDelta = B.Add<UMaterialExpressionSubtract>(Master); CameraDelta->A.Connect(0, Raw); CameraDelta->B.Connect(0, Camera);
    auto* DX = B.Add<UMaterialExpressionDDX>(Master); auto* DY = B.Add<UMaterialExpressionDDY>(Master);
    DX->Value.Connect(0, Raw); DY->Value.Connect(0, Raw);
    const auto Physical = [&](UMaterialExpression* E) {
        auto* M = B.Add<UMaterialExpressionMultiply>(Master); M->A.Connect(0, E); M->B.Connect(0, InverseScale);
        M->B.Mask = M->B.MaskR = 1; M->B.MaskG = M->B.MaskB = M->B.MaskA = 0; return M;
    };
    auto* PhysicalDX = Physical(DX); auto* PhysicalDY = Physical(DY); auto* PhysicalCamera = Physical(CameraDelta);
    const auto Input = [](UMaterialExpressionCustom* C, const TCHAR* Name, const FExpressionInput& Value) {
        FCustomInput I; I.InputName = Name; I.Input = Value; C->Inputs.Add(I);
    };
    const auto Edge = [](UMaterialExpression* E, int32 Output = 0) { FExpressionInput I; I.Connect(Output, E); return I; };
    auto* Signal = B.Add<UMaterialExpressionCustom>(Master); Signal->OutputType = CMOT_Float1;
    Signal->Description = TEXT("APS isolated lava detail: centred fine RGB, physical footprint and 100..500m fade"); Signal->Inputs.Empty();
    Input(Signal, TEXT("Fine"), Edge(Micro, XYZ));
    FExpressionInput MeanRGB = Edge(Mean); MeanRGB.Mask = MeanRGB.MaskR = MeanRGB.MaskG = MeanRGB.MaskB = 1;
    Input(Signal, TEXT("Mean"), MeanRGB); Input(Signal, TEXT("DX"), Edge(PhysicalDX)); Input(Signal, TEXT("DY"), Edge(PhysicalDY));
    Input(Signal, TEXT("CameraDelta"), Edge(PhysicalCamera)); Input(Signal, TEXT("Period"), Edge(BoundPeriod)); Input(Signal, TEXT("Strength"), Edge(Strength));
    FCustomOutput Gate; Gate.OutputName = TEXT("Gate"); Gate.OutputType = CMOT_Float1; Signal->AdditionalOutputs.Add(Gate);
    Signal->Outputs.Reset(); Signal->Outputs.Add(FExpressionOutput(TEXT("Signal"))); Signal->Outputs.Add(FExpressionOutput(TEXT("Gate")));
    Signal->Code = TEXT("float footprint=max(length(DX),length(DY))/max(Period,100.0);\n")
        TEXT("Gate=saturate(Strength)*(1-smoothstep(0.125,0.5,footprint))*(1-smoothstep(10000.0,50000.0,length(CameraDelta)));\n")
        TEXT("return clamp(2.0*dot(Fine.rgb-Mean.rgb,float3(0.2126,0.7152,0.0722)),-1.0,1.0);\n");
    for (EMaterialProperty P : {MP_BaseColor, MP_EmissiveColor, MP_Roughness})
    {
        auto* Detail = B.Add<UMaterialExpressionCustom>(Master);
        Detail->Description = TEXT("APS isolated lava detail: preserve original branch exactly at zero gate");
        Detail->OutputType = P == MP_Roughness ? CMOT_Float1 : CMOT_Float3; Detail->Inputs.Empty();
        Input(Detail, TEXT("Original"), LavaBefore[P]); Input(Detail, TEXT("Signal"), Edge(Signal)); Input(Detail, TEXT("Gate"), Edge(Signal, 1));
        // Achromatic, mean-centred modulation; inherited hue/brightness parameters stay untouched.
        Detail->Code = P == MP_Roughness ? TEXT("if(Gate<=0) return Original; return lerp(Original,saturate(Original+0.12*Signal),Gate);")
            : P == MP_EmissiveColor ? TEXT("if(Gate<=0) return Original; return Original*(1.0+0.08*Gate*Signal);")
            : TEXT("if(Gate<=0) return Original; return Original*(1.0+0.16*Gate*Signal);");
        Blends[P]->B = Edge(Detail);
    }
    auto* Bump = B.Add<UMaterialExpressionCustom>(Master); Bump->OutputType = CMOT_Float3; Bump->Inputs.Empty();
    Bump->Description = TEXT("APS isolated lava world surface-gradient bump: 1cm height, bounded slope, no displacement");
    Input(Bump, TEXT("Original"), LavaBefore[MP_Normal]); Input(Bump, TEXT("Signal"), Edge(Signal));
    Input(Bump, TEXT("Gate"), Edge(Signal, 1)); Input(Bump, TEXT("DX"), Edge(PhysicalDX)); Input(Bump, TEXT("DY"), Edge(PhysicalDY));
    // Derivatives are evaluated uniformly BEFORE the fade branch. Fade the slope,
    // not the height before ddx/ddy, to avoid adding a distance-fade ridge.
    Bump->Code = TEXT("float2 dh=float2(ddx(Signal),ddy(Signal))*1.0;\n")
        TEXT("if(Gate<=0) return Original;\n")
        TEXT("float3 N=normalize(Original); float scale=max(max(length(DX),length(DY)),1.e-6);\n")
        TEXT("float3 X=DX/scale,Y=DY/scale,R1=cross(Y,N),R2=cross(N,X); float det=dot(X,R1);\n")
        TEXT("float safeDet=(det<0?-1.0:1.0)*max(abs(det),1.e-6);\n")
        TEXT("float3 gradient=((dh.x/scale)*R1+(dh.y/scale)*R2)/safeDet;\n")
        TEXT("gradient*=smoothstep(1.e-5,1.e-4,abs(det)); gradient/=max(1.0,length(gradient)/0.35);\n")
        TEXT("return normalize(N-Gate*gradient);\n");
    Blends[MP_Normal]->B = Edge(Bump); // Already world space; never convert to tangent space.
    for (const auto& Pair : OutputsBefore)
        if (!SameInput(*Master->GetExpressionInputForProperty(Pair.Key), Pair.Value)) return Refuse(TEXT("Material output/geometry edge changed"));
    for (EMaterialProperty P : Properties)
        if (!SameInput(Blends[P]->A, RockBefore[P]) || !SameInput(Blends[P]->Alpha, AlphaBefore[P])
            || ((P == MP_Metallic || P == MP_Specular || P == MP_AmbientOcclusion) && !SameInput(Blends[P]->B, LavaBefore[P])))
            return Refuse(TEXT("Protected rock, shore or non-detail lava edge changed"));
    for (auto* E : Reader.Graph(Master)) Reader.Register(Master, E);
    if (!B.Error.IsEmpty()) return Refuse(TEXT("Candidate graph closure"));
    // DuplicateAsset can leave multiple editor-data subobjects; synchronize only
    // our material copy, never the read-only source functions.
    const FMaterialExpressionCollection Collection = Master->GetExpressionCollection();
    TArray<UObject*> Owned; GetObjectsWithOuter(Master, Owned, false);
    for (auto* O : Owned) if (auto* Data = Cast<UMaterialEditorOnlyData>(O)) Data->ExpressionCollection = Collection;
    MIC->SetParentEditorOnly(Master, false); MIC->CopyMaterialUniformParametersEditorOnly(SourceMIC, true);
    Master->PostEditChange(); MIC->PostEditChange();
    if (B.Outputs.Num() != 2 || MIC->GetBlendMode() != BLEND_Opaque || Master->bTangentSpaceNormal)
        return Refuse(TEXT("Candidate output/normal contract changed"));
    for (auto* M : {static_cast<UMaterialInterface*>(Master), static_cast<UMaterialInterface*>(MIC)}) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M : {static_cast<UMaterialInterface*>(Master), static_cast<UMaterialInterface*>(MIC)})
    {
        auto* R = M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        float Value = -1;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num() || !Map
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
            || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_UnifiedDetailStrength")), Value) || Value != 1.0f)
            return Refuse(TEXT("Candidate complete LocalVF/default strength required before saving"));
    }
    if (!SourcesUnchanged()) return Refuse(TEXT("Protected source changed before save"));
    for (auto* Output : B.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(Destination) + TEXT("/"))) return Refuse(TEXT("Candidate escaped isolated directory"));
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Output, *Filename(Package->GetName()), Args)) return Refuse(TEXT("Candidate save failed"));
    }
    if (!SourcesUnchanged()) return Refuse(TEXT("Protected source changed after save"));
    UE_LOG(LogTemp, Display, TEXT("[APS.UnifiedLavaDetail] SAVED outputs=2 protected=23 periodCm=200 strength=1 strength0=originalBranch footprintFade=0.125..0.5 distanceFadeCm=10000..50000 bumpHeightCm=1 maxSlope=0.35 addedTextureSamples=4 sourceFunctionsReadOnly=1 rawAbsoluteDF=1 nearPrecisionRenderedUnverified=1 NOT_visual_acceptance=1"));
    return true;
}
}
#endif
