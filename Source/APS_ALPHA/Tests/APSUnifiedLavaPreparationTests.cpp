#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaMaterialPreparation.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaSurface.h"
#include "Components/SceneComponent.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSUnifiedLavaPreparationTests
{
    using namespace APSUnifiedLavaSurface;

    constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
    constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");
    constexpr double RadiusCm = 675000000.0;
    constexpr double TimeoutSeconds = 180.0;

    FAPSResolvedPlanetSurfaceProfile LavaProfile(EPlanetType Type)
    {
        // Same factory fixture as Factory.FreshLoadRHI; no surface/render claim.
        FAPSResolvedPlanetSurfaceProfile Profile;
        Profile.PlanetType = Type;
        Profile.Archetype = EAPSPlanetSurfaceArchetype::Magmatic;
        Profile.LiquidType = EAPSPlanetLiquidType::Lava;
        Profile.OceanLevel = 0.125f;
        Profile.NoiseIntensity = 800000.0f;
        return Profile;
    }

    const TCHAR* StateName(EPreparationState State)
    {
        switch (State)
        {
        case EPreparationState::Unrequested: return TEXT("Unrequested");
        case EPreparationState::Pending: return TEXT("Pending");
        case EPreparationState::Ready: return TEXT("Ready");
        case EPreparationState::Failed: return TEXT("Failed");
        }
        return TEXT("Unknown");
    }

    class FPrepareAndCreate final : public IAutomationLatentCommand
    {
    public:
        explicit FPrepareAndCreate(FAutomationTestBase* InTest)
            : Test(InTest), FeatureLevel(GMaxRHIFeatureLevel) {}

        virtual bool Update() override
        {
            if (!bStarted)
            {
                bStarted = true;
                StartSeconds = FPlatformTime::Seconds();
                // Check again when the latent command actually starts. Never
                // unload assets or manufacture a cold cache in another session.
                if (FindObject<UMaterialInstance>(nullptr, TemplatePath)
                    || FindObject<UMaterial>(nullptr, MasterPath))
                {
                    Test->AddError(TEXT("Preparation fresh-load precondition failed: unified MIC/master already loaded; run this test alone in a fresh RHI process."));
                    return true;
                }
                const EPreparationState InitialState = Poll();
                Report(TEXT("FirstPoll"), InitialState);
                if (InitialState == EPreparationState::Failed) return ReportFailure();
                if (!Test->TestNotNull(TEXT("production Poll retains the saved MIC"), Preparation.GetMaterial()))
                    return Complete();
                FirstMaterial = Preparation.GetMaterial();
                Test->TestEqual(TEXT("retained MIC is exact saved asset"), FirstMaterial->GetPathName(), FString(TemplatePath));
                CheckRepeatedPoll();
                if (Test->HasAnyErrors()) return Complete();

                // A second owner can retire while the first still needs these
                // shared jobs. Reset must not cancel the engine's compilation.
                const EPreparationState CancelState = CancelledOwner.Poll(FeatureLevel);
                Test->TestTrue(TEXT("second owner observes a valid preparation"),
                    CancelState == EPreparationState::Pending || CancelState == EPreparationState::Ready);
                Test->TestTrue(TEXT("both owners retain the same saved MIC"),
                    CancelledOwner.GetMaterial() == FirstMaterial.Get());
                FMaterialResource* Resource = FirstMaterial->GetMaterialResource(FeatureLevel);
                const bool bCompilationFinished = !Resource || Resource->IsCompilationFinished();
                FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
                bCancelledWhilePending = CancelState == EPreparationState::Pending;
                CancelledOwner.Reset();
                Test->TestNull(TEXT("cancelled owner releases its material"), CancelledOwner.GetMaterial());
                Test->TestEqual(TEXT("cancelled owner clears failure"), FString(CancelledOwner.GetFailureReason()), FString(TEXT("None")));
                FMaterialResource* AfterReset = FirstMaterial->GetMaterialResource(FeatureLevel);
                Test->TestTrue(TEXT("cancellation preserves the other owner's resource"), AfterReset == Resource);
                if (AfterReset)
                {
                    Test->TestEqual(TEXT("cancellation preserves shared compilation state"),
                        AfterReset->IsCompilationFinished(), bCompilationFinished);
                    Test->TestTrue(TEXT("cancellation preserves the shared shader map"), AfterReset->GetGameThreadShaderMap() == Map);
                }
                if (Test->HasAnyErrors()) return Complete();
                // Yield even on a warm-cache Ready result to verify ownership
                // across latent updates, without an artificial blocking delay.
                return false;
            }

            if (FPlatformTime::Seconds() - StartSeconds >= TimeoutSeconds)
            {
                Report(TEXT("Timeout"), EPreparationState::Pending);
                Test->AddError(FString::Printf(TEXT("Unified lava preparation did not become Ready within %.0f seconds; no synchronous shader completion was requested."), TimeoutSeconds));
                return Complete();
            }
            Test->TestTrue(TEXT("same MIC survives across latent updates"),
                FirstMaterial.IsValid() && Preparation.GetMaterial() == FirstMaterial.Get());
            Test->TestNull(TEXT("cancelled owner is not resurrected on later frames"), CancelledOwner.GetMaterial());
            if (Test->HasAnyErrors()) return Complete();

            const EPreparationState State = Poll();
            if (State == EPreparationState::Failed) return ReportFailure();
            if (FPlatformTime::Seconds() - StartSeconds >= TimeoutSeconds)
            {
                Test->AddError(TEXT("Unified lava preparation exceeded its wall-clock deadline during Poll."));
                return Complete();
            }
            if (State == EPreparationState::Pending) return false;
            if (!Test->TestEqual(TEXT("production preparation reaches Ready"), State, EPreparationState::Ready))
                return Complete();

            Report(TEXT("Ready"), State);
            CheckRepeatedPoll();
            CheckFactory();
            if (!bSawPending)
                Test->AddInfo(TEXT("Preparation was already Ready on first Poll: warm-cache fast path verified; asynchronous Pending branch was not observed."));
            if (!bCancelledWhilePending)
                Test->AddInfo(TEXT("Second owner was already Ready at Reset: cancellation while Pending was not observed."));

            // A different feature must require an explicit lifecycle reset,
            // without silently preparing another permutation for this owner.
            const ERHIFeatureLevel::Type OtherFeature = FeatureLevel == ERHIFeatureLevel::SM5
                ? ERHIFeatureLevel::SM6 : ERHIFeatureLevel::SM5;
            Test->TestEqual(TEXT("feature change fails explicitly"), Preparation.Poll(OtherFeature), EPreparationState::Failed);
            Test->TestEqual(TEXT("feature change reports reset requirement"),
                FString(Preparation.GetFailureReason()), FString(TEXT("FeatureLevelChangedRequiresReset")));
            Test->TestTrue(TEXT("failed owner retains MIC until Reset"), Preparation.GetMaterial() == FirstMaterial.Get());
            Preparation.Reset();
            Test->TestNull(TEXT("Reset clears retained MIC"), Preparation.GetMaterial());
            Test->TestEqual(TEXT("Reset clears failure reason"), FString(Preparation.GetFailureReason()), FString(TEXT("None")));
            Test->TestEqual(TEXT("reset owner can reuse completed preparation"), Preparation.Poll(FeatureLevel), EPreparationState::Ready);
            Test->TestTrue(TEXT("reset reuses exact same loaded MIC"), Preparation.GetMaterial() == FirstMaterial.Get());
            Test->AddInfo(TEXT("Preparation/factory contract only; no rendered appearance, root swap, terrain or collision acceptance."));
            return Complete();
        }

    private:
        EPreparationState Poll()
        {
            const double Before = FPlatformTime::Seconds();
            const EPreparationState Result = Preparation.Poll(FeatureLevel);
            MaxPollMilliseconds = FMath::Max(MaxPollMilliseconds, (FPlatformTime::Seconds() - Before) * 1000.0);
            ++PollCount;
            bSawPending |= Result == EPreparationState::Pending;
            return Result;
        }

        void CheckRepeatedPoll()
        {
            UMaterialInstance* Material = Preparation.GetMaterial();
            FMaterialResource* Resource = Material ? Material->GetMaterialResource(FeatureLevel) : nullptr;
            if (!Test->TestNotNull(TEXT("retained MIC has actual resource"), Resource)) return;
            const bool bCompilationFinished = Resource->IsCompilationFinished();
            FMaterialShaderMap* Map = Resource->GetGameThreadShaderMap();
            for (int32 I = 0; I < 3; ++I)
            {
                const EPreparationState State = Poll();
                Test->TestTrue(TEXT("same-feature repeated Poll remains valid"),
                    State == EPreparationState::Pending || State == EPreparationState::Ready);
                Test->TestTrue(TEXT("same-feature Poll retains MIC identity"), Preparation.GetMaterial() == Material);
                FMaterialResource* After = Material->GetMaterialResource(FeatureLevel);
                if (!Test->TestTrue(TEXT("same-feature Poll retains resource identity"), After == Resource)) return;
                Test->TestTrue(TEXT("same-frame Poll does not replace shader map"), After->GetGameThreadShaderMap() == Map);
                Test->TestEqual(TEXT("same-frame Poll does not restart completed compilation"),
                    After->IsCompilationFinished(), bCompilationFinished);
            }
        }

        void CheckFactory()
        {
            UMaterialInstance* Material = Preparation.GetMaterial();
            FMaterialResource* Resource = Material ? Material->GetMaterialResource(FeatureLevel) : nullptr;
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Test->TestTrue(TEXT("Ready is the MIC's complete LocalVF permutation"),
                Resource && Resource->IsGameThreadShaderMapComplete() && Map
                && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType)
                && Resource->GetCompileErrors().IsEmpty())) return;

            TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
            if (!Test->TestNotNull(TEXT("transient factory frame"), Frame.Get())) return;
            Frame->SetWorldTransform(FTransform(FRotator(17.0, 31.0, -9.0),
                FVector(100000000.125, -200000000.25, 300000000.5), FVector(2.0)));
            for (EPlanetType Type : {EPlanetType::Lava, EPlanetType::Melted, EPlanetType::Volcanic})
            {
                const FString Label = StaticEnum<EPlanetType>()->GetNameStringByValue(static_cast<int64>(Type));
                ECreateFailure Failure = ECreateFailure::MissingResource;
                TStrongObjectPtr<UMaterialInstanceDynamic> Instance(Create(Frame.Get(), Frame.Get(),
                    LavaProfile(Type), RadiusCm, FeatureLevel, &Failure));
                Test->TestEqual(Label + TEXT(" strict factory reports success"), Failure, ECreateFailure::None);
                if (!Test->TestNotNull(Label + TEXT(" strict factory creates MID after production readiness"), Instance.Get())) continue;
                Test->TestTrue(Label + TEXT(" factory uses SAME prepared saved MIC"), Instance->Parent.Get() == Material);
                Test->TestEqual(Label + TEXT(" remains opaque"), Instance->GetBlendMode(), BLEND_Opaque);
                Test->TestTrue(Label + TEXT(" frame binding installed"), Frame->TransformUpdated.IsBoundToObject(Instance.Get()));
            }
        }

        void Report(const TCHAR* Phase, EPreparationState State)
        {
            UMaterialInstance* Material = Preparation.GetMaterial();
            FMaterialResource* Resource = Material ? Material->GetMaterialResource(FeatureLevel) : nullptr;
            const bool bCompilationFinished = !Resource || Resource->IsCompilationFinished();
            FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            int32 Shaders = 0, Pipelines = 0;
            if (Map) Map->CountNumShaders(Shaders, Pipelines);
            // The game-thread map can be a finalized clone whose public ID is
            // zero while jobs exist. It is diagnostic data, not a readiness gate.
            Test->AddInfo(FString::Printf(TEXT("UNIFIED_PREPARATION phase=%s state=%s elapsed=%.3f polls=%d maxPollMs=%.3f pendingObserved=%d cancelPending=%d feature=%d shaders=%d complete=%d localVF=%d compilationFinished=%d mapCompilingId=%u reason=%s"),
                Phase, StateName(State), FPlatformTime::Seconds() - StartSeconds, PollCount, MaxPollMilliseconds,
                bSawPending ? 1 : 0, bCancelledWhilePending ? 1 : 0, int32(FeatureLevel), Shaders,
                Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
                Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0,
                bCompilationFinished ? 1 : 0, Map ? Map->GetCompilingId() : 0, Preparation.GetFailureReason()));
        }

        bool ReportFailure()
        {
            Report(TEXT("Failed"), EPreparationState::Failed);
            Test->AddError(FString::Printf(TEXT("Production unified lava preparation failed: %s; compilation availability and asset errors are not skipped."), Preparation.GetFailureReason()));
            return Complete();
        }

        bool Complete()
        {
            Preparation.Reset();
            CancelledOwner.Reset();
            return true;
        }

        FAutomationTestBase* Test;
        const ERHIFeatureLevel::Type FeatureLevel;
        FMaterialPreparation Preparation;
        FMaterialPreparation CancelledOwner;
        TWeakObjectPtr<UMaterialInstance> FirstMaterial;
        double StartSeconds = 0.0;
        double MaxPollMilliseconds = 0.0;
        int32 PollCount = 0;
        bool bStarted = false;
        bool bSawPending = false;
        bool bCancelledWhilePending = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSUnifiedLavaPreparationFreshRHITest,
    "APS.Gameplay.World.PlanetSurface.UnifiedLava.Preparation.FreshLoadRHI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUnifiedLavaPreparationFreshRHITest::RunTest(const FString&)
{
    if (GUsingNullRHI || !FApp::CanEverRender())
    {
        AddError(TEXT("Preparation.FreshLoadRHI requires a rendering process; use Preparation.InvalidInputs for CPU/NullRHI validation."));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(APSUnifiedLavaPreparationTests::FPrepareAndCreate(this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSUnifiedLavaPreparationInvalidInputsTest,
    "APS.Gameplay.World.PlanetSurface.UnifiedLava.Preparation.InvalidInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUnifiedLavaPreparationInvalidInputsTest::RunTest(const FString&)
{
    using namespace APSUnifiedLavaSurface;
    using namespace APSUnifiedLavaPreparationTests;
    UMaterialInstance* MaterialBefore = FindObject<UMaterialInstance>(nullptr, TemplatePath);
    UMaterial* MasterBefore = FindObject<UMaterial>(nullptr, MasterPath);
    FMaterialPreparation Preparation;
    TestNull(TEXT("new helper owns no material"), Preparation.GetMaterial());
    TestEqual(TEXT("new helper has no failure"), FString(Preparation.GetFailureReason()), FString(TEXT("None")));
    TestEqual(TEXT("invalid feature fails immediately"), Preparation.Poll(ERHIFeatureLevel::Num), EPreparationState::Failed);
    TestEqual(TEXT("invalid feature reason"), FString(Preparation.GetFailureReason()), FString(TEXT("InvalidFeatureLevel")));
    TestEqual(TEXT("failure is terminal without Reset"), Preparation.Poll(ERHIFeatureLevel::SM5), EPreparationState::Failed);
    TestNull(TEXT("invalid request never retains a material"), Preparation.GetMaterial());
    Preparation.Reset();
    TestNull(TEXT("Reset of failed helper owns no material"), Preparation.GetMaterial());
    TestEqual(TEXT("Reset clears failure"), FString(Preparation.GetFailureReason()), FString(TEXT("None")));
    if (GUsingNullRHI)
    {
        TestEqual(TEXT("NullRHI fails without polling forever"), Preparation.Poll(ERHIFeatureLevel::SM5), EPreparationState::Failed);
        TestEqual(TEXT("NullRHI reason is explicit"), FString(Preparation.GetFailureReason()), FString(TEXT("NullRHI")));
        TestNull(TEXT("NullRHI loads no template"), Preparation.GetMaterial());
        Preparation.Reset();
    }
    else
    {
        AddInfo(TEXT("NullRHI branch not exercised in this RHI process; run InvalidInputs with -NullRHI to cover it."));
    }
    TestTrue(TEXT("rejected CPU requests preserve template load state"), FindObject<UMaterialInstance>(nullptr, TemplatePath) == MaterialBefore);
    TestTrue(TEXT("rejected CPU requests preserve master load state"), FindObject<UMaterial>(nullptr, MasterPath) == MasterBefore);
    return true;
}

#endif
