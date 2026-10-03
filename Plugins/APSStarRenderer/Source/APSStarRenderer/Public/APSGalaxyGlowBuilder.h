// Rio 03.10 (galaxy phase 3): CPU builder of the galaxy glow map from catalogue samples.
// The glow is derived from the same catalogue as the points (any type, subclass or future
// morphology), so there is no second copy of the morphology to keep in sync.
#pragma once

#include "CoreMinimal.h"
#include "APSStarRendererAPI.h"

namespace APSStarRenderer
{
	/**
	 * Accumulates catalogue stars into a square map over [-ExtentXY, ExtentXY]^2 (local units; the
	 * galaxy disk lies in the local XY plane). Not thread-safe: one builder per task, then Merge.
	 *
	 * Per map column the vertical light is split into a thin component (disk; its shape runs from
	 * exponential through Gaussian to a flat slab, from E|u| / sigma) and a thick Gaussian component
	 * (bulge/halo stars far from the mid-plane). Column statistics are computed per texel without
	 * blurring raw moments (a few halo stars must not inflate a disk column), then smoothed in
	 * log space weighted by star counts. Light (counts x smoothed light per star) and shape use
	 * separate kernels: counts are dense, the light per star is dominated by rare O/B stars.
	 * Memory: 8 bytes per added star until Build (second pass over the samples).
	 */
	class APSSTARRENDERER_API FGlowMapBuilder
	{
	public:
		/** Resolution 16..1024 texels per side. */
		FGlowMapBuilder(int32 InResolution, float InExtentXY);

		/** One star: local position, linear colour (palette colour), decoded intensity, dust tracer 0..1 (young/hot stars ~1). */
		void AddStar(const FVector3f& LocalPosition, const FLinearColor& Color, float Intensity, float DustWeight);
		/** Same resolution and extent required; returns false otherwise. */
		bool Merge(const FGlowMapBuilder& Other);
		/** Fills OutMap. False when no light was added. */
		bool Build(FGlowMap& OutMap, float ShapeBlurTexels = 1.0f, float ColorBlurTexels = 3.0f, float ProfileBlurTexels = 2.5f) const;

		int64 GetStarCount() const { return StarCount; }
		double GetIntensitySum() const { return IntensitySum; }
		int32 GetResolution() const { return Resolution; }
		float GetExtentXY() const { return ExtentXY; }

	private:
		struct FCell
		{
			float Count = 0.0f;
			float SumR = 0.0f;
			float SumG = 0.0f;
			float SumB = 0.0f;
			float SumZ = 0.0f;
			float SumDust = 0.0f;
		};

		int32 Resolution = 0;
		float ExtentXY = 1.0f;
		TArray<FCell> Cells;
		/** Per added star: map cell and height (map units), for the column statistics. */
		TArray<uint32> SampleCells;
		TArray<float> SampleHeights;
		int64 StarCount = 0;
		double IntensitySum = 0.0;
		float MaxAbsZ = 0.0f;
	};
}
