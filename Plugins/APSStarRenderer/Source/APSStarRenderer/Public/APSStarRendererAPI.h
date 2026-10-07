// Rio 03.10 (galaxy phase 3, "GPU points and the glow"): plain C++ API of the GPU star renderer.
// No UObjects. Every call is safe from any thread (work is marshalled to the game thread, then to
// the render thread). Design and integration: Docs/Design/GALAXY_GPU_STARS.md.
#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace APSStarRenderer
{
	/**
	 * One star on the GPU path, 8 bytes. The CPU catalogue stays the only source of truth: the caller
	 * resolves catalogue records (APSGalaxyCatalogBatch::ResolveStars) and packs them, so GPU, picking,
	 * saves and StableIds agree by construction. Position: 16 bits per axis inside the set's LocalBounds.
	 */
	struct FPackedStar
	{
		/** Quantised X (bits 0..15) and Y (bits 16..31). */
		uint32 XY = 0;
		/** Quantised Z (bits 0..15), palette colour index (16..23), intensity code (24..31). */
		uint32 ZColorIntensity = 0;
	};
	static_assert(sizeof(FPackedStar) == 8, "FPackedStar is the GPU stride (8 bytes)");

	/** Handle of a point set or glow volume; 0 is invalid. */
	using FHandle = uint32;

	/** Hard cap per point set (512 MB GPU buffer); split larger catalogues into several sets. */
	inline constexpr int32 MaxPointsPerSet = 64 * 1024 * 1024;

	/** Exclusion spheres one point set can use (ExclusionCenterLocal/RadiusLocal plus ExtraExclusionSpheresLocal). */
	inline constexpr int32 MaxExclusionSpheres = 8;

	/** Intensity code: I = 2^((Code - 128) / 8), 2^-16 .. 2^15.9 in ~9% steps; 0 = dark. */
	APSSTARRENDERER_API uint8 EncodeIntensity(float Intensity);
	APSSTARRENDERER_API float DecodeIntensity(uint8 Code);

	/** Packs one star; the position is clamped into Bounds (local units of the set). */
	APSSTARRENDERER_API FPackedStar PackStar(const FVector3f& LocalPosition, const FBox3f& Bounds,
		uint8 ColorIndex, float Intensity);
	/** Decoded position (debug and pick-tolerance checks). */
	APSSTARRENDERER_API FVector3f UnpackStarPosition(const FPackedStar& Star, const FBox3f& Bounds);

	/**
	 * APOSFERA's presentation compresses distances around the observer and keeps every direction
	 * exact (FAPSContinuousPreviewFrame::ProjectSphere, gameplay stellar projection):
	 *   P = LocalToWorld(local);  render = ObserverRender + (P - ObserverPhysical) * S / (1 + S * |P - ObserverPhysical| / Far)
	 * Disabled: render = LocalToWorld(local). Brightness always follows the uncompressed distance.
	 */
	struct FFarEnvelope
	{
		bool bEnabled = false;
		/** Observer in LocalToWorld's output space (e.g. canonical root cm). */
		FVector ObserverPhysical = FVector::ZeroVector;
		/** World position the observer is rendered at (the preview camera). */
		FVector ObserverRender = FVector::ZeroVector;
		/** S: render cm per physical cm at the observer (RenderCmPerPhysicalCm). */
		double RenderPerPhysical = 1.0;
		/** Far: the envelope in render cm (FarEnvelopeCm); <= 0 keeps the linear scale S. */
		double FarEnvelope = 0.0;
	};

	struct FPointSetDesc
	{
		/** Local (packing) space -> world, or -> physical space when FarEnvelope is enabled. Uniform scale. */
		FTransform LocalToWorld = FTransform::Identity;
		FFarEnvelope FarEnvelope;
		/** Box the points were packed in (local units). Fixed at registration. */
		FBox3f LocalBounds = FBox3f(ForceInit);
		/**
		 * Pixel value = DecodeIntensity(code) * IntensityScale / (distance in LOCAL units)^2 / pixel solid angle
		 * (pre-exposed scene colour). The glow's TotalIntensity uses the same units.
		 */
		float IntensityScale = 1.0f;
		/** 0..1 (daylight etc.), multiplied with SetWorldVisibility. */
		float Visibility = 1.0f;
		/**
		 * Rio 05.10: this set's brightness LOD as a multiple of aps.Stars.GpuPointMinPixel (0.01..1000). Below 1 it follows
		 * a dimmed Visibility, so dimming a set never removes its stars ("the points must not disappear"); above 1 only
		 * stars bright enough to be seen one by one are drawn. A drawn star's brightness is not touched by it.
		 */
		float MinPixelScale = 1.0f;
		/** Stars inside this local sphere are not drawn (home system exclusion); radius <= 0 = off. */
		FVector3f ExclusionCenterLocal = FVector3f::ZeroVector;
		float ExclusionRadiusLocal = 0.0f;
		/**
		 * Rio 03.10 (gameplay sky): further spheres hidden the same way (other materialized systems): local centre xyz,
		 * radius w. Together with the sphere above at most MaxExclusionSpheres are used.
		 */
		TArray<FVector4f> ExtraExclusionSpheresLocal;
		/**
		 * Rio 03.10 (gameplay sky): a star nearer than this (local units) shines as if it were this far, so the law is
		 * I / max(d, floor)^2. 0 keeps the inverse square down to one quantisation step (the menu). Used when the
		 * set must match star glyphs of fixed brightness and nearby stars must not flare.
		 */
		float BrightnessFloorDistanceLocal = 0.0f;
		/** Higher first under aps.Stars.GpuPointBudget. */
		int32 Priority = 0;
		/** Logical stars this set stands for (stats only). */
		int64 Population = 0;
		bool bEnabled = true;
		bool bDrawInSceneCaptures = false;
		FString DebugName;
	};

	/** Points are moved in; the GPU upload is spread over frames (aps.Stars.GpuUploadPointsPerFrame). */
	APSSTARRENDERER_API FHandle RegisterPointSet(const UWorld* World, const FPointSetDesc& Desc, TArray<FPackedStar>&& Points);
	/** Everything except LocalBounds and the points. */
	APSSTARRENDERER_API void UpdatePointSet(FHandle Handle, const FPointSetDesc& Desc);

	/**
	 * Glow input: a 2.5D map of the unresolved light built from catalogue samples (FGlowMapBuilder):
	 * per texel the surface brightness, a dust tracer and a vertical profile made of a thin component
	 * (exponential .. Gaussian .. flat slab) and a thick Gaussian one (bulge/halo).
	 * Map units: local / ExtentXY, so the map spans [-1, 1]^2.
	 */
	struct FGlowMap
	{
		int32 Resolution = 0;
		/** Half size of the square map in local units (x, y). */
		float ExtentXY = 0.0f;
		/** Half thickness of the ray-march box in map units. */
		float ExtentZ = 0.0f;
		/** Emission texel * EmissionNorm = share of the population light per unit map area. */
		float EmissionNorm = 0.0f;
		/** Mean decoded intensity per sampled star (before IntensityScale). */
		double MeanIntensity = 0.0;
		int64 SampleCount = 0;
		/** Resolution^2 texels, row-major: rgb emission (normalised), a dust tracer (peak 1). */
		TArray<FVector4f> Emission;
		/** Resolution^2 texels: x = thin sigma, y = mid-plane offset, z = thick sigma (map units), w = thin share. */
		TArray<FVector4f> Profile;
		/** Resolution^2 texels: thin profile shape, -1 exponential, 0 Gaussian, +1 flat slab (equal variance). */
		TArray<float> Shape;

		bool IsValid() const
		{
			const int32 Texels = Resolution * Resolution;
			return Resolution >= 2 && ExtentXY > 0.0f && ExtentZ > 0.0f && EmissionNorm > 0.0f
				&& Emission.Num() == Texels && Profile.Num() == Texels && Shape.Num() == Texels;
		}
	};

	struct FGlowVolumeDesc
	{
		/** Same local space as the point set (the map was built in it). Uniform scale. */
		FTransform LocalToWorld = FTransform::Identity;
		FFarEnvelope FarEnvelope;
		/**
		 * Light of the stars NOT drawn as points, in the points' units (decoded intensity * IntensityScale),
		 * e.g. (Population - PlacedStars) * Map.MeanIntensity * IntensityScale.
		 */
		double TotalIntensity = 0.0;
		/** Face-on optical depth at the dust peak; 0 for ellipticals and S0. */
		float DustOpacity = 0.0f;
		/** Dust scale height = DustHeightScale * star scale height, capped by DustHeightMax (map units). */
		float DustHeightScale = 0.35f;
		float DustHeightMax = 0.006f;
		/** Rio 03.10 (gameplay sky): FPointSetDesc::BrightnessFloorDistanceLocal for the light along each ray (0 = off). */
		float BrightnessFloorDistanceLocal = 0.0f;
		float Visibility = 1.0f;
		bool bEnabled = true;
		bool bDrawInSceneCaptures = false;
		FString DebugName;
	};

	/** One glow volume is drawn per scene (the first enabled one). The map is moved in. */
	APSSTARRENDERER_API FHandle RegisterGlowVolume(const UWorld* World, const FGlowVolumeDesc& Desc, FGlowMap&& Map);
	APSSTARRENDERER_API void UpdateGlowVolume(FHandle Handle, const FGlowVolumeDesc& Desc);

	/** Point sets and glow volumes. */
	APSSTARRENDERER_API void SetTransform(FHandle Handle, const FTransform& LocalToWorld);
	APSSTARRENDERER_API void SetFarEnvelope(FHandle Handle, const FFarEnvelope& Envelope);
	APSSTARRENDERER_API void SetVisibility(FHandle Handle, float Visibility);
	/** Point sets: FPointSetDesc::MinPixelScale (clamped to 0.01..1000). */
	APSSTARRENDERER_API void SetMinPixelScale(FHandle Handle, float Scale);
	APSSTARRENDERER_API void SetEnabled(FHandle Handle, bool bEnabled);
	APSSTARRENDERER_API void Remove(FHandle Handle);

	/** Every set and volume of the world (also done automatically on world cleanup). */
	APSSTARRENDERER_API void RemoveAll(const UWorld* World);
	/** One envelope for every set and volume of the world: one call per presentation frame. */
	APSSTARRENDERER_API void SetWorldFarEnvelope(const UWorld* World, const FFarEnvelope& Envelope);
	/** 0..1 for the whole world (gameplay daylight: APSGameplayStellarDay::PointVisibility). */
	APSSTARRENDERER_API void SetWorldVisibility(const UWorld* World, float Visibility);
	/** Rio 04.10: the world's points fade over a bright scene (an atmosphere outshines the stars behind it; gameplay). */
	APSSTARRENDERER_API void SetWorldSkyMask(const UWorld* World, bool bEnabled);

	/** Up to 256 linear colours (normalised to unit luminance on upload). Default: black body 1500..40000 K. */
	APSSTARRENDERER_API void SetColorPalette(TConstArrayView<FLinearColor> Colors);
	APSSTARRENDERER_API uint8 GetDefaultPaletteIndex(float TemperatureKelvin);
	APSSTARRENDERER_API FLinearColor GetDefaultPaletteColor(uint8 Index);

	struct FStats
	{
		int32 PointSets = 0;
		int32 GlowVolumes = 0;
		int64 PointsResident = 0;
		int64 PointsPending = 0;
		/** Last frame, all views, after the budget. */
		int64 PointsSubmitted = 0;
		/** GPU counters of the last read-back frame (aps.Stars.GpuStats 1): tested, in frustum, in front of the scene, written. */
		int64 GpuTested = 0;
		int64 GpuInFrustum = 0;
		int64 GpuVisible = 0;
		int64 GpuWritten = 0;
		bool bShadersCompiled = false;
		bool bAtomic64 = false;
		uint32 FrameNumber = 0;
	};
	APSSTARRENDERER_API FStats GetStats();

	/** True when the plugin shaders were compiled at startup (aps.Stars.CompileShaders=1 and no crash guard). */
	APSSTARRENDERER_API bool AreShadersEnabled();
}
