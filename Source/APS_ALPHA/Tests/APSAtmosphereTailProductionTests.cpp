#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "APS_ALPHA/Core/Rendering/APSAtmosphereTailMaterial.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereTailProductionTest,
    "APS.Contracts.FrozenDescent.AtmosphereTail.Production",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmosphereTailProductionTest::RunTest(const FString& Parameters)
{
    using namespace APSAtmosphereTailMaterial;

    // Cover every represented byte, including unknown/future IDs. No actor or
    // world is created; only the explicitly accepted Frozen/Icy routes may opt in.
    for (int32 Value = 0; Value <= 255; ++Value)
    {
        const EPlanetType PlanetType = static_cast<EPlanetType>(Value);
        const EMoonType MoonType = static_cast<EMoonType>(Value);
        TestEqual(FString::Printf(TEXT("planet ID %d: Frozen only"), Value),
            EnabledFor(PlanetType), PlanetType == EPlanetType::Frozen);
        TestEqual(FString::Printf(TEXT("moon ID %d: Icy only"), Value),
            EnabledFor(MoonType), MoonType == EMoonType::Icy);
    }
    TestFalse(TEXT("accepted Theon Volcanic atmosphere remains native"), EnabledFor(EPlanetType::Volcanic));
    TestFalse(TEXT("Volcanic moons remain native"), EnabledFor(EMoonType::Volcanic));
    TestFalse(TEXT("Ice is not silently included in the Frozen rollout"), EnabledFor(EPlanetType::Ice));

    constexpr const TCHAR* NativeMasterPath = TEXT("/AtmoScape/Materials/Master/MM_PlanetaryAtmo.MM_PlanetaryAtmo");
    UMaterialInterface* Production = LoadObject<UMaterialInterface>(nullptr, MasterPath,
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    TestNotNull(TEXT("production atmosphere master must be baked; absence is a contract failure"), Production);
    if (Production)
    {
        TestEqual(TEXT("production master path"), Production->GetPathName(), FString(MasterPath));
        float Tail = 0.0f;
        const bool bHasTail = Production->GetScalarParameterValue(FMaterialParameterInfo(TailParameter), Tail);
        TestTrue(TEXT("production master exposes APS_AtmosphereTail"), bHasTail);
        if (bHasTail) TestEqual(TEXT("production atmosphere tail default is enabled"), Tail, 1.0f);
    }

    UMaterialInterface* Native = LoadObject<UMaterialInterface>(nullptr, NativeMasterPath,
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    TestNotNull(TEXT("original native atmosphere master is still available"), Native);
    if (Native)
    {
        float UnusedTail = 0.0f;
        TestFalse(TEXT("original native master has no production tail scalar"),
            Native->GetScalarParameterValue(FMaterialParameterInfo(TailParameter), UnusedTail));
    }

    // This is the native generator CDO's serialized object reference, not a
    // runtime load fallback. Run in a fresh process after creating the asset.
    const APlanetarySurfaceGenerator* Defaults = GetDefault<APlanetarySurfaceGenerator>();
    TestNotNull(TEXT("generator CDO exists without spawning a world"), Defaults);
    if (Defaults)
    {
        const UMaterialInterface* Reference = Defaults->ContinuousAtmosphereMaterial.Get();
        TestNotNull(TEXT("generator CDO keeps the production atmosphere hard reference"), Reference);
        if (Reference)
            TestEqual(TEXT("generator CDO hard reference names the production master"),
                Reference->GetPathName(), FString(MasterPath));
        if (Reference && Production)
            TestTrue(TEXT("generator CDO and loaded production master are the same object"), Reference == Production);
    }

    // Parameter/routing/load contracts are not rendered atmosphere acceptance.
    return true;
}

#endif
