#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalRelief.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCoastalReliefBounds,
    "APS.Gameplay.World.PlanetSurface.CoastalRelief.Bounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCoastalReliefBounds::RunTest(const FString& Parameters)
{
    for (double Sea : {-0.15, 0.0, 0.15})
        for (double Offset : {-0.004, -0.000001, 0.0, 0.000001, 0.004})
            for (double Detail : {-0.1, -0.01, 0.0, 0.01, 0.1})
            {
                const double Base = Sea + Offset;
                const double H = APSCoastalRelief::Height(Base, 0.0, Base + Detail, Sea, 0.0);
                TestTrue(TEXT("Finite coast"), FMath::IsFinite(H));
                TestTrue(TEXT("Detail remains within 65% of broad sea clearance"),
                    FMath::Abs(H - Base) <= FMath::Abs(Offset) * 0.65 + 1.e-15);
                TestTrue(TEXT("Detail does not change coast sign"), Offset == 0.0 ? H == Sea : (H > Sea) == (Offset > 0.0));
                for (double Inland : {-0.8, -0.12, 0.12, 0.8})
                    TestTrue(TEXT("Interior is bit-exact"), APSCoastalRelief::Height(Base, 0.012,
                        Base + Detail, Sea, Inland) == Base + Detail);
                for (double Boundary : {-0.12, -0.025, 0.025, 0.12})
                    TestTrue(TEXT("Continuous transition, no height step"), FMath::Abs(
                        APSCoastalRelief::Height(Base, 0.001, Base + Detail, Sea, Boundary - 1.e-8)
                        - APSCoastalRelief::Height(Base, 0.001, Base + Detail, Sea, Boundary + 1.e-8)) < 1.e-7);
            }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCoastalReliefField,
    "APS.Gameplay.World.PlanetSurface.CoastalRelief.Field",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCoastalReliefField::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("World"), World)) return false;
    APlanet* Planet = World->SpawnActor<APlanet>();
    const auto* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
    if (!TestNotNull(TEXT("Planet"), Planet) || !TestNotNull(TEXT("Saved catalog"), Catalog))
    { World->DestroyWorld(false); return false; }
    Planet->RadiusKM = 6371.0; Planet->PlanetRadiusKM = 6371;
    Planet->Temperature = 288; Planet->PlanetAtmosphere.Humidity = 42.0f;
    Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
    constexpr double Radius = 637100000.0, PreviewRatio = 600000.0 / Radius;
    int32 TotalNativeEdges = 0, TotalCandidateEdges = 0, ChangedEligible = 0;
    for (EPlanetType Type : {EPlanetType::Terrestrial, EPlanetType::Water, EPlanetType::Oasis,
        EPlanetType::Ice, EPlanetType::Rocky, EPlanetType::Lava})
        for (int32 Seed : {1337, 424242, 911})
        {
            Planet->PlanetType = Type; Planet->WorldScapeSeed = Seed;
            const auto P = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
            const bool bEligible = APSCoastalRelief::Allows(P);
            CustomNoise N(Seed); N.SetSeed(Seed + 1); N.SetSeed(Seed);
            const double Scale = FMath::Max(1.0, double(FMath::RoundToInt(P.NoiseScale)));
            const double Intensity = FMath::Max(1.0, double(FMath::RoundToInt(P.NoiseIntensity)));
            const double Sea = double(P.OceanLevel) * Intensity;
            auto Sample = [&](const FVector& D, bool Candidate)
            {
                return UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(P, N, DVector(D * Radius),
                    DVector(0.0), Scale, Intensity, Radius, D.Z, Candidate);
            };
            FVector Wet = FVector::ZeroVector, Dry = FVector::ZeroVector;
            double MaxError = 0.0;
            int32 Changed = 0;
            UAPSWorldScapePlanetNoise* Instance = NewObject<UAPSWorldScapePlanetNoise>(World);
            Instance->Configure(P, false, true);
            for (int32 I = 0; I < 1024; ++I)
            {
                const double Z = 1.0 - 2.0 * (I + 0.5) / 1024.0, R = FMath::Sqrt(1.0 - Z * Z);
                const FVector D(R * FMath::Cos(I * 2.39996322972865332), R * FMath::Sin(I * 2.39996322972865332), Z);
                DVector NP;
                const double Native = Sample(D, false), Candidate = Sample(D, true);
                const FNoiseData Full = Instance->SampleResolved(N, DVector(D * Radius), DVector(0.0),
                    Scale, Intensity, Radius, D.Z, NP);
                const FNoiseData Worker = Instance->GetNoise(N, DVector(D * Radius), DVector(0.0),
                    Scale, Intensity, Radius, false, D.Z, NP, FNoiseData(), true);
                const FNoiseData OldFull = UAPSWorldScapePlanetNoise::SampleResolvedProfile(P, N,
                    DVector(D * Radius), DVector(0.0), Scale, Intensity, Radius, D.Z, NP, false);
                TestTrue(TEXT("Worker and immutable sampler share the candidate"), Worker.Height == Full.Height);
                TestTrue(TEXT("Accepted palette and climate unchanged"), OldFull.HeightNormalize == Full.HeightNormalize
                    && OldFull.Temperature == Full.Temperature && OldFull.Humidity == Full.Humidity);
                MaxError = FMath::Max(MaxError, FMath::Abs(Full.Height - Candidate));
                TestTrue(TEXT("Finite candidate"), FMath::IsFinite(Candidate));
                Changed += FMath::Abs(Native - Candidate) > 1.e-6 ? 1 : 0;
                if (bEligible)
                {
                    if (Candidate < Sea && Wet.IsZero()) Wet = D;
                    if (Candidate > Sea && Dry.IsZero()) Dry = D;
                }
                else TestTrue(TEXT("Excluded family unchanged"), Native == Candidate);
                const auto A = UAPSWorldScapePlanetNoise::SampleResolvedProfile(P, N, DVector(D * 600000.0),
                    DVector(0.0), Scale, Intensity * PreviewRatio, 600000.0, D.Z, NP, false);
                const auto B = UAPSWorldScapePlanetNoise::SampleResolvedProfile(P, N, DVector(D * 600000.0),
                    DVector(0.0), Scale, Intensity * PreviewRatio, 600000.0, D.Z, NP, true);
                TestTrue(TEXT("Compressed preview unchanged"), A.Height == B.Height && A.HeightNormalize == B.HeightNormalize);
            }
            TestTrue(TEXT("Height-only and full field agree to 1e-6cm"), MaxError <= 1.e-6);
            if (bEligible)
            {
                ChangedEligible += Changed;
                if (!TestFalse(TEXT("Wet bracket exists"), Wet.IsZero()) || !TestFalse(TEXT("Dry bracket exists"), Dry.IsZero())) continue;
                for (int32 I = 0; I < 45; ++I)
                {
                    const FVector Mid = (Wet + Dry).GetSafeNormal();
                    if (Sample(Mid, true) < Sea) Wet = Mid; else Dry = Mid;
                }
                FVector U, V; Wet.FindBestAxisVectors(U, V);
                TArray<double> NativeDepth, CandidateDepth;
                int32 NativeEdges = 0, CandidateEdges = 0;
                constexpr int32 Side = 65;
                for (int32 Y = 0; Y < Side; ++Y)
                    for (int32 X = 0; X < Side; ++X)
                    {
                        const FVector D = (Wet + (U * (X - Side / 2) + V * (Y - Side / 2)) * (50000.0 / Radius)).GetSafeNormal();
                        const double A = Sea - Sample(D, false), B = Sea - Sample(D, true);
                        NativeDepth.Add(A); CandidateDepth.Add(B);
                        const int32 I = NativeDepth.Num() - 1;
                        for (int32 Prev : {X ? I - 1 : -1, Y ? I - Side : -1})
                            if (Prev >= 0)
                            {
                                NativeEdges += (A > 0.0) != (NativeDepth[Prev] > 0.0) ? 1 : 0;
                                CandidateEdges += (B > 0.0) != (CandidateDepth[Prev] > 0.0) ? 1 : 0;
                            }
                    }
                TotalNativeEdges += NativeEdges; TotalCandidateEdges += CandidateEdges;
                UE_LOG(LogTemp, Display, TEXT("[APS.CoastalRelief.Field] type=%s seed=%d changed=%d nativeEdges=%d candidateEdges=%d heightErrorCm=%.9g; same 32km patch, 500m sampling, not visual acceptance"),
                    *UEnum::GetValueAsString(Type), Seed, Changed, NativeEdges, CandidateEdges, MaxError);
            }
        }
    TestTrue(TEXT("Candidate exercised"), ChangedEligible > 0);
    TestTrue(TEXT("Combined measured shoreline is less fragmented"), TotalCandidateEdges < TotalNativeEdges);
    UE_LOG(LogTemp, Display, TEXT("[APS.CoastalRelief.Total] nativeEdges=%d candidateEdges=%d"), TotalNativeEdges, TotalCandidateEdges);
    Planet->Destroy(); World->DestroyWorld(false);
    return true;
}
#endif
