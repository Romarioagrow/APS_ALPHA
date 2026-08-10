#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace APSPlanetSurfaceProfileTests
{
	UWorld* CreateWorld()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.RequiresHitProxies(false)
			.CreatePhysicsScene(false)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.SetTransactional(false);
		return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
	}

	void DestroyWorld(UWorld*& World)
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	struct FSurfaceFieldStats
	{
		uint32 FieldHash = 0;
		double MeanHeight = 0.0;
		double HeightDeviation = 0.0;
		double HeightRange = 0.0;
		double MinimumMaterialHeight = 0.0;
		double MaximumMaterialHeight = 0.0;
		double PhysicalHeightRange = 0.0;
		double MeanWater = 0.0;
		double MeanTemperature = 0.0;
		double MeanHumidity = 0.0;
		double HumidityDeviation = 0.0;
		double MeanFoliage = 0.0;
	};

	struct FSurfaceCalibrationStats
	{
		double LandFraction = 0.0;
		double HeightP01 = 0.0;
		double HeightP50 = 0.0;
		double HeightP99 = 0.0;
		double ClampFraction = 0.0;
		double PaletteP10 = 0.0;
		double PaletteP50 = 0.0;
		double PaletteP90 = 0.0;
		double PaletteSaturationFraction = 0.0;
	};

	struct FLocalReliefStats
	{
		bool bFoundLand = false;
		int32 PatchCount = 0;
		// 8 km regional window (legacy names retained for the existing contract).
		double MinimumPatchRangeCm = 0.0;
		double MedianPatchRangeCm = 0.0;
		double MedianRmsSlope = 0.0;
		double MaximumSlope = 0.0;
		// 100 m walking window.
		double MinimumWalkPatchRangeCm = 0.0;
		double MedianWalkPatchRangeCm = 0.0;
		double MedianWalkRmsSlope = 0.0;
		double MaximumWalkSlope = 0.0;
		// 10 m foot-scale window. This is the smallest range a player can inspect
		// directly without relying on normal maps or material-only displacement.
		double MinimumFootPatchRangeCm = 0.0;
		double MedianFootPatchRangeCm = 0.0;
		double MedianFootRmsSlope = 0.0;
		double MaximumFootSlope = 0.0;
		// 250 m near-field window visible from the third-person camera.
		double MinimumNearPatchRangeCm = 0.0;
		double MedianNearPatchRangeCm = 0.0;
		double MedianNearRmsSlope = 0.0;
		double MaximumNearSlope = 0.0;
		// 1 km local-landform window.
		double MinimumLocalPatchRangeCm = 0.0;
		double MedianLocalPatchRangeCm = 0.0;
		double MedianLocalRmsSlope = 0.0;
		double MaximumLocalSlope = 0.0;
	};

	FSurfaceFieldStats SampleSurfaceFields(
		const FAPSResolvedPlanetSurfaceProfile& Profile,
		int32 SampleNoiseSeed = 0x2468ACE,
		int32 SampleCount = 72)
	{
		constexpr double PlanetScale = 120000000.0;
		constexpr double GoldenAngle = 2.39996322972865332;
		SampleCount = FMath::Max(SampleCount, 8);
		UAPSWorldScapePlanetNoise* Noise = NewObject<UAPSWorldScapePlanetNoise>();
		Noise->Configure(Profile);
		CustomNoise SampleNoise(SampleNoiseSeed);
		// WorldScape 5.4's constructor compares against an uninitialised Seed guard.
		// Two explicit transitions guarantee that the final permutation table exists.
		SampleNoise.SetSeed(SampleNoiseSeed + 1);
		SampleNoise.SetSeed(SampleNoiseSeed);

		FSurfaceFieldStats Result;
		Result.FieldHash = 0xA5F17E31u;
		double HeightSum = 0.0;
		double HeightSquaredSum = 0.0;
		double HumiditySquaredSum = 0.0;
		double MinHeight = TNumericLimits<double>::Max();
		double MaxHeight = -TNumericLimits<double>::Max();
		double MinPhysicalHeight = TNumericLimits<double>::Max();
		double MaxPhysicalHeight = -TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const double Z = 1.0 - 2.0 * (static_cast<double>(Index) + 0.5) / SampleCount;
			const double RingRadius = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			const double Azimuth = GoldenAngle * Index;
			const DVector Position(
				FMath::Cos(Azimuth) * RingRadius * PlanetScale,
				FMath::Sin(Azimuth) * RingRadius * PlanetScale,
				Z * PlanetScale);
			DVector NoisePosition;
			const FNoiseData Data = Noise->GetNoise(
				SampleNoise, Position, DVector(0.0), Profile.NoiseScale,
				Profile.NoiseIntensity, PlanetScale, false, Z, NoisePosition, FNoiseData(), true);

			HeightSum += Data.HeightNormalize;
			HeightSquaredSum += Data.HeightNormalize * Data.HeightNormalize;
			MinHeight = FMath::Min(MinHeight, Data.HeightNormalize);
			MaxHeight = FMath::Max(MaxHeight, Data.HeightNormalize);
			MinPhysicalHeight = FMath::Min(MinPhysicalHeight, Data.Height);
			MaxPhysicalHeight = FMath::Max(MaxPhysicalHeight, Data.Height);
			Result.MeanWater += Data.WaterMask;
			Result.MeanTemperature += Data.Temperature;
			Result.MeanHumidity += Data.Humidity;
			HumiditySquaredSum += Data.Humidity * Data.Humidity;
			Result.MeanFoliage += Data.FoliageMask;

			// Deliberately exclude PlanetType/profile identity. This hash represents
			// only sampled terrain and climate fields at fixed world directions.
			Result.FieldHash = HashCombine(Result.FieldHash,
				GetTypeHash(FMath::RoundToInt(Data.HeightNormalize * 100000.0)));
			Result.FieldHash = HashCombine(Result.FieldHash,
				GetTypeHash(FMath::RoundToInt(Data.WaterMask * 10000.0f)));
			Result.FieldHash = HashCombine(Result.FieldHash,
				GetTypeHash(FMath::RoundToInt(Data.Temperature * 10000.0f)));
			Result.FieldHash = HashCombine(Result.FieldHash,
				GetTypeHash(FMath::RoundToInt(Data.Humidity * 10000.0f)));
			Result.FieldHash = HashCombine(Result.FieldHash,
				GetTypeHash(FMath::RoundToInt(Data.FoliageMask * 10000.0f)));
		}

		const double InverseCount = 1.0 / SampleCount;
		Result.MeanHeight = HeightSum * InverseCount;
		Result.HeightDeviation = FMath::Sqrt(FMath::Max(
			0.0, HeightSquaredSum * InverseCount - Result.MeanHeight * Result.MeanHeight));
		Result.MinimumMaterialHeight = MinHeight;
		Result.MaximumMaterialHeight = MaxHeight;
		Result.HeightRange = MaxHeight - MinHeight;
		Result.PhysicalHeightRange = MaxPhysicalHeight - MinPhysicalHeight;
		Result.MeanWater *= InverseCount;
		Result.MeanTemperature *= InverseCount;
		Result.MeanHumidity *= InverseCount;
		Result.HumidityDeviation = FMath::Sqrt(FMath::Max(
			0.0, HumiditySquaredSum * InverseCount - Result.MeanHumidity * Result.MeanHumidity));
		Result.MeanFoliage *= InverseCount;
		return Result;
	}

	bool HasVectorParameter(const UMaterialInterface* Material, const TCHAR* ParameterName)
	{
		if (!IsValid(Material)) return false;
		const FName Name(ParameterName);
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Material->GetAllVectorParameterInfo(ParameterInfos, ParameterIds);
		return ParameterInfos.ContainsByPredicate([Name](const FMaterialParameterInfo& Info)
		{
			return Info.Name == Name;
		});
	}

	bool HasScalarParameter(const UMaterialInterface* Material, const TCHAR* ParameterName)
	{
		if (!IsValid(Material)) return false;
		const FName Name(ParameterName);
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
		return ParameterInfos.ContainsByPredicate([Name](const FMaterialParameterInfo& Info)
		{
			return Info.Name == Name;
		});
	}

	bool GetVectorParameter(
		const UMaterialInterface* Material, const TCHAR* ParameterName, FLinearColor& OutValue)
	{
		return IsValid(Material) && Material->GetVectorParameterValue(
			FHashedMaterialParameterInfo(FName(ParameterName)), OutValue);
	}

	bool GetScalarParameter(
		const UMaterialInterface* Material, const TCHAR* ParameterName, float& OutValue)
	{
		return IsValid(Material) && Material->GetScalarParameterValue(
			FHashedMaterialParameterInfo(FName(ParameterName)), OutValue);
	}

	bool HasOwnVectorOverride(const UMaterialInstance* Material, const TCHAR* ParameterName)
	{
		if (!IsValid(Material)) return false;
		const FName Name(ParameterName);
		return Material->VectorParameterValues.ContainsByPredicate(
			[Name](const FVectorParameterValue& Value)
			{
				return Value.ParameterInfo.Name == Name;
			});
	}

	bool HasOwnScalarOverride(const UMaterialInstance* Material, const TCHAR* ParameterName)
	{
		if (!IsValid(Material)) return false;
		const FName Name(ParameterName);
		return Material->ScalarParameterValues.ContainsByPredicate(
			[Name](const FScalarParameterValue& Value)
			{
				return Value.ParameterInfo.Name == Name;
			});
	}

	bool HasConnectedMaterialProperty(UMaterial* Material, EMaterialProperty Property)
	{
		if (!IsValid(Material)) return false;
		const FExpressionInput* Input = Material->GetExpressionInputForProperty(Property);
		return Input && IsValid(Input->Expression)
			&& Material->GetExpressions().Contains(Input->Expression);
	}

	double VisualStatsDistance(const FSurfaceFieldStats& A, const FSurfaceFieldStats& B)
	{
		return FMath::Abs(A.MeanHeight - B.MeanHeight) / 0.08
			+ FMath::Abs(A.HeightDeviation - B.HeightDeviation) / 0.05
			+ FMath::Abs(A.HeightRange - B.HeightRange) / 0.12
			+ FMath::Abs(A.MeanWater - B.MeanWater)
			+ FMath::Abs(A.MeanTemperature - B.MeanTemperature)
			+ FMath::Abs(A.MeanHumidity - B.MeanHumidity)
			+ FMath::Abs(A.HumidityDeviation - B.HumidityDeviation) / 0.08
			+ FMath::Abs(A.MeanFoliage - B.MeanFoliage);
	}

	FSurfaceCalibrationStats SampleSurfaceCalibration(
		const FAPSResolvedPlanetSurfaceProfile& Profile, int32 BodySeed, int32 SampleCount)
	{
		constexpr double PlanetScale = 120000000.0;
		constexpr double GoldenAngle = 2.39996322972865332;
		UAPSWorldScapePlanetNoise* Noise = NewObject<UAPSWorldScapePlanetNoise>();
		Noise->Configure(Profile);
		CustomNoise SampleNoise(BodySeed);
		SampleNoise.SetSeed(BodySeed == MAX_int32 ? BodySeed - 1 : BodySeed + 1);
		SampleNoise.SetSeed(BodySeed);

		TArray<double> Heights;
		Heights.Reserve(SampleCount);
		TArray<double> LandPaletteHeights;
		LandPaletteHeights.Reserve(SampleCount);
		int32 LandSamples = 0;
		int32 ClampedSamples = 0;
		int32 PaletteSaturatedSamples = 0;
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const double Z = 1.0 - 2.0 * (static_cast<double>(Index) + 0.5) / SampleCount;
			const double RingRadius = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			const double Azimuth = GoldenAngle * Index;
			const DVector Position(
				FMath::Cos(Azimuth) * RingRadius * PlanetScale,
				FMath::Sin(Azimuth) * RingRadius * PlanetScale,
				Z * PlanetScale);
			DVector NoisePosition;
			const FNoiseData Data = Noise->GetNoise(
				SampleNoise, Position, DVector(0.0), Profile.NoiseScale,
				Profile.NoiseIntensity, PlanetScale, false, Z, NoisePosition, FNoiseData(), true);
			const double PhysicalHeightNormalized = Data.Height
				/ FMath::Max(FMath::Abs(static_cast<double>(Profile.NoiseIntensity)), 1.0);
			Heights.Add(PhysicalHeightNormalized);
			// LandCoverage describes terrain above the profile datum even when the
			// selected dry subtype deliberately has no liquid renderer.
			const bool bVisibleLand = Data.WaterMask < 0.5f;
			LandSamples += bVisibleLand ? 1 : 0;
			if (bVisibleLand)
			{
				const double PaletteHeight = FMath::Clamp(
					static_cast<double>(Data.HeightNormalize), 0.0, 1.0);
				LandPaletteHeights.Add(PaletteHeight);
				PaletteSaturatedSamples += PaletteHeight <= 0.005
					|| PaletteHeight >= 0.995 ? 1 : 0;
			}
			ClampedSamples += PhysicalHeightNormalized <= -0.349999
				|| PhysicalHeightNormalized >= 0.649999 ? 1 : 0;
		}

		Heights.Sort();
		LandPaletteHeights.Sort();
		auto Percentile = [&Heights, SampleCount](double Fraction)
		{
			const int32 Index = FMath::Clamp(
				FMath::RoundToInt(Fraction * static_cast<double>(SampleCount - 1)), 0, SampleCount - 1);
			return Heights[Index];
		};

		FSurfaceCalibrationStats Result;
		Result.LandFraction = static_cast<double>(LandSamples) / SampleCount;
		Result.HeightP01 = Percentile(0.01);
		Result.HeightP50 = Percentile(0.50);
		Result.HeightP99 = Percentile(0.99);
		Result.ClampFraction = static_cast<double>(ClampedSamples) / SampleCount;
		if (LandPaletteHeights.Num() > 0)
		{
			auto PalettePercentile = [&LandPaletteHeights](double Fraction)
			{
				const int32 LastIndex = LandPaletteHeights.Num() - 1;
				const int32 Index = FMath::Clamp(
					FMath::RoundToInt(Fraction * static_cast<double>(LastIndex)), 0, LastIndex);
				return LandPaletteHeights[Index];
			};
			Result.PaletteP10 = PalettePercentile(0.10);
			Result.PaletteP50 = PalettePercentile(0.50);
			Result.PaletteP90 = PalettePercentile(0.90);
			Result.PaletteSaturationFraction = static_cast<double>(PaletteSaturatedSamples)
				/ LandPaletteHeights.Num();
		}
		return Result;
	}

	FLocalReliefStats SampleLocalRelief(
		const FAPSResolvedPlanetSurfaceProfile& Profile, int32 BodySeed)
	{
		// A full-scale Earth-radius body is deliberate: the regression was invisible
		// to global spherical samples but obvious to a pawn moving over 2 m..8 km.
		constexpr double PlanetScale = 637100000.0;
		constexpr double GoldenAngle = 2.39996322972865332;
		constexpr int32 CandidateCount = 384;
		constexpr int32 DesiredPatchCount = 6;
		const double SampleDistancesCm[] =
		{
			200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0,
			25000.0, 50000.0, 100000.0, 200000.0, 400000.0, 800000.0
		};
		constexpr double WalkPatchRadiusCm = 10000.0;
		constexpr double FootPatchRadiusCm = 1000.0;
		constexpr double NearPatchRadiusCm = 25000.0;
		constexpr double LocalPatchRadiusCm = 100000.0;

		UAPSWorldScapePlanetNoise* Noise = NewObject<UAPSWorldScapePlanetNoise>();
		Noise->Configure(Profile);
		CustomNoise SampleNoise(BodySeed);
		SampleNoise.SetSeed(BodySeed == MAX_int32 ? BodySeed - 1 : BodySeed + 1);
		SampleNoise.SetSeed(BodySeed);

		TArray<DVector> LandDirections;
		LandDirections.Reserve(DesiredPatchCount);
		for (int32 Index = 0;
			Index < CandidateCount && LandDirections.Num() < DesiredPatchCount; ++Index)
		{
			const double Z = 1.0 - 2.0 * (static_cast<double>(Index) + 0.5) / CandidateCount;
			const double RingRadius = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			const double Azimuth = GoldenAngle * Index;
			const DVector Direction(
				FMath::Cos(Azimuth) * RingRadius,
				FMath::Sin(Azimuth) * RingRadius,
				Z);
			DVector NoisePosition;
			const FNoiseData Data = Noise->GetNoise(
				SampleNoise, Direction * PlanetScale, DVector(0.0), Profile.NoiseScale,
				Profile.NoiseIntensity, PlanetScale, false, Z, NoisePosition, FNoiseData(), true);
			if (Data.WaterMask <= 0.08f)
			{
				LandDirections.Add(Direction);
			}
		}

		FLocalReliefStats Result;
		Result.bFoundLand = LandDirections.Num() > 0;
		TArray<double> PatchRanges;
		TArray<double> WalkPatchRanges;
		TArray<double> FootPatchRanges;
		TArray<double> NearPatchRanges;
		TArray<double> LocalPatchRanges;
		TArray<double> PatchRmsSlopes;
		TArray<double> WalkPatchRmsSlopes;
		TArray<double> FootPatchRmsSlopes;
		TArray<double> NearPatchRmsSlopes;
		TArray<double> LocalPatchRmsSlopes;
		for (DVector CenterDirection : LandDirections)
		{
			DVector Reference = FMath::Abs(CenterDirection.Z) < 0.82
				? DVector(0.0, 0.0, 1.0)
				: DVector(1.0, 0.0, 0.0);
			DVector TangentU = CenterDirection ^ Reference;
			TangentU.Normalize();
			DVector TangentV = CenterDirection ^ TangentU;
			TangentV.Normalize();

			DVector NoisePosition;
			const FNoiseData CenterData = Noise->GetNoise(
				SampleNoise, CenterDirection * PlanetScale, DVector(0.0), Profile.NoiseScale,
				Profile.NoiseIntensity, PlanetScale, false, CenterDirection.Z,
				NoisePosition, FNoiseData(), true);
			double MinimumHeight = CenterData.Height;
			double MaximumHeight = CenterData.Height;
			double MinimumWalkHeight = CenterData.Height;
			double MaximumWalkHeight = CenterData.Height;
			double MinimumFootHeight = CenterData.Height;
			double MaximumFootHeight = CenterData.Height;
			double MinimumNearHeight = CenterData.Height;
			double MaximumNearHeight = CenterData.Height;
			double MinimumLocalHeight = CenterData.Height;
			double MaximumLocalHeight = CenterData.Height;
			double SquaredSlopeSum = 0.0;
			double WalkSquaredSlopeSum = 0.0;
			double FootSquaredSlopeSum = 0.0;
			double NearSquaredSlopeSum = 0.0;
			double LocalSquaredSlopeSum = 0.0;
			int32 SlopeCount = 0;
			int32 WalkSlopeCount = 0;
			int32 FootSlopeCount = 0;
			int32 NearSlopeCount = 0;
			int32 LocalSlopeCount = 0;

			const DVector Tangents[] = {TangentU, TangentU * -1.0, TangentV, TangentV * -1.0};
			for (const DVector& Tangent : Tangents)
			{
				double PreviousHeight = CenterData.Height;
				double PreviousDistanceCm = 0.0;
				for (const double DistanceCm : SampleDistancesCm)
				{
					DVector Direction = CenterDirection + Tangent * (DistanceCm / PlanetScale);
					Direction.Normalize();
					const FNoiseData Data = Noise->GetNoise(
						SampleNoise, Direction * PlanetScale, DVector(0.0), Profile.NoiseScale,
						Profile.NoiseIntensity, PlanetScale, false, Direction.Z,
						NoisePosition, FNoiseData(), true);
					MinimumHeight = FMath::Min(MinimumHeight, Data.Height);
					MaximumHeight = FMath::Max(MaximumHeight, Data.Height);
					if (DistanceCm <= WalkPatchRadiusCm)
					{
						MinimumWalkHeight = FMath::Min(MinimumWalkHeight, Data.Height);
						MaximumWalkHeight = FMath::Max(MaximumWalkHeight, Data.Height);
					}
					if (DistanceCm <= FootPatchRadiusCm)
					{
						MinimumFootHeight = FMath::Min(MinimumFootHeight, Data.Height);
						MaximumFootHeight = FMath::Max(MaximumFootHeight, Data.Height);
					}
					if (DistanceCm <= NearPatchRadiusCm)
					{
						MinimumNearHeight = FMath::Min(MinimumNearHeight, Data.Height);
						MaximumNearHeight = FMath::Max(MaximumNearHeight, Data.Height);
					}
					if (DistanceCm <= LocalPatchRadiusCm)
					{
						MinimumLocalHeight = FMath::Min(MinimumLocalHeight, Data.Height);
						MaximumLocalHeight = FMath::Max(MaximumLocalHeight, Data.Height);
					}
					const double SegmentLengthCm = DistanceCm - PreviousDistanceCm;
					const double Slope = FMath::Abs(Data.Height - PreviousHeight)
						/ FMath::Max(SegmentLengthCm, 1.0);
					SquaredSlopeSum += Slope * Slope;
					Result.MaximumSlope = FMath::Max(Result.MaximumSlope, Slope);
					++SlopeCount;
					if (DistanceCm <= WalkPatchRadiusCm)
					{
						WalkSquaredSlopeSum += Slope * Slope;
						Result.MaximumWalkSlope = FMath::Max(Result.MaximumWalkSlope, Slope);
						++WalkSlopeCount;
					}
					if (DistanceCm <= FootPatchRadiusCm)
					{
						FootSquaredSlopeSum += Slope * Slope;
						Result.MaximumFootSlope = FMath::Max(Result.MaximumFootSlope, Slope);
						++FootSlopeCount;
					}
					if (DistanceCm <= NearPatchRadiusCm)
					{
						NearSquaredSlopeSum += Slope * Slope;
						Result.MaximumNearSlope = FMath::Max(Result.MaximumNearSlope, Slope);
						++NearSlopeCount;
					}
					if (DistanceCm <= LocalPatchRadiusCm)
					{
						LocalSquaredSlopeSum += Slope * Slope;
						Result.MaximumLocalSlope = FMath::Max(Result.MaximumLocalSlope, Slope);
						++LocalSlopeCount;
					}
					PreviousHeight = Data.Height;
					PreviousDistanceCm = DistanceCm;
				}
			}

			PatchRanges.Add(MaximumHeight - MinimumHeight);
			WalkPatchRanges.Add(MaximumWalkHeight - MinimumWalkHeight);
			FootPatchRanges.Add(MaximumFootHeight - MinimumFootHeight);
			NearPatchRanges.Add(MaximumNearHeight - MinimumNearHeight);
			LocalPatchRanges.Add(MaximumLocalHeight - MinimumLocalHeight);
			PatchRmsSlopes.Add(FMath::Sqrt(
				SquaredSlopeSum / FMath::Max(SlopeCount, 1)));
			WalkPatchRmsSlopes.Add(FMath::Sqrt(
				WalkSquaredSlopeSum / FMath::Max(WalkSlopeCount, 1)));
			FootPatchRmsSlopes.Add(FMath::Sqrt(
				FootSquaredSlopeSum / FMath::Max(FootSlopeCount, 1)));
			NearPatchRmsSlopes.Add(FMath::Sqrt(
				NearSquaredSlopeSum / FMath::Max(NearSlopeCount, 1)));
			LocalPatchRmsSlopes.Add(FMath::Sqrt(
				LocalSquaredSlopeSum / FMath::Max(LocalSlopeCount, 1)));
		}

		PatchRanges.Sort();
		WalkPatchRanges.Sort();
		FootPatchRanges.Sort();
		NearPatchRanges.Sort();
		LocalPatchRanges.Sort();
		PatchRmsSlopes.Sort();
		WalkPatchRmsSlopes.Sort();
		FootPatchRmsSlopes.Sort();
		NearPatchRmsSlopes.Sort();
		LocalPatchRmsSlopes.Sort();
		Result.PatchCount = PatchRanges.Num();
		if (PatchRanges.Num() > 0)
		{
			Result.MinimumPatchRangeCm = PatchRanges[0];
			Result.MedianPatchRangeCm = PatchRanges[PatchRanges.Num() / 2];
			Result.MinimumWalkPatchRangeCm = WalkPatchRanges[0];
			Result.MedianWalkPatchRangeCm = WalkPatchRanges[WalkPatchRanges.Num() / 2];
			Result.MinimumFootPatchRangeCm = FootPatchRanges[0];
			Result.MedianFootPatchRangeCm = FootPatchRanges[FootPatchRanges.Num() / 2];
			Result.MinimumNearPatchRangeCm = NearPatchRanges[0];
			Result.MedianNearPatchRangeCm = NearPatchRanges[NearPatchRanges.Num() / 2];
			Result.MinimumLocalPatchRangeCm = LocalPatchRanges[0];
			Result.MedianLocalPatchRangeCm = LocalPatchRanges[LocalPatchRanges.Num() / 2];
			Result.MedianRmsSlope = PatchRmsSlopes[PatchRmsSlopes.Num() / 2];
			Result.MedianWalkRmsSlope = WalkPatchRmsSlopes[WalkPatchRmsSlopes.Num() / 2];
			Result.MedianFootRmsSlope = FootPatchRmsSlopes[FootPatchRmsSlopes.Num() / 2];
			Result.MedianNearRmsSlope = NearPatchRmsSlopes[NearPatchRmsSlopes.Num() / 2];
			Result.MedianLocalRmsSlope = LocalPatchRmsSlopes[LocalPatchRmsSlopes.Num() / 2];
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceAllSolidTypesTest,
	"APS.Gameplay.World.PlanetSurface.AllSolidTypesResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceAllSolidTypesTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPlanetSurfaceProfileTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;

	for (uint8 Value = 0; Value <= static_cast<uint8>(EPlanetType::Unknown); ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		const bool bGas = Type == EPlanetType::GasGiant
			|| Type == EPlanetType::HotGiant
			|| Type == EPlanetType::IceGiant;
		TestEqual(FString::Printf(TEXT("WorldScape support for type %d"), Value),
			UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type), !bGas);
		if (bGas)
		{
			APlanet* GasPlanet = World->SpawnActor<APlanet>();
			if (TestNotNull(FString::Printf(TEXT("Gas planet %d"), Value), GasPlanet))
			{
				GasPlanet->PlanetType = Type;
				GasPlanet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
				TestTrue(FString::Printf(TEXT("Gas type %d never allocates a WorldScape root"), Value),
					!IsValid(GasPlanet->PlanetaryEnvironmentGenerator)
					|| !IsValid(GasPlanet->PlanetaryEnvironmentGenerator->WorldScapeRootInstance));
				TestEqual(FString::Printf(TEXT("Gas type %d remains unloaded"), Value),
					GasPlanet->GetWorldScapeStreamingState(), EWorldScapeSurfaceState::Unloaded);
				GasPlanet->Destroy();
			}
			continue;
		}

		APlanet* Planet = World->SpawnActor<APlanet>();
		if (!TestNotNull(FString::Printf(TEXT("Planet %d"), Value), Planet)) continue;
		Planet->PlanetType = Type;
		Planet->RadiusKM = 6371.0;
		Planet->PlanetRadiusKM = 6371;
		Planet->WorldScapeSeed = 1000 + Value;
		const FAPSResolvedPlanetSurfaceProfile Profile =
			UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
		TestEqual(FString::Printf(TEXT("Resolved type %d"), Value), Profile.PlanetType, Type);
		TestTrue(FString::Printf(TEXT("Valid noise scale %d"), Value), Profile.NoiseScale > 0.0f);
		TestTrue(FString::Printf(TEXT("Valid amplitude %d"), Value), Profile.NoiseIntensity >= 200000.0f);
		TestTrue(FString::Printf(TEXT("Valid land coverage %d"), Value),
			Profile.LandCoverage >= 0.02f && Profile.LandCoverage <= 1.0f);
		Planet->Destroy();
	}

	AMoon* GasMoon = World->SpawnActor<AMoon>();
	if (TestNotNull(TEXT("Generated gas moon"), GasMoon))
	{
		GasMoon->PlanetType = EPlanetType::GasGiant;
		GasMoon->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
		TestTrue(TEXT("Gas moon never allocates a WorldScape root"),
			!IsValid(GasMoon->PlanetaryEnvironmentGenerator)
			|| !IsValid(GasMoon->PlanetaryEnvironmentGenerator->WorldScapeRootInstance));
		TestEqual(TEXT("Gas moon remains unloaded"),
			GasMoon->GetWorldScapeStreamingState(), EWorldScapeSurfaceState::Unloaded);
		GasMoon->Destroy();
	}

	APSPlanetSurfaceProfileTests::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceGroundScaleReliefTest,
	"APS.Gameplay.World.PlanetSurface.GroundScaleRelief",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceGroundScaleReliefTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Ground-relief world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Ground-relief planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->Temperature = 288;
	const UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	if (!TestNotNull(TEXT("Ground relief uses the runtime surface catalog"), Catalog))
	{
		Planet->Destroy();
		DestroyWorld(World);
		return false;
	}

	for (uint8 Value = 0; Value <= static_cast<uint8>(EPlanetType::Unknown); ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;

		Planet->PlanetType = Type;
		Planet->WorldScapeSeed = 62011 + Value * 131;
		const FAPSResolvedPlanetSurfaceProfile Profile =
			UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
		const FLocalReliefStats Stats = SampleLocalRelief(Profile, Planet->WorldScapeSeed);
		const FString TypeName = StaticEnum<EPlanetType>()->GetNameStringByValue(Value);
		UE_LOG(LogTemp, Display,
			TEXT("[APS.GroundRelief] type=%s intensity=%.0fcm 10m[min=%.2fm median=%.2fm rmsSlope=%.5f maxSlope=%.5f] 100m[min=%.2fm median=%.2fm rmsSlope=%.5f maxSlope=%.5f] 250m[min=%.2fm median=%.2fm rmsSlope=%.5f maxSlope=%.5f] 1km[min=%.2fm median=%.2fm rmsSlope=%.5f maxSlope=%.5f] 8km[min=%.2fm median=%.2fm rmsSlope=%.5f maxSlope=%.5f]"),
			*TypeName, static_cast<double>(Profile.NoiseIntensity),
			Stats.MinimumFootPatchRangeCm / 100.0, Stats.MedianFootPatchRangeCm / 100.0,
			Stats.MedianFootRmsSlope, Stats.MaximumFootSlope,
			Stats.MinimumWalkPatchRangeCm / 100.0, Stats.MedianWalkPatchRangeCm / 100.0,
			Stats.MedianWalkRmsSlope, Stats.MaximumWalkSlope,
			Stats.MinimumNearPatchRangeCm / 100.0, Stats.MedianNearPatchRangeCm / 100.0,
			Stats.MedianNearRmsSlope, Stats.MaximumNearSlope,
			Stats.MinimumLocalPatchRangeCm / 100.0, Stats.MedianLocalPatchRangeCm / 100.0,
			Stats.MedianLocalRmsSlope, Stats.MaximumLocalSlope,
			Stats.MinimumPatchRangeCm / 100.0, Stats.MedianPatchRangeCm / 100.0,
			Stats.MedianRmsSlope, Stats.MaximumSlope);

		TestTrue(*(TypeName + TEXT(" has at least one sampled dry gameplay patch")),
			Stats.bFoundLand && Stats.PatchCount > 0);
		if (!Stats.bFoundLand || Stats.PatchCount == 0) continue;

		// Require coherent ground geometry at every gameplay-relevant radius. The old
		// non-zero contract accepted a texture-flat collision patch as soon as any two
		// vertices differed by centimetres. These bounds scale with the user's Relief
		// control and are sampled only on dry land, including oceanic islands.
		const double RequiredFootRangeCm = FMath::Max(
			8.0, static_cast<double>(Profile.NoiseIntensity) * 0.000015);
		const double RequiredWalkRangeCm = FMath::Max(
			300.0, static_cast<double>(Profile.NoiseIntensity) * 0.00030);
		const double RequiredNearRangeCm = FMath::Max(
			700.0, static_cast<double>(Profile.NoiseIntensity) * 0.00070);
		const double RequiredLocalRangeCm = FMath::Max(
			1800.0, static_cast<double>(Profile.NoiseIntensity) * 0.00180);
		const double RequiredRegionalRangeCm = FMath::Max(
			3500.0, static_cast<double>(Profile.NoiseIntensity) * 0.00350);
		TestTrue(*FString::Printf(TEXT("%s has physical relief over an actual 0..10 m walk (%.2f cm)"),
			*TypeName, Stats.MedianFootPatchRangeCm),
			Stats.MedianFootPatchRangeCm >= RequiredFootRangeCm);
		TestTrue(*FString::Printf(TEXT("%s has no mathematically flat 10 m ground patch (%.2f cm)"),
			*TypeName, Stats.MinimumFootPatchRangeCm),
			Stats.MinimumFootPatchRangeCm >= 2.0);
		TestTrue(*FString::Printf(TEXT("%s has visible physical relief over 0..100 m (%.2f cm)"),
			*TypeName, Stats.MedianWalkPatchRangeCm),
			Stats.MedianWalkPatchRangeCm >= RequiredWalkRangeCm);
		TestTrue(*FString::Printf(TEXT("%s has no flat 100 m gameplay patch (%.2f cm)"),
			*TypeName, Stats.MinimumWalkPatchRangeCm),
			Stats.MinimumWalkPatchRangeCm >= 100.0);
		TestTrue(*FString::Printf(TEXT("%s has visible rolling relief over 0..250 m (%.2f cm)"),
			*TypeName, Stats.MedianNearPatchRangeCm),
			Stats.MedianNearPatchRangeCm >= RequiredNearRangeCm);
		TestTrue(*FString::Printf(TEXT("%s has no flat 250 m near-field patch (%.2f cm)"),
			*TypeName, Stats.MinimumNearPatchRangeCm),
			Stats.MinimumNearPatchRangeCm >= 200.0);
		TestTrue(*FString::Printf(TEXT("%s has regional landform variation over 0..8 km (%.2f cm)"),
			*TypeName, Stats.MedianPatchRangeCm),
			Stats.MedianPatchRangeCm >= RequiredRegionalRangeCm);
		TestTrue(*FString::Printf(TEXT("%s has visible local-landform variation over 0..1 km (%.2f cm)"),
			*TypeName, Stats.MedianLocalPatchRangeCm),
			Stats.MedianLocalPatchRangeCm >= RequiredLocalRangeCm);
		TestTrue(*FString::Printf(TEXT("%s sampled 8 km land does not contain a flat patch (%.2f cm)"),
			*TypeName, Stats.MinimumPatchRangeCm),
			Stats.MinimumPatchRangeCm >= 1500.0);
		TestTrue(*FString::Printf(TEXT("%s has a readable walk-scale RMS slope (%.6f)"),
			*TypeName, Stats.MedianWalkRmsSlope),
			Stats.MedianWalkRmsSlope >= 0.0040);
		TestTrue(*FString::Printf(TEXT("%s has a readable ten-metre RMS slope (%.6f)"),
			*TypeName, Stats.MedianFootRmsSlope),
			Stats.MedianFootRmsSlope >= 0.0020);
		TestTrue(*FString::Printf(TEXT("%s exposes a visible local slope (%.6f)"),
			*TypeName, Stats.MaximumSlope),
			Stats.MaximumSlope >= 0.0050);
		TestTrue(*FString::Printf(TEXT("%s does not turn local relief into impassable noise (%.6f)"),
			*TypeName, Stats.MaximumWalkSlope),
			Stats.MaximumWalkSlope <= 0.50);
		TestTrue(*FString::Printf(TEXT("%s foot-scale detail remains traversable (%.6f)"),
			*TypeName, Stats.MaximumFootSlope),
			Stats.MaximumFootSlope <= 0.45);

		if (Type == EPlanetType::Frozen)
		{
			TestTrue(TEXT("Frozen has at least 30 cm median physical relief across 10 m"),
				Stats.MedianFootPatchRangeCm >= 30.0);
			TestTrue(TEXT("Every sampled Frozen patch has at least 10 cm across 10 m"),
				Stats.MinimumFootPatchRangeCm >= 10.0);
			TestTrue(TEXT("Frozen ten-metre slope is readable"),
				Stats.MedianFootRmsSlope >= 0.010);
			TestTrue(TEXT("Frozen ten-metre slope remains traversable"),
				Stats.MaximumFootSlope <= 0.35);
			// Frozen is the standard generated-gameplay handoff profile. It must show
			// rolling physical terrain at character scale without relying on a special
			// HighMountain preset or on material-only displacement cues.
			TestTrue(TEXT("Frozen has at least 15 m median relief across 100 m"),
				Stats.MedianWalkPatchRangeCm >= 1500.0);
			TestTrue(TEXT("Every sampled Frozen patch has at least 7 m across 100 m"),
				Stats.MinimumWalkPatchRangeCm >= 700.0);
			TestTrue(TEXT("Frozen has at least 25 m median relief across 250 m"),
				Stats.MedianNearPatchRangeCm >= 2500.0);
			TestTrue(TEXT("Every sampled Frozen patch has at least 12 m across 250 m"),
				Stats.MinimumNearPatchRangeCm >= 1200.0);
			TestTrue(TEXT("Frozen has at least 60 m median relief across 1 km"),
				Stats.MedianLocalPatchRangeCm >= 6000.0);
			TestTrue(TEXT("Every sampled Frozen patch has at least 30 m across 1 km"),
				Stats.MinimumLocalPatchRangeCm >= 3000.0);
			TestTrue(TEXT("Frozen has at least 150 m median relief across 8 km"),
				Stats.MedianPatchRangeCm >= 15000.0);
			TestTrue(TEXT("Every sampled Frozen patch has at least 90 m across 8 km"),
				Stats.MinimumPatchRangeCm >= 9000.0);
			TestTrue(TEXT("Frozen walk-scale slope is visible"),
				Stats.MedianWalkRmsSlope >= 0.020);
			TestTrue(TEXT("Frozen walk-scale slope remains traversable"),
				Stats.MaximumWalkSlope <= 0.35);
		}

		if (Type == EPlanetType::HighMountain)
		{
			TestTrue(TEXT("High Mountain has at least 15 cm of physical relief across 10 m"),
				Stats.MedianFootPatchRangeCm >= 15.0);
			// High Mountain is the runtime handoff profile and must read as broad physical
			// landforms from a pawn, not as a flat material or high-frequency ripple.
			TestTrue(TEXT("High Mountain has at least 3 m of relief across 100 m"),
				Stats.MedianWalkPatchRangeCm >= 300.0);
			TestTrue(TEXT("Every sampled High Mountain patch has at least 1.5 m across 100 m"),
				Stats.MinimumWalkPatchRangeCm >= 150.0);
			TestTrue(TEXT("High Mountain has at least 35 m of relief across 1 km"),
				Stats.MedianLocalPatchRangeCm >= 3500.0);
			TestTrue(TEXT("Every sampled High Mountain patch has at least 10 m across 1 km"),
				Stats.MinimumLocalPatchRangeCm >= 1000.0);
			TestTrue(TEXT("High Mountain has at least 180 m of relief across 8 km"),
				Stats.MedianPatchRangeCm >= 18000.0);
			TestTrue(TEXT("Every sampled High Mountain patch has at least 120 m across 8 km"),
				Stats.MinimumPatchRangeCm >= 12000.0);
			TestTrue(TEXT("High Mountain walk-scale slope is visible"),
				Stats.MedianWalkRmsSlope >= 0.015);
			TestTrue(TEXT("High Mountain walk-scale slope is not noisy/rippled"),
				Stats.MaximumWalkSlope <= 0.45);
		}
	}

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceMaterialHeightChannelContractTest,
	"APS.Gameplay.World.PlanetSurface.MaterialHeightChannelContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceMaterialHeightChannelContractTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	constexpr double PlanetScale = 120000000.0;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Material-channel world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Material-channel planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->Temperature = 288;
	Planet->PlanetAtmosphere.Humidity = 42.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;

	for (uint8 Value = 0; Value <= static_cast<uint8>(EPlanetType::Unknown); ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;

		Planet->PlanetType = Type;
		Planet->WorldScapeSeed = 51001 + Value * 97;
		const FAPSResolvedPlanetSurfaceProfile Profile =
			UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
		const FSurfaceFieldStats Stats = SampleSurfaceFields(
			Profile, Planet->WorldScapeSeed, 512);
		const FString TypeName = StaticEnum<EPlanetType>()->GetNameStringByValue(Value);

		TestTrue(*(TypeName + TEXT(" material height minimum is finite")),
			FMath::IsFinite(Stats.MinimumMaterialHeight));
		TestTrue(*(TypeName + TEXT(" material height maximum is finite")),
			FMath::IsFinite(Stats.MaximumMaterialHeight));
		TestTrue(*(TypeName + TEXT(" material height obeys WorldScape 0..1 contract")),
			Stats.MinimumMaterialHeight >= 0.0
				&& Stats.MaximumMaterialHeight <= 1.0);
		TestTrue(*(TypeName + TEXT(" reaches multiple terrain material layers")),
			Stats.HeightRange >= 0.12 && Stats.MaximumMaterialHeight >= 0.24);
		TestTrue(*(TypeName + TEXT(" retains non-flat signed physical relief")),
			Stats.PhysicalHeightRange >= Profile.NoiseIntensity * 0.008);

		if (Profile.LiquidType != EAPSPlanetLiquidType::None)
		{
			UAPSWorldScapePlanetNoise* Noise = NewObject<UAPSWorldScapePlanetNoise>();
			Noise->Configure(Profile);
			CustomNoise OceanSampleNoise(Planet->WorldScapeSeed);
			OceanSampleNoise.SetSeed(Planet->WorldScapeSeed + 1);
			OceanSampleNoise.SetSeed(Planet->WorldScapeSeed);
			DVector NoisePosition;
			const FNoiseData OceanData = Noise->GetOceanNoise(
				OceanSampleNoise, DVector(PlanetScale, 0.0, 0.0), DVector(0.0),
				Profile.NoiseScale, Profile.NoiseIntensity, PlanetScale, false, 0.0,
				NoisePosition, FNoiseData(), true);
			TestTrue(*(TypeName + TEXT(" ocean material height obeys 0..1 contract")),
				OceanData.HeightNormalize >= 0.0 && OceanData.HeightNormalize <= 1.0);
			TestTrue(*(TypeName + TEXT(" ocean material channel is independent of displacement")),
				FMath::IsNearlyEqual(OceanData.HeightNormalize, 0.08, UE_DOUBLE_SMALL_NUMBER));
			TestTrue(*(TypeName + TEXT(" ocean keeps its authored physical altitude")),
				FMath::IsNearlyEqual(OceanData.Height,
					static_cast<double>(Profile.OceanLevel) * Profile.NoiseIntensity, 0.01));
		}
	}

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceMaterialCatalogIntegrityTest,
	"APS.Gameplay.World.PlanetSurface.MaterialCatalogIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceMaterialCatalogIntegrityTest::RunTest(const FString& Parameters)
{
	const UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	if (!TestNotNull(TEXT("Project planet surface catalog"), Catalog)) return false;
	TestEqual(TEXT("Catalog contains every surface archetype"), Catalog->Archetypes.Num(), 9);

	UMaterialInstance* WaterMaterial = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water.MI_APS_WS_Water"));
	UMaterialInstance* AmmoniaMaterial = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia"));
	UMaterialInstance* LavaMaterial = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava"));
	TestNotNull(TEXT("Project water material"), WaterMaterial);
	TestNotNull(TEXT("Project ammonia material"), AmmoniaMaterial);
	TestNotNull(TEXT("Project lava material"), LavaMaterial);
	TestNotEqual(TEXT("Water and ammonia are independent material assets"),
		static_cast<UObject*>(WaterMaterial), static_cast<UObject*>(AmmoniaMaterial));
	TestNotEqual(TEXT("Water and lava are independent material assets"),
		static_cast<UObject*>(WaterMaterial), static_cast<UObject*>(LavaMaterial));

	UMaterial* OrbitalTerrain = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalTerrain.M_APS_OrbitalTerrain"));
	UMaterial* WorldScapeTerrain = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeTerrain.M_APS_WorldScapeTerrain"));
	UMaterial* WorldScapeLiquid = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeLiquid.M_APS_WorldScapeLiquid"));
	UMaterial* OrbitalLiquid = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalLiquid.M_APS_OrbitalLiquid"));
	UMaterialInstance* OrbitalWater = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Water.MI_APS_OrbitalLiquid_Water"));
	UMaterialInstance* OrbitalAmmonia = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Ammonia.MI_APS_OrbitalLiquid_Ammonia"));
	UMaterialInstance* OrbitalLava = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Lava.MI_APS_OrbitalLiquid_Lava"));
	if (TestNotNull(TEXT("Opaque orbital terrain material"), OrbitalTerrain))
	{
		TestEqual(TEXT("Orbital terrain is opaque"), OrbitalTerrain->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("Orbital terrain is lit"),
			OrbitalTerrain->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestTrue(TEXT("Orbital terrain consumes the WorldScape vertex payload"),
			OrbitalTerrain->GetExpressions().ContainsByPredicate([](const UMaterialExpression* Expression)
			{
				return Expression && Expression->IsA<UMaterialExpressionVertexColor>();
			}));
		int32 HeightBandCount = 0;
		for (const UMaterialExpression* Expression : OrbitalTerrain->GetExpressions())
		{
			if (const UMaterialExpressionSmoothStep* SmoothStep =
				Cast<UMaterialExpressionSmoothStep>(Expression))
			{
				++HeightBandCount;
				TestTrue(TEXT("Orbital terrain palette transitions are broad enough to hide LOD isolines"),
					SmoothStep->ConstMax - SmoothStep->ConstMin >= 0.25f);
			}
		}
		TestTrue(TEXT("Orbital terrain uses a separate smooth transition for every palette band"),
			HeightBandCount >= 5);
		TestFalse(TEXT("Canonical WorldScape terrain has no UV texture sampling that can expose patch grids"),
			OrbitalTerrain->GetExpressions().ContainsByPredicate([](const UMaterialExpression* Expression)
			{
				return Expression && Expression->IsA<UMaterialExpressionTextureSample>();
			}));
		const UMaterialExpressionOneMinus* LowlandEmissiveMask = nullptr;
		for (const UMaterialExpression* Expression : OrbitalTerrain->GetExpressions())
		{
			if (const UMaterialExpressionOneMinus* Candidate =
				Cast<UMaterialExpressionOneMinus>(Expression))
			{
				LowlandEmissiveMask = Candidate;
				break;
			}
		}
		if (TestNotNull(TEXT("Orbital terrain derives emissive mask without WorldScape hole alpha"),
			LowlandEmissiveMask))
		{
			const UMaterialExpressionSmoothStep* EmissiveHeightBand =
				Cast<UMaterialExpressionSmoothStep>(LowlandEmissiveMask->Input.Expression);
			if (TestNotNull(TEXT("Orbital emissive mask consumes a smooth height band"),
				EmissiveHeightBand))
			{
				TestEqual(TEXT("Orbital emissive mask reads WorldScape normalized-height R, never hole alpha"),
					EmissiveHeightBand->Value.OutputIndex, 1);
				TestTrue(TEXT("Lava emissive transition is broad enough to suppress LOD vertex grids"),
					EmissiveHeightBand->ConstMax - EmissiveHeightBand->ConstMin >= 0.70f);
			}
		}
		TestTrue(TEXT("Orbital terrain bounds authored lava glow below display white"),
			OrbitalTerrain->GetExpressions().ContainsByPredicate([](const UMaterialExpression* Expression)
			{
				const UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(Expression);
				return Clamp && Clamp->ClampMode == CMODE_Clamp
					&& FMath::IsNearlyEqual(Clamp->MinDefault, 0.0f)
					&& FMath::IsNearlyEqual(Clamp->MaxDefault, 0.38f);
			}));
		TestTrue(TEXT("Orbital terrain connects its generated colour graph to Base Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalTerrain, MP_BaseColor));
		TestTrue(TEXT("Orbital terrain connects its bounded output to Emissive Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalTerrain, MP_EmissiveColor));
		for (const TCHAR* ParameterName :
			{TEXT("BottomColor"), TEXT("Color1"), TEXT("Color2"), TEXT("Color3"),
				TEXT("Color4"), TEXT("Color5"), TEXT("EmissiveColor")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital terrain exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(OrbitalTerrain, ParameterName));
		}
		for (const TCHAR* ParameterName :
			{TEXT("ClimateBlend"), TEXT("TerrainAmbientFill"), TEXT("Roughness"),
				TEXT("Metallic"), TEXT("Specular")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital terrain exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(OrbitalTerrain, ParameterName));
		}
		float DefaultClimateBlend = 0.0f;
		if (TestTrue(TEXT("Orbital terrain resolves its climate blend"),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				OrbitalTerrain, TEXT("ClimateBlend"), DefaultClimateBlend)))
		{
			TestTrue(TEXT("Orbital terrain keeps climate tint subordinate to height bands"),
				DefaultClimateBlend >= 0.08f && DefaultClimateBlend <= 0.25f);
		}
	}
	if (TestNotNull(TEXT("Canonical full-scale WorldScape terrain master"), WorldScapeTerrain))
	{
		TestEqual(TEXT("WorldScape terrain is opaque"),
			WorldScapeTerrain->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("WorldScape terrain is lit"),
			WorldScapeTerrain->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestTrue(TEXT("WorldScape terrain consumes the authoritative vertex payload"),
			WorldScapeTerrain->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionVertexColor>();
				}));
		TestFalse(TEXT("WorldScape terrain has no UV sampling that can reveal cube-patch grids"),
			WorldScapeTerrain->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionTextureSample>();
				}));
		int32 BroadHeightBandCount = 0;
		for (const UMaterialExpression* Expression : WorldScapeTerrain->GetExpressions())
		{
			if (const UMaterialExpressionSmoothStep* SmoothStep =
				Cast<UMaterialExpressionSmoothStep>(Expression))
			{
				if (SmoothStep->Value.OutputIndex == 1
					&& SmoothStep->ConstMax - SmoothStep->ConstMin >= 0.25f)
				{
					++BroadHeightBandCount;
				}
			}
		}
		TestTrue(TEXT("WorldScape terrain softens per-vertex LOD transitions with broad bands"),
			BroadHeightBandCount >= 5);
		TestTrue(TEXT("WorldScape terrain connects its generated palette to Base Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				WorldScapeTerrain, MP_BaseColor));
		TestTrue(TEXT("WorldScape terrain connects bounded lava glow to Emissive Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				WorldScapeTerrain, MP_EmissiveColor));
	}

	struct FCanonicalTerrainFamily
	{
		EAPSPlanetSurfaceArchetype Archetype;
		const TCHAR* AssetName;
	};
	const FCanonicalTerrainFamily CanonicalTerrainFamilies[] =
	{
		{EAPSPlanetSurfaceArchetype::Rocky, TEXT("MI_APS_WS_Rocky")},
		{EAPSPlanetSurfaceArchetype::Temperate, TEXT("MI_APS_WS_Temperate")},
		{EAPSPlanetSurfaceArchetype::Oceanic, TEXT("MI_APS_WS_Oceanic")},
		{EAPSPlanetSurfaceArchetype::Biosphere, TEXT("MI_APS_WS_Biosphere")},
		{EAPSPlanetSurfaceArchetype::Desert, TEXT("MI_APS_WS_Desert")},
		{EAPSPlanetSurfaceArchetype::Cryogenic, TEXT("MI_APS_WS_Cryogenic")},
		{EAPSPlanetSurfaceArchetype::Magmatic, TEXT("MI_APS_WS_Magmatic")},
		{EAPSPlanetSurfaceArchetype::Metallic, TEXT("MI_APS_WS_Metallic")},
		{EAPSPlanetSurfaceArchetype::ExoticChemical, TEXT("MI_APS_WS_ExoticChemical")}
	};
	for (const FCanonicalTerrainFamily& Family : CanonicalTerrainFamilies)
	{
		const FString Context = FString::Printf(TEXT("Canonical terrain family %s"),
			Family.AssetName);
		const FString MaterialPath = FString::Printf(
			TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/%s.%s"),
			Family.AssetName, Family.AssetName);
		UMaterialInstance* FamilyMaterial = LoadObject<UMaterialInstance>(
			nullptr, *MaterialPath);
		if (!TestNotNull(*(Context + TEXT(" asset exists")), FamilyMaterial))
		{
			continue;
		}

		const FAPSPlanetSurfaceArchetypeDefinition* CatalogDefinition =
			Catalog->Archetypes.Find(Family.Archetype);
		if (!TestNotNull(*(Context + TEXT(" has a catalog definition")), CatalogDefinition))
		{
			continue;
		}
		TestEqual(*(Context + TEXT(" is selected by the catalog")),
			CatalogDefinition->TerrainMaterial.LoadSynchronous(), FamilyMaterial);
		if (WorldScapeTerrain)
		{
			TestEqual(*(Context + TEXT(" directly inherits the canonical WorldScape master")),
				FamilyMaterial->Parent.Get(),
				static_cast<UMaterialInterface*>(WorldScapeTerrain));
			TestNotEqual(*(Context + TEXT(" never inherits the orbital preview master")),
				FamilyMaterial->Parent.Get(), static_cast<UMaterialInterface*>(OrbitalTerrain));
		}
		float SlopeTintStrength = 0.0f;
		if (TestTrue(*(Context + TEXT(" authors geological slope readability")),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, TEXT("SlopeTintStrength"), SlopeTintStrength)))
		{
			TestTrue(*(Context + TEXT(" keeps slope tint bounded")),
				SlopeTintStrength >= 0.08f && SlopeTintStrength <= 0.28f);
		}
		for (const TCHAR* ScaleParameter :
			{TEXT("MacroDetailScaleCm"), TEXT("MesoDetailScaleCm"), TEXT("NearDetailScaleCm")})
		{
			float ScaleValue = 0.0f;
			TestTrue(*FString::Printf(TEXT("%s resolves %s"), *Context, ScaleParameter),
				APSPlanetSurfaceProfileTests::GetScalarParameter(
					FamilyMaterial, ScaleParameter, ScaleValue)
					&& FMath::IsFinite(ScaleValue) && ScaleValue > 0.0f);
		}
	}
	if (TestNotNull(TEXT("Depth-writing WorldScape liquid material"), WorldScapeLiquid))
	{
		TestEqual(TEXT("WorldScape liquid writes the opaque depth pass"),
			WorldScapeLiquid->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("WorldScape liquid remains lit"),
			WorldScapeLiquid->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("Safe baseline does not claim an incomplete SingleLayerWater graph"),
			WorldScapeLiquid->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
		TestTrue(TEXT("WorldScape liquid is robust across ring/stitch winding"),
			WorldScapeLiquid->IsTwoSided());
		TestFalse(TEXT("WorldScape liquid never reconnects translucent opacity"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				WorldScapeLiquid, MP_Opacity));
		float WaveScaleCm = 0.0f;
		float WaveColorStrength = 0.0f;
		float WaveNormalStrength = 0.0f;
		TestTrue(TEXT("WorldScape liquid exposes finite non-tiling wave scale"),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				WorldScapeLiquid, TEXT("WaveScaleCm"), WaveScaleCm)
				&& FMath::IsNearlyEqual(WaveScaleCm, 18000.0f));
		TestTrue(TEXT("WorldScape liquid keeps colour waves subtle"),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				WorldScapeLiquid, TEXT("WaveColorStrength"), WaveColorStrength)
				&& WaveColorStrength > 0.0f && WaveColorStrength <= 0.01f);
		TestTrue(TEXT("WorldScape liquid keeps readable bounded normal waves"),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				WorldScapeLiquid, TEXT("WaveNormalStrength"), WaveNormalStrength)
				&& WaveNormalStrength >= 0.02f && WaveNormalStrength <= 0.04f);
	}
	if (TestNotNull(TEXT("Orbital liquid material"), OrbitalLiquid))
	{
		TestEqual(TEXT("Orbital liquid uses ordinary translucency"),
			OrbitalLiquid->GetBlendMode(), BLEND_Translucent);
		TestTrue(TEXT("Orbital liquid is lit"),
			OrbitalLiquid->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("Orbital liquid does not use SingleLayerWater"),
			OrbitalLiquid->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
		TestTrue(TEXT("Orbital liquid connects its generated colour graph to Base Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalLiquid, MP_BaseColor));
		TestTrue(TEXT("Orbital liquid connects its bounded output to Emissive Color"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalLiquid, MP_EmissiveColor));
		TestTrue(TEXT("Orbital liquid connects its authored opacity control"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalLiquid, MP_Opacity));
		// The closed orbital proxy deliberately selects the resolver-authored alpha water
		// mask with OrbitalNormalBlend=1.  Physical WorldScape oceans use the separate
		// opaque master validated above and never depend on this translucent mask.
		const UMaterialExpressionLinearInterpolate* VisibilityContext = nullptr;
		for (const UMaterialExpression* Expression : OrbitalLiquid->GetExpressions())
		{
			const UMaterialExpressionLinearInterpolate* Lerp =
				Cast<UMaterialExpressionLinearInterpolate>(Expression);
			const UMaterialExpressionScalarParameter* BlendParameter = Lerp
				? Cast<UMaterialExpressionScalarParameter>(Lerp->Alpha.Expression) : nullptr;
			if (BlendParameter
				&& BlendParameter->ParameterName == TEXT("OrbitalNormalBlend"))
			{
				const UMaterialExpressionConstant* PhysicalVisibility =
					Cast<UMaterialExpressionConstant>(Lerp->A.Expression);
				const UMaterialExpressionSmoothStep* OrbitalMask =
					Cast<UMaterialExpressionSmoothStep>(Lerp->B.Expression);
				if (PhysicalVisibility && FMath::IsNearlyEqual(PhysicalVisibility->R, 1.0f)
					&& OrbitalMask && OrbitalMask->Value.Expression
						&& OrbitalMask->Value.Expression->IsA<UMaterialExpressionVertexColor>())
				{
					VisibilityContext = Lerp;
					break;
				}
			}
		}
		TestNotNull(TEXT("Canonical liquid isolates orbital water alpha from physical WorldScape visibility"),
			VisibilityContext);
		TestFalse(TEXT("Canonical liquid has no UV sampling that can reveal cube-patch grids"),
			OrbitalLiquid->GetExpressions().ContainsByPredicate([](const UMaterialExpression* Expression)
			{
				return Expression && Expression->IsA<UMaterialExpressionTextureSample>();
			}));
		const FExpressionInput* OpacityInput =
			OrbitalLiquid->GetExpressionInputForProperty(MP_Opacity);
		const UMaterialExpressionMultiply* VisibilityMaskedOpacity = OpacityInput
			? Cast<UMaterialExpressionMultiply>(OpacityInput->Expression) : nullptr;
		const UMaterialExpressionClamp* OpacityClamp = VisibilityMaskedOpacity
			? Cast<UMaterialExpressionClamp>(VisibilityMaskedOpacity->A.Expression) : nullptr;
		TestNotNull(TEXT("Canonical liquid applies its context visibility after opacity is bounded"),
			VisibilityMaskedOpacity);
		if (TestNotNull(TEXT("Canonical liquid opacity is clamped"), OpacityClamp))
		{
			TestTrue(TEXT("Canonical liquid opacity stays translucent but visible"),
				FMath::IsNearlyEqual(OpacityClamp->MinDefault, 0.05f)
					&& FMath::IsNearlyEqual(OpacityClamp->MaxDefault, 0.68f));
			const UMaterialExpressionMultiply* FresnelOpacity =
				Cast<UMaterialExpressionMultiply>(OpacityClamp->Input.Expression);
			if (TestNotNull(TEXT("Canonical liquid attenuates opacity before its final clamp"),
				FresnelOpacity))
			{
				const UMaterialExpressionLinearInterpolate* OpacityScale =
					Cast<UMaterialExpressionLinearInterpolate>(FresnelOpacity->B.Expression);
				if (TestNotNull(TEXT("Canonical liquid uses a Fresnel opacity scale"), OpacityScale))
				{
					TestTrue(TEXT("Face-on liquid preserves terrain relief"),
						OpacityScale->ConstA < 0.7f
							&& FMath::IsNearlyEqual(OpacityScale->ConstB, 1.0f));
					const UMaterialExpressionClamp* OpacityFresnel =
						Cast<UMaterialExpressionClamp>(OpacityScale->Alpha.Expression);
					TestTrue(TEXT("Opacity uses the bounded Fresnel signal"),
						OpacityFresnel && OpacityFresnel->Input.Expression
							&& OpacityFresnel->Input.Expression->IsA<UMaterialExpressionFresnel>());
				}
			}
		}
		const FExpressionInput* EmissiveInput =
			OrbitalLiquid->GetExpressionInputForProperty(MP_EmissiveColor);
		const UMaterialExpressionMultiply* VisibilityMaskedEmissive = EmissiveInput
			? Cast<UMaterialExpressionMultiply>(EmissiveInput->Expression) : nullptr;
		const UMaterialExpressionClamp* EmissiveClamp = VisibilityMaskedEmissive
			? Cast<UMaterialExpressionClamp>(VisibilityMaskedEmissive->A.Expression) : nullptr;
		TestNotNull(TEXT("Canonical liquid applies its context visibility after emissive is bounded"),
			VisibilityMaskedEmissive);
		if (TestNotNull(TEXT("Canonical liquid emissive is clamped"), EmissiveClamp))
		{
			TestTrue(TEXT("Canonical liquid emissive stays display-safe"),
				FMath::IsNearlyEqual(EmissiveClamp->MinDefault, 0.0f)
					&& FMath::IsNearlyEqual(EmissiveClamp->MaxDefault, 1.25f));
		}
		TestTrue(TEXT("Canonical liquid bounds Fresnel before colour blending"),
			OrbitalLiquid->GetExpressions().ContainsByPredicate([](const UMaterialExpression* Expression)
			{
				const UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(Expression);
				return Clamp && Clamp->Input.Expression
					&& Clamp->Input.Expression->IsA<UMaterialExpressionFresnel>()
					&& FMath::IsNearlyEqual(Clamp->MinDefault, 0.0f)
					&& FMath::IsNearlyEqual(Clamp->MaxDefault, 1.0f);
			}));
		for (const TCHAR* ParameterName :
			{TEXT("LiquidDeepColor"), TEXT("LiquidShallowColor"), TEXT("LiquidEmissiveColor")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital liquid exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(OrbitalLiquid, ParameterName));
		}
		for (const TCHAR* ParameterName :
			{TEXT("Opacity"), TEXT("Roughness"), TEXT("Metallic"), TEXT("Specular")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital liquid exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(OrbitalLiquid, ParameterName));
		}
	}
	for (UMaterialInstance* Preset : {OrbitalWater, OrbitalAmmonia, OrbitalLava})
	{
		if (TestNotNull(TEXT("Orbital liquid preset"), Preset) && OrbitalLiquid)
		{
			TestEqual(TEXT("Orbital liquid preset shares the canonical liquid parent"),
				Preset->Parent.Get(), static_cast<UMaterialInterface*>(OrbitalLiquid));
			TestEqual(TEXT("Orbital liquid preset authors its three colours"),
				Preset->VectorParameterValues.Num(), 3);
			TestEqual(TEXT("Orbital liquid preset authors bounded optical controls"),
				Preset->ScalarParameterValues.Num(), 4);
			float PreviewOpacity = 0.0f;
			if (TestTrue(TEXT("Orbital liquid preset resolves Opacity"),
				APSPlanetSurfaceProfileTests::GetScalarParameter(
					Preset, TEXT("Opacity"), PreviewOpacity)))
			{
				TestTrue(TEXT("Orbital liquid opacity keeps terrain legible"),
					PreviewOpacity >= 0.50f && PreviewOpacity <= 0.68f);
			}
			FLinearColor PreviewEmissive;
			if (TestTrue(TEXT("Orbital liquid preset resolves bounded emissive colour"),
				APSPlanetSurfaceProfileTests::GetVectorParameter(
					Preset, TEXT("LiquidEmissiveColor"), PreviewEmissive)))
			{
				TestTrue(TEXT("Orbital liquid authored emissive stays display-safe"),
					PreviewEmissive.R <= 1.0f && PreviewEmissive.G <= 1.0f
					&& PreviewEmissive.B <= 1.0f);
			}
		}
	}
	if (WaterMaterial && AmmoniaMaterial && LavaMaterial && WorldScapeLiquid
		&& OrbitalLiquid)
	{
		TestNotEqual(TEXT("Physical and orbital liquid masters use separate render passes"),
			WorldScapeLiquid, OrbitalLiquid);
		TestEqual(TEXT("Water directly inherits the depth-writing WorldScape graph"),
			WaterMaterial->Parent.Get(), static_cast<UMaterialInterface*>(WorldScapeLiquid));
		TestEqual(TEXT("Ammonia directly inherits the depth-writing WorldScape graph"),
			AmmoniaMaterial->Parent.Get(), static_cast<UMaterialInterface*>(WorldScapeLiquid));
		TestEqual(TEXT("Lava directly inherits the depth-writing WorldScape graph"),
			LavaMaterial->Parent.Get(), static_cast<UMaterialInterface*>(WorldScapeLiquid));
		TestEqual(TEXT("Water resolves the project-owned WorldScape graph"),
			WaterMaterial->GetMaterial(), WorldScapeLiquid);
		TestEqual(TEXT("Ammonia resolves the project-owned WorldScape graph"),
			AmmoniaMaterial->GetMaterial(), WorldScapeLiquid);
		TestEqual(TEXT("Lava resolves the project-owned WorldScape graph"),
			LavaMaterial->GetMaterial(), WorldScapeLiquid);

		for (const TCHAR* ParameterName
			: {TEXT("LiquidDeepColor"), TEXT("LiquidShallowColor"), TEXT("LiquidEmissiveColor")})
		{
			TestTrue(*FString::Printf(TEXT("Water exposes canonical vector %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(WaterMaterial, ParameterName));
			TestTrue(*FString::Printf(TEXT("Ammonia exposes canonical vector %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(AmmoniaMaterial, ParameterName));
			TestTrue(*FString::Printf(TEXT("Lava exposes canonical vector %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(LavaMaterial, ParameterName));
		}
		for (const TCHAR* ParameterName
			: {TEXT("Opacity"), TEXT("Roughness"), TEXT("Metallic"), TEXT("Specular")})
		{
			TestTrue(*FString::Printf(TEXT("Water exposes canonical scalar %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(WaterMaterial, ParameterName));
			TestTrue(*FString::Printf(TEXT("Ammonia exposes canonical scalar %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(AmmoniaMaterial, ParameterName));
			TestTrue(*FString::Printf(TEXT("Lava exposes canonical scalar %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(LavaMaterial, ParameterName));
		}

		TestEqual(TEXT("Water authors exactly three real vector overrides"),
			WaterMaterial->VectorParameterValues.Num(), 3);
		TestEqual(TEXT("Water authors exactly four real scalar overrides"),
			WaterMaterial->ScalarParameterValues.Num(), 4);
		TestEqual(TEXT("Ammonia authors exactly three real vector overrides"),
			AmmoniaMaterial->VectorParameterValues.Num(), 3);
		TestEqual(TEXT("Ammonia authors exactly four real scalar overrides"),
			AmmoniaMaterial->ScalarParameterValues.Num(), 4);
		TestEqual(TEXT("Lava authors exactly three real vector overrides"),
			LavaMaterial->VectorParameterValues.Num(), 3);
		TestEqual(TEXT("Lava authors exactly four real scalar overrides"),
			LavaMaterial->ScalarParameterValues.Num(), 4);

		auto TestVectorValue = [this](const TCHAR* Label, const UMaterialInterface* Material,
			const TCHAR* ParameterName, const FLinearColor& Expected)
		{
			FLinearColor Actual;
			if (TestTrue(Label, APSPlanetSurfaceProfileTests::GetVectorParameter(
				Material, ParameterName, Actual)))
			{
				TestTrue(*FString::Printf(TEXT("%s has the authored value"), Label),
					Actual.Equals(Expected, 1.0e-4f));
			}
		};
		auto TestScalarValue = [this](const TCHAR* Label, const UMaterialInterface* Material,
			const TCHAR* ParameterName, float Expected)
		{
			float Actual = 0.0f;
			if (TestTrue(Label, APSPlanetSurfaceProfileTests::GetScalarParameter(
				Material, ParameterName, Actual)))
			{
				TestTrue(*FString::Printf(TEXT("%s has the authored value"), Label),
					FMath::IsNearlyEqual(Actual, Expected, 1.0e-4f));
			}
		};

		TestVectorValue(TEXT("Water deep colour"), WaterMaterial, TEXT("LiquidDeepColor"),
			FLinearColor(0.002f, 0.008f, 0.025f));
		TestVectorValue(TEXT("Water shallow colour"), WaterMaterial, TEXT("LiquidShallowColor"),
			FLinearColor(0.008f, 0.055f, 0.085f));
		TestVectorValue(TEXT("Water emissive colour"), WaterMaterial, TEXT("LiquidEmissiveColor"),
			FLinearColor(0.0005f, 0.002f, 0.004f));
		TestScalarValue(TEXT("Water Opacity"), WaterMaterial, TEXT("Opacity"), 0.34f);
		TestScalarValue(TEXT("Water Roughness"), WaterMaterial, TEXT("Roughness"), 0.30f);
		TestScalarValue(TEXT("Water Metallic"), WaterMaterial, TEXT("Metallic"), 0.0f);
		TestScalarValue(TEXT("Water Specular"), WaterMaterial, TEXT("Specular"), 0.42f);
		TestVectorValue(TEXT("Ammonia deep colour"), AmmoniaMaterial, TEXT("LiquidDeepColor"),
			FLinearColor(0.008f, 0.055f, 0.025f));
		TestVectorValue(TEXT("Ammonia shallow colour"), AmmoniaMaterial, TEXT("LiquidShallowColor"),
			FLinearColor(0.160f, 0.480f, 0.250f));
		TestVectorValue(TEXT("Ammonia emissive colour"), AmmoniaMaterial, TEXT("LiquidEmissiveColor"),
			FLinearColor(0.006f, 0.035f, 0.015f));
		TestScalarValue(TEXT("Ammonia Opacity"), AmmoniaMaterial, TEXT("Opacity"), 0.32f);
		TestScalarValue(TEXT("Ammonia Roughness"), AmmoniaMaterial, TEXT("Roughness"), 0.22f);
		TestScalarValue(TEXT("Ammonia Metallic"), AmmoniaMaterial, TEXT("Metallic"), 0.0f);
		TestScalarValue(TEXT("Ammonia Specular"), AmmoniaMaterial, TEXT("Specular"), 0.58f);
		TestVectorValue(TEXT("Lava deep colour"), LavaMaterial, TEXT("LiquidDeepColor"),
			FLinearColor(0.055f, 0.001f, 0.0005f));
		TestVectorValue(TEXT("Lava shallow colour"), LavaMaterial, TEXT("LiquidShallowColor"),
			FLinearColor(0.720f, 0.025f, 0.001f));
		TestVectorValue(TEXT("Lava emissive colour"), LavaMaterial, TEXT("LiquidEmissiveColor"),
			FLinearColor(0.420f, 0.018f, 0.001f));
		TestScalarValue(TEXT("Lava Opacity"), LavaMaterial, TEXT("Opacity"), 0.42f);
		TestScalarValue(TEXT("Lava Roughness"), LavaMaterial, TEXT("Roughness"), 0.42f);
		TestScalarValue(TEXT("Lava Metallic"), LavaMaterial, TEXT("Metallic"), 0.04f);
		TestScalarValue(TEXT("Lava Specular"), LavaMaterial, TEXT("Specular"), 0.30f);

		for (UMaterialInstance* Liquid : {WaterMaterial, AmmoniaMaterial, LavaMaterial})
		{
			const FString LiquidName = Liquid->GetName();
			for (const TCHAR* ParameterName
				: {TEXT("LiquidDeepColor"), TEXT("LiquidShallowColor"), TEXT("LiquidEmissiveColor")})
			{
				TestTrue(*FString::Printf(TEXT("%s owns vector override %s"),
					*LiquidName, ParameterName),
					APSPlanetSurfaceProfileTests::HasOwnVectorOverride(Liquid, ParameterName));
			}
			for (const TCHAR* ParameterName
				: {TEXT("Opacity"), TEXT("Roughness"), TEXT("Metallic"), TEXT("Specular")})
			{
				TestTrue(*FString::Printf(TEXT("%s owns scalar override %s"),
					*LiquidName, ParameterName),
					APSPlanetSurfaceProfileTests::HasOwnScalarOverride(Liquid, ParameterName));
			}
		}
	}

	for (const TPair<EAPSPlanetSurfaceArchetype, FAPSPlanetSurfaceArchetypeDefinition>& Entry
		: Catalog->Archetypes)
	{
		const FString Context = FString::Printf(TEXT("Surface archetype %d"),
			static_cast<int32>(Entry.Key));
		UMaterialInstance* FamilyMaterial = Entry.Value.TerrainMaterial.LoadSynchronous();
		if (!TestNotNull(*(Context + TEXT(" has a terrain material")), FamilyMaterial)) continue;
		const UMaterial* BaseMaterial = FamilyMaterial->GetMaterial();
		if (TestNotNull(*(Context + TEXT(" resolves a base material")), BaseMaterial))
		{
			TestTrue(*(Context + TEXT(" resolves a non-empty WorldScape graph")),
				BaseMaterial->GetExpressions().Num() > 0);
		}

		if (Entry.Value.LiquidType != EAPSPlanetLiquidType::None)
		{
			UMaterialInstance* LiquidMaterial = Entry.Value.OceanMaterial.LoadSynchronous();
			if (!TestNotNull(*(Context + TEXT(" has its authored liquid material")), LiquidMaterial))
			{
				continue;
			}
			UMaterialInstance* ExpectedLiquidMaterial = Entry.Value.LiquidType == EAPSPlanetLiquidType::Lava
				? LavaMaterial
				: Entry.Value.LiquidType == EAPSPlanetLiquidType::Ammonia
					? AmmoniaMaterial
					: WaterMaterial;
			TestEqual(*(Context + TEXT(" references the canonical material for its liquid")),
				LiquidMaterial, ExpectedLiquidMaterial);
			if (WorldScapeLiquid)
			{
				TestEqual(*(Context + TEXT(" directly uses the depth-writing WorldScape liquid master")),
					LiquidMaterial->Parent.Get(), static_cast<UMaterialInterface*>(WorldScapeLiquid));
				TestEqual(*(Context + TEXT(" resolves no marketplace or overlay graph")),
					LiquidMaterial->GetMaterial(), WorldScapeLiquid);
			}
			float GameplayOpacity = 0.0f;
			if (TestTrue(*(Context + TEXT(" resolves bounded gameplay opacity")),
				APSPlanetSurfaceProfileTests::GetScalarParameter(
					LiquidMaterial, TEXT("Opacity"), GameplayOpacity)))
			{
				TestTrue(*(Context + TEXT(" keeps ground relief visible through the liquid")),
					GameplayOpacity >= 0.05f && GameplayOpacity <= 0.42f);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceDeterminismAndModifiersTest,
	"APS.Gameplay.World.PlanetSurface.DeterminismAndModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceDeterminismAndModifiersTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPlanetSurfaceProfileTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;

	APlanet* Forest = World->SpawnActor<APlanet>();
	Forest->PlanetType = EPlanetType::Forest;
	Forest->RadiusKM = 9000.0;
	Forest->PlanetRadiusKM = 9000;
	Forest->WorldScapeSeed = 88421;
	Forest->PlanetAtmosphere.Humidity = 82.0f;
	Forest->PlanetAtmosphere.AtmosphericPressure = 145000.0f;
	Forest->PlanetBiosphere.Biomass = 7200.0f;
	Forest->PlanetBiosphere.BiodiversityIndexValue = 5.5f;
	Forest->PlanetGeosphere.SeismicActivity = 5.5f;
	Forest->PlanetGeosphere.CrustThickness = 80.0f;

	const FAPSResolvedPlanetSurfaceProfile A = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Forest);
	const FAPSResolvedPlanetSurfaceProfile B = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Forest);
	TestEqual(TEXT("Same seed and model resolve identically"),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(A),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(B));
	TestTrue(TEXT("Large planet receives Super-Earth modifier"), A.HasModifier(EAPSPlanetSurfaceModifier::SuperEarth));
	TestTrue(TEXT("Seismic world receives active geology"), A.HasModifier(EAPSPlanetSurfaceModifier::ActiveGeology));
	TestTrue(TEXT("Dense atmosphere becomes a modifier"), A.HasModifier(EAPSPlanetSurfaceModifier::DenseAtmosphere));
	TestTrue(TEXT("Living model receives rich biosphere"), A.HasModifier(EAPSPlanetSurfaceModifier::RichBiosphere));
	TestTrue(TEXT("Forest remains humid"), A.Humidity >= 0.7f);
	TestEqual(TEXT("Forest uses water"), A.LiquidType, EAPSPlanetLiquidType::Water);

	Forest->SurfaceFeatureScale = 1.5;
	Forest->SurfaceReliefScale = 0.5;
	Forest->SurfaceLandCoverageScale = 0.6;
	Forest->SurfaceMountainScale = 0.5;
	Forest->SurfaceCraterScale = 0.5;
	Forest->SurfaceRoughnessScale = 0.5;
	const FAPSResolvedPlanetSurfaceProfile Tuned = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Forest);
	TestTrue(TEXT("Feature control changes resolved scale"), Tuned.NoiseScale > A.NoiseScale);
	TestTrue(TEXT("Relief control changes resolved amplitude"), Tuned.NoiseIntensity < A.NoiseIntensity);
	TestNotEqual(TEXT("Surface controls participate in profile identity"),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(A),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Tuned));
	Forest->SurfaceFeatureScale = 1.0;
	Forest->SurfaceReliefScale = 1.0;
	Forest->SurfaceLandCoverageScale = 1.0;
	Forest->SurfaceMountainScale = 1.0;
	Forest->SurfaceCraterScale = 1.0;
	Forest->SurfaceRoughnessScale = 1.0;

	Forest->WorldScapeSeed++;
	const FAPSResolvedPlanetSurfaceProfile DifferentSeed = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Forest);
	TestNotEqual(TEXT("Different seed changes resolved profile"),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(A),
		UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(DifferentSeed));

	Forest->PlanetType = EPlanetType::Lava;
	const FAPSResolvedPlanetSurfaceProfile Lava = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Forest);
	TestEqual(TEXT("Lava uses magmatic generator"), Lava.Archetype, EAPSPlanetSurfaceArchetype::Magmatic);
	TestEqual(TEXT("Lava uses lava ocean"), Lava.LiquidType, EAPSPlanetLiquidType::Lava);
	TestTrue(TEXT("Lava has emissive surface"), Lava.EmissiveStrength > 0.0f);

	APSPlanetSurfaceProfileTests::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceLandCoverageResponseTest,
	"APS.Gameplay.World.PlanetSurface.LandCoverageResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceLandCoverageResponseTest::RunTest(const FString& Parameters)
{
	FAPSResolvedPlanetSurfaceProfile LowLandProfile;
	LowLandProfile.PlanetType = EPlanetType::Water;
	LowLandProfile.Archetype = EAPSPlanetSurfaceArchetype::Oceanic;
	LowLandProfile.LiquidType = EAPSPlanetLiquidType::Water;
	LowLandProfile.TerrainSeed = 918273;
	LowLandProfile.BiomeSeed = 817263;
	LowLandProfile.LandCoverage = 0.15f;
	LowLandProfile.MountainStrength = 0.0f;
	LowLandProfile.HillStrength = 0.0f;
	LowLandProfile.CraterStrength = 0.0f;
	LowLandProfile.OceanLevel = 0.0125f;
	LowLandProfile.Temperature = 0.55f;
	LowLandProfile.Humidity = 0.65f;

	FAPSResolvedPlanetSurfaceProfile HighLandProfile = LowLandProfile;
	HighLandProfile.LandCoverage = 0.85f;

	UAPSWorldScapePlanetNoise* LowLandNoise = NewObject<UAPSWorldScapePlanetNoise>();
	UAPSWorldScapePlanetNoise* HighLandNoise = NewObject<UAPSWorldScapePlanetNoise>();
	if (!TestNotNull(TEXT("Low-land noise instance"), LowLandNoise)
		|| !TestNotNull(TEXT("High-land noise instance"), HighLandNoise))
	{
		return false;
	}
	LowLandNoise->Configure(LowLandProfile);
	HighLandNoise->Configure(HighLandProfile);
	constexpr int32 SampleNoiseSeed = 424242;
	CustomNoise SampleNoise(SampleNoiseSeed);
	SampleNoise.SetSeed(SampleNoiseSeed + 1);
	SampleNoise.SetSeed(SampleNoiseSeed);

	constexpr double PlanetScale = 120000000.0;
	constexpr double NoiseScale = 650.0;
	constexpr double NoiseIntensity = 900000.0;
	const FVector SampleDirections[] =
	{
		FVector(1.0, 0.0, 0.0), FVector(-1.0, 0.0, 0.0),
		FVector(0.0, 1.0, 0.0), FVector(0.0, -1.0, 0.0),
		FVector(0.0, 0.0, 1.0), FVector(0.0, 0.0, -1.0),
		FVector(1.0, 1.0, 1.0), FVector(-1.0, 1.0, 1.0),
		FVector(1.0, -1.0, 1.0), FVector(1.0, 1.0, -1.0),
		FVector(-1.0, -1.0, 1.0), FVector(-1.0, 1.0, -1.0),
		FVector(1.0, -1.0, -1.0), FVector(-1.0, -1.0, -1.0),
		FVector(2.0, 1.0, 0.5), FVector(-2.0, 1.0, 0.5),
		FVector(0.5, 2.0, 1.0), FVector(0.5, -2.0, 1.0),
		FVector(1.0, 0.5, 2.0), FVector(1.0, 0.5, -2.0)
	};

	double LowHeightSum = 0.0;
	double HighHeightSum = 0.0;
	double LowWaterSum = 0.0;
	double HighWaterSum = 0.0;
	int32 StrictlyHigherSamples = 0;
	for (FVector Direction : SampleDirections)
	{
		Direction.Normalize();
		const DVector Position(
			Direction.X * PlanetScale,
			Direction.Y * PlanetScale,
			Direction.Z * PlanetScale);
		DVector LowNoisePosition;
		DVector HighNoisePosition;
		const FNoiseData Low = LowLandNoise->GetNoise(
			SampleNoise, Position, DVector(0.0), NoiseScale, NoiseIntensity,
			PlanetScale, false, Direction.Z, LowNoisePosition, FNoiseData(), true);
		const FNoiseData High = HighLandNoise->GetNoise(
			SampleNoise, Position, DVector(0.0), NoiseScale, NoiseIntensity,
			PlanetScale, false, Direction.Z, HighNoisePosition, FNoiseData(), true);

		const double LowPhysicalHeight = Low.Height / NoiseIntensity;
		const double HighPhysicalHeight = High.Height / NoiseIntensity;
		LowHeightSum += LowPhysicalHeight;
		HighHeightSum += HighPhysicalHeight;
		LowWaterSum += Low.WaterMask;
		HighWaterSum += High.WaterMask;
		if (HighPhysicalHeight > LowPhysicalHeight + 0.075)
		{
			++StrictlyHigherSamples;
		}
	}

	constexpr int32 SampleCountInt = static_cast<int32>(UE_ARRAY_COUNT(SampleDirections));
	const double SampleCount = static_cast<double>(SampleCountInt);
	TestTrue(TEXT("Increasing land coverage raises deterministic mean terrain"),
		HighHeightSum / SampleCount > LowHeightSum / SampleCount + 0.075);
	TestEqual(TEXT("Every deterministic sample responds monotonically to land coverage"),
		StrictlyHigherSamples, SampleCountInt);
	TestTrue(TEXT("Increasing land coverage reduces the sampled ocean mask"),
		HighWaterSum < LowWaterSum);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceThermalSubtypeBoundsTest,
	"APS.Gameplay.World.PlanetSurface.ThermalSubtypeBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceThermalSubtypeBoundsTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPlanetSurfaceProfileTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Thermal test planet"), Planet))
	{
		APSPlanetSurfaceProfileTests::DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->WorldScapeSeed = 774411;

	auto ResolveAtKelvin = [Planet](EPlanetType Type, int32 Kelvin)
	{
		Planet->PlanetType = Type;
		Planet->Temperature = Kelvin;
		return UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
	};

	const FAPSResolvedPlanetSurfaceProfile Temperate = ResolveAtKelvin(EPlanetType::Terrestrial, 288);
	TestTrue(TEXT("Earth-like Kelvin input no longer saturates the temperature channel"),
		Temperate.Temperature > 0.30f && Temperate.Temperature < 0.70f);

	const FAPSResolvedPlanetSurfaceProfile Rogue = ResolveAtKelvin(EPlanetType::Rogue, 330);
	const FAPSResolvedPlanetSurfaceProfile Frozen = ResolveAtKelvin(EPlanetType::Frozen, 330);
	const FAPSResolvedPlanetSurfaceProfile Ice = ResolveAtKelvin(EPlanetType::Ice, 330);
	const FAPSResolvedPlanetSurfaceProfile Tundra = ResolveAtKelvin(EPlanetType::Tundra, 330);
	TestTrue(TEXT("Rogue thermal ceiling"), Rogue.Temperature <= 0.1001f);
	TestTrue(TEXT("Frozen thermal ceiling"), Frozen.Temperature <= 0.1601f);
	TestTrue(TEXT("Ice thermal ceiling"), Ice.Temperature <= 0.2201f);
	TestTrue(TEXT("Tundra thermal ceiling"), Tundra.Temperature <= 0.3201f);
	TestTrue(TEXT("Cold presets retain deterministic thermal ordering"),
		Rogue.Temperature < Frozen.Temperature
		&& Frozen.Temperature < Ice.Temperature
		&& Ice.Temperature < Tundra.Temperature);

	const FAPSResolvedPlanetSurfaceProfile Greenhouse = ResolveAtKelvin(EPlanetType::Greenhouse, 180);
	const FAPSResolvedPlanetSurfaceProfile Magmatic = ResolveAtKelvin(EPlanetType::Volcanic, 180);
	TestTrue(TEXT("Greenhouse thermal floor"), Greenhouse.Temperature >= 0.8199f);
	TestTrue(TEXT("Magmatic thermal floor"), Magmatic.Temperature >= 0.8799f);
	TestTrue(TEXT("Hot and cold subtypes remain visually separable"),
		Greenhouse.Temperature - Tundra.Temperature >= 0.49f);

	const FAPSResolvedPlanetSurfaceProfile Ammonia = ResolveAtKelvin(EPlanetType::Ammonia, 180);
	const FAPSResolvedPlanetSurfaceProfile Exoplanet = ResolveAtKelvin(EPlanetType::Exoplanet, 180);
	TestEqual(TEXT("Ammonia preset retains ammonia liquid"),
		Ammonia.LiquidType, EAPSPlanetLiquidType::Ammonia);
	TestEqual(TEXT("Generic exoplanet does not inherit ammonia liquid"),
		Exoplanet.LiquidType, EAPSPlanetLiquidType::None);

	Planet->Destroy();
	APSPlanetSurfaceProfileTests::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfacePresetFieldDiversityTest,
	"APS.Gameplay.World.PlanetSurface.PresetFieldDiversity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfacePresetFieldDiversityTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Preset sampling planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->WorldScapeSeed = 73531;
	Planet->Temperature = 288;
	Planet->PlanetAtmosphere.Humidity = 42.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
	Planet->PlanetGeosphere.SeismicActivity = 2.0f;
	Planet->PlanetGeosphere.CrustThickness = 38.0f;

	TMap<EPlanetType, FSurfaceFieldStats> Samples;
	TMap<uint32, EPlanetType> TypeByFieldHash;
	for (uint8 Value = 0; Value <= static_cast<uint8>(EPlanetType::Unknown); ++Value)
	{
		const EPlanetType Type = static_cast<EPlanetType>(Value);
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type)) continue;
		Planet->PlanetType = Type;
		const FAPSResolvedPlanetSurfaceProfile Profile =
			UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
		const FSurfaceFieldStats First = SampleSurfaceFields(Profile);
		const FSurfaceFieldStats Second = SampleSurfaceFields(Profile);
		const FString TypeName = StaticEnum<EPlanetType>()->GetNameStringByValue(Value);
		TestEqual(*(TypeName + TEXT(" sampled field is deterministic")),
			First.FieldHash, Second.FieldHash);

		if (const EPlanetType* ExistingType = TypeByFieldHash.Find(First.FieldHash))
		{
			const FString ExistingName = StaticEnum<EPlanetType>()->GetNameStringByValue(
				static_cast<int64>(*ExistingType));
			AddError(FString::Printf(TEXT("Preset field duplicate: %s and %s -> %u"),
				*ExistingName, *TypeName, First.FieldHash));
		}
		else
		{
			TypeByFieldHash.Add(First.FieldHash, Type);
		}
		Samples.Add(Type, First);
	}

	struct FRelatedPresetPair
	{
		EPlanetType A;
		EPlanetType B;
		const TCHAR* Label;
	};
	const FRelatedPresetPair RelatedPairs[] =
	{
		{EPlanetType::Rocky, EPlanetType::Dwarf, TEXT("Rocky / Dwarf")},
		{EPlanetType::Terrestrial, EPlanetType::Pangea, TEXT("Terrestrial / Pangea")},
		{EPlanetType::Terrestrial, EPlanetType::Nordic, TEXT("Terrestrial / Nordic")},
		{EPlanetType::SuperEarth, EPlanetType::HighMountain, TEXT("Super-Earth / High Mountain")},
		{EPlanetType::Ocean, EPlanetType::Water, TEXT("Ocean / Water")},
		{EPlanetType::Water, EPlanetType::Archipelago, TEXT("Water / Archipelago")},
		{EPlanetType::Forest, EPlanetType::Oasis, TEXT("Forest / Oasis")},
		{EPlanetType::Greenhouse, EPlanetType::Desert, TEXT("Greenhouse / Desert")},
		{EPlanetType::Desert, EPlanetType::Sand, TEXT("Desert / Sand")},
		{EPlanetType::Ice, EPlanetType::Frozen, TEXT("Ice / Frozen")},
		{EPlanetType::Frozen, EPlanetType::Tundra, TEXT("Frozen / Tundra")},
		{EPlanetType::Volcanic, EPlanetType::Melted, TEXT("Volcanic / Melted")},
		{EPlanetType::Melted, EPlanetType::Lava, TEXT("Melted / Lava")},
		{EPlanetType::Metal, EPlanetType::Metallic, TEXT("Metal / Metallic")},
		{EPlanetType::Metallic, EPlanetType::Carbon, TEXT("Metallic / Carbon")},
		{EPlanetType::Ammonia, EPlanetType::Exoplanet, TEXT("Ammonia / Exoplanet")}
	};
	for (const FRelatedPresetPair& Pair : RelatedPairs)
	{
		const FSurfaceFieldStats* A = Samples.Find(Pair.A);
		const FSurfaceFieldStats* B = Samples.Find(Pair.B);
		if (!TestNotNull(FString::Printf(TEXT("%s first sample"), Pair.Label), A)
			|| !TestNotNull(FString::Printf(TEXT("%s second sample"), Pair.Label), B))
		{
			continue;
		}
		const double Distance = VisualStatsDistance(*A, *B);
		TestTrue(FString::Printf(TEXT("%s has materially different sampled statistics (%.3f)"),
			Pair.Label, Distance), Distance >= 0.18);
	}

	if (const FSurfaceFieldStats* Forest = Samples.Find(EPlanetType::Forest))
	{
		TestTrue(TEXT("Forest writes an orbital-scale humidity patch signal"),
			Forest->HumidityDeviation >= 0.045);
		TestTrue(TEXT("Forest writes a non-zero vegetation field"), Forest->MeanFoliage >= 0.08);
	}

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceLandFractionCalibrationTest,
	"APS.Gameplay.World.PlanetSurface.LandFractionCalibration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceLandFractionCalibrationTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	struct FTarget
	{
		EPlanetType Type;
		double MinimumLand;
		double MaximumLand;
	};
	const FTarget Targets[] =
	{
		{EPlanetType::Rocky, 0.97, 1.00},
		{EPlanetType::Terrestrial, 0.50, 0.65},
		{EPlanetType::Greenhouse, 0.97, 1.00},
		{EPlanetType::Melted, 0.35, 0.48},
		{EPlanetType::Dwarf, 0.97, 1.00},
		{EPlanetType::Ocean, 0.04, 0.12},
		{EPlanetType::Water, 0.30, 0.45},
		{EPlanetType::Desert, 0.97, 1.00},
		{EPlanetType::Forest, 0.58, 0.75},
		{EPlanetType::Volcanic, 0.82, 0.94},
		{EPlanetType::Ice, 0.97, 1.00},
		{EPlanetType::Frozen, 0.97, 1.00},
		{EPlanetType::Ammonia, 0.42, 0.58},
		{EPlanetType::Metal, 0.97, 1.00},
		{EPlanetType::Carbon, 0.97, 1.00},
		{EPlanetType::SuperEarth, 0.60, 0.78},
		{EPlanetType::Lava, 0.18, 0.30},
		{EPlanetType::Metallic, 0.97, 1.00},
		{EPlanetType::Nordic, 0.55, 0.70},
		{EPlanetType::Tundra, 0.82, 0.95},
		{EPlanetType::HighMountain, 0.68, 0.84},
		{EPlanetType::Sand, 0.97, 1.00},
		{EPlanetType::Oasis, 0.90, 0.97},
		{EPlanetType::Archipelago, 0.18, 0.30},
		{EPlanetType::Pangea, 0.75, 0.88},
		{EPlanetType::Rogue, 0.97, 1.00},
		{EPlanetType::Exoplanet, 0.72, 0.90},
		{EPlanetType::Unknown, 0.55, 0.80}
	};
	const int32 Seeds[] = {10101, 27271, 49333, 88421};
	constexpr int32 SamplesPerSeed = 4096;

	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Calibration world"), World)) return false;
	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Calibration planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->Temperature = 288;
	Planet->PlanetAtmosphere.Humidity = 42.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
	const UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	TestNotNull(TEXT("Calibration uses the project surface catalog"), Catalog);

	for (const FTarget& Target : Targets)
	{
		Planet->PlanetType = Target.Type;
		double LandSum = 0.0;
		double MinimumSeedLand = 1.0;
		double MaximumSeedLand = 0.0;
		double MaximumClampFraction = 0.0;
		double MinimumReliefSpan = TNumericLimits<double>::Max();
		double MinimumPaletteSpread = TNumericLimits<double>::Max();
		double MaximumPaletteSaturation = 0.0;
		bool bCryogenicTarget = false;
		for (const int32 Seed : Seeds)
		{
			Planet->WorldScapeSeed = Seed;
			const FAPSResolvedPlanetSurfaceProfile Profile =
				UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
			const FSurfaceCalibrationStats Stats =
				SampleSurfaceCalibration(Profile, Seed, SamplesPerSeed);
			LandSum += Stats.LandFraction;
			MinimumSeedLand = FMath::Min(MinimumSeedLand, Stats.LandFraction);
			MaximumSeedLand = FMath::Max(MaximumSeedLand, Stats.LandFraction);
			MaximumClampFraction = FMath::Max(MaximumClampFraction, Stats.ClampFraction);
			MinimumReliefSpan = FMath::Min(MinimumReliefSpan, Stats.HeightP99 - Stats.HeightP01);
			MinimumPaletteSpread = FMath::Min(
				MinimumPaletteSpread, Stats.PaletteP90 - Stats.PaletteP10);
			MaximumPaletteSaturation = FMath::Max(
				MaximumPaletteSaturation, Stats.PaletteSaturationFraction);
			bCryogenicTarget = bCryogenicTarget
				|| Profile.Archetype == EAPSPlanetSurfaceArchetype::Cryogenic;
		}

		const double MeanLand = LandSum / UE_ARRAY_COUNT(Seeds);
		const FString TypeName = StaticEnum<EPlanetType>()->GetNameStringByValue(
			static_cast<int64>(Target.Type));
		AddInfo(FString::Printf(
			TEXT("Surface calibration %s land mean=%.4f seeds=[%.4f, %.4f] target=[%.2f, %.2f] p99-p01>=%.4f clamp<=%.4f palette[p90-p10>=%.4f saturation<=%.4f]"),
			*TypeName, MeanLand, MinimumSeedLand, MaximumSeedLand,
			Target.MinimumLand, Target.MaximumLand, MinimumReliefSpan, MaximumClampFraction,
			MinimumPaletteSpread, MaximumPaletteSaturation));
		TestTrue(*(TypeName + TEXT(" mean land fraction is in its authored band")),
			MeanLand >= Target.MinimumLand && MeanLand <= Target.MaximumLand);
		const double SeedToleranceMinimum = FMath::Max(0.0, Target.MinimumLand - 0.15);
		const double SeedToleranceMaximum = FMath::Min(1.0, Target.MaximumLand + 0.15);
		TestTrue(*(TypeName + TEXT(" has no catastrophic all-ocean/all-land seed")),
			MinimumSeedLand >= SeedToleranceMinimum && MaximumSeedLand <= SeedToleranceMaximum);
		TestTrue(*(TypeName + TEXT(" has a non-flat robust relief span")), MinimumReliefSpan >= 0.008);
		TestTrue(*(TypeName + TEXT(" does not saturate the height clamp")), MaximumClampFraction <= 0.005);
		TestTrue(*(TypeName + TEXT(" preserves material elevation contrast across dry terrain")),
			MinimumPaletteSpread >= 0.08);
		TestTrue(*(TypeName + TEXT(" does not pin the WorldScape height palette to 0/1")),
			MaximumPaletteSaturation <= 0.02);
		if (bCryogenicTarget)
		{
			TestTrue(*(TypeName + TEXT(" Cryogenic palette keeps a clearly readable elevation spread")),
				MinimumPaletteSpread >= 0.14);
		}
	}

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceControlSensitivityTest,
	"APS.Gameplay.World.PlanetSurface.SurfaceControlSensitivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceControlSensitivityTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	constexpr int32 SampleCount = 2048;
	constexpr int32 Seed = 551923;

	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Surface-control world"), World)) return false;
	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Surface-control planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->PlanetType = EPlanetType::Terrestrial;
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->WorldScapeSeed = Seed;
	Planet->Temperature = 288;
	Planet->PlanetAtmosphere.Humidity = 42.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
	Planet->PlanetGeosphere.SeismicActivity = 0.0f;
	Planet->PlanetGeosphere.CrustThickness = 35.0f;

	auto ResetControls = [Planet]()
	{
		Planet->SurfaceFeatureScale = 1.0;
		Planet->SurfaceReliefScale = 1.0;
		Planet->SurfaceLandCoverageScale = 1.0;
		Planet->SurfaceMountainScale = 1.0;
		Planet->SurfaceCraterScale = 1.0;
		Planet->SurfaceRoughnessScale = 1.0;
	};
	auto Resolve = [Planet]()
	{
		return UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
	};
	auto Sample = [=](const FAPSResolvedPlanetSurfaceProfile& Profile)
	{
		return SampleSurfaceFields(Profile, Seed, SampleCount);
	};

	ResetControls();
	Planet->SurfaceFeatureScale = 0.25;
	const FAPSResolvedPlanetSurfaceProfile FeatureLowProfile = Resolve();
	const FSurfaceFieldStats FeatureLow = Sample(FeatureLowProfile);
	Planet->SurfaceFeatureScale = 4.0;
	const FAPSResolvedPlanetSurfaceProfile FeatureHighProfile = Resolve();
	const FSurfaceFieldStats FeatureHigh = Sample(FeatureHighProfile);
	TestTrue(TEXT("Feature endpoints remain ordered without early clamp saturation"),
		FeatureLowProfile.NoiseScale < FeatureHighProfile.NoiseScale);
	TestNotEqual(TEXT("Feature endpoints produce different sampled terrain"),
		FeatureLow.FieldHash, FeatureHigh.FieldHash);
	const float FeatureLowMaterialWarpScale =
		UAPSPlanetSurfaceProfileResolver::ResolveMaterialWarpScale(
			FeatureLowProfile.MaterialFamily);
	const float FeatureHighMaterialWarpScale =
		UAPSPlanetSurfaceProfileResolver::ResolveMaterialWarpScale(
			FeatureHighProfile.MaterialFamily);
	TestEqual(TEXT("UI feature scale does not retile the WorldScape material"),
		FeatureLowMaterialWarpScale, FeatureHighMaterialWarpScale);
	TestTrue(TEXT("WorldScape material warp scale stays inside its display-safe bounds"),
		FeatureLowMaterialWarpScale >= 0.75f && FeatureLowMaterialWarpScale <= 1.25f);
	const float TemperateClimateBlend =
		UAPSPlanetSurfaceProfileResolver::ResolveMaterialClimateBlend(
			EAPSPlanetSurfaceArchetype::Temperate);
	const float BiosphereClimateBlend =
		UAPSPlanetSurfaceProfileResolver::ResolveMaterialClimateBlend(
			EAPSPlanetSurfaceArchetype::Biosphere);
	const float MetallicClimateBlend =
		UAPSPlanetSurfaceProfileResolver::ResolveMaterialClimateBlend(
			EAPSPlanetSurfaceArchetype::Metallic);
	TestTrue(TEXT("Canonical terrain climate tint stays subordinate to elevation"),
		TemperateClimateBlend >= 0.04f && TemperateClimateBlend <= 0.20f);
	TestTrue(TEXT("Living worlds retain more climate variation than metallic worlds"),
		BiosphereClimateBlend > MetallicClimateBlend);

	ResetControls();
	Planet->SurfaceReliefScale = 0.25;
	const FAPSResolvedPlanetSurfaceProfile ReliefLowProfile = Resolve();
	const FSurfaceFieldStats ReliefLow = Sample(ReliefLowProfile);
	Planet->SurfaceReliefScale = 2.5;
	const FAPSResolvedPlanetSurfaceProfile ReliefHighProfile = Resolve();
	const FSurfaceFieldStats ReliefHigh = Sample(ReliefHighProfile);
	TestTrue(TEXT("Relief endpoints remain ordered without early clamp saturation"),
		ReliefLowProfile.NoiseIntensity < ReliefHighProfile.NoiseIntensity);
	TestTrue(TEXT("Relief has a material physical-height response"),
		ReliefHigh.PhysicalHeightRange > ReliefLow.PhysicalHeightRange * 2.0);

	ResetControls();
	Planet->SurfaceLandCoverageScale = 0.25;
	const FAPSResolvedPlanetSurfaceProfile LandLowProfile = Resolve();
	const FSurfaceCalibrationStats LandLow =
		SampleSurfaceCalibration(LandLowProfile, Seed, SampleCount);
	Planet->SurfaceLandCoverageScale = 2.0;
	const FAPSResolvedPlanetSurfaceProfile LandHighProfile = Resolve();
	const FSurfaceCalibrationStats LandHigh =
		SampleSurfaceCalibration(LandHighProfile, Seed, SampleCount);
	TestTrue(TEXT("Land endpoints remain ordered without early clamp saturation"),
		LandLowProfile.LandCoverage < LandHighProfile.LandCoverage);
	TestTrue(TEXT("Land coverage visibly changes ocean/land area"),
		LandHigh.LandFraction > LandLow.LandFraction + 0.45);

	ResetControls();
	Planet->SurfaceMountainScale = 0.0;
	const FAPSResolvedPlanetSurfaceProfile MountainLowProfile = Resolve();
	const FSurfaceCalibrationStats MountainLow =
		SampleSurfaceCalibration(MountainLowProfile, Seed, SampleCount);
	const FSurfaceFieldStats MountainLowField = Sample(MountainLowProfile);
	Planet->SurfaceMountainScale = 2.0;
	const FAPSResolvedPlanetSurfaceProfile MountainHighProfile = Resolve();
	const FSurfaceCalibrationStats MountainHigh =
		SampleSurfaceCalibration(MountainHighProfile, Seed, SampleCount);
	const FSurfaceFieldStats MountainHighField = Sample(MountainHighProfile);
	TestTrue(TEXT("Mountain endpoints remain ordered even for authored-zero families"),
		MountainLowProfile.MountainStrength < MountainHighProfile.MountainStrength);
	TestNotEqual(TEXT("Mountain endpoints produce different sampled terrain"),
		MountainLowField.FieldHash, MountainHighField.FieldHash);
	TestTrue(TEXT("Mountain control raises the robust upper terrain tail"),
		MountainHigh.HeightP99 > MountainLow.HeightP99 + 0.004);

	ResetControls();
	Planet->SurfaceCraterScale = 0.0;
	const FAPSResolvedPlanetSurfaceProfile CraterLowProfile = Resolve();
	const FSurfaceCalibrationStats CraterLow =
		SampleSurfaceCalibration(CraterLowProfile, Seed, SampleCount);
	const FSurfaceFieldStats CraterLowField = Sample(CraterLowProfile);
	Planet->SurfaceCraterScale = 2.0;
	const FAPSResolvedPlanetSurfaceProfile CraterHighProfile = Resolve();
	const FSurfaceCalibrationStats CraterHigh =
		SampleSurfaceCalibration(CraterHighProfile, Seed, SampleCount);
	const FSurfaceFieldStats CraterHighField = Sample(CraterHighProfile);
	TestTrue(TEXT("Crater endpoints remain ordered even for authored-zero families"),
		CraterLowProfile.CraterStrength < CraterHighProfile.CraterStrength);
	TestNotEqual(TEXT("Crater endpoints produce different sampled terrain"),
		CraterLowField.FieldHash, CraterHighField.FieldHash);
	TestTrue(TEXT("Crater control lowers the robust terrain floor"),
		CraterHigh.HeightP01 < CraterLow.HeightP01 - 0.003);

	ResetControls();
	Planet->SurfaceRoughnessScale = 0.25;
	const FAPSResolvedPlanetSurfaceProfile RoughnessLowProfile = Resolve();
	const FSurfaceFieldStats RoughnessLow = Sample(RoughnessLowProfile);
	Planet->SurfaceRoughnessScale = 2.0;
	const FAPSResolvedPlanetSurfaceProfile RoughnessHighProfile = Resolve();
	const FSurfaceFieldStats RoughnessHigh = Sample(RoughnessHighProfile);
	TestTrue(TEXT("Roughness endpoints remain ordered without early clamp saturation"),
		RoughnessLowProfile.Roughness < RoughnessHighProfile.Roughness);
	TestNotEqual(TEXT("Roughness endpoints alter sampled micro-detail"),
		RoughnessLow.FieldHash, RoughnessHigh.FieldHash);

	ResetControls();
	const FAPSResolvedPlanetSurfaceProfile FirstSeedProfile = Resolve();
	const FSurfaceFieldStats FirstSeed = Sample(FirstSeedProfile);
	Planet->WorldScapeSeed = Seed + 1;
	const FAPSResolvedPlanetSurfaceProfile SecondSeedProfile = Resolve();
	const FSurfaceFieldStats SecondSeed = SampleSurfaceFields(SecondSeedProfile, Seed + 1, SampleCount);
	TestNotEqual(TEXT("Seed changes the sampled terrain pattern"),
		FirstSeed.FieldHash, SecondSeed.FieldHash);

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetSurfaceRuntimeIsolationTest,
	"APS.Gameplay.World.PlanetSurface.RuntimeIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceRuntimeIsolationTest::RunTest(const FString& Parameters)
{
	UWorld* World = APSPlanetSurfaceProfileTests::CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;

	APlanet* First = World->SpawnActor<APlanet>();
	APlanet* Second = World->SpawnActor<APlanet>();
	APlanet* DryExoplanet = World->SpawnActor<APlanet>();
	First->PlanetType = EPlanetType::Terrestrial;
	Second->PlanetType = EPlanetType::Terrestrial;
	DryExoplanet->PlanetType = EPlanetType::Exoplanet;
	First->RadiusKM = Second->RadiusKM = 6371.0;
	First->PlanetRadiusKM = Second->PlanetRadiusKM = 6371;
	DryExoplanet->RadiusKM = 6371.0;
	DryExoplanet->PlanetRadiusKM = 6371;
	First->WorldScapeSeed = 101;
	Second->WorldScapeSeed = 202;
	DryExoplanet->WorldScapeSeed = 303;

	First->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	Second->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	DryExoplanet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Preloaded);
	APlanetarySurfaceGenerator* FirstGenerator = First->PlanetaryEnvironmentGenerator;
	APlanetarySurfaceGenerator* SecondGenerator = Second->PlanetaryEnvironmentGenerator;
	if (TestNotNull(TEXT("First generator"), FirstGenerator)
		&& TestNotNull(TEXT("Second generator"), SecondGenerator))
	{
		TestNotNull(TEXT("First uses custom noise"), Cast<UAPSWorldScapePlanetNoise>(FirstGenerator->ResolvedNoiseInstance));
		TestNotNull(TEXT("Second uses custom noise"), Cast<UAPSWorldScapePlanetNoise>(SecondGenerator->ResolvedNoiseInstance));
		TestNotEqual(TEXT("Planets never share mutable noise instances"),
			FirstGenerator->ResolvedNoiseInstance, SecondGenerator->ResolvedNoiseInstance);
		TestNotEqual(TEXT("Planets never share dynamic terrain materials"),
			FirstGenerator->ResolvedTerrainMaterialInstance, SecondGenerator->ResolvedTerrainMaterialInstance);
		TestNotEqual(TEXT("Different seeds produce different surface signatures"),
			FirstGenerator->AppliedSurfaceProfileSignature, SecondGenerator->AppliedSurfaceProfileSignature);
	}
	APlanetarySurfaceGenerator* DryGenerator = DryExoplanet->PlanetaryEnvironmentGenerator;
	if (TestNotNull(TEXT("Dry exoplanet generator"), DryGenerator)
		&& TestNotNull(TEXT("Dry exoplanet root"),
			DryGenerator ? DryGenerator->WorldScapeRootInstance : nullptr))
	{
		TestEqual(TEXT("Dry subtype keeps its resolver liquid selection"),
			DryGenerator->ResolvedSurfaceProfile.LiquidType, EAPSPlanetLiquidType::None);
		TestNull(TEXT("Dry subtype does not inherit its archetype's ammonia material"),
			DryGenerator->ResolvedOceanMaterialInstance);
		TestNull(TEXT("Dry WorldScape root has no stale ocean material"),
			DryGenerator->WorldScapeRootInstance->OceanMaterial.DefaultMaterial);
		TestFalse(TEXT("Dry WorldScape root keeps ocean generation disabled"),
			DryGenerator->WorldScapeRootInstance->bOcean);
	}

	First->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	Second->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	DryExoplanet->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
	APSPlanetSurfaceProfileTests::DestroyWorld(World);
	return true;
}

#endif
