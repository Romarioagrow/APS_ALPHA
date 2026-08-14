#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfacePlacementDeterminismTest,
	"APS.Surface.Placement.DeterministicContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfacePlacementDeterminismTest::RunTest(const FString& Parameters)
{
	const FAPSCivilizationFootprintRequest Defaults;
	TestEqual(TEXT("Default base length is 80m"), Defaults.BaseSizeCm.X, 8000.0);
	TestEqual(TEXT("Default base width is 55m"), Defaults.BaseSizeCm.Y, 5500.0);
	TestEqual(TEXT("Default pad diameter is 90m"), Defaults.PadDiameterCm, 9000.0);
	TestEqual(TEXT("Default separation is 140m"), Defaults.SeparationCm, 14000.0);

	const uint32 KeyA = UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
		7788, 1337, 4, 6378);
	const uint32 KeyB = UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(
		7788, 1337, 4, 6378);
	const uint32 ChangedManifestKey =
		UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(7789, 1337, 4, 6378);
	const uint32 ChangedSurfaceKey =
		UAPSPlanetSurfacePlacementResolver::BuildPlacementKey(7788, 1338, 4, 6378);
	TestEqual(TEXT("Identical canonical inputs preserve the placement key"), KeyA, KeyB);
	TestNotEqual(TEXT("Manifest seed changes the placement key"), KeyA, ChangedManifestKey);
	TestNotEqual(TEXT("Surface seed changes the placement key"), KeyA, ChangedSurfaceKey);

	FVector OutwardA;
	FVector ForwardA;
	FVector OutwardB;
	FVector ForwardB;
	UAPSPlanetSurfacePlacementResolver::BuildSeedFrame(KeyA, OutwardA, ForwardA);
	UAPSPlanetSurfacePlacementResolver::BuildSeedFrame(KeyA, OutwardB, ForwardB);
	TestTrue(TEXT("Seed frame outward is normalized"), OutwardA.IsUnit(1.0e-6));
	TestTrue(TEXT("Seed frame forward is normalized"), ForwardA.IsUnit(1.0e-6));
	TestTrue(TEXT("Seed frame is tangent"),
		FMath::Abs(FVector::DotProduct(OutwardA, ForwardA)) <= 1.0e-6);
	TestTrue(TEXT("Seed frame outward is deterministic"), OutwardA.Equals(OutwardB, 1.0e-9));
	TestTrue(TEXT("Seed frame forward is deterministic"), ForwardA.Equals(ForwardB, 1.0e-9));
	return true;
}

#endif
