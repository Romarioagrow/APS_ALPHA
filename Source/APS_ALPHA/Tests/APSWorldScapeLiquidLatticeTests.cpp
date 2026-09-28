#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeLiquidLattice.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeLiquidLatticeTest,
    "APS.Gameplay.World.PlanetSurface.LiquidLattice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeLiquidLatticeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Requested default is 256"), APSWorldScapeLiquidLattice::DefaultTerrainResolution, 256);
    TestEqual(TEXT("Default is not silently clamped to 192"),
        APSWorldScapeLiquidLattice::TerrainResolution(true, false,
            APSWorldScapeLiquidLattice::DefaultTerrainResolution), 256);
    for (int32 Requested : {-1, 96, 97, 128, 191, 192, 193, 255, 256, 300, 512})
    {
        const int32 Actual = APSWorldScapeLiquidLattice::TerrainResolution(true, false, Requested);
        TestTrue(TEXT("Owned resolution is bounded and divisible by four"), Actual >= 96 && Actual <= 256 && Actual % 4 == 0);
        TestEqual(TEXT("Authored resolution policy preserved"), APSWorldScapeLiquidLattice::TerrainResolution(false, false, Requested), 96);
        TestEqual(TEXT("Preview resolution policy preserved"), APSWorldScapeLiquidLattice::TerrainResolution(true, true, Requested), 48);
    }
    TestEqual(TEXT("Previous geometry budget remains selectable"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 96), 96);
    TestEqual(TEXT("Previous 192 budget remains selectable"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 192), 192);
    TestEqual(TEXT("Base resolution alignment"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 193), 196);
    TestEqual(TEXT("512 remains capped at approved 256"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 512), 256);
    for (int32 Requested : {-1, 0, 96, 193, 256, 999})
    {
        using namespace APSWorldScapeLiquidLattice;
        TestEqual(TEXT("Dry bodies preserve budget"), CoastResolution(192, true, false, false, Requested), 192);
        TestEqual(TEXT("Authored bodies preserve budget"), CoastResolution(96, false, false, true, Requested), 96);
        TestEqual(TEXT("Preview bodies preserve budget"), CoastResolution(48, true, true, true, Requested), 48);
        const int32 Actual = CoastResolution(192, true, false, true, Requested);
        TestTrue(TEXT("Coast resolution is bounded, aligned and never reduces base"),
            Actual >= 192 && Actual <= 256 && Actual % 4 == 0);
    }
    TestEqual(TEXT("Coast refinement rollback"), APSWorldScapeLiquidLattice::CoastResolution(192, true, false, true, 0), 192);
    TestEqual(TEXT("Coast refinement alignment"), APSWorldScapeLiquidLattice::CoastResolution(192, true, false, true, 193), 196);
    TestEqual(TEXT("Coast refinement target"), APSWorldScapeLiquidLattice::CoastResolution(192, true, false, true, 256), 256);
    TestEqual(TEXT("Wet body inherits 256 default without separate coast override"),
        APSWorldScapeLiquidLattice::CoastResolution(256, true, false, true, 0), 256);
    TestEqual(TEXT("Dry body retains approved 256 base"),
        APSWorldScapeLiquidLattice::CoastResolution(256, true, false, false, 0), 256);
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
        ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Lattice test world"), World)) return false;
    auto* Root = World->SpawnActor<AWorldScapeRoot>();
    if (!TestNotNull(TEXT("WorldScape root"), Root)) { World->DestroyWorld(false); return false; }
    Root->MaxLod = 10; Root->LodResolution = 192; Root->TriangleSize = 120.0f;
    for (int32 Case = 0; Case < 8; ++Case)
    {
        const bool Owned = (Case & 1) != 0, Preview = (Case & 2) != 0, Ocean = (Case & 4) != 0;
        Root->bOcean = Ocean;
        Root->OceanMaxLod = 9; Root->OceanLodResolution = 64; Root->OceanTriangleSize = 200.0f;
        const bool Match = Owned && !Preview && Ocean;
        TestEqual(TEXT("Only owned full-scale liquid roots change"),
            APSWorldScapeLiquidLattice::MatchTerrain(*Root, Owned, Preview), Match);
        TestEqual(TEXT("Ocean max LOD"), Root->OceanMaxLod, Match ? 10 : 9);
        TestEqual(TEXT("Ocean resolution"), Root->OceanLodResolution, Match ? 192 : 64);
        TestEqual(TEXT("Ocean spacing"), Root->OceanTriangleSize, Match ? 120.0f : 200.0f);
        TestEqual(TEXT("Terrain max LOD preserved"), Root->MaxLod, 10);
        TestEqual(TEXT("Terrain resolution preserved"), Root->LodResolution, 192);
        TestEqual(TEXT("Terrain spacing preserved"), Root->TriangleSize, 120.0f);
    }
    const int32 CollisionResolution = Root->CollisionResolution;
    const float CollisionTriangleSize = Root->CollisionTriangleSize;
    Root->bOcean = true;
    Root->LodResolution = 256;
    APSWorldScapeLiquidLattice::MatchTerrain(*Root, true, false);
    TestEqual(TEXT("Refined coast uses matched liquid resolution"), Root->OceanLodResolution, 256);
    TestEqual(TEXT("Coast refinement preserves collision resolution"), Root->CollisionResolution, CollisionResolution);
    TestEqual(TEXT("Coast refinement preserves collision spacing"), Root->CollisionTriangleSize, CollisionTriangleSize);
    World->DestroyWorld(false);
    return true;
}
#endif
