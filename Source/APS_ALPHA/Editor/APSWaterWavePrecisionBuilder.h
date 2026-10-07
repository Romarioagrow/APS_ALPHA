#pragma once
#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"

// Only the water noise domain is replaced. The gradient-noise kernel, amplitude,
// coverage, bathymetry, normals and shading outputs remain in the existing graph.
namespace APSWaterWavePrecisionBuilder
{
using FCore = APSSharedTerrainMaterialBuilder::FBuild;
inline UMaterialExpressionCustom* Add(FCore& B, UMaterial* M,
    UMaterialExpression* ExistingDomain, UMaterialExpressionScalarParameter* Size, int32 Period, bool bCameraRelative = false)
{
    UMaterialExpressionDoubleVectorParameter* Center = nullptr;
    int32 Centers = 0;
    // Follow this wave band's actual inputs, not unrelated/unconnected frame
    // parameters created for other consumers (e.g. the Fresnel rotation).
    TArray<UMaterialExpression*> Pending{ExistingDomain};
    TSet<UMaterialExpression*> Seen;
    for (int32 Index = 0; Index < Pending.Num(); ++Index)
    {
        auto* E = Pending[Index];
        if (!E || Seen.Contains(E)) continue;
        if (!E->IsIn(M) || Seen.Num() >= 256)
        { B.Error = TEXT("Water domain input closure escaped its master or bound"); return nullptr; }
        Seen.Add(E);
        if (auto* P = Cast<UMaterialExpressionDoubleVectorParameter>(E);
            P && P->ParameterName == TEXT("APS_SharedPlanetCenter"))
        { Center = P; ++Centers; }
        for (FExpressionInput* Input : E->GetInputsView())
            if (Input && Input->Expression) Pending.Add(Input->Expression);
    }
    if (Centers != 1 || !Size || Period <= 0 || Period % 3)
    { B.Error = FString::Printf(TEXT("Water compensated domain contract centers=%d size=%d period=%d"), Centers, Size != nullptr, Period); return nullptr; }

    auto* Raw = B.Add<UMaterialExpressionWorldPosition>(M);
    Raw->WorldPositionShaderOffset = bCameraRelative ? WPT_CameraRelativeNoOffsets : WPT_ExcludeAllShaderOffsets;
    auto* Domain = B.Add<UMaterialExpressionCustom>(M);
    Domain->Description = TEXT("Water planet-fixed DoubleFloat domain; unwrapped derivatives before bounded phase");
    Domain->OutputType = CMOT_Float3;
    Domain->Inputs.Empty();
    const auto Input = [Domain](const TCHAR* Name, UMaterialExpression* E, bool RGB = false)
    {
        FCustomInput I; I.InputName = Name; I.Input.Connect(0, E);
        if (RGB) I.Input.Mask = I.Input.MaskR = I.Input.MaskG = I.Input.MaskB = 1;
        Domain->Inputs.Add(I);
    };
    Input(TEXT("Raw"), Raw); Input(TEXT("Center"), Center, true); Input(TEXT("Size"), Size);
    const auto Row = [&](const TCHAR* Name, const FLinearColor& Default)
    {
        // Reuse the same rows for both bands, with one runtime physical frame.
        for (auto* E : FCore::Expressions(M))
            if (auto* P = Cast<UMaterialExpressionVectorParameter>(E); P && P->ParameterName == Name)
                return P;
        auto* P = B.Add<UMaterialExpressionVectorParameter>(M);
        P->ParameterName = Name; P->DefaultValue = Default;
        P->Group = TEXT("APS Physical Coordinate Contract"); P->UpdateParameterGuid(true, true);
        return P;
    };
    Input(TEXT("XHigh"), Row(TEXT("APS_SharedDetailRowXHigh"), FLinearColor(1,0,0,0)), true);
    Input(TEXT("XLow"), Row(TEXT("APS_SharedDetailRowXLow"), FLinearColor(0,0,0,0)), true);
    Input(TEXT("YHigh"), Row(TEXT("APS_SharedDetailRowYHigh"), FLinearColor(0,1,0,0)), true);
    Input(TEXT("YLow"), Row(TEXT("APS_SharedDetailRowYLow"), FLinearColor(0,0,0,0)), true);
    Input(TEXT("ZHigh"), Row(TEXT("APS_SharedDetailRowZHigh"), FLinearColor(0,0,1,0)), true);
    Input(TEXT("ZLow"), Row(TEXT("APS_SharedDetailRowZLow"), FLinearColor(0,0,0,0)), true);
    Domain->Outputs.Reset(); Domain->Outputs.Add(FExpressionOutput(TEXT("return")));
    Domain->bShowOutputNameOnPin = true;
    for (const TCHAR* Name : {TEXT("DomainDX"), TEXT("DomainDY")})
    {
        FCustomOutput O; O.OutputName = Name; O.OutputType = CMOT_Float3;
        Domain->AdditionalOutputs.Add(O); Domain->Outputs.Add(FExpressionOutput(Name));
    }
    // Reconstruct directly from the near-camera interpolant in the second
    // candidate. Converting an already reconstructed FWS position to DF cannot
    // recover precision lost earlier in the material's tile-offset arithmetic.
    Domain->Code = bCameraRelative
        ? TEXT("FDFVector3 R = DFSubtract(DFSubtract(MakeDFVector3(Raw, float3(0,0,0)), ResolvedView.PreViewTranslation), WSToDF(LWCCenter));\n")
        : TEXT("FDFVector3 R = DFSubtract(WSToDF(LWCRaw), WSToDF(LWCCenter));\n");
    Domain->Code +=
        TEXT("FDFVector3 PX = DFMultiply(R, MakeDFVector3(XHigh, XLow));\n")
        TEXT("FDFVector3 PY = DFMultiply(R, MakeDFVector3(YHigh, YLow));\n")
        TEXT("FDFVector3 PZ = DFMultiply(R, MakeDFVector3(ZHigh, ZLow));\n")
        TEXT("FDFScalar X = DFAdd(DFAdd(DFGetX(PX), DFGetY(PX)), DFGetZ(PX));\n")
        TEXT("FDFScalar Y = DFAdd(DFAdd(DFGetX(PY), DFGetY(PY)), DFGetZ(PY));\n")
        TEXT("FDFScalar Z = DFAdd(DFAdd(DFGetX(PZ), DFGetY(PZ)), DFGetZ(PZ));\n")
        TEXT("FDFVector3 U = DFDivide(MakeDFVector(X, Y, Z), max(Size, 1.0f));\n")
        TEXT("DomainDX = DFDdxDemote(U); DomainDY = DFDdyDemote(U);\n")
        + FString::Printf(TEXT("return DFFracDemote(DFDivide(U, %d.0f)) * %d.0f;\n"), Period, Period);
    return Domain;
}
}
#endif
