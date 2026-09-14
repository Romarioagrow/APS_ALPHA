#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPreviewDisplayNameTest,
	"APS.Editor.MasterFix.DisplayNamePersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewDisplayNameTest::RunTest(const FString& Parameters)
{
	UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
	const FString PlanetKey(TEXT("SYS0/S0/P0"));
	const FString MoonKey(TEXT("SYS0/S0/P0/M0"));
	const int32 SeedBefore = UGeneratedWorld::ResolveCanonicalSurfaceSeed(0, 42, PlanetKey);
	TestTrue(TEXT("Rename accepts trimmed Unicode labels"),
		Model->SetPreviewDisplayNameOverride(PlanetKey, TEXT("  Земля / Terra  ")));
	TestEqual(TEXT("Outer whitespace removed"), *Model->FindPreviewDisplayNameOverride(PlanetKey),
		FString(TEXT("Земля / Terra")));
	TestTrue(TEXT("Moon has independent address"), Model->SetPreviewDisplayNameOverride(MoonKey, TEXT("Luna")));
	TestFalse(TEXT("Whitespace-only name rejected"), Model->SetPreviewDisplayNameOverride(PlanetKey, TEXT("  ")));
	TestFalse(TEXT("Control characters rejected"), Model->SetPreviewDisplayNameOverride(PlanetKey, TEXT("A\nB")));
	TestFalse(TEXT("FName empty sentinel cannot swallow a label"), Model->SetPreviewDisplayNameOverride(PlanetKey, TEXT("nOnE")));
	TestFalse(TEXT("Oversized label rejected"), Model->SetPreviewDisplayNameOverride(PlanetKey, FString::ChrN(129, TEXT('X'))));
	TestFalse(TEXT("Unaddressed body rejected"), Model->SetPreviewDisplayNameOverride(TEXT(""), TEXT("Lost")));
	TestEqual(TEXT("Renaming cannot change physical generation seed"),
		UGeneratedWorld::ResolveCanonicalSurfaceSeed(0, 42, PlanetKey), SeedBefore);
	UGeneratedWorld* Committed = DuplicateObject<UGeneratedWorld>(Model, GetTransientPackage());
	const FGeneratedWorldData Saved = Committed->SaveWorldData();
	TestEqual(TEXT("Save metadata also retains non-materialized body names"),
		Saved.PreviewDisplayNameOverrides.FindRef(MoonKey), FString(TEXT("Luna")));
	TestEqual(TEXT("Committed world retains names"), *Committed->FindPreviewDisplayNameOverride(MoonKey), FString(TEXT("Luna")));
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	APlanet* Rebuilt = World->SpawnActor<APlanet>();
	if (TestNotNull(TEXT("Rebuilt body"), Rebuilt))
	{
		const FName ObjectName = Rebuilt->GetFName();
		TestTrue(TEXT("Recreated body receives accepted display name"),
			AAstroGenerator::ApplyPreviewDisplayNameByKey(Committed, PlanetKey, Rebuilt));
		TestEqual(TEXT("Display label updated"), Rebuilt->AstroName, FName(TEXT("Земля / Terra")));
		TestEqual(TEXT("UObject identity unchanged"), Rebuilt->GetFName(), ObjectName);
		TestFalse(TEXT("Unrelated body is untouched"),
			AAstroGenerator::ApplyPreviewDisplayNameByKey(Committed, TEXT("SYS0/S0/P1"), Rebuilt));
	}
	World->DestroyWorld(false);
	Model->ClearPreviewDisplayNameOverrides();
	TestNull(TEXT("Explicit reroll clears authored labels"), Model->FindPreviewDisplayNameOverride(PlanetKey));
	TestNotNull(TEXT("Clearing editor does not mutate committed copy"), Committed->FindPreviewDisplayNameOverride(PlanetKey));
	return true;
}
#endif
