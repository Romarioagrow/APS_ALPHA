#pragma once
#if WITH_EDITOR
#include "APSLavaFineStochasticUpdate.h"
#include "APSLavaCrustReflectanceUpdate.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionTransform.h"

// Candidate-only physical surface-gradient bump. Reuses the already filtered
// fine RGB height field; no extra texture fetches, displacement or mask changes.
namespace APSLavaCrustNormalUpdate
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
using APSLavaAntiGridUpdate::Filename;
using APSLavaAntiGridUpdate::Hash;
inline bool Run(IAssetTools& Tools)
{
    FCore B(Tools, APSSharedLavaMaterialBuilder::Destination);
    const auto Refuse = [&B](const FString& Why) {
        UE_LOG(LogTemp, Error, TEXT("[APS.LavaCrustNormal] Refused: %s %s"), *Why, *B.Error); return false;
    };
    if (!IsRunningCommandlet()) return Refuse(TEXT("Offline only"));
    TArray<APSLavaFinePeriodUpdate::FExpected> Protected;
    Protected.Append(APSLavaFinePeriodUpdate::Expected, UE_ARRAY_COUNT(APSLavaFinePeriodUpdate::Expected));
    Protected[0].SHA1 = TEXT("7CB431F8384A27832DA5E17BCDB37B5D8B306B16");
    Protected.Add({TEXT("MF_APS_LavaFineStochastic_v1"), TEXT("75CF7DF479BB11412917E1869FA0018CEADEE645")});
    for (const auto& E : Protected)
        if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(FString(TEXT("Asset drift: ")) + E.Name);
    const FString Root(APSSharedLavaMaterialBuilder::Destination);
    auto* Master = LoadObject<UMaterial>(nullptr, *(Root / TEXT("M_APS_SharedLava.M_APS_SharedLava")));
    auto* Native = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLavaNative.MI_APS_SharedLavaNative")));
    auto* APS = LoadObject<UMaterialInstanceConstant>(nullptr, *(Root / TEXT("MI_APS_SharedLava.MI_APS_SharedLava")));
    if (!Master || !Native || !APS || Master->GetOutermost()->IsDirty()
        || Native->Parent.Get() != Master || APS->Parent.Get() != Native
        || FCore::Expressions(Master).Num() != 128 || !Master->bTangentSpaceNormal)
        return Refuse(TEXT("Unexpected candidate graph/normal space"));
    auto* Normal = Master->GetExpressionInputForProperty(MP_Normal);
    auto* Fine = FindObject<UMaterialExpressionMaterialFunctionCall>(Master, TEXT("MaterialExpressionMaterialFunctionCall_14"));
    if (!Normal || Normal->Expression || !Fine || !Fine->MaterialFunction
        || Fine->MaterialFunction->GetName() != TEXT("MF_APS_LavaFineStochastic_v1"))
        return Refuse(TEXT("Expected empty normal and stochastic fine field"));
    UMaterialExpressionDoubleVectorParameter* InverseScale = nullptr;
    for (auto* E : FCore::Expressions(Master))
        if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E); P && P->ParameterName == TEXT("APS_SharedInverseScale"))
            InverseScale = P;
    if (!InverseScale) return Refuse(TEXT("Missing physical scale"));
    for (const TCHAR* Name : {TEXT("APS_LavaFineStochastic"), TEXT("APS_LavaThermalCoverage")})
    {
        float Value = -1;
        if (!APS->GetScalarParameterValue(FMaterialParameterInfo(Name), Value) || Value != 0)
            return Refuse(TEXT("Candidate defaults must remain disabled"));
    }
    TMap<int32, FExpressionInput> Properties;
    for (int32 P = 0; P < MP_MAX; ++P)
        if (const auto* In = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(P))) Properties.Add(P, *In);
    const FString Backup = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SharedLavaCrustNormalBackup_20260927"));
    if (IFileManager::Get().DirectoryExists(*Backup) || !IFileManager::Get().MakeDirectory(*Backup, true))
        return Refuse(TEXT("Immutable backup exists/unavailable"));
    for (const auto& E : Protected)
        if (Hash(Filename(E.Name)) != E.SHA1
            || IFileManager::Get().Copy(*(Backup / (FString(E.Name)+TEXT(".uasset"))), *Filename(E.Name), false, false) != COPY_OK
            || Hash(Backup / (FString(E.Name)+TEXT(".uasset"))) != E.SHA1) return Refuse(TEXT("Backup mismatch"));

    auto* Position = B.Add<UMaterialExpressionWorldPosition>(Master);
    Position->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
    auto* DX = B.Add<UMaterialExpressionDDX>(Master);
    auto* DY = B.Add<UMaterialExpressionDDY>(Master);
    DX->Value.Connect(0, Position); DY->Value.Connect(0, Position);
    auto* PhysicalDX = B.Add<UMaterialExpressionMultiply>(Master);
    auto* PhysicalDY = B.Add<UMaterialExpressionMultiply>(Master);
    PhysicalDX->A.Connect(0, DX); PhysicalDY->A.Connect(0, DY);
    for (auto* D : {PhysicalDX, PhysicalDY})
    {
        D->B.Connect(0, InverseScale);
        D->B.Mask = D->B.MaskR = 1; D->B.MaskG = D->B.MaskB = D->B.MaskA = 0;
    }
    auto* Geometric = B.Add<UMaterialExpressionVertexNormalWS>(Master);
    auto* Height = B.Add<UMaterialExpressionScalarParameter>(Master);
    Height->ParameterName = TEXT("APS_LavaCrustHeightCm"); Height->DefaultValue = 0;
    Height->SliderMin = 0; Height->SliderMax = 100; Height->Group = TEXT("APS Lava Surface"); Height->UpdateParameterGuid(true, true);
    auto* Bump = B.Add<UMaterialExpressionCustom>(Master);
    Bump->Description = TEXT("APS lava physical surface-gradient crust normal v1: existing filtered height, no new texture reads");
    Bump->OutputType = CMOT_Float3; Bump->Inputs.Empty();
    const auto Add = [Bump](const TCHAR* Name, UMaterialExpression* E, int32 Output = 0) {
        FCustomInput In; In.InputName = Name; In.Input.Connect(Output, E); Bump->Inputs.Add(In);
    };
    Add(TEXT("Fine"), Fine, 2); Add(TEXT("PhysicalDX"), PhysicalDX); Add(TEXT("PhysicalDY"), PhysicalDY);
    Add(TEXT("Geometric"), Geometric); Add(TEXT("HeightCm"), Height);
    // Differentiate native LWC position before custom-node float demotion.
    // Scale the derivative frame by its footprint to avoid horizon determinant
    // overflow/underflow. Signed determinant handles mirrored screen frames.
    // Height comes from continuous RGB, NOT thresholded thermal alpha.
    Bump->Code = TEXT(
        "float3 N = normalize(Geometric);\n"
        "float footprint = max(length(PhysicalDX), length(PhysicalDY));\n"
        "float scale = max(footprint, 1.e-6);\n"
        "float3 X = PhysicalDX / scale, Y = PhysicalDY / scale;\n"
        "float3 R1 = cross(Y, N), R2 = cross(N, X);\n"
        "float determinant = dot(X, R1);\n"
        "float height = (1.0 - Fine.r) * clamp(HeightCm, 0.0, 100.0);\n"
        "float2 dh = float2(ddx(height), ddy(height)) / scale;\n"
        "float safeDet = (determinant < 0.0 ? -1.0 : 1.0) * max(abs(determinant), 1.e-6);\n"
        "float3 gradient = (dh.x * R1 + dh.y * R2) / safeDet;\n"
        "float valid = smoothstep(1.e-5, 1.e-4, abs(determinant));\n"
        "gradient *= valid / max(1.0, length(gradient));\n"
        "return normalize(N - gradient);\n");
    auto* Tangent = B.Add<UMaterialExpressionTransform>(Master);
    Tangent->TransformSourceType = TRANSFORMSOURCE_World; Tangent->TransformType = TRANSFORM_Tangent;
    Tangent->Input.Connect(0, Bump); Normal->Connect(0, Tangent);
    if (FCore::Expressions(Master).Num() != 137) return Refuse(TEXT("Unexpected normal node count"));
    for (const auto& Pair : Properties)
        if (Pair.Key != MP_Normal)
        {
            const auto* Now = Master->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Pair.Key));
            if (!Now || !APSLavaCrustReflectanceUpdate::SameInput(*Now, Pair.Value)) return Refuse(TEXT("Non-normal output changed"));
        }
    Master->PostEditChange();
    UMaterialInterface* Materials[] = {Master, Native, APS};
    for (auto* M : Materials) M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* M : Materials)
    {
        auto* R = M->GetMaterialResource(GMaxRHIFeatureLevel); const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        float Value = -1;
        if (!R || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
            || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_LavaCrustHeightCm")), Value) || Value != 0)
            return Refuse(TEXT("Complete LocalVF and disabled inherited height required"));
    }
    for (const auto& E : Protected) if (Hash(Filename(E.Name)) != E.SHA1) return Refuse(TEXT("Concurrent edit"));
    auto* Package = Master->GetOutermost(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Master, *Filename(TEXT("M_APS_SharedLava")), Args)) return Refuse(TEXT("Save failed"));
    for (int32 I = 1; I < Protected.Num(); ++I) if (Hash(Filename(Protected[I].Name)) != Protected[I].SHA1) return Refuse(TEXT("Protected asset changed"));
    UE_LOG(LogTemp, Display, TEXT("[APS.LavaCrustNormal] SAVED normalOnly=1 defaultHeightCm=0 physicalDerivativeFrame=1 colourRadianceMaskUnchanged=1 additionalTextureReads=0 backup=%s renderedAcceptancePending=1"), *Backup);
    return true;
}
}
#endif
