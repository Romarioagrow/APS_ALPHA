#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitRecipeShapeTest, "APS.Preview.Orbits.RecognizableBoundedRecipes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSOrbitRecipeShapeTest::RunTest(const FString& Parameters)
{
    const EOrbitDistributionType Types[] = {EOrbitDistributionType::Uniform, EOrbitDistributionType::Gaussian,
        EOrbitDistributionType::Chaotic, EOrbitDistributionType::InnerOuter, EOrbitDistributionType::Dense};
    for (int32 Seed = 0; Seed < 32; ++Seed)
    {
        TArray<TArray<double>> Layouts;
        for (auto Type : Types)
        {
            FRandomStream Random(Seed);
            auto Radii = UPlanetarySystemGenerator::BuildPlanetOrbitLayout(7, Type, 1.0, 8.0, 50.0, Random);
            FRandomStream Repeat(Seed);
            TestTrue(TEXT("Repeat generation is deterministic"), Radii == UPlanetarySystemGenerator::BuildPlanetOrbitLayout(7, Type, 1.0, 8.0, 50.0, Repeat));
            for (const auto& Previous : Layouts) TestTrue(TEXT("Every recipe has a distinct radial layout"), Radii != Previous);
            Layouts.Add(Radii);
        }
        for (int32 I = 2; I < 7; ++I) TestTrue(TEXT("Uniform gaps are equal"),
            FMath::IsNearlyEqual(Layouts[0][I] - Layouts[0][I-1], Layouts[0][1] - Layouts[0][0], 1.e-10));
        TestTrue(TEXT("Gaussian packs the centre more than its tails"), Layouts[1][1]-Layouts[1][0] > 2.0*(Layouts[1][3]-Layouts[1][2]));
        TestTrue(TEXT("InnerOuter retains a wide empty middle"), Layouts[3][4]-Layouts[3][3] > 7.0*0.4);
        TestTrue(TEXT("Dense is a compact inner group"), Layouts[4].Last() < 1.0+7.0*0.35);
    }
    for (auto Type : Types) for (int32 Count : {0,1,2,3,7,32,120}) for (double Radius : {1.0,50.0,2000.0})
    {
        FRandomStream Random(123);
        auto Radii = UPlanetarySystemGenerator::BuildPlanetOrbitLayout(Count, Type, 1.0, 8.0, Radius, Random);
        TestEqual(TEXT("No lost/extra planets"), Radii.Num(), Count);
        double Min = 1.0, Max = 8.0;
        UPlanetarySystemGenerator::ResolvePlanetOrbitRange(Min, Max, Radius);
        for (int32 I=0; I<Radii.Num(); ++I)
        {
            TestTrue(TEXT("Giant clearance is resolved before sampling and never explodes"), FMath::IsFinite(Radii[I]) && Radii[I]>=Min-1.e-9 && Radii[I]<=Max+1.e-9 && Radii[I]<20.0);
            if (I) TestTrue(TEXT("Distinct ascending radii"), Radii[I]>Radii[I-1]);
        }
    }
    FRandomStream UniformPair(123), ChaoticPair(123);
    TestTrue(TEXT("Chaotic does not alias Uniform even for two planets"),
        UPlanetarySystemGenerator::BuildPlanetOrbitLayout(2,EOrbitDistributionType::Uniform,1.0,8.0,1.0,UniformPair)
        != UPlanetarySystemGenerator::BuildPlanetOrbitLayout(2,EOrbitDistributionType::Chaotic,1.0,8.0,1.0,ChaoticPair));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitEditEnvelopeTest, "APS.Preview.Orbits.SwitchingRetainsEnvelopeNotOldLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSOrbitEditEnvelopeTest::RunTest(const FString& Parameters)
{
    UGeneratedWorld* World = NewObject<UGeneratedWorld>();
    FAPSPreviewStarEditOverride Star;
    Star.CapturePlanetOrbitLayout({1.0,3.0,8.0}, EOrbitDistributionType::Uniform, 1.0, 8.0);
    World->SetPreviewStarEditOverride(TEXT("SYS0/S0"), Star);
    World->SetPreviewStarEditOverride(TEXT("SYS-other/S0"), Star);
    FAPSPreviewSystemEditOverride Edit;
    Edit.bOverrideOrbitDistribution = true;
    for (int32 I=0; I<50; ++I)
    {
        Edit.OrbitDistribution = static_cast<EOrbitDistributionType>(I%5);
        World->SetPreviewSystemEditOverride(TEXT("SYS0"), Edit);
        Star = *World->FindPreviewStarEditOverride(TEXT("SYS0/S0"));
        TestFalse(TEXT("Explicit distribution edit disables exact old-layout replay"), Star.bReplayPlanetOrbitLayout);
        FRandomStream Random(5);
        double Min=400.0, Max=4000.0;
        TestTrue(TEXT("Sampling envelope always available"), Star.TryGetPlanetOrbitRangeAu(Min,Max));
        TestEqual(TEXT("No inward range contraction after Dense"), Min, 1.0);
        TestEqual(TEXT("No giant mass multiplier or cumulative growth"), Max, 8.0);
        const auto Radii = UPlanetarySystemGenerator::BuildPlanetOrbitLayout(7,Edit.OrbitDistribution,Min,Max,50.0,Random);
        Star.CapturePlanetOrbitLayout(Radii,Edit.OrbitDistribution,Min,Max);
        World->SetPreviewStarEditOverride(TEXT("SYS0/S0"), Star);
    }
    TestTrue(TEXT("Another system's accepted snapshot is untouched"), World->FindPreviewStarEditOverride(TEXT("SYS-other/S0"))->bReplayPlanetOrbitLayout);
    FAPSPreviewStarEditOverride OldCollapsed;
    OldCollapsed.PlanetOrbitRadiiAu={5.219,5.237,5.254,5.272,5.289,5.307,5.325};
    double Min=400.0,Max=4000.0;
    TestTrue(TEXT("Old belt snapshot can recover"), OldCollapsed.TryGetPlanetOrbitRangeAu(Min,Max));
    TestEqual(TEXT("Old snapshot repair NEVER increases outer boundary"),Max,5.325);
    TestTrue(TEXT("Old belt gains inward space for distinct recipes"),Min<1.0);
    UGeneratedWorld* Copy=DuplicateObject<UGeneratedWorld>(World,GetTransientPackage());
    TestEqual(TEXT("Envelope and replay policy survive reflected transport"),Copy->GetPreviewStarEditHash(),World->GetPreviewStarEditHash());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitInclinationTest, "APS.Preview.Orbits.InclinationLimitAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSOrbitInclinationTest::RunTest(const FString& Parameters)
{
    for (double Limit : {0.0,1.0,8.0,30.0,90.0}) for (int32 Seed=0; Seed<128; ++Seed)
    {
        FRandomStream Random(Seed), Repeat(Seed);
        const FRotator Rotation=UPlanetarySystemGenerator::SamplePlanetOrbitRotation(Random,Limit);
        TestTrue(TEXT("Plane orientation is deterministic"),Rotation.Equals(UPlanetarySystemGenerator::SamplePlanetOrbitRotation(Repeat,Limit),1.e-10));
        const double Actual=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Rotation.Quaternion().GetAxisZ().Z,-1.0,1.0)));
        TestTrue(TEXT("Actual orbital PLANE obeys limit, not just Euler component"),Actual<=Limit+1.e-6);
        if (Limit==0.0) TestTrue(TEXT("Zero means truly flat"),FMath::Abs(Rotation.Quaternion().GetAxisX().Z)<1.e-12);
    }
    UGeneratedWorld* World=NewObject<UGeneratedWorld>();
    FAPSPreviewSystemEditOverride Edit;
    Edit.bOverrideOrbitInclination=true;
    Edit.MaxOrbitInclinationDegrees=0.0;
    World->SetPreviewSystemEditOverride(TEXT("SYS0"),Edit);
    const uint32 FlatHash=World->GetPreviewSystemEditHash();
    Edit.MaxOrbitInclinationDegrees=80.0;
    World->SetPreviewSystemEditOverride(TEXT("SYS0"),Edit);
    TestTrue(TEXT("Angle invalidates preview cache"),FlatHash!=World->GetPreviewSystemEditHash());
    TestEqual(TEXT("Other systems remain unchanged"),World->GetSystemMaxOrbitInclinationDegrees(TEXT("SYS-other")),8.0);
    auto* Copy=DuplicateObject<UGeneratedWorld>(World,GetTransientPackage());
    TestEqual(TEXT("Angle survives menu/gameplay transport"),Copy->GetSystemMaxOrbitInclinationDegrees(TEXT("SYS0")),80.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCompactOrbitTest, "APS.Preview.Orbits.CompactAutomaticEnvelope",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCompactOrbitTest::RunTest(const FString& Parameters)
{
    auto* Generator = NewObject<UPlanetarySystemGenerator>();
    auto* Planets = NewObject<UPlanetGenerator>();
    auto* Moons = NewObject<UMoonGenerator>();
    FAPSPreviewStarEditOverride Retained;
    for (double Radius : {1.0,50.0,2000.0}) for (double Mass : {1.0,40.0,400.0})
    for (int32 Type = 0; Type < 5; ++Type) for (int32 Count : {1,2,7,32,120})
    {
        Generator->SetGenerationSeed(271828);
        auto Star = MakeShared<FStarModel>();
        Star->StellarType = EStellarType::HyperGiant;
        Star->SpectralClass = ESpectralClass::G;
        Star->Mass = Mass; Star->Radius = Radius; Star->RadiusKM = Radius * 695700.0;
        Star->Luminosity = 100.0; Star->SurfaceTemperature = 5772;
        auto Family = MakeShared<FPlanetarySystemModel>();
        Family->AmountOfPlanets = Count;
        Family->PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
        Family->OrbitDistributionType = static_cast<EOrbitDistributionType>(Type);
        Generator->GenerateCustomPlanetarySystemModel(Family,Star,Planets,Moons,nullptr,true);
        TestEqual(TEXT("Compact mode retains planet count"), Family->PlanetsList.Num(),Count);
        for (const auto& Data : Family->PlanetsList)
            TestTrue(TEXT("Even unedited hypergiant mass never expands the system to thousands of AU"),
                Data->OrbitRadius >= UGeneratedWorld::MinimumPlanetOrbitAu(Radius,0.0) && Data->OrbitRadius < 32.0);
    }
    double Min=1.0,Max=10.0;
    UPlanetarySystemGenerator::CompactPlanetOrbitRange(Min,Max,1.0);
    TestTrue(TEXT("Ordinary AUTO system is approximately 25% smaller"),Max>7.4 && Max<7.6);
    Retained.CapturePlanetOrbitLayout({Min,(Min+Max)*0.5,Max},EOrbitDistributionType::Uniform,Min,Max);
    Retained.bCompactOrbitEnvelope=true;
    for (int32 Cycle=0; Cycle<50; ++Cycle)
    {
        double ReadMin=400.0,ReadMax=4000.0;
        Retained.TryGetPlanetOrbitRangeAu(ReadMin,ReadMax);
        if (!Retained.bCompactOrbitEnvelope) UPlanetarySystemGenerator::CompactPlanetOrbitRange(ReadMin,ReadMax,1.0);
        TestEqual(TEXT("Repeated stellar editing does not compound compactness"),ReadMax,Max);
        Retained.CapturePlanetOrbitLayout({ReadMin,ReadMax},EOrbitDistributionType::Dense,ReadMin,ReadMax);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSManualOrbitTest, "APS.Preview.Orbits.ManualDistanceInclinationAndSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSManualOrbitTest::RunTest(const FString& Parameters)
{
    auto* World=NewObject<UGeneratedWorld>();
    FAPSPreviewPlanetOrbitEdit Manual;
    Manual.bOverrideDistance=true; Manual.DistanceAu=6.0;
    Manual.bOverrideInclination=true; Manual.InclinationDegrees=37.0;
    World->SetPlanetOrbitEdit(TEXT("SYS0/S0/P0"),Manual);
    const uint32 Hash=World->GetPreviewSystemEditHash();
    for (int32 Type=0; Type<5; ++Type)
    {
        FRandomStream Random(73);
        const auto Radii=UPlanetarySystemGenerator::BuildPlanetOrbitLayout(7,static_cast<EOrbitDistributionType>(Type),0.75,7.5,1.0,Random);
        FPlanetarySystemModel Family;
        for (double Radius : Radii)
        {
            auto Data=MakeShared<FPlanetData>(); Data->PlanetModel=MakeShared<FPlanetModel>();
            Data->PlanetModel->Radius=1.0; Data->PlanetModel->RadiusKM=6371.0;
            Data->OrbitRadius=Data->PlanetModel->OrbitDistance=Radius;
            Family.PlanetsList.Add(Data);
        }
        World->ApplyPlanetOrbitEdits(Family,TEXT("SYS0/S0"),1.0);
        TestEqual(TEXT("Manual distance wins after recipe changes"),Family.PlanetsList[0]->OrbitRadius,6.0);
        for (int32 I=1; I<7; ++I) TestEqual(TEXT("Moving P0 outside P1 does not fling all later planets outward"),Family.PlanetsList[I]->OrbitRadius,Radii[I]);
        TestEqual(TEXT("Serializable body model has manual AU value"),Family.PlanetsList[0]->PlanetModelData.OrbitDistance,6.0);
        const auto Auto=UPlanetarySystemGenerator::SamplePlanetOrbitRotation(Random,8.0);
        const auto Rotation=World->ResolvePlanetOrbitRotation(TEXT("SYS0/S0/P0"),Auto);
        const double Actual=FMath::RadiansToDegrees(FMath::Acos(Rotation.Quaternion().GetAxisZ().Z));
        TestTrue(TEXT("Manual tilt is exact, not a random maximum"),FMath::IsNearlyEqual(Actual,37.0,1.e-6));
        TestTrue(TEXT("Other planets retain their automatic plane"),World->ResolvePlanetOrbitRotation(TEXT("SYS0/S0/P1"),Auto).Equals(Auto));
        World->ApplyPlanetOrbitEdits(Family,TEXT("SYS0/S0"),2000.0);
        TestTrue(TEXT("Enlarged star cannot engulf a manual orbit"),Family.PlanetsList[0]->OrbitRadius>=UGeneratedWorld::MinimumPlanetOrbitAu(2000.0,6371.0));
    }
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Out(Writer,false); Out.ArNoDelta=true; World->Serialize(Out); Out.Close();
    auto* Restored=NewObject<UGeneratedWorld>();
    FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive In(Reader,true); In.ArNoDelta=true; Restored->Serialize(In); In.Close();
    const auto* Saved=Restored->FindPlanetOrbitEdit(TEXT("SYS0/S0/P0"));
    TestTrue(TEXT("Both manual controls survive tagged save/load"),Saved && Saved->DistanceAu==6.0 && Saved->InclinationDegrees==37.0 && Saved->bOverrideDistance && Saved->bOverrideInclination);
    TestEqual(TEXT("Manual edits invalidate and persist manifest hash"),Restored->GetPreviewSystemEditHash(),Hash);
    Manual.InclinationDegrees=0.0;
    Restored->SetPlanetOrbitEdit(TEXT("SYS0/S0/P0"),Manual);
    TestTrue(TEXT("Zero manual tilt means truly flat"),FMath::Abs(Restored->ResolvePlanetOrbitRotation(TEXT("SYS0/S0/P0"),FRotator(14,23,7)).Quaternion().GetAxisZ().Z-1.0)<1.e-10);
    FAPSPreviewStarEditOverride StarEdit; StarEdit.bReplayPlanetOrbitLayout=true;
    Restored->SetPreviewStarEditOverride(TEXT("SYS0/S0"),StarEdit);
    Restored->ResetPlanetOrbitEdit(TEXT("SYS0/S0/P0"));
    TestNull(TEXT("AUTO removes both overrides"),Restored->FindPlanetOrbitEdit(TEXT("SYS0/S0/P0")));
    TestFalse(TEXT("AUTO cannot replay a captured manual distance"),Restored->FindPreviewStarEditOverride(TEXT("SYS0/S0"))->bReplayPlanetOrbitLayout);
    return true;
}
#endif
