#pragma once

#if WITH_EDITOR
// Isolated normal-only experiment. Mathematical basis: Mikkelsen,
// Practical Real-Time Hex-Tiling, JCGT 11(3), 2022:
// https://jcgt.org/published/0011/03/05/
// Project adaptation: compensated physical coordinates, integer hash, quarter
// turns with inverse normal rotation, UE normal unpacking/shared sampler.
namespace APSNormalHexSampling
{
    template<class TBuild, class TReader>
    bool Patch(TBuild& B, TReader& Reader, UMaterialFunction* Function)
    {
        auto Graph=Reader.Graph(Function);
        UMaterialExpressionCustom* Domain=nullptr;
        TArray<UMaterialExpressionTextureSample*> Samples;
        for(UMaterialExpression* E:Graph)
        {
            Reader.Register(Function,E);
            if(auto* C=Cast<UMaterialExpressionCustom>(E); C && C->Description==TEXT("APS compensated planet-fixed detail UV, bounded detiling and unwrapped gradients"))
            { if(Domain){B.Error=TEXT("Duplicate compensated domain");return false;} Domain=C; }
            if(auto* S=Cast<UMaterialExpressionTextureSample>(E))Samples.Add(S);
        }
        if(!Domain || Samples.Num()!=3 || Domain->Outputs.Num()!=3 || Domain->AdditionalOutputs.Num()!=2)
        { B.Error=TEXT("Normal hex requires exact native domain and three projections");return false; }
        FExpressionInput Size;
        for(const auto& I:Domain->Inputs)if(I.InputName==TEXT("Size"))Size=I.Input;
        if(!Size.Expression){B.Error=TEXT("Normal hex missing physical texture period");return false;}
        if(Domain->Code.ReplaceInline(TEXT("return DFFracDemote(Unwrapped);"),
            TEXT("HexHigh = Unwrapped.High;\nHexLow = Unwrapped.Low;\nreturn DFFracDemote(Unwrapped);"),ESearchCase::CaseSensitive)!=1)
        { B.Error=TEXT("Normal hex domain anchor changed");return false; }
        for(const TCHAR* Name:{TEXT("HexHigh"),TEXT("HexLow")})
        {
            FCustomOutput Out;Out.OutputName=Name;Out.OutputType=CMOT_Float3;
            Domain->AdditionalOutputs.Add(Out);Domain->Outputs.Add(FExpressionOutput(Name));
        }
        auto* Mode=B.template Add<UMaterialExpressionScalarParameter>(Function);
        Mode->ParameterName=TEXT("APS_NormalMacroWarpMode");Mode->DefaultValue=1;
        Mode->Group=TEXT("APS Normal Macro Diagnostic");Mode->UpdateParameterGuid(true,true);
        TMap<UMaterialExpression*,UMaterialExpression*> Replacements;
        for(auto* Sample:Samples)
        {
            auto* Projection=Cast<UMaterialExpressionComponentMask>(Sample->Coordinates.Expression);
            if(!Projection || Projection->Input.Expression!=Domain || !Sample->TextureObject.Expression
                || Sample->MipValueMode!=TMVM_Derivative || Sample->SamplerSource!=SSM_Wrap_WorldGroupSettings
                || Sample->AutomaticViewMipBiasValue.Expression || !Sample->AutomaticViewMipBias)
            { B.Error=TEXT("Normal hex sample contract changed");return false; }
            auto* Custom=B.template Add<UMaterialExpressionCustom>(Function);
            Custom->Description=TEXT("APS normal hex: stable compensated cells, signed normal rotation and slope blend");
            Custom->OutputType=CMOT_Float4;Custom->Inputs.Empty();
            const auto Input=[Custom](const TCHAR* Name,const FExpressionInput& Value)
            {FCustomInput I;I.InputName=Name;I.Input=Value;Custom->Inputs.Add(I);};
            Input(TEXT("NMap"),Sample->TextureObject);Input(TEXT("UV"),Sample->Coordinates);
            Input(TEXT("DX"),Sample->CoordinatesDX);Input(TEXT("DY"),Sample->CoordinatesDY);
            Input(TEXT("PeriodSize"),Size);
            FExpressionInput M;M.Expression=Mode;Input(TEXT("Mode"),M);
            for(int32 Index=3;Index<=4;++Index)
            {
                auto* Mask=B.template Add<UMaterialExpressionComponentMask>(Function);
                Mask->Input.Expression=Domain;Mask->Input.OutputIndex=Index;
                Mask->R=Projection->R;Mask->G=Projection->G;Mask->B=Projection->B;Mask->A=false;
                FExpressionInput I;I.Expression=Mask;Input(Index==3?TEXT("High"):TEXT("Low"),I);
            }
            Custom->Code=TEXT(R"HLSL(
struct FAPSNormalHex
{
    uint Hash(int2 p)
    {
        uint h=uint(p.x)*0x9e3779b9u+uint(p.y)*0x85ebca6bu+0xc2b2ae35u;
        h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);
    }
    float2 Turn(float2 v,uint r)
    {
        if(r==1u)return float2(-v.y,v.x);
        if(r==2u)return -v;
        if(r==3u)return float2(v.y,-v.x);
        return v;
    }
};
FAPSNormalHex H;
float3 Period=abs(PeriodSize);
float Amount=saturate(Mode)*smoothstep(5000.0,20000.0,min(Period.x,min(Period.y,Period.z)));
if(Amount<=0.0)
    return UnpackNormalMap(Texture2DSampleGrad(NMap,GetMaterialSharedSampler(NMapSampler,View.MaterialTextureBilinearWrapedSampler),UV,DX,DY));

// Retain the low component through the skew and fractional cell position.
FDFVector2 Q=MakeDFVector2(High,Low);
FDFScalar X=DFSubtract(DFMultiply(DFGetX(Q),3.46410161514),DFMultiply(DFGetY(Q),2.0));
FDFScalar Y=DFMultiply(DFGetY(Q),4.0);
FDFVector2 Lattice=MakeDFVector(X,Y);
float2 F=DFFracDemote(Lattice);
int2 Cell=int2(round(DFSubtractDemote(Lattice,F)));
float3 Weights;
int2 C[3];
if(F.x+F.y<=1.0)
{
    C[0]=Cell;C[1]=Cell+int2(1,0);C[2]=Cell+int2(0,1);
    Weights=float3(1.0-F.x-F.y,F.x,F.y);
}
else
{
    C[0]=Cell+int2(1,1);C[1]=Cell+int2(0,1);C[2]=Cell+int2(1,0);
    Weights=float3(F.x+F.y-1.0,1.0-F.x,1.0-F.y);
}
float2 Sum=0.0;float Total=0.0;
[unroll]for(int i=0;i<3;++i)
{
    uint Bits=H.Hash(C[i]);uint Rotation=Bits>>30;
    float2 Offset=float2(Bits&32767u,(Bits>>15)&32767u)/32768.0;
    float2 P=frac(H.Turn(UV,Rotation)+Offset);
    float3 N=UnpackNormalMap(Texture2DSampleGrad(NMap,GetMaterialSharedSampler(NMapSampler,View.MaterialTextureBilinearWrapedSampler),P,H.Turn(DX,Rotation),H.Turn(DY,Rotation))).xyz;
    float2 Slope=N.xy/max(abs(N.z),max(max(abs(N.x),abs(N.y))/128.0,1e-6));
    Slope=H.Turn(Slope,(4u-Rotation)&3u);
    float Len2=dot(Slope,Slope);
    float W=pow(saturate(Weights[i]),7.0)*(.4+.6*sqrt(Len2/(1.0+Len2)));
    Sum+=W*Slope;Total+=W;
}
float2 Slope=Sum/max(Total,1e-8);
if(Amount<1.0)
{
    float3 Native=UnpackNormalMap(Texture2DSampleGrad(NMap,GetMaterialSharedSampler(NMapSampler,View.MaterialTextureBilinearWrapedSampler),UV,DX,DY)).xyz;
    float2 NativeSlope=Native.xy/max(abs(Native.z),max(max(abs(Native.x),abs(Native.y))/128.0,1e-6));
    Slope=lerp(NativeSlope,Slope,Amount);
}
return float4(normalize(float3(Slope,1.0)),1.0);
)HLSL");
            Replacements.Add(Sample,Custom);
        }
        int32 Consumers=0;
        for(UMaterialExpression* E:Graph)
            for(FExpressionInput* I:E->GetInputsView())if(I)
                if(auto* const* Replacement=Replacements.Find(I->Expression))
                {
                    // Native sample pin0=RGB and pin4=alpha; preserve their masks.
                    if(I->OutputIndex!=0 && I->OutputIndex!=4)
                    {B.Error=TEXT("Unexpected normal sample output consumer");return false;}
                    if(I->OutputIndex==4){I->Mask=1;I->MaskR=I->MaskG=I->MaskB=0;I->MaskA=1;}
                    else if(!I->Mask){I->Mask=1;I->MaskR=I->MaskG=I->MaskB=1;I->MaskA=0;}
                    I->OutputIndex=0;I->Expression=*Replacement;++Consumers;
                }
        if(Consumers<3){B.Error=TEXT("No normal sample consumers replaced");return false;}
        UE_LOG(LogTemp,Display,TEXT("[APS.NormalHex] Three normal projections rewired, consumers=%d; 1 native or 3 hex samples per projection; 50..200m period gate; no altitude gate"),Consumers);
        return true;
    }
}
#endif
