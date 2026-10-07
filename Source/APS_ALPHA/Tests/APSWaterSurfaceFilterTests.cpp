#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWaterSurfaceFilter.h"
#include "APS_ALPHA/Core/Planetary/APSWaterAnalyticWaves.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWaterSurfaceFilterContract,
    "APS.Gameplay.World.PlanetSurface.WaterSurface.FootprintContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWaterSurfaceFilterContract::RunTest(const FString& Parameters)
{
    using namespace APSWaterSurfaceFilter;
    TestTrue(TEXT("Simplex periods divisible by three"), PrimaryPeriod % 3 == 0 && SecondaryPeriod % 3 == 0);
    TestEqual(TEXT("Resolved waves retained"), Visibility(0.0f), 1.0f);
    TestEqual(TEXT("Resolved threshold is continuous"), Visibility(0.25f), 1.0f);
    TestEqual(TEXT("Unresolved threshold is continuous"), Visibility(1.0f), 0.0f);
    TestEqual(TEXT("Subpixel waves vanish"), Visibility(100.0f), 0.0f);
    TestEqual(TEXT("Invalid derivative fails quiet"), Visibility(std::numeric_limits<float>::infinity()), 0.0f);
    TestEqual(TEXT("NaN derivative fails quiet"), Visibility(std::numeric_limits<float>::quiet_NaN()), 0.0f);
    float Previous = 1.0f;
    for (int32 I = 0; I <= 10000; ++I)
    {
        const float Weight = Visibility(float(I) / 4000.0f);
        TestTrue(TEXT("Bounded, monotonic zoom response"), FMath::IsFinite(Weight) && Weight >= 0 && Weight <= Previous);
        TestTrue(TEXT("No scalar discontinuity"), FMath::Abs(Weight - Previous) <= 0.00051f);
        Previous = Weight;
    }
    // Precision reference for folding, not proof of compiled GPU LWC arithmetic.
    for (double Radius : {637100032.0, 675000000.0, 1200000000.0})
        for (double Sign : {-1.0, 1.0})
            for (double Scale : {double(PrimaryScaleCm), double(SecondaryScaleCm)})
                for (int32 Period : {PrimaryPeriod, SecondaryPeriod})
                {
                    const double A = Sign * Radius / (Scale * Period);
                    const double B = (Sign * Radius + 0.01) / (Scale * Period);
                    const double FoldA = (A - FMath::FloorToDouble(A)) * Period;
                    const double FoldB = (B - FMath::FloorToDouble(B)) * Period;
                    double Delta = FoldB - FoldA;
                    if (Delta < 0) Delta += Period;
                    TestTrue(TEXT("0.1mm physical phase remains in double reference"),
                        FMath::Abs(Delta * Scale - 0.01) < 0.000001);
                }
    UE_LOG(LogTemp, Display, TEXT("[APS.WaterSurfaceFilter] Numerical contract only; shader precision, visible folds and GPU cost require rendered tests"));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWaterAnalyticContract,
    "APS.Gameplay.World.PlanetSurface.WaterSurface.AnalyticKernelContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWaterAnalyticContract::RunTest(const FString& Parameters)
{
    using namespace APSWaterAnalyticWaves;
    const FVector Axes[] = {FVector(1,0,0), FVector(0,1,0), FVector(0,0,1)};
    for (int32 I = 0; I < 64; ++I)
    {
        const FVector P(I * 0.377 - 12.3, I * 1.113 + .5, I * -.731);
        const FVector4 V = Evaluate(P);
        TestTrue(TEXT("Finite bounded wave height/gradient"), !V.ContainsNaN()
            && FMath::Abs(V.W) <= 1.000001 && FVector(V.X,V.Y,V.Z).Size() <= UE_DOUBLE_TWO_PI + 1.e-6);
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            constexpr double Eps = 1.e-5;
            const double Derivative = (Evaluate(P + Axes[Axis] * Eps).W - Evaluate(P - Axes[Axis] * Eps).W) / (2 * Eps);
            TestTrue(TEXT("Analytic normal equals height derivative"), FMath::Abs(Derivative - V[Axis]) < 1.e-7);
            for (const int32 Period : {APSWaterSurfaceFilter::PrimaryPeriod, APSWaterSurfaceFilter::SecondaryPeriod})
            {
                const FVector4 Wrapped = Evaluate(P + Axes[Axis] * Period);
                TestTrue(TEXT("Existing domain folds preserve height and gradient"), V.Equals(Wrapped, 1.e-8));
            }
        }
    }
    // Moving the view origin must not move the planet-fixed wave phase.
    for (double Scale : {41.0,120.0,410.0,1200.0})
        for (double Period : {1023.0,1533.0})
            for (double Translation : {-1234567.3,-17.0,0.0,19.7,1234567.3})
            {
                const auto Fold = [Period](const FVector& P)
                {
                    FVector V;
                    for (int32 Axis=0;Axis<3;++Axis) V[Axis]=(P[Axis]/Period-FMath::FloorToDouble(P[Axis]/Period))*Period;
                    return V;
                };
                const FVector Point(637100032.17,-591236879.23,1234567890.56);
                const FVector Origin=Point+FVector(Translation,Translation*.37,Translation*-.81);
                const FVector Anchored=Fold(Fold(Origin/Scale)+(Point-Origin)/Scale);
                TestTrue(TEXT("Anchor split preserves planet phase across camera translations"),
                    Evaluate(Anchored).Equals(Evaluate(Fold(Point/Scale)),0.00001));
            }
    AddInfo(TEXT("CPU reference only: GPU image, temporal stability and cost remain required."));
    return true;
}
#endif
