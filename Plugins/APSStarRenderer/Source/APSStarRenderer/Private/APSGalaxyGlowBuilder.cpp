// Rio 03.10 (galaxy phase 3): glow map builder (CPU, background-thread friendly).
// The model was checked offline against the point photometry (numpy port, 1M stars): outside views and
// in-plane views from inside the disk agree within ~10%; the faint halo far from the plane is ~0.4x.
#include "APSGalaxyGlowBuilder.h"

#include <limits>

namespace APSStarRenderer
{
	namespace
	{
		constexpr float RatioLaplace = 0.70710678f;  // E|u| / sigma of an exponential (Laplace) profile
		constexpr float RatioGauss = 0.79788456f;    // ... of a Gaussian
		constexpr float RatioUniform = 0.86602540f;  // ... of a flat slab

		/** Separable Gaussian with zero padding (the galaxy never touches the map edge, so mass is kept). */
		void BlurSeparable(TArray<float>& Data, const int32 Resolution, const float Sigma)
		{
			if (!(Sigma > 0.05f) || Data.Num() != Resolution * Resolution)
			{
				return;
			}
			const int32 Radius = FMath::Clamp(FMath::CeilToInt(Sigma * 3.0f), 1, 32);
			TArray<float> Kernel;
			Kernel.SetNumUninitialized(2 * Radius + 1);
			float KernelSum = 0.0f;
			for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
			{
				const float Weight = FMath::Exp(-0.5f * FMath::Square(static_cast<float>(Offset) / Sigma));
				Kernel[Offset + Radius] = Weight;
				KernelSum += Weight;
			}
			for (float& Weight : Kernel)
			{
				Weight /= KernelSum;
			}

			TArray<float> Temp;
			Temp.SetNumZeroed(Data.Num());
			for (int32 Y = 0; Y < Resolution; ++Y)
			{
				for (int32 X = 0; X < Resolution; ++X)
				{
					float Sum = 0.0f;
					for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
					{
						const int32 SampleX = X + Offset;
						if (SampleX >= 0 && SampleX < Resolution)
						{
							Sum += Data[Y * Resolution + SampleX] * Kernel[Offset + Radius];
						}
					}
					Temp[Y * Resolution + X] = Sum;
				}
			}
			for (int32 Y = 0; Y < Resolution; ++Y)
			{
				for (int32 X = 0; X < Resolution; ++X)
				{
					float Sum = 0.0f;
					for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
					{
						const int32 SampleY = Y + Offset;
						if (SampleY >= 0 && SampleY < Resolution)
						{
							Sum += Temp[SampleY * Resolution + X] * Kernel[Offset + Radius];
						}
					}
					Data[Y * Resolution + X] = Sum;
				}
			}
		}

		TArray<float> Blurred(const TArray<float>& Data, const int32 Resolution, const float Sigma)
		{
			TArray<float> Result = Data;
			BlurSeparable(Result, Resolution, Sigma);
			return Result;
		}

		/** Count-weighted smoothing: sum(w v) / sum(w) under the kernel; NaN where no weight. */
		TArray<float> WeightedSmooth(const TArray<float>& Values, const TArray<float>& Weights, const int32 Resolution,
			const float Sigma)
		{
			TArray<float> Numerator;
			Numerator.SetNumUninitialized(Values.Num());
			for (int32 Index = 0; Index < Values.Num(); ++Index)
			{
				Numerator[Index] = Weights[Index] > 0.0f ? Values[Index] * Weights[Index] : 0.0f;
			}
			BlurSeparable(Numerator, Resolution, Sigma);
			const TArray<float> Denominator = Blurred(Weights, Resolution, Sigma);
			for (int32 Index = 0; Index < Values.Num(); ++Index)
			{
				Numerator[Index] = Denominator[Index] > 1.0e-6f ? Numerator[Index] / Denominator[Index]
					: std::numeric_limits<float>::quiet_NaN();
			}
			return Numerator;
		}

		float ShapeFromRatio(const float Ratio)
		{
			const float Shape = Ratio < RatioGauss
				? -(RatioGauss - Ratio) / (RatioGauss - RatioLaplace)
				: (Ratio - RatioGauss) / (RatioUniform - RatioGauss);
			return FMath::Clamp(Shape, -1.0f, 1.0f);
		}
	}

	FGlowMapBuilder::FGlowMapBuilder(const int32 InResolution, const float InExtentXY)
		: Resolution(FMath::Clamp(InResolution, 16, 1024))
		, ExtentXY(FMath::IsFinite(InExtentXY) && InExtentXY > 0.0f ? InExtentXY : 1.0f)
	{
		Cells.SetNum(Resolution * Resolution);
	}

	void FGlowMapBuilder::AddStar(const FVector3f& LocalPosition, const FLinearColor& Color, const float Intensity,
		const float DustWeight)
	{
		const float U = LocalPosition.X / ExtentXY * 0.5f + 0.5f;
		const float V = LocalPosition.Y / ExtentXY * 0.5f + 0.5f;
		const float Z = LocalPosition.Z / ExtentXY;
		// Written so that NaN fails every test.
		if (!(U >= 0.0f && U < 1.0f && V >= 0.0f && V < 1.0f && FMath::IsFinite(Z)))
		{
			return;
		}
		const float Light = FMath::IsFinite(Intensity) ? FMath::Max(Intensity, 0.0f) : 0.0f;
		const int32 X = FMath::Min(static_cast<int32>(U * Resolution), Resolution - 1);
		const int32 Y = FMath::Min(static_cast<int32>(V * Resolution), Resolution - 1);
		const int32 CellIndex = Y * Resolution + X;
		FCell& Cell = Cells[CellIndex];
		Cell.Count += 1.0f;
		Cell.SumR += Light * FMath::Max(Color.R, 0.0f);
		Cell.SumG += Light * FMath::Max(Color.G, 0.0f);
		Cell.SumB += Light * FMath::Max(Color.B, 0.0f);
		Cell.SumZ += Z;
		Cell.SumDust += FMath::Clamp(DustWeight, 0.0f, 1.0f);
		SampleCells.Add(static_cast<uint32>(CellIndex));
		SampleHeights.Add(Z);
		++StarCount;
		IntensitySum += Light;
		MaxAbsZ = FMath::Max(MaxAbsZ, FMath::Abs(Z));
	}

	bool FGlowMapBuilder::Merge(const FGlowMapBuilder& Other)
	{
		if (Other.Resolution != Resolution || !FMath::IsNearlyEqual(Other.ExtentXY, ExtentXY))
		{
			return false;
		}
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			FCell& Cell = Cells[Index];
			const FCell& Source = Other.Cells[Index];
			Cell.Count += Source.Count;
			Cell.SumR += Source.SumR;
			Cell.SumG += Source.SumG;
			Cell.SumB += Source.SumB;
			Cell.SumZ += Source.SumZ;
			Cell.SumDust += Source.SumDust;
		}
		SampleCells.Append(Other.SampleCells);
		SampleHeights.Append(Other.SampleHeights);
		StarCount += Other.StarCount;
		IntensitySum += Other.IntensitySum;
		MaxAbsZ = FMath::Max(MaxAbsZ, Other.MaxAbsZ);
		return true;
	}

	bool FGlowMapBuilder::Build(FGlowMap& OutMap, const float ShapeBlurTexels, const float ColorBlurTexels,
		const float ProfileBlurTexels) const
	{
		if (StarCount <= 0 || !(IntensitySum > 0.0) || SampleCells.Num() != SampleHeights.Num())
		{
			return false;
		}
		const int32 TexelCount = Resolution * Resolution;
		TArray<float> Count;
		TArray<float> SumZ;
		TArray<float> Dust;
		TArray<float> SumR;
		TArray<float> SumG;
		TArray<float> SumB;
		Count.SetNumUninitialized(TexelCount);
		SumZ.SetNumUninitialized(TexelCount);
		Dust.SetNumUninitialized(TexelCount);
		SumR.SetNumUninitialized(TexelCount);
		SumG.SetNumUninitialized(TexelCount);
		SumB.SetNumUninitialized(TexelCount);
		for (int32 Index = 0; Index < TexelCount; ++Index)
		{
			const FCell& Cell = Cells[Index];
			Count[Index] = Cell.Count;
			SumZ[Index] = Cell.SumZ;
			Dust[Index] = Cell.SumDust;
			SumR[Index] = Cell.SumR;
			SumG[Index] = Cell.SumG;
			SumB[Index] = Cell.SumB;
		}

		// ---- Light: star density (narrow kernel) x smoothed light per star (wide kernel) ----
		const TArray<float> CountShape = Blurred(Count, Resolution, ShapeBlurTexels);
		const TArray<float> CountColor = Blurred(Count, Resolution, ColorBlurTexels);
		BlurSeparable(SumR, Resolution, ColorBlurTexels);
		BlurSeparable(SumG, Resolution, ColorBlurTexels);
		BlurSeparable(SumB, Resolution, ColorBlurTexels);
		BlurSeparable(Dust, Resolution, ShapeBlurTexels);

		// ---- Column statistics ----
		// Centre: smoothed mean height. Spread s0 = mean |u| per texel, refined to s1 over |u| <= 3 s0,
		// thin = |u| <= 2.5 s1, thick = the rest. Raw sums are never blurred across texels.
		const TArray<float> CountProfile = Blurred(Count, Resolution, ProfileBlurTexels);
		TArray<float> Centre = Blurred(SumZ, Resolution, ProfileBlurTexels);
		for (int32 Index = 0; Index < TexelCount; ++Index)
		{
			Centre[Index] = CountProfile[Index] > 1.0e-3f ? Centre[Index] / CountProfile[Index] : 0.0f;
		}
		const int32 Samples = SampleCells.Num();
		TArray<float> AbsSum;
		AbsSum.SetNumZeroed(TexelCount);
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const uint32 Cell = SampleCells[Sample];
			AbsSum[Cell] += FMath::Abs(SampleHeights[Sample] - Centre[Cell]);
		}
		TArray<float> Spread0;
		Spread0.SetNumUninitialized(TexelCount);
		for (int32 Index = 0; Index < TexelCount; ++Index)
		{
			Spread0[Index] = FMath::Max(AbsSum[Index] / FMath::Max(Count[Index], 1.0e-3f), 1.0e-4f);
		}
		TArray<float> NearCount;
		TArray<float> NearAbs;
		NearCount.SetNumZeroed(TexelCount);
		NearAbs.SetNumZeroed(TexelCount);
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const uint32 Cell = SampleCells[Sample];
			const float Height = FMath::Abs(SampleHeights[Sample] - Centre[Cell]);
			if (Height <= 3.0f * Spread0[Cell])
			{
				NearCount[Cell] += 1.0f;
				NearAbs[Cell] += Height;
			}
		}
		TArray<float> ThinCount;
		TArray<float> ThinU2;
		TArray<float> ThinAbs;
		TArray<float> ThickCount;
		TArray<float> ThickU2;
		ThinCount.SetNumZeroed(TexelCount);
		ThinU2.SetNumZeroed(TexelCount);
		ThinAbs.SetNumZeroed(TexelCount);
		ThickCount.SetNumZeroed(TexelCount);
		ThickU2.SetNumZeroed(TexelCount);
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const uint32 Cell = SampleCells[Sample];
			const float Offset = SampleHeights[Sample] - Centre[Cell];
			const float Height = FMath::Abs(Offset);
			const float Spread1 = FMath::Max(NearAbs[Cell] / FMath::Max(NearCount[Cell], 1.0e-3f), 1.0e-4f);
			if (Height <= 2.5f * Spread1)
			{
				ThinCount[Cell] += 1.0f;
				ThinU2[Cell] += Offset * Offset;
				ThinAbs[Cell] += Height;
			}
			else
			{
				ThickCount[Cell] += 1.0f;
				ThickU2[Cell] += Offset * Offset;
			}
		}

		// Per-texel sigma and E|u| / sigma, smoothed in log space weighted by counts.
		TArray<float> LogSigmaThin;
		TArray<float> RatioThin;
		TArray<float> LogSigmaThick;
		TArray<float> WeightThin;
		TArray<float> WeightThick;
		LogSigmaThin.SetNumUninitialized(TexelCount);
		RatioThin.SetNumUninitialized(TexelCount);
		LogSigmaThick.SetNumUninitialized(TexelCount);
		WeightThin.SetNumUninitialized(TexelCount);
		WeightThick.SetNumUninitialized(TexelCount);
		for (int32 Index = 0; Index < TexelCount; ++Index)
		{
			const float ThinN = FMath::Max(ThinCount[Index], 1.0e-3f);
			const float SigmaThin = FMath::Sqrt(FMath::Max(ThinU2[Index] / ThinN, 1.0e-10f));
			LogSigmaThin[Index] = FMath::Loge(SigmaThin);
			RatioThin[Index] = FMath::Clamp(ThinAbs[Index] / ThinN / FMath::Max(SigmaThin, 1.0e-8f), 0.5f, 1.0f);
			const float SigmaThick = FMath::Sqrt(FMath::Max(ThickU2[Index] / FMath::Max(ThickCount[Index], 1.0e-3f), 1.0e-10f));
			LogSigmaThick[Index] = FMath::Loge(SigmaThick);
			WeightThin[Index] = ThinCount[Index] > 0.5f ? ThinCount[Index] : 0.0f;
			WeightThick[Index] = ThickCount[Index] > 0.5f ? ThickCount[Index] : 0.0f;
		}
		constexpr float WideBlurTexels = 8.0f;
		const TArray<float> LogSigmaThinSmooth = WeightedSmooth(LogSigmaThin, WeightThin, Resolution, ProfileBlurTexels);
		const TArray<float> LogSigmaThinWide = WeightedSmooth(LogSigmaThin, WeightThin, Resolution, WideBlurTexels);
		const TArray<float> RatioSmooth = WeightedSmooth(RatioThin, WeightThin, Resolution, ProfileBlurTexels);
		const TArray<float> RatioWide = WeightedSmooth(RatioThin, WeightThin, Resolution, WideBlurTexels);
		const TArray<float> LogSigmaThickSmooth = WeightedSmooth(LogSigmaThick, WeightThick, Resolution, ProfileBlurTexels);
		const TArray<float> LogSigmaThickWide = WeightedSmooth(LogSigmaThick, WeightThick, Resolution, WideBlurTexels);
		const TArray<float> ThinSupport = Blurred(WeightThin, Resolution, ProfileBlurTexels);
		const TArray<float> ThickSupport = Blurred(WeightThick, Resolution, ProfileBlurTexels);
		const TArray<float> ThinShare = Blurred(ThinCount, Resolution, ProfileBlurTexels);
		const TArray<float> ThickShare = Blurred(ThickCount, Resolution, ProfileBlurTexels);

		// ---- Output ----
		const double TexelArea = FMath::Square(2.0 / Resolution);
		const double InvTotal = 1.0 / IntensitySum;
		OutMap.Resolution = Resolution;
		OutMap.ExtentXY = ExtentXY;
		OutMap.SampleCount = StarCount;
		OutMap.MeanIntensity = IntensitySum / static_cast<double>(StarCount);
		OutMap.Emission.SetNumZeroed(TexelCount);
		OutMap.Profile.SetNumZeroed(TexelCount);
		OutMap.Shape.SetNumZeroed(TexelCount);

		double PeakEmission = 0.0;
		float PeakDust = 0.0f;
		float MaxColumn = 0.0f;
		for (int32 Index = 0; Index < TexelCount; ++Index)
		{
			// Expected light here = local star density x smoothed light per star (rgb), per unit map area.
			const double Scale = static_cast<double>(CountShape[Index]) / FMath::Max(CountColor[Index], 1.0e-3f)
				* InvTotal / TexelArea;
			const FVector3d Light(SumR[Index] * Scale, SumG[Index] * Scale, SumB[Index] * Scale);
			OutMap.Emission[Index] = FVector4f(static_cast<float>(Light.X), static_cast<float>(Light.Y),
				static_cast<float>(Light.Z), Dust[Index]);
			PeakEmission = FMath::Max(PeakEmission, FMath::Max3(Light.X, Light.Y, Light.Z));
			PeakDust = FMath::Max(PeakDust, Dust[Index]);

			// Sparse texels fall back to a wide count-weighted field, never to a fixed thickness.
			const bool bThinValid = ThinSupport[Index] > 3.0f && FMath::IsFinite(LogSigmaThinSmooth[Index]);
			const bool bThickValid = ThickSupport[Index] > 1.0f && FMath::IsFinite(LogSigmaThickSmooth[Index]);
			float SigmaThin = FMath::Exp(bThinValid ? LogSigmaThinSmooth[Index] : LogSigmaThinWide[Index]);
			float Ratio = bThinValid ? RatioSmooth[Index] : RatioWide[Index];
			float SigmaThick = FMath::Exp(bThickValid ? LogSigmaThickSmooth[Index] : LogSigmaThickWide[Index]);
			if (!FMath::IsFinite(SigmaThin) || !(SigmaThin > 0.0f))
			{
				SigmaThin = 0.02f;
			}
			if (!FMath::IsFinite(Ratio))
			{
				Ratio = RatioGauss;
			}
			if (!FMath::IsFinite(SigmaThick) || !(SigmaThick > 0.0f))
			{
				SigmaThick = 0.2f;
			}
			SigmaThin = FMath::Max(SigmaThin, 1.0e-4f);
			SigmaThick = FMath::Max(SigmaThick, SigmaThin);
			const float ShareSum = ThinShare[Index] + ThickShare[Index];
			const float ThinFraction = ThickSupport[Index] > 1.0f && ShareSum > 1.0e-6f
				? FMath::Clamp(ThinShare[Index] / ShareSum, 0.0f, 1.0f) : 1.0f;
			OutMap.Profile[Index] = FVector4f(SigmaThin, Centre[Index], SigmaThick, ThinFraction);
			OutMap.Shape[Index] = ShapeFromRatio(Ratio);
			if (CountShape[Index] > 1.0e-3f)
			{
				MaxColumn = FMath::Max(MaxColumn, FMath::Abs(Centre[Index]) + 4.0f * (ThinFraction < 1.0f ? SigmaThick : SigmaThin));
			}
		}
		if (!(PeakEmission > 0.0))
		{
			return false;
		}
		const float InvPeakEmission = static_cast<float>(1.0 / PeakEmission);
		const float InvPeakDust = PeakDust > 0.0f ? 1.0f / PeakDust : 0.0f;
		for (FVector4f& Texel : OutMap.Emission)
		{
			Texel.X *= InvPeakEmission;
			Texel.Y *= InvPeakEmission;
			Texel.Z *= InvPeakEmission;
			Texel.W *= InvPeakDust;
		}
		OutMap.EmissionNorm = static_cast<float>(PeakEmission);
		// Stars beyond the march box still count in the map; keep the box around every populated column.
		OutMap.ExtentZ = FMath::Clamp(FMath::Max(MaxColumn, MaxAbsZ * 1.02f), 0.01f, 2.0f);
		return true;
	}
}
