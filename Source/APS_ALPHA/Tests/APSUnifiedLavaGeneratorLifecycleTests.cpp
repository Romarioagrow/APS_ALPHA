#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "RHIGlobals.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

namespace APSUnifiedLavaGeneratorLifecycleTests
{
    constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
    constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");

    class FLifecycle final : public IAutomationLatentCommand
    {
    public:
        explicit FLifecycle(FAutomationTestBase* InTest) : Test(InTest) {}
        virtual ~FLifecycle() override { Cleanup(); }

        virtual bool Update() override
        {
            if (!bStarted)
            {
                bStarted = true;
                StartSeconds = FPlatformTime::Seconds();
                if (!Start()) return Complete();
                LastTimerFrame = GFrameCounter;
                return false;
            }
            if (FPlatformTime::Seconds() - StartSeconds >= 180.0)
            {
                Test->AddError(TEXT("Unified lava generator lifecycle exceeded 180 seconds; actual timer-driven preparation did not complete."));
                return Complete();
            }
            if (LastTimerFrame == GFrameCounter) return false;
            LastTimerFrame = GFrameCounter;
            if (!Test->TestNotNull(TEXT("active fixture body retained"), Body.Get())
                || !Test->TestNotNull(TEXT("cancelled fixture owner retained"), CancelledGenerator.Get())) return Complete();
            // Only this isolated timer manager advances. Never tick the world,
            // actors, WorldScape producer or geometry/collision workers.
            World->GetTimerManager().Tick(0.11f);
            ++TimerFrames;
            Test->TestNull(TEXT("unloaded owner is not resurrected by a stale timer"), CancelledGenerator->WorldScapeRootInstance);
            Test->TestFalse(TEXT("unloaded owner has no pending request"), CancelledGenerator->IsSurfaceProfileApplyPending());
            if (Test->HasAnyErrors()) return Complete();

            APlanetarySurfaceGenerator* Generator = Body->PlanetaryEnvironmentGenerator;
            if (Generator->IsSurfaceProfileApplyPending())
            {
                CheckInert(Generator);
                return Test->HasAnyErrors() ? Complete() : false;
            }
            // Observe at least two timer frames after unload even with a warm DDC.
            if (TimerFrames < 2) return false;
            CheckReady(Body.Get());
            if (Test->HasAnyErrors()) return Complete();
            for (EPlanetType Type : {EPlanetType::Lava, EPlanetType::Melted, EPlanetType::Volcanic})
            {
                AWorldScapeRoot* PreviousRoot = Generator->WorldScapeRootInstance;
                Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
                Test->TestNull(TEXT("warm unload releases root"), Generator->WorldScapeRootInstance);
                Body->PlanetType = Type;
                ++Body->WorldScapeSeed;
                Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
                Test->TestTrue(TEXT("warm re-entry creates a new root"),
                    IsValid(Generator->WorldScapeRootInstance) && Generator->WorldScapeRootInstance != PreviousRoot);
                CheckReady(Body.Get());
                if (Test->HasAnyErrors()) return Complete();
            }
            Test->AddInfo(FString::Printf(TEXT("UNIFIED_GENERATOR_LIFECYCLE elapsed=%.3f timerFrames=%d coldPending=%d latestModelPending=%d; profile/material/activation flags only, no rendered or worker collision acceptance"),
                FPlatformTime::Seconds() - StartSeconds, TimerFrames, bColdPending ? 1 : 0, bLatestModelPending ? 1 : 0));
            return Complete();
        }

    private:
        struct FOverride
        {
            IConsoleVariable* Variable = nullptr;
            FString Saved;
            EConsoleVariableFlags Flags = ECVF_Default;
        };

        bool Override(const TCHAR* Name, int32 Value)
        {
            IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
            if (!Test->TestNotNull(FString(TEXT("fixture CVar ")) + Name, Variable)) return false;
            FOverride& Saved = Overrides.AddDefaulted_GetRef();
            Saved.Variable = Variable;
            Saved.Saved = Variable->GetString();
            Saved.Flags = static_cast<EConsoleVariableFlags>((Variable->GetFlags() & ECVF_SetByMask) | ECVF_Set_SetOnly_Unsafe);
            Variable->Set(Value, Saved.Flags);
            return true;
        }

        APlanet* NewBody(int32 Seed)
        {
            APlanet* Result = World->SpawnActor<APlanet>();
            if (!Test->TestNotNull(TEXT("isolated generated lava body"), Result)) return nullptr;
            Result->IsManual = false;
            Result->PlanetType = EPlanetType::Lava;
            Result->RadiusKM = 3685.972;
            Result->PlanetRadiusKM = 3686;
            Result->WorldScapePresentationScale = 1.0;
            Result->WorldScapeSeed = Seed;
            return Result;
        }

        bool Start()
        {
            if (FindObject<UMaterialInstance>(nullptr, TemplatePath) || FindObject<UMaterial>(nullptr, MasterPath))
            {
                Test->AddError(TEXT("FreshLifecycleRHI requires an unloaded unified MIC/master; run this test alone in a fresh rendering process."));
                return false;
            }
            if (!Override(TEXT("aps.WorldScapeFoliage.Enable"), 0)
                || !Override(TEXT("aps.Surface.UnifiedLavaSurface"), 1)) return false;
            const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
                .RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World.Reset(UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, GMaxRHIFeatureLevel, &Values));
            if (!Test->TestNotNull(TEXT("isolated lifecycle world"), World.Get())) return false;
            CancelledBody = NewBody(824823);
            if (!CancelledBody.IsValid()) return false;
            CancelledBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            CancelledGenerator = CancelledBody->PlanetaryEnvironmentGenerator;
            if (!Test->TestNotNull(TEXT("cold generator"), CancelledGenerator.Get())) return false;
            RetainedMIC.Reset(FindObject<UMaterialInstance>(nullptr, TemplatePath));
            if (!Test->TestNotNull(TEXT("real cold request loaded saved MIC"), RetainedMIC.Get())) return false;
            bColdPending = CancelledGenerator->IsSurfaceProfileApplyPending();
            if (bColdPending) CheckInert(CancelledGenerator.Get());
            else
            {
                CheckReady(CancelledBody.Get());
                Test->AddInfo(TEXT("Cold owner had no pending request: pending unload branch was not observed; ready profile checks remain required."));
            }
            if (Test->HasAnyErrors()) return false;
            CancelledBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
            Test->TestNull(TEXT("cold owner unload releases root"), CancelledGenerator->WorldScapeRootInstance);
            Test->TestFalse(TEXT("cold owner unload cancels preparation"), CancelledGenerator->IsSurfaceProfileApplyPending());
            Body = NewBody(824824);
            if (!Body.IsValid()) return false;
            Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Active);
            APlanetarySurfaceGenerator* Generator = Body->PlanetaryEnvironmentGenerator;
            if (!Test->TestNotNull(TEXT("replacement owner"), Generator)) return false;
            bLatestModelPending = Generator->IsSurfaceProfileApplyPending();
            if (bLatestModelPending)
            {
                CheckInert(Generator);
                Body->PlanetType = EPlanetType::Volcanic;
                Body->WorldScapeSeed = 824999;
                // No new Apply/streaming call: the actual timer must re-resolve
                // these latest model fields when the material becomes ready.
            }
            else
            {
                CheckReady(Body.Get());
                Test->AddInfo(TEXT("Replacement owner had no pending request: pending model-change branch was not observed; warm re-entry covers all three types."));
            }
            return !Test->HasAnyErrors();
        }

        void CheckInert(APlanetarySurfaceGenerator* Generator)
        {
            AWorldScapeRoot* Root = Generator->WorldScapeRootInstance;
            if (!Test->TestNotNull(TEXT("pending request retains inert root"), Root)) return;
            Test->TestTrue(TEXT("fresh pending root stays hidden and stopped"), Root->IsHidden()
                && !Root->IsActorTickEnabled() && !Root->GetActorEnableCollision() && !Root->bGenerateWorldScape);
            Test->TestTrue(TEXT("fresh pending request has not published a profile"),
                !Generator->bSurfaceProfileApplied && Generator->AppliedSurfaceProfileSignature == 0);
            CheckNoWorkers(Root);
        }

        void CheckNoWorkers(AWorldScapeRoot* Root)
        {
            Test->TestTrue(TEXT("fixture never generates meshes or workers"), Root->WorldScapeLodInGeneration.IsEmpty()
                && Root->WorldScapeLod.IsEmpty() && Root->WorldScapeLodOcean.IsEmpty() && Root->CollisionLods.IsEmpty()
                && !Root->bGenerateFoliages);
        }

        void CheckReady(APlanet* CurrentBody)
        {
            APlanetarySurfaceGenerator* Generator = CurrentBody->PlanetaryEnvironmentGenerator;
            AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
            if (!Test->TestNotNull(TEXT("prepared root"), Root)) return;
            const FString Label = UEnum::GetValueAsString(CurrentBody->PlanetType);
            Test->TestTrue(Label + TEXT(" latest profile is current"), Generator->IsSurfaceProfileCurrent(CurrentBody)
                && Generator->bSurfaceProfileApplied && !Generator->IsSurfaceProfileApplyPending());
            Test->TestTrue(Label + TEXT(" latest seed reaches root"), Root->Seed == CurrentBody->WorldScapeSeed);
            Test->TestEqual(Label + TEXT(" latest type reaches resolved profile"), Generator->ResolvedSurfaceProfile.PlanetType, CurrentBody->PlanetType);
            UMaterialInstanceDynamic* Material = Generator->ResolvedTerrainMaterialInstance;
            if (Test->TestNotNull(Label + TEXT(" prepared terrain MID"), Material))
            {
                Test->TestTrue(Label + TEXT(" exact unified saved MIC parent"), Material->Parent.Get() == RetainedMIC.Get());
                if (Test->TestNotNull(Label + TEXT(" unified master exists"), Material->GetMaterial()))
                    Test->TestEqual(Label + TEXT(" exact unified master"), Material->GetMaterial()->GetPathName(), FString(MasterPath));
                Test->TestEqual(Label + TEXT(" opaque surface"), Material->GetBlendMode(), BLEND_Opaque);
            }
            Test->TestTrue(Label + TEXT(" unified publication removes separate ocean"), !Root->bOcean
                && !Root->OceanMaterial.DefaultMaterial && !Generator->ResolvedOceanMaterialInstance);
            Test->TestTrue(Label + TEXT(" Active producer/collision flags restored"), Root->bGenerateWorldScape
                && !Root->bFreezeGeneration && Root->IsActorTickEnabled() && Root->GetActorEnableCollision()
                && CurrentBody->GetWorldScapeStreamingState() == EWorldScapeSurfaceState::Active);
            Test->TestTrue(Label + TEXT(" visibility still awaits real geometry"), Root->IsHidden() && !CurrentBody->bWorldScapeSurfaceReady);
            CheckNoWorkers(Root);
        }

        void Cleanup()
        {
            if (Body.IsValid()) Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
            if (CancelledBody.IsValid()) CancelledBody->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
            Body.Reset(); CancelledBody.Reset(); CancelledGenerator.Reset(); RetainedMIC.Reset();
            if (World.IsValid()) { World->DestroyWorld(false); World.Reset(); }
            for (const FOverride& Saved : Overrides) Saved.Variable->Set(*Saved.Saved, Saved.Flags);
            Overrides.Reset();
        }

        bool Complete() { Cleanup(); return true; }
        FAutomationTestBase* Test;
        TStrongObjectPtr<UWorld> World;
        TStrongObjectPtr<UMaterialInstance> RetainedMIC;
        TWeakObjectPtr<APlanet> Body, CancelledBody;
        TWeakObjectPtr<APlanetarySurfaceGenerator> CancelledGenerator;
        TArray<FOverride> Overrides;
        double StartSeconds = 0.0;
        uint64 LastTimerFrame = MAX_uint64;
        int32 TimerFrames = 0;
        bool bStarted = false, bColdPending = false, bLatestModelPending = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSUnifiedLavaGeneratorFreshLifecycleRHITest,
    "APS.Gameplay.World.PlanetSurface.UnifiedLava.Generator.FreshLifecycleRHI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUnifiedLavaGeneratorFreshLifecycleRHITest::RunTest(const FString&)
{
    if (GUsingNullRHI || !FApp::CanEverRender())
    {
        AddError(TEXT("Unified lava generator lifecycle requires a fresh rendering process."));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(APSUnifiedLavaGeneratorLifecycleTests::FLifecycle(this));
    return true;
}

#endif
