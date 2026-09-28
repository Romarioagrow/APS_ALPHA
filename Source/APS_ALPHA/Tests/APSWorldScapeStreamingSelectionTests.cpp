#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace APSWorldScapeStreamingSelectionTests
{
    struct FScopedWorld
    {
        UWorld* World;
        ~FScopedWorld() { if (World) World->DestroyWorld(false); }
    };

    struct FScopedStandbyBudget
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.MaxStandbyRoots"));
        FString Saved = Variable ? Variable->GetString() : FString();
        EConsoleVariableFlags Flags = Variable ? static_cast<EConsoleVariableFlags>(
            (Variable->GetFlags() & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe) : ECVF_Default;
        ~FScopedStandbyBudget() { if (Variable) Variable->Set(*Saved, Flags); }
        void Set(int32 Value) { if (Variable) Variable->Set(Value, Flags); }
    };

    void Place(AActor* Actor, const FVector& Location)
    {
        if (!Actor->GetRootComponent())
        {
            auto* Component = NewObject<USceneComponent>(Actor);
            Actor->AddInstanceComponent(Component);
            Actor->SetRootComponent(Component);
            Component->RegisterComponent();
        }
        Actor->SetActorLocation(Location);
    }

    void Configure(APlanetaryBody* Body, double RadiusKM, const FVector& Location)
    {
        Body->bStreamWorldScapeSurface = true;
        Body->PlanetType = EPlanetType::Rocky;
        Body->RadiusKM = RadiusKM;
        Body->PlanetRadiusKM = static_cast<int32>(RadiusKM);
        Body->WorldScapePresentationScale = 1.0;
        Place(Body, Location);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeStreamingSelectionTest,
    "APS.Gameplay.World.PlanetSurface.StreamingSelectionAndStandby",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeStreamingSelectionTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldScapeStreamingSelectionTests;
    FScopedStandbyBudget Budget;
    if (!TestNotNull(TEXT("Standby budget CVar"), Budget.Variable)) return false;
    Budget.Set(2);
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    FScopedWorld Fixture{UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
        false, ERHIFeatureLevel::Num, &Values)};
    UWorld* World = Fixture.World;
    if (!TestNotNull(TEXT("Selection test world"), World)) return false;
    auto* Streaming = World->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>();
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Pawn = World->SpawnActor<APawn>();
    auto* First = World->SpawnActor<APlanet>();
    auto* Second = World->SpawnActor<APlanet>();
    auto* GasParent = World->SpawnActor<APlanet>();
    auto* SolidMoon = World->SpawnActor<AMoon>();
    auto* GasMoon = World->SpawnActor<AMoon>();
    if (!TestTrue(TEXT("Selection fixture actors"), Streaming && Controller && Pawn
        && First && Second && GasParent && SolidMoon && GasMoon)) return false;

    // Only manually tick the subsystem: no world/WorldScape tick, workers, movement
    // or render acceptance. Exercise real family selection and root lifecycle.
    Configure(First, 1000, FVector::ZeroVector);
    Configure(Second, 1000, FVector(6e8, 0, 0));
    Configure(GasParent, 1000, FVector(3e9, 0, 0));
    GasParent->PlanetType = EPlanetType::GasGiant;
    Configure(SolidMoon, 100, FVector(2.95e9, 0, 0));
    Configure(GasMoon, 100, FVector(2.94e9, 0, 0));
    SolidMoon->ParentPlanet = GasParent;
    GasMoon->ParentPlanet = GasParent;
    GasMoon->PlanetType = EPlanetType::GasGiant;
    Place(Pawn, FVector(1.1e8, 0, 0));
    Controller->Possess(Pawn);
    if (!TestTrue(TEXT("First controller supplies observer"),
        World->GetFirstPlayerController() == Controller && Controller->GetPawn() == Pawn)) return false;
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Initially nearest family selected"), Streaming->GetResidentFamily() == First);
    TestTrue(TEXT("Initially nearest surface active"), Streaming->GetActiveBody() == First);

    Place(Pawn, FVector(5e8, 0, 0)); // Still well inside the old family's retained core.
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Closer overlapping family overrides old resident lock"), Streaming->GetResidentFamily() == Second);
    TestTrue(TEXT("Closer overlapping body active"), Streaming->GetActiveBody() == Second);
    TestTrue(TEXT("Old family actually released"), First->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded);
    FAPSCivilizationFootprintRequest DeferredRequest;
    FAPSCivilizationFootprintResult DeferredSite;
    TestFalse(TEXT("Remote unloaded home cannot steal streaming ownership for placement"),
        UAPSPlanetSurfacePlacementResolver::TryResolveCivilizationFootprint(First, DeferredRequest, DeferredSite));
    TestTrue(TEXT("Deferred placement does not recreate the remote home surface"),
        First->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
        && Streaming->GetActiveBody() == Second);
    Place(Pawn, FVector(1.1e8, 0, 0));
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Return can select first family again"), Streaming->GetActiveBody() == First);

    Place(Pawn, FVector(2.9401e9, 0, 0)); // Gas moon is closest but must not take the slot.
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Solid moon of gas parent remains eligible"), Streaming->GetResidentFamily() == GasParent
        && Streaming->GetActiveBody() == SolidMoon);
    TestTrue(TEXT("Gas moon never acquires root"), GasMoon->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
        && !IsValid(GasMoon->PlanetaryEnvironmentGenerator));
    SolidMoon->PlanetType = EPlanetType::GasGiant;
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Solid to gas edit leaves active candidates"), Streaming->GetActiveBody() != SolidMoon);
    TestTrue(TEXT("Solid to gas edit releases old root"), SolidMoon->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded
        && (!IsValid(SolidMoon->PlanetaryEnvironmentGenerator)
            || !IsValid(SolidMoon->PlanetaryEnvironmentGenerator->WorldScapeRootInstance)));

    TArray<AMoon*> Siblings;
    for (int32 Index = 0; Index < 6; ++Index)
    {
        AMoon* Moon = World->SpawnActor<AMoon>();
        if (!TestNotNull(TEXT("Standby moon"), Moon)) return false;
        Configure(Moon, 100, FVector(2e8 + Index * 1e7, 0, 0));
        Moon->ParentPlanet = First;
        Siblings.Add(Moon);
    }
    Place(Pawn, FVector(0, 1.01e8, 0));
    for (int32 Poll = 0; Poll < 6; ++Poll) Streaming->Tick(0.5f);
    int32 PreloadedCount = 0;
    AMoon* PublishedSibling = nullptr;
    for (AMoon* Moon : Siblings)
    {
        if (Moon->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded)
        { ++PreloadedCount; PublishedSibling = Moon; }
    }
    TestTrue(TEXT("Selected body bypasses standby cap"), Streaming->GetActiveBody() == First);
    TestEqual(TEXT("Repeated polls cannot preload entire family"), PreloadedCount, 2);
    if (!TestNotNull(TEXT("Warm sibling available"), PublishedSibling)) return false;

    // Still no WorldScape ticks: test scheduling/ownership, not rendered readiness.
    First->bWorldScapeSurfaceReady = true;
    Streaming->Tick(0.5f);
    AWorldScapeRoot* WarmingRoot = nullptr;
    int32 BuildingSiblings = 0;
    for (AMoon* Moon : Siblings)
    {
        auto* Generator = Moon->PlanetaryEnvironmentGenerator;
        auto* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
        if (Root && Moon->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Preloaded
            && Root->bGenerateWorldScape && !Root->bFreezeGeneration)
        {
            ++BuildingSiblings;
            WarmingRoot = Root;
            TestTrue(TEXT("Speculative generation is hidden and collision-free"),
                Root->IsHidden() && !Root->GetActorEnableCollision() && !Root->bGenerateCollision);
        }
    }
    TestEqual(TEXT("Only one sibling producer warms at once"), BuildingSiblings, 1);
    if (!TestNotNull(TEXT("Real speculative producer exists"), WarmingRoot)) return false;
    const FVector WarmObserver = WarmingRoot->OverridedPlayerPosition;
    Place(Pawn, FVector(0, 1.02e8, 0));
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Initial warmup cannot chase a moving observer forever"),
        WarmingRoot->OverridedPlayerPosition == WarmObserver);

    PublishedSibling->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
    if (!TestTrue(TEXT("Published fixture has root"), IsValid(PublishedSibling->PlanetaryEnvironmentGenerator)
        && IsValid(PublishedSibling->PlanetaryEnvironmentGenerator->WorldScapeRootInstance))) return false;
    PublishedSibling->bWorldScapeSurfaceReady = true; // Synthetic latch, NOT actual geometry.
    PublishedSibling->RefreshWorldScapeSurfaceVisibility();
    PublishedSibling->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
    AWorldScapeRoot* PublishedRoot = PublishedSibling->PlanetaryEnvironmentGenerator->WorldScapeRootInstance;
    Budget.Set(0);
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Reducing standby cap never evicts published sibling"), PublishedSibling->bWorldScapeSurfaceReady
        && PublishedSibling->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::FrozenVisible
        && PublishedSibling->PlanetaryEnvironmentGenerator->WorldScapeRootInstance == PublishedRoot
        && !PublishedRoot->IsHidden());
    for (AMoon* Moon : Siblings)
        if (Moon != PublishedSibling)
            TestTrue(TEXT("Zero budget releases hidden siblings"), Moon->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Unloaded);

    Place(Pawn, FVector(1e13, 0, 0));
    Streaming->Tick(0.5f);
    TestTrue(TEXT("Leaving family still unloads published surface"), !Streaming->GetResidentFamily()
        && !Streaming->GetActiveBody() && !PublishedSibling->bWorldScapeSurfaceReady);
    return true;
}
#endif
