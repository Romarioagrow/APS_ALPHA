#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Misc/Parse.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

// Opt-in automation fixture only. Never patches resolved profiles, assets,
// materials, terrain payloads, user saves or production cameras. The separate
// atmosphere-boundary mode changes only the transient model's shell height.
namespace APSFrozenDescentProbe
{
    struct FReference
    {
        FString Name;
        int32 SurfaceSeed = 0;
        double RadiusKm = 0.0;
        int32 RoundedNoiseScale = 0;
        int32 RoundedNoiseIntensity = 0;
    };

    inline bool TryResolveReference(const FString& Name, FReference& Out, FString& Error)
    {
        Out = {};
        Error.Reset();
        if (Name.Equals(TEXT("Lidim"), ESearchCase::IgnoreCase))
            Out = {TEXT("Lidim"), 257455, 1280.896, 683, 908291};
        else if (Name.Equals(TEXT("Jaim"), ESearchCase::IgnoreCase))
            Out = {TEXT("Jaim"), 597932, 500.3573, 562, 882510};
        else
        {
            Error = FString::Printf(TEXT("Frozen descent reference '%s' is invalid; expected Lidim or Jaim"), *Name);
            return false;
        }
        return true;
    }

    inline bool ValidateReference(const FReference& Reference, FString& Error)
    {
        FReference Canonical;
        if (!TryResolveReference(Reference.Name, Canonical, Error)) return false;
        if (!FMath::IsFinite(Reference.RadiusKm))
        {
            Error = FString::Printf(TEXT("Frozen descent reference %s has non-finite radiusKm"), *Reference.Name);
            return false;
        }
        if (Reference.SurfaceSeed != Canonical.SurfaceSeed
            || Reference.RadiusKm != Canonical.RadiusKm
            || Reference.RoundedNoiseScale != Canonical.RoundedNoiseScale
            || Reference.RoundedNoiseIntensity != Canonical.RoundedNoiseIntensity)
        {
            Error = FString::Printf(TEXT("Frozen descent reference %s was modified: seed=%d radiusKm=%.17g noise=%d/%d; expected seed=%d radiusKm=%.17g noise=%d/%d"),
                *Reference.Name, Reference.SurfaceSeed, Reference.RadiusKm,
                Reference.RoundedNoiseScale, Reference.RoundedNoiseIntensity,
                Canonical.SurfaceSeed, Canonical.RadiusKm,
                Canonical.RoundedNoiseScale, Canonical.RoundedNoiseIntensity);
            return false;
        }
        return true;
    }

    // Absence succeeds with bRequested=false. Malformed/duplicate requests fail
    // with bRequested=true; callers must not silently fall back to another probe.
    inline bool ParseCommandLine(const TCHAR* CommandLine, FReference& Out,
        bool& bRequested, FString& Error)
    {
        Out = {};
        bRequested = false;
        Error.Reset();
        const TCHAR* Cursor = CommandLine ? CommandLine : TEXT("");
        FString Token;
        FReference Parsed;
        while (FParse::Token(Cursor, Token, false))
        {
            const bool bBare = Token.Equals(TEXT("-APSProbeFrozenDescent"), ESearchCase::IgnoreCase);
            const bool bValue = Token.StartsWith(TEXT("-APSProbeFrozenDescent="), ESearchCase::IgnoreCase);
            if (!bBare && !bValue) continue;
            if (bRequested)
            {
                Error = TEXT("Frozen descent flag must occur exactly once");
                return false;
            }
            bRequested = true;
            if (bBare)
            {
                Error = TEXT("Frozen descent flag requires =Lidim or =Jaim");
                return false;
            }
            if (!TryResolveReference(Token.Mid(FCString::Strlen(TEXT("-APSProbeFrozenDescent="))), Parsed, Error))
                return false;
        }
        if (bRequested) Out = Parsed;
        return true;
    }

    inline bool ConfigureModel(UGeneratedWorld& Model, const FReference& Reference, FString& Error)
    {
        if (!ValidateReference(Reference, Error)) return false;
        Model.PlanetType = EPlanetType::Frozen;
        Model.PlanetSurfaceSeed = Reference.SurfaceSeed;
        Model.PlanetRadius = Reference.RadiusKm;
        Model.SurfaceFeatureScale = 1.0;
        Model.SurfaceReliefScale = 1.0;
        Model.SurfaceLandCoverageScale = 1.0;
        Model.SurfaceMountainScale = 1.0;
        Model.SurfaceCraterScale = 1.0;
        Model.SurfaceRoughnessScale = 1.0;
        UE_LOG(LogTemp, Display, TEXT("[APS.FrozenDescent.Fixture] reference=%s type=Frozen seed=%d radiusKm=%.17g ordinary model inputs, all surface multipliers=1; expected runtime rounded noise=%d/%d, not injected; BODY FIXTURE ONLY, not exact save/camera/atmosphere replay; user saves untouched"),
            *Reference.Name, Reference.SurfaceSeed, Reference.RadiusKm,
            Reference.RoundedNoiseScale, Reference.RoundedNoiseIntensity);
        return true;
    }

    // Separate opt-in: the established descent fixture is deliberately unchanged.
    // This mode reproduces production moon shell HEIGHT, not moon subtype tint,
    // climate, lighting, a saved world, or the user's camera/ship trajectory.
    inline bool ParseAtmosphereBoundaryCommandLine(const TCHAR* CommandLine,
        bool& bRequested, FString& Error)
    {
        bRequested = false;
        Error.Reset();
        bool bDefaultAtmosphere = false;
        const TCHAR* Cursor = CommandLine ? CommandLine : TEXT("");
        FString Token;
        while (FParse::Token(Cursor, Token, false))
        {
            bDefaultAtmosphere |= Token.Equals(TEXT("-APSProbeDefaultAtmosphere"), ESearchCase::IgnoreCase);
            const bool bBare = Token.Equals(TEXT("-APSProbeFrozenAtmosphereBoundary"), ESearchCase::IgnoreCase);
            const bool bValue = Token.StartsWith(TEXT("-APSProbeFrozenAtmosphereBoundary="), ESearchCase::IgnoreCase);
            if (!bBare && !bValue) continue;
            if (bRequested || bValue)
            {
                bRequested = true;
                Error = TEXT("Atmosphere boundary flag must occur once and have no value");
                return false;
            }
            bRequested = true;
        }
        if (!bRequested) return true;
        FReference Reference;
        bool bDescentRequested = false;
        if (!ParseCommandLine(CommandLine, Reference, bDescentRequested, Error)) return false;
        if (!bDescentRequested || !bDefaultAtmosphere)
        {
            Error = TEXT("Atmosphere boundary requires -APSProbeFrozenDescent=Lidim|Jaim and -APSProbeDefaultAtmosphere");
            return false;
        }
        return true;
    }

    // Call only in the explicit boundary mode, after ConfigureModel. Do not
    // silently repair a mismatching model or rewrite any other climate fields.
    inline bool ConfigureAtmosphereBoundaryModel(UGeneratedWorld& Model,
        const FReference& Reference, FString& Error)
    {
        if (!ValidateReference(Reference, Error)) return false;
        if (Model.PlanetType != EPlanetType::Frozen || Model.PlanetSurfaceSeed != Reference.SurfaceSeed
            || Model.PlanetRadius != Reference.RadiusKm)
        {
            Error = TEXT("Atmosphere boundary requires the matching configured Frozen body fixture");
            return false;
        }
        // FCelestialBodyModel::RadiusKM is float; production MoonModel computes
        // MoonAtmosphereHeight = RadiusKM / 30 before widening to the actor.
        Model.AtmosphereHeight = static_cast<double>(static_cast<float>(Reference.RadiusKm) / 30.0f);
        Error.Reset();
        return true;
    }

    // Numeric readback makes mismatch handling testable without spawning actors.
    // The resolved TerrainSeed is derived by production and is NOT the body seed.
    struct FRuntimeSnapshot
    {
        EPlanetType BodyType = EPlanetType::Unknown;
        EPlanetType ProfileType = EPlanetType::Unknown;
        int32 BodySeed = 0;
        int32 RootSeed = 0;
        double BodyRadiusKm = 0.0;
        double PresentationScale = 0.0;
        double RootRadiusCm = 0.0;
        double RootRadiusCodeCm = 0.0;
        double NoiseScale = 0.0;
        double NoiseIntensity = 0.0;
        double ProfileNoiseScale = 0.0;
        double ProfileNoiseIntensity = 0.0;
        bool bRootOcean = true;
        EAPSPlanetLiquidType ProfileLiquidType = EAPSPlanetLiquidType::Water;
    };

    inline bool ValidateSnapshot(const FRuntimeSnapshot& Actual, const FReference& Reference, FString& Error)
    {
        if (!ValidateReference(Reference, Error)) return false;
        TArray<FString> Failures;
        const auto ExactInteger = [&Failures](const TCHAR* Field, int32 Value, int32 Expected)
        {
            if (Value != Expected)
                Failures.Add(FString::Printf(TEXT("%s=%d expected=%d"), Field, Value, Expected));
        };
        const auto Close = [&Failures](const TCHAR* Field, double Value, double Expected, double Tolerance)
        {
            if (!FMath::IsFinite(Value) || FMath::Abs(Value - Expected) > Tolerance)
                Failures.Add(FString::Printf(TEXT("%s=%.17g expected=%.17g tolerance=%.17g"), Field, Value, Expected, Tolerance));
        };
        const auto Rounded = [&Failures](const TCHAR* Field, double Value, int32 Expected)
        {
            // Bound before conversion: NaN/infinity/out-of-int-range must fail,
            // never turn into a coincidentally matching rounded integer.
            if (!FMath::IsFinite(Value) || Value < 0.0 || Value > static_cast<double>(MAX_int32) - 1.0
                || FMath::RoundToInt(Value) != Expected)
                Failures.Add(FString::Printf(TEXT("%s=%.17g expectedRounded=%d"), Field, Value, Expected));
        };
        ExactInteger(TEXT("body.type"), static_cast<int32>(Actual.BodyType), static_cast<int32>(EPlanetType::Frozen));
        ExactInteger(TEXT("profile.type"), static_cast<int32>(Actual.ProfileType), static_cast<int32>(EPlanetType::Frozen));
        ExactInteger(TEXT("body.seed"), Actual.BodySeed, Reference.SurfaceSeed);
        ExactInteger(TEXT("root.seed"), Actual.RootSeed, Reference.SurfaceSeed);
        // FCelestialBodyModel::RadiusKM (CelestialGenerationModel.h) is float;
        // PlanetGenerator copies it into the actor's double RadiusKM. Require
        // that exact normal-model conversion, not a widened radius tolerance.
        const double ExpectedBodyRadiusKm = static_cast<double>(static_cast<float>(Reference.RadiusKm));
        Close(TEXT("body.radiusKm"), Actual.BodyRadiusKm, ExpectedBodyRadiusKm, 0.0);
        Close(TEXT("body.presentationScale"), Actual.PresentationScale, 1.0, 1.0e-9);
        // Installed WorldScape exposes float PlanetScale; PlanetScaleCode copies
        // that value into double. Convert the already-quantized MODEL radius to
        // centimetres before the plugin float conversion (Jaim: 50035728 cm).
        const double ExpectedRootRadiusCm = static_cast<double>(static_cast<float>(ExpectedBodyRadiusKm * 100000.0));
        Close(TEXT("root.radiusCm"), Actual.RootRadiusCm, ExpectedRootRadiusCm, 0.0);
        Close(TEXT("root.radiusCodeCm"), Actual.RootRadiusCodeCm, ExpectedRootRadiusCm, 0.0);
        // WorldScape's production boundary intentionally stores integral noise.
        Close(TEXT("root.noiseScale"), Actual.NoiseScale, Reference.RoundedNoiseScale, 0.0);
        Close(TEXT("root.noiseIntensity"), Actual.NoiseIntensity, Reference.RoundedNoiseIntensity, 0.0);
        Rounded(TEXT("profile.noiseScale"), Actual.ProfileNoiseScale, Reference.RoundedNoiseScale);
        Rounded(TEXT("profile.noiseIntensity"), Actual.ProfileNoiseIntensity, Reference.RoundedNoiseIntensity);
        if (Actual.bRootOcean) Failures.Add(TEXT("root.ocean=true expected=false"));
        ExactInteger(TEXT("profile.liquidType"), static_cast<int32>(Actual.ProfileLiquidType), static_cast<int32>(EAPSPlanetLiquidType::None));
        if (!Failures.IsEmpty())
        {
            Error = FString::Printf(TEXT("Frozen descent %s runtime mismatch: %s; fixture not repaired or substituted"),
                *Reference.Name, *FString::Join(Failures, TEXT("; ")));
            return false;
        }
        Error.Reset();
        return true;
    }

    inline bool ValidateRuntime(const APlanetaryBody* Body, const APlanetarySurfaceGenerator* Surface,
        const AWorldScapeRoot* Root, const FReference& Reference, FString& Error)
    {
        if (!ValidateReference(Reference, Error)) return false;
        if (!IsValid(Body) || !IsValid(Surface) || !IsValid(Root)
            || Surface->WorldScapeRootInstance != Root)
        {
            Error = FString::Printf(TEXT("Frozen descent %s lacks matching valid body/surface/root: body=%s surface=%s root=%s boundRoot=%s"),
                *Reference.Name, *GetPathNameSafe(Body), *GetPathNameSafe(Surface), *GetPathNameSafe(Root),
                IsValid(Surface) ? *GetPathNameSafe(Surface->WorldScapeRootInstance) : TEXT("None"));
            return false;
        }
        const auto& Profile = Surface->ResolvedSurfaceProfile;
        FRuntimeSnapshot Actual;
        Actual.BodyType = Body->PlanetType;
        Actual.ProfileType = Profile.PlanetType;
        Actual.BodySeed = Body->WorldScapeSeed;
        Actual.RootSeed = Root->Seed;
        Actual.BodyRadiusKm = Body->RadiusKM;
        Actual.PresentationScale = Body->WorldScapePresentationScale;
        Actual.RootRadiusCm = Root->PlanetScale;
        Actual.RootRadiusCodeCm = Root->PlanetScaleCode;
        Actual.NoiseScale = Root->NoiseScale;
        Actual.NoiseIntensity = Root->NoiseIntensity;
        Actual.ProfileNoiseScale = Profile.NoiseScale;
        Actual.ProfileNoiseIntensity = Profile.NoiseIntensity;
        Actual.bRootOcean = Root->bOcean;
        Actual.ProfileLiquidType = Profile.LiquidType;
        return ValidateSnapshot(Actual, Reference, Error);
    }

    inline constexpr double CaptureIntervalSeconds = 0.125; // 8 Hz, caller-owned capture.
    inline constexpr double HoldSeconds = 4.0;
    inline constexpr double LegSeconds = 5.0;
    inline constexpr double WaypointHeightsKm[] = {30.5, 9.1, 3.5, 2.7, 0.034, 0.002, 0.034, 2.7, 3.5, 9.1, 30.5};
    inline constexpr int32 WaypointCount = UE_ARRAY_COUNT(WaypointHeightsKm);
    inline constexpr double DurationSeconds = WaypointCount * HoldSeconds + (WaypointCount - 1) * LegSeconds; // 94 s.

    struct FRouteSample
    {
        double HeightKm = WaypointHeightsKm[0];
        const TCHAR* Phase = TEXT("invalid");
        bool bHold = false;
        int32 WaypointIndex = INDEX_NONE;
        bool bFinished = false;
        bool bValid = false;
    };

    // Height above the caller's runtime surface anchor. The caller retains its
    // coordinate/rebase handling; this pure route never moves an actor itself.
    // Cubic easing in log(height) keeps position AND speed continuous at holds.
    inline FRouteSample Route(double ElapsedSeconds)
    {
        FRouteSample Out;
        if (!FMath::IsFinite(ElapsedSeconds)) return Out;
        const double T = FMath::Clamp(ElapsedSeconds, 0.0, DurationSeconds);
        Out.bValid = true;
        Out.bFinished = ElapsedSeconds >= DurationSeconds;
        Out.WaypointIndex = FMath::Min(FMath::FloorToInt(T / (HoldSeconds + LegSeconds)), WaypointCount - 1);
        const double Local = T - Out.WaypointIndex * (HoldSeconds + LegSeconds);
        Out.HeightKm = WaypointHeightsKm[Out.WaypointIndex];
        Out.bHold = Local < HoldSeconds || Out.WaypointIndex == WaypointCount - 1;
        if (Out.bHold)
        {
            Out.Phase = Out.bFinished ? TEXT("complete") : Out.WaypointIndex < 5 ? TEXT("descent_hold")
                : Out.WaypointIndex == 5 ? TEXT("near_hold") : TEXT("ascent_hold");
        }
        else
        {
            const double U = FMath::Clamp((Local - HoldSeconds) / LegSeconds, 0.0, 1.0);
            const double Ease = U * U * (3.0 - 2.0 * U);
            Out.HeightKm = FMath::Exp(FMath::Lerp(FMath::Loge(WaypointHeightsKm[Out.WaypointIndex]),
                FMath::Loge(WaypointHeightsKm[Out.WaypointIndex + 1]), Ease));
            Out.Phase = Out.WaypointIndex < 5 ? TEXT("descent") : TEXT("ascent");
        }
        return Out;
    }
    struct FAtmosphereBoundaryRoute
    {
        TArray<double> WaypointHeightsKm;
        double ShellHeightKm = 0.0;
    };

    // All inputs are actual runtime kilometres, not the requested model radius.
    // SurfaceAnchorRadiusKm = (Root->PlanetScale + Ground) / 100000. The caller
    // must first verify concentric, unit-scale atmosphere/root transforms; this
    // scalar planner cannot validate actor attachment or presentation overrides.
    inline bool BuildAtmosphereBoundaryRoute(double AtmospherePlanetRadiusKm,
        double AtmosphereHeightKm, double SurfaceAnchorRadiusKm,
        FAtmosphereBoundaryRoute& Out, FString& Error)
    {
        Out = {};
        Error.Reset();
        if (!FMath::IsFinite(AtmospherePlanetRadiusKm) || AtmospherePlanetRadiusKm <= 0.0
            || !FMath::IsFinite(AtmosphereHeightKm) || AtmosphereHeightKm <= 0.0
            || !FMath::IsFinite(SurfaceAnchorRadiusKm) || SurfaceAnchorRadiusKm <= 0.0)
        {
            Error = TEXT("Atmosphere boundary needs finite positive runtime radii and shell height in km");
            return false;
        }
        const double ShellHeightKm = AtmospherePlanetRadiusKm + AtmosphereHeightKm - SurfaceAnchorRadiusKm;
        if (!FMath::IsFinite(ShellHeightKm) || ShellHeightKm <= 2.0)
        {
            Error = TEXT("Atmosphere boundary shell must be more than 2 km above the surface anchor");
            return false;
        }
        FAtmosphereBoundaryRoute Planned;
        Planned.ShellHeightKm = ShellHeightKm;
        Planned.WaypointHeightsKm = {FMath::Max(60.0, ShellHeightKm + 10.0), 34.5,
            ShellHeightKm + 2.0, ShellHeightKm + .05, ShellHeightKm - .05, ShellHeightKm - 2.0};
        Planned.WaypointHeightsKm.Sort([](double A, double B) { return A > B; });
        for (int32 Index = 0; Index < Planned.WaypointHeightsKm.Num(); ++Index)
        {
            const double Height = Planned.WaypointHeightsKm[Index];
            if (!FMath::IsFinite(Height) || Height <= 0.0
                || (Index > 0 && Planned.WaypointHeightsKm[Index - 1] - Height <= 1.0e-6))
            {
                Error = TEXT("Atmosphere boundary anchors must be finite, positive, and distinct by more than 1 mm");
                return false;
            }
        }
        // Six sorted descending anchors, then their mirror without repeating the
        // innermost hold: eleven waypoints, the same 94 s / 8 Hz sample budget.
        for (int32 Index = 4; Index >= 0; --Index)
        {
            const double MirroredHeight = Planned.WaypointHeightsKm[Index];
            Planned.WaypointHeightsKm.Add(MirroredHeight);
        }
        Out = MoveTemp(Planned);
        return true;
    }

    inline FRouteSample AtmosphereBoundaryRoute(double ElapsedSeconds,
        const FAtmosphereBoundaryRoute& Planned)
    {
        FRouteSample Out;
        if (!FMath::IsFinite(ElapsedSeconds) || !FMath::IsFinite(Planned.ShellHeightKm)
            || Planned.ShellHeightKm <= 2.0 || Planned.WaypointHeightsKm.Num() != WaypointCount) return Out;
        const auto& Heights = Planned.WaypointHeightsKm;
        for (int32 Index = 0; Index < WaypointCount; ++Index)
        {
            if (!FMath::IsFinite(Heights[Index]) || Heights[Index] <= 0.0
                || Heights[Index] != Heights[WaypointCount - 1 - Index]
                || (Index > 0 && Index <= 5 && Heights[Index - 1] - Heights[Index] <= 1.0e-6)) return Out;
        }
        if (Heights[0] < FMath::Max(60.0, Planned.ShellHeightKm + 10.0)
            || !Heights.Contains(34.5) || !Heights.Contains(Planned.ShellHeightKm + 2.0)
            || !Heights.Contains(Planned.ShellHeightKm + .05) || !Heights.Contains(Planned.ShellHeightKm - .05)
            || !Heights.Contains(Planned.ShellHeightKm - 2.0)) return Out;
        const double T = FMath::Clamp(ElapsedSeconds, 0.0, DurationSeconds);
        Out.bValid = true;
        Out.bFinished = ElapsedSeconds >= DurationSeconds;
        Out.WaypointIndex = FMath::Min(FMath::FloorToInt(T / (HoldSeconds + LegSeconds)), WaypointCount - 1);
        const double Local = T - Out.WaypointIndex * (HoldSeconds + LegSeconds);
        Out.HeightKm = Heights[Out.WaypointIndex];
        Out.bHold = Local < HoldSeconds || Out.WaypointIndex == WaypointCount - 1;
        if (Out.bHold)
        {
            Out.Phase = Out.bFinished ? TEXT("atmosphere_complete") : Out.WaypointIndex < 5 ? TEXT("atmosphere_descent_hold")
                : Out.WaypointIndex == 5 ? TEXT("atmosphere_inner_hold") : TEXT("atmosphere_ascent_hold");
        }
        else
        {
            const double U = FMath::Clamp((Local - HoldSeconds) / LegSeconds, 0.0, 1.0);
            const double Ease = U * U * (3.0 - 2.0 * U);
            Out.HeightKm = FMath::Exp(FMath::Lerp(FMath::Loge(Heights[Out.WaypointIndex]),
                FMath::Loge(Heights[Out.WaypointIndex + 1]), Ease));
            Out.Phase = Out.WaypointIndex < 5 ? TEXT("atmosphere_descent") : TEXT("atmosphere_ascent");
        }
        return Out;
    }
}
#endif
