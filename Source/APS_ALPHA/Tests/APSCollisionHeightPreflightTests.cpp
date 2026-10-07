#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

// PRE-INTEGRATION measurement. No production collision callback, density, task
// count or readiness policy changes. Only tests call SampleCollisionHeight yet.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCollisionHeightPreflight,
    "APS.Gameplay.World.PlanetSurface.CollisionHeightPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCollisionHeightPreflight::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
        ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Collision sampler world"), World)) return false;
    APlanet* Planet = World->SpawnActor<APlanet>();
    AWorldScapeRoot* Root = World->SpawnActor<AWorldScapeRoot>();
    const auto* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
    if (!TestNotNull(TEXT("Planet"), Planet) || !TestNotNull(TEXT("Root"), Root)
        || !TestNotNull(TEXT("Saved catalog"), Catalog))
    { World->DestroyWorld(false); return false; }
    Root->SetActorTickEnabled(false);
    Root->bFlatWorld = false;
    Root->bUsePlanetaryHeightMap = false;
    Root->NoiseOffset = FVector(1.25, -2.5, 4.75);
    Root->PlanetLocation = DVector(1700000000.0, -2200000000.0, 300000000.0);
    auto* Noise = NewObject<UAPSWorldScapePlanetNoise>(Root);
    Root->WorldScapeNoise = Noise;
    Planet->Temperature = 288;
    Planet->PlanetAtmosphere.Humidity = 42.0f;
    Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
    const TArray<FNoiseVolumeData> NoiseVolumes;
    const TArray<FHeightMapVolumeDataCopy> HeightVolumes;
    const TArray<FTerrainHoleVolumeData> HoleVolumes;
    const TArray<FTransform> VolumeTransforms;
    FAPSResolvedPlanetSurfaceProfile Profile;
    auto Reference = [&](const DVector& Position)
    {
        return Root->GetNoise(Position, NoiseVolumes, HeightVolumes, HoleVolumes, VolumeTransforms).Height;
    };
    auto Candidate = [&](const DVector& Position)
    {
        return Noise->SampleCollisionHeight(Root->PlanetNoise,
            Position + Root->NoiseOffset, Root->PlanetLocation,
            Root->NoiseScale, Root->NoiseIntensity, Root->PlanetScaleCode,
            static_cast<float>(Root->GetLattitude(Position)));
    };
    int64 Samples = 0;
    int32 Families = 0;
    double MaxHeightErrorCm = 0.0, MaxVertexErrorCm = 0.0;
    for (uint8 Id = 0; Id <= APSPlanetTypes::LastValue; ++Id)
    {
        const auto Type = static_cast<EPlanetType>(Id);
        if (Type == EPlanetType::GasGiant || Type == EPlanetType::HotGiant || Type == EPlanetType::IceGiant)
            continue;
        ++Families;
        Planet->PlanetType = Type;
        for (int32 Seed : {424242, 82276})
        for (double RadiusKm : {375.0, 5831.706, 6750.0})
        for (int32 Mode : {0, 1, 2})
        {
            Planet->RadiusKM = RadiusKm;
            Planet->PlanetRadiusKM = FMath::RoundToInt(RadiusKm);
            Planet->WorldScapeSeed = Seed;
            Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
            Noise->Configure(Profile, Mode == 1, Mode == 2);
            Root->PlanetScaleCode = RadiusKm * 100000.0;
            Root->NoiseScale = FMath::Max(1, FMath::RoundToInt(Profile.NoiseScale));
            Root->NoiseIntensity = FMath::Max(1, FMath::RoundToInt(Profile.NoiseIntensity));
            Root->PlanetNoise.SetSeed(Seed + 1);
            Root->PlanetNoise.SetSeed(Seed);
            double CaseHeightError = 0.0, CaseVertexError = 0.0;
            constexpr int32 Count = 512;
            for (int32 I = 0; I < Count; ++I)
            {
                const double Z = 1.0 - 2.0 * (I + 0.5) / Count;
                const double R = FMath::Sqrt(1.0 - Z * Z);
                const DVector P = DVector(R * FMath::Cos(I * 2.39996322972865332),
                    R * FMath::Sin(I * 2.39996322972865332), Z) * Root->PlanetScaleCode;
                const double Full = Reference(P), Fast = Candidate(P);
                if (!FMath::IsFinite(Full) || !FMath::IsFinite(Fast))
                { AddError(TEXT("Nonfinite collision height")); World->DestroyWorld(false); return false; }
                CaseHeightError = FMath::Max(CaseHeightError, FMath::Abs(Full - Fast));
                // Native helper is not DLL-exported. Mirror ONLY its spherical
                // displacement expression (WorldScapeWorldType.cpp:188-189);
                // this is a bound on displacement, not a cooked collision test.
                DVector Normal = P;
                Normal.Normalize();
                const FVector A = (Normal * Full).ToFVector();
                const FVector B = (Normal * Fast).ToFVector();
                CaseVertexError = FMath::Max(CaseVertexError, FVector::Distance(A, B));
                ++Samples;
            }
            TestTrue(FString::Printf(TEXT("Native height bounded 1e-6cm type=%d seed=%d radius=%g mode=%d error=%.17g"),
                Id, Seed, RadiusKm, Mode, CaseHeightError), CaseHeightError <= 1.e-6);
            TestTrue(TEXT("Transformed collision vertex bounded 1e-6cm"), CaseVertexError <= 1.e-6);
            MaxHeightErrorCm = FMath::Max(MaxHeightErrorCm, CaseHeightError);
            MaxVertexErrorCm = FMath::Max(MaxVertexErrorCm, CaseVertexError);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("[APS.CollisionHeightParity] families=%d samples=%lld maxHeightErrorCm=%.17g maxVertexErrorCm=%.17g productionEnabled=0"),
        Families, Samples, MaxHeightErrorCm, MaxVertexErrorCm);

    // Nine local 64x64 patches, 1.2m spacing; same four joined batches per patch
    // as current collision sampling. This excludes mesh publication/cooking,
    // therefore the result is NOT an end-to-end flight frame improvement.
    constexpr int32 PatchSize = 64 * 64, PatchCount = 9, Rounds = 5;
    TArray<DVector> Positions;
    Positions.Reserve(PatchSize * PatchCount);
    constexpr double RadiusCm = 675000000.0;
    for (int32 Patch = 0; Patch < PatchCount; ++Patch)
    for (int32 Y = 0; Y < 64; ++Y)
    for (int32 X = 0; X < 64; ++X)
    {
        DVector P(((Patch % 3 - 1) * 63 + X - 31.5) * 120.0,
            ((Patch / 3 - 1) * 63 + Y - 31.5) * 120.0, RadiusCm);
        P.Normalize();
        Positions.Add(P * RadiusCm);
    }
    TArray<double> Heights;
    Heights.SetNumUninitialized(Positions.Num());
    double Checksum = 0.0;
    for (EPlanetType Type : {EPlanetType::Water, EPlanetType::Terrestrial, EPlanetType::Oasis,
        EPlanetType::Frozen, EPlanetType::Volcanic, EPlanetType::Desert})
    {
        Planet->PlanetType = Type;
        Planet->RadiusKM = 6750.0; Planet->PlanetRadiusKM = 6750;
        Planet->WorldScapeSeed = 424242;
        Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
        Noise->Configure(Profile);
        Root->NoiseScale = FMath::Max(1, FMath::RoundToInt(Profile.NoiseScale));
        Root->NoiseIntensity = FMath::Max(1, FMath::RoundToInt(Profile.NoiseIntensity));
        Root->PlanetScaleCode = RadiusCm;
        Root->PlanetNoise.SetSeed(424241); Root->PlanetNoise.SetSeed(424242);
        TArray<double> FullTimes, FastTimes;
        // Warm both paths before measurement; then alternate order each round.
        for (int32 Round = -1; Round < Rounds; ++Round)
        for (int32 Slot = 0; Slot < 2; ++Slot)
        {
            const bool bFast = ((Round + Slot + 2) % 2) != 0;
            const double Start = FPlatformTime::Seconds();
            for (int32 Patch = 0; Patch < PatchCount; ++Patch)
                ParallelFor(TEXT("APS_CollisionHeightPreflight"), PatchSize, PatchSize / 4,
                    [&](int32 Local)
                    {
                        const int32 I = Patch * PatchSize + Local;
                        Heights[I] = bFast ? Candidate(Positions[I]) : Reference(Positions[I]);
                    });
            const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
            for (double H : Heights) Checksum += H;
            if (Round >= 0) (bFast ? FastTimes : FullTimes).Add(Ms);
        }
        FullTimes.Sort(); FastTimes.Sort();
        UE_LOG(LogTemp, Display, TEXT("[APS.CollisionHeightTiming] type=%s samples=%d rounds=%d tasksPerPatch=4 fullMedianMs=%.6f heightMedianMs=%.6f savingPercent=%.3f fullMinMax=%.6f,%.6f heightMinMax=%.6f,%.6f acceptance=SAMPLER_ONLY"),
            *UEnum::GetValueAsString(Type), Positions.Num(), Rounds, FullTimes[Rounds / 2], FastTimes[Rounds / 2],
            (1.0 - FastTimes[Rounds / 2] / FullTimes[Rounds / 2]) * 100.0,
            FullTimes[0], FullTimes.Last(), FastTimes[0], FastTimes.Last());
    }
    TestTrue(TEXT("Timing outputs consumed"), FMath::IsFinite(Checksum) && FMath::Abs(Checksum) > 0.0);
    World->DestroyWorld(false);
    return true;
}
#endif
