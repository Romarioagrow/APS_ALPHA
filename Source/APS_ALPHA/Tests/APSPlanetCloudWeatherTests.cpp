#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudWeather.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudWeatherClimateTest,"APS.Gameplay.World.PlanetSurface.Clouds.Weather.Climate",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSCloudWeatherClimateTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudWeather;
    FAPSResolvedPlanetSurfaceProfile P;
    P.PlanetType=EPlanetType::Terrestrial;P.Humidity=.6f;P.Temperature=.45f;
    P.AtmosphericPressure=1;P.SeismicActivity=.5f;P.BiomeSeed=424242;
    FAPSPlanetCloudSettings S;
    const auto Water=Resolve(P,100,false,30,S);
    auto FullCoverage=S;FullCoverage.CoverageScale=1.;
    TestEqual(TEXT("Default coverage is half the unsaturated climate multiplier"),
        Water.Coverage,Resolve(P,100,false,30,FullCoverage).Coverage*.5f);
    TestTrue(TEXT("Wet climate has water clouds"),Water.Enabled&&Water.Condensate==ECondensate::Water);
    TestEqual(TEXT("Identical model repeats weather"),Water.Offset,Resolve(P,100,false,30,S).Offset);
    TestTrue(TEXT("Wind controls storms"),Water.Swirl>0&&Resolve(P,100,false,0,S).Swirl==0);
    P.Humidity=.8f;TestTrue(TEXT("Wetter model increases coverage"),Resolve(P,100,false,30,S).Coverage>Water.Coverage);
    P.Humidity=.6f;P.Temperature=.1f;
    TestTrue(TEXT("Cold water-bearing planet has ice clouds"),Resolve(P,100,false,30,S).Condensate==ECondensate::Ice);
    P.Temperature=.45f;
    struct FCase { EPlanetType Type; ECondensate Cloud; };
    for(const FCase C: { FCase{EPlanetType::Ammonia,ECondensate::Ammonia},
        FCase{EPlanetType::Greenhouse,ECondensate::AcidAerosol},
        FCase{EPlanetType::Sulfur,ECondensate::AcidAerosol},
        FCase{EPlanetType::Volcanic,ECondensate::Ash},
        FCase{EPlanetType::Desert,ECondensate::Dust},
        FCase{EPlanetType::Carbon,ECondensate::Hydrocarbon} })
    {
        P.PlanetType=C.Type;
        const auto L=Resolve(P,100,false,30,S);
        TestTrue(TEXT("Non-terrestrial material follows the model family"),L.Enabled&&L.Condensate==C.Cloud);
        TestFalse(TEXT("Not just the same white cloud tint"),L.Albedo.Equals(Water.Albedo));
    }
    for(int32 Type=0;Type<=APSPlanetTypes::LastValue;++Type)
        for(double H:{4.,10.,100.,2000.}) for(float Pressure:{0.f,.05f,.5f,4.f})
    {
        P.PlanetType=EPlanetType(Type);P.AtmosphericPressure=Pressure;
        const auto L=Resolve(P,H,false,30,S);
        if(L.Enabled) TestTrue(TEXT("Layer remains inside atmosphere with finite optics"),
            L.BottomKm>0&&L.ThicknessKm>0&&L.BottomKm+L.ThicknessKm<H
            &&FMath::IsFinite(L.Density)&&L.Coverage>0&&L.Coverage<=1);
        if(Pressure==0) TestFalse(TEXT("No family paints clouds into vacuum"),L.Enabled);
        TestFalse(TEXT("Authored bodies untouched"),Resolve(P,H,true,30,S).Enabled);
    }
    P.PlanetType=EPlanetType::Terrestrial;P.AtmosphericPressure=1;
    S.CoverageScale=0;TestFalse(TEXT("Coverage zero removes the layer"),Resolve(P,100,false,30,S).Enabled);
    S.CoverageScale=1;S.DensityScale=0;TestFalse(TEXT("Density zero removes the layer"),Resolve(P,100,false,30,S).Enabled);
    S.DensityScale=1;S.SeedOffset=7;
    TestFalse(TEXT("Weather seed independent of terrain seed"),Resolve(P,100,false,30,S).Offset.Equals(Water.Offset));
    TestEqual(TEXT("Resolver never changes terrain identity"),P.BiomeSeed,424242);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudWeatherBoundsTest,"APS.Gameplay.World.PlanetSurface.Clouds.Weather.Bounds",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSCloudWeatherBoundsTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudWeather;
    FAPSResolvedPlanetSurfaceProfile P;P.PlanetType=EPlanetType::Terrestrial;
    P.Humidity=.5f;P.Temperature=.45f;P.AtmosphericPressure=1;
    FAPSPlanetCloudSettings S;S.CoverageScale=std::numeric_limits<double>::quiet_NaN();
    S.DensityScale=std::numeric_limits<double>::infinity();S.AltitudeScale=1.e20;S.SeedOffset=MAX_int32;
    S.FeatureScale=S.WindScale=S.StormScale=std::numeric_limits<double>::quiet_NaN();
    const auto Safe=S.Sanitized();
    TestEqual(TEXT("Bad coverage returns the half-coverage default"),Safe.CoverageScale,.5);
    TestEqual(TEXT("Bad density returns automatic"),Safe.DensityScale,1.);
    TestEqual(TEXT("Bad feature scale retains its unit fallback"),Safe.FeatureScale,1.);
    TestEqual(TEXT("Bad wind retains its unit fallback"),Safe.WindScale,1.);
    TestEqual(TEXT("Bad storms retain their unit fallback"),Safe.StormScale,1.);
    TestEqual(TEXT("Huge altitude bounded"),Safe.AltitudeScale,2.);
    TestEqual(TEXT("Seed bounded before use"),Safe.SeedOffset,999983);
    S.AltitudeScale=std::numeric_limits<double>::quiet_NaN();
    TestEqual(TEXT("Bad altitude retains its unit fallback"),S.Sanitized().AltitudeScale,1.);
    S.CoverageScale=1.;
    TestEqual(TEXT("Finite authored full coverage is not migrated"),S.Sanitized().CoverageScale,1.);
    for(float* V:{&P.Humidity,&P.Temperature,&P.AtmosphericPressure})
    {
        const float Old=*V;*V=std::numeric_limits<float>::quiet_NaN();
        TestFalse(TEXT("Nonfinite climate is not rendered"),Resolve(P,100,false,30,S).Enabled);*V=Old;
    }
    for(double T:{0.,1.,86400.,1.e8})
    {
        TestTrue(TEXT("Wind rotation finite unit vector"),FMath::IsNearlyEqual(WindRotation(T,6750,.05f).Size(),1.,1.e-12));
        TestEqual(TEXT("Zero wind stays static"),WindRotation(T,6750,0),FVector(1,0,0));
    }
    const double Period=2.*UE_DOUBLE_PI*6750./double(.05f);
    TestTrue(TEXT("Advection wrap has no seam"),WindRotation(Period-.001,6750,.05f).Equals(WindRotation(Period+.001,6750,.05f),1.e-7));
    TestTrue(TEXT("Ordinary timestep is continuous"),WindRotation(100.,6750,.05f).Equals(WindRotation(100.+1./60.,6750,.05f),1.e-6));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudWeatherWindContinuityTest,"APS.Gameplay.World.PlanetSurface.Clouds.Weather.WindContinuity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSCloudWeatherWindContinuityTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudWeather;
    constexpr double Radius=1000.;
    constexpr float Speed=.05f;
    const double Moving=AdvanceWindPhase(0.,1000.,Radius,Speed);
    TestTrue(TEXT("Wind first reaches a nonzero phase"),Moving>0.);
    const double Paused=AdvanceWindPhase(Moving,600.,Radius,0.f);
    TestEqual(TEXT("Zero wind freezes the current phase"),Paused,Moving);
    TestEqual(TEXT("Paused clouds retain their current orientation"),RotationFromPhase(Paused),RotationFromPhase(Moving));
    const double Resumed=AdvanceWindPhase(Paused,10.,Radius,Speed);
    TestTrue(TEXT("Resuming continues from the paused phase"),FMath::IsNearlyEqual(
        Resumed,Moving+10.*double(Speed)/Radius,1.e-12));
    TestEqual(TEXT("Changing speed without a timestep cannot jump phase"),
        AdvanceWindPhase(Moving,0.,Radius,.15f),Moving);
    const double Faster=AdvanceWindPhase(Moving,10.,Radius,.15f);
    TestTrue(TEXT("New speed affects only the next timestep"),FMath::IsNearlyEqual(
        Faster,Moving+10.*double(.15f)/Radius,1.e-12));
    double Segmented=Moving;
    for(int32 I=0;I<60;++I) Segmented=AdvanceWindPhase(Segmented,1./60.,Radius,Speed);
    TestTrue(TEXT("Segmented and combined timesteps agree"),FMath::IsNearlyEqual(
        Segmented,AdvanceWindPhase(Moving,1.,Radius,Speed),1.e-12));
    const double BeforeWrap=2.*UE_DOUBLE_PI-.00001;
    const double Wrapped=AdvanceWindPhase(BeforeWrap,1.,Radius,Speed);
    TestTrue(TEXT("Wrapped phase stays finite and bounded"),FMath::IsFinite(Wrapped)&&Wrapped>=0.&&Wrapped<2.*UE_DOUBLE_PI);
    TestTrue(TEXT("Rotation remains continuous across wrap"),RotationFromPhase(Wrapped).Equals(
        RotationFromPhase(BeforeWrap+double(Speed)/Radius),1.e-12));
    TestEqual(TEXT("Negative timestep cannot rewind weather"),AdvanceWindPhase(Moving,-1.,Radius,Speed),Moving);
    TestEqual(TEXT("Invalid radius preserves current phase"),AdvanceWindPhase(Moving,1.,0.,Speed),Moving);
    TestEqual(TEXT("Nonfinite timestep preserves current phase"),
        AdvanceWindPhase(Moving,std::numeric_limits<double>::infinity(),Radius,Speed),Moving);
    TestEqual(TEXT("Nonfinite speed preserves current phase"),
        AdvanceWindPhase(Moving,1.,Radius,std::numeric_limits<float>::quiet_NaN()),Moving);
    TestEqual(TEXT("Nonfinite phase recovers without invalid rotation"),
        RotationFromPhase(std::numeric_limits<double>::quiet_NaN()),FVector(1,0,0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudWeatherSaveTest,"APS.Gameplay.World.PlanetSurface.Clouds.Weather.SaveReplay",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSCloudWeatherSaveTest::RunTest(const FString& Parameters)
{
    const auto RoundTrip=[this](UGameSave* Save,const FString& Label)->UGameSave*
    {
        TArray<uint8> Bytes;
        if(!TestTrue(Label+TEXT(" serializes through SaveGame"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return nullptr;
        auto* Loaded=Cast<UGameSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        TestNotNull(Label+TEXT(" deserializes through SaveGame"),Loaded);
        return Loaded;
    };
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    auto* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,false,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("Replay world"),W))return false;
    for(double Coverage:{0.,1.,.65})
    {
        const FString Label=FString::Printf(TEXT("Coverage %.2f"),Coverage);
        auto* Model=NewObject<UGeneratedWorld>();
        Model->GenerationSeed=12345;Model->PlanetSurfaceSeed=777;Model->AtmosphereColor=FLinearColor(2,7,40,0);
        Model->CloudSettings.CoverageScale=Coverage;Model->CloudSettings.WindScale=2.;
        FAPSPreviewBodyEditOverride A,B;
        A.CloudSettings=Model->CloudSettings;A.CloudSettings.SeedOffset=193;
        B.CloudSettings.CoverageScale=Coverage==1. ? .65 : 1.;B.CloudSettings.DensityScale=.3;B.CloudSettings.SeedOffset=731;
        Model->SetPreviewBodyEditOverride(TEXT("SYS0/S0/P0"),A);
        Model->SetPreviewBodyEditOverride(TEXT("SYS0/S0/P1/M0"),B);
        auto* Save=NewObject<UGameSave>();
        Save->GeneratedWorldsDataArray.Add(Model->SaveWorldData());
        if(!TestTrue(Label+TEXT(" reflected snapshot captured"),APSWorldSaveSnapshot::Capture(Model,Save->GeneratedWorldModelData)))continue;
        auto* Loaded=RoundTrip(Save,Label);
        if(!Loaded)continue;
        auto* Restored=APSWorldSaveSnapshot::Restore(Loaded,GetTransientPackage(),TEXT("CLOUD_SETTINGS_TEST"));
        if(!TestNotNull(Label+TEXT(" cloud snapshot restored"),Restored))continue;
        TestEqual(Label+TEXT(" editor coverage retained"),Restored->CloudSettings.CoverageScale,Coverage);
        TestEqual(Label+TEXT(" sky untouched"),Restored->AtmosphereColor,Model->AtmosphereColor);
        TestEqual(Label+TEXT(" terrain seed untouched"),Restored->PlanetSurfaceSeed,777);
        auto* Planet=W->SpawnActor<APlanet>();auto* Moon=W->SpawnActor<AMoon>();
        if(Planet&&Moon)
        {
            TestTrue(Label+TEXT(" planet edit replays"),AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Restored,TEXT("SYS0/S0/P0"),Planet));
            TestTrue(Label+TEXT(" moon edit replays"),AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Restored,TEXT("SYS0/S0/P1/M0"),Moon));
            TestEqual(Label+TEXT(" planet coverage survives actor recreation"),Planet->CloudSettings.CoverageScale,Coverage);
            TestEqual(Label+TEXT(" moon keeps independent coverage"),Moon->CloudSettings.CoverageScale,B.CloudSettings.CoverageScale);
            TestEqual(Label+TEXT(" planet weather seed survives"),Planet->CloudSettings.SeedOffset,193);
            TestEqual(Label+TEXT(" moon keeps independent weather seed"),Moon->CloudSettings.SeedOffset,731);
            TestEqual(Label+TEXT(" generation record retains coverage"),Planet->PlanetData.PlanetModelData.CloudSettings.CoverageScale,Coverage);
            TestEqual(Label+TEXT(" generation record retains wind"),Planet->PlanetData.PlanetModelData.CloudSettings.WindScale,2.);
        }
        else AddError(Label+TEXT(" replay actors unavailable"));
        // Exercise the external serializer again without the authoritative snapshot.
        Save->GeneratedWorldModelData.Reset();
        auto* LegacySave=RoundTrip(Save,Label+TEXT(" summary"));
        if(!LegacySave)continue;
        auto* Legacy=APSWorldSaveSnapshot::Restore(LegacySave,GetTransientPackage(),TEXT("CLOUD_SUMMARY_TEST"));
        if(TestNotNull(Label+TEXT(" summary restoration"),Legacy))
            TestEqual(Label+TEXT(" summary retains authored coverage"),Legacy->CloudSettings.CoverageScale,Coverage);
    }
    W->DestroyWorld(false);
    TestEqual(TEXT("New/default summary uses half coverage"),FGeneratedWorldData().CloudSettings.CoverageScale,.5);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCloudWeatherControlsTest,"APS.Gameplay.World.PlanetSurface.Clouds.Weather.Controls",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSCloudWeatherControlsTest::RunTest(const FString& Parameters)
{
    auto* Model=NewObject<UGeneratedWorld>();auto* VM=NewObject<UWorldGenerationViewModel>();
    TestEqual(TEXT("New model starts with half coverage"),Model->CloudSettings.CoverageScale,.5);
    VM->GeneratedWorld=Model;Model->PlanetSurfaceSeed=923;Model->AtmosphereOpacity=4.;Model->SurfaceReliefScale=1.7;
    VM->SetCloudParameter(EAPSCloudControl::Coverage,.4);
    VM->SetCloudParameter(EAPSCloudControl::Density,1.5);
    VM->SetCloudParameter(EAPSCloudControl::Scale,2.);
    VM->SetCloudParameter(EAPSCloudControl::Altitude,.7);
    VM->SetCloudParameter(EAPSCloudControl::Wind,0.);
    VM->SetCloudParameter(EAPSCloudControl::Storms,.2);
    VM->SetCloudParameter(EAPSCloudControl::Seed,345.);
    TestEqual(TEXT("Coverage slider binds model"),Model->CloudSettings.CoverageScale,.4);
    TestEqual(TEXT("Feature slider binds model"),Model->CloudSettings.FeatureScale,2.);
    TestEqual(TEXT("Seed slider binds model"),Model->CloudSettings.SeedOffset,345);
    TestEqual(TEXT("No terrain reroll"),Model->PlanetSurfaceSeed,923);
    TestEqual(TEXT("No sky opacity change"),Model->AtmosphereOpacity,4.);
    TestEqual(TEXT("No ground relief change"),Model->SurfaceReliefScale,1.7);
    VM->ResetCloudParameters();
    TestEqual(TEXT("Reset restores half coverage"),Model->CloudSettings.CoverageScale,.5);
    for(double Value:{Model->CloudSettings.DensityScale,Model->CloudSettings.FeatureScale,
        Model->CloudSettings.AltitudeScale,Model->CloudSettings.WindScale,Model->CloudSettings.StormScale})
        TestEqual(TEXT("Reset retains unit defaults for other multipliers"),Value,1.);
    TestEqual(TEXT("Reset restores zero weather seed offset"),Model->CloudSettings.SeedOffset,0);
    TestEqual(TEXT("Reset does not change terrain seed"),Model->PlanetSurfaceSeed,923);
    return true;
}
#endif
