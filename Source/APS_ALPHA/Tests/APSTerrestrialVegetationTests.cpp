#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSTerrestrialVegetation.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSTerrestrialVegetationTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.TerrestrialVegetation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSTerrestrialVegetationTest::RunTest(const FString& Parameters)
{
    using namespace APSTerrestrialVegetation;
    FAPSResolvedPlanetSurfaceProfile Base;
    Base.PlanetType = EPlanetType::Terrestrial;
    Base.Archetype = EAPSPlanetSurfaceArchetype::Temperate;
    Base.LiquidType = EAPSPlanetLiquidType::Water;
    Base.LandCoverage = .6f; Base.OceanLevel = .0125f;
    Base.Temperature = .58f; Base.Humidity = .65f;
    for (int32 Mode : {-1, 0, 1, 2}) for (bool Generated : {false, true})
    {
        auto P = Base;
        TestEqual(TEXT("Only explicit candidate on generated body"), Apply(P, Generated, Mode), Generated && Mode == 1);
    }
    for (uint8 T = 0; T <= APSPlanetTypes::LastValue; ++T)
    {
        auto P = Base; P.PlanetType = EPlanetType(T);
        TestEqual(TEXT("Other planet types preserved"), Apply(P, true, 1), P.PlanetType == EPlanetType::Terrestrial);
    }
    for (float Bio : {.01f, .8f, -1.f, std::numeric_limits<float>::quiet_NaN()})
    {
        auto P = Base; P.Biomass = Bio;
        TestFalse(TEXT("Explicit or invalid biomass never replaced"), Apply(P, true, 1));
        P = Base; P.Biodiversity = Bio;
        TestFalse(TEXT("Explicit or invalid biodiversity never replaced"), Apply(P, true, 1));
        P = Base; P.VisualFoliageDensity = Bio;
        TestFalse(TEXT("Existing or invalid visual density never replaced"), Apply(P, true, 1));
    }
    auto Authored = Base; Authored.Foliage.bEnabled = true;
    TestFalse(TEXT("Authored enabled policy preserved"), Apply(Authored, true, 1));
    Authored.Foliage.bEnabled = false;
    Authored.Foliage.Collections.Add(TSoftObjectPtr<UWorldScapeFoliagesCollection>(FSoftObjectPath(TEXT("/Game/Authored.Authored"))));
    TestFalse(TEXT("Disabled authored collection preserved"), Apply(Authored, true, 1));
    auto Candidate = Base;
    TestTrue(TEXT("Candidate applied"), Apply(Candidate, true, 1));
    TestFalse(TEXT("Repeated application stable"), Apply(Candidate, true, 1));
    TestEqual(TEXT("Gameplay biomass unchanged"), Candidate.Biomass, Base.Biomass);
    TestEqual(TEXT("Gameplay biodiversity unchanged"), Candidate.Biodiversity, Base.Biodiversity);
    TestTrue(TEXT("Visual-only density admits the biological palette"), APSPlanetSurfaceScatter::HasHabitat(Candidate));
    TestFalse(TEXT("Original zero-life palette remains mineral"), APSPlanetSurfaceScatter::HasHabitat(Base));
    TestTrue(TEXT("Exact reviewed visual-only preset is published"), APSPlanetSurfaceScatter::UsesScatter(Candidate, 2));
    for (float Density : {0.f, .1f, .55f, .9f})
    {
        auto P = Base; P.VisualFoliageDensity = Density;
        TestEqual(TEXT("No other visual density promoted"), APSPlanetSurfaceScatter::UsesScatter(P, 2),
            Density == 0.f || Density == DefaultVisualDensity);
        P.Biomass = .7f;
        TestFalse(TEXT("Explicit gameplay biosphere branch remains unreviewed"), APSPlanetSurfaceScatter::UsesScatter(P, 2));
    }
    auto Restored = Candidate; Restored.VisualFoliageDensity = Base.VisualFoliageDensity;
    TestEqual(TEXT("No other profile signature input changed"),
        UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Restored),
        UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Base));

    int32 Living = 0;
    for (int32 Seed : {1, 424242, 82276, 73875})
    {
        CustomNoise Noise(Seed); Noise.SetSeed(Seed + 1); Noise.SetSeed(Seed);
        Base.TerrainSeed = Candidate.TerrainSeed = Seed;
        Base.BiomeSeed = Candidate.BiomeSeed = Seed + 71;
        constexpr double Radius = 637100000.;
        for (int32 I = 0; I < 128; ++I)
        {
            const double Z = 1. - 2. * (I + .5) / 128.;
            const double R = FMath::Sqrt(1. - Z * Z), A = I * 2.39996322972865332;
            const DVector Position(Radius * R * FMath::Cos(A), Radius * R * FMath::Sin(A), Radius * Z);
            DVector OldPosition, NewPosition;
            const auto Old = UAPSWorldScapePlanetNoise::SampleResolvedProfile(Base, Noise,
                Position, DVector(0.), Base.NoiseScale, Base.NoiseIntensity, Radius, Z, OldPosition);
            const auto New = UAPSWorldScapePlanetNoise::SampleResolvedProfile(Candidate, Noise,
                Position, DVector(0.), Candidate.NoiseScale, Candidate.NoiseIntensity, Radius, Z, NewPosition);
            TestEqual(TEXT("Physical height unchanged exactly"), New.Height, Old.Height);
            TestEqual(TEXT("Material height unchanged exactly"), New.HeightNormalize, Old.HeightNormalize);
            TestEqual(TEXT("Temperature channel unchanged exactly"), New.Temperature, Old.Temperature);
            TestEqual(TEXT("Humidity channel unchanged exactly"), New.Humidity, Old.Humidity);
            TestEqual(TEXT("Water mask unchanged exactly"), New.WaterMask, Old.WaterMask);
            TestEqual(TEXT("Hole state unchanged"), New.Hole, Old.Hole);
            TestEqual(TEXT("Original zero-biomass mask remains zero"), Old.FoliageMask, 0.f);
            TestTrue(TEXT("Vegetation mask finite and bounded"), FMath::IsFinite(New.FoliageMask) && New.FoliageMask >= 0 && New.FoliageMask <= .55f);
            if (New.Humidity <= .18f || New.WaterMask >= 1.f)
                TestEqual(TEXT("Dry or submerged sample remains vegetation-free"), New.FoliageMask, 0.f);
            Living += New.FoliageMask > .02f;
        }
    }
    TestTrue(TEXT("Suitable sampled regions can grow vegetation"), Living > 0);
    return true;
}
#endif
