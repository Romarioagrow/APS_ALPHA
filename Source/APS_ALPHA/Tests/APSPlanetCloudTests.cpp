#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetCloudPolicy.h"
#include "APSCloudFlightProbe.h"
#include <limits>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudPolicyTest,"APS.Gameplay.World.PlanetSurface.Clouds.Policy",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudPolicyTest::RunTest(const FString& Parameters)
{
    using namespace APSPlanetCloudPolicy;
    FAPSResolvedPlanetSurfaceProfile P;
    P.PlanetType=EPlanetType::Terrestrial; P.LiquidType=EAPSPlanetLiquidType::Water;
    P.Humidity=.5; P.Temperature=.45; P.AtmosphericPressure=1; P.BiomeSeed=137;
    auto L=Resolve(P,80,false);
    TestTrue(TEXT("Eligible wet atmosphere"),L.Enabled);
    TestTrue(TEXT("Finite layer contained in atmosphere"),L.BottomKm>0 && L.ThicknessKm>0 && L.BottomKm+L.ThicknessKm<80);
    TestEqual(TEXT("Repeatable phase"),L.Offset,Resolve(P,80,false).Offset);
    ++P.BiomeSeed; TestFalse(TEXT("Different phase per seed"),L.Offset.Equals(Resolve(P,80,false).Offset));
    TestFalse(TEXT("Manual untouched"),Resolve(P,80,true).Enabled);
    for(double H:{0.,2.,11.99,std::numeric_limits<double>::quiet_NaN()})
        TestFalse(TEXT("No valid cloud shell"),Resolve(P,H,false).Enabled);
    for(int32 Type=0;Type<=APSPlanetTypes::LastValue;++Type)
    {
        P.PlanetType=EPlanetType(Type);
        const bool Allowed=P.PlanetType==EPlanetType::Terrestrial || P.PlanetType==EPlanetType::Oasis || P.PlanetType==EPlanetType::Water;
        TestEqual(TEXT("Narrow water-condensate rollout"),Resolve(P,80,false).Enabled,Allowed);
    }
    P.PlanetType=EPlanetType::Terrestrial;
    for(float* Value:{&P.Humidity,&P.Temperature,&P.AtmosphericPressure})
    {
        const float Old=*Value;
        *Value=0; TestFalse(TEXT("Reject vacuum/dry/frozen"),Resolve(P,80,false).Enabled);
        *Value=std::numeric_limits<float>::quiet_NaN(); TestFalse(TEXT("Reject nonfinite climate"),Resolve(P,80,false).Enabled);
        *Value=Old;
    }
    P.Temperature=.99; TestFalse(TEXT("Reject extreme heat"),Resolve(P,80,false).Enabled);
    TestEqual(TEXT("Ground visible"),Visibility(6750,6751),1.f);
    TestEqual(TEXT("Subpixel body omitted"),Visibility(6750,6750/.0004),0.f);
    TestEqual(TEXT("NaN rejected"),Visibility(6750,std::numeric_limits<double>::quiet_NaN()),0.f);
    const FVector PhysicalBody(6.e13,2.e13,-7.e12), Star=PhysicalBody+FVector(1.e10,2.e10,3.e10);
    const FQuat Rotation=FRotator(23,64,-11).Quaternion();
    const FVector Sun=SunDirection(Star,PhysicalBody,Rotation);
    TestTrue(TEXT("Star direction uses physical body frame"),Sun.Equals(Rotation.UnrotateVector(FVector(1,2,3).GetSafeNormal()),1.e-9));
    const FVector Rebase(8.e10,-6.e10,3.e10);
    TestTrue(TEXT("Common origin rebase preserves cloud lighting"),Sun.Equals(SunDirection(Star-Rebase,PhysicalBody-Rebase,Rotation),1.e-9));
    TestTrue(TEXT("Coincident star does not make a NaN sun"),SunDirection(Star,Star,Rotation).IsNearlyZero());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudDepthTest,"APS.Gameplay.World.PlanetSurface.Clouds.PerspectiveDepth",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudDepthTest::RunTest(const FString& Parameters)
{
    // Reproduce UE's reciprocal safety bias. This is the projection algebra,
    // not rendered acceptance; the GPU readout and visible route cover that.
    constexpr float Near=10.f;
    for(double Scale:{1.,.011315417037,.0001})
        for(double Km:{.002,6.,100.,2000.,20000.})
        {
            const float CmPerKm=float(100000.*Scale), Z=float(Km)*CmPerKm;
            const float DeviceZ=Near/Z;
            const float Exact=Near/DeviceZ;
            TestTrue(TEXT("Raw projection preserves physical depth"),FMath::Abs(Exact-Z)<FMath::Max(.0001f,Z*1.e-6f));
            const float Guarded=1.f/(DeviceZ/Near+1.e-8f);
            TestTrue(TEXT("Guarded depth never further than the surface"),Guarded<=Exact*1.000001f);
        }
    const float PreviewCmPerKm=float(100000.*.011315417037);
    const float GuardedKm=1.f/(1.f/(2000.f*PreviewCmPerKm)+1.e-8f)/PreviewCmPerKm;
    TestTrue(TEXT("Observed 44km preview bias explained"),FMath::IsNearlyEqual(GuardedKm,1955.74f,.01f));
    TestTrue(TEXT("Guarded depth would hide a 6-8km layer"),GuardedKm<1992.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudMarchTest,"APS.Gameplay.World.PlanetSurface.Clouds.NearWeightedMarch",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudMarchTest::RunTest(const FString& Parameters)
{
    // Float mirror of the shader's quadrature, not visual acceptance. Covers
    // interval conservation, bounded work and the near-field grazing regression.
    for(float Thickness:{1.2f,1.812f,3.f}) for(float Length:{.00001f,.001f,.1f,1.812f,10.f,107.f,214.f,2000.f})
    {
        const int32 Samples=FMath::Clamp(FMath::CeilToInt(Length/Thickness*16.f),16,32);
        const float Scale=FMath::Max(Thickness*.5f,.05f);
        float Growth=FMath::Pow(1.f+Length/Scale,1.f/float(Samples));
        float NextStep=Scale*(Growth-1.f);
        if(Growth<1.00001f){Growth=1.f;NextStep=Length/float(Samples);}
        const float FirstStep=NextStep;
        float Start=0,PreviousSample=-1;
        for(int32 I=0;I<Samples;++I)
        {
            const float Step=FMath::Max(0.f,FMath::Min(NextStep,Length-Start));
            const float At=Start+.5f*Step;
            TestTrue(TEXT("Finite ordered in-segment samples"),FMath::IsFinite(At)&&At>=PreviousSample&&At<=Length&&Step>=0);
            PreviousSample=At;Start+=Step;NextStep*=Growth;
        }
        TestTrue(TEXT("Full integration length conserved"),FMath::Abs(Start-Length)<=FMath::Max(.00001f,Length*.003f));
        if(Length>=107.f && Length<=214.f)
            TestTrue(TEXT("Grazing near field stays below quarter-layer footprint"),FirstStep<Thickness*.25f);
    }
    const auto DetailWeight=[](float Step)
    { const float T=FMath::Clamp((Step*.5f*.32f-.35f)/.85f,0.f,1.f);return 1.f-T*T*(3.f-2.f*T); };
    TestTrue(TEXT("Old214km ray erased almost all near detail"),DetailWeight(214.f/32.f)<.07f);
    TestTrue(TEXT("New first interval retains near billows"),DetailWeight(.169f)>.999f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudDensityFilterTest,"APS.Gameplay.World.PlanetSurface.Clouds.DensityFilter",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudDensityFilterTest::RunTest(const FString& Parameters)
{
    // Float mirror of the shader's Gaussian moment filter. This algebra test
    // cannot accept the rendered sky; the Monte Carlo audit checks its model.
    const auto CDF=[](float Z,float Gaussian)
    {
        const float T=1.f/(1.f+.2316419f*FMath::Abs(Z));
        const float Tail=.3989422804f*Gaussian*T*(.319381530f+T*(-.356563782f
            +T*(1.781477937f+T*(-1.821255978f+T*1.330274429f))));
        return Z>=0 ? 1.f-Tail : Tail;
    };
    const auto Expected=[&](float Mean,float Sigma,float Threshold)
    {
        if(Sigma<.001f)return FMath::Clamp((Mean-Threshold)*7.f,0.f,1.f);
        const float A=(Threshold-Mean)/Sigma,B=(Threshold+1.f/7.f-Mean)/Sigma;
        const float EA=FMath::Exp(-.5f*A*A),EB=FMath::Exp(-.5f*B*B),CA=CDF(A,EA),CB=CDF(B,EB);
        return FMath::Clamp(7.f*((Mean-Threshold)*(CB-CA)+Sigma*.3989422804f*(EA-EB))+1.f-CB,0.f,1.f);
    };
    const float FullSigma=.135f*FMath::Sqrt(.72f*.72f+.28f*.28f);
    TestTrue(TEXT("Retains measured .60 density tail"),FMath::IsNearlyEqual(Expected(.5f,FullSigma,.6f),.0647f,.01f));
    TestTrue(TEXT("Retains measured .68 density tail"),FMath::IsNearlyEqual(Expected(.5f,FullSigma,.68f),.0103f,.005f));
    for(float Mean:{.2f,.5f,.8f}) for(float Sigma:{0.f,.0005f,.005f,.04f,FullSigma})
    {
        float Previous=1;
        for(int I=-10;I<=110;++I)
        {
            const float Threshold=I*.01f,Value=Expected(Mean,Sigma,Threshold);
            TestTrue(TEXT("Finite bounded density monotone in threshold"),FMath::IsFinite(Value)&&Value>=0&&Value<=1&&Value<=Previous+.00001f);
            if(Sigma<.001f)TestEqual(TEXT("Fully resolved field unchanged"),Value,FMath::Clamp((Mean-Threshold)*7.f,0.f,1.f));
            Previous=Value;
        }
    }
    const float Left=Expected(.5f,FullSigma,.5999f),Right=Expected(.5f,FullSigma,.6001f);
    TestTrue(TEXT("No hard .60 cutoff"),Right>.05f && FMath::Abs(Left-Right)<.001f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudAerialOcclusionTest,"APS.Gameplay.World.PlanetSurface.Clouds.AerialOcclusion",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudAerialOcclusionTest::RunTest(const FString& Parameters)
{
    // Front air changes cloud radiance, never coverage. This float mirror is
    // a compositing regression, not an atmosphere model or rendered acceptance.
    for(float OpticalDepth:{0.f,.01f,.5f,2.f,4.f}) for(float Air:{0.f,.02f,.2f,1.f})
        for(int32 Samples:{1,16,32})
        {
            const float Extinction=1.f-FMath::Exp(-OpticalDepth/Samples);
            float Trans=1.f,Alpha=0.f,Sum=0.f;
            for(int32 I=0;I<Samples;++I)
            {
                const float Contribution=Trans*Extinction;
                Sum+=Contribution*.7f*Air; Alpha+=Contribution; Trans*=1.f-Extinction;
            }
            const float Expected=1.f-FMath::Exp(-OpticalDepth);
            TestTrue(TEXT("Cloud opacity independent of air depth"),FMath::IsNearlyEqual(Alpha,Expected,.00001f));
            TestTrue(TEXT("Radiance still attenuated by front air"),FMath::IsNearlyEqual(Sum,.7f*Air*Expected,.00001f));
            TestTrue(TEXT("Cloud energy closes"),FMath::IsNearlyEqual(Alpha+Trans,1.f,.00001f));
        }
    const float Opaque=.999f, Air=.2f;
    TestTrue(TEXT("V20 incorrectly exposed background"),1.f-Opaque*Air>.8f);
    TestTrue(TEXT("V21 leaves only physical cloud transmission"),1.f-Opaque<.00101f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudForegroundAirTest,"APS.Gameplay.World.PlanetSurface.Clouds.ForegroundAir",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudForegroundAirTest::RunTest(const FString& Parameters)
{
    // Transfer algebra and unit conversion, not acceptance of the sky image.
    for(double Alpha:{0.,.01,.3,.8,1.}) for(double AirT:{0.,.02,.5,1.})
        for(double Front:{0.,.03,.8})
        {
            const double Background=Front+AirT*.2;
            const double Cloud=Front+AirT*.7;
            const double Composite=Background*(1.-Alpha)+Cloud*Alpha;
            TestTrue(TEXT("Foreground air survives once regardless of cloud opacity"),
                FMath::IsNearlyEqual(Composite,Front+AirT*(.2*(1.-Alpha)+.7*Alpha),1.e-12));
        }
    for(double Radius:{6370.,6750.,10000.}) for(double Scale:{1.,.011315417037,.0001})
    {
        const double DisplayedCoefficient=33.1*.05/(Radius*2000.*Scale);
        const double PerKm=DisplayedCoefficient*100000.*Scale;
        TestTrue(TEXT("Sky optical coefficients invariant under presentation scale"),
            FMath::IsNearlyEqual(PerKm,33.1*2.5/Radius,1.e-12));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudAirUnitsTest,"APS.Gameplay.World.PlanetSurface.Clouds.AirColorUnits",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudAirUnitsTest::RunTest(const FString& Parameters)
{
    // Exercise the production reader, not a separate unit-conversion mirror.
    for(float Physical:{.0001f,.002f,.061f,1.f,33.1f})
        for(double Scale:{1.,.011315417037,.00001,1.e-9,1.e-10})
        {
            const float Factor=float(100000.*Scale);
            const FLinearColor Displayed(Physical/Factor,Physical*.5f/Factor,Physical*.1f/Factor,0);
            FLinearColor Decoded;
            TestTrue(TEXT("Finite physical coefficients accepted"),APSPlanetCloudPolicy::ReadAirColor(Displayed,Factor,Decoded));
            TestTrue(TEXT("Preview and gameplay recover the same physical RGB"),
                Decoded.Equals(FLinearColor(Physical,Physical*.5f,Physical*.1f,0),FMath::Max(1.e-8f,Physical*1.e-6f)));
        }
    const FLinearColor Fallback(.1f,.2f,.3f,0);
    FLinearColor Result=Fallback;
    TestFalse(TEXT("Non-finite input rejected"),APSPlanetCloudPolicy::ReadAirColor(
        FLinearColor(std::numeric_limits<float>::quiet_NaN(),1,1,0),1,Result));
    TestEqual(TEXT("Invalid value preserves fallback"),Result,Fallback);
    TestFalse(TEXT("Overflow rejected"),APSPlanetCloudPolicy::ReadAirColor(
        FLinearColor(std::numeric_limits<float>::max(),1,1,0),2,Result));
    TestEqual(TEXT("Overflow preserves fallback"),Result,Fallback);
    TestFalse(TEXT("Invalid scale rejected"),APSPlanetCloudPolicy::ReadAirColor(FLinearColor::White,-1,Result));
    TestTrue(TEXT("Negative channels clamp to zero"),APSPlanetCloudPolicy::ReadAirColor(FLinearColor(-1,.5f,1,0),2,Result));
    TestEqual(TEXT("Positive channels stay linear"),Result,FLinearColor(0,1,2,0));
    TestTrue(TEXT("Old reader lost more than80 percent at scale1e-9"),FMath::Clamp(610.f,0.f,100.f)*.0001f < .061f*.2f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCloudGroundProbeTest,"APS.Gameplay.World.PlanetSurface.Clouds.GroundProbeFloor",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCloudGroundProbeTest::RunTest(const FString& Parameters)
{
    using APSCloudFlightProbe::GroundSeaAltitudeKm;
    TestEqual(TEXT("Dry terrain uses two metre eye height"),GroundSeaAltitudeKm(100000.,900000.,false),1.002);
    TestEqual(TEXT("Ocean hides a submerged seabed"),GroundSeaAltitudeKm(-400000.,0.,true),.002);
    TestEqual(TEXT("Island stays above the water"),GroundSeaAltitudeKm(200000.,0.,true),2.002);
    TestEqual(TEXT("Nonzero sea level is respected"),GroundSeaAltitudeKm(100000.,300000.,true),3.002);
    const FVector Direction=APSCloudFlightProbe::GroundViewOutward(FVector::UpVector,FVector::ForwardVector,637000000.);
    TestTrue(TEXT("Ground viewpoint remains on a unit sphere"),FMath::IsNearlyEqual(Direction.Size(),1.,1.e-12));
    TestTrue(TEXT("Only diagnostic viewpoint moves five km from the covered spawn"),FMath::IsNearlyEqual(FMath::Atan2(Direction.X,Direction.Z)*637000000.,500000.,1.));
    return true;
}
#endif
