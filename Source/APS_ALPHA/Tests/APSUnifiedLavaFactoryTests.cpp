#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSUnifiedLavaSurface.h"
#include "AssetCompilingManager.h"
#include "Components/SceneComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RHIGlobals.h"
#include "ShaderCompiler.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include <limits>

namespace APSUnifiedLavaFactoryTests
{
    constexpr const TCHAR* TemplatePath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface");
    constexpr const TCHAR* MasterPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/M_APS_UnifiedLavaSurface.M_APS_UnifiedLavaSurface");
    constexpr double RadiusCm = 675000000.0;

    FAPSResolvedPlanetSurfaceProfile LavaProfile(EPlanetType Type)
    {
        // A factory fixture, not a claim about generated climate or rendered terrain.
        FAPSResolvedPlanetSurfaceProfile P;
        P.PlanetType = Type;
        P.Archetype = EAPSPlanetSurfaceArchetype::Magmatic;
        P.LiquidType = EAPSPlanetLiquidType::Lava;
        P.OceanLevel = 0.125f;
        P.NoiseIntensity = 800000.0f;
        return P;
    }

    TSet<UMaterialInterface*> LoadedMaterials()
    {
        TSet<UMaterialInterface*> Result;
        for (TObjectIterator<UMaterialInterface> It; It; ++It) Result.Add(*It);
        return Result;
    }

    bool SameMaterials(const TSet<UMaterialInterface*>& Before, const TSet<UMaterialInterface*>& After)
    {
        if (Before.Num() != After.Num()) return false;
        for (UMaterialInterface* M : Before) if (!After.Contains(M)) return false;
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSUnifiedLavaFactoryInvalidInputsTest,
    "APS.Gameplay.World.PlanetSurface.UnifiedLava.Factory.InvalidInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUnifiedLavaFactoryInvalidInputsTest::RunTest(const FString&)
{
    using namespace APSUnifiedLavaSurface;
    using namespace APSUnifiedLavaFactoryTests;
    TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
    if (!TestNotNull(TEXT("transient CPU frame"), Frame.Get())) return false;
    const auto MaterialsBefore = LoadedMaterials();
    UMaterialInstance* TemplateBefore = FindObject<UMaterialInstance>(nullptr, TemplatePath);
    UMaterial* MasterBefore = FindObject<UMaterial>(nullptr, MasterPath);
    const FTransform FrameBefore = Frame->GetComponentTransform();

    const auto CheckRejected = [&](const TCHAR* Label, USceneComponent* InputFrame,
        const FAPSResolvedPlanetSurfaceProfile& Profile, double Radius, ECreateFailure Expected)
    {
        AddExpectedMessagePlain(FString(TEXT("[APS.UnifiedLava] rejected=")) + FailureName(Expected),
            ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
        ECreateFailure Failure = ECreateFailure::None;
        UMaterialInstanceDynamic* Result = Create(Frame.Get(), InputFrame, Profile, Radius,
            ERHIFeatureLevel::SM5, &Failure);
        TestNull(FString(Label) + TEXT(" has no instance"), Result);
        TestEqual(FString(Label) + TEXT(" exact failure"), Failure, Expected);
        TestTrue(FString(Label) + TEXT(" loads/creates no material"), SameMaterials(MaterialsBefore, LoadedMaterials()));
        TestTrue(FString(Label) + TEXT(" leaves template load state unchanged"),
            FindObject<UMaterialInstance>(nullptr, TemplatePath) == TemplateBefore);
        TestTrue(FString(Label) + TEXT(" leaves master load state unchanged"),
            FindObject<UMaterial>(nullptr, MasterPath) == MasterBefore);
        TestTrue(FString(Label) + TEXT(" leaves frame unchanged"), Frame->GetComponentTransform().Equals(FrameBefore));
        TestFalse(FString(Label) + TEXT(" adds no frame binding"), Frame->TransformUpdated.IsBound());
    };

    const auto Lava = LavaProfile(EPlanetType::Lava);
    CheckRejected(TEXT("null frame"), nullptr, Lava, RadiusCm, ECreateFailure::InvalidFrame);
    auto Water = Lava;
    Water.LiquidType = EAPSPlanetLiquidType::Water;
    CheckRejected(TEXT("non-lava"), Frame.Get(), Water, RadiusCm, ECreateFailure::InvalidLiquid);
    CheckRejected(TEXT("zero radius"), Frame.Get(), Lava, 0.0, ECreateFailure::InvalidRadius);
    CheckRejected(TEXT("negative radius"), Frame.Get(), Lava, -1.0, ECreateFailure::InvalidRadius);
    CheckRejected(TEXT("NaN radius"), Frame.Get(), Lava, std::numeric_limits<double>::quiet_NaN(), ECreateFailure::InvalidRadius);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSUnifiedLavaFactoryFreshLoadRHITest,
    "APS.Gameplay.World.PlanetSurface.UnifiedLava.Factory.FreshLoadRHI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUnifiedLavaFactoryFreshLoadRHITest::RunTest(const FString&)
{
    using namespace APSUnifiedLavaSurface;
    using namespace APSUnifiedLavaFactoryTests;
    if (GUsingNullRHI)
    {
        AddWarning(TEXT("SKIP UnifiedLava.Factory.FreshLoadRHI: NullRHI cannot validate the saved candidate shader map or LocalVF."));
        return true;
    }
    // Never unload somebody else's material or flush GC to manufacture a cold load.
    if (FindObject<UMaterialInstance>(nullptr, TemplatePath) || FindObject<UMaterial>(nullptr, MasterPath))
    {
        AddError(TEXT("Fresh-load precondition failed: UnifiedLava master/MIC already loaded. Run this test in a fresh RHI process; no assets were unloaded."));
        return false;
    }
    TStrongObjectPtr<USceneComponent> Frame(NewObject<USceneComponent>(GetTransientPackage()));
    if (!TestNotNull(TEXT("transient RHI frame"), Frame.Get())) return false;
    const FVector Center(100000000.125, -200000000.25, 300000000.5);
    Frame->SetWorldTransform(FTransform(FRotator(17.0, 31.0, -9.0), Center, FVector(2.0)));
    ECreateFailure InitialFailure = ECreateFailure::InvalidFrame;
    TStrongObjectPtr<UMaterialInstanceDynamic> Initial(Create(Frame.Get(), Frame.Get(),
        LavaProfile(EPlanetType::Lava), RadiusCm, GMaxRHIFeatureLevel, &InitialFailure));
    AddInfo(FString::Printf(TEXT("UNIFIED_FACTORY_FIRST freshLoad=1 installed=%d failure=%s feature=%d; no shader wait before this result"),
        Initial.IsValid() ? 1 : 0, FailureName(InitialFailure), int32(GMaxRHIFeatureLevel)));
    // A later successful retry must not turn the production one-shot failure green.
    TestNotNull(TEXT("first factory call succeeds without test shader waits"), Initial.Get());
    TestEqual(TEXT("first factory call reports no failure"), InitialFailure, ECreateFailure::None);

    TStrongObjectPtr<UMaterialInstance> Template(FindObject<UMaterialInstance>(nullptr, TemplatePath));
    if (!TestNotNull(TEXT("factory loaded actual saved template"), Template.Get())) return false;
    const auto ReportShaderState = [&](const TCHAR* Phase)
    {
        FMaterialResource* Resource = Template->GetMaterialResource(GMaxRHIFeatureLevel);
        FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
        int32 Shaders = 0, Pipelines = 0;
        if (Map) Map->CountNumShaders(Shaders, Pipelines);
        AddInfo(FString::Printf(TEXT("UNIFIED_SHADER_STATE phase=%s jobCacheDDC=%d materialMapDDC=%d compiling=%d resource=%d map=%d shaders=%d pipelines=%d complete=%d localVF=%d errors=%d"),
            Phase, IsShaderJobCacheDDCEnabled() ? 1 : 0, IsMaterialMapDDCEnabled() ? 1 : 0,
            GShaderCompilingManager && GShaderCompilingManager->IsCompiling() ? 1 : 0,
            Resource ? 1 : 0, Map ? 1 : 0, Shaders, Pipelines,
            Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
            Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0,
            Resource ? Resource->GetCompileErrors().Num() : 0));
    };
    ReportShaderState(TEXT("Loaded"));
    // Diagnostic-only completion: distinguish load-time rejection from a broken
    // saved permutation. No recompile, asset write, production wait or CVar change.
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    ReportShaderState(TEXT("AfterWaitOnly"));
    if (!TestNotNull(TEXT("saved template master"), Template->GetMaterial())) return false;
    TestEqual(TEXT("saved candidate master identity"), Template->GetMaterial()->GetPathName(), FString(MasterPath));

    for (EPlanetType Type : {EPlanetType::Lava, EPlanetType::Melted, EPlanetType::Volcanic})
    {
        const FString Label = StaticEnum<EPlanetType>()->GetNameStringByValue(static_cast<int64>(Type));
        const auto Profile = LavaProfile(Type);
        ECreateFailure Failure = ECreateFailure::InvalidRadius;
        TStrongObjectPtr<UMaterialInstanceDynamic> Instance(Create(Frame.Get(), Frame.Get(), Profile,
            RadiusCm, GMaxRHIFeatureLevel, &Failure));
        AddInfo(FString::Printf(TEXT("UNIFIED_FACTORY_READY type=%s installed=%d failure=%s"),
            *Label, Instance.IsValid() ? 1 : 0, FailureName(Failure)));
        TestEqual(Label + TEXT(" ready factory clears OutFailure"), Failure, ECreateFailure::None);
        if (!TestNotNull(Label + TEXT(" ready saved candidate"), Instance.Get())) continue;
        TestTrue(Label + TEXT(" exact saved MIC parent"), Instance->Parent.Get() == Template.Get());
        TestEqual(Label + TEXT(" opaque candidate"), Instance->GetBlendMode(), BLEND_Opaque);
        TestTrue(Label + TEXT(" frame binding installed"), Frame->TransformUpdated.IsBoundToObject(Instance.Get()));

        const auto CheckVector = [&](const TCHAR* Name, const FVector4& Expected)
        {
            FVector4 Value(0, 0, 0, 0);
            if (TestTrue(Label + TEXT(" has ") + Name,
                Instance->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), Value)))
                TestTrue(Label + TEXT(" correct ") + Name, Value.Equals(Expected, 1.e-9));
        };
        CheckVector(TEXT("APS_SharedPlanetCenter"), FVector4(Center.X, Center.Y, Center.Z, 0.0));
        CheckVector(TEXT("APS_SharedInverseScale"), FVector4(0.5, 0.0, 0.0, 0.0));
        const FQuat Rotation = Frame->GetComponentQuat().GetNormalized();
        const FVector Axes[] = {Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ()};
        const TCHAR* Names[] = {TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ")};
        for (int32 I = 0; I < UE_ARRAY_COUNT(Axes); ++I)
            CheckVector(Names[I], FVector4(Axes[I].X, Axes[I].Y, Axes[I].Z, 0.0));

        const auto CheckScalar = [&](const TCHAR* Name, float Expected)
        {
            float Value = -1.0f;
            if (TestTrue(Label + TEXT(" has ") + Name,
                Instance->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), Value)))
                TestEqual(Label + TEXT(" correct ") + Name, Value, Expected);
        };
        CheckScalar(TEXT("APS_UnifiedSeaRadiusCm"), static_cast<float>(RadiusCm + double(Profile.OceanLevel) * Profile.NoiseIntensity));
        CheckScalar(TEXT("APS_UnifiedRadiusToleranceCm"), static_cast<float>(FMath::Max(200.0, RadiusCm * 1.e-6)));
        CheckScalar(TEXT("APS_FarNormalStartCm"), 200000.0f);
        CheckScalar(TEXT("APS_FarNormalEndCm"), 2000000.0f);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("APSUnifiedLavaCompileDiagnostic")))
    {
        // Explicit diagnostic only: editor demand compilation may not have
        // submitted any jobs for a material rejected before its first draw.
        // Request those jobs on the SAME saved MIC without saving the asset.
        // Never remove the first-call assertions or turn that failure green.
        Template->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Default);
        FAssetCompilingManager::Get().FinishAllCompilation();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        ReportShaderState(TEXT("AfterExplicitCompile"));
        ECreateFailure Failure = ECreateFailure::MissingResource;
        TStrongObjectPtr<UMaterialInstanceDynamic> Diagnostic(Create(Frame.Get(), Frame.Get(),
            LavaProfile(EPlanetType::Lava), RadiusCm, GMaxRHIFeatureLevel, &Failure));
        AddInfo(FString::Printf(TEXT("UNIFIED_FACTORY_COMPILE_DIAGNOSTIC installed=%d failure=%s; first-call result above remains authoritative"),
            Diagnostic.IsValid() ? 1 : 0, FailureName(Failure)));
    }
    AddInfo(TEXT("Factory/LocalVF/uniform contract only: no rendered appearance or terrain/ocean installation claim."));
    return true;
}

#endif
