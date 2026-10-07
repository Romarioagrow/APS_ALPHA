#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSTerrestrialMaterialPalette.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSTerrestrialMaterialPaletteTest,
    "APS.Gameplay.World.PlanetSurface.TerrestrialMaterialPalette",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSTerrestrialMaterialPaletteTest::RunTest(const FString& Parameters)
{
    using namespace APSTerrestrialMaterialPalette;
    for (const EPlanetType Type : {EPlanetType::Terrestrial, EPlanetType::Frozen,
        EPlanetType::Oasis, EPlanetType::Metallic, EPlanetType::Forest})
        TestEqual(TEXT("Initial scope is the requested Earth-like material"),
            Allows(Type, APSTerrainContinuityMaterial::TemplatePath), Type == EPlanetType::Terrestrial);
    TestFalse(TEXT("Authored/native/bespoke materials are not opted in"), Allows(EPlanetType::Terrestrial, TEXT("/Game/Custom")));
    const FName Names[] = {TEXT("Color1"), TEXT("Color2"), TEXT("Color3"), TEXT("Color4"), TEXT("Color5"),
        TEXT("BottomColor"), TEXT("Sedimentcolor"), TEXT("2_Color1"), TEXT("2_Color2"), TEXT("2_Color3"),
        TEXT("2_Color4"), TEXT("Color2_3")};
    for (const auto& C : {FLinearColor(FColor(45,92,48)), FLinearColor(FColor(65,125,57)),
        FLinearColor(FColor(184,162,105)), FLinearColor(FColor(165,132,75)), FLinearColor(.3f,.3f,.3f,.7f)})
        for (FName Name : Names)
        {
            const auto Result = Refine(Name, C);
            TestTrue(TEXT("Material luminance preserved, not a gain/exposure change"),
                FMath::IsNearlyEqual(Luminance(Result), Luminance(C), 0.000001f));
            TestEqual(TEXT("Alpha unchanged"), Result.A, C.A);
            const float OldChroma = FMath::Max3(C.R,C.G,C.B)-FMath::Min3(C.R,C.G,C.B);
            const float NewChroma = FMath::Max3(Result.R,Result.G,Result.B)-FMath::Min3(Result.R,Result.G,Result.B);
            TestTrue(TEXT("Predictable bounded chroma compression"),
                FMath::IsNearlyEqual(NewChroma, OldChroma*ChromaScale(Name), 0.000001f));
            TestTrue(TEXT("Convex result stays in original RGB interval"),
                FMath::Min3(Result.R,Result.G,Result.B) >= FMath::Min3(C.R,C.G,C.B)-0.000001f
                && FMath::Max3(Result.R,Result.G,Result.B) <= FMath::Max3(C.R,C.G,C.B)+0.000001f);
        }
    const FLinearColor Marker(.08f,.24f,.12f,.7f);
    for (FName Name : {FName(TEXT("Color1_3")), FName(TEXT("SlopeColor")), FName(TEXT("EmissiveColor")),
        FName(TEXT("ColotTint")), FName(TEXT("APS_SharedDetailRowXHigh"))})
        TestEqual(TEXT("Unrelated endpoints/physical frame remain exact"), Refine(Name, Marker), Marker);
    return true; // Arithmetic/selection evidence, not rendered appearance acceptance.
}
#endif
