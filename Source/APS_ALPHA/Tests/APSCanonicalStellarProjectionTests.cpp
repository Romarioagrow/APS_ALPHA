#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSCanonicalStellarProjectionContractTest,
	"APS.FullScale.CanonicalStellarProjection.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSCanonicalStellarProjectionContractTest::RunTest(const FString& Parameters)
{
	constexpr uint32 ContextHash = 0x6a09e667u;
	constexpr double GalaxyHalfExtent = 1.0e6;
	constexpr double ClusterHalfExtent = 2.0e5;
	constexpr double ClusterToGalaxyScale = 0.37;
	const FVector HomeClusterLocal(1.1e5, -8.0e4, 3.0e4);

	FAPSCanonicalStellarProjectionFrame GalaxyMenu;
	FAPSCanonicalStellarProjectionFrame ClusterMenu;
	FAPSCanonicalStellarProjectionFrame GalaxyGameplay;
	FAPSCanonicalStellarProjectionFrame ClusterGameplay;
	const bool bMenuConfigured = APSCanonicalStellarProjection::ConfigureSharedHomeCentredFrames(
		GalaxyHalfExtent, ClusterHalfExtent, ClusterToGalaxyScale, HomeClusterLocal,
		ContextHash, GalaxyMenu, ClusterMenu);
	const bool bGameplayConfigured = APSCanonicalStellarProjection::ConfigureSharedHomeCentredFrames(
		GalaxyHalfExtent, ClusterHalfExtent, ClusterToGalaxyScale, HomeClusterLocal,
		ContextHash, GalaxyGameplay, ClusterGameplay);
	TestTrue(TEXT("menu shared frame configured"), bMenuConfigured);
	TestTrue(TEXT("gameplay shared frame configured"), bGameplayConfigured);
	TestEqual(TEXT("projection version"), GalaxyMenu.ProjectionVersion,
		FAPSCanonicalStellarProjectionFrame::CurrentVersion);
	TestEqual(TEXT("menu/gameplay context"), GalaxyMenu.ContextHash, GalaxyGameplay.ContextHash);
	TestEqual(TEXT("galaxy/cluster share one root position scale"),
		GalaxyMenu.PositionScale, ClusterMenu.PositionScale);
	TestEqual(TEXT("menu/gameplay root position scale"),
		GalaxyMenu.PositionScale, GalaxyGameplay.PositionScale);
	TestEqual(TEXT("menu/gameplay cluster layout scale"),
		ClusterMenu.LayerToRootPositionScale, ClusterGameplay.LayerToRootPositionScale);
	TestTrue(TEXT("galaxy frame finite"), GalaxyMenu.IsFinite());
	TestTrue(TEXT("cluster frame finite"), ClusterMenu.IsFinite());

	const FVector HomeProxy = ClusterMenu.ProjectCanonicalUnits(HomeClusterLocal);
	TestTrue(TEXT("canonical home maps exactly to render anchor"),
		HomeProxy.Equals(ClusterMenu.RenderAnchorCm, 0.0));
	TestTrue(TEXT("menu/gameplay home projection deterministic"),
		HomeProxy.Equals(ClusterGameplay.ProjectCanonicalUnits(HomeClusterLocal), 0.0));

	const FVector GalaxyCanonical(8.75e5, -6.25e5, 2.5e5);
	const FVector ClusterCanonical(-1.4e5, 1.1e5, -4.0e4);
	const FVector GalaxyProxy = GalaxyMenu.ProjectCanonicalUnits(GalaxyCanonical);
	const FVector ClusterProxy = ClusterMenu.ProjectCanonicalUnits(ClusterCanonical);
	const FVector HomeCanonicalRootCm = ClusterMenu.GetCanonicalRootPositionCm(HomeClusterLocal);
	const FVector ClusterCanonicalRootCm =
		ClusterMenu.GetCanonicalRootPositionCm(ClusterCanonical);
	TestTrue(TEXT("home root address equals descriptor canonical anchor"),
		HomeCanonicalRootCm.Equals(ClusterMenu.CanonicalAnchorCm, 0.0));
	TestTrue(TEXT("canonical distance is independent of render scale"),
		FMath::IsNearlyEqual(
			ClusterMenu.GetCanonicalDistanceFromAnchorCm(ClusterCanonical),
			FVector::Distance(ClusterCanonicalRootCm, HomeCanonicalRootCm), 0.01));
	TestTrue(TEXT("same galaxy record maps identically across menu/gameplay"),
		GalaxyProxy.Equals(GalaxyGameplay.ProjectCanonicalUnits(GalaxyCanonical), 0.0));
	TestTrue(TEXT("same cluster record maps identically across menu/gameplay"),
		ClusterProxy.Equals(ClusterGameplay.ProjectCanonicalUnits(ClusterCanonical), 0.0));
	TestTrue(TEXT("repeat projection deterministic"),
		GalaxyProxy.Equals(GalaxyMenu.ProjectCanonicalUnits(GalaxyCanonical), 0.0));
	TestTrue(TEXT("galaxy inverse preserves canonical address"),
		GalaxyMenu.UnprojectToCanonicalUnits(GalaxyProxy).Equals(GalaxyCanonical, 1.0e-6));
	TestTrue(TEXT("cluster inverse preserves canonical address"),
		ClusterMenu.UnprojectToCanonicalUnits(ClusterProxy).Equals(ClusterCanonical, 1.0e-6));

	const FVector HomeRoot = HomeClusterLocal * ClusterToGalaxyScale;
	const FVector GalaxyRoot = GalaxyCanonical;
	const FVector ClusterRoot = ClusterCanonical * ClusterToGalaxyScale;
	const double CanonicalDistanceRatio = FVector::Distance(GalaxyRoot, HomeRoot)
		/ FVector::Distance(ClusterRoot, HomeRoot);
	const double ProxyDistanceRatio = FVector::Distance(GalaxyProxy, HomeProxy)
		/ FVector::Distance(ClusterProxy, HomeProxy);
	TestTrue(TEXT("cross-layer distance ratios preserve one affine root"),
		FMath::IsNearlyEqual(CanonicalDistanceRatio, ProxyDistanceRatio, 1.0e-9));
	TestTrue(TEXT("cross-layer direction preserves one affine root"),
		FVector::DotProduct((GalaxyRoot - HomeRoot).GetSafeNormal(),
			(GalaxyProxy - HomeProxy).GetSafeNormal()) > 1.0 - 1.0e-9);

	for (const double XSign : {-1.0, 1.0})
	{
		for (const double YSign : {-1.0, 1.0})
		{
			for (const double ZSign : {-1.0, 1.0})
			{
				const FVector GalaxyCorner(
					XSign * GalaxyHalfExtent, YSign * GalaxyHalfExtent,
					ZSign * GalaxyHalfExtent);
				const FVector ClusterCorner(
					XSign * ClusterHalfExtent, YSign * ClusterHalfExtent,
					ZSign * ClusterHalfExtent);
				TestTrue(TEXT("galaxy envelope stays inside declared hard bound"),
					APSCanonicalStellarProjection::MaxAbsComponent(
						GalaxyMenu.ProjectCanonicalUnits(GalaxyCorner))
						<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm + 0.01);
				TestTrue(TEXT("cluster envelope stays inside declared hard bound"),
					APSCanonicalStellarProjection::MaxAbsComponent(
						ClusterMenu.ProjectCanonicalUnits(ClusterCorner))
						<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm + 0.01);
			}
		}
	}

	constexpr double SmallRadiusSolar = 0.42;
	constexpr double LargeRadiusSolar = 9.0;
	const double GalaxySmallPhysical = APSCanonicalStellarProjection::ProjectPhysicalRadiusSolar(
		GalaxyMenu, SmallRadiusSolar);
	const double GalaxyLargePhysical = APSCanonicalStellarProjection::ProjectPhysicalRadiusSolar(
		GalaxyMenu, LargeRadiusSolar);
	const double ClusterLargePhysical = APSCanonicalStellarProjection::ProjectPhysicalRadiusSolar(
		ClusterMenu, LargeRadiusSolar);
	TestTrue(TEXT("physical radius ratio preserved before angular LOD"),
		FMath::IsNearlyEqual(GalaxySmallPhysical / GalaxyLargePhysical,
			SmallRadiusSolar / LargeRadiusSolar, 1.0e-10));
	TestTrue(TEXT("cluster layout scale never changes physical stellar radius"),
		FMath::IsNearlyEqual(GalaxyLargePhysical, ClusterLargePhysical, 1.0e-10));
	const double GalaxySmallVisual = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
		EAPSCanonicalStellarProxyLayer::Galaxy, GalaxyMenu, SmallRadiusSolar);
	const double GalaxyLargeVisual = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
		EAPSCanonicalStellarProxyLayer::Galaxy, GalaxyMenu, LargeRadiusSolar);
	TestTrue(TEXT("bounded readability LOD retains stellar class ordering"),
		GalaxyLargeVisual > GalaxySmallVisual);
	TestTrue(TEXT("bounded readability LOD has an explicit immutable floor"),
		GalaxySmallVisual >= GalaxyMenu.ProxyHalfExtentCm
			* GalaxyMenu.VisualRadiusFloorFraction
			* GalaxyMenu.VisualRadiusMinClassScale);
	TestTrue(TEXT("bounded readability LOD never becomes a catalogue blob"),
		GalaxyLargeVisual <= GalaxyMenu.ProxyHalfExtentCm
			* GalaxyMenu.VisualRadiusCeilingFraction + 0.01);
	TestEqual(TEXT("menu/gameplay use the same immutable readability result"),
		GalaxyLargeVisual,
		APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
			EAPSCanonicalStellarProxyLayer::Galaxy, GalaxyGameplay, LargeRadiusSolar));

	const FTransform UnitRoot(FQuat::Identity, FVector::ZeroVector, FVector::OneVector);
	const FTransform GalaxyProxyTransform(
		FQuat::Identity, GalaxyProxy, FVector(GalaxyLargePhysical));
	TestTrue(TEXT("unit root matrix remains bounded"),
		APSCanonicalStellarProjection::TransformMatrixMagnitude(UnitRoot) <= 1.0);
	TestTrue(TEXT("proxy matrix remains bounded"),
		APSCanonicalStellarProjection::TransformMatrixMagnitude(GalaxyProxyTransform)
			<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm + 0.01);

	const FGuid HomeId(0x01234567, 0x89abcdef, 0x10203040, 0x50607080);
	const FGuid NeighbourId(0x89abcdef, 0x01234567, 0x50607080, 0x10203040);
	FTransform SuppressedHome(FQuat::Identity, HomeProxy, FVector(12.0));
	FTransform VisibleNeighbour(FQuat::Identity, ClusterProxy, FVector(7.0));
	TestTrue(TEXT("matching home proxy suppressed"),
		APSCanonicalStellarProjection::SuppressExactMaterializedProxy(
			HomeId, HomeId, SuppressedHome));
	TestFalse(TEXT("neighbour proxy not suppressed"),
		APSCanonicalStellarProjection::SuppressExactMaterializedProxy(
			NeighbourId, HomeId, VisibleNeighbour));
	TestTrue(TEXT("home suppression keeps projected address"),
		SuppressedHome.GetLocation().Equals(HomeProxy, 0.0));
	TestTrue(TEXT("home scale is exactly zero"),
		SuppressedHome.GetScale3D() == FVector::ZeroVector);
	TestTrue(TEXT("neighbour remains visible"),
		VisibleNeighbour.GetScale3D() == FVector(7.0));

	FAPSCanonicalStellarDataset PersistedDataset;
	PersistedDataset.ResetForInput(0x12345678u, 271828);
	PersistedDataset.DatasetHash = 0x87654321u;
	PersistedDataset.GalaxyGenerationSeed = 1;
	PersistedDataset.GalaxyModeledStarCount = 100000000;
	PersistedDataset.ClusterGenerationSeed = 2;
	PersistedDataset.ClusterModeledCount = 1;
	PersistedDataset.HomeCanonicalIndex = 0;
	PersistedDataset.HomeStableId = HomeId;
	FAPSCanonicalClusterSystemRecord& PersistedHome =
		PersistedDataset.ClusterRecords.AddDefaulted_GetRef();
	PersistedHome.StableId = HomeId;
	PersistedHome.CanonicalIndex = 0;
	PersistedHome.SystemModel.StableId = HomeId;
	PersistedHome.PrimaryStarModel.MinOrbit = 0.37;
	PersistedHome.PrimaryStarModel.MaxOrbit = 41.5;
	PersistedDataset.bFinalized = true;
	TestTrue(TEXT("finalized dataset survives as one reusable value contract"),
		PersistedDataset.IsUsable(0x12345678u));
	TestFalse(TEXT("changed model input invalidates finalized dataset"),
		PersistedDataset.IsUsable(0x12345679u));
	PersistedHome.SystemModel.StableId = NeighbourId;
	TestFalse(TEXT("record/system identity mismatch invalidates finalized dataset"),
		PersistedDataset.IsUsable(0x12345678u));
	PersistedHome.SystemModel.StableId = HomeId;
	FAPSCanonicalStellarDataset DuplicateIdDataset = PersistedDataset;
	DuplicateIdDataset.ClusterModeledCount = 2;
	FAPSCanonicalClusterSystemRecord& DuplicateRecord =
		DuplicateIdDataset.ClusterRecords.AddDefaulted_GetRef();
	DuplicateRecord = PersistedHome;
	DuplicateRecord.CanonicalIndex = 1;
	TestFalse(TEXT("duplicate canonical StableId invalidates finalized dataset"),
		DuplicateIdDataset.IsUsable(0x12345678u));
	FAPSCanonicalStellarDataset LocationMismatchDataset = PersistedDataset;
	LocationMismatchDataset.ClusterRecords[0].SystemModel.Location = FVector(1.0, 0.0, 0.0);
	TestFalse(TEXT("record/model location mismatch invalidates finalized dataset"),
		LocationMismatchDataset.IsUsable(0x12345678u));

	UGeneratedWorld* SourceWorld = NewObject<UGeneratedWorld>(GetTransientPackage());
	TestNotNull(TEXT("source generated world allocated"), SourceWorld);
	if (SourceWorld)
	{
		SourceWorld->CanonicalStellarDataset = PersistedDataset;
		UGeneratedWorld* DuplicatedWorld = DuplicateObject<UGeneratedWorld>(
			SourceWorld, GetTransientPackage());
		TestNotNull(TEXT("generated world dataset duplicates for gameplay handoff"), DuplicatedWorld);
		if (DuplicatedWorld)
		{
			const FAPSCanonicalClusterSystemRecord& DuplicatedHome =
				DuplicatedWorld->CanonicalStellarDataset.ClusterRecords[0];
			TestEqual(TEXT("duplicate preserves canonical MinOrbit"),
				DuplicatedHome.PrimaryStarModel.MinOrbit, 0.37);
			TestEqual(TEXT("duplicate preserves canonical MaxOrbit"),
				DuplicatedHome.PrimaryStarModel.MaxOrbit, 41.5);
			TestTrue(TEXT("duplicate preserves structural dataset validity"),
				DuplicatedWorld->CanonicalStellarDataset.IsUsable(0x12345678u));
		}
	}

	const APSCanonicalStellarProjection::FNestedCatalogPermutation SmallLod =
		APSCanonicalStellarProjection::MakeNestedCatalogPermutation(271828, 100000000);
	const APSCanonicalStellarProjection::FNestedCatalogPermutation LargeLod =
		APSCanonicalStellarProjection::MakeNestedCatalogPermutation(271828, 100000000);
	TSet<int64> UniqueCatalogIndices;
	for (int32 Index = 0; Index < 256; ++Index)
	{
		const int64 CatalogIndex = LargeLod.Resolve(Index);
		TestTrue(TEXT("nested catalog sample has no duplicate StableId ordinal"),
			!UniqueCatalogIndices.Contains(CatalogIndex));
		UniqueCatalogIndices.Add(CatalogIndex);
		if (Index < 64)
		{
			TestEqual(TEXT("larger LOD retains smaller LOD prefix"),
				CatalogIndex, SmallLod.Resolve(Index));
		}
	}

	TestEqual(TEXT("empty canonical catalog has no home"),
		APSCanonicalStellarProjection::SelectSharedHomeInstanceIndex(271828, 314159, 0),
		INDEX_NONE);
	for (const int32 ModeledCount : {1, 50, 99, 100, 12000})
	{
		const int32 HomeIndex = APSCanonicalStellarProjection::SelectSharedHomeInstanceIndex(
			271828, 314159, ModeledCount);
		TestTrue(TEXT("home identity stays inside canonical shared window"),
			HomeIndex >= 0 && HomeIndex < FMath::Min(
				ModeledCount, APSCanonicalStellarProjection::SharedHomeSelectionWindow));
	}
	return true;
}

#endif
