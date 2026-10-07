#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageCollisionComponent.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceScatter.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageCollisionShapeTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.WalkingCollisionShapes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSFoliageCollisionShapeTest::RunTest(const FString& Parameters)
{
    using Collision = UAPSFoliageCollisionComponent;
    FVector Center, Extent;
    const FBoxSphereBounds Bounds(FVector(0, 0, 300), FVector(100, 100, 300), 340);
    for (const TCHAR* Name : {TEXT("SM_APS_Scatter_Pebble"), TEXT("SM_APS_Scatter_Grass"),
        TEXT("SM_APS_Scatter_ColdGrass"), TEXT("SM_APS_Scatter_DryGrass"), TEXT("UnownedRock")})
        TestFalse(TEXT("Small decoration, grass and unknown geometry stay nonblocking"),
            Collision::MakeLocalBox(Name, Bounds, Center, Extent));
    for (const TCHAR* Name : {TEXT("SM_APS_Scatter_RockA"), TEXT("SM_APS_Scatter_RockB"),
        TEXT("SM_APS_Scatter_SlabA"), TEXT("SM_APS_Scatter_SlabB")})
    {
        TestTrue(TEXT("Every blocking mineral role is admitted"), Collision::MakeLocalBox(Name, Bounds, Center, Extent));
        TestTrue(TEXT("Box fits inside visual bounds"), Extent.GetMax() <= Bounds.BoxExtent.GetMax());
    }
    TestTrue(TEXT("Tree admitted"), Collision::MakeLocalBox(TEXT("SM_APS_Scatter_TreeA"), Bounds, Center, Extent));
    TestEqual(TEXT("Trunk bottom follows source pivot"), Center.Z - Extent.Z, Bounds.Origin.Z - Bounds.BoxExtent.Z);
    TestTrue(TEXT("Canopy stays nonblocking"), Extent.X <= Bounds.BoxExtent.X * .11);
    TestFalse(TEXT("Invalid bounds rejected"), Collision::MakeLocalBox(TEXT("SM_APS_Scatter_RockA"),
        FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0), Center, Extent));
    TestEqual(TEXT("Fixed per-root physics ceiling"), Collision::MaximumProxies, 32);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageCollisionLifecycleTest,
    "APS.Gameplay.World.PlanetSurface.Foliage.WalkingCollisionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSFoliageCollisionLifecycleTest::RunTest(const FString& Parameters)
{
    struct FFixture
    {
        UWorld* World = nullptr;
        IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.WorldScapeFoliage.WalkingCollision"));
        int32 Original = Mode ? Mode->GetInt() : 0;
        EConsoleVariableFlags Flags = Mode ? EConsoleVariableFlags(
            (Mode->GetFlags() & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe) : ECVF_Default;
        ~FFixture()
        {
            if (World) { World->DestroyWorld(false); if (GEngine) GEngine->DestroyWorldContext(World); }
            if (Mode) Mode->Set(Original, Flags);
        }
    } Fixture;
    if (!TestNotNull(TEXT("Walking collision switch"), Fixture.Mode)) return false;
    Fixture.Mode->Set(1, Fixture.Flags);
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    Fixture.World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        false, ERHIFeatureLevel::Num, &Values);
    auto* World = Fixture.World;
    if (!TestNotNull(TEXT("Collision test world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* PC = World->SpawnActor<APlayerController>();
    auto* Walker = World->SpawnActor<ACharacter>();
    auto* FlightPawn = World->SpawnActor<APawn>();
    auto* Root = World->SpawnActor<AWorldScapeRoot>();
    auto* Rock = LoadObject<UStaticMesh>(nullptr, *(FString(APSPlanetSurfaceScatter::Root)
        + TEXT("/SM_APS_Scatter_RockA.SM_APS_Scatter_RockA")));
    if (!TestTrue(TEXT("Fixture actors and published owned mesh"), PC && Walker && FlightPawn && Root && Rock)) return false;
    // Do not tick the world or start terrain workers. Exercise only the collision component.
    Root->bGenerateFoliages = true; Root->SetActorTickEnabled(true); Root->SetActorEnableCollision(true);
    PC->Possess(Walker);
    auto* ISM = NewObject<UInstancedStaticMeshComponent>(Root);
    ISM->SetupAttachment(Root->GetRootComponent()); ISM->SetStaticMesh(Rock);
    ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Root->AddInstanceComponent(ISM); ISM->RegisterComponent();
    const auto Bounds = Rock->GetBounds();
    const double Scale = 50. / Bounds.BoxExtent.GetMax();
    for (int32 I = 0; I < 40; ++I)
        ISM->AddInstance(FTransform(FQuat::Identity,
            FVector(200 + I * 70., 0, 0) - Bounds.Origin * Scale, FVector(Scale)), true);
    auto* Collision = NewObject<UAPSFoliageCollisionComponent>(Root);
    Root->AddInstanceComponent(Collision); Collision->RegisterComponent();
    auto Refresh = [&]() { Collision->TickComponent(.2f, LEVELTICK_All, nullptr); };
    Refresh();
    TestEqual(TEXT("Physics is capped even with more visible objects"), Collision->GetActiveProxyCount(), 32);
    FHitResult Hit;
    FCollisionQueryParams Query; Query.AddIgnoredActor(Walker); Query.AddIgnoredActor(FlightPawn);
    TestTrue(TEXT("Nearby visible rock blocks a real physics trace"), World->LineTraceSingleByChannel(
        Hit, FVector(200, -200, 0), FVector(200, 200, 0), ECC_Visibility, Query));
    TestTrue(TEXT("Hit belongs to local proxy, not visual HISM"), Hit.GetComponent() && Hit.GetComponent()->IsA<UBoxComponent>());
    PC->Possess(FlightPawn); Refresh();
    TestEqual(TEXT("Boarding releases active walking physics"), Collision->GetActiveProxyCount(), 0);
    TArray<UBoxComponent*> Boxes; Root->GetComponents(Boxes);
    TestEqual(TEXT("Boarding destroys pool components"), Boxes.Num(), 0);
    PC->Possess(Walker); Refresh();
    TestEqual(TEXT("Returning on foot restores collision"), Collision->GetActiveProxyCount(), 32);
    const auto ExpectReleased = [&](const TCHAR* Reason)
    {
        Refresh();
        TestEqual(Reason, Collision->GetActiveProxyCount(), 0);
        TestEqual(TEXT("Disabled gate releases the allocated pool"), Collision->GetAllocatedProxyCount(), 0);
    };
    Walker->GetCharacterMovement()->Velocity = FVector(3100, 0, 0);
    ExpectReleased(TEXT("Fast airborne character releases walking physics"));
    Walker->GetCharacterMovement()->StopMovementImmediately(); Refresh();
    TestEqual(TEXT("Slowing on foot restores collision"), Collision->GetActiveProxyCount(), 32);
    Root->SetActorHiddenInGame(true); ExpectReleased(TEXT("Hidden root releases physics"));
    Root->SetActorHiddenInGame(false); Refresh();
    TestEqual(TEXT("Visible root restores collision"), Collision->GetActiveProxyCount(), 32);
    Root->SetActorTickEnabled(false); ExpectReleased(TEXT("Suspended root releases physics"));
    Root->SetActorTickEnabled(true); Refresh();
    TestEqual(TEXT("Resumed root restores collision"), Collision->GetActiveProxyCount(), 32);
    Root->SetActorEnableCollision(false); ExpectReleased(TEXT("Disabled owner collision releases proxies"));
    Root->SetActorEnableCollision(true); Refresh();
    Fixture.Mode->Set(0, Fixture.Flags); ExpectReleased(TEXT("Runtime OFF removes all proxies"));
    Fixture.Mode->Set(1, Fixture.Flags); Refresh();
    TestEqual(TEXT("Runtime ON restores without respawning root"), Collision->GetActiveProxyCount(), 32);
    ISM->UnregisterComponent(); Refresh();
    TestEqual(TEXT("Unregistered visual sector cannot retain ghost collision"), Collision->GetActiveProxyCount(), 0);
    TestFalse(TEXT("Unregistered old rock position no longer blocks"), World->LineTraceSingleByChannel(
        Hit, FVector(200, -200, 0), FVector(200, 200, 0), ECC_Visibility, Query));
    ISM->RegisterComponent(); Refresh();
    TestEqual(TEXT("Re-registered sector restores bounded walking proxies"), Collision->GetActiveProxyCount(), 32);
    ISM->ClearInstances(); Refresh();
    TestEqual(TEXT("Removed foliage leaves no active ghost collision"), Collision->GetActiveProxyCount(), 0);
    TestFalse(TEXT("Old rock position no longer blocks"), World->LineTraceSingleByChannel(
        Hit, FVector(200, -200, 0), FVector(200, 200, 0), ECC_Visibility, Query));
    Root->bGenerateFoliages = false; Refresh();
    Boxes.Reset(); Root->GetComponents(Boxes);
    TestEqual(TEXT("Suspending root releases allocated proxies"), Boxes.Num(), 0);
    return true;
}
#endif
