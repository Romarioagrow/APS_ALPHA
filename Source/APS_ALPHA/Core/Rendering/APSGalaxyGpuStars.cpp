// Rio 03.10 (galaxy phase 3): the galaxy's GPU star layer and glow (Plugins/APSStarRenderer).
// The catalogue stays the only source of truth: the layer is ordinals [ISM prefix, prefix + N) of the same nested order,
// resolved with APSGalaxyCatalogBatch::ResolveStars, packed to 8 bytes and drawn by the plugin; the glow carries the rest
// of the population. Two presentations: the menu's continuous preview (far envelope around the preview observer) and the
// gameplay sky (the full-scale 3D hierarchy of UAPSStellarVisualSubsystem, the galaxy HISM component's own frame).
// Everything is inert while aps.Stars.GpuPoints / aps.Stars.GalaxyGlow are 0, the gameplay sky also with aps.Stars.GameplayGpu 0.
#include "APSGalaxyGpuStars.h"

#include "APSGameplayStarAppearance.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSContinuousPreviewFrame.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APSGalaxyGlowBuilder.h"
#include "APSStarRendererAPI.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Containers/Ticker.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "UObject/Package.h"
#include "UnrealClient.h"
#include <atomic>

namespace APSGalaxyGpuStars
{
	namespace
	{
		TAutoConsoleVariable<int32> CVarGpuStars(
			TEXT("aps.Galaxy.GpuStars"), 8000000,
			TEXT("Rio 03.10 (galaxy phase 3): catalogue stars drawn as GPU points after the menu's ISM prefix while ")
			TEXT("aps.Stars.GpuPoints is on (8 bytes each, built on a background task; a change rebuilds the layer)."));

		// Rio 04.10 evening ("the stars themselves must stay; only the glow drops"), then 5% / 5% / 1%, and 05.10 ("the orbits
		// on SYSTEM still can't be read, with white stars it's all washed out ... the glow off completely on the system, star
		// and planet maps, and everything else dimmed by half, the way the stars dim in a planet's daytime sky; but the star
		// points themselves must not disappear"): share of the menu galaxy glow per screen.
		TAutoConsoleVariable<float> CVarMenuGlowSystem(
			TEXT("aps.Stars.MenuGlowSystem"), 0.0f,
			TEXT("Share of the menu galaxy glow on the SYSTEM screen (1 = as on GALAXY and CLUSTER)."));
		TAutoConsoleVariable<float> CVarMenuGlowStar(
			TEXT("aps.Stars.MenuGlowStar"), 0.0f,
			TEXT("Share of the menu galaxy glow on the STAR screen (1 = as on GALAXY and CLUSTER)."));
		TAutoConsoleVariable<float> CVarMenuGlowPlanet(
			TEXT("aps.Stars.MenuGlowPlanet"), 0.0f,
			TEXT("Share of the menu galaxy glow on the PLANET screen (1 = as on GALAXY and CLUSTER)."));
		// The GPU points' brightness per screen, eased with the camera like the glow. Every point GALAXY draws stays drawn:
		// the brightness LOD drops with the share (aps.Stars.MenuMinPixel*), so dimming never removes a star.
		TAutoConsoleVariable<float> CVarMenuPointsSystem(
			TEXT("aps.Stars.MenuPointsSystem"), 0.5f,
			TEXT("Brightness of the menu galaxy's GPU star points on the SYSTEM screen (1 = as on GALAXY and CLUSTER). Points never vanish."));
		TAutoConsoleVariable<float> CVarMenuPointsStar(
			TEXT("aps.Stars.MenuPointsStar"), 0.5f,
			TEXT("Brightness of the menu galaxy's GPU star points on the STAR screen (1 = as on GALAXY and CLUSTER). Points never vanish."));
		TAutoConsoleVariable<float> CVarMenuPointsPlanet(
			TEXT("aps.Stars.MenuPointsPlanet"), 0.5f,
			TEXT("Brightness of the menu galaxy's GPU star points on the PLANET screen (1 = as on GALAXY and CLUSTER). Points never vanish."));
		// Optional, off by default: on top of that, a close screen may draw only the points bright enough to be seen one by
		// one (times aps.Stars.GpuPointMinPixel); above 1 the faint millions that merge into a mat are skipped.
		TAutoConsoleVariable<float> CVarMenuMinPixelSystem(
			TEXT("aps.Stars.MenuMinPixelSystem"), 1.0f,
			TEXT("SYSTEM screen: above 1 only GPU points this many times brighter than the GALAXY cut are drawn (1 = every point GALAXY draws)."));
		TAutoConsoleVariable<float> CVarMenuMinPixelStar(
			TEXT("aps.Stars.MenuMinPixelStar"), 1.0f,
			TEXT("STAR screen: above 1 only GPU points this many times brighter than the GALAXY cut are drawn (1 = every point GALAXY draws)."));
		TAutoConsoleVariable<float> CVarMenuMinPixelPlanet(
			TEXT("aps.Stars.MenuMinPixelPlanet"), 1.0f,
			TEXT("PLANET screen: above 1 only GPU points this many times brighter than the GALAXY cut are drawn (1 = every point GALAXY draws)."));
		// Rio 06.10 (menu: "going planet -> star -> system the stars' glow first drops at once and then returns to its value; it
		// must follow the camera animation smoothly, no brightness jumps").
		TAutoConsoleVariable<int32> CVarMenuGlowContinuous(
			TEXT("aps.Stars.MenuGlowContinuous"), 1,
			TEXT("Rio 06.10: 1 starts every menu screen change and every camera flight from the brightness on screen (glow, ")
			TEXT("points and their brightness cut taken at the same moment) and eases it with the camera's smoothstep over the ")
			TEXT("rest of the flight; a change without a flight (or a flight cut short) eases over the shortest flight's time. ")
			TEXT("0: the 06.10 way (the start taken at a screen change only, so the next flight on the same kind of screen, ")
			TEXT("e.g. CLUSTER -> GALAXY after SYSTEM -> CLUSTER, restarted from the older screen's value: drop, then back)."));

		TAutoConsoleVariable<float> CVarMenuFadeInSeconds(
			TEXT("aps.Stars.MenuFadeInSeconds"), 0.6f,
			TEXT("Rio 04.10 (\"after Regenerate the GPU glow comes on with a jump\"): seconds over which a newly built menu ")
			TEXT("galaxy layer fades in. 0: at once (the old way)."));

		TAutoConsoleVariable<float> CVarPopulationNormalize(
			TEXT("aps.Stars.PopulationNormalize"), 0.0f,
			TEXT("Rio 04.10 (test only): exponent k pulling a very bright population's GPU layer (giants) back towards an ")
			TEXT("ordinary one by (ordinary mean intensity / its mean)^k. 0: off. Takes effect at the next layer build."));
		constexpr double OrdinaryMeanIntensity = 4.0;

		TAutoConsoleVariable<int32> CVarGpuStarsFirst(
			TEXT("aps.Galaxy.GpuStarsFirst"), -1,
			TEXT("First catalogue ordinal of the GPU points. -1 (default): right after the ISM prefix, so no star is drawn ")
			TEXT("twice. 0 with aps.Galaxy.GpuStars (menu) or aps.Galaxy.GameplayGpuStars (gameplay) = the STARS count draws ")
			TEXT("the ISM stars again as GPU points (brightness and position check: toggle aps.Stars.GpuPoints and compare)."));

		TAutoConsoleVariable<int32> CVarGameplayGpu(
			TEXT("aps.Stars.GameplayGpu"), 1,
			TEXT("Rio 03.10 (galaxy phase 3, \"and in the gameplay sky too\"): 1 adds the galaxy's GPU points and glow to the ")
			TEXT("gameplay sky (with aps.Stars.GpuPoints / aps.Stars.GalaxyGlow). 0: the gameplay sky is exactly the ISM one."));

		TAutoConsoleVariable<int32> CVarGameplayGpuStars(
			TEXT("aps.Galaxy.GameplayGpuStars"), 4000000,
			TEXT("Catalogue stars drawn as GPU points in the gameplay sky after its ISM prefix (fewer than the menu: the ")
			TEXT("gameplay frame also renders planets and atmospheres). A change rebuilds the layer."));

		TAutoConsoleVariable<int32> CVarGameplayLaw(
			TEXT("aps.Stars.GameplayGpuLaw"), 1,
			TEXT("Rio 03.10 (\"the menu shows millions of stars; in gameplay they vanish as the camera comes closer\"): 1 draws ")
			TEXT("the gameplay sky's GPU points and glow with the menu's photometry (inverse square from the camera, no floor), ")
			TEXT("so the sky and the map look like the menu's views. 0: brightness matched to the glyphs with a one-galaxy-")
			TEXT("radius floor (points fade out where they stop overlapping). A change rebuilds the layer."));
		TAutoConsoleVariable<float> CVarGameplayGain(
			TEXT("aps.Stars.GameplayGpuGain"), 1.0f,
			TEXT("GameplayGpuLaw 1: the gameplay GPU points' and glow's brightness relative to the menu's. A change rebuilds."));
		TAutoConsoleVariable<float> CVarGameplayGlowFloor(
			TEXT("aps.Stars.GameplayGlowFloor"), 0.15f,
			TEXT("Rio 03.10 (the gameplay sky turned a pink-white haze in the galaxy's core): GameplayGpuLaw 1 fades the glow's ")
			TEXT("light nearer than this share of the galaxy radius (by distance squared), so the local core does not fill ")
			TEXT("every direction and the disk reads as a band. 0: none. A change rebuilds."));
		TAutoConsoleVariable<float> CVarMapPointHoldRadii(
			TEXT("aps.Stars.MapPointHoldRadii"), 1.0f,
			TEXT("Rio 06.10 (strategic map GALAXY: 'on My Galaxy the GPU stars switch off, a little closer they come back'): while the ")
			TEXT("F10 map is open, beyond this many galaxy radii from the galaxy's centre the gameplay GPU points (GameplayGpuLaw 1) ")
			TEXT("keep the brightness they have there ((distance/hold)^2 on their sets' IntensityScale), so the whole disc stays drawn ")
			TEXT("at every map zoom with its look from inside. The pilot's sky and the cluster/system maps are untouched. 0: off ")
			TEXT("(the inverse square drops the whole layer under aps.Stars.GpuPointMinPixel at the GALAXY preset)."));

		// Rio 05.10 night ("when I fly to a star in REAL SCALE the hand-over from the GPU point to the real star must be strict
		// and simultaneous, visually no difference at all: the star keeps growing as I come closer and just becomes the real
		// one"). Before: the point stood at its 16-bit quantized place (thousands of AU off at hundreds of parsecs, 8-16
		// degrees at 0.25 ly), and at materialization a far glyph of other photometry took over at the exact place.
		TAutoConsoleVariable<int32> CVarApproachPoint(
			TEXT("aps.Stars.ApproachPoint"), 1,
			TEXT("Rio 05.10 night: 1 draws the GPU stars near the camera in a REAL SCALE world as exact points of their own (the ")
			TEXT("level's photometry, no quantization), taken over where the quantized point stands, eased to the exact place on ")
			TEXT("the way in and crossfaded into the star's sphere at aps.Stars.FarGlyphPixels; a materialized system's far glyph ")
			TEXT("is then only a fallback. 0: the quantized point and, once the system stands, the far glyph (05.10 evening). ")
			TEXT("Legacy worlds never use it."));
		TAutoConsoleVariable<float> CVarApproachPointLy(
			TEXT("aps.Stars.ApproachPointLy"), 3.0f,
			TEXT("Rio 05.10 night: light years from the camera within which a drawn GPU star gets its approach point (it goes ")
			TEXT("back to its quantized point 10% farther out, where it stands on it again)."));
		TAutoConsoleVariable<float> CVarApproachPointFade(
			TEXT("aps.Stars.ApproachPointFade"), 0.6f,
			TEXT("Rio 05.10 night: the approach point's crossfade into the star's sphere starts when the disc radius reaches this ")
			TEXT("share of aps.Stars.FarGlyphPixels and ends at aps.Stars.FarGlyphPixels."));
		// Rio 06.10 (star approach v2, stage A: "approaching a star its point vanishes, comes back somewhere else and goes
		// down"; canon: like a planet, a point that grows and the real star switching on at the same place, stitched to the
		// still ship's sky place).
		TAutoConsoleVariable<int32> CVarApproachOwnByIdentity(
			TEXT("aps.Stars.ApproachOwnByIdentity"), 1,
			TEXT("Rio 06.10: 1 lets an approach point own the system the materializer stands for its catalogue star by identity ")
			TEXT("(that system's star carries the registry name), not only by place (3 star radii): it is never released for its ")
			TEXT("own system's sphere and crossfades into it once on it; when that star stands more than 3 radii off the catalogue ")
			TEXT("place (a registry skew) the point slides onto it over a second ('re-anchored', a warning) and stays where it stood ")
			TEXT("once the system is gone, its glide taking it back to its twin. 0: by place only (the 06.10 way: a skewed point is ")
			TEXT("released as 'inside another system's sphere' and a far glyph stands in somewhere else)."));
		TAutoConsoleVariable<int32> CVarApproachRetakeAtExact(
			TEXT("aps.Stars.ApproachRetakeAtExact"), 1,
			TEXT("Rio 06.10: 1 takes a point inside its own standing system (after the map, a teleport or a load) at the exact ")
			TEXT("place, where its twin is hidden anyway, and at the crossfade's current value: no full point over a grown sphere. ")
			TEXT("0: taken where the twin is drawn with the glide pinned at 1 (the 06.10 way: drawn ~1550 px off the sphere, and a ")
			TEXT("quarter-second flash)."));
		TAutoConsoleVariable<int32> CVarApproachTrace(
			TEXT("aps.Stars.ApproachTrace"), 0,
			TEXT("Rio 06.10 (diagnostics, visual runs only): 1 logs one '[APS.StarTrace] f=... tgt=...' line a frame for the traced ")
			TEXT("star (aps.Stars.ApproachTraceTarget) and event lines (target, take, release, crossfade steps, standup, re-anchor, ")
			TEXT("map, distance crossings, pays). 0: off (one console read a frame)."));
		TAutoConsoleVariable<FString> CVarApproachTraceTarget(
			TEXT("aps.Stars.ApproachTraceTarget"), TEXT("-1"),
			TEXT("Rio 06.10: the catalogue index aps.Stars.ApproachTrace follows. -1: the course (the autopilot target, else the ")
			TEXT("HUD's selected contact) when it is a galaxy star, else the nearest approach point, else the galaxy star the ")
			TEXT("materializer stands."));
		TAutoConsoleVariable<int32> CVarApproachTraceShots(
			TEXT("aps.Stars.ApproachTraceShots"), 0,
			TEXT("Rio 06.10 (visual runs only, ~150 ms of game thread a shot): with aps.Stars.ApproachTrace each trace event takes ")
			TEXT("this many consecutive frames, even ones with the UI, odd ones the scene only, to Saved/Screenshots/ShipDrive/")
			TEXT("<aps.Stars.ApproachTraceLabel>_tE<k>-<event>_f<j>.png. 0: none."));
		TAutoConsoleVariable<FString> CVarApproachTraceLabel(
			TEXT("aps.Stars.ApproachTraceLabel"), TEXT("star"),
			TEXT("Rio 06.10: the file name prefix of aps.Stars.ApproachTraceShots."));
		// Rio 06.10 (star approach v2, stage B: "on the way out the point vanishes and a fallback glyph stands in", "the course
		// star must be a point long before the twin's error shows", the F10 map round trip). Every switch at 0 is stage A.
		TAutoConsoleVariable<int32> CVarApproachKeepWhileStanding(
			TEXT("aps.Stars.ApproachKeepWhileStanding"), 1,
			TEXT("Rio 06.10 (stage B, change 7): 1 keeps an approach point past its release radius while its own system stands (the ")
			TEXT("materializer's star by identity, or by place), so leaving a system the point is still there in the frame the ")
			TEXT("system goes; it is released then, on its twin. 0: released past the take radius (stage A: a far glyph stood in)."));
		TAutoConsoleVariable<float> CVarApproachCourseMaxLy(
			TEXT("aps.Stars.ApproachCourseMaxLy"), 500.0f,
			TEXT("Rio 06.10 (stage B, change 9): the course star (autopilot target, else the HUD's course, else in STELLAR or the ")
			TEXT("star drive the star along the flight direction) gets its approach point where its twin's error would reach ")
			TEXT("aps.Stars.ApproachCoursePixels, at most this many light years out; one such point, outside the 8. 0: off (stage A)."));
		TAutoConsoleVariable<float> CVarApproachCoursePixels(
			TEXT("aps.Stars.ApproachCoursePixels"), 0.5f,
			TEXT("Rio 06.10 (aps.Stars.ApproachCourseMaxLy): the twin's error in pilot pixels at which the course star is taken."));
		TAutoConsoleVariable<int32> CVarApproachCourseRetired(
			TEXT("aps.Stars.ApproachCourseRetired"), 2,
			TEXT("Rio 06.10 (aps.Stars.ApproachCourseMaxLy): a star that stops being the course keeps its point and glide law until ")
			TEXT("it stands on its twin ('retired'); at most this many retired points beyond the take radius, the one with the ")
			TEXT("smallest slide hands back to its twin over aps.Stars.ApproachHandBackSeconds. 0: hand back at once."));
		TAutoConsoleVariable<float> CVarApproachHandBackSeconds(
			TEXT("aps.Stars.ApproachHandBackSeconds"), 0.4f,
			TEXT("Rio 06.10: seconds over which a retired approach point slides onto its twin before it is released. 0: a cut."));
		// Rio 07.10 (offscreen sb-boresight-fix-x4, diagnosed): not the take/hand-back (every take at 0 px, every release at
		// glide 1, no hand-back). The flight direction was a 0.25 s average refreshed four times a second, so after each turn
		// the star now ahead was dropped as "passed" and one behind latched: the star ahead flew by as a bare twin (375 px at
		// 0.1 ly); above 2000 ly/s the boresight paused. Fixed behind aps.Stars.ApproachBoresightFrameHeading/RequeryDeg/
		// AdaptiveCadence/NavHold/KeepNeeded (below); sb-boresight-fix-x6 passes every row with them: on by default.
		TAutoConsoleVariable<int32> CVarApproachBoresight(
			TEXT("aps.Stars.ApproachBoresight"), 1,
			TEXT("Rio 06.10 (aps.Stars.ApproachCourseMaxLy): without an autopilot target that is a galaxy star (a cluster system or ")
			TEXT("body target leaves the boresight on: the course is then the star along the flight direction), in STELLAR or the ")
			TEXT("star drive, the drawn star nearest the flight direction (aps.Stars.ApproachBoresightDeg cone, out to the course radius) is the course ")
			TEXT("when the HUD's course is not in that cone. 0: off."));
		TAutoConsoleVariable<float> CVarApproachBoresightDeg(
			TEXT("aps.Stars.ApproachBoresightDeg"), 1.0f,
			TEXT("Rio 06.10 (aps.Stars.ApproachBoresight): the cone's half angle in degrees (dropped beyond three times it)."));
		TAutoConsoleVariable<int32> CVarApproachBoresightMaxStars(
			TEXT("aps.Stars.ApproachBoresightMaxStars"), 20000,
			TEXT("Rio 06.10 (aps.Stars.ApproachBoresight): at most this many stars tested per cone query (reported as capped)."));
		// Rio 07.10 (aps.Stars.ApproachBoresight fixes; each 0 is the 06.10 night behaviour exactly).
		TAutoConsoleVariable<int32> CVarApproachBoresightFrameHeading(
			TEXT("aps.Stars.ApproachBoresightFrameHeading"), 1,
			TEXT("Rio 07.10 (aps.Stars.ApproachBoresight): 1 takes the flight direction and speed from the pilot's travel of the last ")
			TEXT("frame (over a kilometre; a step twenty times faster than the last one is a jump and skipped) and queries the boresight ")
			TEXT("in that frame when the direction turned past aps.Stars.ApproachBoresightRequeryDeg or the latched star fell behind; ")
			TEXT("below a kilometre a frame the 0.25 s sample stands. 0: the 0.25 s sample only (stale after a turn)."));
		TAutoConsoleVariable<float> CVarApproachBoresightRequeryDeg(
			TEXT("aps.Stars.ApproachBoresightRequeryDeg"), 0.5f,
			TEXT("Rio 07.10 (aps.Stars.ApproachBoresightFrameHeading): a turn of more than this many degrees since the last query (and ")
			TEXT("since the last forced one) queries in the same frame. 0: only a latched star fallen behind does."));
		TAutoConsoleVariable<int32> CVarApproachBoresightAdaptiveCadence(
			TEXT("aps.Stars.ApproachBoresightAdaptiveCadence"), 1,
			TEXT("Rio 07.10 (aps.Stars.ApproachBoresight): 1 shortens the query cadence (and the next resolve) so one cadence of travel ")
			TEXT("covers at most the course radius, down to 1/120 s, so the boresight keeps running in the star drive. 0: paused there."));
		TAutoConsoleVariable<int32> CVarApproachBoresightNavHold(
			TEXT("aps.Stars.ApproachBoresightNavHold"), 1,
			TEXT("Rio 07.10 (aps.Stars.ApproachBoresight): 1 keeps the HUD's course as the course, once in the cone, until it is three ")
			TEXT("cones (at most 85 deg) off the flight direction. 0: in or out at one cone."));
		TAutoConsoleVariable<int32> CVarApproachBoresightKeepNeeded(
			TEXT("aps.Stars.ApproachBoresightKeepNeeded"), 1,
			TEXT("Rio 07.10 (aps.Stars.ApproachBoresight): 1 keeps the latched star while the flight line passes it within its course ")
			TEXT("take radius (it needs its point there): not dropped as turned away, not replaced by a farther candidate. 0: by angle."));
		TAutoConsoleVariable<int32> CVarApproachOwnTake(
			TEXT("aps.Stars.ApproachOwnTake"), 1,
			TEXT("Rio 06.10 (stage B): the GPU star of the system the materializer stands gets its approach point at once (at the ")
			TEXT("exact place, outside the 8), so its far glyph is never needed; the glyph stays a safety net with a warning. 0: off."));
		TAutoConsoleVariable<int32> CVarApproachScanBounded(
			TEXT("aps.Stars.ApproachScanBounded"), 1,
			TEXT("Rio 06.10 (stage B): 1 scans for new approach points only four times a second while the camera is too fast to ")
			TEXT("take one (stage A also scanned every tenth of the take radius travelled). Course and retired points never drive ")
			TEXT("the scan in either case. 0: the stage A gate."));
		TAutoConsoleVariable<int32> CVarApproachMapGain(
			TEXT("aps.Stars.ApproachMapGain"), 1,
			TEXT("Rio 06.10 (stage B): 1 gives the approach points the strategic map's hold (aps.Stars.MapPointHoldRadii) like the ")
			TEXT("level sets around them, so a kept or course point is as bright as its neighbours on the map. 0: their own scale."));
		TAutoConsoleVariable<float> CVarApproachFadeHeadroom(
			TEXT("aps.Stars.ApproachFadeHeadroom"), 0.0f,
			TEXT("Rio 06.10 (stage B, change 10, prepared): while an approach point crossfades its visibility is capped at this many ")
			TEXT("times aps.Stars.GpuPointMaxPixel of its unclamped value, so the fade dims a clamp-saturated point. 0: off."));
		TAutoConsoleVariable<int32> CVarApproachExposureLog(
			TEXT("aps.Stars.ApproachExposureLog"), 1,
			TEXT("Rio 06.10 (diagnostics): logs the view's pre-exposure once after an approach point is taken or starts its ")
			TEXT("crossfade (at most every 10 s, 20 a session; scene captures skipped). 0: off."));
		/** Rio 06.10 (aps.Stars.ApproachOwnByIdentity): a re-anchored approach point slides onto its standing star (and back) this long. */
		constexpr double ReanchorSeconds = 1.0;
		constexpr int32 MaxApproachPoints = 8;
		/** A new approach point only for a camera that takes at least this long to cross the take radius (the drive passes
		 * stars by the dozen a second; a point taken already stays until it is back on its twin). */
		constexpr double ApproachTakeSeconds = 1.0;
		constexpr double ApproachReleaseFactor = 1.1;
		constexpr double ApproachScanSeconds = 0.25;
		constexpr double ApproachLogSeconds = 0.5;
		/** The crossfade's fastest step per second: a sphere that shows up late still takes over within a quarter second. */
		constexpr float ApproachFadeRate = 4.0f;
		/** The one-point set's packing box (local units, ~0.05 AU): its quantization step never reaches the inverse square. */
		constexpr float ApproachBoxLocal = 1.0e-3f;
		constexpr double LightYearCm = 9.4607304725808e17;
		constexpr double AstronomicalUnitCm = 1.495978707e13;

		/** Glow statistics need a few million samples at most (the first ones of the layer: a uniform sample). */
		constexpr int32 GlowSampleLimit = 2000000;
		constexpr int32 GlowMapResolution = 256;
		constexpr int32 ResolveChunk = 65536;
		constexpr int32 PaletteClasses = 14;
		/**
		 * Menu calibration (first estimate, to be checked on frames): integrated pre-exposed pixel value of a reference star
		 * (intensity 1 = G-type flux with a white colour, at the reference budget's brightness) seen from three galaxy
		 * radii, for a 2560 px, 50 degree view. With the historic star mix (mean intensity ~3.9) the glow's mean surface
		 * value over a face-on disk is ~0.07. aps.Stars.GpuPointIntensity / aps.Stars.GalaxyGlowIntensity scale it live.
		 */
		constexpr double ReferencePixelValue = 8.0;
		constexpr double ReferencePixelTangent = 3.64e-4;
		/**
		 * Gameplay calibration (first estimate): integrated pre-exposed pixel value of a GPU point drawn at an ISM star of
		 * a G0 dwarf (aps.Galaxy.GpuStarsFirst 0 overlay), i.e. of the ISM glyph it must match. The gameplay ISM glyphs
		 * keep their brightness at any distance; the GPU points do the same inside one galaxy radius (brightness floor).
		 */
		// Rio 03.10 ("in the gameplay sky too"): 12x the glyph-match estimate (2.0). In orbit frames 1x was invisible
		// (GPU stats: 37k pixels written, all below display), 20x a dense grain and glow; 12x keeps the field and the
		// galaxy glow visible behind the ISM stars without outshining them. Gameplay only: the menu keeps its own value.
		constexpr double GameplayOverlayPixelValue = 24.0;
		/**
		 * Gameplay precision: nested cubes around the home system (half size halves per level), each a point set with its
		 * own 16-bit packing box, so a star's quantisation stays below ~3e-5 of its distance (0.1 px) at any distance.
		 */
		constexpr int32 GameplayLevels = 6;
		constexpr double ExclusionScanSeconds = 0.5;

		enum class ELayerMode : uint8
		{
			None,
			Menu,
			Gameplay
		};

		/** What the CVars ask for in one presentation; a change rebuilds the layer. */
		struct FRequest
		{
			bool bPoints = false;
			bool bGlow = false;
			int32 PointCount = 0;
			int32 FirstOrdinal = -1;
			/** Gameplay only: aps.Stars.GameplayGpuLaw / GameplayGpuGain / GameplayGlowFloor. */
			int32 Law = 0;
			float Gain = 1.0f;
			float GlowFloor = 0.0f;

			bool operator==(const FRequest& Other) const
			{
				return bPoints == Other.bPoints && bGlow == Other.bGlow && PointCount == Other.PointCount
					&& FirstOrdinal == Other.FirstOrdinal && Law == Other.Law && Gain == Other.Gain
					&& GlowFloor == Other.GlowFloor;
			}
			bool operator!=(const FRequest& Other) const { return !(*this == Other); }
			bool Wants() const { return bPoints || bGlow; }
		};

		/** The last continuous preview presentation (copied every frame). */
		struct FPresentationInput
		{
			TWeakObjectPtr<AGalaxy> Galaxy;
			FVector ObserverCm = FVector::ZeroVector;
			double RenderCmPerPhysicalCm = 1.0;
			double FarEnvelopeCm = 0.0;
			FVector HomeExclusionCenterCm = FVector::ZeroVector;
			double HomeExclusionRadiusCm = 0.0;
			bool bValid = false;
		};

		/** The gameplay sky's galaxy and home system (copied every frame). */
		struct FGameplayInput
		{
			TWeakObjectPtr<AGalaxy> Galaxy;
			TWeakObjectPtr<AStarSystem> Home;
			double NextExclusionScanSeconds = 0.0;
			bool bValid = false;
		};

		/** Rio 05.10 night: how an approach star is drawn now (logged when it changes). */
		enum class EApproachLook : uint8
		{
			Point,
			Crossfade,
			Sphere
		};

		/** Rio 06.10 (aps.Stars.ApproachOwnByIdentity): where an approach point's exact place stands. */
		enum class EApproachAnchor : uint8
		{
			/** The catalogue place (always, without a registry skew). */
			Catalogue,
			/** Sliding onto, then staying on, its standing star (owned by identity, more than 3 radii away). */
			OwnStar,
			/** Its star went: left where the star stood (AnchorStartSeconds < 0), or sliding back once owned by place again. */
			Back
		};

		/** Rio 05.10 night: a drawn GPU star near the camera with a one-point set of its own (UpdateApproachPoints). */
		struct FApproachPoint
		{
			int64 CatalogIndex = INDEX_NONE;
			int32 Ordinal = INDEX_NONE;
			int32 Level = 0;
			/** The level set that draws its quantized twin (index into PointSets) and the twin as that set's shader places it. */
			int32 TwinSet = INDEX_NONE;
			FVector3f TwinLocal = FVector3f::ZeroVector;
			float TwinRadiusLocal = 0.0f;
			APSStarRenderer::FHandle Handle = 0;
			FVector ExactLocal = FVector::ZeroVector;
			/** Where the way in from the twin to the exact place began (the take radius, or nearer for a late take), local units. */
			double GlideFromLocal = 1.0;
			double RadiusCm = 0.0;
			float Intensity = 0.0f;
			/** The crossfade into the star's sphere (1: the point alone) and the visibility last sent. */
			float Fade = 1.0f;
			float AppliedVisibility = 1.0f;
			/** Where it was last placed (catalogue-local; origin shifts re-place it with the layer) and the transform sent. */
			FVector DrawLocal = FVector::ZeroVector;
			FTransform PushedTransform = FTransform::Identity;
			EApproachLook Look = EApproachLook::Point;
			bool bRelease = false;
			/** Rio 06.10 (aps.Stars.ApproachOwnByIdentity): the catalogue place (ExactLocal leaves it only for a re-anchor). */
			FVector CatalogueLocal = FVector::ZeroVector;
			EApproachAnchor Anchor = EApproachAnchor::Catalogue;
			FVector AnchorFromLocal = FVector::ZeroVector;
			double AnchorStartSeconds = 0.0;
			/**
			 * Rio 06.10 (star approach v2, stage B, change 9): > 0 a course or retired point, released past
			 * max(take radius, this) x ApproachReleaseFactor; bCourse the current course star's point (else retired while > 0).
			 */
			double CourseTakeCm = 0.0;
			bool bCourse = false;
			/** Rio 06.10 (aps.Stars.ApproachKeepWhileStanding): the 'kept' line was logged (its release then says why). */
			bool bKeptLogged = false;
			/** Kept past its release radius this frame by its standing system (trace keep=). */
			bool bKeptNow = false;
			/** Its own system stands this frame (identity or place, not the home): never handed back by the retired pool. */
			bool bOwnStanding = false;
			/** Rio 06.10 (aps.Stars.ApproachHandBackSeconds): a hand-back onto the twin (world seconds; < 0 none), its start glide. */
			double HandBackStart = -1.0;
			double HandBackFromGlide = 1.0;
			const TCHAR* HandBackReason = nullptr;
			/** Rio 06.10 (aps.Stars.ApproachMapGain): the strategic map hold its IntensityScale carries now (1: its own scale). */
			float AppliedGain = 1.0f;
			/** The pilot's distance to its exact place last frame, cm (the budget counts). */
			double LastPilotCm = 0.0;
			/** Rio 06.10 (aps.Stars.SystemGlare): the glare serial its visibility was last sent with. */
			uint32 GlareSerial = 0;
		};

		/**
		 * Rio 06.10 (star approach v2, aps.Stars.ApproachOwnByIdentity): the galaxy star the materializer stands now. Its system
		 * actor is the materializer's own (no accessor); its star carries the registry name (FAPSSystemMaterializer::Begin),
		 * which is how a standing system is known as this star's own wherever the registry put it.
		 */
		struct FActiveGalaxySystem
		{
			int64 CatalogIndex = INDEX_NONE;
			FName StarName;
		};

		/** Rio 06.10 (stage B, change 9): the course star's twin as its level set draws it (refilled on a course change). */
		struct FCourseTwin
		{
			bool bValid = false;
			int64 CatalogIndex = INDEX_NONE;
			/** Drawn by the GPU layer and lit (a dark code is not drawn: no point either). */
			bool bGpu = false;
			int32 Ordinal = INDEX_NONE;
			int32 Level = 0;
			int32 TwinSet = INDEX_NONE;
			FVector ExactLocal = FVector::ZeroVector;
			double TwinOffsetLocal = 0.0;
			/** Inside a foreign system's sphere at the last try: tried again from RetrySeconds (world seconds). */
			bool bBlocked = false;
			double RetrySeconds = 0.0;
		};

		/** Rio 06.10 (stage B): FindActiveGalaxySystem cached by its key (registry, active index, its galaxy index and name). */
		struct FActiveCache
		{
			const FAPSStarSystems* Registry = nullptr;
			int32 ActiveIndex = -2;
			int64 GalaxyIndex = INDEX_NONE;
			FString Name;
			FActiveGalaxySystem Value;
			/** Its GPU twin (aps.Stars.ApproachOwnTake): drawn by the layer and lit, its ordinal and exact catalogue place. */
			bool bGpu = false;
			int32 Ordinal = INDEX_NONE;
			FVector ExactLocal = FVector::ZeroVector;
			/** The own take's failures: the first one (world seconds; < 0 none), the warning given. */
			double OwnFailSince = -1.0;
			double OwnRetrySeconds = 0.0;
			bool bOwnFailLogged = false;
		};

		/** What the plugin currently holds for the active galaxy (one layer at a time). */
		struct FLayerState
		{
			ELayerMode Mode = ELayerMode::None;
			TWeakObjectPtr<AGalaxy> Galaxy;
			/** Every point set of the layer (one in the menu, one per precision level in gameplay). */
			TArray<APSStarRenderer::FHandle> PointSets;
			TArray<APSStarRenderer::FPointSetDesc> PointDescs;
			/** The points of every set (the menu card counts them with the ISM stars). */
			int32 PointCount = 0;
			/** Rio 06.10 (strategic map GALAXY): the base IntensityScale of every point set, without the map hold. */
			float PointIntensityScale = 0.0f;
			/** The galaxy radius in catalogue-local units (the map hold's unit of distance). */
			float RadiusLocal = 0.0f;
			/** The map hold's gain last sent to the point sets (1: none, the pilot's sky). */
			float AppliedMapGain = 1.0f;
			/** Menu: when the layer was registered (its fade-in), the eased glow share of the screen and what was last sent. */
			double ReadySeconds = 0.0;
			double LastPresentSeconds = 0.0;
			float GlowFocus = 1.0f;
			float PointFocus = 1.0f;
			float AppliedPointVisibility = 1.0f;
			float AppliedGlowVisibility = 1.0f;
			/** The screen's extra brightness LOD (aps.Stars.MenuMinPixel*), eased, and the scale last sent (with the share). */
			float MinPixelFocus = 1.0f;
			float AppliedMinPixelScale = 1.0f;
			FTransform PushedTransform = FTransform::Identity;
			FRequest Request;
			uint32 CatalogKey = 0;
			/** Gameplay: home system first, then the nearest other materialized systems (local centre xyz, radius w). */
			TArray<FVector4f> Exclusions;
			/**
			 * Gameplay (Rio 05.10 night, approach points): each set's precision level, the levels' half sizes around the home,
			 * the GPU ordinals, and the palette luminances and G0 reference the points were packed with (BuildOnWorker).
			 */
			TArray<int32> PointLevels;
			TArray<float> LevelHalf;
			FVector3f HomeLocal = FVector3f::ZeroVector;
			int32 FirstOrdinal = 0;
			int32 PointOrdinals = 0;
			TArray<float> PaletteLuminance;
			float G0Intensity = 1.0f;
			/** Rio 05.10 night: the approach points, the last scan for new ones and the camera's last place (its speed). */
			TArray<FApproachPoint> Approach;
			FVector LastApproachScanLocal = FVector::ZeroVector;
			double LastApproachScanSeconds = 0.0;
			/** Rio 06.10 (stage B, change 12): the pilot's place (the camera's while the view is the pilot's). */
			FVector LastApproachCameraLocal = FVector::ZeroVector;
			double LastApproachCameraSeconds = 0.0;
			double NextApproachLogSeconds = 0.0;
			/** Rio 06.10 (stage B, change 12): the eyes were detached last frame; the pilot's pixel tangent of the last frame. */
			bool bLastDetached = false;
			double LastPilotPixelTangent = 0.0;
			/** Rio 06.10 (stage B, change 9): the course star's twin; the active system's cache (both reset with the layer). */
			FCourseTwin CourseTwin;
			FActiveCache ActiveCache;
			/** Rio 06.10 (aps.Stars.ApproachMapGain): the approach points carry the map hold now (the flip is logged once). */
			bool bApproachGainOn = false;
		};

		/**
		 * Rio 03.10 ("every star must be reachable, only the nearest ones loaded"): the drawn stars of the gameplay layer
		 * (its ISM prefix and GPU points) in a uniform grid of the catalogue box, 16 bytes a star, for nearest-star queries.
		 * The catalogue index of a star is its ordinal through the nested order (O(1)), so only ordinals are kept.
		 */
		struct FStarIndex
		{
			FVector3f Min = FVector3f::ZeroVector;
			float CellSize = 1.0f;
			int32 Resolution = 0;
			/** Resolution^3 + 1 offsets into Positions/Ordinals, which are sorted by cell. */
			TArray<int32> CellStart;
			TArray<FVector3f> Positions;
			TArray<int32> Ordinals;
			APSCanonicalStellarProjection::FNestedCatalogPermutation Order;
			/** The farthest indexed star from the galaxy centre (the local origin), local units: charted space. */
			float BoundRadius = 0.0f;

			FIntVector CellOf(const FVector3f& Local) const
			{
				const FVector3f Cell = (Local - Min) / CellSize;
				return FIntVector(FMath::Clamp(FMath::FloorToInt(Cell.X), 0, Resolution - 1),
					FMath::Clamp(FMath::FloorToInt(Cell.Y), 0, Resolution - 1),
					FMath::Clamp(FMath::FloorToInt(Cell.Z), 0, Resolution - 1));
			}
			int32 LinearOf(const FIntVector& Cell) const
			{
				return (Cell.Z * Resolution + Cell.Y) * Resolution + Cell.X;
			}
		};
		constexpr int32 IndexResolution = 96;
		constexpr int32 NearRingLimit = 20;

		struct FBuildSpec
		{
			FGalaxyCatalogDescriptor Catalog;
			ELayerMode Mode = ELayerMode::None;
			int32 FirstOrdinal = 0;
			int32 PointCount = 0;
			int32 GlowSamples = 0;
			double EmissionMin = 1.0;
			double EmissionMax = 1.0;
			TArray<FLinearColor> Palette;
			/** Gameplay: the home system in catalogue-local units (centre of the precision levels). */
			FVector3f HomeLocal = FVector3f::ZeroVector;
			int32 Levels = 1;
			/** Gameplay: also build the nearest-star index (prefix ordinals [0, FirstOrdinal) and the points). */
			bool bIndex = false;
		};

		struct FBuildResult
		{
			ELayerMode Mode = ELayerMode::None;
			/** One array and packing box per precision level (the menu: one, the catalogue box). */
			TArray<TArray<APSStarRenderer::FPackedStar>> Points;
			TArray<FBox3f> LevelBounds;
			/** Rio 05.10 night (approach points): the levels' half sizes and their centre, to find a star's level again. */
			TArray<float> LevelHalf;
			FVector3f HomeLocal = FVector3f::ZeroVector;
			APSStarRenderer::FGlowMap GlowMap;
			TArray<FLinearColor> Palette;
			FBox3f Bounds = FBox3f(ForceInit);
			int32 FirstOrdinal = 0;
			int32 PointOrdinals = 0;
			bool bGlowMap = false;
			double Seconds = 0.0;
			TSharedPtr<const FStarIndex, ESPMode::ThreadSafe> Index;
		};

		FPresentationInput GInput;
		FGameplayInput GGameplay;
		FLayerState GLayer;
		/** The menu screen (SetMenuGlowScope), the share shown when the camera's flight to it began, and the flight's
		 * eased progress; the screen's own share is read live from the CVars. */
		EMenuGlowScope GMenuGlowScope = EMenuGlowScope::Far;
		float GMenuGlowFrom = 1.0f;
		float GMenuPointFrom = 1.0f;
		float GMenuMinPixelFrom = 1.0f;
		float GMenuGlowProgress = 1.0f;
		FTSTicker::FDelegateHandle GMenuTicker;
		/**
		 * Rio 06.10 (aps.Stars.MenuGlowContinuous): the camera's flight progress last given (its clock restarts at 0 for every
		 * flight), the eased share of the flight already flown when the current start was taken (the rest maps to 0..1), and
		 * the clock of an ease without a flight (negative: none).
		 */
		float GMenuLastFlightAlpha = 1.0f;
		float GMenuFlightEaseBase = 0.0f;
		double GMenuStillStartSeconds = -1.0;
		/** The shortest camera flight (AAstroGenerator::StartContinuousPreviewTransition clamps it to 0.55..1.6 s). */
		constexpr double MenuStillEaseSeconds = 0.55;

		bool MenuGlowContinuous()
		{
			return CVarMenuGlowContinuous.GetValueOnGameThread() != 0;
		}

		/** The eased progress from the start values to the screen's: the camera's flight or, with aps.Stars.MenuGlowContinuous,
		 * an ease without a flight on its own clock, the same smoothstep. */
		float MenuProgress()
		{
			if (GMenuStillStartSeconds >= 0.0 && MenuGlowContinuous())
			{
				const float T = static_cast<float>(FMath::Clamp(
					(FPlatformTime::Seconds() - GMenuStillStartSeconds) / MenuStillEaseSeconds, 0.0, 1.0));
				return T * T * (3.0f - 2.0f * T);
			}
			return GMenuGlowProgress;
		}

		float ScopeShare(const EMenuGlowScope Scope, const bool bPoints)
		{
			const float Share = bPoints
				? (Scope == EMenuGlowScope::Planet ? CVarMenuPointsPlanet.GetValueOnGameThread()
					: Scope == EMenuGlowScope::Star ? CVarMenuPointsStar.GetValueOnGameThread()
					: Scope == EMenuGlowScope::System ? CVarMenuPointsSystem.GetValueOnGameThread() : 1.0f)
				: (Scope == EMenuGlowScope::Planet ? CVarMenuGlowPlanet.GetValueOnGameThread()
					: Scope == EMenuGlowScope::Star ? CVarMenuGlowStar.GetValueOnGameThread()
					: Scope == EMenuGlowScope::System ? CVarMenuGlowSystem.GetValueOnGameThread() : 1.0f);
			return FMath::Clamp(Share, 0.0f, 1.0f);
		}

		/** The share for this moment of the flight: geometric between the start and the screen's share (1, 0.56, 0.32,
		 * 0.18, 0.1 from GALAXY to PLANET), so it eases like the camera's distance does. */
		float FlightShare(const bool bPoints)
		{
			const float To = ScopeShare(GMenuGlowScope, bPoints);
			const float Progress = MenuProgress();
			if (Progress >= 1.0f)
			{
				return To;
			}
			const float From = FMath::Clamp(bPoints ? GMenuPointFrom : GMenuGlowFrom, 1.0e-3f, 1.0f);
			return FMath::Exp(FMath::Lerp(FMath::Loge(From), FMath::Loge(FMath::Max(To, 1.0e-3f)), Progress));
		}

		float MenuGlowShare() { return FlightShare(false); }
		float MenuPointShare() { return FlightShare(true); }

		float ScopeMinPixel(const EMenuGlowScope Scope)
		{
			if (Scope == EMenuGlowScope::Far)
			{
				return 1.0f;
			}
			const float Scale = Scope == EMenuGlowScope::Planet ? CVarMenuMinPixelPlanet.GetValueOnGameThread()
				: Scope == EMenuGlowScope::Star ? CVarMenuMinPixelStar.GetValueOnGameThread()
				: CVarMenuMinPixelSystem.GetValueOnGameThread();
			return FMath::IsFinite(Scale) ? FMath::Clamp(Scale, 1.0f, 1000.0f) : 1.0f;
		}

		/**
		 * The scale sent with the points: the screen's extra cut times their brightness share, so a point dimmed by the
		 * share is measured against a cut dimmed alike and stays drawn (Rio 05.10: "the points must not disappear").
		 */
		float PointMinPixelScale(const float ExtraCut, const float PointShare)
		{
			return FMath::Clamp(ExtraCut * FMath::Max(PointShare, 0.0f), 0.01f, 1000.0f);
		}

		/** The brightness LOD for this moment of the flight, geometric between the start and the screen's, like the shares. */
		float MenuMinPixelScale()
		{
			const float To = ScopeMinPixel(GMenuGlowScope);
			const float Progress = MenuProgress();
			if (Progress >= 1.0f)
			{
				return To;
			}
			const float From = FMath::Clamp(GMenuMinPixelFrom, 1.0f, 1000.0f);
			return FMath::Exp(FMath::Lerp(FMath::Loge(From), FMath::Loge(To), Progress));
		}

		/**
		 * Rio 06.10 (aps.Stars.MenuGlowContinuous): what the menu layer shows now, the start of the next ease. A layer still
		 * building (RebuildLayer resets its eased values to 1) or not there at all starts from this moment of the current
		 * ease, the value FinishBuild registers it with.
		 */
		float DisplayedMenuShare(const bool bPoints)
		{
			if (GLayer.Mode == ELayerMode::Menu && GLayer.ReadySeconds > 0.0)
			{
				return bPoints ? GLayer.PointFocus : GLayer.GlowFocus;
			}
			return FlightShare(bPoints);
		}

		float DisplayedMenuMinPixel()
		{
			if (GLayer.Mode == ELayerMode::Menu && GLayer.ReadySeconds > 0.0)
			{
				return GLayer.MinPixelFocus;
			}
			return MenuMinPixelScale();
		}

		const TCHAR* MenuScopeName(const EMenuGlowScope Scope)
		{
			return Scope == EMenuGlowScope::Planet ? TEXT("PLANET") : Scope == EMenuGlowScope::Star ? TEXT("STAR")
				: Scope == EMenuGlowScope::System ? TEXT("SYSTEM") : TEXT("FAR");
		}

		/** The gameplay layer's nearest-star index and its galaxy (one at a time, like the layer). */
		TSharedPtr<const FStarIndex, ESPMode::ThreadSafe> GIndex;
		TWeakObjectPtr<AGalaxy> GIndexGalaxy;
		int32 GActiveLayers = 0;
		bool GDelegatesBound = false;
		bool GShiftBound = false;
		/** Rio 06.10 (still ship): the sky moved this frame (re-read after the actors' ticks). */
		bool GSkyMoved = false;
		bool GLoggedShadersOff = false;
		FRequest GLastMenuRequest;
		FRequest GLastGameplayRequest;
		TWeakObjectPtr<const UWorld> GVisibilityWorld;
		float GVisibility = -1.0f;
		/**
		 * Rio 06.10 (aps.Stars.SystemGlare, SetWorldSystemGlare): the stellar view's glare for the gameplay layer, stored even
		 * without a layer (one built later starts from it). The serial moves with every send that changes an approach point's
		 * base visibility (a level send, the own value, the own index), so every point resends in the same frame.
		 */
		TWeakObjectPtr<const UWorld> GSystemGlareWorld;
		float GSystemGlareOthers = 1.0f;
		int64 GSystemGlareOwnIndex = INDEX_NONE;
		float GSystemGlareAppliedOwn = 1.0f;
		uint32 GSystemGlareSerial = 0;
		bool GSystemGlareLoggedOn = false;

		IConsoleVariable* FindVariable(IConsoleVariable*& Cache, const TCHAR* Name)
		{
			if (Cache == nullptr)
			{
				Cache = IConsoleManager::Get().FindConsoleVariable(Name);
			}
			return Cache;
		}

		IConsoleVariable* PointsVariable()
		{
			static IConsoleVariable* Cache = nullptr;
			return FindVariable(Cache, TEXT("aps.Stars.GpuPoints"));
		}

		IConsoleVariable* GlowVariable()
		{
			static IConsoleVariable* Cache = nullptr;
			return FindVariable(Cache, TEXT("aps.Stars.GalaxyGlow"));
		}

		FRequest CurrentRequest(const ELayerMode Mode)
		{
			FRequest Request;
			if (Mode == ELayerMode::None || (Mode == ELayerMode::Gameplay && CVarGameplayGpu.GetValueOnGameThread() == 0))
			{
				return Request;
			}
			const IConsoleVariable* Points = PointsVariable();
			const IConsoleVariable* Glow = GlowVariable();
			Request.bPoints = Points != nullptr && Points->GetInt() != 0;
			Request.bGlow = Glow != nullptr && Glow->GetInt() != 0;
			const int32 Count = Mode == ELayerMode::Gameplay
				? CVarGameplayGpuStars.GetValueOnGameThread() : CVarGpuStars.GetValueOnGameThread();
			Request.PointCount = FMath::Clamp(Count, 0, APSStarRenderer::MaxPointsPerSet);
			Request.FirstOrdinal = FMath::Max(CVarGpuStarsFirst.GetValueOnGameThread(), -1);
			if (Mode == ELayerMode::Gameplay)
			{
				Request.Law = CVarGameplayLaw.GetValueOnGameThread() != 0 ? 1 : 0;
				Request.Gain = FMath::Max(CVarGameplayGain.GetValueOnGameThread(), 0.0f);
				Request.GlowFloor = FMath::Max(CVarGameplayGlowFloor.GetValueOnGameThread(), 0.0f);
			}
			return Request;
		}

		/** The plugin compiles its shaders only with aps.Stars.CompileShaders=1 at start-up; without them nothing can draw. */
		bool ShadersReady()
		{
			if (APSStarRenderer::AreShadersEnabled())
			{
				return true;
			}
			if (!GLoggedShadersOff)
			{
				GLoggedShadersOff = true;
				UE_LOG(LogTemp, Warning, TEXT("[APS.GalaxyGpu] GPU points/glow requested, but the plugin shaders are not compiled ")
					TEXT("(start with aps.Stars.CompileShaders=1, see aps.Stars.GpuReport): no layer is built"));
			}
			return false;
		}

		bool IsRequestedFor(const ELayerMode Mode)
		{
			return CurrentRequest(Mode).Wants() && ShadersReady();
		}

		/** Which presentation a galaxy belongs to (the one whose input names it). */
		ELayerMode ModeOf(const AGalaxy& Galaxy)
		{
			if (GGameplay.bValid && GGameplay.Galaxy.Get() == &Galaxy)
			{
				return ELayerMode::Gameplay;
			}
			if (GInput.bValid && GInput.Galaxy.Get() == &Galaxy)
			{
				return ELayerMode::Menu;
			}
			return ELayerMode::None;
		}

		uint32 CatalogKeyOf(const FGalaxyCatalogDescriptor& Catalog, const double RealScaleLengthFactor = 1.0)
		{
			uint32 Key = GetTypeHash(Catalog.GenerationSeed);
			Key = HashCombine(Key, GetTypeHash(Catalog.ModeledStarCount));
			Key = HashCombine(Key, GetTypeHash(Catalog.RenderedSampleCount));
			Key = HashCombine(Key, GetTypeHash(Catalog.GalaxySize));
			Key = HashCombine(Key, GetTypeHash(Catalog.StarDensity));
			Key = HashCombine(Key, GetTypeHash(Catalog.GalaxyType));
			Key = HashCombine(Key, GetTypeHash(Catalog.GalaxyClass));
			Key = HashCombine(Key, GetTypeHash(Catalog.CatalogHalfExtent));
			Key = HashCombine(Key, GetTypeHash(Catalog.StarPopulation));
			Key = HashCombine(Key, GetTypeHash(Catalog.StarComposition));
			// Rio 05.10 (real scale experiment): a REAL SCALE galaxy is another layer; 1 (every legacy world) keeps the key.
			return RealScaleLengthFactor != 1.0 ? HashCombine(Key, GetTypeHash(RealScaleLengthFactor)) : Key;
		}

		/**
		 * Phase 2's density rule (APSGalaxyMorphology::GetDensityCompensation, menu reference budget): the first N ordinals
		 * of the nested order carry the light of Lambda(N) reference stars, N^0.2 above the reference. The ISM prefix
		 * already follows it; the GPU points get Lambda(First + Count) - Lambda(First), the glow the rest of the population.
		 */
		double ReferenceStars(const int64 Ordinals)
		{
			if (Ordinals <= 0)
			{
				return 0.0;
			}
			return static_cast<double>(Ordinals) * APSGalaxyMorphology::GetDensityCompensation(
				Ordinals, APSGalaxyMorphology::PreviewReferenceBudget).EmissionScale;
		}

		/** Menu: the preview's galaxy points, GetCanonicalRootPositionCm(local) - CanonicalAnchorCm (far envelope applies). */
		bool MakeLocalToPhysical(const AGalaxy& Galaxy, FTransform& OutTransform)
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = Galaxy.CanonicalProjectionFrame;
			const double Scale = Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit;
			if (!Frame.IsFinite() || !FMath::IsFinite(Scale) || !(Scale > 0.0))
			{
				return false;
			}
			OutTransform = FTransform(FQuat::Identity,
				Frame.LayerOriginCanonicalUnits * Frame.CanonicalCmPerUnit - Frame.CanonicalAnchorCm, FVector(Scale));
			return true;
		}

		/**
		 * Gameplay: the ISM point of a catalogue star sits at Frame.ProjectCanonicalUnits(local) inside the galaxy HISM
		 * (AAstroGenerator::ComposeCanonicalStellarProjection writes RenderedProxyBaseTransforms that way), and the sky
		 * draws that HISM in 3D under the generator's root scale (UAPSStellarVisualSubsystem::UpdateGameplayStellarView).
		 * The same affine map times the component's current world transform puts a GPU point on the same direction;
		 * reading the component every time follows origin shifts without touching cached values.
		 */
		bool MakeGameplayLocalToWorld(const AGalaxy& Galaxy, FTransform& OutTransform)
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = Galaxy.CanonicalProjectionFrame;
			const UHierarchicalInstancedStaticMeshComponent* Source = Galaxy.StarMeshInstances;
			const double ProxyScale = Frame.GetEquivalentComponentScale();
			if (!IsValid(Source) || !Frame.IsFinite() || !FMath::IsFinite(ProxyScale) || !(ProxyScale > 0.0))
			{
				return false;
			}
			const FTransform ProxyFromLocal(FQuat::Identity, Frame.ProjectCanonicalUnits(FVector::ZeroVector), FVector(ProxyScale));
			OutTransform = ProxyFromLocal * Source->GetComponentTransform();
			const double Scale = OutTransform.GetMaximumAxisScale();
			return !OutTransform.ContainsNaN() && FMath::IsFinite(Scale) && Scale > 0.0;
		}

		/** FAPSContinuousPreviewFrame::ProjectSphere: the camera sits at the world origin and the ISM points use Offset * Scale. */
		APSStarRenderer::FFarEnvelope MakeEnvelope()
		{
			APSStarRenderer::FFarEnvelope Envelope;
			Envelope.bEnabled = true;
			Envelope.ObserverPhysical = GInput.ObserverCm;
			Envelope.ObserverRender = FVector::ZeroVector;
			Envelope.RenderPerPhysical = GInput.RenderCmPerPhysicalCm;
			Envelope.FarEnvelope = GInput.FarEnvelopeCm;
			return Envelope;
		}

		/** Menu: the home system's extent (the preview hides catalogue stars inside it) in the layer's local units. */
		void ApplyMenuExclusion(const FTransform& LocalToPhysical, APSStarRenderer::FPointSetDesc& Desc)
		{
			const double Scale = LocalToPhysical.GetMaximumAxisScale();
			if (GInput.HomeExclusionRadiusCm > 0.0 && Scale > 0.0)
			{
				Desc.ExclusionCenterLocal = FVector3f(LocalToPhysical.InverseTransformPosition(GInput.HomeExclusionCenterCm));
				Desc.ExclusionRadiusLocal = static_cast<float>(GInput.HomeExclusionRadiusCm / Scale);
			}
			else
			{
				Desc.ExclusionCenterLocal = FVector3f::ZeroVector;
				Desc.ExclusionRadiusLocal = 0.0f;
			}
		}

		/**
		 * Gameplay: the home system and the nearest other materialized systems (AStarSystem actors), each hidden like the
		 * menu's home exclusion: its sphere (StarSystemRadius) with SystemProxyExclusionPadding, in local units.
		 * Rio 05.10 night: these spheres act on the level sets only. An approach point (UpdateApproachPoints) is a set of its
		 * own without them: its star stays a point inside its own system until the disc takes over, and its twin in the level
		 * set is hidden by a small sphere of its own (ApplySetExclusions).
		 */
		TArray<FVector4f> ScanGameplayExclusions(UWorld* World, const FTransform& LocalToWorld)
		{
			TArray<FVector4f> Spheres;
			const double Scale = LocalToWorld.GetMaximumAxisScale();
			if (World == nullptr || !(Scale > 0.0))
			{
				return Spheres;
			}
			const AStarSystem* Home = GGameplay.Home.Get();
			const FVector HomeLocation = IsValid(Home) ? Home->GetActorLocation() : FVector::ZeroVector;
			// Rio 06.10 (still ship): while a fast REAL SCALE ship owes its travel only the sky moves (the galaxy component, so
			// LocalToWorld): a system's sphere goes where the sky draws it (SkyPlace: a system riding with the sky where it is,
			// any other at its world place + the sky offset).
			TArray<TPair<double, FVector4f>> Found;
			for (TActorIterator<AStarSystem> It(World); It; ++It)
			{
				const AStarSystem* System = *It;
				if (!IsValid(System) || !(System->StarSystemRadius > 0.0) || !FMath::IsFinite(System->StarSystemRadius))
				{
					continue;
				}
				const FVector Center = LocalToWorld.InverseTransformPosition(UAPSWorldOriginSubsystem::SkyPlace(*System));
				const double Radius = System->StarSystemRadius * APSCanonicalStellarProjection::SystemProxyExclusionPadding / Scale;
				const double Order = System == Home ? -1.0 : FVector::DistSquared(System->GetActorLocation(), HomeLocation);
				Found.Emplace(Order, FVector4f(FVector3f(Center), static_cast<float>(Radius)));
			}
			Found.Sort([](const TPair<double, FVector4f>& A, const TPair<double, FVector4f>& B) { return A.Key < B.Key; });
			for (const TPair<double, FVector4f>& Entry : Found)
			{
				if (Spheres.Num() >= APSStarRenderer::MaxExclusionSpheres)
				{
					break;
				}
				Spheres.Add(Entry.Value);
			}
			return Spheres;
		}

		bool SameSpheres(const TArray<FVector4f>& A, const TArray<FVector4f>& B)
		{
			if (A.Num() != B.Num())
			{
				return false;
			}
			for (int32 Index = 0; Index < A.Num(); ++Index)
			{
				const float Tolerance = 1.0e-4f * FMath::Max(FMath::Abs(A[Index].W), 1.0f);
				if (!A[Index].Equals(B[Index], Tolerance))
				{
					return false;
				}
			}
			return true;
		}

		void ApplyGameplayExclusions(const TArray<FVector4f>& Spheres, APSStarRenderer::FPointSetDesc& Desc)
		{
			Desc.ExclusionCenterLocal = Spheres.Num() > 0 ? FVector3f(Spheres[0].X, Spheres[0].Y, Spheres[0].Z) : FVector3f::ZeroVector;
			Desc.ExclusionRadiusLocal = Spheres.Num() > 0 ? Spheres[0].W : 0.0f;
			Desc.ExtraExclusionSpheresLocal.Reset();
			for (int32 Index = 1; Index < Spheres.Num(); ++Index)
			{
				Desc.ExtraExclusionSpheresLocal.Add(Spheres[Index]);
			}
		}

		/**
		 * Rio 05.10 night: one level set's spheres: the scan's (home first) with the quantized twins of the set's approach
		 * points right after the home, so the cap (MaxExclusionSpheres) drops a far system rather than a twin.
		 */
		void ApplySetExclusions(const int32 SetIndex, APSStarRenderer::FPointSetDesc& Desc)
		{
			TArray<FVector4f> Spheres = GLayer.Exclusions;
			int32 Slot = FMath::Min(Spheres.Num(), 1);
			// Rio 06.10 (stage B, change 9): the course star's twin first, so a full set never drops the one the pilot flies to.
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				if (Point.bCourse && Point.TwinSet == SetIndex && Point.Handle != 0)
				{
					Spheres.Insert(FVector4f(Point.TwinLocal, Point.TwinRadiusLocal), Slot++);
				}
			}
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				if (!Point.bCourse && Point.TwinSet == SetIndex && Point.Handle != 0)
				{
					Spheres.Insert(FVector4f(Point.TwinLocal, Point.TwinRadiusLocal), Slot++);
				}
			}
			if (Spheres.Num() > APSStarRenderer::MaxExclusionSpheres)
			{
				Spheres.SetNum(APSStarRenderer::MaxExclusionSpheres);
			}
			ApplyGameplayExclusions(Spheres, Desc);
		}

		void PushSetExclusions(const int32 SetIndex)
		{
			if (GLayer.PointSets.IsValidIndex(SetIndex) && GLayer.PointDescs.IsValidIndex(SetIndex))
			{
				ApplySetExclusions(SetIndex, GLayer.PointDescs[SetIndex]);
				APSStarRenderer::UpdatePointSet(GLayer.PointSets[SetIndex], GLayer.PointDescs[SetIndex]);
			}
		}

		/** Rio 05.10 night: the approach points follow the layer (origin shifts, the sky offset) from where they stand. */
		void PushApproachTransforms(const FTransform& LocalToWorld)
		{
			for (FApproachPoint& Point : GLayer.Approach)
			{
				if (Point.Handle != 0)
				{
					Point.PushedTransform = FTransform(FQuat::Identity, Point.DrawLocal) * LocalToWorld;
					APSStarRenderer::SetTransform(Point.Handle, Point.PushedTransform);
				}
			}
		}

		/** 14 spectral classes x 10 subclasses of UStarGenerator::GetStarColor (the plugin normalises them to unit luminance). */
		TArray<FLinearColor> MakePalette()
		{
			TArray<FLinearColor> Palette;
			Palette.Init(FLinearColor::White, PaletteClasses * 10);
			for (int32 Class = 0; Class < PaletteClasses; ++Class)
			{
				for (int32 Subclass = 0; Subclass < 10; ++Subclass)
				{
					Palette[Class * 10 + Subclass] = UStarGenerator::GetStarColor(static_cast<ESpectralClass>(Class), Subclass);
				}
			}
			return Palette;
		}

		/** Same weights as the plugin's palette normalisation. */
		float Luminance(const FLinearColor& Color)
		{
			return 0.2126f * FMath::Max(Color.R, 0.0f) + 0.7152f * FMath::Max(Color.G, 0.0f) + 0.0722f * FMath::Max(Color.B, 0.0f);
		}

		uint8 ColorIndexFor(const FGalaxyCatalogStarRecord& Record)
		{
			const int32 Class = FMath::Clamp(static_cast<int32>(Record.SpectralClass), 0, PaletteClasses - 1);
			return static_cast<uint8>(Class * 10 + FMath::Clamp(Record.SpectralSubclass, 0, 9));
		}

		/**
		 * Menu: the ISM point's relative flux, so both paths rank stars alike: CalculateEmission(luminosity x 25) clamped to
		 * [EmissionMin, EmissionMax] times the physical disk area (GetFarStarVisualEmission keeps emission x radius^2),
		 * with the galaxy POPULATION size factor and the colour's own luminance (the palette is normalised).
		 * Without colour: G 1, O ~405, B ~115, A ~22, F ~2.7, K ~0.6, M ~0.18.
		 */
		float MenuIntensityFor(const FGalaxyCatalogStarRecord& Record, const float ColorLuminance, const double EmissionMin,
			const double EmissionMax)
		{
			const double Radius = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
				* Record.RadiusScale;
			const double Luminosity = APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
				* APSGalaxyMorphology::GetRadiusScaleLuminosity(Record.RadiusScale) * 25.0;
			const double Emission = FMath::Clamp(Luminosity, EmissionMin, EmissionMax);
			return static_cast<float>(Emission * Radius * Radius / EmissionMin * ColorLuminance);
		}

		/**
		 * Gameplay: the ISM sky's own ranking (APSGameplayStarAppearance, the gameplay point material's luminosity gain,
		 * 0.5..1.2) with the colour's luminance, relative to a G0 dwarf, so an overlay matches the glyphs class by class.
		 */
		float GameplayIntensityFor(const FGalaxyCatalogStarRecord& Record, const float ColorLuminance, const float G0Intensity)
		{
			const float Gain = APSGameplayStarAppearance::GetLuminosityGain(
				APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
				* APSGalaxyMorphology::GetRadiusScaleLuminosity(Record.RadiusScale));
			return Gain * ColorLuminance / FMath::Max(G0Intensity, 1.0e-6f);
		}

		/** Young (hot) stars trace the dust of the glow; the rest weakly. */
		float DustWeightFor(const ESpectralClass SpectralClass)
		{
			return SpectralClass == ESpectralClass::O || SpectralClass == ESpectralClass::B || SpectralClass == ESpectralClass::A
				|| SpectralClass == ESpectralClass::PS ? 1.0f : 0.3f;
		}

		/** Face-on optical depth of the dust lanes by type (first estimates, to tune on frames). */
		float DustOpacityFor(const EGalaxyType Type)
		{
			switch (Type)
			{
			case EGalaxyType::Elliptical: return 0.0f;
			case EGalaxyType::Lenticular: return 0.15f;
			case EGalaxyType::Spiral: return 1.2f;
			case EGalaxyType::BarredSpiral: return 1.2f;
			case EGalaxyType::Irregular: return 0.6f;
			case EGalaxyType::Peculiar: return 0.8f;
			default: return 0.5f;
			}
		}

		/** Background task: resolve, pack (per precision level) and build the glow map. Pure catalogue reads on copies. */
		FBuildResult BuildOnWorker(FBuildSpec&& Spec)
		{
			const double Started = FPlatformTime::Seconds();
			FBuildResult Result;
			Result.Mode = Spec.Mode;
			Result.Palette = MoveTemp(Spec.Palette);
			Result.FirstOrdinal = Spec.FirstOrdinal;
			Result.PointOrdinals = Spec.PointCount;
			const FGalaxyCatalogDescriptor& Catalog = Spec.Catalog;
			const FVector3f HalfExtent(Catalog.CatalogHalfExtent.GetAbs());
			Result.Bounds = FBox3f(-HalfExtent, HalfExtent);
			// Rio 04.10 evening (a polar-ring galaxy drew tall jagged "curtains" of glow above and below its host): the glow
			// map keeps one vertical profile per column of the galaxy plane, and the polar ring (radius 0.62, upright) puts
			// stars both above and below the host in the same columns, which that profile spreads into one tall sheet. Its
			// stars farther than 0.15 galaxy radius from the host plane stay points only. Other classes are untouched.
			const bool bPolarRing = Catalog.GalaxyClass == EGalaxyClass::PecPolarRing;
			const float PolarRingGlowHalfHeight = 0.15f * HalfExtent.Z / 1.05f;

			// Level 0: the catalogue box. Level k > 0: a cube around the home with half size HalfExtent / 2^k.
			const int32 Levels = FMath::Clamp(Spec.Levels, 1, 16);
			const float LevelZeroHalf = HalfExtent.GetMax();
			TArray<float> LevelHalf;
			Result.LevelBounds.Add(Result.Bounds);
			LevelHalf.Add(LevelZeroHalf);
			for (int32 Level = 1; Level < Levels; ++Level)
			{
				const float Half = LevelZeroHalf / static_cast<float>(1 << Level);
				LevelHalf.Add(Half);
				Result.LevelBounds.Add(FBox3f(Spec.HomeLocal - FVector3f(Half), Spec.HomeLocal + FVector3f(Half)));
			}
			Result.Points.SetNum(Levels);
			Result.Points[0].Reserve(Spec.PointCount);
			Result.LevelHalf = LevelHalf;
			Result.HomeLocal = Spec.HomeLocal;

			// The plugin normalises palette colours to unit luminance; the colour's luminance moves into the intensity.
			TArray<float> PaletteLuminance;
			TArray<FLinearColor> PaletteUnit;
			for (const FLinearColor& Color : Result.Palette)
			{
				const float ColorLuminance = Luminance(Color);
				PaletteLuminance.Add(ColorLuminance);
				PaletteUnit.Add(ColorLuminance > 1.0e-6f ? FLinearColor(FMath::Max(Color.R, 0.0f) / ColorLuminance,
					FMath::Max(Color.G, 0.0f) / ColorLuminance, FMath::Max(Color.B, 0.0f) / ColorLuminance, 1.0f)
					: FLinearColor::White);
			}
			const int32 G0Index = static_cast<int32>(ESpectralClass::G) * 10;
			const float G0Intensity = APSGameplayStarAppearance::GetLuminosityGain(1.0)
				* (PaletteLuminance.IsValidIndex(G0Index) ? PaletteLuminance[G0Index] : 1.0f);

			const APSCanonicalStellarProjection::FNestedCatalogPermutation Order =
				APSCanonicalStellarProjection::MakeNestedCatalogPermutation(Catalog.GenerationSeed, Catalog.ModeledStarCount);
			const int32 ResolveCount = FMath::Max(Spec.PointCount, Spec.GlowSamples);
			TUniquePtr<APSStarRenderer::FGlowMapBuilder> Glow;
			if (Spec.GlowSamples > 0)
			{
				Glow = MakeUnique<APSStarRenderer::FGlowMapBuilder>(GlowMapResolution, HalfExtent.GetMax());
			}
			TArray<FGalaxyCatalogStarRecord> Chunk;
			// The nearest-star index: the prefix (ISM stars) first, then every point as it is resolved below.
			TArray<FVector3f> IndexPositions;
			TArray<int32> IndexOrdinals;
			if (Spec.bIndex)
			{
				IndexPositions.Reserve(Spec.FirstOrdinal + Spec.PointCount);
				IndexOrdinals.Reserve(Spec.FirstOrdinal + Spec.PointCount);
				for (int32 Start = 0; Start < Spec.FirstOrdinal; Start += ResolveChunk)
				{
					const int32 Count = FMath::Min(ResolveChunk, Spec.FirstOrdinal - Start);
					APSGalaxyCatalogBatch::ResolveStars(Catalog, Order, Start, Count, Chunk);
					for (int32 Local = 0; Local < Chunk.Num(); ++Local)
					{
						if (Chunk[Local].CatalogIndex != INDEX_NONE)
						{
							IndexPositions.Add(FVector3f(Chunk[Local].GalaxyLocalLocation));
							IndexOrdinals.Add(Start + Local);
						}
					}
				}
			}
			for (int32 Start = 0; Start < ResolveCount; Start += ResolveChunk)
			{
				const int32 Count = FMath::Min(ResolveChunk, ResolveCount - Start);
				APSGalaxyCatalogBatch::ResolveStars(Catalog, Order, Spec.FirstOrdinal + Start, Count, Chunk);
				for (int32 Local = 0; Local < Chunk.Num(); ++Local)
				{
					const FGalaxyCatalogStarRecord& Record = Chunk[Local];
					if (Record.CatalogIndex == INDEX_NONE)
					{
						continue;
					}
					const int32 Ordinal = Start + Local;
					if (Spec.bIndex && Ordinal < Spec.PointCount)
					{
						IndexPositions.Add(FVector3f(Record.GalaxyLocalLocation));
						IndexOrdinals.Add(Spec.FirstOrdinal + Ordinal);
					}
					const uint8 ColorIndex = ColorIndexFor(Record);
					const float Intensity = Spec.Mode == ELayerMode::Gameplay
						? GameplayIntensityFor(Record, PaletteLuminance[ColorIndex], G0Intensity)
						: MenuIntensityFor(Record, PaletteLuminance[ColorIndex], Spec.EmissionMin, Spec.EmissionMax);
					const FVector3f Position(Record.GalaxyLocalLocation);
					if (Ordinal < Spec.PointCount)
					{
						int32 Level = 0;
						if (Levels > 1)
						{
							const FVector3f FromHome = (Position - Spec.HomeLocal).GetAbs();
							const float Chebyshev = FromHome.GetMax();
							for (int32 Candidate = Levels - 1; Candidate >= 1; --Candidate)
							{
								if (Chebyshev <= LevelHalf[Candidate])
								{
									Level = Candidate;
									break;
								}
							}
						}
						Result.Points[Level].Add(APSStarRenderer::PackStar(Position, Result.LevelBounds[Level], ColorIndex, Intensity));
					}
					if (Glow.IsValid() && Ordinal < Spec.GlowSamples
						&& !(bPolarRing && FMath::Abs(Position.Z) > PolarRingGlowHalfHeight))
					{
						Glow->AddStar(Position, PaletteUnit[ColorIndex], Intensity, DustWeightFor(Record.SpectralClass));
					}
				}
			}
			if (Glow.IsValid())
			{
				Result.bGlowMap = Glow->Build(Result.GlowMap);
			}
			if (Spec.bIndex && IndexPositions.Num() > 0)
			{
				// Counting sort into a uniform grid over the catalogue box.
				TSharedPtr<FStarIndex, ESPMode::ThreadSafe> Index = MakeShared<FStarIndex, ESPMode::ThreadSafe>();
				Index->Resolution = IndexResolution;
				Index->Min = -HalfExtent;
				Index->CellSize = FMath::Max(2.0f * HalfExtent.GetMax() / static_cast<float>(IndexResolution), 1.0f);
				Index->Order = Order;
				const int32 Cells = IndexResolution * IndexResolution * IndexResolution;
				TArray<int32> CellOfStar;
				CellOfStar.SetNumUninitialized(IndexPositions.Num());
				Index->CellStart.Init(0, Cells + 1);
				for (int32 Star = 0; Star < IndexPositions.Num(); ++Star)
				{
					CellOfStar[Star] = Index->LinearOf(Index->CellOf(IndexPositions[Star]));
					++Index->CellStart[CellOfStar[Star] + 1];
					Index->BoundRadius = FMath::Max(Index->BoundRadius, IndexPositions[Star].Size());
				}
				for (int32 Cell = 0; Cell < Cells; ++Cell)
				{
					Index->CellStart[Cell + 1] += Index->CellStart[Cell];
				}
				TArray<int32> Cursor(Index->CellStart.GetData(), Cells);
				Index->Positions.SetNumUninitialized(IndexPositions.Num());
				Index->Ordinals.SetNumUninitialized(IndexPositions.Num());
				for (int32 Star = 0; Star < IndexPositions.Num(); ++Star)
				{
					const int32 Slot = Cursor[CellOfStar[Star]]++;
					Index->Positions[Slot] = IndexPositions[Star];
					Index->Ordinals[Slot] = IndexOrdinals[Star];
				}
				Result.Index = Index;
			}
			Result.Seconds = FPlatformTime::Seconds() - Started;
			return Result;
		}

		void FinishBuild(const TWeakObjectPtr<AGalaxy>& WeakGalaxy, const uint32 Serial, FBuildResult&& Result)
		{
			AGalaxy* Galaxy = WeakGalaxy.Get();
			if (!IsValid(Galaxy) || Galaxy->GpuStarLayerSerial != Serial)
			{
				return; // Released or rebuilt meanwhile.
			}
			Galaxy->bGpuStarLayerBuilding = false;
			UWorld* World = Galaxy->GetWorld();
			const ELayerMode Mode = Result.Mode;
			FTransform LocalToWorld;
			const bool bTransform = Mode == ELayerMode::Gameplay
				? MakeGameplayLocalToWorld(*Galaxy, LocalToWorld) : MakeLocalToPhysical(*Galaxy, LocalToWorld);
			if (World == nullptr || Mode == ELayerMode::None || ModeOf(*Galaxy) != Mode || !IsRequestedFor(Mode) || !bTransform)
			{
				ReleaseLayer(*Galaxy);
				return;
			}
			APSStarRenderer::SetColorPalette(Result.Palette);
			const FGalaxyCatalogDescriptor& Catalog = Galaxy->StarCatalog;
			const double RadiusLocal = FMath::Max(static_cast<double>(Result.Bounds.Max.GetMax()) / 1.05, 1.0);
			const int64 PointEnd = static_cast<int64>(Result.FirstOrdinal) + Result.PointOrdinals;
			const double PointShare = Result.PointOrdinals > 0
				? (ReferenceStars(PointEnd) - ReferenceStars(Result.FirstOrdinal)) / Result.PointOrdinals : 0.0;
			// The glow carries what the ISM prefix and the points do not draw (never the overlap of a calibration run).
			const int64 GlowStart = FMath::Max<int64>(PointEnd, Catalog.RenderedSampleCount);
			const double GlowStars = FMath::Max(ReferenceStars(Catalog.ModeledStarCount) - ReferenceStars(GlowStart), 0.0);
			double BaseScale = 0.0;
			float FloorLocal = 0.0f;
			float GlowFloorLocal = -1.0f;
			APSStarRenderer::FFarEnvelope Envelope;
			const FRequest Request = CurrentRequest(Mode);
			if (Mode == ELayerMode::Gameplay && Request.Law == 1)
			{
				// Rio 03.10 ("the same look in gameplay as in the menu"): the menu's photometry in the same catalogue units,
				// so the sky from the home system is the menu's SYSTEM view and a camera coming closer brightens the points.
				BaseScale = Request.Gain * ReferencePixelValue * FMath::Square(3.0 * RadiusLocal) * FMath::Square(ReferencePixelTangent);
				GlowFloorLocal = static_cast<float>(Request.GlowFloor * RadiusLocal);
			}
			else if (Mode == ELayerMode::Gameplay)
			{
				// A point of the ISM prefix (share Lambda(N)/N) at most one galaxy radius away matches its glyph at
				// GameplayOverlayPixelValue x intensity; later ordinals keep their Lambda share of that.
				const int64 Prefix = FMath::Max<int64>(Catalog.RenderedSampleCount, 1);
				const double PrefixShare = FMath::Max(ReferenceStars(Prefix) / static_cast<double>(Prefix), 1.0e-12);
				FloorLocal = static_cast<float>(RadiusLocal);
				BaseScale = GameplayOverlayPixelValue / PrefixShare * FMath::Square(RadiusLocal) * FMath::Square(ReferencePixelTangent);
			}
			else
			{
				// Pixel value of a reference star = ReferencePixelValue at three galaxy radii (inverse square, local units).
				BaseScale = ReferencePixelValue * FMath::Square(3.0 * RadiusLocal) * FMath::Square(ReferencePixelTangent);
				Envelope = MakeEnvelope();
			}
			// Rio 04.10 ("a galaxy of only giants merges into one blinding blob; think about it, only test runs for now"): its
			// stars average hundreds of times an ordinary population's brightness (layer mean intensity ~850-1800 against ~4).
			// aps.Stars.PopulationNormalize k > 0 pulls such a layer back by (ordinary mean / its mean)^k, never brightening.
			// 0 (default): the look as it is.
			const double Normalize = FMath::Max(static_cast<double>(CVarPopulationNormalize.GetValueOnGameThread()), 0.0);
			const double LayerMean = Result.bGlowMap ? Result.GlowMap.MeanIntensity : 0.0;
			if (Normalize > 0.0 && LayerMean > OrdinaryMeanIntensity)
			{
				BaseScale *= FMath::Pow(OrdinaryMeanIntensity / LayerMean, Normalize);
			}
			if (Mode == ELayerMode::Gameplay)
			{
				GLayer.Exclusions = ScanGameplayExclusions(World, LocalToWorld);
				GGameplay.NextExclusionScanSeconds = FPlatformTime::Seconds() + ExclusionScanSeconds;
				GIndex = Result.Index;
				GIndexGalaxy = Galaxy;
				if (GIndex.IsValid())
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] nearest-star index: %d stars in %d^3 cells of %.3g local units (%.3g AU), %.1f MB"),
						GIndex->Positions.Num(), GIndex->Resolution, GIndex->CellSize,
						GIndex->CellSize * LocalToWorld.GetMaximumAxisScale() / 1.495978707e13,
						(GIndex->Positions.Num() * 16.0 + GIndex->CellStart.Num() * 4.0) / (1024.0 * 1024.0));
				}
			}
			GLayer.Mode = Mode;
			GLayer.Galaxy = Galaxy;
			GLayer.PushedTransform = LocalToWorld;
			GLayer.PointSets.Reset();
			GLayer.PointDescs.Reset();
			// Rio 05.10 night: what an approach point needs to find its star's twin and to shine exactly like it.
			GLayer.PointLevels.Reset();
			GLayer.LevelHalf = Result.LevelHalf;
			GLayer.HomeLocal = Result.HomeLocal;
			GLayer.FirstOrdinal = Result.FirstOrdinal;
			GLayer.PointOrdinals = Result.PointOrdinals;
			GLayer.PaletteLuminance.Reset();
			for (const FLinearColor& Color : Result.Palette)
			{
				GLayer.PaletteLuminance.Add(Luminance(Color));
			}
			const int32 G0Index = static_cast<int32>(ESpectralClass::G) * 10;
			GLayer.G0Intensity = APSGameplayStarAppearance::GetLuminosityGain(1.0)
				* (GLayer.PaletteLuminance.IsValidIndex(G0Index) ? GLayer.PaletteLuminance[G0Index] : 1.0f);
			int32 PointCount = 0;
			const bool bMenuFade = Mode == ELayerMode::Menu && CVarMenuFadeInSeconds.GetValueOnGameThread() > 0.0f;
			// Rio 06.10 (aps.Stars.SystemGlare): a gameplay layer starts at the glare of its world (1 without one).
			const float GameplayStartVisibility = GSystemGlareWorld.Get() == World ? GSystemGlareOthers : 1.0f;
			const float StartVisibility = Mode == ELayerMode::Menu ? (bMenuFade ? 0.0f : MenuPointShare()) : GameplayStartVisibility;
			const float StartGlowVisibility = Mode == ELayerMode::Menu ? (bMenuFade ? 0.0f : MenuGlowShare()) : GameplayStartVisibility;
			GLayer.ReadySeconds = FPlatformTime::Seconds();
			GLayer.LastPresentSeconds = 0.0;
			GLayer.GlowFocus = MenuGlowShare();
			GLayer.PointFocus = MenuPointShare();
			GLayer.AppliedPointVisibility = StartVisibility;
			GLayer.AppliedGlowVisibility = StartGlowVisibility;
			GLayer.MinPixelFocus = Mode == ELayerMode::Menu ? MenuMinPixelScale() : 1.0f;
			GLayer.AppliedMinPixelScale = Mode == ELayerMode::Menu
				? PointMinPixelScale(GLayer.MinPixelFocus, GLayer.PointFocus) : 1.0f;
			for (int32 Level = 0; Level < Result.Points.Num(); ++Level)
			{
				TArray<APSStarRenderer::FPackedStar>& Points = Result.Points[Level];
				if (Points.Num() == 0 || !(PointShare > 0.0))
				{
					continue;
				}
				APSStarRenderer::FPointSetDesc Desc;
				Desc.LocalToWorld = LocalToWorld;
				Desc.FarEnvelope = Envelope;
				Desc.LocalBounds = Result.LevelBounds[Level];
				Desc.IntensityScale = static_cast<float>(BaseScale * PointShare);
				Desc.BrightnessFloorDistanceLocal = FloorLocal;
				// Inner levels (nearest stars) first under aps.Stars.GpuPointBudget.
				Desc.Priority = Level;
				Desc.Visibility = StartVisibility;
				Desc.MinPixelScale = GLayer.AppliedMinPixelScale;
				Desc.Population = Catalog.ModeledStarCount;
				Desc.DebugName = Mode == ELayerMode::Gameplay ? FString::Printf(TEXT("GameplayGalaxy L%d"), Level) : TEXT("MenuGalaxy");
				if (Mode == ELayerMode::Gameplay)
				{
					ApplyGameplayExclusions(GLayer.Exclusions, Desc);
				}
				else
				{
					ApplyMenuExclusion(LocalToWorld, Desc);
				}
				PointCount += Points.Num();
				const APSStarRenderer::FHandle Handle = APSStarRenderer::RegisterPointSet(World, Desc, MoveTemp(Points));
				if (Handle != 0)
				{
					GLayer.PointSets.Add(Handle);
					GLayer.PointDescs.Add(Desc);
					GLayer.PointLevels.Add(Level);
				}
			}
			GLayer.PointCount = GLayer.PointSets.Num() > 0 ? PointCount : 0;
			// Rio 06.10 (strategic map GALAXY): what the map hold scales (used in gameplay only); the sets start without it.
			GLayer.PointIntensityScale = static_cast<float>(BaseScale * PointShare);
			GLayer.RadiusLocal = static_cast<float>(RadiusLocal);
			GLayer.AppliedMapGain = 1.0f;
			Galaxy->GpuPointSet = GLayer.PointSets.Num() > 0 ? GLayer.PointSets[0] : 0;
			const double MeanIntensity = Result.bGlowMap ? Result.GlowMap.MeanIntensity : 0.0;
			const double GlowTotal = GlowStars * MeanIntensity * BaseScale;
			if (Result.bGlowMap && GlowTotal > 0.0)
			{
				APSStarRenderer::FGlowVolumeDesc GlowDesc;
				GlowDesc.LocalToWorld = LocalToWorld;
				GlowDesc.FarEnvelope = Envelope;
				GlowDesc.TotalIntensity = GlowTotal;
				GlowDesc.DustOpacity = DustOpacityFor(Catalog.GalaxyType);
				GlowDesc.BrightnessFloorDistanceLocal = GlowFloorLocal >= 0.0f ? GlowFloorLocal : FloorLocal;
				GlowDesc.DebugName = Mode == ELayerMode::Gameplay ? TEXT("GameplayGalaxy") : TEXT("MenuGalaxy");
				GlowDesc.Visibility = StartGlowVisibility;
				Galaxy->GpuGlowVolume = APSStarRenderer::RegisterGlowVolume(World, GlowDesc, MoveTemp(Result.GlowMap));
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.GalaxyGpu] %s layer ready in %.2f s (background): %d points in %d set(s) from ordinal %d (%.1f MB, %.3g ")
				TEXT("reference stars each), glow %s (%.4g reference stars, mean intensity %.2f, dust %.2f), ISM prefix %d of %lld, ")
				TEXT("exclusions %d, %s"),
				Mode == ELayerMode::Gameplay ? TEXT("gameplay") : TEXT("menu"), Result.Seconds, PointCount,
				GLayer.PointSets.Num(), Result.FirstOrdinal, PointCount * 8.0 / (1024.0 * 1024.0), PointShare,
				Galaxy->GpuGlowVolume != 0 ? TEXT("on") : TEXT("off"), GlowStars, MeanIntensity,
				DustOpacityFor(Catalog.GalaxyType), Catalog.RenderedSampleCount, Catalog.ModeledStarCount,
				Mode == ELayerMode::Gameplay ? GLayer.Exclusions.Num() : (GInput.HomeExclusionRadiusCm > 0.0 ? 1 : 0),
				Mode != ELayerMode::Gameplay ? TEXT("menu photometry")
					: Request.Law == 1 ? *FString::Printf(TEXT("menu photometry x%.2f"), Request.Gain) : TEXT("glyph-matched floor"));
		}

		void PushMenuPresentation(AGalaxy& Galaxy)
		{
			UWorld* World = Galaxy.GetWorld();
			FTransform LocalToPhysical;
			if ((GLayer.PointSets.Num() == 0 && Galaxy.GpuGlowVolume == 0) || World == nullptr
				|| !MakeLocalToPhysical(Galaxy, LocalToPhysical))
			{
				return; // Still building, or nothing to draw.
			}
			const APSStarRenderer::FFarEnvelope Envelope = MakeEnvelope();
			APSStarRenderer::SetWorldFarEnvelope(World, Envelope);
			const bool bTransformChanged = !LocalToPhysical.Equals(GLayer.PushedTransform, 0.0);
			for (int32 Index = 0; Index < GLayer.PointSets.Num() && Index < GLayer.PointDescs.Num(); ++Index)
			{
				APSStarRenderer::FPointSetDesc Desc = GLayer.PointDescs[Index];
				Desc.LocalToWorld = LocalToPhysical;
				ApplyMenuExclusion(LocalToPhysical, Desc);
				if (bTransformChanged || Desc.ExclusionRadiusLocal != GLayer.PointDescs[Index].ExclusionRadiusLocal
					|| Desc.ExclusionCenterLocal != GLayer.PointDescs[Index].ExclusionCenterLocal)
				{
					// UpdatePointSet replaces the whole description: carry this frame's envelope.
					Desc.FarEnvelope = Envelope;
					APSStarRenderer::UpdatePointSet(GLayer.PointSets[Index], Desc);
					GLayer.PointDescs[Index] = Desc;
				}
			}
			if (Galaxy.GpuGlowVolume != 0 && bTransformChanged)
			{
				APSStarRenderer::SetTransform(Galaxy.GpuGlowVolume, LocalToPhysical);
			}
			GLayer.PushedTransform = LocalToPhysical;

			// Rio 04.10: the fade-in of a new layer (Regenerate showed the base galaxy, then the glow with a jump), and in the
			// evening ("the stars must stay, only the glow drops on the close screens") the screen's share of the glow alone,
			// eased. The points never take a screen share. Sent on a 0.4% change and at the ends.
			const double Now = FPlatformTime::Seconds();
			const float DeltaSeconds = GLayer.LastPresentSeconds > 0.0
				? static_cast<float>(FMath::Clamp(Now - GLayer.LastPresentSeconds, 0.0, 0.25)) : 0.0f;
			GLayer.LastPresentSeconds = Now;
			// The flight already eases the target; this only smooths a screen change without a flight (and frame steps).
			const float GlowTarget = MenuGlowShare();
			const float PointTarget = MenuPointShare();
			GLayer.GlowFocus = FMath::FInterpTo(GLayer.GlowFocus, GlowTarget, DeltaSeconds, 8.0f);
			GLayer.PointFocus = FMath::FInterpTo(GLayer.PointFocus, PointTarget, DeltaSeconds, 8.0f);
			const double FadeSeconds = FMath::Max(static_cast<double>(CVarMenuFadeInSeconds.GetValueOnGameThread()), 0.0);
			const float FadeIn = FadeSeconds > 0.0
				? static_cast<float>(FMath::SmoothStep(0.0, FadeSeconds, Now - GLayer.ReadySeconds)) : 1.0f;
			const float PointVisibility = FMath::Clamp(FadeIn * GLayer.PointFocus, 0.0f, 1.0f);
			const float GlowVisibility = FMath::Clamp(FadeIn * GLayer.GlowFocus, 0.0f, 1.0f);
			const bool bSettled = FadeIn >= 1.0f && FMath::IsNearlyEqual(GLayer.GlowFocus, GlowTarget, 1.0e-4f)
				&& FMath::IsNearlyEqual(GLayer.PointFocus, PointTarget, 1.0e-4f);
			const auto ShouldSend = [bSettled](const float Wanted, const float Applied)
			{
				return Wanted != Applied && (bSettled || FMath::Abs(Wanted - Applied) > 0.004f);
			};
			if (ShouldSend(PointVisibility, GLayer.AppliedPointVisibility))
			{
				for (int32 Index = 0; Index < GLayer.PointSets.Num(); ++Index)
				{
					APSStarRenderer::SetVisibility(GLayer.PointSets[Index], PointVisibility);
					if (GLayer.PointDescs.IsValidIndex(Index))
					{
						GLayer.PointDescs[Index].Visibility = PointVisibility;
					}
				}
				GLayer.AppliedPointVisibility = PointVisibility;
			}
			if (Galaxy.GpuGlowVolume != 0 && ShouldSend(GlowVisibility, GLayer.AppliedGlowVisibility))
			{
				APSStarRenderer::SetVisibility(Galaxy.GpuGlowVolume, GlowVisibility);
				GLayer.AppliedGlowVisibility = GlowVisibility;
			}
			// Rio 05.10: the points' brightness LOD follows their share (a dimmed point keeps being drawn), times the
			// screen's optional extra cut, eased the same way; sent on a 1% change and at the end.
			const float MinPixelTarget = MenuMinPixelScale();
			GLayer.MinPixelFocus = FMath::FInterpTo(GLayer.MinPixelFocus, MinPixelTarget, DeltaSeconds, 8.0f);
			const bool bMinPixelSettled = FMath::IsNearlyEqual(GLayer.MinPixelFocus, MinPixelTarget, 1.0e-3f);
			if (bMinPixelSettled)
			{
				GLayer.MinPixelFocus = MinPixelTarget;
			}
			const float MinPixelScale = PointMinPixelScale(GLayer.MinPixelFocus, GLayer.PointFocus);
			if (MinPixelScale != GLayer.AppliedMinPixelScale
				&& ((bSettled && bMinPixelSettled) || FMath::Abs(MinPixelScale / GLayer.AppliedMinPixelScale - 1.0f) > 0.01f))
			{
				for (int32 Index = 0; Index < GLayer.PointSets.Num(); ++Index)
				{
					APSStarRenderer::SetMinPixelScale(GLayer.PointSets[Index], MinPixelScale);
					if (GLayer.PointDescs.IsValidIndex(Index))
					{
						GLayer.PointDescs[Index].MinPixelScale = MinPixelScale;
					}
				}
				GLayer.AppliedMinPixelScale = MinPixelScale;
			}
		}

		/**
		 * Rio 04.10 evening ("the glow came on only after a few seconds"; "the stars faded only when I turned the camera"):
		 * the preview generator stops ticking while its view is still, so the menu layer's fade-in and screen share are
		 * driven from here every frame instead of from its applied frames.
		 */
		bool TickMenuPresentation(float)
		{
			if (GLayer.Mode == ELayerMode::Menu)
			{
				if (AGalaxy* Galaxy = GLayer.Galaxy.Get(); IsValid(Galaxy) && Galaxy->bGpuStarLayerActive)
				{
					PushMenuPresentation(*Galaxy);
				}
			}
			return true;
		}

		/**
		 * Rio 06.10 (strategic map GALAXY: "on My Galaxy the GPU stars switch off, a little closer they come back"): law 1 has
		 * no point floor and the gameplay intensities sit in a narrow band, so the whole layer fell under the shader's
		 * MinPixel cut at about the same distance, just short of the GALAXY preset (~3.4 radii). While the F10 map is open
		 * and its camera stands beyond aps.Stars.MapPointHoldRadii galaxy radii from the centre (the catalogue's local
		 * origin), the points keep the brightness they have there: (distance / hold)^2. 1 everywhere else (the pilot's sky,
		 * map views inside the galaxy). The live map camera actor, not a camera cache that may lag an origin shift by a frame.
		 * Law 1 only (the layer's own request): aps.Stars.GameplayGpuLaw 0 keeps its glyph-matched photometry untouched.
		 * OutRadii (for the log): the view target's distance from the centre in galaxy radii, 0 when there is none.
		 */
		float MapPointGain(UWorld* World, const FTransform& LocalToWorld, double* OutRadii = nullptr)
		{
			if (OutRadii)
			{
				*OutRadii = 0.0;
			}
			const float HoldRadii = CVarMapPointHoldRadii.GetValueOnGameThread();
			if (GLayer.Mode != ELayerMode::Gameplay || GLayer.Request.Law != 1 || !(HoldRadii > 0.0f) || !(GLayer.RadiusLocal > 0.0f)
				|| World == nullptr)
			{
				return 1.0f;
			}
			const AGravityPlayerController* PC = Cast<AGravityPlayerController>(World->GetFirstPlayerController());
			const AActor* ViewTarget = PC ? PC->GetViewTarget() : nullptr;
			if (!IsValid(ViewTarget))
			{
				return 1.0f;
			}
			const double Dist = LocalToWorld.InverseTransformPosition(ViewTarget->GetActorLocation()).Size();
			if (OutRadii)
			{
				*OutRadii = Dist / GLayer.RadiusLocal;
			}
			if (!PC->IsStrategicMapOpen())
			{
				return 1.0f; // The pilot's sky: exactly today's photometry.
			}
			const double Hold = static_cast<double>(HoldRadii) * GLayer.RadiusLocal;
			return Dist > Hold && FMath::IsFinite(Dist)
				? static_cast<float>(FMath::Min(FMath::Square(Dist / Hold), 1.0e4)) : 1.0f;
		}

		/** Gameplay: transform from the component's final world transform; exclusions every ExclusionScanSeconds. */
		void PushGameplayPresentation(AGalaxy& Galaxy, const bool bScanExclusions)
		{
			UWorld* World = Galaxy.GetWorld();
			FTransform LocalToWorld;
			if ((GLayer.PointSets.Num() == 0 && Galaxy.GpuGlowVolume == 0) || World == nullptr
				|| !MakeGameplayLocalToWorld(Galaxy, LocalToWorld))
			{
				return; // Still building, or nothing to draw.
			}
			if (!LocalToWorld.Equals(GLayer.PushedTransform, 0.0))
			{
				for (int32 Index = 0; Index < GLayer.PointSets.Num() && Index < GLayer.PointDescs.Num(); ++Index)
				{
					APSStarRenderer::SetTransform(GLayer.PointSets[Index], LocalToWorld);
					GLayer.PointDescs[Index].LocalToWorld = LocalToWorld;
				}
				if (Galaxy.GpuGlowVolume != 0)
				{
					APSStarRenderer::SetTransform(Galaxy.GpuGlowVolume, LocalToWorld);
				}
				PushApproachTransforms(LocalToWorld);
				GLayer.PushedTransform = LocalToWorld;
			}
			// Rio 06.10 (strategic map GALAXY): the map hold on the points' IntensityScale (MapPointGain), sent only when it
			// moves by more than half a percent or returns to 1. Not on the world-shift re-read (bScanExclusions false): its
			// camera may still be the one before the shift. The glow is untouched; PushSetExclusions sends the held descs.
			if (bScanExclusions)
			{
				double ViewRadii = 0.0;
				const float Gain = MapPointGain(World, LocalToWorld, &ViewRadii);
				if (Gain != GLayer.AppliedMapGain && (Gain == 1.0f || FMath::Abs(Gain / GLayer.AppliedMapGain - 1.0f) > 0.005f))
				{
					for (int32 Index = 0; Index < GLayer.PointSets.Num() && Index < GLayer.PointDescs.Num(); ++Index)
					{
						GLayer.PointDescs[Index].IntensityScale = GLayer.PointIntensityScale * Gain;
						APSStarRenderer::UpdatePointSet(GLayer.PointSets[Index], GLayer.PointDescs[Index]);
					}
					if ((Gain > 1.0f) != (GLayer.AppliedMapGain > 1.0f))
					{
						UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] map point hold %s (x%.2f at %.2f radii)"),
							Gain > 1.0f ? TEXT("on") : TEXT("off"), Gain, ViewRadii);
					}
					GLayer.AppliedMapGain = Gain;
				}
			}
			if (!bScanExclusions)
			{
				return;
			}
			const double Now = FPlatformTime::Seconds();
			if (Now < GGameplay.NextExclusionScanSeconds)
			{
				return;
			}
			GGameplay.NextExclusionScanSeconds = Now + ExclusionScanSeconds;
			TArray<FVector4f> Spheres = ScanGameplayExclusions(World, LocalToWorld);
			if (SameSpheres(Spheres, GLayer.Exclusions))
			{
				return;
			}
			GLayer.Exclusions = MoveTemp(Spheres);
			for (int32 Index = 0; Index < GLayer.PointSets.Num() && Index < GLayer.PointDescs.Num(); ++Index)
			{
				// Rio 05.10 night: with the twins of the set's approach points.
				PushSetExclusions(Index);
			}
		}

		/** Floating origin (APSWorldShiftEvents: the engine origin event and the double shift): read final transforms. */
		void OnWorldShifted(UWorld* World)
		{
			AGalaxy* Galaxy = GLayer.Galaxy.Get();
			if (GLayer.Mode == ELayerMode::Gameplay && IsValid(Galaxy) && Galaxy->GetWorld() == World)
			{
				PushGameplayPresentation(*Galaxy, false);
			}
		}

		void EnsureShiftBound()
		{
			if (!GShiftBound)
			{
				GShiftBound = true;
				APSWorldShiftEvents::BindPostShift(GetTransientPackage(), [](UWorld* World) { OnWorldShifted(World); });
				// Rio 06.10 (still ship): a deferred travel moves only the sky (the generator carrying the galaxy component):
				// the same re-read, at once and again once every actor has ticked, since the generator may be moved by a
				// listener called after this one.
				UAPSWorldOriginSubsystem::OnSkyOffsetChanged().AddLambda([](UWorld* World, const FVector&)
				{
					OnWorldShifted(World);
					GSkyMoved = true;
				});
				FWorldDelegates::OnWorldPostActorTick.AddLambda([](UWorld* World, ELevelTick, float)
				{
					if (GSkyMoved)
					{
						GSkyMoved = false;
						OnWorldShifted(World);
					}
				});
			}
		}

		void OnRequestChanged(IConsoleVariable* /*Variable*/)
		{
			const FRequest MenuRequest = CurrentRequest(ELayerMode::Menu);
			const FRequest GameplayRequest = CurrentRequest(ELayerMode::Gameplay);
			if (MenuRequest == GLastMenuRequest && GameplayRequest == GLastGameplayRequest)
			{
				return;
			}
			GLastMenuRequest = MenuRequest;
			GLastGameplayRequest = GameplayRequest;
			// Typing a CVar must not wait for the next camera move (the menu's preview tick sleeps while idle).
			for (AGalaxy* Galaxy : { GGameplay.Galaxy.Get(), GInput.Galaxy.Get() })
			{
				if (!IsValid(Galaxy))
				{
					continue;
				}
				const ELayerMode Mode = ModeOf(*Galaxy);
				if (Mode == ELayerMode::None)
				{
					continue;
				}
				if (IsRequestedFor(Mode))
				{
					Galaxy->RebuildGpuStarLayer();
				}
				else if (Galaxy->bGpuStarLayerActive)
				{
					Galaxy->ReleaseGpuStarLayer();
				}
				break; // One layer at a time: the gameplay galaxy wins over a stale menu one.
			}
		}

		void EnsureDelegatesBound()
		{
			if (GDelegatesBound)
			{
				return;
			}
			IConsoleVariable* Points = PointsVariable();
			IConsoleVariable* Glow = GlowVariable();
			if (Points == nullptr || Glow == nullptr)
			{
				return; // Plugin not loaded: nothing to drive.
			}
			GDelegatesBound = true;
			GMenuTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickMenuPresentation), 0.0f);
			GLastMenuRequest = CurrentRequest(ELayerMode::Menu);
			GLastGameplayRequest = CurrentRequest(ELayerMode::Gameplay);
			Points->OnChangedDelegate().AddStatic(&OnRequestChanged);
			Glow->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGpuStars.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGpuStarsFirst.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGameplayGpu.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGameplayGpuStars.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGameplayLaw.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGameplayGain.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
			CVarGameplayGlowFloor.AsVariable()->OnChangedDelegate().AddStatic(&OnRequestChanged);
		}

		/**
		 * Gameplay self-check, logged once per build: the ISM centres re-derived from their catalogue records through the
		 * same frame the GPU layer uses must equal the HISM base transforms (AAstroGenerator writes them that way).
		 */
		void LogGameplayMappingCheck(const AGalaxy& Galaxy, const FVector& HomeProxy)
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = Galaxy.CanonicalProjectionFrame;
			double MaxError = 0.0;
			double MaxRelative = 0.0;
			int32 Checked = 0;
			const int32 Count = FMath::Min(256, Galaxy.RenderedProxyBaseTransforms.Num());
			for (int32 Index = 0; Index < Count; ++Index)
			{
				FGalaxyCatalogStarRecord Record;
				if (!Galaxy.GetRenderedCatalogRecord(Index, Record))
				{
					continue;
				}
				const FVector Base = Galaxy.RenderedProxyBaseTransforms[Index].GetLocation();
				const double Error = FVector::Distance(Frame.ProjectCanonicalUnits(Record.GalaxyLocalLocation), Base);
				MaxError = FMath::Max(MaxError, Error);
				MaxRelative = FMath::Max(MaxRelative, Error / FMath::Max(FVector::Distance(Base, HomeProxy), 1.0e-6));
				++Checked;
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.GalaxyGpu] gameplay mapping: %d ISM centres re-derived from the catalogue frame, max error %.3g proxy cm ")
				TEXT("(%.3g of their distance from home, %.3f px at a 2560 px / 50 deg view)"),
				Checked, MaxError, MaxRelative, MaxRelative / ReferencePixelTangent);
		}
	}

	bool IsRequested()
	{
		return IsRequestedFor(ELayerMode::Menu);
	}

	bool HasLayers()
	{
		return GActiveLayers > 0;
	}

	void SetMenuGlowScope(const EMenuGlowScope Scope, const float FlightAlpha)
	{
		// The camera's own easing (FAPSContinuousPreviewOrbit::Interpolate): smoothstep of the flight's progress.
		const float T = FMath::Clamp(FlightAlpha, 0.0f, 1.0f);
		const float Eased = T * T * (3.0f - 2.0f * T);
		if (!MenuGlowContinuous())
		{
			if (Scope != GMenuGlowScope)
			{
				const bool bMenuLayer = GLayer.Mode == ELayerMode::Menu;
				GMenuGlowFrom = bMenuLayer ? GLayer.GlowFocus : ScopeShare(GMenuGlowScope, false);
				GMenuPointFrom = bMenuLayer ? GLayer.PointFocus : ScopeShare(GMenuGlowScope, true);
				GMenuMinPixelFrom = bMenuLayer ? GLayer.MinPixelFocus : ScopeMinPixel(GMenuGlowScope);
				GMenuGlowScope = Scope;
			}
			GMenuGlowProgress = Eased;
			GMenuLastFlightAlpha = T;
			GMenuFlightEaseBase = 0.0f;
			GMenuStillStartSeconds = -1.0;
			return;
		}
		// Rio 06.10 ("the glow first drops at once and then returns to its value"): the start used to be taken at a screen
		// change only, while the generator restarts its flight clock at 0 for every flight. The next flight on the same kind
		// of screen (CLUSTER -> GALAXY after SYSTEM -> CLUSTER, one planet -> another after GALAXY -> PLANET) went back to
		// that older start (the glow of the close screen it came from: dropped at once, eased back with the camera), and a
		// layer still building gave its reset 1. Now a screen change, a new flight (its clock went back) and a flight cut
		// short (orbit or zoom grabbed mid-way) all start from what is on screen, glow, points and cut at the same moment:
		// a flight eases over what is left of it, anything else over the shortest flight's time, with the same smoothstep.
		const bool bScopeChanged = Scope != GMenuGlowScope;
		const bool bNewFlight = T < GMenuLastFlightAlpha;
		const bool bCutShort = T >= 1.0f && GMenuLastFlightAlpha < 1.0f && GMenuStillStartSeconds < 0.0
			&& GMenuGlowProgress < 0.95f;
		if (bScopeChanged || bNewFlight || bCutShort)
		{
			// Read before anything changes: a layer not shown yet takes this moment of the current ease.
			const float GlowFrom = DisplayedMenuShare(false);
			const float PointFrom = DisplayedMenuShare(true);
			const float MinPixelFrom = DisplayedMenuMinPixel();
			const EMenuGlowScope Previous = GMenuGlowScope;
			GMenuGlowFrom = GlowFrom;
			GMenuPointFrom = PointFrom;
			GMenuMinPixelFrom = MinPixelFrom;
			GMenuGlowScope = Scope;
			GMenuStillStartSeconds = T >= 1.0f ? FPlatformTime::Seconds() : -1.0;
			GMenuFlightEaseBase = T >= 1.0f ? 0.0f : Eased;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.GalaxyGpu] menu brightness ease %s -> %s (%s at flight %.2f): glow %.3f -> %.3f, points %.3f -> %.3f, cut x%.2f -> x%.2f"),
				MenuScopeName(Previous), MenuScopeName(Scope),
				bScopeChanged ? TEXT("screen change") : bNewFlight ? TEXT("new flight") : TEXT("flight cut short"), T,
				GlowFrom, ScopeShare(Scope, false), PointFrom, ScopeShare(Scope, true), MinPixelFrom, ScopeMinPixel(Scope));
		}
		GMenuLastFlightAlpha = T;
		GMenuGlowProgress = GMenuFlightEaseBase < 1.0f
			? FMath::Clamp((Eased - GMenuFlightEaseBase) / (1.0f - GMenuFlightEaseBase), 0.0f, 1.0f) : 1.0f;
	}

	int32 GetDrawnPointCount(const UWorld* World)
	{
		const AGalaxy* Galaxy = GLayer.Galaxy.Get();
		return World && Galaxy && Galaxy->GetWorld() == World && Galaxy->bGpuStarLayerActive ? GLayer.PointCount : 0;
	}

	void ReleaseLayer(AGalaxy& Galaxy)
	{
		++Galaxy.GpuStarLayerSerial;
		Galaxy.bGpuStarLayerBuilding = false;
		if (GIndexGalaxy.Get() == &Galaxy || !GIndexGalaxy.IsValid())
		{
			GIndex.Reset();
			GIndexGalaxy.Reset();
		}
		if (GLayer.Galaxy.Get() == &Galaxy)
		{
			for (const APSStarRenderer::FHandle Handle : GLayer.PointSets)
			{
				APSStarRenderer::Remove(Handle);
			}
			// Rio 05.10 night: and the approach points with their layer.
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				APSStarRenderer::Remove(Point.Handle);
			}
			GLayer.Approach.Reset();
			GLayer.PointSets.Reset();
			GLayer.PointDescs.Reset();
			GLayer.PointLevels.Reset();
			GLayer.PointCount = 0;
		}
		if (Galaxy.GpuPointSet != 0)
		{
			APSStarRenderer::Remove(Galaxy.GpuPointSet);
			Galaxy.GpuPointSet = 0;
		}
		if (Galaxy.GpuGlowVolume != 0)
		{
			APSStarRenderer::Remove(Galaxy.GpuGlowVolume);
			Galaxy.GpuGlowVolume = 0;
		}
		if (Galaxy.bGpuStarLayerActive)
		{
			Galaxy.bGpuStarLayerActive = false;
			GActiveLayers = FMath::Max(GActiveLayers - 1, 0);
			if (GLayer.Galaxy.Get() == &Galaxy)
			{
				GLayer = FLayerState();
			}
			GVisibility = -1.0f;
			UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] layer released (%s)"), *Galaxy.GetName());
		}
	}

	void RebuildLayer(AGalaxy& Galaxy)
	{
		ReleaseLayer(Galaxy);
		const ELayerMode Mode = ModeOf(Galaxy);
		const FRequest Request = CurrentRequest(Mode);
		UWorld* World = Galaxy.GetWorld();
		if (Mode == ELayerMode::None || !Request.Wants() || World == nullptr || !ShadersReady())
		{
			return;
		}
		// One layer at a time.
		if (AGalaxy* Other = GLayer.Galaxy.Get(); IsValid(Other) && Other != &Galaxy)
		{
			ReleaseLayer(*Other);
		}
		const FGalaxyCatalogDescriptor& Catalog = Galaxy.StarCatalog;
		const int32 FirstOrdinal = Request.FirstOrdinal >= 0 ? Request.FirstOrdinal : FMath::Max(Catalog.RenderedSampleCount, 0);
		const int64 Available = FMath::Min<int64>(Catalog.ModeledStarCount - FirstOrdinal,
			static_cast<int64>(MAX_int32) - FirstOrdinal);
		const int32 PointCount = Request.bPoints
			? static_cast<int32>(FMath::Clamp<int64>(Request.PointCount, 0, FMath::Max<int64>(Available, 0))) : 0;
		const int32 GlowSamples = Request.bGlow ? static_cast<int32>(FMath::Clamp<int64>(Available, 0, GlowSampleLimit)) : 0;
		if (PointCount <= 0 && GlowSamples <= 0)
		{
			return;
		}
		FBuildSpec Spec;
		Spec.Catalog = Catalog;
		Spec.Mode = Mode;
		Spec.FirstOrdinal = FirstOrdinal;
		Spec.PointCount = PointCount;
		Spec.GlowSamples = GlowSamples;
		// The ISM emission clamp, read from the generator (CalculateEmission) rather than copied.
		UStarGenerator* EmissionSource = GetMutableDefault<UStarGenerator>();
		Spec.EmissionMin = FMath::Max(EmissionSource->CalculateEmission(0.0f), 1.0e-6);
		Spec.EmissionMax = FMath::Max(EmissionSource->CalculateEmission(1.0e30f), Spec.EmissionMin);
		Spec.Palette = MakePalette();
		if (Mode == ELayerMode::Gameplay)
		{
			FTransform LocalToWorld;
			const AStarSystem* Home = GGameplay.Home.Get();
			if (!MakeGameplayLocalToWorld(Galaxy, LocalToWorld) || !IsValid(Home) || !IsValid(Galaxy.StarMeshInstances))
			{
				return;
			}
			// Rio 06.10 (still ship): the home actor keeps its world place while a deferred travel moves the sky; the levels
			// centre on the home as the sky draws it.
			const FVector HomeInSky = Home->GetActorLocation() + UAPSWorldOriginSubsystem::SkyOffsetOf(Galaxy.GetWorld());
			Spec.HomeLocal = FVector3f(LocalToWorld.InverseTransformPosition(HomeInSky));
			Spec.Levels = GameplayLevels;
			Spec.bIndex = true;
			LogGameplayMappingCheck(Galaxy, Galaxy.StarMeshInstances->GetComponentTransform().InverseTransformPosition(HomeInSky));
			EnsureShiftBound();
		}

		Galaxy.bGpuStarLayerActive = true;
		Galaxy.bGpuStarLayerBuilding = true;
		++GActiveLayers;
		GLayer = FLayerState();
		GLayer.Mode = Mode;
		GLayer.Galaxy = &Galaxy;
		GLayer.Request = Request;
		GLayer.CatalogKey = CatalogKeyOf(Catalog, Galaxy.CanonicalProjectionFrame.RealScaleLengthFactor);
		const uint32 Serial = Galaxy.GpuStarLayerSerial;
		UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] building the %s layer: %d GPU points + %d glow samples from ordinal %d of %lld (%s)"),
			Mode == ELayerMode::Gameplay ? TEXT("gameplay") : TEXT("menu"), PointCount, GlowSamples, FirstOrdinal,
			Catalog.ModeledStarCount, *Galaxy.GetName());
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
			[WeakGalaxy = TWeakObjectPtr<AGalaxy>(&Galaxy), Serial, Spec = MoveTemp(Spec)]() mutable
			{
				FBuildResult Result = BuildOnWorker(MoveTemp(Spec));
				AsyncTask(ENamedThreads::GameThread, [WeakGalaxy, Serial, Result = MoveTemp(Result)]() mutable
				{
					FinishBuild(WeakGalaxy, Serial, MoveTemp(Result));
				});
			});
	}

	void PresentContinuousFrame(AGalaxy* Galaxy, const FAPSContinuousPreviewFrame& Frame,
		const FVector& HomeExclusionCenterCm, const double HomeExclusionRadiusCm)
	{
		if (!IsValid(Galaxy))
		{
			return;
		}
		GInput.Galaxy = Galaxy;
		GInput.ObserverCm = Frame.ObserverCm;
		GInput.RenderCmPerPhysicalCm = Frame.RenderCmPerPhysicalCm;
		GInput.FarEnvelopeCm = Frame.FarEnvelopeCm;
		GInput.HomeExclusionCenterCm = HomeExclusionCenterCm;
		GInput.HomeExclusionRadiusCm = HomeExclusionRadiusCm;
		GInput.bValid = true;
		EnsureDelegatesBound();
		if (!IsRequestedFor(ELayerMode::Menu))
		{
			if (Galaxy->bGpuStarLayerActive)
			{
				Galaxy->ReleaseGpuStarLayer();
			}
			return;
		}
		if (!Galaxy->bGpuStarLayerActive || GLayer.Galaxy.Get() != Galaxy || GLayer.Mode != ELayerMode::Menu
			|| GLayer.Request != CurrentRequest(ELayerMode::Menu)
			|| GLayer.CatalogKey != CatalogKeyOf(Galaxy->StarCatalog, Galaxy->CanonicalProjectionFrame.RealScaleLengthFactor))
		{
			Galaxy->RebuildGpuStarLayer();
			return;
		}
		PushMenuPresentation(*Galaxy);
	}

	void PresentGameplayFrame(UWorld* World, AStarSystem* HomeSystem, const TConstArrayView<AActor*> AttachedActors)
	{
		AGalaxy* Galaxy = nullptr;
		for (AActor* Actor : AttachedActors)
		{
			if (AGalaxy* Candidate = Cast<AGalaxy>(Actor); IsValid(Candidate))
			{
				Galaxy = Candidate;
				break;
			}
		}
		if (World == nullptr || !IsValid(Galaxy) || !IsValid(HomeSystem) || Galaxy->GetWorld() != World)
		{
			return;
		}
		GGameplay.Galaxy = Galaxy;
		GGameplay.Home = HomeSystem;
		GGameplay.bValid = true;
		EnsureDelegatesBound();
		if (!IsRequestedFor(ELayerMode::Gameplay))
		{
			if (Galaxy->bGpuStarLayerActive)
			{
				Galaxy->ReleaseGpuStarLayer();
			}
			return;
		}
		if (!Galaxy->bGpuStarLayerActive || GLayer.Galaxy.Get() != Galaxy || GLayer.Mode != ELayerMode::Gameplay
			|| GLayer.Request != CurrentRequest(ELayerMode::Gameplay)
			|| GLayer.CatalogKey != CatalogKeyOf(Galaxy->StarCatalog, Galaxy->CanonicalProjectionFrame.RealScaleLengthFactor))
		{
			Galaxy->RebuildGpuStarLayer();
			return;
		}
		PushGameplayPresentation(*Galaxy, true);
	}

	namespace
	{
		FAutoConsoleCommandWithWorldAndArgs NearStarsCommand(TEXT("aps.Stars.Near"),
			TEXT("Logs the drawn catalogue stars (ISM prefix and GPU points) nearest to the player: aps.Stars.Near [count=12]."),
			FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
			{
				const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
				const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
				AGalaxy* Galaxy = GetIndexedGalaxy(World);
				TArray<FNearStar> Stars;
				const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 200) : 12;
				if (!Pawn || !Galaxy || !FindNearStars(World, Pawn->GetActorLocation(), Count, 1.0e30, Stars))
				{
					UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] near: no player or no nearest-star index yet"));
					return;
				}
				constexpr double AuCm = 1.495978707e13;
				for (const FNearStar& Star : Stars)
				{
					FGalaxyCatalogStarRecord Record;
					Galaxy->StarCatalog.ResolveStar(Star.CatalogIndex, Record);
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] near %.3f AU: catalogue %lld (ordinal %d) class %d.%d radius x%.2f %s"),
						Star.DistanceCm / AuCm, Star.CatalogIndex, Star.Ordinal, static_cast<int32>(Record.SpectralClass),
						Record.SpectralSubclass, Record.RadiusScale, Record.bPotentialStarSystem ? TEXT("system") : TEXT("lone"));
				}
			}));
	}

	AGalaxy* GetIndexedGalaxy(const UWorld* World)
	{
		AGalaxy* Galaxy = GIndexGalaxy.Get();
		return GIndex.IsValid() && IsValid(Galaxy) && Galaxy->GetWorld() == World ? Galaxy : nullptr;
	}

	bool FindNearStars(const UWorld* World, const FVector& WorldLocation, const int32 MaxCount, const double MaxDistanceCm,
		TArray<FNearStar>& OutStars)
	{
		OutStars.Reset();
		AGalaxy* Galaxy = GetIndexedGalaxy(World);
		FTransform LocalToWorld;
		if (!Galaxy || MaxCount <= 0 || !(MaxDistanceCm > 0.0) || !MakeGameplayLocalToWorld(*Galaxy, LocalToWorld))
		{
			return false;
		}
		const FStarIndex& Index = *GIndex;
		const double Scale = LocalToWorld.GetMaximumAxisScale();
		if (!(Scale > 0.0) || Index.Resolution <= 0)
		{
			return false;
		}
		const FVector LocalCentre = LocalToWorld.InverseTransformPosition(WorldLocation);
		const FVector3f Centre(LocalCentre);
		const double MaxLocal = MaxDistanceCm / Scale;
		const double MaxLocalSq = MaxLocal * MaxLocal;
		// A centre outside the grid (far beyond the galaxy) searches from its nearest cell, with the distance check exact.
		const FIntVector Home = Index.CellOf(Centre);
		// At most a fifth of the grid out: past the disc (an empty corner of the box) the rings would walk ~10^5 cells.
		// (The ratio is clamped as a double first: an "unbounded" distance of 1e30 cm would overflow the int conversion.)
		const int32 MaxRing = FMath::Min(FMath::CeilToInt(FMath::Min(MaxLocal / Index.CellSize, static_cast<double>(Index.Resolution))) + 1,
			NearRingLimit);
		TArray<TPair<double, int32>> Found;
		for (int32 Ring = 0; Ring <= MaxRing; ++Ring)
		{
			for (int32 Z = Home.Z - Ring; Z <= Home.Z + Ring; ++Z)
			{
				if (Z < 0 || Z >= Index.Resolution) continue;
				for (int32 Y = Home.Y - Ring; Y <= Home.Y + Ring; ++Y)
				{
					if (Y < 0 || Y >= Index.Resolution) continue;
					const bool bShellYZ = FMath::Abs(Z - Home.Z) == Ring || FMath::Abs(Y - Home.Y) == Ring;
					for (int32 X = Home.X - Ring; X <= Home.X + Ring; X += (bShellYZ || Ring == 0) ? 1 : 2 * Ring)
					{
						if (X < 0 || X >= Index.Resolution) continue;
						const int32 Cell = Index.LinearOf(FIntVector(X, Y, Z));
						for (int32 Slot = Index.CellStart[Cell]; Slot < Index.CellStart[Cell + 1]; ++Slot)
						{
							const double DistanceSq = FVector::DistSquared(LocalCentre, FVector(Index.Positions[Slot]));
							if (DistanceSq <= MaxLocalSq)
							{
								Found.Emplace(DistanceSq, Slot);
							}
						}
					}
				}
			}
			// Every star in a further ring is at least Ring cells away: stop once MaxCount nearer ones are known.
			if (Found.Num() >= MaxCount)
			{
				Found.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
				Found.SetNum(MaxCount, EAllowShrinking::No);
				const double Reach = static_cast<double>(Ring) * Index.CellSize;
				if (Found.Last().Key <= Reach * Reach)
				{
					break;
				}
			}
		}
		Found.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
		if (Found.Num() > MaxCount)
		{
			Found.SetNum(MaxCount, EAllowShrinking::No);
		}
		OutStars.Reserve(Found.Num());
		// Rio 05.10 (real scale, stage 2): the index keeps float positions, ~10 AU off at REAL SCALE; the stars found are
		// placed exactly from their catalogue record (as RegisterGalaxyStar and the materializer place them), sorted again.
		const bool bExact = APSRealScale::IsActive(World);
		for (const TPair<double, int32>& Hit : Found)
		{
			FNearStar& Star = OutStars.AddDefaulted_GetRef();
			Star.Ordinal = Index.Ordinals[Hit.Value];
			Star.CatalogIndex = Index.Order.Resolve(Star.Ordinal);
			Star.WorldLocation = LocalToWorld.TransformPosition(FVector(Index.Positions[Hit.Value]));
			Star.DistanceCm = FMath::Sqrt(Hit.Key) * Scale;
			FGalaxyCatalogStarRecord Record;
			if (bExact && Galaxy->StarCatalog.ResolveStar(Star.CatalogIndex, Record))
			{
				Star.WorldLocation = LocalToWorld.TransformPosition(Record.GalaxyLocalLocation);
				Star.DistanceCm = FVector::Dist(Star.WorldLocation, WorldLocation);
			}
		}
		if (bExact)
		{
			OutStars.Sort([](const FNearStar& A, const FNearStar& B) { return A.DistanceCm < B.DistanceCm; });
		}
		return true;
	}

	bool ProjectCatalogueLocation(const UWorld* World, const FVector& GalaxyLocalLocation, FVector& OutWorldLocation)
	{
		AGalaxy* Galaxy = GetIndexedGalaxy(World);
		FTransform LocalToWorld;
		if (!Galaxy || !MakeGameplayLocalToWorld(*Galaxy, LocalToWorld))
		{
			return false;
		}
		OutWorldLocation = LocalToWorld.TransformPosition(GalaxyLocalLocation);
		return true;
	}

	bool PickAlongRay(const UWorld* World, const FVector& RayOrigin, const FVector& RayDirection, const double MaxAngleRadians,
		FNearStar& OutStar)
	{
		AGalaxy* Galaxy = GetIndexedGalaxy(World);
		FTransform LocalToWorld;
		if (!Galaxy || !MakeGameplayLocalToWorld(*Galaxy, LocalToWorld) || RayDirection.IsNearlyZero() || !(MaxAngleRadians > 0.0))
		{
			return false;
		}
		const FStarIndex& Index = *GIndex;
		const double Scale = LocalToWorld.GetMaximumAxisScale();
		const FVector Origin = LocalToWorld.InverseTransformPosition(RayOrigin);
		const FVector Direction = LocalToWorld.InverseTransformVectorNoScale(RayDirection).GetSafeNormal();
		const double MaxTangentSq = FMath::Square(FMath::Tan(FMath::Min(MaxAngleRadians, 0.5)));
		// Chunks in parallel, each keeping its best (smallest angle, then nearer).
		constexpr int32 ChunkSize = 65536;
		const int32 Count = Index.Positions.Num();
		const int32 Chunks = FMath::DivideAndRoundUp(Count, ChunkSize);
		struct FBest
		{
			int32 Slot{INDEX_NONE};
			double TangentSq{TNumericLimits<double>::Max()};
			double Along{TNumericLimits<double>::Max()};
		};
		TArray<FBest> Best;
		Best.SetNum(Chunks);
		ParallelFor(Chunks, [&](const int32 Chunk)
		{
			FBest& Mine = Best[Chunk];
			const int32 End = FMath::Min(Count, (Chunk + 1) * ChunkSize);
			for (int32 Slot = Chunk * ChunkSize; Slot < End; ++Slot)
			{
				const FVector Offset = FVector(Index.Positions[Slot]) - Origin;
				const double Along = FVector::DotProduct(Offset, Direction);
				if (Along <= 0.0) continue;
				const double TangentSq = (Offset.SizeSquared() - Along * Along) / (Along * Along);
				if (TangentSq > MaxTangentSq) continue;
				// Within a tenth of the tolerance two stars count as one direction: the nearer wins.
				const bool bSameDirection = FMath::Abs(TangentSq - Mine.TangentSq) < MaxTangentSq * 0.01;
				if (bSameDirection ? Along < Mine.Along : TangentSq < Mine.TangentSq)
				{
					Mine.Slot = Slot;
					Mine.TangentSq = TangentSq;
					Mine.Along = Along;
				}
			}
		});
		const FBest* Winner = nullptr;
		for (const FBest& Each : Best)
		{
			if (Each.Slot == INDEX_NONE) continue;
			if (!Winner || (FMath::Abs(Each.TangentSq - Winner->TangentSq) < MaxTangentSq * 0.01
				? Each.Along < Winner->Along : Each.TangentSq < Winner->TangentSq))
			{
				Winner = &Each;
			}
		}
		if (!Winner)
		{
			return false;
		}
		OutStar.Ordinal = Index.Ordinals[Winner->Slot];
		OutStar.CatalogIndex = Index.Order.Resolve(OutStar.Ordinal);
		OutStar.WorldLocation = LocalToWorld.TransformPosition(FVector(Index.Positions[Winner->Slot]));
		// Rio 05.10 (real scale, stage 2): placed exactly from its catalogue record at REAL SCALE (see FindNearStars).
		FGalaxyCatalogStarRecord Record;
		if (APSRealScale::IsActive(World) && Galaxy->StarCatalog.ResolveStar(OutStar.CatalogIndex, Record))
		{
			OutStar.WorldLocation = LocalToWorld.TransformPosition(Record.GalaxyLocalLocation);
		}
		OutStar.DistanceCm = FVector::Dist(OutStar.WorldLocation, RayOrigin);
		return true;
	}

	bool GetIndexedBounds(const UWorld* World, FVector& OutCentre, double& OutRadiusCm)
	{
		AGalaxy* Galaxy = GetIndexedGalaxy(World);
		FTransform LocalToWorld;
		if (!Galaxy || !MakeGameplayLocalToWorld(*Galaxy, LocalToWorld) || !(GIndex->BoundRadius > 0.0f))
		{
			return false;
		}
		OutCentre = LocalToWorld.GetLocation();
		OutRadiusCm = static_cast<double>(GIndex->BoundRadius) * LocalToWorld.GetMaximumAxisScale();
		return FMath::IsFinite(OutRadiusCm) && OutRadiusCm > 0.0;
	}

	void SetWorldDaylightVisibility(const UWorld* World, const float Visibility)
	{
		if (GActiveLayers <= 0 || World == nullptr)
		{
			return;
		}
		// Rio 04.10: the end values always go through. A near-equal skip could keep a 0.001 layer in full daylight: the
		// passes kept running (and its dust kept dimming the sky) instead of switching off at an exact 0.
		// Rio 04.10 ("leaving the planet the stars come out in jerks, not smoothly as before"): a relative step, as the
		// catalogue points' material takes it (ApplyPointVisibility). The fade starts at 0.2%, where a fixed 0.001 step
		// raised the brightest stars by half at once.
		const bool bSameEnds = (Visibility <= 0.0f) == (GVisibility <= 0.0f) && (Visibility >= 1.0f) == (GVisibility >= 1.0f);
		if (GVisibilityWorld.Get() == World && bSameEnds
			&& FMath::Abs(Visibility - GVisibility) <= 0.01f * FMath::Max(Visibility, 0.0005f))
		{
			return;
		}
		GVisibilityWorld = World;
		GVisibility = Visibility;
		APSStarRenderer::SetWorldVisibility(World, Visibility);
	}

	void SetWorldSystemGlare(const UWorld* World, const float Others, const int64 OwnCatalogIndex, const float Own)
	{
		// Rio 06.10 (aps.Stars.SystemGlare): per set, not the scene value: the daylight owns SetWorldVisibility and multiplies
		// on top, so neither overwrites the other. Sent on a 1% relative step and at the full end, like the daylight.
		const float OthersWanted = FMath::IsFinite(Others) ? FMath::Clamp(Others, 0.0f, 1.0f) : 1.0f;
		const float OwnWanted = FMath::IsFinite(Own) ? FMath::Clamp(Own, 0.0f, 1.0f) : 1.0f;
		const auto ShouldSendGlare = [](const float Wanted, const float Applied)
		{
			return Wanted != Applied && (Wanted >= 1.0f || FMath::Abs(Wanted - Applied) > 0.01f * FMath::Max(Wanted, 5.0e-4f));
		};
		static IConsoleVariable* GlareLogVariable = nullptr;
		const IConsoleVariable* GlareLog = FindVariable(GlareLogVariable, TEXT("aps.Stars.SystemGlareLog"));
		const bool bLogSends = GlareLog && GlareLog->GetInt() != 0;
		GSystemGlareWorld = World;
		GSystemGlareOthers = OthersWanted;
		bool bSent = false;
		if (OwnCatalogIndex != GSystemGlareOwnIndex)
		{
			GSystemGlareOwnIndex = OwnCatalogIndex;
			++GSystemGlareSerial;
			bSent = true;
		}
		if (ShouldSendGlare(OwnWanted, GSystemGlareAppliedOwn))
		{
			GSystemGlareAppliedOwn = OwnWanted;
			++GSystemGlareSerial;
			bSent = true;
		}
		// The level sets (every twin) and the glow: the gameplay layer of this world only, never the menu's.
		AGalaxy* Galaxy = GLayer.Galaxy.Get();
		const bool bLayer = World != nullptr && GLayer.Mode == ELayerMode::Gameplay && IsValid(Galaxy) && Galaxy->GetWorld() == World;
		if (bLayer && ShouldSendGlare(OthersWanted, GLayer.AppliedPointVisibility))
		{
			for (int32 Index = 0; Index < GLayer.PointSets.Num(); ++Index)
			{
				APSStarRenderer::SetVisibility(GLayer.PointSets[Index], OthersWanted);
				if (GLayer.PointDescs.IsValidIndex(Index))
				{
					GLayer.PointDescs[Index].Visibility = OthersWanted;
				}
			}
			GLayer.AppliedPointVisibility = OthersWanted;
			++GSystemGlareSerial;
			bSent = true;
		}
		if (bLayer && Galaxy->GpuGlowVolume != 0 && ShouldSendGlare(OthersWanted, GLayer.AppliedGlowVisibility))
		{
			APSStarRenderer::SetVisibility(Galaxy->GpuGlowVolume, OthersWanted);
			GLayer.AppliedGlowVisibility = OthersWanted;
			bSent = true;
		}
		const bool bOn = OthersWanted < 1.0f || OwnWanted < 1.0f;
		if (bOn != GSystemGlareLoggedOn || (bSent && bLogSends))
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] system glare %s: others %.4f, own %lld %.4f (%d set(s), glow %s)"),
				bOn ? TEXT("on") : TEXT("off"), bLayer ? GLayer.AppliedPointVisibility : OthersWanted,
				static_cast<long long>(GSystemGlareOwnIndex), GSystemGlareAppliedOwn, bLayer ? GLayer.PointSets.Num() : 0,
				bLayer && Galaxy->GpuGlowVolume != 0 ? TEXT("on") : TEXT("off"));
			GSystemGlareLoggedOn = bOn;
		}
	}

	namespace
	{
		/** Rio 05.10 night: the catalogue stars APSGalaxyNearStars shows as photospheres now (one world at a time). */
		TSet<int64> GNearPhotospheres;
		TWeakObjectPtr<const UWorld> GNearPhotosphereWorld;

		float ConsoleFloat(IConsoleVariable*& Cache, const TCHAR* Name, const float Fallback)
		{
			const IConsoleVariable* Variable = FindVariable(Cache, Name);
			return Variable ? Variable->GetFloat() : Fallback;
		}

		const TCHAR* LookName(const EApproachLook Look)
		{
			return Look == EApproachLook::Point ? TEXT("point") : Look == EApproachLook::Crossfade ? TEXT("crossfade") : TEXT("sphere");
		}

		/** The angle between two directions from the camera, in pixels of the view. */
		double PixelsApart(const FVector& A, const FVector& B, const double PixelTangent)
		{
			return FMath::Atan2((A ^ B).Size(), A | B) / FMath::Max(PixelTangent, 1.0e-12);
		}

		/** Rio 05.10 night: a system that stands now, in the frame the sky is drawn in. */
		struct FStandingSystem
		{
			FVector CentreInSky = FVector::ZeroVector;
			double ExclusionRadiusCm = 0.0;
			bool bHome = false;
			/** Its star: where the sky has it, where its sphere is drawn, its radius (0: no star) and whether it is shown. */
			FVector StarInSky = FVector::ZeroVector;
			FVector StarDrawn = FVector::ZeroVector;
			double StarRadiusCm = 0.0;
			bool bStarShown = false;
			/** Rio 06.10 (aps.Stars.ApproachOwnByIdentity): its star's name (FAPSSystemMaterializer::Begin gives the registry's). */
			FName StarName;
		};
		using FStandingSystems = TArray<FStandingSystem, TInlineAllocator<4>>;

		/** Rio 06.10 (still ship): the system actors keep their world place while a deferred travel moves only the sky. */
		void GatherStandingSystems(UWorld* World, FStandingSystems& OutSystems)
		{
			const AStarSystem* Home = GGameplay.Home.Get();
			for (TActorIterator<AStarSystem> It(World); It; ++It)
			{
				const AStarSystem* System = *It;
				if (!IsValid(System) || !(System->StarSystemRadius > 0.0) || !FMath::IsFinite(System->StarSystemRadius))
				{
					continue;
				}
				FStandingSystem& Entry = OutSystems.AddDefaulted_GetRef();
				Entry.CentreInSky = UAPSWorldOriginSubsystem::SkyPlace(*System);
				Entry.ExclusionRadiusCm = System->StarSystemRadius * APSCanonicalStellarProjection::SystemProxyExclusionPadding;
				Entry.bHome = System == Home;
				if (const AStar* Star = System->MainStar; IsValid(Star))
				{
					Entry.StarDrawn = Star->GetActorLocation();
					Entry.StarInSky = UAPSWorldOriginSubsystem::SkyPlace(*Star);
					Entry.StarRadiusCm = FMath::Max(static_cast<double>(Star->StarRadiusKM), 1.0) * 100000.0;
					Entry.bStarShown = !Star->IsHidden();
					Entry.StarName = Star->AstroName;
				}
			}
		}

		/** The system this star stands up as (APSSystemMaterializer puts its star exactly at the catalogue place), or null. */
		const FStandingSystem* OwnSystemOf(const FStandingSystems& Systems, const FVector& ExactInSky)
		{
			for (const FStandingSystem& System : Systems)
			{
				if (System.StarRadiusCm > 0.0 && FVector::DistSquared(System.StarInSky, ExactInSky)
					<= FMath::Square(FMath::Max(System.StarRadiusCm, 1.0e10) * 3.0))
				{
					return &System;
				}
			}
			return nullptr;
		}

		/** Inside the sphere of the home or of a system that is not its own: the level sets hide it, so no approach point. */
		bool InForeignSystem(const FStandingSystems& Systems, const FVector& ExactInSky, const FStandingSystem* Own)
		{
			for (const FStandingSystem& System : Systems)
			{
				if ((&System != Own || System.bHome)
					&& FVector::DistSquared(System.CentreInSky, ExactInSky) <= FMath::Square(System.ExclusionRadiusCm))
				{
					return true;
				}
			}
			return false;
		}

		/** Rio 06.10 (star approach v2, stage B): a catalogue star's quantized twin as its level set packs and draws it. */
		struct FTwinInfo
		{
			FGalaxyCatalogStarRecord Record;
			int32 Level = 0;
			int32 TwinSet = INDEX_NONE;
			FVector3f TwinLocal = FVector3f::ZeroVector;
			uint8 ColorIndex = 0;
			float Intensity = 0.0f;
			/** Its code is not dark (the layer draws it). */
			bool bLit = false;
			float TwinRadiusLocal = 0.0f;
			/** The twin's distance from the exact catalogue place, local units. */
			double OffsetLocal = 0.0;
		};

		/**
		 * Rio 06.10 (stage B): the level BuildOnWorker chose for a star (the innermost cube around the home that holds it), its
		 * set and its packed twin: the take, the trace and the course star share it. False without a record (Out.Record keeps
		 * CatalogIndex INDEX_NONE) or without a set for its level.
		 */
		bool ResolveTwinOf(const AGalaxy& Galaxy, const int64 CatalogIndex, FTwinInfo& Out)
		{
			Out = FTwinInfo();
			if (CatalogIndex == INDEX_NONE || !Galaxy.StarCatalog.ResolveStar(CatalogIndex, Out.Record) || Out.Record.CatalogIndex == INDEX_NONE)
			{
				Out.Record.CatalogIndex = INDEX_NONE;
				return false;
			}
			const FVector3f Position(Out.Record.GalaxyLocalLocation);
			int32 Level = 0;
			if (GLayer.LevelHalf.Num() > 1)
			{
				const FVector3f FromHome = (Position - GLayer.HomeLocal).GetAbs();
				const float Chebyshev = FromHome.GetMax();
				for (int32 Candidate = GLayer.LevelHalf.Num() - 1; Candidate >= 1; --Candidate)
				{
					if (Chebyshev <= GLayer.LevelHalf[Candidate])
					{
						Level = Candidate;
						break;
					}
				}
			}
			Out.Level = Level;
			Out.TwinSet = GLayer.PointLevels.IndexOfByKey(Level);
			if (!GLayer.PointDescs.IsValidIndex(Out.TwinSet) || !GLayer.PointSets.IsValidIndex(Out.TwinSet))
			{
				return false;
			}
			const FBox3f& Bounds = GLayer.PointDescs[Out.TwinSet].LocalBounds;
			Out.ColorIndex = ColorIndexFor(Out.Record);
			Out.Intensity = GameplayIntensityFor(Out.Record,
				GLayer.PaletteLuminance.IsValidIndex(Out.ColorIndex) ? GLayer.PaletteLuminance[Out.ColorIndex] : 1.0f, GLayer.G0Intensity);
			const APSStarRenderer::FPackedStar Packed = APSStarRenderer::PackStar(Position, Bounds, Out.ColorIndex, Out.Intensity);
			Out.bLit = (Packed.ZColorIntensity >> 24) != 0u;
			Out.TwinLocal = APSStarRenderer::UnpackStarPosition(Packed, Bounds);
			// Its own lattice point only: half the finest step, never below a few float steps of the shader's decode.
			const FVector3f Step = (Bounds.Max - Bounds.Min) / 65535.0f;
			Out.TwinRadiusLocal = FMath::Max(0.5f * Step.GetMin(), 4.8e-7f * FMath::Max(Out.TwinLocal.GetAbsMax(), 1.0f));
			Out.OffsetLocal = FVector::Dist(FVector(Out.TwinLocal), Out.Record.GalaxyLocalLocation);
			return true;
		}

		/**
		 * Rio 06.10 (stage B): a star's ordinal in the drawn sequence, matched by catalogue index among the drawn stars nearest
		 * its exact place (the trace's way: the index keeps float places, ~10 AU off at REAL SCALE). On a course or active
		 * change and in the bench only, never per frame. True when the GPU layer draws it (OutOrdinal INDEX_NONE: not found).
		 */
		bool GpuOrdinalOf(const UWorld* World, const int64 CatalogIndex, const FVector& GalaxyLocal, const FTransform& LocalToWorld,
			int32& OutOrdinal)
		{
			OutOrdinal = INDEX_NONE;
			TArray<FNearStar> Candidates;
			FindNearStars(World, LocalToWorld.TransformPosition(GalaxyLocal), 8, 1.0e16, Candidates);
			const FNearStar* Found = Candidates.FindByPredicate([CatalogIndex](const FNearStar& Candidate)
			{
				return Candidate.CatalogIndex == CatalogIndex;
			});
			if (!Found)
			{
				return false;
			}
			OutOrdinal = Found->Ordinal;
			const int64 GpuOrdinal = static_cast<int64>(Found->Ordinal) - GLayer.FirstOrdinal;
			return GpuOrdinal >= 0 && GpuOrdinal < GLayer.PointOrdinals;
		}

		/** Rio 06.10 (aps.Stars.ApproachMapGain): the strategic map hold an approach point carries (the level sets' own). */
		float MapGainTarget()
		{
			return CVarApproachMapGain.GetValueOnGameThread() != 0 ? GLayer.AppliedMapGain : 1.0f;
		}

		/**
		 * Rio 06.10 (aps.Stars.SystemGlare): an approach point's visibility before its crossfade: the own glare value for the
		 * standing system's star, else its level's (its twin's), so point and twin dim together at every take and release.
		 */
		float ApproachBaseVisibility(const FApproachPoint& Point)
		{
			const AGalaxy* LayerGalaxy = GLayer.Galaxy.Get();
			if (Point.CatalogIndex != INDEX_NONE && Point.CatalogIndex == GSystemGlareOwnIndex && LayerGalaxy != nullptr
				&& GSystemGlareWorld.Get() == LayerGalaxy->GetWorld())
			{
				return GSystemGlareAppliedOwn;
			}
			return GLayer.PointDescs.IsValidIndex(Point.TwinSet) ? GLayer.PointDescs[Point.TwinSet].Visibility : 1.0f;
		}

		/** Rio 06.10 (aps.Stars.ApproachHandBackSeconds): how far a started hand-back has come (1: on its twin). */
		double HandBackAlpha(const FApproachPoint& Point, const double WorldNow)
		{
			const double Seconds = FMath::Max(static_cast<double>(CVarApproachHandBackSeconds.GetValueOnGameThread()), 0.0);
			return Seconds > 0.0 ? FMath::SmoothStep(0.0, 1.0, (WorldNow - Point.HandBackStart) / Seconds) : 1.0;
		}

		/**
		 * Rio 05.10 night, 06.10 (stage B): the point's way from its twin (1) to the exact place (0) by the pilot's distance,
		 * (d / d0)^2: its angle off the exact direction shrinks linearly with the distance, so it never runs ahead of the
		 * twin's own error. A hand-back eases it from where it was onto the twin (aps.Stars.ApproachHandBackSeconds).
		 */
		double GlideOf(const FApproachPoint& Point, const FVector& PilotLocal, const double WorldNow)
		{
			const double Distance = FVector::Dist(PilotLocal, Point.ExactLocal);
			double Way = FMath::Clamp(FMath::Square(Distance / FMath::Max(Point.GlideFromLocal, 1.0e-12)), 0.0, 1.0);
			if (Point.HandBackStart >= 0.0)
			{
				Way = FMath::Lerp(Point.HandBackFromGlide, 1.0, HandBackAlpha(Point, WorldNow));
			}
			return Way;
		}

		/**
		 * Rio 05.10 night: where an approach point stands this frame (catalogue-local). From where its twin is drawn (the
		 * raster shader's float maths: the twin's decoded place minus the view's place rounded to float, so taking over and
		 * handing back do not move it), at Glide (GlideOf) between it and the exact place. Lifted two radii towards the view,
		 * in front of its own sphere, whose depth would otherwise drop it pixel by pixel while the disc is still sub-pixel.
		 * Rio 06.10 (stage B, change 12): the glide comes from the pilot's eyes, the twin's float maths and the lift from the
		 * view (the same as stage A while the view is the pilot's).
		 */
		FVector PlaceApproachPoint(const FApproachPoint& Point, const FVector& ViewLocal, const double Glide, const double Scale,
			FVector& OutTwinDrawn)
		{
			const FVector3f ViewLocalF(ViewLocal);
			OutTwinDrawn = ViewLocal + FVector(Point.TwinLocal - ViewLocalF);
			const double Distance = FVector::Dist(ViewLocal, Point.ExactLocal);
			const FVector Glided = Point.ExactLocal + (OutTwinDrawn - Point.ExactLocal) * Glide;
			const double Lift = FMath::Min(2.0 * Point.RadiusCm / FMath::Max(Scale, 1.0e-30), 0.25 * Distance);
			return Glided + (ViewLocal - Glided).GetSafeNormal() * Lift;
		}

		/**
		 * Rio 06.10 (stage B): an approach point's one-point set as the take registers it: its level's photometry with its own
		 * scale (never the level's strategic-map hold) times Gain (aps.Stars.ApproachMapGain), its pushed transform and box,
		 * no exclusions, ahead of every level, at its applied visibility. The take and the map gain sync both use it.
		 */
		APSStarRenderer::FPointSetDesc MakeApproachDesc(const FApproachPoint& Point, const APSStarRenderer::FPointSetDesc& LevelDesc,
			const float Gain)
		{
			APSStarRenderer::FPointSetDesc Desc = LevelDesc;
			// Rio 06.10 (strategic map GALAXY): never the level's map hold by itself. An approach point stands within ~3 ly of
			// the camera and shines by its own inverse square.
			Desc.IntensityScale = (GLayer.PointIntensityScale > 0.0f ? GLayer.PointIntensityScale : LevelDesc.IntensityScale) * Gain;
			Desc.LocalToWorld = Point.PushedTransform;
			Desc.LocalBounds = FBox3f(FVector3f::ZeroVector, FVector3f(ApproachBoxLocal));
			Desc.ExclusionCenterLocal = FVector3f::ZeroVector;
			Desc.ExclusionRadiusLocal = 0.0f;
			Desc.ExtraExclusionSpheresLocal.Reset();
			// Ahead of every level under aps.Stars.GpuPointBudget.
			Desc.Priority = GameplayLevels + 1;
			Desc.Population = 1;
			Desc.DebugName = FString::Printf(TEXT("GameplayApproach %lld"), Point.CatalogIndex);
			Desc.Visibility = Point.AppliedVisibility;
			return Desc;
		}

		/**
		 * Rio 06.10 (stage B, aps.Stars.ApproachExposureLog): the view's pre-exposure once after a take or a crossfade start
		 * (the approach line's pre-clamp is "x pre-exposure"). A view extension armed for one game-thread SetupView of a
		 * player view; scene, reflection and planar captures are skipped. Log only.
		 */
		class FAPSApproachExposureProbe final : public FSceneViewExtensionBase
		{
		public:
			explicit FAPSApproachExposureProbe(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

			std::atomic<bool> bArmed{false};
			const TCHAR* ArmedReason = TEXT("");

			virtual void SetupViewFamily(FSceneViewFamily&) override {}
			virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
			virtual void SetupView(FSceneViewFamily&, FSceneView& InView) override
			{
				if (!bArmed.load() || InView.bIsSceneCapture || InView.bIsReflectionCapture || InView.bIsPlanarReflection)
				{
					return;
				}
				bArmed.store(false);
				static IConsoleVariable* const PreExposureOverride =
					IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.PreExposureOverride"), false);
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Stars] view pre-exposure %.6g (last eye adaptation %.6g, r.EyeAdaptation.PreExposureOverride %.3g) at %s"),
					InView.State ? InView.State->GetPreExposure() : -1.0f, InView.GetLastEyeAdaptationExposure(),
					PreExposureOverride ? PreExposureOverride->GetFloat() : -1.0f, ArmedReason);
			}

		protected:
			virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext&) const override
			{
				return bArmed.load() && CVarApproachExposureLog.GetValueOnAnyThread() != 0;
			}
		};
		TSharedPtr<FAPSApproachExposureProbe, ESPMode::ThreadSafe> GApproachExposureProbe;
		double GApproachExposureNextSeconds = 0.0;
		int32 GApproachExposureArms = 0;

		/** Rio 06.10 (aps.Stars.ApproachExposureLog): arms the probe (at most every 10 s, 20 a session). */
		void ArmExposureProbe(const TCHAR* Reason)
		{
			if (CVarApproachExposureLog.GetValueOnGameThread() == 0 || GApproachExposureArms >= 20)
			{
				return;
			}
			const double Seconds = FPlatformTime::Seconds();
			if (Seconds < GApproachExposureNextSeconds)
			{
				return;
			}
			GApproachExposureNextSeconds = Seconds + 10.0;
			++GApproachExposureArms;
			if (!GApproachExposureProbe.IsValid())
			{
				GApproachExposureProbe = FSceneViewExtensions::NewExtension<FAPSApproachExposureProbe>();
			}
			GApproachExposureProbe->ArmedReason = Reason;
			GApproachExposureProbe->bArmed.store(true);
		}

		/**
		 * Rio 05.10 night: an approach point for a drawn GPU star, taken over where its twin stands now.
		 * Rio 06.10 (aps.Stars.ApproachRetakeAtExact): inside its own standing system (bStartAtExact) the twin is hidden, so the
		 * point starts at the exact place, at the crossfade's current value (InitialFade).
		 * Rio 06.10 (stage B, change 12): the distance and the glide from the pilot's eyes, the place and the logged pixels from
		 * the view. It never checks budgets (callers do).
		 */
		bool TakeApproachPoint(UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld, const FVector& ViewLocal,
			const FVector& PilotLocal, const FNearStar& Star, const double TakeLocal, const double ViewPixelTangent,
			const bool bStartAtExact = false, const float InitialFade = 1.0f, const double WorldNow = 0.0)
		{
			FTwinInfo Twin;
			if (!ResolveTwinOf(Galaxy, Star.CatalogIndex, Twin) || !Twin.bLit)
			{
				return false; // No record or set; or a dark code: the layer does not draw it either.
			}
			const double Scale = LocalToWorld.GetMaximumAxisScale();
			const FGalaxyCatalogStarRecord& Record = Twin.Record;
			const APSStarRenderer::FPointSetDesc& LevelDesc = GLayer.PointDescs[Twin.TwinSet];
			FApproachPoint Point;
			Point.CatalogIndex = Record.CatalogIndex;
			Point.Ordinal = Star.Ordinal;
			Point.Level = Twin.Level;
			Point.TwinSet = Twin.TwinSet;
			Point.TwinLocal = Twin.TwinLocal;
			Point.TwinRadiusLocal = Twin.TwinRadiusLocal;
			Point.ExactLocal = Record.GalaxyLocalLocation;
			Point.CatalogueLocal = Point.ExactLocal;
			Point.RadiusCm = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
				* FMath::Max(static_cast<double>(Record.RadiusScale), 0.0) * APSCanonicalStellarProjection::SolarRadiusCm;
			Point.Intensity = Twin.Intensity;
			const double DistanceLocal = FVector::Dist(PilotLocal, Point.ExactLocal);
			// Rio 06.10 (aps.Stars.ApproachRetakeAtExact): a take inside its own standing system glides from the take radius, so
			// it stands at the exact place now ((d / TakeLocal)^2), not on its hidden twin with the glide pinned at 1.
			Point.GlideFromLocal = bStartAtExact ? FMath::Max(TakeLocal, 1.0e-9) : FMath::Max(FMath::Min(DistanceLocal, TakeLocal), 1.0e-9);
			Point.Fade = FMath::Clamp(InitialFade, 0.0f, 1.0f);
			Point.Look = Point.Fade >= 1.0f ? EApproachLook::Point : Point.Fade <= 0.0f ? EApproachLook::Sphere : EApproachLook::Crossfade;
			const double Glide = GlideOf(Point, PilotLocal, WorldNow);
			FVector TwinDrawn;
			Point.DrawLocal = PlaceApproachPoint(Point, ViewLocal, Glide, Scale, TwinDrawn);
			Point.PushedTransform = FTransform(FQuat::Identity, Point.DrawLocal) * LocalToWorld;
			// Rio 06.10 (aps.Stars.ApproachRetakeAtExact): registered at the crossfade's value, so its first frame is no flash.
			// Rio 06.10 (aps.Stars.SystemGlare): from its base value (its twin's, or the own star's glare).
			Point.AppliedVisibility = ApproachBaseVisibility(Point);
			if (Point.Fade < 1.0f)
			{
				Point.AppliedVisibility *= Point.Fade;
			}
			Point.GlareSerial = GSystemGlareSerial;
			Point.AppliedGain = MapGainTarget();
			Point.LastPilotCm = DistanceLocal * Scale;
			const APSStarRenderer::FPointSetDesc Desc = MakeApproachDesc(Point, LevelDesc, Point.AppliedGain);
			TArray<APSStarRenderer::FPackedStar> Points;
			Points.Add(APSStarRenderer::PackStar(FVector3f::ZeroVector, Desc.LocalBounds, Twin.ColorIndex, Twin.Intensity));
			Point.Handle = APSStarRenderer::RegisterPointSet(World, Desc, MoveTemp(Points));
			if (Point.Handle == 0)
			{
				return false;
			}
			GLayer.Approach.Add(Point);
			// The twin goes in the same frame the point comes (both reach the render thread before it draws).
			PushSetExclusions(Twin.TwinSet);
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Stars] approach point %lld (ordinal %d, L%d) takes over from its quantized point at %.4g ly: the twin is ")
				TEXT("%.0f AU off the exact place (%.2f px now), taken over %.3f px from where it is drawn; the way in eases it ")
				TEXT("to the exact place"),
				Point.CatalogIndex, Point.Ordinal, Twin.Level, DistanceLocal * Scale / LightYearCm,
				Twin.OffsetLocal * Scale / AstronomicalUnitCm,
				PixelsApart(TwinDrawn - ViewLocal, Point.ExactLocal - ViewLocal, ViewPixelTangent),
				PixelsApart(Point.DrawLocal - ViewLocal, TwinDrawn - ViewLocal, ViewPixelTangent));
			if (bStartAtExact)
			{
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Stars] approach point %lld taken inside its own standing system: at the exact place (glide %.4f, ")
					TEXT("its twin hidden there), point %.3f"), Point.CatalogIndex, Glide, Point.Fade);
			}
			ArmExposureProbe(TEXT("take"));
			return true;
		}

		/** Removes approach points (their twins come back in the same frame). Index INDEX_NONE: all of them. */
		void ReleaseApproachPoints(const int32 Only, const TCHAR* Reason, const double DistanceCm = -1.0)
		{
			TArray<int32, TInlineAllocator<4>> Sets;
			for (int32 Index = GLayer.Approach.Num() - 1; Index >= 0; --Index)
			{
				if (Only != INDEX_NONE && Index != Only)
				{
					continue;
				}
				const FApproachPoint& Point = GLayer.Approach[Index];
				APSStarRenderer::Remove(Point.Handle);
				Sets.AddUnique(Point.TwinSet);
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld (ordinal %d) back to its quantized point (%s%s)"),
					Point.CatalogIndex, Point.Ordinal, Reason,
					DistanceCm >= 0.0 ? *FString::Printf(TEXT(", %.4g ly out"), DistanceCm / LightYearCm) : TEXT(""));
				GLayer.Approach.RemoveAt(Index);
			}
			for (const int32 Set : Sets)
			{
				PushSetExclusions(Set);
			}
		}

		/** Rio 06.10 (star approach v2): the F10 strategic map is open (its own camera holds the view). */
		bool IsMapOpen(const UWorld* World)
		{
			const AGravityPlayerController* Controller = World ? Cast<AGravityPlayerController>(World->GetFirstPlayerController()) : nullptr;
			return Controller && Controller->IsStrategicMapOpen();
		}

		/** Rio 06.10 (star approach v2): the galaxy star the materializer stands now (FActiveGalaxySystem, with FLayerState). */
		FActiveGalaxySystem FindActiveGalaxySystem(const UWorld* World)
		{
			FActiveGalaxySystem Active;
			const FAPSStarSystems* Registry = APSStarSystemsFind(World);
			const FAPSSystemMaterializer* Materializer = Registry ? Registry->GetMaterializer() : nullptr;
			const FAPSStarSystemInfo* Info = Materializer ? Registry->Get(Materializer->GetActiveIndex()) : nullptr;
			if (Info && Info->GalaxyIndex != INDEX_NONE && !Info->Name.IsEmpty())
			{
				Active.CatalogIndex = Info->GalaxyIndex;
				Active.StarName = FName(*Info->Name);
			}
			return Active;
		}

		/**
		 * Rio 06.10 (aps.Stars.ApproachOwnByIdentity): the system a catalogue star stands up as. By place first (OwnSystemOf,
		 * 3 star radii); the star the materializer stands also by identity (its star's name; the nearest such system that is
		 * not the home), so a registry skew never makes its own sphere a foreign one. bAllowIdentity false: by place only.
		 */
		const FStandingSystem* OwnOf(const FStandingSystems& Systems, const FVector& CatalogueInSky, const int64 CatalogIndex,
			const FActiveGalaxySystem& Active, const bool bAllowIdentity, bool* bOutByIdentity = nullptr)
		{
			if (bOutByIdentity)
			{
				*bOutByIdentity = false;
			}
			const FStandingSystem* Own = OwnSystemOf(Systems, CatalogueInSky);
			if (Own || !bAllowIdentity || CatalogIndex == INDEX_NONE || CatalogIndex != Active.CatalogIndex || Active.StarName.IsNone())
			{
				return Own;
			}
			double BestSquared = TNumericLimits<double>::Max();
			for (const FStandingSystem& System : Systems)
			{
				const double DistanceSquared = FVector::DistSquared(System.StarInSky, CatalogueInSky);
				if (!System.bHome && System.StarRadiusCm > 0.0 && System.StarName == Active.StarName && DistanceSquared < BestSquared)
				{
					BestSquared = DistanceSquared;
					Own = &System;
				}
			}
			if (Own && bOutByIdentity)
			{
				*bOutByIdentity = true;
			}
			return Own;
		}

		/**
		 * The sphere an approach point crossfades into: its own star, shown and drawn where the point stands (its exact place,
		 * or its star's sky place once a re-anchor has brought it there). Owned by identity counts only from then on: while the
		 * point still slides, or while an owed travel draws the sphere away from its sky place, the point stays a point (no
		 * crossfade into a sphere drawn somewhere else).
		 */
		bool IsOwnSphere(const FStandingSystem* Own, const FVector& ExactInSky)
		{
			return Own && Own->bStarShown && FVector::DistSquared(Own->StarDrawn, ExactInSky)
				<= FMath::Square(FMath::Max(Own->StarRadiusCm, 1.0e10) * 3.0);
		}

		/**
		 * Rio 06.10 (aps.Stars.ApproachOwnByIdentity): a point owned by identity only (its standing star more than 3 radii off
		 * the catalogue place: a registry skew) slides onto that star over ReanchorSeconds (at once when taken on it) and
		 * stays on it, so point and sphere converge instead of the point vanishing and a far glyph standing in elsewhere. Once
		 * the star is gone it stays where its star stood (where the registry, the COURSE marker and the autopilot have it):
		 * no sweep across the sky; the glide (d / d0)^2 carries it onto its twin as the camera leaves, and the release past
		 * the take radius hands it back there. Owned by place again: back to the catalogue place over ReanchorSeconds. With
		 * the sky and the registry in one frame this is never reached: the star approach check expects no 're-anchored' line.
		 */
		void FollowOwnStar(FApproachPoint& Point, const FStandingSystem* Own, const bool bByIdentity, const FTransform& LocalToWorld,
			const double Scale, const double Now, const bool bAtOnce)
		{
			if (Own && bByIdentity)
			{
				const FVector OnStar = LocalToWorld.InverseTransformPosition(Own->StarInSky);
				if (Point.Anchor != EApproachAnchor::OwnStar)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.Stars] approach point %lld re-anchored to its standing star %.4g AU off (registry skew)%s"),
						Point.CatalogIndex, FVector::Dist(OnStar, Point.CatalogueLocal) * Scale / AstronomicalUnitCm,
						bAtOnce ? TEXT(", taken on it") : TEXT(", sliding onto it"));
					Point.Anchor = EApproachAnchor::OwnStar;
					Point.AnchorFromLocal = Point.ExactLocal;
					Point.AnchorStartSeconds = bAtOnce ? Now - ReanchorSeconds : Now;
				}
				const double Alpha = FMath::SmoothStep(0.0, 1.0, (Now - Point.AnchorStartSeconds) / ReanchorSeconds);
				Point.ExactLocal = FMath::Lerp(Point.AnchorFromLocal, OnStar, Alpha);
				return;
			}
			if (Point.Anchor == EApproachAnchor::OwnStar)
			{
				// Its star went: it stays where the star stood (no sweep across the sky); the glide takes it to its twin.
				if (!Own)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld stays where its standing star stood (the star went); ")
						TEXT("its glide hands it back to its twin"), Point.CatalogIndex);
				}
				Point.Anchor = EApproachAnchor::Back;
				Point.AnchorFromLocal = Point.ExactLocal;
				Point.AnchorStartSeconds = -1.0;
			}
			if (Point.Anchor == EApproachAnchor::Back && Own)
			{
				// Owned by place again (its system stands at the catalogue place): back there over ReanchorSeconds.
				if (Point.AnchorStartSeconds < 0.0)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld slides back to its catalogue place (owned by place)"),
						Point.CatalogIndex);
					Point.AnchorFromLocal = Point.ExactLocal;
					Point.AnchorStartSeconds = Now;
				}
				const double Alpha = FMath::SmoothStep(0.0, 1.0, (Now - Point.AnchorStartSeconds) / ReanchorSeconds);
				Point.ExactLocal = FMath::Lerp(Point.AnchorFromLocal, Point.CatalogueLocal, Alpha);
				if (Alpha >= 1.0)
				{
					Point.ExactLocal = Point.CatalogueLocal;
					Point.Anchor = EApproachAnchor::Catalogue;
				}
			}
		}

		/**
		 * Rio 06.10 (star approach v2): where the registry has a galaxy star now (Info.Location: its anchor, the HUD's COURSE
		 * marker, the autopilot target and the materializer's spawn) and its name; false while it is not registered.
		 */
		bool RegistryPlaceOf(const UWorld* World, const int64 CatalogIndex, FVector& OutPlace, FString* OutName = nullptr)
		{
			const FAPSStarSystems* Registry = APSStarSystemsFind(World);
			const FAPSStarSystemInfo* Info = Registry && CatalogIndex != INDEX_NONE
				? Registry->Get(Registry->IndexOfGalaxyStar(CatalogIndex)) : nullptr;
			if (!Info)
			{
				return false;
			}
			OutPlace = Info->Location;
			if (OutName)
			{
				*OutName = Info->Name;
			}
			return true;
		}

		/** Rio 06.10 (star approach v2): the 0.5 s approach line's registry field (the registry place A against E and the dot). */
		FString RegistryOffText(const UWorld* World, const FApproachPoint& Point, const FTransform& LocalToWorld,
			const FVector& Camera, const double PixelTangent)
		{
			FVector RegistryPlace;
			if (!RegistryPlaceOf(World, Point.CatalogIndex, RegistryPlace))
			{
				return TEXT(" | registry: not registered");
			}
			const FVector Exact = LocalToWorld.TransformPosition(Point.CatalogueLocal);
			const FVector Drawn = LocalToWorld.TransformPosition(Point.DrawLocal);
			return FString::Printf(TEXT(" | registry %.4g AU (%.2f px) off the exact place, %.2f px off the drawn point"),
				FVector::Dist(RegistryPlace, Exact) / AstronomicalUnitCm, PixelsApart(RegistryPlace - Camera, Exact - Camera, PixelTangent),
				PixelsApart(RegistryPlace - Camera, Drawn - Camera, PixelTangent));
		}

		/** Rio 06.10 (stage B): a galaxy star in a log line: its registry name (spaces as '_'), else its catalogue index. */
		FString StarLabelOf(const UWorld* World, const int64 CatalogIndex)
		{
			FString Label;
			FVector RegistryPlace;
			if (!RegistryPlaceOf(World, CatalogIndex, RegistryPlace, &Label) || Label.IsEmpty())
			{
				return FString::Printf(TEXT("%lld"), static_cast<long long>(CatalogIndex));
			}
			Label.ReplaceInline(TEXT(" "), TEXT("_"));
			return Label;
		}

		/**
		 * Rio 06.10 (stage B): a course actor as a galaxy catalogue index: its registry anchor, else (inside the standing system)
		 * its own star or system actor by the active star's name (CourseCatalogIndex's rule).
		 */
		int64 CourseIndexOfActor(const AActor* CourseActor, const FAPSStarSystems* Registry, const FActiveGalaxySystem& Active)
		{
			if (!CourseActor || !Registry)
			{
				return INDEX_NONE;
			}
			FGuid Id;
			if (FAPSStarSystems::AnchorSystem(CourseActor, Id))
			{
				const FAPSStarSystemInfo* Info = Registry->Find(Id);
				return Info ? Info->GalaxyIndex : INDEX_NONE;
			}
			// Inside the standing system the course may be its own star or system actor.
			const AStar* Star = Cast<AStar>(CourseActor);
			if (const AStarSystem* System = Cast<AStarSystem>(CourseActor))
			{
				Star = System->MainStar;
			}
			return IsValid(Star) && !Active.StarName.IsNone() && Star->AstroName == Active.StarName ? Active.CatalogIndex : INDEX_NONE;
		}

		/** Rio 06.10 (stage B): a navigation label of a galaxy star (no actor): "STAR_SYSTEM:<guid>" to its catalogue index. */
		int64 NavLabelIndex(const FString& StableId, const FAPSStarSystems* Registry)
		{
			static const FString LabelPrefix(TEXT("STAR_SYSTEM:"));
			FGuid Id;
			if (!Registry || !StableId.StartsWith(LabelPrefix, ESearchCase::CaseSensitive)
				|| !FGuid::Parse(StableId.RightChop(LabelPrefix.Len()), Id))
			{
				return INDEX_NONE;
			}
			const FAPSStarSystemInfo* Info = Registry->Find(Id);
			return Info ? Info->GalaxyIndex : INDEX_NONE;
		}

		/**
		 * Rio 06.10 (stage B): FindActiveGalaxySystem, recomputed only when its key changes (the registry, the materializer's
		 * active index, that entry's galaxy index and name: a renamed entry is a new key, so the result is stage A's), with its
		 * GPU twin for the own take. Kept with the layer (a rebuilt layer checks again).
		 */
		FActiveGalaxySystem CachedActive(const UWorld* World)
		{
			const FAPSStarSystems* Registry = APSStarSystemsFind(World);
			const FAPSSystemMaterializer* Materializer = Registry ? Registry->GetMaterializer() : nullptr;
			const int32 ActiveIndex = Materializer ? Materializer->GetActiveIndex() : -3;
			const FAPSStarSystemInfo* Info = Materializer ? Registry->Get(ActiveIndex) : nullptr;
			const int64 GalaxyIndex = Info ? Info->GalaxyIndex : INDEX_NONE;
			FActiveCache& Cache = GLayer.ActiveCache;
			if (Cache.ActiveIndex != -2 && Cache.Registry == Registry && Cache.ActiveIndex == ActiveIndex && Cache.GalaxyIndex == GalaxyIndex
				&& (Info ? Info->Name.Equals(Cache.Name, ESearchCase::CaseSensitive) : Cache.Name.IsEmpty()))
			{
				return Cache.Value;
			}
			Cache = FActiveCache();
			Cache.Registry = Registry;
			Cache.ActiveIndex = ActiveIndex;
			Cache.GalaxyIndex = GalaxyIndex;
			Cache.Name = Info ? Info->Name : FString();
			Cache.Value = FindActiveGalaxySystem(World);
			const AGalaxy* LayerGalaxy = GLayer.Galaxy.Get();
			FTransform LocalToWorld;
			FTwinInfo Twin;
			if (Cache.Value.CatalogIndex != INDEX_NONE && IsValid(LayerGalaxy) && MakeGameplayLocalToWorld(*LayerGalaxy, LocalToWorld)
				&& ResolveTwinOf(*LayerGalaxy, Cache.Value.CatalogIndex, Twin))
			{
				int32 Ordinal = INDEX_NONE;
				Cache.bGpu = GpuOrdinalOf(World, Cache.Value.CatalogIndex, Twin.Record.GalaxyLocalLocation, LocalToWorld, Ordinal) && Twin.bLit;
				Cache.Ordinal = Ordinal;
				Cache.ExactLocal = Twin.Record.GalaxyLocalLocation;
			}
			return Cache.Value;
		}

		/** Rio 06.10 (stage B): a course source in logs and the trace (csrc=). */
		const TCHAR* CourseSourceName(const ECourseSource Source)
		{
			switch (Source)
			{
			case ECourseSource::Autopilot: return TEXT("ap");
			case ECourseSource::Navigation: return TEXT("nav");
			case ECourseSource::Boresight: return TEXT("bore");
			default: return TEXT("none");
			}
		}

		/**
		 * Rio 06.10 (stage B, change 9): where a star's twin error reaches aps.Stars.ApproachCoursePixels for the pilot
		 * (T / (px x pixel tangent)), clamped to [aps.Stars.ApproachPointLy, CapLy]; OutRawCm the unclamped radius.
		 */
		double CourseTakeRadiusCm(const double TwinOffsetCm, const double PilotPixelTangent, const double CapLy, double* OutRawCm = nullptr)
		{
			const double Pixels = FMath::Max(static_cast<double>(CVarApproachCoursePixels.GetValueOnGameThread()), 1.0e-3);
			const double Raw = FMath::IsFinite(PilotPixelTangent) && PilotPixelTangent > 0.0 ? TwinOffsetCm / (Pixels * PilotPixelTangent) : 0.0;
			if (OutRawCm)
			{
				*OutRawCm = Raw;
			}
			const double LiveCm = FMath::Max(static_cast<double>(CVarApproachPointLy.GetValueOnGameThread()), 0.01) * LightYearCm;
			return FMath::Max(LiveCm, FMath::Min(Raw, FMath::Max(CapLy, 0.0) * LightYearCm));
		}

		/**
		 * Rio 06.10 (stage B, change 9): the piloted ship's course, polled from inside the layer (nothing is pushed): the
		 * autopilot target every frame (O(1)), the ship, the HUD's course, the flight direction and the boresight star four
		 * times a second. Local places are catalogue-local (origin shifts and the sky offset do not touch them).
		 */
		struct FCourseTracker
		{
			TWeakObjectPtr<const UWorld> World;
			TWeakObjectPtr<const APawn> Pawn;
			TWeakObjectPtr<const UAPSShipFlightModel> Flight;
			TWeakObjectPtr<const UShipNavigationComponent> Nav;
			TWeakObjectPtr<const AActor> LastApTarget;
			int64 ApIndex = INDEX_NONE;
			FString NavStableId;
			int64 NavIndex = INDEX_NONE;
			bool bNavExact = false;
			FVector NavExactLocal = FVector::ZeroVector;
			bool bInterstellar = false;
			int64 Current = INDEX_NONE;
			ECourseSource CurrentSource = ECourseSource::None;
			double NextResolveSeconds = 0.0;
			bool bDirty = true;
			/** The boresight star (latched), where it stands, whether the ISM prefix draws it, its tangent off the heading. */
			int64 Bore = INDEX_NONE;
			FVector BoreLocal = FVector::ZeroVector;
			bool bBoreIsm = false;
			double BoreTan = 0.0;
			/** The pilot's last sample (world seconds), its speed through the sky and the flight direction (zero: none yet). */
			FVector LastPilotLocal = FVector::ZeroVector;
			double LastPilotSeconds = -1.0;
			double SpeedLyPerSecond = 0.0;
			FVector HeadingLocal = FVector::ZeroVector;
			FVector QueryHeadingLocal = FVector::ZeroVector;
			double LastQuerySeconds = -1.0;
			bool bPausedLogged = false;
			/**
			 * Rio 07.10 (aps.Stars.ApproachBoresightFrameHeading): the pilot's sample of the last frame, that frame's step and
			 * speed (cm, cm/s), the direction of a forced query that has not run yet (zero once a query runs), and a query
			 * forced for the next resolve.
			 */
			FVector LastFramePilotLocal = FVector::ZeroVector;
			double LastFrameSeconds = -1.0;
			double LastFrameStepCm = 0.0;
			double LastFrameSpeedCm = 0.0;
			FVector ForceHeadingLocal = FVector::ZeroVector;
			bool bForceQuery = false;
			/** Rio 07.10 (aps.Stars.ApproachBoresightKeepNeeded): the latched star's course take radius at its pick (local; 0: none). */
			double BoreRadiusLocal = 0.0;
			/** Rio 07.10 (aps.Stars.ApproachBoresightNavHold): the HUD's course was the course by the cone at the last resolve. */
			bool bNavHeld = false;
		};
		FCourseTracker GCourse;

		/** Rio 06.10 (aps.Stars.ApproachTrace): the traced star, refreshed four times a second, and what it showed last frame. */
		struct FTraceTarget
		{
			int64 CatalogIndex = INDEX_NONE;
			const TCHAR* Source = TEXT("none");
			/** The registry name (spaces as '_'), or the catalogue index while it is not registered. */
			FString Name;
			bool bRecord = false;
			FGalaxyCatalogStarRecord Record;
			/** Drawn by the GPU layer (its twin: level set and catalogue-local place), by the galaxy HISM (ISM prefix), or not. */
			bool bGpu = false;
			bool bIsm = false;
			int32 TwinSet = INDEX_NONE;
			FVector3f TwinLocal = FVector3f::ZeroVector;
			float Intensity = 0.0f;
			/** Rio 06.10 (stage B): the twin's distance from the exact place, local units (the course take radius). */
			double TwinOffsetLocal = 0.0;
			FString LastRep;
			bool bLastPoint = false;
			bool bLastStanding = false;
			EApproachAnchor LastAnchor = EApproachAnchor::Catalogue;
			double LastDistanceCm = -1.0;
		};
		FTraceTarget GTraceTarget;
		TWeakObjectPtr<const UWorld> GTraceWorld;
		double GTraceNextResolveSeconds = 0.0;
		int32 GTraceLastMap = -1;
		/** The sky listener: the frame of the last sky move (an owed step or a pay) and of the last pay. */
		uint64 GTraceSkyFrame = MAX_uint64;
		uint64 GTracePayFrame = MAX_uint64;
		bool GTraceSkyBound = false;
		/** Events so far (k in the shot names), bursts waiting, the burst being shot, its next frame and the last shot frame. */
		int32 GTraceEvents = 0;
		TArray<FString> GTraceBursts;
		FString GTraceBurst;
		int32 GTraceBurstShot = 0;
		uint64 GTraceShotFrame = MAX_uint64;

		/** A file-name-safe event: "point->crossfade" -> "point-to-crossfade", "cross:1au-in" -> "cross-1au-in". */
		FString TraceFileName(const FString& Event)
		{
			FString Safe = Event.Replace(TEXT("->"), TEXT("-to-")).Replace(TEXT(":"), TEXT("-"));
			for (TCHAR& Character : Safe.GetCharArray())
			{
				if (Character != 0 && !FChar::IsAlnum(Character) && Character != TEXT('-') && Character != TEXT('_') && Character != TEXT('.'))
				{
					Character = TEXT('_');
				}
			}
			return Safe;
		}

		/** With aps.Stars.ApproachTraceShots, queues the burst of screenshots of event number GTraceEvents. */
		void QueueTraceBurst(const FString& Event)
		{
			if (CVarApproachTraceShots.GetValueOnGameThread() > 0 && GTraceBursts.Num() < 16)
			{
				GTraceBursts.Add(FString::Printf(TEXT("tE%d-%s"), GTraceEvents, *TraceFileName(Event)));
			}
		}

		/** Logs a trace event for the traced star and, with aps.Stars.ApproachTraceShots, queues its burst of screenshots. */
		void TraceEvent(const FString& Event, const FString& Detail = FString(), const bool bShots = true)
		{
			++GTraceEvents;
			UE_LOG(LogTemp, Log, TEXT("[APS.StarTrace] f=%llu event=%s tgt=%s gi=%lld k=%d%s%s"),
				static_cast<unsigned long long>(GFrameCounter), *Event, GTraceTarget.Name.IsEmpty() ? TEXT("none") : *GTraceTarget.Name,
				GTraceTarget.CatalogIndex, GTraceEvents, Detail.IsEmpty() ? TEXT("") : TEXT(" "), *Detail);
			if (bShots)
			{
				QueueTraceBurst(Event);
			}
		}

		/** One screenshot a frame from the queued bursts: even shots with the UI (the COURSE marker), odd ones the scene only. */
		void TraceShoot()
		{
			const int32 Shots = CVarApproachTraceShots.GetValueOnGameThread();
			if (Shots <= 0)
			{
				GTraceBursts.Reset();
				GTraceBurst.Reset();
				return;
			}
			if (GTraceShotFrame == GFrameCounter)
			{
				return;
			}
			if (GTraceBurst.IsEmpty() && GTraceBursts.Num() > 0)
			{
				GTraceBurst = GTraceBursts[0];
				GTraceBursts.RemoveAt(0);
				GTraceBurstShot = 0;
			}
			if (GTraceBurst.IsEmpty())
			{
				return;
			}
			const bool bShowUI = GTraceBurstShot % 2 == 0;
			const FString File = FPaths::ScreenShotDir() / TEXT("ShipDrive") / FString::Printf(TEXT("%s_%s_f%d.png"),
				*CVarApproachTraceLabel.GetValueOnGameThread(), *GTraceBurst, GTraceBurstShot);
			FScreenshotRequest::RequestScreenshot(File, bShowUI, false);
			UE_LOG(LogTemp, Log, TEXT("[APS.StarTrace] f=%llu shot=%s ui=%d"), static_cast<unsigned long long>(GFrameCounter), *File,
				bShowUI ? 1 : 0);
			GTraceShotFrame = GFrameCounter;
			if (++GTraceBurstShot >= Shots)
			{
				GTraceBurst.Reset();
			}
		}

		/** Tags owe and pay frames (order-free: it only notes the frame; a world-less settle logs its own pay line). */
		void BindTraceSky()
		{
			if (GTraceSkyBound)
			{
				return;
			}
			GTraceSkyBound = true;
			UAPSWorldOriginSubsystem::OnSkyOffsetChanged().AddLambda([](UWorld* World, const FVector& Change)
			{
				if (CVarApproachTrace.GetValueOnGameThread() == 0)
				{
					return;
				}
				GTraceSkyFrame = GFrameCounter;
				// A pay clears the debt before it tells (UAPSWorldOriginSubsystem::PayDebt): no sky offset is left.
				if (UAPSWorldOriginSubsystem::SkyOffsetOf(World).IsNearlyZero())
				{
					GTracePayFrame = GFrameCounter;
					TraceEvent(TEXT("pay"), FString::Printf(TEXT("sky=%.6g ly"), Change.Size() / LightYearCm));
				}
			});
		}

		/** The course: the piloted ship's autopilot target, else its HUD's selected contact, as a galaxy catalogue index. */
		int64 CourseCatalogIndex(const UWorld* World, const FAPSStarSystems* Registry, const FActiveGalaxySystem& Active)
		{
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			if (!Pawn || !Registry)
			{
				return INDEX_NONE;
			}
			const AActor* Course = nullptr;
			if (const UAPSShipFlightModel* Flight = Pawn->FindComponentByClass<UAPSShipFlightModel>(); Flight && Flight->IsAutopilotEngaged())
			{
				Course = Flight->GetAutopilotTarget();
			}
			if (!Course)
			{
				const UShipNavigationComponent* Navigation = Pawn->FindComponentByClass<UShipNavigationComponent>();
				const FShipNavigationContact* Contact = Navigation ? Navigation->GetSelectedContact() : nullptr;
				Course = Contact ? Contact->Actor.Get() : nullptr;
			}
			// Rio 06.10 (stage B): the actor rule is shared with the course tracker (CourseIndexOfActor).
			return CourseIndexOfActor(Course, Registry, Active);
		}

		/** aps.Stars.ApproachTraceTarget, else the course, else the nearest approach point (the traced one first), else the standing one. */
		int64 ResolveTraceTarget(const UWorld* World, const FAPSStarSystems* Registry, const FActiveGalaxySystem& Active,
			const FVector& CameraLocal, const TCHAR*& OutSource)
		{
			const FString Chosen = CVarApproachTraceTarget.GetValueOnGameThread();
			const int64 ChosenIndex = Chosen.IsNumeric() ? FCString::Atoi64(*Chosen) : -1;
			if (ChosenIndex >= 0)
			{
				OutSource = TEXT("cvar");
				return ChosenIndex;
			}
			// Rio 06.10 (stage B): the course tracker's course while it runs (aps.Stars.ApproachCourseMaxLy), else stage A's poll.
			const int64 Course = CVarApproachCourseMaxLy.GetValueOnGameThread() > 0.0f ? GetCourseStar(World)
				: CourseCatalogIndex(World, Registry, Active);
			if (Course != INDEX_NONE)
			{
				OutSource = TEXT("course");
				return Course;
			}
			const FApproachPoint* Nearest = nullptr;
			double NearestSquared = TNumericLimits<double>::Max();
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				const double DistanceSquared = Point.CatalogIndex == GTraceTarget.CatalogIndex ? -1.0
					: FVector::DistSquared(CameraLocal, Point.ExactLocal);
				if (Point.Handle != 0 && DistanceSquared < NearestSquared)
				{
					NearestSquared = DistanceSquared;
					Nearest = &Point;
				}
			}
			if (Nearest)
			{
				OutSource = TEXT("point");
				return Nearest->CatalogIndex;
			}
			if (Active.CatalogIndex != INDEX_NONE)
			{
				OutSource = TEXT("active");
				return Active.CatalogIndex;
			}
			OutSource = TEXT("none");
			return INDEX_NONE;
		}

		/** The level sets' exclusion spheres (the home, the standing systems) cover this catalogue-local place. */
		bool InLayerExclusions(const FVector3f& Local)
		{
			for (const FVector4f& Sphere : GLayer.Exclusions)
			{
				if (FVector3f::DistSquared(FVector3f(Sphere.X, Sphere.Y, Sphere.Z), Local) <= FMath::Square(Sphere.W))
				{
					return true;
				}
			}
			return false;
		}

		/** The traced star's record, name and how the layer draws it (four times a second: a rebuilt layer is picked up). */
		void RefreshTraceTarget(const UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld)
		{
			FTraceTarget& Target = GTraceTarget;
			Target.bRecord = Target.CatalogIndex != INDEX_NONE && Galaxy.StarCatalog.ResolveStar(Target.CatalogIndex, Target.Record)
				&& Target.Record.CatalogIndex != INDEX_NONE;
			Target.bGpu = false;
			Target.bIsm = false;
			Target.TwinSet = INDEX_NONE;
			FVector RegistryPlace;
			if (!RegistryPlaceOf(World, Target.CatalogIndex, RegistryPlace, &Target.Name))
			{
				Target.Name = Target.CatalogIndex != INDEX_NONE ? FString::Printf(TEXT("%lld"), Target.CatalogIndex) : FString();
			}
			Target.Name.ReplaceInline(TEXT(" "), TEXT("_"));
			if (!Target.bRecord)
			{
				return;
			}
			// Its ordinal (the index keeps float places, ~10 AU off in REAL SCALE: a wide search, matched by catalogue index).
			int32 Ordinal = INDEX_NONE;
			const bool bDrawnByGpu = GpuOrdinalOf(World, Target.CatalogIndex, Target.Record.GalaxyLocalLocation, LocalToWorld, Ordinal);
			if (Ordinal == INDEX_NONE)
			{
				return;
			}
			Target.bIsm = static_cast<int64>(Ordinal) - GLayer.FirstOrdinal < 0;
			Target.bGpu = bDrawnByGpu;
			if (!Target.bGpu)
			{
				return;
			}
			// Its twin as its level set packs it (Rio 06.10, stage B: ResolveTwinOf, the take's level choice).
			FTwinInfo Twin;
			if (!ResolveTwinOf(Galaxy, Target.CatalogIndex, Twin))
			{
				return;
			}
			Target.Intensity = Twin.Intensity;
			Target.TwinSet = Twin.TwinSet;
			Target.TwinLocal = Twin.TwinLocal;
			Target.TwinOffsetLocal = Twin.OffsetLocal;
		}

		FString TraceLookName(const FString& Rep)
		{
			return Rep == TEXT("cross") ? FString(TEXT("crossfade")) : Rep;
		}

		/**
		 * Rio 06.10 (aps.Stars.ApproachTrace): one line a frame for the traced star, from UpdateApproachPoints after its points
		 * moved (what this frame draws). The star approach check joins lines by f. Fields:
		 *  f       GFrameCounter (the far glyphs' trace lines of the frame carry the same f); t seconds since start; dt the
		 *          frame's delta in ms, unclamped (a shot's stall shows here)
		 *  tgt gi  the registry name (or the catalogue index while unregistered) and the catalogue index
		 *  d_ly    camera to E, the exact catalogue place where the sky draws it (ProjectCatalogueLocation)
		 *  rep     twin (its catalogue point: the GPU twin, or the HISM point of an ISM-prefix star), point / cross / sphere (an
		 *          approach point's look), glyph / sphere without a point (predicted from the standing star's disc against
		 *          aps.Stars.FarGlyphPixels: the far glyphs' own line of the frame confirms), none
		 *  AE_km   the registry place A (Info.Location: anchor, COURSE marker, autopilot, spawn) to E, km; -1 unregistered
		 *  pt_px   the drawn dot (point, twin, or the standing star) to E on screen; -1 when nothing is drawn
		 *  hud_px  A to E on screen (where the COURSE marker stands against the star); -1 unregistered
		 *  lum     the dot's modelled pixel value: the 0.5 s line's pre-clamp, clamped at aps.Stars.GpuPointMaxPixel (times the
		 *          view's pre-exposure, not logged); 0 without a point or twin
		 *  fade    the point's crossfade (1 a point alone; 1 for a twin, 0 without either)
		 *  glide   the point's way from its twin (1) to E (0); 1 for a twin; -1 without either
		 *  owed    the sky moved (an owed step or a pay) earlier in this frame; pay: that move was a pay (a world-less settle
		 *          after the world tick logs its own 'event=pay' line with the same f)
		 *  after | src gpu/ism/none, own pos/id/none (its standing system by place or by identity), disc (that star's disc
		 *          px), dot_hud_px (the drawn dot to A), pre (unclamped), map (the F10 map is open)
		 * Rio 06.10 (stage B), after xy: course (the course star, -1 none), csrc (ap/nav/bore/none), ctake_ly (its course take
		 *          radius: the point's, else computed with the last pilot pixel tangent; 0 for a non-GPU star or with
		 *          aps.Stars.ApproachCourseMaxLy 0), ctake_raw_ly (unclamped; 0 for a non-GPU star), cls (none/near/course/
		 *          retired), eye (view: the view decides; pilot: the pilot's eyes do, the map or a far view), pd_ly (the pilot to
		 *          E), gain (the level sets' map hold), pscale (the point's map hold, 1 without a point), hb (a hand-back runs),
		 *          keep (kept past its release radius by its standing system), gl (aps.Stars.SystemGlare: the point's base value,
		 *          a twin's level value, else 1). The px fields stay the view's.
		 */
		void TraceApproach(UWorld* World, const FApproachEyes& Eyes, const FTransform& LocalToWorld, const FStandingSystems* Gathered)
		{
			const AGalaxy* Galaxy = GLayer.Galaxy.Get();
			if (!IsValid(Galaxy) || World == nullptr)
			{
				return;
			}
			BindTraceSky();
			const FVector& Camera = Eyes.ViewCamera;
			const double PixelTangent = Eyes.ViewPixelTangent;
			const double Now = FPlatformTime::Seconds();
			const double WorldNow = World->GetTimeSeconds();
			const FVector CameraLocal = LocalToWorld.InverseTransformPosition(Camera);
			const FVector PilotLocal = Eyes.bDetached ? LocalToWorld.InverseTransformPosition(Eyes.PilotCamera) : CameraLocal;
			const FActiveGalaxySystem Active = CachedActive(World);
			const bool bMapOpen = IsMapOpen(World);
			if (GTraceLastMap >= 0 && GTraceLastMap != (bMapOpen ? 1 : 0))
			{
				TraceEvent(bMapOpen ? TEXT("map-open") : TEXT("map-close"));
			}
			GTraceLastMap = bMapOpen ? 1 : 0;
			// The traced star, four times a second.
			if (GTraceWorld.Get() != World || Now >= GTraceNextResolveSeconds)
			{
				GTraceNextResolveSeconds = Now + ApproachScanSeconds;
				const TCHAR* Source = TEXT("none");
				const int64 Chosen = ResolveTraceTarget(World, APSStarSystemsFind(World), Active, CameraLocal, Source);
				if (Chosen != GTraceTarget.CatalogIndex || GTraceWorld.Get() != World)
				{
					GTraceTarget = FTraceTarget();
					GTraceTarget.CatalogIndex = Chosen;
					GTraceWorld = World;
					RefreshTraceTarget(World, *Galaxy, LocalToWorld);
					TraceEvent(TEXT("target"), FString::Printf(TEXT("source=%s src=%s"), Source,
						GTraceTarget.bGpu ? TEXT("gpu") : GTraceTarget.bIsm ? TEXT("ism") : TEXT("none")), false);
				}
				else
				{
					RefreshTraceTarget(World, *Galaxy, LocalToWorld);
				}
				GTraceTarget.Source = Source;
			}
			FTraceTarget& Target = GTraceTarget;
			if (Target.CatalogIndex == INDEX_NONE || !Target.bRecord)
			{
				TraceShoot();
				return;
			}
			const FVector ExactWorld = LocalToWorld.TransformPosition(Target.Record.GalaxyLocalLocation);
			const double DistanceCm = FVector::Dist(Camera, ExactWorld);
			const FApproachPoint* Point = GLayer.Approach.FindByPredicate([&Target](const FApproachPoint& Candidate)
			{
				return Candidate.CatalogIndex == Target.CatalogIndex && Candidate.Handle != 0;
			});
			FStandingSystems GatheredHere;
			if (!Gathered)
			{
				GatherStandingSystems(World, GatheredHere);
				Gathered = &GatheredHere;
			}
			bool bOwnByIdentity = false;
			const FStandingSystem* Own = OwnOf(*Gathered, ExactWorld, Target.CatalogIndex, Active, true, &bOwnByIdentity);
			const bool bStanding = Active.CatalogIndex == Target.CatalogIndex;
			const double OwnDiscPixels = Own
				? Own->StarRadiusCm / FMath::Max(FVector::Dist(Camera, Own->StarDrawn) * PixelTangent, 1.0e-30) : 0.0;
			static IConsoleVariable* GlyphPixelsVariable = nullptr;
			static IConsoleVariable* PointIntensityVariable = nullptr;
			static IConsoleVariable* MaxPixelVariable = nullptr;
			const float GlyphPixels = ConsoleFloat(GlyphPixelsVariable, TEXT("aps.Stars.FarGlyphPixels"), 2.2f);
			const float MaxPixel = ConsoleFloat(MaxPixelVariable, TEXT("aps.Stars.GpuPointMaxPixel"), 16.0f);
			const float PointIntensity = ConsoleFloat(PointIntensityVariable, TEXT("aps.Stars.GpuPointIntensity"), 1.0f);
			const float WorldVisibility = GVisibility >= 0.0f ? GVisibility : 1.0f;
			// What draws it now.
			const TCHAR* Rep = TEXT("none");
			bool bDrawn = false;
			FVector DrawnWorld = ExactWorld;
			double Fade = 0.0;
			double Glide = -1.0;
			int32 Set = INDEX_NONE;
			if (Point)
			{
				Rep = Point->Look == EApproachLook::Point ? TEXT("point") : Point->Look == EApproachLook::Crossfade ? TEXT("cross") : TEXT("sphere");
				bDrawn = true;
				DrawnWorld = LocalToWorld.TransformPosition(Point->DrawLocal);
				Fade = Point->Fade;
				// Rio 06.10 (stage B): its glide as drawn (the pilot's distance, a hand-back).
				Glide = GlideOf(*Point, PilotLocal, WorldNow);
				Set = Point->TwinSet;
			}
			else if (Target.bGpu && GLayer.PointDescs.IsValidIndex(Target.TwinSet))
			{
				if (!InLayerExclusions(Target.TwinLocal) && GLayer.PointDescs[Target.TwinSet].Visibility > 0.0f && WorldVisibility > 0.0f)
				{
					const FVector3f CameraLocalF(CameraLocal);
					Rep = TEXT("twin");
					bDrawn = true;
					DrawnWorld = LocalToWorld.TransformPosition(CameraLocal + FVector(Target.TwinLocal - CameraLocalF));
					Fade = 1.0;
					Glide = 1.0;
					Set = Target.TwinSet;
				}
			}
			else if (Target.bIsm && !bStanding)
			{
				// Its HISM point (taken at E here).
				Rep = TEXT("twin");
				bDrawn = true;
				Fade = 1.0;
				Glide = 1.0;
			}
			if (!bDrawn && bStanding && Own)
			{
				Rep = Own->bStarShown && OwnDiscPixels >= GlyphPixels ? TEXT("sphere") : TEXT("glyph");
				bDrawn = true;
				DrawnWorld = Own->StarDrawn;
			}
			double PreClamp = 0.0;
			if (GLayer.PointDescs.IsValidIndex(Set))
			{
				const APSStarRenderer::FPointSetDesc& LevelDesc = GLayer.PointDescs[Set];
				const double DistanceLocal = FMath::Max(FVector::Dist(CameraLocal, Point ? Point->ExactLocal : Target.Record.GalaxyLocalLocation),
					static_cast<double>(LevelDesc.BrightnessFloorDistanceLocal));
				// An approach point's own scale (never the level's strategic-map hold by itself; Rio 06.10, stage B: times the
				// hold it carries, aps.Stars.ApproachMapGain); a twin's is its level's.
				const float SetScale = (Point && GLayer.PointIntensityScale > 0.0f ? GLayer.PointIntensityScale : LevelDesc.IntensityScale)
					* (Point ? Point->AppliedGain : 1.0f);
				PreClamp = APSStarRenderer::DecodeIntensity(APSStarRenderer::EncodeIntensity(Point ? Point->Intensity : Target.Intensity))
					* SetScale * (Point ? Point->AppliedVisibility : LevelDesc.Visibility) * WorldVisibility * PointIntensity
					/ FMath::Max(FMath::Square(PixelTangent) * FMath::Square(DistanceLocal), 1.0e-300);
			}
			FVector RegistryPlace = FVector::ZeroVector;
			const bool bRegistered = RegistryPlaceOf(World, Target.CatalogIndex, RegistryPlace);
			const double AEKm = bRegistered ? FVector::Dist(RegistryPlace, ExactWorld) / 1.0e5 : -1.0;
			const double HudPixels = bRegistered ? PixelsApart(RegistryPlace - Camera, ExactWorld - Camera, PixelTangent) : -1.0;
			const double DotPixels = bDrawn ? PixelsApart(DrawnWorld - Camera, ExactWorld - Camera, PixelTangent) : -1.0;
			const double DotHudPixels = bDrawn && bRegistered ? PixelsApart(DrawnWorld - Camera, RegistryPlace - Camera, PixelTangent) : -1.0;
			const uint64 Frame = GFrameCounter;
			// The drawn dot on screen (viewport pixels, as the screenshots; -1,-1 behind the view or without a dot): the star
			// approach check crops the scene-only shots there.
			FVector2D Screen(-1.0, -1.0);
			const APlayerController* ViewController = World->GetFirstPlayerController();
			if (!bDrawn || !ViewController || !ViewController->ProjectWorldLocationToScreen(DrawnWorld, Screen, false))
			{
				Screen = FVector2D(-1.0, -1.0);
			}
			// Rio 06.10 (stage B): the course, the point's class and eyes, the map holds, the glare.
			ECourseSource CourseSource = ECourseSource::None;
			const int64 CourseIndex = GetCourseStar(World, &CourseSource);
			const double CourseMaxLy = FMath::Max(static_cast<double>(CVarApproachCourseMaxLy.GetValueOnGameThread()), 0.0);
			const double TraceScale = LocalToWorld.GetMaximumAxisScale();
			double CourseRawCm = 0.0;
			double CourseCm = 0.0;
			if (Target.bGpu)
			{
				const double ComputedCm = CourseTakeRadiusCm(Target.TwinOffsetLocal * TraceScale, GLayer.LastPilotPixelTangent,
					CourseMaxLy, &CourseRawCm);
				CourseCm = CourseMaxLy > 0.0 ? (Point && Point->CourseTakeCm > 0.0 ? Point->CourseTakeCm : ComputedCm) : 0.0;
			}
			const TCHAR* PointClass = !Point ? TEXT("none") : Point->bCourse ? TEXT("course")
				: Point->CourseTakeCm > 0.0 ? TEXT("retired") : TEXT("near");
			const double PilotDistanceCm = FVector::Dist(Eyes.PilotCamera, ExactWorld);
			const float GlareValue = Point ? ApproachBaseVisibility(*Point)
				: (FCString::Strcmp(Rep, TEXT("twin")) == 0 && GLayer.PointDescs.IsValidIndex(Set) ? GLayer.PointDescs[Set].Visibility : 1.0f);
			UE_LOG(LogTemp, Log,
				TEXT("[APS.StarTrace] f=%llu t=%.4f dt=%.2f tgt=%s gi=%lld d_ly=%.6g rep=%s AE_km=%.6g pt_px=%.3f hud_px=%.3f ")
				TEXT("lum=%.4g fade=%.3f glide=%.4f owed=%d pay=%d | src=%s own=%s disc=%.4g dot_hud_px=%.3f pre=%.4g map=%d xy=%.1f,%.1f")
				TEXT(" course=%lld csrc=%s ctake_ly=%.6g ctake_raw_ly=%.6g cls=%s eye=%s pd_ly=%.6g gain=%.6g pscale=%.6g hb=%d keep=%d gl=%.4f"),
				static_cast<unsigned long long>(Frame), Now - GStartTime, World->GetDeltaSeconds() * 1000.0f, *Target.Name,
				Target.CatalogIndex, DistanceCm / LightYearCm, Rep, AEKm, DotPixels, HudPixels,
				FMath::Min(PreClamp, static_cast<double>(MaxPixel)), Fade, Glide, GTraceSkyFrame == Frame ? 1 : 0,
				GTracePayFrame == Frame ? 1 : 0, Target.bGpu ? TEXT("gpu") : Target.bIsm ? TEXT("ism") : TEXT("none"),
				Own ? (bOwnByIdentity ? TEXT("id") : TEXT("pos")) : TEXT("none"), OwnDiscPixels, DotHudPixels, PreClamp,
				bMapOpen ? 1 : 0, Screen.X, Screen.Y,
				static_cast<long long>(CourseIndex), CourseSourceName(CourseSource), CourseCm / LightYearCm, CourseRawCm / LightYearCm,
				PointClass, Eyes.bDetached ? TEXT("pilot") : TEXT("view"), PilotDistanceCm / LightYearCm, GLayer.AppliedMapGain,
				Point ? Point->AppliedGain : 1.0f, Point && Point->HandBackStart >= 0.0 ? 1 : 0, Point && Point->bKeptNow ? 1 : 0,
				GlareValue);
			// The star approach harness's pilot without navigation (APSShipFlightBenchmark, aps.Test.Steer) aims at the drawn
			// dot: '<frame> <x> <y> <z>', the unit direction from the view. Test builds only register that variable.
			static IConsoleVariable* const SteerDrawnDir = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Test.SteerDrawnDir"), false);
			if (SteerDrawnDir && bDrawn)
			{
				const FVector Direction = (DrawnWorld - Camera).GetSafeNormal();
				SteerDrawnDir->Set(*FString::Printf(TEXT("%llu %.9f %.9f %.9f"), static_cast<unsigned long long>(Frame), Direction.X,
					Direction.Y, Direction.Z), ECVF_SetByConsole);
			}
			// Events (each with its burst of screenshots under aps.Stars.ApproachTraceShots).
			const FString RepName = Rep;
			const bool bPoint = Point != nullptr;
			const EApproachAnchor Anchor = Point ? Point->Anchor : EApproachAnchor::Catalogue;
			if (!Target.LastRep.IsEmpty())
			{
				if (bPoint && !Target.bLastPoint)
				{
					TraceEvent(TEXT("take"), FString::Printf(TEXT("d_ly=%.6g from=%s"), DistanceCm / LightYearCm, *Target.LastRep));
				}
				else if (!bPoint && Target.bLastPoint)
				{
					TraceEvent(TEXT("release"), FString::Printf(TEXT("d_ly=%.6g to=%s"), DistanceCm / LightYearCm, Rep));
				}
				else if (RepName != Target.LastRep)
				{
					TraceEvent(FString::Printf(TEXT("%s->%s"), *TraceLookName(Target.LastRep), *TraceLookName(RepName)),
						FString::Printf(TEXT("d_au=%.6g disc=%.4g"), DistanceCm / AstronomicalUnitCm, OwnDiscPixels));
				}
				if (bStanding != Target.bLastStanding)
				{
					TraceEvent(bStanding ? TEXT("standup") : TEXT("standdown"),
						FString::Printf(TEXT("d_ly=%.6g AE_km=%.6g own=%s"), DistanceCm / LightYearCm, AEKm,
							Own ? (bOwnByIdentity ? TEXT("id") : TEXT("pos")) : TEXT("none")));
				}
				if (Anchor != Target.LastAnchor)
				{
					TraceEvent(Anchor == EApproachAnchor::OwnStar ? TEXT("reanchor") : Anchor == EApproachAnchor::Back ? TEXT("unanchor")
						: TEXT("anchor-catalogue"));
				}
			}
			struct FMark
			{
				double Cm;
				const TCHAR* Name;
			};
			static const FMark Marks[] = {
				{3.0 * LightYearCm, TEXT("3ly")}, {1.0 * LightYearCm, TEXT("1ly")}, {0.5 * LightYearCm, TEXT("0.5ly")},
				{0.25 * LightYearCm, TEXT("0.25ly")}, {8000.0 * AstronomicalUnitCm, TEXT("8000au")},
				{1000.0 * AstronomicalUnitCm, TEXT("1000au")}, {100.0 * AstronomicalUnitCm, TEXT("100au")},
				{10.0 * AstronomicalUnitCm, TEXT("10au")}, {3.0 * AstronomicalUnitCm, TEXT("3au")},
				{2.0 * AstronomicalUnitCm, TEXT("2au")}, {1.0 * AstronomicalUnitCm, TEXT("1au")}, {0.5 * AstronomicalUnitCm, TEXT("0.5au")}};
			if (Target.LastDistanceCm > 0.0)
			{
				for (const FMark& Mark : Marks)
				{
					if ((Target.LastDistanceCm > Mark.Cm) != (DistanceCm > Mark.Cm))
					{
						TraceEvent(FString::Printf(TEXT("cross:%s-%s"), Mark.Name, DistanceCm < Mark.Cm ? TEXT("in") : TEXT("out")),
							FString::Printf(TEXT("rep=%s"), Rep));
					}
				}
			}
			Target.LastRep = RepName;
			Target.bLastPoint = bPoint;
			Target.bLastStanding = bStanding;
			Target.LastAnchor = Anchor;
			Target.LastDistanceCm = DistanceCm;
			TraceShoot();
		}

		/** Rio 06.10: other trace sources log an event for the traced star and its burst through this command (the far glyphs,
		 * which log their own glyph <-> sphere event line, ask for the burst alone: aps.Stars.ApproachTraceBurst below). */
		FAutoConsoleCommand GApproachTraceEventCommand(
			TEXT("aps.Stars.ApproachTraceEvent"),
			TEXT("Rio 06.10 (star approach trace): with aps.Stars.ApproachTrace logs '[APS.StarTrace] f=<n> event=<name>' for the ")
			TEXT("traced star and queues its burst of aps.Stars.ApproachTraceShots screenshots. Arguments: the event name."),
			FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				if (CVarApproachTrace.GetValueOnGameThread() != 0)
				{
					TraceEvent(Args.Num() > 0 ? FString::Join(Args, TEXT("_")) : FString(TEXT("event")));
				}
			}));

		/**
		 * Rio 06.10: the far glyphs (APSFarStarGlyphs.cpp, TraceGlyphFrame) log their own glyph <-> sphere event line and ask
		 * for its burst only, by this name; the burst's k (tE<k> in the shot names) is logged here.
		 */
		FAutoConsoleCommand GApproachTraceBurstCommand(
			TEXT("aps.Stars.ApproachTraceBurst"),
			TEXT("Rio 06.10 (star approach trace): with aps.Stars.ApproachTrace queues a burst of aps.Stars.ApproachTraceShots ")
			TEXT("screenshots for an event another trace source logged itself. Arguments: the event name."),
			FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				if (CVarApproachTrace.GetValueOnGameThread() != 0)
				{
					const FString Event = Args.Num() > 0 ? FString::Join(Args, TEXT("_")) : FString(TEXT("event"));
					++GTraceEvents;
					UE_LOG(LogTemp, Log, TEXT("[APS.StarTrace] f=%llu burst=%s k=%d"), static_cast<unsigned long long>(GFrameCounter),
						*Event, GTraceEvents);
					QueueTraceBurst(Event);
				}
			}));

		/** Rio 06.10 (stage B): the standing systems, gathered once a frame on the first need (never on the light path). */
		struct FLazyStanding
		{
			UWorld* World = nullptr;
			FStandingSystems Systems;
			bool bGathered = false;

			const FStandingSystems& Get()
			{
				if (!bGathered)
				{
					bGathered = true;
					GatherStandingSystems(World, Systems);
				}
				return Systems;
			}
		};

		/** Rio 06.10 (stage B): the approach point of a catalogue star, or null. */
		FApproachPoint* FindApproachPointOf(const int64 CatalogIndex)
		{
			return CatalogIndex == INDEX_NONE ? nullptr : GLayer.Approach.FindByPredicate([CatalogIndex](const FApproachPoint& Point)
			{
				return Point.CatalogIndex == CatalogIndex && Point.Handle != 0;
			});
		}

		/** aps.Stars.ApproachPointLy in cm (the near take radius). */
		double ApproachTakeCm()
		{
			return FMath::Max(static_cast<double>(CVarApproachPointLy.GetValueOnGameThread()), 0.01) * LightYearCm;
		}

		/**
		 * Rio 06.10 (stage B): the points that count against MaxApproachPoints: near points, and retired ones while they are
		 * near (the course point and retired points beyond the take radius do not).
		 */
		int32 NumNearSlots(const double TakeCm)
		{
			const double NearCm = TakeCm * ApproachReleaseFactor;
			int32 Count = 0;
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				if (!Point.bCourse && (Point.CourseTakeCm <= 0.0 || Point.LastPilotCm <= NearCm))
				{
					++Count;
				}
			}
			return Count;
		}

		/** Rio 06.10 (stage B): a retired point beyond the take radius the pool may hand back (not its standing system's). */
		bool IsRetiredOut(const FApproachPoint& Point, const double TakeCm)
		{
			return !Point.bCourse && Point.CourseTakeCm > 0.0 && Point.LastPilotCm > TakeCm * ApproachReleaseFactor
				&& !Point.bOwnStanding && Point.HandBackStart < 0.0;
		}

		int32 RetiredOut(const double TakeCm)
		{
			int32 Count = 0;
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				Count += IsRetiredOut(Point, TakeCm) ? 1 : 0;
			}
			return Count;
		}

		int32 NumRetiredPoints()
		{
			int32 Count = 0;
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				Count += !Point.bCourse && Point.CourseTakeCm > 0.0 ? 1 : 0;
			}
			return Count;
		}

		/** Rio 06.10 (aps.Stars.ApproachHandBackSeconds): the point slides from its glide now onto its twin, then is released. */
		void StartHandBack(FApproachPoint& Point, const TCHAR* Reason, const double WorldNow, const FVector& PilotLocal)
		{
			Point.HandBackFromGlide = GlideOf(Point, PilotLocal, WorldNow);
			Point.HandBackStart = WorldNow;
			Point.HandBackReason = Reason;
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld hands back to its twin (%s, glide %.4f -> 1 over %.2f s)"),
				Point.CatalogIndex, Reason, Point.HandBackFromGlide,
				FMath::Max(static_cast<double>(CVarApproachHandBackSeconds.GetValueOnGameThread()), 0.0));
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("handback"), FString::Printf(TEXT("star=%lld glide=%.4f"), Point.CatalogIndex, Point.HandBackFromGlide));
			}
		}

		/**
		 * Rio 06.10 (stage B, change 9): the course star's existing point becomes the course point. Its glide law is kept, so
		 * nothing moves; it is released past max(its radius, the course take radius) x ApproachReleaseFactor.
		 */
		void PromoteCoursePoint(FApproachPoint& Point, const FVector& PilotLocal, const double Scale)
		{
			const double TakeCm = ApproachTakeCm();
			const FCourseTwin& Twin = GLayer.CourseTwin;
			const double CapLy = FMath::Max(static_cast<double>(CVarApproachCourseMaxLy.GetValueOnGameThread()), 0.0);
			const double RadiusCm = Twin.bValid && Twin.bGpu && Twin.CatalogIndex == Point.CatalogIndex
				? CourseTakeRadiusCm(Twin.TwinOffsetLocal * Scale, GLayer.LastPilotPixelTangent, CapLy) : TakeCm;
			Point.bCourse = true;
			Point.CourseTakeCm = FMath::Max3(Point.CourseTakeCm, RadiusCm, TakeCm);
			const double DistanceCm = FVector::Dist(PilotLocal, Point.ExactLocal) * Scale;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Stars] approach point %lld is the course star now (promoted at %.6g ly, its glide law kept, release radius %.4g ly)"),
				Point.CatalogIndex, DistanceCm / LightYearCm, Point.CourseTakeCm * ApproachReleaseFactor / LightYearCm);
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("course-promote"), FString::Printf(TEXT("star=%lld d_ly=%.6g"), Point.CatalogIndex, DistanceCm / LightYearCm));
			}
			// Its twin first in its set's spheres (ApplySetExclusions).
			PushSetExclusions(Point.TwinSet);
		}

		/**
		 * Rio 06.10 (stage B, change 9): a point that stops being the course: within the take radius a near point again;
		 * beyond it retired (its glide law and course radius kept, released on its twin at glide 1).
		 */
		void DemoteCoursePoint(FApproachPoint& Point, const FVector& PilotLocal, const double Scale)
		{
			const double TakeCm = ApproachTakeCm();
			const double DistanceCm = FVector::Dist(PilotLocal, Point.ExactLocal) * Scale;
			Point.bCourse = false;
			if (Point.CourseTakeCm <= TakeCm)
			{
				Point.CourseTakeCm = 0.0;
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld no longer the course at %.6g ly: a near point again"),
					Point.CatalogIndex, DistanceCm / LightYearCm);
			}
			else
			{
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Stars] approach point %lld no longer the course at %.6g ly: retired (keeps its glide law to %.4g ly; %d retired)"),
					Point.CatalogIndex, DistanceCm / LightYearCm, Point.CourseTakeCm * ApproachReleaseFactor / LightYearCm, NumRetiredPoints());
			}
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("course-demote"), FString::Printf(TEXT("star=%lld d_ly=%.6g"), Point.CatalogIndex, DistanceCm / LightYearCm));
			}
			PushSetExclusions(Point.TwinSet);
		}

		/** Rio 06.10 (stage B): a point taken for the current course star by another rule (scan, own take) is the course point. */
		void PromoteIfCourse(const UWorld* World, FApproachPoint& Point, const FVector& PilotLocal, const double Scale)
		{
			if (CVarApproachCourseMaxLy.GetValueOnGameThread() > 0.0f && GCourse.World.Get() == World && !Point.bCourse
				&& Point.CatalogIndex != INDEX_NONE && Point.CatalogIndex == GCourse.Current)
			{
				PromoteCoursePoint(Point, PilotLocal, Scale);
			}
		}

		/** Rio 06.10 (stage B): the course star's twin as its level draws it (a course change or a rebuilt layer). */
		void RefillCourseTwin(const UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld, const int64 CatalogIndex)
		{
			FCourseTwin& Twin = GLayer.CourseTwin;
			Twin = FCourseTwin();
			Twin.bValid = true;
			Twin.CatalogIndex = CatalogIndex;
			if (CatalogIndex == INDEX_NONE)
			{
				return;
			}
			FTwinInfo TwinInfo;
			const bool bResolved = ResolveTwinOf(Galaxy, CatalogIndex, TwinInfo);
			if (TwinInfo.Record.CatalogIndex != INDEX_NONE)
			{
				Twin.ExactLocal = TwinInfo.Record.GalaxyLocalLocation;
			}
			if (!bResolved && TwinInfo.Record.CatalogIndex == INDEX_NONE)
			{
				return;
			}
			int32 Ordinal = INDEX_NONE;
			const bool bDrawnByGpu = GpuOrdinalOf(World, CatalogIndex, TwinInfo.Record.GalaxyLocalLocation, LocalToWorld, Ordinal);
			Twin.Ordinal = Ordinal;
			Twin.bGpu = bResolved && bDrawnByGpu && TwinInfo.bLit;
			Twin.Level = TwinInfo.Level;
			Twin.TwinSet = TwinInfo.TwinSet;
			Twin.TwinOffsetLocal = TwinInfo.OffsetLocal;
		}

		/** Rio 06.10 (stage B, change 9): the course changed: the old point steps down, the new one (if any) steps up. */
		void ChangeCourseStar(const UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld, const FVector& PilotLocal,
			const int64 NewIndex, const ECourseSource NewSource)
		{
			const double Scale = LocalToWorld.GetMaximumAxisScale();
			const ECourseSource OldSource = GCourse.CurrentSource;
			if (FApproachPoint* OldPoint = FindApproachPointOf(GCourse.Current); OldPoint && OldPoint->bCourse)
			{
				DemoteCoursePoint(*OldPoint, PilotLocal, Scale);
			}
			GCourse.Current = NewIndex;
			GCourse.CurrentSource = NewIndex != INDEX_NONE ? NewSource : ECourseSource::None;
			RefillCourseTwin(World, Galaxy, LocalToWorld, NewIndex);
			const FCourseTwin& Twin = GLayer.CourseTwin;
			if (NewIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] course star none (source %s)"), CourseSourceName(OldSource));
			}
			else
			{
				const double CapLy = FMath::Max(static_cast<double>(CVarApproachCourseMaxLy.GetValueOnGameThread()), 0.0);
				const double TwinOffsetCm = Twin.bGpu ? Twin.TwinOffsetLocal * Scale : 0.0;
				double RawCm = 0.0;
				const double RadiusCm = Twin.bGpu ? CourseTakeRadiusCm(TwinOffsetCm, GLayer.LastPilotPixelTangent, CapLy, &RawCm) : 0.0;
				const TCHAR* NoPoint = Twin.bGpu ? TEXT("")
					: Twin.Ordinal == INDEX_NONE ? TEXT(" (not drawn by the GPU layer: no approach point)")
					: static_cast<int64>(Twin.Ordinal) < GLayer.FirstOrdinal ? TEXT(" (an ISM-prefix star: no approach point)")
					: TEXT(" (a dark code: no approach point)");
				UE_LOG(LogTemp, Log,
					TEXT("[APS.Stars] course star %lld %s source=%s: twin %.4g AU off the exact place (L%d), take radius %.4g ly ")
					TEXT("(unclamped %.4g ly), now %.6g ly%s"),
					NewIndex, *StarLabelOf(World, NewIndex), CourseSourceName(GCourse.CurrentSource), TwinOffsetCm / AstronomicalUnitCm,
					Twin.bGpu ? Twin.Level : -1, RadiusCm / LightYearCm, RawCm / LightYearCm,
					FVector::Dist(PilotLocal, Twin.ExactLocal) * Scale / LightYearCm, NoPoint);
			}
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("course"), FString::Printf(TEXT("star=%lld csrc=%s"), NewIndex, CourseSourceName(GCourse.CurrentSource)), false);
			}
			if (FApproachPoint* NewPoint = FindApproachPointOf(NewIndex))
			{
				PromoteCoursePoint(*NewPoint, PilotLocal, Scale);
			}
		}

		/** Rio 06.10 (stage B, boresight): the result of a cone query along the flight direction. */
		struct FAlongHit
		{
			int32 Ordinal = INDEX_NONE;
			double TanSq = 0.0;
			double Along = 0.0;
			int32 Visited = 0;
			int32 Cells = 0;
			double Milliseconds = 0.0;
			bool bCapped = false;
		};

		/**
		 * Rio 06.10 (stage B, aps.Stars.ApproachBoresight): the drawn star nearest the ray from OriginLocal along DirLocal
		 * within the cone tan MaxTan, out to MaxLocal: a ray-march over the index in half-cell steps, each step visiting the
		 * cells its cone slice overlaps (once each, nearest first); the smallest angle wins, within a hundredth of the
		 * tolerance the nearer star. Steps whose slice misses the index box are skipped. Stops after MaxVisited stars (capped).
		 * PickAlongRay (a click, the whole index) is untouched.
		 */
		bool FindAlongHeading(const FStarIndex& Index, const FVector& OriginLocal, const FVector& DirLocal, const double MaxTan,
			const double MaxLocal, const int32 MaxVisited, const double MinAlongLocal, FAlongHit& Out)
		{
			const double Started = FPlatformTime::Seconds();
			Out = FAlongHit();
			const FVector Direction = DirLocal.GetSafeNormal();
			if (Index.Resolution <= 0 || !(MaxLocal > 0.0) || !(MaxTan > 0.0) || Direction.IsZero())
			{
				return false;
			}
			const double CellSize = Index.CellSize;
			const double StepLocal = 0.5 * CellSize;
			const FVector BoxMin(Index.Min);
			const FVector BoxMax = BoxMin + FVector(CellSize * Index.Resolution);
			const double MaxTanSq = MaxTan * MaxTan;
			TSet<int32, DefaultKeyFuncs<int32>, TInlineSetAllocator<128>> Seen;
			int32 BestSlot = INDEX_NONE;
			double BestTanSq = TNumericLimits<double>::Max();
			double BestAlong = TNumericLimits<double>::Max();
			// (The ratio is clamped as a double first: a huge window would overflow the int conversion.)
			const int32 Steps = FMath::CeilToInt(FMath::Min(MaxLocal / StepLocal, 100000.0));
			for (int32 StepIndex = 0; StepIndex <= Steps && !Out.bCapped; ++StepIndex)
			{
				const double Travel = FMath::Min(StepIndex * StepLocal, MaxLocal);
				const FVector Probe = OriginLocal + Direction * Travel;
				const double Reach = (Travel + StepLocal) * MaxTan + 0.5 * StepLocal;
				const FVector Low = Probe - FVector(Reach);
				const FVector High = Probe + FVector(Reach);
				if (High.X < BoxMin.X || High.Y < BoxMin.Y || High.Z < BoxMin.Z || Low.X > BoxMax.X || Low.Y > BoxMax.Y || Low.Z > BoxMax.Z)
				{
					continue;
				}
				const FIntVector CellLow = Index.CellOf(FVector3f(Low));
				const FIntVector CellHigh = Index.CellOf(FVector3f(High));
				for (int32 Z = CellLow.Z; Z <= CellHigh.Z && !Out.bCapped; ++Z)
				{
					for (int32 Y = CellLow.Y; Y <= CellHigh.Y && !Out.bCapped; ++Y)
					{
						for (int32 X = CellLow.X; X <= CellHigh.X && !Out.bCapped; ++X)
						{
							const int32 Cell = Index.LinearOf(FIntVector(X, Y, Z));
							bool bAlreadySeen = false;
							Seen.Add(Cell, &bAlreadySeen);
							if (bAlreadySeen)
							{
								continue;
							}
							++Out.Cells;
							for (int32 Slot = Index.CellStart[Cell]; Slot < Index.CellStart[Cell + 1]; ++Slot)
							{
								if (++Out.Visited > MaxVisited)
								{
									Out.bCapped = true;
									break;
								}
								const FVector Offset = FVector(Index.Positions[Slot]) - OriginLocal;
								const double Along = Offset | Direction;
								if (Along <= MinAlongLocal || Along > MaxLocal)
								{
									continue;
								}
								const double TanSq = (Offset.SizeSquared() - Along * Along) / (Along * Along);
								if (TanSq > MaxTanSq)
								{
									continue;
								}
								const bool bSameDirection = FMath::Abs(TanSq - BestTanSq) < 0.01 * MaxTanSq;
								if (BestSlot == INDEX_NONE || (bSameDirection ? Along < BestAlong : TanSq < BestTanSq))
								{
									BestSlot = Slot;
									BestTanSq = TanSq;
									BestAlong = Along;
								}
							}
						}
					}
				}
			}
			Out.Milliseconds = (FPlatformTime::Seconds() - Started) * 1000.0;
			if (BestSlot == INDEX_NONE)
			{
				return false;
			}
			Out.Ordinal = Index.Ordinals[BestSlot];
			Out.TanSq = BestTanSq;
			Out.Along = BestAlong;
			return true;
		}

		/**
		 * Rio 06.10 (stage B, aps.Stars.ApproachBoresight): the latched star along the flight direction. Dropped once passed or
		 * more than three cones off; queried four times a second without one or while turning (else once a second) in
		 * STELLAR or the star drive, out to the course radius plus one cadence of travel; paused while one cadence covers more
		 * than the course radius. A clearly better candidate (half the angle, the latch more than 0.05 deg off) takes over.
		 * A dark code is ignored; an ISM-prefix star may be latched (it blocks the GPU stars behind it) but gets no point.
		 * Rio 07.10: a query forced by a turn or a passed latch (aps.Stars.ApproachBoresightFrameHeading) skips the cadence;
		 * aps.Stars.ApproachBoresightAdaptiveCadence shortens the cadence instead of pausing; aps.Stars.ApproachBoresightKeepNeeded
		 * keeps a latch whose flight line passes within its course take radius.
		 */
		void UpdateBoresight(const UWorld* World, const AGalaxy& Galaxy, const FVector& PilotLocal, const double Scale,
			const double CourseMaxLy, const double WorldNow)
		{
			const bool bForce = GCourse.bForceQuery;
			GCourse.bForceQuery = false;
			const bool bKeepNeeded = CVarApproachBoresightKeepNeeded.GetValueOnGameThread() != 0;
			const double ConeRadians = FMath::DegreesToRadians(
				FMath::Clamp(static_cast<double>(CVarApproachBoresightDeg.GetValueOnGameThread()), 0.01, 30.0));
			const double ConeTan = FMath::Tan(ConeRadians);
			const FVector Heading = GCourse.HeadingLocal;
			const bool bHeading = !Heading.IsNearlyZero();
			// Rio 07.10 (aps.Stars.ApproachBoresightKeepNeeded): the flight line passes the latched star (still ahead) within its
			// course take radius, so it needs its point there.
			const auto BoreNeeded = [&Heading, &PilotLocal, bKeepNeeded]()
			{
				if (!bKeepNeeded || GCourse.Bore == INDEX_NONE || !(GCourse.BoreRadiusLocal > 0.0))
				{
					return false;
				}
				const FVector BoreOffset = GCourse.BoreLocal - PilotLocal;
				const double BoreAlong = BoreOffset | Heading;
				return BoreAlong > 0.0 && BoreOffset.SizeSquared() - BoreAlong * BoreAlong <= FMath::Square(GCourse.BoreRadiusLocal);
			};
			if (GCourse.Bore != INDEX_NONE && bHeading)
			{
				const FVector Offset = GCourse.BoreLocal - PilotLocal;
				const double Along = Offset | Heading;
				const double Tangent = Along > 0.0 ? FMath::Sqrt(FMath::Max(Offset.SizeSquared() - Along * Along, 0.0)) / Along
					: TNumericLimits<double>::Max();
				if (Along <= 0.0 || (Tangent > FMath::Tan(FMath::Min(3.0 * ConeRadians, 1.5)) && !BoreNeeded()))
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] boresight star none (%s)"), Along <= 0.0 ? TEXT("passed") : TEXT("turned away"));
					GCourse.Bore = INDEX_NONE;
					GCourse.bBoreIsm = false;
				}
				else
				{
					GCourse.BoreTan = Tangent;
				}
			}
			if (!GCourse.bInterstellar || !bHeading || !GIndex.IsValid() || !(Scale > 0.0))
			{
				return;
			}
			const double TurnedDegrees = GCourse.QueryHeadingLocal.IsNearlyZero() ? 180.0
				: FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Heading | GCourse.QueryHeadingLocal, -1.0, 1.0)));
			double Cadence = GCourse.Bore == INDEX_NONE || TurnedDegrees > 0.25 ? ApproachScanSeconds : 1.0;
			// Rio 07.10 (aps.Stars.ApproachBoresightAdaptiveCadence): one cadence of travel at most the course radius (down to
			// 1/120 s), the next resolve no later, so the window below stays within it instead of pausing. 0.999: Speed x
			// (CourseMaxLy / Speed) rounds above CourseMaxLy for about one speed in seven, which would pause after all.
			if (CVarApproachBoresightAdaptiveCadence.GetValueOnGameThread() != 0 && GCourse.SpeedLyPerSecond > 0.0)
			{
				Cadence = FMath::Max(FMath::Min(Cadence, 0.999 * CourseMaxLy / GCourse.SpeedLyPerSecond), 1.0 / 120.0);
				GCourse.NextResolveSeconds = FMath::Min(GCourse.NextResolveSeconds, WorldNow + Cadence);
			}
			if (!bForce && GCourse.LastQuerySeconds >= 0.0 && WorldNow - GCourse.LastQuerySeconds < Cadence - 0.02)
			{
				return;
			}
			const double WindowLy = GCourse.SpeedLyPerSecond * Cadence;
			if (WindowLy > CourseMaxLy)
			{
				if (!GCourse.bPausedLogged)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] boresight paused above %.4g ly/s"), CourseMaxLy / Cadence);
					GCourse.bPausedLogged = true;
				}
				return;
			}
			GCourse.bPausedLogged = false;
			GCourse.LastQuerySeconds = WorldNow;
			GCourse.QueryHeadingLocal = Heading;
			// Rio 07.10 (aps.Stars.ApproachBoresightFrameHeading): a query ran, so no forced one is left standing.
			GCourse.ForceHeadingLocal = FVector::ZeroVector;
			const double MaxLocal = FMath::Min(CourseMaxLy + WindowLy, 2.0 * CourseMaxLy) * LightYearCm / Scale;
			FAlongHit Hit;
			if (!FindAlongHeading(*GIndex, PilotLocal, Heading, ConeTan, MaxLocal, FMath::Max(CVarApproachBoresightMaxStars.GetValueOnGameThread(), 1),
				0.01 * LightYearCm / Scale, Hit))
			{
				return;
			}
			const int64 Candidate = GIndex->Order.Resolve(Hit.Ordinal);
			const bool bIsm = static_cast<int64>(Hit.Ordinal) - GLayer.FirstOrdinal < 0;
			FTwinInfo Twin;
			const bool bResolved = ResolveTwinOf(Galaxy, Candidate, Twin);
			if (Twin.Record.CatalogIndex == INDEX_NONE || (!bIsm && (!bResolved || !Twin.bLit)))
			{
				return; // A dark code (or no record): ignored for this query.
			}
			const FVector CandidateLocal = Twin.Record.GalaxyLocalLocation;
			const FVector Offset = CandidateLocal - PilotLocal;
			const double Along = Offset | Heading;
			if (!(Along > 0.0))
			{
				return;
			}
			const double CandidateTan = FMath::Sqrt(FMath::Max(Offset.SizeSquared() - Along * Along, 0.0)) / Along;
			bool bSwitch = Candidate != GCourse.Bore && (GCourse.Bore == INDEX_NONE
				|| (CandidateTan < 0.5 * GCourse.BoreTan && FMath::RadiansToDegrees(FMath::Atan(GCourse.BoreTan)) > 0.05));
			// Rio 07.10 (aps.Stars.ApproachBoresightKeepNeeded): a latch that needs its point and is nearer than the candidate stays.
			if (bSwitch && BoreNeeded() && FVector::DistSquared(GCourse.BoreLocal, PilotLocal) < Offset.SizeSquared())
			{
				bSwitch = false;
			}
			if (!bSwitch)
			{
				return;
			}
			GCourse.Bore = Candidate;
			GCourse.BoreLocal = CandidateLocal;
			GCourse.bBoreIsm = bIsm;
			GCourse.BoreTan = CandidateTan;
			// Rio 07.10 (aps.Stars.ApproachBoresightKeepNeeded): its course take radius (none for an ISM-prefix star: no point).
			GCourse.BoreRadiusLocal = !bIsm && bResolved
				? CourseTakeRadiusCm(Twin.OffsetLocal * Scale, GLayer.LastPilotPixelTangent, CourseMaxLy) / Scale : 0.0;
			const double Degrees = FMath::RadiansToDegrees(FMath::Atan(CandidateTan));
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Stars] boresight star %lld %s at %.6g ly, %.4f deg off the flight direction (%d star(s) in %d cell(s), %.3f ms%s%s)"),
				Candidate, *StarLabelOf(World, Candidate), Offset.Size() * Scale / LightYearCm, Degrees, Hit.Visited, Hit.Cells,
				Hit.Milliseconds, Hit.bCapped ? TEXT(", capped") : TEXT(""), bIsm ? TEXT(", ISM prefix: no point") : TEXT(""));
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("boresight"), FString::Printf(TEXT("gi=%lld deg=%.4f ly=%.6g"), Candidate, Degrees, Offset.Size() * Scale / LightYearCm));
			}
		}

		/** Rio 06.10 (stage B): the piloted ship (the pawn, else what it stands on or rides, up to a few parents). */
		void FindCourseShip(const UWorld* World)
		{
			const APlayerController* Controller = World->GetFirstPlayerController();
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			const UAPSShipFlightModel* Cached = GCourse.Flight.Get();
			if (Pawn == GCourse.Pawn.Get() && Cached && Cached->GetOwner() == Pawn)
			{
				return;
			}
			GCourse.Pawn = Pawn;
			const AActor* Ship = nullptr;
			const UAPSShipFlightModel* Found = nullptr;
			const auto Consider = [&Ship, &Found](const AActor* Candidate)
			{
				if (!Found && Candidate)
				{
					if (const UAPSShipFlightModel* Model = Candidate->FindComponentByClass<UAPSShipFlightModel>())
					{
						Found = Model;
						Ship = Candidate;
					}
				}
			};
			if (Pawn)
			{
				Consider(Pawn);
				int32 Depth = 0;
				for (const AActor* Base = APawn::GetMovementBaseActor(Pawn); Base && !Found && Depth < 5; Base = Base->GetAttachParentActor(), ++Depth)
				{
					Consider(Base);
				}
				Depth = 0;
				for (const AActor* Parent = Pawn->GetAttachParentActor(); Parent && !Found && Depth < 4; Parent = Parent->GetAttachParentActor(), ++Depth)
				{
					Consider(Parent);
				}
			}
			if (Found != Cached)
			{
				GCourse.LastApTarget.Reset();
				GCourse.ApIndex = INDEX_NONE;
				GCourse.NavStableId.Reset();
				GCourse.NavIndex = INDEX_NONE;
				GCourse.bNavExact = false;
				GCourse.bDirty = true;
			}
			GCourse.Flight = Found;
			GCourse.Nav = Ship ? Ship->FindComponentByClass<UShipNavigationComponent>() : nullptr;
		}

		/**
		 * Rio 06.10 (stage B, change 9): the autopilot's target and the band, every frame (O(1)). A target destroyed while
		 * engaged makes both sides null: the engaged flag clears the course then (review).
		 */
		void PollCourseAutopilot(const UWorld* World)
		{
			const UAPSShipFlightModel* Flight = GCourse.Flight.Get();
			if (!Flight)
			{
				if (GCourse.ApIndex != INDEX_NONE || GCourse.bInterstellar)
				{
					GCourse.ApIndex = INDEX_NONE;
					GCourse.bInterstellar = false;
					GCourse.bDirty = true;
				}
				return;
			}
			const AActor* ApTarget = Flight->GetAutopilotTarget();
			if (ApTarget != GCourse.LastApTarget.Get())
			{
				GCourse.LastApTarget = ApTarget;
				GCourse.ApIndex = Flight->IsAutopilotEngaged() ? CourseIndexOfActor(ApTarget, APSStarSystemsFind(World), CachedActive(World))
					: INDEX_NONE;
				GCourse.bDirty = true;
			}
			else if (GCourse.ApIndex != INDEX_NONE && !Flight->IsAutopilotEngaged())
			{
				GCourse.ApIndex = INDEX_NONE;
				GCourse.bDirty = true;
			}
			const bool bInterstellar = Flight->GetFlightBand() == EAPSFlightBand::Stellar || Flight->IsStarDriveActive();
			if (bInterstellar != GCourse.bInterstellar)
			{
				GCourse.bInterstellar = bInterstellar;
				GCourse.bDirty = true;
			}
		}

		/** Rio 06.10 (stage B): the crossfade a point taken at TakePlace starts at, from the view (1 without its sphere). */
		float ViewFadeTarget(const FStandingSystem* Own, const FVector& TakePlace, const FApproachEyes& Eyes, const double FadeStart,
			const double FadeEnd)
		{
			if (!IsOwnSphere(Own, TakePlace))
			{
				return 1.0f;
			}
			return static_cast<float>(1.0 - FMath::SmoothStep(FadeStart, FadeEnd,
				Own->StarRadiusCm / FMath::Max(FVector::Dist(Eyes.ViewCamera, TakePlace) * Eyes.ViewPixelTangent, 1.0e-30)));
		}

		/**
		 * Rio 06.10 (stage B, change 9, aps.Stars.ApproachCourseMaxLy): the course star (the autopilot's target, else in STELLAR
		 * or the star drive the HUD's course when it lies in the boresight cone, else the boresight star, else the HUD's course)
		 * and its far take. The boresight runs without an autopilot target that is a galaxy star (a cluster system or body
		 * target leaves the boresight on: the course is then the star along the flight direction). The far take is: one point, outside the 8, taken where its twin's error would reach aps.Stars.ApproachCoursePixels
		 * for the pilot (at most the cap, at least the near take radius). No speed gate: at the drive's speed it may land a
		 * frame late, on its twin (glide 1), so nothing jumps. 0: no course, a course point steps down.
		 */
		void TickCourseStar(UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld, const FApproachEyes& Eyes,
			const FVector& ViewLocal, const FVector& PilotLocal, const bool bIdentity, const bool bRetakeAtExact, const double FadeStart,
			const double FadeEnd, FLazyStanding& Standing, const double WorldNow)
		{
			if (GCourse.World.Get() != World)
			{
				GCourse = FCourseTracker();
				GCourse.World = World;
			}
			const double CourseMaxLy = FMath::Max(static_cast<double>(CVarApproachCourseMaxLy.GetValueOnGameThread()), 0.0);
			if (!(CourseMaxLy > 0.0))
			{
				if (GCourse.Current != INDEX_NONE)
				{
					ChangeCourseStar(World, Galaxy, LocalToWorld, PilotLocal, INDEX_NONE, ECourseSource::None);
				}
				GCourse.bDirty = true;
				return;
			}
			const double Scale = LocalToWorld.GetMaximumAxisScale();
			// 1. Every frame, O(1): the autopilot's target and the band.
			PollCourseAutopilot(World);
			// 1b. Rio 07.10 (aps.Stars.ApproachBoresightFrameHeading): every frame, O(1), while the boresight runs: the flight
			// direction and speed of the last frame; a turn past aps.Stars.ApproachBoresightRequeryDeg (in STELLAR or the drive)
			// or a latched star fallen behind resolves and queries in this frame. The 0.25 s sample was the average of the old
			// direction for a whole resolve after each turn: the star ahead was dropped as passed and one behind latched.
			const bool bFrameHeading = CVarApproachBoresight.GetValueOnGameThread() != 0
				&& CVarApproachBoresightFrameHeading.GetValueOnGameThread() != 0;
			if (!bFrameHeading)
			{
				GCourse.LastFrameSeconds = -1.0;
				GCourse.LastFrameStepCm = 0.0;
				GCourse.LastFrameSpeedCm = 0.0;
			}
			else if (GCourse.LastFrameSeconds < 0.0 || WorldNow - GCourse.LastFrameSeconds > 1.0e-4)
			{
				if (GCourse.LastFrameSeconds >= 0.0 && Scale > 0.0)
				{
					const double FrameSeconds = WorldNow - GCourse.LastFrameSeconds;
					const FVector Step = PilotLocal - GCourse.LastFramePilotLocal;
					const double StepCm = Step.Size() * Scale;
					const double StepSpeedCm = StepCm / FrameSeconds;
					// Twenty times faster than the last frame (2000 km/s from rest) is a jump (a map trip), not a direction. The
					// last accepted speed counts too, so a frame without travel does not make the next real step a jump.
					const bool bJump = StepSpeedCm > 20.0 * FMath::Max3(GCourse.LastFrameSpeedCm, GCourse.SpeedLyPerSecond * LightYearCm, 1.0e7);
					if (StepCm > 1.0e5 && !bJump)
					{
						GCourse.HeadingLocal = Step / Step.Size();
						GCourse.SpeedLyPerSecond = StepSpeedCm / LightYearCm;
						const double RequeryDegrees = FMath::Max(static_cast<double>(CVarApproachBoresightRequeryDeg.GetValueOnGameThread()), 0.0);
						const double RequeryCos = FMath::Cos(FMath::DegreesToRadians(FMath::Min(RequeryDegrees, 180.0)));
						const FVector Heading = GCourse.HeadingLocal;
						// Turned since the last query and since a forced one that could not run (e.g. paused: it does not force
						// every frame; a query that runs clears ForceHeadingLocal, so a turn back to an earlier heading forces).
						const bool bTurned = RequeryDegrees > 0.0 && GCourse.bInterstellar && !GCourse.QueryHeadingLocal.IsNearlyZero()
							&& (Heading | GCourse.QueryHeadingLocal) < RequeryCos
							&& (GCourse.ForceHeadingLocal.IsNearlyZero() || (Heading | GCourse.ForceHeadingLocal) < RequeryCos);
						// The boresight drops a latch behind the ship as passed at once (UpdateBoresight), so this fires once.
						const bool bBehind = GCourse.Bore != INDEX_NONE && ((GCourse.BoreLocal - PilotLocal) | Heading) <= 0.0;
						if ((bTurned || bBehind) && GCourse.ApIndex == INDEX_NONE)
						{
							GCourse.bDirty = true;
							GCourse.bForceQuery = true;
							GCourse.ForceHeadingLocal = Heading;
						}
					}
					GCourse.LastFrameStepCm = StepCm;
					GCourse.LastFrameSpeedCm = StepSpeedCm;
				}
				GCourse.LastFramePilotLocal = PilotLocal;
				GCourse.LastFrameSeconds = WorldNow;
			}
			// 2. Four times a second (and on a change): the ship, the HUD's course, the flight direction, the boresight.
			if (GCourse.bDirty || WorldNow >= GCourse.NextResolveSeconds)
			{
				GCourse.bDirty = false;
				GCourse.NextResolveSeconds = WorldNow + ApproachScanSeconds;
				FindCourseShip(World);
				PollCourseAutopilot(World);
				GCourse.bDirty = false;
				const FAPSStarSystems* Registry = APSStarSystemsFind(World);
				const UShipNavigationComponent* Nav = GCourse.Nav.Get();
				const FShipNavigationContact* Selected = Nav ? Nav->GetSelectedContact() : nullptr;
				const FString SelectedId = Selected ? Selected->StableId : FString();
				if (!SelectedId.Equals(GCourse.NavStableId, ESearchCase::CaseSensitive))
				{
					GCourse.NavStableId = SelectedId;
					GCourse.NavIndex = !Selected ? INDEX_NONE : Selected->Actor.IsValid()
						? CourseIndexOfActor(Selected->Actor.Get(), Registry, CachedActive(World)) : NavLabelIndex(SelectedId, Registry);
					GCourse.bNavExact = false;
					GCourse.bNavHeld = false;
					FGalaxyCatalogStarRecord Record;
					if (GCourse.NavIndex != INDEX_NONE && Galaxy.StarCatalog.ResolveStar(GCourse.NavIndex, Record) && Record.CatalogIndex != INDEX_NONE)
					{
						GCourse.NavExactLocal = Record.GalaxyLocalLocation;
						GCourse.bNavExact = true;
					}
				}
				// The flight direction: the pilot's travel through the sky since the last sample (more than a kilometre). Rio 07.10
				// (aps.Stars.ApproachBoresightFrameHeading): the last frame's travel (1b) instead while it is over a kilometre.
				const bool bFrameSample = bFrameHeading && GCourse.LastFrameStepCm > 1.0e5;
				if (GCourse.LastPilotSeconds >= 0.0 && !bFrameSample)
				{
					const FVector Delta = PilotLocal - GCourse.LastPilotLocal;
					const double DeltaCm = Delta.Size() * Scale;
					const double DeltaSeconds = WorldNow - GCourse.LastPilotSeconds;
					if (DeltaCm > 1.0e5)
					{
						GCourse.HeadingLocal = Delta.GetSafeNormal();
					}
					if (DeltaSeconds > 1.0e-4)
					{
						GCourse.SpeedLyPerSecond = DeltaCm / DeltaSeconds / LightYearCm;
					}
				}
				GCourse.LastPilotLocal = PilotLocal;
				GCourse.LastPilotSeconds = WorldNow;
				const bool bBoresight = CVarApproachBoresight.GetValueOnGameThread() != 0;
				if (!bBoresight && GCourse.Bore != INDEX_NONE)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] boresight star none (off)"));
					GCourse.Bore = INDEX_NONE;
					GCourse.bBoreIsm = false;
				}
				if (bBoresight && GCourse.ApIndex == INDEX_NONE)
				{
					UpdateBoresight(World, Galaxy, PilotLocal, Scale, CourseMaxLy, WorldNow);
				}
				GCourse.bForceQuery = false;
				// The effective course.
				int64 Wanted = INDEX_NONE;
				ECourseSource WantedSource = ECourseSource::None;
				const bool bNavWasHeld = GCourse.bNavHeld;
				GCourse.bNavHeld = false;
				if (GCourse.ApIndex != INDEX_NONE)
				{
					Wanted = GCourse.ApIndex;
					WantedSource = ECourseSource::Autopilot;
				}
				else if (bBoresight && (GCourse.bInterstellar || GCourse.Bore != INDEX_NONE))
				{
					bool bNavInCone = false;
					if (GCourse.NavIndex != INDEX_NONE && GCourse.bNavExact && !GCourse.HeadingLocal.IsNearlyZero())
					{
						const FVector ToNav = (GCourse.NavExactLocal - PilotLocal).GetSafeNormal();
						double ConeDegrees = FMath::Clamp(static_cast<double>(CVarApproachBoresightDeg.GetValueOnGameThread()), 0.01, 30.0);
						// Rio 07.10 (aps.Stars.ApproachBoresightNavHold): once the course by the cone, held to three cones.
						if (bNavWasHeld && CVarApproachBoresightNavHold.GetValueOnGameThread() != 0)
						{
							ConeDegrees = FMath::Min(3.0 * ConeDegrees, 85.0);
						}
						bNavInCone = !ToNav.IsZero()
							&& FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ToNav | GCourse.HeadingLocal, -1.0, 1.0))) <= ConeDegrees;
					}
					GCourse.bNavHeld = bNavInCone;
					if (bNavInCone || (GCourse.Bore == INDEX_NONE && GCourse.NavIndex != INDEX_NONE))
					{
						Wanted = GCourse.NavIndex;
						WantedSource = ECourseSource::Navigation;
					}
					else if (GCourse.Bore != INDEX_NONE)
					{
						Wanted = GCourse.Bore;
						WantedSource = ECourseSource::Boresight;
					}
				}
				else if (GCourse.NavIndex != INDEX_NONE)
				{
					Wanted = GCourse.NavIndex;
					WantedSource = ECourseSource::Navigation;
				}
				if (Wanted != GCourse.Current)
				{
					ChangeCourseStar(World, Galaxy, LocalToWorld, PilotLocal, Wanted, WantedSource);
				}
				else
				{
					GCourse.CurrentSource = WantedSource;
					if (GCourse.Current != INDEX_NONE && (!GLayer.CourseTwin.bValid || GLayer.CourseTwin.CatalogIndex != GCourse.Current))
					{
						RefillCourseTwin(World, Galaxy, LocalToWorld, GCourse.Current);
					}
				}
			}
			// 3. The far take, every frame, O(1) until the course star is within its radius.
			FCourseTwin& Twin = GLayer.CourseTwin;
			if (GCourse.Current == INDEX_NONE || !Twin.bValid || Twin.CatalogIndex != GCourse.Current || !Twin.bGpu
				|| (Twin.bBlocked && WorldNow < Twin.RetrySeconds) || FindApproachPointOf(GCourse.Current) != nullptr)
			{
				return;
			}
			const double DistanceCm = FVector::Dist(PilotLocal, Twin.ExactLocal) * Scale;
			const double RadiusCm = CourseTakeRadiusCm(Twin.TwinOffsetLocal * Scale, Eyes.PilotPixelTangent, CourseMaxLy);
			if (DistanceCm > RadiusCm)
			{
				return;
			}
			const FStandingSystems& Systems = Standing.Get();
			const FVector ExactWorld = LocalToWorld.TransformPosition(Twin.ExactLocal);
			bool bByIdentity = false;
			const FActiveGalaxySystem Active = bIdentity ? CachedActive(World) : FActiveGalaxySystem();
			const FStandingSystem* Own = bIdentity ? OwnOf(Systems, ExactWorld, Twin.CatalogIndex, Active, true, &bByIdentity)
				: OwnSystemOf(Systems, ExactWorld);
			if (InForeignSystem(Systems, ExactWorld, Own))
			{
				Twin.bBlocked = true;
				Twin.RetrySeconds = WorldNow + ApproachScanSeconds;
				return;
			}
			const bool bAtExact = bRetakeAtExact && Own != nullptr && !Own->bHome;
			const FVector TakePlace = bAtExact && bByIdentity ? Own->StarInSky : ExactWorld;
			FNearStar Star;
			Star.CatalogIndex = Twin.CatalogIndex;
			Star.Ordinal = Twin.Ordinal;
			Star.WorldLocation = ExactWorld;
			Star.DistanceCm = DistanceCm;
			if (!TakeApproachPoint(World, Galaxy, LocalToWorld, ViewLocal, PilotLocal, Star, RadiusCm / Scale, Eyes.ViewPixelTangent, bAtExact,
				bAtExact ? ViewFadeTarget(Own, TakePlace, Eyes, FadeStart, FadeEnd) : 1.0f, WorldNow))
			{
				Twin.bBlocked = true;
				Twin.RetrySeconds = WorldNow + ApproachScanSeconds;
				return;
			}
			FApproachPoint& Point = GLayer.Approach.Last();
			Point.bCourse = true;
			Point.CourseTakeCm = RadiusCm;
			if (bAtExact && bByIdentity)
			{
				FollowOwnStar(Point, Own, true, LocalToWorld, Scale, FPlatformTime::Seconds(), true);
			}
			// Its twin first in its set's spheres.
			PushSetExclusions(Point.TwinSet);
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld is the course star's far take at %.6g ly (take radius %.4g ly, source %s)"),
				Point.CatalogIndex, DistanceCm / LightYearCm, RadiusCm / LightYearCm, CourseSourceName(GCourse.CurrentSource));
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceEvent(TEXT("course-take"), FString::Printf(TEXT("star=%lld d_ly=%.6g ctake_ly=%.6g csrc=%s"), Point.CatalogIndex,
					DistanceCm / LightYearCm, RadiusCm / LightYearCm, CourseSourceName(GCourse.CurrentSource)));
			}
		}

		/**
		 * Rio 06.10 (stage B, aps.Stars.ApproachOwnTake): the GPU star of the system the materializer stands gets its point at
		 * once (at the exact place, at the view's crossfade value, outside the 8), before the far glyphs look for a stand-in
		 * in the same frame. A failure retries four times a second; one that lasts a second warns once per standing system
		 * (the far glyph then stands in).
		 */
		void TickOwnTake(UWorld* World, const AGalaxy& Galaxy, const FTransform& LocalToWorld, const FApproachEyes& Eyes,
			const FVector& ViewLocal, const FVector& PilotLocal, const FActiveGalaxySystem& Active, const double FadeStart,
			const double FadeEnd, FLazyStanding& Standing, const double WorldNow)
		{
			FActiveCache& Cache = GLayer.ActiveCache;
			if (Active.CatalogIndex == INDEX_NONE || !Cache.bGpu || WorldNow < Cache.OwnRetrySeconds
				|| FindApproachPointOf(Active.CatalogIndex) != nullptr)
			{
				return;
			}
			const double Scale = LocalToWorld.GetMaximumAxisScale();
			const double TakeCm = ApproachTakeCm();
			const double DistanceCm = FVector::Dist(PilotLocal, Cache.ExactLocal) * Scale;
			// Without the keep a point beyond the radius would be released next frame and taken again: no churn.
			if (CVarApproachKeepWhileStanding.GetValueOnGameThread() == 0 && DistanceCm > TakeCm)
			{
				return;
			}
			const FStandingSystems& Systems = Standing.Get();
			const FVector ExactWorld = LocalToWorld.TransformPosition(Cache.ExactLocal);
			bool bByIdentity = false;
			const FStandingSystem* Own = OwnOf(Systems, ExactWorld, Active.CatalogIndex, Active, true, &bByIdentity);
			const TCHAR* Failure = !Own ? TEXT("no standing system holds it")
				: Own->bHome ? TEXT("it is the home system")
				: InForeignSystem(Systems, ExactWorld, Own) ? TEXT("inside another system's sphere") : nullptr;
			if (!Failure)
			{
				const FVector TakePlace = bByIdentity ? Own->StarInSky : ExactWorld;
				FNearStar Star;
				Star.CatalogIndex = Active.CatalogIndex;
				Star.Ordinal = Cache.Ordinal;
				Star.WorldLocation = ExactWorld;
				Star.DistanceCm = DistanceCm;
				if (TakeApproachPoint(World, Galaxy, LocalToWorld, ViewLocal, PilotLocal, Star, TakeCm / Scale, Eyes.ViewPixelTangent, true,
					ViewFadeTarget(Own, TakePlace, Eyes, FadeStart, FadeEnd), WorldNow))
				{
					FApproachPoint& Point = GLayer.Approach.Last();
					if (bByIdentity)
					{
						FollowOwnStar(Point, Own, true, LocalToWorld, Scale, FPlatformTime::Seconds(), true);
					}
					Cache.OwnFailSince = -1.0;
					UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld taken for its standing system at once (own take%s)"),
						Point.CatalogIndex, bByIdentity ? TEXT(", by identity") : TEXT(", by place"));
					if (CVarApproachTrace.GetValueOnGameThread() != 0)
					{
						TraceEvent(TEXT("own-take"), FString::Printf(TEXT("star=%lld d_ly=%.6g"), Point.CatalogIndex, DistanceCm / LightYearCm));
					}
					PromoteIfCourse(World, Point, PilotLocal, Scale);
					return;
				}
				Failure = TEXT("the take failed");
			}
			Cache.OwnRetrySeconds = WorldNow + ApproachScanSeconds;
			if (Cache.OwnFailSince < 0.0)
			{
				Cache.OwnFailSince = WorldNow;
			}
			if (!Cache.bOwnFailLogged && WorldNow - Cache.OwnFailSince >= 1.0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] own take of %lld failed (%s)"), Active.CatalogIndex, Failure);
				Cache.bOwnFailLogged = true;
			}
		}

		/**
		 * Rio 06.10 (stage B, aps.Stars.ApproachCourseRetired): at most that many retired points beyond the take radius; on
		 * overflow the one with the smallest slide ((1 - glide) x its twin's error in pilot pixels) hands back to its twin.
		 */
		void EnforceRetiredPool(const FVector& PilotLocal, const double Scale, const double TakeCm, const double WorldNow)
		{
			const int32 Allowed = FMath::Max(CVarApproachCourseRetired.GetValueOnGameThread(), 0);
			const double PilotTangent = FMath::Max(GLayer.LastPilotPixelTangent, 1.0e-12);
			while (RetiredOut(TakeCm) > Allowed)
			{
				FApproachPoint* Smallest = nullptr;
				double SmallestSlide = TNumericLimits<double>::Max();
				for (FApproachPoint& Point : GLayer.Approach)
				{
					if (!IsRetiredOut(Point, TakeCm))
					{
						continue;
					}
					const double Slide = (1.0 - GlideOf(Point, PilotLocal, WorldNow)) * FVector::Dist(FVector(Point.TwinLocal), Point.ExactLocal)
						* Scale / (FMath::Max(Point.LastPilotCm, 1.0) * PilotTangent);
					if (!Smallest || Slide < SmallestSlide)
					{
						SmallestSlide = Slide;
						Smallest = &Point;
					}
				}
				if (!Smallest)
				{
					break;
				}
				StartHandBack(*Smallest, TEXT("retired pool full"), WorldNow, PilotLocal);
			}
		}
	}

	bool AreApproachPointsActive(const UWorld* World)
	{
		return World != nullptr && CVarApproachPoint.GetValueOnGameThread() != 0 && APSRealScale::IsActive(World);
	}

	bool DrawsApproachPoint(const UWorld* World, const int64 CatalogIndex)
	{
		const AGalaxy* Galaxy = GLayer.Galaxy.Get();
		return World != nullptr && IsValid(Galaxy) && Galaxy->GetWorld() == World
			&& GLayer.Approach.ContainsByPredicate([CatalogIndex](const FApproachPoint& Point)
			{
				return Point.CatalogIndex == CatalogIndex && Point.Handle != 0;
			});
	}

	void SetNearPhotospheres(const UWorld* World, const TConstArrayView<int64> CatalogIndices)
	{
		GNearPhotosphereWorld = World;
		GNearPhotospheres.Reset();
		for (const int64 CatalogIndex : CatalogIndices)
		{
			GNearPhotospheres.Add(CatalogIndex);
		}
	}

	void RescanExclusions(UWorld* World)
	{
		AGalaxy* Galaxy = GLayer.Galaxy.Get();
		if (!AreApproachPointsActive(World) || GLayer.Mode != ELayerMode::Gameplay || !IsValid(Galaxy) || Galaxy->GetWorld() != World)
		{
			return;
		}
		// Before: up to ExclusionScanSeconds (0.5 s) with the old spheres, the point and the new star both drawn, or neither.
		GGameplay.NextExclusionScanSeconds = 0.0;
		PushGameplayPresentation(*Galaxy, true);
		UE_LOG(LogTemp, Log, TEXT("[APS.GalaxyGpu] exclusions scanned again at once (a system stood up or went): %d sphere(s)"),
			GLayer.Exclusions.Num());
	}

	void UpdateApproachPoints(UWorld* World, const FVector& Camera, const double PixelTangent)
	{
		// Rio 06.10 (stage B): the view is the pilot's (stage A).
		FApproachEyes Eyes;
		Eyes.ViewCamera = Camera;
		Eyes.ViewPixelTangent = PixelTangent;
		Eyes.PilotCamera = Camera;
		Eyes.PilotPixelTangent = PixelTangent;
		Eyes.bDetached = false;
		UpdateApproachPoints(World, Eyes);
	}

	void UpdateApproachPoints(UWorld* World, const FApproachEyes& Eyes)
	{
		// Rio 06.10 (audit: trace scopes, instrumentation only).
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_ApproachPoints);
		AGalaxy* Galaxy = GLayer.Galaxy.Get();
		FTransform LocalToWorld;
		const FVector& Camera = Eyes.ViewCamera;
		const double PixelTangent = Eyes.ViewPixelTangent;
		const bool bActive = AreApproachPointsActive(World) && GLayer.Mode == ELayerMode::Gameplay && IsValid(Galaxy)
			&& Galaxy->GetWorld() == World && GLayer.PointSets.Num() > 0 && GLayer.PointLevels.Num() == GLayer.PointSets.Num()
			&& GetIndexedGalaxy(World) == Galaxy && FMath::IsFinite(PixelTangent) && PixelTangent > 0.0
			&& FMath::IsFinite(Eyes.PilotPixelTangent) && Eyes.PilotPixelTangent > 0.0 && (!Eyes.bDetached || !Eyes.PilotCamera.ContainsNaN())
			&& MakeGameplayLocalToWorld(*Galaxy, LocalToWorld);
		if (!bActive)
		{
			if (GLayer.Approach.Num() > 0 && (!IsValid(Galaxy) || Galaxy->GetWorld() == World))
			{
				ReleaseApproachPoints(INDEX_NONE, TEXT("approach points off"));
			}
			// Rio 06.10 (aps.Stars.ApproachTrace): the far glyphs' bursts (aps.Stars.ApproachTraceBurst) are shot here, too.
			if (CVarApproachTrace.GetValueOnGameThread() != 0)
			{
				TraceShoot();
			}
			return;
		}
		const double Scale = LocalToWorld.GetMaximumAxisScale();
		// Rio 06.10 (stage B, change 12 "pilot's eyes"): the view (CameraLocal) draws: the dot's place, the fade, the
		// visibility; the pilot (PilotLocal: the view itself while attached) decides: takes, releases, the glide, the speed.
		const FVector CameraLocal = LocalToWorld.InverseTransformPosition(Camera);
		const FVector PilotLocal = Eyes.bDetached ? LocalToWorld.InverseTransformPosition(Eyes.PilotCamera) : CameraLocal;
		const double Now = FPlatformTime::Seconds();
		const double WorldNow = World->GetTimeSeconds();
		const float DeltaSeconds = FMath::Clamp(World->GetDeltaSeconds(), 0.0f, 0.25f);
		const double TakeCm = FMath::Max(static_cast<double>(CVarApproachPointLy.GetValueOnGameThread()), 0.01) * LightYearCm;
		const double TakeLocal = TakeCm / Scale;
		// The pilot's speed through the sky (an origin shift moves camera and sky alike; the still ship's sky offset moves
		// the sky under a camera that stays).
		const double Since = Now - GLayer.LastApproachCameraSeconds;
		const double SpeedCm = GLayer.LastApproachCameraSeconds > 0.0 && Since > 1.0e-4
			? FVector::Dist(PilotLocal, GLayer.LastApproachCameraLocal) * Scale / Since : 0.0;
		GLayer.LastApproachCameraLocal = PilotLocal;
		GLayer.LastApproachCameraSeconds = Now;
		GLayer.LastPilotPixelTangent = Eyes.PilotPixelTangent;
		const bool bEyesFlipped = Eyes.bDetached != GLayer.bLastDetached;
		const bool bTrace = CVarApproachTrace.GetValueOnGameThread() != 0;

		// The crossfade's band (a point taken inside its own standing system starts at its value).
		static IConsoleVariable* GlyphPixelsVariable = nullptr;
		static IConsoleVariable* PointIntensityVariable = nullptr;
		static IConsoleVariable* MaxPixelVariable = nullptr;
		const float GlyphPixels = ConsoleFloat(GlyphPixelsVariable, TEXT("aps.Stars.FarGlyphPixels"), 2.2f);
		const double FadeEnd = GlyphPixels > 0.0f ? GlyphPixels : 2.2;
		const double FadeStart = FadeEnd * FMath::Clamp(static_cast<double>(CVarApproachPointFade.GetValueOnGameThread()), 0.05, 1.0);
		// Rio 06.10 (star approach v2, stage A): a point's own system by identity (aps.Stars.ApproachOwnByIdentity) and a take
		// inside it at the exact place (aps.Stars.ApproachRetakeAtExact). Not switched by the F10 map: opening it must not
		// release a point its own sphere holds or slide a re-anchored one away, and a take while the map camera is near
		// (SYSTEM view) must not start on the hidden twin (glide pinned at 1, still there after the map closes). Neither
		// touches the map hold: an approach point keeps its own IntensityScale (TakeApproachPoint), the hold is the level sets'.
		const bool bIdentity = CVarApproachOwnByIdentity.GetValueOnGameThread() != 0;
		const bool bRetakeAtExact = CVarApproachRetakeAtExact.GetValueOnGameThread() != 0;
		// Rio 06.10 (stage B): cached by its key (CachedActive), the same result as FindActiveGalaxySystem.
		const FActiveGalaxySystem Active = bIdentity ? CachedActive(World) : FActiveGalaxySystem();
		FLazyStanding Standing;
		Standing.World = World;
		// Rio 06.10 (stage B, change 9): the course star and its far take; (aps.Stars.ApproachOwnTake) the standing system's
		// star at once. Both before the early return and the far glyphs of this frame.
		TickCourseStar(World, *Galaxy, LocalToWorld, Eyes, CameraLocal, PilotLocal, bIdentity, bRetakeAtExact, FadeStart, FadeEnd,
			Standing, WorldNow);
		if (CVarApproachOwnTake.GetValueOnGameThread() != 0)
		{
			TickOwnTake(World, *Galaxy, LocalToWorld, Eyes, CameraLocal, PilotLocal, Active, FadeStart, FadeEnd, Standing, WorldNow);
		}

		// New approach points: the drawn GPU stars within the take radius, four times a second (and each tenth of it travelled).
		// Rio 06.10 (stage B): course and retired points beyond the take radius never drive the scan (NumNearSlots); with
		// aps.Stars.ApproachScanBounded only the timer scans while the pilot is too fast to take one.
		const int32 NearSlots = NumNearSlots(TakeCm);
		const bool bMayTake = SpeedCm * ApproachTakeSeconds <= TakeCm;
		const bool bScanTimer = Now - GLayer.LastApproachScanSeconds >= ApproachScanSeconds;
		const bool bScanMoved = FVector::Dist(PilotLocal, GLayer.LastApproachScanLocal) >= 0.1 * TakeLocal;
		const bool bScan = CVarApproachScanBounded.GetValueOnGameThread() != 0
			? (bMayTake && (bScanTimer || bScanMoved)) || (NearSlots > 0 && bScanTimer)
			: (bMayTake || NearSlots > 0) && (bScanTimer || bScanMoved);
		if (!bScan && GLayer.Approach.Num() == 0)
		{
			if (bEyesFlipped)
			{
				GLayer.bLastDetached = Eyes.bDetached;
				if (bTrace)
				{
					TraceEvent(Eyes.bDetached ? TEXT("eyes-pilot") : TEXT("eyes-view"));
				}
			}
			// Rio 06.10 (aps.Stars.ApproachTrace): the traced star is followed while it is only its twin, too.
			if (bTrace)
			{
				TraceApproach(World, Eyes, LocalToWorld, Standing.bGathered ? &Standing.Systems : nullptr);
			}
			return; // Nothing near: no actor walk, no index query.
		}
		// Rio 06.10 (stage B): the light path. Without a scan, when every point stands at its catalogue place beyond the
		// release radius and is a course or retired point or the standing system's own star (identity, O(1)), no actor walk:
		// no sphere is near enough to draw (its disc is far below a pixel there) and the keep needs only the identity.
		bool bLight = !bScan;
		if (bLight)
		{
			const double LightCm = TakeCm * ApproachReleaseFactor;
			for (const FApproachPoint& Point : GLayer.Approach)
			{
				const bool bKind = Point.CourseTakeCm > 0.0 || (bIdentity && Active.CatalogIndex != INDEX_NONE && Active.CatalogIndex == Point.CatalogIndex);
				if (!bKind || Point.Anchor != EApproachAnchor::Catalogue || !(FVector::Dist(PilotLocal, Point.ExactLocal) * Scale > LightCm))
				{
					bLight = false;
					break;
				}
			}
		}
		if (!bLight)
		{
			Standing.Get();
		}
		const bool bGathered = Standing.bGathered;
		const FStandingSystems& Systems = Standing.Systems;
		if (bScan)
		{
			GLayer.LastApproachScanSeconds = Now;
			GLayer.LastApproachScanLocal = PilotLocal;
			TArray<FNearStar> Near;
			FindNearStars(World, Eyes.bDetached ? Eyes.PilotCamera : Camera, MaxApproachPoints, TakeCm * ApproachReleaseFactor, Near);
			for (const FNearStar& Star : Near)
			{
				// The sky's ISM prefix is drawn by the galaxy HISM, not by the GPU layer.
				const int64 GpuOrdinal = static_cast<int64>(Star.Ordinal) - GLayer.FirstOrdinal;
				if (GpuOrdinal < 0 || GpuOrdinal >= GLayer.PointOrdinals)
				{
					continue;
				}
				bool bStarOwnByIdentity = false;
				const FStandingSystem* StarOwn = bIdentity
					? OwnOf(Systems, Star.WorldLocation, Star.CatalogIndex, Active, true, &bStarOwnByIdentity)
					: OwnSystemOf(Systems, Star.WorldLocation);
				const bool bForeign = InForeignSystem(Systems, Star.WorldLocation, StarOwn);
				if (FApproachPoint* Existing = GLayer.Approach.FindByPredicate([&Star](const FApproachPoint& Point)
					{
						return Point.CatalogIndex == Star.CatalogIndex;
					}))
				{
					Existing->bRelease = Existing->bRelease || bForeign;
					continue;
				}
				if (!bForeign && bMayTake && Star.DistanceCm <= TakeCm && NumNearSlots(TakeCm) < MaxApproachPoints)
				{
					// Rio 06.10 (aps.Stars.ApproachRetakeAtExact): inside its own standing system (after the map, a teleport or
					// a load) its twin is hidden: the point starts at the exact place, at the crossfade's value there.
					const bool bAtExact = bRetakeAtExact && StarOwn != nullptr && !StarOwn->bHome;
					// Where the point stands from this frame on: its exact place, or (owned by identity, taken on its star at
					// once below) its star's sky place; the same predicate and disc the per-frame crossfade target uses there.
					const FVector TakePlace = bAtExact && bStarOwnByIdentity ? StarOwn->StarInSky : Star.WorldLocation;
					const float InitialFade = bAtExact ? ViewFadeTarget(StarOwn, TakePlace, Eyes, FadeStart, FadeEnd) : 1.0f;
					if (TakeApproachPoint(World, *Galaxy, LocalToWorld, CameraLocal, PilotLocal, Star, TakeLocal, PixelTangent, bAtExact,
						InitialFade, WorldNow))
					{
						if (bAtExact && bStarOwnByIdentity)
						{
							// Its star stands off the catalogue place (a registry skew): the point starts on that star.
							FollowOwnStar(GLayer.Approach.Last(), StarOwn, true, LocalToWorld, Scale, Now, true);
						}
						PromoteIfCourse(World, GLayer.Approach.Last(), PilotLocal, Scale);
					}
				}
			}
		}

		// Every frame: place each point, crossfade it into its sphere, hand it back past the take radius (on its twin again).
		// Rio 06.10 (stage B): kept while its system stands (change 7); course and retired points released past their
		// course radius; a hand-back's end; the fade snaps while the eyes are detached and on a flip (change 12); the map
		// hold (aps.Stars.ApproachMapGain) and the glare (aps.Stars.SystemGlare) follow the level sets.
		const bool bKeepWhileStanding = CVarApproachKeepWhileStanding.GetValueOnGameThread() != 0;
		const double HandBackSeconds = FMath::Max(static_cast<double>(CVarApproachHandBackSeconds.GetValueOnGameThread()), 0.0);
		const bool bSnapFade = Eyes.bDetached || bEyesFlipped;
		const float Headroom = FMath::Max(CVarApproachFadeHeadroom.GetValueOnGameThread(), 0.0f);
		const float GainTarget = MapGainTarget();
		const float WorldVisibility = GVisibility >= 0.0f ? GVisibility : 1.0f;
		const bool bLog = Now >= GLayer.NextApproachLogSeconds;
		int64 Nearest = INDEX_NONE;
		double NearestCm = TNumericLimits<double>::Max();
		double NearestPilotCm = 0.0;
		struct FSample
		{
			FVector TwinDrawn = FVector::ZeroVector;
			double Glide = 1.0;
			double DiscPixels = 0.0;
			bool bSphere = false;
		} NearestSample;
		for (int32 Index = GLayer.Approach.Num() - 1; Index >= 0; --Index)
		{
			FApproachPoint& Point = GLayer.Approach[Index];
			const double PilotCm = FVector::Dist(PilotLocal, Point.ExactLocal) * Scale;
			Point.LastPilotCm = PilotCm;
			const double DistanceCm = Eyes.bDetached ? FVector::Dist(CameraLocal, Point.ExactLocal) * Scale : PilotCm;
			// Rio 06.10 (aps.Stars.ApproachOwnByIdentity): its own system by its catalogue place or by identity; a point owned
			// by identity only follows its standing star (FollowOwnStar). Off: by its place only, at the catalogue place.
			bool bOwnByIdentity = false;
			const FStandingSystem* IdentityOwn = bIdentity && bGathered ? OwnOf(Systems, LocalToWorld.TransformPosition(Point.CatalogueLocal),
				Point.CatalogIndex, Active, true, &bOwnByIdentity) : nullptr;
			// Rio 06.10 (stage B, change 7): its system stands: by identity (O(1)) or by place (gathered), never the home.
			const FStandingSystem* StandingOwn = bIdentity ? IdentityOwn
				: bGathered ? OwnSystemOf(Systems, LocalToWorld.TransformPosition(Point.ExactLocal)) : nullptr;
			Point.bOwnStanding = (bIdentity && Active.CatalogIndex != INDEX_NONE && Active.CatalogIndex == Point.CatalogIndex)
				|| (StandingOwn != nullptr && !StandingOwn->bHome);
			const double ReleaseCm = FMath::Max(TakeCm, Point.CourseTakeCm) * ApproachReleaseFactor;
			const bool bKeep = bKeepWhileStanding && Point.bOwnStanding;
			Point.bKeptNow = bKeep && PilotCm > ReleaseCm;
			if (Point.bRelease)
			{
				ReleaseApproachPoints(Index, TEXT("inside another system's sphere"), PilotCm);
				continue;
			}
			if (Point.HandBackStart >= 0.0 && HandBackAlpha(Point, WorldNow) >= 1.0)
			{
				if (!Point.bCourse || PilotCm > ReleaseCm)
				{
					const FString Reason = FString::Printf(TEXT("handed back: %s"), Point.HandBackReason ? Point.HandBackReason : TEXT("pool"));
					ReleaseApproachPoints(Index, *Reason, PilotCm);
					continue;
				}
				// Rio 06.10 (review): the course again while it handed back (PromoteCoursePoint keeps a running hand-back, so
				// nothing moves): it stays, on its twin now (glide 1), and glides on from here as a far take does (the release
				// and a take next frame would blink its twin in and the point out for a frame).
				Point.GlideFromLocal = FMath::Max(FMath::Min(PilotCm, FMath::Max(Point.CourseTakeCm, TakeCm)) / Scale, 1.0e-9);
				Point.HandBackStart = -1.0;
				Point.HandBackFromGlide = 1.0;
				Point.HandBackReason = nullptr;
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld stays: the course again at the end of its hand-back (%.6g ly)"),
					Point.CatalogIndex, PilotCm / LightYearCm);
			}
			if (PilotCm > ReleaseCm && !bKeep && Point.HandBackStart < 0.0)
			{
				const TCHAR* Reason = Point.bKeptLogged ? TEXT("its system went")
					: Point.CourseTakeCm > 0.0 ? TEXT("past its course radius") : TEXT("past the take radius");
				if (Point.CourseTakeCm > 0.0 && HandBackSeconds > 0.0 && GlideOf(Point, PilotLocal, WorldNow) < 0.999)
				{
					StartHandBack(Point, Reason, WorldNow, PilotLocal); // Defensive: a course or retired point not on its twin yet.
				}
				else
				{
					ReleaseApproachPoints(Index, Reason, PilotCm);
					continue;
				}
			}
			if (Point.bKeptNow && !Point.bKeptLogged)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld kept past its take radius: its own system stands (%.6g ly out)"),
					Point.CatalogIndex, PilotCm / LightYearCm);
				Point.bKeptLogged = true;
			}
			if (bIdentity)
			{
				FollowOwnStar(Point, IdentityOwn, bOwnByIdentity, LocalToWorld, Scale, Now, false);
			}
			else if (Point.Anchor != EApproachAnchor::Catalogue)
			{
				Point.ExactLocal = Point.CatalogueLocal;
				Point.Anchor = EApproachAnchor::Catalogue;
			}
			FSample Sample;
			Sample.Glide = GlideOf(Point, PilotLocal, WorldNow);
			Point.DrawLocal = PlaceApproachPoint(Point, CameraLocal, Sample.Glide, Scale, Sample.TwinDrawn);
			const FTransform Transform = FTransform(FQuat::Identity, Point.DrawLocal) * LocalToWorld;
			if (!Transform.Equals(Point.PushedTransform, 0.0))
			{
				APSStarRenderer::SetTransform(Point.Handle, Transform);
				Point.PushedTransform = Transform;
			}
			// Its sphere: a materialized star drawn at its sky place (no travel owed), or a near photosphere.
			const FVector ExactInSky = LocalToWorld.TransformPosition(Point.ExactLocal);
			const FStandingSystem* Own = bIdentity ? IdentityOwn : bGathered ? OwnSystemOf(Systems, ExactInSky) : nullptr;
			// (Owned by identity: a sphere once the re-anchor has the point on its star, IsOwnSphere.)
			const bool bOwnSphere = IsOwnSphere(Own, ExactInSky);
			const bool bNearSphere = GNearPhotosphereWorld.Get() == World && GNearPhotospheres.Contains(Point.CatalogIndex);
			Sample.bSphere = bOwnSphere || bNearSphere;
			Sample.DiscPixels = (bOwnSphere ? Own->StarRadiusCm : Point.RadiusCm) / FMath::Max(DistanceCm * PixelTangent, 1.0e-30);
			// The point while the disc is smaller than the far glyph's threshold (B7), the sphere from there on: a crossfade
			// over the last stretch of disc growth, never faster than ApproachFadeRate (snapped while the eyes are detached).
			const float Target = Sample.bSphere ? static_cast<float>(1.0 - FMath::SmoothStep(FadeStart, FadeEnd, Sample.DiscPixels)) : 1.0f;
			Point.Fade = bSnapFade ? FMath::Clamp(Target, 0.0f, 1.0f)
				: FMath::Clamp(FMath::FInterpConstantTo(Point.Fade, Target, DeltaSeconds, ApproachFadeRate), 0.0f, 1.0f);
			// Rio 06.10 (aps.Stars.SystemGlare): its base value is its twin's level value (or the own star's glare).
			const float BaseVisibility = ApproachBaseVisibility(Point);
			float Visibility = BaseVisibility * Point.Fade;
			if (Headroom > 0.0f && Point.Fade > 0.0f && Point.Fade < 1.0f && GLayer.PointDescs.IsValidIndex(Point.TwinSet))
			{
				// Rio 06.10 (stage B, change 10, aps.Stars.ApproachFadeHeadroom): a clamp-saturated point is capped near the clamp
				// while it crossfades, so the fade dims what is seen.
				const APSStarRenderer::FPointSetDesc& HeadroomDesc = GLayer.PointDescs[Point.TwinSet];
				const double HeadroomDistance = FMath::Max(DistanceCm / Scale, static_cast<double>(HeadroomDesc.BrightnessFloorDistanceLocal));
				const float HeadroomScale = (GLayer.PointIntensityScale > 0.0f ? GLayer.PointIntensityScale : HeadroomDesc.IntensityScale)
					* Point.AppliedGain;
				const double PreFull = APSStarRenderer::DecodeIntensity(APSStarRenderer::EncodeIntensity(Point.Intensity)) * HeadroomScale
					* BaseVisibility * WorldVisibility * ConsoleFloat(PointIntensityVariable, TEXT("aps.Stars.GpuPointIntensity"), 1.0f)
					/ FMath::Max(FMath::Square(PixelTangent) * FMath::Square(HeadroomDistance), 1.0e-300);
				if (PreFull > 0.0)
				{
					Visibility *= static_cast<float>(FMath::Min(1.0, Headroom
						* ConsoleFloat(MaxPixelVariable, TEXT("aps.Stars.GpuPointMaxPixel"), 16.0f) / PreFull));
				}
			}
			const bool bGlareMoved = Point.GlareSerial != GSystemGlareSerial;
			Point.GlareSerial = GSystemGlareSerial;
			if (Visibility != Point.AppliedVisibility
				&& (bGlareMoved || FMath::Abs(Visibility - Point.AppliedVisibility) > 0.002f || Point.Fade <= 0.0f || Point.Fade >= 1.0f))
			{
				APSStarRenderer::SetVisibility(Point.Handle, Visibility);
				Point.AppliedVisibility = Visibility;
			}
			// Rio 06.10 (aps.Stars.ApproachMapGain): the strategic map's hold like its neighbours' (1 again off the map).
			if (Point.AppliedGain != GainTarget && GLayer.PointDescs.IsValidIndex(Point.TwinSet))
			{
				Point.AppliedGain = GainTarget;
				APSStarRenderer::UpdatePointSet(Point.Handle, MakeApproachDesc(Point, GLayer.PointDescs[Point.TwinSet], GainTarget));
			}
			const EApproachLook Look = Point.Fade >= 1.0f ? EApproachLook::Point
				: Point.Fade <= 0.0f ? EApproachLook::Sphere : EApproachLook::Crossfade;
			if (Look != Point.Look)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach point %lld at %.4g AU: %s -> %s (disc %.3f px, %s)"),
					Point.CatalogIndex, DistanceCm / AstronomicalUnitCm, LookName(Point.Look), LookName(Look), Sample.DiscPixels,
					bOwnSphere ? TEXT("its materialized star") : bNearSphere ? TEXT("its near photosphere") : TEXT("no sphere"));
				if (Look == EApproachLook::Crossfade)
				{
					ArmExposureProbe(TEXT("crossfade"));
				}
				Point.Look = Look;
			}
			if (DistanceCm < NearestCm)
			{
				NearestCm = DistanceCm;
				NearestPilotCm = PilotCm;
				Nearest = Point.CatalogIndex;
				NearestSample = Sample;
			}
		}
		// Rio 06.10 (stage B): the retired pool; the map hold's flip; the eyes' flip.
		EnforceRetiredPool(PilotLocal, Scale, TakeCm, WorldNow);
		const bool bGainOn = GainTarget > 1.0f;
		if (bGainOn != GLayer.bApproachGainOn && GLayer.Approach.Num() > 0)
		{
			GLayer.bApproachGainOn = bGainOn;
			if (bGainOn)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach points take the map hold x%.2f (%d point(s))"), GainTarget, GLayer.Approach.Num());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach points back to their own scale"));
			}
		}
		if (bEyesFlipped)
		{
			GLayer.bLastDetached = Eyes.bDetached;
			if (bTrace)
			{
				TraceEvent(Eyes.bDetached ? TEXT("eyes-pilot") : TEXT("eyes-view"));
			}
		}

		// Rio 05.10 night: the nearest approach point twice a second, so a hand-over can be checked from the log alone.
		// (Found by its star: a release in the loop above moves the ones after it.)
		const FApproachPoint* NearestPoint = Nearest != INDEX_NONE ? GLayer.Approach.FindByPredicate(
			[Nearest](const FApproachPoint& Point) { return Point.CatalogIndex == Nearest; }) : nullptr;
		if (bLog && NearestPoint)
		{
			GLayer.NextApproachLogSeconds = Now + ApproachLogSeconds;
			const FApproachPoint& Point = *NearestPoint;
			const APSStarRenderer::FPointSetDesc* LevelDesc = GLayer.PointDescs.IsValidIndex(Point.TwinSet)
				? &GLayer.PointDescs[Point.TwinSet] : nullptr;
			const double DistanceLocal = FMath::Max(FVector::Dist(CameraLocal, Point.ExactLocal),
				LevelDesc ? static_cast<double>(LevelDesc->BrightnessFloorDistanceLocal) : 0.0);
			// The raster shader's value before its clamp (APSStarPoints.usf), still to be multiplied by the view's pre-exposure.
			// Rio 06.10: the approach point's own scale (TakeApproachPoint), never the level's strategic-map hold by itself
			// (stage B: times the hold it carries, aps.Stars.ApproachMapGain).
			const float ApproachScale = (!LevelDesc ? 0.0f
				: GLayer.PointIntensityScale > 0.0f ? GLayer.PointIntensityScale : LevelDesc->IntensityScale) * Point.AppliedGain;
			const double PreClamp = APSStarRenderer::DecodeIntensity(APSStarRenderer::EncodeIntensity(Point.Intensity))
				* ApproachScale * Point.AppliedVisibility * WorldVisibility
				* ConsoleFloat(PointIntensityVariable, TEXT("aps.Stars.GpuPointIntensity"), 1.0f)
				/ FMath::Max(FMath::Square(PixelTangent) * FMath::Square(DistanceLocal), 1.0e-300);
			// Rio 06.10 (star approach v2): the registry's place of this star (Info.Location: its anchor, the HUD's COURSE marker,
			// the autopilot target, the materializer's spawn) against the exact place and the drawn point; 0 AU once the sky and
			// the registry are one frame.
			const FString RegistryOff = RegistryOffText(World, Point, LocalToWorld, Camera, PixelTangent);
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Stars] approach %lld (ordinal %d, L%d, %d point(s)): exact %.4g ly = %.4g AU, twin %.4g ly (%.2f px off the ")
				TEXT("exact direction), drawn %.3f px off it, glide %.4f | pre-clamp %.4g x pre-exposure (clamp %.3g) | disc %.3f px ")
				TEXT("(crossfade %.2f-%.2f px), point %.3f, %s, sphere %s, glare %.4f%s%s"),
				Point.CatalogIndex, Point.Ordinal, Point.Level, GLayer.Approach.Num(), NearestCm / LightYearCm,
				NearestCm / AstronomicalUnitCm, FVector::Dist(NearestSample.TwinDrawn, CameraLocal) * Scale / LightYearCm,
				PixelsApart(NearestSample.TwinDrawn - CameraLocal, Point.ExactLocal - CameraLocal, PixelTangent),
				PixelsApart(Point.DrawLocal - CameraLocal, Point.ExactLocal - CameraLocal, PixelTangent), NearestSample.Glide,
				PreClamp, ConsoleFloat(MaxPixelVariable, TEXT("aps.Stars.GpuPointMaxPixel"), 16.0f), NearestSample.DiscPixels,
				FadeStart, FadeEnd, Point.Fade, LookName(Point.Look), NearestSample.bSphere ? TEXT("yes") : TEXT("no"),
				ApproachBaseVisibility(Point), *RegistryOff,
				Eyes.bDetached ? *FString::Printf(TEXT(" | pilot %.6g ly"), NearestPilotCm / LightYearCm) : TEXT(""));
		}
		// Rio 06.10 (aps.Stars.ApproachTrace): one line a frame for the traced star, with its events (after the points moved).
		if (bTrace)
		{
			TraceApproach(World, Eyes, LocalToWorld, bGathered ? &Systems : nullptr);
		}
	}

	int64 GetCourseStar(const UWorld* World, ECourseSource* OutSource)
	{
		const bool bCourse = World != nullptr && CVarApproachCourseMaxLy.GetValueOnGameThread() > 0.0f && GCourse.World.Get() == World;
		if (OutSource)
		{
			*OutSource = bCourse ? GCourse.CurrentSource : ECourseSource::None;
		}
		return bCourse ? GCourse.Current : INDEX_NONE;
	}

	bool DescribeGpuStar(const UWorld* World, const int64 CatalogIndex, FGpuStarInfo& Out)
	{
		Out = FGpuStarInfo();
		const AGalaxy* Galaxy = GLayer.Galaxy.Get();
		FTransform LocalToWorld;
		if (World == nullptr || GLayer.Mode != ELayerMode::Gameplay || !IsValid(Galaxy) || Galaxy->GetWorld() != World
			|| GetIndexedGalaxy(World) != Galaxy || !(GLayer.LastPilotPixelTangent > 0.0) || !MakeGameplayLocalToWorld(*Galaxy, LocalToWorld))
		{
			return false;
		}
		FTwinInfo Twin;
		const bool bResolved = ResolveTwinOf(*Galaxy, CatalogIndex, Twin);
		if (Twin.Record.CatalogIndex == INDEX_NONE)
		{
			return false;
		}
		const double Scale = LocalToWorld.GetMaximumAxisScale();
		Out.ExactWorld = LocalToWorld.TransformPosition(Twin.Record.GalaxyLocalLocation);
		if (!bResolved)
		{
			return true; // No set for its level: not drawn by the GPU layer.
		}
		int32 Ordinal = INDEX_NONE;
		const bool bDrawnByGpu = GpuOrdinalOf(World, CatalogIndex, Twin.Record.GalaxyLocalLocation, LocalToWorld, Ordinal);
		Out.bGpu = bDrawnByGpu && Twin.bLit;
		Out.Ordinal = Ordinal;
		Out.Level = Twin.Level;
		Out.TwinOffsetCm = Twin.OffsetLocal * Scale;
		// While aps.Stars.ApproachCourseMaxLy is 0 the cap is 500 ly, so an OFF run still reports the radius.
		const double CapLy = CVarApproachCourseMaxLy.GetValueOnGameThread() > 0.0f
			? static_cast<double>(CVarApproachCourseMaxLy.GetValueOnGameThread()) : 500.0;
		Out.CourseTakeCm = CourseTakeRadiusCm(Out.TwinOffsetCm, GLayer.LastPilotPixelTangent, CapLy, &Out.CourseTakeRawCm);
		return true;
	}
}
