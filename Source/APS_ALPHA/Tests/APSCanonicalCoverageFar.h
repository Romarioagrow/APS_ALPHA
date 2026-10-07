#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "APSFrozenDescentProbe.h"
#include "Misc/CommandLine.h"

// Separate opt-in route, shared by ORIGINAL control and coverage candidate.
// No material/actor setters. The existing harness owns movement, streaming,
// capture and timeout; the ordinary 94-second descent is unchanged.
namespace APSCanonicalCoverageFar
{
    inline constexpr double HeightsKm[] = {30.5, 100.0, 300.0, 1000.0, 300.0, 100.0, 30.5};
    inline constexpr int32 WaypointCount = UE_ARRAY_COUNT(HeightsKm);
    inline constexpr double HoldSeconds = 6.0;
    inline constexpr double LegSeconds = 8.0;
    inline constexpr double DurationSeconds = WaypointCount * HoldSeconds + (WaypointCount - 1) * LegSeconds; // 90s.
    inline constexpr double PitchDegrees = 75.0; // From local tangent, NOT from nadir.

    inline bool Requested()
    {
        const TCHAR* Cursor = FCommandLine::Get(); FString Token;
        while (FParse::Token(Cursor, Token, false))
            if (Token.Equals(TEXT("-APSProbeCanonicalCoverageFar"), ESearchCase::IgnoreCase)
                || Token.StartsWith(TEXT("-APSProbeCanonicalCoverageFar="), ESearchCase::IgnoreCase)) return true;
        return false;
    }

    inline bool Parse(const TCHAR* CommandLine, bool& bRequested, FString& Error)
    {
        bRequested = false; Error.Reset();
        int32 Count = 0; FString Token; const TCHAR* Cursor = CommandLine;
        while (FParse::Token(Cursor, Token, false))
        {
            if (Token.Equals(TEXT("-APSProbeCanonicalCoverageFar"), ESearchCase::IgnoreCase)) ++Count;
            else if (Token.StartsWith(TEXT("-APSProbeCanonicalCoverageFar="), ESearchCase::IgnoreCase))
            { Error = TEXT("CanonicalCoverageFar is a bare flag without a value"); return false; }
        }
        if (!Count) return true;
        if (Count != 1) { Error = TEXT("Duplicate CanonicalCoverageFar flag"); return false; }
        // The harness must enforce unattended editor/Lidim/published/no competing
        // mode guards. CoverageFlight is OPTIONAL: absence is the original control.
        bRequested = true; return true;
    }

    inline APSFrozenDescentProbe::FRouteSample Route(double Elapsed)
    {
        APSFrozenDescentProbe::FRouteSample Out;
        if (!FMath::IsFinite(Elapsed)) return Out;
        const double T = FMath::Clamp(Elapsed, 0.0, DurationSeconds);
        Out.bValid = true; Out.bFinished = Elapsed >= DurationSeconds;
        Out.WaypointIndex = FMath::Min(FMath::FloorToInt(T / (HoldSeconds + LegSeconds)), WaypointCount - 1);
        const double Local = T - Out.WaypointIndex * (HoldSeconds + LegSeconds);
        Out.HeightKm = HeightsKm[Out.WaypointIndex];
        Out.bHold = Local < HoldSeconds || Out.WaypointIndex == WaypointCount - 1;
        if (Out.bHold)
            Out.Phase = Out.bFinished ? TEXT("complete") : Out.WaypointIndex < 3 ? TEXT("far_out_hold")
                : Out.WaypointIndex == 3 ? TEXT("far_apex_hold") : TEXT("far_return_hold");
        else
        {
            const double U = FMath::Clamp((Local - HoldSeconds) / LegSeconds, 0.0, 1.0);
            const double Ease = U * U * (3.0 - 2.0 * U);
            Out.HeightKm = FMath::Exp(FMath::Lerp(FMath::Loge(HeightsKm[Out.WaypointIndex]),
                FMath::Loge(HeightsKm[Out.WaypointIndex + 1]), Ease));
            Out.Phase = Out.WaypointIndex < 3 ? TEXT("far_out") : TEXT("far_return");
        }
        return Out;
    }

    // Use ACTUAL camera cache position minus current root centre and ACTUAL
    // camera forward. Both must be in the same world frame after any origin shift.
    // This is analytic sphere coverage, not proof that GPU terrain was rendered.
    inline bool CenterRayHitsBody(const FVector& RelativeCamera, const FVector& Forward,
        double RadiusCm, double& ImpactRadiusRatio, FString& Error)
    {
        ImpactRadiusRatio = 0.0; Error.Reset();
        const double Distance = RelativeCamera.Size();
        if (RelativeCamera.ContainsNaN() || Forward.ContainsNaN() || !FMath::IsFinite(Distance)
            || !FMath::IsFinite(RadiusCm) || RadiusCm <= 0 || Distance <= RadiusCm
            || !FMath::IsNearlyEqual(Forward.SizeSquared(), 1.0, 1.e-6))
        { Error = TEXT("Canonical far ray has invalid camera/frame/radius"); return false; }
        const double Along = FVector::DotProduct(RelativeCamera, Forward);
        const double ImpactSquared = FMath::Max(0.0, RelativeCamera.SizeSquared() - Along * Along);
        ImpactRadiusRatio = FMath::Sqrt(ImpactSquared) / RadiusCm;
        if (Along >= 0 || !FMath::IsFinite(ImpactRadiusRatio) || ImpactRadiusRatio >= .9)
        { Error = TEXT("Canonical far actual central ray does not safely intersect the body"); return false; }
        return true;
    }
}
#endif
