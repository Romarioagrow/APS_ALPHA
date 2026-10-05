// Rio 03.10 (galaxy phase 3): the galaxy's GPU star layer and glow (Plugins/APSStarRenderer).
// The catalogue stays the only source of truth: the layer is ordinals [ISM prefix, prefix + N) of the same nested order,
// resolved with APSGalaxyCatalogBatch::ResolveStars, packed to 8 bytes and drawn by the plugin; the glow carries the rest
// of the population. Two presentations: the menu's continuous preview (far envelope around the preview observer) and the
// gameplay sky (the full-scale 3D hierarchy of UAPSStellarVisualSubsystem, the galaxy HISM component's own frame).
// Everything is inert while aps.Stars.GpuPoints / aps.Stars.GalaxyGlow are 0, the gameplay sky also with aps.Stars.GameplayGpu 0.
#include "APSGalaxyGpuStars.h"

#include "APSGameplayStarAppearance.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSContinuousPreviewFrame.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
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
#include "UObject/Package.h"

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
			if (GMenuGlowProgress >= 1.0f)
			{
				return To;
			}
			const float From = FMath::Clamp(bPoints ? GMenuPointFrom : GMenuGlowFrom, 1.0e-3f, 1.0f);
			return FMath::Exp(FMath::Lerp(FMath::Loge(From), FMath::Loge(FMath::Max(To, 1.0e-3f)), GMenuGlowProgress));
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
			if (GMenuGlowProgress >= 1.0f)
			{
				return To;
			}
			const float From = FMath::Clamp(GMenuMinPixelFrom, 1.0f, 1000.0f);
			return FMath::Exp(FMath::Lerp(FMath::Loge(From), FMath::Loge(To), GMenuGlowProgress));
		}
		/** The gameplay layer's nearest-star index and its galaxy (one at a time, like the layer). */
		TSharedPtr<const FStarIndex, ESPMode::ThreadSafe> GIndex;
		TWeakObjectPtr<AGalaxy> GIndexGalaxy;
		int32 GActiveLayers = 0;
		bool GDelegatesBound = false;
		bool GShiftBound = false;
		bool GLoggedShadersOff = false;
		FRequest GLastMenuRequest;
		FRequest GLastGameplayRequest;
		TWeakObjectPtr<const UWorld> GVisibilityWorld;
		float GVisibility = -1.0f;

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
			TArray<TPair<double, FVector4f>> Found;
			for (TActorIterator<AStarSystem> It(World); It; ++It)
			{
				const AStarSystem* System = *It;
				if (!IsValid(System) || !(System->StarSystemRadius > 0.0) || !FMath::IsFinite(System->StarSystemRadius))
				{
					continue;
				}
				const FVector Center = LocalToWorld.InverseTransformPosition(System->GetActorLocation());
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
			int32 PointCount = 0;
			const bool bMenuFade = Mode == ELayerMode::Menu && CVarMenuFadeInSeconds.GetValueOnGameThread() > 0.0f;
			const float StartVisibility = Mode == ELayerMode::Menu ? (bMenuFade ? 0.0f : MenuPointShare()) : 1.0f;
			const float StartGlowVisibility = Mode == ELayerMode::Menu ? (bMenuFade ? 0.0f : MenuGlowShare()) : 1.0f;
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
				}
			}
			GLayer.PointCount = GLayer.PointSets.Num() > 0 ? PointCount : 0;
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
				GLayer.PushedTransform = LocalToWorld;
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
				APSStarRenderer::FPointSetDesc& Desc = GLayer.PointDescs[Index];
				ApplyGameplayExclusions(GLayer.Exclusions, Desc);
				APSStarRenderer::UpdatePointSet(GLayer.PointSets[Index], Desc);
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
		if (Scope != GMenuGlowScope)
		{
			const bool bMenuLayer = GLayer.Mode == ELayerMode::Menu;
			GMenuGlowFrom = bMenuLayer ? GLayer.GlowFocus : ScopeShare(GMenuGlowScope, false);
			GMenuPointFrom = bMenuLayer ? GLayer.PointFocus : ScopeShare(GMenuGlowScope, true);
			GMenuMinPixelFrom = bMenuLayer ? GLayer.MinPixelFocus : ScopeMinPixel(GMenuGlowScope);
			GMenuGlowScope = Scope;
		}
		// The camera's own easing (FAPSContinuousPreviewOrbit::Interpolate): smoothstep of the flight's progress.
		const float T = FMath::Clamp(FlightAlpha, 0.0f, 1.0f);
		GMenuGlowProgress = T * T * (3.0f - 2.0f * T);
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
			GLayer.PointSets.Reset();
			GLayer.PointDescs.Reset();
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
			Spec.HomeLocal = FVector3f(LocalToWorld.InverseTransformPosition(Home->GetActorLocation()));
			Spec.Levels = GameplayLevels;
			Spec.bIndex = true;
			LogGameplayMappingCheck(Galaxy,
				Galaxy.StarMeshInstances->GetComponentTransform().InverseTransformPosition(Home->GetActorLocation()));
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
}
