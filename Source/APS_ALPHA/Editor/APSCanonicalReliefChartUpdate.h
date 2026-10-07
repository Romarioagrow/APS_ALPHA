#pragma once
#if WITH_EDITOR
#include "APSPlanetReliefNormalUpdate.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

// Explicit offline experiment on the SAME master: disabled pixel lighting normal.
// No existing colour/slope/coordinate consumers or material instances change.
namespace APSCanonicalReliefChartUpdate
{
    inline bool Patch(UMaterial* Master, FString& Error)
    {
        if (!IsRunningCommandlet() || !IsInGameThread() || !IsValid(Master))
        { Error = TEXT("Canonical chart patch requires offline owned master"); return false; }
        auto* RootNormal = Master->GetExpressionInputForProperty(MP_Normal);
        auto* Final = RootNormal ? Cast<UMaterialExpressionCustom>(RootNormal->Expression) : nullptr;
        if (!Final || Final->Description != TEXT("APS planet vector to rendered world")
            || Final->Code != TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;"))
        { Error = TEXT("Unexpected original final normal frame"); return false; }
        UMaterialExpressionCustom* Baseline = nullptr;
        TArray<UObject*> Children; GetObjectsWithOuter(Master, Children, false);
        for (UObject* Child : Children)
            if (auto* C = Cast<UMaterialExpressionCustom>(Child))
            {
                if (C->Description == TEXT("APS bounded canonical chart pixel sample v1"))
                { Error = TEXT("Canonical chart already installed"); return false; }
                if (C->Description == TEXT("APS physical-distance geometric normal continuity v1"))
                {
                    if (Baseline) { Error = TEXT("Ambiguous baseline normal filter"); return false; }
                    Baseline = C;
                }
            }
        const auto Input = [](UMaterialExpressionCustom* C, const TCHAR* Name)
        { FExpressionInput Found; if (C) for (const auto& I : C->Inputs) if (I.InputName == Name) Found = I.Input; return Found; };
        const auto Relative = Input(Baseline, TEXT("Relative"));
        const auto AxisX = Input(Final, TEXT("X")), AxisY = Input(Final, TEXT("Y")), AxisZ = Input(Final, TEXT("Z"));
        if (!Relative.Expression || !AxisX.Expression || !AxisY.Expression || !AxisZ.Expression)
        { Error = TEXT("Canonical chart missing original LWC relative position/planet axes"); return false; }
        // Existing linear EXR is a serialisable, disabled fallback. Never alter
        // engine texture settings or save a transient runtime chart as a default.
        UTexture2D* Default = LoadObject<UTexture2D>(nullptr,
            TEXT("/Engine/Functions/Engine_MaterialFunctions02/PivotPainter2/Black_1x1_EXR_Texture.Black_1x1_EXR_Texture"));
        if (!Default || Default->SRGB || Default->GetMaterialType() != MCT_Texture2D
            || UMaterialExpressionTextureBase::GetSamplerTypeForTexture(Default) != SAMPLERTYPE_LinearColor)
        { Error = TEXT("Canonical chart needs the existing non-sRGB non-VT linear EXR default; no asset settings changed"); return false; }
        TArray<UMaterialExpression*> Added;
        const auto Register = [&](UMaterialExpression* E)
        { E->Material=Master; E->MaterialExpressionGuid=FGuid::NewGuid(); Master->GetExpressionCollection().AddExpression(E); Added.Add(E); };
        const auto Vector = [&](const TCHAR* Name, FLinearColor Value)
        {
            auto* P=NewObject<UMaterialExpressionVectorParameter>(Master); Register(P);
            P->ParameterName=Name; P->DefaultValue=Value; P->Group=TEXT("APS Canonical Chart Diagnostic");
            P->UpdateParameterGuid(true,true); return P;
        };
        auto* Texture=NewObject<UMaterialExpressionTextureObjectParameter>(Master); Register(Texture);
        Texture->ParameterName=TEXT("APS_CanonicalChartTexture"); Texture->Texture=Default;
        Texture->SamplerType=SAMPLERTYPE_LinearColor; Texture->UpdateParameterGuid(true,true);
        auto* Available=NewObject<UMaterialExpressionScalarParameter>(Master); Register(Available);
        Available->ParameterName=TEXT("APS_CanonicalChartAvailable"); Available->DefaultValue=0;
        Available->UpdateParameterGuid(true,true);
        auto* C=Vector(TEXT("APS_CanonicalChartCenter"),FLinearColor(0,0,1,0));
        auto* U=Vector(TEXT("APS_CanonicalChartU"),FLinearColor(1,0,0,0));
        auto* V=Vector(TEXT("APS_CanonicalChartV"),FLinearColor(0,1,0,0));
        // Radius/width, half-texel, full-width cm (provenance), unused.
        auto* Metrics=Vector(TEXT("APS_CanonicalChartMetrics"),FLinearColor(1,.5,1,0));
        auto* Sample=NewObject<UMaterialExpressionCustom>(Master); Register(Sample);
        Sample->Description=TEXT("APS bounded canonical chart pixel sample v1"); Sample->OutputType=CMOT_Float4;
        Sample->Inputs.Reset(); Sample->Outputs.Reset(); Sample->Outputs.Add(FExpressionOutput());
        Sample->Code=TEXT(R"HLSL(
float3 p=float3(dot(Relative.xyz,X.xyz),dot(Relative.xyz,Y.xyz),dot(Relative.xyz,Z.xyz));
float p2=dot(p,p);
float3 d=p*rsqrt(max(p2,1.e-12));
float front=dot(d,C.xyz);
float2 uv=.5+Metrics.x*float2(dot(d,U.xyz),dot(d,V.xyz))/max(front,1.e-4);
// Gradients precede clamp/coverage; distance never multiplies relief amplitude.
float2 gx=ddx(uv),gy=ddy(uv);
float edge=max(abs(uv.x-.5),abs(uv.y-.5));
float weight=(1.0-smoothstep(.45,.49,edge))*step(.001,front)*step(1.e-6,p2);
float2 bounded=clamp(uv,Metrics.y,1.0-Metrics.y);
float3 n=Texture2DSampleGrad(Chart,ChartSampler,bounded,gx,gy).xyz;
float n2=dot(n,n);
if(!all(isfinite(n)) || n2<1.e-8) return float4(0,0,1,0);
n*=rsqrt(n2);
weight*=step(.01,dot(n,d));
float3 world=n.x*X.xyz+n.y*Y.xyz+n.z*Z.xyz;
return float4(world,weight);
)HLSL");
        const auto Add=[&](const TCHAR* Name,const FExpressionInput& I)
        { FCustomInput P; P.InputName=Name; P.Input=I; Sample->Inputs.Add(P); };
        const auto Wire=[](UMaterialExpression* E) { FExpressionInput I; I.Expression=E; return I; };
        Add(TEXT("Chart"),Wire(Texture)); Add(TEXT("Relative"),Relative);
        Add(TEXT("X"),AxisX); Add(TEXT("Y"),AxisY); Add(TEXT("Z"),AxisZ);
        Add(TEXT("C"),Wire(C)); Add(TEXT("U"),Wire(U)); Add(TEXT("V"),Wire(V)); Add(TEXT("Metrics"),Wire(Metrics));
        APSPlanetReliefNormalUpdate::FInputs Inputs;
        Inputs.ExpectedMasterPath=Master->GetPathName(); Inputs.ExpectedRockNormal=*RootNormal;
        Inputs.BaselineWorldNormal=Wire(Baseline); Inputs.Availability=Wire(Available);
        Inputs.CanonicalWorldNormal=Wire(Sample);
        Inputs.CanonicalWorldNormal.Mask=Inputs.CanonicalWorldNormal.MaskR=Inputs.CanonicalWorldNormal.MaskG=Inputs.CanonicalWorldNormal.MaskB=1;
        Inputs.Blend=Wire(Sample); Inputs.Blend.Mask=Inputs.Blend.MaskA=1;
        APSPlanetReliefNormalUpdate::FResult Result;
        if (!APSPlanetReliefNormalUpdate::Patch(Master,Inputs,Result,Error))
        {
            for (auto* E:Added) { Master->GetExpressionCollection().RemoveExpression(E); E->MarkAsGarbage(); }
            return false;
        }
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalChart] sameMaster=%s disabledDefault=1 finalNormalOnly=1 newNodes=%d; local experiment, not orbital coverage or visual acceptance"),
            *Master->GetPathName(),Added.Num()+1);
        return true;
    }
}
#endif
