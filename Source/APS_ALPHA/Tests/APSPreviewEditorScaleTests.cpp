#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStellarEditPlanetOrbitTest,
	"APS.Preview.Editor.StellarEditKeepsPlanetOrbits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarEditPlanetOrbitTest::RunTest(const FString& Parameters)
{
	FAPSPreviewStarEditOverride Edit;
	Edit.PlanetOrbitRadiiAu = {1.0, 3.0, 8.0};
	Edit.PlanetOrbitDistribution = EOrbitDistributionType::Uniform;
	Edit.Model.Radius = 50.0;
	Edit.Model.RadiusKM = 50.0 * 695700.0;
	Edit.Model.Mass = 400.0;
	FPlanetarySystemModel Family;
	Family.OrbitDistributionType = EOrbitDistributionType::Uniform;
	for (const double Orbit : {400.0, 1200.0, 3200.0})
	{
		TSharedPtr<FPlanetData> Data = MakeShared<FPlanetData>();
		Data->PlanetModel = MakeShared<FPlanetModel>();
		Data->PlanetModel->Radius = 1.0;
		Data->PlanetModel->RadiusKM = 6371.0;
		Data->OrbitRadius = Data->PlanetModel->OrbitDistance = Orbit;
		Family.PlanetsList.Add(Data);
	}
	TestTrue(TEXT("A star edit restores the original physical AU layout"), Edit.ApplyToPlanetOrbits(Family));
	UPlanetarySystemGenerator::EnforcePlanetSurfaceClearance(Family);
	for (int32 Index = 0; Index < Family.PlanetsList.Num(); ++Index)
	{
		TestEqual(TEXT("Nonoverlapping planets do not inherit the giant's mass multiplier"),
			Family.PlanetsList[Index]->PlanetModel->OrbitDistance, Edit.PlanetOrbitRadiiAu[Index]);
		TestEqual(TEXT("Serialized planet data has the same physical orbit"),
			Family.PlanetsList[Index]->PlanetModelData.OrbitDistance, Edit.PlanetOrbitRadiiAu[Index]);
	}
	TestEqual(TEXT("The actual giant radius is not normalized down"), Edit.Model.RadiusKM, 50.0f * 695700.0f);
	TestEqual(TEXT("The actual giant mass is not replaced with a cosmetic value"), Edit.Model.Mass, 400.0f);
	UGeneratedWorld* World = NewObject<UGeneratedWorld>();
	World->SetPreviewStarEditOverride(TEXT("SYS0/S0"), Edit);
	UGeneratedWorld* Copy = DuplicateObject<UGeneratedWorld>(World, GetTransientPackage());
	const auto* Copied = Copy->FindPreviewStarEditOverride(TEXT("SYS0/S0"));
	TestTrue(TEXT("Accepted orbital baseline survives gameplay reflection transport"),
		Copied && Copied->PlanetOrbitRadiiAu == Edit.PlanetOrbitRadiiAu);
	TestEqual(TEXT("Copied orbital layout retains its deterministic hash"), Copy->GetPreviewStarEditHash(), World->GetPreviewStarEditHash());
	Edit.PlanetOrbitRadiiAu[1] = 4.0;
	World->SetPreviewStarEditOverride(TEXT("SYS0/S0"), Edit);
	TestTrue(TEXT("Changing physical orbital baseline invalidates the preview manifest"),
		World->GetPreviewStarEditHash() != Copy->GetPreviewStarEditHash());
	Family.OrbitDistributionType = EOrbitDistributionType::Dense;
	TestFalse(TEXT("Exact replay does not undo a different recipe sampled in the retained AU range"), Edit.ApplyToPlanetOrbits(Family));
	Family.OrbitDistributionType = EOrbitDistributionType::Uniform;
	Edit.PlanetOrbitRadiiAu[2] = -1.0;
	TestFalse(TEXT("Invalid snapshot cannot partially overwrite a family"), Edit.ApplyToPlanetOrbits(Family));
	TestEqual(TEXT("Validation rejects all writes atomically"), Family.PlanetsList[1]->OrbitRadius, 3.0);
	Edit.PlanetOrbitRadiiAu.Pop();
	TestFalse(TEXT("Different planet count cannot alias a partial old family"), Edit.ApplyToPlanetOrbits(Family));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStellarEditOrbitDistributionRangeTest,
	"APS.Preview.Editor.StellarEditDistributionKeepsPhysicalRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarEditOrbitDistributionRangeTest::RunTest(const FString& Parameters)
{
	UPlanetarySystemGenerator* Generator = NewObject<UPlanetarySystemGenerator>();
	UPlanetGenerator* Planets = NewObject<UPlanetGenerator>();
	UMoonGenerator* Moons = NewObject<UMoonGenerator>();
	FAPSPreviewStarEditOverride Edit;
	Edit.PlanetOrbitRadiiAu = {1.0, 3.0, 8.0};
	const EOrbitDistributionType Distributions[] = {
		EOrbitDistributionType::Uniform, EOrbitDistributionType::Gaussian,
		EOrbitDistributionType::Chaotic, EOrbitDistributionType::InnerOuter, EOrbitDistributionType::Dense};
	const EStellarType StellarTypes[] = {EStellarType::MainSequence, EStellarType::Giant,
		EStellarType::BrightGiant, EStellarType::SuperGiant, EStellarType::HyperGiant};
	const auto Generate = [&](const FAPSPreviewStarEditOverride* Override, EOrbitDistributionType Distribution,
		EStellarType Type, int32 Count, double Mass, double Radius, bool bRandomRecipe = false)
	{
		Generator->SetGenerationSeed(271828);
		auto Star = MakeShared<FStarModel>();
		Star->StellarType = Type;
		Star->SpectralClass = ESpectralClass::G;
		Star->Mass = Mass;
		Star->Radius = Radius;
		Star->RadiusKM = Radius * 695700.0;
		Star->Luminosity = 100.0;
		Star->SurfaceTemperature = 5772;
		auto Family = MakeShared<FPlanetarySystemModel>();
		Family->AmountOfPlanets = Count;
		Family->PlanetarySystemType = Count == 0 ? EPlanetarySystemType::NoPlanetSystem
			: Count == 1 ? EPlanetarySystemType::SinglePlanetSystem : EPlanetarySystemType::MultiPlanetSystem;
		Family->OrbitDistributionType = Distribution;
		if (bRandomRecipe) Generator->GeneratePlanetarySystemModelByStar(Family, Star, Planets, Moons, Override);
		else Generator->GenerateCustomPlanetarySystemModel(Family, Star, Planets, Moons, Override);
		if (Override) Override->ApplyToPlanetOrbits(*Family);
		if (!bRandomRecipe) TestEqual(TEXT("An explicit planet count is respected"), Family->PlanetsList.Num(), Count);
		TArray<double> Result;
		for (const auto& Data : Family->PlanetsList)
		{
			if (!Data || !Data->PlanetModel)
			{
				AddError(TEXT("Generated orbit has no planet model"));
				continue;
			}
			Result.Add(Data->OrbitRadius);
			TestEqual(TEXT("Model and actor input share physical AU"), Data->PlanetModel->OrbitDistance, Data->OrbitRadius);
		}
		return Result;
	};
	for (const EOrbitDistributionType Original : Distributions)
	{
		Edit.PlanetOrbitDistribution = Original;
		for (const EOrbitDistributionType Target : Distributions)
		{
			for (const EStellarType Type : StellarTypes)
			{
				for (const int32 Count : {0, 1, 3, 9})
				{
					const auto Orbits = Generate(&Edit, Target, Type, Count, 400.0, 50.0);
					for (const double Orbit : Orbits)
						TestTrue(TEXT("All recipes and counts stay in the retained physical range despite giant mass"),
							FMath::IsFinite(Orbit) && Orbit >= 1.0 - 1.0e-9 && Orbit <= 8.0 + 1.0e-9);
					if (Count == 3 && Original == Target)
						TestTrue(TEXT("Unchanged recipe still restores exact accepted positions"), Orbits == Edit.PlanetOrbitRadiiAu);
				}
			}
		}
	}
	Edit.PlanetOrbitDistribution = EOrbitDistributionType::Uniform;
	TArray<TArray<double>> RecipeLayouts;
	for (const EOrbitDistributionType Target : Distributions)
	{
		const auto Orbits = Generate(&Edit, Target, EStellarType::BrightGiant, 9, 400.0, 50.0);
		TestTrue(TEXT("Recipe is deterministic after unrelated generation"),
			Orbits == Generate(&Edit, Target, EStellarType::BrightGiant, 9, 400.0, 50.0));
		TestTrue(TEXT("Changing stellar mass does not change orbit scale"),
			Orbits == Generate(&Edit, Target, EStellarType::BrightGiant, 9, 40.0, 50.0));
		for (const auto& Other : RecipeLayouts)
			TestTrue(TEXT("Different distribution controls still produce distinct layouts"), Orbits != Other);
		RecipeLayouts.Add(Orbits);
	}
	for (const EStellarType Type : StellarTypes)
	{
		for (const double Orbit : Generate(&Edit, EOrbitDistributionType::Uniform, Type, 3, 400.0, 50.0, true))
			TestTrue(TEXT("Random-family route also respects the retained range"), Orbit >= 1.0 - 1.0e-9 && Orbit <= 8.0 + 1.0e-9);
	}
	for (const double Orbit : Generate(nullptr, EOrbitDistributionType::Uniform, EStellarType::Giant, 3, 400.0, 50.0))
		TestTrue(TEXT("Unedited/legacy generation retains its original mass-based defaults"), Orbit >= 400.0);
	// When the star actually engulfs an orbit, the existing photosphere pass may
	// extend the range for safety, but never to the mass-derived thousands of AU.
	for (const double Orbit : Generate(&Edit, EOrbitDistributionType::Dense, EStellarType::HyperGiant, 9, 400.0, 2000.0))
		TestTrue(TEXT("Real stellar surface clearance is retained without giant mass scaling"),
			Orbit >= 2000.0 * 0.00465047 * 1.35 - 1.0e-9 && Orbit < 20.0);
	Edit.PlanetOrbitRadiiAu = {3.0};
	for (const double Orbit : Generate(&Edit, EOrbitDistributionType::Dense, EStellarType::Giant, 9, 400.0, 50.0))
		TestTrue(TEXT("Adding planets to a one-planet baseline cannot restore giant mass scaling"), Orbit >= 3.0 - 1.0e-9 && Orbit <= 3.25 + 1.0e-9);
	double Min = 400.0, Max = 4000.0;
	Edit.PlanetOrbitRadiiAu = {8.0, 1.0, 3.0};
	TestTrue(TEXT("Range extraction does not require actor traversal order"), Edit.TryGetPlanetOrbitRangeAu(Min, Max));
	TestEqual(TEXT("Physical minimum"), Min, 1.0);
	TestEqual(TEXT("Physical maximum"), Max, 8.0);
	for (const TArray<double>& Invalid : {TArray<double>{}, TArray<double>{1.0, -1.0}, TArray<double>{1.0, 0.0}})
	{
		Edit.PlanetOrbitRadiiAu = Invalid;
		Min = 400.0; Max = 4000.0;
		TestFalse(TEXT("Incomplete snapshots do not override generator defaults"), Edit.TryGetPlanetOrbitRangeAu(Min, Max));
		TestEqual(TEXT("Minimum unchanged on invalid snapshot"), Min, 400.0);
		TestEqual(TEXT("Maximum unchanged on invalid snapshot"), Max, 4000.0);
	}
	return true;
}

#endif
