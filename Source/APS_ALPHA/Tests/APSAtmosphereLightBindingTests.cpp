#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "PlanetaryAtmosphere.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmosphereSharedLightBindingTest,
	"APS.Gameplay.Generation.AtmosphereLightBinding.PlanetsAndMoons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmosphereSharedLightBindingTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		false, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	AStar* FirstStar = World->SpawnActor<AStar>();
	AStar* SecondStar = World->SpawnActor<AStar>();
	APlanet* Planet = World->SpawnActor<APlanet>();
	AMoon* Moon = World->SpawnActor<AMoon>();
	APlanetarySurfaceGenerator* Surface = World->SpawnActor<APlanetarySurfaceGenerator>();
	if (!FirstStar || !SecondStar || !Planet || !Moon || !Surface)
	{
		AddError(TEXT("All atmosphere test actors must spawn"));
		World->DestroyWorld(false);
		return false;
	}
	FirstStar->SetActorLocation(FVector(1000000.0, -2000000.0, 3000000.0));
	SecondStar->SetActorLocation(FVector(-4000000.0, 5000000.0, -6000000.0));
	Planet->RadiusKM = 6750.0;
	Planet->AtmosphereHeight = 100.0;
	Planet->WorldScapePresentationScale = 1.0;
	Planet->ParentStar = FirstStar;
	Moon->RadiusKM = 848.4;
	Moon->AtmosphereHeight = 15.0;
	Moon->WorldScapePresentationScale = 1.0;
	Moon->ParentPlanet = Planet;

	const auto VerifyLight = [this, Surface](const TCHAR* Phase, AActor* ExpectedStar)
	{
		AAtmoScape* Atmosphere = Surface->PlanetAtmosphere;
		if (!TestNotNull(FString(Phase) + TEXT(" atmosphere"), Atmosphere)) return;
		TestTrue(FString(Phase) + TEXT(" correct actual light actor"),
			Atmosphere->LightSource == ExpectedStar);
		TestTrue(FString(Phase) + TEXT(" relative scale retained"), Atmosphere->bKeepRelativeScale);
		TestEqual(FString(Phase) + TEXT(" production opacity unchanged"),
			Atmosphere->PresentationOpacityScale, 1.0f);
		TInlineComponentArray<UStaticMeshComponent*> Shells;
		Atmosphere->GetComponents(Shells);
		int32 CheckedScatteringShells = 0;
		for (const UStaticMeshComponent* Shell : Shells)
		{
			if (!IsValid(Shell) || (Shell->GetName() != TEXT("PlanetaryAtmoMesh")
				&& Shell->GetName() != TEXT("SpacePlanetaryAtmoMesh"))) continue;
			++CheckedScatteringShells;
			UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0));
			if (!TestNotNull(FString(Phase) + TEXT(" scattering MID"), Material)) continue;
			float CustomLight = -1.0f;
			TestTrue(FString(Phase) + TEXT(" custom-light parameter readable"),
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("CustomLightSource")), CustomLight));
			TestEqual(FString(Phase) + TEXT(" correct material light branch"),
				CustomLight, ExpectedStar ? 1.0f : 0.0f);
			if (ExpectedStar)
			{
				FLinearColor Position;
				TestTrue(FString(Phase) + TEXT(" light position readable"),
					Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("LightPosition")), Position));
				TestTrue(FString(Phase) + TEXT(" current parent-star position published"),
					Position.Equals(FLinearColor(ExpectedStar->GetActorLocation()), 0.01f));
			}
		}
		TestEqual(FString(Phase) + TEXT(" inside and outside scattering shells checked"),
			CheckedScatteringShells, 2);
	};

	Surface->InitAtmoScape(World, Planet->RadiusKM, Planet);
	VerifyLight(TEXT("Planet factory without caller repair"), FirstStar);
	AAtmoScape* OriginalShell = Surface->PlanetAtmosphere;
	Surface->InitAtmoScape(World, Moon->RadiusKM, Moon);
	VerifyLight(TEXT("Moon factory without caller repair"), FirstStar);
	TestTrue(TEXT("Reinitialization reuses the existing atmosphere"),
		Surface->PlanetAtmosphere == OriginalShell);
	Planet->ParentStar = SecondStar;
	Surface->InitAtmoScape(World, Moon->RadiusKM, Moon);
	VerifyLight(TEXT("Retained moon rebinds changed star"), SecondStar);
	Moon->ParentPlanet = nullptr;
	Surface->InitAtmoScape(World, Moon->RadiusKM, Moon);
	VerifyLight(TEXT("Orphan moon clears stale light"), nullptr);
	Planet->ParentStar = nullptr;
	Surface->InitAtmoScape(World, Planet->RadiusKM, Planet);
	VerifyLight(TEXT("Orphan planet clears stale light"), nullptr);
	Planet->ParentStar = FirstStar;
	FirstStar->Destroy();
	Surface->InitAtmoScape(World, Planet->RadiusKM, Planet);
	VerifyLight(TEXT("Destroyed parent star is not retained"), nullptr);
	World->DestroyWorld(false);
	return true;
}

#endif
