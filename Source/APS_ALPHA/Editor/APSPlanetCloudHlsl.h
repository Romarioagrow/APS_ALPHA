#pragma once
namespace APSPlanetCloudHlsl
{
// Keep each wide literal below MSVC's per-literal byte limit (C2026).
inline const TCHAR* FieldCode() { return TEXT(R"HLSL(
// Back faces only: one volume pass both outside and inside the bounds sphere.
// Mode 1 covers both sides and can conceal missing back-face coverage. Mode 5
// isolates the exact raster coverage used by the ordinary volume integral.
if (Debug > 4.5 && Debug < 5.5) return Face < 0 ? float4(1,0,1,.5) : float4(0,0,0,0);
if (Debug > .5 && Debug < 1.5) return float4(1,0,1,.5);
if ((Face > 0 && (Debug < .5 || Debug > 5.5)) || Visibility <= 0) return float4(0,0,0,0);
struct CloudField
{
    float gradient(float3 cell, float3 delta)
    {
        // Integer lattice hashing is invariant to floating-point reassociation.
        // Gradients avoid value-noise plateaus that read as square cloud tiles.
        uint3 c=asuint(int3(cell));
        uint h=c.x*1597334677u ^ c.y*3812015801u ^ c.z*2798796415u;
        h^=h>>16; h*=2246822519u; h^=h>>13; h&=15u;
        float u=h<8u?delta.x:delta.y;
        float v=h<4u?delta.y:((h==12u||h==14u)?delta.x:delta.z);
        return ((h&1u)==0u?u:-u)+((h&2u)==0u?v:-v);
    }
    float noise(float3 p)
    {
        float3 i=floor(p), d=frac(p), f=d*d*d*(d*(d*6-15)+10);
        return .5+.5*lerp(lerp(lerp(gradient(i,d),gradient(i+float3(1,0,0),d-float3(1,0,0)),f.x),
            lerp(gradient(i+float3(0,1,0),d-float3(0,1,0)),gradient(i+float3(1,1,0),d-float3(1,1,0)),f.x),f.y),
            lerp(lerp(gradient(i+float3(0,0,1),d-float3(0,0,1)),gradient(i+float3(1,0,1),d-float3(1,0,1)),f.x),
            lerp(gradient(i+float3(0,1,1),d-float3(0,1,1)),gradient(i+float3(1,1,1),d-float3(1,1,1)),f.x),f.y),f.z);
    }
    float3 rotateDomain(float3 p)
    {
        return float3(dot(p,float3(0,.8,.6)),dot(p,float3(-.8,.36,-.48)),dot(p,float3(-.6,-.48,.64)));
    }
    float bodyThreshold(float h, float weatherBody)
    {
        float heightDistance=abs((h-.46)/lerp(.46,.54,step(.46,h)));
        return lerp(.68,.43,weatherBody)+.48*heightDistance*heightDistance;
    }
    float3 advect(float3 p, float2 rotation)
    {
        return float3(rotation.x*p.x-rotation.y*p.y,rotation.y*p.x+rotation.x*p.y,p.z);
    }
    float3 vortex(float3 n,float3 axis,float amount)
    {
        // Compact, smooth spherical rotation: no longitude seam, polar pinch
        // or tiled cyclone stamps. Zero wind/storm strength is exactly identity.
        float support=1.-smoothstep(0.,.20,1.-dot(n,axis));
        float angle=amount*support*support, s=sin(angle), c=cos(angle);
        return n*c+cross(axis,n)*s+axis*dot(axis,n)*(1.-c);
    }
    float weather(float3 n,float3 seed,float featureSize,float banding,float swirl)
    {
        float3 axis=normalize(float3(.7+seed.x*.006,.4+seed.y*.006,.65));
        n=vortex(n,axis,3.5*swirl);
        n=vortex(n,normalize(float3(-axis.y,axis.x,-.6)),-2.8*swirl);
        // Broad connected fronts with low-frequency domain warping. Small noise
        // erodes the edges, it no longer determines a disconnected island map.
        float frequency=5./max(featureSize,.25);
        float warp=noise(rotateDomain(n*2.7)+seed);
        float3 q=n*frequency+float3(1.8*n.z*n.z,1.1*n.x*n.z,0)+seed;
        q+=float3(warp-.5,(warp-.5)*.7,0);
        float broad=noise(q), front=noise(float3(q.xy*.45,q.z*2.3));
        float detail=noise(rotateDomain(q)*2.1+float3(17,3,41));
        return lerp(broad,front,saturate(banding)*.65)*.88+detail*.12;
    }
)HLSL") TEXT(R"HLSL(
    float normalCDF(float z, float gaussian)
    {
        float t=rcp(1.+.2316419*abs(z));
        float tail=.3989422804*gaussian*t*(.319381530+t*(-.356563782
            +t*(1.781477937+t*(-1.821255978+t*1.330274429))));
        return z>=0. ? 1.-tail : tail;
    }
    float expectedDensity(float mean, float sigma, float threshold)
    {
        if(sigma<.001) return saturate((mean-threshold)*7.);
        // Integrate the CLAMPED density, not its input noise. The V19 three
        // point rule returned exactly zero above threshold .60, although the
        // physical billow field has mean density .065 at .60 and .010 at .68.
        // Such a truncated tail cuts distant cloud tops into a hard layer.
        float a=(threshold-mean)/sigma, b=(threshold+1./7.-mean)/sigma;
        float ea=exp(-.5*a*a), eb=exp(-.5*b*b);
        float ca=normalCDF(a,ea), cb=normalCDF(b,eb);
        return saturate(7.*((mean-threshold)*(cb-ca)
            +sigma*.3989422804*(ea-eb))+1.-cb);
    }
    float filteredDensity(float3 p, float3 seed, float footprint, float threshold, float frequencyScale)
    {
        // The weather-size control must scale physical billows as well as
        // fronts. Transform BOTH coordinates and their integration/pixel
        // footprint; otherwise larger billows are blurred at the old cutoff.
        // Seed is applied afterwards, so changing size is not a seed change.
        p*=frequencyScale;
        footprint*=frequencyScale;
        // Fade unresolved octave detail by half a noise-domain unit per pixel;
        // expectedDensity below retains the variance removed by this filter.
        float coarseWeight=1.-smoothstep(.25,.5,footprint*.32);
        float fineWeight=1.-smoothstep(.25,.5,footprint*.93);
        float coarse=.5, fine=.5;
        if(coarseWeight>.001) coarse=noise(p*.32+seed);
        if(fineWeight>.001) fine=noise(rotateDomain(p*.93)+seed);
        float mean=.5+.72*coarseWeight*(coarse-.5)+.28*fineWeight*(fine-.5);
        // Variance lost by each low-pass octave must survive the nonlinear
        // threshold. .135 is the measured single-octave gradient-noise sigma.
        // Full resolution is exactly the original two-octave density field.
        float sigma=.135*sqrt(max(0.,.5184*(1.-coarseWeight*coarseWeight)
            +.0784*(1.-fineWeight*fineWeight)));
        return expectedDensity(mean,sigma,threshold);
    }
    float2 sphere(float3 o, float3 d, float radius)
    {
        float b=dot(o,d), l=length(o), c=(l-radius)*(l+radius), h=b*b-c;
        if(h<0) return float2(-1,-1);
        float s=sqrt(h); return float2(-b-s,-b+s);
    }
    float viewDepth(float deviceZ, float orthoDepth)
    {
        if(IsOrthoProjection(ResolvedView)) return orthoDepth;
        // CalcSceneDepth adds 1e-8 to reciprocal Z. At planetary distances it
        // pulls the opaque surface tens of km forward. Reconstruct directly
        // from projection, retaining opaque ship/terrain occlusion and sky.
        float denominator=deviceZ-ResolvedView.ViewToClip[2][2];
        return denominator>0 ? ResolvedView.ViewToClip[3][2]/denominator : 1.e30;
    }
    void foregroundAir(float3 o, float3 d, float3 sun, float distance,
        float radius, float height, float2 scaleHeights, float3 rayleigh,
        float3 mie, float3 ozone, float opacity, float particulate, float intensity,
        float3 fill, float eye, out float3 transmission, out float3 radiance)
    {
        transmission=1.; radiance=0.;
        float2 hit=sphere(o,d,radius+height);
        float begin=max(0.,hit.x), end=min(distance,hit.y);
        if(end<=begin) return;
        // Bounded partial version of the CURRENT AtmoScape optical model.
        // Exported MM_PlanetaryAtmo: same RGB coefficients, phase functions,
        // particulate extinction, fill and multi-scatter colour. No sky writes.
        // Twelve view samples, at most sixteen light samples; one evaluation
        // per cloudy pixel, not another atmosphere march per cloud sample.
        const int Count=12;
        float ds=(end-begin)/float(Count);
        float lightStep=2.*sqrt(max(0.,height*(2.*radius+height)))/16.;
        float mu=clamp(dot(d,sun),-1.,1.), g=.76;
        float phaseR=3./(16.*3.14159265359)*(1.+mu*mu);
        float phaseM=3./(10.*3.14159265359)*((1.-g*g)*(1.+mu*mu))
            /((2.+g*g)*pow(max(.001,1.+g*g-2.*g*mu),2.9));
        float2 depth=0.; float3 scatter=0.;
        [loop] for(int i=0;i<Count;i++)
        {
            float3 p=o+d*(begin+(i+.5)*ds);
            float h=max(0.,length(p)-radius);
            float2 segment=exp(-h/max(scaleHeights,float2(.05,.05)))*ds;
            float2 lightDepth=0.;
            [loop] for(int j=0;j<16;j++)
            {
                float lh=length(p+sun*((j+.5)*lightStep))-radius;
                // The accepted sky suppresses the night side through the
                // optical depth below ground. Explicit shadow avoids overflow.
                if(lh<0.) { lightDepth=float2(1.e6,1.e6); break; }
                lightDepth+=exp(-lh/max(scaleHeights,float2(.05,.05)))*lightStep;
            }
            depth+=segment;
            float3 attenuation=exp(-min(float3(80,80,80),
                ozone*(depth.x+lightDepth.x)+mie*(1.1*particulate)*(depth.y+lightDepth.y)));
            scatter+=attenuation*(rayleigh*segment.x*phaseR+mie*segment.y*phaseM);
        }
        transmission=exp(-min(float3(80,80,80),ozone*depth.x+mie*(1.1*particulate)*depth.y));
        float airAlpha=1.-exp(-min(80.,(depth.x+depth.y)*opacity/(.6*max(radius,1.))));
        float ratio=height/max(radius,1.);
        float multi=smoothstep(0.,1.,dot(sun,normalize(o))+.19+3.*ratio)/max(50.*ratio,.001);
        // HSV S/2.5 and V*20 from the accepted graph, expressed in RGB.
        float3 diffuse=lerp(max(scatter.x,max(scatter.y,scatter.z)).xxx,scatter,.4)*20.;
        radiance=(scatter*opacity*intensity+(fill+diffuse*multi)*airAlpha)/max(eye,.0001);
    }
};
CloudField F;
)HLSL"); }
inline const TCHAR* RayCode() { return TEXT(R"HLSL(
// Subtract in LWC, then demote in kilometres, never subtract large float worlds.
FDFVector3 Rel=DFSubtract(DFNegate(ResolvedView.PreViewTranslation),WSToDF(LWCCenter));
float3 WorldO=DFDemote(DFDivide(Rel,max(CmPerKm,1.e-8)));
float3 O=float3(dot(WorldO,AxisX),dot(WorldO,AxisY),dot(WorldO,AxisZ));
// The bounds triangles must not define a volume's ray directions. Reconstruct
// the per-pixel ray from the resolved view, at a well-conditioned near depth.
float3 WorldD=IsOrthoProjection(ResolvedView) ? -normalize(CameraVector)
    : normalize(SvPositionToResolvedTranslatedWorld(float4(Parameters.SvPosition.xy,.5,1)));
float3 D=float3(dot(WorldD,AxisX),dot(WorldD,AxisY),dot(WorldD,AxisZ));
float RayFootprint=max(length(ddx(D)),length(ddy(D)));
if(Debug>5.5 && Debug<6.5) return float4(D*.5+.5,1);
)HLSL"); }
inline const TCHAR* SingleLayerCode() { return TEXT(R"HLSL(float Inner=Radius+Bottom, Outer=Inner+Thickness;
// Offline depth probe: rows show camera radius, central entry, central scene
// depth (all km), then the engine's guarded depth. No production cost at Debug=0.
if(Debug>3.5 && Debug<4.5)
{
    float3 V=normalize(ResolvedView.ViewForward);
    float3 VD=float3(dot(V,AxisX),dot(V,AxisY),dot(V,AxisZ));
    float2 PH=F.sphere(O,VD,Outer);
    float2 UV=ViewportUVToBufferUV(float2(.5,.5));
    float Guarded=CalcSceneDepth(UV);
    float S=F.viewDepth(LookupDeviceZ(UV),Guarded)/max(CmPerKm,1.e-8);
    float2 Cell=(Parameters.SvPosition.xy-float2(600,160))/float2(64,128);
    int Row=(int)floor(Cell.y), Col=(int)floor(Cell.x);
    float Value=Row==0?length(O):(Row==1?max(0,PH.x):(Row==2?S:Guarded/max(CmPerKm,1.e-8)));
    if(Row<0||Row>3||Col<0||Col>5) return float4(0,0,0,1);
    int Number=(int)clamp(floor(Value+.5),0.,999999.);
    int Divisors[6]={100000,10000,1000,100,10,1};
    int Digit=(Number/Divisors[Col])%10;
    int Masks[10]={63,6,91,79,102,109,125,7,127,111}; int Bits=Masks[Digit];
    float2 G=frac(Cell); bool Stroke=false;
    if(G.x>.15&&G.x<.75)
    { if(G.y<.10)Stroke=(Bits&1)!=0; if(abs(G.y-.43)<.05)Stroke=(Bits&64)!=0; if(G.y>.76&&G.y<.86)Stroke=(Bits&8)!=0; }
    if(G.x<.15&&G.y>.05&&G.y<.43)Stroke=(Bits&32)!=0;
    if(G.x<.15&&G.y>.43&&G.y<.81)Stroke=(Bits&16)!=0;
    if(G.x>.75&&G.x<.9&&G.y>.05&&G.y<.43)Stroke=(Bits&2)!=0;
    if(G.x>.75&&G.x<.9&&G.y>.43&&G.y<.81)Stroke=(Bits&4)!=0;
    return float4(Stroke?float3(1,1,1):float3(0,0,0),1);
}
float2 OuterHit=F.sphere(O,D,Outer), InnerHit=F.sphere(O,D,Inner);
float Start=max(0,OuterHit.x), End=OuterHit.y;
if(End<=Start) return Debug>1.5 ? float4(1,0,0,.5) : float4(0,0,0,0);
if(length(O)<Inner) Start=max(Start,InnerHit.y);
else if(InnerHit.x>Start) End=min(End,InnerHit.x);
if(End<=Start) return Debug>1.5 ? float4(0,1,1,.5) : float4(0,0,0,0);
// Opaque ground/ships occlude in ray distance, also when the camera is IN cloud.
float RawDepth=LookupDeviceZ(ScreenAlignedPosition(GetScreenPosition(Parameters)));
float SceneRay=F.viewDepth(RawDepth,SceneZ)/max(dot(WorldD,ResolvedView.ViewForward),.001)/max(CmPerKm,1.e-8);
if(Debug<2.5 || Debug>5.5) End=min(End,SceneRay);
if(End<=Start) return Debug>1.5 ? float4(0,0,1,.5) : float4(0,0,0,0);
float2 Ground=F.sphere(O,D,Radius);
if(Ground.x>0) End=min(End,Ground.x);
if(End<=Start) return Debug>1.5 ? float4(1,1,0,.5) : float4(0,0,0,0);
if(Debug>1.5 && Debug<2.5) return float4(0,1,0,.5);
if(Debug>6.5 && Debug<7.5) return float4(saturate((End-Start)/max(Thickness,.001)).xxx,1);
)HLSL") TEXT(R"HLSL(
// Bounded work; identical field at all distances, no shell/volume double blend.
// Grazing rays cross a much longer segment. Fixed, phase-aligned midpoint
// samples exposed the layer as horizontal bands in the V12 horizon capture.
// Keep the original16 samples at normal incidence, cap grazing work at32,
// and de-correlate the integration phase per pixel without changing weather.
int Samples=(int)clamp(ceil((End-Start)/max(Thickness,.001)*16.),16.,32.);
uint2 Pixel=uint2(Parameters.SvPosition.xy);
uint Phase=Pixel.x*1597334677u ^ Pixel.y*3812015801u;
Phase^=Phase>>16; Phase*=2246822519u; Phase^=Phase>>13;
// Keep decorrelation even at normal incidence: removing it exposed concentric
// integration bands under sparse clouds in the V23 Oasis ground capture.
float SamplePhase=.05+.90*float(Phase&65535u)/65535.;
// Keep near billows resolved even on a214km grazing ray. Uniform6.7km
// intervals made the filter replace NEAR density with its statistical mean,
// manufacturing a solid horizontal fog stripe inside an otherwise clear cell.
// Geometric intervals retain the same16-32 sample budget and exact ray extent.
float RayLength=End-Start, DistributionScale=max(Thickness*.5,.05);
float Growth=pow(1.+RayLength/DistributionScale,1./float(Samples));
float NextStep=DistributionScale*(Growth-1.);
// Avoid cancellation on sub-metre tangent intervals; their uniform limit is1.
if(Growth<1.00001) { Growth=1.; NextStep=RayLength/float(Samples); }
float NextStart=Start, Trans=1., CloudAlpha=0., CloudDistance=0.; float3 Sum=0;
// Kilometre-space extinction must describe a cloud, not a translucent fog bank.
// At typical density .2, the old1.35/km transmitted76% through a full km.
// 8/km transmits20%; use the identical coefficient on camera and sunlight rays.
const float ExtinctionPerKm=8.*max(CloudDensity,0.);
// Match the model's [.25,3] range, with exactly the accepted field at1.
// Shared by view and light taps; no extra noise octaves or march samples.
float BillowFrequencyScale=rcp(clamp(WeatherScale,.25,3.));
[loop] for(int k=0;k<Samples;k++)
{
    float Step=max(0.,min(NextStep,End-NextStart));
    float SampleDistance=NextStart+SamplePhase*Step;
    NextStart+=Step; NextStep*=Growth;
    float3 P=O+D*SampleDistance;
    float r=length(P), h=saturate((r-Inner)/Thickness);
    float3 N=P/max(r,1.);
    float3 WeatherN=F.advect(N,WindRotation.xy);
    float Weather=F.weather(WeatherN,SeedOffset,WeatherScale,Banding,Swirl);
    // A broad shoulder supplies wisps between a clear region and an opaque
    // front. Do not force equal opacity into every small thresholded patch.
    float Threshold=lerp(.80,.18,saturate(Coverage));
    float Shape=smoothstep(Threshold-.10,Threshold+.18,Weather);
    // Subpixel billows converge to their mean, not a new map. Evaluate ray
    // derivatives outside the divergent loop, then filter in physical km.
    // Filter in both screen footprint AND integration footprint: a long
    // horizon segment cannot resolve kilometre-sized billows with32 steps.
    float Footprint=max(RayFootprint*SampleDistance,Step*.5);
    float Meso=F.noise(F.rotateDomain(WeatherN*(32./max(WeatherScale,.25)))+SeedOffset);
)HLSL") TEXT(R"HLSL(
    // V15: height erodes the iso-density body BEFORE thresholding. Multiplying
    // a pre-existing body by a narrow vertical fade cut every billow into the
    // same slab in V14; increasing detail alone could never round its contours.
    // Do not double this envelope into saturation: V22 flattened whole weather
    // fronts to the same maximum density once subpixel billows were filtered.
    // Keep their continuous thickness variation at orbit AND ground scales.
    float WeatherBody=saturate(Shape+.22*(Meso-.5));
    float BodyThreshold=F.bodyThreshold(h,WeatherBody);
    float Density=F.filteredDensity(F.advect(P,WindRotation.xy),SeedOffset,Footprint,BodyThreshold,BillowFrequencyScale)*smoothstep(0.,.20,Shape);
    if(Density<.0001) continue;
    float Extinction=1.-exp(-Density*Step*ExtinctionPerKm);
    float3 ToSun=normalize(Sun);
    float Day=smoothstep(-.08,.24,dot(N,ToSun));
    // Two bounded light samples inside the SAME density field. V15 only used
    // local density and height, so a billow facing the sun looked like its back.
    // Weather varies over tens of km: hold that envelope over this <=8km ray,
    // but evaluate the physical billows and shell height at both light taps.
    float ShadowDepth=0.;
    float LightLength=min(8.,Thickness*(1.-h)/max(dot(N,ToSun),.12));
    if(Day>.001)
    {
        [unroll] for(int j=0;j<2;j++)
        {
            float3 LP=P+ToSun*(LightLength*(.25+.5*j));
            float LH=(length(LP)-Inner)/Thickness;
            if(LH>0. && LH<1.)
            {
                float LT=F.bodyThreshold(LH,WeatherBody);
                float LD=F.filteredDensity(F.advect(LP,WindRotation.xy),SeedOffset,Footprint,LT,BillowFrequencyScale);
                ShadowDepth+=LD*smoothstep(0.,.20,Shape)*LightLength*.5;
            }
        }
    }
    float SunTransmission=exp(-ShadowDepth*ExtinctionPerKm);
    float3 Light=float3(.008,.011,.018)+SunColor*Day*(.12+.08*h+.90*SunTransmission);
    float Contribution=Trans*Extinction;
    Sum+=Contribution*Light*Albedo; CloudAlpha+=Contribution; Trans*=1.-Extinction;
    CloudDistance+=Contribution*SampleDistance;
    if(Trans<.015) break;
}
float Alpha=CloudAlpha*Visibility;
if(Debug>7.5) return float4(Alpha.xxx,1);
float3 AirTransmission=1., AirRadiance=0.;
if(Aerial>.5 && CloudAlpha>.0001)
    F.foregroundAir(O,D,normalize(Sun),CloudDistance/CloudAlpha,AirRadius,AirHeight,
        float2(AirOptics.z,AirMieHeight),AirRayleigh,AirMie,AirOzone,
        AirSettings.x,AirSettings.y,AirSettings.z,AirFill,EyeAdaptation,
        AirTransmission,AirRadiance);
// Foreground air replaces the part occluded by clouds, exactly once. Cloud
// alpha NEVER includes air transmission. The centroid is a bounded depth
// approximation; it does not alter weather, density, accepted sky or terrain.
return float4((Sum/max(CloudAlpha,.0001))*AirTransmission+AirRadiance,Alpha);
)HLSL"); }
// These shared pieces are also used by the isolated multilayer candidate.
// Concatenation preserves the previous single-layer shader byte-for-byte.
inline FString Code() { return FString(FieldCode())+RayCode()+SingleLayerCode(); }
// Rio 06.10 (clouds vanish at an altitude): V27 keeps ONE interval per ray. A ray
// that dips under the deck base and misses the datum sphere crosses the deck twice.
// Below the base V27 kept the far crossing, at/above it only the near one, so the
// horizon band switched off in one frame at camera radius Radius+Bottom. V33
// marches both crossings with the same 16-32 sample budget and skips the clear gap
// between them. Built from exact splices of the V27 source: Code() above stays
// byte-identical (V27 remains rebuildable) and a changed anchor fails closed.
inline FString SingleLayerTwoCrossingCode()
{
    FString Shader(SingleLayerCode());
    const auto ReplaceOne=[&Shader](const TCHAR* Before,const TCHAR* After)
    { return Shader.ReplaceInline(Before,After,ESearchCase::CaseSensitive)==1; };
    // Gap = clear air between the two crossings, kept in REAL ray distance.
    if(!ReplaceOne(TEXT("float Start=max(0,OuterHit.x), End=OuterHit.y;"),
        TEXT("float Start=max(0,OuterHit.x), End=OuterHit.y, GapAt=1.e30, Gap=0.;"))) return FString();
    if(!ReplaceOne(TEXT("else if(InnerHit.x>Start) End=min(End,InnerHit.x);"),TEXT(R"HLSL(else if(InnerHit.y>Start)
{
    // Rio 06.10 (clouds vanish at an altitude): a dipping ray that misses the
    // datum re-enters the deck at InnerHit.y. Keep both crossings in one march.
    // At/above the base InnerHit.x>=0 exactly; with length(O)==Inner in float it
    // rounds to 0 or -ulp. Near clamps that, so the result equals the below-base
    // interval there instead of one sparse march through the clear gap.
    float Near=max(InnerHit.x,Start);
    float2 Dip=F.sphere(O,D,Radius);
    if(Dip.x>0.||InnerHit.y>=End) End=min(End,Near);
    else { GapAt=Near; Gap=InnerHit.y-Near; }
})HLSL"))) return FString();
    // SceneRay and Ground clamps above compare REAL distances; only then collapse.
    if(!ReplaceOne(TEXT("if(End<=Start) return Debug>1.5 ? float4(1,1,0,.5) : float4(0,0,0,0);"),TEXT(R"HLSL(// Scene/ground before the far crossing re-enters: only the near crossing remains.
if(End<GapAt+Gap) { End=min(End,GapAt); Gap=0.; GapAt=1.e30; }
if(End<=Start) return Debug>1.5 ? float4(1,1,0,.5) : float4(0,0,0,0);)HLSL"))) return FString();
    if(!ReplaceOne(TEXT("return float4(saturate((End-Start)/max(Thickness,.001)).xxx,1);"),
        TEXT("return float4(saturate((End-Start-Gap)/max(Thickness,.001)).xxx,1);"))) return FString();
    // Same 16-32 budget over the cloud length only, never over the clear gap.
    if(!ReplaceOne(TEXT("int Samples=(int)clamp(ceil((End-Start)/max(Thickness,.001)*16.),16.,32.);"),TEXT(R"HLSL(float RayLength=End-Start-Gap;
int Samples=(int)clamp(ceil(RayLength/max(Thickness,.001)*16.),16.,32.);)HLSL"))) return FString();
    if(!ReplaceOne(TEXT("float RayLength=End-Start, DistributionScale=max(Thickness*.5,.05);"),
        TEXT("float DistributionScale=max(Thickness*.5,.05);"))) return FString();
    // NextStart/Step live in the collapsed march coordinate [Start, End-Gap].
    if(!ReplaceOne(TEXT("float Step=max(0.,min(NextStep,End-NextStart));"),
        TEXT("float Step=max(0.,min(NextStep,End-Gap-NextStart));"))) return FString();
    // Back to REAL distance before P, Footprint, light taps and CloudDistance use it.
    if(!ReplaceOne(TEXT("float SampleDistance=NextStart+SamplePhase*Step;"),TEXT(R"HLSL(float SampleDistance=NextStart+SamplePhase*Step;
    SampleDistance+=SampleDistance>GapAt?Gap:0.;)HLSL"))) return FString();
    return Shader;
}
// Empty when an anchor changed; the builder then refuses to create the asset.
inline FString TwoCrossingCode()
{
    const FString Layer=SingleLayerTwoCrossingCode();
    return Layer.IsEmpty()?FString():FString(FieldCode())+RayCode()+Layer;
}
}
