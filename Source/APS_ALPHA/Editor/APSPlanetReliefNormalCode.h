#pragma once

#include <cmath>
#include <limits>

// Isolated lighting-normal code, not an installed material patch.
// Do not connect this output to slope, colour, texture coordinates or coverage.
// UnifiedLava must use it on the rock normal BEFORE its existing shore blend;
// the lava normal and the six shared geometric-normal adapters stay unchanged.
namespace APSPlanetReliefNormalCode
{
    // Custom-expression inputs (all directions in the SAME world frame):
    // AuthoredNormal: the original final rock lighting normal, including detail.
    // BaselineNormal: the EXACT geometric baseline used by that original path.
    // CanonicalNormal: one already filtered, decoded, frame-rotated cache sample.
    // Availability: 0 for a missing/stale/unvalidated physical-height field.
    // RequestedBlend: explicit [0,1] near-to-macro weight supplied by the caller.
    //
    // The caller samples the field once in the PIXEL stage, using footprint only
    // for mip/gradient selection, never as a relief-amplitude multiplier here.
    // No radius, distance, texture encoding or cache-resolution policy is guessed.
    // RequestedBlend=0 or Availability=0 returns AuthoredNormal exactly.
    // Otherwise apply a weighted shortest-arc rotation baseline -> canonical to
    // the ENTIRE authored normal. This transports its detail residual without
    // adding the geometric slope twice or introducing a relief gain.
    // Opposite/invalid directions have no trustworthy unique shortest arc: keep
    // the original output. This safety fallback is NOT cache-validity evidence.
    // ASCII HLSL; an Unreal builder can assign UTF8_TO_TCHAR(RockNormalHLSL()).
    inline const char* RockNormalHLSL()
    {
        return R"HLSL(
if (!isfinite(Availability) || !isfinite(RequestedBlend)) return AuthoredNormal;
float weight = saturate(Availability) * saturate(RequestedBlend);
if (weight <= 0.0) return AuthoredNormal;
if (!all(isfinite(AuthoredNormal)) || !all(isfinite(BaselineNormal))
    || !all(isfinite(CanonicalNormal))) return AuthoredNormal;
float a2 = dot(AuthoredNormal, AuthoredNormal);
float b2 = dot(BaselineNormal, BaselineNormal);
float c2 = dot(CanonicalNormal, CanonicalNormal);
if (min(a2, min(b2, c2)) < 1.e-12 || max(a2, max(b2, c2)) > 1.e12)
    return AuthoredNormal;
float3 baseline = BaselineNormal * rsqrt(b2);
float3 canonical = CanonicalNormal * rsqrt(c2);
float cosine = clamp(dot(baseline, canonical), -1.0, 1.0);
float3 arc = cross(baseline, canonical);
float arc2 = dot(arc, arc);
if (cosine <= -0.999999 || arc2 <= 1.e-20) return AuthoredNormal;
float arcLength = sqrt(arc2);
float3 axis = arc / arcLength;
float angle = atan2(arcLength, cosine) * weight;
float sineAngle, cosineAngle;
sincos(angle, sineAngle, cosineAngle);
float3 authored = AuthoredNormal * rsqrt(a2);
float3 rotated = authored * cosineAngle + cross(axis, authored) * sineAngle
    + axis * dot(axis, authored) * (1.0 - cosineAngle);
float r2 = dot(rotated, rotated);
if (!all(isfinite(rotated)) || !isfinite(r2) || r2 < 1.e-12)
    return AuthoredNormal;
return rotated * rsqrt(r2);
)HLSL";
    }

    // Portable reference for offline contracts. No UObject, RHI or UE headers.
    // Double precision reference is compared to shader float with a tolerance;
    // zero-weight and safety returns deliberately preserve the exact input.
    struct FVector3 { double X, Y, Z; };

    inline double Dot(FVector3 A, FVector3 B)
    { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
    inline FVector3 Cross(FVector3 A, FVector3 B)
    { return {A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X}; }
    inline FVector3 Scale(FVector3 A, double S) { return {A.X * S, A.Y * S, A.Z * S}; }
    inline bool Finite(FVector3 V)
    { return std::isfinite(V.X) && std::isfinite(V.Y) && std::isfinite(V.Z); }
    inline double Clamp(double V, double Low, double High)
    { return V < Low ? Low : (V > High ? High : V); }

    inline FVector3 EvaluateReference(FVector3 AuthoredNormal, FVector3 BaselineNormal,
        FVector3 CanonicalNormal, double Availability, double RequestedBlend)
    {
        if (!std::isfinite(Availability) || !std::isfinite(RequestedBlend)) return AuthoredNormal;
        const double Weight = Clamp(Availability, 0.0, 1.0) * Clamp(RequestedBlend, 0.0, 1.0);
        if (Weight <= 0.0 || !Finite(AuthoredNormal) || !Finite(BaselineNormal)
            || !Finite(CanonicalNormal)) return AuthoredNormal;
        const double A2 = Dot(AuthoredNormal, AuthoredNormal);
        const double B2 = Dot(BaselineNormal, BaselineNormal);
        const double C2 = Dot(CanonicalNormal, CanonicalNormal);
        if (!std::isfinite(A2) || !std::isfinite(B2) || !std::isfinite(C2)
            || A2 < 1.e-12 || B2 < 1.e-12 || C2 < 1.e-12
            || A2 > 1.e12 || B2 > 1.e12 || C2 > 1.e12) return AuthoredNormal;
        const FVector3 Baseline = Scale(BaselineNormal, 1.0 / std::sqrt(B2));
        const FVector3 Canonical = Scale(CanonicalNormal, 1.0 / std::sqrt(C2));
        const double Cosine = Clamp(Dot(Baseline, Canonical), -1.0, 1.0);
        const FVector3 Arc = Cross(Baseline, Canonical);
        const double Arc2 = Dot(Arc, Arc);
        if (Cosine <= -0.999999 || Arc2 <= 1.e-20) return AuthoredNormal;
        const double ArcLength = std::sqrt(Arc2);
        const FVector3 Axis = Scale(Arc, 1.0 / ArcLength);
        const double Angle = std::atan2(ArcLength, Cosine) * Weight;
        const double SineAngle = std::sin(Angle), CosineAngle = std::cos(Angle);
        const FVector3 Authored = Scale(AuthoredNormal, 1.0 / std::sqrt(A2));
        const FVector3 A = Scale(Authored, CosineAngle);
        const FVector3 B = Scale(Cross(Axis, Authored), SineAngle);
        const FVector3 C = Scale(Axis, Dot(Axis, Authored) * (1.0 - CosineAngle));
        const FVector3 Rotated{A.X + B.X + C.X, A.Y + B.Y + C.Y, A.Z + B.Z + C.Z};
        const double R2 = Dot(Rotated, Rotated);
        if (!Finite(Rotated) || !std::isfinite(R2) || R2 < 1.e-12) return AuthoredNormal;
        return Scale(Rotated, 1.0 / std::sqrt(R2));
    }

    inline bool ReferenceContractsPass()
    {
        const auto Same = [](FVector3 A, FVector3 B)
        { return A.X == B.X && A.Y == B.Y && A.Z == B.Z; };
        const auto Close = [](FVector3 A, FVector3 B)
        { return std::abs(A.X - B.X) < 1.e-10 && std::abs(A.Y - B.Y) < 1.e-10
            && std::abs(A.Z - B.Z) < 1.e-10; };
        const FVector3 Z{0.0, 0.0, 1.0}, X{1.0, 0.0, 0.0}, Authored{0.1, 0.2, 0.3};
        const double NaN = std::numeric_limits<double>::quiet_NaN();
        if (!Same(EvaluateReference(Authored, Z, X, 1.0, 0.0), Authored)
            || !Same(EvaluateReference(Authored, Z, X, 0.0, 1.0), Authored)
            || !Same(EvaluateReference(Authored, Z, Z, 1.0, 1.0), Authored)
            || !Same(EvaluateReference(Authored, Z, {NaN, 0.0, 1.0}, 1.0, 1.0), Authored)
            || !Same(EvaluateReference(Authored, Z, X, NaN, 1.0), Authored)
            || !Same(EvaluateReference(Authored, Z, {0.0, 0.0, -1.0}, 1.0, 1.0), Authored)
            || !Same(EvaluateReference(Authored, {0.0, 0.0, 0.0}, X, 1.0, 1.0), Authored)) return false;
        const double Small = 1.e-5;
        if (!Close(EvaluateReference(Z, Z, {std::sin(Small), 0.0, std::cos(Small)}, 1.0, 1.0),
            {std::sin(Small), 0.0, std::cos(Small)})) return false;
        // Residual angle .2 is preserved while the baseline rotates by .6*.5.
        return Close(EvaluateReference({std::sin(.2), 0.0, std::cos(.2)}, Z,
            {std::sin(.6), 0.0, std::cos(.6)}, 1.0, .5), {std::sin(.5), 0.0, std::cos(.5)})
            && Close(EvaluateReference(Z, Z, X, 1.0, 1.0), X);
    }
}
