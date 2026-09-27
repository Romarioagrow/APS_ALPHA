#pragma once

#include "CoreMinimal.h"

/**
 * Deterministic, scale-free morphology used by the title-screen hero. Presentation
 * code supplies its own radius, proxy size and material policy; this file owns only
 * the astronomical shape and stays reusable for a later generated-world type.
 */
namespace APSGrandDesignGalaxy
{
	constexpr int32 SpiralArmCount = 4;
	constexpr double SpiralPopulationFraction = 7200.0 / 9200.0;
	constexpr double BulgePopulationFraction = 1500.0 / 9200.0;

	enum class EPopulation : uint8
	{
		SpiralArm,
		Bulge,
		Halo
	};

	struct FSample
	{
		FVector UnitPosition{FVector::ZeroVector};
		EPopulation Population{EPopulation::SpiralArm};
		double RadiusAlpha{0.0};
		float StyleA{0.0f};
		float StyleB{0.0f};
		float StyleC{0.0f};
		float StyleD{0.0f};
		float PointSeed{0.0f};
	};

	inline uint64 Mix64(uint64 Value)
	{
		Value ^= Value >> 30;
		Value *= 0xbf58476d1ce4e5b9ull;
		Value ^= Value >> 27;
		Value *= 0x94d049bb133111ebull;
		Value ^= Value >> 31;
		return Value;
	}

	inline FRandomStream MakeStream(const int32 Seed, const int64 StableIndex, const uint64 Salt)
	{
		const uint64 SeedBits = static_cast<uint64>(static_cast<uint32>(Seed)) << 32;
		const uint64 Mixed = Mix64(SeedBits ^ static_cast<uint64>(StableIndex) ^ Salt);
		return FRandomStream(FMath::Max(1, static_cast<int32>(Mixed & 0x7fffffffull)));
	}

	inline double BellNoise(FRandomStream& Random)
	{
		// Three draws form a bounded bell curve: no single tail can inflate bounds.
		return static_cast<double>(Random.GetFraction() + Random.GetFraction()
			+ Random.GetFraction()) - 1.5;
	}

	inline EPopulation SelectPopulation(const int32 Seed, const int64 StableIndex)
	{
		const uint64 Key = Mix64(
			(static_cast<uint64>(static_cast<uint32>(Seed)) << 32)
			^ static_cast<uint64>(StableIndex) ^ 0x4752414e445f504full); // GRAND_PO
		const double Selector = static_cast<double>(Key >> 11)
			* (1.0 / 9007199254740992.0);
		if (Selector < SpiralPopulationFraction)
		{
			return EPopulation::SpiralArm;
		}
		if (Selector < SpiralPopulationFraction + BulgePopulationFraction)
		{
			return EPopulation::Bulge;
		}
		return EPopulation::Halo;
	}

	inline void FillStyle(FSample& Sample, FRandomStream& Random)
	{
		Sample.StyleA = Random.GetFraction();
		Sample.StyleB = Random.GetFraction();
		Sample.StyleC = Random.GetFraction();
		Sample.StyleD = Random.GetFraction();
		Sample.PointSeed = Random.GetFraction();
	}

	inline FSample SampleSpiral(const int32 Seed, const int64 StableIndex)
	{
		FRandomStream Random = MakeStream(Seed, StableIndex, 0x4752414e445f4152ull); // GRAND_AR
		FSample Sample;
		Sample.Population = EPopulation::SpiralArm;
		Sample.RadiusAlpha = FMath::Pow(static_cast<double>(Random.GetFraction()), 0.62);
		const double Radius = 0.055 + Sample.RadiusAlpha * 0.945;
		const int32 ArmIndex = static_cast<int32>(static_cast<uint64>(StableIndex)
			% static_cast<uint64>(SpiralArmCount));
		const double ArmAngle = ArmIndex * UE_TWO_PI / SpiralArmCount;
		const double Scatter = BellNoise(Random)
			* FMath::Lerp(0.10, 0.42, Sample.RadiusAlpha);
		const double Angle = ArmAngle + Sample.RadiusAlpha * UE_TWO_PI * 1.72 + Scatter;
		const double EffectiveRadius = FMath::Max(Radius + BellNoise(Random)
			* FMath::Lerp(0.008, 0.035, Sample.RadiusAlpha), 0.0);
		const double Thickness = FMath::Lerp(0.012, 0.065, Sample.RadiusAlpha);
		Sample.UnitPosition = FVector(
			FMath::Cos(Angle) * EffectiveRadius,
			FMath::Sin(Angle) * EffectiveRadius * 0.78,
			BellNoise(Random) * Thickness);
		FillStyle(Sample, Random);
		return Sample;
	}

	inline FSample SampleBulge(const int32 Seed, const int64 StableIndex)
	{
		FRandomStream Random = MakeStream(Seed, StableIndex, 0x4752414e445f4255ull); // GRAND_BU
		FSample Sample;
		Sample.Population = EPopulation::Bulge;
		Sample.RadiusAlpha = FMath::Pow(static_cast<double>(Random.GetFraction()), 2.35);
		const double Angle = Random.GetFraction() * UE_TWO_PI;
		const double Radius = 0.31 * Sample.RadiusAlpha;
		Sample.UnitPosition = FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius * 0.72,
			BellNoise(Random) * 0.075 * (1.0 - Sample.RadiusAlpha * 0.55));
		FillStyle(Sample, Random);
		return Sample;
	}

	inline FSample SampleHalo(const int32 Seed, const int64 StableIndex)
	{
		FRandomStream Random = MakeStream(Seed, StableIndex, 0x4752414e445f4841ull); // GRAND_HA
		FSample Sample;
		Sample.Population = EPopulation::Halo;
		Sample.RadiusAlpha = FMath::Sqrt(static_cast<double>(Random.GetFraction()));
		const double Angle = Random.GetFraction() * UE_TWO_PI;
		const double Radius = FMath::Lerp(0.48, 1.12, Sample.RadiusAlpha);
		Sample.UnitPosition = FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius * 0.82,
			BellNoise(Random) * 0.18);
		FillStyle(Sample, Random);
		return Sample;
	}

	inline FSample SampleCatalog(const int32 Seed, const int64 StableIndex)
	{
		switch (SelectPopulation(Seed, StableIndex))
		{
		case EPopulation::Bulge: return SampleBulge(Seed, StableIndex);
		case EPopulation::Halo: return SampleHalo(Seed, StableIndex);
		case EPopulation::SpiralArm:
		default: return SampleSpiral(Seed, StableIndex);
		}
	}
}
