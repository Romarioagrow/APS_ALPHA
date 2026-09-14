#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Structs/PlanetAtmosphereModel.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"

namespace APSAtmosphereRelativeScaleTests
{
	UWorld* CreateWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
			.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGeneratedAtmosphereRelativeScaleTest,
	"APS.Gameplay.Generation.AtmosphereRelativeScale.SharedFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSGeneratedAtmosphereRelativeScaleTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSAtmosphereRelativeScaleTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	APlanet* Planet = World->SpawnActor<APlanet>();
	AMoon* Moon = World->SpawnActor<AMoon>();
	APlanetarySurfaceGenerator* Generator = World->SpawnActor<APlanetarySurfaceGenerator>();
	if (!Planet || !Moon || !Generator) { World->DestroyWorld(false); return false; }
	for (double RadiusKm : {100.0, 6371.0, 6750.0, 70000.0})
	for (uint8 Type = 0; Type <= static_cast<uint8>(EPlanetType::Unknown); ++Type)
	{
		Planet->PlanetType = static_cast<EPlanetType>(Type);
		Planet->WorldScapeSeed = 41771;
		Planet->RadiusKM = RadiusKm;
		Planet->AtmosphereHeight = 100.0;
		Generator->InitAtmoScape(World, RadiusKm, Planet);
		AAtmoScape* Atmosphere = Generator->PlanetAtmosphere;
		if (!TestNotNull(TEXT("Every generated subtype has an atmosphere"), Atmosphere)) continue;
		TestTrue(TEXT("All generated atmospheres keep relative scaling enabled"), Atmosphere->bKeepRelativeScale);
		const float Rayleigh = Atmosphere->RayleighHeight, Mie = Atmosphere->MieHeight;
		TestEqual(TEXT("Generation adopts its authored height once"), Atmosphere->AtmosphereHeight, 100.0f);
		Atmosphere->bKeepRelativeScale = false; // Simulate a retained old shell.
		Generator->InitAtmoScape(World, RadiusKm, Planet);
		TestTrue(TEXT("Reinitialization repairs an old false flag"), Atmosphere->bKeepRelativeScale);
		TestEqual(TEXT("Reinitialization reuses the existing shell"), Generator->PlanetAtmosphere, Atmosphere);
		TestEqual(TEXT("No repeated scaling of Rayleigh height"), Atmosphere->RayleighHeight, Rayleigh);
		TestEqual(TEXT("No repeated scaling of Mie height"), Atmosphere->MieHeight, Mie);
	}
	Moon->RadiusKM = 1737.0;
	Moon->AtmosphereHeight = 15.0;
	Generator->InitAtmoScape(World, Moon->RadiusKM, Moon);
	TestTrue(TEXT("Shared moon generation also keeps the flag enabled"), Generator->PlanetAtmosphere->bKeepRelativeScale);
	// Do not merely set a decorative checkbox: the plugin must still scale relative edits.
	AAtmoScape* Atmosphere = Generator->PlanetAtmosphere;
	const float Height = Atmosphere->AtmosphereHeight, Rayleigh = Atmosphere->RayleighHeight;
	Atmosphere->PlanetRadius *= 2.0f;
	Atmosphere->UpdateScale();
	TestEqual(TEXT("Subsequent radius edit really scales atmosphere height"), Atmosphere->AtmosphereHeight, Height * 2.0f);
	TestEqual(TEXT("Subsequent radius edit really scales scattering height"), Atmosphere->RayleighHeight, Rayleigh * 2.0f);
	Atmosphere->UpdateScale();
	TestEqual(TEXT("Unchanged update does not compound relative scaling"), Atmosphere->AtmosphereHeight, Height * 2.0f);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSModelAtmosphereRelativeScaleTest,
	"APS.Gameplay.Generation.AtmosphereRelativeScale.ModelFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSModelAtmosphereRelativeScaleTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSAtmosphereRelativeScaleTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	APlanet* Planet = World->SpawnActor<APlanet>();
	UPlanetGenerator* Generator = NewObject<UPlanetGenerator>();
	if (!Planet) { World->DestroyWorld(false); return false; }
	auto Model = MakeShared<FPlanetAtmosphereModel>();
	Model->AtmosphereRadiusKm = 12000.0;
	Model->AtmosphereHeight = 137.0;
	Generator->GeneratePlanetAtmosphere(Planet, Model);
	TArray<AActor*> Attached;
	Planet->GetAttachedActors(Attached);
	int32 Count = 0;
	for (AActor* Actor : Attached) if (AAtmoScape* Atmosphere = Cast<AAtmoScape>(Actor))
	{
		++Count;
		TestTrue(TEXT("Model factory enables relative scaling"), Atmosphere->bKeepRelativeScale);
		TestEqual(TEXT("Explicit model height is not rescaled twice"), Atmosphere->AtmosphereHeight, 137.0f);
	}
	TestEqual(TEXT("Model factory produces one attached atmosphere"), Count, 1);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSRetainedAtmosphereRelativeScaleTest,
	"APS.Gameplay.Generation.AtmosphereRelativeScale.RetainedBodyEdits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSRetainedAtmosphereRelativeScaleTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSAtmosphereRelativeScaleTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	APlanet* Planet = World->SpawnActor<APlanet>();
	APlanetarySurfaceGenerator* Surface = World->SpawnActor<APlanetarySurfaceGenerator>();
	if (!Planet || !Surface) { World->DestroyWorld(false); return false; }
	Planet->PlanetaryEnvironmentGenerator = Surface;
	Planet->RadiusKM = 6750.0; Planet->PlanetRadiusKM = 6750;
	Surface->InitAtmoScape(World, Planet->RadiusKM, Planet);
	UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
	FAPSPreviewBodyEditOverride Edit;
	Edit.RadiusKm = 6750.0;
	for (double Height : {100.0, 0.0, 40.0, 200.0})
	{
		Edit.AtmosphereHeight = Height;
		Model->SetPreviewBodyEditOverride(TEXT("SYS0/S0/P0"), Edit);
		Surface->PlanetAtmosphere->bKeepRelativeScale = false;
		TestTrue(TEXT("Retained body edit is applied"), AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(Model, TEXT("SYS0/S0/P0"), Planet));
		const AAtmoScape* Atmosphere = Surface->PlanetAtmosphere;
		TestTrue(TEXT("Applying retained edits never turns relative scaling off"), Atmosphere->bKeepRelativeScale);
		TestEqual(TEXT("Explicit height survives including zero-to-positive edits"), Atmosphere->AtmosphereHeight, static_cast<float>(Height));
		TestTrue(TEXT("Zero-height transitions keep finite scattering"), FMath::IsFinite(Atmosphere->RayleighHeight) && FMath::IsFinite(Atmosphere->MieHeight));
	}
	World->DestroyWorld(false);
	return true;
}
#endif
