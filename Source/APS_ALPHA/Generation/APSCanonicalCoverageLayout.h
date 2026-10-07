#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

// Pure CPU layout math. Immutable planet anchor; no camera, UObject, cache,
// height evaluation, texture allocation or assertion of resolved bandwidth.
namespace APSCanonicalCoverageLayout
{
    constexpr int ChartResolution = 1024;
    constexpr int MaximumLevels = 9; // 8 only reaches R=62,914.56 km at this base width.
    constexpr double BaseTexelSpacingCm = 1500.0;
    constexpr double BaseWidthCm = ChartResolution * BaseTexelSpacingCm;
    constexpr double MinimumRadiusCm = BaseWidthCm / 4.0;
    constexpr double MaximumRadiusCm = 100000.0 * 100000.0;
    constexpr double FadeStart = .42, FadeEnd = .49;

    struct FVector3 { double X = 0.0, Y = 0.0, Z = 0.0; };
    struct FFrame { FVector3 Center, AxisU, AxisV; }; // Unit, orthogonal, U cross V = C.
    struct FLevel { double WidthCm = 0.0, CenterTexelSpacingCm = 0.0; };
    struct FLayout
    {
        FFrame Frame;
        double RadiusCm = 0.0;
        int Resolution = ChartResolution, Count = 0;
        std::array<FLevel, MaximumLevels> Levels{};
    };
    struct FWeights { int Count = 0; std::array<double, MaximumLevels> Values{}; };

    inline double Dot(FVector3 A, FVector3 B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
    inline FVector3 Scale(FVector3 V, double S) { return {V.X * S, V.Y * S, V.Z * S}; }
    inline FVector3 Add(FVector3 A, FVector3 B) { return {A.X + B.X, A.Y + B.Y, A.Z + B.Z}; }
    inline FVector3 Cross(FVector3 A, FVector3 B)
    { return {A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X}; }
    inline bool Finite(FVector3 V) { return std::isfinite(V.X) && std::isfinite(V.Y) && std::isfinite(V.Z); }
    inline bool Unit(FVector3 V) { return Finite(V) && std::abs(Dot(V, V) - 1.0) <= 1.e-8; }
    inline bool ValidFrame(const FFrame& F)
    {
        const FVector3 W = Cross(F.AxisU, F.AxisV);
        return Unit(F.Center) && Unit(F.AxisU) && Unit(F.AxisV)
            && std::abs(Dot(F.Center, F.AxisU)) <= 1.e-10
            && std::abs(Dot(F.Center, F.AxisV)) <= 1.e-10
            && std::abs(Dot(F.AxisU, F.AxisV)) <= 1.e-10
            && std::abs(W.X - F.Center.X) <= 1.e-10
            && std::abs(W.Y - F.Center.Y) <= 1.e-10
            && std::abs(W.Z - F.Center.Z) <= 1.e-10;
    }
    inline bool ValidLayout(const FLayout& L)
    {
        if (!ValidFrame(L.Frame) || !std::isfinite(L.RadiusCm)
            || L.RadiusCm < MinimumRadiusCm || L.RadiusCm > MaximumRadiusCm
            || L.Resolution != ChartResolution || L.Count < 1 || L.Count > MaximumLevels) return false;
        double Expected = BaseWidthCm;
        for (int I = 0; I < L.Count; ++I)
        {
            if (L.Levels[I].WidthCm != Expected || L.Levels[I].CenterTexelSpacingCm != Expected / L.Resolution)
                return false;
            if (I + 1 == L.Count) return Expected == 4.0 * L.RadiusCm;
            if (Expected >= 4.0 * L.RadiusCm) return false;
            Expected = std::min(Expected * 4.0, 4.0 * L.RadiusCm);
        }
        return false;
    }

    // Empty output only; failure leaves Out unchanged. Last width is exactly4R,
    // without duplicate levels or a terminal decrease in centre texel spacing.
    inline bool Build(double RadiusCm, const FFrame& Frame, FLayout& Out, std::string& Error)
    {
        Error.clear();
        if (Out.Count != 0 || Out.RadiusCm != 0.0)
        { Error = "Coverage layout output must be empty"; return false; }
        if (!ValidFrame(Frame) || !std::isfinite(RadiusCm)
            || RadiusCm < MinimumRadiusCm || RadiusCm > MaximumRadiusCm)
        { Error = "Coverage needs right-handed unit frame and radius3.84..100000km"; return false; }
        FLayout Result; Result.Frame = Frame; Result.RadiusCm = RadiusCm;
        double Width = BaseWidthCm;
        for (;;)
        {
            if (Result.Count == MaximumLevels)
            { Error = "Coverage level cap cannot reach front-hemisphere terminal chart"; return false; }
            Result.Levels[Result.Count++] = {Width, Width / ChartResolution};
            if (Width == 4.0 * RadiusCm) break;
            Width = std::min(Width * 4.0, 4.0 * RadiusCm);
        }
        if (!ValidLayout(Result)) { Error = "Coverage internal layout invariant failed"; return false; }
        Out = Result; return true;
    }

    namespace Private
    {
        // Per-sample helpers require a Build-produced, unchanged layout. Cheap
        // address guards avoid re-walking all levels inside height-worker loops.
        inline bool Level(const FLayout& L, int I)
        {
            return I >= 0 && I < L.Count && L.Count <= MaximumLevels
                && L.Resolution == ChartResolution && std::isfinite(L.RadiusCm)
                && L.RadiusCm >= MinimumRadiusCm && L.RadiusCm <= MaximumRadiusCm
                && std::isfinite(L.Levels[I].WidthCm) && L.Levels[I].WidthCm > 0.0;
        }
        inline bool Coordinates(const FLayout& L, int I, double U, double V, double& SX, double& SY, double& S2)
        {
            if (!Level(L, I) || !std::isfinite(U) || !std::isfinite(V)) return false;
            const double K = L.Levels[I].WidthCm / (2.0 * L.RadiusCm);
            SX = (U - .5) * K; SY = (V - .5) * K; S2 = SX * SX + SY * SY;
            return std::isfinite(S2);
        }
    }

    // Finite halo UVs outside0..1 are supported for producer derivative stencils.
    // Chart square corners may map behind the front hemisphere; that is valid.
    inline bool Direction(const FLayout& L, int I, double U, double V, FVector3& Out)
    {
        double SX, SY, S2;
        if (!Private::Coordinates(L, I, U, V, SX, SY, S2)) return false;
        const double Denom = 1.0 + S2;
        FVector3 D = Add(Scale(L.Frame.Center, (1.0 - S2) / Denom),
            Add(Scale(L.Frame.AxisU, 2.0 * SX / Denom), Scale(L.Frame.AxisV, 2.0 * SY / Denom)));
        const double D2 = Dot(D, D);
        if (!Finite(D) || !std::isfinite(D2) || D2 < 1.e-20) return false;
        Out = Scale(D, 1.0 / std::sqrt(D2)); return true;
    }
    // Inverse returns unbounded UV: fine charts legitimately miss coarse points.
    // Only the antipodal singularity is rejected here, not all back directions.
    inline bool Inverse(const FLayout& L, int I, FVector3 D, double& OutU, double& OutV)
    {
        if (!Private::Level(L, I) || !Unit(D)) return false;
        const double Denom = 1.0 + Dot(D, L.Frame.Center);
        if (!std::isfinite(Denom) || Denom <= 1.e-12) return false;
        const double K = (2.0 * L.RadiusCm / L.Levels[I].WidthCm) / Denom;
        const double U = .5 + K * Dot(D, L.Frame.AxisU), V = .5 + K * Dot(D, L.Frame.AxisV);
        if (!std::isfinite(U) || !std::isfinite(V)) return false;
        OutU = U; OutV = V; return true;
    }
    // Differential physical step on the sphere, not a bandwidth/anti-alias proof.
    inline bool LocalTexelSpacing(const FLayout& L, int I, double U, double V, double& OutCm)
    {
        double SX, SY, S2;
        if (!Private::Coordinates(L, I, U, V, SX, SY, S2)) return false;
        const double Step = L.Levels[I].WidthCm / L.Resolution / (1.0 + S2);
        if (!std::isfinite(Step) || Step <= 0.0) return false;
        OutCm = Step; return true;
    }

    inline double Coverage(double U, double V)
    {
        const double Edge = std::max(std::abs(U - .5), std::abs(V - .5));
        const double T = std::max(0.0, std::min(1.0, (Edge - FadeStart) / (FadeEnd - FadeStart)));
        return 1.0 - T * T * (3.0 - 2.0 * T);
    }
    // Front hemisphere only. Spatial partition, finest first; terminal coverage
    // is1 everywhere on the front, NOT faded at its square edge/limb. No distance
    // or camera-height amplitude multiplier. Output unchanged on failure.
    inline bool Weights(const FLayout& L, FVector3 D, FWeights& Out)
    {
        if (!ValidLayout(L) || !Unit(D) || Dot(D, L.Frame.Center) < -1.e-12) return false;
        FWeights Result; Result.Count = L.Count; double Remaining = 1.0;
        for (int I = 0; I + 1 < L.Count; ++I)
        {
            double U, V; if (!Inverse(L, I, D, U, V)) return false;
            Result.Values[I] = Remaining * Coverage(U, V);
            Remaining -= Result.Values[I];
        }
        Result.Values[L.Count - 1] = Remaining;
        Out = Result; return true;
    }
    // All supplied normals must be in the SAME planet frame, using the same
    // physical heightfield/filter contract. This only defines the blend math.
    inline bool BlendNormals(const FLayout& L, FVector3 D,
        const std::array<FVector3, MaximumLevels>& Normals, FVector3& Out)
    {
        FWeights W; if (!Weights(L, D, W)) return false;
        FVector3 Sum;
        for (int I = 0; I < W.Count; ++I) if (W.Values[I] > 0.0)
        {
            if (!Unit(Normals[I]) || Dot(Normals[I], D) <= 0.0) return false;
            Sum = Add(Sum, Scale(Normals[I], W.Values[I]));
        }
        const double L2 = Dot(Sum, Sum);
        if (!Finite(Sum) || !std::isfinite(L2) || L2 < 1.e-20) return false;
        Out = Scale(Sum, 1.0 / std::sqrt(L2)); return true;
    }
}
