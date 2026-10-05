#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Editor/APSPlanetCloudClusteredHlsl.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h"

namespace
{
int32 CountCloudClusterToken(const FString& Text, const TCHAR* Token)
{
    int32 Count = 0, Offset = 0;
    while ((Offset = Text.Find(Token, ESearchCase::CaseSensitive, ESearchDir::FromStart, Offset)) != INDEX_NONE)
    {
        ++Count;
        Offset += FCString::Strlen(Token);
    }
    return Count;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudClusteredShaderTest,
    "APS.Gameplay.World.PlanetSurface.Clouds.Weather.ClusteredShader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudClusteredShaderTest::RunTest(const FString& Parameters)
{
    const FString Baseline = APSPlanetCloudHlsl::Code();
    const FString Refined = APSPlanetCloudRefinedHlsl::Code();
    const FString Clustered = APSPlanetCloudClusteredHlsl::Code();
    TestFalse(TEXT("Both exact V32 splice anchors still exist once"), Clustered.IsEmpty());
    if (Clustered.IsEmpty()) return false;
    TestEqual(TEXT("Constructing V32 does not mutate V27"), APSPlanetCloudHlsl::Code(), Baseline);
    TestEqual(TEXT("Constructing V32 does not mutate V31"), APSPlanetCloudRefinedHlsl::Code(), Refined);
    for (const FString* Prior : { &Baseline, &Refined })
    {
        TestFalse(TEXT("Prior shaders do not acquire clustered meso"), Prior->Contains(TEXT("MesoFrequency")));
        TestTrue(TEXT("Prior weather envelope remains unchanged"), Prior->Contains(
            TEXT("float WeatherBody=saturate(Shape+.22*(Meso-.5));")));
    }
    TestEqual(TEXT("Existing meso fetch is replaced once"), CountCloudClusterToken(Refined,
        TEXT("float Meso=F.noise(F.rotateDomain(WeatherN*(32./max(WeatherScale,.25)))+SeedOffset);")), 1);
    TestFalse(TEXT("Old planet-normalized meso is absent in V32"), Clustered.Contains(
        TEXT("WeatherN*(32./max(WeatherScale,.25))")));
    for (const TCHAR* Anchor : {
        TEXT("float MesoFrequency=BillowFrequencyScale/24.;"),
        TEXT("float MesoResolved=1.-smoothstep(.25,.5,Footprint*MesoFrequency);"),
        TEXT("float Meso=.5;"),
        TEXT("if(MesoResolved>.001)"),
        TEXT("WeatherN*(max(Radius,1.)*MesoFrequency))+SeedOffset)-.5);"),
        TEXT("float WeatherBody=saturate(Shape+.65*(smoothstep(.32,.68,Meso)-.5));") })
        TestEqual(TEXT("V32 exact generated anchor occurs once"), CountCloudClusterToken(Clustered, Anchor), 1);
    TestEqual(TEXT("Meso still uses one noise evaluation in the view march"),
        CountCloudClusterToken(Clustered, TEXT("F.noise(")), CountCloudClusterToken(Refined, TEXT("F.noise(")));
    TestEqual(TEXT("Density octave evaluation count is unchanged"),
        CountCloudClusterToken(Clustered, TEXT("=noise(")), CountCloudClusterToken(Refined, TEXT("=noise(")));
    for (const TCHAR* Retained : {
        TEXT("float coarseWeight=1.-smoothstep(.25,.5,footprint*.32);"),
        TEXT("float fineWeight=1.-smoothstep(.25,.5,footprint*.93);"),
        TEXT("float mean=.5+.72*coarseWeight*(coarse-.5)+.28*fineWeight*(fine-.5);"),
        TEXT("float heightDistance=abs((h-.46)/lerp(.46,.54,step(.46,h)));"),
        TEXT("float3 WeatherN=F.advect(N,WindRotation.xy);"),
        TEXT("float Weather=F.weather(WeatherN,SeedOffset,WeatherScale,Banding,Swirl);"),
        TEXT("float Footprint=max(RayFootprint*SampleDistance,Step*.5);"),
        TEXT("int Samples=(int)clamp(ceil((End-Start)/max(Thickness,.001)*16.),16.,32.);"),
        TEXT("[unroll] for(int j=0;j<2;j++)"),
        TEXT("float LT=F.bodyThreshold(LH,WeatherBody);"),
        TEXT("float LightFootprint=max(Footprint,LightLength*.25);"),
        TEXT("float StablePhase=lerp(SamplePhase,.5+.2*(SamplePhase-.5),FarPhaseBlend);"),
        TEXT("*smoothstep(0.,.20,Shape);"),
        TEXT("Sum+=Contribution*Light*Albedo;") })
        TestTrue(TEXT("Billows, climate envelope, height, lighting and work budget are retained"), Clustered.Contains(Retained));
    TestTrue(TEXT("Default production material remains V27"),
        FString(APSPlanetCloudWeather::MaterialPath).Contains(TEXT("CloudWeather20261002V27")));
    return true; // Source construction contract only, not rendered acceptance.
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudClusteredFilterTest,
    "APS.Gameplay.World.PlanetSurface.Clouds.Weather.ClusteredFilter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCloudClusteredFilterTest::RunTest(const FString& Parameters)
{
    const auto Smooth = [](float A, float B, float Value)
    {
        const float T = FMath::Clamp((Value-A)/(B-A), 0.f, 1.f);
        return T*T*(3.f-2.f*T);
    };
    for (float Size : { .25f, 1.f, 3.f })
    {
        const float Frequency = 1.f / (24.f*Size);
        TestEqual(TEXT("Fully resolved meso retains its original sample"),
            1.f-Smooth(.25f, .5f, 0.f*Frequency), 1.f);
        TestEqual(TEXT("Meso reaches its mean at half a noise-domain unit"),
            1.f-Smooth(.25f, .5f, (12.f*Size)*Frequency), 0.f);
        float Previous = 1.f;
        for (int32 I=0; I<=48; ++I)
        {
            const float Resolved = 1.f-Smooth(.25f, .5f, float(I)*Size*Frequency);
            TestTrue(TEXT("Increasing footprint smoothly removes meso detail"),
                Resolved>=0.f && Resolved<=Previous);
            Previous = Resolved;
        }
    }
    for (float Shape : { 0.f, .25f, .5f, .75f, 1.f })
    {
        const float CentralBody = FMath::Clamp(Shape+.65f*(Smooth(.32f,.68f,.5f)-.5f),0.f,1.f);
        TestTrue(TEXT("Unresolved meso returns the central weather envelope"),
            FMath::IsNearlyEqual(CentralBody,Shape,1.e-6f));
    }
    TestTrue(TEXT("Resolved low meso reduces the core threshold input"),
        FMath::Clamp(.5f+.65f*(Smooth(.32f,.68f,.25f)-.5f),0.f,1.f)<.5f);
    TestTrue(TEXT("Resolved high meso strengthens the core threshold input"),
        FMath::Clamp(.5f+.65f*(Smooth(.32f,.68f,.75f)-.5f),0.f,1.f)>.5f);
    return true; // Filter algebra only; mean density and opacity are not conserved by this assertion.
}
#endif
