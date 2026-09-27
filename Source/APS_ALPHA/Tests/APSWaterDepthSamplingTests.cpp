#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

// Measures the actual saved-catalog APS field before promoting the separate
// bathymetry prototype. No shader, production default or generated-world edit.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWaterDepthSamplingPreflight,
    "APS.Gameplay.World.PlanetSurface.WaterDepthSamplingPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWaterDepthSamplingPreflight::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Sampling world"), World)) return false;
    APlanet* Planet = World->SpawnActor<APlanet>();
    const UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
    if (!TestNotNull(TEXT("Sampling planet"), Planet) || !TestNotNull(TEXT("Saved production catalog"), Catalog))
    {
        World->DestroyWorld(false);
        return false;
    }
    Planet->RadiusKM = 6371.0;
    Planet->PlanetRadiusKM = 6371;
    Planet->Temperature = 288;
    Planet->PlanetAtmosphere.Humidity = 42.0f;
    Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
    constexpr int32 Count = 4096;
    constexpr int32 Rounds = 3;
    constexpr double RadiusCm = 637100000.0;
    constexpr double PreviewRadiusCm = 600000.0;
    constexpr double Scale = PreviewRadiusCm / RadiusCm;
    TArray<DVector> Directions;
    Directions.Reserve(Count);
    for (int32 I = 0; I < Count; ++I)
    {
        const double Z = 1.0 - 2.0 * (static_cast<double>(I) + 0.5) / Count;
        const double R = FMath::Sqrt(1.0 - Z * Z);
        Directions.Emplace(R * FMath::Cos(2.39996322972865332 * I),
            R * FMath::Sin(2.39996322972865332 * I), Z);
    }
    for (EPlanetType Type : {EPlanetType::Terrestrial, EPlanetType::Ocean,
        EPlanetType::Water, EPlanetType::Forest, EPlanetType::Oasis, EPlanetType::Savanna})
    {
        Planet->PlanetType = Type;
        Planet->WorldScapeSeed = 774411 + static_cast<int32>(Type) * 131;
        const auto Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
        TestEqual(TEXT("Requested family resolves to Water"), Profile.LiquidType, EAPSPlanetLiquidType::Water);
        UAPSWorldScapePlanetNoise* Noise = NewObject<UAPSWorldScapePlanetNoise>(World);
        Noise->Configure(Profile);
        CustomNoise Seeded(Planet->WorldScapeSeed);
        Seeded.SetSeed(Planet->WorldScapeSeed + 1);
        Seeded.SetSeed(Planet->WorldScapeSeed);
        const double NoiseScale = FMath::Max(1.0, static_cast<double>(FMath::RoundToInt(Profile.NoiseScale)));
        const double Intensity = FMath::Max(1.0, static_cast<double>(FMath::RoundToInt(Profile.NoiseIntensity)));
        const double PreviewIntensity = FMath::Max(1.0, static_cast<double>(FMath::RoundToInt(Profile.NoiseIntensity * Scale)));
        const double Sea = FMath::Max(Profile.OceanLevel * static_cast<double>(Profile.NoiseIntensity), Profile.OceanLevel * Intensity);
        const double PreviewSea = Profile.OceanLevel * static_cast<double>(Profile.NoiseIntensity) * Scale;
        double SumSquaredError = 0.0, MaxAbsError = 0.0, MaxShallowError = 0.0;
        double MaxHeightOnlyErrorCm = 0.0;
        int32 HeightOnlyRoundingDifferences = 0;
        int32 ShoreSignDisagreements = 0, ShallowSamples = 0;
        for (const DVector& Dir : Directions)
        {
            DVector NoisePosition;
            const FNoiseData Native = Noise->GetNoise(Seeded, Dir * RadiusCm, DVector(0.0),
                NoiseScale, Intensity, RadiusCm, false, Dir.Z, NoisePosition, FNoiseData(), true);
            const FNoiseData Exact = Noise->SampleResolved(Seeded, Dir * RadiusCm, DVector(0.0),
                NoiseScale, Intensity, RadiusCm, Dir.Z, NoisePosition);
            TestTrue(TEXT("Value sampler matches native full-scale height"), Exact.Height == Native.Height);
            const double HeightOnly = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(
                Profile, Seeded, Dir * RadiusCm, DVector(0.0), NoiseScale, Intensity, RadiusCm, Dir.Z);
            // Independent optimized specializations can round the same terrain
            // expression differently. Measure this rather than using TestEqual's
            // implicit tolerance or requiring compiler-dependent bit identity.
            MaxHeightOnlyErrorCm = FMath::Max(MaxHeightOnlyErrorCm, FMath::Abs(HeightOnly - Native.Height));
            HeightOnlyRoundingDifferences += HeightOnly != Native.Height ? 1 : 0;
            TestTrue(TEXT("Height-only sample is finite"), FMath::IsFinite(HeightOnly));
            const FNoiseData Preview = Noise->SampleResolved(Seeded, Dir * PreviewRadiusCm, DVector(0.0),
                NoiseScale, PreviewIntensity, PreviewRadiusCm, Dir.Z, NoisePosition);
            TestTrue(TEXT("Height-only sampler retains the accepted compressed geometry"),
                UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Profile, Seeded,
                    Dir * PreviewRadiusCm, DVector(0.0), NoiseScale, PreviewIntensity, PreviewRadiusCm, Dir.Z) ==
                Preview.Height);
            const double DepthM = (Sea - Native.Height) * 0.01;
            const double PreviewDepthM = (PreviewSea - Preview.Height) * 0.01 / Scale;
            const double Error = FMath::Abs(DepthM - PreviewDepthM);
            SumSquaredError += Error * Error;
            MaxAbsError = FMath::Max(MaxAbsError, Error);
            ShoreSignDisagreements += (DepthM > 0.0) != (PreviewDepthM > 0.0) ? 1 : 0;
            if (FMath::Abs(DepthM) <= 100.0 || FMath::Abs(PreviewDepthM) <= 100.0)
            {
                ++ShallowSamples;
                MaxShallowError = FMath::Max(MaxShallowError, Error);
            }
            TestTrue(TEXT("Both candidate inputs finite"), FMath::IsFinite(DepthM) && FMath::IsFinite(PreviewDepthM));
        }
        // Ten nanometres in physical full-scale centimetres. This is far below
        // both rendered terrain precision and the optional FP16 depth payload.
        TestTrue(TEXT("Height-only terrain error is bounded to 1e-6 cm"), MaxHeightOnlyErrorCm <= 1.0e-6);
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterHeightPrecision] type=%s maxErrorCm=%.17g bitDifferences=%d samples=%d toleranceCm=1e-6"),
            *UEnum::GetValueAsString(Type), MaxHeightOnlyErrorCm, HeightOnlyRoundingDifferences, Count);
        // Alternate orders. Report medians of synchronous per-sample CPU time,
        // not game-thread time or FPS. Keep results observable to the optimizer.
        double Checksum = 0.0;
        TArray<double> OceanTimes, CombinedTimes, HeightOnlyTimes;
        auto TimeBatch = [&](int32 GroundMode)
        {
            const double Start = FPlatformTime::Seconds();
            for (const DVector& Dir : Directions)
            {
                DVector NoisePosition;
                const FNoiseData Ocean = Noise->GetOceanNoise(Seeded, Dir * RadiusCm, DVector(0.0),
                    NoiseScale, Intensity, RadiusCm, false, Dir.Z, NoisePosition, FNoiseData(), true);
                Checksum += Ocean.Height;
                if (GroundMode == 1)
                {
                    const FNoiseData Ground = Noise->GetNoise(Seeded, Dir * RadiusCm, DVector(0.0),
                        NoiseScale, Intensity, RadiusCm, false, Dir.Z, NoisePosition, FNoiseData(), true);
                    Checksum += (FMath::Max(Ocean.Height, Sea) - Ground.Height) * 0.01;
                }
                else if (GroundMode == 2)
                {
                    const double GroundHeight = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(
                        Profile, Seeded, Dir * RadiusCm, DVector(0.0), NoiseScale, Intensity, RadiusCm, Dir.Z);
                    Checksum += (FMath::Max(Ocean.Height, Sea) - GroundHeight) * 0.01;
                }
            }
            return (FPlatformTime::Seconds() - Start) * 1.0e6 / Count;
        };
        for (int32 Round = 0; Round < Rounds; ++Round)
        {
            // Rotate all three paths through each timing position.
            for (int32 Slot = 0; Slot < 3; ++Slot)
            {
                const int32 Mode = (Round + Slot) % 3;
                const double Duration = TimeBatch(Mode);
                if (Mode == 0) OceanTimes.Add(Duration);
                else if (Mode == 1) CombinedTimes.Add(Duration);
                else HeightOnlyTimes.Add(Duration);
            }
        }
        OceanTimes.Sort(); CombinedTimes.Sort(); HeightOnlyTimes.Sort();
        const double ExtraUs = FMath::Max(0.0, CombinedTimes[Rounds / 2] - OceanTimes[Rounds / 2]);
        TestTrue(TEXT("Measured work observed"), FMath::IsFinite(Checksum) && FMath::Abs(Checksum) > 0.0);
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterDepthSampling] type=%s seed=%d samples=%d radiusCm=%.0f previewScale=%.9g fullIntensity=%.9g previewIntensity=%.9g rmsDifferenceM=%.6f maxDifferenceM=%.6f shallowSamples=%d maxShallowDifferenceM=%.6f signDisagreements=%d oceanUs=%.6f oceanPlusGroundUs=%.6f extraUs=%.6f approximate4096VertexExtraMs=%.3f checksum=%.9g acceptance=MEASUREMENT_ONLY"),
            *UEnum::GetValueAsString(Type), Planet->WorldScapeSeed, Count, RadiusCm, Scale,
            Intensity, PreviewIntensity, FMath::Sqrt(SumSquaredError / Count), MaxAbsError,
            ShallowSamples, MaxShallowError, ShoreSignDisagreements, OceanTimes[Rounds / 2],
            CombinedTimes[Rounds / 2], ExtraUs, ExtraUs * 4096.0 / 1000.0, Checksum);
        const double HeightExtraUs = FMath::Max(0.0, HeightOnlyTimes[Rounds / 2] - OceanTimes[Rounds / 2]);
        UE_LOG(LogTemp, Display, TEXT("[APS.WaterHeightOnly] type=%s fullExtraUs=%.6f heightOnlyExtraUs=%.6f savingPercent=%.3f approximate9216VertexExtraMs=%.3f acceptance=BOUNDED_HEIGHT_CPU_ONLY"),
            *UEnum::GetValueAsString(Type), ExtraUs, HeightExtraUs,
            ExtraUs > 0.0 ? (1.0 - HeightExtraUs / ExtraUs) * 100.0 : 0.0,
            HeightExtraUs * 9216.0 / 1000.0);
        // Exact-field material input can reuse the pure full-scale sampler in
        // preview without changing its lower-bandwidth geometry. Measure that
        // option, rather than asserting that scaled heights are already equivalent.
    }
    Planet->Destroy();
    World->DestroyWorld(false);
    return true;
}
#endif
