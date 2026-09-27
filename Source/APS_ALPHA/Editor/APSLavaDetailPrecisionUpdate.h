#pragma once
#if WITH_EDITOR
#include "APSLavaCrustReflectanceUpdate.h"
#include "APSSharedTerrainNormalContinuity.h"

// Offline, exact-baseline update of the two native fine lava projections only.
// No detiling/warp, palette, texture, scale, noise or thermal-coverage changes.
namespace APSLavaDetailPrecisionUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
struct FExpected { const TCHAR* Name; const TCHAR* SHA1; };
inline constexpr FExpected Expected[] = {
    {TEXT("M_APS_SharedLava"), TEXT("2DB3405BECA5CE8B178CE728F731AB76F24AEFF3")},
    {TEXT("MI_APS_SharedLavaNative"), TEXT("1F3F7FE8F5562477149A54E4F6B42EF31D155E51")},
    {TEXT("MI_APS_SharedLava"), TEXT("9E5D5EF9183A800DA87D88F42B4CE47ECFDE6342")},
    {TEXT("MF_APS_LavaWAT20kmAntiGrid_v1"), TEXT("27640676C83642BF2A50C432F1D605C2025FA306")},
    {TEXT("MF_APS_WorldAlignedTexture_a83aa78c"), TEXT("92D0F30659A78568DC2510E4D3CC25FD17150C83")}
};
inline bool Run(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse = [&B](const FString& Why) {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaDetailPrecision] Refused: %s %s"), *Why, *B.Error);
        return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    TArray<UObject*> Objects;
    for (const auto& E : Expected)
    {
        const FString File = Filename(E.Name);
        if (Hash(File) != E.SHA1) return Refuse(FString(TEXT("Baseline drift: ")) + E.Name);
        if (IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("uexp")))
            || IFileManager::Get().FileExists(*FPaths::ChangeExtension(File, TEXT("ubulk"))))
            return Refuse(TEXT("Unexpected split package"));
        const FString Path = FString(APSSharedLavaMaterialBuilder::Destination) / E.Name + TEXT(".") + E.Name;
        UObject* O = LoadObject<UObject>(nullptr, *Path);
        if (!O || O->GetOutermost()->IsDirty() || O->GetOutermost()->bIsCookedForEditor)
            return Refuse(FString(TEXT("Missing/dirty/cooked asset: ")) + E.Name);
        Objects.Add(O);
    }
    auto* Master = Cast<UMaterial>(Objects[0]);
    auto* Native = Cast<UMaterialInstanceConstant>(Objects[1]);
    auto* APS = Cast<UMaterialInstanceConstant>(Objects[2]);
    auto* Detail = Cast<UMaterialFunction>(Objects[4]);
    if (!Master || !Native || !APS || !Detail || Native->Parent.Get() != Master || APS->Parent.Get() != Native)
        return Refuse(TEXT("Unexpected material chain"));
    int32 Calls = 0;
    for (auto* E : FCore::Expressions(Master))
    {
        auto* C = Cast<UMaterialExpressionMaterialFunctionCall>(E);
        if (!C || C->MaterialFunction != Detail) continue;
        if (C->GetName() != TEXT("MaterialExpressionMaterialFunctionCall_14")
            && C->GetName() != TEXT("MaterialExpressionMaterialFunctionCall_16"))
            return Refuse(TEXT("Unexpected detail caller"));
        const auto* Pin = C->FunctionInputs.FindByPredicate([](const FFunctionExpressionInput& P)
            {return P.Input.InputName == TEXT("WorldPosition");});
        if (!Pin || Pin->Input.Expression) return Refuse(TEXT("Authored position override requires separate audit"));
        ++Calls;
    }
    if (Calls != 2) return Refuse(TEXT("Expected 400m and 5431m calls only"));
    APSSharedTerrainNormalContinuity::TTransform<FCore> Reader(B);
    const auto Graph = Reader.Graph(Detail);
    if (!B.Error.IsEmpty()) return Refuse(TEXT("Function graph traversal"));
    UMaterialExpressionDoubleVectorParameter* Center = nullptr;
    UMaterialExpressionFunctionInput* Position = nullptr;
    UMaterialExpressionFunctionInput* Size = nullptr;
    UMaterialExpressionDivide* Divide = nullptr;
    TArray<UMaterialExpressionTextureSample*> Samples;
    int32 Centers = 0;
    for (auto* E : Graph)
    {
        if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E);
            P && P->ParameterName == TEXT("APS_SharedPlanetCenter")) {Center = P; ++Centers;}
        if (auto* P = Cast<UMaterialExpressionFunctionInput>(E))
        {
            if (P->InputName == TEXT("WorldPosition")) Position = P;
            if (P->InputName == TEXT("TextureSize")) Size = P;
        }
        if (auto* T = Cast<UMaterialExpressionTextureSample>(E)) Samples.Add(T);
    }
    for (auto* E : Graph)
        if (auto* D = Cast<UMaterialExpressionDivide>(E); D && D->A.Expression == Position) Divide = D;
    auto* SignedSize = Divide ? Cast<UMaterialExpressionMultiply>(Divide->B.Expression) : nullptr;
    auto* Absolute = SignedSize ? Cast<UMaterialExpressionAbs>(SignedSize->A.Expression) : nullptr;
    if (Centers != 1 || !Position || !Position->bUsePreviewValueAsDefault || !Position->Preview.Expression
        || !Size || !SignedSize || !Absolute || Absolute->Input.Expression != Size
        || SignedSize->B.Expression || SignedSize->ConstB != -1 || Samples.Num() != 3)
        return Refuse(TEXT("Native coordinate contract changed"));
    uint32 Projections = 0;
    for (auto* T : Samples)
    {
        auto* M = Cast<UMaterialExpressionComponentMask>(T->Coordinates.Expression);
        if (!M || M->Input.Expression != Divide || M->A || int32(M->R)+int32(M->G)+int32(M->B) != 2
            || T->SamplerSource != SSM_Wrap_WorldGroupSettings || T->MipValueMode != TMVM_None
            || T->CoordinatesDX.Expression || T->CoordinatesDY.Expression || T->MipValue.Expression)
            return Refuse(TEXT("Native projection/sampler topology changed"));
        Projections |= 1u << ((M->R?1u:0u) | (M->G?2u:0u) | (M->B?4u:0u));
    }
    if (Projections != ((1u<<3)|(1u<<5)|(1u<<6))) return Refuse(TEXT("Missing projection"));
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaDetailPrecisionBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup)) return Refuse(TEXT("Immutable backup exists"));
    if (!IFileManager::Get().MakeDirectory(*Backup, true)) return Refuse(TEXT("Cannot create backup"));
    for (const auto& E : Expected)
    {
        const FString Copy = Backup / (FString(E.Name)+TEXT(".uasset"));
        if (Hash(Filename(E.Name)) != E.SHA1 || IFileManager::Get().Copy(*Copy, *Filename(E.Name), false, false) != COPY_OK
            || Hash(Copy) != E.SHA1) return Refuse(TEXT("Backup mismatch"));
    }
    const int32 Count = FCore::Expressions(Detail).Num();
    auto* Raw = B.Add<UMaterialExpressionWorldPosition>(Detail);
    Raw->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
    auto* UV = B.Add<UMaterialExpressionCustom>(Detail);
    UV->Description = TEXT("APS lava precise physical UV: compensated transform/division, wrap after unwrapped gradients, no warp");
    UV->OutputType = CMOT_Float3;
    UV->Inputs.Empty();
    const auto Input = [UV](const TCHAR* Name, UMaterialExpression* E, bool RGB = false) {
        FCustomInput I; I.InputName = Name; I.Input.Connect(0, E);
        if (RGB) I.Input.Mask = I.Input.MaskR = I.Input.MaskG = I.Input.MaskB = 1;
        UV->Inputs.Add(I);
    };
    Input(TEXT("Raw"), Raw); Input(TEXT("Center"), Center, true); Input(TEXT("Size"), SignedSize);
    const auto Row = [&](const TCHAR* Name, const FLinearColor& Default) {
        auto* P = B.Add<UMaterialExpressionVectorParameter>(Detail);
        P->ParameterName = Name; P->DefaultValue = Default;
        P->Group = TEXT("APS Physical Coordinate Contract"); P->UpdateParameterGuid(true, true); return P;
    };
    Input(TEXT("XHigh"), Row(TEXT("APS_SharedDetailRowXHigh"), FLinearColor(1,0,0,0)), true);
    Input(TEXT("XLow"), Row(TEXT("APS_SharedDetailRowXLow"), FLinearColor(0,0,0,0)), true);
    Input(TEXT("YHigh"), Row(TEXT("APS_SharedDetailRowYHigh"), FLinearColor(0,1,0,0)), true);
    Input(TEXT("YLow"), Row(TEXT("APS_SharedDetailRowYLow"), FLinearColor(0,0,0,0)), true);
    Input(TEXT("ZHigh"), Row(TEXT("APS_SharedDetailRowZHigh"), FLinearColor(0,0,1,0)), true);
    Input(TEXT("ZLow"), Row(TEXT("APS_SharedDetailRowZLow"), FLinearColor(0,0,0,0)), true);
    UV->Outputs.Reset(); UV->Outputs.Add(FExpressionOutput(TEXT("return"))); UV->bShowOutputNameOnPin = true;
    for (const TCHAR* Name : {TEXT("UVdx"), TEXT("UVdy")})
    {
        FCustomOutput O; O.OutputName = Name; O.OutputType = CMOT_Float3;
        UV->AdditionalOutputs.Add(O); UV->Outputs.Add(FExpressionOutput(Name));
    }
    UV->Code = TEXT("FDFVector3 R = DFSubtract(WSToDF(LWCRaw), WSToDF(LWCCenter));\n")
        TEXT("FDFVector3 PX = DFMultiply(R, MakeDFVector3(XHigh, XLow));\n")
        TEXT("FDFVector3 PY = DFMultiply(R, MakeDFVector3(YHigh, YLow));\n")
        TEXT("FDFVector3 PZ = DFMultiply(R, MakeDFVector3(ZHigh, ZLow));\n")
        TEXT("FDFScalar X = DFAdd(DFAdd(DFGetX(PX), DFGetY(PX)), DFGetZ(PX));\n")
        TEXT("FDFScalar Y = DFAdd(DFAdd(DFGetX(PY), DFGetY(PY)), DFGetZ(PY));\n")
        TEXT("FDFScalar Z = DFAdd(DFAdd(DFGetX(PZ), DFGetY(PZ)), DFGetZ(PZ));\n")
        TEXT("FDFVector3 U = DFDivide(MakeDFVector(X, Y, Z), Size);\n")
        TEXT("UVdx = DFDdxDemote(U); UVdy = DFDdyDemote(U);\n")
        TEXT("return DFFracDemote(U);\n");
    for (auto* T : Samples)
    {
        auto* M = CastChecked<UMaterialExpressionComponentMask>(T->Coordinates.Expression);
        const auto Projection = [&](int32 Output) {
            auto* P = B.Add<UMaterialExpressionComponentMask>(Detail); P->Input.Connect(Output, UV);
            P->R = M->R; P->G = M->G; P->B = M->B; P->A = false; return P;
        };
        T->Coordinates = FExpressionInput(); T->Coordinates.Connect(0, Projection(0));
        T->CoordinatesDX.Connect(0, Projection(1)); T->CoordinatesDY.Connect(0, Projection(2));
        T->MipValueMode = TMVM_Derivative;
    }
    if (FCore::Expressions(Detail).Num() != Count+17) return Refuse(TEXT("Unexpected node count"));
    Detail->PostEditChange(); UMaterialEditingLibrary::UpdateMaterialFunction(Detail); Master->PostEditChange();
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
            return Refuse(TEXT("Complete error-free LocalVF required"));
    }
    for (const auto& E : Expected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent asset edit"));
    UPackage* Package = Detail->GetOutermost(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Detail, *Filename(Expected[4].Name), Args)) return Refuse(TEXT("WAT save failed"));
    for (int32 I=0; I<4; ++I)
        if (Hash(Filename(Expected[I].Name)) != Expected[I].SHA1) return Refuse(TEXT("Protected master/MIC/20km function changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaDetailPrecision] SAVED calls=2 newNodes=17 addedTextureSamples=0 noWarp=1 paletteScalesTexturesUnchanged=1 masterMICs20kmUnchanged=1 backup=%s renderedAcceptancePending=1"), *Backup);
    return true;
}
}
#endif
