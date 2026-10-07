#pragma once
#if WITH_EDITOR
#include "APSSharedAmmoniaMaterialBuilder.h"
#include "APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h"

// Emissive diagnostic only: separate raw interpolated position, compensated
// physical phase and filtered gradient. Never a production water material.
namespace APSWaterDomainAuditBuilder
{
inline bool Build(IAssetTools& AssetTools)
{
    using FCore = APSSharedTerrainMaterialBuilder::FBuild;
    using namespace APSSharedAmmoniaMaterialBuilder;
    const bool bFinalNormal = FParse::Param(FCommandLine::Get(), TEXT("APSWaterFinalNormalAudit"));
    const bool bSecondary = FParse::Param(FCommandLine::Get(), TEXT("APSWaterSecondaryDomainAudit"));
    const TCHAR* Folder = bSecondary ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSecondaryAudit20260930") : bFinalNormal
        ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterFinalNormalAudit20260930")
        : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDomainAudit20260930");
    FCore B(AssetTools, Folder);
    auto Fail = [&B](const TCHAR* Why) { UE_LOG(LogTemp, Error, TEXT("[APS.WaterDomainAudit] %s %s"), Why, *B.Error); return false; };
    auto* Source = LoadObject<UMaterial>(nullptr, APSWaterAnalyticWaves::MasterPath);
    auto* SourceMI = LoadObject<UMaterialInstanceConstant>(nullptr, APSWaterAnalyticWaves::TemplatePath);
    // Pins below are checked separately; both saved source packages are read-only.
    const auto Matches = [&]() { return SavedSourceMatches(Source, TEXT("2C3C1A66927E2E3F0E80893DC60D36428C5440A4"))
        && SavedSourceMatches(SourceMI, TEXT("BF9CBB7F05B6D6F33B45E809EDF1CD4DB6D7F906")); };
    if (!Matches() || SourceMI->Parent.Get() != Source || Source->bTangentSpaceNormal) return Fail(TEXT("Analytic source drift"));
    auto* M = Cast<UMaterial>(B.Duplicate(Source, TEXT("M_APS_WaterDomainAudit")));
    auto* MI = Cast<UMaterialInstanceConstant>(B.Duplicate(SourceMI, TEXT("MI_APS_WaterDomainAudit")));
    if (!M || !MI) return Fail(TEXT("New outputs only"));
    UMaterialExpressionCustom* Domain = nullptr;
    UMaterialExpressionCustom* Filter = nullptr;
    for (auto* E : FCore::Expressions(M))
    {
        auto* C = Cast<UMaterialExpressionCustom>(E);
        if (!C || !C->Code.Contains(TEXT("1023.0f")) || C->AdditionalOutputs.Num() != 2) continue;
        if (Domain) return Fail(TEXT("Ambiguous primary domain"));
        Domain = C;
    }
    if (!Domain || Domain->Inputs.Num() != 9) return Fail(TEXT("Primary domain inputs changed"));
    for (auto* E : FCore::Expressions(M))
        if (auto* C = Cast<UMaterialExpressionCustom>(E); C && C->Inputs.Num() == 3
            && C->Inputs[1].Input.Expression == Domain && C->Inputs[2].Input.Expression == Domain)
        { if (Filter) return Fail(TEXT("Ambiguous footprint consumer")); Filter = C; }
    if (!Filter) return Fail(TEXT("Primary filtered gradient absent"));
    auto* Mode = B.Add<UMaterialExpressionScalarParameter>(M);
    Mode->ParameterName = TEXT("APS_WaterDomainAuditMode"); Mode->DefaultValue = 0;
    Mode->UpdateParameterGuid(true, true);
    auto* Debug = B.Add<UMaterialExpressionCustom>(M);
    Debug->Inputs.Empty(); Debug->OutputType = CMOT_Float3;
    const auto Add = [&](const TCHAR* Name, const FExpressionInput& Input)
    { FCustomInput I; I.InputName = Name; I.Input = Input; Debug->Inputs.Add(I); };
    Add(TEXT("Raw"), Domain->Inputs[0].Input); Add(TEXT("Size"), Domain->Inputs[2].Input);
    Add(TEXT("X"), Domain->Inputs[3].Input); Add(TEXT("Y"), Domain->Inputs[5].Input); Add(TEXT("Z"), Domain->Inputs[7].Input);
    for (auto Pair : {TPair<const TCHAR*, UMaterialExpression*>(TEXT("Folded"), Domain),
        {TEXT("Gradient"), Filter}, {TEXT("Mode"), Mode}})
    { FExpressionInput I; I.Connect(0, Pair.Value); Add(Pair.Key, I); }
    Debug->Description = TEXT("DIAGNOSTIC: 0 camera-relative phase, 1 planet-fixed phase, 2 filtered gradient");
    Debug->Code = TEXT("float3 small = float3(dot(Raw,X),dot(Raw,Y),dot(Raw,Z)) / max(Size,1.0);\n")
        TEXT("if (Mode > 1.5) return saturate(0.5 + Gradient.rgb * 0.08);\n")
        TEXT("float3 p = Mode < 0.5 ? small : Folded;\nreturn 0.5 + 0.5 * cos(p * 6.28318530718);\n");
    if (bFinalNormal || bSecondary)
    {
        // Follow the actual final normal closure, not a re-created wave formula.
        auto* WorldNormal = Cast<UMaterialExpressionCustom>(M->GetExpressionInputForProperty(MP_Normal)->Expression);
        if (!WorldNormal || WorldNormal->Code != TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;")) return Fail(TEXT("World normal closure drift"));
        // The existing frame adapter retains UE's unnamed default custom pin.
        // Resolve real pins by name rather than assuming a packed array layout.
        const auto Named = [WorldNormal](const TCHAR* Name) -> const FExpressionInput*
        {
            const auto* P = WorldNormal->Inputs.FindByPredicate([Name](const FCustomInput& I) { return I.InputName == FName(Name); });
            return P && P->Input.Expression ? &P->Input : nullptr;
        };
        const auto* V = Named(TEXT("V")); const auto* RX = Named(TEXT("X"));
        const auto* RY = Named(TEXT("Y")); const auto* RZ = Named(TEXT("Z"));
        if (!V || !RX || !RY || !RZ) return Fail(TEXT("World rotation pins absent"));
        UMaterialExpression* LocalNormal = V->Expression;
        if (!LocalNormal || LocalNormal->GetInputsView().Num() != 1) return Fail(TEXT("Local normalize closure drift"));
        UMaterialExpression* Sum = LocalNormal->GetInputsView()[0]->Expression;
        if (!Sum || Sum->GetInputsView().Num() != 2) return Fail(TEXT("Local wave sum closure drift"));
        UMaterialExpression* Radial = Sum->GetInputsView()[0]->Expression;
        UMaterialExpressionCustom* Secondary = nullptr;
        for (auto* E : FCore::Expressions(M))
            if (auto* C = Cast<UMaterialExpressionCustom>(E); C && C != Filter && C->Inputs.Num() == 3
                && C->Inputs[1].Input.Expression == C->Inputs[2].Input.Expression
                && C->Inputs[1].Input.Expression != Domain && C->Code.Contains(TEXT("footprint")))
            { if (Secondary) return Fail(TEXT("Secondary gradient ambiguous")); Secondary = C; }
        if (!Radial || !Secondary) return Fail(TEXT("Final normal diagnostic inputs absent"));
        Debug->Inputs.Empty();
        for (auto Pair : {TPair<const TCHAR*, UMaterialExpression*>(TEXT("Secondary"), Secondary),
            {TEXT("Local"), LocalNormal}, {TEXT("Radial"), Radial}, {TEXT("World"), WorldNormal}, {TEXT("Mode"), Mode}})
        { FExpressionInput I; I.Connect(0, Pair.Value); Add(Pair.Key, I); }
        Add(TEXT("RX"), *RX); Add(TEXT("RY"), *RY); Add(TEXT("RZ"), *RZ);
        Debug->Description = TEXT("DIAGNOSTIC: secondary gradient, local normal residual, world normal residual before GBuffer");
        Debug->Code = TEXT("if (Mode < 0.5) return saturate(0.5 + Secondary.rgb * 0.08);\n")
            TEXT("float3 r = normalize(Radial);\nif (Mode < 1.5) return saturate(0.5 + (normalize(Local)-r)*16.0);\n")
            TEXT("float3 rw = normalize(r.x*RX+r.y*RY+r.z*RZ);\nreturn saturate(0.5+(normalize(World)-rw)*16.0);\n");
        if (bSecondary)
        {
            auto* SD = Cast<UMaterialExpressionCustom>(Secondary->Inputs[1].Input.Expression);
            if (!SD || SD->AdditionalOutputs.Num() != 2) return Fail(TEXT("Secondary domain drift"));
            Debug->Inputs.Empty();
            FExpressionInput P; P.Connect(0, SD); Add(TEXT("Phase"), P);
            Add(TEXT("Unfiltered"), Secondary->Inputs[0].Input);
            Add(TEXT("DX"), Secondary->Inputs[1].Input); Add(TEXT("DY"), Secondary->Inputs[2].Input);
            FExpressionInput ModeInput; ModeInput.Connect(0, Mode); Add(TEXT("Mode"), ModeInput);
            Debug->Description = TEXT("DIAGNOSTIC: secondary phase, unfiltered gradient, footprint visibility");
            Debug->Code = TEXT("if (Mode < 0.5) return 0.5 + 0.5*cos(Phase*6.28318530718);\n")
                TEXT("if (Mode < 1.5) return saturate(0.5+Unfiltered.rgb*0.08);\n")
                TEXT("float f=max(length(DX),length(DY)); return (1.0-smoothstep(0.25,1.0,f)).xxx;\n");
        }
    }
    auto* Zero = B.Add<UMaterialExpressionConstant>(M); Zero->R = 0;
    for (auto Property : {MP_BaseColor, MP_Specular, MP_Metallic})
    { auto* I = M->GetExpressionInputForProperty(Property); *I = FExpressionInput(); I->Connect(0, Zero); }
    M->GetExpressionInputForProperty(MP_EmissiveColor)->Connect(0, Debug);
    MI->SetParentEditorOnly(M, false); M->PostEditChange(); MI->PostEditChange();
    for (auto* Output : B.Outputs) CastChecked<UMaterialInterface>(Output)->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    for (auto* Output : B.Outputs)
    {
        auto* R = CastChecked<UMaterialInterface>(Output)->GetMaterialResource(GMaxRHIFeatureLevel);
        auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
        if (!R || !Map || !R->IsGameThreadShaderMapComplete() || R->GetCompileErrors().Num()
            || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)) return Fail(TEXT("Incomplete debug shaders"));
    }
    if (!Matches() || B.Outputs.Num() != 2) return Fail(TEXT("Input/output drift"));
    for (auto* Output : B.Outputs)
    {
        auto* Package = Output->GetOutermost();
        if (!Package->GetName().StartsWith(FString(Folder) + TEXT("/"))) return Fail(TEXT("Output escaped folder"));
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, Output, *Filename, Args)) return Fail(TEXT("Save failed"));
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterDomainAudit] saved=2 bound=0 threeDiagnosticModes=1; not water art or performance acceptance"));
    return true;
}
}
#endif
