#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

namespace APSPlanetaryGenerationDeterminism
{
FString SnapshotStar(const FStarModel& Star)
{
	return FString::Printf(TEXT("%d,%d,%d,%d,%.17g,%.17g,%.17g,%.17g,%d,%s;"),
		static_cast<int32>(Star.StellarType), static_cast<int32>(Star.SpectralClass),
		static_cast<int32>(Star.SpectralType), Star.SpectralSubclass, Star.Mass,
		Star.Radius, Star.RadiusKM, Star.Luminosity, Star.SurfaceTemperature, *Star.Age);
}

FString SnapshotMoons(const FPlanetModel& Planet)
{
	FString Result;
	for (const TSharedPtr<FMoonData>& Data : Planet.MoonsList)
	{
		if (!Data || !Data->MoonModel) { Result += TEXT("invalid;"); continue; }
		const FMoonModel& Moon = *Data->MoonModel;
		Result += FString::Printf(TEXT("%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g;"),
			static_cast<int32>(Moon.Type), static_cast<int32>(Moon.PlanetType),
			Data->OrbitRadius, Moon.Mass, Moon.RadiusKM, Moon.MoonDensity,
			Moon.MoonGravity, Moon.MoonAtmosphereHeight);
	}
	return Result;
}

FString SnapshotSystem(const FPlanetarySystemModel& System)
{
	FString Result = FString::Printf(TEXT("%d/%d;"), System.AmountOfPlanets, static_cast<int32>(System.OrbitDistributionType));
	for (const TSharedPtr<FPlanetData>& Data : System.PlanetsList)
	{
		if (!Data || !Data->PlanetModel) { Result += TEXT("invalid;"); continue; }
		const FPlanetModel& Planet = *Data->PlanetModel;
		Result += FString::Printf(TEXT("%d,%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,[%s];"),
			static_cast<int32>(Planet.PlanetType), static_cast<int32>(Planet.PlanetZone),
			Data->OrbitRadius, Planet.Mass, Planet.RadiusKM, Planet.PlanetDensity,
			Planet.Temperature, Planet.AtmosphereHeight, *SnapshotMoons(Planet));
	}
	return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetaryGenerationDeterminismTest,
	"APS.Generation.Coherence.OwnedPlanetaryRandom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetaryGenerationDeterminismTest::RunTest(const FString& Parameters)
{
	UPlanetarySystemGenerator* Generator = NewObject<UPlanetarySystemGenerator>();
	UPlanetGenerator* Planets = NewObject<UPlanetGenerator>();
	UMoonGenerator* Moons = NewObject<UMoonGenerator>();
	const auto Generate = [&](int32 Seed, int32 Count, bool bRandomType)
	{
		Generator->SetGenerationSeed(Seed);
		TSharedPtr<FStarModel> Star = MakeShared<FStarModel>();
		Star->StellarType = EStellarType::MainSequence;
		Star->Mass = 1.0;
		Star->Radius = 1.0;
		Star->RadiusKM = 695700.0;
		Star->Luminosity = 1.0;
		Star->SurfaceTemperature = 5772;
		TSharedPtr<FPlanetarySystemModel> System = MakeShared<FPlanetarySystemModel>();
		System->AmountOfPlanets = Count;
		System->PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
		System->OrbitDistributionType = EOrbitDistributionType::Gaussian;
		if (bRandomType) Generator->GeneratePlanetarySystemModelByStar(System, Star, Planets, Moons);
		else Generator->GenerateCustomPlanetarySystemModel(System, Star, Planets, Moons);
		return APSPlanetaryGenerationDeterminism::SnapshotSystem(*System);
	};
	for (const bool bRandomType : { false, true })
	{
		const FString First = Generate(271828, 6, bRandomType);
		for (int32 Index = 0; Index < 1000; ++Index) FMath::Rand();
		Generate(42, 9, bRandomType);
		Generate(88117, 3, bRandomType);
		TestEqual(TEXT("Unrelated global random draws and intervening systems do not alter canonical physical models"),
			Generate(271828, 6, bRandomType), First);
		TestNotEqual(TEXT("A different model seed still produces a different world"), Generate(271829, 6, bRandomType), First);
		TestFalse(TEXT("Generated physical models contain no invalid model entries"), First.Contains(TEXT("invalid")));
	}
	const auto GenerateEditedFamily = [&](int32 Count)
	{
		TSharedPtr<FPlanetModel> Planet = MakeShared<FPlanetModel>();
		Planet->PlanetType = EPlanetType::Ocean;
		Planet->PlanetZone = EPlanetaryZoneType::HabitableZone;
		Planet->Radius = 6750.0 / 6371.0;
		Planet->RadiusKM = 6750.0;
		Generator->GeneratePlanetMoonsList(Planets, Moons, Planet, Planet->Radius, Count, 0);
		return APSPlanetaryGenerationDeterminism::SnapshotMoons(*Planet);
	};
	Generator->SetGenerationSeed(271828);
	const FString FirstFamily = GenerateEditedFamily(2);
	Generate(271828, 9, false);
	GenerateEditedFamily(7);
	TestEqual(TEXT("An unchanged edited moon family ignores sibling count and previous family draws"), GenerateEditedFamily(2), FirstFamily);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStellarGenerationDeterminismTest,
	"APS.Generation.Coherence.OwnedStellarRandom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarGenerationDeterminismTest::RunTest(const FString& Parameters)
{
	UStarGenerator* Generator = NewObject<UStarGenerator>();
	const auto Generate = [&](int32 Seed, EStellarType Type, ESpectralClass Spectrum)
	{
		Generator->SetGenerationSeed(Seed);
		TSharedPtr<FStarModel> Star = MakeShared<FStarModel>();
		Star->StellarType = Type;
		Star->SpectralClass = Spectrum;
		Generator->GenerateStarModel(Star);
		TestTrue(TEXT("Seeded stellar physical fields are finite and nondegenerate"),
			FMath::IsFinite(Star->Mass) && Star->Mass > 0.0
			&& FMath::IsFinite(Star->Radius) && Star->Radius > 0.0
			&& FMath::IsFinite(Star->RadiusKM) && Star->RadiusKM > 0.0
			&& FMath::IsFinite(Star->Luminosity) && Star->Luminosity >= 0.0);
		return APSPlanetaryGenerationDeterminism::SnapshotStar(*Star);
	};
	for (int32 TypeIndex = 0; TypeIndex < static_cast<int32>(EStellarType::Unknown); ++TypeIndex)
	{
		const EStellarType Type = static_cast<EStellarType>(TypeIndex);
		for (const int32 Seed : {271828, 17, -88117, 0})
		{
			const FString Initial = Generate(Seed, Type, ESpectralClass::Unknown);
			for (int32 Index = 0; Index < 129; ++Index) FMath::Rand();
			Generate(42042, EStellarType::HyperGiant, ESpectralClass::O);
			TestEqual(TEXT("A stellar address retains all physical fields after unrelated random draws: ")
				+ UEnum::GetValueAsString(Type), Generate(Seed, Type, ESpectralClass::Unknown), Initial);
		}
	}
	for (const ESpectralClass Spectrum : {ESpectralClass::O, ESpectralClass::B, ESpectralClass::A,
		ESpectralClass::F, ESpectralClass::G, ESpectralClass::K, ESpectralClass::M})
	{
		const FString Initial = Generate(12345, EStellarType::MainSequence, Spectrum);
		Generate(54321, EStellarType::BrownDwarf, ESpectralClass::Y);
		TestEqual(TEXT("Explicit main-sequence spectrum retains mass, temperature and capped luminosity"),
			Generate(12345, EStellarType::MainSequence, Spectrum), Initial);
	}
	TestNotEqual(TEXT("Signed canonical seeds do not alias one another"),
		Generate(-1, EStellarType::Giant, ESpectralClass::K),
		Generate(-2, EStellarType::Giant, ESpectralClass::K));
	TestNotEqual(TEXT("Explicit zero seed remains a distinct address"),
		Generate(0, EStellarType::MainSequence, ESpectralClass::G),
		Generate(1, EStellarType::MainSequence, ESpectralClass::G));
	const auto RandomSequence = [&]()
	{
		Generator->SetGenerationSeed(271828);
		FString Result;
		for (int32 Index = 0; Index < 32; ++Index)
		{
			TSharedPtr<FStarModel> Star = MakeShared<FStarModel>();
			Generator->GenerateRandomStarModel(Star);
			Result += APSPlanetaryGenerationDeterminism::SnapshotStar(*Star);
		}
		return Result;
	};
	const FString InitialRandom = RandomSequence();
	for (int32 Index = 0; Index < 1000; ++Index) FMath::Rand();
	TestEqual(TEXT("Random stellar types and spectra use the owned stream too"), RandomSequence(), InitialRandom);
	const auto ClusterSequence = [&]()
	{
		Generator->SetGenerationSeed(87522);
		TSharedPtr<FStarClusterModel> Cluster = MakeShared<FStarClusterModel>();
		Cluster->StarClusterComposition = EStarClusterComposition::AllSpectral;
		Cluster->StarClusterPopulation = EStarClusterPopulation::Unknown;
		FString Result;
		for (int32 Index = 0; Index < 16; ++Index)
		{
			TSharedPtr<FStarModel> Star = MakeShared<FStarModel>();
			Generator->GenerateStarModelByProbability(Star, Cluster);
			Result += APSPlanetaryGenerationDeterminism::SnapshotStar(*Star);
		}
		return Result;
	};
	const FString InitialCluster = ClusterSequence();
	RandomSequence();
	TestEqual(TEXT("Cluster-weighted stellar generation is repeatable"), ClusterSequence(), InitialCluster);
	FMath::RandInit(76123);
	const int32 ExpectedGlobalNext = FMath::Rand();
	FMath::RandInit(76123);
	RandomSequence();
	ClusterSequence();
	Generate(771, EStellarType::HyperGiant, ESpectralClass::O);
	TestEqual(TEXT("Seeded stellar generation does not consume the process-global stream"), FMath::Rand(), ExpectedGlobalNext);
	FStarModel Compact;
	Compact.SurfaceTemperature = 0;
	Generator->SetGenerationSeed(111);
	TestTrue(TEXT("Compact radius override is valid"), Generator->ApplyRadiusOverrideSolar(Compact, 0.00002));
	const FString FirstOverride = APSPlanetaryGenerationDeterminism::SnapshotStar(Compact);
	Generator->ApplyRadiusOverrideSolar(Compact, 0.00002);
	TestEqual(TEXT("Repeated radius override is idempotent, including the compact luminosity floor"),
		APSPlanetaryGenerationDeterminism::SnapshotStar(Compact), FirstOverride);
	return true;
}

#endif
