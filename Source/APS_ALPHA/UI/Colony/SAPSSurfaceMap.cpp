#include "SAPSSurfaceMap.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceRadius.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Math/RandomStream.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "TextureResource.h"
#include "UObject/Package.h"
#include <atomic>

#define LOCTEXT_NAMESPACE "APSSurfaceMap"

namespace APSSurfaceMapPrivate
{
	constexpr int32 MapWidth = 512;
	constexpr int32 MapHeight = 256;
	/** Relief is a few km on a world thousands of km across: the shading slope is exaggerated to read at map scale. */
	constexpr double ReliefExaggeration = 14.0;
	/** Where the liquid ends in the material's normalized height channel (APSPlanetNoise::MaterialCoastHeight). */
	constexpr double CoastHeight = 0.08;
	constexpr double LandHeightRange = 0.78;
	/** Survey data shows the surface in blocks of this many texels (Rio 02.10: coarse until studied). */
	constexpr int32 CoarseBlock = 8;
	/** A settled close view is sampled again at this size over its window, with a margin for small pans. */
	constexpr int32 DetailWidth = 512;
	constexpr int32 DetailHeight = 256;
	constexpr double DetailMargin = 1.2;
	constexpr float DetailFromZoom = 2.0f;
	constexpr double DetailSettleSeconds = 0.25;
	/** x1.25 per wheel notch, x1 to x12 (Rio 02.10: "zoom"). */
	constexpr float ZoomStep = 1.25f;
	constexpr float MaxZoom = 12.0f;
	/** The view's centre stays off the poles, where the globe's basis turns over. */
	constexpr double MaxCentreLatitude = 1.45;
	/** A press that moves less than this (screen pixels) is a click, not a drag. */
	constexpr double ClickSlop = 4.0;
	/** Auto-turn waits this long after the last touch. */
	constexpr double IdleSeconds = 2.5;

	/** What one texel of a sampled window holds; colouring reads only this, so a new look never samples again. */
	struct FTexel
	{
		/** Signed displacement from the reference sphere, cm. */
		float Elevation{0.0f};
		/** The material height channel, 0..1: liquid lies below CoastHeight. */
		float Level{0.0f};
		float Liquid{0.0f};
		float Temperature{0.0f};
		float Humidity{0.0f};
		/** Rise per distance toward the east and toward the north. */
		FVector2f Slope{0.0f, 0.0f};
	};

	/** How a job colours its texels. */
	struct FStyle
	{
		SAPSSurfaceMap::EMode Mode{SAPSSurfaceMap::EMode::Terrain};
		/** Survey data only: blocks, pale. */
		bool bCoarse{false};
	};
}

/** One world's sampler: value copies of its profile and seeded noise, and what colours it. */
struct SAPSSurfaceMap::FSampler
{
	FAPSResolvedPlanetSurfaceProfile Profile;
	CustomNoise Noise;
	double RadiusCm{100000.0};
	double NoiseScale{1.0};
	double NoiseIntensity{1.0};
	bool bLava{false};
	/** The atmosphere's density: 0 airless, 1 Earth-like. */
	float Air{0.0f};
};

/** One sampled window of the unwrapped map: the whole surface, or a zoomed view sampled finer. */
struct SAPSSurfaceMap::FBake
{
	/** Left and top edge and size in map UV; U may run past 1 (longitude wraps). */
	double U0{0.0};
	double V0{0.0};
	double USize{1.0};
	double VSize{1.0};
	int32 Width{0};
	int32 Height{0};
	/** Row by row from the north edge. */
	TArray<APSSurfaceMapPrivate::FTexel> Texels;
	float MinElevation{0.0f};
	float MaxElevation{0.0f};
	float WaterShare{0.0f};
	/** The hillshade's slope exaggeration for this window's texel spacing. */
	float Relief{1.0f};
	bool bValid{false};
};

/** Work for the thread pool: sample the window when not sampled yet, then colour it. */
struct SAPSSurfaceMap::FJob
{
	TSharedPtr<FBake, ESPMode::ThreadSafe> Fields;
	TSharedPtr<const FSampler, ESPMode::ThreadSafe> Sampler;
	APSSurfaceMapPrivate::FStyle Style;
	bool bDetail{false};
	/** The colours, row by row; the whole surface repeats half a turn past its end, so a globe triangle across the
	 * date line samples one continuous strip. */
	TArray<FColor> Pixels;
	int32 PixelWidth{0};
	std::atomic<bool> bDone{false};
	/** Set when a newer job replaces this one: the sampling stops early. */
	std::atomic<bool> bCancelled{false};
};

namespace APSSurfaceMapPrivate
{
	FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, const float T)
	{
		return A + (B - A) * FMath::Clamp(T, 0.0f, 1.0f);
	}

	FLinearColor Ramp(const FAPSPlanetSurfacePalette& Palette, const double Land)
	{
		if (Land < 0.06) return Mix(Palette.Coast, Palette.Lowland, float(Land / 0.06));
		if (Land < 0.35) return Mix(Palette.Lowland, Palette.MidLowland, float((Land - 0.06) / 0.29));
		if (Land < 0.65) return Mix(Palette.MidLowland, Palette.Highland, float((Land - 0.35) / 0.30));
		return Mix(Palette.Highland, Palette.Peak, float((Land - 0.65) / 0.35));
	}

	/** Samples the window on worker threads (the sampler the terrain workers use), then the slopes between texels. */
	void SampleWindow(SAPSSurfaceMap::FBake& Out, const SAPSSurfaceMap::FSampler& Sampler, const std::atomic<bool>& bCancelled)
	{
		const int32 W = Out.Width;
		const int32 H = Out.Height;
		if (W < 2 || H < 2) return;
		TArray<FNoiseData> Samples;
		Samples.SetNum(W * H);
		TArray<uint8> RowsValid;
		RowsValid.SetNumZeroed(H);
		ParallelFor(H, [&](const int32 Row)
		{
			if (bCancelled)
			{
				return;
			}
			CustomNoise RowNoise = Sampler.Noise;
			const double Latitude = (0.5 - (Out.V0 + (Row + 0.5) / H * Out.VSize)) * UE_PI;
			bool bRowValid = true;
			for (int32 Column = 0; Column < W; ++Column)
			{
				const double Longitude = (Out.U0 + (Column + 0.5) / W * Out.USize - 0.5) * UE_TWO_PI;
				const FVector Direction(FMath::Cos(Latitude) * FMath::Cos(Longitude),
					FMath::Cos(Latitude) * FMath::Sin(Longitude), FMath::Sin(Latitude));
				DVector NoisePosition;
				FNoiseData& Sample = Samples[Row * W + Column];
				Sample = UAPSWorldScapePlanetNoise::SampleResolvedProfile(Sampler.Profile, RowNoise, DVector(Direction * Sampler.RadiusCm),
					DVector(0.0, 0.0, 0.0), Sampler.NoiseScale, Sampler.NoiseIntensity, Sampler.RadiusCm, Direction.Z, NoisePosition);
				bRowValid = bRowValid && FMath::IsFinite(Sample.Height) && FMath::IsFinite(Sample.HeightNormalize)
					&& FMath::IsFinite(Sample.WaterMask);
			}
			RowsValid[Row] = bRowValid ? 1 : 0;
		});
		if (RowsValid.Contains(0))
		{
			return;
		}

		// A whole turn wraps east to west; a window repeats its edge texels.
		const bool bWholeTurn = Out.USize >= 1.0 - 1.0e-6;
		const double RadiusCm = Sampler.RadiusCm;
		Out.Texels.SetNumUninitialized(W * H);
		TArray<float> RowLow;
		TArray<float> RowHigh;
		TArray<int32> RowWater;
		RowLow.SetNumUninitialized(H);
		RowHigh.SetNumUninitialized(H);
		RowWater.SetNumZeroed(H);
		ParallelFor(H, [&](const int32 Row)
		{
			const double Latitude = (0.5 - (Out.V0 + (Row + 0.5) / H * Out.VSize)) * UE_PI;
			const double SpacingX = FMath::Max(UE_TWO_PI * Out.USize * RadiusCm * FMath::Cos(Latitude) / W, RadiusCm * 1.0e-3 * Out.USize);
			const double SpacingY = UE_PI * Out.VSize * RadiusCm / H;
			const int32 North = FMath::Max(Row - 1, 0);
			const int32 South = FMath::Min(Row + 1, H - 1);
			float Low = TNumericLimits<float>::Max();
			float High = TNumericLimits<float>::Lowest();
			int32 Water = 0;
			for (int32 Column = 0; Column < W; ++Column)
			{
				const int32 East = bWholeTurn ? (Column + 1) % W : FMath::Min(Column + 1, W - 1);
				const int32 West = bWholeTurn ? (Column + W - 1) % W : FMath::Max(Column - 1, 0);
				const double Across = bWholeTurn ? 2.0 : double(FMath::Max(East - West, 1));
				const FNoiseData& Sample = Samples[Row * W + Column];
				FTexel& Texel = Out.Texels[Row * W + Column];
				Texel.Elevation = float(Sample.Height);
				Texel.Level = float(Sample.HeightNormalize);
				Texel.Liquid = FMath::Clamp(Sample.WaterMask, 0.0f, 1.0f);
				Texel.Temperature = Sample.Temperature;
				Texel.Humidity = Sample.Humidity;
				Texel.Slope = FVector2f(
					float((Samples[Row * W + East].Height - Samples[Row * W + West].Height) / (Across * SpacingX)),
					float((Samples[North * W + Column].Height - Samples[South * W + Column].Height)
						/ (double(FMath::Max(South - North, 1)) * SpacingY)));
				Low = FMath::Min(Low, Texel.Elevation);
				High = FMath::Max(High, Texel.Elevation);
				Water += Texel.Liquid > 0.5f ? 1 : 0;
			}
			RowLow[Row] = Low;
			RowHigh[Row] = High;
			RowWater[Row] = Water;
		});
		Out.MinElevation = RowLow[0];
		Out.MaxElevation = RowHigh[0];
		int32 WaterCells = 0;
		for (int32 Row = 0; Row < H; ++Row)
		{
			Out.MinElevation = FMath::Min(Out.MinElevation, RowLow[Row]);
			Out.MaxElevation = FMath::Max(Out.MaxElevation, RowHigh[Row]);
			WaterCells += RowWater[Row];
		}
		Out.WaterShare = float(WaterCells) / float(W * H);
		// A finer window shows steeper slopes between its texels: softer exaggeration keeps the shading readable.
		Out.Relief = float(ReliefExaggeration / FMath::Sqrt(FMath::Max(1.0 / FMath::Max(Out.USize, 1.0e-3), 1.0)));
		Out.bValid = true;
	}

	/** The palette and hillshade look (the map as it was first made). */
	FLinearColor TerrainColour(const FTexel& Texel, const SAPSSurfaceMap::FSampler& Sampler, const float Shade,
		const FLinearColor& LavaGlow)
	{
		const FAPSPlanetSurfacePalette& Palette = Sampler.Profile.Palette;
		const double Land = FMath::Clamp((Texel.Level - CoastHeight) / LandHeightRange, 0.0, 1.0);
		const float Cold = FMath::Clamp((0.2f - Texel.Temperature) / 0.2f, 0.0f, 1.0f);
		FLinearColor Ground = Ramp(Palette, Land);
		Ground = Mix(Ground, Palette.Dryland, FMath::Clamp(1.0f - Texel.Humidity, 0.0f, 1.0f) * 0.45f * float(1.0 - Land));
		Ground = Mix(Ground, FLinearColor(0.78f, 0.82f, 0.86f), Cold * 0.85f) * Shade;
		FLinearColor Sea;
		if (Sampler.bLava)
		{
			Sea = Mix(LavaGlow * 0.85f, FLinearColor(1.0f, 0.76f, 0.32f), 0.25f);
		}
		else
		{
			const float Shallow = FMath::Clamp(Texel.Level / float(CoastHeight), 0.0f, 1.0f);
			Sea = Mix(FLinearColor(0.010f, 0.032f, 0.080f), FLinearColor(0.035f, 0.14f, 0.21f), Shallow * Shallow);
			Sea = Mix(Sea, FLinearColor(0.70f, 0.76f, 0.82f), Cold * 0.6f);
		}
		return Mix(Ground, Sea, Texel.Liquid);
	}

	/** The relief shade (0.42..1.16) as 0..1. */
	float ShadeShare(const float Shade)
	{
		return FMath::Clamp((Shade - 0.42f) / 0.74f, 0.0f, 1.0f);
	}

	/**
	 * Natural tones over the palette (Rio 02.10, "realistic"): forest where it is warm and wet on a living world, sand
	 * where it is dry, bare rock up high, snow where it is cold; a softer light and deeper seas.
	 */
	FLinearColor RealisticColour(const FTexel& Texel, const SAPSSurfaceMap::FSampler& Sampler, const float Shade,
		const FLinearColor& LavaGlow)
	{
		const FLinearColor Base = TerrainColour(Texel, Sampler, 1.0f, LavaGlow);
		const double Land = FMath::Clamp((Texel.Level - CoastHeight) / LandHeightRange, 0.0, 1.0);
		const float Wet = FMath::Clamp(Texel.Humidity, 0.0f, 1.0f);
		const float Warm = FMath::Clamp((Texel.Temperature - 0.15f) / 0.45f, 0.0f, 1.0f);
		const float Cold = FMath::Clamp((0.2f - Texel.Temperature) / 0.2f, 0.0f, 1.0f);
		const bool bLiving = Sampler.Profile.Biomass > 0.02f;
		FLinearColor Natural = Mix(FLinearColor(0.46f, 0.38f, 0.26f),
			bLiving ? FLinearColor(0.06f, 0.13f, 0.045f) : FLinearColor(0.30f, 0.27f, 0.23f), Wet * Warm);
		Natural = Mix(Natural, FLinearColor(0.32f, 0.30f, 0.28f), float(FMath::SmoothStep(0.45, 0.85, Land)));
		Natural = Mix(Natural, FLinearColor(0.86f, 0.88f, 0.92f), Cold * 0.9f);
		const FLinearColor Ground = Mix(Base, Natural, 0.55f) * FMath::Lerp(0.72f, 1.0f, ShadeShare(Shade));
		if (Texel.Liquid <= 0.0f || Sampler.bLava)
		{
			return Texel.Liquid <= 0.0f ? Ground : Mix(Ground, Base, Texel.Liquid);
		}
		const float Shallow = FMath::Clamp(Texel.Level / float(CoastHeight), 0.0f, 1.0f);
		FLinearColor Sea = Mix(FLinearColor(0.004f, 0.018f, 0.055f), FLinearColor(0.03f, 0.12f, 0.17f), Shallow * Shallow * Shallow);
		Sea = Mix(Sea, FLinearColor(0.80f, 0.84f, 0.88f), Cold * 0.7f);
		return Mix(Ground, Sea, Texel.Liquid);
	}

	/**
	 * Elevation tints with contour lines (Rio 02.10, "geology"): depths in blue, lowlands green, highlands yellow to
	 * brown, peaks grey and white, and a strong relief shade.
	 */
	FLinearColor GeologyColour(const FTexel& Texel, const float Shade)
	{
		const float Light = 0.45f + 0.8f * ShadeShare(Shade);
		if (Texel.Liquid > 0.5f)
		{
			const float Depth = FMath::Clamp(1.0f - Texel.Level / float(CoastHeight), 0.0f, 1.0f);
			return Mix(FLinearColor(0.42f, 0.66f, 0.86f), FLinearColor(0.03f, 0.10f, 0.32f), Depth) * FMath::Lerp(0.85f, 1.05f, Light - 0.45f);
		}
		static const FLinearColor Stops[] = {FLinearColor(0.16f, 0.42f, 0.20f), FLinearColor(0.56f, 0.66f, 0.30f),
			FLinearColor(0.86f, 0.78f, 0.42f), FLinearColor(0.66f, 0.42f, 0.20f), FLinearColor(0.52f, 0.47f, 0.45f),
			FLinearColor(0.96f, 0.96f, 0.97f)};
		const float Land = float(FMath::Clamp((Texel.Level - CoastHeight) / LandHeightRange, 0.0, 1.0));
		const float Scaled = Land * 5.0f;
		const int32 Index = FMath::Min(int32(Scaled), 4);
		FLinearColor Colour = Mix(Stops[Index], Stops[Index + 1], Scaled - float(Index));
		// A contour every tenth of the land's height range.
		const float Band = FMath::Frac(Land * 10.0f);
		if (Band < 0.07f || Band > 0.97f)
		{
			Colour *= 0.62f;
		}
		return Colour * Light;
	}

	/** The scanner's look (Rio 02.10, "anomalies"): a dark teal relief under which the sites and finds stand out. */
	FLinearColor ScanColour(const FTexel& Texel, const float Shade)
	{
		const float Land = float(FMath::Clamp((Texel.Level - CoastHeight) / LandHeightRange, 0.0, 1.0));
		const FLinearColor Ground = Mix(FLinearColor(0.015f, 0.07f, 0.08f), FLinearColor(0.08f, 0.30f, 0.30f), Land);
		return Mix(Ground, FLinearColor(0.005f, 0.02f, 0.035f), Texel.Liquid) * (0.45f + 0.9f * ShadeShare(Shade));
	}

	/** Colours a sampled window for the job's look; survey data only reads one texel per block and pales it. */
	void Colour(SAPSSurfaceMap::FJob& Job)
	{
		const SAPSSurfaceMap::FBake& Fields = *Job.Fields;
		const SAPSSurfaceMap::FSampler& Sampler = *Job.Sampler;
		const FStyle Style = Job.Style;
		const int32 W = Fields.Width;
		const int32 H = Fields.Height;
		Job.PixelWidth = Job.bDetail ? W : W + W / 2;
		Job.Pixels.SetNumUninitialized(Job.PixelWidth * H);
		const FAPSPlanetSurfacePalette& Palette = Sampler.Profile.Palette;
		const float EmissivePeak = FMath::Max3(Palette.Emissive.R, Palette.Emissive.G, Palette.Emissive.B);
		const FLinearColor LavaGlow = EmissivePeak > 0.01f ? Palette.Emissive / EmissivePeak : FLinearColor(1.0f, 0.32f, 0.06f);
		const FVector Light = FVector(-0.55, 0.55, 0.63).GetSafeNormal();
		ParallelFor(H, [&](const int32 Row)
		{
			FColor* Line = Job.Pixels.GetData() + Row * Job.PixelWidth;
			for (int32 Column = 0; Column < W; ++Column)
			{
				const int32 SourceRow = Style.bCoarse ? FMath::Min(Row / CoarseBlock * CoarseBlock + CoarseBlock / 2, H - 1) : Row;
				const int32 SourceColumn = Style.bCoarse ? FMath::Min(Column / CoarseBlock * CoarseBlock + CoarseBlock / 2, W - 1) : Column;
				const FTexel& Texel = Fields.Texels[SourceRow * W + SourceColumn];
				const FVector Normal = FVector(-Texel.Slope.X * Fields.Relief, -Texel.Slope.Y * Fields.Relief, 1.0).GetSafeNormal();
				const float Lit = float(FVector::DotProduct(Normal, Light));
				const float Shade = FMath::Clamp(0.36f + 0.84f * Lit, 0.42f, 1.16f);
				FLinearColor Final;
				switch (Style.Mode)
				{
				case SAPSSurfaceMap::EMode::Realistic:
					Final = RealisticColour(Texel, Sampler, Shade, LavaGlow);
					break;
				case SAPSSurfaceMap::EMode::Geology:
					Final = GeologyColour(Texel, Shade);
					break;
				case SAPSSurfaceMap::EMode::Scan:
					// Faint scan lines.
					Final = ScanColour(Texel, Shade) * (Row % 4 == 0 ? 1.3f : 1.0f);
					break;
				default:
					Final = TerrainColour(Texel, Sampler, Shade, LavaGlow);
					break;
				}
				if (Style.bCoarse)
				{
					const float Grey = Final.GetLuminance();
					Final = Mix(Final, FLinearColor(Grey, Grey * 1.02f, Grey * 1.06f), 0.65f) * 0.92f;
				}
				Final.A = 1.0f;
				Line[Column] = Final.ToFColor(true);
			}
			for (int32 Column = W; Column < Job.PixelWidth; ++Column)
			{
				Line[Column] = Line[Column - W];
			}
		});
	}

	void RunJob(SAPSSurfaceMap::FJob& Job)
	{
		if (!Job.Fields->bValid)
		{
			SampleWindow(*Job.Fields, *Job.Sampler, Job.bCancelled);
		}
		if (Job.Fields->bValid && !Job.bCancelled)
		{
			Colour(Job);
		}
		Job.bDone = true;
	}

	UTexture2D* MakeTexture(const TArray<FColor>& Pixels, const int32 Width, const int32 Height)
	{
		if (Width <= 0 || Height <= 0 || Pixels.Num() != Width * Height) return nullptr;
		// A name of its own: a transient texture of the same name would be recycled while the old one is still drawn.
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), TEXT("APSSurfaceMap"));
		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, Name);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty()) return nullptr;
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->NeverStream = true;
		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}

	double BodyRadiusCm(const APlanetaryBody* Body)
	{
		return APSPlanetSurfaceRadius::Kilometres(Body->RadiusKM, Body->PlanetRadiusKM) * 100000.0
			* FMath::Clamp(Body->WorldScapePresentationScale, 1.0e-9, 1.0);
	}

	const AStar* StarOf(const APlanetaryBody* Body)
	{
		const APlanet* Planet = Cast<APlanet>(Body);
		if (const AMoon* Moon = Cast<AMoon>(Body)) Planet = Moon->ParentPlanet;
		return Planet ? Planet->ParentStar : nullptr;
	}

	FString Coordinates(const FVector& Direction)
	{
		const double Latitude = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Direction.Z, -1.0, 1.0)));
		const double Longitude = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
		return FString::Printf(TEXT("%.1f° %s   %.1f° %s"), FMath::Abs(Latitude), Latitude >= 0.0 ? TEXT("N") : TEXT("S"),
			FMath::Abs(Longitude), Longitude >= 0.0 ? TEXT("E") : TEXT("W"));
	}
}

void SAPSSurfaceMap::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	bGlobeOnly = InArgs._GlobeOnly;
	OnOpenObject = InArgs._OnOpenObject;
	SetCanTick(true);
}

SAPSSurfaceMap::~SAPSSurfaceMap()
{
	// Workers still running finish on their own copies; they need not sample on.
	if (BaseJob.IsValid()) BaseJob->bCancelled = true;
	if (DetailJob.IsValid()) DetailJob->bCancelled = true;
}

APlanetaryBody* SAPSSurfaceMap::FindDefaultBody(UWorld* InWorld)
{
	const APlayerController* Controller = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn) return nullptr;
	APlanetaryBody* Best = nullptr;
	double BestSurface = TNumericLimits<double>::Max();
	for (TActorIterator<APlanetaryBody> It(InWorld); It; ++It)
	{
		if (!IsValid(*It)) continue;
		const double Surface = FVector::Distance(It->GetActorLocation(), Pawn->GetActorLocation()) - It->GetWorldScapeBodyRadiusCm();
		if (Surface < BestSurface)
		{
			BestSurface = Surface;
			Best = *It;
		}
	}
	return Best;
}

SAPSSurfaceMap::EKnown SAPSSurfaceMap::ReadKnown() const
{
	const APlanetaryBody* LiveBody = Body.Get();
	const FAPSFleetCommand* Fleet = LiveBody ? APSFleetFind(World.Get()) : nullptr;
	if (!Fleet)
	{
		return EKnown::Studied;
	}
	switch (Fleet->GetSurvey(LiveBody))
	{
	case APSFleet::ESurvey::Studied:
		return EKnown::Studied;
	case APSFleet::ESurvey::Surveyed:
		return EKnown::Surveyed;
	default:
		return EKnown::Unknown;
	}
}

void SAPSSurfaceMap::SetBody(APlanetaryBody* NewBody)
{
	using namespace APSSurfaceMapPrivate;
	if (NewBody == Body.Get() && (Sampler.IsValid() || bNoSurface)) return;
	Body = NewBody;
	MapTexture.Reset();
	MapBrush = FSlateBrush();
	Sampler.Reset();
	BaseFields.Reset();
	if (BaseJob.IsValid()) BaseJob->bCancelled = true;
	BaseJob.Reset();
	DropDetail();
	bNoSurface = false;
	BakeSummary = FText::GetEmpty();
	Markers.Reset();
	// A new world opens whole, its turn where the last one stood.
	Zoom = 1.0f;
	CentreLatitude = 0.35;
	Known = ReadKnown();
	if (!IsValid(NewBody))
	{
		BodyTitle = LOCTEXT("NoWorld", "NO WORLD");
		return;
	}
	const FString Designation = APSBodyDesignation::Of(NewBody);
	BodyTitle = FText::FromString(FString::Printf(TEXT("%s%s%s"), *Designation, Designation.IsEmpty() ? TEXT("") : TEXT("  "),
		*NewBody->AstroName.ToString().ToUpper()));
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(NewBody->PlanetType))
	{
		bNoSurface = true;
		RefreshMarkers();
		return;
	}
	// The same catalogue and seed rules as the streamed surface (APlanetarySurfaceGenerator::ApplySurfaceProfileNow).
	static const TCHAR* CatalogPath = TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog");
	const UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr, CatalogPath);
	const TSharedRef<FSampler, ESPMode::ThreadSafe> NewSampler = MakeShared<FSampler, ESPMode::ThreadSafe>();
	NewSampler->Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(NewBody, Catalog);
	int32 Seed = NewBody->WorldScapeSeed;
	if (Seed == 0)
	{
		const uint32 IdentityHash = HashCombine(GetTypeHash(NewBody->GetFName()), GetTypeHash(NewBody->GetActorLocation()));
		Seed = 10 + static_cast<int32>(IdentityHash % 999983u);
	}
	NewSampler->Noise.SetSeed(Seed == MAX_int32 ? Seed - 1 : Seed + 1);
	NewSampler->Noise.SetSeed(Seed);
	const double PresentationScale = FMath::Clamp(NewBody->WorldScapePresentationScale, 1.0e-9, 1.0);
	NewSampler->RadiusCm = FMath::Max(BodyRadiusCm(NewBody), 100000.0);
	NewSampler->NoiseScale = FMath::Max(1.0, double(FMath::RoundToInt(NewSampler->Profile.NoiseScale)));
	NewSampler->NoiseIntensity = FMath::Max(1.0, double(FMath::RoundToInt(NewSampler->Profile.NoiseIntensity * PresentationScale)));
	NewSampler->bLava = NewSampler->Profile.Archetype == EAPSPlanetSurfaceArchetype::Magmatic;
	NewSampler->Air = APSAtmosphereModel::Density(NewBody);
	Sampler = NewSampler;
	// Rio 02.10 ("only the scanned ones"): an unsurveyed world is not sampled until a survey reaches it.
	if (Known != EKnown::Unknown)
	{
		StartBaseJob();
	}
	RefreshMarkers();
}

void SAPSSurfaceMap::LaunchJob(TSharedPtr<FJob, ESPMode::ThreadSafe>& Slot, const TSharedPtr<FBake, ESPMode::ThreadSafe>& Fields,
	const bool bDetail)
{
	if (!Sampler.IsValid() || !Fields.IsValid()) return;
	if (Slot.IsValid()) Slot->bCancelled = true;
	const TSharedPtr<FJob, ESPMode::ThreadSafe> Job = MakeShared<FJob, ESPMode::ThreadSafe>();
	Job->Fields = Fields;
	Job->Sampler = Sampler;
	Job->Style.Mode = Mode;
	Job->Style.bCoarse = !bDetail && Known == EKnown::Surveyed;
	Job->bDetail = bDetail;
	Slot = Job;
	Async(EAsyncExecution::ThreadPool, [Job]()
	{
		APSSurfaceMapPrivate::RunJob(*Job);
	});
}

void SAPSSurfaceMap::StartBaseJob()
{
	using namespace APSSurfaceMapPrivate;
	TSharedPtr<FBake, ESPMode::ThreadSafe> Fields = BaseFields;
	if (!Fields.IsValid())
	{
		Fields = MakeShared<FBake, ESPMode::ThreadSafe>();
		Fields->Width = MapWidth;
		Fields->Height = MapHeight;
	}
	LaunchJob(BaseJob, Fields, false);
}

void SAPSSurfaceMap::DropDetail()
{
	if (DetailJob.IsValid()) DetailJob->bCancelled = true;
	DetailJob.Reset();
	DetailFields.Reset();
	DetailTexture.Reset();
	DetailBrush = FSlateBrush();
}

void SAPSSurfaceMap::UpdateDetail()
{
	using namespace APSSurfaceMapPrivate;
	// Only a studied world has more to show; survey data stays coarse.
	if (bGlobeOnly || Known != EKnown::Studied || !BaseFields.IsValid() || !Sampler.IsValid() || Zoom < DetailFromZoom)
	{
		if (DetailFields.IsValid() || DetailJob.IsValid()) DropDetail();
		return;
	}
	if (bDragging || FPlatformTime::Seconds() - LastViewChangeSeconds < DetailSettleSeconds) return;
	double U0 = 0.0;
	double V0 = 0.0;
	double Size = 1.0;
	MapWindow(U0, V0, Size);
	// The view's window lies inside a sampled one made for about this zoom.
	const auto Covers = [&](const FBake& Window, const float MadeFor)
	{
		if (FMath::Abs(FMath::Loge(Zoom / FMath::Max(MadeFor, 1.0f))) > FMath::Loge(1.3f)) return false;
		double Shift = U0 - Window.U0;
		Shift -= FMath::FloorToDouble(Shift);
		return Shift + Size <= Window.USize + 1.0e-6 && V0 >= Window.V0 - 1.0e-6 && V0 + Size <= Window.V0 + Window.VSize + 1.0e-6;
	};
	if (DetailFields.IsValid() && Covers(*DetailFields, DetailZoom)) return;
	if (DetailJob.IsValid() && Covers(*DetailJob->Fields, DetailJobZoom)) return;
	const double WindowSize = FMath::Min(Size * DetailMargin, 1.0);
	const TSharedPtr<FBake, ESPMode::ThreadSafe> Fields = MakeShared<FBake, ESPMode::ThreadSafe>();
	Fields->USize = WindowSize;
	Fields->VSize = WindowSize;
	Fields->U0 = U0 + (Size - WindowSize) * 0.5;
	Fields->V0 = FMath::Clamp(V0 + (Size - WindowSize) * 0.5, 0.0, 1.0 - WindowSize);
	Fields->Width = DetailWidth;
	Fields->Height = DetailHeight;
	LaunchJob(DetailJob, Fields, true);
	DetailJobZoom = Zoom;
}

void SAPSSurfaceMap::UploadJobs()
{
	using namespace APSSurfaceMapPrivate;
	if (BaseJob.IsValid() && BaseJob->bDone)
	{
		const TSharedPtr<FJob, ESPMode::ThreadSafe> Job = BaseJob;
		BaseJob.Reset();
		if (!Job->Fields->bValid)
		{
			BakeSummary = LOCTEXT("BakeFailed", "The surface could not be sampled.");
		}
		else
		{
			BaseFields = Job->Fields;
			if (UTexture2D* Texture = MakeTexture(Job->Pixels, Job->PixelWidth, Job->Fields->Height))
			{
				MapTexture.Reset(Texture);
				MapBrush = FSlateBrush();
				MapBrush.SetResourceObject(Texture);
				MapBrush.ImageSize = FVector2D(Job->PixelWidth, Job->Fields->Height);
				MapBrush.DrawAs = ESlateBrushDrawType::Image;
				MapTurnU = float(Job->Fields->Width) / float(Job->PixelWidth);
				PaintedMode = Job->Style.Mode;
				bPaintedCoarse = Job->Style.bCoarse;
			}
			BakeSummary = FText::Format(LOCTEXT("BakeSummary", "Liquid covers {0}% of the surface."),
				APSUINumber::Number(FMath::RoundToInt(BaseFields->WaterShare * 100.0f)));
			// The look or the survey level changed while it was made: colour it again.
			if (PaintedMode != Mode || bPaintedCoarse != (Known == EKnown::Surveyed))
			{
				StartBaseJob();
			}
		}
	}
	if (DetailJob.IsValid() && DetailJob->bDone)
	{
		const TSharedPtr<FJob, ESPMode::ThreadSafe> Job = DetailJob;
		DetailJob.Reset();
		// A stale window (the world, its survey or the zoom moved on) is dropped.
		if (Job->Fields->bValid && Known == EKnown::Studied && Zoom >= DetailFromZoom)
		{
			if (UTexture2D* Texture = MakeTexture(Job->Pixels, Job->PixelWidth, Job->Fields->Height))
			{
				DetailFields = Job->Fields;
				DetailZoom = DetailJobZoom;
				DetailTexture.Reset(Texture);
				DetailBrush = FSlateBrush();
				DetailBrush.SetResourceObject(Texture);
				DetailBrush.ImageSize = FVector2D(Job->PixelWidth, Job->Fields->Height);
				DetailBrush.DrawAs = ESlateBrushDrawType::Image;
				DetailPaintedMode = Job->Style.Mode;
				if (DetailPaintedMode != Mode)
				{
					LaunchJob(DetailJob, DetailFields, true);
					DetailJobZoom = DetailZoom;
				}
			}
		}
	}
}

void SAPSSurfaceMap::UpdateKnown()
{
	const EKnown Latest = ReadKnown();
	if (Latest == Known) return;
	Known = Latest;
	if (Latest != EKnown::Studied) DropDetail();
	if (!Sampler.IsValid() || Latest == EKnown::Unknown) return;
	// The first survey samples the world; a study colours the sampled one in full. A sampling job under way colours
	// again for the new level when it lands.
	if (BaseFields.IsValid() || !BaseJob.IsValid())
	{
		StartBaseJob();
	}
}

void SAPSSurfaceMap::RefreshMarkers()
{
	using namespace APSSurfaceMapPrivate;
	Markers.Reset();
	AnomalyNote = FText::GetEmpty();
	const APlanetaryBody* LiveBody = Body.Get();
	UWorld* LiveWorld = World.Get();
	if (!LiveBody || !LiveWorld) return;
	UpdateKnown();
	// Without a survey only what orbits the world is known, not what stands on it.
	const bool bSurfaceKnown = Known != EKnown::Unknown;
	const FVector Centre = LiveBody->GetActorLocation();
	const FQuat Frame = LiveBody->GetActorQuat();
	const double RadiusCm = FMath::Max(LiveBody->GetWorldScapeBodyRadiusCm(), 1.0);
	if (const AStar* Star = StarOf(LiveBody))
	{
		SunDirection = Frame.UnrotateVector(Star->GetActorLocation() - Centre).GetSafeNormal();
	}
	const auto Add = [&](const EMarker Kind, const FVector& WorldLocation, const FString& Label, const FLinearColor& Color,
		const double MaxRadii, const AActor* Actor)
	{
		const FVector Local = Frame.UnrotateVector(WorldLocation - Centre);
		const double Distance = Local.Size();
		if (Distance <= 1.0 || Distance > RadiusCm * MaxRadii) return;
		FMarker& Marker = Markers.AddDefaulted_GetRef();
		Marker.Kind = Kind;
		Marker.Actor = const_cast<AActor*>(Actor);
		Marker.Direction = Local / Distance;
		Marker.Label = Label;
		Marker.Color = Color;
		Marker.AltitudeKm = (Distance - RadiusCm) / 100000.0;
	};
	if (bSurfaceKnown)
	{
		for (TActorIterator<AActor> It(LiveWorld); It; ++It)
		{
			if (!IsValid(*It) || !It->ActorHasTag(TEXT("APS.Civilization.Materialized"))) continue;
			const UAPSCivilizationIdentityComponent* Identity = It->FindComponentByClass<UAPSCivilizationIdentityComponent>();
			if (Identity && Identity->Role == EAPSCivilizationEntityRole::BaseModule)
			{
				Add(EMarker::Colony, It->GetActorLocation(), TEXT("COLONY"), APSChrome::Amber(), 1.05, *It);
			}
		}
	}
	if (const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld))
	{
		for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
		{
			if (Record.Body.Get() != LiveBody || !bSurfaceKnown) continue;
			for (const TWeakObjectPtr<AActor>& Outpost : Record.Outposts)
			{
				if (Outpost.IsValid())
				{
					Add(EMarker::Outpost, Outpost->GetActorLocation(), FAPSFleetCommand::DisplayName(Outpost.Get()).ToString().ToUpper(),
						APSChrome::Cyan(), 1.5, Outpost.Get());
				}
			}
			// Rio 02.10 ("the anomalies look"): a located anomaly's site, studied ones dimmer; a detected one only says so.
			if (Record.bHasAnomaly && Record.Anomaly >= APSFleet::EAnomalyState::Located)
			{
				const bool bStudied = Record.Anomaly == APSFleet::EAnomalyState::Investigated;
				FString Name = APSFleet::AnomalyName(Record.AnomalyKind).ToString().ToUpper();
				if (bStudied) Name += TEXT("  (STUDIED)");
				Add(EMarker::Anomaly, Centre + Frame.RotateVector(Record.AnomalyDirection.GetSafeNormal()) * RadiusCm * 1.001, Name,
					bStudied ? FLinearColor(0.62f, 0.52f, 0.74f) : FLinearColor(0.92f, 0.45f, 1.0f), 1.05, LiveBody);
			}
			else if (Record.bHasAnomaly && Record.Anomaly == APSFleet::EAnomalyState::Detected)
			{
				AnomalyNote = LOCTEXT("AnomalyDetected", "An anomaly is detected on this world: a science ship's study locates its site.");
			}
		}
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			if (Structure.Body.Get() == LiveBody && Structure.Actor.IsValid())
			{
				Add(EMarker::Station, Structure.Actor->GetActorLocation(),
					FAPSFleetCommand::DisplayName(Structure.Actor.Get()).ToString().ToUpper(), FLinearColor(0.62f, 0.8f, 1.0f), 50.0,
					Structure.Actor.Get());
			}
		}
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (const ASpaceship* Ship = Unit.Ship.Get())
			{
				Add(EMarker::Ship, Ship->GetActorLocation(), Unit.CallSign.IsEmpty() ? Ship->GetName() : Unit.CallSign,
					APSFleet::DivisionColour(Unit.Division), 4.0, Ship);
			}
		}
	}
	// Rio 04.10 ("the new buildings show on the regular map, not on the surface map"): the infrastructure catalogue's
	// structures at this world too (raised by ships or by hand in build mode), the civilization's own, so known without a
	// survey; on the ground an outpost mark, in orbit a station mark, in their department's colour.
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld))
	{
		TArray<const FAPSBuiltStructure*> Here;
		Infrastructure->GetAt(LiveBody, Here);
		for (const FAPSBuiltStructure* Built : Here)
		{
			const AActor* Actor = Built ? Built->Actor.Get() : nullptr;
			if (!Actor) continue;
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type);
			const bool bOrbital = FVector::Dist(Actor->GetActorLocation(), Centre) > RadiusCm * 1.02;
			Add(bOrbital ? EMarker::Station : EMarker::Outpost, Actor->GetActorLocation(),
				Type ? Type->Name.ToString().ToUpper() : FAPSFleetCommand::DisplayName(Actor).ToString().ToUpper(),
				Type ? APSInfrastructure::DepartmentColour(Type->Department) : APSChrome::Cyan(), 50.0, Actor);
		}
	}
	const APlayerController* Controller = LiveWorld->GetFirstPlayerController();
	if (const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr)
	{
		Add(EMarker::Pilot, Pawn->GetActorLocation(), TEXT("YOU"), FLinearColor(1.0f, 0.86f, 0.42f), 6.0, Pawn);
	}
}

FText SAPSSurfaceMap::GetStatusText() const
{
	if (!Body.IsValid()) return LOCTEXT("StatusNone", "No world picked.");
	if (bNoSurface) return LOCTEXT("StatusGas", "A gas world: no solid surface to map. Markers show what orbits it.");
	if (Known == EKnown::Unknown)
	{
		return LOCTEXT("StatusUnknown", "NO SURVEY DATA. Send an exploration or science ship to survey this world (FLEET ORDERS); until then only what orbits it is shown.");
	}
	if (!BaseFields.IsValid()) return BakeSummary.IsEmpty() ? LOCTEXT("StatusSampling", "Sampling the surface...") : BakeSummary;
	if (Known == EKnown::Surveyed)
	{
		return FText::Format(LOCTEXT("StatusSurveyed", "SURVEY DATA: the surface in coarse blocks; a science ship's study maps it in full. {0}"),
			BakeSummary);
	}
	return BakeSummary;
}

FText SAPSSurfaceMap::GetPilotText() const
{
	using namespace APSSurfaceMapPrivate;
	for (const FMarker& Marker : Markers)
	{
		if (Marker.Kind == EMarker::Pilot)
		{
			return FText::FromString(FString::Printf(TEXT("%s   /   %s"), *Coordinates(Marker.Direction),
				Marker.AltitudeKm < 1.0 ? *FString::Printf(TEXT("%.0f m up"), FMath::Max(Marker.AltitudeKm * 1000.0, 0.0))
					: *FString::Printf(TEXT("%.1f km up"), Marker.AltitudeKm)));
		}
	}
	return LOCTEXT("PilotAway", "The pilot is away from this world.");
}

FText SAPSSurfaceMap::GetLegendText() const
{
	int32 Counts[6] = {0, 0, 0, 0, 0, 0};
	for (const FMarker& Marker : Markers) ++Counts[static_cast<int32>(Marker.Kind)];
	if (Known == EKnown::Unknown && !bNoSurface)
	{
		return FText::Format(LOCTEXT("LegendUnknown", "Only what orbits it is known: stations {0}   /   ships {1}"),
			APSUINumber::Number(Counts[2]), APSUINumber::Number(Counts[3]));
	}
	return FText::Format(LOCTEXT("LegendAnomalies", "Colony {0}   /   outposts {1}   /   stations {2}   /   ships {3}   /   anomalies {4}"),
		APSUINumber::Number(Counts[0]), APSUINumber::Number(Counts[1]), APSUINumber::Number(Counts[2]), APSUINumber::Number(Counts[3]),
		APSUINumber::Number(Counts[5]));
}

void SAPSSurfaceMap::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	using namespace APSSurfaceMapPrivate;
	UploadJobs();
	// Rio 04.10: what stands on and around the world changes while the map is open (a structure finished, a ship came).
	if (InCurrentTime >= NextMarkerRefreshSeconds)
	{
		NextMarkerRefreshSeconds = InCurrentTime + 1.0;
		RefreshMarkers();
	}
	// The world turns on its own only whole and untouched for a while (Rio 02.10).
	if (!bDragging && Zoom <= 1.0f && FPlatformTime::Seconds() - LastInputSeconds > IdleSeconds)
	{
		// The world turns east under a fixed viewer: the longitude at the centre decreases.
		CentreLongitude = FMath::UnwindRadians(CentreLongitude - InDeltaTime * 0.11);
	}
	UpdateDetail();
}

SAPSSurfaceMap::FLayout SAPSSurfaceMap::LayoutViews(const FVector2D& Size) const
{
	FLayout View;
	if (bGlobeOnly)
	{
		// The whole widget is the globe; the atmosphere rim keeps a few pixels.
		View.GlobeRadius = float(FMath::Max(FMath::Min(Size.X, Size.Y) * 0.5 - 9.0, 8.0));
		View.GlobeCentre = Size * 0.5;
		const FVector2D Half(View.GlobeRadius + 9.0, View.GlobeRadius + 9.0);
		View.GlobeBox = FBox2D(View.GlobeCentre - Half, View.GlobeCentre + Half);
		View.MapBox = FBox2D(Size * 0.5, Size * 0.5);
		return View;
	}
	// The globe takes a square on the left, the map the rest at 2:1.
	const double Gap = 24.0;
	const double GlobeSide = FMath::Min(Size.Y, Size.X * 0.36);
	View.GlobeRadius = float(GlobeSide * 0.5 - 10.0);
	View.GlobeCentre = FVector2D(GlobeSide * 0.5, Size.Y * 0.5);
	View.GlobeBox = FBox2D(View.GlobeCentre - FVector2D(GlobeSide * 0.5, GlobeSide * 0.5), View.GlobeCentre + FVector2D(GlobeSide * 0.5, GlobeSide * 0.5));
	const double MapLeft = GlobeSide + Gap;
	const double MapWidth = FMath::Max(Size.X - MapLeft, 40.0);
	const double MapHeight = FMath::Min(MapWidth * 0.5, Size.Y);
	const double MapTop = (Size.Y - MapHeight) * 0.5;
	View.MapBox = FBox2D(FVector2D(MapLeft, MapTop), FVector2D(MapLeft + MapHeight * 2.0, MapTop + MapHeight));
	return View;
}

void SAPSSurfaceMap::GlobeBasis(FVector& OutRight, FVector& OutUp, FVector& OutForward) const
{
	OutForward = FVector(FMath::Cos(CentreLatitude) * FMath::Cos(CentreLongitude), FMath::Cos(CentreLatitude) * FMath::Sin(CentreLongitude),
		FMath::Sin(CentreLatitude));
	OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal();
	OutUp = FVector::CrossProduct(OutForward, OutRight);
}

FVector2D SAPSSurfaceMap::MapUV(const FVector& Direction)
{
	return FVector2D(0.5 + FMath::Atan2(Direction.Y, Direction.X) / UE_TWO_PI,
		0.5 - FMath::Asin(FMath::Clamp(Direction.Z, -1.0, 1.0)) / UE_PI);
}

void SAPSSurfaceMap::MapWindow(double& OutU0, double& OutV0, double& OutSize) const
{
	// Whole at x1, west to east as charts are read; closer, a window around the shared centre.
	if (Zoom <= 1.0f)
	{
		OutU0 = 0.0;
		OutV0 = 0.0;
		OutSize = 1.0;
		return;
	}
	OutSize = 1.0 / Zoom;
	OutU0 = 0.5 + CentreLongitude / UE_TWO_PI - OutSize * 0.5;
	OutU0 -= FMath::FloorToDouble(OutU0);
	OutV0 = FMath::Clamp(0.5 - CentreLatitude / UE_PI - OutSize * 0.5, 0.0, 1.0 - OutSize);
}

bool SAPSSurfaceMap::MapPoint(const FVector2D& UV, const FBox2D& MapBox, FVector2D& OutPoint) const
{
	double U0 = 0.0;
	double V0 = 0.0;
	double Size = 1.0;
	MapWindow(U0, V0, Size);
	double U = UV.X - U0;
	U -= FMath::FloorToDouble(U);
	const double V = UV.Y - V0;
	OutPoint = MapBox.Min + FVector2D(U / Size, V / Size) * MapBox.GetSize();
	return U <= Size && V >= 0.0 && V <= Size;
}

bool SAPSSurfaceMap::GlobePoint(const FVector& Direction, const FLayout& View, const FVector& Right, const FVector& Up,
	const FVector& Forward, FVector2D& OutPoint) const
{
	OutPoint = View.GlobeCentre + FVector2D(FVector::DotProduct(Direction, Right), -FVector::DotProduct(Direction, Up)) * View.GlobeRadius * Zoom;
	return FVector::DotProduct(Direction, Forward) > 0.05 && View.GlobeBox.IsInside(OutPoint);
}

void SAPSSurfaceMap::PaintNoSurvey(const FGeometry& Geometry, FSlateWindowElementList& Elements, const int32 LayerId,
	const FVector2D& Centre, const float Radius, const FBox2D* Box) const
{
	// Rio 02.10 ("if not, add some cool darkening"): a dark field under a scanner's drifting lines and static.
	const double Seconds = FPlatformTime::Seconds();
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
	const FLinearColor Dark(0.008f, 0.022f, 0.030f, 0.97f);
	FVector2D Min = Centre - FVector2D(Radius, Radius);
	FVector2D Max = Centre + FVector2D(Radius, Radius);
	if (Box)
	{
		Min = Box->Min;
		Max = Box->Max;
		FSlateDrawElement::MakeBox(Elements, LayerId, Geometry.ToPaintGeometry(FVector2f(Box->GetSize()), FSlateLayoutTransform(FVector2f(Box->Min))),
			White, ESlateDrawEffect::None, Dark);
	}
	else
	{
		// A filled disc: a fan of untextured triangles.
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		const FColor Fill = Dark.ToFColor(true);
		constexpr int32 Steps = 72;
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(Centre), FVector2f(0.0f, 0.0f), Fill));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Steps;
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform,
				FVector2f(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius), FVector2f(0.0f, 0.0f), Fill));
			if (Step > 0)
			{
				Indices.Append({SlateIndex(0), SlateIndex(Step), SlateIndex(Step + 1)});
			}
		}
		FSlateDrawElement::MakeCustomVerts(Elements, LayerId, FSlateResourceHandle(), Vertices, Indices, nullptr, 0, 0);
	}
	// Where a horizontal line at this height crosses the field.
	const auto Span = [&](const double Y, double& OutLeft, double& OutRight)
	{
		if (Box)
		{
			OutLeft = Min.X;
			OutRight = Max.X;
			return Y >= Min.Y && Y <= Max.Y;
		}
		const double Offset = Y - Centre.Y;
		const double Half = FMath::Sqrt(FMath::Max(double(Radius) * Radius - Offset * Offset, 0.0));
		OutLeft = Centre.X - Half;
		OutRight = Centre.X + Half;
		return Half > 0.5;
	};
	const auto Scan = [&](const double Y, const FLinearColor& Color, const float Thickness)
	{
		double Left = 0.0;
		double Right = 0.0;
		if (Span(Y, Left, Right))
		{
			FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
				TArray<FVector2D>{FVector2D(Left, Y), FVector2D(Right, Y)}, ESlateDrawEffect::None, Color, true, Thickness);
		}
	};
	// Drifting scanlines, and a brighter sweep top to bottom every few seconds with a fading trail.
	constexpr double Spacing = 5.0;
	for (double Y = Min.Y + FMath::Fmod(Seconds * 12.0, Spacing); Y < Max.Y; Y += Spacing)
	{
		Scan(Y, FLinearColor(0.30f, 0.80f, 0.90f, 0.07f), 1.0f);
	}
	const double Sweep = FMath::Fmod(Seconds * 0.3, 1.0) * 1.3;
	if (Sweep < 1.0)
	{
		const double Y = FMath::Lerp(Min.Y, Max.Y, Sweep);
		Scan(Y, FLinearColor(0.40f, 0.90f, 1.0f, 0.30f), 2.0f);
		for (int32 Trail = 1; Trail <= 6; ++Trail)
		{
			Scan(Y - Trail * 3.0, FLinearColor(0.40f, 0.90f, 1.0f, 0.12f * (1.0f - Trail / 7.0f)), 1.0f);
		}
	}
	// Static: specks dealt anew ten times a second.
	FRandomStream Stream(static_cast<int32>(FMath::Fmod(Seconds * 10.0, 100000.0)) * 7919 + 13);
	const int32 Specks = Box ? 160 : 70;
	for (int32 Speck = 0; Speck < Specks; ++Speck)
	{
		const FVector2D At(FMath::Lerp(Min.X, Max.X, double(Stream.FRand())), FMath::Lerp(Min.Y, Max.Y, double(Stream.FRand())));
		const float Width = Stream.FRandRange(1.0f, 4.0f);
		const float Alpha = Stream.FRandRange(0.05f, 0.3f);
		double Left = 0.0;
		double Right = 0.0;
		if (!Span(At.Y, Left, Right) || At.X < Left || At.X + Width > Right) continue;
		FSlateDrawElement::MakeBox(Elements, LayerId + 1, Geometry.ToPaintGeometry(FVector2f(Width, 1.0f), FSlateLayoutTransform(FVector2f(At))),
			White, ESlateDrawEffect::None, FLinearColor(0.55f, 0.92f, 1.0f, Alpha));
	}
	// The verdict, pulsing.
	const bool bRoomy = Box || Radius > 80.0f;
	const FSlateFontInfo TitleFont = APSChrome::Font("Bold", Box ? 14 : (bRoomy ? 12 : 10));
	const FSlateFontInfo HintFont = APSChrome::Font("Bold", 9);
	const FText Title = LOCTEXT("NoSurveyTitle", "NO SURVEY DATA");
	const FText Hint = LOCTEXT("NoSurveyHint", "SEND AN EXPLORATION OR SCIENCE SHIP");
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Pulse = 0.7f + 0.3f * float(FMath::Sin(Seconds * 3.0));
	const FVector2D Middle = (Min + Max) * 0.5;
	const FVector2D TitleSize = Measure->Measure(Title, TitleFont);
	FLinearColor TitleColor = APSChrome::Amber();
	TitleColor.A = Pulse;
	// Rio 03.10: centred by its capitals, not by the line box (the display face holds them 0.11 em high).
	FSlateDrawElement::MakeText(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2f(TitleSize),
		FSlateLayoutTransform(FVector2f(Middle - TitleSize * 0.5 - FVector2D(0.0, (bRoomy ? 8.0 : 0.0) - APSChrome::CapsCenterOffset(TitleFont))))),
		Title, TitleFont, ESlateDrawEffect::None, TitleColor);
	if (bRoomy)
	{
		const FVector2D HintSize = Measure->Measure(Hint, HintFont);
		FSlateDrawElement::MakeText(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2f(HintSize),
			FSlateLayoutTransform(FVector2f(Middle + FVector2D(-HintSize.X * 0.5, TitleSize.Y * 0.5 - 4.0)))), Hint, HintFont,
			ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.7f, 0.8f, 0.86f, 0.85f)));
	}
}

int32 SAPSSurfaceMap::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	using namespace APSSurfaceMapPrivate;
	const FLayout View = LayoutViews(Geometry.GetLocalSize());
	const FVector2D GlobeCentre = View.GlobeCentre;
	const float GlobeRadius = View.GlobeRadius;
	const FBox2D MapBox = View.MapBox;
	// The globe enlarges about its centre; past x1 both views are clipped to their frames.
	const float ScreenRadius = GlobeRadius * Zoom;
	const bool bClip = Zoom > 1.0f;
	const int32 LayerSurface = LayerId + 1;
	const int32 LayerDetail = LayerId + 2;
	const int32 LayerGrid = LayerId + 3;
	const int32 LayerFrame = LayerId + 4;
	const int32 LayerMarker = LayerId + 5;
	const int32 LayerNote = LayerId + 8;
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo LabelFont = APSChrome::Font("Bold", 9);
	const FSlateFontInfo HintFont = APSChrome::Font("Regular", 10);
	const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector Right, Up, Forward;
	GlobeBasis(Right, Up, Forward);
	// Finer grid as the view closes in.
	const int32 GridStep = Zoom < 3.0f ? 30 : (Zoom < 7.0f ? 10 : 5);
	const FLinearColor GridColor(0.85f, 0.95f, 1.0f, 0.10f);
	const FLinearColor EquatorColor(0.85f, 0.95f, 1.0f, 0.22f);
	// The cap of the sphere that can show inside the globe's square.
	double Cap = UE_HALF_PI;
	if (bClip)
	{
		const double Reach = View.GlobeBox.GetExtent().X * 1.41421356 / FMath::Max(double(ScreenRadius), 1.0);
		if (Reach < 1.0)
		{
			Cap = FMath::Min(FMath::Asin(Reach) + 0.03, UE_HALF_PI);
		}
	}

	const auto Circle = [&](const FVector2D& Centre, const float Radius, const FLinearColor& Color, const float Thickness, const int32 Layer)
	{
		TArray<FVector2D> Points;
		const int32 Steps = FMath::Clamp(FMath::CeilToInt(Radius * 0.8f), 16, 120);
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Steps;
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	};
	const auto PushClip = [&](const FBox2D& Box)
	{
		Elements.PushClip(FSlateClippingZone(Geometry.ToPaintGeometry(FVector2f(Box.GetSize()), FSlateLayoutTransform(FVector2f(Box.Min)))));
	};
	const auto Line = [&](const FVector2D& From, const FVector2D& To, const FLinearColor& Color, const int32 Layer)
	{
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{From, To}, ESlateDrawEffect::None, Color, true, 1.0f);
	};
	// Rio 02.10 ("labels must not overlap"): a view's icons first, then its labels, the picked marker first, then the
	// pilot, the colony, anomalies, stations, outposts and ships, each in the first free spot around its icon.
	const int32 Picked = SelectedIndex();
	struct FLabelAsk
	{
		FVector2D At;
		const FMarker* Marker;
		int32 Rank;
	};
	const auto Rank = [Picked](const FMarker& Marker, const int32 Index)
	{
		if (Index == Picked) return 0;
		switch (Marker.Kind)
		{
		case EMarker::Pilot: return 1;
		case EMarker::Colony: return 2;
		case EMarker::Anomaly: return 3;
		case EMarker::Station: return 4;
		case EMarker::Outpost: return 5;
		default: return 6;
		}
	};
	const auto PlaceLabels = [&](TArray<FLabelAsk>& Asks, const FBox2D& Bounds)
	{
		Asks.StableSort([](const FLabelAsk& A, const FLabelAsk& B) { return A.Rank < B.Rank; });
		TArray<FBox2D> Taken;
		for (const FLabelAsk& Ask : Asks)
		{
			Taken.Add(FBox2D(Ask.At - FVector2D(6.0, 6.0), Ask.At + FVector2D(6.0, 6.0)));
		}
		int32 ShipLabels = 0;
		for (const FLabelAsk& Ask : Asks)
		{
			const FMarker& Marker = *Ask.Marker;
			if (Marker.Label.IsEmpty() || (Ask.Rank > 0 && Marker.Kind == EMarker::Ship && ShipLabels >= 6)) continue;
			const FVector2D TextSize = Measure->Measure(Marker.Label, LabelFont);
			const FVector2D Offsets[] = {FVector2D(11.0, -TextSize.Y - 3.0), FVector2D(11.0, 3.0),
				FVector2D(-TextSize.X - 11.0, -TextSize.Y - 3.0), FVector2D(-TextSize.X - 11.0, 3.0),
				FVector2D(-TextSize.X * 0.5, -TextSize.Y - 12.0), FVector2D(-TextSize.X * 0.5, 12.0)};
			for (const FVector2D& Offset : Offsets)
			{
				const FVector2D TextAt = Ask.At + Offset;
				const FBox2D Box(TextAt - FVector2D(4.0, 1.0), TextAt + TextSize + FVector2D(4.0, 1.0));
				if (Box.Min.X < Bounds.Min.X || Box.Min.Y < Bounds.Min.Y || Box.Max.X > Bounds.Max.X || Box.Max.Y > Bounds.Max.Y)
				{
					continue;
				}
				if (Taken.ContainsByPredicate([&Box](const FBox2D& Other) { return Other.Intersect(Box); }))
				{
					continue;
				}
				Taken.Add(Box);
				ShipLabels += Marker.Kind == EMarker::Ship ? 1 : 0;
				FSlateDrawElement::MakeBox(Elements, LayerMarker + 1, Geometry.ToPaintGeometry(FVector2f(Box.GetSize()),
					FSlateLayoutTransform(FVector2f(Box.Min))), White, ESlateDrawEffect::None,
					Ask.Rank == 0 ? APSUITheme::Retint(FLinearColor(0.03f, 0.10f, 0.12f, 0.9f)) : APSUITheme::Retint(FLinearColor(0.01f, 0.03f, 0.04f, 0.72f)));
				// The name on its plate by its capitals (Rio 03.10).
				FSlateDrawElement::MakeText(Elements, LayerMarker + 2, Geometry.ToPaintGeometry(FVector2f(TextSize),
					FSlateLayoutTransform(FVector2f(TextAt + FVector2D(0.0, APSChrome::CapsCenterOffset(LabelFont))))),
					Marker.Label, LabelFont, ESlateDrawEffect::None, Marker.Color);
				break;
			}
		}
	};
	const auto DrawMarker = [&](const FMarker& Marker, const FVector2D& At, const bool bPicked)
	{
		const int32 Layer = LayerMarker;
		if (bPicked)
		{
			Circle(At, 13.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.85f), 1.6f, Layer);
		}
		switch (Marker.Kind)
		{
		case EMarker::Anomaly:
		{
			// A star of three strokes in a ring; larger in the anomalies look.
			const double Size = Mode == EMode::Scan ? 8.0 : 6.0;
			Circle(At, float(Size + 2.0), Marker.Color, 1.4f, Layer);
			for (int32 Stroke = 0; Stroke < 3; ++Stroke)
			{
				const double Angle = UE_PI * Stroke / 3.0;
				const FVector2D Along(FMath::Cos(Angle) * Size, FMath::Sin(Angle) * Size);
				FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At - Along, At + Along},
					ESlateDrawEffect::None, Marker.Color, true, 1.6f);
			}
			break;
		}
		case EMarker::Colony:
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At + FVector2D(0, -7), At + FVector2D(7, 0),
				At + FVector2D(0, 7), At + FVector2D(-7, 0), At + FVector2D(0, -7)}, ESlateDrawEffect::None, Marker.Color, true, 2.0f);
			FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(4.0f, 4.0f), FSlateLayoutTransform(FVector2f(At - FVector2D(2, 2)))),
				White, ESlateDrawEffect::None, Marker.Color);
			break;
		case EMarker::Outpost:
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At + FVector2D(0, -6), At + FVector2D(5.5, 4),
				At + FVector2D(-5.5, 4), At + FVector2D(0, -6)}, ESlateDrawEffect::None, Marker.Color, true, 1.6f);
			break;
		case EMarker::Station:
			Circle(At, 6.0f, Marker.Color, 1.4f, Layer);
			Circle(At, 2.0f, Marker.Color, 1.4f, Layer);
			break;
		case EMarker::Ship:
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At + FVector2D(0, -4.5), At + FVector2D(4.5, 0),
				At + FVector2D(0, 4.5), At + FVector2D(-4.5, 0), At + FVector2D(0, -4.5)}, ESlateDrawEffect::None, Marker.Color, true, 1.3f);
			break;
		case EMarker::Pilot:
			Circle(At, 8.0f, Marker.Color, 1.8f, Layer);
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At + FVector2D(-13, 0), At + FVector2D(-5, 0)},
				ESlateDrawEffect::None, Marker.Color, true, 1.4f);
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), TArray<FVector2D>{At + FVector2D(5, 0), At + FVector2D(13, 0)},
				ESlateDrawEffect::None, Marker.Color, true, 1.4f);
			break;
		}
	};

	// Rio 02.10 ("only the scanned ones"): an unsurveyed world shows no surface.
	const bool bUnknown = Body.IsValid() && !bNoSurface && Known == EKnown::Unknown;
	const bool bTexture = !bUnknown && MapTexture.IsValid() && MapBrush.GetResourceObject() != nullptr;
	const FSlateResourceHandle Handle = bTexture ? FSlateApplication::Get().GetRenderer()->GetResourceHandle(MapBrush) : FSlateResourceHandle();
	const bool bSurface = bTexture && Handle.IsValid();
	// The finer window over the base, its left edge folded into the first turn.
	const bool bDetail = bSurface && !bGlobeOnly && Zoom >= DetailFromZoom && DetailFields.IsValid() && DetailTexture.IsValid()
		&& DetailBrush.GetResourceObject() != nullptr;
	const FSlateResourceHandle DetailHandle = bDetail ? FSlateApplication::Get().GetRenderer()->GetResourceHandle(DetailBrush) : FSlateResourceHandle();
	const bool bDetailShown = bDetail && DetailHandle.IsValid();
	double DetailU0 = 0.0;
	double DetailV0 = 0.0;
	double DetailUSize = 1.0;
	double DetailVSize = 1.0;
	if (bDetailShown)
	{
		DetailU0 = DetailFields->U0 - FMath::FloorToDouble(DetailFields->U0);
		DetailV0 = DetailFields->V0;
		DetailUSize = DetailFields->USize;
		DetailVSize = DetailFields->VSize;
	}

	// ---- The globe.
	if (bClip) PushClip(View.GlobeBox);
	// Atmosphere rim.
	for (int32 Ring = 0; Ring < 5; ++Ring)
	{
		Circle(GlobeCentre, ScreenRadius + 1.5f + Ring * 1.6f, FLinearColor(0.35f, 0.62f, 1.0f, 0.16f - Ring * 0.03f), 1.6f, LayerId);
	}
	if (bUnknown)
	{
		PaintNoSurvey(Geometry, Elements, LayerSurface, GlobeCentre, ScreenRadius, nullptr);
	}
	else if (bSurface)
	{
		// A polar grid over the visible cap (Rio 02.10: "keep it fine when zoomed"), each vertex looking up the unwrapped
		// map; the triangles inside the finer window are drawn again from it.
		constexpr int32 Rings = 32;
		constexpr int32 Segments = 72;
		struct FGridVertex
		{
			FVector2f Position;
			FVector2f UV;
			FColor Color;
		};
		TArray<FGridVertex> Grid;
		Grid.SetNumUninitialized((Rings + 1) * Segments);
		for (int32 Ring = 0; Ring <= Rings; ++Ring)
		{
			const double Theta = Cap * Ring / Rings;
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const double Phi = UE_TWO_PI * Segment / Segments;
				const FVector Facing(FMath::Sin(Theta) * FMath::Cos(Phi), FMath::Sin(Theta) * FMath::Sin(Phi), FMath::Cos(Theta));
				const FVector Direction = Right * Facing.X + Up * Facing.Y + Forward * Facing.Z;
				const double Day = FMath::Max(0.0, FVector::DotProduct(Direction, SunDirection));
				const float Light = float(FMath::Clamp((0.30 + 0.88 * Day) * (0.80 + 0.20 * Facing.Z), 0.0, 1.0));
				FGridVertex& Vertex = Grid[Ring * Segments + Segment];
				Vertex.Position = FVector2f(GlobeCentre + FVector2D(Facing.X, -Facing.Y) * ScreenRadius);
				Vertex.UV = FVector2f(MapUV(Direction));
				Vertex.Color = FLinearColor(Light, Light, Light, 1.0f).ToFColor(true);
			}
		}
		GlobeVertices.Reset();
		GlobeIndices.Reset();
		DetailVertices.Reset();
		DetailIndices.Reset();
		const auto Triangle = [&](const int32 A, const int32 B, const int32 C)
		{
			const FGridVertex* Corners[3] = {&Grid[A], &Grid[B], &Grid[C]};
			float U[3] = {Corners[0]->UV.X, Corners[1]->UV.X, Corners[2]->UV.X};
			if (FMath::Max3(U[0], U[1], U[2]) - FMath::Min3(U[0], U[1], U[2]) > 0.5f)
			{
				for (float& Value : U) Value += Value < 0.5f ? 1.0f : 0.0f;
			}
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				GlobeIndices.Add(GlobeVertices.Num());
				GlobeVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Corners[Corner]->Position,
					FVector2f(U[Corner] * MapTurnU, Corners[Corner]->UV.Y), Corners[Corner]->Color));
			}
			if (!bDetailShown) return;
			// The window starts within the first turn and may run into the second.
			for (int32 Turn = 0; Turn < 2; ++Turn)
			{
				FVector2f Inside[3];
				bool bInside = true;
				for (int32 Corner = 0; Corner < 3 && bInside; ++Corner)
				{
					const double DetailU = (U[Corner] + Turn - DetailU0) / DetailUSize;
					const double DetailV = (Corners[Corner]->UV.Y - DetailV0) / DetailVSize;
					bInside = DetailU >= 0.0 && DetailU <= 1.0 && DetailV >= 0.0 && DetailV <= 1.0;
					Inside[Corner] = FVector2f(float(DetailU), float(DetailV));
				}
				if (!bInside) continue;
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					DetailIndices.Add(DetailVertices.Num());
					DetailVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Corners[Corner]->Position, Inside[Corner],
						Corners[Corner]->Color));
				}
				break;
			}
		};
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const int32 Next = (Segment + 1) % Segments;
			Triangle(0, Segments + Segment, Segments + Next);
			for (int32 Ring = 1; Ring < Rings; ++Ring)
			{
				const int32 A = Ring * Segments + Segment;
				const int32 B = Ring * Segments + Next;
				const int32 C = (Ring + 1) * Segments + Next;
				const int32 D = (Ring + 1) * Segments + Segment;
				Triangle(A, B, C);
				Triangle(A, C, D);
			}
		}
		FSlateDrawElement::MakeCustomVerts(Elements, LayerSurface, Handle, GlobeVertices, GlobeIndices, nullptr, 0, 0);
		if (!DetailVertices.IsEmpty())
		{
			FSlateDrawElement::MakeCustomVerts(Elements, LayerDetail, DetailHandle, DetailVertices, DetailIndices, nullptr, 0, 0);
		}
	}
	else
	{
		// No surface yet (sampling) or none at all (gas world): a plain disc.
		for (int32 Ring = 1; Ring <= 12; ++Ring)
		{
			Circle(GlobeCentre, ScreenRadius * Ring / 12.0f, APSUITheme::Retint(FLinearColor(0.2f, 0.5f, 0.6f, 0.05f + (Ring == 12 ? 0.25f : 0.0f))), 1.0f, LayerSurface);
		}
	}
	// The visible runs of the parallels and meridians, sampled over the visible cap only.
	{
		const auto GlobeLine = [&](const TFunctionRef<FVector(double)> Curve, const double From, const double To, const FLinearColor& Color)
		{
			TArray<FVector2D> Run;
			constexpr int32 Steps = 96;
			const auto Flush = [&]()
			{
				if (Run.Num() > 1)
				{
					FSlateDrawElement::MakeLines(Elements, LayerGrid, Geometry.ToPaintGeometry(), Run, ESlateDrawEffect::None, Color, true, 1.0f);
				}
				Run.Reset();
			};
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const FVector Direction = Curve(FMath::Lerp(From, To, double(Step) / Steps));
				if (FVector::DotProduct(Direction, Forward) > 0.02)
				{
					Run.Add(GlobeCentre + FVector2D(FVector::DotProduct(Direction, Right), -FVector::DotProduct(Direction, Up)) * ScreenRadius);
					continue;
				}
				Flush();
			}
			Flush();
		};
		const double Reach = FMath::Min(Cap + 0.05, UE_PI);
		for (int32 Latitude = -90 + GridStep; Latitude < 90; Latitude += GridStep)
		{
			const double Phi = FMath::DegreesToRadians(double(Latitude));
			if (FMath::Abs(Phi - CentreLatitude) > Reach) continue;
			// The longitudes of this parallel within the cap around the centre.
			double HalfWidth = UE_PI;
			const double Across = FMath::Cos(Phi) * FMath::Cos(CentreLatitude);
			if (Across > 1.0e-6)
			{
				const double CosHalf = (FMath::Cos(Reach) - FMath::Sin(Phi) * FMath::Sin(CentreLatitude)) / Across;
				if (CosHalf >= 1.0) continue;
				HalfWidth = CosHalf <= -1.0 ? UE_PI : FMath::Acos(CosHalf);
			}
			GlobeLine([Phi](const double Lambda)
			{
				return FVector(FMath::Cos(Phi) * FMath::Cos(Lambda), FMath::Cos(Phi) * FMath::Sin(Lambda), FMath::Sin(Phi));
			}, CentreLongitude - HalfWidth, CentreLongitude + HalfWidth, Latitude == 0 ? EquatorColor : GridColor);
		}
		for (int32 Longitude = -180; Longitude < 180; Longitude += GridStep)
		{
			const double Lambda = FMath::DegreesToRadians(double(Longitude));
			// A meridian's plane farther from the centre than the cap cannot show.
			if (FMath::Abs(FVector::DotProduct(FVector(-FMath::Sin(Lambda), FMath::Cos(Lambda), 0.0), Forward)) > FMath::Sin(FMath::Min(Reach, UE_HALF_PI)))
			{
				continue;
			}
			GlobeLine([Lambda](const double Phi)
			{
				return FVector(FMath::Cos(Phi) * FMath::Cos(Lambda), FMath::Cos(Phi) * FMath::Sin(Lambda), FMath::Sin(Phi));
			}, FMath::Max(-UE_HALF_PI, CentreLatitude - Reach), FMath::Min(UE_HALF_PI, CentreLatitude + Reach), GridColor);
		}
	}
	Circle(GlobeCentre, ScreenRadius, APSUITheme::RetintHighlight(FLinearColor(0.45f, 0.75f, 0.9f, 0.55f)), 1.2f, LayerFrame);
	{
		TArray<FLabelAsk> Asks;
		for (int32 Index = 0; Index < Markers.Num(); ++Index)
		{
			FVector2D At;
			if (GlobePoint(Markers[Index].Direction, View, Right, Up, Forward, At))
			{
				DrawMarker(Markers[Index], At, Index == Picked);
				if (!bGlobeOnly)
				{
					Asks.Add({At, &Markers[Index], Rank(Markers[Index], Index)});
				}
			}
		}
		// Unclipped, the globe's labels may use the whole left part of the widget.
		PlaceLabels(Asks, bClip ? View.GlobeBox
			: FBox2D(FVector2D::ZeroVector, FVector2D(MapBox.Min.X - 6.0, Geometry.GetLocalSize().Y)));
	}
	if (bClip) Elements.PopClip();
	if (bGlobeOnly)
	{
		return LayerId + 9;
	}

	// ---- The unwrapped map.
	double U0 = 0.0;
	double V0 = 0.0;
	double WindowSize = 1.0;
	MapWindow(U0, V0, WindowSize);
	const FVector2D MapSize = MapBox.GetSize();
	if (bClip) PushClip(MapBox);
	if (bUnknown)
	{
		PaintNoSurvey(Geometry, Elements, LayerSurface, FVector2D::ZeroVector, 0.0f, &MapBox);
	}
	else if (bSurface)
	{
		const auto Quad = [&](const FSlateResourceHandle& Resource, const FVector2D& Min, const FVector2D& Max, const FVector2f& UVMin,
			const FVector2f& UVMax, const int32 Layer)
		{
			TArray<FSlateVertex> QuadVertices;
			const TArray<SlateIndex> QuadIndices = {0, 1, 2, 0, 2, 3};
			const FColor Plain = FColor::White;
			QuadVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(Min), UVMin, Plain));
			QuadVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(float(Max.X), float(Min.Y)),
				FVector2f(UVMax.X, UVMin.Y), Plain));
			QuadVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(Max), UVMax, Plain));
			QuadVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(float(Min.X), float(Max.Y)),
				FVector2f(UVMin.X, UVMax.Y), Plain));
			FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, QuadVertices, QuadIndices, nullptr, 0, 0);
		};
		// The window may run across the date line: two pieces then.
		const double FirstEnd = FMath::Min(U0 + WindowSize, 1.0);
		const double SplitX = MapBox.Min.X + (FirstEnd - U0) / WindowSize * MapSize.X;
		Quad(Handle, MapBox.Min, FVector2D(SplitX, MapBox.Max.Y), FVector2f(float(U0 * MapTurnU), float(V0)),
			FVector2f(float(FirstEnd * MapTurnU), float(V0 + WindowSize)), LayerSurface);
		if (U0 + WindowSize > 1.0 + 1.0e-9)
		{
			Quad(Handle, FVector2D(SplitX, MapBox.Min.Y), MapBox.Max, FVector2f(0.0f, float(V0)),
				FVector2f(float((U0 + WindowSize - 1.0) * MapTurnU), float(V0 + WindowSize)), LayerSurface);
		}
		if (bDetailShown)
		{
			for (int32 Turn = -1; Turn <= 1; ++Turn)
			{
				const double Left = DetailU0 + Turn;
				if (Left + DetailUSize <= U0 || Left >= U0 + WindowSize) continue;
				Quad(DetailHandle,
					MapBox.Min + FVector2D((Left - U0) / WindowSize, (DetailV0 - V0) / WindowSize) * MapSize,
					MapBox.Min + FVector2D((Left + DetailUSize - U0) / WindowSize, (DetailV0 + DetailVSize - V0) / WindowSize) * MapSize,
					FVector2f(0.0f, 0.0f), FVector2f(1.0f, 1.0f), LayerDetail);
			}
		}
	}
	else
	{
		FSlateDrawElement::MakeBox(Elements, LayerSurface, Geometry.ToPaintGeometry(FVector2f(MapSize), FSlateLayoutTransform(FVector2f(MapBox.Min))),
			White, ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.02f, 0.05f, 0.07f, 0.9f)));
		FSlateDrawElement::MakeText(Elements, LayerDetail, Geometry.ToPaintGeometry(FVector2f(MapSize.X - 24.0f, 20.0f),
			FSlateLayoutTransform(FVector2f(MapBox.Min + FVector2D(12.0, MapSize.Y * 0.5 - 10.0)))), GetStatusText(), HintFont,
			ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.7f, 0.8f, 0.86f, 0.9f)));
	}
	// Graticule, finer as the view closes in; the equator brighter.
	for (int32 Latitude = -90 + GridStep; Latitude < 90; Latitude += GridStep)
	{
		const double V = 0.5 - Latitude / 180.0 - V0;
		if (V < 0.0 || V > WindowSize) continue;
		const double Y = MapBox.Min.Y + V / WindowSize * MapSize.Y;
		Line(FVector2D(MapBox.Min.X, Y), FVector2D(MapBox.Max.X, Y), Latitude == 0 ? EquatorColor : GridColor, LayerGrid);
	}
	for (int32 Longitude = -180; Longitude < 180; Longitude += GridStep)
	{
		double U = 0.5 + Longitude / 360.0 - U0;
		U -= FMath::FloorToDouble(U);
		if (U > WindowSize) continue;
		const double X = MapBox.Min.X + U / WindowSize * MapSize.X;
		Line(FVector2D(X, MapBox.Min.Y), FVector2D(X, MapBox.Max.Y), GridColor, LayerGrid);
	}
	// The point under the star.
	FVector2D Sun;
	if (bSurface && MapPoint(MapUV(SunDirection), MapBox, Sun))
	{
		Circle(Sun, 5.0f, FLinearColor(1.0f, 0.9f, 0.55f, 0.8f), 1.4f, LayerFrame);
		for (int32 Ray = 0; Ray < 8; ++Ray)
		{
			const double Angle = UE_TWO_PI * Ray / 8.0;
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			FSlateDrawElement::MakeLines(Elements, LayerFrame, Geometry.ToPaintGeometry(), TArray<FVector2D>{Sun + Direction * 7.0, Sun + Direction * 10.0},
				ESlateDrawEffect::None, FLinearColor(1.0f, 0.9f, 0.55f, 0.8f), true, 1.2f);
		}
	}
	{
		TArray<FLabelAsk> Asks;
		for (int32 Index = 0; Index < Markers.Num(); ++Index)
		{
			FVector2D At;
			if (MapPoint(MapUV(Markers[Index].Direction), MapBox, At))
			{
				DrawMarker(Markers[Index], At, Index == Picked);
				Asks.Add({At, &Markers[Index], Rank(Markers[Index], Index)});
			}
		}
		PlaceLabels(Asks, MapBox);
	}
	if (bClip) Elements.PopClip();
	FSlateDrawElement::MakeLines(Elements, LayerFrame, Geometry.ToPaintGeometry(),
		TArray<FVector2D>{MapBox.Min, FVector2D(MapBox.Max.X, MapBox.Min.Y), MapBox.Max, FVector2D(MapBox.Min.X, MapBox.Max.Y), MapBox.Min},
		ESlateDrawEffect::None, APSUITheme::RetintHighlight(FLinearColor(0.45f, 0.75f, 0.9f, 0.55f)), true, 1.2f);
	// How close the view is, and whether finer data is on its way.
	if (bClip && !bUnknown)
	{
		FNumberFormattingOptions OneDigit;
		OneDigit.SetMaximumFractionalDigits(1);
		const FText Note = FText::Format(LOCTEXT("ZoomNote", "x{0}{1}"), APSUINumber::Number(Zoom, &OneDigit),
			DetailJob.IsValid() ? LOCTEXT("ZoomSampling", "   SAMPLING DETAIL") : bDetailShown ? LOCTEXT("ZoomDetail", "   DETAIL") : FText::GetEmpty());
		const FVector2D NoteSize = Measure->Measure(Note, LabelFont);
		const FVector2D NoteAt(MapBox.Max.X - NoteSize.X - 10.0, MapBox.Min.Y + 7.0);
		FSlateDrawElement::MakeBox(Elements, LayerNote, Geometry.ToPaintGeometry(FVector2f(NoteSize.X + 8.0f, NoteSize.Y + 2.0f),
			FSlateLayoutTransform(FVector2f(NoteAt - FVector2D(4.0, 1.0)))), White, ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.01f, 0.03f, 0.04f, 0.78f)));
		FSlateDrawElement::MakeText(Elements, LayerNote + 1, Geometry.ToPaintGeometry(FVector2f(NoteSize),
			FSlateLayoutTransform(FVector2f(NoteAt + FVector2D(0.0, APSChrome::CapsCenterOffset(LabelFont))))),
			Note, LabelFont, ESlateDrawEffect::None, APSChrome::Cyan());
	}
	FSlateDrawElement::MakeText(Elements, LayerNote, Geometry.ToPaintGeometry(FVector2f(GlobeRadius * 2.0f + 20.0f, 16.0f),
		FSlateLayoutTransform(FVector2f(GlobeCentre + FVector2D(-GlobeRadius, GlobeRadius + 6.0)))),
		LOCTEXT("GlobeHint", "Drag to turn, wheel to zoom"), HintFont, ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.6f, 0.7f, 0.76f, 0.7f)));
	return LayerId + 10;
}

FReply SAPSSurfaceMap::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	const FLayout View = LayoutViews(Geometry.GetLocalSize());
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	bDragging = true;
	bPressMoved = false;
	DragView = !bGlobeOnly && View.MapBox.IsInside(Local) ? 1 : 0;
	DragLast = Event.GetScreenSpacePosition();
	PressPosition = DragLast;
	LastInputSeconds = FPlatformTime::Seconds();
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAPSSurfaceMap::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bDragging) return FReply::Unhandled();
	bDragging = false;
	LastInputSeconds = FPlatformTime::Seconds();
	if (!bPressMoved)
	{
		// A click (Rio 02.10: "objects on the map can be clicked") picks the marker under it, or clears the pick.
		const int32 Index = PickMarker(Geometry, Event.GetScreenSpacePosition());
		SelectedActor = Markers.IsValidIndex(Index) ? Markers[Index].Actor : TWeakObjectPtr<AActor>();
		SelectedLabel = Markers.IsValidIndex(Index) ? Markers[Index].Label : FString();
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAPSSurfaceMap::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	const int32 Index = PickMarker(Geometry, Event.GetScreenSpacePosition());
	if (!Markers.IsValidIndex(Index) || !Markers[Index].Actor.IsValid()) return FReply::Unhandled();
	SelectedActor = Markers[Index].Actor;
	SelectedLabel = Markers[Index].Label;
	OnOpenObject.ExecuteIfBound(Markers[Index].Actor.Get());
	return FReply::Handled();
}

int32 SAPSSurfaceMap::PickMarker(const FGeometry& Geometry, const FVector2D& ScreenPosition) const
{
	const FLayout View = LayoutViews(Geometry.GetLocalSize());
	const FVector2D Local = Geometry.AbsoluteToLocal(ScreenPosition);
	FVector Right, Up, Forward;
	GlobeBasis(Right, Up, Forward);
	int32 Best = INDEX_NONE;
	double BestDistance = 12.0;
	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		FVector2D At;
		if (GlobePoint(Markers[Index].Direction, View, Right, Up, Forward, At) && FVector2D::Distance(At, Local) < BestDistance)
		{
			BestDistance = FVector2D::Distance(At, Local);
			Best = Index;
		}
		if (!bGlobeOnly && MapPoint(MapUV(Markers[Index].Direction), View.MapBox, At) && FVector2D::Distance(At, Local) < BestDistance)
		{
			BestDistance = FVector2D::Distance(At, Local);
			Best = Index;
		}
	}
	return Best;
}

int32 SAPSSurfaceMap::SelectedIndex() const
{
	if (!SelectedActor.IsValid() && SelectedLabel.IsEmpty()) return INDEX_NONE;
	for (int32 Index = 0; Index < Markers.Num(); ++Index)
	{
		if (Markers[Index].Label == SelectedLabel && Markers[Index].Actor == SelectedActor)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void SAPSSurfaceMap::SetMode(const EMode NewMode)
{
	if (NewMode == Mode) return;
	Mode = NewMode;
	// The sampled fields stay: only the colours are made again (a job under way colours again when it lands).
	if (BaseFields.IsValid() && !BaseJob.IsValid())
	{
		StartBaseJob();
	}
	if (DetailFields.IsValid() && !DetailJob.IsValid())
	{
		LaunchJob(DetailJob, DetailFields, true);
		DetailJobZoom = DetailZoom;
	}
	RefreshMarkers();
}

FText SAPSSurfaceMap::ModeName(const EMode InMode)
{
	switch (InMode)
	{
	case EMode::Realistic: return LOCTEXT("ModeRealistic", "REALISTIC");
	case EMode::Geology: return LOCTEXT("ModeGeology", "GEOLOGY");
	case EMode::Scan: return LOCTEXT("ModeScan", "ANOMALIES");
	default: return LOCTEXT("ModeTerrain", "AS IS");
	}
}

FText SAPSSurfaceMap::GetSelectionText() const
{
	using namespace APSSurfaceMapPrivate;
	const int32 Index = SelectedIndex();
	if (!Markers.IsValidIndex(Index))
	{
		return AnomalyNote.IsEmpty() ? LOCTEXT("PickHint", "Click a marker to pick it; a double click opens its page.") : AnomalyNote;
	}
	const FMarker& Marker = Markers[Index];
	const FString Height = Marker.AltitudeKm < 1.0
		? FString::Printf(TEXT("%.0f m up"), FMath::Max(Marker.AltitudeKm * 1000.0, 0.0))
		: FString::Printf(TEXT("%.1f km up"), Marker.AltitudeKm);
	return FText::FromString(FString::Printf(TEXT("%s\n%s   /   %s%s"), *Marker.Label, *Coordinates(Marker.Direction), *Height,
		Marker.Actor.IsValid() ? TEXT("\nDouble click: open its page") : TEXT("")));
}

FReply SAPSSurfaceMap::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	using namespace APSSurfaceMapPrivate;
	if (!bDragging) return FReply::Unhandled();
	const FVector2D Position = Event.GetScreenSpacePosition();
	if (!bPressMoved && FVector2D::Distance(Position, PressPosition) < ClickSlop) return FReply::Handled();
	bPressMoved = true;
	// Local units: the surface follows the cursor on either view.
	const FVector2D Delta = (Position - DragLast) / FMath::Max(double(Geometry.Scale), 0.01);
	DragLast = Position;
	const FLayout View = LayoutViews(Geometry.GetLocalSize());
	if (DragView == 1)
	{
		double U0 = 0.0;
		double V0 = 0.0;
		double Size = 1.0;
		MapWindow(U0, V0, Size);
		const FVector2D MapSize = View.MapBox.GetSize();
		CentreLongitude -= Delta.X / FMath::Max(MapSize.X, 1.0) * Size * UE_TWO_PI;
		CentreLatitude += Delta.Y / FMath::Max(MapSize.Y, 1.0) * Size * UE_PI;
	}
	else
	{
		const double PixelsPerRadian = FMath::Max(double(View.GlobeRadius) * Zoom, 1.0);
		CentreLongitude -= Delta.X / (PixelsPerRadian * FMath::Max(FMath::Cos(CentreLatitude), 0.25));
		CentreLatitude += Delta.Y / PixelsPerRadian;
	}
	CentreLongitude = FMath::UnwindRadians(CentreLongitude);
	CentreLatitude = FMath::Clamp(CentreLatitude, -MaxCentreLatitude, MaxCentreLatitude);
	LastInputSeconds = FPlatformTime::Seconds();
	LastViewChangeSeconds = LastInputSeconds;
	return FReply::Handled();
}

FReply SAPSSurfaceMap::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	using namespace APSSurfaceMapPrivate;
	// The order panel's globe leaves the wheel to its scroll box; nothing to come closer to without a surface.
	if (bGlobeOnly || !Body.IsValid() || bNoSurface || Known == EKnown::Unknown) return FReply::Unhandled();
	float NewZoom = FMath::Clamp(Zoom * FMath::Pow(ZoomStep, Event.GetWheelDelta()), 1.0f, MaxZoom);
	if (NewZoom < 1.01f) NewZoom = 1.0f;
	if (FMath::IsNearlyEqual(NewZoom, Zoom)) return FReply::Handled();
	const FLayout View = LayoutViews(Geometry.GetLocalSize());
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	if (NewZoom > 1.0f && View.MapBox.IsInside(Local))
	{
		// Over the map the point under the cursor stays where it is; the globe turns to the new centre.
		double U0 = 0.0;
		double V0 = 0.0;
		double Size = 1.0;
		MapWindow(U0, V0, Size);
		const FVector2D Fraction = (Local - View.MapBox.Min) / View.MapBox.GetSize();
		const double NewSize = 1.0 / NewZoom;
		const double CentreU = U0 + Fraction.X * Size - (Fraction.X - 0.5) * NewSize;
		const double CentreV = V0 + Fraction.Y * Size - (Fraction.Y - 0.5) * NewSize;
		CentreLongitude = FMath::UnwindRadians((CentreU - 0.5) * UE_TWO_PI);
		CentreLatitude = FMath::Clamp((0.5 - CentreV) * UE_PI, -MaxCentreLatitude, MaxCentreLatitude);
	}
	Zoom = NewZoom;
	LastInputSeconds = FPlatformTime::Seconds();
	LastViewChangeSeconds = LastInputSeconds;
	return FReply::Handled();
}

FCursorReply SAPSSurfaceMap::OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const
{
	return FCursorReply::Cursor(bDragging ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
}

#undef LOCTEXT_NAMESPACE
