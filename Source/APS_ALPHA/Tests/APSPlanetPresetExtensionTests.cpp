#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Saves/APSWorldSaveSnapshot.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Generation/APSPlanetPresetMorphology.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "UObject/Package.h"

namespace APSPlanetPresetExtensionTests
{
	constexpr EPlanetType Added[] = {
		EPlanetType::Basalt, EPlanetType::Savanna, EPlanetType::Sulfur, EPlanetType::Crystal
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetPresetIdsTest,
	"APS.PlanetSurface.PresetExtension.StableIdsAndFamilies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetPresetIdsTest::RunTest(const FString& Parameters)
{
	const UEnum* Enum = StaticEnum<EPlanetType>();
	const TCHAR* LegacyNames[] = {
		TEXT("Rocky"), TEXT("Terrestrial"), TEXT("Greenhouse"), TEXT("Melted"),
		TEXT("HotGiant"), TEXT("GasGiant"), TEXT("IceGiant"), TEXT("Dwarf"),
		TEXT("Ocean"), TEXT("Water"), TEXT("Desert"), TEXT("Forest"),
		TEXT("Volcanic"), TEXT("Ice"), TEXT("Frozen"), TEXT("Ammonia"),
		TEXT("Metal"), TEXT("Carbon"), TEXT("SuperEarth"), TEXT("Lava"),
		TEXT("Metallic"), TEXT("Nordic"), TEXT("Tundra"), TEXT("HighMountain"),
		TEXT("Sand"), TEXT("Oasis"), TEXT("Archipelago"), TEXT("Pangea"),
		TEXT("Rogue"), TEXT("Exoplanet"), TEXT("Unknown")
	};
	static_assert(UE_ARRAY_COUNT(LegacyNames) == APSPlanetTypes::LegacyLastValue + 1);
	for (int32 Id = 0; Id < UE_ARRAY_COUNT(LegacyNames); ++Id)
		TestEqual(TEXT("Every legacy ID keeps its serialized name"),
			Enum->GetNameStringByValue(Id), FString(LegacyNames[Id]));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(APSPlanetPresetExtensionTests::Added); ++Index)
		TestEqual(TEXT("New types are append-only"), static_cast<int32>(
			APSPlanetPresetExtensionTests::Added[Index]), 31 + Index);
	TestEqual(TEXT("Iteration limit tracks reflected MAX"),
		Enum->GetMaxEnumValue(), static_cast<int64>(APSPlanetTypes::LastValue + 1));
	TestEqual(TEXT("Rocky display label"),
		Enum->GetDisplayNameTextByValue(0).ToString(), FString(TEXT("Rocky Planet")));
	TestEqual(TEXT("Terrestrial display label"),
		Enum->GetDisplayNameTextByValue(1).ToString(), FString(TEXT("Terrestrial Planet")));
	TestEqual(TEXT("Dwarf display label is unchanged"),
		Enum->GetDisplayNameTextByValue(7).ToString(), FString(TEXT("Dwarf Planet")));
	TestEqual(TEXT("Legacy Exoplanet is not silently relabelled"),
		Enum->GetDisplayNameTextByValue(29).ToString(), FString(TEXT("Exoplanet")));
	TestFalse(TEXT("Legacy Exoplanet is not offered for new selection"),
		APSPlanetTypes::IsSelectable(EPlanetType::Exoplanet));
	TestFalse(TEXT("Unknown is not offered for new selection"),
		APSPlanetTypes::IsSelectable(EPlanetType::Unknown));
	TestTrue(TEXT("Legacy Exoplanet remains resolvable"),
		UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(EPlanetType::Exoplanet));
	TMap<EAPSPlanetSurfaceArchetype, int32> Counts;
	for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		TestTrue(TEXT("All-type iteration contains valid IDs"), Enum->IsValidEnumValue(Value));
		if (APSPlanetTypes::IsSelectable(Type)
			&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type))
			++Counts.FindOrAdd(UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(Type));
	}
	TestEqual(TEXT("Nine solid-world families remain"), Counts.Num(), 9);
	for (const auto& Pair : Counts)
		TestTrue(TEXT("Every solid family has at least three selectable presets"), Pair.Value >= 3);
	TestEqual(TEXT("Basalt is Rocky"), UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(
		EPlanetType::Basalt), EAPSPlanetSurfaceArchetype::Rocky);
	TestEqual(TEXT("Savanna is Living"), UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(
		EPlanetType::Savanna), EAPSPlanetSurfaceArchetype::Biosphere);
	for (EPlanetType Type : {EPlanetType::Sulfur, EPlanetType::Crystal})
		TestEqual(TEXT("New chemical family"), UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(
			Type), EAPSPlanetSurfaceArchetype::ExoticChemical);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetPresetSnapshotTest,
	"APS.PlanetSurface.PresetExtension.SnapshotAndLegacyIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetPresetSnapshotTest::RunTest(const FString& Parameters)
{
	for (EPlanetType Type : {EPlanetType::Exoplanet, EPlanetType::Unknown,
		EPlanetType::Basalt, EPlanetType::Savanna, EPlanetType::Sulfur, EPlanetType::Crystal})
	{
		UGeneratedWorld* Model = NewObject<UGeneratedWorld>();
		Model->PlanetType = Type;
		Model->GenerationSeed = 146938;
		Model->PlanetSurfaceSeed = 146938;
		Model->SurfaceFeatureScale = 2.85;
		Model->SurfaceReliefScale = 1.85;
		FAPSPreviewBodyEditOverride Edit;
		Edit.PlanetType = Type;
		Edit.SurfaceSeed = 146938;
		Edit.RadiusKm = 6450.0;
		Model->PreviewBodyEditOverrides.Add(TEXT("SYS0/S0/P2"), Edit);
		UGameSave* Save = NewObject<UGameSave>();
		Save->SaveFormatVersion = APSWorldSaveSnapshot::LatestSaveFormatVersion;
		Save->GeneratedWorldsDataArray.Add(Model->SaveWorldData());
		if (!TestTrue(TEXT("Capture reflected snapshot"),
			APSWorldSaveSnapshot::Capture(Model, Save->GeneratedWorldModelData))) continue;
		UGeneratedWorld* Restored = APSWorldSaveSnapshot::Restore(
			Save, GetTransientPackage(), TEXT("APS_PRESET_MEMORY_ONLY"));
		if (!TestNotNull(TEXT("Restore reflected snapshot"), Restored)) continue;
		TestEqual(TEXT("Exact type survives, with no Exoplanet migration"), Restored->PlanetType, Type);
		TestEqual(TEXT("Surface seed survives"), Restored->PlanetSurfaceSeed, 146938);
		TestEqual(TEXT("Feature control survives"), Restored->SurfaceFeatureScale, 2.85);
		TestEqual(TEXT("Relief control survives"), Restored->SurfaceReliefScale, 1.85);
		const FAPSPreviewBodyEditOverride* RestoredEdit =
			Restored->PreviewBodyEditOverrides.Find(TEXT("SYS0/S0/P2"));
		if (TestNotNull(TEXT("Nested override survives"), RestoredEdit))
		{
			TestEqual(TEXT("Nested type survives"), RestoredEdit->PlanetType, Type);
			TestEqual(TEXT("Nested radius survives"), RestoredEdit->RadiusKm, 6450.0);
		}
		Save->GeneratedWorldModelData.Reset();
		Restored = APSWorldSaveSnapshot::Restore(
			Save, GetTransientPackage(), TEXT("APS_PRESET_MEMORY_ONLY"));
		if (TestNotNull(TEXT("Restore legacy summary"), Restored))
			TestEqual(TEXT("Legacy summary does not repurpose Exoplanet"), Restored->PlanetType, Type);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetPresetMorphologyTest,
	"APS.PlanetSurface.PresetExtension.GeometryNotPalette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetPresetMorphologyTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetPresetMorphology;
	for (uint8 Value = 0; Value <= APSPlanetTypes::LegacyLastValue; ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		TestFalse(TEXT("Old type cannot enter appended geometry"), HasDedicatedMorphology(Type));
		TestEqual(TEXT("Old resolved profile receives exactly zero delta"),
			HeightDelta(Type, 0.17, 0.43, -0.21, 0.9, 1.0, 1.0), 0.0);
	}
	TArray<TArray<double>> Fields;
	for (EPlanetType Type : APSPlanetPresetExtensionTests::Added)
	{
		TArray<double>& Field = Fields.AddDefaulted_GetRef();
		double Energy = 0.0;
		for (int32 I = 0; I < 96; ++I)
		{
			const double Regional = -0.5 + (I % 12) / 11.0;
			const double Ridge = ((I * 7) % 19) / 18.0;
			const double Cell = -1.0 + ((I * 13) % 23) / 11.0;
			const double Delta = HeightDelta(Type, Regional, Ridge, Cell, 0.9, 1.0, 1.0);
			Field.Add(Delta);
			Energy += FMath::Abs(Delta);
			TestTrue(TEXT("Bounded finite dedicated displacement"),
				FMath::IsFinite(Delta) && FMath::Abs(Delta) <= 0.060);
			TestEqual(TEXT("Dedicated shape repeats exactly"), Delta,
				HeightDelta(Type, Regional, Ridge, Cell, 0.9, 1.0, 1.0));
		}
		TestTrue(TEXT("Every added type changes geometry"), Energy > 0.01);
	}
	for (int32 A = 0; A < Fields.Num(); ++A)
	for (int32 B = A + 1; B < Fields.Num(); ++B)
	{
		double Difference = 0.0;
		for (int32 I = 0; I < Fields[A].Num(); ++I)
			Difference += FMath::Abs(Fields[A][I] - Fields[B][I]);
		TestTrue(TEXT("New morphologies are pairwise different"), Difference > 0.01);
	}

	// Hold every resolved numeric field and palette constant: only Type differs.
	// This catches a disconnected helper or a colour-only implementation.
	FAPSResolvedPlanetSurfaceProfile Base;
	Base.PlanetType = EPlanetType::Rocky;
	Base.Archetype = EAPSPlanetSurfaceArchetype::Rocky;
	Base.LiquidType = EAPSPlanetLiquidType::None;
	Base.LandCoverage = 1.0f;
	Base.NoiseScale = 600.0f;
	Base.NoiseIntensity = 900000.0f;
	Base.TerrainPatternStrength = 0.9f;
	Base.TerrainSeed = 146938;
	Base.BiomeSeed = 146939;
	Base.PaletteSeed = 146940;
	CustomNoise Noise(146938);
	Noise.SetSeed(146939);
	Noise.SetSeed(146938);
	for (double Scale : {1.0, 0.001883535})
	for (EPlanetType Type : APSPlanetPresetExtensionTests::Added)
	{
		FAPSResolvedPlanetSurfaceProfile Candidate = Base;
		Candidate.PlanetType = Type;
		double Difference = 0.0;
		for (int32 I = 0; I < 48; ++I)
		{
			const double Z = 1.0 - 2.0 * (I + 0.5) / 48.0;
			const double R = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			const double Angle = I * 2.39996322972865332;
			const double Radius = 645000000.0 * Scale;
			const DVector Position(R * FMath::Cos(Angle) * Radius,
				R * FMath::Sin(Angle) * Radius, Z * Radius);
			DVector P0, P1;
			const FNoiseData Before = UAPSWorldScapePlanetNoise::SampleResolvedProfile(
				Base, Noise, Position, DVector(0.0, 0.0, 0.0), Base.NoiseScale,
				Base.NoiseIntensity * Scale, Radius, Z, P0);
			const FNoiseData After = UAPSWorldScapePlanetNoise::SampleResolvedProfile(
				Candidate, Noise, Position, DVector(0.0, 0.0, 0.0), Base.NoiseScale,
				Base.NoiseIntensity * Scale, Radius, Z, P1);
			Difference += FMath::Abs(After.Height - Before.Height) / Scale;
		}
		TestTrue(TEXT("PLANET and physical samplers consume new geometry"), Difference > 1.0);
	}
	return true;
}
#endif
