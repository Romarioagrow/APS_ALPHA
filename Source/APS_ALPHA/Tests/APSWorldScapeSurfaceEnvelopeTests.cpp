#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeSurfaceEnvelopeMatrix,
    "APS.Gameplay.World.PlanetSurface.UnifiedLavaFamilyMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeSurfaceEnvelopeMatrix::RunTest(const FString& Parameters)
{
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
        .ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        false, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("isolated world"), World)) return false;
    int32 Solids = 0, Gas = 0, Lava = 0, Water = 0, Ammonia = 0, Dry = 0;
    for (uint8 Id = 0; Id <= APSPlanetTypes::LastValue; ++Id)
    {
        const EPlanetType Type = static_cast<EPlanetType>(Id);
        if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) { ++Gas; continue; }
        ++Solids;
        APlanet* Body = World->SpawnActor<APlanet>();
        if (!TestNotNull(TEXT("matrix planet"), Body)) continue;
        Body->PlanetType = Type; Body->IsManual = false;
        Body->RadiusKM = 6750.0; Body->PlanetRadiusKM = 6750;
        for (int32 Seed : {1337, 7927})
        {
            Body->WorldScapeSeed = Seed;
            const auto P = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body);
            const bool IsLava = P.LiquidType == EAPSPlanetLiquidType::Lava;
            const bool HasOcean = P.LiquidType != EAPSPlanetLiquidType::None && P.LandCoverage < 0.995f;
            const bool Enabled = APSWorldScapeSurfaceEnvelope::Eligible(IsLava, HasOcean, false, 1.0);
            if (Seed == 1337)
            {
                Lava += IsLava;
                Water += P.LiquidType == EAPSPlanetLiquidType::Water;
                Ammonia += P.LiquidType == EAPSPlanetLiquidType::Ammonia;
                Dry += P.LiquidType == EAPSPlanetLiquidType::None;
                AddInfo(FString::Printf(TEXT("id=%d type=%s liquid=%s ocean=%d unifiedCandidate=%d"),
                    Id, *UEnum::GetValueAsString(Type), *UEnum::GetValueAsString(P.LiquidType), HasOcean, Enabled));
            }
            TestFalse(TEXT("authored roots excluded"), APSWorldScapeSurfaceEnvelope::Eligible(IsLava, HasOcean, true, 1.0));
            TestFalse(TEXT("preview roots excluded"), APSWorldScapeSurfaceEnvelope::Eligible(IsLava, HasOcean, false, 0.002));
            auto* Noise = NewObject<UAPSWorldScapePlanetNoise>(World);
            // Deliberately request it for EVERY family: Configure must also guard
            // dry/water/ammonia even if a caller bypasses the runtime selector.
            Noise->Configure(P, true);
            CustomNoise State;
            State.SetSeed(Seed + 1); State.SetSeed(Seed);
            int32 Basins = 0;
            for (int32 Sample = 0; Sample < 128; ++Sample)
            {
                const double Z = 1.0 - 2.0 * (Sample + 0.5) / 128.0;
                const double Angle = Sample * 2.399963229728653;
                const double R = FMath::Sqrt(1.0 - Z * Z);
                const DVector Position(R * FMath::Cos(Angle) * 675000000.0,
                    R * FMath::Sin(Angle) * 675000000.0, Z * 675000000.0);
                const DVector Center(0, 0, 0);
                DVector RawPosition, NewPosition;
                const FNoiseData Raw = UAPSWorldScapePlanetNoise::SampleResolvedProfile(
                    P, State, Position, Center, P.NoiseScale, P.NoiseIntensity, 675000000.0, Z, RawPosition);
                const FNoiseData Unified = Noise->GetNoise(State, Position, Center,
                    P.NoiseScale, P.NoiseIntensity, 675000000.0, false, Z, NewPosition, FNoiseData(), true);
                const double Sea = static_cast<double>(P.OceanLevel) * P.NoiseIntensity;
                const double Expected = APSWorldScapeSurfaceEnvelope::Height(Raw.Height, Sea, Enabled);
                TestTrue(TEXT("render/collision noise equals visible envelope (or unchanged non-lava)"),
                    FMath::Abs(Unified.Height - Expected) < 1.0e-6);
                TestEqual(TEXT("accepted palette red preserved"), Unified.HeightNormalize, Raw.HeightNormalize);
                TestEqual(TEXT("temperature preserved"), Unified.Temperature, Raw.Temperature);
                TestEqual(TEXT("humidity preserved"), Unified.Humidity, Raw.Humidity);
                TestEqual(TEXT("original water classification preserved"), Unified.WaterMask, Raw.WaterMask);
                if (Raw.Height < Sea) ++Basins;
            }
            if (Enabled) TestTrue(TEXT("sampled an actual lava basin"), Basins > 0);
        }
        Body->Destroy();
    }
    TestEqual(TEXT("gas presets excluded"), Gas, 3);
    TestEqual(TEXT("solid presets including saved legacy IDs"), Solids, 32);
    TestEqual(TEXT("lava variants"), Lava, 3);
    TestEqual(TEXT("water variants"), Water, 11);
    TestEqual(TEXT("ammonia variants"), Ammonia, 1);
    TestEqual(TEXT("dry variants"), Dry, 17);
    World->DestroyWorld(false);
    return true;
}
#endif
