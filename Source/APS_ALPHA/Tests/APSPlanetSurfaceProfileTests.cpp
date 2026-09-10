#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVectorNoise.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

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
		// 10 m foot-scale window. Geometry at this radius is a band-limit safety
		// signal, not the visual micro-detail source: the continuous world-space
		// terrain material owns sub-60 m albedo, roughness and normal variation.
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
		FLinearColor ResolvedValue = FLinearColor::Black;
		if (Material->GetVectorParameterValue(
			FHashedMaterialParameterInfo(Name), ResolvedValue))
		{
			return true;
		}
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

	bool IsMaterialParameterReachable(
		UMaterialExpression* Expression, const FName ParameterName,
		TSet<const UMaterialExpression*>& Visited)
	{
		if (!IsValid(Expression) || Visited.Contains(Expression)) return false;
		Visited.Add(Expression);
		if (const UMaterialExpressionVectorParameter* Vector =
			Cast<UMaterialExpressionVectorParameter>(Expression))
		{
			if (Vector->ParameterName == ParameterName) return true;
		}
		if (const UMaterialExpressionScalarParameter* Scalar =
			Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			if (Scalar->ParameterName == ParameterName) return true;
		}
		for (FExpressionInput* Input : Expression->GetInputsView())
		{
			if (Input && IsMaterialParameterReachable(
				Input->Expression, ParameterName, Visited))
			{
				return true;
			}
		}
		return false;
	}

	bool IsMaterialParameterReachableFromProperty(
		UMaterial* Material, EMaterialProperty Property, const TCHAR* ParameterName)
	{
		if (!IsValid(Material)) return false;
		const FExpressionInput* PropertyInput =
			Material->GetExpressionInputForProperty(Property);
		TSet<const UMaterialExpression*> Visited;
		return PropertyInput && IsMaterialParameterReachable(
			PropertyInput->Expression, FName(ParameterName), Visited);
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

		// Require coherent macro/collision geometry at gameplay-relevant radii. Visual
		// micro-detail is intentionally material-only: WorldScape's noise callback has
		// no LOD argument, so forcing 14/22 m displacement into every clipmap ring makes
		// the nested square patch topology visible. The 10 m window is therefore bounded
		// relative to the 100 m landform instead of requiring high-frequency relief.
		const double RequiredWalkRangeCm = FMath::Max(
			300.0, static_cast<double>(Profile.NoiseIntensity) * 0.00030);
		const double RequiredNearRangeCm = FMath::Max(
			700.0, static_cast<double>(Profile.NoiseIntensity) * 0.00070);
		const double RequiredLocalRangeCm = FMath::Max(
			1800.0, static_cast<double>(Profile.NoiseIntensity) * 0.00180);
		const double RequiredRegionalRangeCm = FMath::Max(
			3500.0, static_cast<double>(Profile.NoiseIntensity) * 0.00350);
		TestTrue(*FString::Printf(TEXT("%s keeps 10 m geometry subordinate to 100 m relief (%.2f / %.2f cm)"),
			*TypeName, Stats.MedianFootPatchRangeCm, Stats.MedianWalkPatchRangeCm),
			Stats.MedianFootPatchRangeCm
				<= Stats.MedianWalkPatchRangeCm * 0.60 + 1.0);
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
		TestTrue(*FString::Printf(TEXT("%s exposes a visible local slope (%.6f)"),
			*TypeName, Stats.MaximumSlope),
			Stats.MaximumSlope >= 0.0050);
		TestTrue(*FString::Printf(TEXT("%s does not turn local relief into impassable noise (%.6f)"),
			*TypeName, Stats.MaximumWalkSlope),
			Stats.MaximumWalkSlope <= 0.50);
		TestTrue(*FString::Printf(TEXT("%s band-limited foot-scale geometry remains traversable (%.6f)"),
			*TypeName, Stats.MaximumFootSlope),
			Stats.MaximumFootSlope <= 0.40);

		if (Type == EPlanetType::Frozen)
		{
			// Frozen is the standard generated-gameplay handoff profile. It must show
			// rolling physical terrain from 100 m upward without reintroducing the
			// sub-LOD ripple bands that exposed square clipmap patches.
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
			// High Mountain is the runtime handoff profile and must read as broad physical
			// landforms from a pawn, not as high-frequency ripple geometry.
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
			const DVector OceanDirections[] =
			{
				DVector(1.0, 0.0, 0.0), DVector(-1.0, 0.0, 0.0),
				DVector(0.0, 1.0, 0.0), DVector(0.0, -1.0, 0.0),
				DVector(0.0, 0.0, 1.0), DVector(0.0, 0.0, -1.0),
				DVector(1.0, 1.0, 1.0)
			};
			const double ExpectedOceanHeight =
				static_cast<double>(Profile.OceanLevel) * Profile.NoiseIntensity;
			for (const DVector& RawDirection : OceanDirections)
			{
				DVector Direction = RawDirection;
				Direction.Normalize();
				DVector NoisePosition;
				const FNoiseData OceanData = Noise->GetOceanNoise(
					OceanSampleNoise, Direction * PlanetScale, DVector(0.0),
					Profile.NoiseScale, Profile.NoiseIntensity, PlanetScale, false,
					Direction.Z, NoisePosition, FNoiseData(), true);
				TestTrue(*(TypeName + TEXT(" ocean material height obeys 0..1 contract")),
					OceanData.HeightNormalize >= 0.0 && OceanData.HeightNormalize <= 1.0);
				TestTrue(*(TypeName + TEXT(" ocean material channel is independent of displacement")),
					FMath::IsNearlyEqual(OceanData.HeightNormalize, 0.08,
						UE_DOUBLE_SMALL_NUMBER));
				TestTrue(*(TypeName + TEXT(" ocean is one smooth equipotential surface")),
					FMath::IsNearlyEqual(OceanData.Height, ExpectedOceanHeight, 0.01));
				TestTrue(*(TypeName + TEXT(" ocean never inherits terrain or lava relief")),
					FMath::IsNearlyEqual(OceanData.WaterMask, 1.0f)
						&& FMath::IsNearlyEqual(OceanData.Temperature, Profile.Temperature)
						&& FMath::IsNearlyEqual(OceanData.Humidity, Profile.Humidity));
			}
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
	UMaterial* WorldScapeWaterMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeLivingWater.M_APS_WorldScapeLivingWater"));
	UMaterial* WorldScapeLiquidMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeLiquid.M_APS_WorldScapeLiquid"));
	UMaterialInstance* MarketplaceWaterTemplate = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_Planetary_Ocean.MI_Planetary_Ocean"));
	UMaterial* MarketplaceWaterMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Water_WorldScape.M_Water_WorldScape"));
	UMaterialInstance* OriginalWaterTemplate = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_Original_Water.MI_Original_Water"));
	UMaterial* OriginalWaterMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_OriginalWater_UE5.M_OriginalWater_UE5"));
	UMaterialInstance* WorldScapeLavaTemplate = LoadObject<UMaterialInstance>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/MI_LavaOcean.MI_LavaOcean"));
	UMaterial* WorldScapeLavaMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/WorldScape/Ressources/Materials/WorldScapeMaterials/Ocean/M_Lava_WorldScape.M_Lava_WorldScape"));
	UMaterial* OrbitalLiquid = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalLiquid.M_APS_OrbitalLiquid"));
	UMaterial* OrbitalWaterMaster = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalWater.M_APS_OrbitalWater"));
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
		TestTrue(TEXT("Orbital terrain connects seamless object-centred detail to world normal"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalTerrain, MP_Normal));
		TestFalse(TEXT("Orbital terrain world normal does not depend on mesh tangents"),
			OrbitalTerrain->bTangentSpaceNormal);
		int32 OrbitalGradientNoiseCount = 0;
		for (const UMaterialExpression* Expression : OrbitalTerrain->GetExpressions())
		{
			const UMaterialExpressionVectorNoise* Noise =
				Cast<UMaterialExpressionVectorNoise>(Expression);
			if (Noise && Noise->NoiseFunction == VNF_GradientALU && !Noise->bTiling)
			{
				++OrbitalGradientNoiseCount;
			}
		}
		TestEqual(TEXT("Orbital terrain evaluates exactly one non-tiled GradientALU detail field"),
			OrbitalGradientNoiseCount, 1);
		TestTrue(TEXT("Orbital terrain centres material detail on its selected globe"),
			OrbitalTerrain->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->GetClass()->GetFName()
						== FName(TEXT("MaterialExpressionObjectPositionWS"));
				}));
		TestTrue(TEXT("Orbital terrain retains the closed globe's displaced relief normal"),
			OrbitalTerrain->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionVertexNormalWS>();
				}));
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
				TEXT("Metallic"), TEXT("Specular"), TEXT("OrbitalMicroDetailScale"),
				TEXT("OrbitalMicroColorStrength"), TEXT("OrbitalMicroNormalStrength"),
				TEXT("OrbitalMicroRoughnessStrength")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital terrain exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(OrbitalTerrain, ParameterName));
		}
		struct FExpectedOrbitalMicroDefault
		{
			const TCHAR* ParameterName;
			float ExpectedValue;
		};
		for (const FExpectedOrbitalMicroDefault& Expected :
			{
				FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroDetailScale"), 28.0f},
				FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroColorStrength"), 0.025f},
				FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroNormalStrength"), 0.0375f},
				FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroRoughnessStrength"), 0.010f}
			})
		{
			float ActualValue = 0.0f;
			if (TestTrue(*FString::Printf(TEXT("Orbital terrain resolves default %s"),
				Expected.ParameterName), APSPlanetSurfaceProfileTests::GetScalarParameter(
					OrbitalTerrain, Expected.ParameterName, ActualValue)))
			{
				TestTrue(*FString::Printf(TEXT("Orbital terrain keeps anti-alias-safe default %s"),
					Expected.ParameterName), FMath::IsNearlyEqual(
						ActualValue, Expected.ExpectedValue, 1.0e-4f));
			}
		}
		UMaterialInstanceDynamic* OrbitalRetuneProbe = UMaterialInstanceDynamic::Create(
			OrbitalTerrain, GetTransientPackage());
		if (TestNotNull(TEXT("Orbital terrain creates a runtime retune probe"),
			OrbitalRetuneProbe))
		{
			FAPSResolvedPlanetSurfaceProfile ProbeProfile;
			ProbeProfile.Archetype = EAPSPlanetSurfaceArchetype::Temperate;
			ProbeProfile.DetailFrequencyMultiplier = 1.75f;
			ProbeProfile.TerrainPatternStrength = 1.0f;
			ProbeProfile.Roughness = 1.5f;
			UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
				OrbitalRetuneProbe, ProbeProfile);
			for (const FExpectedOrbitalMicroDefault& Expected :
				{
					FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroDetailScale"), 49.0f},
					FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroColorStrength"), 0.032f},
					FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroNormalStrength"), 0.050f},
					FExpectedOrbitalMicroDefault{TEXT("OrbitalMicroRoughnessStrength"), 0.014f}
				})
			{
				float ActualValue = 0.0f;
				if (TestTrue(*FString::Printf(TEXT("Orbital retune probe resolves %s"),
					Expected.ParameterName), APSPlanetSurfaceProfileTests::GetScalarParameter(
						OrbitalRetuneProbe, Expected.ParameterName, ActualValue)))
				{
					TestTrue(*FString::Printf(TEXT("Orbital retune probe bounds %s"),
						Expected.ParameterName), FMath::IsNearlyEqual(
							ActualValue, Expected.ExpectedValue, 1.0e-4f));
				}
			}
			UMaterialInstanceDynamic* ClimateProbe = UMaterialInstanceDynamic::Create(
				OrbitalTerrain, GetTransientPackage());
			if (TestNotNull(TEXT("Orbital terrain creates a subtype climate probe"), ClimateProbe))
			{
				FAPSResolvedPlanetSurfaceProfile MetallicProfile;
				MetallicProfile.Archetype = EAPSPlanetSurfaceArchetype::Metallic;
				MetallicProfile.BiomeContrast = 0.45f;
				MetallicProfile.ClimatePatchStrength = 0.10f;
				UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
					ClimateProbe, MetallicProfile);
				float MetallicClimateBlend = 0.0f;
				const bool bResolvedMetallicClimate =
					APSPlanetSurfaceProfileTests::GetScalarParameter(
					ClimateProbe, TEXT("ClimateBlend"), MetallicClimateBlend);
				TestTrue(TEXT("Metallic subtype resolves a restrained climate signal"),
					bResolvedMetallicClimate);

				FAPSResolvedPlanetSurfaceProfile ForestProfile;
				ForestProfile.Archetype = EAPSPlanetSurfaceArchetype::Biosphere;
				ForestProfile.BiomeContrast = 1.50f;
				ForestProfile.ClimatePatchStrength = 0.90f;
				UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(
					ClimateProbe, ForestProfile);
				float ForestClimateBlend = 0.0f;
				const bool bResolvedForestClimate =
					APSPlanetSurfaceProfileTests::GetScalarParameter(
					ClimateProbe, TEXT("ClimateBlend"), ForestClimateBlend);
				TestTrue(TEXT("Biosphere subtype resolves its stronger biome signal"),
					bResolvedForestClimate);
				if (bResolvedMetallicClimate && bResolvedForestClimate)
				{
					TestTrue(TEXT("Subtype climate structure is more than a palette-only change"),
						ForestClimateBlend >= MetallicClimateBlend + 0.15f);
					TestTrue(TEXT("Subtype climate signal remains subordinate to elevation"),
						MetallicClimateBlend >= 0.04f && ForestClimateBlend <= 0.24f);
				}
			}
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
		int32 NonTiledDetailNoiseCount = 0;
		const UMaterialExpressionVectorParameter* TerrainSeedOffset = nullptr;
		for (const UMaterialExpression* Expression : WorldScapeTerrain->GetExpressions())
		{
			if (const UMaterialExpressionNoise* Noise = Cast<UMaterialExpressionNoise>(Expression))
			{
				++NonTiledDetailNoiseCount;
				TestFalse(TEXT("WorldScape scalar detail noise is explicitly non-tiled"),
					Noise->bTiling);
			}
			else if (const UMaterialExpressionVectorNoise* VectorNoise =
				Cast<UMaterialExpressionVectorNoise>(Expression))
			{
				++NonTiledDetailNoiseCount;
				TestFalse(TEXT("WorldScape gradient detail noise is explicitly non-tiled"),
					VectorNoise->bTiling);
			}
			else if (const UMaterialExpressionVectorParameter* Parameter =
				Cast<UMaterialExpressionVectorParameter>(Expression))
			{
				if (Parameter->ParameterName == TEXT("OrbitalSeedOffset"))
				{
					TerrainSeedOffset = Parameter;
				}
			}
		}
		TestTrue(TEXT("WorldScape terrain keeps macro, meso and near non-tiled detail bands"),
			NonTiledDetailNoiseCount >= 3);
		int32 SeededDomainCount = 0;
		if (TerrainSeedOffset)
		{
			for (const UMaterialExpression* Expression : WorldScapeTerrain->GetExpressions())
			{
				const UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(Expression);
				SeededDomainCount += Add && Add->B.Expression == TerrainSeedOffset ? 1 : 0;
			}
		}
		TestTrue(TEXT("Resolved terrain seed phases every physical detail domain"),
			SeededDomainCount >= 3);

		// WorldScape owns separate normal buffers for its Main/PatchA/PatchB sections.
		// The physical master must preserve those normals near the camera, then use one
		// normalized root-radial basis before the first clipmap boundary reaches view.
		const FExpressionInput* PhysicalNormalInput =
			WorldScapeTerrain->GetExpressionInputForProperty(MP_Normal);
		const UMaterialExpressionNormalize* FinalWorldNormal = PhysicalNormalInput
			? Cast<UMaterialExpressionNormalize>(PhysicalNormalInput->Expression) : nullptr;
		const UMaterialExpressionAdd* PerturbedWorldNormal = FinalWorldNormal
			? Cast<UMaterialExpressionAdd>(FinalWorldNormal->VectorInput.Expression) : nullptr;
		const UMaterialExpressionNormalize* BlendedPhysicalBaseNormal = PerturbedWorldNormal
			? Cast<UMaterialExpressionNormalize>(PerturbedWorldNormal->A.Expression) : nullptr;
		const UMaterialExpressionLinearInterpolate* SectionNormalBlend =
			BlendedPhysicalBaseNormal
				? Cast<UMaterialExpressionLinearInterpolate>(
					BlendedPhysicalBaseNormal->VectorInput.Expression)
				: nullptr;
		const UMaterialExpressionClamp* BoundedSectionNormalBlend = SectionNormalBlend
			? Cast<UMaterialExpressionClamp>(SectionNormalBlend->Alpha.Expression) : nullptr;
		const UMaterialExpressionAdd* CombinedSectionNormalBlend = BoundedSectionNormalBlend
			? Cast<UMaterialExpressionAdd>(BoundedSectionNormalBlend->Input.Expression) : nullptr;
		const UMaterialExpressionMultiply* WeightedSectionNormalFade =
			CombinedSectionNormalBlend
				? Cast<UMaterialExpressionMultiply>(CombinedSectionNormalBlend->B.Expression)
				: nullptr;
		const UMaterialExpressionSmoothStep* SectionNormalFade = WeightedSectionNormalFade
			? Cast<UMaterialExpressionSmoothStep>(WeightedSectionNormalFade->A.Expression)
			: nullptr;
		const UMaterialExpressionDistance* SectionCameraDistance = SectionNormalFade
			? Cast<UMaterialExpressionDistance>(SectionNormalFade->Value.Expression) : nullptr;
		const UMaterialExpressionScalarParameter* FadeStart = SectionNormalFade
			? Cast<UMaterialExpressionScalarParameter>(SectionNormalFade->Min.Expression) : nullptr;
		const UMaterialExpressionScalarParameter* FadeEnd = SectionNormalFade
			? Cast<UMaterialExpressionScalarParameter>(SectionNormalFade->Max.Expression) : nullptr;
		const UMaterialExpressionScalarParameter* FarBlend = WeightedSectionNormalFade
			? Cast<UMaterialExpressionScalarParameter>(WeightedSectionNormalFade->B.Expression)
			: nullptr;
		const UMaterialExpressionScalarParameter* OrbitalBlend = CombinedSectionNormalBlend
			? Cast<UMaterialExpressionScalarParameter>(CombinedSectionNormalBlend->A.Expression)
			: nullptr;
		const UMaterialExpressionNormalize* SectionRadialNormal = SectionNormalBlend
			? Cast<UMaterialExpressionNormalize>(SectionNormalBlend->B.Expression) : nullptr;
		const UMaterialExpressionSubtract* RootRelativeWorldPosition = SectionRadialNormal
			? Cast<UMaterialExpressionSubtract>(SectionRadialNormal->VectorInput.Expression)
			: nullptr;
		const UMaterialExpressionWorldPosition* AbsoluteSectionWorldPosition =
			RootRelativeWorldPosition
				? Cast<UMaterialExpressionWorldPosition>(
					RootRelativeWorldPosition->A.Expression)
				: nullptr;
		if (TestNotNull(TEXT("WorldScape normal output terminates in a normalize"),
			FinalWorldNormal)
			&& TestNotNull(TEXT("WorldScape detail perturbs one normalized base normal"),
				PerturbedWorldNormal)
			&& TestNotNull(TEXT("WorldScape section-normal blend is normalized before use"),
				BlendedPhysicalBaseNormal)
			&& TestNotNull(TEXT("WorldScape base normal blends streamed and radial normals"),
				SectionNormalBlend)
			&& TestNotNull(TEXT("WorldScape section-normal blend is clamped"),
				BoundedSectionNormalBlend)
			&& TestNotNull(TEXT("WorldScape section-normal fade combines presentation and distance"),
				CombinedSectionNormalBlend)
			&& TestNotNull(TEXT("WorldScape section-normal distance fade is weighted"),
				WeightedSectionNormalFade)
			&& TestNotNull(TEXT("WorldScape section-normal distance transition is smooth"),
				SectionNormalFade)
			&& TestNotNull(TEXT("WorldScape section-normal fade uses camera distance"),
				SectionCameraDistance)
			&& TestNotNull(TEXT("WorldScape section-normal fade exposes its start"), FadeStart)
			&& TestNotNull(TEXT("WorldScape section-normal fade exposes its end"), FadeEnd)
			&& TestNotNull(TEXT("WorldScape section-normal fade exposes its far blend"), FarBlend)
			&& TestNotNull(TEXT("WorldScape section-normal fade retains presentation blend"),
				OrbitalBlend)
			&& TestNotNull(TEXT("WorldScape seam target is one root-radial normal"),
				SectionRadialNormal)
			&& TestNotNull(TEXT("WorldScape radial normal is root-relative"),
				RootRelativeWorldPosition)
			&& TestNotNull(TEXT("WorldScape seam blend uses absolute world position"),
				AbsoluteSectionWorldPosition))
		{
			TestTrue(TEXT("WorldScape normal blend starts from the displaced mesh normal"),
				SectionNormalBlend->A.Expression
					&& SectionNormalBlend->A.Expression->IsA<UMaterialExpressionVertexNormalWS>());
			TestTrue(TEXT("WorldScape normal blend targets the root-radial normal"),
				SectionNormalBlend->B.Expression
					&& SectionNormalBlend->B.Expression->IsA<UMaterialExpressionNormalize>());
			TestTrue(TEXT("WorldScape camera-distance input starts at absolute world position"),
				SectionCameraDistance->A.Expression == AbsoluteSectionWorldPosition
					&& AbsoluteSectionWorldPosition->WorldPositionShaderOffset
						== WPT_ExcludeAllShaderOffsets);
			TestTrue(TEXT("WorldScape camera-distance input ends at CameraPositionWS"),
				SectionCameraDistance->B.Expression
					&& SectionCameraDistance->B.Expression->IsA<UMaterialExpressionCameraPositionWS>());
			TestTrue(TEXT("WorldScape radial target shares the same absolute world position"),
				RootRelativeWorldPosition->A.Expression == AbsoluteSectionWorldPosition);
			TestTrue(TEXT("WorldScape radial target uses the root actor centre, never a patch centre"),
				RootRelativeWorldPosition->B.Expression
					&& RootRelativeWorldPosition->B.Expression->GetClass()->GetFName()
						== FName(TEXT("MaterialExpressionActorPositionWS")));
			TestEqual(TEXT("WorldScape section-normal fade start parameter name"),
				FadeStart->ParameterName, FName(TEXT("LodSeamNormalFadeStartCm")));
			TestEqual(TEXT("WorldScape section-normal fade end parameter name"),
				FadeEnd->ParameterName, FName(TEXT("LodSeamNormalFadeEndCm")));
			TestEqual(TEXT("WorldScape section-normal far blend parameter name"),
				FarBlend->ParameterName, FName(TEXT("LodSeamNormalFarBlend")));
			TestEqual(TEXT("WorldScape section-normal presentation parameter name"),
				OrbitalBlend->ParameterName, FName(TEXT("OrbitalNormalBlend")));
			TestTrue(TEXT("WorldScape section-normal fade starts at 24 m"),
				FMath::IsNearlyEqual(FadeStart->DefaultValue, 2400.0f));
			TestTrue(TEXT("WorldScape section-normal fade completes at 50 m"),
				FMath::IsNearlyEqual(FadeEnd->DefaultValue, 5000.0f));
			TestTrue(TEXT("WorldScape section-normal far field retains twelve percent relief"),
				FMath::IsNearlyEqual(FarBlend->DefaultValue, 0.88f));
			TestTrue(TEXT("WorldScape section-normal blend is explicitly saturated"),
				BoundedSectionNormalBlend->ClampMode == CMODE_Clamp
					&& FMath::IsNearlyZero(BoundedSectionNormalBlend->MinDefault)
					&& FMath::IsNearlyEqual(BoundedSectionNormalBlend->MaxDefault, 1.0f));
			TestFalse(TEXT("WorldScape world normal never depends on streamed tangents"),
				WorldScapeTerrain->bTangentSpaceNormal);
			TestFalse(TEXT("WorldScape seam mitigation never displaces geometry"),
				APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
					WorldScapeTerrain, MP_WorldPositionOffset));
		}

		const FExpressionInput* BaseColorInput =
			WorldScapeTerrain->GetExpressionInputForProperty(MP_BaseColor);
		const UMaterialExpressionLinearInterpolate* GeologicalSlopeBlend =
			BaseColorInput
				? Cast<UMaterialExpressionLinearInterpolate>(BaseColorInput->Expression)
				: nullptr;
		if (TestNotNull(TEXT("WorldScape Base Color terminates in a geological slope blend"),
			GeologicalSlopeBlend))
		{
			TestNotNull(TEXT("Slope blend retains the resolved height/detail palette"),
				GeologicalSlopeBlend->A.Expression);
			TestNotNull(TEXT("Slope blend retains the authored geological tint"),
				GeologicalSlopeBlend->B.Expression);
			const UMaterialExpressionMultiply* WeightedSlope =
				Cast<UMaterialExpressionMultiply>(GeologicalSlopeBlend->Alpha.Expression);
			if (TestNotNull(TEXT("Slope blend alpha is bounded by authored strength"), WeightedSlope))
			{
				const UMaterialExpressionSmoothStep* SlopeMask =
					Cast<UMaterialExpressionSmoothStep>(WeightedSlope->A.Expression);
				if (TestNotNull(TEXT("Slope blend consumes a smooth physical steepness mask"),
					SlopeMask))
				{
					TestTrue(TEXT("Slope cue starts on readable walkable relief"),
						SlopeMask->ConstMin <= 0.01f && SlopeMask->ConstMax <= 0.16f);
					const UMaterialExpressionOneMinus* PhysicalSteepness =
						Cast<UMaterialExpressionOneMinus>(SlopeMask->Value.Expression);
					const UMaterialExpressionDotProduct* PhysicalSlopeAlignment =
						PhysicalSteepness
							? Cast<UMaterialExpressionDotProduct>(
								PhysicalSteepness->Input.Expression)
							: nullptr;
					if (TestNotNull(TEXT("Slope cue measures one physical normal alignment"),
						PhysicalSlopeAlignment)
						&& TestNotNull(TEXT("Slope cue can reuse the blended base normal"),
							BlendedPhysicalBaseNormal))
					{
						TestTrue(TEXT("Lighting detail and slope tint share the normalized seam-safe normal"),
							PhysicalSlopeAlignment->A.Expression == BlendedPhysicalBaseNormal
								&& PerturbedWorldNormal
								&& PerturbedWorldNormal->A.Expression
									== BlendedPhysicalBaseNormal);
						TestTrue(TEXT("Slope cue compares against the same root-radial target"),
							SectionNormalBlend
								&& PhysicalSlopeAlignment->B.Expression
									== SectionNormalBlend->B.Expression);
					}
				}
			}
		}
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
		for (const TPair<const TCHAR*, float>& SeamParameter :
			{TPair<const TCHAR*, float>(TEXT("LodSeamNormalFadeStartCm"), 2400.0f),
				TPair<const TCHAR*, float>(TEXT("LodSeamNormalFadeEndCm"), 5000.0f),
				TPair<const TCHAR*, float>(TEXT("LodSeamNormalFarBlend"), 0.88f)})
		{
			float ResolvedValue = 0.0f;
			const bool bResolvedValue = APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, SeamParameter.Key, ResolvedValue);
			TestTrue(*FString::Printf(TEXT("%s resolves %s"), *Context,
				SeamParameter.Key), bResolvedValue);
			if (bResolvedValue)
			{
				TestTrue(*FString::Printf(TEXT("%s pins %s to the full-scale seam contract"),
					*Context, SeamParameter.Key), FMath::IsNearlyEqual(
						ResolvedValue, SeamParameter.Value, 1.0e-4f));
			}
		}
		float SlopeTintStrength = 0.0f;
		if (TestTrue(*(Context + TEXT(" authors geological slope readability")),
			APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, TEXT("SlopeTintStrength"), SlopeTintStrength)))
		{
			TestTrue(*(Context + TEXT(" keeps slope tint bounded")),
				SlopeTintStrength >= 0.12f && SlopeTintStrength <= 0.28f);
		}
		float NearScale = 0.0f;
		for (const TCHAR* ScaleParameter :
			{TEXT("MacroDetailScaleCm"), TEXT("MesoDetailScaleCm"), TEXT("NearDetailScaleCm")})
		{
			float ScaleValue = 0.0f;
			const bool bResolvedScale = APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, ScaleParameter, ScaleValue)
				&& FMath::IsFinite(ScaleValue) && ScaleValue > 0.0f;
			TestTrue(*FString::Printf(TEXT("%s resolves %s"), *Context, ScaleParameter),
				bResolvedScale);
			if (bResolvedScale && FCString::Strcmp(ScaleParameter, TEXT("NearDetailScaleCm")) == 0)
			{
				NearScale = ScaleValue;
			}
		}
		TestTrue(*(Context + TEXT(" resolves a non-periodic ground-detail scale")),
			NearScale >= 1200.0f && NearScale <= 3000.0f);
		float MacroColorStrength = 0.0f;
		float MesoColorStrength = 0.0f;
		float NearColorStrength = 0.0f;
		float NearRoughnessStrength = 0.0f;
		const bool bResolvedHierarchy =
				APSPlanetSurfaceProfileTests::GetScalarParameter(
					FamilyMaterial, TEXT("MacroColorStrength"), MacroColorStrength)
			&& APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, TEXT("MesoColorStrength"), MesoColorStrength)
			&& APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, TEXT("NearColorStrength"), NearColorStrength)
			&& APSPlanetSurfaceProfileTests::GetScalarParameter(
				FamilyMaterial, TEXT("DetailRoughnessStrength"), NearRoughnessStrength);
		if (TestTrue(*(Context + TEXT(" resolves scale-separated colour hierarchy")),
			bResolvedHierarchy))
		{
			TestTrue(*(Context + TEXT(" gives large form more contrast than micro detail")),
				MacroColorStrength >= 0.040f
				&& MesoColorStrength >= 0.028f
				&& NearColorStrength <= 0.025f
				&& MacroColorStrength >= NearColorStrength * 1.8f);
			TestTrue(*(Context + TEXT(" keeps near roughness from becoming grey speckle")),
				NearRoughnessStrength <= 0.03f);
		}
	}
	if (TestNotNull(TEXT("Project WorldScape physical water master"), WorldScapeWaterMaster))
	{
		TestEqual(TEXT("Physical water writes the opaque depth pass"),
			WorldScapeWaterMaster->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("Physical water uses its colour-stable optical Unlit pass"),
			WorldScapeWaterMaster->GetShadingModels().HasShadingModel(
				MSM_Unlit));
		TestFalse(TEXT("Physical water is not whitened by per-section DefaultLit normals"),
			WorldScapeWaterMaster->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("Physical water avoids the optically absent SingleLayerWater pass"),
			WorldScapeWaterMaster->GetShadingModels().HasShadingModel(
				MSM_SingleLayerWater));
		TestFalse(TEXT("Physical water owns no unused SingleLayerWater custom output"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<
						UMaterialExpressionSingleLayerWaterMaterialOutput>();
				}));
		TestFalse(TEXT("Physical water does not expose section-local vertex normals"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionVertexNormalWS>();
				}));
		TestFalse(TEXT("Physical water avoids absolute-position base-pass dependencies"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionWorldPosition>();
				}));
		TestFalse(TEXT("Physical water never aliases a streamed patch ActorPositionWS"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->GetClass()->GetFName()
						== FName(TEXT("MaterialExpressionActorPositionWS"));
				}));
		TestFalse(TEXT("Physical water never evaluates a section-local object centre"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->GetClass()->GetFName()
						== FName(TEXT("MaterialExpressionObjectPositionWS"));
				}));
		TestFalse(TEXT("Physical water needs no runtime root-centre parameter"),
			APSPlanetSurfaceProfileTests::HasVectorParameter(
				WorldScapeWaterMaster, TEXT("PlanetLocation")));
		TestFalse(TEXT("Physical water has no UV texture grid fallback"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionTextureSample>();
				}));
		TestFalse(TEXT("Physical water has no procedural relief graph"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && (Expression->IsA<UMaterialExpressionNoise>()
						|| Expression->IsA<UMaterialExpressionVectorNoise>());
				}));
		for (const TCHAR* ParameterName :
			{TEXT("WaterDeepColor"), TEXT("WaterShallowColor"),
				TEXT("WaterRadianceFloor")})
		{
			TestTrue(*FString::Printf(TEXT("Physical water exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasVectorParameter(
					WorldScapeWaterMaster, ParameterName));
		}
		TestFalse(TEXT("Optical Unlit water owns no inert PBR scalar controls"),
			APSPlanetSurfaceProfileTests::HasScalarParameter(
				WorldScapeWaterMaster, TEXT("Roughness"))
			|| APSPlanetSurfaceProfileTests::HasScalarParameter(
				WorldScapeWaterMaster, TEXT("Metallic"))
			|| APSPlanetSurfaceProfileTests::HasScalarParameter(
				WorldScapeWaterMaster, TEXT("Specular")));
		TestFalse(TEXT("Physical water never displaces ocean geometry"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				WorldScapeWaterMaster, MP_WorldPositionOffset));
		TestFalse(TEXT("Physical water avoids the unsupported normal-dependent Fresnel permutation"),
			WorldScapeWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && Expression->IsA<UMaterialExpressionFresnel>();
				}));
		const FExpressionInput* EmissiveInput =
			WorldScapeWaterMaster->GetExpressionInputForProperty(MP_EmissiveColor);
		const UMaterialExpressionClamp* EmissiveClamp = EmissiveInput
			? Cast<UMaterialExpressionClamp>(EmissiveInput->Expression) : nullptr;
		const UMaterialExpressionAdd* WaterRadiance = EmissiveClamp
			? Cast<UMaterialExpressionAdd>(EmissiveClamp->Input.Expression) : nullptr;
		const UMaterialExpressionVectorParameter* RadianceFloor = WaterRadiance
			? Cast<UMaterialExpressionVectorParameter>(WaterRadiance->B.Expression) : nullptr;
		const UMaterialExpressionLinearInterpolate* MarineBlend = WaterRadiance
			? Cast<UMaterialExpressionLinearInterpolate>(WaterRadiance->A.Expression) : nullptr;
		TestTrue(TEXT("Physical water keeps a stable optical-depth blend plus bounded ambient radiance"),
			MarineBlend && RadianceFloor
				&& MarineBlend->A.Expression
				&& MarineBlend->A.Expression->IsA<UMaterialExpressionVectorParameter>()
				&& MarineBlend->B.Expression
				&& MarineBlend->B.Expression->IsA<UMaterialExpressionVectorParameter>()
				&& MarineBlend->Alpha.Expression == nullptr
				&& FMath::IsNearlyEqual(MarineBlend->ConstAlpha, 0.46f)
				&& RadianceFloor->ParameterName == TEXT("WaterRadianceFloor")
				&& FMath::IsNearlyEqual(EmissiveClamp->MinDefault, 0.0f)
				&& EmissiveClamp->MaxDefault <= 0.20f);
	}
	if (TestNotNull(TEXT("Project opaque ammonia liquid master"), WorldScapeLiquidMaster))
	{
		TestEqual(TEXT("Ammonia master retains the opaque depth pass"),
			WorldScapeLiquidMaster->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("Ammonia master retains stable DefaultLit shading"),
			WorldScapeLiquidMaster->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("Ammonia master is not reinterpreted as water"),
			WorldScapeLiquidMaster->GetShadingModels().HasShadingModel(
				MSM_SingleLayerWater));
	}
	if (TestNotNull(TEXT("Marketplace planetary water template"), MarketplaceWaterTemplate)
		&& TestNotNull(TEXT("Marketplace planetary water master"), MarketplaceWaterMaster))
	{
		TestEqual(TEXT("Marketplace template retains its marketplace master"),
			MarketplaceWaterTemplate->GetMaterial(), MarketplaceWaterMaster);
		TestNotEqual(TEXT("Project physical water no longer aliases the marketplace master"),
			static_cast<UObject*>(WorldScapeWaterMaster),
			static_cast<UObject*>(MarketplaceWaterMaster));
	}
	if (TestNotNull(TEXT("WorldScape original Water-plugin template"), OriginalWaterTemplate)
		&& TestNotNull(TEXT("WorldScape original Water-plugin master"), OriginalWaterMaster))
	{
		TestEqual(TEXT("Original Water template retains its distinct Water-plugin parent"),
			OriginalWaterTemplate->Parent.Get(),
			static_cast<UMaterialInterface*>(OriginalWaterMaster));
		TestEqual(TEXT("Original Water graph remains masked and unsuitable for raw WorldScape sections"),
			OriginalWaterMaster->GetBlendMode(), BLEND_Masked);
		TestNotEqual(TEXT("Project WorldScape water never aliases the Water-plugin template"),
			static_cast<UObject*>(WorldScapeWaterMaster),
			static_cast<UObject*>(OriginalWaterTemplate));
		TestNotEqual(TEXT("Project WorldScape water never aliases the masked Water-plugin master"),
			static_cast<UObject*>(WorldScapeWaterMaster),
			static_cast<UObject*>(OriginalWaterMaster));
	}
	if (TestNotNull(TEXT("WorldScape lava template"), WorldScapeLavaTemplate)
		&& TestNotNull(TEXT("WorldScape lava master"), WorldScapeLavaMaster))
	{
		TestEqual(TEXT("WorldScape lava template directly owns its procedural lava master"),
			WorldScapeLavaTemplate->Parent.Get(),
			static_cast<UMaterialInterface*>(WorldScapeLavaMaster));
		TestEqual(TEXT("WorldScape lava template resolves its procedural lava master"),
			WorldScapeLavaTemplate->GetMaterial(), WorldScapeLavaMaster);
		TestEqual(TEXT("WorldScape lava template remains opaque"),
			WorldScapeLavaTemplate->GetBlendMode(), BLEND_Opaque);
		TestEqual(TEXT("WorldScape lava writes the opaque depth pass"),
			WorldScapeLavaMaster->GetBlendMode(), BLEND_Opaque);
		TestTrue(TEXT("WorldScape lava keeps its dedicated lit emissive model"),
			WorldScapeLavaMaster->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("WorldScape lava is not reinterpreted as water"),
			WorldScapeLavaMaster->GetShadingModels().HasShadingModel(
				MSM_SingleLayerWater));
		TestTrue(TEXT("WorldScape lava exposes its authored emissive colour"),
			APSPlanetSurfaceProfileTests::HasVectorParameter(
				WorldScapeLavaMaster, TEXT("EmissiveColor")));
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
		TestTrue(TEXT("Orbital liquid retains its object-centred closed-globe path"),
			APSPlanetSurfaceProfileTests::HasScalarParameter(
				OrbitalLiquid, TEXT("OrbitalNormalBlend"))
				&& OrbitalLiquid->GetExpressions().ContainsByPredicate(
					[](const UMaterialExpression* Expression)
					{
						return Expression && Expression->GetClass()->GetFName()
							== FName(TEXT("MaterialExpressionObjectPositionWS"));
					}));
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
			{TEXT("Opacity"), TEXT("Roughness"), TEXT("Metallic"), TEXT("Specular"),
				TEXT("WaveScaleCm"), TEXT("WaveColorStrength"),
				TEXT("WaveNormalStrength")})
		{
			TestTrue(*FString::Printf(TEXT("Orbital liquid exposes %s"), ParameterName),
				APSPlanetSurfaceProfileTests::HasScalarParameter(OrbitalLiquid, ParameterName));
		}
	}
	if (TestNotNull(TEXT("Dedicated orbital living-water material"), OrbitalWaterMaster))
	{
		TestEqual(TEXT("Living water writes a masked depth pass"),
			OrbitalWaterMaster->GetBlendMode(), BLEND_Masked);
		TestTrue(TEXT("Living water uses lit dielectric shading"),
			OrbitalWaterMaster->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestTrue(TEXT("Living water connects its coastline mask"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalWaterMaster, MP_OpacityMask));
		TestTrue(TEXT("Living water connects a smooth radial normal"),
			APSPlanetSurfaceProfileTests::HasConnectedMaterialProperty(
				OrbitalWaterMaster, MP_Normal));
		TestFalse(TEXT("Living water has no procedural relief noise"),
			OrbitalWaterMaster->GetExpressions().ContainsByPredicate(
				[](const UMaterialExpression* Expression)
				{
					return Expression && (Expression->IsA<UMaterialExpressionNoise>()
						|| Expression->IsA<UMaterialExpressionVectorNoise>());
				}));
	}
	if (OrbitalWater && OrbitalWaterMaster)
	{
		TestEqual(TEXT("Orbital water alone uses the dedicated living-water parent"),
			OrbitalWater->Parent.Get(), static_cast<UMaterialInterface*>(OrbitalWaterMaster));
		TestEqual(TEXT("Orbital water authors three marine colours"),
			OrbitalWater->VectorParameterValues.Num(), 3);
		TestEqual(TEXT("Orbital water authors dielectric PBR controls"),
			OrbitalWater->ScalarParameterValues.Num(), 3);
	}
	for (UMaterialInstance* Preset : {OrbitalAmmonia, OrbitalLava})
	{
		if (TestNotNull(TEXT("Orbital liquid preset"), Preset) && OrbitalLiquid)
		{
			TestEqual(TEXT("Orbital liquid preset shares the canonical liquid parent"),
				Preset->Parent.Get(), static_cast<UMaterialInterface*>(OrbitalLiquid));
			TestEqual(TEXT("Orbital liquid preset authors its three colours"),
				Preset->VectorParameterValues.Num(), 3);
			TestEqual(TEXT("Orbital liquid preset authors optical and wave controls"),
				Preset->ScalarParameterValues.Num(), 7);
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
	auto TestOrbitalScalar = [this](const TCHAR* Label,
		const UMaterialInterface* Material, const TCHAR* ParameterName,
		const float ExpectedValue)
	{
		float ActualValue = 0.0f;
		if (TestTrue(Label, APSPlanetSurfaceProfileTests::GetScalarParameter(
			Material, ParameterName, ActualValue)))
		{
			TestTrue(*FString::Printf(TEXT("%s has the authored value"), Label),
				FMath::IsNearlyEqual(ActualValue, ExpectedValue, 1.0e-4f));
		}
	};
	if (OrbitalWater && OrbitalAmmonia && OrbitalLava)
	{
		TestOrbitalScalar(TEXT("Orbital water low reflection roughness"), OrbitalWater,
			TEXT("Roughness"), 0.10f);
		TestOrbitalScalar(TEXT("Orbital water dielectric specular response"), OrbitalWater,
			TEXT("Specular"), 0.25f);
		TestOrbitalScalar(TEXT("Orbital ammonia broad wave scale"), OrbitalAmmonia,
			TEXT("WaveScaleCm"), 62000.0f);
		TestOrbitalScalar(TEXT("Orbital ammonia restrained colour variation"), OrbitalAmmonia,
			TEXT("WaveColorStrength"), 0.0040f);
		TestOrbitalScalar(TEXT("Orbital ammonia smooth normal response"), OrbitalAmmonia,
			TEXT("WaveNormalStrength"), 0.014f);
		TestOrbitalScalar(TEXT("Orbital lava retains a tighter flow scale"), OrbitalLava,
			TEXT("WaveScaleCm"), 16000.0f);
		TestOrbitalScalar(TEXT("Orbital lava retains visible flow colour"), OrbitalLava,
			TEXT("WaveColorStrength"), 0.018f);
		TestOrbitalScalar(TEXT("Orbital lava remains rougher than water"), OrbitalLava,
			TEXT("WaveNormalStrength"), 0.050f);
	}
	if (WaterMaterial && AmmoniaMaterial && LavaMaterial
		&& WorldScapeWaterMaster && WorldScapeLiquidMaster
		&& WorldScapeLavaTemplate && WorldScapeLavaMaster
		&& OriginalWaterTemplate && OriginalWaterMaster && OrbitalLiquid)
	{
		TestNotEqual(TEXT("WorldScape water and orbital liquid use separate render passes"),
			static_cast<UMaterialInterface*>(WorldScapeWaterMaster),
			static_cast<UMaterialInterface*>(OrbitalLiquid));
		TestNotEqual(TEXT("WorldScape lava and orbital liquid use separate render passes"),
			static_cast<UMaterialInterface*>(WorldScapeLavaMaster),
			static_cast<UMaterialInterface*>(OrbitalLiquid));
		TestEqual(TEXT("Water directly wraps the project physical water master"),
			WaterMaterial->Parent.Get(),
			static_cast<UMaterialInterface*>(WorldScapeWaterMaster));
		TestEqual(TEXT("Ammonia directly wraps the project opaque liquid master"),
			AmmoniaMaterial->Parent.Get(),
			static_cast<UMaterialInterface*>(WorldScapeLiquidMaster));
		TestEqual(TEXT("Lava directly wraps the WorldScape lava template"),
			LavaMaterial->Parent.Get(),
			static_cast<UMaterialInterface*>(WorldScapeLavaTemplate));
		TestEqual(TEXT("Water resolves the project water-volume master"),
			WaterMaterial->GetMaterial(), WorldScapeWaterMaster);
		TestEqual(TEXT("Ammonia resolves the project opaque liquid master"),
			AmmoniaMaterial->GetMaterial(), WorldScapeLiquidMaster);
		TestEqual(TEXT("Lava resolves the WorldScape procedural lava master"),
			LavaMaterial->GetMaterial(), WorldScapeLavaMaster);
		for (UMaterialInstance* Liquid : {WaterMaterial, AmmoniaMaterial, LavaMaterial})
		{
			const FString LiquidName = Liquid->GetName();
			TestNotEqual(*FString::Printf(TEXT("%s never wraps MI_Original_Water"), *LiquidName),
				Liquid->Parent.Get(), static_cast<UMaterialInterface*>(OriginalWaterTemplate));
			TestNotEqual(*FString::Printf(TEXT("%s never resolves the masked Water-plugin graph"),
				*LiquidName), Liquid->GetMaterial(), OriginalWaterMaster);
			TestNotEqual(*FString::Printf(TEXT("%s never uses the orbital preview master"),
				*LiquidName), static_cast<UMaterialInterface*>(Liquid->GetMaterial()),
				static_cast<UMaterialInterface*>(OrbitalLiquid));
			TestEqual(*FString::Printf(TEXT("%s writes the opaque physical depth pass"),
				*LiquidName), Liquid->GetBlendMode(), BLEND_Opaque);
		}
		TestTrue(TEXT("Water retains its colour-stable optical Unlit shading"),
			WaterMaterial->GetShadingModels().HasShadingModel(MSM_Unlit));
		TestFalse(TEXT("Water avoids per-section DefaultLit normal discontinuities"),
			WaterMaterial->GetShadingModels().HasShadingModel(MSM_DefaultLit));
		TestFalse(TEXT("Water avoids the optically absent SingleLayerWater pass"),
			WaterMaterial->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
		TestTrue(TEXT("Ammonia retains WorldScape opaque lit shading"),
			AmmoniaMaterial->GetShadingModels().HasShadingModel(MSM_DefaultLit));

		TestEqual(TEXT("Water authors three marine colour vectors"),
			WaterMaterial->VectorParameterValues.Num(), 3);
		TestEqual(TEXT("Water owns no inert PBR scalar overrides"),
			WaterMaterial->ScalarParameterValues.Num(), 0);
		TestEqual(TEXT("Ammonia authors the same seven-vector chemistry contract"),
			AmmoniaMaterial->VectorParameterValues.Num(), 7);
		TestEqual(TEXT("Ammonia authors the same eleven-scalar wave contract"),
			AmmoniaMaterial->ScalarParameterValues.Num(), 11);
		TestEqual(TEXT("Lava authors only its real emissive vector"),
			LavaMaterial->VectorParameterValues.Num(), 1);
		TestEqual(TEXT("Lava owns no local scalar overrides"),
			LavaMaterial->ScalarParameterValues.Num(), 0);

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
			const TCHAR* ParameterName, const float Expected)
		{
			float Actual = 0.0f;
			if (TestTrue(Label, APSPlanetSurfaceProfileTests::GetScalarParameter(
				Material, ParameterName, Actual)))
			{
				TestTrue(*FString::Printf(TEXT("%s has the authored value"), Label),
					FMath::IsNearlyEqual(Actual, Expected, 1.0e-4f));
			}
		};
		for (const TCHAR* ParameterName :
			{TEXT("WaterDeepColor"), TEXT("WaterShallowColor"),
				TEXT("WaterRadianceFloor")})
		{
			TestTrue(*FString::Printf(TEXT("Water owns direct project vector %s"),
				ParameterName), APSPlanetSurfaceProfileTests::HasOwnVectorOverride(
					WaterMaterial, ParameterName));
		}
		for (UMaterialInstance* Liquid : {WaterMaterial, AmmoniaMaterial})
		{
			const FString LiquidName = Liquid->GetName();
			for (const TCHAR* ParameterName :
				{TEXT("Param_1"), TEXT("Param_2"), TEXT("WaveDir"),
					TEXT("WaveShape1"), TEXT("WaveShape2")})
			{
				TestFalse(*FString::Printf(TEXT("%s owns no opaque marketplace vector %s"),
					*LiquidName, ParameterName),
					APSPlanetSurfaceProfileTests::HasOwnVectorOverride(Liquid, ParameterName));
			}
			for (const TCHAR* ParameterName : {TEXT("OrbitalNormalBlend"), TEXT("Opacity")})
			{
				TestFalse(*FString::Printf(TEXT("%s owns no orbital scalar %s"),
					*LiquidName, ParameterName),
					APSPlanetSurfaceProfileTests::HasOwnScalarOverride(Liquid, ParameterName));
			}
		}
		for (const TCHAR* ParameterName :
			{TEXT("LiquidDeepColor"), TEXT("LiquidShallowColor"),
				TEXT("LiquidEmissiveColor"), TEXT("WaterScatteringCoefficients"),
				TEXT("WaterAbsorptionCoefficients"), TEXT("WaterColorScaleBehind"),
				TEXT("WaterBodyTintColor")})
		{
			TestTrue(*FString::Printf(TEXT("Ammonia owns project vector %s"),
				ParameterName), APSPlanetSurfaceProfileTests::HasOwnVectorOverride(
					AmmoniaMaterial, ParameterName));
		}
		for (const TCHAR* ParameterName :
			{TEXT("WaterPhaseG"), TEXT("WaterSurfaceOpacity"),
				TEXT("WaterBodyTintStrength"), TEXT("Roughness"), TEXT("Metallic"),
				TEXT("Specular"), TEXT("WaveScaleCm"),
				TEXT("PhysicalWaveDetailScaleCm"), TEXT("WaveColorStrength"),
				TEXT("WaveNormalStrength"), TEXT("PhysicalWaveRoughnessStrength")})
		{
			TestTrue(*FString::Printf(TEXT("Ammonia owns project scalar %s"),
				ParameterName), APSPlanetSurfaceProfileTests::HasOwnScalarOverride(
					AmmoniaMaterial, ParameterName));
		}
		TestVectorValue(TEXT("Water deep marine colour"), WaterMaterial,
			TEXT("WaterDeepColor"),
			FLinearColor(0.0040f, 0.0550f, 0.2400f, 1.0f));
		TestVectorValue(TEXT("Water shallow grazing colour"), WaterMaterial,
			TEXT("WaterShallowColor"),
			FLinearColor(0.0180f, 0.2200f, 0.5200f, 1.0f));
		TestVectorValue(TEXT("Water bounded radiance floor"), WaterMaterial,
			TEXT("WaterRadianceFloor"),
			FLinearColor(0.0010f, 0.0250f, 0.1600f, 1.0f));
		TestScalarValue(TEXT("Ammonia bounded body tint"), AmmoniaMaterial,
			TEXT("WaterBodyTintStrength"), 0.20f);
		TestVectorValue(TEXT("Ammonia explicit full-scale body tint"), AmmoniaMaterial,
			TEXT("WaterBodyTintColor"),
			FLinearColor(0.018f, 0.200f, 0.070f, 1.0f));
		TestVectorValue(TEXT("Ammonia low-radiance emissive"), AmmoniaMaterial,
			TEXT("LiquidEmissiveColor"),
			FLinearColor(0.0001f, 0.0005f, 0.0002f, 1.0f));
		TestScalarValue(TEXT("Ammonia primary wave scale"), AmmoniaMaterial,
			TEXT("WaveScaleCm"), 90000.0f);
		TestScalarValue(TEXT("Ammonia detail wave scale"), AmmoniaMaterial,
			TEXT("PhysicalWaveDetailScaleCm"), 20000.0f);
		TestScalarValue(TEXT("Ammonia smooth surface roughness"), AmmoniaMaterial,
			TEXT("Roughness"), 0.20f);
		TestScalarValue(TEXT("Ammonia dielectric highlight response"), AmmoniaMaterial,
			TEXT("Specular"), 0.60f);
		TestScalarValue(TEXT("Ammonia restrained wave colour"), AmmoniaMaterial,
			TEXT("WaveColorStrength"), 0.0040f);
		TestScalarValue(TEXT("Ammonia restrained wave normal"), AmmoniaMaterial,
			TEXT("WaveNormalStrength"), 0.015f);
		TestScalarValue(TEXT("Ammonia restrained wave roughness"), AmmoniaMaterial,
			TEXT("PhysicalWaveRoughnessStrength"), 0.010f);
		TestVectorValue(TEXT("Ammonia scattering coefficients"), AmmoniaMaterial,
			TEXT("WaterScatteringCoefficients"),
			FLinearColor(0.000240f, 0.005200f, 0.000520f, 1.0f));
		TestVectorValue(TEXT("Ammonia absorption coefficients"), AmmoniaMaterial,
			TEXT("WaterAbsorptionCoefficients"),
			FLinearColor(0.003000f, 0.000350f, 0.001800f, 1.0f));
		TestTrue(TEXT("Lava owns EmissiveColor"),
			APSPlanetSurfaceProfileTests::HasOwnVectorOverride(
				LavaMaterial, TEXT("EmissiveColor")));
		TestVectorValue(TEXT("Lava emissive colour"), LavaMaterial,
			TEXT("EmissiveColor"), FLinearColor(0.420f, 0.018f, 0.001f, 1.0f));

		TestFalse(TEXT("Lava owns no SingleLayerWater coefficients"),
			APSPlanetSurfaceProfileTests::HasOwnVectorOverride(
				LavaMaterial, TEXT("WaterScatteringCoefficients")));
	}

	TSet<EAPSPlanetLiquidType> CatalogLiquidTypes;
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
			CatalogLiquidTypes.Add(Entry.Value.LiquidType);
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
			UMaterialInterface* ExpectedWorldScapeParent =
				Entry.Value.LiquidType == EAPSPlanetLiquidType::Lava
					? static_cast<UMaterialInterface*>(WorldScapeLavaTemplate)
					: Entry.Value.LiquidType == EAPSPlanetLiquidType::Ammonia
						? static_cast<UMaterialInterface*>(WorldScapeLiquidMaster)
						: static_cast<UMaterialInterface*>(WorldScapeWaterMaster);
			UMaterial* ExpectedWorldScapeMaster =
				Entry.Value.LiquidType == EAPSPlanetLiquidType::Lava
					? WorldScapeLavaMaster
					: Entry.Value.LiquidType == EAPSPlanetLiquidType::Ammonia
						? WorldScapeLiquidMaster : WorldScapeWaterMaster;
			TestEqual(*(Context + TEXT(" directly wraps its physical WorldScape parent")),
				LiquidMaterial->Parent.Get(), ExpectedWorldScapeParent);
			TestEqual(*(Context + TEXT(" resolves its WorldScape procedural master")),
				LiquidMaterial->GetMaterial(), ExpectedWorldScapeMaster);
			TestNotEqual(*(Context + TEXT(" never selects MI_Original_Water")),
				LiquidMaterial->Parent.Get(),
				static_cast<UMaterialInterface*>(OriginalWaterTemplate));
			TestNotEqual(*(Context + TEXT(" never resolves the masked Water-plugin master")),
				LiquidMaterial->GetMaterial(), OriginalWaterMaster);
			TestEqual(*(Context + TEXT(" writes the opaque physical depth pass")),
				LiquidMaterial->GetBlendMode(), BLEND_Opaque);
			if (Entry.Value.LiquidType == EAPSPlanetLiquidType::Water)
			{
				TestTrue(*(Context + TEXT(" retains dedicated colour-stable water shading")),
					LiquidMaterial->GetShadingModels().HasShadingModel(
						MSM_Unlit));
			}
			else if (Entry.Value.LiquidType == EAPSPlanetLiquidType::Ammonia)
			{
				TestTrue(*(Context + TEXT(" retains stable opaque ammonia shading")),
					LiquidMaterial->GetShadingModels().HasShadingModel(
						MSM_DefaultLit));
			}
		}
	}
	TestTrue(TEXT("Catalog keeps at least one Water surface archetype"),
		CatalogLiquidTypes.Contains(EAPSPlanetLiquidType::Water));
	TestTrue(TEXT("Catalog keeps at least one Ammonia surface archetype"),
		CatalogLiquidTypes.Contains(EAPSPlanetLiquidType::Ammonia));
	TestTrue(TEXT("Catalog keeps at least one Lava surface archetype"),
		CatalogLiquidTypes.Contains(EAPSPlanetLiquidType::Lava));
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
	FAPSPlanetSurfaceSeedFieldUniquenessTest,
	"APS.Gameplay.World.PlanetSurface.SurfaceSeedFieldUniqueness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetSurfaceSeedFieldUniquenessTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Surface-seed world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	if (!TestNotNull(TEXT("Surface-seed planet"), Planet))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->Temperature = 288;
	Planet->PlanetAtmosphere.Humidity = 42.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;

	const EPlanetType RepresentativeTypes[] =
	{
		EPlanetType::Terrestrial, EPlanetType::Water, EPlanetType::Forest,
		EPlanetType::Frozen, EPlanetType::Lava
	};
	const int32 SurfaceSeeds[] = {11021, 28411, 61717, 90379};
	for (const EPlanetType Type : RepresentativeTypes)
	{
		Planet->PlanetType = Type;
		TSet<uint32> FieldHashes;
		TSet<int32> TerrainSeeds;
		TSet<int32> BiomeSeeds;
		TSet<int32> PaletteSeeds;
		for (const int32 SurfaceSeed : SurfaceSeeds)
		{
			Planet->WorldScapeSeed = SurfaceSeed;
			const FAPSResolvedPlanetSurfaceProfile Profile =
				UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet);
			const FSurfaceFieldStats First = SampleSurfaceFields(
				Profile, SurfaceSeed, 192);
			const FSurfaceFieldStats Repeat = SampleSurfaceFields(
				Profile, SurfaceSeed, 192);
			const FString Context = FString::Printf(TEXT("%s seed %d"),
				*StaticEnum<EPlanetType>()->GetNameStringByValue(
					static_cast<int64>(Type)), SurfaceSeed);
			TestEqual(*(Context + TEXT(" deterministically repeats sampled fields")),
				First.FieldHash, Repeat.FieldHash);
			TestNotEqual(*(Context + TEXT(" separates terrain and biome streams")),
				Profile.TerrainSeed, Profile.BiomeSeed);
			TestNotEqual(*(Context + TEXT(" separates terrain and palette streams")),
				Profile.TerrainSeed, Profile.PaletteSeed);
			TestNotEqual(*(Context + TEXT(" separates biome and palette streams")),
				Profile.BiomeSeed, Profile.PaletteSeed);
			FieldHashes.Add(First.FieldHash);
			TerrainSeeds.Add(Profile.TerrainSeed);
			BiomeSeeds.Add(Profile.BiomeSeed);
			PaletteSeeds.Add(Profile.PaletteSeed);
		}
		const FString TypeName = StaticEnum<EPlanetType>()->GetNameStringByValue(
			static_cast<int64>(Type));
		TestEqual(*(TypeName + TEXT(" Surface Seed uniquely phases sampled terrain/climate")),
			FieldHashes.Num(), static_cast<int32>(UE_ARRAY_COUNT(SurfaceSeeds)));
		TestEqual(*(TypeName + TEXT(" Surface Seed uniquely derives terrain noise")),
			TerrainSeeds.Num(), static_cast<int32>(UE_ARRAY_COUNT(SurfaceSeeds)));
		TestEqual(*(TypeName + TEXT(" Surface Seed uniquely derives biome noise")),
			BiomeSeeds.Num(), static_cast<int32>(UE_ARRAY_COUNT(SurfaceSeeds)));
		TestEqual(*(TypeName + TEXT(" Surface Seed uniquely derives palette phase")),
			PaletteSeeds.Num(), static_cast<int32>(UE_ARRAY_COUNT(SurfaceSeeds)));
	}

	Planet->Destroy();
	DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSPlanetAtmosphereDeterministicVariationTest,
	"APS.Gameplay.World.PlanetSurface.AtmosphereDeterministicVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPlanetAtmosphereDeterministicVariationTest::RunTest(const FString& Parameters)
{
	using namespace APSPlanetSurfaceProfileTests;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Atmosphere variation world"), World)) return false;

	APlanet* Planet = World->SpawnActor<APlanet>();
	APlanetarySurfaceGenerator* Generator =
		World->SpawnActor<APlanetarySurfaceGenerator>();
	if (!TestNotNull(TEXT("Atmosphere variation planet"), Planet)
		|| !TestNotNull(TEXT("Atmosphere variation generator"), Generator))
	{
		DestroyWorld(World);
		return false;
	}
	Planet->RadiusKM = 6371.0;
	Planet->PlanetRadiusKM = 6371;
	Planet->PlanetType = EPlanetType::Terrestrial;
	Planet->Temperature = 288;
	Planet->WorldScapeSeed = 41771;
	Planet->WorldScapePresentationScale = 1.0;
	Planet->PlanetAtmosphere.Humidity = 62.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;

	struct FAtmosphereProbe
	{
		FLinearColor Rayleigh = FLinearColor::Black;
		FLinearColor Mie = FLinearColor::Black;
		float Particulates = 0.0f;
		float MieHeight = 0.0f;
		float MiePhase = 0.0f;
		float AirGlow = 0.0f;
		float Opacity = 0.0f;
		float MultiScattering = 0.0f;
		float PresentationOpacityScale = 0.0f;
		float MainMaterialOpacity = -1.0f;
		float SpaceMaterialOpacity = -1.0f;
		FLinearColor OuterAirglow = FLinearColor::Black;
		FLinearColor InsideAirglow = FLinearColor::Black;
	};
	auto ResolveProbe = [Generator, Planet, World]()
	{
		Generator->InitAtmoScape(World, Planet->RadiusKM, Planet);
		FAtmosphereProbe Probe;
		if (IsValid(Generator->PlanetAtmosphere))
		{
			Probe.Rayleigh = Generator->PlanetAtmosphere->RayleighScattering;
			Probe.Mie = Generator->PlanetAtmosphere->MieScattering;
			Probe.Particulates = Generator->PlanetAtmosphere->AtmosphereParticulatesDensity;
			Probe.MieHeight = Generator->PlanetAtmosphere->MieHeight;
			Probe.MiePhase = Generator->PlanetAtmosphere->MiePhase;
			Probe.AirGlow = Generator->PlanetAtmosphere->AirGlowIntensity;
			Probe.Opacity = Generator->PlanetAtmosphere->AtmosphereOpacity;
			Probe.MultiScattering = Generator->PlanetAtmosphere->MultiScatering;
			Probe.PresentationOpacityScale =
				Generator->PlanetAtmosphere->PresentationOpacityScale;
			Probe.OuterAirglow = Generator->PlanetAtmosphere->OutterColor;
			Probe.InsideAirglow = Generator->PlanetAtmosphere->InsideColor;
			TInlineComponentArray<UStaticMeshComponent*> ProbeMeshes;
			Generator->PlanetAtmosphere->GetComponents(ProbeMeshes);
			for (UStaticMeshComponent* ProbeMesh : ProbeMeshes)
			{
				if (!IsValid(ProbeMesh)) continue;
				UMaterialInstanceDynamic* ProbeMaterial =
					Cast<UMaterialInstanceDynamic>(ProbeMesh->GetMaterial(0));
				if (!IsValid(ProbeMaterial)) continue;
				const FString ComponentName = ProbeMesh->GetName();
				if (ComponentName.Contains(TEXT("SpacePlanetaryAtmoMesh")))
				{
					Probe.SpaceMaterialOpacity =
						ProbeMaterial->K2_GetScalarParameterValue(TEXT("AtmosOpacity"));
				}
				else if (ComponentName.Contains(TEXT("PlanetaryAtmoMesh")))
				{
					Probe.MainMaterialOpacity =
						ProbeMaterial->K2_GetScalarParameterValue(TEXT("AtmosOpacity"));
				}
			}
		}
		return Probe;
	};

	const FAtmosphereProbe EarthFirst = ResolveProbe();
	const FAtmosphereProbe EarthRepeat = ResolveProbe();
	if (TestNotNull(TEXT("Atmosphere generator creates one reusable scattering shell"),
		Generator->PlanetAtmosphere))
	{
		TestTrue(TEXT("Same Surface Seed deterministically repeats Rayleigh scattering"),
			EarthFirst.Rayleigh.Equals(EarthRepeat.Rayleigh, 1.0e-6f));
		TestTrue(TEXT("Same Surface Seed deterministically repeats aerosol scattering"),
			EarthFirst.Mie.Equals(EarthRepeat.Mie, 1.0e-6f));
		TestTrue(TEXT("Same Surface Seed deterministically repeats scalar scattering"),
			FMath::IsNearlyEqual(EarthFirst.Particulates, EarthRepeat.Particulates, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.MieHeight, EarthRepeat.MieHeight, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.MiePhase, EarthRepeat.MiePhase, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.AirGlow, EarthRepeat.AirGlow, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.Opacity, EarthRepeat.Opacity, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.MultiScattering,
					EarthRepeat.MultiScattering, 1.0e-6f));
		TestTrue(TEXT("Full-scale atmosphere keeps its bounded optical-depth calibration"),
			FMath::IsNearlyEqual(EarthFirst.PresentationOpacityScale, 0.055f, 1.0e-6f));
		TestTrue(TEXT("Main atmosphere applies the full-scale opacity calibration exactly once"),
			EarthFirst.MainMaterialOpacity >= 0.0f
				&& FMath::IsNearlyEqual(EarthFirst.MainMaterialOpacity,
					EarthFirst.Opacity * EarthFirst.PresentationOpacityScale, 1.0e-4f));
		TestTrue(TEXT("Space atmosphere uses the same bounded calibration for a readable limb"),
			EarthFirst.SpaceMaterialOpacity >= 0.0f
				&& FMath::IsNearlyEqual(EarthFirst.SpaceMaterialOpacity,
					EarthFirst.Opacity * EarthFirst.PresentationOpacityScale, 1.0e-4f)
				&& EarthFirst.SpaceMaterialOpacity >= 0.20f
				&& EarthFirst.SpaceMaterialOpacity <= 1.0f);
		TestTrue(TEXT("Full-scale calibration does not modify deterministic scattering inputs"),
			FMath::IsNearlyEqual(EarthFirst.MainMaterialOpacity,
				EarthRepeat.MainMaterialOpacity, 1.0e-6f)
				&& FMath::IsNearlyEqual(EarthFirst.SpaceMaterialOpacity,
					EarthRepeat.SpaceMaterialOpacity, 1.0e-6f));
		TestTrue(TEXT("Same Surface Seed deterministically repeats natural airglow hues"),
			EarthFirst.OuterAirglow.Equals(EarthRepeat.OuterAirglow, 1.0e-6f)
				&& EarthFirst.InsideAirglow.Equals(EarthRepeat.InsideAirglow, 1.0e-6f));
		TestTrue(TEXT("Rayleigh coefficients remain in AtmoScape's visible physical range"),
			FMath::Max3(EarthFirst.Rayleigh.R, EarthFirst.Rayleigh.G,
				EarthFirst.Rayleigh.B) >= 16.0f
				&& !FMath::IsNearlyEqual(EarthFirst.Rayleigh.R,
					EarthFirst.Rayleigh.B, 0.1f));
		TestTrue(TEXT("Airglow palette retains a cool limb and warm horizon"),
			EarthFirst.OuterAirglow.B > EarthFirst.OuterAirglow.G
				&& EarthFirst.OuterAirglow.G > EarthFirst.OuterAirglow.R
				&& EarthFirst.InsideAirglow.R > EarthFirst.InsideAirglow.G
				&& EarthFirst.InsideAirglow.G > EarthFirst.InsideAirglow.B);
	}

	// AtmoScape owns Visible and toggles it on camera shell transitions. APS owns
	// HiddenInGame for duplicate full-scale passes; prove a later plugin visibility
	// toggle cannot resurrect the uniform cap, and re-init restores both flags.
	UStaticMeshComponent* OuterAirglowShell = nullptr;
	UStaticMeshComponent* SkylightShell = nullptr;
	UStaticMeshComponent* AbsorptionShell = nullptr;
	UStaticMeshComponent* MainScatteringShell = nullptr;
	if (!IsValid(Generator->PlanetAtmosphere))
	{
		Planet->Destroy();
		Generator->Destroy();
		DestroyWorld(World);
		return false;
	}
	TInlineComponentArray<UStaticMeshComponent*> AtmosphereMeshes;
	Generator->PlanetAtmosphere->GetComponents(AtmosphereMeshes);
	for (UStaticMeshComponent* AtmosphereMesh : AtmosphereMeshes)
	{
		if (!IsValid(AtmosphereMesh)) continue;
		const FString Name = AtmosphereMesh->GetName();
		if (Name.Contains(TEXT("PlanetarOutterMesh"))) OuterAirglowShell = AtmosphereMesh;
		else if (Name.Contains(TEXT("PlanetarySkylightMesh"))) SkylightShell = AtmosphereMesh;
		else if (Name.Contains(TEXT("PlanetaryAbsorptionMesh"))) AbsorptionShell = AtmosphereMesh;
		else if (Name.Contains(TEXT("PlanetaryAtmoMesh"))
			&& !Name.Contains(TEXT("SpacePlanetaryAtmoMesh")))
		{
			MainScatteringShell = AtmosphereMesh;
		}
	}
	const FName SuppressedPassTag(TEXT("APS.SuppressedAtmospherePass"));
	for (UStaticMeshComponent* SuppressedShell :
		{OuterAirglowShell, SkylightShell, AbsorptionShell})
	{
		if (TestNotNull(TEXT("Atmosphere owns each expected duplicate shell"),
			SuppressedShell))
		{
			TestTrue(TEXT("APS marks the duplicate atmosphere pass as suppressed"),
				SuppressedShell->ComponentHasTag(SuppressedPassTag));
			TestTrue(TEXT("Duplicate atmosphere pass is authoritatively hidden"),
				SuppressedShell->bHiddenInGame);
			TestEqual(TEXT("Duplicate atmosphere pass cannot collide"),
				SuppressedShell->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			SuppressedShell->SetVisibility(true, false);
			TestTrue(TEXT("Plugin visibility toggle cannot override APS hidden ownership"),
				SuppressedShell->GetVisibleFlag() && SuppressedShell->bHiddenInGame);
		}
	}
	if (TestNotNull(TEXT("Atmosphere retains its nonuniform main scattering shell"),
		MainScatteringShell))
	{
		TestFalse(TEXT("Main scattering shell is never suppressed"),
			MainScatteringShell->ComponentHasTag(SuppressedPassTag));
		TestFalse(TEXT("Main scattering shell remains renderable"),
			MainScatteringShell->bHiddenInGame);
	}
	Generator->InitAtmoScape(World, Planet->RadiusKM, Planet);
	for (UStaticMeshComponent* SuppressedShell :
		{OuterAirglowShell, SkylightShell, AbsorptionShell})
	{
		if (IsValid(SuppressedShell))
		{
			TestFalse(TEXT("Atmosphere re-init restores duplicate pass visibility flag"),
				SuppressedShell->GetVisibleFlag());
			TestTrue(TEXT("Atmosphere re-init retains duplicate pass hidden flag"),
				SuppressedShell->bHiddenInGame);
		}
	}

	Planet->WorldScapeSeed = 77839;
	const FAtmosphereProbe EarthDifferentSeed = ResolveProbe();
	TestTrue(TEXT("Surface Seed visibly phases atmosphere without global RNG"),
		!EarthFirst.Rayleigh.Equals(EarthDifferentSeed.Rayleigh, 1.0e-5f)
			|| !FMath::IsNearlyEqual(EarthFirst.Particulates,
				EarthDifferentSeed.Particulates, 1.0e-5f));

	Planet->PlanetType = EPlanetType::Desert;
	Planet->PlanetAtmosphere.Humidity = 5.0f;
	Planet->PlanetAtmosphere.AtmosphericPressure = 150000.0f;
	const FAtmosphereProbe Desert = ResolveProbe();
	TestTrue(TEXT("Dry subtype resolves warmer particulate scattering than humid Earth"),
		Desert.Mie.R > EarthDifferentSeed.Mie.R
			&& Desert.Mie.B < EarthDifferentSeed.Mie.B);
	for (const FAtmosphereProbe& Probe : {EarthFirst, EarthDifferentSeed, Desert})
	{
		TestTrue(TEXT("Atmosphere particulate density stays physically bounded"),
			Probe.Particulates >= 2.0f && Probe.Particulates <= 30.0f);
		TestTrue(TEXT("Atmosphere aerosol height stays physically bounded"),
			Probe.MieHeight >= 0.05f && Probe.MieHeight <= 15.0f);
		TestTrue(TEXT("Atmosphere anisotropy stays physically bounded"),
			Probe.MiePhase >= 0.08f && Probe.MiePhase <= 0.82f);
		TestTrue(TEXT("Atmosphere airglow remains a visible bounded limb cue"),
			Probe.AirGlow >= 0.018f && Probe.AirGlow <= 0.070f);
		TestTrue(TEXT("Atmosphere opacity avoids a uniform colour cap"),
			Probe.Opacity >= 4.5f && Probe.Opacity <= 18.0f);
		TestTrue(TEXT("Atmosphere multi-scattering remains performant and bounded"),
			Probe.MultiScattering >= 3.5f && Probe.MultiScattering <= 10.0f);
	}

	Planet->Destroy();
	Generator->Destroy();
	DestroyWorld(World);
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
