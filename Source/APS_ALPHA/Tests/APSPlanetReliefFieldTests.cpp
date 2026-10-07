#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Generation/APSPlanetReliefField.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetReliefFieldContract,
    "APS.Contracts.PlanetSurface.ReliefFieldCPU", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetReliefFieldContract::RunTest(const FString& Parameters)
{
    using namespace APSPlanetReliefField;
    constexpr double R = 1000000.0, Step = 100.0, A = 50000.0;
    const FVector3d Axis = FVector3d(1.0, 2.0, -3.0).GetSafeNormal();
    for (const FVector3d& D : {FVector3d::UnitX(), FVector3d::UnitY(), FVector3d::UnitZ(),
        FVector3d(1, 1, 1).GetSafeNormal(), FVector3d(-2, 1, 3).GetSafeNormal()})
    {
        FVector3f Normal(9, 8, 7);
        const auto Flat = [](const FVector3d&, double& H) { H = 25000.0; return true; };
        TestTrue(TEXT("Constant-height derivative valid"), EvaluateNormal(D, R, Step, Flat, Normal));
        TestTrue(TEXT("Constant-height sphere is radial"), FVector3d(Normal).Equals(D, 2.e-6));
        const auto Linear = [&](const FVector3d& P, double& H) { H = A * FVector3d::DotProduct(P, Axis); return true; };
        TestTrue(TEXT("Analytic radial-height derivative valid"), EvaluateNormal(D, R, Step, Linear, Normal));
        const double H = A * FVector3d::DotProduct(D, Axis);
        const FVector3d Gradient = (Axis - D * FVector3d::DotProduct(D, Axis)) * A;
        const FVector3d Expected = (D - Gradient / (R + H)).GetSafeNormal();
        TestTrue(TEXT("Finite derivative matches analytic normal"), FVector3d(Normal).Equals(Expected, 2.e-6));
        TestFalse(TEXT("Missing explicit bandwidth rejected"), EvaluateNormal(D, R, 0.0, Linear, Normal));
        const auto Failure = [](const FVector3d&, double&) { return false; };
        Normal = FVector3f(9, 8, 7);
        TestFalse(TEXT("Failed sample rejected"), EvaluateNormal(D, R, Step, Failure, Normal));
        TestTrue(TEXT("Failed derivative leaves output untouched"), Normal == FVector3f(9, 8, 7));
        const auto LavaFlat = [](const FVector3d& P, double& OutHeight)
        { OutHeight = APSWorldScapeSurfaceEnvelope::Height(-1000.0 + 100.0 * P.X, 0.0, true); return true; };
        TestTrue(TEXT("Lava envelope derivative valid"), EvaluateNormal(D, R, Step, LavaFlat, Normal));
        TestTrue(TEXT("Clamped sea is radial, not buried rock slope"), FVector3d(Normal).Equals(D, 2.e-6));
    }
    APSClosedGlobeMesh::FSamplingFrame Frame;
    Frame.Radius = R; Frame.NoiseScale = 16.0; Frame.NoiseIntensity = 0.0;
    Frame.SeededNoise.SetSeed(1338); Frame.SeededNoise.SetSeed(1337);
    FBuildOptions Options; Options.FaceResolution = 16; Options.InputSignature = 123;
    FFieldData Field; FString Error;
    TestFalse(TEXT("Default zero bandwidth rejected"), Build(Frame, Options, Field, Error));
    TestTrue(TEXT("Rejected build has no partial field"), Field.PlanetNormals.IsEmpty() && Field.Metadata.InputSignature == 0);
    Options.DerivativeHalfStepCm = Step;
    if (!TestTrue(TEXT("Small real height-only field builds"), Build(Frame, Options, Field, Error))) return false;
    TestEqual(TEXT("Raw seam-inclusive node count"), Field.PlanetNormals.Num(), 6 * 17 * 17);
    TestEqual(TEXT("Unique cube surface directions"), Field.Metadata.UniqueDirections, 6 * 16 * 16 + 2);
    TestEqual(TEXT("Explicit bandwidth retained"), Field.Metadata.DerivativeHalfStepCm, Step);
    TMap<FIntVector, FVector3f> Seams;
    for (int32 F = 0; F < 6; ++F) for (int32 Y = 0; Y <= 16; ++Y) for (int32 X = 0; X <= 16; ++X)
    {
        FIntVector Key; FVector3d D;
        TestTrue(TEXT("Valid face address"), DirectionAt(F, X, Y, 16, Key, D));
        const FVector3f N = Field.PlanetNormals[F * 17 * 17 + Y * 17 + X];
        if (const FVector3f* Other = Seams.Find(Key)) TestTrue(TEXT("Seam copies are bit-identical"), N == *Other);
        else Seams.Add(Key, N);
        TestTrue(TEXT("Zero displacement field is outward and unit"), FVector3d(N).Equals(D, 2.e-6));
    }
    TestFalse(TEXT("Nonempty output rejected"), Build(Frame, Options, Field, Error));
    for (int32 N : {15, 257})
    { FFieldData Empty; Options.FaceResolution = N; TestFalse(TEXT("Outside resolution budget rejected"), Build(Frame, Options, Empty, Error)); }
    Options.FaceResolution = 16; Options.DerivativeHalfStepCm = R / 16.0 + 1.0;
    FFieldData Empty;
    TestFalse(TEXT("Derivative wider than field bandwidth rejected"), Build(Frame, Options, Empty, Error));
    return true;
}
#endif
