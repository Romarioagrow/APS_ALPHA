#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"

// New preset geometry only. Keep legacy noise, climate, palette and modifier
// paths byte-for-byte independent of these additive, deterministic shapes.
namespace APSPlanetPresetMorphology
{
	inline constexpr bool HasDedicatedMorphology(EPlanetType Type)
	{
		return Type == EPlanetType::Basalt || Type == EPlanetType::Savanna
			|| Type == EPlanetType::Sulfur || Type == EPlanetType::Crystal;
	}

	inline double SoftTerrace(double Regional)
	{
		const double Bands = FMath::Clamp(Regional + 0.5, 0.0, 1.0) * 4.0;
		const double Floor = static_cast<double>(FMath::FloorToInt(Bands));
		return (Floor + FMath::SmoothStep(0.30, 0.70, Bands - Floor)) * 0.25 - 0.5;
	}

	inline double HeightDelta(EPlanetType Type, double Regional, double Ridges,
		double Cellular, double Strength, double LandMask, double DeepLandMask)
	{
		// Return before evaluating any shape for all historical type IDs.
		if (!HasDedicatedMorphology(Type)) return 0.0;
		const double Pattern = FMath::Clamp(Strength, 0.0, 1.0);
		const double Land = FMath::Clamp(LandMask, 0.0, 1.0);
		const double Inland = FMath::Clamp(DeepLandMask, 0.0, 1.0);
		const double Ridge = FMath::Clamp(Ridges, 0.0, 1.0);
		const double Cell = FMath::Clamp(FMath::Abs(Cellular), 0.0, 1.0);
		switch (Type)
		{
		case EPlanetType::Basalt:
			// Flood-basalt benches separated by narrow, non-emissive fissures.
			return (SoftTerrace(Regional) * 0.050
				- FMath::Pow(Ridge, 5.0) * 0.012) * Pattern * Inland;
		case EPlanetType::Savanna:
		{
			// Broad low plateaux dissected by smoothly incised drainage corridors.
			const double Drainage = FMath::Pow(
				FMath::Max(0.0, 1.0 - FMath::Abs(Ridge * 2.0 - 1.0)), 4.0);
			const double Plateau = FMath::SmoothStep(0.14, 0.35, Regional);
			return (Regional * 0.010 + Plateau * 0.009 - Drainage * 0.015)
				* Pattern * Inland;
		}
		case EPlanetType::Sulfur:
		{
			// Flat evaporite/vent depressions, raised rims, terraced dry deposits.
			const double Basin = 1.0 - FMath::SmoothStep(0.18, 0.50, Cell);
			const double Rim = FMath::SmoothStep(0.20, 0.32, Cell)
				* (1.0 - FMath::SmoothStep(0.32, 0.48, Cell));
			return (-Basin * 0.025 + Rim * 0.014
				+ SoftTerrace(Regional) * 0.009) * Pattern * Land;
		}
		case EPlanetType::Crystal:
		{
			// Clustered angular crests, rather than the legacy Exoplanet's broad
			// alien ridge/region mixture. No geometric change is assigned to ID 29.
			const double Facet = FMath::Max(
				0.0, 1.0 - FMath::Abs(Ridge * 2.0 - 1.0) * 3.0);
			const double Cluster = FMath::SmoothStep(0.30, 0.64, Regional + 0.5);
			return (Facet * FMath::Square(1.0 - Cell) * Cluster * 0.048
				- FMath::Pow(Ridge, 5.0) * 0.010) * Pattern * Inland;
		}
		default:
			return 0.0;
		}
	}
}
