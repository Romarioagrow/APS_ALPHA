#pragma once
#include "APSPlanetSurfaceProfile.h"

// Presentation only. No new save fields, terrain seed or atmosphere overrides.
namespace APSPlanetCloudPolicy
{
inline constexpr const TCHAR* MaterialPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/CloudVolume20261001V24/M_APS_PlanetCloud.M_APS_PlanetCloud");
// Decode committed sky RGB into physical cloud units. A coefficient expressed
// per displayed centimetre can legitimately exceed100 on a tiny preview body.
// Clamping BEFORE unit conversion made its cloud optics scale-dependent.
inline bool ReadAirColor(const FLinearColor& Value, float Factor, FLinearColor& Result)
{
    if (!FMath::IsFinite(Factor) || Factor < 0 || !FMath::IsFinite(Value.R)
        || !FMath::IsFinite(Value.G) || !FMath::IsFinite(Value.B)) return false;
    const FLinearColor Converted(FMath::Max(0.f,Value.R)*Factor,
        FMath::Max(0.f,Value.G)*Factor,FMath::Max(0.f,Value.B)*Factor,0);
    if (!FMath::IsFinite(Converted.R) || !FMath::IsFinite(Converted.G)
        || !FMath::IsFinite(Converted.B)) return false;
    Result=Converted;
    return true;
}
struct FLayer
{
    bool Enabled = false;
    float BottomKm = 0, ThicknessKm = 0, Coverage = 0;
    FVector Offset = FVector::ZeroVector;
};
inline FVector SunDirection(const FVector& PhysicalStar, const FVector& PhysicalBody, const FQuat& BodyRotation)
{
    if(PhysicalStar.ContainsNaN() || PhysicalBody.ContainsNaN() || BodyRotation.ContainsNaN()) return FVector::ZeroVector;
    // The atmosphere's displayed centre is camera-local in menu previews.
    // Never subtract it from the star's physical-world position.
    return BodyRotation.UnrotateVector((PhysicalStar-PhysicalBody).GetSafeNormal());
}
inline FLayer Resolve(const FAPSResolvedPlanetSurfaceProfile& P, double AtmosphereKm, bool Manual)
{
    FLayer L;
    // First rollout is water condensate only; no pretend water clouds on lava,
    // ammonia, vacuum or ice. Expanding families requires separate rendered tests.
    if (Manual || (P.PlanetType != EPlanetType::Terrestrial && P.PlanetType != EPlanetType::Oasis
        && P.PlanetType != EPlanetType::Water) || P.LiquidType != EAPSPlanetLiquidType::Water
        || !FMath::IsFinite(AtmosphereKm) || AtmosphereKm < 12
        || !FMath::IsFinite(P.Humidity) || P.Humidity < .25f
        || !FMath::IsFinite(P.AtmosphericPressure) || P.AtmosphericPressure < .1f
        || !FMath::IsFinite(P.Temperature) || P.Temperature < .20f || P.Temperature > .76f) return L;
    L.Enabled = true;
    L.BottomKm = FMath::Min(float(AtmosphereKm * .20), 6.f);
    L.ThicknessKm = FMath::Clamp(1.2f + P.Humidity * 1.8f, 1.2f, 3.f);
    L.Coverage = FMath::Clamp(.25f + P.Humidity * .35f, .30f, .60f);
    FRandomStream R(P.BiomeSeed ^ 0x2A43F91);
    L.Offset = FVector(R.FRandRange(-50,50), R.FRandRange(-50,50), R.FRandRange(-50,50));
    return L;
}
// No distance-driven field/seed substitution. Only fade subpixel planets out.
inline float Visibility(double RadiusCm, double DistanceCm)
{
    if (!FMath::IsFinite(RadiusCm) || !FMath::IsFinite(DistanceCm) || RadiusCm <= 0 || DistanceCm < 0) return 0;
    const double AngularRadius = RadiusCm / FMath::Max(RadiusCm, DistanceCm);
    const double T = FMath::Clamp((AngularRadius - .0005) / .0015, 0., 1.);
    return float(T*T*(3.-2.*T));
}
}
