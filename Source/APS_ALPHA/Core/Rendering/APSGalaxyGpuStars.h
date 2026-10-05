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

	/** Gameplay daylight: a GPU layer of the world fades like the catalogue points (no-op without a layer). */
	void SetWorldDaylightVisibility(const UWorld* World, float Visibility);

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
}
