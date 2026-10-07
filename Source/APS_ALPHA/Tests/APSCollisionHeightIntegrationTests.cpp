#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#if WITH_DEV_AUTOMATION_TESTS && defined(APS_COLLISION_HEIGHT_HOOK)
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/TriangleMeshImplicitObject.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCollisionHeightIntegration,
    "APS.Gameplay.World.PlanetSurface.CollisionHeightIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCollisionHeightIntegration::RunTest(const FString& Parameters)
{
    auto* Switch = IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.CollisionHeightOnly"));
    if (!TestNotNull(TEXT("Native collision switch"), Switch)) return false;
    const int32 Saved = Switch->GetInt();
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
        ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Collision integration world"), World)) return false;
    auto* Planet = World->SpawnActor<APlanet>();
    auto* Root = World->SpawnActor<AWorldScapeRoot>();
    const auto* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
    if (!TestNotNull(TEXT("Planet"), Planet) || !TestNotNull(TEXT("Root"), Root)
        || !TestNotNull(TEXT("Catalog"), Catalog))
    { World->DestroyWorld(false); return false; }
    Root->SetActorTickEnabled(false);
    Root->bFlatWorld = false;
    Root->bUsePlanetaryHeightMap = false;
    Root->bPlanetaryInvertMapBlend = false;
    Root->GenerationType = EWorldScapeType::Planet;
    Root->Tags.AddUnique(TEXT("APS.Collision.ParallelSamples"));
    constexpr double Radius = 583170600.0;
    Root->PlanetScaleCode = Radius;
    Root->NoiseOffset = FVector(1.25, -2.5, 4.75);
    Root->PlanetNoise.SetSeed(424241); Root->PlanetNoise.SetSeed(424242);
    auto* Noise = NewObject<UAPSWorldScapePlanetNoise>(Root);
    Root->WorldScapeNoise = Noise;
    TestTrue(TEXT("Exact APS generator supports height-only"), Noise->SupportsCollisionHeightOnly());
    Planet->RadiusKM = 5831.706; Planet->PlanetRadiusKM = 5832;
    Planet->WorldScapeSeed = 424242;
    Planet->Temperature = 288;
    Planet->PlanetAtmosphere.Humidity = 42.0f;
    Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
    int64 VerticesChecked = 0;
    int32 CookedCases = 0;
    for (EPlanetType Type : {EPlanetType::Water, EPlanetType::Terrestrial, EPlanetType::Oasis,
        EPlanetType::Frozen, EPlanetType::Volcanic, EPlanetType::Desert})
    for (int32 Resolution : {17, 67})
    for (int32 Mode : {0, 1, 2})
    {
        Planet->PlanetType = Type;
        const auto P = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
        Noise->Configure(P, Mode == 1, Mode == 2);
        Root->NoiseScale = FMath::Max(1, FMath::RoundToInt(P.NoiseScale));
        Root->NoiseIntensity = FMath::Max(1, FMath::RoundToInt(P.NoiseIntensity));
        const DVector Position(10000, -3000, Radius);
        auto* Lod = NewObject<UWorldScapeLod>(Root);
        Lod->Mesh->bUseAsyncCooking = false;
        Lod->Init(0, Resolution, 120.0f, Position, FWSMaterialLodArray{},
            Radius, DVector(0), false, true);
        auto Generate = [&]()
        {
            ColisionGeneration Work(Lod, Root, Position, DVector(0), FVector2D::ZeroVector,
                Radius, 0, 120.0f, Resolution, 0, FVector::UpVector, false);
            Lod->SetMesh();
        };
        Switch->Set(0, ECVF_SetByCode);
        Generate();
        const auto ExpectedVertices = Lod->Vertices;
        const auto ExpectedColors = Lod->VertexColors;
        const auto ExpectedTriangles = Lod->Triangles;
        FTriMeshCollisionData ExpectedPhysics;
        Lod->Mesh->GetPhysicsTriMeshData(&ExpectedPhysics, true);
        Switch->Set(1, ECVF_SetByCode);
        Generate();
        TestEqual(TEXT("Height-only path actually used"), Lod->CollisionHeightOnlyPositions.Num(), ExpectedVertices.Num());
        TestTrue(TEXT("Topology retained exactly"), Lod->Triangles == ExpectedTriangles);
        TestEqual(TEXT("Vertex count retained"), Lod->Vertices.Num(), ExpectedVertices.Num());
        double MaxError = 0.0;
        for (int32 I = 0; I < FMath::Min(ExpectedVertices.Num(), Lod->Vertices.Num()); ++I)
            MaxError = FMath::Max(MaxError, FVector::Distance(ExpectedVertices[I], Lod->Vertices[I]));
        TestTrue(TEXT("Native collision positions differ by at most 1e-6cm"), MaxError <= 1.e-6);
        VerticesChecked += Lod->Vertices.Num();
        FTriMeshCollisionData ActualPhysics;
        Lod->Mesh->GetPhysicsTriMeshData(&ActualPhysics, true);
        TestTrue(TEXT("Float physics vertices exactly equal"), ActualPhysics.Vertices == ExpectedPhysics.Vertices);
        TestEqual(TEXT("Physics triangle count equal"), ActualPhysics.Indices.Num(), ExpectedPhysics.Indices.Num());
        bool SameIndices = ActualPhysics.Indices.Num() == ExpectedPhysics.Indices.Num();
        for (int32 I = 0; I < FMath::Min(ActualPhysics.Indices.Num(), ExpectedPhysics.Indices.Num()); ++I)
            SameIndices &= ActualPhysics.Indices[I].v0 == ExpectedPhysics.Indices[I].v0
                && ActualPhysics.Indices[I].v1 == ExpectedPhysics.Indices[I].v1
                && ActualPhysics.Indices[I].v2 == ExpectedPhysics.Indices[I].v2;
        TestTrue(TEXT("Physics triangle indices exactly equal"), SameIndices);
        TestTrue(TEXT("Physics material indices equal"), ActualPhysics.MaterialIndices == ExpectedPhysics.MaterialIndices);
        UBodySetup* Body = Lod->Mesh->GetBodySetup();
        if (!TestNotNull(TEXT("Cooked collision body"), Body))
        {
            Switch->Set(Saved, ECVF_SetByCode);
            Lod->Mesh->DestroyComponent(); Lod->DestroyComponent();
            World->DestroyWorld(false);
            return false;
        }
        const bool Cooked = Body && Body->bCreatedPhysicsMeshes && !Body->bFailedToCreatePhysicsMeshes
            && !Body->TriMeshGeometries.IsEmpty();
        TestTrue(TEXT("Nonempty Chaos trimesh successfully cooked"), Cooked);
        CookedCases += Cooked ? 1 : 0;
        const auto CookedGeometry = Body->TriMeshGeometries;
        const auto BeforeDebugPositions = Lod->Vertices;
        Root->RestoreCollisionSampleColors(Lod);
        TestTrue(TEXT("Debug semantic colours restored exactly"), Lod->VertexColors == ExpectedColors);
        TestTrue(TEXT("Debug restoration releases sample cache"), Lod->CollisionHeightOnlyPositions.IsEmpty());
        TestTrue(TEXT("Debug restoration does not move vertices"), Lod->Vertices == BeforeDebugPositions);
        TestTrue(TEXT("Debug restoration retains cooked body"), Body == Lod->Mesh->GetBodySetup()
            && CookedGeometry == Body->TriMeshGeometries);
        auto* Section = Lod->Mesh->GetProcMeshSection(0);
        bool RenderColorsEqual = Section && Section->PlanetVertexBuffer.Num() == ExpectedColors.Num();
        if (RenderColorsEqual)
            for (int32 I = 0; I < ExpectedColors.Num(); ++I)
                RenderColorsEqual &= Section->PlanetVertexBuffer[I].Color == ExpectedColors[I].ToFColor(false);
        TestTrue(TEXT("Debug render colours restored too"), RenderColorsEqual);
        // Each unsupported mode must use the old full path, not a new fallback
        // approximation. No live actor-volume fixture is claimed by these guards.
        for (int32 Guard : {0, 1, 2, 3})
        {
            Root->bDisplayCollision = Guard == 0;
            Root->bUsePlanetaryHeightMap = Guard == 1;
            Root->bPlanetaryInvertMapBlend = Guard == 2;
            if (Guard == 3) Root->Tags.Remove(TEXT("APS.Collision.ParallelSamples"));
            Generate();
            TestTrue(TEXT("Unsupported mode preserves full sampler"), Lod->CollisionHeightOnlyPositions.IsEmpty());
            Root->Tags.AddUnique(TEXT("APS.Collision.ParallelSamples"));
        }
        Root->bDisplayCollision = false; Root->bUsePlanetaryHeightMap = false;
        Root->bPlanetaryInvertMapBlend = false;
        Lod->Mesh->DestroyComponent(); Lod->DestroyComponent();
    }
    Switch->Set(Saved, ECVF_SetByCode);
    World->DestroyWorld(false);
    AddInfo(FString::Printf(TEXT("Height-only native integration: %lld vertices, %d cooked cases; contact/ship motion remains separate"),
        VerticesChecked, CookedCases));
    return true;
}
#endif
