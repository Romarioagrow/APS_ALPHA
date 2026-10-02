#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeResidentResumeTest,
    "APS.Gameplay.World.PlanetSurface.ResidentResume",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeResidentResumeTest::RunTest(const FString& Parameters)
{
    // This fixture covers terrain worker draining, including live seed edits.
    // Published foliage is now enabled in project defaults; its immutable-root
    // contract intentionally rejects those edits and requires root replacement.
    // Do not make a terrain-only sentinel test depend on release CVar defaults.
    IConsoleVariable* FoliageEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.WorldScapeFoliage.Enable"));
    if (!TestNotNull(TEXT("Foliage gate exists for isolated terrain lifecycle"), FoliageEnable)) return false;
    const FString SavedFoliageEnable = FoliageEnable->GetString();
    const auto SavedFoliageFlags = static_cast<EConsoleVariableFlags>(
        (FoliageEnable->GetFlags() & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe);
    ON_SCOPE_EXIT { FoliageEnable->Set(*SavedFoliageEnable, SavedFoliageFlags); };
    FoliageEnable->Set(0, SavedFoliageFlags);

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
        ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Resident resume test world"), World)) return false;

    // State-machine regression, NOT rendered acceptance. No world tick or worker
    // is launched. Inject the published latch and a pending-result sentinel to
    // reproduce planet -> moon -> planet while the returning root catches up.
    for (const EPlanetType Type : {EPlanetType::Rocky, EPlanetType::Frozen,
        EPlanetType::Terrestrial, EPlanetType::Volcanic})
    {
        for (const bool bMoon : {false, true})
        {
            APlanetaryBody* Body = bMoon
                ? static_cast<APlanetaryBody*>(World->SpawnActor<AMoon>())
                : static_cast<APlanetaryBody*>(World->SpawnActor<APlanet>());
            if (!TestNotNull(TEXT("Test body"), Body)) continue;
            const FString Label = FString::Printf(TEXT("%s/%s"), bMoon ? TEXT("moon") : TEXT("planet"),
                *UEnum::GetValueAsString(Type));
            auto Check = [this, &Label](const TCHAR* What, bool Value)
            {
                TestTrue(Label + TEXT(": ") + What, Value);
            };
            Body->PlanetType = Type;
            Body->RadiusKM = 3685.972;
            Body->PlanetRadiusKM = 3686;
            Body->WorldScapeSeed = 824823;
            Body->WorldScapePresentationScale = 1.0;
            auto* Placeholder = NewObject<UStaticMeshComponent>(Body);
            Body->AddInstanceComponent(Placeholder);
            Placeholder->SetupAttachment(Body->GetRootComponent());
            Placeholder->RegisterComponent();

            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
            APlanetarySurfaceGenerator* Generator = Body->PlanetaryEnvironmentGenerator;
            AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
            if (!TestNotNull(Label + TEXT(": preloaded root"), Root)) continue;
            Body->bWorldScapeSurfaceReady = true; // Synthetic completed warmup.
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
            Check(TEXT("Standby publication survives the freeze transition"),
                Body->bWorldScapeSurfaceReady && !Root->IsHidden()
                && !Placeholder->IsVisible() && Root->bFreezeGeneration
                && !Root->IsActorTickEnabled() && !Root->GetActorEnableCollision());
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            Check(TEXT("First activation still waits for real geometry"),
                !Body->bWorldScapeSurfaceReady && Root->IsHidden() && Placeholder->IsVisible());

            UWorldScapeLod* ResidentLod = NewObject<UWorldScapeLod>(Root);
            Root->WorldScapeLod.Add(ResidentLod);
            auto* TerrainMaterial = Root->TerrainMaterial.DefaultMaterial;
            auto* OceanMaterial = Root->OceanMaterial.DefaultMaterial;
            auto* Noise = Root->WorldScapeNoise;
            const uint32 Signature = Generator->AppliedSurfaceProfileSignature;
            Body->bWorldScapeSurfaceReady = true;
            Root->WorldScapeLodInGeneration.Add(nullptr, false);
            Body->RefreshWorldScapeSurfaceVisibility();

            for (int32 Return = 0; Return < 3; ++Return)
            {
                Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
                Check(TEXT("Frozen published sibling remains visible and collision-free"),
                    Body->bWorldScapeSurfaceReady && !Root->IsHidden()
                    && !Placeholder->IsVisible() && !Root->GetActorEnableCollision());
                Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
                Check(TEXT("Return retains visibility through pending LOD generation"),
                    Body->bWorldScapeSurfaceReady && !Root->IsHidden()
                    && !Placeholder->IsVisible() && Placeholder->bHiddenInGame);
                Check(TEXT("Return resumes producer and collision on the same root"),
                    Generator->WorldScapeRootInstance == Root && Root->bGenerateWorldScape
                    && !Root->bFreezeGeneration && Root->IsActorTickEnabled()
                    && Root->GetActorEnableCollision());
                Check(TEXT("Return preserves LOD storage, pending work and profile"),
                    Root->WorldScapeLod.Contains(ResidentLod)
                    && Root->WorldScapeLodInGeneration.Num() == 1
                    && Generator->AppliedSurfaceProfileSignature == Signature
                    && Root->WorldScapeNoise == Noise
                    && Root->TerrainMaterial.DefaultMaterial == TerrainMaterial
                    && Root->OceanMaterial.DefaultMaterial == OceanMaterial);
                Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
                Check(TEXT("Repeated Active request does not restore the globe"),
                    Body->bWorldScapeSurfaceReady && !Root->IsHidden() && !Placeholder->IsVisible());
            }
            Root->WorldScapeLodInGeneration.Empty();
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::FrozenVisible);
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            Check(TEXT("Worker-free return also retains the published latch"),
                Body->bWorldScapeSurfaceReady && !Root->IsHidden() && !Placeholder->IsVisible());

            // A real profile edit is different: it MUST revoke the old latch and
            // hold the old noise/payload until the actual worker drain completes.
            Root->WorldScapeLodInGeneration.Add(nullptr, false);
            ++Body->WorldScapeSeed;
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            Check(TEXT("Changed profile is not mistaken for a resident resume"),
                Generator->IsSurfaceProfileApplyPending() && !Body->bWorldScapeSurfaceReady
                && Root->IsHidden() && Placeholder->IsVisible()
                && Root->WorldScapeNoise == Noise && Root->bFreezeGeneration);
            Root->WorldScapeLodInGeneration.Empty(); // synthetic sentinel, no worker to join
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
            Check(TEXT("Actual unload invalidates readiness and restores fallback"),
                !Body->bWorldScapeSurfaceReady && Placeholder->IsVisible());
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            Check(TEXT("New root must pass first-frame readiness again"),
                IsValid(Generator->WorldScapeRootInstance)
                && Generator->WorldScapeRootInstance != Root && !Body->bWorldScapeSurfaceReady
                && Generator->WorldScapeRootInstance->IsHidden() && Placeholder->IsVisible());
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif
