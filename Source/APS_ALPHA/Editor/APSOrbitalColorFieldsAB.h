#pragma once

#if WITH_EDITOR
#include "APSSharedTerrainMaterialBuilder.h"
#include "Engine/VolumeTexture.h"
#include "Materials/MaterialExpressionTextureSampleParameterVolume.h"
#include "Materials/MaterialExpressionVertexInterpolator.h"

// Diagnostic only. Keep the native near field and every terrain/biome input.
// The far color modulation has no wrapped volume tile or cubic interpolation grid.
namespace APSOrbitalColorFieldsAB
{
    inline const TCHAR* Shader = TEXT(R"HLSL(
#if !PIXELSHADER
// Some original noise consumers also feed vertex work. Preserve their exact
// native value: this candidate changes only pixel color, never displacement.
return Legacy;
#else
struct APSField
{
    uint hash(int3 p)
    {
        uint3 u=asuint(p);
        uint h=u.x*1597334677u ^ u.y*3812015801u ^ u.z*2798796415u;
        h^=h>>16; h*=2246822519u; h^=h>>13; h*=3266489917u; return h^(h>>16);
    }
    float gradient(int3 cell,float3 d)
    {
        uint g=hash(cell)%12u;
        float2 v=g<4u?d.xy:g<8u?d.xz:d.yz;
        return ((g&1u)!=0u?v.x:-v.x)+((g&2u)!=0u?v.y:-v.y);
    }
    float noise(float3 p)
    {
        int3 i=(int3)floor(p+dot(p,float3(1.0/3.0,1.0/3.0,1.0/3.0)));
        float3 x=p-float3(i)+dot(float3(i),float3(1.0/6.0,1.0/6.0,1.0/6.0));
        float3 e=step(x.yzx,x);
        int3 i1=(int3)(e*(1.0-e.zxy)),i2=(int3)(1.0-e.zxy*(1.0-e));
        float3 x1=x-float3(i1)+1.0/6.0,x2=x-float3(i2)+1.0/3.0,x3=x-0.5;
        float4 t=max(0.6-float4(dot(x,x),dot(x1,x1),dot(x2,x2),dot(x3,x3)),0.0);
        t*=t; t*=t;
        return 32.0*dot(t,float4(gradient(i,x),gradient(i+i1,x1),gradient(i+i2,x2),gradient(i+1,x3)));
    }
};
// Native UV already carries physical scale and planet-fixed orientation.
float3 p=float3(dot(P,float3(0.36,-0.48,0.80)),dot(P,float3(0.80,0.60,0.00)),
               dot(P,float3(-0.48,0.64,0.60)))*8.0+Salt;
float footprint=max(length(ddx(p)),length(ddy(p)));
float w=smoothstep(500000.0,5000000.0,length(CameraDelta)*abs(Scale));
if(w<=0.0) return Legacy;
APSField f;
float a=1.0-smoothstep(0.25,1.0,footprint);
float b=1.0-smoothstep(0.25,1.0,footprint*2.03);
float n=0.70*f.noise(p)*a+0.30*f.noise(p*2.03+float3(3.7,9.2,1.4))*b;
float3 value=saturate(Mean+n*StdDev*(0.65/0.30));
return lerp(Legacy,value,w);
#endif
)HLSL");

    template<class TBuild> bool Patch(TBuild& B, UMaterial* Master, UMaterialInstanceConstant* Template,
        UMaterialExpression* Enabled=nullptr)
    {
        APSSharedTerrainNormalContinuity::TTransform<TBuild> Reader(B);
        const auto Graph=Reader.Graph(Master);
        UMaterialExpression* Scale=nullptr;
        for(auto* E:Graph)
        {
            Reader.Register(Master,E);
            if(auto* P=Cast<UMaterialExpressionDoubleVectorParameter>(E);
                P && P->ParameterName==TEXT("APS_SharedInverseScale")) Scale=P;
        }
        if(!Scale) { B.Error=TEXT("Orbital fields require the native physical scale"); return false; }
        auto* Camera=Reader.template Add<UMaterialExpressionCameraPositionWS>(Master);
        auto* World=Reader.template Add<UMaterialExpressionWorldPosition>(Master);
        World->WorldPositionShaderOffset=WPT_ExcludeAllShaderOffsets;
        auto* Delta=Reader.template Add<UMaterialExpressionSubtract>(Master);
        Delta->A.Expression=Camera; Delta->B.Expression=World;
        int32 Count=0,Consumers=0,Preserved=0,PreservedConsumers=0;
        for(auto* E:Graph)
        {
            auto* Sample=Cast<UMaterialExpressionTextureSampleParameterVolume>(E);
            if(!Sample || Sample->ParameterName==TEXT("WapredNoise")) continue;
            // Native Noise1/Scale2 establishes the large coherent ice/color areas.
            // Replacing its mean/variance preserved statistics but erased that
            // organization in the full-planet V2 control. Keep its exact domain,
            // warp, interpolation and consumers; replace the other color fields.
            if(Sample->ParameterName==TEXT("Noise1"))
            {
                ++Preserved;
                for(auto* Consumer:Graph)
                    for(auto* In:Consumer->GetInputsView())
                        if(In && In->Expression==Sample) ++PreservedConsumers;
                continue;
            }
            if(!Sample->Coordinates.Expression || Sample->Coordinates.OutputIndex!=0)
            { B.Error=TEXT("Unexpected color volume coordinates"); return false; }
            UTexture* Source=nullptr;
            Template->GetTextureParameterValue(FHashedMaterialParameterInfo(Sample->ParameterName),Source);
            auto* Volume=Cast<UVolumeTexture>(Source);
            TArray64<uint8> Pixels;
            if(!Volume || Volume->Source.GetFormat()!=TSF_BGRA8 || !Volume->Source.GetMipData(Pixels,0) || Pixels.Num()%4 || Pixels.IsEmpty())
            { B.Error=TEXT("Color volume statistics unavailable"); return false; }
            FVector3d Sum(0),Sum2(0);
            for(int64 I=0;I<Pixels.Num();I+=4)
            {
                const FColor C(Pixels[I+2],Pixels[I+1],Pixels[I],Pixels[I+3]);
                const FLinearColor L=Volume->SRGB?FLinearColor(C):C.ReinterpretAsLinear();
                const FVector3d V(L.R,L.G,L.B); Sum+=V; Sum2+=V*V;
            }
            const FVector3d Mean=Sum/double(Pixels.Num()/4), Variance=Sum2/double(Pixels.Num()/4)-Mean*Mean;
            auto* M=Reader.template Add<UMaterialExpressionConstant3Vector>(Master);
            M->Constant=FLinearColor(Mean.X,Mean.Y,Mean.Z);
            auto* S=Reader.template Add<UMaterialExpressionConstant3Vector>(Master);
            S->Constant=FLinearColor(FMath::Sqrt(FMath::Max(0.0,Variance.X)),FMath::Sqrt(FMath::Max(0.0,Variance.Y)),FMath::Sqrt(FMath::Max(0.0,Variance.Z)));
            auto* Salt=Reader.template Add<UMaterialExpressionConstant3Vector>(Master);
            const uint32 Seed=FCrc::StrCrc32(*Sample->ParameterName.ToString());
            Salt->Constant=FLinearColor(11.3f+float(Seed%97),37.1f+float((Seed>>8)%89),71.7f+float((Seed>>16)%83));
            auto* Filter=Reader.template Add<UMaterialExpressionCustom>(Master);
            Filter->Description=TEXT("APS orbital color field diagnostic v3; native Noise1 and near");
            Filter->OutputType=CMOT_Float3; Filter->Code=Shader; Filter->Inputs.Empty();
            // Optional combined-candidate control. Mode0 must retain every native
            // field, not merely disable its normal-map half. Existing V3 is unchanged.
            if(Enabled && Filter->Code.ReplaceInline(TEXT("if(w<=0.0) return Legacy;"),
                TEXT("w*=saturate(FieldMode);\nif(w<=0.0) return Legacy;"),ESearchCase::CaseSensitive)!=1)
            { B.Error=TEXT("Orbital field enable anchor changed"); return false; }
            auto Add=[Filter](const TCHAR* Name,UMaterialExpression* Node)
            { FCustomInput In; In.InputName=Name; In.Input.Expression=Node; Filter->Inputs.Add(In); };
            auto* Domain=Sample->Coordinates.Expression;
            if(auto* VI=Cast<UMaterialExpressionVertexInterpolator>(Domain))
            {
                if(!VI->Input.Expression || VI->Input.OutputIndex!=0 || VI->Input.Mask)
                { B.Error=TEXT("Unexpected color field interpolator"); return false; }
                Domain=VI->Input.Expression;
            }
            // The authored warp adds the SAME sampled scalar to all three axes.
            // Magnifying that field for orbital noise folds it into visible curls.
            // Remove only this audited warp from the new FAR field; Legacy keeps
            // its exact warped/vertex-interpolated path, including ground detail.
            auto* Warped=Cast<UMaterialExpressionAdd>(Domain);
            auto* Warp=Warped?Cast<UMaterialExpressionDivide>(Warped->B.Expression):nullptr;
            auto* WarpSample=Warp?Cast<UMaterialExpressionTextureSampleParameterVolume>(Warp->A.Expression):nullptr;
            if(!Warped || !WarpSample
                || WarpSample->ParameterName!=TEXT("WapredNoise") || Warp->B.Expression || Warp->ConstB!=3.0f)
            { B.Error=TEXT("Audited far coordinate warp contract changed"); return false; }
            Domain=Warped->A.Expression;
            if(auto* Nested=Cast<UMaterialExpressionAdd>(Domain))
            {
                auto* Offset=Cast<UMaterialExpressionDivide>(Nested->A.Expression);
                auto* OffsetSample=Offset?Cast<UMaterialExpressionTextureSampleParameterVolume>(Offset->A.Expression):nullptr;
                if(!OffsetSample || OffsetSample->ParameterName!=TEXT("Noise3") || Offset->B.Expression || Offset->ConstB!=2.0f)
                { B.Error=TEXT("Audited secondary color warp contract changed"); return false; }
                Domain=Nested->B.Expression;
            }
            if(!Cast<UMaterialExpressionDivide>(Domain))
            { B.Error=TEXT("Orbital domain is not a physical-scale division"); return false; }
            Add(TEXT("Legacy"),Sample); Add(TEXT("P"),Domain);
            Add(TEXT("Mean"),M); Add(TEXT("StdDev"),S); Add(TEXT("Salt"),Salt);
            Add(TEXT("CameraDelta"),Delta); Add(TEXT("Scale"),Scale);
            Filter->Inputs.Last().Input.Mask=Filter->Inputs.Last().Input.MaskR=1;
            if(Enabled)Add(TEXT("FieldMode"),Enabled);
            for(auto* Consumer:Graph)
                for(auto* In:Consumer->GetInputsView())
                    if(In && In->Expression==Sample)
                    {
                        if(In->OutputIndex!=0 || In->MaskA) { B.Error=TEXT("Unexpected color volume output channel"); return false; }
                        In->Expression=Filter; ++Consumers;
                    }
            UE_LOG(LogTemp,Display,TEXT("[APS.OrbitalFieldsAB] parameter=%s mean=%s stddev=%s"),
                *Sample->ParameterName.ToString(),*M->Constant.ToString(),*S->Constant.ToString());
            ++Count;
        }
        if(Count!=5 || Consumers!=9 || Preserved!=1 || PreservedConsumers!=1)
        { B.Error=FString::Printf(TEXT("Expected 5/9 replaced and 1/1 native Noise1, got %d/%d and %d/%d"),Count,Consumers,Preserved,PreservedConsumers); return false; }
        UE_LOG(LogTemp,Display,TEXT("[APS.OrbitalFieldsAB] v3 preservedNoise1=%d consumers=%d replaced=%d/%d"),Preserved,PreservedConsumers,Count,Consumers);
        return B.Error.IsEmpty();
    }
}
#endif
