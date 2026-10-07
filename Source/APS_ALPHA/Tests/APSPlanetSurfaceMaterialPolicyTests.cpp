#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceMaterialPolicy.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetSurfaceMaterialPolicyRoutingTest,
    "APS.Contracts.PlanetSurface.MaterialPolicy.Routing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceMaterialPolicyRoutingTest::RunTest(const FString& Parameters)
{
    namespace Policy = APSPlanetSurfaceMaterialPolicy;
    TestNull(TEXT("Legacy split terrain fallback must not be re-enabled by a console setting"),
        IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.SharedTerrainLegacyDiagnosticFallback")));
    TestNull(TEXT("Obsolete ground-only native replacement switch is removed"),
        IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.UseNativeTerrainMaterial")));
    TestTrue(TEXT("The original terrain route cannot be disabled"), APSTerrainContinuityMaterial::Enabled());
    for (const TCHAR* Name : {TEXT("aps.Surface.TerrainContinuity"), TEXT("aps.Surface.UnifiedLavaSurface")})
    {
        IConsoleVariable* Marker = IConsoleManager::Get().FindConsoleVariable(Name);
        if (!TestNotNull(Name, Marker)) return false;
        TestTrue(FString(Name) + TEXT(" is not an interactive rollback"), Marker->TestFlags(ECVF_ReadOnly));
    }
    TestFalse(TEXT("Diagnostic lava parent can no longer replace the original"), APSUnifiedLavaAssets::DetailCandidate());
    // Independent saved IDs, not a copy/iteration of the policy's accepted table.
    const TArray<int32> AcceptedTerrain = {0, 1, 10, 11, 13, 14, 18, 21, 22, 23, 24, 25, 27, 32};
    const TArray<int32> AcceptedWater = {1, 9, 25};
    TestEqual(TEXT("Explicit all-type fixture must be extended with the enum"), int32(APSPlanetTypes::LastValue), 34);
    const bool bProcessCandidate = Policy::UnifiedRoutesEnabled();
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
    TestEqual(TEXT("Unified routes default on; explicit legacy control alone opts out"), bProcessCandidate,
        !FParse::Param(FCommandLine::Get(), TEXT("APSLegacySurfacePipelineControl")));
#else
    TestTrue(TEXT("Unified routes enabled in non-editor builds"), bProcessCandidate);
#endif
    int32 ReleaseTerrainCount = 0, ReleaseWaterCount = 0, CandidateTerrainCount = 0, CandidateWaterCount = 0;
    for (int32 Value = 0; Value <= 255; ++Value)
    {
        FAPSResolvedPlanetSurfaceProfile P;
        P.PlanetType = static_cast<EPlanetType>(Value);
        P.Archetype = UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(P.PlanetType);
        P.LiquidType = EAPSPlanetLiquidType::Water;
        P.LandCoverage = .5f;
        const bool bSolid = Value <= 34 && Value != 4 && Value != 5 && Value != 6 && Value != 30;
        const bool bNonMagmatic = bSolid && Value != 3 && Value != 12 && Value != 19;
        const bool bAcceptedTerrain = AcceptedTerrain.Contains(Value);
        const bool bAcceptedWater = AcceptedWater.Contains(Value);
        TestEqual(FString::Printf(TEXT("Known concrete solid ID %d"), Value), Policy::IsConcreteSolidType(P.PlanetType), bSolid);
        for (bool bCandidate : {false, true})
        {
            const FString Prefix = FString::Printf(TEXT("candidate%d type%d "), bCandidate, Value);
            TestEqual(Prefix + TEXT("terrain gate"), Policy::AllowsContinuousTerrain(P.PlanetType, bCandidate),
                bCandidate ? bNonMagmatic : bAcceptedTerrain);
            TestEqual(Prefix + TEXT("water profile gate"), Policy::AllowsCoastalWater(P, bCandidate),
                bCandidate ? bSolid : bAcceptedWater);
        }
        TestEqual(FString::Printf(TEXT("Runtime terrain wrapper ID %d"), Value), APSTerrainContinuityMaterial::Allows(P.PlanetType),
            bNonMagmatic);
        TestEqual(FString::Printf(TEXT("Runtime water wrapper ID %d"), Value), APSCoastalWaterMaterial::Allows(P),
            bProcessCandidate ? bSolid : bAcceptedWater);
        const bool bRouteContinuity = bNonMagmatic;
        TestEqual(FString::Printf(TEXT("Actual saved terrain route ID %d"), Value), FString(APSSharedTerrainMaterial::TemplatePath(P)),
            FString(bRouteContinuity ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra")
                : (Value == 3 || Value == 12 || Value == 19)
                    ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma")
                    : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra")));
        // A catalog may override the archetype. Preserve the existing outer
        // magmatic veto even for accepted or candidate nonmagmatic preset IDs.
        P.Archetype = EAPSPlanetSurfaceArchetype::Magmatic;
        TestEqual(FString::Printf(TEXT("Resolved magmatic veto ID %d"), Value), FString(APSSharedTerrainMaterial::TemplatePath(P)),
            FString(TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma")));
        ReleaseTerrainCount += Policy::AllowsContinuousTerrain(P.PlanetType, false) ? 1 : 0;
        ReleaseWaterCount += Policy::AllowsCoastalWater(P, false) ? 1 : 0;
        CandidateTerrainCount += Policy::AllowsContinuousTerrain(P.PlanetType, true) ? 1 : 0;
        CandidateWaterCount += Policy::AllowsCoastalWater(P, true) ? 1 : 0;
    }
    TestEqual(TEXT("Preserved legacy control terrain count"), ReleaseTerrainCount, 14);
    TestEqual(TEXT("Preserved legacy control coastal count"), ReleaseWaterCount, 3);
    TestEqual(TEXT("Unified nonmagmatic terrain count including legacy 29"), CandidateTerrainCount, 28);
    TestEqual(TEXT("Unified Water-profile eligible count including legacy 29"), CandidateWaterCount, 31);

    FAPSResolvedPlanetSurfaceProfile P;
    P.PlanetType = EPlanetType::Terrestrial;
    P.LiquidType = EAPSPlanetLiquidType::Water;
    for (bool bCandidate : {false, true})
    {
        for (float Coverage : {0.0f, .5f, .994f})
        {
            P.LandCoverage = Coverage;
            TestTrue(TEXT("Water coverage inside the unchanged envelope"), Policy::AllowsCoastalWater(P, bCandidate));
        }
        for (float Coverage : {-.01f, .995f, 1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
        {
            P.LandCoverage = Coverage;
            TestFalse(TEXT("Invalid/dry Water coverage rejected"), Policy::AllowsCoastalWater(P, bCandidate));
        }
        P.LandCoverage = .5f;
        for (EAPSPlanetLiquidType Liquid : {EAPSPlanetLiquidType::None, EAPSPlanetLiquidType::Lava, EAPSPlanetLiquidType::Ammonia})
        {
            P.LiquidType = Liquid;
            TestFalse(TEXT("No other liquid chemistry is promoted"), Policy::AllowsCoastalWater(P, bCandidate));
        }
        P.LiquidType = EAPSPlanetLiquidType::Water;
    }
    P.PlanetType = EPlanetType::Volcanic;
    P.Archetype = EAPSPlanetSurfaceArchetype::Magmatic;
    P.LiquidType = EAPSPlanetLiquidType::Lava;
    P.TerrainSeed = 793597;
    const uint32 Signature = UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P);
    for (bool bCandidate : {false, true})
    {
        TestFalse(TEXT("Theon preset never reroutes to ContinuousTerra"), Policy::AllowsContinuousTerrain(P.PlanetType, bCandidate));
        TestFalse(TEXT("Theon lava never reroutes to Water"), Policy::AllowsCoastalWater(P, bCandidate));
    }
    TestEqual(TEXT("Gate queries do not mutate the resolved profile"), UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(P), Signature);
    P.PlanetType = EPlanetType::Frozen;
    P.Archetype = EAPSPlanetSurfaceArchetype::Cryogenic;
    TStrongObjectPtr<UMaterialInstance> Original(LoadObject<UMaterialInstance>(nullptr, Policy::ContinuousTemplatePath));
    TStrongObjectPtr<UMaterialInstance> OldShared(LoadObject<UMaterialInstance>(nullptr,
        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra")));
    if (!TestNotNull(TEXT("Original saved terrain"), Original.Get())
        || !TestNotNull(TEXT("Legacy shared fixture"), OldShared.Get())) return false;
    TestTrue(TEXT("Exact original template and master accepted"), APSSharedTerrainMaterial::IsExactTemplate(Original.Get(), P));
    TestTrue(TEXT("Stock SharedTerra must pass through canonical selection"), APSSharedTerrainMaterial::IsStockGeneratedTemplate(OldShared.Get()));
    TStrongObjectPtr<UMaterialInstanceDynamic> Custom(UMaterialInstanceDynamic::Create(Original.Get(), GetTransientPackage()));
    if (!TestNotNull(TEXT("Custom material fixture"), Custom.Get())) return false;
    TestFalse(TEXT("Custom instance of original graph is not a stock migration target"), APSSharedTerrainMaterial::IsStockGeneratedTemplate(Custom.Get()));
    TestFalse(TEXT("Old SharedTerra cannot masquerade as canonical Frozen"), APSSharedTerrainMaterial::IsExactTemplate(OldShared.Get(), P));
    TestFalse(TEXT("Missing original is never accepted as a substitute"), APSSharedTerrainMaterial::IsExactTemplate(nullptr, P));
    AddInfo(TEXT("Routing contracts only: no claim of rendered family acceptance or exact live Theon profile reproduction."));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetSurfaceMaterialPolicyFarNormalTest,
    "APS.Contracts.PlanetSurface.MaterialPolicy.FarNormalUniforms",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceMaterialPolicyFarNormalTest::RunTest(const FString& Parameters)
{
    namespace Policy = APSPlanetSurfaceMaterialPolicy;
    TestFalse(TEXT("Null material is rejected"), Policy::ApplyFarNormalPolicy(nullptr));
    struct FFixture { const TCHAR* TemplatePath; Policy::ETerrainGraph Graph; };
    const FFixture Fixtures[] = {
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"), Policy::ETerrainGraph::Shared},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedMagma.MI_APS_SharedMagma"), Policy::ETerrainGraph::Shared},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra"), Policy::ETerrainGraph::Continuous},
        {TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface"), Policy::ETerrainGraph::UnifiedLava},
        {TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra"), Policy::ETerrainGraph::Other}
    };
    for (const FFixture& Fixture : Fixtures)
    {
        TStrongObjectPtr<UMaterialInstance> Parent(LoadObject<UMaterialInstance>(nullptr, Fixture.TemplatePath));
        if (!TestNotNull(Fixture.TemplatePath, Parent.Get())) return false;
        TStrongObjectPtr<UMaterialInstanceDynamic> Before(UMaterialInstanceDynamic::Create(Parent.Get(), GetTransientPackage()));
        TStrongObjectPtr<UMaterialInstanceDynamic> After(UMaterialInstanceDynamic::Create(Parent.Get(), GetTransientPackage()));
        if (!TestNotNull(TEXT("Baseline transient MID"), Before.Get()) || !TestNotNull(TEXT("Policy transient MID"), After.Get())) return false;
        TestEqual(TEXT("Exact saved graph classification"), uint8(Policy::TerrainGraph(After.Get())), uint8(Fixture.Graph));
        const bool bEligible = Fixture.Graph != Policy::ETerrainGraph::Other;
        // Non-default sentinels expose an accidental extra macro/warp write,
        // including when the saved default happens to match such an override.
        for (const TCHAR* Name : {TEXT("APS_OrbitalMacroMode"), TEXT("APS_NormalMacroWarpMode")})
        {
            float Existing = 0;
            if (Before->GetScalarParameterValue(FHashedMaterialParameterInfo(Name), Existing))
            {
                Before->SetScalarParameterValue(Name, .375f);
                After->SetScalarParameterValue(Name, .375f);
            }
        }
        const int32 BeforeOverrides = After->ScalarParameterValues.Num();
        TestEqual(TEXT("Only supported exact graphs accept the common filter"), Policy::ApplyFarNormalPolicy(After.Get()), bEligible);
        TestTrue(TEXT("The exact template is retained"), Before->Parent == After->Parent);
        TestEqual(TEXT("Only two far-normal overrides may be introduced"), After->ScalarParameterValues.Num(), BeforeOverrides + (bEligible ? 2 : 0));
        if (bEligible)
        {
            float Start = -1, End = -1;
            TestTrue(TEXT("Start uniform exists"), After->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), Start));
            TestTrue(TEXT("End uniform exists"), After->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), End));
            TestEqual(TEXT("Accepted physical start 2 km"), Start, 200000.0f);
            TestEqual(TEXT("Accepted physical end 20 km"), End, 2000000.0f);
        }
        TArray<FMaterialParameterInfo> Infos;
        TArray<FGuid> Ids;
        Before->GetAllScalarParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            if (bEligible && (Info.Name == TEXT("APS_FarNormalStartCm") || Info.Name == TEXT("APS_FarNormalEndCm"))) continue;
            float A = 0, B = 0;
            TestTrue(TEXT("Unchanged scalar: ") + Info.Name.ToString(), Before->GetScalarParameterValue(Info, A)
                && After->GetScalarParameterValue(Info, B) && A == B);
        }
        Infos.Reset(); Ids.Reset(); Before->GetAllVectorParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            FLinearColor A, B;
            TestTrue(TEXT("Unchanged palette/vector: ") + Info.Name.ToString(), Before->GetVectorParameterValue(Info, A)
                && After->GetVectorParameterValue(Info, B) && A == B);
        }
        Infos.Reset(); Ids.Reset(); Before->GetAllTextureParameterInfo(Infos, Ids);
        for (const FMaterialParameterInfo& Info : Infos)
        {
            UTexture* A = nullptr; UTexture* B = nullptr;
            TestTrue(TEXT("Unchanged texture: ") + Info.Name.ToString(), Before->GetTextureParameterValue(Info, A)
                && After->GetTextureParameterValue(Info, B) && A == B);
        }
        for (const TCHAR* Name : {TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"), TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ")})
        {
            FVector4 A, B;
            const bool bBefore = Before->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), A);
            const bool bAfter = After->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), B);
            TestTrue(FString(TEXT("Unchanged physical frame: ")) + Name, bBefore == bAfter && (!bBefore || A == B));
        }
        const int32 AfterOverrides = After->ScalarParameterValues.Num();
        Policy::ApplyFarNormalPolicy(After.Get());
        TestEqual(TEXT("Repeated application does not add overrides"), After->ScalarParameterValues.Num(), AfterOverrides);
    }
    AddInfo(TEXT("Saved parents are read-only; only transient MIDs change. No asset bake, world, shader-readiness or visual acceptance claim."));
    return true;
}

#endif
