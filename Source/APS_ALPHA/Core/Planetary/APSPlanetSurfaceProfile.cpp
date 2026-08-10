#include "APSPlanetSurfaceProfile.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Planetary/PlanetAtmosphere.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSPlanetSurface
{
	constexpr double EarthRadiusKm = 6371.0;

	FLinearColor SRGB(uint8 R, uint8 G, uint8 B)
	{
		return FLinearColor::FromSRGBColor(FColor(R, G, B));
	}

	float Draw(FRandomStream& Stream, const FVector2D& Range)
	{
		return Stream.FRandRange(static_cast<float>(Range.X), static_cast<float>(Range.Y));
	}

	int32 DeriveSeed(int32 Seed, uint32 Salt)
	{
		const uint32 Hash = HashCombine(GetTypeHash(Seed), Salt);
		return 1 + static_cast<int32>(Hash % static_cast<uint32>(MAX_int32 - 1));
	}

	void AddModifier(int64& Mask, EAPSPlanetSurfaceModifier Modifier)
	{
		if (Modifier != EAPSPlanetSurfaceModifier::None)
		{
			Mask |= int64{1} << static_cast<uint8>(Modifier);
		}
	}

	FAPSPlanetSurfacePalette MakePalette(
		const FLinearColor& Coast, const FLinearColor& Lowland, const FLinearColor& MidLowland,
		const FLinearColor& Highland, const FLinearColor& Dryland, const FLinearColor& Peak,
		const FLinearColor& Slope, const FLinearColor& Emissive)
	{
		FAPSPlanetSurfacePalette Result;
		Result.Coast = Coast;
		Result.Lowland = Lowland;
		Result.MidLowland = MidLowland;
		Result.Highland = Highland;
		Result.Dryland = Dryland;
		Result.Peak = Peak;
		Result.Slope = Slope;
		Result.Emissive = Emissive;
		return Result;
	}

	FLinearColor JitterColor(const FLinearColor& Color, FRandomStream& Stream, float HueDegrees)
	{
		FLinearColor HSV = Color.LinearRGBToHSV();
		HSV.R = FMath::Fmod(HSV.R + Stream.FRandRange(-HueDegrees, HueDegrees) + 360.0f, 360.0f);
		HSV.G = FMath::Clamp(HSV.G * Stream.FRandRange(0.9f, 1.1f), 0.0f, 1.0f);
		HSV.B = FMath::Clamp(HSV.B * Stream.FRandRange(0.88f, 1.12f), 0.0f, 1.0f);
		return HSV.HSVToLinearRGB();
	}

	void JitterPalette(FAPSPlanetSurfacePalette& Palette, int32 Seed, float HueDegrees)
	{
		FRandomStream Stream(Seed);
		Palette.Coast = JitterColor(Palette.Coast, Stream, HueDegrees);
		Palette.Lowland = JitterColor(Palette.Lowland, Stream, HueDegrees);
		Palette.MidLowland = JitterColor(Palette.MidLowland, Stream, HueDegrees);
		Palette.Highland = JitterColor(Palette.Highland, Stream, HueDegrees);
		Palette.Dryland = JitterColor(Palette.Dryland, Stream, HueDegrees);
		Palette.Peak = JitterColor(Palette.Peak, Stream, HueDegrees * 0.35f);
		Palette.Slope = JitterColor(Palette.Slope, Stream, HueDegrees * 0.5f);
	}

	float ApplyCenteredSurfaceControl(
		float AuthoredValue, double RawControl, double MinimumControl, double MaximumControl,
		float MinimumValue, float MaximumValue)
	{
		// A direct multiplier reaches the final clamp very early (for example a
		// roughness of 0.9 stops responding above 1.11). Interpolate from the
		// authored value toward each safe physical bound instead. The default 1.0
		// remains an exact identity while every non-saturated slider step remains
		// monotonic across the complete UI range.
		const double Control = FMath::Clamp(RawControl, MinimumControl, MaximumControl);
		const float Baseline = FMath::Clamp(AuthoredValue, MinimumValue, MaximumValue);
		if (Control < 1.0)
		{
			const float Alpha = static_cast<float>(
				(1.0 - Control) / FMath::Max(1.0 - MinimumControl, UE_DOUBLE_SMALL_NUMBER));
			return FMath::Lerp(Baseline, MinimumValue, FMath::Clamp(Alpha, 0.0f, 1.0f));
		}
		if (Control > 1.0)
		{
			const float Alpha = static_cast<float>(
				(Control - 1.0) / FMath::Max(MaximumControl - 1.0, UE_DOUBLE_SMALL_NUMBER));
			return FMath::Lerp(Baseline, MaximumValue, FMath::Clamp(Alpha, 0.0f, 1.0f));
		}
		return Baseline;
	}

	void ApplyPresetProfile(FAPSResolvedPlanetSurfaceProfile& P)
	{
		// These adjustments intentionally happen after catalog lookup. A catalog owns
		// the broad family/assets; the selected subtype owns its recognisable shape,
		// climate and palette. Values remain conservative enough for WorldScape LODs.
		switch (P.PlanetType)
		{
		case EPlanetType::Rocky:
			P.LandCoverage = 1.0f;
			P.CraterStrength = FMath::Max(P.CraterStrength, 0.72f);
			P.CellularFrequencyMultiplier = 0.9f;
			P.TerrainPatternStrength = 0.18f;
			P.Palette = MakePalette(SRGB(112, 91, 74), SRGB(78, 69, 62), SRGB(119, 101, 84), SRGB(137, 116, 95),
				SRGB(158, 126, 91), SRGB(187, 178, 164), SRGB(66, 59, 54), FLinearColor::Black);
			break;

		case EPlanetType::Dwarf:
			P.LandCoverage = 1.0f;
			P.NoiseScale *= 1.22f;
			P.NoiseIntensity *= 0.82f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.30f);
			P.HillStrength = FMath::Min(P.HillStrength, 0.20f);
			P.CraterStrength = FMath::Max(P.CraterStrength, 1.38f);
			P.CellularFrequencyMultiplier = 0.32f;
			P.TerrainPatternStrength = 0.96f;
			P.Palette = MakePalette(SRGB(82, 77, 78), SRGB(54, 52, 55), SRGB(91, 86, 88), SRGB(119, 111, 108),
				SRGB(139, 122, 110), SRGB(178, 171, 164), SRGB(42, 41, 44), FLinearColor::Black);
			break;

		case EPlanetType::Terrestrial:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.52f, 0.64f);
			P.ContinentalFrequencyMultiplier = 0.86f;
			P.RegionalFrequencyMultiplier = 1.05f;
			P.ClimatePatchStrength = 0.16f;
			P.BiomeContrast = FMath::Max(P.BiomeContrast, 1.12f);
			P.Palette = MakePalette(SRGB(184, 162, 105), SRGB(45, 92, 48), SRGB(65, 125, 57), SRGB(112, 116, 78),
				SRGB(165, 132, 75), SRGB(228, 232, 226), SRGB(76, 73, 65), FLinearColor::Black);
			break;

		case EPlanetType::Pangea:
			P.LandCoverage = FMath::Max(P.LandCoverage, 0.84f);
			P.ContinentalFrequencyMultiplier = 0.38f;
			P.RegionalFrequencyMultiplier = 0.64f;
			P.RidgeFrequencyMultiplier = 0.72f;
			P.TerrainPatternStrength = 0.72f;
			P.Humidity = FMath::Min(P.Humidity, 0.52f);
			P.Palette = MakePalette(SRGB(181, 148, 91), SRGB(66, 105, 52), SRGB(91, 121, 55), SRGB(133, 112, 69),
				SRGB(190, 142, 69), SRGB(218, 218, 202), SRGB(91, 75, 59), FLinearColor::Black);
			break;

		case EPlanetType::Nordic:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.58f, 0.68f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.66f);
			P.RidgeFrequencyMultiplier = 1.42f;
			P.CellularFrequencyMultiplier = 1.3f;
			P.TerrainPatternStrength = 0.48f;
			P.ClimatePatchStrength = 0.24f;
			P.Palette = MakePalette(SRGB(129, 151, 146), SRGB(35, 74, 61), SRGB(55, 96, 75), SRGB(93, 111, 98),
				SRGB(121, 116, 85), SRGB(234, 240, 239), SRGB(61, 74, 72), FLinearColor::Black);
			break;

		case EPlanetType::SuperEarth:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.64f, 0.76f);
			P.NoiseIntensity = FMath::Max(P.NoiseIntensity, 1350000.0f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.78f);
			P.ContinentalFrequencyMultiplier = 0.68f;
			P.RidgeFrequencyMultiplier = 0.82f;
			P.TerrainPatternStrength = 0.42f;
			P.Palette = MakePalette(SRGB(176, 151, 92), SRGB(38, 83, 52), SRGB(55, 116, 65), SRGB(104, 114, 75),
				SRGB(151, 112, 61), SRGB(232, 230, 217), SRGB(66, 66, 59), FLinearColor::Black);
			break;

		case EPlanetType::HighMountain:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.70f, 0.82f);
			P.NoiseIntensity = FMath::Max(P.NoiseIntensity, 1650000.0f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 1.18f);
			P.HillStrength = FMath::Min(P.HillStrength, 0.34f);
			P.RidgeFrequencyMultiplier = 0.58f;
			P.TerrainPatternStrength = 0.92f;
			P.Palette = MakePalette(SRGB(139, 126, 99), SRGB(55, 73, 49), SRGB(76, 91, 55), SRGB(111, 104, 87),
				SRGB(137, 113, 78), SRGB(241, 239, 232), SRGB(60, 58, 58), FLinearColor::Black);
			break;

		case EPlanetType::Ocean:
			// Predominantly oceanic, but retain sparse volcanic island peaks.
			// The abyssal-basin pattern has a deliberate negative height bias, so
			// author the pre-erosion area slightly above the visible island target.
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.09f, 0.13f);
			P.NoiseIntensity *= 0.58f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.22f);
			P.ContinentalFrequencyMultiplier = 0.55f;
			P.RegionalFrequencyMultiplier = 0.72f;
			P.TerrainPatternStrength = 0.34f;
			P.OceanLevel = 0.024f;
			P.Palette = MakePalette(SRGB(211, 195, 142), SRGB(28, 84, 72), SRGB(42, 111, 83), SRGB(79, 111, 89),
				SRGB(116, 109, 71), SRGB(221, 232, 226), SRGB(48, 71, 68), FLinearColor::Black);
			break;

		case EPlanetType::Water:
			// Continental shelves consume some marginal coast vertices. Keep the
			// resolved area biased upward so the rendered land band remains 30-45%.
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.38f, 0.48f);
			P.ContinentalFrequencyMultiplier = 0.78f;
			P.RegionalFrequencyMultiplier = 0.92f;
			P.TerrainPatternStrength = 0.38f;
			P.OceanLevel = 0.016f;
			P.Palette = MakePalette(SRGB(224, 203, 139), SRGB(48, 102, 63), SRGB(66, 132, 74), SRGB(101, 123, 88),
				SRGB(146, 126, 76), SRGB(229, 234, 225), SRGB(67, 81, 73), FLinearColor::Black);
			break;

		case EPlanetType::Archipelago:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.20f, 0.28f);
			P.ContinentalFrequencyMultiplier = 1.72f;
			P.RegionalFrequencyMultiplier = 1.35f;
			P.RidgeFrequencyMultiplier = 0.74f;
			P.TerrainPatternStrength = 0.88f;
			P.OceanLevel = 0.014f;
			P.Palette = MakePalette(SRGB(236, 211, 143), SRGB(35, 116, 67), SRGB(55, 151, 75), SRGB(89, 126, 79),
				SRGB(156, 134, 77), SRGB(229, 236, 226), SRGB(59, 79, 67), FLinearColor::Black);
			break;

		case EPlanetType::Forest:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.60f, 0.72f);
			P.Biomass = FMath::Max(P.Biomass, 0.94f);
			P.Biodiversity = FMath::Max(P.Biodiversity, 0.78f);
			P.Humidity = FMath::Max(P.Humidity, 0.86f);
			P.BiomeContrast = FMath::Max(P.BiomeContrast, 1.38f);
			P.ClimatePatchStrength = 0.52f;
			P.TerrainPatternStrength = 0.24f;
			P.Palette = MakePalette(SRGB(104, 91, 54), SRGB(12, 55, 25), SRGB(24, 92, 34), SRGB(58, 104, 43),
				SRGB(116, 111, 55), SRGB(211, 225, 207), SRGB(42, 57, 39), FLinearColor::Black);
			break;

		case EPlanetType::Oasis:
			P.LiquidType = EAPSPlanetLiquidType::Water;
			// Sparse wet basins, not an almost completely dry sphere. The terrain
			// signal lifts coast-adjacent samples, hence the lower authored datum.
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.88f, 0.91f);
			P.Humidity = FMath::Clamp(P.Humidity, 0.20f, 0.34f);
			P.Biomass = FMath::Max(P.Biomass, 0.42f);
			P.ClimatePatchStrength = 0.82f;
			P.ContinentalFrequencyMultiplier = 0.72f;
			P.TerrainPatternStrength = 0.52f;
			P.OceanLevel = 0.005f;
			P.Palette = MakePalette(SRGB(206, 167, 88), SRGB(105, 113, 54), SRGB(51, 124, 48), SRGB(154, 132, 67),
				SRGB(221, 175, 84), SRGB(213, 195, 155), SRGB(103, 72, 45), FLinearColor::Black);
			break;

		case EPlanetType::Greenhouse:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.28f);
			P.CraterStrength = FMath::Min(P.CraterStrength, 0.04f);
			P.HillStrength = FMath::Max(P.HillStrength, 0.55f);
			P.RegionalFrequencyMultiplier = 0.62f;
			P.TerrainPatternStrength = 0.42f;
			P.Palette = MakePalette(SRGB(207, 125, 55), SRGB(139, 73, 42), SRGB(181, 94, 45), SRGB(122, 69, 53),
				SRGB(229, 151, 67), SRGB(219, 178, 118), SRGB(83, 50, 43), FLinearColor::Black);
			break;

		case EPlanetType::Desert:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.54f);
			P.HillStrength = FMath::Min(P.HillStrength, 0.36f);
			P.CellularFrequencyMultiplier = 0.72f;
			P.TerrainPatternStrength = 0.74f;
			P.Palette = MakePalette(SRGB(183, 119, 59), SRGB(151, 84, 43), SRGB(193, 116, 54), SRGB(124, 75, 50),
				SRGB(225, 159, 74), SRGB(205, 174, 127), SRGB(89, 57, 45), FLinearColor::Black);
			break;

		case EPlanetType::Sand:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.10f);
			P.HillStrength = FMath::Max(P.HillStrength, 0.86f);
			P.DetailFrequencyMultiplier = 1.9f;
			P.RegionalFrequencyMultiplier = 1.35f;
			P.TerrainPatternStrength = 1.0f;
			P.Palette = MakePalette(SRGB(219, 174, 94), SRGB(187, 126, 58), SRGB(225, 166, 79), SRGB(168, 111, 59),
				SRGB(239, 195, 104), SRGB(225, 203, 160), SRGB(119, 78, 47), FLinearColor::Black);
			break;

		case EPlanetType::Ice:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.NoiseIntensity *= 0.58f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.25f);
			P.CraterStrength = FMath::Min(P.CraterStrength, 0.18f);
			P.DetailFrequencyMultiplier = 0.62f;
			P.TerrainPatternStrength = 0.38f;
			P.Palette = MakePalette(SRGB(193, 221, 229), SRGB(139, 183, 199), SRGB(184, 218, 226), SRGB(126, 157, 174),
				SRGB(211, 231, 233), SRGB(248, 252, 252), SRGB(94, 126, 144), FLinearColor::Black);
			break;

		case EPlanetType::Frozen:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.72f);
			P.CraterStrength = FMath::Max(P.CraterStrength, 0.38f);
			P.RidgeFrequencyMultiplier = 0.68f;
			P.CellularFrequencyMultiplier = 1.5f;
			P.TerrainPatternStrength = 0.82f;
			P.Palette = MakePalette(SRGB(145, 181, 194), SRGB(76, 111, 132), SRGB(124, 164, 181), SRGB(78, 100, 120),
				SRGB(176, 202, 210), SRGB(235, 245, 247), SRGB(54, 75, 91), FLinearColor::Black);
			break;

		case EPlanetType::Tundra:
			P.LiquidType = EAPSPlanetLiquidType::None;
			// The spherical fBm CDF retains a small highland bias on this low-frequency
			// family; cap the authored macro field so visible dry terrain stays <=95%.
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.82f, 0.84f);
			P.MountainStrength = FMath::Clamp(P.MountainStrength, 0.36f, 0.58f);
			P.Humidity = FMath::Clamp(P.Humidity, 0.30f, 0.48f);
			P.LatitudeClimateStrength = 1.55f;
			P.ClimatePatchStrength = 0.22f;
			P.TerrainPatternStrength = 0.48f;
			P.Palette = MakePalette(SRGB(141, 149, 131), SRGB(76, 87, 65), SRGB(103, 111, 78), SRGB(119, 111, 91),
				SRGB(153, 137, 101), SRGB(229, 235, 228), SRGB(73, 77, 68), FLinearColor::Black);
			break;

		case EPlanetType::Rogue:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = 1.0f;
			P.CraterStrength = FMath::Max(P.CraterStrength, 1.08f);
			P.MountainStrength = FMath::Clamp(P.MountainStrength, 0.32f, 0.52f);
			P.CellularFrequencyMultiplier = 0.62f;
			P.TerrainPatternStrength = 0.62f;
			P.Palette = MakePalette(SRGB(70, 79, 90), SRGB(31, 37, 47), SRGB(48, 57, 70), SRGB(61, 66, 76),
				SRGB(83, 83, 86), SRGB(155, 168, 177), SRGB(26, 29, 36), FLinearColor::Black);
			break;

		case EPlanetType::Volcanic:
			P.LandCoverage = FMath::Max(P.LandCoverage, 0.88f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.95f);
			P.RidgeFrequencyMultiplier = 0.72f;
			P.CellularFrequencyMultiplier = 0.72f;
			P.TerrainPatternStrength = 0.86f;
			P.EmissiveStrength = FMath::Clamp(P.EmissiveStrength, 2.8f, 5.0f);
			P.OceanLevel = 0.006f;
			P.Palette = MakePalette(SRGB(62, 28, 19), SRGB(33, 29, 28), SRGB(69, 45, 38), SRGB(103, 63, 46),
				SRGB(139, 78, 42), SRGB(168, 120, 91), SRGB(23, 20, 20), SRGB(255, 74, 8));
			break;

		case EPlanetType::Melted:
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.38f, 0.48f);
			P.NoiseIntensity *= 0.72f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.38f);
			P.ContinentalFrequencyMultiplier = 0.62f;
			P.TerrainPatternStrength = 0.58f;
			P.EmissiveStrength = FMath::Clamp(P.EmissiveStrength, 5.5f, 8.0f);
			P.OceanLevel = 0.020f;
			P.Palette = MakePalette(SRGB(74, 26, 15), SRGB(43, 26, 24), SRGB(82, 42, 29), SRGB(116, 60, 36),
				SRGB(167, 77, 28), SRGB(197, 118, 61), SRGB(28, 18, 18), SRGB(255, 47, 2));
			break;

		case EPlanetType::Lava:
			// Lava-sea fissures and calderas lower a sizeable part of the nominal
			// crust. This is the pre-caldera coverage that yields 18-30% dry crust.
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.33f, 0.41f);
			P.MountainStrength = FMath::Clamp(P.MountainStrength, 0.48f, 0.68f);
			P.RegionalFrequencyMultiplier = 1.28f;
			P.RidgeFrequencyMultiplier = 1.55f;
			P.TerrainPatternStrength = 1.0f;
			P.EmissiveStrength = FMath::Max(P.EmissiveStrength, 9.0f);
			P.OceanLevel = 0.027f;
			P.Palette = MakePalette(SRGB(48, 18, 13), SRGB(21, 19, 20), SRGB(53, 29, 25), SRGB(82, 42, 31),
				SRGB(131, 55, 25), SRGB(151, 88, 54), SRGB(15, 14, 15), SRGB(255, 31, 0));
			break;

		case EPlanetType::Metal:
			P.LandCoverage = 1.0f;
			P.Metallic = FMath::Clamp(P.Metallic, 0.68f, 0.82f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.72f);
			P.CraterStrength = FMath::Max(P.CraterStrength, 1.02f);
			P.CellularFrequencyMultiplier = 0.38f;
			P.TerrainPatternStrength = 0.96f;
			P.Palette = MakePalette(SRGB(104, 57, 38), SRGB(77, 57, 49), SRGB(130, 74, 49), SRGB(105, 75, 61),
				SRGB(164, 82, 43), SRGB(184, 157, 137), SRGB(55, 47, 44), FLinearColor::Black);
			break;

		case EPlanetType::Metallic:
			P.LandCoverage = 1.0f;
			P.Metallic = FMath::Max(P.Metallic, 0.94f);
			P.Roughness = FMath::Min(P.Roughness, 0.34f);
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.34f);
			P.CraterStrength = FMath::Min(P.CraterStrength, 0.14f);
			P.CellularFrequencyMultiplier = 1.85f;
			P.TerrainPatternStrength = 0.84f;
			P.Palette = MakePalette(SRGB(120, 132, 141), SRGB(72, 83, 92), SRGB(132, 146, 154), SRGB(91, 102, 110),
				SRGB(162, 171, 174), SRGB(220, 224, 223), SRGB(52, 60, 66), FLinearColor::Black);
			break;

		case EPlanetType::Carbon:
			P.LandCoverage = 1.0f;
			P.Metallic = FMath::Clamp(P.Metallic * 0.45f, 0.14f, 0.34f);
			P.Roughness = FMath::Max(P.Roughness, 0.84f);
			P.MountainStrength = FMath::Max(P.MountainStrength, 0.82f);
			P.RidgeFrequencyMultiplier = 1.75f;
			P.TerrainPatternStrength = 0.92f;
			P.Palette = MakePalette(SRGB(41, 40, 43), SRGB(19, 21, 24), SRGB(45, 45, 50), SRGB(64, 59, 67),
				SRGB(81, 69, 72), SRGB(140, 136, 143), SRGB(17, 18, 21), FLinearColor::Black);
			break;

		case EPlanetType::Ammonia:
			P.LiquidType = EAPSPlanetLiquidType::Ammonia;
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.44f, 0.56f);
			P.NoiseIntensity *= 0.72f;
			P.MountainStrength = FMath::Min(P.MountainStrength, 0.38f);
			P.ContinentalFrequencyMultiplier = 0.76f;
			P.TerrainPatternStrength = 0.58f;
			P.ClimatePatchStrength = 0.26f;
			P.OceanLevel = 0.017f;
			P.Palette = MakePalette(SRGB(196, 216, 172), SRGB(91, 135, 117), SRGB(146, 188, 151), SRGB(91, 121, 115),
				SRGB(194, 199, 137), SRGB(231, 238, 210), SRGB(61, 89, 82), FLinearColor::Black);
			break;

		case EPlanetType::Exoplanet:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.76f, 0.90f);
			P.ContinentalFrequencyMultiplier = 1.32f;
			P.RegionalFrequencyMultiplier = 0.58f;
			P.RidgeFrequencyMultiplier = 1.62f;
			P.TerrainPatternStrength = 0.92f;
			P.ClimatePatchStrength = 0.48f;
			P.Palette = MakePalette(SRGB(115, 147, 123), SRGB(66, 61, 103), SRGB(112, 79, 128), SRGB(63, 103, 111),
				SRGB(161, 151, 84), SRGB(205, 217, 178), SRGB(48, 50, 72), FLinearColor::Black);
			break;

		case EPlanetType::Unknown:
			P.LiquidType = EAPSPlanetLiquidType::None;
			P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.58f, 0.74f);
			P.ContinentalFrequencyMultiplier = 1.48f;
			P.CellularFrequencyMultiplier = 0.48f;
			P.RidgeFrequencyMultiplier = 1.9f;
			P.TerrainPatternStrength = 1.0f;
			P.ClimatePatchStrength = 0.62f;
			P.Palette = MakePalette(SRGB(105, 87, 129), SRGB(41, 54, 82), SRGB(69, 104, 104), SRGB(87, 63, 105),
				SRGB(143, 116, 72), SRGB(196, 192, 181), SRGB(39, 38, 58), FLinearColor::Black);
			break;

		default:
			break;
		}
	}
}

EAPSPlanetSurfaceArchetype UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(EPlanetType PlanetType)
{
	switch (PlanetType)
	{
	case EPlanetType::Terrestrial:
	case EPlanetType::Pangea:
	case EPlanetType::Nordic:
	case EPlanetType::SuperEarth:
	case EPlanetType::HighMountain:
		return EAPSPlanetSurfaceArchetype::Temperate;

	case EPlanetType::Ocean:
	case EPlanetType::Water:
	case EPlanetType::Archipelago:
		return EAPSPlanetSurfaceArchetype::Oceanic;

	case EPlanetType::Forest:
	case EPlanetType::Oasis:
		return EAPSPlanetSurfaceArchetype::Biosphere;

	case EPlanetType::Greenhouse:
	case EPlanetType::Desert:
	case EPlanetType::Sand:
		return EAPSPlanetSurfaceArchetype::Desert;

	case EPlanetType::Ice:
	case EPlanetType::Frozen:
	case EPlanetType::Tundra:
	case EPlanetType::Rogue:
		return EAPSPlanetSurfaceArchetype::Cryogenic;

	case EPlanetType::Volcanic:
	case EPlanetType::Melted:
	case EPlanetType::Lava:
		return EAPSPlanetSurfaceArchetype::Magmatic;

	case EPlanetType::Metal:
	case EPlanetType::Metallic:
	case EPlanetType::Carbon:
		return EAPSPlanetSurfaceArchetype::Metallic;

	case EPlanetType::Ammonia:
	case EPlanetType::Exoplanet:
		return EAPSPlanetSurfaceArchetype::ExoticChemical;

	case EPlanetType::Dwarf:
	case EPlanetType::Rocky:
	case EPlanetType::Unknown:
	default:
		return EAPSPlanetSurfaceArchetype::Rocky;
	}
}

int64 UAPSPlanetSurfaceProfileResolver::GetPresetModifierMask(EPlanetType PlanetType)
{
	using namespace APSPlanetSurface;
	int64 Mask = 0;
	switch (PlanetType)
	{
	case EPlanetType::SuperEarth: AddModifier(Mask, EAPSPlanetSurfaceModifier::SuperEarth); break;
	case EPlanetType::Dwarf: AddModifier(Mask, EAPSPlanetSurfaceModifier::Dwarf); break;
	case EPlanetType::Rogue: AddModifier(Mask, EAPSPlanetSurfaceModifier::Rogue); break;
	case EPlanetType::HighMountain: AddModifier(Mask, EAPSPlanetSurfaceModifier::HighMountain); break;
	case EPlanetType::Pangea: AddModifier(Mask, EAPSPlanetSurfaceModifier::Pangea); break;
	case EPlanetType::Archipelago:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::Archipelago);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::ArchipelagoChains);
		break;
	case EPlanetType::Nordic: AddModifier(Mask, EAPSPlanetSurfaceModifier::Nordic); break;
	case EPlanetType::Oasis:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::Oasis);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::OasisWetlands);
		break;
	case EPlanetType::Greenhouse:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::Greenhouse);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::AtmosphericErosion);
		break;
	case EPlanetType::Ocean: AddModifier(Mask, EAPSPlanetSurfaceModifier::AbyssalBasins); break;
	case EPlanetType::Water: AddModifier(Mask, EAPSPlanetSurfaceModifier::ContinentalShelves); break;
	case EPlanetType::Forest: AddModifier(Mask, EAPSPlanetSurfaceModifier::ForestCanopy); break;
	case EPlanetType::Ice: AddModifier(Mask, EAPSPlanetSurfaceModifier::IceSheets); break;
	case EPlanetType::Frozen: AddModifier(Mask, EAPSPlanetSurfaceModifier::GlacialRifts); break;
	case EPlanetType::Tundra: AddModifier(Mask, EAPSPlanetSurfaceModifier::TundraBands); break;
	case EPlanetType::Melted: AddModifier(Mask, EAPSPlanetSurfaceModifier::LavaSeas); break;
	case EPlanetType::Volcanic: AddModifier(Mask, EAPSPlanetSurfaceModifier::CalderaFields); break;
	case EPlanetType::Lava:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::LavaSeas);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::VolcanicFissures);
		break;
	case EPlanetType::Metal:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::CalderaFields);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::MesaFields);
		break;
	case EPlanetType::Metallic: AddModifier(Mask, EAPSPlanetSurfaceModifier::MetallicPlates); break;
	case EPlanetType::Carbon: AddModifier(Mask, EAPSPlanetSurfaceModifier::CarbonRidges); break;
	case EPlanetType::Ammonia: AddModifier(Mask, EAPSPlanetSurfaceModifier::ChemicalBands); break;
	case EPlanetType::Exoplanet:
		AddModifier(Mask, EAPSPlanetSurfaceModifier::ChemicalBands);
		AddModifier(Mask, EAPSPlanetSurfaceModifier::AlienTerrain);
		break;
	case EPlanetType::Sand: AddModifier(Mask, EAPSPlanetSurfaceModifier::SandDunes); break;
	case EPlanetType::Desert: AddModifier(Mask, EAPSPlanetSurfaceModifier::MesaFields); break;
	case EPlanetType::Unknown: AddModifier(Mask, EAPSPlanetSurfaceModifier::AlienTerrain); break;
	default: break;
	}
	return Mask;
}

bool UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(EPlanetType PlanetType)
{
	return PlanetType != EPlanetType::GasGiant
		&& PlanetType != EPlanetType::HotGiant
		&& PlanetType != EPlanetType::IceGiant;
}

FAPSPlanetSurfaceArchetypeDefinition UAPSPlanetSurfaceProfileResolver::GetNativeDefinition(
	EAPSPlanetSurfaceArchetype Archetype)
{
	using namespace APSPlanetSurface;
	FAPSPlanetSurfaceArchetypeDefinition D;

	switch (Archetype)
	{
	case EAPSPlanetSurfaceArchetype::Temperate:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Temperate;
		D.LiquidType = EAPSPlanetLiquidType::Water;
		D.NoiseScale = FVector2D(440, 700);
		D.NoiseIntensity = FVector2D(780000, 1250000);
		D.LandCoverage = FVector2D(0.5, 0.74);
		D.MountainStrength = FVector2D(0.3, 0.65);
		D.HillStrength = FVector2D(0.25, 0.5);
		D.CraterStrength = FVector2D(0.0, 0.12);
		D.BaseTemperature = FVector2D(0.45, 0.68);
		D.BaseHumidity = FVector2D(0.38, 0.68);
		D.Roughness = FVector2D(0.72, 0.9);
		D.BiomeContrast = FVector2D(0.85, 1.2);
		D.Palette = MakePalette(SRGB(128, 110, 76), SRGB(55, 89, 45), SRGB(73, 112, 52), SRGB(116, 105, 79),
			SRGB(155, 126, 76), SRGB(222, 226, 224), SRGB(82, 76, 68), FLinearColor::Black);
		break;

	case EAPSPlanetSurfaceArchetype::Oceanic:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Temperate;
		D.LiquidType = EAPSPlanetLiquidType::Water;
		D.NoiseScale = FVector2D(150, 420);
		D.NoiseIntensity = FVector2D(320000, 780000);
		D.LandCoverage = FVector2D(0.06, 0.38);
		D.MountainStrength = FVector2D(0.15, 0.42);
		D.HillStrength = FVector2D(0.1, 0.32);
		D.CraterStrength = FVector2D(0.0, 0.05);
		D.BaseTemperature = FVector2D(0.42, 0.68);
		D.BaseHumidity = FVector2D(0.7, 0.96);
		D.Roughness = FVector2D(0.62, 0.82);
		D.Palette = MakePalette(SRGB(196, 179, 124), SRGB(44, 96, 69), SRGB(61, 122, 75), SRGB(99, 111, 83),
			SRGB(143, 129, 80), SRGB(221, 229, 226), SRGB(69, 78, 73), FLinearColor::Black);
		break;

	case EAPSPlanetSurfaceArchetype::Biosphere:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Temperate;
		D.LiquidType = EAPSPlanetLiquidType::Water;
		D.NoiseScale = FVector2D(480, 760);
		D.NoiseIntensity = FVector2D(650000, 1050000);
		D.LandCoverage = FVector2D(0.55, 0.82);
		D.MountainStrength = FVector2D(0.2, 0.5);
		D.HillStrength = FVector2D(0.28, 0.55);
		D.CraterStrength = FVector2D(0, 0.04);
		D.BaseTemperature = FVector2D(0.5, 0.72);
		D.BaseHumidity = FVector2D(0.68, 0.95);
		D.Roughness = FVector2D(0.75, 0.92);
		D.BiomeContrast = FVector2D(1.05, 1.45);
		D.Palette = MakePalette(SRGB(110, 93, 59), SRGB(27, 70, 35), SRGB(39, 105, 45), SRGB(78, 112, 54),
			SRGB(139, 121, 69), SRGB(220, 228, 220), SRGB(65, 66, 53), FLinearColor::Black);
		break;

	case EAPSPlanetSurfaceArchetype::Desert:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Temperate;
		D.LiquidType = EAPSPlanetLiquidType::None;
		D.NoiseScale = FVector2D(330, 820);
		D.NoiseIntensity = FVector2D(480000, 1250000);
		D.LandCoverage = FVector2D(0.92, 1.0);
		D.MountainStrength = FVector2D(0.18, 0.58);
		D.HillStrength = FVector2D(0.35, 0.7);
		D.CraterStrength = FVector2D(0.02, 0.2);
		D.BaseTemperature = FVector2D(0.62, 0.92);
		D.BaseHumidity = FVector2D(0.01, 0.2);
		D.Roughness = FVector2D(0.7, 0.95);
		D.Palette = MakePalette(SRGB(167, 117, 62), SRGB(177, 119, 59), SRGB(202, 148, 76), SRGB(137, 91, 53),
			SRGB(221, 179, 101), SRGB(192, 173, 142), SRGB(93, 68, 51), FLinearColor::Black);
		D.PaletteHueVariationDegrees = 12.0f;
		break;

	case EAPSPlanetSurfaceArchetype::Cryogenic:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;
		D.LiquidType = EAPSPlanetLiquidType::None;
		D.NoiseScale = FVector2D(360, 760);
		D.NoiseIntensity = FVector2D(480000, 950000);
		D.LandCoverage = FVector2D(0.95, 1.0);
		D.MountainStrength = FVector2D(0.2, 0.55);
		D.HillStrength = FVector2D(0.12, 0.38);
		D.CraterStrength = FVector2D(0.12, 0.55);
		D.BaseTemperature = FVector2D(0.02, 0.24);
		D.BaseHumidity = FVector2D(0.15, 0.55);
		D.Roughness = FVector2D(0.38, 0.72);
		D.Palette = MakePalette(SRGB(151, 179, 187), SRGB(115, 145, 157), SRGB(165, 194, 201), SRGB(112, 128, 139),
			SRGB(197, 213, 215), SRGB(236, 243, 244), SRGB(80, 98, 109), FLinearColor::Black);
		break;

	case EAPSPlanetSurfaceArchetype::Magmatic:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Magmatic;
		D.LiquidType = EAPSPlanetLiquidType::Lava;
		D.NoiseScale = FVector2D(270, 650);
		D.NoiseIntensity = FVector2D(850000, 1800000);
		D.LandCoverage = FVector2D(0.58, 0.94);
		D.MountainStrength = FVector2D(0.5, 0.95);
		D.HillStrength = FVector2D(0.25, 0.6);
		D.CraterStrength = FVector2D(0.05, 0.28);
		D.BaseTemperature = FVector2D(0.88, 1.0);
		D.BaseHumidity = FVector2D(0, 0.04);
		D.Roughness = FVector2D(0.62, 0.9);
		D.EmissiveStrength = FVector2D(2.5, 9.0);
		D.Palette = MakePalette(SRGB(55, 25, 17), SRGB(36, 30, 29), SRGB(62, 45, 40), SRGB(86, 60, 48),
			SRGB(122, 75, 45), SRGB(149, 112, 90), SRGB(25, 22, 22), SRGB(255, 58, 5));
		break;

	case EAPSPlanetSurfaceArchetype::Metallic:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;
		D.LiquidType = EAPSPlanetLiquidType::None;
		D.NoiseScale = FVector2D(520, 980);
		D.NoiseIntensity = FVector2D(650000, 1350000);
		D.LandCoverage = FVector2D(1, 1);
		D.MountainStrength = FVector2D(0.3, 0.72);
		D.HillStrength = FVector2D(0.12, 0.38);
		D.CraterStrength = FVector2D(0.25, 0.72);
		D.BaseTemperature = FVector2D(0.2, 0.72);
		D.BaseHumidity = FVector2D(0, 0.08);
		D.Roughness = FVector2D(0.38, 0.72);
		D.Metallic = FVector2D(0.55, 0.92);
		D.Palette = MakePalette(SRGB(74, 62, 55), SRGB(92, 78, 68), SRGB(120, 101, 84), SRGB(80, 78, 76),
			SRGB(143, 105, 72), SRGB(176, 174, 169), SRGB(55, 53, 52), FLinearColor::Black);
		break;

	case EAPSPlanetSurfaceArchetype::ExoticChemical:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;
		D.LiquidType = EAPSPlanetLiquidType::Ammonia;
		D.NoiseScale = FVector2D(420, 900);
		D.NoiseIntensity = FVector2D(520000, 1100000);
		D.LandCoverage = FVector2D(0.65, 0.96);
		D.MountainStrength = FVector2D(0.18, 0.58);
		D.HillStrength = FVector2D(0.18, 0.52);
		D.CraterStrength = FVector2D(0.08, 0.42);
		D.BaseTemperature = FVector2D(0.08, 0.4);
		D.BaseHumidity = FVector2D(0.3, 0.8);
		D.Roughness = FVector2D(0.45, 0.82);
		D.Palette = MakePalette(SRGB(151, 174, 156), SRGB(103, 139, 126), SRGB(156, 190, 171), SRGB(91, 116, 110),
			SRGB(192, 200, 157), SRGB(224, 230, 211), SRGB(65, 87, 83), FLinearColor::Black);
		D.PaletteHueVariationDegrees = 18.0f;
		break;

	case EAPSPlanetSurfaceArchetype::Rocky:
	default:
		D.MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;
		D.LiquidType = EAPSPlanetLiquidType::None;
		D.NoiseScale = FVector2D(580, 980);
		D.NoiseIntensity = FVector2D(720000, 1350000);
		D.LandCoverage = FVector2D(1, 1);
		D.MountainStrength = FVector2D(0.3, 0.7);
		D.HillStrength = FVector2D(0.15, 0.4);
		D.CraterStrength = FVector2D(0.35, 0.9);
		D.BaseTemperature = FVector2D(0.15, 0.65);
		D.BaseHumidity = FVector2D(0, 0.12);
		D.Roughness = FVector2D(0.78, 1.0);
		D.Palette = MakePalette(SRGB(105, 88, 75), SRGB(87, 77, 68), SRGB(117, 102, 88), SRGB(127, 112, 98),
			SRGB(142, 116, 88), SRGB(172, 163, 151), SRGB(67, 62, 58), FLinearColor::Black);
		break;
	}

	return D;
}

FAPSResolvedPlanetSurfaceProfile UAPSPlanetSurfaceProfileResolver::ResolveForBody(
	const APlanetaryBody* Body, const UAPSPlanetSurfaceCatalog* Catalog)
{
	using namespace APSPlanetSurface;
	FAPSResolvedPlanetSurfaceProfile P;
	if (!IsValid(Body))
	{
		return P;
	}

	P.PlanetType = Body->PlanetType;
	P.Archetype = GetArchetypeForType(P.PlanetType);
	P.ModifierMask = GetPresetModifierMask(P.PlanetType);

	FAPSPlanetSurfaceArchetypeDefinition D = GetNativeDefinition(P.Archetype);
	if (IsValid(Catalog))
	{
		if (const FAPSPlanetSurfaceArchetypeDefinition* Override = Catalog->Archetypes.Find(P.Archetype))
		{
			D = *Override;
		}
	}

	int32 BaseSeed = Body->WorldScapeSeed;
	if (BaseSeed <= 0)
	{
		BaseSeed = DeriveSeed(GetTypeHash(Body->GetFName()), GetTypeHash(Body->GetActorLocation()));
	}
	P.TerrainSeed = DeriveSeed(BaseSeed, 0x54455252u); // TERR
	P.BiomeSeed = DeriveSeed(BaseSeed, 0x42494F4Du); // BIOM
	P.PaletteSeed = DeriveSeed(BaseSeed, 0x50414C45u); // PALE
	FRandomStream TerrainRandom(P.TerrainSeed);
	FRandomStream BiomeRandom(P.BiomeSeed);

	P.MaterialFamily = D.MaterialFamily;
	P.LiquidType = D.LiquidType;
	P.NoiseScale = Draw(TerrainRandom, D.NoiseScale);
	P.NoiseIntensity = Draw(TerrainRandom, D.NoiseIntensity);
	P.LandCoverage = Draw(TerrainRandom, D.LandCoverage);
	P.MountainStrength = Draw(TerrainRandom, D.MountainStrength);
	P.HillStrength = Draw(TerrainRandom, D.HillStrength);
	P.CraterStrength = Draw(TerrainRandom, D.CraterStrength);
	P.Temperature = Draw(BiomeRandom, D.BaseTemperature);
	P.Humidity = Draw(BiomeRandom, D.BaseHumidity);
	P.Roughness = Draw(BiomeRandom, D.Roughness);
	P.Metallic = Draw(BiomeRandom, D.Metallic);
	P.EmissiveStrength = Draw(BiomeRandom, D.EmissiveStrength);
	P.BiomeContrast = Draw(BiomeRandom, D.BiomeContrast);
	P.Palette = D.Palette;
	P.Foliage = D.Foliage;

	FPlanetAtmosphere Atmosphere = Body->PlanetAtmosphere;
	const float HumidityPercent = Atmosphere.Humidity > 0.0f
		? Atmosphere.Humidity
		: Atmosphere.CalculateHumidity(Atmosphere.HumidityLevel);
	const float PressurePa = Atmosphere.AtmosphericPressure > 0.0f
		? Atmosphere.AtmosphericPressure
		: Atmosphere.CalculateAtmosphericPressure(Atmosphere.PressureLevel);

	// PlanetaryProceduralGenerator stores equilibrium temperature in Kelvin. The
	// profile's 0..1 climate channel historically spans roughly -120..180 C, so
	// convert Kelvin into that same interval instead of treating it as Celsius.
	constexpr float ProfileMinimumKelvin = 153.15f; // -120 C
	constexpr float ProfileTemperatureSpanKelvin = 300.0f;
	const float ModelTemperature = FMath::Clamp(
		(static_cast<float>(Body->Temperature) - ProfileMinimumKelvin) / ProfileTemperatureSpanKelvin,
		0.0f, 1.0f);
	if (Body->Temperature != 0)
	{
		P.Temperature = FMath::Lerp(P.Temperature, ModelTemperature, 0.72f);
	}
	P.Humidity = FMath::Lerp(P.Humidity, FMath::Clamp(HumidityPercent / 100.0f, 0.0f, 1.0f), 0.65f);
	P.Biomass = FMath::Clamp(Body->PlanetBiosphere.Biomass / 10000.0f, 0.0f, 1.0f);
	P.Biodiversity = FMath::Clamp(Body->PlanetBiosphere.BiodiversityIndexValue / 7.0f, 0.0f, 1.0f);
	P.SeismicActivity = FMath::Clamp(Body->PlanetGeosphere.SeismicActivity / 7.0f, 0.0f, 1.0f);
	P.CrustThickness = FMath::Clamp(Body->PlanetGeosphere.CrustThickness / 100.0f, 0.0f, 1.0f);
	P.AtmosphericPressure = FMath::Clamp(PressurePa / 101325.0f, 0.0f, 4.0f);

	const double RadiusKm = FMath::Max(Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM));
	if (RadiusKm > EarthRadiusKm * 1.3)
	{
		AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::SuperEarth);
	}
	else if (RadiusKm > 0.0 && RadiusKm < EarthRadiusKm * 0.55)
	{
		AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::Dwarf);
	}
	if (P.SeismicActivity >= 0.55f) AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::ActiveGeology);
	if (P.AtmosphericPressure >= 1.25f) AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::DenseAtmosphere);
	if (P.Biomass >= 0.35f || P.Biodiversity >= 0.45f) AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::RichBiosphere);
	if (P.SeismicActivity + P.CrustThickness > 1.25f && TerrainRandom.FRand() < 0.35f)
	{
		AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::HighMountain);
	}

	if (P.HasModifier(EAPSPlanetSurfaceModifier::SuperEarth))
	{
		P.NoiseIntensity *= 1.2f;
		P.MountainStrength *= 1.12f;
		P.CraterStrength *= 0.75f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Dwarf))
	{
		P.CraterStrength *= 1.4f;
		P.HillStrength *= 0.82f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Rogue))
	{
		P.Temperature *= 0.28f;
		P.Humidity *= 0.65f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::HighMountain))
	{
		P.MountainStrength *= 1.45f;
		P.NoiseIntensity *= 1.3f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Pangea))
	{
		P.LandCoverage = FMath::Max(P.LandCoverage, 0.72f);
		P.NoiseScale *= 0.78f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Archipelago))
	{
		P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.14f, 0.34f);
		P.NoiseScale *= 1.25f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Nordic))
	{
		P.Temperature *= 0.62f;
		P.Humidity = FMath::Max(P.Humidity, 0.48f);
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Oasis))
	{
		P.LiquidType = EAPSPlanetLiquidType::Water;
		P.LandCoverage = FMath::Max(P.LandCoverage, 0.86f);
		P.Humidity = FMath::Max(P.Humidity, 0.28f);
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::Greenhouse))
	{
		P.Temperature = FMath::Max(P.Temperature, 0.82f);
		P.Humidity *= 0.35f;
		AddModifier(P.ModifierMask, EAPSPlanetSurfaceModifier::DenseAtmosphere);
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::ActiveGeology))
	{
		P.MountainStrength *= 1.18f;
		P.NoiseIntensity *= 1.1f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::DenseAtmosphere))
	{
		P.CraterStrength *= 0.55f;
		P.HillStrength *= 1.08f;
	}
	if (P.HasModifier(EAPSPlanetSurfaceModifier::RichBiosphere))
	{
		P.Humidity = FMath::Max(P.Humidity, 0.5f);
		P.BiomeContrast *= 1.15f;
	}

	ApplyPresetProfile(P);

	// UI/model variation is deliberately applied only to numeric characteristics
	// after the planet-type profile has been resolved. The resolver remains the
	// sole owner of generator and material selection.
	P.NoiseScale = ApplyCenteredSurfaceControl(
		P.NoiseScale, Body->SurfaceFeatureScale, 0.25, 4.0, 60.0f, 4000.0f);
	P.NoiseIntensity = ApplyCenteredSurfaceControl(
		P.NoiseIntensity, Body->SurfaceReliefScale, 0.25, 2.5, 200000.0f, 2600000.0f);
	P.LandCoverage = ApplyCenteredSurfaceControl(
		P.LandCoverage, Body->SurfaceLandCoverageScale, 0.25, 2.0, 0.02f, 1.0f);
	P.MountainStrength = ApplyCenteredSurfaceControl(
		P.MountainStrength, Body->SurfaceMountainScale, 0.0, 2.0, 0.0f, 1.5f);
	P.CraterStrength = ApplyCenteredSurfaceControl(
		P.CraterStrength, Body->SurfaceCraterScale, 0.0, 2.0, 0.0f, 1.5f);
	P.Roughness = ApplyCenteredSurfaceControl(
		P.Roughness, Body->SurfaceRoughnessScale, 0.25, 2.0, 0.08f, 1.0f);

	// A physical model can produce a temperature that conflicts with an explicit
	// surface subtype (for example, an Ice preset on a newly randomized orbit).
	// Preserve the physical response inside subtype-safe ceilings/floors so presets
	// remain visibly and semantically distinct while still varying below the bound.
	switch (P.PlanetType)
	{
	case EPlanetType::Rogue:
		P.Temperature = FMath::Min(P.Temperature, 0.10f);
		break;
	case EPlanetType::Frozen:
		P.Temperature = FMath::Min(P.Temperature, 0.16f);
		break;
	case EPlanetType::Ice:
		P.Temperature = FMath::Min(P.Temperature, 0.22f);
		break;
	case EPlanetType::Tundra:
		P.Temperature = FMath::Min(P.Temperature, 0.32f);
		break;
	case EPlanetType::Greenhouse:
		P.Temperature = FMath::Max(P.Temperature, 0.82f);
		break;
	default:
		break;
	}
	if (P.Archetype == EAPSPlanetSurfaceArchetype::Magmatic)
	{
		P.Temperature = FMath::Max(P.Temperature, 0.88f);
	}

	P.NoiseScale = FMath::Clamp(P.NoiseScale, 60.0f, 4000.0f);
	P.NoiseIntensity = FMath::Clamp(P.NoiseIntensity, 200000.0f, 2600000.0f);
	P.LandCoverage = FMath::Clamp(P.LandCoverage, 0.02f, 1.0f);
	P.MountainStrength = FMath::Clamp(P.MountainStrength, 0.0f, 1.5f);
	P.HillStrength = FMath::Clamp(P.HillStrength, 0.0f, 1.2f);
	P.CraterStrength = FMath::Clamp(P.CraterStrength, 0.0f, 1.5f);
	P.Roughness = FMath::Clamp(P.Roughness, 0.08f, 1.0f);
	P.Temperature = FMath::Clamp(P.Temperature, 0.0f, 1.0f);
	P.Humidity = FMath::Clamp(P.Humidity, 0.0f, 1.0f);
	P.ContinentalFrequencyMultiplier = FMath::Clamp(P.ContinentalFrequencyMultiplier, 0.25f, 2.5f);
	P.RegionalFrequencyMultiplier = FMath::Clamp(P.RegionalFrequencyMultiplier, 0.25f, 2.5f);
	P.DetailFrequencyMultiplier = FMath::Clamp(P.DetailFrequencyMultiplier, 0.25f, 2.5f);
	P.RidgeFrequencyMultiplier = FMath::Clamp(P.RidgeFrequencyMultiplier, 0.25f, 2.5f);
	P.CellularFrequencyMultiplier = FMath::Clamp(P.CellularFrequencyMultiplier, 0.25f, 2.5f);
	P.TerrainPatternStrength = FMath::Clamp(P.TerrainPatternStrength, 0.0f, 1.0f);
	P.ClimatePatchStrength = FMath::Clamp(P.ClimatePatchStrength, 0.0f, 1.0f);
	P.LatitudeClimateStrength = FMath::Clamp(P.LatitudeClimateStrength, 0.5f, 1.75f);
	P.OceanLevel = P.LiquidType == EAPSPlanetLiquidType::None
		? 0.0f
		: (P.OceanLevel > 0.0f ? FMath::Clamp(P.OceanLevel, 0.002f, 0.035f) : 0.0125f);
	JitterPalette(P.Palette, P.PaletteSeed, D.PaletteHueVariationDegrees);
	P.Palette.Emissive *= P.EmissiveStrength;
	return P;
}

void UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
	UMaterialInstanceDynamic* Material, const FAPSResolvedPlanetSurfaceProfile& Profile)
{
	if (!IsValid(Material)) return;

	Material->SetVectorParameterValue(TEXT("BottomColor"), Profile.Palette.Coast);
	Material->SetVectorParameterValue(TEXT("Color1"), Profile.Palette.Lowland);
	Material->SetVectorParameterValue(TEXT("Color2"), Profile.Palette.MidLowland);
	Material->SetVectorParameterValue(TEXT("Color3"), Profile.Palette.Highland);
	Material->SetVectorParameterValue(TEXT("Color4"), Profile.Palette.Dryland);
	Material->SetVectorParameterValue(TEXT("Color5"), Profile.Palette.Peak);
	Material->SetVectorParameterValue(TEXT("SlopeColor"), Profile.Palette.Slope);
	Material->SetVectorParameterValue(TEXT("Sedimentcolor"), Profile.Palette.Coast);
	Material->SetVectorParameterValue(TEXT("EmissiveColor"), Profile.Palette.Emissive);

	// The catalog's family MICs inherit the project-owned canonical WorldScape graph.
	// It consumes the same normalized height/climate payload as every generated LOD;
	// no proxy globe or second visual height field participates in this material.
	Material->SetScalarParameterValue(TEXT("HeightContrast"), Profile.BiomeContrast);
	Material->SetScalarParameterValue(TEXT("ContrastTemp"), Profile.BiomeContrast);
	Material->SetScalarParameterValue(TEXT("WarpedScale"),
		ResolveMaterialWarpScale(Profile.MaterialFamily));
	Material->SetScalarParameterValue(TEXT("ClimateBlend"),
		ResolveMaterialClimateBlend(Profile.Archetype));

	// The physical terrain profile also drives the presentation hierarchy. These
	// values never replace WorldScape displacement; they only keep palette identity
	// and non-periodic macro/meso/near shading coherent from orbit to gameplay.
	float PaletteGain = 1.0f;
	float PaletteLift = 0.003f;
	float PaletteSaturation = 1.05f;
	float PaletteContrast = 1.08f;
	float MacroDetailScaleCm = 8000000.0f;
	float MesoDetailScaleCm = 450000.0f;
	float NearDetailScaleCm = 1800.0f;
	float MacroColorStrength = 0.060f;
	float MesoColorStrength = 0.042f;
	float NearColorStrength = 0.006f;
	float DetailNormalStrength = 0.100f;
	float MesoRoughnessStrength = 0.020f;
	float DetailRoughnessStrength = 0.012f;
	float TerrainAmbientFill = 0.090f;
	float SlopeTintStrength = 0.20f;
	switch (Profile.Archetype)
	{
	case EAPSPlanetSurfaceArchetype::Temperate:
		PaletteGain = 1.00f;
		PaletteSaturation = 1.12f;
		PaletteContrast = 1.07f;
		MacroDetailScaleCm = 9000000.0f;
		MesoDetailScaleCm = 600000.0f;
		NearDetailScaleCm = 2200.0f;
		MacroColorStrength = 0.052f;
		MesoColorStrength = 0.038f;
		NearColorStrength = 0.007f;
		DetailNormalStrength = 0.095f;
		TerrainAmbientFill = 0.105f;
		SlopeTintStrength = 0.20f;
		break;
	case EAPSPlanetSurfaceArchetype::Oceanic:
		PaletteGain = 0.99f;
		PaletteSaturation = 1.10f;
		PaletteContrast = 1.06f;
		MacroDetailScaleCm = 12000000.0f;
		MesoDetailScaleCm = 700000.0f;
		NearDetailScaleCm = 2600.0f;
		MacroColorStrength = 0.040f;
		MesoColorStrength = 0.028f;
		NearColorStrength = 0.004f;
		DetailNormalStrength = 0.075f;
		TerrainAmbientFill = 0.095f;
		SlopeTintStrength = 0.14f;
		break;
	case EAPSPlanetSurfaceArchetype::Biosphere:
		PaletteGain = 1.00f;
		PaletteSaturation = 1.18f;
		PaletteContrast = 1.06f;
		MacroDetailScaleCm = 7500000.0f;
		MesoDetailScaleCm = 320000.0f;
		NearDetailScaleCm = 1600.0f;
		MacroColorStrength = 0.048f;
		MesoColorStrength = 0.034f;
		NearColorStrength = 0.008f;
		DetailNormalStrength = 0.110f;
		TerrainAmbientFill = 0.115f;
		SlopeTintStrength = 0.19f;
		break;
	case EAPSPlanetSurfaceArchetype::Desert:
		PaletteGain = 0.97f;
		PaletteSaturation = 1.10f;
		PaletteContrast = 1.07f;
		MacroDetailScaleCm = 11000000.0f;
		MesoDetailScaleCm = 900000.0f;
		NearDetailScaleCm = 2500.0f;
		MacroColorStrength = 0.052f;
		MesoColorStrength = 0.036f;
		NearColorStrength = 0.006f;
		DetailNormalStrength = 0.085f;
		TerrainAmbientFill = 0.100f;
		SlopeTintStrength = 0.23f;
		break;
	case EAPSPlanetSurfaceArchetype::Cryogenic:
		PaletteGain = 0.98f;
		PaletteLift = 0.0f;
		PaletteSaturation = 1.03f;
		PaletteContrast = 1.08f;
		MacroDetailScaleCm = 6000000.0f;
		MesoDetailScaleCm = 350000.0f;
		NearDetailScaleCm = 1600.0f;
		MacroColorStrength = 0.050f;
		MesoColorStrength = 0.040f;
		NearColorStrength = 0.004f;
		DetailNormalStrength = 0.105f;
		TerrainAmbientFill = 0.105f;
		SlopeTintStrength = 0.14f;
		break;
	case EAPSPlanetSurfaceArchetype::Magmatic:
		PaletteGain = 0.94f;
		PaletteSaturation = 1.12f;
		PaletteContrast = 1.08f;
		MacroDetailScaleCm = 5000000.0f;
		MesoDetailScaleCm = 250000.0f;
		NearDetailScaleCm = 1800.0f;
		MacroColorStrength = 0.050f;
		MesoColorStrength = 0.036f;
		NearColorStrength = 0.005f;
		DetailNormalStrength = 0.100f;
		TerrainAmbientFill = 0.045f;
		SlopeTintStrength = 0.23f;
		break;
	case EAPSPlanetSurfaceArchetype::Rocky:
		PaletteGain = 1.02f;
		PaletteLift = 0.004f;
		PaletteSaturation = 0.98f;
		PaletteContrast = 1.07f;
		MacroDetailScaleCm = 7000000.0f;
		MesoDetailScaleCm = 300000.0f;
		NearDetailScaleCm = 1800.0f;
		MacroColorStrength = 0.055f;
		MesoColorStrength = 0.040f;
		NearColorStrength = 0.005f;
		DetailNormalStrength = 0.110f;
		TerrainAmbientFill = 0.110f;
		SlopeTintStrength = 0.27f;
		break;
	case EAPSPlanetSurfaceArchetype::Metallic:
		PaletteGain = 1.06f;
		PaletteLift = 0.006f;
		PaletteSaturation = 0.94f;
		PaletteContrast = 1.05f;
		MacroDetailScaleCm = 10000000.0f;
		MesoDetailScaleCm = 500000.0f;
		NearDetailScaleCm = 2200.0f;
		MacroColorStrength = 0.044f;
		MesoColorStrength = 0.032f;
		NearColorStrength = 0.003f;
		DetailNormalStrength = 0.080f;
		MesoRoughnessStrength = 0.018f;
		TerrainAmbientFill = 0.100f;
		SlopeTintStrength = 0.18f;
		break;
	case EAPSPlanetSurfaceArchetype::ExoticChemical:
		PaletteGain = 1.00f;
		PaletteSaturation = 1.16f;
		PaletteContrast = 1.06f;
		MacroDetailScaleCm = 6000000.0f;
		MesoDetailScaleCm = 320000.0f;
		NearDetailScaleCm = 1900.0f;
		MacroColorStrength = 0.050f;
		MesoColorStrength = 0.036f;
		NearColorStrength = 0.006f;
		DetailNormalStrength = 0.090f;
		TerrainAmbientFill = 0.115f;
		SlopeTintStrength = 0.19f;
		break;
	default:
		break;
	}

	const float PatternResponse = FMath::Clamp(Profile.TerrainPatternStrength, 0.0f, 1.0f);
	const float RoughnessResponse = FMath::Clamp(Profile.Roughness, 0.08f, 1.0f);
	MacroColorStrength *= FMath::Lerp(0.90f, 1.18f, PatternResponse);
	MesoColorStrength *= FMath::Lerp(0.88f, 1.22f, PatternResponse);
	NearColorStrength *= FMath::Lerp(0.78f, 1.18f, RoughnessResponse);
	DetailNormalStrength *= FMath::Lerp(0.72f, 1.18f, RoughnessResponse);
	DetailRoughnessStrength *= FMath::Lerp(0.82f, 1.12f, RoughnessResponse);
	// Dry terrain has no liquid shell to supply broad visual separation. Bias its
	// material response toward real height/slope and regional structure, while the
	// centimetre-scale band becomes larger and quieter. This changes shading only;
	// the resolved WorldScape displacement, collision and single-root ownership stay
	// authoritative.
	if (Profile.LiquidType == EAPSPlanetLiquidType::None)
	{
		NearDetailScaleCm *= 1.15f;
		MacroColorStrength *= 1.06f;
		MesoColorStrength *= 1.05f;
		NearColorStrength *= 0.82f;
		DetailNormalStrength *= 0.90f;
		SlopeTintStrength = FMath::Min(SlopeTintStrength + 0.02f, 0.28f);
	}

	Material->SetScalarParameterValue(TEXT("PaletteGain"), PaletteGain);
	Material->SetScalarParameterValue(TEXT("PaletteLift"), PaletteLift);
	Material->SetScalarParameterValue(TEXT("PaletteSaturation"), PaletteSaturation);
	Material->SetScalarParameterValue(TEXT("PaletteContrast"), PaletteContrast);
	Material->SetScalarParameterValue(TEXT("MacroDetailScaleCm"), FMath::Clamp(
		MacroDetailScaleCm / FMath::Max(Profile.ContinentalFrequencyMultiplier, 0.25f),
		1500000.0f, 20000000.0f));
	Material->SetScalarParameterValue(TEXT("MesoDetailScaleCm"), FMath::Clamp(
		MesoDetailScaleCm / FMath::Max(Profile.RegionalFrequencyMultiplier, 0.25f),
		100000.0f, 1800000.0f));
	Material->SetScalarParameterValue(TEXT("NearDetailScaleCm"), FMath::Clamp(
		NearDetailScaleCm / FMath::Max(Profile.DetailFrequencyMultiplier, 0.25f),
		800.0f, 30000.0f));
	Material->SetScalarParameterValue(TEXT("MacroColorStrength"), MacroColorStrength);
	Material->SetScalarParameterValue(TEXT("MesoColorStrength"), MesoColorStrength);
	Material->SetScalarParameterValue(TEXT("NearColorStrength"), NearColorStrength);
	Material->SetScalarParameterValue(TEXT("DetailNormalStrength"), DetailNormalStrength);
	Material->SetScalarParameterValue(TEXT("MesoRoughnessStrength"), MesoRoughnessStrength);
	Material->SetScalarParameterValue(TEXT("DetailRoughnessStrength"), DetailRoughnessStrength);
	Material->SetScalarParameterValue(TEXT("TerrainAmbientFill"), TerrainAmbientFill);
	Material->SetScalarParameterValue(TEXT("SlopeTintStrength"), SlopeTintStrength);
	Material->SetScalarParameterValue(TEXT("MidVarient1Rough"), Profile.Roughness);
	Material->SetScalarParameterValue(TEXT("MidVarient2Rough"), Profile.Roughness);
	Material->SetScalarParameterValue(TEXT("MidVarient3Rough"), Profile.Roughness);

	// Retain generic PBR writes for any compatible project template variants; they
	// are harmless no-ops when a selected WorldScape graph does not expose them.
	Material->SetScalarParameterValue(TEXT("Roughness"), Profile.Roughness);
	Material->SetScalarParameterValue(TEXT("Metallic"), Profile.Metallic);
	Material->SetScalarParameterValue(TEXT("Specular"),
		FMath::Lerp(0.24f, 0.48f, Profile.Metallic));
}

float UAPSPlanetSurfaceProfileResolver::ResolveMaterialWarpScale(
	EAPSPlanetSurfaceMaterialFamily MaterialFamily)
{
	// 1.0 is the neutral authored scale of the canonical WorldScape graph.
	// Keep the mapping explicit so calibrated per-family values can be introduced
	// later without ever coupling material UVs back to procedural terrain frequency.
	constexpr float MinimumWarpScale = 0.75f;
	constexpr float MaximumWarpScale = 1.25f;
	float FamilyWarpScale = 1.0f;
	switch (MaterialFamily)
	{
	case EAPSPlanetSurfaceMaterialFamily::Temperate:
	case EAPSPlanetSurfaceMaterialFamily::Barren:
	case EAPSPlanetSurfaceMaterialFamily::Magmatic:
	default:
		FamilyWarpScale = 1.0f;
		break;
	}
	return FMath::Clamp(FamilyWarpScale, MinimumWarpScale, MaximumWarpScale);
}

float UAPSPlanetSurfaceProfileResolver::ResolveMaterialClimateBlend(
	EAPSPlanetSurfaceArchetype Archetype)
{
	// Climate is intentionally weaker than the five broad elevation transitions.
	// This keeps Pangea/desert/forest silhouettes readable while still allowing the
	// temperature and humidity channels to distinguish living and oceanic worlds.
	float Blend = 0.08f;
	switch (Archetype)
	{
	case EAPSPlanetSurfaceArchetype::Biosphere: Blend = 0.18f; break;
	case EAPSPlanetSurfaceArchetype::Temperate: Blend = 0.14f; break;
	case EAPSPlanetSurfaceArchetype::Oceanic: Blend = 0.12f; break;
	case EAPSPlanetSurfaceArchetype::ExoticChemical: Blend = 0.10f; break;
	case EAPSPlanetSurfaceArchetype::Desert:
	case EAPSPlanetSurfaceArchetype::Cryogenic: Blend = 0.08f; break;
	case EAPSPlanetSurfaceArchetype::Rocky:
	case EAPSPlanetSurfaceArchetype::Magmatic: Blend = 0.06f; break;
	case EAPSPlanetSurfaceArchetype::Metallic: Blend = 0.04f; break;
	default: break;
	}
	return FMath::Clamp(Blend, 0.04f, 0.20f);
}

uint32 UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(const FAPSResolvedPlanetSurfaceProfile& P)
{
	uint32 Signature = GetTypeHash(static_cast<uint8>(P.PlanetType));
	Signature = HashCombine(Signature, GetTypeHash(static_cast<uint8>(P.Archetype)));
	Signature = HashCombine(Signature, GetTypeHash(static_cast<uint8>(P.MaterialFamily)));
	Signature = HashCombine(Signature, GetTypeHash(static_cast<uint8>(P.LiquidType)));
	Signature = HashCombine(Signature, GetTypeHash(P.ModifierMask));
	Signature = HashCombine(Signature, GetTypeHash(P.TerrainSeed));
	Signature = HashCombine(Signature, GetTypeHash(P.BiomeSeed));
	Signature = HashCombine(Signature, GetTypeHash(P.PaletteSeed));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.NoiseScale * 100.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.NoiseIntensity)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.LandCoverage * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.MountainStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.HillStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.CraterStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Roughness * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.ContinentalFrequencyMultiplier * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.RegionalFrequencyMultiplier * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.DetailFrequencyMultiplier * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.RidgeFrequencyMultiplier * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.CellularFrequencyMultiplier * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.TerrainPatternStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.ClimatePatchStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.LatitudeClimateStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.OceanLevel * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Temperature * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Humidity * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Biomass * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Biodiversity * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.Metallic * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.EmissiveStrength * 10000.0f)));
	Signature = HashCombine(Signature, GetTypeHash(FMath::RoundToInt(P.BiomeContrast * 10000.0f)));
	Signature = HashCombine(Signature,
		GetTypeHash(static_cast<uint8>(P.Foliage.bEnabled)));
	Signature = HashCombine(Signature, GetTypeHash(P.Foliage.MaxCollections));
	Signature = HashCombine(Signature, GetTypeHash(P.Foliage.MaxTypesPerCollection));
	Signature = HashCombine(Signature,
		GetTypeHash(P.Foliage.MaxInstancesPerSectorPerCollection));
	Signature = HashCombine(Signature, GetTypeHash(P.Foliage.MaxClusterMeshesPerType));
	Signature = HashCombine(Signature,
		GetTypeHash(FMath::RoundToInt(P.Foliage.MinSectorSizeCm * 10.0f)));
	Signature = HashCombine(Signature,
		GetTypeHash(FMath::RoundToInt(P.Foliage.MaxCullDistanceMultiplier * 10000.0f)));
	Signature = HashCombine(Signature,
		GetTypeHash(FMath::RoundToInt(P.Foliage.MinimumBiomass * 10000.0f)));
	Signature = HashCombine(Signature,
		GetTypeHash(static_cast<uint8>(P.Foliage.bUseNoiseMask)));
	Signature = HashCombine(Signature,
		GetTypeHash(static_cast<uint8>(P.Foliage.bCastShadows)));
	Signature = HashCombine(Signature, GetTypeHash(P.Foliage.Collections.Num()));
	for (const TSoftObjectPtr<UWorldScapeFoliagesCollection>& Collection :
		P.Foliage.Collections)
	{
		// Ordered soft paths are part of profile identity without loading the assets.
		Signature = HashCombine(Signature,
			GetTypeHash(Collection.ToSoftObjectPath().ToString()));
	}
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Coast.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Lowland.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.MidLowland.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Highland.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Dryland.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Peak.ToFColor(false).DWColor()));
	Signature = HashCombine(Signature, GetTypeHash(P.Palette.Emissive.ToFColor(false).DWColor()));
	return Signature == 0 ? 1u : Signature;
}
