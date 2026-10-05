#pragma once
#if WITH_EDITOR
#include "APSPlanetReliefNormalUpdate.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

// Same-master Normal-only opt-in. Immutable stereographic charts contain
// filtered physical-height slopes; no colour/UV/mask/geometry consumers change.
namespace APSCanonicalCoverageUpdate
{
    inline bool Patch(UMaterial* Master, FString& Error)
    {
        if (!IsRunningCommandlet() || !IsInGameThread() || !IsValid(Master))
        { Error=TEXT("Canonical coverage requires offline owned master"); return false; }
        auto* RootNormal=Master->GetExpressionInputForProperty(MP_Normal);
        auto* Final=RootNormal ? Cast<UMaterialExpressionCustom>(RootNormal->Expression) : nullptr;
        if (!Final || Final->Description!=TEXT("APS planet vector to rendered world")
            || Final->Code!=TEXT("return V.x*X.xyz + V.y*Y.xyz + V.z*Z.xyz;"))
        { Error=TEXT("Coverage requires unmodified original final normal"); return false; }
        UMaterialExpressionCustom* Baseline=nullptr;
        TArray<UObject*> Children; GetObjectsWithOuter(Master,Children,false);
        for (UObject* Child:Children) if (auto* C=Cast<UMaterialExpressionCustom>(Child))
        {
            if (C->Description==TEXT("APS canonical stereo coverage sample v1"))
            { Error=TEXT("Coverage already installed"); return false; }
            if (C->Description==TEXT("APS physical-distance geometric normal continuity v1"))
            {
                if (Baseline) { Error=TEXT("Ambiguous coverage baseline"); return false; }
                Baseline=C;
            }
        }
        const auto Input=[](UMaterialExpressionCustom* C,const TCHAR* Name)
        { FExpressionInput R; if(C)for(const auto& I:C->Inputs)if(I.InputName==Name)R=I.Input; return R; };
        const auto Relative=Input(Baseline,TEXT("Relative"));
        const auto X=Input(Final,TEXT("X")),Y=Input(Final,TEXT("Y")),Z=Input(Final,TEXT("Z"));
        const auto Native=Input(Baseline,TEXT("NativeNormal")),Delta=Input(Baseline,TEXT("CameraDelta"));
        const auto Scale=Input(Baseline,TEXT("InverseScale")),Start=Input(Baseline,TEXT("StartCm")),End=Input(Baseline,TEXT("EndCm"));
        if(!Relative.Expression||!X.Expression||!Y.Expression||!Z.Expression||!Native.Expression
            ||!Delta.Expression||!Scale.Expression||!Start.Expression||!End.Expression)
        { Error=TEXT("Coverage missing original coordinate/filter links"); return false; }
        UTexture2D* Default=LoadObject<UTexture2D>(nullptr,TEXT("/Engine/Functions/Engine_MaterialFunctions02/PivotPainter2/Black_1x1_EXR_Texture.Black_1x1_EXR_Texture"));
        if(!Default||Default->SRGB||Default->GetMaterialType()!=MCT_Texture2D
            ||UMaterialExpressionTextureBase::GetSamplerTypeForTexture(Default)!=SAMPLERTYPE_LinearColor)
        { Error=TEXT("Coverage requires existing linear EXR fallback"); return false; }
        TArray<UMaterialExpression*> Added;
        const auto Register=[&](UMaterialExpression* E)
        { E->Material=Master; E->MaterialExpressionGuid=FGuid::NewGuid(); Master->GetExpressionCollection().AddExpression(E); Added.Add(E); };
        const auto Vector=[&](const FString& Name,FLinearColor Value)
        {
            auto* P=NewObject<UMaterialExpressionVectorParameter>(Master); Register(P);
            P->ParameterName=*Name; P->DefaultValue=Value; P->Group=TEXT("APS Canonical Coverage");
            P->UpdateParameterGuid(true,true); return P;
        };
        auto* Available=NewObject<UMaterialExpressionScalarParameter>(Master); Register(Available);
        Available->ParameterName=TEXT("APS_CanonicalCoverageAvailable"); Available->DefaultValue=0;
        Available->UpdateParameterGuid(true,true);
        auto* C=Vector(TEXT("APS_CanonicalCoverageCenter"),FLinearColor(0,0,1,0));
        auto* U=Vector(TEXT("APS_CanonicalCoverageU"),FLinearColor(1,0,0,0));
        auto* V=Vector(TEXT("APS_CanonicalCoverageV"),FLinearColor(0,1,0,0));
        auto* Sample=NewObject<UMaterialExpressionCustom>(Master); Register(Sample);
        Sample->Description=TEXT("APS canonical stereo coverage sample v1"); Sample->OutputType=CMOT_Float4;
        Sample->Inputs.Reset(); Sample->Outputs.Reset(); Sample->Outputs.Add(FExpressionOutput());
        const auto Wire=[](UMaterialExpression* E){FExpressionInput I;I.Expression=E;return I;};
        const auto Add=[&](const FString& Name,const FExpressionInput& I)
        {FCustomInput P;P.InputName=*Name;P.Input=I;Sample->Inputs.Add(P);};
        Add(TEXT("Relative"),Relative);Add(TEXT("X"),X);Add(TEXT("Y"),Y);Add(TEXT("Z"),Z);
        Add(TEXT("Native"),Native);Add(TEXT("CameraDelta"),Delta);Add(TEXT("InverseScale"),Scale);
        Add(TEXT("StartCm"),Start);Add(TEXT("EndCm"),End);
        Add(TEXT("C"),Wire(C));Add(TEXT("U"),Wire(U));Add(TEXT("V"),Wire(V));
        // VectorParameter output0 is RGB in UE5.4; count uses its explicit A pin.
        auto CountPin=Wire(C);CountPin.OutputIndex=4;CountPin.Mask=CountPin.MaskA=1;
        Add(TEXT("Count"),CountPin);
        Sample->Code=TEXT(R"HLSL(
float3 p=float3(dot(Relative.xyz,X.xyz),dot(Relative.xyz,Y.xyz),dot(Relative.xyz,Z.xyz));
float p2=dot(p,p);
float3 d=p*rsqrt(max(p2,1.e-12));
float front=dot(d,C.xyz);
float2 stereo=float2(dot(d,U.xyz),dot(d,V.xyz))/max(1.0+front,1.e-4);
float remain=1.0;
float3 slope=0;
)HLSL");
        for(int32 I=0;I<9;++I)
        {
            auto* T=NewObject<UMaterialExpressionTextureObjectParameter>(Master);Register(T);
            T->ParameterName=*FString::Printf(TEXT("APS_CanonicalCoverageTexture%d"),I);
            T->Texture=Default;T->SamplerType=SAMPLERTYPE_LinearColor;T->UpdateParameterGuid(true,true);
            auto* M=Vector(FString::Printf(TEXT("APS_CanonicalCoverageMetrics%d"),I),FLinearColor(1,.5f/1024,1,0));
            Add(FString::Printf(TEXT("Chart%d"),I),Wire(T));Add(FString::Printf(TEXT("Metrics%d"),I),Wire(M));
            // All bound textures have identical clamp/trilinear sampler state.
            // One explicit sampler reused across nine resources, not nine states.
            Sample->Code+=FString::Printf(TEXT(R"HLSL(
{
 float2 uv=.5+Metrics%d.x*stereo;
 float2 gx=ddx(uv),gy=ddy(uv);
 float edge=max(abs(uv.x-.5),abs(uv.y-.5));
 float cover=(%d>=Count-.5)?1.0:1.0-smoothstep(.42,.49,edge);
 float weight=remain*cover*step(%d+.5,Count);
 if(weight>0.0)
 {
   float3 g=Texture2DSampleGrad(Chart%d,Chart0Sampler,clamp(uv,Metrics%d.y,1.0-Metrics%d.y),gx,gy).xyz;
   if(!all(isfinite(g))) return float4(0,0,1,0);
   slope+=g*weight;
 }
 remain-=weight;
}
)HLSL"),I,I+1,I,I,I,I);
        }
        Sample->Code+=TEXT(R"HLSL(
slope-=d*dot(slope,d);
float3 n=normalize(d-slope);
float3 world=n.x*X.xyz+n.y*Y.xyz+n.z*Z.xyz;
// Preserve accepted close-range normal exactly. Replace only the old
// native-to-radial lighting target, smoothly, not the material or its masks.
float distanceCm=length(CameraDelta.xyz)*max(InverseScale.x,0.0);
float t=saturate((distanceCm-StartCm)/max(EndCm-StartCm,1.0));
t=t*t*(3.0-2.0*t);
float3 target=normalize(lerp(Native.xyz,world,t));
float valid=step(1.e-6,p2)*step(0.0,front)*step(.5,Count)*step(Count,9.5)*step(remain,1.e-4);
return float4(target,valid*step(1.e-8,t));
)HLSL");
        APSPlanetReliefNormalUpdate::FInputs Inputs;
        Inputs.ExpectedMasterPath=Master->GetPathName();Inputs.ExpectedRockNormal=*RootNormal;
        Inputs.BaselineWorldNormal=Wire(Baseline);Inputs.Availability=Wire(Available);
        Inputs.CanonicalWorldNormal=Wire(Sample);
        Inputs.CanonicalWorldNormal.Mask=Inputs.CanonicalWorldNormal.MaskR=Inputs.CanonicalWorldNormal.MaskG=Inputs.CanonicalWorldNormal.MaskB=1;
        Inputs.Blend=Wire(Sample);Inputs.Blend.Mask=Inputs.Blend.MaskA=1;
        APSPlanetReliefNormalUpdate::FResult Result;
        if(!APSPlanetReliefNormalUpdate::Patch(Master,Inputs,Result,Error))
        {
            for(auto* E:Added){Master->GetExpressionCollection().RemoveExpression(E);E->MarkAsGarbage();}
            return false;
        }
        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] sameMaster=%s disabledDefault=1 finalNormalOnly=1 maxCharts=9 samplerStates=1 immutableAnchor=1; motion/all-orbit proof pending"),*Master->GetPathName());
        return true;
    }
}
#endif
