#pragma once
#include "CoreMinimal.h"

// Diagnostic replacement of the tetrahedral gradient kernel only. No WPO,
// coverage, depth, illumination or global postprocessing change.
namespace APSWaterAnalyticWaves
{
    inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930");
    inline constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930/M_APS_WaterAnalytic.M_APS_WaterAnalytic");
    inline constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterAnalytic20260930/MI_APS_WaterAnalytic.MI_APS_WaterAnalytic");
    // Integer/3 directions preserve both existing fold periods (multiples of3).
    // Every direction has unit length, so WaveScaleCm remains a wavelength.
    inline const FVector Directions[] = {FVector(1,2,2)/3, FVector(-2,1,2)/3,
        FVector(2,-2,1)/3, FVector(2,1,-2)/3};
    inline constexpr double Weights[] = {0.52, 0.27, 0.14, 0.07};
    inline constexpr double Phases[] = {0.21, 1.43, 2.71, 4.07};
    inline FVector4 Evaluate(const FVector& P)
    {
        FVector Gradient = FVector::ZeroVector; double Height = 0;
        for (int32 I = 0; I < 4; ++I)
        {
            const double Phase = UE_DOUBLE_TWO_PI * FVector::DotProduct(P, Directions[I]) + Phases[I];
            Height += Weights[I] * FMath::Sin(Phase);
            Gradient += Directions[I] * (UE_DOUBLE_TWO_PI * Weights[I] * FMath::Cos(Phase));
        }
        return FVector4(Gradient, Height);
    }
    inline FString Hlsl()
    {
        FString Code(TEXT("float3 G = float3(0,0,0); float H = 0;\n"));
        for (int32 I = 0; I < 4; ++I)
        {
            const FVector Integer = Directions[I] * 3.0;
            Code += FString::Printf(TEXT("{ float3 K = float3(%.0f,%.0f,%.0f) / 3.0; float S,C; sincos(6.28318530718 * dot(P,K) + %.9f, S,C); H += %.9f * S; G += (6.28318530718 * %.9f * C) * K; }\n"),
                Integer.X, Integer.Y, Integer.Z, Phases[I], Weights[I], Weights[I]);
        }
        return Code + TEXT("return float4(G,H);");
    }
}
