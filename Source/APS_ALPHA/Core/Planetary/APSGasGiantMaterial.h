#pragma once

#include "CoreMinimal.h"

namespace APSGasGiantMaterial
{
	inline constexpr const TCHAR* CandidateFolder = TEXT("/Game/APS/APS_ALPHA/Diagnostics/GasCloudBelts20261002V2");
	inline constexpr const TCHAR* CandidateName = TEXT("M_APS_GasGiantAtmosphere_V2");
	inline constexpr const TCHAR* CandidatePath = TEXT("/Game/APS/APS_ALPHA/Diagnostics/GasCloudBelts20261002V2/M_APS_GasGiantAtmosphere_V2.M_APS_GasGiantAtmosphere_V2");

#if WITH_EDITOR
	inline FString CandidateCloudCode()
	{
		// Separate short literals avoid MSVC C2026. Three value-noise evaluations,
		// two bounded local rotations; no textures, time, ray march, WPO or emission.
		const FString Noise = TEXT(R"APSGASV2(
struct FGasBeltsNoise
{
    float Hash(float3 p)
    {
        p = frac(p * 0.1031);
        p += dot(p, p.yzx + 33.33);
        return frac((p.x + p.y) * p.z);
    }
    float Noise(float3 p)
    {
        float3 i = floor(p), f = frac(p);
        f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(lerp(Hash(i), Hash(i + float3(1,0,0)), f.x),
                         lerp(Hash(i + float3(0,1,0)), Hash(i + float3(1,1,0)), f.x), f.y),
                    lerp(lerp(Hash(i + float3(0,0,1)), Hash(i + float3(1,0,1)), f.x),
                         lerp(Hash(i + float3(0,1,1)), Hash(i + float3(1,1,1)), f.x), f.y), f.z);
    }
    float3 Curl(float3 q, float3 center, float width, float height, float spin, out float mask)
    {
        // Tangent coordinates are dot products, not longitude/atan2: no seam.
        float3 east = normalize(cross(float3(0,0,1), center));
        float3 north = cross(center, east);
        float2 uv = float2(dot(q, east) / width, dot(q, north) / height);
        float r2 = dot(uv, uv);
        mask = exp(-r2 * 0.72) * smoothstep(0.65, 0.88, dot(q, center));
        float angle = spin * mask;
        float cs = cos(angle), sn = sin(angle);
        // Rotate the cloud coordinates themselves, so belts enter/leave the front.
        return q * cs + cross(center, q) * sn + center * dot(center, q) * (1.0 - cs);
    }
};
FGasBeltsNoise N;
float maxDirection = max(max(abs(DirectionLocal.x), abs(DirectionLocal.y)), abs(DirectionLocal.z));
float3 q = normalize(DirectionLocal / max(maxDirection, 1.0e-30));
float phase = frac(Seed * 0.0618034) * 6.2831853;
float cs = cos(phase), sn = sin(phase);
q.xy = float2(cs * q.x - sn * q.y, sn * q.x + cs * q.y);
float originalLatitude = q.z;
float3 seedOffset = float3(frac(Seed * 0.137), frac(Seed * 0.319), frac(Seed * 0.731)) * 23.0;
)APSGASV2");
		const FString Circulation = TEXT(R"APSGASV2(
// The two stable, separated fronts have seed-dependent latitude, width and spin.
// Their axes stay away from the poles so the tangent basis remains well-defined.
float latitudeA = -0.42 + frac(Seed * 0.017) * 0.36;
float latitudeB = 0.12 + frac(Seed * 0.029) * 0.42;
float longitudeB = 1.9 + frac(Seed * 0.043) * 1.9;
float3 centerA = normalize(float3(1.0, 0.0, latitudeA));
float3 centerB = normalize(float3(cos(longitudeB), sin(longitudeB), latitudeB));
float maskA, maskB;
float spin = (frac(Seed * 0.053) < 0.5 ? -1.0 : 1.0) * (1.6 + frac(Seed * 0.071) * 1.0);
q = N.Curl(q, centerA, 0.22 + frac(Seed * 0.097) * 0.10, 0.105, spin * StormStrength, maskA);
q = N.Curl(q, centerB, 0.16 + frac(Seed * 0.113) * 0.10, 0.075, -spin * 0.78 * StormStrength, maskB);
float broad = N.Noise(q * float3(4.5, 4.5, 10.0) + seedOffset);
float middle = N.Noise(q * float3(16.0, 16.0, 36.0) + seedOffset.yzx);
float detail = N.Noise(q * 59.0 + seedOffset.zxy);
float footprint = max(length(ddx(q)), length(ddy(q)));
float middleWeight = 1.0 - smoothstep(0.025, 0.10, footprint);
float detailWeight = 1.0 - smoothstep(0.006, 0.035, footprint);
middle = lerp(0.5, middle, middleWeight);
detail = lerp(0.5, detail, detailWeight);
// Low-frequency meridional stretching and seeded edge positions give unequal
// belt widths. Neighboring cells share endpoint values, avoiding a cut at frac().
float latitude = q.z + sin(q.z * 2.6 + phase) * 0.09 + sin(q.z * 7.0 - phase) * 0.012;
float beltCoord = latitude * (8.5 + frac(Seed * 0.083) * 2.0)
    + (broad - 0.5) * 0.55 + (middle - 0.5) * 0.17;
float cell = floor(beltCoord), t = frac(beltCoord);
float a = N.Hash(float3(cell, Seed * 0.019, 7.0));
float b = N.Hash(float3(cell + 1.0, Seed * 0.019, 7.0));
float edge = 0.12 + N.Hash(float3(cell, Seed * 0.023, 19.0)) * 0.42;
float width = 0.16 + N.Hash(float3(cell, Seed * 0.031, 29.0)) * 0.22;
float belt = lerp(a, b, smoothstep(edge, edge + width, t));
float beltWeight = 1.0 - smoothstep(0.18, 0.75, fwidth(beltCoord));
belt = lerp(0.5, belt, beltWeight);
)APSGASV2");
		const FString Color = TEXT(R"APSGASV2(
float cloud = 0.50 + (belt - 0.5) * Contrast;
cloud += (broad - 0.5) * 0.16 + (middle - 0.5) * 0.075 + (detail - 0.5) * 0.025;
float3 color = lerp(DarkCloud, LightCloud, smoothstep(0.08, 0.92, saturate(cloud)));
// A modest pigment shift follows the same advected cloud field; there is no
// pasted oval core or repeated bright ring hiding the connected circulation.
float frontPigment = saturate(maskA + maskB * 0.65) * StormStrength;
frontPigment *= 0.12 + smoothstep(0.24, 0.72, broad + (middle - 0.5) * 0.18) * 0.20;
color = lerp(color, StormColor, frontPigment);
color *= 0.96 + middle * 0.07 + (detail - 0.5) * 0.025;
float polarHaze = smoothstep(0.72, 0.98, abs(originalLatitude));
color = lerp(color, lerp(DarkCloud, LightCloud, 0.58), polarHaze * 0.24);
return saturate(color);
)APSGASV2");
		return Noise + Circulation + Color;
	}
#endif
}
