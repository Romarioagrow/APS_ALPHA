#pragma once
#include "APSPlanetCloudHlsl.h"

// Isolated V30: one mesh/pass, ordered shell intervals, one shared 32-tap budget.
// V27 assets and the single-layer shader body are not overwritten.
namespace APSPlanetCloudLayeredHlsl
{
inline const TCHAR* FieldCode() { return TEXT(R"HLSL(
struct LayeredCloudField
{
    float threshold(float h,float body,int layer)
    {
        // Low cumulus has a flatter condensation base and rounded towers;
        // the middle deck grows vertically. Height changes the iso-surface,
        // not the alpha of a pre-cut slab.
        float peak=layer==0?.30:.42;
        float below=max(0.,(peak-h)/peak), above=max(0.,(h-peak)/(1.-peak));
        return lerp(.69,.40,body)+.58*below*below+.43*above*above;
    }
    float billows(float3 p,float3 seed,float footprint,float h,float2 body,
        int layer,float feature,float2 wind)
    {
        if(h<=0. || h>=1. || body.y<=.0001) return 0.;
        CloudField B;
        float3 q=B.advect(p,wind);
        if(layer==2)
        {
            // Thin, stretched cirrus, not a third copy of cumulus. Rotated
            // Cartesian coordinates have no equirectangular seam/polar pinch.
            q=B.rotateDomain(q)/feature;
            float resolved=1.-smoothstep(.25,.5,footprint*.48/feature);
            float n=.5;
            if(resolved>.001) n=B.noise(q*float3(.035,.16,.48)+seed);
            float mean=.5+resolved*(n-.5);
            float sigma=.135*sqrt(max(0.,1.-resolved*resolved));
            float edge=abs(h*2.-1.);
            return B.expectedDensity(mean,sigma,lerp(.64,.42,body.x)+.55*edge*edge)*body.y;
        }
        float scale=(layer==0?1.05:.55)/feature;
        return B.filteredDensity(q,seed,footprint,threshold(h,body.x,layer),scale)*body.y;
    }
    float density(float3 p,float radius,float3 deck,float coverage,int layer,
        float3 seed,float feature,float2 wind,float banding,float swirl,
        float footprint,out float h,out float2 body)
    {
        h=(length(p)-radius-deck.x)/max(deck.y,.001); body=0.;
        if(h<=0. || h>=1. || coverage<=0. || deck.z<=0.) return 0.;
        CloudField B;
        float3 n=B.advect(normalize(p),wind);
        float front=B.weather(n,seed,feature,banding,swirl);
        // Regional clusters survive the orbital low pass, whereas kilometre
        // billows resolve only during approach. Both share the same weather,
        // never swap to a camera-dependent cloud map.
        float regional=B.noise(B.rotateDomain(n*(28./feature))+seed);
        float weight=1.-smoothstep(.25,.5,footprint*110./(max(radius,1.)*feature));
        float meso=.5;
        if(weight>.001) meso=B.noise(B.rotateDomain(n*(110./feature))+seed+float3(7,19,3));
        float weather=.57*front+.28*regional+.15*lerp(.5,meso,weight);
        float cutoff=lerp(.70,.30,saturate(coverage));
        float shape=smoothstep(cutoff-.065,cutoff+.095,weather);
        body=float2(saturate(shape+.28*(regional-.5)),smoothstep(0.,.22,shape));
        // Weather already gates the occupied area through body.y. Keep a
        // substantial low/middle core inside that area instead of suppressing
        // it again with a high iso-threshold. Empty sky and cirrus are unchanged.
        if(layer<2) body.x=lerp(.72,1.,body.x);
        return billows(p,seed,footprint,h,body,layer,feature,wind);
    }
};
LayeredCloudField LF;
)HLSL"); }
inline const TCHAR* IntervalsCode() { return TEXT(R"HLSL(
float3 Decks[3]={LowDeck,MidDeck,HighDeck};
float Covers[3]={DeckCoverage.x,DeckCoverage.y,DeckCoverage.z};
// Numeric Debug4 belongs to the single-shell instrument. Explicit magenta
// sentinel here, never an unclipped volume masquerading as that depth probe.
if(Debug>3.5 && Debug<4.5) return float4(1,0,1,.5);
float RawDepth=LookupDeviceZ(ScreenAlignedPosition(GetScreenPosition(Parameters)));
float Limit=1.e30;
if(Debug<2.5 || Debug>5.5)
    Limit=F.viewDepth(RawDepth,SceneZ)/max(dot(WorldD,ResolvedView.ViewForward),.001)/max(CmPerKm,1.e-8);
float2 Ground=F.sphere(O,D,Radius);
if(Ground.x>0.) Limit=min(Limit,Ground.x);

// A shell can produce TWO intervals on a limb ray. Splitting the occupied
// intervals avoids losing the far shell and avoids spending samples in gaps.
float Begins[6], Ends[6], Weights[6]; int Layers[6]; int Count=0;
[unroll] for(int layer=0;layer<3;layer++)
{
    float3 deck=Decks[layer];
    if(deck.y<=0. || deck.z<=0. || Covers[layer]<=0.) continue;
    float2 outer=F.sphere(O,D,Radius+deck.x+deck.y);
    float a=max(0.,outer.x), b=min(Limit,outer.y);
    if(b<=a) continue;
    float2 inner=F.sphere(O,D,Radius+deck.x);
    bool split=inner.y>a && inner.x<b;
    float starts[2]={a,max(a,inner.y)};
    float stops[2]={split?min(b,inner.x):b,b};
    [unroll] for(int part=0;part<2;part++)
    {
        if(part==1 && !split) continue;
        if(stops[part]<=starts[part]) continue;
        Begins[Count]=starts[part]; Ends[Count]=stops[part]; Layers[Count]=layer;
        // Thin cirrus gets fewer taps; tall middle volumes get more. Length
        // weighting saturates on grazing rays so one horizon does not starve
        // the other visible decks.
        Weights[Count]=(layer==0?1.0:(layer==1?1.6:.45))
            *sqrt(clamp((stops[part]-starts[part])/max(deck.y,.001),.1,16.));
        Count++;
    }
}
if(Count==0) return float4(0,0,0,0);
// Fixed small insertion sort; a single Trans integrates near-to-far across
// ALL decks, avoiding independent translucency passes/double atmosphere.
[loop] for(int i=1;i<Count;i++)
{
    float a=Begins[i], b=Ends[i], w=Weights[i]; int layer=Layers[i], j=i-1;
    [loop] while(j>=0)
    {
        if(Begins[j]<=a) break;
        Begins[j+1]=Begins[j]; Ends[j+1]=Ends[j]; Weights[j+1]=Weights[j]; Layers[j+1]=Layers[j]; j--;
    }
    Begins[j+1]=a; Ends[j+1]=b; Weights[j+1]=w; Layers[j+1]=layer;
}
if(Debug>1.5 && Debug<2.5) return float4(0,1,0,.5);
if(Debug>6.5 && Debug<7.5) return float4(float(Count)/6.,0,0,1);
float TotalWeight=0., MaxRelativeSpan=1.;
[loop] for(int i=0;i<Count;i++)
{
    TotalWeight+=Weights[i];
    MaxRelativeSpan=max(MaxRelativeSpan,(Ends[i]-Begins[i])/max(Decks[Layers[i]].y,.001));
}
// No layer multiplies the ray budget: maximum32 view samples total, including
// two guaranteed samples for every occupied interval (up to six intervals).
int TotalSamples=(int)clamp(ceil(16.*MaxRelativeSpan),16.,32.);
int Remaining=TotalSamples-2*Count;
float Trans=1., CloudAlpha=0., CloudDistance=0.; float3 Sum=0.;
float3 ToSun=normalize(Sun);
float feature=clamp(WeatherScale,.25,3.);
uint2 Pixel=uint2(Parameters.SvPosition.xy);
uint Phase=Pixel.x*1597334677u ^ Pixel.y*3812015801u;
Phase^=Phase>>16; Phase*=2246822519u; Phase^=Phase>>13;
float SamplePhase=.05+.90*float(Phase&65535u)/65535.;
)HLSL"); }
inline const TCHAR* IntegralCode() { return TEXT(R"HLSL(
[loop] for(int segment=0;segment<Count;segment++)
{
    int extra=segment==Count-1?Remaining:(int)floor(float(Remaining)*Weights[segment]/max(TotalWeight,.0001));
    int Samples=2+extra; Remaining-=extra; TotalWeight-=Weights[segment];
    int layer=Layers[segment]; float3 deck=Decks[layer];
    float3 seed=SeedOffset+float3(13,29,7)*float(layer+1);
    float Start=Begins[segment], End=Ends[segment], RayLength=End-Start;
    float DistributionScale=max(deck.y*.5,.05);
    float Growth=pow(1.+RayLength/DistributionScale,1./float(Samples));
    float NextStep=DistributionScale*(Growth-1.);
    if(Growth<1.00001) { Growth=1.; NextStep=RayLength/float(Samples); }
    float NextStart=Start;
    const float ExtinctionPerKm=8.*max(deck.z,0.);
    [loop] for(int k=0;k<Samples;k++)
    {
        float Step=max(0.,min(NextStep,End-NextStart));
        float Distance=NextStart+SamplePhase*Step;
        NextStart+=Step; NextStep*=Growth;
        float3 P=O+D*Distance, N=normalize(P);
        float footprint=max(RayFootprint*Distance,Step*.5);
        float h; float2 body;
        float density=LF.density(P,Radius,deck,Covers[layer],layer,seed,feature,
            WindRotation.xy,Banding,Swirl,footprint,h,body);
        if(density<.0001) continue;
        float opacity=1.-exp(-density*Step*ExtinctionPerKm);
        float Day=smoothstep(-.08,.24,dot(N,ToSun));
        float ShadowDepth=0.;
        // True spherical exit, not thickness / N.L at the horizon. Two light
        // taps, sharing the local weather envelope, as in the original budget.
        float LightLength=min(12.,max(0.,F.sphere(P,ToSun,Radius+deck.x+deck.y).y));
        if(Day>.001)
        {
            [unroll] for(int j=0;j<2;j++)
            {
                float3 LP=P+ToSun*(LightLength*(j==0?.18:.68));
                float lh=(length(LP)-Radius-deck.x)/max(deck.y,.001);
                float ld=LF.billows(LP,seed,max(footprint,LightLength*.25),lh,body,layer,feature,WindRotation.xy);
                ShadowDepth+=ld*LightLength*.5;
            }
        }
        float direct=exp(-ShadowDepth*ExtinctionPerKm);
        // Forward scattering makes sunlit thin edges bright; interior colour
        // comes from depth, height, condensate and the actual star, not random
        // colour patches. A bounded diffuse term keeps dense cores readable.
        float mu=clamp(dot(D,ToSun),-1.,1.), g=.55;
        float phase=(1.-g*g)/pow(max(.08,1.+g*g-2.*g*mu),1.5);
        float diffuse=.055+.085*h+.14*pow(direct,.35);
        float3 Light=float3(.006,.008,.012)*(1.+Day)
            +SunColor*Day*(diffuse+direct*(.65+.22*phase));
        float Contribution=Trans*opacity;
        Sum+=Contribution*Light*Albedo; CloudAlpha+=Contribution;
        CloudDistance+=Contribution*Distance; Trans*=1.-opacity;
        if(Trans<.015) break;
    }
    if(Trans<.015) break;
}
float Alpha=CloudAlpha*Visibility;
if(Debug>7.5) return float4(Alpha.xxx,1);
float3 AirTransmission=1., AirRadiance=0.;
if(Aerial>.5 && CloudAlpha>.0001)
    F.foregroundAir(O,D,ToSun,CloudDistance/CloudAlpha,AirRadius,AirHeight,
        float2(AirOptics.z,AirMieHeight),AirRayleigh,AirMie,AirOzone,
        AirSettings.x,AirSettings.y,AirSettings.z,AirFill,EyeAdaptation,
        AirTransmission,AirRadiance);
return float4((Sum/max(CloudAlpha,.0001))*AirTransmission+AirRadiance,Alpha);
)HLSL"); }
inline FString Code()
{
    // Chemical haze/dust/ash keep the single-layer path; the new water/ice
    // vertical policy cannot silently put terrestrial cumulus on every world.
    return FString(APSPlanetCloudHlsl::FieldCode())+APSPlanetCloudHlsl::RayCode()
        +TEXT("\nif (LayeredStyle < .5) {\n")+APSPlanetCloudHlsl::SingleLayerCode()+TEXT("\n}\n")
        +FieldCode()+IntervalsCode()+IntegralCode();
}
}
