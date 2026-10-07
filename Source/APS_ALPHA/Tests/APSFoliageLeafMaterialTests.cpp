#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSFoliageLeafMaterial.h"
#include <limits>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSFoliageLeafFadeContract,
    "APS.Gameplay.World.PlanetSurface.Foliage.LeafFadeContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSFoliageLeafFadeContract::RunTest(const FString& Parameters)
{
    using namespace APSFoliageLeafMaterial;
    TestEqual(TEXT("Near crown retained"), Visibility(1300), 1.f);
    TestEqual(TEXT("Retained to 80m"), Visibility(8000), 1.f);
    TestEqual(TEXT("Fade midpoint"), Visibility(8750), .5f);
    TestEqual(TEXT("Gone by 95m"), Visibility(9500), 0.f);
    TestEqual(TEXT("No leaf at native 100m cull"), Visibility(10000), 0.f);
    TestEqual(TEXT("Bad coordinates fail quiet"), Visibility(std::numeric_limits<float>::infinity()), 0.f);
    for (const TCHAR* Mesh : {TEXT("SM_APS_Scatter_Grass"), TEXT("SM_APS_Scatter_ColdGrass")})
    {
        TestTrue(TEXT("Temperate and cold near cards receive owned fade"), NeedsOwnedGrassFade(Mesh,
            TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst.MI_Grass_Inst")));
        TestTrue(TEXT("Temperate and cold LOD cards retain their own atlas with owned fade"), NeedsOwnedGrassFade(Mesh,
            TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_InstLOD.MI_Grass_InstLOD")));
        TestFalse(TEXT("Unrelated material binding is preserved"), NeedsOwnedGrassFade(Mesh,
            TEXT("/Game/Authored/MI_Grass_Inst.MI_Grass_Inst")));
    }
    for (const TCHAR* Path : {
        TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst_2.MI_Grass_Inst_2"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_InstLOD_2.MI_Grass_InstLOD_2")})
        TestTrue(TEXT("Accepted dry near/LOD treatment is unchanged"), NeedsOwnedGrassFade(TEXT("SM_APS_Scatter_DryGrass"), Path));
    TestFalse(TEXT("Unowned geometry is never substituted"), NeedsOwnedGrassFade(TEXT("SM_Grass_Temp"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst.MI_Grass_Inst")));
    TestFalse(TEXT("Rock binding cannot become vegetation"), NeedsOwnedGrassFade(TEXT("SM_APS_Scatter_RockA"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst.MI_Grass_Inst")));
    TestFalse(TEXT("Role-specific texture atlas is not silently exchanged"), NeedsOwnedGrassFade(TEXT("SM_APS_Scatter_DryGrass"),
        TEXT("/WorldScape/Ressources/Mesh/Grass/MI_Grass_Inst.MI_Grass_Inst")));
    float Last = 1;
    for (int32 Cm = 0; Cm <= 11000; ++Cm)
    {
        const float Value = Visibility(float(Cm));
        TestTrue(TEXT("Bounded continuous nonincreasing fade"), Value >= 0 && Value <= Last && Last - Value <= .0011f);
        Last = Value;
    }
    AddInfo(TEXT("Scalar contract only; leaves, distance transition and GPU cost require rendered validation."));
    return true;
}
#endif
