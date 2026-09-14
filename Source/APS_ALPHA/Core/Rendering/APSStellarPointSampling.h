#pragma once

#include "CoreMinimal.h"

// The point-only material's spatial reconstruction. Physical photospheres and
// their accepted corona material never use this code. Keep individual wide
// literals small enough for MSVC; compose them at runtime.
namespace APSStellarPointSampling
{
inline const TCHAR* VertexFrame = TEXT(R"APSFRAME(
float4x4 instanceToTranslated = DFFastToTranslatedWorld(
    GetInstanceToWorldDF(Parameters), ResolvedView.PreViewTranslation);
float4 centreClip = mul(float4(instanceToTranslated[3].xyz,1.0),
    ResolvedView.TranslatedWorldToClip);
float3 axis = instanceToTranslated[0].xyz;
float axisMax = max(max(abs(axis.x),abs(axis.y)),abs(axis.z));
float uniformScale = axisMax*length(axis/max(axisMax,1.0e-20));
float3 localExtent = GetPrimitiveData(Parameters).InstanceLocalBoundsExtent;
float opticalRadius = max(max(localExtent.x,localExtent.y),localExtent.z)*uniformScale;
float3 fromCamera = instanceToTranslated[3].xyz-ResolvedView.TranslatedWorldCameraOrigin;
float distanceMax = max(max(abs(fromCamera.x),abs(fromCamera.y)),abs(fromCamera.z));
float cameraDistance = distanceMax*length(fromCamera/max(distanceMax,1.0e-20));
bool validPoint = all(isfinite(centreClip)) && isfinite(opticalRadius)
    && isfinite(cameraDistance) && centreClip.w>0.0 && opticalRadius>1.0e-20;
)APSFRAME");

inline FString ProjectionCode()
{
    return FString(VertexFrame) + TEXT(R"APSPROJECT(
if (!validPoint) return float4(0.0,0.0,-1.0,-1.0);
float angularRadiusPixels = opticalRadius/max(cameraDistance,1.0e-20)
    *0.5*ResolvedView.ViewSizeAndInvSize.x*ResolvedView.ViewToClip[0][0]
    /max(ResolvedView.ViewResolutionFraction,1.0e-6);
// An unresolved PSF does not brighten merely by moving towards the frame edge.
// Retain the accepted perspective radius for resolved large halos.
float opticalDistance = lerp(cameraDistance,centreClip.w,smoothstep(4.0,8.0,angularRadiusPixels));
float2 inverseRadiusNDC = (opticalDistance/opticalRadius)
    /float2(ResolvedView.ViewToClip[0][0],ResolvedView.ViewToClip[1][1]);
if (!all(isfinite(inverseRadiusNDC)) || any(inverseRadiusNDC<=0.0))
    return float4(0.0,0.0,-1.0,-1.0);
return float4(centreClip.xy/centreClip.w,inverseRadiusNDC);
)APSPROJECT");
}

inline const TCHAR* CarrierPadding = TEXT(R"APSPADDING(
float3 paddingOffset = float3(0.0,0.0,0.0);
if (validPoint)
{
    // Padding belongs to the raster carrier, not the optical radius or catalogue.
    // Four PRIMARY pixels are conservative when the late pass is native-sized.
    // The extra support prevents a Gaussian apron being cut by coarse triangles.
    float minCarrierRadius = cameraDistance*8.0
        /max(abs(ResolvedView.ViewToClip[0][0])*ResolvedView.ViewSizeAndInvSize.x,1.0);
    float padding = max(3.0,minCarrierRadius/opticalRadius);
    float3 radialOffset;
    #if USE_INSTANCING || USE_INSTANCE_CULLING
        radialOffset = mul(float4(Parameters.InstanceLocalPosition,0.0),instanceToTranslated).xyz;
    #else
        radialOffset = Parameters.WorldPosition-instanceToTranslated[3].xyz;
    #endif
    paddingOffset = radialOffset*(padding-1.0);
    if (!all(isfinite(paddingOffset))) paddingOffset = float3(0.0,0.0,0.0);
}
)APSPADDING");

inline FString CarrierCode()
{
    return FString(VertexFrame) + CarrierPadding + TEXT("\nreturn paddingOffset;\n");
}

inline FString RasterClipCode()
{
    // BasePassVertexShader evaluates interpolators from the pre-WPO parameters.
    // Reproduce precisely the static carrier offset used by the rasterizer.
    return FString(VertexFrame) + CarrierPadding + TEXT(R"APSRASTER(
return mul(float4(Parameters.WorldPosition+paddingOffset,1.0),
    ResolvedView.TranslatedWorldToClip);
)APSRASTER");
}

inline const TCHAR* SpatialFilter = TEXT(R"APSFILTER(
    float2 qDx = ddx(pointQ);
    float2 qDy = ddy(pointQ);
    float maxPixelQSq = max(max(dot(qDx,qDx),dot(qDy,qDy)),1.0e-12);
    float2 sigmaSq = rcp(1.38629436112*float2(coreSharpness,haloSharpness));
    // This is spatial, not temporal: the same point has the same profile at rest
    // and in motion. Broaden only undersampled lobes and compensate their peak.
    float2 pixelVariance = max(1.0/12.0,0.85*0.85-sigmaSq/maxPixelQSq);
    float2 covXX = pixelVariance*(qDx.x*qDx.x+qDy.x*qDy.x);
    float2 covXY = pixelVariance*(qDx.x*qDx.y+qDy.x*qDy.y);
    float2 covYY = pixelVariance*(qDx.y*qDx.y+qDy.y*qDy.y);
    float2 filteredXX = sigmaSq+covXX;
    float2 filteredYY = sigmaSq+covYY;
    float2 determinant = max(filteredXX*filteredYY-covXY*covXY,1.0e-12);
    float2 quadratic = (filteredYY*(pointQ.x*pointQ.x)
        -2.0*covXY*(pointQ.x*pointQ.y)+filteredXX*(pointQ.y*pointQ.y))/determinant;
    float2 filteredLobes = sigmaSq*rsqrt(determinant)*exp(-0.5*quadratic);
    // Conserve the accepted compact lobe's integral, including its smooth cutoff.
    // Integrate the cutoff interval in r^2 with four-point Gauss-Legendre nodes.
    float2 sharpness = float2(coreSharpness,haloSharpness);
    float2 lambda = sharpness*0.69314718056;
    float2 retained = 1.0-exp2(-sharpness*0.36);
    const float4 t = float4(0.0694318442,0.3300094782,0.6699905218,0.9305681558);
    const float4 weights = float4(0.1739274226,0.3260725774,0.3260725774,0.1739274226);
    [unroll] for (int s=0;s<4;++s)
        retained += lambda*0.28*weights[s]*exp2(-sharpness*(0.36+0.28*t[s]))
                    *(1.0-t[s]*t[s]*(3.0-2.0*t[s]));
    float resolvedWeight = smoothstep(4.0,8.0,rsqrt(maxPixelQSq));
    float2 envelope = lerp(retained,float2(edgeFade,edgeFade),resolvedWeight);
    hotCore = lerp(hotCore,filteredLobes.x*envelope.x*projectionValid,rasterProfile);
    softHalo = lerp(softHalo,filteredLobes.y*envelope.y*projectionValid,rasterProfile);
)APSFILTER");

inline bool BuildPointCode(const FString& AcceptedCode, FString& OutCode)
{
    const int32 Start = AcceptedCode.Find(TEXT("    // Spatial prefilter for unresolved points:"));
    const int32 End = AcceptedCode.Find(TEXT("\n}\nfloat seedGain"), ESearchCase::CaseSensitive,
        ESearchDir::FromStart, FMath::Max(Start,0));
    if (Start == INDEX_NONE || End <= Start) return false;
    OutCode = AcceptedCode.Left(Start) + SpatialFilter + AcceptedCode.Mid(End);
    const FString Profile = TEXT("float gameplayProfile = saturate(GameplayPointProfile) * (1.0 - shellMode);");
    if (!OutCode.Contains(Profile)) return false;
    OutCode.ReplaceInline(*Profile, *(Profile + TEXT("\nfloat rasterProfile = 1.0 - shellMode;")));
    // Menu photometry remains profile0; only reconstruction is common to both.
    OutCode.ReplaceInline(TEXT("if (gameplayProfile > 0.0"), TEXT("if (rasterProfile > 0.0"));
    OutCode.ReplaceInline(TEXT("dot(pointQ, pointQ), gameplayProfile"), TEXT("dot(pointQ, pointQ), rasterProfile"));
    OutCode.ReplaceInline(TEXT("roundEnvelope * projectionValid, gameplayProfile"), TEXT("roundEnvelope * projectionValid, rasterProfile"));
    OutCode.ReplaceInline(TEXT("float4 pixelClip = GetScreenPosition(Parameters);"), TEXT("float4 pixelClip = RasterClip;"));
    // The padded carrier may extend beyond2 optical radii. Preserve derivatives
    // throughout the spatial apron; malformed values are still bounded safely.
    OutCode.ReplaceInline(TEXT("pointQ = clamp(candidateQ, -2.0, 2.0);"),
        TEXT("pointQ = clamp(candidateQ, -10000.0, 10000.0);"));
    const FString DepthGuard = TEXT(R"APSDEPTH(
if (!isfinite(SceneDepthForOcclusion) || !all(isfinite(RasterClip)) || RasterClip.w<=0.0)
    return float3(0.0,0.0,0.0);
// UE5.4's late-pass View UB can retain primary resolution. The real interpolated
// clip position works with either primary or native translucent raster targets.
float2 rasterViewportUV = RasterClip.xy/max(RasterClip.w,1.0e-20)*float2(0.5,-0.5)+0.5;
float2 depthUV=ViewportUVToBufferUV(rasterViewportUV);
float sceneDeviceZ=LookupDeviceZ(depthUV);
clip(Parameters.SvPosition.z-sceneDeviceZ);
)APSDEPTH");
    OutCode = DepthGuard + OutCode;
    return true;
}
}
