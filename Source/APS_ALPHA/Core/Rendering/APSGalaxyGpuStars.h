// Rio 03.10 (galaxy phase 3): the galaxy's GPU star layer and glow (Plugins/APSStarRenderer), in the menu's continuous
// preview and in the gameplay sky. Inert while aps.Stars.GpuPoints and aps.Stars.GalaxyGlow are 0 (the gameplay sky also
// while aps.Stars.GameplayGpu is 0): no tasks, no allocations, no render commands.
#pragma once

#include "CoreMinimal.h"

class AActor;
class AGalaxy;
class AStarSystem;
class UWorld;
struct FAPSContinuousPreviewFrame;

namespace APSGalaxyGpuStars
{
	/** The menu layer is requested: aps.Stars.GpuPoints or aps.Stars.GalaxyGlow is on and the plugin shaders exist. */
	bool IsRequested();

	/** A galaxy owns a GPU layer or a build is running (game thread). */
	bool HasLayers();

	/** AGalaxy::RebuildGpuStarLayer: background resolve, pack and glow map; registration on the game thread. */
	void RebuildLayer(AGalaxy& Galaxy);

	/** AGalaxy::ReleaseGpuStarLayer: removes the GPU point sets and glow volume and drops a build in flight. */
	void ReleaseLayer(AGalaxy& Galaxy);

	/**
	 * Every continuous preview frame (AAstroGenerator::ApplyContinuousPreviewFrame): remembers the presentation, builds
	 * the layer once the CVars ask for it, keeps far envelope, transform and home exclusion in sync, releases it when
	 * they are switched off. The ISM/HISM presentation is not touched.
	 */
	void PresentContinuousFrame(AGalaxy* Galaxy, const FAPSContinuousPreviewFrame& Frame,
		const FVector& HomeExclusionCenterCm, double HomeExclusionRadiusCm);

	/**
	 * Rio 03.10 (gameplay sky): every UAPSStellarVisualSubsystem::UpdateGameplayStellarView frame of a consumed
	 * full-scale catalogue. Builds the layer for the generator's galaxy (the same catalogue order after the sky's ISM
	 * prefix, the same catalogue frame as its ISM points), follows the component's world transform (origin shifts too)
	 * and hides it inside the home and other materialized systems. The ISM sky is not touched.
	 */
	void PresentGameplayFrame(UWorld* World, AStarSystem* HomeSystem, TConstArrayView<AActor*> AttachedActors);

	/** Rio 04.10 ("the labels lie: a million points are drawn"): the GPU points the world's galaxy layer draws now, beside its
	 * ISM stars (0 while it has none built). */
	int32 GetDrawnPointCount(const UWorld* World);

	/** Rio 04.10 evening ("every visible star must stay exactly as it is; only the glow must drop on the system, star and
	 * planet screens: by half on SYSTEM, by 90% on PLANET"): the menu screen, for the share of the galaxy glow it shows. The
	 * points are never dimmed by a screen. */
	enum class EMenuGlowScope : uint8 { Far, System, Star, Planet };
	/** The screen and how far the camera is along its flight to it (0..1; 1 = arrived). Rio: "not at once, exponentially,
	 * together with the camera": the share follows the flight in log space from the share shown when the flight began. */
	void SetMenuGlowScope(EMenuGlowScope Scope, float FlightAlpha = 1.0f);

	/**
	 * Rio 08.10 (STAR BRIGHTNESS on GALAXY / CLUSTER, "obviously strong overexposure, let me pick it by hand"): a viewing aid
	 * of the menu preview only. Multiplies the menu layer's GPU points and glow on every screen (0.05..4; 1 = exactly as
	 * before: nothing is sent). The points' brightness cut follows it, so no point appears or disappears. The gameplay sky,
	 * the catalogue, seeds and saves are never touched.
	 */
	void SetMenuStarBrightness(float Brightness);

	/** Gameplay daylight: a GPU layer of the world fades like the catalogue points (no-op without a layer). */
	void SetWorldDaylightVisibility(const UWorld* World, float Visibility);

	/** Rio 06.10 (aps.Stars.SystemGlare, each gameplay stellar view frame): Others multiplies the gameplay layer: every level set
	 * (so every twin), the galaxy glow and every approach point through its level's value; the approach point of OwnCatalogIndex
	 * takes Own. Both already carry the day blend; the daylight scene value multiplies on top. (1, INDEX_NONE, 1): exactly as before. */
	void SetWorldSystemGlare(const UWorld* World, float Others, int64 OwnCatalogIndex, float Own);

	/** Rio 03.10 ("every star must be reachable"): a drawn catalogue star near a point of the gameplay world. */
	struct FNearStar
	{
		int64 CatalogIndex{INDEX_NONE};
		/** Order in the catalogue's nested sequence; below the galaxy HISM's instance count it is that instance. */
		int32 Ordinal{INDEX_NONE};
		FVector WorldLocation{FVector::ZeroVector};
		double DistanceCm{0.0};
	};

	/**
	 * The drawn catalogue stars (the sky's ISM prefix and its GPU points) nearest to WorldLocation, nearest first, at most
	 * MaxCount within MaxDistanceCm. Built with the gameplay layer on its background task (a uniform grid of the catalogue
	 * box); false while there is none yet. Positions follow the galaxy component (origin shifts).
	 */
	bool FindNearStars(const UWorld* World, const FVector& WorldLocation, int32 MaxCount, double MaxDistanceCm,
		TArray<FNearStar>& OutStars);

	/** The gameplay galaxy the index belongs to (null without one). */
	AGalaxy* GetIndexedGalaxy(const UWorld* World);

	/** A catalogue position (FGalaxyCatalogStarRecord::GalaxyLocalLocation) where the indexed galaxy draws it now, exactly. */
	bool ProjectCatalogueLocation(const UWorld* World, const FVector& GalaxyLocalLocation, FVector& OutWorldLocation);

	/** The sphere around the galaxy centre that holds every indexed star (charted space); false without an index. */
	bool GetIndexedBounds(const UWorld* World, FVector& OutCentre, double& OutRadiusCm);

	/**
	 * Rio 04.10 ("how do I set a course to the far end of the galaxy?"): the drawn catalogue star a click points at: the
	 * one nearest the ray within MaxAngleRadians of it (the nearer one of two at the same angle). A pass over the whole
	 * index (a few ms, on a click). False when none is that close or there is no index.
	 */
	bool PickAlongRay(const UWorld* World, const FVector& RayOrigin, const FVector& RayDirection, double MaxAngleRadians,
		FNearStar& OutStar);

	/**
	 * Rio 05.10 night ("flying to a star in REAL SCALE the point must become the real star strictly and at once, no
	 * difference at all: it just keeps growing"): in a REAL SCALE world each drawn GPU star within aps.Stars.ApproachPointLy
	 * of the camera gets a one-point set of its own with its level's photometry (the same 1/d^2 law, clamp and PSF) and its
	 * 16-bit quantized twin is hidden by a small sphere. It is taken over where the twin is drawn and eased to the exact
	 * catalogue place on the way in, so neither end of it jumps, and it crossfades into the star's sphere (a materialized
	 * system's star or a near photosphere) while that disc grows to aps.Stars.FarGlyphPixels. Each frame from the gameplay
	 * stellar view (APSFarStarGlyphs::Update, after PresentGameplayFrame). Nothing in legacy worlds or with
	 * aps.Stars.ApproachPoint 0.
	 */
	void UpdateApproachPoints(UWorld* World, const FVector& Camera, double PixelTangent);

	/** Rio 06.10 (star approach v2, stage B, change 12 "pilot's eyes"): who decides and who draws. */
	struct FApproachEyes
	{
		/** The frame's view (the F10 map's camera while it is open): fade, visibility, where the dot is drawn. */
		FVector ViewCamera = FVector::ZeroVector;
		double ViewPixelTangent = 0.0;
		/** The pilot's eyes: takes, releases, glide, speed, the course star's take radius (the view itself while attached). */
		FVector PilotCamera = FVector::ZeroVector;
		double PilotPixelTangent = 0.0;
		/** The view is not the pilot's: the map is open or the view stands farther than aps.Stars.ApproachPilotViewKm. */
		bool bDetached = false;
	};
	/** Stage B entry (APSFarStarGlyphs::Update). The (Camera, PixelTangent) overload stays: eyes = that view, attached. */
	void UpdateApproachPoints(UWorld* World, const FApproachEyes& Eyes);

	/** Rio 06.10 (stage B, change 9): where the course star comes from (polled from the piloted ship, never pushed). */
	enum class ECourseSource : uint8 { None, Autopilot, Navigation, Boresight };
	/** The course star now (INDEX_NONE: none, or aps.Stars.ApproachCourseMaxLy 0) and its source (trace, bench). */
	int64 GetCourseStar(const UWorld* World, ECourseSource* OutSource = nullptr);

	/** Test and diagnostics: how the gameplay GPU layer draws a catalogue star. */
	struct FGpuStarInfo
	{
		bool bGpu = false;
		int32 Ordinal = INDEX_NONE;
		int32 Level = INDEX_NONE;
		double TwinOffsetCm = 0.0;
		/** The course far-take radius with the last pilot pixel tangent: unclamped, and clamped to [aps.Stars.ApproachPointLy,
		 * cap] with cap = aps.Stars.ApproachCourseMaxLy, or 500 ly while that is 0 (so an OFF run still reports it). */
		double CourseTakeRawCm = 0.0;
		double CourseTakeCm = 0.0;
		FVector ExactWorld = FVector::ZeroVector;
	};
	bool DescribeGpuStar(const UWorld* World, int64 CatalogIndex, FGpuStarInfo& Out);

	/** Rio 05.10 night: aps.Stars.ApproachPoint is on and the world is a REAL SCALE one. */
	bool AreApproachPointsActive(const UWorld* World);

	/** Rio 05.10 night: an approach point draws this catalogue star now (it needs no far glyph). */
	bool DrawsApproachPoint(const UWorld* World, int64 CatalogIndex);

	/** Rio 05.10 night: the catalogue stars APSGalaxyNearStars shows as photospheres now (an approach point fades into one). */
	void SetNearPhotospheres(const UWorld* World, TConstArrayView<int64> CatalogIndices);

	/**
	 * Rio 05.10 night: a system stood up or went (APSSystemMaterializer): the exclusion spheres are scanned again in this
	 * frame instead of at the next half-second scan. REAL SCALE with aps.Stars.ApproachPoint only.
	 */
	void RescanExclusions(UWorld* World);
}
