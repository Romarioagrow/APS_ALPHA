#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WorldScapeRoot.h"
#include "WorldScapeNoise/Public/WorldScapeNoiseClass.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeCollisionSamplingTest,
    "APS.Gameplay.World.PlanetSurface.CollisionSamplingParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeCollisionSamplingTest::RunTest(const FString& Parameters)
{
    auto* Tasks = IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.CollisionSampleTasks"));
    if (!TestNotNull(TEXT("Bounded sampling switch"), Tasks)) return false;
    const int32 SavedTasks = Tasks->GetInt();
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Collision parity world"), World)) return false;
    auto* Root = World->SpawnActor<AWorldScapeRoot>();
    if (!TestNotNull(TEXT("Collision parity root"), Root))
    { World->DestroyWorld(false); return false; }
    Root->SetActorTickEnabled(false);
    constexpr double Radius = 10000000.0;
    Root->PlanetScaleCode = Radius;
    Root->NoiseIntensity = 100000.0f;
    Root->NoiseScale = 800.0f;
    Root->bUsePlanetaryHeightMap = false;
    Root->PlanetNoise.SetSeed(1337);
    auto* Noise = NewObject<UWorldScapeCustomNoise>(Root);
    if (!Noise->NoiseParameters) Noise->NoiseParameters = NewObject<UCustomNoiseParameter>(Noise);
    Root->WorldScapeNoise = Noise;
    UWorldScapeLod* Lod = NewObject<UWorldScapeLod>(Root);
    const FName Tag(TEXT("APS.Collision.ParallelSamples"));
    int64 Checked = 0;
    for (bool bFlat : {false, true})
    for (int32 Resolution : {17, 65})
    for (const DVector Position : {DVector(0, 0, Radius), DVector(10000, -3000, Radius)})
    {
        Root->bFlatWorld = bFlat;
        Lod->Init(0, Resolution, 200.0f, Position, FWSMaterialLodArray{},
            Radius, DVector(0), false, true);
        Root->Tags.AddUnique(Tag);
        Tasks->Set(1, ECVF_SetByCode);
        ColisionGeneration Serial(Lod, Root, Position, DVector(0), FVector2D::ZeroVector,
            Radius, 0, 200.0f, Resolution, 0, FVector::UpVector, bFlat);
        const TArray<FVector> ExpectedVertices = Lod->Vertices;
        const TArray<FLinearColor> ExpectedColors = Lod->VertexColors;
        const TArray<int32> ExpectedTriangles = Lod->Triangles;
        // Repeat both opt-in and opt-out with a >1 task setting. Untagged noise
        // stays serial; all variants retain exactly the same complete payload.
        Tasks->Set(4, ECVF_SetByCode);
        for (bool bOptIn : {true, false})
        {
            if (bOptIn) Root->Tags.AddUnique(Tag); else Root->Tags.Remove(Tag);
            ColisionGeneration Actual(Lod, Root, Position, DVector(0), FVector2D::ZeroVector,
                Radius, 0, 200.0f, Resolution, 0, FVector::UpVector, bFlat);
            TestTrue(TEXT("Exact collision vertex parity"), Lod->Vertices == ExpectedVertices);
            TestTrue(TEXT("Exact semantic color parity"), Lod->VertexColors == ExpectedColors);
            TestTrue(TEXT("Topology and resolution retained"), Lod->Triangles == ExpectedTriangles);
            Checked += Lod->Vertices.Num();
        }
    }
    Tasks->Set(SavedTasks, ECVF_SetByCode);
    Lod->Mesh->DestroyComponent();
    Lod->DestroyComponent();
    World->DestroyWorld(false);
    AddInfo(FString::Printf(TEXT("Collision serial/parallel/untagged parity: %lld vertices; not a flight performance or render acceptance"), Checked));
    return true;
}
#endif
