#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WorldScapeRoot.h"
#include "WorldScapeOceanDepth.h"
#include "WorldScapeOceanDepthTestNoise.h"
#include "Async/Async.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldScapeOceanDepthWorkerTest,
    "WorldScape.APS.OceanDepth.WorkerCoordinates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldScapeOceanDepthWorkerTest::RunTest(const FString& Parameters)
{
    // Exercise the real worker calculation on the thread pool, with a known field
    // and explicit original coordinates. This is not APS catalog or GPU coverage.
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        false, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Isolated worker world"), World)) return false;
    AWorldScapeRoot* Root = World->SpawnActorDeferred<AWorldScapeRoot>(
        AWorldScapeRoot::StaticClass(), FTransform::Identity);
    if (!TestNotNull(TEXT("Deferred root; no live generation"), Root))
    {
        World->DestroyWorld(false);
        return false;
    }
    constexpr double Radius = 675000000.0;
    Root->GenerationType = EWorldScapeType::Planet;
    Root->PlanetScaleCode = Radius;
    Root->bFlatWorld = false;
    Root->bUsePlanetaryHeightMap = false;
    Root->bEnableVolumes = false;
    Root->NoiseOffset = FVector::ZeroVector;
    Root->PlanetLocation = DVector(0.0);
    Root->OceanHeight = 1000.0;
    UWorldScapeOceanDepthTestNoise* Noise = NewObject<UWorldScapeOceanDepthTestNoise>(Root);
    Root->WorldScapeNoise = Noise;
    UWorldScapeLod* Lod = NewObject<UWorldScapeLod>(Root);
    Lod->Mesh = NewObject<UWorldScapeMeshComponent>(Root);
    Lod->RelativePosition = DVector(0.0, 0.0, Radius);

    int32 Wet = 0, Dry = 0, CheckedVertices = 0;
    for (int32 Level : {0, 1, 5, 9})
    {
        const double Step = 120.0 * FMath::Pow(2.0, Level);
        TArray<DVector> SurfacePositions;
        for (int32 Y = -2; Y <= 2; ++Y)
            for (int32 X = -2; X <= 2; ++X)
            {
                DVector P(X * Step * 8.0, Y * Step * 8.0, Radius);
                P.Normalize();
                SurfacePositions.Add(P * Radius);
            }
        TArray<FVector2D> FirstOriginDepth;
        for (int32 Origin = 0; Origin < 2; ++Origin)
        {
            const DVector Relative(Origin * 138124.5, Origin * -218737.25, Radius - Origin * 500.5);
            const DVector Offset(Origin * 417.25, Origin * -118.5, Origin * 62.25);
            TArray<LodData> Baseline;
            for (int32 Mode = 0; Mode < 4; ++Mode)
            {
                const bool bPayload = Mode == 1;
                Lod->WaterBody = Mode != 2;
                const bool bFlat = Mode == 3;
                Root->bFlatWorld = bFlat;
                // Modes: ocean off, ocean on, tagged ground, tagged flat ocean.
                Root->Tags.Remove(WorldScapeOceanDepth::RootTag());
                if (Mode != 0) Root->Tags.Add(WorldScapeOceanDepth::RootTag());
                LodGenerationThread Worker(Lod, Root, Relative, DVector(0.0),
                    FVector2D::ZeroVector, Radius, Level, 120.f, 16, 1000.0,
                    FVector::UpVector, bFlat, false);
                Worker.OffSetHelper = Offset;
                LodData* Patches[] = {&Worker.MainPatch, &Worker.PatchA, &Worker.PatchB};
                for (int32 Section = 0; Section < 3; ++Section)
                {
                    for (int32 I = 0; I < SurfacePositions.Num(); ++I)
                    {
                        const int32 Index = (I + Section * 7) % SurfacePositions.Num();
                        Patches[Section]->Vertices.Add(SurfacePositions[Index] - Relative - Offset);
                    }
                }
                Noise->GroundCalls = Noise->OceanCalls = 0;
                const bool bRanOffGameThread = Async(EAsyncExecution::ThreadPool, [&Worker]()
                {
                    const bool bOffThread = !IsInGameThread();
                    Worker.CalculateNoise();
                    return bOffThread;
                }).Get();
                TestTrue(TEXT("Real calculation ran on worker pool"), bRanOffGameThread);
                const int32 Count = SurfacePositions.Num() * 3;
                TestEqual(TEXT("No unnecessary second ground evaluation outside opt-in ocean"),
                    Noise->GroundCalls, Mode == 1 || Mode == 2 ? Count : 0);
                TestEqual(TEXT("One ocean evaluation per ocean vertex"),
                    Noise->OceanCalls, Mode == 2 ? 0 : Count);
                for (int32 Section = 0; Section < 3; ++Section)
                {
                    const LodData& Patch = *Patches[Section];
                    TestEqual(TEXT("Payload present only for opted-in spherical ocean"),
                        Patch.UV1.Num(), bPayload ? SurfacePositions.Num() : 0);
                    if (Mode == 0) Baseline.Add(Patch);
                    if (!bPayload) continue;
                    TestTrue(TEXT("Enabling depth leaves ocean vertices exactly unchanged"),
                        Patch.Vertices == Baseline[Section].Vertices);
                    TestTrue(TEXT("Enabling depth leaves semantic colors exactly unchanged"),
                        Patch.VertexColors == Baseline[Section].VertexColors);
                    for (int32 I = 0; I < Patch.UV1.Num(); ++I)
                    {
                        const int32 Index = (I + Section * 7) % SurfacePositions.Num();
                        const DVector& P = SurfacePositions[Index];
                        const double OceanCm = FMath::Max(1000.0,
                            UWorldScapeOceanDepthTestNoise::SeaHeight(P));
                        const double ExpectedKm = (OceanCm -
                            UWorldScapeOceanDepthTestNoise::GroundHeight(P)) * 1.0e-5;
                        const FVector2D Actual = Patch.UV1[I];
                        TestTrue(TEXT("Full-scale coordinate, clamp, units and sign match analytic field"),
                            Actual.Equals(FVector2D(ExpectedKm, 1.0), 1.0e-10));
                        if (Origin == 0 && Section == 0) FirstOriginDepth.Add(Actual);
                        else TestTrue(TEXT("Shared points agree across section layout and shifted origin"),
                            Actual.Equals(FirstOriginDepth[Index], 1.0e-10));
                        Wet += ExpectedKm > 0.0; Dry += ExpectedKm < 0.0;
                        ++CheckedVertices;
                    }
                }
            }
        }
    }
    TestTrue(TEXT("Both wet and dry terrain were covered"), Wet > 0 && Dry > 0);
    // Now let the worker construct and publish its actual 96-resolution ring,
    // instead of supplying vertices ourselves. Reuse the LOD after opting out.
    Root->bFlatWorld = false;
    Root->NoiseIntensity = 100000;
    Root->HeightAnchor = 10000.0;
    Lod->WaterBody = true;
    Root->WorldScapeLodInGeneration.Add(Lod, false);
    DVector Axis(.11, .28, 1.0);
    Axis.Normalize();
    const DVector RingOrigin = Axis * (Radius + 20000.0);
    int32 GeneratedVertices = 0;
    for (int32 Level : {0, 5, 9})
    {
        for (const FVector2D Sub : {FVector2D(0,0), FVector2D(1,1)})
        {
            TArray<TArray<FVector>> OriginalVertices;
            TArray<TArray<FLinearColor>> OriginalColors;
            for (int32 Mode = 0; Mode < 3; ++Mode)
            {
                Root->Tags.Remove(WorldScapeOceanDepth::RootTag());
                if (Mode == 1) Root->Tags.Add(WorldScapeOceanDepth::RootTag());
                LodGenerationThread Worker(Lod, Root, RingOrigin, DVector(0.0),
                    Sub, Radius, Level, 120.f, 96, 12000.0, Axis.ToFVector(), false, false);
                Async(EAsyncExecution::ThreadPool, [&Worker]() { Worker.DoWork(); }).Get();
                const TArray<FVector>* Positions[] = {&Lod->Vertices, &Lod->VerticesPA, &Lod->VerticesPB};
                const TArray<FLinearColor>* Colors[] = {&Lod->VertexColors, &Lod->VerticesColorPA, &Lod->VerticesColorPB};
                const TArray<FVector2D>* Depth[] = {&Lod->UV1, &Lod->UV1PA, &Lod->UV1PB};
                for (int32 Section = 0; Section < 3; ++Section)
                {
                    const auto& P = *Positions[Section];
                    const auto& UV1 = *Depth[Section];
                    TestTrue(TEXT("Generated section is nonempty"), P.Num() > 0);
                    if (Mode == 0)
                    {
                        OriginalVertices.Add(P);
                        OriginalColors.Add(*Colors[Section]);
                        continue;
                    }
                    TestTrue(TEXT("Full ring construction preserves original geometry"), P == OriginalVertices[Section]);
                    TestTrue(TEXT("Full ring construction preserves original semantic colors"), *Colors[Section] == OriginalColors[Section]);
                    if (!TestEqual(TEXT("Published UV1 covers the whole generated section"), UV1.Num(), P.Num())) continue;
                    for (int32 I = 0; I < P.Num(); ++I)
                    {
                        if (Mode == 2)
                        {
                            TestEqual(TEXT("Reused published ring clears obsolete validity"), UV1[I], FVector2D::ZeroVector);
                            continue;
                        }
                        DVector Position = DVector(P[I]) + Lod->RelativePosition;
                        Position.Normalize();
                        Position = Position * Radius;
                        const double ExpectedKm = (FMath::Max(1000.0,
                            UWorldScapeOceanDepthTestNoise::SeaHeight(Position)) -
                            UWorldScapeOceanDepthTestNoise::GroundHeight(Position)) * 1.0e-5;
                        TestTrue(TEXT("Generated LOD payload agrees with physical analytic column"),
                            UV1[I].Equals(FVector2D(ExpectedKm, 1.0), 1.0e-9));
                        ++GeneratedVertices;
                    }
                }
                TestTrue(TEXT("Worker completed publication"), Root->WorldScapeLodInGeneration[Lod]);
                TestTrue(TEXT("Temporary payload recycled after publication"), Worker.MainPatch.UV1.IsEmpty()
                    && Worker.PatchA.UV1.IsEmpty() && Worker.PatchB.UV1.IsEmpty());
            }
        }
    }
    Root->WorldScapeLodInGeneration.Remove(Lod);
    UE_LOG(LogTemp, Display, TEXT("[WS.OceanDepthWorker] vertices=%d wet=%d dry=%d levels=0,1,5,9 sections=3 origins=2 threadPool=1 fixture=analytic gameplayCoverage=0"),
        CheckedVertices, Wet, Dry);
    UE_LOG(LogTemp, Display, TEXT("[WS.OceanDepthPublished] vertices=%d resolution=96 levels=0,5,9 subpositions=2 optOutReused=1 fixture=analytic gameplayCoverage=0"), GeneratedVertices);
    Root->WorldScapeNoise = nullptr;
    World->DestroyWorld(false);
    return true;
}
#endif
