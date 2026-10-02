#include "APSStellarVisualSubsystem.h"

#include "APSGameplayStellarProjection.h"
#include "APSGameplayStarAppearance.h"
#include "APSFarStarGlyphs.h"
#include "APSStellarViewOptics.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
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
#include "GameFramework/PlayerController.h"
#include "Async/ParallelFor.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(APSGameplayStars, true);

void UAPSStellarVisualSubsystem::ResetGameplayStellarView()
{
	APSFarStarGlyphs::Reset(GetWorld());
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
	TAutoConsoleVariable<int32> CVarRayRule(TEXT("aps.Stars.RayRule"), 1,
		TEXT("Rays on the catalogue stars in flight: 0 a stable share of the bright ones (accepted), 1 every bright enough ")
		TEXT("star, 2 none (comparison for Rio, 01.10)."));
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
	 * Sets the points' daylight brightness on their gameplay material (APSGameplayStarAppearance); false when the
	 * material has no such term (not regenerated yet), and the caller switches the points off in a day sky instead.
	 */
	bool ApplyPointVisibility(UMaterialInterface* Material, const float DayFactor, const AActor* Owner)
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
		const float Visibility = PointVisibility(DayFactor);
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
			}
			const FVector FromCentre = CameraLocation - Body->GetActorLocation();
			const double Altitude = FromCentre.Size() - RadiusCm;
			if (Altitude >= AtmosphereCm)
			{
				continue;
			}
			const double SunSine = FVector::DotProduct(FromCentre.GetSafeNormal(),
				(TargetStarLocation - CameraLocation).GetSafeNormal());
			const double Height = FMath::Clamp(FMath::Max(Altitude, 0.0) / (0.95 * AtmosphereCm), 0.0, 1.0);
			// A thin sky hides fewer stars by day; an airless one none (Rio, 01.10: dark starless skies on weak air).
			const double Masking = APSAtmosphereModel::DaySkyMasking(APSAtmosphereModel::Density(Body));
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
		const FVector HomeLocation = Home->GetActorLocation();
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
		int32 Width = 0, Height = 0;
		Controller->GetViewportSize(Width, Height);
		const double PixelTangent = APSStellarViewOptics::PixelTangent(Controller,
			2.0 * FMath::Tan(FMath::DegreesToRadians(
				Controller->PlayerCameraManager->GetFOVAngle() * 0.5)) / FMath::Max(Width, 320));
		const FVector ObserverFromHome = Camera - HomeLocation;
		TArray<AActor*> Attached;
		Generator->GetAttachedActors(Attached, true, true);
		uint32 TopologyHash = 0;
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
		const bool bGeometryChanged = bNewBuild
			|| TopologyHash != GameplayNativeTopologyHash
			|| GameplayNativeMutationSerial != Descriptor.TransformMutationSerial;
		if (bGeometryChanged)
		{
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
		const bool bUpdatePointSizes = bGeometryChanged
			|| !APSGameplayStellarProjection::CanReuseOptics(LastStellarPixelTangent, PixelTangent);
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
		const FQuat ViewRotation = Rotation.Quaternion();
		const double TanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(
			Controller->PlayerCameraManager->GetFOVAngle() * 0.5));
		const double TanHalfVertical = TanHalfHorizontal * FMath::Max(Height, 1) / FMath::Max(Width, 1);
		// Selection is view-dependent even when distance/FOV allow point-size reuse.
		// Refresh before crossing the 32px admission guard, without reuploading HISM.
		const bool bDemandTurned = GameplayNativeDemandRotation.AngularDistance(ViewRotation) > PixelTangent * 8.0
			|| !FMath::IsNearlyEqual(GameplayNativeTanHalfHorizontal, TanHalfHorizontal, 1.0e-6)
			|| !FMath::IsNearlyEqual(GameplayNativeTanHalfVertical, TanHalfVertical, 1.0e-6);
		// A turn changes which stars are on screen, not how large they look: only travel (or new geometry/optics)
		// rebuilds the short list of stars bright enough to matter, and a turn re-selects among those. Every mouse
		// turn of 8 px used to walk all 61k points (3-9 ms, the stutter when looking around; 29.09). Travel
		// re-measures the candidates at the 2% step and walks the whole catalogue only at the 25% step, before which
		// no point outside the list can reach admission (CanReuseDemandCandidates; 30.09).
		const bool bFullDemand = bUpdatePointSizes || !bGameplayNativeDemandCandidatesValid
			|| !APSGameplayStellarProjection::CanReuseDemandCandidates(
				FVector::Distance(ObserverFromHome, GameplayNativeDemandObserver),
				FMath::Min(GameplayNativeDemandClosestCm, ClosestNowCm));
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
				const bool bFades = APSGameplayStellarDay::ApplyPointVisibility(Source->GetMaterial(0),
					GameplayDaylightFactor, Source->GetOwner());
				// A material without the fade (not regenerated yet) still switches the points off in a day sky.
				const bool bHidden = !bFades && bGameplayDaylightStarsHidden;
				Source->SetVisibility(!bHidden, false);
				Source->SetHiddenInGame(bHidden, false);
			}
			if (!IsValid(Source) || !BaseTransforms || !IsValid(Source->GetStaticMesh())) continue;
			FAPSGameplayStellarResizePass& Pass = GameplayNativeResizePasses.FindOrAdd(Source);
			const bool bResizeTravelled = !bUpdatePointSizes && !bResizePassTaken
				&& FVector::Distance(ObserverFromHome, Pass.Observer) > FMath::Max(Pass.SlackCm, 1.0)
				&& NowSeconds - Pass.Seconds >= FMath::Max(APSGameplayStellarDay::CVarResizeInterval.GetValueOnGameThread(), 0.0f);
			if (!bRefreshDemand && !bResizeTravelled) continue;
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
					bool bChanged = false;
					double SlackCm = TNumericLimits<double>::Max();
					double NearestCm = TNumericLimits<double>::Max();
					for (int32 Index = 0; Index < SizedDistances.Num(); ++Index)
					{
						if (SizedDistances[Index] <= 0.0 || !BaseTransforms->IsValidIndex(Index)) continue;
						const double DistanceCm = FVector::Distance((*BaseTransforms)[Index].GetLocation(), LocalCamera)
							* ComponentScale;
						NearestCm = FMath::Min(NearestCm, DistanceCm);
						if (!APSGameplayStellarProjection::CanReusePointSize(DistanceCm, SizedDistances[Index]))
						{
							FTransform Transform;
							if (Source->GetInstanceTransform(Index, Transform, false)
								&& SizePoint(Index, Transform, false, SizedDistances[Index]))
							{
								Source->UpdateInstanceTransform(Index, Transform, false, false, true);
								bChanged = true;
							}
						}
						if (SizedDistances[Index] > 0.0)
						{
							SlackCm = FMath::Min(SlackCm,
								APSGameplayStellarProjection::PointSizeSlackCm(DistanceCm, SizedDistances[Index]));
						}
					}
					// The next pass waits until the observer has travelled the smallest slack found now.
					Pass.Observer = ObserverFromHome;
					Pass.SlackCm = SlackCm < TNumericLimits<double>::Max() ? FMath::Max(SlackCm, 0.0) : 0.0;
					Pass.NearestCm = NearestCm;
					Pass.Seconds = NowSeconds;
					if (bChanged)
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
		// B7: a materialized star smaller than its glyph (the home sun from its planets) keeps its catalogue glyph.
		APSFarStarGlyphs::Update(GetWorld(), Attached, Camera, PixelTangent, bGameplayDaylightStarsHidden);

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
	const FVector ObserverFromHome = Camera - HomeLocation;
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
