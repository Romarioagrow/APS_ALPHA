#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageExclusionComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "WorldScapeRoot.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageExclusionGeometryTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.StructureExclusionGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSFoliageExclusionGeometryTest::RunTest(const FString& Parameters)
{
    using E = UAPSFoliageExclusionComponent;
    const FBox Mesh(FVector(-50), FVector(50));
    const FTransform Volume(FRotator(73, 41, -10), FVector(1.e9, -2.e9, 3.e9), FVector(2, .5, 1.5));
    E::FVolume V{Volume, FBox(FVector(-100, -500, -100), FVector(100, 500, 100))};
    const FTransform In(Volume.GetRotation(), Volume.TransformPosition(FVector(110, 0, 0)));
    const FTransform Out(Volume.GetRotation(), Volume.TransformPosition(FVector(160, 0, 0)));
    TestTrue(TEXT("Mesh edge intersects rotated/nonuniform footprint even when pivot outside"), E::Intersects(Mesh, In, V));
    TestFalse(TEXT("Neighbor beyond full extent is preserved"), E::Intersects(Mesh, Out, V));
    FTransform ShiftIn = In; ShiftIn.AddToTranslation(-Volume.GetLocation());
    V.ToRoot.SetLocation(FVector::ZeroVector);
    TestTrue(TEXT("Common origin shift does not change exclusion"), E::Intersects(Mesh, ShiftIn, V));
    V.ToRoot.SetScale3D(FVector::ZeroVector);
    TestFalse(TEXT("Degenerate footprint rejected"), E::Intersects(Mesh, ShiftIn, V));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageExclusionLifecycleTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.StructureExclusionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSFoliageExclusionLifecycleTest::RunTest(const FString& Parameters)
{
    using E = UAPSFoliageExclusionComponent;
    struct FFixture
    {
        UWorld* World = nullptr;
        IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.WorldScapeFoliage.StructureExclusion"));
        int32 Original = Mode ? Mode->GetInt() : 0;
        EConsoleVariableFlags Flags = Mode ? EConsoleVariableFlags((Mode->GetFlags() & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe) : ECVF_Default;
        ~FFixture()
        {
            if (World) { World->DestroyWorld(false); if (GEngine) GEngine->DestroyWorldContext(World); }
            if (Mode) Mode->Set(Original, Flags);
        }
    } F;
    if (!TestNotNull(TEXT("Exclusion cvar"), F.Mode)) return false;
    F.Mode->Set(1, F.Flags);
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    F.World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Fixture world"), F.World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(F.World);
    F.World->InitializeActorsForPlay(FURL());
    auto MakeActor = [&]()
    {
        auto* A = F.World->SpawnActor<AActor>();
        auto* C = NewObject<USceneComponent>(A); A->SetRootComponent(C); A->AddInstanceComponent(C); C->RegisterComponent();
        return A;
    };
    auto* Body = MakeActor(); auto* OtherBody = MakeActor();
    auto* Root = F.World->SpawnActor<AWorldScapeRoot>();
    Root->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
    Root->bGenerateFoliages = true; Root->SetActorTickEnabled(true);
    auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* Rock = LoadObject<UStaticMesh>(nullptr, *(FString(APSPlanetSurfaceScatter::Root) + TEXT("/SM_APS_Scatter_RockA.SM_APS_Scatter_RockA")));
    if (!TestTrue(TEXT("Published mesh available"), Cube && Rock)) return false;
    auto MakeBuilding = [&](AActor* Parent, const TCHAR* Tag, FVector Location)
    {
        auto* A = MakeActor(); A->Tags.Add(Tag); A->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform); A->SetActorLocation(Location);
        auto* Mesh = NewObject<UStaticMeshComponent>(A); Mesh->SetupAttachment(A->GetRootComponent()); Mesh->SetStaticMesh(Cube);
        Mesh->SetRelativeScale3D(FVector(10, 10, 2)); A->AddInstanceComponent(Mesh); Mesh->RegisterComponent();
        return A;
    };
    auto* Base = MakeBuilding(Body, TEXT("APS.Civilization.Base"), FVector::ZeroVector);
    auto* Pad = MakeBuilding(Body, TEXT("APS.Civilization.LandingPad"), FVector(4000, 0, 0));
    MakeBuilding(OtherBody, TEXT("APS.Colony.Module"), FVector(0, 2500, 0));
    auto MakeISM = [&](UStaticMesh* Mesh)
    {
        auto* ISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(Root);
        ISM->SetupAttachment(Root->GetRootComponent()); ISM->SetStaticMesh(Mesh); ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Root->AddInstanceComponent(ISM); ISM->RegisterComponent(); return ISM;
    };
    auto* ISM = MakeISM(Rock); auto* Unowned = MakeISM(Cube);
    const double Scale = 50. / Rock->GetBounds().BoxExtent.GetMax();
    TArray<FTransform> Original;
    for (const FVector Position : {FVector::ZeroVector, FVector(4000, 0, 0), FVector(2000, 0, 0), FVector(0, 2500, 0)})
        Original.Add(FTransform(FQuat::Identity, Position - Rock->GetBounds().Origin * Scale, FVector(Scale)));
    ISM->AddInstances(Original, false); Unowned->AddInstance(FTransform::Identity);
    auto* Exclusion = NewObject<E>(Root); Root->AddInstanceComponent(Exclusion); Exclusion->RegisterComponent();
    auto Refresh = [&]() { Exclusion->RefreshFootprints(); Exclusion->TickComponent(.016f, LEVELTICK_All, nullptr); };
    Refresh();
    TestEqual(TEXT("Two footprints and one access corridor"), Exclusion->GetVolumeCount(), 3);
    TestEqual(TEXT("Building and access instances actually removed"), ISM->GetInstanceCount(), 1);
    TestEqual(TEXT("Removed transforms retained only for this live sector"), Exclusion->GetSuppressedCount(), 3);
    TestEqual(TEXT("Authored/unowned meshes untouched"), Unowned->GetInstanceCount(), 1);
    FTransform Neighbor; ISM->GetInstanceTransform(0, Neighbor, false);
    TestTrue(TEXT("Other-body neighbor stays at exact deterministic position"), Neighbor.Equals(Original[3], .001));
    // Rebase entire hierarchy: stored transforms must remain component-local.
    Body->SetActorLocation(FVector(1.e8, 2.e8, -3.e8)); Refresh();
    TestEqual(TEXT("Rebase does not restore or double-remove objects"), ISM->GetInstanceCount(), 1);
    Pad->Destroy(); Refresh();
    TestEqual(TEXT("Demolition restores pad and corridor but not base"), ISM->GetInstanceCount(), 3);
    F.Mode->Set(0, F.Flags); Refresh();
    TestEqual(TEXT("Switch OFF reversibly restores all natural instances"), ISM->GetInstanceCount(), 4);
    for (const auto& Expected : Original)
    {
        bool Found = false;
        for (int32 I = 0; I < ISM->GetInstanceCount(); ++I)
        { FTransform T; ISM->GetInstanceTransform(I, T, false); Found |= T.Equals(Expected, .001); }
        TestTrue(TEXT("Restoration preserves original local transform set"), Found);
    }
    F.Mode->Set(1, F.Flags); Refresh();
    TestEqual(TEXT("Re-enable reapplies base exclusion"), Exclusion->GetSuppressedCount(), 1);
    // Mirror native sector retirement. New sector has new component identity.
    ISM->ClearInstances(); ISM->DestroyComponent(); Refresh();
    TestEqual(TEXT("Retirement releases removed-transform cache"), Exclusion->GetSuppressedCount(), 0);
    auto* ReturnISM = MakeISM(Rock); ReturnISM->AddInstances(Original, false); Refresh();
    TestEqual(TEXT("Returning regenerated sector filtered again"), ReturnISM->GetInstanceCount(), 3);
    Base->Destroy(); Refresh();
    TestEqual(TEXT("Last structure removal restores survivor positions"), ReturnISM->GetInstanceCount(), 4);
    TestEqual(TEXT("No unprocessed records"), Exclusion->GetPendingCount(), 0);
    return true;
}
#endif
