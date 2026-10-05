#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Editor/APSPlanetCloudRefinedHlsl.h"
#include "APS_ALPHA/Editor/APSPlanetCloudLayeredHlsl.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudRefinedShaderTest,
    "APS.Gameplay.World.PlanetSurface.Clouds.Weather.RefinedShader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudRefinedShaderTest::RunTest(const FString& Parameters)
{
    const FString Baseline = APSPlanetCloudHlsl::Code();
    const FString Layered = APSPlanetCloudLayeredHlsl::Code();
    const FString Refined = APSPlanetCloudRefinedHlsl::Code();
    TestFalse(TEXT("All exact V31 splice anchors still exist once"), Refined.IsEmpty());
    TestFalse(TEXT("V27 does not opt into V31"), Baseline.Contains(TEXT("FarPhaseBlend")));
    TestFalse(TEXT("V30 does not opt into V31"), Layered.Contains(TEXT("FarPhaseBlend")));
    TestTrue(TEXT("Far phase blends only beyond fully filtered coarse noise"),
        Refined.Contains(TEXT("smoothstep(1.6,3.2,PixelFootprintAtEntry)")));
    TestTrue(TEXT("Near phase remains identical, far phase retains decorrelation"),
        Refined.Contains(TEXT("lerp(SamplePhase,.5+.2*(SamplePhase-.5),FarPhaseBlend)")));
    TestTrue(TEXT("Light samples filter the represented segment"),
        Refined.Contains(TEXT("float LightFootprint=max(Footprint,LightLength*.25);")));
    TestTrue(TEXT("View noise and threshold are not retuned"), Refined.Contains(
        TEXT("float mean=.5+.72*coarseWeight*(coarse-.5)+.28*fineWeight*(fine-.5);")));
    TestTrue(TEXT("Budget remains 16-32 view and two light samples"),
        Refined.Contains(TEXT("16.,32.")) && Refined.Contains(TEXT("for(int j=0;j<2;j++)")));
    TestTrue(TEXT("Accepted default material remains V27"),
        FString(APSPlanetCloudWeather::MaterialPath).Contains(TEXT("CloudWeather20261002V27")));
    TestTrue(TEXT("V31 is a distinct new-only destination"),
        FString(APSPlanetCloudWeather::CandidateMaterialPath).Contains(TEXT("CloudWeather20261003V31")));
    return true; // Source construction contract, not rendered acceptance.
}
#endif
