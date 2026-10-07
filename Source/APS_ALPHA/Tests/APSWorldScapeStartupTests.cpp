#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeFreshStartupTest,
    "APS.Gameplay.World.PlanetSurface.FreshRootStartup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWorldScapeFreshStartupTest::RunTest(const FString& Parameters)
{
    const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false)
        .RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, false, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Isolated world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    APlanet* Body = World->SpawnActor<APlanet>();
    APlanetarySurfaceGenerator* Generator = World->SpawnActor<APlanetarySurfaceGenerator>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Generator"), Generator)) return false;
    Body->PlanetType = EPlanetType::Terrestrial;
    Body->IsManual = false;
    Body->RadiusKM = 1000.; Body->PlanetRadiusKM = 1000;
    Body->WorldScapeSeed = 27183;
    // Real production profile, not a duplicated cache model. Keep allocation in
    // the final base-build check small; this test measures semantics, not FPS.
    if (!TestTrue(TEXT("Owned root created"), Generator->CreateRuntimeWorldScapeRoot(Body))) return false;
    Generator->ApplySurfaceProfile(Body);
    AWorldScapeRoot* Root = Generator->WorldScapeRootInstance;
    if (!TestTrue(TEXT("Real profile applied"), Generator->bSurfaceProfileApplied)) return false;
    Root->MaxLod = 2; Root->OceanMaxLod = 2;
    Root->LodResolution = 8; Root->OceanLodResolution = 8;
    TestTrue(TEXT("Cold profile cache initially needs regeneration"), Root->CheckForRegenerate(false));
    auto* Noise = Root->WorldScapeNoise;
    const auto Terrain = Root->TerrainMaterial;
    const auto Ocean = Root->OceanMaterial;
    const auto Seed = Root->Seed;
    const auto Radius = Root->PlanetScale;
    Generator->SpawnWorldScapeRoot();
    TestFalse(TEXT("Activation primes only the empty parameter cache"), Root->CheckForRegenerate(false));
    TestFalse(TEXT("Activation leaves native first initialization pending"), Root->init);
    TestTrue(TEXT("No geometry allocated by activation"), Root->WorldScapeLod.IsEmpty()
        && Root->WorldScapeLodOcean.IsEmpty() && Root->CollisionLods.IsEmpty());
    TestEqual(TEXT("Noise identity retained"), Root->WorldScapeNoise, Noise);
    TestEqual(TEXT("Seed retained"), Root->Seed, Seed);
    TestEqual(TEXT("Physical radius retained"), Root->PlanetScale, Radius);
    TestTrue(TEXT("Terrain binding retained"), !(Root->TerrainMaterial != Terrain));
    TestTrue(TEXT("Ocean binding retained"), !(Root->OceanMaterial != Ocean));

    // No world ticking occurs in this synthetic fixture. Null sentinels model
    // protected resident/worker states and are removed BEFORE any cleanup.
    const auto ExpectPreserved = [&](const TCHAR* Label)
    {
        ++Root->Seed;
        Generator->SpawnWorldScapeRoot();
        TestTrue(Label, Root->CheckForRegenerate(false));
    };
    Generator->bOwnsWorldScapeRootInstance = false;
    ExpectPreserved(TEXT("Borrowed root dirty cache is preserved"));
    Generator->bOwnsWorldScapeRootInstance = true;
    Generator->bSurfaceProfileApplied = false;
    ExpectPreserved(TEXT("Unconfigured root dirty cache is preserved"));
    Generator->bSurfaceProfileApplied = true;
    Body->IsManual = true;
    ExpectPreserved(TEXT("Authored planet dirty cache is preserved"));
    Body->IsManual = false;
    Root->init = true;
    ExpectPreserved(TEXT("Initialized root dirty cache is preserved"));
    Root->init = false;
    Root->WorldScapeLod.Add(nullptr);
    ExpectPreserved(TEXT("Resident terrain dirty cache is preserved"));
    Root->WorldScapeLod.Empty();
    Root->WorldScapeLodOcean.Add(nullptr);
    ExpectPreserved(TEXT("Resident ocean dirty cache is preserved"));
    Root->WorldScapeLodOcean.Empty();
    Root->CollisionLods.Add(nullptr);
    ExpectPreserved(TEXT("Resident collision dirty cache is preserved"));
    Root->CollisionLods.Empty();
    Root->WorldScapeLodInGeneration.Add(nullptr, false);
    ExpectPreserved(TEXT("In-flight worker dirty cache is preserved"));
    Root->WorldScapeLodInGeneration.Empty();

    Generator->SpawnWorldScapeRoot();
    TestFalse(TEXT("Truly empty root can prime after guards clear"), Root->CheckForRegenerate(false));
    Root->PlanetLocation = Root->GetActorLocation();
    Root->PlanetScaleCode = Root->PlanetScale;
    Root->GenerateBaseMesh();
    TestEqual(TEXT("First native build creates terrain rings"), Root->WorldScapeLod.Num(), 2);
    TestEqual(TEXT("First native build creates ocean rings"), Root->WorldScapeLodOcean.Num(), Root->bOcean ? 2 : 0);
    TestFalse(TEXT("First native build does not request a second build"), Root->CheckForRegenerate(false));
    UWorldScapeLod* FirstLod = Root->WorldScapeLod.IsEmpty() ? nullptr : Root->WorldScapeLod[0];
    ++Root->Seed;
    Generator->SpawnWorldScapeRoot();
    TestTrue(TEXT("A later genuine profile edit still requests regeneration"), Root->CheckForRegenerate(false));
    TestTrue(TEXT("Reactivation does not replace resident geometry"),
        FirstLod && Root->WorldScapeLod.Num() == 2 && Root->WorldScapeLod[0] == FirstLod);
    return true;
}
#endif
