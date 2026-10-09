#include "APSStellarVisualSubsystem.h"

#include "APSGameplayStellarProjection.h"
#include "APSGameplayStarAppearance.h"
#include "APSFarStarGlyphs.h"
#include "APSGalaxyNearStars.h"
#include "APSGalaxyGpuStars.h"
#include "APSStellarViewOptics.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
#include "APS_ALPHA/Core/Rendering/APSAtmosphereTailMaterial.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/GameModes/MainMenuGameModeBase.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PlanetaryAtmosphere.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(APSGameplayStars, true);

namespace APSGameplayStellarOptics
{
	/** Rio 03.10 (the F10 freeze): a camera blend changes the field of view a little every frame, and every such frame
	 * resized the whole catalogue (~61k instances, ~50 ms). Optics-only resizes are at least this far apart; the last
	 * change of a blend lands one interval after it, glyphs a fraction of a pixel off until then. */
	TAutoConsoleVariable<float> CVarOpticsResizeInterval(TEXT("aps.Stars.OpticsResizeInterval"), 0.25f,
		TEXT("Seconds between full star-catalogue resizes caused only by a field-of-view change (0 = every frame, the old way)."));
	double LastOpticsResizeSeconds = -1.0e9;

	/**
	 * Rio 04.10 ("turning the strategic map's camera drops to 30-40 fps, the faster the lower"): the map camera circles
	 * hundreds of AU out, so it crosses a quarter of the nearest catalogue star's distance every frame, and every frame
	 * walked all ~61k points (3-9 ms) and rebuilt the catalogue trees (~12 ms a pass). While the observer moves that fast
	 * (more than FastObserverPerSecond of that distance a second) the full walks and resize passes are spaced out; the
	 * resolved stars and point sizes catch up a few times a second, as soon as it slows down in full.
	 */
	TAutoConsoleVariable<float> CVarFastFullDemandInterval(TEXT("aps.Stars.FastFullDemandInterval"), 0.2f,
		TEXT("Seconds between full star-catalogue demand walks while the observer moves fast (0 = every frame, the old way)."));
	TAutoConsoleVariable<float> CVarFastResizeInterval(TEXT("aps.Stars.FastResizeInterval"), 0.5f,
		TEXT("Seconds between resize passes of one star source while the observer moves fast (0 = aps.Stars.ResizeInterval)."));
	TAutoConsoleVariable<int32> CVarResizeChunk(TEXT("aps.Stars.ResizeChunk"), 4096,
		TEXT("Rio 05.10 afternoon (flight FPS: a resize pass took 17 ms, four times a second at drive speed): points of a star ")
		TEXT("source checked per frame; a pass spreads over frames. 0: a whole source in one frame, as before."));
	/** A resize pass split over frames (aps.Stars.ResizeChunk): the next point to check, the observer at its start and
	 * what it has found so far; no entry while no pass of the source is under way. */
	struct FResizeRun
	{
		int32 Cursor{0};
		FVector Observer{FVector::ZeroVector};
		double SlackCm{TNumericLimits<double>::Max()};
		double NearestCm{TNumericLimits<double>::Max()};
		bool bChanged{false};
	};
	TMap<TWeakObjectPtr<const UObject>, FResizeRun> GResizeRuns;
	constexpr double FastObserverPerSecond = 0.5;
	struct FObserverMotion
	{
		FVector Observer{FVector::ZeroVector};
		double Seconds{0.0};
		double LastFullDemandSeconds{-1.0e9};
	};
	TMap<TWeakObjectPtr<const UObject>, FObserverMotion> GObserverMotion;
}

namespace APSGameplayStellarGlare
{
	/** Rio 06.10 (aps.Stars.SystemGlare): forgets a stellar view's glare state (defined with the glare, below); true when it
	 * had one. */
	bool ForgetSystemGlare(const UObject* Owner);
}

void UAPSStellarVisualSubsystem::ResetGameplayStellarView()
{
	// Rio 06.10 (aps.Stars.SystemGlare): a reset view starts its glare over (snapped on its first frame) and the GPU layer
	// is back to its unglared values. Only a view that had a glare state can have sent any: the menu world (reset every
	// frame, no gameplay generator) and a view that never ran the glare do not call into the GPU layer from here.
	if (APSGameplayStellarGlare::ForgetSystemGlare(this))
	{
		APSGalaxyGpuStars::SetWorldSystemGlare(GetWorld(), 1.0f, INDEX_NONE, 1.0f);
	}
	APSFarStarGlyphs::Reset(GetWorld());
	APSGalaxyNearStars::Reset(GetWorld());
	ResetGameplayNativeStars();
	for (FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
	{
		if (UInstancedStaticMeshComponent* View = Layer.View.Get()) View->DestroyComponent();
		if (UHierarchicalInstancedStaticMeshComponent* Source = Layer.Source.Get())
		{
			Source->SetVisibility(Layer.bSourceVisible, false);
			Source->SetHiddenInGame(Layer.bSourceHidden, false);
		}
	}
	GameplayStellarLayers.Reset();
	GameplayStellarGenerator.Reset();
	GameplayStellarBuildSerial = 0;
	LastStellarPixelTangent = -1.0;
	ClosestStellarPointCm = 0.0;
	ClosestStellarRenderDistanceCm = 0.0;
	LastStellarOccluders.Reset();
}

namespace APSGameplayStellarDay
{
	// Catalogue stars fade in their material with daylight and altitude: none in a full day at the ground, back in
	// full at the top of the atmosphere (Rio, 30.09: "a key feature"; e1-ascent-1). 0 restores the old switch-off.
	TAutoConsoleVariable<int32> CVarDayFadeLog(TEXT("aps.Stars.DayFadeLog"), 0,
		TEXT("1 logs every change of the stars' daylight visibility (smoothness checks)."));
	TAutoConsoleVariable<int32> CVarDayFade(TEXT("aps.Stars.DayFade"), 1,
		TEXT("1 fades the catalogue stars with daylight and altitude in their material; 0 switches them off in a day sky."));
	TAutoConsoleVariable<float> CVarAtmosphereDensityGain(TEXT("aps.Sky.AtmosphereDensityGain"), 12.0f,
		TEXT("How much denser an Earth-like or thicker atmosphere is drawn than its generated base, from the ground and from ")
		TEXT("space alike (1 = the base). Thin air gets less of it, an airless body none."));
	// Rio 06.10 ("why are the stars visible by day? it was all fine"): on the Iron moon Zevyar (0.05 of Earth's air) the
	// sky is still drawn blue with clouds, and a starry day over it reads as broken. The stars' day masking keeps at
	// least this much under any drawn atmosphere shell; the sky's own look does not change. 0: the 01.10 rule (a thin
	// sky hides fewer stars by day).
	// Rio 06.10 (Krathys: "the atmosphere is cut off with a hard outline, make it prettier"): thin moon shells end in
	// a hard circle where the plugin stops the density; the tail master fades it out over the top fifth of the shell.
	TAutoConsoleVariable<float> CVarAtmosphereTail(TEXT("aps.Sky.AtmosphereTail"), 1.0f,
		TEXT("Weight of the soft upper edge on the thin moon shells moved to the tail master on 06.10 (0..1, scaled by how thin ")
		TEXT("the shell is). Frozen planets and Icy moons always keep their accepted tail. 0: those moons draw the plugin's native ")
		TEXT("density (hard cut) as before 06.10 evening."));
	TAutoConsoleVariable<int32> CVarDayFromLocalGround(TEXT("aps.Stars.DayFromLocalGround"), 1,
		TEXT("1: the day sky thins with height above the ground under the observer; 0: above the body's base radius."));
	TAutoConsoleVariable<float> CVarDaySkyMaskingFloor(TEXT("aps.Stars.DaySkyMaskingFloor"), 0.85f,
		TEXT("Least day masking of the stars under a drawn atmosphere, whatever its air (0..1). 0.85 hides them at the ")
		TEXT("ground in daylight; 0 lets thin air show them (the 01.10 rule)."));
	// Rio 06.10 evening (SORYX by day: dozens to hundreds of faint stars over a blue sky at 2-3 km): the GPU galaxy points
	// (03.10, REAL SCALE photometry) took the catalogue's day value, but at the same value they show 10-20x more stars than
	// the catalogue glyphs the e1-ascent curve was tuned on. Below the knee their day value falls faster; above it (the top
	// of the climb, from ~50 km of a 100 km sky) it is today's value exactly, so the return out of the atmosphere keeps its look.
	TAutoConsoleVariable<float> CVarGpuDayDepth(TEXT("aps.Stars.GpuDayDepth"), 1.5f,
		TEXT("Below aps.Stars.GpuDayKnee the GPU star points' day value is the catalogue's times (value/knee)^this: higher ")
		TEXT("shows fewer GPU stars by day and brings them back later. 0: the catalogue's value (as before 06.10 evening)."));
	TAutoConsoleVariable<float> CVarGpuDayKnee(TEXT("aps.Stars.GpuDayKnee"), 0.05f,
		TEXT("The catalogue day value from which the GPU star points take it unchanged (0.05: ~50 km of a 100 km Earth-like sky)."));
	// Rio 06.10 evening ("under a normal atmosphere no stars by day, under weak air only the sun and the brightest"): air the
	// survey calls EARTH-LIKE or denser masks a day sky in full; the 0.85 floor stays the thin-air allowance for the brightest.
	TAutoConsoleVariable<float> CVarDayEarthLikeDensity(TEXT("aps.Stars.DayEarthLikeDensity"), 0.5f,
		TEXT("Air at least this dense (Earth = 1; 0.5 is where the survey says EARTH-LIKE) hides every star in a full day at ")
		TEXT("the ground. 0: day masking from the density curve and aps.Stars.DaySkyMaskingFloor only (as before 06.10 evening)."));

	/**
	 * Raises the plugin's AtmosOpacity on the sky seen from inside and on the shell seen from space, by the same gain,
	 * so that a climb into orbit keeps the look of the sky from the ground (Rio, 01.10: dense and rich from the
	 * ground, suddenly pale in orbit). The plugin writes its base to all its materials in its own tick, before this
	 * subsystem's; skylight, absorption and outer glow keep the base.
	 */
	void ApplyAtmosphereDensityGain(AAtmoScape& Atmosphere, const float DensityGain)
	{
		static const FName InsideSky(TEXT("PlanetaryAtmoMesh"));
		static const FName SpaceShell(TEXT("SpacePlanetaryAtmoMesh"));
		if (FMath::IsNearlyEqual(DensityGain, 1.0f))
		{
			return;
		}
		const float Base = Atmosphere.AtmosphereOpacity * FMath::Max(Atmosphere.PresentationOpacityScale, 0.0f);
		TInlineComponentArray<UStaticMeshComponent*> Meshes(&Atmosphere);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			const FName Name = Mesh ? Mesh->GetFName() : NAME_None;
			if (Name == InsideSky || Name == SpaceShell)
			{
				if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
				{
					Material->SetScalarParameterValue(TEXT("AtmosOpacity"), Base * DensityGain);
				}
			}
		}
	}

	/**
	 * A4 (Rio, 01.10: stutters in flight): every resize pass that changes a point restarts the source's HISM tree build,
	 * whose game-thread part copies the whole catalogue (up to 12.6 ms, 17-19 passes a second flying fast through the
	 * cluster, a4-trace-1). A longer interval trades that for point sizes that catch up in coarser steps; tests only
	 * until a visual check accepts a value.
	 */
	TAutoConsoleVariable<float> CVarResizeInterval(TEXT("aps.Stars.ResizeInterval"),
		static_cast<float>(APSGameplayStellarProjection::PointResizeIntervalSeconds),
		TEXT("Seconds between two resize passes of one star source in flight (default 0.1). Each pass that changes a point ")
		TEXT("rebuilds the source's tree (~12 ms of game thread for the big catalogues)."));

	/** A5: which catalogue stars carry rays in flight (APSStellarOpticalSupport::Select); a change re-sizes them all. */
	// Rio 02.10: a random share with rays read as uneven; every bright enough star sparkles.
	// Rio 08.10 (0.6.1): the crosses stand out of the point field and look unrealistic between stars; none for now, the
	// rules stay for later reuse (1 brings back 02.10's look).
	TAutoConsoleVariable<int32> CVarRayRule(TEXT("aps.Stars.RayRule"), 2,
		TEXT("Rays on the catalogue stars in flight: 0 a stable share of the bright ones, 1 every bright enough ")
		TEXT("star (02.10), 2 none (default since 08.10)."));
	TAutoConsoleVariable<float> CVarRayBrightness(TEXT("aps.Stars.RayBrightness"), 0.1f,
		TEXT("RayRule 1: the brightness from which a catalogue star carries rays (lower: more stars sparkle)."));
	TAutoConsoleVariable<float> CVarRaySize(TEXT("aps.Stars.RaySize"), 1.0f,
		TEXT("RayRule 1: the reach of the rays (0.25..2; 1 = about two thirds of rule 0)."));

	TAutoConsoleVariable<float> CVarDayFadeDepth(TEXT("aps.Stars.DayFadeDepth"), 6.25f,
		TEXT("How deep a day sky dims the catalogue stars, in e-folds of brightness. 6.25: the brightest show from ~20 km of ")
		TEXT("a 100 km atmosphere, all from ~50 km, full brightness at 95 km (e1-ascent-4); lower shows them earlier."));

	/**
	 * The points' brightness for a day factor. Against the dark day sky and the fixed exposure every catalogue star shows
	 * from ~6% of its brightness and all are bright from ~30% (e1-ascent-2 shots: 906 of 906 stars at 6%, 72 of them
	 * bright), so the brightness is exponential in the factor: each step multiplies it by the same amount, from 0.2%
	 * (below the brightest star) to full, and the last 5% of the day ramps that 0.2% down to none.
	 */
	float PointVisibility(const float DayFactor)
	{
		const float Clear = FMath::Clamp(1.0f - DayFactor, 0.0f, 1.0f);
		return FMath::Exp(-FMath::Max(CVarDayFadeDepth.GetValueOnGameThread(), 0.0f) * (1.0f - Clear))
			* FMath::Min(Clear / 0.05f, 1.0f);
	}

	/**
	 * Rio 06.10 evening: the GPU galaxy points' day value (aps.Stars.GpuDayDepth/GpuDayKnee). The catalogue's value from
	 * the knee up (night and space stay exactly 1), below it a steeper fall to the same 0 in a full day; continuous at the
	 * knee and monotonic, so a climb out of the atmosphere brings them back as smoothly as before, only later.
	 */
	float GpuPointVisibility(const float DayFactor)
	{
		const float Visibility = PointVisibility(DayFactor);
		const float Depth = FMath::Max(CVarGpuDayDepth.GetValueOnGameThread(), 0.0f);
		const float Knee = FMath::Clamp(CVarGpuDayKnee.GetValueOnGameThread(), 1.0e-4f, 1.0f);
		if (Visibility <= 0.0f || Visibility >= Knee || Depth <= 0.0f)
		{
			return Visibility;
		}
		return Visibility * FMath::Pow(Visibility / Knee, Depth);
	}

	/**
	 * Sets the points' daylight brightness on their gameplay material (APSGameplayStarAppearance); false when the
	 * material has no such term (not regenerated yet), and the caller switches the points off in a day sky instead.
	 */
	bool ApplyPointVisibility(UMaterialInterface* Material, const float DayFactor, const AActor* Owner, const float Glare = 1.0f)
	{
		static const FName VisibilityParameter(TEXT("GameplayPointVisibility"));
		UMaterialInstanceDynamic* Points = Cast<UMaterialInstanceDynamic>(Material);
		float Applied = 1.0f;
		if (!Points || CVarDayFade.GetValueOnGameThread() == 0 || !Points->GetScalarParameterValue(VisibilityParameter, Applied))
		{
			return false;
		}
		// None in a full day at the ground (the day sky here is dark and the exposure fixed, so even 1% of a point
		// shows), then an even return with altitude up to the Karman line.
		// Rio 06.10 (aps.Stars.SystemGlare): times the system glare (it already carries the day blend); 1.0f: as before.
		const float Visibility = PointVisibility(DayFactor) * Glare;
		// A relative step: the first stars live at a fraction of a percent, where a fixed step would stair. The ends are
		// set exactly (a 1% step left space at 0.9974, e1-ascent-4).
		const bool bEnd = (Visibility <= 0.0f || Visibility >= 1.0f) && Applied != Visibility;
		if (bEnd || FMath::Abs(Applied - Visibility) > 0.01f * FMath::Max(Visibility, 0.0005f))
		{
			if (FMath::Abs(Applied - Visibility) > 0.1f || CVarDayFadeLog.GetValueOnGameThread() != 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.StellarView] %s points fade to %.4f (day %.3f)"),
					*GetNameSafe(Owner), Visibility, DayFactor);
			}
			Points->SetScalarParameterValue(VisibilityParameter, Visibility);
		}
		return true;
	}
}

namespace APSGameplayStellarGlare
{
	/**
	 * Rio 06.10 (system glare): inside a star system its sun outshines the rest of the sky. The stars standing near the
	 * camera (every AStar of the game world) dim the other stars (catalogue points, GPU points and their twins, approach
	 * points) and the galaxy glow by their light at the camera: full glare inside InnerAU of a Sun, none beyond OuterAU
	 * (both times the square root of the luminosity), eased in depth so nothing steps. A standing star's own glyph and
	 * approach point are dimmed only by the stars of other systems; its sphere, corona and material never, nor the near
	 * photospheres, the resolved native stars or any hide rule. A star behind a body (the night side, a planet's shadow,
	 * an eclipse) adds nothing. By day the day sky and the glare add as sky backgrounds, so they never dim twice.
	 * 0 restores the previous path exactly: no scans, every value 1.0f, no GPU sends, glyphs on the shared material.
	 */
	TAutoConsoleVariable<int32> CVarGlare(TEXT("aps.Stars.SystemGlare"), 1,
		TEXT("Rio 06.10: 1 dims the other stars and the galaxy glow inside a star system by the light of its stars (REAL SCALE ")
		TEXT("worlds with aps.Stars.DayFade 1); 0: exactly as before."));
	TAutoConsoleVariable<float> CVarGlareStrength(TEXT("aps.Stars.SystemGlareStrength"), 2.5f,
		TEXT("E-folds of brightness the other stars lose at full system glare (0..12; 2.5: x0.08 at 1 AU of a Sun)."));
	TAutoConsoleVariable<float> CVarGlareRadiusScale(TEXT("aps.Stars.SystemGlareRadiusScale"), 1.0f,
		TEXT("Scales both system glare radii (aps.Stars.SystemGlareInnerAU and aps.Stars.SystemGlareOuterAU)."));
	TAutoConsoleVariable<float> CVarGlareInnerAU(TEXT("aps.Stars.SystemGlareInnerAU"), 1.0f,
		TEXT("Full system glare inside this distance of a Sun-like star, AU (times the square root of its luminosity)."));
	TAutoConsoleVariable<float> CVarGlareOuterAU(TEXT("aps.Stars.SystemGlareOuterAU"), 150.0f,
		TEXT("No system glare beyond this distance of a Sun-like star, AU (times the square root of its luminosity); ")
		TEXT("at or below aps.Stars.SystemGlareInnerAU the glare is off."));
	TAutoConsoleVariable<float> CVarGlareInSeconds(TEXT("aps.Stars.SystemGlareInSeconds"), 0.5f,
		TEXT("Time constant of the system glare while it grows, seconds (0.05..2)."));
	TAutoConsoleVariable<float> CVarGlareOutSeconds(TEXT("aps.Stars.SystemGlareOutSeconds"), 1.0f,
		TEXT("Time constant of the system glare on the way out, seconds (0.05..1)."));
	TAutoConsoleVariable<int32> CVarGlareDayBlend(TEXT("aps.Stars.SystemGlareDayBlend"), 1,
		TEXT("1: a day sky and the system glare add as sky backgrounds (no double dimming by day); 0: their plain product."));
	TAutoConsoleVariable<int32> CVarGlareLog(TEXT("aps.Stars.SystemGlareLog"), 0,
		TEXT("1 logs the system glare on every 1% change and every 0.5 s while it is on (smoothness checks)."));
	// Rio 06.10 (audit: after F10 the sky dimmed back over ~1.5 s): the map camera's glare eased back to the pilot's.
	TAutoConsoleVariable<int32> CVarGlareMapReturnSnap(TEXT("aps.Stars.SystemGlareMapReturnSnap"), 1,
		TEXT("Rio 06.10 review: 1 snaps the system glare to the view while it returns from the F10 map (the view still detached ")
		TEXT("from the pilot after the map closed) and on the frame it re-attaches, so the sky after F10 is the pilot's at once ")
		TEXT("(no ~1.5 s dim). 0: eased as before. The map-open path is unchanged."));

	/** A standing star, as of the last scan (four times a second), and its glare this frame. */
	struct FGlareStar
	{
		TWeakObjectPtr<AStar> Star;
		/** Luminosity, solar (0..1000). */
		double L{0.0};
		double RadiusCm{1.0e5};
		/** Stars of one system share a group and never dim each other (a close companion would dim the main star's glyph). */
		int32 Group{INDEX_NONE};
		/** Eased depth (e-folds) of the glare on this star's own glyph and approach point: the stars of other systems'. */
		double Depth{0.0};
		/** Light at the camera this frame (the Sun at 1 AU = 1), the distance it was measured at and the disc in sight. */
		double E{0.0};
		double DistanceCm{0.0};
		double Visible{1.0};
	};

	/** A body that can hide a star (its sky place is read each frame, its radius at the scan). */
	struct FGlareBody
	{
		TWeakObjectPtr<const APlanetaryBody> Body;
		double RadiusCm{0.0};
	};

	struct FGlareState
	{
		TArray<FGlareStar, TInlineAllocator<8>> Stars;
		TArray<FGlareBody, TInlineAllocator<32>> Bodies;
		double NextScanSeconds{0.0};
		uint32 StandingHash{0};
		/** The materializer's active system and what it stands for (resolved only when that index changes). */
		bool bOwnResolved{false};
		int32 ActiveIndex{INDEX_NONE};
		int64 OwnCatalogIndex{INDEX_NONE};
		FName OwnName;
		/** Eased depth (e-folds) of the glare on everything that does not stand. */
		double OthersDepth{0.0};
		bool bStarted{false};
		bool bWasEmpty{true};
		/** Rio 06.10 (audit: aps.Stars.SystemGlareMapReturnSnap): the view stood detached from the pilot last frame. */
		bool bWasDetached{false};
		bool bOn{false};
		double OnSeconds{0.0};
		double NextLogSeconds{0.0};
		float LoggedCatalogue{-1.0f};
		float LoggedGpu{-1.0f};
		float LoggedOwn{-1.0f};
	};
	TMap<TWeakObjectPtr<const UObject>, FGlareState> GGlareStates;

	/** One frame's glare values; all exactly 1 (and no own index) while it is off. */
	struct FGlareFrame
	{
		bool bEnabled{false};
		/** The catalogue points and any far glyph without a value of its own; the day blend included. */
		float OthersCat{1.0f};
		/** The GPU level sets, glow and approach points; the GPU day blend included. */
		float OthersGpu{1.0f};
		/** The galaxy catalogue star of the materialized system and its approach point's value. */
		int64 OwnCatalogIndex{INDEX_NONE};
		float OwnGpu{1.0f};
		/** Each standing star's far glyph value (the catalogue day blend included). */
		TArray<TPair<const AStar*, float>, TInlineAllocator<8>> OwnCat;
	};

	bool ForgetSystemGlare(const UObject* Owner)
	{
		return GGlareStates.Remove(TWeakObjectPtr<const UObject>(Owner)) > 0;
	}

	/** The glare's target depth (e-folds) for the light at the camera (the Sun at 1 AU = 1); radii already scaled. */
	double GlareDepthFor(const double Light, const double Strength, const double InnerAU, const double OuterAU)
	{
		if (!(Light > 0.0) || !FMath::IsFinite(Light))
		{
			return 0.0;
		}
		const double X = (FMath::Loge(Light) + 2.0 * FMath::Loge(OuterAU)) / (2.0 * FMath::Loge(OuterAU / InnerAU));
		return X <= 0.0 ? 0.0 : Strength * FMath::SmoothStep(0.0, 1.0, FMath::Min(X, 1.0));
	}

	/** Eases a depth toward its target (In while it grows, Out while it falls); held at dt 0, exact within 0.002. */
	double GlareEase(const double Depth, const double Target, const double DeltaSeconds, const double InSeconds,
		const double OutSeconds)
	{
		if (DeltaSeconds <= 0.0)
		{
			return Depth;
		}
		const double Next = Target + (Depth - Target) * FMath::Exp(-DeltaSeconds / (Target > Depth ? InSeconds : OutSeconds));
		return FMath::Abs(Next - Target) < 0.002 ? Target : Next;
	}

	/** The brightness factor of a depth: exactly 1.0f at none. */
	float GlareFactor(const double Depth)
	{
		return Depth > 0.0 ? static_cast<float>(FMath::Exp(-Depth)) : 1.0f;
	}

	/**
	 * How much of a star's disc is in sight past the cached bodies (1: all; behind a planet: none), with a soft limb, so a
	 * sunset, a terminator crossing or an eclipse eases the glare instead of switching it. Direction is the unit vector to
	 * the star, DistanceCm its (clamped) distance.
	 */
	double GlareSunVisible(const FVector& Camera, const FVector& Direction, const double DistanceCm, const double RadiusCm,
		const FGlareState& State, const UAPSWorldOriginSubsystem* Origin)
	{
		const double StarAngle = FMath::Asin(FMath::Min(RadiusCm / DistanceCm, 1.0));
		double Visible = 1.0;
		for (const FGlareBody& Entry : State.Bodies)
		{
			const APlanetaryBody* Body = Entry.Body.Get();
			if (!Body)
			{
				continue;
			}
			const FVector ToBody = (Origin ? Origin->SkyPlaceOf(*Body) : Body->GetActorLocation()) - Camera;
			const double BodyDistance = ToBody.Size();
			// Only a body in front of the star, and not a speck (below 1e-4 rad it hides nothing worth a test).
			if (!(BodyDistance > 0.0) || BodyDistance >= DistanceCm || Entry.RadiusCm < 1.0e-4 * BodyDistance)
			{
				continue;
			}
			const double BodyAngle = FMath::Asin(FMath::Min(Entry.RadiusCm / BodyDistance, 1.0 - 1.0e-9));
			const FVector BodyDirection = ToBody / BodyDistance;
			const double Separation = FMath::Atan2(FVector::CrossProduct(Direction, BodyDirection).Size(),
				FVector::DotProduct(Direction, BodyDirection));
			const double Above = Separation - BodyAngle;
			const double Margin = FMath::Min(0.035, 0.5 * BodyAngle);
			const double Cover = (1.0 - FMath::SmoothStep(-StarAngle - Margin, StarAngle + Margin, Above))
				* FMath::Min(1.0, FMath::Square(BodyAngle / FMath::Max(StarAngle, 1.0e-12)));
			Visible = FMath::Min(Visible, 1.0 - Cover);
		}
		return FMath::Clamp(Visible, 0.0, 1.0);
	}

	/** A glare factor under a day sky of DayVisibility: the two add as sky backgrounds (bBlend), else their product. */
	float GlareUnderDay(const float Glare, const float DayVisibility, const bool bBlend)
	{
		if (!bBlend)
		{
			return Glare;
		}
		return Glare >= 1.0f || DayVisibility <= 0.0f ? 1.0f
			: static_cast<float>(1.0 / (1.0 + static_cast<double>(DayVisibility) * (1.0 / Glare - 1.0)));
	}

	FString GlareStarName(const AStar* Star)
	{
		return !Star ? FString(TEXT("none")) : Star->AstroName.IsNone() ? Star->GetName() : Star->AstroName.ToString();
	}

	/**
	 * Each gameplay stellar view frame of a consumed catalogue, before the GPU layer presents it: the standing stars (a
	 * scan four times a second), their light at Camera, the eased depths and this frame's values. Off (CVar 0, DayFade 0,
	 * not REAL SCALE, no strength, no radius span): every value exactly 1, no scans.
	 */
	FGlareFrame UpdateSystemGlare(UWorld* World, const UObject* Owner, const FVector& Camera, const float DayFactor)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_SystemGlare);
		FGlareFrame Frame;
		if (!World || !Owner)
		{
			return Frame;
		}
		const float StrengthSetting = CVarGlareStrength.GetValueOnGameThread();
		const float InnerSetting = CVarGlareInnerAU.GetValueOnGameThread();
		const float OuterSetting = CVarGlareOuterAU.GetValueOnGameThread();
		const bool bEnabled = CVarGlare.GetValueOnGameThread() != 0
			&& APSGameplayStellarDay::CVarDayFade.GetValueOnGameThread() != 0
			&& StrengthSetting > 0.0f && InnerSetting > 0.0f && OuterSetting > InnerSetting
			&& APSRealScale::IsActive(World);
		const TWeakObjectPtr<const UObject> Key(Owner);
		const double Now = FPlatformTime::Seconds();
		FGlareState* Existing = GGlareStates.Find(Key);
		if (!bEnabled)
		{
			// Off now: the values are 1 this frame; switched on again, the glare eases in from none.
			if (Existing)
			{
				if (Existing->bOn)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars.Glare] off: the other stars are back in full (%.1f s)"),
						Now - Existing->OnSeconds);
				}
				Existing->bOn = false;
				Existing->OthersDepth = 0.0;
				for (FGlareStar& Entry : Existing->Stars)
				{
					Entry.Depth = 0.0;
				}
			}
			return Frame;
		}
		if (!Existing || Now >= Existing->NextScanSeconds)
		{
			for (auto It = GGlareStates.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid())
				{
					It.RemoveCurrent();
				}
			}
		}
		FGlareState& State = GGlareStates.FindOrAdd(Key);
		bool bSnap = !State.bStarted;
		if (Now >= State.NextScanSeconds)
		{
			State.NextScanSeconds = Now + 0.25;
			// The standing stars, grouped by their system (a star of none is a group of its own), and the bodies.
			TArray<const AStarSystem*, TInlineAllocator<8>> Systems;
			for (TActorIterator<AStarSystem> It(World); It; ++It)
			{
				if (IsValid(*It) && !APSGameplayStarAppearance::IsPreviewHierarchy(*It))
				{
					Systems.Add(*It);
				}
			}
			TArray<FGlareStar, TInlineAllocator<8>> Stars;
			int32 LooseGroups = 0;
			uint32 Hash = 0;
			for (TActorIterator<AStar> It(World); It; ++It)
			{
				AStar* Star = *It;
				if (!IsValid(Star) || APSGameplayStarAppearance::IsPreviewHierarchy(Star))
				{
					continue;
				}
				int32 Group = INDEX_NONE;
				for (int32 Index = 0; Index < Systems.Num() && Group == INDEX_NONE; ++Index)
				{
					if (Systems[Index]->MainStar == Star || Systems[Index]->GetStars().Contains(Star))
					{
						Group = Index;
					}
				}
				FGlareStar& Entry = Stars.AddDefaulted_GetRef();
				Entry.Star = Star;
				Entry.L = FMath::IsFinite(Star->Luminosity) ? FMath::Clamp(static_cast<double>(Star->Luminosity), 0.0, 1000.0) : 0.0;
				Entry.RadiusCm = FMath::Max(Star->StarRadiusKM, 1) * 1.0e5;
				Entry.Group = Group != INDEX_NONE ? Group : Systems.Num() + LooseGroups++;
				// A star that stood already keeps its eased depth; a new one starts where the others are.
				const FGlareStar* Previous = State.Stars.FindByPredicate([Star](const FGlareStar& Old)
				{
					return Old.Star.Get() == Star;
				});
				Entry.Depth = Previous ? Previous->Depth : State.OthersDepth;
				Hash = HashCombine(Hash, GetTypeHash(Star));
				Hash = HashCombine(Hash, GetTypeHash(Entry.Group));
			}
			if (Hash != State.StandingHash || Stars.Num() != State.Stars.Num())
			{
				State.StandingHash = Hash;
				FString List;
				for (const FGlareStar& Entry : Stars)
				{
					List += FString::Printf(TEXT(" %s (L %.3g, group %d)"), *GlareStarName(Entry.Star.Get()), Entry.L, Entry.Group);
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars.Glare] standing stars %d:%s"), Stars.Num(), *List);
			}
			// The world populated or loaded: its first stars snap, so a game start does not show the sky dimming.
			bSnap |= State.bWasEmpty && !Stars.IsEmpty();
			State.bWasEmpty = Stars.IsEmpty();
			State.Stars = MoveTemp(Stars);
			State.Bodies.Reset();
			for (TActorIterator<APlanetaryBody> It(World); It; ++It)
			{
				const APlanetaryBody* Body = *It;
				if (!IsValid(Body) || APSGameplayStarAppearance::IsPreviewHierarchy(Body))
				{
					continue;
				}
				double RadiusCm = Body->GetWorldScapeBodyRadiusCm();
				if (!(RadiusCm > 0.0))
				{
					RadiusCm = FMath::Max(Body->RadiusKM, 0.0) * 1.0e5;
				}
				if (RadiusCm > 0.0 && FMath::IsFinite(RadiusCm))
				{
					FGlareBody& Entry = State.Bodies.AddDefaulted_GetRef();
					Entry.Body = Body;
					Entry.RadiusCm = RadiusCm;
				}
			}
		}
		// Rio 06.10 (audit: aps.Stars.SystemGlareMapReturnSnap): whether the view stands detached from the pilot this frame
		// (the F10 map open, or the view farther than aps.Stars.ApproachPilotViewKm from the pawn's eyes: the map camera on
		// its way back), by the rule of APSFarStarGlyphs' MakeApproachEyes, recomputed here (its cache is a frame stale).
		// Coming back from the map (detached, the map closed) and on the frame the view re-attaches, the glare snaps to the
		// view; opening the map and while it stays open the glare eases as before.
		{
			const APlayerController* GlarePlayer = World->GetFirstPlayerController();
			const APawn* GlarePilot = GlarePlayer ? GlarePlayer->GetPawn() : nullptr;
			bool bGlareDetached = false;
			bool bGlareMapOpen = false;
			if (IsValid(GlarePilot))
			{
				const AGravityPlayerController* GlareGravity = Cast<AGravityPlayerController>(GlarePlayer);
				bGlareMapOpen = GlareGravity && GlareGravity->IsStrategicMapOpen();
				static IConsoleVariable* const GlarePilotViewKm =
					IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ApproachPilotViewKm"));
				const double MaxApartCm = FMath::Max(GlarePilotViewKm ? static_cast<double>(GlarePilotViewKm->GetFloat()) : 1000.0,
					1.0) * 1.0e5;
				const FVector Eye = GlarePilot->GetPawnViewLocation();
				bGlareDetached = bGlareMapOpen || (!Eye.ContainsNaN() && FVector::Dist(Camera, Eye) > MaxApartCm);
			}
			if (CVarGlareMapReturnSnap.GetValueOnGameThread() != 0)
			{
				bSnap |= (bGlareDetached && !bGlareMapOpen) || (State.bWasDetached && !bGlareDetached);
			}
			State.bWasDetached = bGlareDetached;
		}

		// Light at the camera from every standing star, from its sky place (the still ship's frame).
		const double Strength = FMath::Clamp(static_cast<double>(StrengthSetting), 0.0, 12.0);
		const double Scale = FMath::Clamp(static_cast<double>(CVarGlareRadiusScale.GetValueOnGameThread()), 1.0e-3, 1.0e3);
		const double InnerAU = InnerSetting * Scale;
		const double OuterAU = OuterSetting * Scale;
		const double OuterLight = 1.0 / FMath::Square(OuterAU);
		const UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
		double AllLight = 0.0;
		int32 Brightest = INDEX_NONE;
		for (int32 Index = 0; Index < State.Stars.Num(); ++Index)
		{
			FGlareStar& Entry = State.Stars[Index];
			Entry.E = 0.0;
			Entry.DistanceCm = 0.0;
			Entry.Visible = 1.0;
			const AStar* Star = Entry.Star.Get();
			// A hidden star (no sphere, no glyph) outshines nothing.
			if (!Star || Star->IsHidden() || Entry.L <= 0.0)
			{
				continue;
			}
			const FVector ToStar = (Origin ? Origin->SkyPlaceOf(*Star) : Star->GetActorLocation()) - Camera;
			const double Raw = ToStar.Size();
			const double Distance = FMath::Max(Raw, 1.5 * Entry.RadiusCm);
			const double Light = Entry.L * FMath::Square(APSStars::AstronomicalUnitCm / Distance);
			// Bodies are tested only for a star bright enough to matter here.
			if (Light > 0.1 * OuterLight && Raw > 0.0)
			{
				Entry.Visible = GlareSunVisible(Camera, ToStar / Raw, Distance, Entry.RadiusCm, State, Origin);
			}
			Entry.DistanceCm = Distance;
			Entry.E = Light * Entry.Visible;
			AllLight += Entry.E;
			if (Brightest == INDEX_NONE || Entry.E > State.Stars[Brightest].E)
			{
				Brightest = Index;
			}
		}

		// Targets and easing in depth space; the first frame and a world's first stars snap.
		const double InSeconds = FMath::Clamp(static_cast<double>(CVarGlareInSeconds.GetValueOnGameThread()), 0.05, 2.0);
		const double OutSeconds = FMath::Clamp(static_cast<double>(CVarGlareOutSeconds.GetValueOnGameThread()), 0.05, 1.0);
		const double DeltaSeconds = FMath::Clamp(static_cast<double>(World->GetDeltaSeconds()), 0.0, 0.25);
		const double OthersTarget = GlareDepthFor(AllLight, Strength, InnerAU, OuterAU);
		State.OthersDepth = bSnap ? OthersTarget : GlareEase(State.OthersDepth, OthersTarget, DeltaSeconds, InSeconds, OutSeconds);
		for (FGlareStar& Entry : State.Stars)
		{
			double OtherSystems = 0.0;
			for (const FGlareStar& Other : State.Stars)
			{
				if (Other.Group != Entry.Group)
				{
					OtherSystems += Other.E;
				}
			}
			const double Target = GlareDepthFor(OtherSystems, Strength, InnerAU, OuterAU);
			Entry.Depth = bSnap ? Target : GlareEase(Entry.Depth, Target, DeltaSeconds, InSeconds, OutSeconds);
		}
		State.bStarted = true;

		// The materializer's system: its galaxy catalogue star takes its own value on the GPU side.
		const FAPSStarSystems* Registry = APSStarSystemsFind(World);
		const FAPSSystemMaterializer* Materializer = Registry ? Registry->GetMaterializer() : nullptr;
		const int32 ActiveIndex = Materializer ? Materializer->GetActiveIndex() : INDEX_NONE;
		if (!State.bOwnResolved || ActiveIndex != State.ActiveIndex)
		{
			const FAPSStarSystemInfo* Info = Registry && ActiveIndex != INDEX_NONE ? Registry->Get(ActiveIndex) : nullptr;
			State.bOwnResolved = Info || ActiveIndex == INDEX_NONE;
			State.ActiveIndex = ActiveIndex;
			State.OwnCatalogIndex = Info ? Info->GalaxyIndex : INDEX_NONE;
			State.OwnName = Info && Info->GalaxyIndex != INDEX_NONE ? FName(*Info->Name) : NAME_None;
		}

		// This frame's values; the day value lags one frame (the daylight update runs after the stellar view).
		const bool bBlend = CVarGlareDayBlend.GetValueOnGameThread() != 0;
		const float DayCatalogue = APSGameplayStellarDay::PointVisibility(DayFactor);
		const float DayGpu = APSGameplayStellarDay::GpuPointVisibility(DayFactor);
		const float Others = GlareFactor(State.OthersDepth);
		Frame.bEnabled = true;
		Frame.OthersCat = GlareUnderDay(Others, DayCatalogue, bBlend);
		Frame.OthersGpu = GlareUnderDay(Others, DayGpu, bBlend);
		Frame.OwnCatalogIndex = State.OwnCatalogIndex;
		// Until the system's star stands, its approach point takes the others' value.
		Frame.OwnGpu = Frame.OthersGpu;
		for (const FGlareStar& Entry : State.Stars)
		{
			const AStar* Star = Entry.Star.Get();
			if (!Star)
			{
				continue;
			}
			const float Own = GlareFactor(Entry.Depth);
			Frame.OwnCat.Emplace(Star, GlareUnderDay(Own, DayCatalogue, bBlend));
			if (State.OwnCatalogIndex != INDEX_NONE && !State.OwnName.IsNone() && Star->AstroName == State.OwnName)
			{
				Frame.OwnGpu = GlareUnderDay(Own, DayGpu, bBlend);
			}
		}

		// Transitions only; the detailed lines with aps.Stars.SystemGlareLog. No string work otherwise.
		const bool bWasOn = State.bOn;
		const FGlareStar* Source = State.Stars.IsValidIndex(Brightest) ? &State.Stars[Brightest] : nullptr;
		if (!bWasOn && OthersTarget > 0.0)
		{
			State.bOn = true;
			State.OnSeconds = Now;
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars.Glare] on: %s (L %.3g) at %.4g AU, target %.4f"),
				*GlareStarName(Source ? Source->Star.Get() : nullptr), Source ? Source->L : 0.0,
				Source ? Source->DistanceCm / APSStars::AstronomicalUnitCm : 0.0, FMath::Exp(-OthersTarget));
		}
		else if (bWasOn && OthersTarget <= 0.0 && State.OthersDepth <= 0.0)
		{
			State.bOn = false;
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars.Glare] off: the other stars are back in full (%.1f s)"), Now - State.OnSeconds);
		}
		if (CVarGlareLog.GetValueOnGameThread() != 0 && (bWasOn || State.bOn))
		{
			const auto Moved = [](const float Value, const float Logged)
			{
				return FMath::Abs(Value - Logged) > 0.01f * FMath::Max(Value, 1.0e-4f);
			};
			if (Now >= State.NextLogSeconds || bWasOn != State.bOn || Moved(Frame.OthersCat, State.LoggedCatalogue)
				|| Moved(Frame.OthersGpu, State.LoggedGpu) || Moved(Frame.OwnGpu, State.LoggedOwn))
			{
				State.NextLogSeconds = Now + 0.5;
				State.LoggedCatalogue = Frame.OthersCat;
				State.LoggedGpu = Frame.OthersGpu;
				State.LoggedOwn = Frame.OwnGpu;
				const AGravityPlayerController* Pilot = Cast<AGravityPlayerController>(World->GetFirstPlayerController());
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars.Glare] f=%llu src=%s L=%.3g d_au=%.5g vis=%.3f E=%.4g target=%.4f g=%.4f ")
					TEXT("cat=%.4f gpu=%.4f own=%lld:%.4f day=%.4f map=%d"),
					static_cast<unsigned long long>(GFrameCounter), *GlareStarName(Source ? Source->Star.Get() : nullptr),
					Source ? Source->L : 0.0, Source ? Source->DistanceCm / APSStars::AstronomicalUnitCm : 0.0,
					Source ? Source->Visible : 1.0, AllLight, FMath::Exp(-OthersTarget), Others, Frame.OthersCat,
					Frame.OthersGpu, static_cast<long long>(Frame.OwnCatalogIndex), Frame.OwnGpu, DayFactor,
					Pilot && Pilot->IsStrategicMapOpen() ? 1 : 0);
			}
		}
		return Frame;
	}
}

void UAPSStellarVisualSubsystem::UpdateGameplayDaylightStars(const FVector& CameraLocation)
{
	// Day factor: how much of the catalogue a lit sky still outshines, on a perceived scale. The key star's height sets
	// day or night (below -3 degrees none, above 7 degrees full). Higher up a lit sky dims and shows ever fainter stars,
	// about the same gain in magnitude per kilometre, so the factor falls linearly with height and is gone just below
	// the top of the atmosphere, the Karman line; the points' brightness follows it on a log scale (PointVisibility).
	// The stars used to be switched off and appeared all at once after take-off; the first fade thinned the air on a
	// fifth of the height and still showed every star by ~10 km of a 100 km atmosphere (Rio, 30.09; e1-ascent-2).
	float Factor = 0.0f;
	// The accepted switch-off curve: full day from 60% of the atmosphere's height down.
	float HideFactor = 0.0f;
	if (bHasTargetStar && GetWorld())
	{
		// Rio 06.10 (audit: the preview route stays separate, APSAtmosphereTailMaterial.h): the menu preview keeps tail 0.
		const bool bPreviewWorld = GetWorld()->GetAuthGameMode<AMainMenuGameModeBase>() != nullptr;
		for (TActorIterator<APlanetaryBody> It(GetWorld()); It; ++It)
		{
			const APlanetaryBody* Body = *It;
			const double AtmosphereCm = IsValid(Body) ? Body->AtmosphereHeight * 100000.0 : 0.0;
			const double RadiusCm = IsValid(Body) ? Body->GetWorldScapeBodyRadiusCm() : 0.0;
			if (AtmosphereCm <= 0.0 || RadiusCm <= 0.0)
			{
				continue;
			}
			// The sky's density (E2, A1): Earth-like air full gain, thin air less, none without air; the same from the
			// ground and from space, for every atmosphere in view.
			if (AAtmoScape* Atmosphere = IsValid(Body->PlanetaryEnvironmentGenerator)
				? Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere : nullptr; IsValid(Atmosphere))
			{
				APSGameplayStellarDay::ApplyAtmosphereDensityGain(*Atmosphere, static_cast<float>(1.0
					+ (FMath::Max(APSGameplayStellarDay::CVarAtmosphereDensityGain.GetValueOnGameThread(), 0.0f) - 1.0)
					* APSAtmosphereModel::DaySkyMasking(APSAtmosphereModel::Density(Body))));
				// Rio 06.10 (audit: the preview route stays separate): a body of the menu preview (its world, or a hierarchy
				// tagged WorldGenerationPreview, as APSPlaceholderGlobe checks) keeps the tail it was built with (0).
				bool bPreviewBody = false;
				for (const AActor* P = Body; IsValid(P); P = P->GetAttachParentActor())
				{
					if (P->ActorHasTag(TEXT("WorldGenerationPreview")))
					{
						bPreviewBody = true;
						break;
					}
				}
				if (!bPreviewWorld && !bPreviewBody)
				{
					// The soft upper edge (aps.Sky.AtmosphereTail), from the live shell height and Rayleigh height, so later
					// overrides of either are followed; only shells whose parent is the tail master carry the parameter.
					const AMoon* Moon = Cast<AMoon>(Body);
					const APlanet* Planet = Cast<APlanet>(Body);
					const bool bFullTail = (Moon && APSAtmosphereTailMaterial::EnabledFor(Moon->MoonType))
						|| (Planet && APSAtmosphereTailMaterial::EnabledFor(Planet->PlanetType));
					// Rio 06.10 (audit: aps.Sky.AtmosphereTail 0 must be the previous path): Frozen planets and Icy moons keep
					// the accepted 03.10 tail (the master's default 1) at any CVar value; it scales only the thin moon shells
					// moved to the tail master on 06.10 (0: their native density, the hard cut). 1 is still written, so a
					// stale 0 is cleared.
					const float Tail = bFullTail ? 1.0f
						: FMath::Clamp(APSGameplayStellarDay::CVarAtmosphereTail.GetValueOnGameThread(), 0.0f, 1.0f)
							* APSAtmosphereTailMaterial::ThinShellWeight(Atmosphere->AtmosphereHeight, Atmosphere->RayleighHeight);
					static const FName TailMaster(TEXT("M_APS_AtmosphereTail"));
					static const FName TailName(APSAtmosphereTailMaterial::TailParameter);
					TInlineComponentArray<UStaticMeshComponent*> ShellMeshes(Atmosphere);
					for (UStaticMeshComponent* ShellMesh : ShellMeshes)
					{
						UMaterialInstanceDynamic* Shell = ShellMesh ? Cast<UMaterialInstanceDynamic>(ShellMesh->GetMaterial(0)) : nullptr;
						float Current = -1.0f;
						if (Shell && Shell->Parent && Shell->Parent->GetFName() == TailMaster
							&& (!Shell->GetScalarParameterValue(FMaterialParameterInfo(TailName), Current)
								|| !FMath::IsNearlyEqual(Current, Tail, 1.0e-3f)))
						{
							Shell->SetScalarParameterValue(TailName, Tail);
						}
					}
				}
			}
			const FVector FromCentre = CameraLocation - Body->GetActorLocation();
			const double Altitude = FromCentre.Size() - RadiusCm;
			if (Altitude >= AtmosphereCm)
			{
				continue;
			}
			const double SunSine = FVector::DotProduct(FromCentre.GetSafeNormal(),
				(TargetStarLocation - CameraLocation).GetSafeNormal());
			// Rio 06.10 (Zevyar: 13 km relief under a ~17 km shell): the air thins from the ground under the observer,
			// not from the base radius, or a high plateau already counts as the top of the sky by day.
			double GroundCm = 0.0;
			AWorldScapeRoot* Root = IsValid(Body->PlanetaryEnvironmentGenerator)
				? Body->PlanetaryEnvironmentGenerator->WorldScapeRootInstance : nullptr;
			if (APSGameplayStellarDay::CVarDayFromLocalGround.GetValueOnGameThread() != 0 && IsValid(Root)
				&& IsValid(Root->WorldScapeNoise) && Root->PlanetScale > 0.0
				&& Body->PlanetaryEnvironmentGenerator->IsSurfaceProfileCurrent(Body))
			{
				const FVector Up = FromCentre.GetSafeNormal();
				GroundCm = FMath::Clamp(Root->GetGroundHeight(Root->GetActorLocation() + Up * Root->PlanetScale, false),
					0.0, 0.8 * AtmosphereCm);
			}
			const double Height = FMath::Clamp(FMath::Max(Altitude - GroundCm, 0.0) / (0.95 * (AtmosphereCm - GroundCm)),
				0.0, 1.0);
			// A thin sky hides fewer stars by day; an airless one none (Rio, 01.10: dark starless skies on weak air).
			// Rio 06.10 evening: EARTH-LIKE air or denser hides them all in a full day (aps.Stars.DayEarthLikeDensity).
			const float Density = APSAtmosphereModel::Density(Body);
			const float EarthLikeDensity = APSGameplayStellarDay::CVarDayEarthLikeDensity.GetValueOnGameThread();
			const double Masking = EarthLikeDensity > 0.0f && Density >= EarthLikeDensity ? 1.0
				: FMath::Max(static_cast<double>(APSAtmosphereModel::DaySkyMasking(Density)),
					FMath::Clamp(static_cast<double>(APSGameplayStellarDay::CVarDaySkyMaskingFloor.GetValueOnGameThread()), 0.0, 1.0));
			Factor = FMath::Max(Factor, static_cast<float>(FMath::SmoothStep(-0.05, 0.12, SunSine) * (1.0 - Height)
				* Masking));
			HideFactor = FMath::Max(HideFactor, static_cast<float>(FMath::SmoothStep(-0.05, 0.12, SunSine)
				* FMath::SmoothStep(0.0, 0.4, 1.0 - FMath::Max(Altitude, 0.0) / AtmosphereCm)));
		}
	}
	// Eased toward the target so no frame jumps; a jump of more than half the range (spawn, a teleport) snaps.
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	GameplayDaylightFactor = FMath::Abs(Factor - GameplayDaylightFactor) > 0.5f || DeltaSeconds <= 0.0f
		? Factor : FMath::FInterpTo(GameplayDaylightFactor, Factor, DeltaSeconds, 6.0f);
	// The catalogue points fade in their material (UpdateGameplayStellarView); only the resolved native stars, separate
	// meshes without that term, still leave a day sky. They follow the points: hidden while the points stay below the
	// faintest visible star, back once those show. Each return rescans the catalogue, so a hysteresis band and three
	// seconds between changes keep a climb through that band from flipping them (six flips in 13 s beside a moon,
	// Rio's playtest 01.10). Without the material fade the old switch-off curve stays.
	const bool bHide = APSGameplayStellarDay::CVarDayFade.GetValueOnGameThread() != 0
		? APSGameplayStellarDay::PointVisibility(GameplayDaylightFactor) < (bGameplayDaylightStarsHidden ? 0.006f : 0.002f)
		: (bGameplayDaylightStarsHidden ? HideFactor > 0.5f : HideFactor > 0.8f);
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (bHide != bGameplayDaylightStarsHidden && Now - GameplayDaylightHideChangeSeconds >= 3.0)
	{
		GameplayDaylightHideChangeSeconds = Now;
		bGameplayDaylightStarsHidden = bHide;
		// Back at night the native stars need one full demand pass; point sizes are still current.
		bGameplayNativeDemandCandidatesValid = false;
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.StellarView] daylight %s the resolved stars (day %.2f)"),
			bHide ? TEXT("hides") : TEXT("shows"), Factor);
	}
	// Rio 03.10 (galaxy phase 3): a GPU star layer of this world fades like the catalogue points (no-op without one):
	// their material fade, or the old switch-off where that is off (aps.Stars.DayFade 0).
	// Rio 06.10 evening (SORYX by day): a steeper day value of their own below the knee (GpuPointVisibility), the same above.
	if (APSGalaxyGpuStars::HasLayers())
	{
		APSGalaxyGpuStars::SetWorldDaylightVisibility(GetWorld(), APSGameplayStellarDay::CVarDayFade.GetValueOnGameThread() != 0
			? APSGameplayStellarDay::GpuPointVisibility(GameplayDaylightFactor) : (bGameplayDaylightStarsHidden ? 0.0f : 1.0f));
		if (APSGameplayStellarDay::CVarDayFadeLog.GetValueOnGameThread() != 0
			&& APSGameplayStellarDay::CVarDayFade.GetValueOnGameThread() != 0)
		{
			// Smoothness checks of the GPU layer next to the catalogue's value: a relative step, like the fade itself.
			static float LastLoggedGpu = -1.0f;
			const float Gpu = APSGameplayStellarDay::GpuPointVisibility(GameplayDaylightFactor);
			if (FMath::Abs(Gpu - LastLoggedGpu) > 0.01f * FMath::Max(Gpu, 1.0e-6f))
			{
				LastLoggedGpu = Gpu;
				UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.StellarView] GPU points day %.6f (catalogue %.5f, day %.3f)"), Gpu,
					APSGameplayStellarDay::PointVisibility(GameplayDaylightFactor), GameplayDaylightFactor);
			}
		}
	}
}

void UAPSStellarVisualSubsystem::UpdateGameplayStellarView()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarView);
	CSV_SCOPED_TIMING_STAT(APSGameplayStars, ObserverView);
	UWorld* World = GetWorld();
	// A new ray rule re-publishes every point's optics: forget the optics the sizes were made for.
	static int32 LastRayRule = 0;
	static float LastRayBrightness = -1.0f;
	static float LastRaySize = -1.0f;
	const float RayBrightness = APSGameplayStellarDay::CVarRayBrightness.GetValueOnGameThread();
	const float RaySize = APSGameplayStellarDay::CVarRaySize.GetValueOnGameThread();
	if (const int32 RayRule = APSGameplayStellarDay::CVarRayRule.GetValueOnGameThread();
		RayRule != LastRayRule || RayBrightness != LastRayBrightness || RaySize != LastRaySize)
	{
		LastRayRule = RayRule;
		LastRayBrightness = RayBrightness;
		LastRaySize = RaySize;
		LastStellarPixelTangent = -1.0;
	}
	// Publish spectral luminosity before selecting optical support. The helper's
	// bounded scan is shared with Tick, so this does not add another catalog scan.
	APSGameplayStarAppearance::Apply(World);
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller || !Controller->PlayerCameraManager) return;
	AAstroGenerator* Generator = GameplayStellarGenerator.Get();
	if (!IsValid(Generator))
	{
		// Full-scale rendering has no replacement layers, but still owns a size
		// cache. A replacement generator must not inherit that cache/build serial.
		ResetGameplayStellarView();
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			// This adapter never participates in the accepted menu presentation.
			if (!It->ActorHasTag(TEXT("WorldGenerationPreview"))
				&& !It->UsesContinuousPreviewFrame()
				&& It->GetCanonicalStellarProjectionDescriptor().bFinalized
				&& It->GetCanonicalStellarProjectionDescriptor().Galaxy.bEnabled
				&& IsValid(It->GetPreviewHomeSystem()))
			{
				Generator = *It;
				GameplayStellarGenerator = Generator;
				break;
			}
		}
	}
	if (!Generator) return;
	const FAPSCanonicalStellarProjectionDescriptor& Descriptor =
		Generator->GetCanonicalStellarProjectionDescriptor();
	AStarSystem* Home = Generator->GetPreviewHomeSystem();
	if (!Descriptor.bFinalized || !IsValid(Home)) { ResetGameplayStellarView(); return; }
	if (Descriptor.bConsumedFinalizedDataset)
	{
		// A committed generated game owns the exact catalog accepted in the menu.
		// Do not replace its three-dimensional hierarchy with ProjectSphere: that
		// collapses every distance onto one camera-centred shell. The immutable HISM
		// transforms already contain one shared affine contraction, so applying its
		// inverse once at the render root restores canonical deltas from the selected
		// home system without rebuilding records. Pixel support below affects only
		// glyph size; it never remaps these centres to an observer shell.
		if (!GameplayStellarLayers.IsEmpty())
		{
			ResetGameplayStellarView();
			GameplayStellarGenerator = Generator;
		}

		const double PositionScale = Descriptor.Galaxy.PositionScale;
		const double PhysicalRootScale = PositionScale > 0.0
			? 1.0 / PositionScale : 0.0;
		if (!FMath::IsFinite(PhysicalRootScale) || PhysicalRootScale <= 0.0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.FullScale.Gameplay] Invalid inverse canonical scale %.9e"),
				PhysicalRootScale);
			return;
		}

		const bool bNewBuild = GameplayStellarBuildSerial != Descriptor.ProxyBuildSerial;
		if (bNewBuild) ResetGameplayNativeStars();
		// Rio 06.10 (still ship): the sky is shown at its world place + the owed travel's sky offset (zero without a
		// debt): the ship stays, the catalogue moves past it.
		const FVector HomeLocation = Home->GetActorLocation() + UAPSWorldOriginSubsystem::SkyOffsetOf(World);
		// Rio 06.10 (audit: a dead sky mover was removed here; it never ran in 61532ed6 and, enabled, would move the catalogue
		// twice): the catalogue is moved by APSWorldOrigin's PlaceSkyCatalogue (aps.Origin.SkyMovesCatalogue 1) or by the
		// snap below (0).
		if (!Generator->GetActorLocation().Equals(HomeLocation, 0.01))
		{
			Generator->SetActorLocation(HomeLocation, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
		const FVector RequiredRootScale(PhysicalRootScale);
		if (!Generator->GetActorScale3D().Equals(RequiredRootScale, 1.0e-3))
		{
			Generator->SetActorScale3D(RequiredRootScale);
		}

		FVector Camera;
		FRotator Rotation;
		Controller->GetPlayerViewPoint(Camera, Rotation);
		// Rio 06.10 (aps.Stars.SystemGlare): the stars standing near the view (the map camera too) outshine the rest of the
		// sky. The GPU layer takes its values before it presents this frame; the catalogue points and far glyphs below.
		const APSGameplayStellarGlare::FGlareFrame Glare = APSGameplayStellarGlare::UpdateSystemGlare(World, this, Camera,
			GameplayDaylightFactor);
		APSGalaxyGpuStars::SetWorldSystemGlare(World, Glare.OthersGpu, Glare.OwnCatalogIndex, Glare.OwnGpu);
		int32 Width = 0, Height = 0;
		Controller->GetViewportSize(Width, Height);
		const double PixelTangent = APSStellarViewOptics::PixelTangent(Controller,
			2.0 * FMath::Tan(FMath::DegreesToRadians(
				Controller->PlayerCameraManager->GetFOVAngle() * 0.5)) / FMath::Max(Width, 320));
		const FVector ObserverFromHome = Camera - HomeLocation;
		TArray<AActor*> Attached;
		uint32 TopologyHash = 0;
		{
			// Rio 06.10 (audit: trace scopes, instrumentation only).
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_StellarAttached);
			Generator->GetAttachedActors(Attached, true, true);
			for (AActor* Actor : Attached)
			{
				UHierarchicalInstancedStaticMeshComponent* Source = nullptr;
				if (AGalaxy* Galaxy = Cast<AGalaxy>(Actor)) Source = Galaxy->StarMeshInstances;
				else if (AStarCluster* Cluster = Cast<AStarCluster>(Actor)) Source = Cluster->StarMeshInstances;
				if (IsValid(Source))
				{
					TopologyHash = HashCombine(TopologyHash, GetTypeHash(TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>(Source)));
					TopologyHash = HashCombine(TopologyHash, GetTypeHash(Source->GetInstanceCount()));
					TopologyHash = HashCombine(TopologyHash, GetTypeHash(Source->GetStaticMesh()));
				}
			}
		}
		// Rio 03.10 (galaxy phase 3, gameplay sky): GPU points + glow of the galaxy catalogue after this sky's ISM prefix,
		// in the same catalogue frame; inert while aps.Stars.GameplayGpu or the plugin CVars are 0.
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_GpuPresent);
			APSGalaxyGpuStars::PresentGameplayFrame(World, Home, Attached);
		}
		// Rio 04.10 (the ~80 ms hitch at every system cruise materialized or released): single proxies hidden or shown
		// again re-size only their own points (GameplayPendingPointRefresh, below); a batch change re-sizes everything.
		Generator->ConsumeCanonicalStellarPointMutations(GameplayPendingPointRefresh);
		const bool bGeometryChanged = bNewBuild
			|| TopologyHash != GameplayNativeTopologyHash
			|| GameplayNativeBatchSerial != Generator->GetCanonicalStellarBatchMutationSerial()
			|| GameplayPendingPointRefresh.Num() > 1024;
		if (bGeometryChanged)
		{
			GameplayPendingPointRefresh.Reset();
			GameplayNativePhysicalRadii.Reset();
			GameplayNativeSizedDistances.Reset();
			GameplayNativeDemandCandidates.Reset();
			bGameplayNativeDemandCandidatesValid = false;
			GameplayNativeResizePasses.Reset();
		}
		// A consumed catalogue keeps every centre at its real position, so observer travel changes only glyph
		// sizes. Geometry and optics (FOV) changes resize the whole catalogue; travel resizes each point on its own
		// once its size error reaches the pixel budget (below). A fast approach to one star then costs a few points
		// per frame instead of a pass over all 61k instances (29.09).
		const bool bOpticsChanged = !APSGameplayStellarProjection::CanReuseOptics(LastStellarPixelTangent, PixelTangent);
		const double OpticsNowSeconds = FPlatformTime::Seconds();
		const bool bOpticsDue = bOpticsChanged && (LastStellarPixelTangent <= 0.0
			|| OpticsNowSeconds - APSGameplayStellarOptics::LastOpticsResizeSeconds
				>= FMath::Max(APSGameplayStellarOptics::CVarOpticsResizeInterval.GetValueOnGameThread(), 0.0f));
		const bool bUpdatePointSizes = bGeometryChanged || bOpticsDue;
		if (bUpdatePointSizes && bOpticsChanged)
		{
			APSGameplayStellarOptics::LastOpticsResizeSeconds = OpticsNowSeconds;
		}
		// No point of a source can leave its size budget before the observer travels that source's smallest slack,
		// so its per-point check is skipped until then (walking on a planet: practically never; 29.09). Among the
		// ~1 AU-spaced cluster stars a CRUISE flight ran out of slack every frame, and each pass restarted the
		// source's HISM tree build and scene proxy (~2.5 ms a frame). Passes of one source are now
		// PointResizeIntervalSeconds apart, and one source at most is checked per frame (30.09).
		const double NowSeconds = FPlatformTime::Seconds();
		bool bResizePassTaken = false;
		// Nearest catalogue point now: at least each source's nearest at its last pass, less the travel since.
		const auto NearestCatalogueCm = [&]()
		{
			double Nearest = TNumericLimits<double>::Max();
			for (const auto& Pass : GameplayNativeResizePasses)
			{
				if (!Pass.Key.IsValid() || Pass.Value.NearestCm == TNumericLimits<double>::Max()) continue;
				Nearest = FMath::Min(Nearest,
					Pass.Value.NearestCm - FVector::Distance(ObserverFromHome, Pass.Value.Observer));
			}
			return Nearest;
		};
		const double ClosestNowCm = NearestCatalogueCm();
		APSGameplayStellarOptics::FObserverMotion& Motion =
			APSGameplayStellarOptics::GObserverMotion.FindOrAdd(TWeakObjectPtr<const UObject>(this));
		const double MotionSeconds = Motion.Seconds > 0.0 ? NowSeconds - Motion.Seconds : 0.0;
		const double MotionCm = FVector::Distance(ObserverFromHome, Motion.Observer);
		Motion.Observer = ObserverFromHome;
		Motion.Seconds = NowSeconds;
		// Rio 05.10 afternoon (flight FPS): a bound at or below zero means the observer has travelled more than the
		// nearest distance since the last pass, i.e. it is fast by this very test. Read as "not fast", it switched the
		// spacing off at drive speed once passes were split over frames (their bound is older): 30-90 full walks a
		// second instead of 10, +1 ms a frame.
		const bool bFastObserver = MotionSeconds > 0.0 && MotionCm > 0.0 && ClosestNowCm < TNumericLimits<double>::Max()
			&& MotionCm / MotionSeconds > FMath::Max(ClosestNowCm, 0.0) * APSGameplayStellarOptics::FastObserverPerSecond;
		const FQuat ViewRotation = Rotation.Quaternion();
		const double TanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(
			Controller->PlayerCameraManager->GetFOVAngle() * 0.5));
		const double TanHalfVertical = TanHalfHorizontal * FMath::Max(Height, 1) / FMath::Max(Width, 1);
		// Selection is view-dependent even when distance/FOV allow point-size reuse.
		// Refresh before crossing the 32px admission guard, without reuploading HISM.
		// Rio 03.10: with aps.Stars.NativeMode 1 the resolved stars are chosen in all directions, so a turn needs nothing.
		const bool bDemandTurned = APSGameplayNativeStars::UsesViewSelection()
			&& (GameplayNativeDemandRotation.AngularDistance(ViewRotation) > PixelTangent * 8.0
				|| !FMath::IsNearlyEqual(GameplayNativeTanHalfHorizontal, TanHalfHorizontal, 1.0e-6)
				|| !FMath::IsNearlyEqual(GameplayNativeTanHalfVertical, TanHalfVertical, 1.0e-6));
		// A turn changes which stars are on screen, not how large they look: only travel (or new geometry/optics)
		// rebuilds the short list of stars bright enough to matter, and a turn re-selects among those. Every mouse
		// turn of 8 px used to walk all 61k points (3-9 ms, the stutter when looking around; 29.09). Travel
		// re-measures the candidates at the 2% step and walks the whole catalogue only at the 25% step, before which
		// no point outside the list can reach admission (CanReuseDemandCandidates; 30.09).
		const bool bFullDemand = bUpdatePointSizes || !bGameplayNativeDemandCandidatesValid
			|| (!APSGameplayStellarProjection::CanReuseDemandCandidates(
				FVector::Distance(ObserverFromHome, GameplayNativeDemandObserver),
				FMath::Min(GameplayNativeDemandClosestCm, ClosestNowCm))
				&& (!bFastObserver || NowSeconds - Motion.LastFullDemandSeconds
					>= APSGameplayStellarOptics::CVarFastFullDemandInterval.GetValueOnGameThread()));
		if (bFullDemand)
		{
			Motion.LastFullDemandSeconds = NowSeconds;
		}
		const bool bDemandTravelled = !APSGameplayStellarProjection::CanReuseDemand(
			FVector::Distance(ObserverFromHome, GameplayNativeDemandRefreshObserver),
			FMath::Min(GameplayNativeDemandRefreshClosestCm, ClosestNowCm));
		const bool bRefreshDemand = bFullDemand || bDemandTurned || bDemandTravelled;
		if (bFullDemand)
		{
			GameplayNativeDemandObserver = ObserverFromHome;
			GameplayNativeDemandCandidates.Reset();
			bGameplayNativeDemandCandidatesValid = true;
		}
		if (bRefreshDemand)
		{
			GameplayNativeDemandRefreshObserver = ObserverFromHome;
		}
		// Candidates keep a 2x margin below the smallest admission radius (RetainPixels 0.65 px).
		constexpr double CandidatePixels = 0.3;
		BeginGameplayNativeStars(Generator, bRefreshDemand, Camera, PixelTangent,
			ViewRotation, TanHalfHorizontal, TanHalfVertical);
		for (AActor* Actor : Attached)
		{
			UHierarchicalInstancedStaticMeshComponent* Source = nullptr;
			const TArray<FTransform>* BaseTransforms = nullptr;
			if (AGalaxy* Galaxy = Cast<AGalaxy>(Actor))
			{
				Source = Galaxy->StarMeshInstances;
				BaseTransforms = &Galaxy->RenderedProxyBaseTransforms;
			}
			else if (AStarCluster* Cluster = Cast<AStarCluster>(Actor))
			{
				Source = Cluster->StarMeshInstances;
				BaseTransforms = &Cluster->SystemProxyBaseTransforms;
			}
			if (IsValid(Source))
			{
				// A day sky fades the points through their gameplay material (APSGameplayStarAppearance), brightest last.
				// Rio 06.10 (aps.Stars.SystemGlare): times the others' glare (1.0f while it is off).
				const bool bFades = APSGameplayStellarDay::ApplyPointVisibility(Source->GetMaterial(0),
					GameplayDaylightFactor, Source->GetOwner(), Glare.OthersCat);
				// A material without the fade (not regenerated yet) still switches the points off in a day sky.
				const bool bHidden = !bFades && bGameplayDaylightStarsHidden;
				Source->SetVisibility(!bHidden, false);
				Source->SetHiddenInGame(bHidden, false);
			}
			if (!IsValid(Source) || !BaseTransforms || !IsValid(Source->GetStaticMesh())) continue;
			TArray<int32> RefreshIndices;
			for (const FAPSGameplayStellarKey& Key : GameplayPendingPointRefresh)
			{
				if (Key.Source.Get() == Source)
				{
					RefreshIndices.Add(Key.Index);
				}
			}
			FAPSGameplayStellarResizePass& Pass = GameplayNativeResizePasses.FindOrAdd(Source);
			const double ResizeInterval = FMath::Max(APSGameplayStellarDay::CVarResizeInterval.GetValueOnGameThread(),
				bFastObserver ? APSGameplayStellarOptics::CVarFastResizeInterval.GetValueOnGameThread() : 0.0f);
			// Rio 05.10 afternoon (flight FPS: a pass took 17 ms, four times a second at drive speed, where every point of a
			// source runs out of its size budget at once): a pass under way goes on where the last frame left it.
			APSGameplayStellarOptics::FResizeRun* Run = APSGameplayStellarOptics::GResizeRuns.Find(Source);
			const bool bResumePass = !bUpdatePointSizes && !bResizePassTaken && Run;
			const bool bResizeTravelled = bResumePass || (!bUpdatePointSizes && !bResizePassTaken
				&& FVector::Distance(ObserverFromHome, Pass.Observer) > FMath::Max(Pass.SlackCm, 1.0)
				&& NowSeconds - Pass.Seconds >= FMath::Max(ResizeInterval, 0.0));
			if (!bRefreshDemand && !bResizeTravelled && RefreshIndices.IsEmpty()) continue;
			if (!APSStellarOpticalSupport::EnsureLayout(Source)) continue;

			// Work inside the existing affine frame: no enormous per-instance
			// translations or root-scale cancellation are sent to the GPU.
			const FTransform ComponentTransform = Source->GetComponentTransform();
			const FVector LocalCamera = ComponentTransform.InverseTransformPosition(Camera);
			const double ComponentScale = ComponentTransform.GetScale3D().GetAbsMax();
			const double MeshRadius = FMath::Max(
				Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
			TArray<double>& PhysicalRadii = GameplayNativePhysicalRadii.FindOrAdd(Source);
			if (PhysicalRadii.Num() != Source->GetInstanceCount())
				PhysicalRadii.Init(-1.0, Source->GetInstanceCount());
			const auto GetPhysicalRadius = [&](const FAPSGameplayStellarKey& Key)
			{
				double& Radius = PhysicalRadii[Key.Index];
				if (Radius < 0.0) Radius = APSGameplayNativeStars::PhysicalRadiusCm(Key);
				return Radius;
			};
			TArray<double>& SizedDistances = GameplayNativeSizedDistances.FindOrAdd(Source);
			TArray<int32>& DemandCandidates = GameplayNativeDemandCandidates.FindOrAdd(Source);
			// Sizes one point for the current observer; true when its transform or optical data changed.
			// OutDistanceCm is the distance it was sized at (0 for suppressed points, which need no size).
			const auto SizePoint = [&](int32 Index, FTransform& Transform, bool bCollectDemand, double& OutDistanceCm)
			{
				OutDistanceCm = 0.0;
				const FAPSGameplayStellarKey Key = Generator->MakeGameplayStellarKey(Source, Index);
				bool bNativeOwnsPoint = false;
				if (!ObserveGameplayNativePoint(Generator, Key, (*BaseTransforms)[Index],
					Transform, bNativeOwnsPoint))
				{
					if (Generator->IsGameplayStellarKeyCurrent(Key)
						&& Generator->GetGameplayStellarSuppression(Key) != 0
						&& Transform.GetScale3D() != FVector::ZeroVector)
					{
						Transform.SetScale3D(FVector::ZeroVector);
						return true;
					}
					return false;
				}
				const FVector BaseScale = (*BaseTransforms)[Index].GetScale3D();
				const double SourceRadius = MeshRadius * BaseScale.GetAbsMax();
				const double PhysicalRadiusCm = GetPhysicalRadius(Key);
				// Consumed datasets retain legacy impostor scales in BaseTransforms.
				// They are identity snapshots, not the physical radius used by the menu.
				const double BaseRadius = ComponentScale > 0.0 ? PhysicalRadiusCm / ComponentScale : 0.0;
				if (!FMath::IsFinite(BaseRadius) || BaseRadius <= 0.0 || SourceRadius <= 0.0) return false;
				const double Distance = FVector::Distance(Transform.GetLocation(), LocalCamera);
				// Measured like the per-point check below (from the immutable centre), so the two never disagree.
				OutDistanceCm = FVector::Distance((*BaseTransforms)[Index].GetLocation(), LocalCamera) * ComponentScale;
				const auto Profile = APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData,
					Source->NumCustomDataFloats, Index, APSGameplayStellarDay::CVarRayRule.GetValueOnGameThread(),
					APSGameplayStellarDay::CVarRayBrightness.GetValueOnGameThread());
				const double PixelWorldRadius = Distance * PixelTangent;
				if (bCollectDemand)
				{
					const double ApparentPixels = PixelWorldRadius > 0.0 ? BaseRadius / PixelWorldRadius : 0.0;
					CollectGameplayNativeDemand(Key, (*BaseTransforms)[Index], PhysicalRadiusCm, ApparentPixels);
					if (ApparentPixels >= CandidatePixels)
					{
						DemandCandidates.Add(Index);
					}
				}
				const double CoreRadius = APSStellarOpticalSupport::CoreRadius(BaseRadius, PixelWorldRadius);
				const double PointRadius = APSStellarOpticalSupport::CarrierRadius(BaseRadius, PixelWorldRadius, Profile);
				bool bChanged = APSStellarOpticalSupport::Publish(Source, Index,
					APSStellarOpticalSupport::CoreScale(CoreRadius, PointRadius),
					APSStellarOpticalSupport::ResolvedRayStrength(Profile, BaseRadius, PixelWorldRadius));
				const FVector PointScale = bNativeOwnsPoint ? FVector::ZeroVector
					: BaseScale * (PointRadius / SourceRadius);
				if (!Transform.GetScale3D().Equals(PointScale, 1.0e-6))
				{
					Transform.SetScale3D(PointScale);
					bChanged = true;
				}
				return bChanged;
			};
			if (!bUpdatePointSizes)
			{
				// The single points first, once the tree build their change started has landed (any instance change
				// during an async build restarts it): sized like every other point for the current observer.
				if (!RefreshIndices.IsEmpty() && SizedDistances.Num() == Source->GetInstanceCount() && !Source->IsAsyncBuilding())
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarPointRefresh);
					bool bChanged = false;
					for (const int32 Index : RefreshIndices)
					{
						FTransform Transform;
						if (BaseTransforms->IsValidIndex(Index) && SizedDistances.IsValidIndex(Index)
							&& Source->GetInstanceTransform(Index, Transform, false)
							&& SizePoint(Index, Transform, false, SizedDistances[Index]))
						{
							Source->UpdateInstanceTransform(Index, Transform, false, false, true);
							bChanged = true;
						}
					}
					GameplayPendingPointRefresh.RemoveAll([Source](const FAPSGameplayStellarKey& Key)
					{
						return Key.Source.Get() == Source;
					});
					if (bChanged)
					{
						Source->BuildTreeIfOutdated(true, false);
					}
				}
				if (bRefreshDemand)
				{
					// A camera turn changes selection, not HISM size or geometry. Avoid
					// allocating/readback of the full transform catalog on every turn.
					const auto CollectDemand = [&](int32 Index)
					{
						const FAPSGameplayStellarKey Key = Generator->MakeGameplayStellarKey(Source, Index);
						const double RadiusCm = GetPhysicalRadius(Key);
						const double PixelRadiusCm = FVector::Distance((*BaseTransforms)[Index].GetLocation(), LocalCamera)
							* ComponentScale * PixelTangent;
						const double ApparentPixels = PixelRadiusCm > 0.0 ? RadiusCm / PixelRadiusCm : 0.0;
						CollectGameplayNativeDemand(Key, (*BaseTransforms)[Index], RadiusCm, ApparentPixels);
						return ApparentPixels;
					};
					if (bFullDemand)
					{
						TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarDemandWalk);
						for (int32 Index = 0; Index < PhysicalRadii.Num(); ++Index)
						{
							if (BaseTransforms->IsValidIndex(Index) && CollectDemand(Index) >= CandidatePixels)
							{
								DemandCandidates.Add(Index);
							}
						}
					}
					else
					{
						for (const int32 Index : DemandCandidates)
						{
							if (BaseTransforms->IsValidIndex(Index) && PhysicalRadii.IsValidIndex(Index))
							{
								CollectDemand(Index);
							}
						}
					}
				}
				if (Run && SizedDistances.Num() != Source->GetInstanceCount())
				{
					// The source changed under a split pass: the next one starts over.
					APSGameplayStellarOptics::GResizeRuns.Remove(Source);
					Run = nullptr;
				}
				if (bResizeTravelled && SizedDistances.Num() == Source->GetInstanceCount())
				{
					if (Source->IsAsyncBuilding())
					{
						// Any instance change during an async tree build makes UE discard and restart it, so the
						// points wait for the build to land (a few frames) and are checked again then.
						continue;
					}
					TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarResizePass);
					// Only points whose own size error reached the budget are re-sized and re-uploaded.
					bResizePassTaken = true;
					// Rio 05.10 afternoon (flight FPS): aps.Stars.ResizeChunk points a frame; the pass's findings carry over
					// in Pass until its last point, and its observer is the one at its start (the next pass comes earlier
					// rather than later). 0: the whole source in one frame, as before.
					if (!Run)
					{
						Run = &APSGameplayStellarOptics::GResizeRuns.Add(Source);
						Run->Observer = ObserverFromHome;
					}
					const int32 Chunk = APSGameplayStellarOptics::CVarResizeChunk.GetValueOnGameThread();
					const int32 End = Chunk > 0 ? FMath::Min(Run->Cursor + Chunk, SizedDistances.Num()) : SizedDistances.Num();
					for (int32 Index = Run->Cursor; Index < End; ++Index)
					{
						if (SizedDistances[Index] <= 0.0 || !BaseTransforms->IsValidIndex(Index)) continue;
						const double DistanceCm = FVector::Distance((*BaseTransforms)[Index].GetLocation(), LocalCamera)
							* ComponentScale;
						Run->NearestCm = FMath::Min(Run->NearestCm, DistanceCm);
						if (!APSGameplayStellarProjection::CanReusePointSize(DistanceCm, SizedDistances[Index]))
						{
							FTransform Transform;
							if (Source->GetInstanceTransform(Index, Transform, false)
								&& SizePoint(Index, Transform, false, SizedDistances[Index]))
							{
								Source->UpdateInstanceTransform(Index, Transform, false, false, true);
								Run->bChanged = true;
							}
						}
						if (SizedDistances[Index] > 0.0)
						{
							Run->SlackCm = FMath::Min(Run->SlackCm,
								APSGameplayStellarProjection::PointSizeSlackCm(DistanceCm, SizedDistances[Index]));
						}
					}
					Run->Cursor = End;
					if (End < SizedDistances.Num())
					{
						continue;
					}
					// The next pass waits until the observer has travelled the smallest slack found now.
					const APSGameplayStellarOptics::FResizeRun Done = *Run;
					APSGameplayStellarOptics::GResizeRuns.Remove(Source);
					Pass.Observer = Done.Observer;
					Pass.SlackCm = Done.SlackCm < TNumericLimits<double>::Max() ? FMath::Max(Done.SlackCm, 0.0) : 0.0;
					Pass.NearestCm = Done.NearestCm;
					Pass.Seconds = NowSeconds;
					if (Done.bChanged)
					{
						// A legacy-mode HISM reaches the GPU through its tree: ApplyBuildTree recreates the
						// render state when the build lands, so no extra proxy rebuild here.
						Source->BuildTreeIfOutdated(true, false);
					}
				}
				continue;
			}
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayStellarFullResize);
			TArray<FTransform> Transforms;
			Transforms.SetNum(Source->GetInstanceCount());
			SizedDistances.Init(0.0, Transforms.Num());
			bool bChanged = false;
			double SlackCm = TNumericLimits<double>::Max();
			double NearestCm = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Transforms.Num(); ++Index)
			{
				FTransform& Transform = Transforms[Index];
				if (!Source->GetInstanceTransform(Index, Transform, false)) return;
				if (!BaseTransforms->IsValidIndex(Index)) continue;
				bChanged |= SizePoint(Index, Transform, true, SizedDistances[Index]);
				if (SizedDistances[Index] > 0.0)
				{
					NearestCm = FMath::Min(NearestCm, SizedDistances[Index]);
					SlackCm = FMath::Min(SlackCm,
						APSGameplayStellarProjection::PointSizeSlackCm(SizedDistances[Index], SizedDistances[Index]));
				}
			}
			APSGameplayStellarOptics::GResizeRuns.Remove(Source);
			Pass.Observer = ObserverFromHome;
			Pass.SlackCm = SlackCm < TNumericLimits<double>::Max() ? FMath::Max(SlackCm, 0.0) : 0.0;
			Pass.NearestCm = NearestCm;
			Pass.Seconds = NowSeconds;
			if (bChanged)
			{
				Source->BatchUpdateInstancesTransforms(0, Transforms, false, false, true);
				// Automatic HISM rebuilds are disabled by the stellar stability policy.
				// Refresh bounds once for this batch so enlarged points are not culled.
				// Asynchronously, as FlushSources does: a pass changes each glyph by at most
				// the size budget, while a synchronous rebuild of the 25k/36k catalogue trees
				// stalled the game thread ~22 ms per source (29.09).
				Source->BuildTreeIfOutdated(true, false);
				Source->MarkRenderStateDirty();
			}
		}
		if (bUpdatePointSizes)
		{
			LastStellarObserverFromHome = ObserverFromHome;
			LastStellarPixelTangent = PixelTangent;
		}
		// The demand steps are measured from the nearest point at the walk / re-measure just made.
		const double ClosestAfterCm = NearestCatalogueCm();
		if (bFullDemand)
		{
			GameplayNativeDemandClosestCm = ClosestAfterCm;
		}
		if (bRefreshDemand)
		{
			GameplayNativeDemandRefreshClosestCm = ClosestAfterCm;
		}
		if (ClosestAfterCm < TNumericLimits<double>::Max())
		{
			ClosestStellarPointCm = ClosestAfterCm;
		}
		GameplayNativeTopologyHash = TopologyHash;
		PresentGameplayNativeStars(Generator);
		// Rio 06.10 (aps.Stars.SystemGlare): a standing star's glyph takes its own value (only other systems' stars dim it),
		// any other the others'; off, the glyphs keep the shared catalogue material.
		APSFarStarGlyphs::SetSystemGlare(World, Glare.bEnabled, APSGameplayStellarDay::CVarDayFade.GetValueOnGameThread() != 0
			? APSGameplayStellarDay::PointVisibility(GameplayDaylightFactor) : 1.0f, Glare.OthersCat, Glare.OwnCat);
		// B7: a materialized star smaller than its glyph (the home sun from its planets) keeps its catalogue glyph.
		APSFarStarGlyphs::Update(GetWorld(), Attached, Camera, PixelTangent, bGameplayDaylightStarsHidden);
		// Rio 03.10: the galaxy GPU-only stars near the camera grow into photospheres (APSGalaxyNearStars).
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_NearStars);
			APSGalaxyNearStars::Update(GetWorld(), Camera, PixelTangent, bGameplayDaylightStarsHidden);
		}

		GameplayStellarBuildSerial = Descriptor.ProxyBuildSerial;
		if (bNewBuild)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[APS.FullScale.Gameplay] Restored accepted stellar hierarchy in physical 3D scale=%.9e anchor=%s pointSupport=%.1fpx"),
				PhysicalRootScale, *HomeLocation.ToCompactString(),
				APSGameplayStellarProjection::PointSupportPixels);
		}
		return;
	}
	if (GameplayStellarBuildSerial != 0 && GameplayStellarBuildSerial != Descriptor.ProxyBuildSerial)
	{
		ResetGameplayStellarView();
		return;
	}
	if (GameplayStellarLayers.IsEmpty())
	{
		TArray<AActor*> Attached;
		Generator->GetAttachedActors(Attached, true, true);
		const auto AddLayer = [&](UHierarchicalInstancedStaticMeshComponent* Source,
			EAPSCanonicalStellarProxyLayer Kind, const FAPSCanonicalStellarProjectionFrame& Frame)
		{
			if (!IsValid(Source) || !IsValid(Source->GetStaticMesh()) || Source->GetInstanceCount() == 0) return;
			FAPSGameplayStellarLayer Layer;
			Layer.Source = Source;
			Layer.bSourceVisible = Source->IsVisible();
			Layer.bSourceHidden = Source->bHiddenInGame;
			UInstancedStaticMeshComponent* View = NewObject<UInstancedStaticMeshComponent>(Generator,
				NAME_None, RF_Transient);
			View->SetAbsolute(true, true, true);
			View->SetMobility(EComponentMobility::Movable);
			View->bDisallowNanite = true;
			View->SetForceDisableNanite(true);
			// Camera-following transforms must not rescan the complete catalog for bounds.
			View->SetUseConservativeBounds(true);
			View->SetStaticMesh(Source->GetStaticMesh());
			for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot)
				View->SetMaterial(Slot, Source->GetMaterial(Slot));
			View->SetNumCustomDataFloats(Source->NumCustomDataFloats);
			View->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			View->bDisableCollision = true;
			View->SetGenerateOverlapEvents(false);
			View->SetCanEverAffectNavigation(false);
			View->SetCastShadow(false);
			View->bAffectDynamicIndirectLighting = false;
			View->bAffectDistanceFieldLighting = false;
			View->bEvaluateWorldPositionOffset = false;
			View->bWorldPositionOffsetWritesVelocity = false;
			View->SetReceivesDecals(false);
			View->SetCullDistances(0, 0);
			View->SetVisibility(false, false);
			Generator->AddInstanceComponent(View);
			View->RegisterComponent();
			Layer.View = View;
			Layer.Transforms.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector),
				Source->GetInstanceCount());
			View->AddInstances(Layer.Transforms, false, false);
			for (int32 Index = 0; Index < Source->GetInstanceCount(); ++Index)
			{
				FAPSCanonicalStellarProxyRecord Record;
				if (Generator->GetCanonicalStellarProxyRecord(Kind, Index, Record)
					&& !Record.bSuppressedMaterializedHome)
				{
					FAPSGameplayStellarPoint& Point = Layer.Points.AddDefaulted_GetRef();
					Point.CenterFromHomeCm = Frame.GetCanonicalRootPositionCm(Record.CanonicalPositionUnits)
						- Frame.CanonicalAnchorCm;
					Point.RadiusCm = Record.CanonicalPhysicalRadiusSolar * APSCanonicalStellarProjection::SolarRadiusCm;
					Point.InstanceIndex = Index;
				}
				for (int32 Field = 0; Field < Source->NumCustomDataFloats; ++Field)
				{
					const int32 Address = Index * Source->NumCustomDataFloats + Field;
					if (Source->PerInstanceSMCustomData.IsValidIndex(Address))
						View->SetCustomDataValue(Index, Field, Source->PerInstanceSMCustomData[Address], false);
				}
			}
			APSStellarOpticalSupport::EnsureLayout(View);
			GameplayStellarLayers.Add(MoveTemp(Layer));
		};
		for (AActor* Actor : Attached)
		{
			if (AGalaxy* Galaxy = Cast<AGalaxy>(Actor))
				AddLayer(Galaxy->StarMeshInstances, EAPSCanonicalStellarProxyLayer::Galaxy, Descriptor.Galaxy);
			else if (AStarCluster* Cluster = Cast<AStarCluster>(Actor))
				AddLayer(Cluster->StarMeshInstances, EAPSCanonicalStellarProxyLayer::StarCluster, Descriptor.StarCluster);
		}
		if (GameplayStellarLayers.IsEmpty()) return;
		GameplayStellarBuildSerial = Descriptor.ProxyBuildSerial;
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.StellarView] Initialized %d physical observer layers; immutable catalog retained"),
			GameplayStellarLayers.Num());
	}

	FVector Camera;
	FRotator Rotation;
	Controller->GetPlayerViewPoint(Camera, Rotation);
	int32 Width = 0, Height = 0;
	Controller->GetViewportSize(Width, Height);
	const double PixelTangent = APSStellarViewOptics::PixelTangent(Controller,
		2.0 * FMath::Tan(FMath::DegreesToRadians(
			Controller->PlayerCameraManager->GetFOVAngle() * 0.5)) / FMath::Max(Width, 320));
	const FVector HomeLocation = Home->GetActorLocation();
	// Rio 06.10 (still ship): the camera-relative layers see the catalogue from where the ship truly is.
	const FVector SkyHomeLocation = HomeLocation + UAPSWorldOriginSubsystem::SkyOffsetOf(GetWorld());
	TArray<FAPSPreviewOccluder> Occluders;
	TArray<AActor*> Bodies;
	Home->GetAttachedActors(Bodies, true, true);
	for (AActor* Body : Bodies)
	{
		if (!IsValid(Body)) continue;
		FVector Center = Body->GetActorLocation();
		double Radius = 0.0;
		if (AStar* Star = Cast<AStar>(Body))
		{
			// Use the actual opaque silhouette, not the additive corona or an AABB
			// sphere inflated by sqrt(3). Do not resize the accepted primary star.
			if (IsValid(Star->StarMesh) && IsValid(Star->StarMesh->GetStaticMesh()))
			{
				const FBoxSphereBounds Bounds = Star->StarMesh->GetStaticMesh()->GetBounds();
				Center = Star->StarMesh->GetComponentTransform().TransformPosition(Bounds.Origin);
				Radius = Bounds.BoxExtent.GetMax() * Star->StarMesh->GetComponentScale().GetAbsMax();
			}
		}
		else if (APlanetaryBody* Planet = Cast<APlanetaryBody>(Body))
		{
			Radius = FMath::Max(Planet->RadiusKM, 0.0) * 1.0e5;
			// Ground can be below the reference sea-level sphere. It must never turn
			// an observer in a valley into an observer with the entire sky occluded.
			Radius = FMath::Min(Radius, FVector::Distance(Center, Camera) * (1.0 - 1.0e-8));
		}
		if (!FMath::IsFinite(Radius) || Radius <= 0.0) continue;
		FAPSPreviewOccluder& Occluder = Occluders.AddDefaulted_GetRef();
		Occluder.Center = Center - Camera;
		Occluder.Radius = Radius;
	}
	const FVector ObserverFromHome = Camera - SkyHomeLocation;
	// UE 5.4 marks ALL instance transforms dirty on a component translation.
	// Keep the render anchor stationary until its own accumulated screen error
	// reaches the same sub-pixel budget. Never move an entire ISM on a cache hit.
	double AnchorTravelCm = 0.0;
	for (const FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
		if (const UInstancedStaticMeshComponent* View = Layer.View.Get())
			AnchorTravelCm = FMath::Max(AnchorTravelCm, FVector::Distance(Camera, View->GetComponentLocation()));
	const double TravelCm = FVector::Distance(ObserverFromHome, LastStellarObserverFromHome);
	const bool bOpticsChanged = !APSGameplayStellarProjection::CanReuseOptics(LastStellarPixelTangent, PixelTangent);
	const bool bPhysicalParallax = !APSGameplayStellarProjection::CanReuseProjection(TravelCm, ClosestStellarPointCm, PixelTangent);
	const bool bRenderParallax = !APSGameplayStellarProjection::CanReuseProjection(AnchorTravelCm, ClosestStellarRenderDistanceCm, PixelTangent);
	const bool bReproject = bOpticsChanged || bPhysicalParallax || bRenderParallax;
	const double NearestPointCm = ClosestStellarPointCm - TravelCm;
	const double MaskTravelCm = FVector::Distance(ObserverFromHome, LastStellarMaskObserver);
	const bool bUpdateMask = bReproject
		|| !APSGameplayStellarProjection::CanReuseProjection(MaskTravelCm, NearestPointCm, PixelTangent)
		|| !APSGameplayStellarProjection::CanReuseOcclusion(LastStellarOccluders, Occluders, NearestPointCm, PixelTangent);
	// The daylight fade follows every frame, as in the full-scale path; each view shares its source's material.
	for (const FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
		if (UInstancedStaticMeshComponent* View = Layer.View.Get())
			APSGameplayStellarDay::ApplyPointVisibility(View->GetMaterial(0), GameplayDaylightFactor,
				Layer.Source.IsValid() ? Layer.Source->GetOwner() : View->GetOwner());
	if (!bUpdateMask) return;
	CSV_SCOPED_TIMING_STAT(APSGameplayStars, Refresh);
	CSV_CUSTOM_STAT(APSGameplayStars, MaskRefreshes, 1, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, ProjectionRefreshes, bReproject ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, OpticsInvalidations, bOpticsChanged ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, PhysicalInvalidations, bPhysicalParallax ? 1 : 0, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(APSGameplayStars, AnchorInvalidations, bRenderParallax ? 1 : 0, ECsvCustomStatOp::Accumulate);
	if (bReproject)
	{
		LastStellarObserverFromHome = ObserverFromHome;
		LastStellarPixelTangent = PixelTangent;
		ClosestStellarPointCm = TNumericLimits<double>::Max();
		ClosestStellarRenderDistanceCm = TNumericLimits<double>::Max();
	}
	LastStellarOccluders = Occluders;
	LastStellarMaskObserver = ObserverFromHome;
	TArray<APSGameplayStellarProjection::FPreparedOccluder> PreparedOccluders;
	for (const FAPSPreviewOccluder& Occluder : Occluders) PreparedOccluders.Emplace(Occluder);
	for (FAPSGameplayStellarLayer& Layer : GameplayStellarLayers)
	{
		UInstancedStaticMeshComponent* View = Layer.View.Get();
		if (!IsValid(View) || !IsValid(View->GetStaticMesh())) continue;
		if (!APSStellarOpticalSupport::EnsureLayout(View)) continue;
		const double MeshRadius = FMath::Max(View->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
		TArray<APSStellarOpticalSupport::FProfile> OpticalProfiles;
		TArray<float> CoreScales, RayStrengths;
		if (bReproject)
		{
			OpticalProfiles.Reserve(Layer.Points.Num());
			CoreScales.Init(1.0f, Layer.Points.Num());
			RayStrengths.Init(0.0f, Layer.Points.Num());
			for (const FAPSGameplayStellarPoint& Point : Layer.Points)
				OpticalProfiles.Add(APSStellarOpticalSupport::Select(View->PerInstanceSMCustomData,
					View->NumCustomDataFloats, Point.InstanceIndex, APSGameplayStellarDay::CVarRayRule.GetValueOnGameThread(),
					APSGameplayStellarDay::CVarRayBrightness.GetValueOnGameThread()));
		}
		constexpr int32 ChunkSize = 1024;
		const int32 ChunkCount = FMath::DivideAndRoundUp(Layer.Points.Num(), ChunkSize);
		TArray<double> ClosestInChunk;
		TArray<double> ClosestRenderInChunk;
		TArray<uint8> Dirty;
		Dirty.SetNumZeroed(Layer.Transforms.Num());
		ClosestInChunk.Init(TNumericLimits<double>::Max(), ChunkCount);
		ClosestRenderInChunk.Init(TNumericLimits<double>::Max(), ChunkCount);
		ParallelFor(ChunkCount, [&](int32 Chunk)
		{
			const int32 End = FMath::Min((Chunk + 1) * ChunkSize, Layer.Points.Num());
			for (int32 Index = Chunk * ChunkSize; Index < End; ++Index)
			{
				FAPSGameplayStellarPoint& Point = Layer.Points[Index];
				const FVector Offset = Point.CenterFromHomeCm - ObserverFromHome;
				const double Distance = Offset.Size();
				ClosestInChunk[Chunk] = FMath::Min(ClosestInChunk[Chunk], Distance);
				if (Distance <= 0.0 || !FMath::IsFinite(Distance)) continue;
				const FVector Direction = Offset / Distance;
				bool bOccluded = false;
				for (const auto& Occluder : PreparedOccluders)
					if (Occluder.Occludes(Direction, Distance)) { bOccluded = true; break; }
				if (bReproject)
				{
					FAPSPreviewProjectedSphere Sphere;
					double OpticalRadius = 0.0;
					if (!APSGameplayStellarProjection::Project(Offset, Point.RadiusCm, PixelTangent,
						Sphere, OpticalRadius, OpticalProfiles[Index].SupportPixels)) continue;
					const double PixelWorldRadius = Sphere.Center.Size() * PixelTangent;
					CoreScales[Index] = APSStellarOpticalSupport::CoreScale(
						APSStellarOpticalSupport::CoreRadius(Sphere.Radius, PixelWorldRadius), OpticalRadius);
					RayStrengths[Index] = APSStellarOpticalSupport::ResolvedRayStrength(
						OpticalProfiles[Index], Sphere.Radius, PixelWorldRadius);
					ClosestRenderInChunk[Chunk] = FMath::Min(ClosestRenderInChunk[Chunk], Sphere.Center.Size());
					Point.ProjectedTransform = FTransform(FQuat::Identity, Sphere.Center, FVector(OpticalRadius / MeshRadius));
				}
				// A changed body does not imply a changed stellar transform. Only
				// stars whose visibility actually changes need a renderer update.
				if (APSGameplayStellarProjection::NeedsInstanceUpload(bReproject, Point.bOccluded, bOccluded))
				{
					Layer.Transforms[Point.InstanceIndex] = Point.ProjectedTransform;
					if (bOccluded) Layer.Transforms[Point.InstanceIndex].SetScale3D(FVector::ZeroVector);
					Dirty[Point.InstanceIndex] = 1;
				}
				Point.bOccluded = bOccluded;
			}
		});
		if (bReproject)
		{
			for (int32 Index = 0; Index < Layer.Points.Num(); ++Index)
				APSStellarOpticalSupport::Publish(View, Layer.Points[Index].InstanceIndex, CoreScales[Index], RayStrengths[Index]);
			for (double Distance : ClosestInChunk) ClosestStellarPointCm = FMath::Min(ClosestStellarPointCm, Distance);
			for (double Distance : ClosestRenderInChunk) ClosestStellarRenderDistanceCm = FMath::Min(ClosestStellarRenderDistanceCm, Distance);
			View->SetWorldLocation(Camera, false, nullptr, ETeleportType::TeleportPhysics);
		}
		int32 DirtyCount = 0;
		for (uint8 Changed : Dirty) DirtyCount += Changed;
		CSV_CUSTOM_STAT(APSGameplayStars, ChangedInstances, DirtyCount, ECsvCustomStatOp::Accumulate);
		if (DirtyCount > Layer.Transforms.Num() / 8)
		{
			View->BatchUpdateInstancesTransforms(0, Layer.Transforms, false, false, true);
		}
		else
		{
			// Coalesce small visibility deltas; do not upload all 60k instances
			// just because one point crossed a planetary limb.
			int32 Start = 0;
			while (Start < Dirty.Num())
			{
				if (!Dirty[Start]) { ++Start; continue; }
				int32 End = Start + 1;
				while (End < Dirty.Num() && Dirty[End]) ++End;
				View->BatchUpdateInstancesTransforms(Start,
					TArrayView<const FTransform>(Layer.Transforms.GetData() + Start, End - Start), false, false, true);
				Start = End;
			}
		}
		// A day sky switches the view off only when its material cannot fade the points (not regenerated yet).
		const bool bHidden = bGameplayDaylightStarsHidden && !APSGameplayStellarDay::ApplyPointVisibility(View->GetMaterial(0),
			GameplayDaylightFactor, Layer.Source.IsValid() ? Layer.Source->GetOwner() : View->GetOwner());
		View->SetVisibility(!bHidden, false);
		View->SetHiddenInGame(bHidden, false);
		// Hide only after a populated replacement is ready; do not propagate into
		// child actors and never alter source instance transforms or custom data.
		if (UHierarchicalInstancedStaticMeshComponent* Source = Layer.Source.Get())
		{
			Source->SetVisibility(false, false);
			Source->SetHiddenInGame(true, false);
		}
	}
}

int32 APSStellarOpticalSupport::RayRuleSetting()
{
	return APSGameplayStellarDay::CVarRayRule.GetValueOnAnyThread();
}

double APSStellarOpticalSupport::RayBrightnessSetting()
{
	return APSGameplayStellarDay::CVarRayBrightness.GetValueOnAnyThread();
}

double APSStellarOpticalSupport::RaySizeSetting()
{
	return APSGameplayStellarDay::CVarRaySize.GetValueOnAnyThread();
}
