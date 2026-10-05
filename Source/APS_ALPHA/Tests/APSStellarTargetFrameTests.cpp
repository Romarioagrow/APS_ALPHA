#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Rendering/APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "WorldScapeRoot.h"

namespace APSStellarTargetFrameTests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;
        FTestWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
                .CreateNavigation(false).CreateAISystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                false, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
                GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }
        void Destroy()
        {
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
                World = nullptr;
            }
        }
        ~FTestWorld() { Destroy(); }
        FTestWorld(const FTestWorld&) = delete;
        FTestWorld& operator=(const FTestWorld&) = delete;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStellarTargetFrameTest,
    "APS.Gameplay.World.PlanetSurface.StellarTargetFrame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarTargetFrameTest::RunTest(const FString& Parameters)
{
    using namespace APSStellarTargetFrameTests;
    // SetNewWorldOrigin broadcasts engine-global events. The installed vendor
    // WorldScape listeners take no world argument; never exercise them in a live
    // terrain/editor session. Run this contract in the isolated unattended gate.
    if (!FApp::IsUnattended())
    {
        AddInfo(TEXT("SKIP StellarTargetFrame: actual origin shifts require an unattended isolated process; no invariant tested."));
        return true;
    }
    for (TObjectIterator<AWorldScapeRoot> It; It; ++It)
        if (IsValid(*It) && !It->IsTemplate())
        {
            AddInfo(TEXT("SKIP StellarTargetFrame: a live WorldScape root owns global origin callbacks; no invariant tested."));
            return true;
        }

    FTestWorld Owned, Unrelated;
    UWorld* World = Owned.World;
    if (!TestNotNull(TEXT("Owned transient game world"), World)
        || !TestNotNull(TEXT("Unrelated transient game world"), Unrelated.World)) return false;
    // Register real controller/camera lifecycle, but do not BeginPlay or tick the
    // world. No generator, material bake, terrain workers or gameplay save exists.
    World->InitializeActorsForPlay(FURL());
    auto* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
    TStrongObjectPtr<UAPSStellarVisualSubsystem> Stellar(
        World->GetSubsystem<UAPSStellarVisualSubsystem>());
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Pawn = World->SpawnActor<APawn>();
    const FVector ObserverGeneration(100000000.125, -200000000.25, 300000000.5);
    const FVector StarGeneration(-700000000.25, 500000000.5, -900000000.125);
    auto* Star = World->SpawnActor<AStar>(StarGeneration, FRotator::ZeroRotator);
    if (!TestTrue(TEXT("Real subsystem/controller/pawn/star fixture"),
        Origin && Stellar.IsValid() && Controller && Pawn && Star)) return false;
    auto* PawnRoot = NewObject<USceneComponent>(Pawn, NAME_None, RF_Transient);
    Pawn->AddInstanceComponent(PawnRoot);
    Pawn->SetRootComponent(PawnRoot);
    PawnRoot->SetMobility(EComponentMobility::Movable);
    PawnRoot->RegisterComponent();
    Pawn->SetActorLocation(ObserverGeneration);
    Star->GetRootComponent()->SetMobility(EComponentMobility::Movable);
    Star->Luminosity = 1.0f;
    Star->SurfaceTemperature = 5800;
    Controller->Possess(Pawn);
    if (!TestTrue(TEXT("Actual first controller supplies observer"),
        World->GetFirstPlayerController() == Controller && Controller->GetPawn() == Pawn)) return false;

    // Select through the production path exactly once. No later Tick/resolve can
    // hide a stale cache: every assertion below immediately follows its barrier.
    Stellar->Tick(0.5f);
    FVector InitialTarget;
    FString Identity;
    if (!TestTrue(TEXT("Production Tick selects a stellar target"),
        Stellar->GetActiveStellarTarget(InitialTarget, Identity))) return false;
    TestEqual(TEXT("Selected actual fixture star"), Identity, Star->GetPathName());
    const FVector InitialRay = (ObserverGeneration - StarGeneration).GetSafeNormal();
    const auto CheckFrame = [&](const TCHAR* Phase)
    {
        const FString Label(Phase);
        FVector Target;
        FString CurrentIdentity;
        if (!TestTrue(Label + TEXT(" target available"), Stellar->GetActiveStellarTarget(Target, CurrentIdentity))) return;
        TestEqual(Label + TEXT(" identity unchanged"), CurrentIdentity, Identity);
        TestTrue(Label + TEXT(" cached target equals actual star"), Target.Equals(Star->GetActorLocation(), 1.0e-5));
        TestTrue(Label + TEXT(" canonical star unchanged"), Origin->ToGenerationFrame(Target).Equals(StarGeneration, 1.0e-5));
        TestTrue(Label + TEXT(" canonical observer unchanged"), Origin->ToGenerationFrame(Pawn->GetActorLocation()).Equals(ObserverGeneration, 1.0e-5));
        TestTrue(Label + TEXT(" physical ray unchanged"),
            (Pawn->GetActorLocation() - Target).GetSafeNormal().Equals(InitialRay, 1.0e-10));
    };
    CheckFrame(TEXT("Initial"));
    TestTrue(TEXT("Integer barrier subscribed"), FCoreDelegates::PostWorldOriginOffset.IsBoundToObject(Stellar.Get()));
    TestTrue(TEXT("Double barrier subscribed"), APSWorldShiftEvents::OnPostDoubleShift().IsBoundToObject(Stellar.Get()));

    // Beyond int32, with fractional centimetres; use the actual production shift.
    const FVector Offset(-3500000000.125, 2750000000.5, -1250000000.25);
    if (!TestTrue(TEXT("Actual double shift succeeds"), Origin->ShiftWorldBy(Offset, TEXT("stellar frame contract")))) return false;
    CheckFrame(TEXT("Double shift"));
    APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Unrelated.World, Offset);
    CheckFrame(TEXT("Unrelated world ignored"));
    APSWorldShiftEvents::OnPostDoubleShift().Broadcast(World, Offset);
    CheckFrame(TEXT("Duplicate barrier is idempotent"));

    // Exercise engine rebase while a nonzero double-origin offset is present.
    if (!TestTrue(TEXT("Actual integer rebase succeeds"), World->SetNewWorldOrigin(FIntVector(123456, -234567, 345678)))) return false;
    CheckFrame(TEXT("Combined double and integer origin"));
    if (!TestTrue(TEXT("Inverse integer rebase succeeds"), World->SetNewWorldOrigin(FIntVector::ZeroValue))) return false;
    CheckFrame(TEXT("Integer inverse"));
    if (!TestTrue(TEXT("Inverse double shift succeeds"), Origin->ShiftWorldBy(-Offset, TEXT("stellar frame contract inverse")))) return false;
    CheckFrame(TEXT("Double inverse"));
    TestTrue(TEXT("Original cached position recovered"),
        Origin->FromGenerationFrame(StarGeneration).Equals(InitialTarget, 1.0e-5));

    // Only the normal owning-world lifecycle deinitializes the subsystem. Keep
    // the UObject alive to check that weak callbacks were explicitly removed,
    // rather than merely relying on a subsequent garbage collection.
    Owned.Destroy();
    TestFalse(TEXT("Integer subscription removed at world teardown"), FCoreDelegates::PostWorldOriginOffset.IsBoundToObject(Stellar.Get()));
    TestFalse(TEXT("Double subscription removed at world teardown"), APSWorldShiftEvents::OnPostDoubleShift().IsBoundToObject(Stellar.Get()));
    AddInfo(TEXT("Actual stellar selection and double/integer origin barriers tested without a post-shift resolver Tick. CPU frame contract only; no rendered brightness/interpolation acceptance."));
    return true;
}

#endif
