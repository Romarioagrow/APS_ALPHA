#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeLiquidLattice.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeLiquidLatticeTest,
    "APS.Gameplay.World.PlanetSurface.LiquidLattice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeLiquidLatticeTest::RunTest(const FString& Parameters)
{
    for (int32 Requested : {-1, 96, 97, 128, 191, 192, 300})
    {
        const int32 Actual = APSWorldScapeLiquidLattice::TerrainResolution(true, false, Requested);
        TestTrue(TEXT("Owned resolution is bounded and divisible by four"), Actual >= 96 && Actual <= 192 && Actual % 4 == 0);
        TestEqual(TEXT("Authored resolution policy preserved"), APSWorldScapeLiquidLattice::TerrainResolution(false, false, Requested), 96);
        TestEqual(TEXT("Preview resolution policy preserved"), APSWorldScapeLiquidLattice::TerrainResolution(true, true, Requested), 48);
    }
    TestEqual(TEXT("Previous geometry budget remains selectable"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 96), 96);
    TestEqual(TEXT("Coast refinement budget"), APSWorldScapeLiquidLattice::TerrainResolution(true, false, 192), 192);
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
    World->DestroyWorld(false);
    return true;
}
#endif
