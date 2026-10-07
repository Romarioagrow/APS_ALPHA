#include "APSFarStarGlyphs.h"

#include "APSGalaxyGpuStars.h"
#include "APSGameplayStarAppearance.h"
#include "APSStellarOpticalSupport.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/Package.h"

namespace APSFarStarGlyphsLocal
{
	TAutoConsoleVariable<float> CVarGlyphPixels(TEXT("aps.Stars.FarGlyphPixels"), 2.2f,
		TEXT("A materialized star (the home sun, a visited system's) whose disc is smaller than this many pixels keeps its ")
		TEXT("catalogue glyph (core and rays) so it stays visible far away. 0: off."));
	// Rio 06.10 (star approach v2, change 13; a star's far stand-in must not switch back and forth): the glyph <-> sphere
	// switch latches by the disc alone. A shown glyph stays until the disc reaches aps.Stars.FarGlyphPixels; once the
	// sphere has it, the glyph comes back only below this share of it, so a star hovering at the threshold (about 1 AU from
	// an M dwarf, 1.7 AU from the home sun) no longer toggles. A new glyph decides at the plain threshold, as before.
	TAutoConsoleVariable<float> CVarGlyphHysteresis(TEXT("aps.Stars.FarGlyphHysteresis"), 0.85f,
		TEXT("Rio 06.10: the sphere -> far glyph switch happens below this share of aps.Stars.FarGlyphPixels (glyph -> sphere ")
		TEXT("at the full value), clamped to 0.5..1. 0 or 1: off, one threshold both ways (as before 06.10)."));
	// Rio 06.10 (star approach v2, change 14; one star, one place): a far glyph is re-placed at its star's sky place in the
	// same call that moves the sky, not a frame late in the stellar view's update.
	TAutoConsoleVariable<int32> CVarGlyphsFollowSky(TEXT("aps.Stars.GlyphsFollowSky"), 1,
		TEXT("Rio 06.10: the far glyphs stand at their star's sky place from the call that moves the sky on (every owed step, ")
		TEXT("a pay of the debt, a world shift). 0: off, placed by the stellar view's update only (as before 06.10)."));
	// Rio 06.10 (star approach v2, stage B, change 12 "pilot's eyes"; the F10 map round trip: sphere -> point -> 'past the
	// take radius', a far glyph somewhere else, a re-take with the glide pinned): the approach points decide from the
	// pilot (takes, releases, glide, speed, the course star's take radius) and draw for the view (fade, visibility, the
	// dot's place). The far glyphs below keep the view.
	TAutoConsoleVariable<int32> CVarApproachPilotView(TEXT("aps.Stars.ApproachPilotView"), 1,
		TEXT("Rio 06.10: 1 lets the approach points decide from the pilot's eyes (the pawn's view) while the F10 map is open or ")
		TEXT("the view stands farther than aps.Stars.ApproachPilotViewKm from the pilot: no take, release or glide by the map ")
		TEXT("camera; fades, visibility and the dot's place still from the view. 0: from the view alone (stage A)."));
	TAutoConsoleVariable<float> CVarApproachPilotViewKm(TEXT("aps.Stars.ApproachPilotViewKm"), 1000.0f,
		TEXT("Rio 06.10 (aps.Stars.ApproachPilotView): a view farther than this many km from the pilot's eyes is not the pilot's ")
		TEXT("(at least 1 km: a smaller value would detach the pilot's own camera every frame)."));
	// Rio 06.10 (star approach v2, stage B; one star, no blink: the glyph's hand-over to the sphere was a cut at
	// aps.Stars.FarGlyphPixels while an approach point crossfades over the band before it): a far glyph fades as the
	// approach point does, over the same stretch of disc growth.
	TAutoConsoleVariable<int32> CVarGlyphCrossfade(TEXT("aps.Stars.FarGlyphCrossfade"), 1,
		TEXT("Rio 06.10: 1 fades a far glyph (cluster stars, the home sun, the galaxy's ISM prefix) into its sphere while the ")
		TEXT("disc grows from aps.Stars.ApproachPointFade x aps.Stars.FarGlyphPixels to aps.Stars.FarGlyphPixels (its emission, ")
		TEXT("custom data 3), one threshold both ways. 0: the cut at the threshold with aps.Stars.FarGlyphHysteresis (stage A); ")
		TEXT("a faded glyph gets its full emission back."));
	// Rio 06.10 (stage B; one star, no blink): the star material reads custom data 3 through log2 into an activity
	// (APSFixStarHISMMaterialCommandlet: core energy lerp(4.8, 12.5, activity)), so an emission fade keeps about a third of
	// a bright glyph's light down to no emission and its hide at the end of the band stayed a cut. GameplayPointVisibility,
	// the term the stellar view dims every catalogue point with, multiplies the whole signal: linear, down to black.
	TAutoConsoleVariable<int32> CVarGlyphLinearFade(TEXT("aps.Stars.FarGlyphLinearFade"), 1,
		TEXT("Rio 06.10 (aps.Stars.FarGlyphCrossfade): 1 fades a far glyph through its own material's GameplayPointVisibility ")
		TEXT("(times the day value and aps.Stars.SystemGlare's, one value; linear, down to black). 0: through its emission ")
		TEXT("(custom data 3) on the shared catalogue material. A material without the term: the emission either way."));
	/**
	 * Rio 06.10 (aps.Stars.FarGlyphCrossfade): the glyph's crossfade step per second at most, the approach point's
	 * ApproachFadeRate (APSGalaxyGpuStars.cpp), so a glyph and an approach point hand over to the sphere in the same time.
	 */
	constexpr float GlyphFadeRate = 4.0f;

	struct FGlyph
	{
		TWeakObjectPtr<AStar> Star;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		int32 Index{INDEX_NONE};
		/** The carrier radius the instance was last sized at. */
		double Carrier{0.0};
		/** Rio 05.10 (real scale, stage 2): a galaxy star's own row of per-star data (empty: the catalogue point's, Index). */
		TArray<float> Row;
		/** Rio 05.10 night: a galaxy star's glyph logs its hand-over to the sphere (shown last frame, next periodic line). */
		bool bGalaxy{false};
		bool bShown{false};
		double NextLogSeconds{0.0};
		/**
		 * Rio 06.10 (aps.Stars.FarGlyphHysteresis): the latched glyph/sphere decision by the disc alone (daylight and a
		 * hidden star do not move it): -1 not decided yet, 1 the glyph's side, 0 the sphere's.
		 */
		int32 GlyphSide{-1};
		/**
		 * Rio 06.10 (aps.Stars.FarGlyphCrossfade): the share of its emission the instance has now (custom data 3 = the
		 * source's emission x this; 1 = the copied row as made by Create).
		 */
		float AppliedAlpha{1.0f};
		/**
		 * Rio 06.10 (aps.Stars.FarGlyphCrossfade): the crossfade share now, eased toward the disc's at GlyphFadeRate as the
		 * approach point's fade is (-1: none yet, it takes the disc's at once).
		 */
		float FadeAlpha{-1.0f};
		/**
		 * Rio 06.10 (aps.Stars.SystemGlare, aps.Stars.FarGlyphLinearFade): the glyph's own material (parent: the shared
		 * catalogue material) and the GameplayPointVisibility last sent to it (-1: none, the shared material).
		 */
		TWeakObjectPtr<UMaterialInstanceDynamic> GlareMaterial;
		float AppliedGlare{-1.0f};
		/**
		 * Rio 06.10 (review): the shared catalogue material the own one was made from (tried once per shared material, so a
		 * failed make is not retried every frame; a new shared material, a new build, makes it again).
		 */
		TWeakObjectPtr<UMaterialInterface> GlareSource;
	};
	TMap<TWeakObjectPtr<const UWorld>, TArray<FGlyph>> GGlyphs;

	/**
	 * Rio 06.10 (aps.Stars.SystemGlare): the stellar view's values for the next Update of a world (SetSystemGlare, each
	 * frame). The stars are only compared with the glyphs' own, never dereferenced.
	 */
	struct FGlyphGlare
	{
		bool bEnabled{false};
		float DayVisibility{1.0f};
		float Others{1.0f};
		TArray<TPair<const AStar*, float>, TInlineAllocator<8>> Own;
	};
	TMap<TWeakObjectPtr<const UWorld>, FGlyphGlare> GGlyphGlare;

	/** The star material's daylight term (APSFixStarHISMMaterialCommandlet; the stellar view sets it on the shared material). */
	const FName& GlyphVisibilityName()
	{
		static const FName GlyphVisibilityParameter(TEXT("GameplayPointVisibility"));
		return GlyphVisibilityParameter;
	}

	/** Rio 06.10: a visibility step worth sending, as the stellar view sends the shared one (relative 1%, the ends exactly). */
	bool ShouldSendGlyphVisibility(const float Applied, const float Wanted)
	{
		return Wanted != Applied && (Applied < 0.0f || Wanted <= 0.0f || Wanted >= 1.0f
			|| FMath::Abs(Wanted - Applied) > 0.01f * FMath::Max(Wanted, 0.0005f));
	}

	/**
	 * Rio 06.10 (aps.Stars.SystemGlare, aps.Stars.FarGlyphLinearFade): a glyph's own material, a copy of the shared catalogue
	 * material holding its own GameplayPointVisibility (a parameter change on the shared one is not relied on to reach it),
	 * set to Visibility and put on the mesh. Null if it could not be made (the mesh keeps what it has; not tried again until
	 * the shared material changes).
	 * Rio 06.10 (review): UE 5.4 takes only a Material or a MaterialInstanceConstant as a material instance's parent
	 * (UMaterialInstance::SetParentInternal warns and leaves it without one, so it draws the default surface material). The
	 * shared catalogue material is the stellar view's dynamic instance (APSGameplayStarAppearance): the copy is made on that
	 * instance's own parent with its overrides (the profile, the day value) copied over.
	 */
	UMaterialInstanceDynamic* InstallGlyphMaterial(FGlyph& Glyph, UInstancedStaticMeshComponent& Mesh,
		UMaterialInterface* SharedMaterial, const float Visibility)
	{
		Glyph.GlareSource = SharedMaterial;
		UMaterialInstanceDynamic* SharedDynamic = Cast<UMaterialInstanceDynamic>(SharedMaterial);
		UMaterialInterface* GlyphParent = SharedDynamic ? SharedDynamic->Parent.Get() : SharedMaterial;
		UMaterialInstanceDynamic* OwnGlyphMaterial = GlyphParent && !Cast<UMaterialInstanceDynamic>(GlyphParent)
			? UMaterialInstanceDynamic::Create(GlyphParent, &Mesh) : nullptr;
		if (!OwnGlyphMaterial || OwnGlyphMaterial->Parent != GlyphParent)
		{
			const AStar* FailedStar = Glyph.Star.Get();
			UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] far glyph for %s keeps the shared material: no own copy of %s (parent %s)"),
				FailedStar ? *FailedStar->AstroName.ToString() : TEXT("?"), *GetNameSafe(SharedMaterial), *GetNameSafe(GlyphParent));
			return nullptr;
		}
		OwnGlyphMaterial->SetFlags(RF_Transient);
		if (SharedDynamic)
		{
			OwnGlyphMaterial->CopyParameterOverrides(SharedDynamic);
		}
		OwnGlyphMaterial->SetScalarParameterValue(GlyphVisibilityName(), Visibility);
		Mesh.SetMaterial(0, OwnGlyphMaterial);
		Glyph.GlareMaterial = OwnGlyphMaterial;
		Glyph.AppliedGlare = Visibility;
		const AStar* Star = Glyph.Star.Get();
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s takes its own material (a copy of %s on %s), visibility %.4f"),
			Star ? *Star->AstroName.ToString() : TEXT("?"), *GetNameSafe(SharedMaterial), *GetNameSafe(GlyphParent), Visibility);
		return OwnGlyphMaterial;
	}

	/**
	 * Rio 06.10 (star approach v2, change 14, aps.Stars.GlyphsFollowSky): every glyph to its star's sky place now. Called
	 * when the sky moves (UAPSWorldOriginSubsystem::OnSkyOffsetChanged: every owed step, after the systems riding with the
	 * sky moved, and a pay of the debt) and after every world shift, so no drawn frame shows a glyph one owed step or a
	 * whole debt away from its star: a pay or a settle from the world-less floating origin ticker runs after the world's
	 * tick, when no stellar view update is left in the frame. The size follows in the next Update.
	 */
	void GlyphsFollowSkyNow(UWorld* World)
	{
		if (!World || CVarGlyphsFollowSky.GetValueOnGameThread() == 0)
		{
			return;
		}
		const TArray<FGlyph>* Glyphs = GGlyphs.Find(World);
		if (!Glyphs)
		{
			return;
		}
		for (const FGlyph& Glyph : *Glyphs)
		{
			const AStar* Star = Glyph.Star.Get();
			AActor* Actor = Glyph.Actor.Get();
			if (!IsValid(Star) || !IsValid(Actor))
			{
				continue;
			}
			const FVector StarInSky = UAPSWorldOriginSubsystem::SkyPlace(*Star);
			if (!Actor->GetActorLocation().Equals(StarInSky, 1.0))
			{
				Actor->SetActorLocation(StarInSky, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	/** Binds GlyphsFollowSkyNow once per process (both events are global; aps.Stars.GlyphsFollowSky is read per call). */
	bool GGlyphsFollowSkyBound = false;
	void EnsureGlyphsFollowSkyBound()
	{
		if (GGlyphsFollowSkyBound)
		{
			return;
		}
		GGlyphsFollowSkyBound = true;
		UAPSWorldOriginSubsystem::OnSkyOffsetChanged().AddLambda([](UWorld* World, const FVector&)
		{
			GlyphsFollowSkyNow(World);
		});
		APSWorldShiftEvents::BindPostShift(GetTransientPackage(), [](UWorld* World) { GlyphsFollowSkyNow(World); });
	}

	/**
	 * Rio 06.10 (star approach v2, trace): the approach trace's switch, aps.Stars.ApproachTrace (APSGalaxyGpuStars.cpp),
	 * found by name so this file needs no header of it; off without it.
	 */
	bool GlyphTraceOn()
	{
		static IConsoleVariable* const Trace = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ApproachTrace"), false);
		return Trace && Trace->GetInt() != 0;
	}

	/** The trace's shot burst for a glyph event, through the command the approach trace registers for it
	 * (aps.Stars.ApproachTraceBurst <event>, APSGalaxyGpuStars.cpp: it logs 'burst=<event> k=<n>' and queues the burst
	 * under aps.Stars.ApproachTraceShots; the event line itself, with star=, is this file's own); nothing without it. */
	void GlyphTraceBurst(UWorld& World, const TCHAR* Event)
	{
		static IConsoleObject* const Command = IConsoleManager::Get().FindConsoleObject(TEXT("aps.Stars.ApproachTraceBurst"), false);
		if (IConsoleCommand* Burst = Command ? Command->AsCommand() : nullptr)
		{
			TArray<FString> Args;
			Args.Add(Event);
			Burst->Execute(Args, &World, *GLog);
		}
	}

	/** One trace line per glyph and frame; its glyph <-> sphere switch as an event line with the trace's shot burst. */
	void TraceGlyphFrame(UWorld& World, const AStar& Star, const FVector& StarInSky, const bool bShow, const int32 PreviousSide,
		const int32 Side, const double DistanceCm, const double ApparentPixels, const double CorePixels, const double Threshold,
		const float Emission, const double Alpha, const bool bCrossfade, const float Glare)
	{
		const unsigned long long Frame = static_cast<unsigned long long>(GFrameCounter);
		const FString Name = Star.AstroName.ToString().Replace(TEXT(" "), TEXT("_"));
		FVector2D Screen(-1.0, -1.0);
		const APlayerController* Player = World.GetFirstPlayerController();
		if (!Player || !Player->ProjectWorldLocationToScreen(StarInSky, Screen, false))
		{
			Screen = FVector2D(-1.0, -1.0);
		}
		const double DistanceAu = DistanceCm / 1.495978707e13;
		// Rio 06.10 (stage B, aps.Stars.FarGlyphCrossfade): alpha = the share of its light the glyph draws with (1 off).
		// Rio 06.10 (aps.Stars.SystemGlare): gl = the glare value its own material draws with (1 without one).
		UE_LOG(LogTemp, Log, TEXT("[APS.StarTrace] f=%llu glyph star=%s shown=%d xy=%.1f,%.1f disc_px=%.6f core_px=%.3f emission=%.4g d_au=%.6g threshold_px=%.6f side=%s alpha=%.4f xfade=%d gl=%.4f"),
			Frame, *Name, bShow ? 1 : 0, Screen.X, Screen.Y, ApparentPixels, CorePixels, Emission, DistanceAu, Threshold,
			Side == 1 ? TEXT("glyph") : TEXT("sphere"), Alpha, bCrossfade ? 1 : 0, Glare);
		if (PreviousSide != -1 && PreviousSide != Side)
		{
			const TCHAR* Event = Side == 1 ? TEXT("sphere->glyph") : TEXT("glyph->sphere");
			UE_LOG(LogTemp, Log, TEXT("[APS.StarTrace] f=%llu event=%s star=%s disc_px=%.6f d_au=%.6g threshold_px=%.6f"),
				Frame, Event, *Name, ApparentPixels, DistanceAu, Threshold);
			GlyphTraceBurst(World, Event);
		}
	}

	/**
	 * Rio 05.10 (real scale, stage 2): the row a galaxy star's glyph draws with, made as the cluster's rows are made
	 * (AAstroGenerator::ComposeCanonicalStellarProjection): its colour, its light at the real distances, seed and gain.
	 */
	TArray<float> MakeGalaxyRow(const AStarCluster& Cluster, const FGalaxyCatalogStarRecord& Record, const int32 Stride)
	{
		TArray<float> Row;
		Row.Init(0.0f, FMath::Max(Stride, 0));
		if (Stride < APSStellarOpticalSupport::RequiredStride)
		{
			return Row;
		}
		const double RadiusSolar = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
			* FMath::Max(static_cast<double>(Record.RadiusScale), 0.0);
		const double Luminosity = APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
			* APSGalaxyMorphology::GetRadiusScaleLuminosity(Record.RadiusScale);
		const FAPSCanonicalStellarProjectionFrame& Frame = Cluster.CanonicalProjectionFrame;
		const double AppliedRadiusCm = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(
			EAPSCanonicalStellarProxyLayer::StarCluster, Frame, RadiusSolar);
		const double AppliedRadiusSolar = APSCanonicalStellarProjection::GetLegacyLayoutRadiusSolar(Frame,
			APSCanonicalStellarProjection::UnprojectPhysicalRadiusSolar(Frame, AppliedRadiusCm));
		const double Emission = UStarGenerator::GetFarStarVisualEmission(RadiusSolar,
			GetMutableDefault<UStarGenerator>()->CalculateEmission(static_cast<float>(Luminosity * 25.0)), AppliedRadiusSolar);
		const FLinearColor Color = UStarGenerator::GetStarColor(Record.SpectralClass, Record.SpectralSubclass);
		Row[0] = Color.R;
		Row[1] = Color.G;
		Row[2] = Color.B;
		Row[3] = static_cast<float>(Emission);
		Row[4] = static_cast<float>(Record.GenerationSeed & 0xffff) / 65535.0f;
		Row[6] = APSGameplayStarAppearance::GetLuminosityGain(Luminosity);
		Row[APSStellarOpticalSupport::CoreScaleIndex] = 1.0f;
		return Row;
	}

	/**
	 * Rio 05.10 (real scale, stage 2): a REAL SCALE galaxy star stands up as a system a quarter of a light year out, where
	 * its sphere is far below a pixel; its catalogue point (ISM prefix) is suppressed and its GPU point hidden by the
	 * system's sphere: its glyph stays, from its record.
	 * Rio 05.10 night: only the fallback now. A star the GPU layer draws keeps its approach point (APSGalaxyGpuStars::
	 * UpdateApproachPoints: the same photometry as before, the exact place, a crossfade into this sphere at the same disc
	 * size), so the glyph is made only for a star of the ISM prefix, without a GPU layer, or with aps.Stars.ApproachPoint 0.
	 */
	void AddGalaxyGlyph(UWorld& World, const AStarCluster& Cluster, const int32 Stride, TArray<FGlyph>& Glyphs,
		TSet<const AStar*>& Present)
	{
		// Rio 06.10 (audit: trace scopes, instrumentation only).
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_FarGalaxyGlyph);
		const FAPSStarSystems* Systems = APSStarSystemsFind(&World);
		const FAPSSystemMaterializer* Materializer = Systems ? Systems->GetMaterializer() : nullptr;
		const FAPSStarSystemInfo* Info = Materializer ? Systems->Get(Materializer->GetActiveIndex()) : nullptr;
		const AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(&World);
		FGalaxyCatalogStarRecord Record;
		if (!Info || Info->GalaxyIndex == INDEX_NONE || !Galaxy || !Galaxy->StarCatalog.ResolveStar(Info->GalaxyIndex, Record)
			|| APSGalaxyGpuStars::DrawsApproachPoint(&World, Info->GalaxyIndex))
		{
			return;
		}
		// The system's star stands at its catalogue point (APSSystemMaterializer's MaterializeGalaxyStar). Rio 06.10 (still
		// ship): the catalogue's places are the sky's; a star actor stands at its sky place (SkyPlace: a system riding with
		// the sky where it is, any other at its world place + the sky offset).
		AStar* Star = nullptr;
		double BestSquared = TNumericLimits<double>::Max();
		for (TActorIterator<AStar> It(&World); It; ++It)
		{
			const double DistanceSquared = FVector::DistSquared(UAPSWorldOriginSubsystem::SkyPlace(**It), Info->Location);
			if (IsValid(*It) && DistanceSquared < BestSquared)
			{
				BestSquared = DistanceSquared;
				Star = *It;
			}
		}
		if (!Star || BestSquared > FMath::Square(FMath::Max(Info->StarRadiusCm, 1.0e10) * 3.0))
		{
			return;
		}
		Present.Add(Star);
		if (!Glyphs.ContainsByPredicate([Star](const FGlyph& Glyph) { return Glyph.Star.Get() == Star; }))
		{
			FGlyph& Glyph = Glyphs.AddDefaulted_GetRef();
			Glyph.Star = Star;
			Glyph.Row = MakeGalaxyRow(Cluster, Record, Stride);
			Glyph.bGalaxy = true;
			// Rio 05.10 night (with aps.Stars.ApproachPoint): a star of the galaxy's ISM prefix keeps the row its own catalogue
			// point drew with (made in the galaxy's frame), not one made again in the cluster's: the same light at the hand-over.
			const UHierarchicalInstancedStaticMeshComponent* GalaxyPoints = Galaxy->StarMeshInstances;
			const int32 Instance = Galaxy->RenderedCatalogIndices.Find(Info->GalaxyIndex);
			if (APSGalaxyGpuStars::AreApproachPointsActive(&World) && IsValid(GalaxyPoints) && Instance != INDEX_NONE
				&& GalaxyPoints->NumCustomDataFloats == Stride
				&& GalaxyPoints->PerInstanceSMCustomData.IsValidIndex((Instance + 1) * Stride - 1))
			{
				Glyph.Row = TArray<float>(GalaxyPoints->PerInstanceSMCustomData.GetData() + Instance * Stride, Stride);
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s takes its ISM point's own row (instance %d)"),
					*Info->Name, Instance);
			}
		}
	}

	void Destroy(FGlyph& Glyph)
	{
		if (AActor* Actor = Glyph.Actor.Get())
		{
			Actor->Destroy();
		}
		Glyph.Actor.Reset();
		Glyph.Mesh.Reset();
	}

	/**
	 * A one-instance copy of the catalogue point: its mesh, its material and its row of per-star data. Rio 06.10
	 * (aps.Stars.SystemGlare, aps.Stars.FarGlyphLinearFade): OwnVisibility >= 0 gives it its own child of that material at
	 * this GameplayPointVisibility, before the render state; below 0 the shared material (as before).
	 */
	bool Create(UWorld& World, FGlyph& Glyph, const UHierarchicalInstancedStaticMeshComponent& Source, const AStar& Star,
		const float OwnVisibility)
	{
		// The stellar view widens the catalogue's data to the optics' layout first; until then there is nothing to copy.
		const int32 Stride = Source.NumCustomDataFloats;
		const int32 Address = Glyph.Index * Stride;
		// Rio 05.10 (real scale, stage 2): a galaxy star's glyph brings its own row (MakeGalaxyRow).
		const bool bOwnRow = !Glyph.Row.IsEmpty();
		if (Stride < APSStellarOpticalSupport::RequiredStride || (bOwnRow ? Glyph.Row.Num() != Stride
			: !Source.PerInstanceSMCustomData.IsValidIndex(Address + Stride - 1)))
		{
			return false;
		}
		FActorSpawnParameters Parameters;
		Parameters.ObjectFlags |= RF_Transient;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World.SpawnActor<AActor>(AActor::StaticClass(), FTransform(Star.GetActorLocation()), Parameters);
		if (!Actor)
		{
			return false;
		}
		UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(Actor, TEXT("FarStarGlyph"), RF_Transient);
		Mesh->SetMobility(EComponentMobility::Movable);
		// Set up as the catalogue points are (APSStarRenderStabilitySubsystem): Nanite cannot draw the additive stellar
		// point material in UE 5.4 (02.10 run: the glyph went to Nanite, was not drawn and warned nine times a frame),
		// no wind offset on a point, never culled by distance, the catalogue's LOD.
		Mesh->bDisallowNanite = true;
		Mesh->SetForceDisableNanite(true);
		Mesh->bEvaluateWorldPositionOffset = false;
		Mesh->bWorldPositionOffsetWritesVelocity = false;
		Mesh->bUseAsOccluder = false;
		Mesh->bNeverDistanceCull = true;
		Mesh->SetCullDistances(0, 0);
		Mesh->SetForcedLodModel(Source.ForcedLodModel);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->SetReceivesDecals(false);
		Mesh->SetStaticMesh(Source.GetStaticMesh());
		Glyph.GlareMaterial.Reset();
		Glyph.AppliedGlare = -1.0f;
		Glyph.GlareSource.Reset();
		if (OwnVisibility < 0.0f || !InstallGlyphMaterial(Glyph, *Mesh, Source.GetMaterial(0), OwnVisibility))
		{
			Mesh->SetMaterial(0, Source.GetMaterial(0));
		}
		Mesh->SetTranslucentSortPriority(Source.TranslucencySortPriority);
		// The instance and its data before the render state, so the proxy is made once, complete.
		Mesh->SetNumCustomDataFloats(Stride);
		Mesh->AddInstance(FTransform::Identity, false);
		Mesh->SetCustomData(0, bOwnRow ? TArrayView<const float>(Glyph.Row)
			: TArrayView<const float>(Source.PerInstanceSMCustomData.GetData() + Address, Stride), false);
		Actor->SetRootComponent(Mesh);
		Actor->AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
#if WITH_EDITOR
		Actor->SetActorLabel(TEXT("FarStarGlyph ") + Star.GetName());
#endif
		Glyph.Actor = Actor;
		Glyph.Mesh = Mesh;
		Glyph.Carrier = 0.0;
		// Rio 06.10 (aps.Stars.FarGlyphCrossfade): the copied row carries the full emission; the loop applies its share.
		Glyph.AppliedAlpha = 1.0f;
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s (%s), catalogue point %d"), *Star.AstroName.ToString(),
			*Star.GetName(), Glyph.Index);
		return true;
	}

	/** A glyph's full emission (custom data 3): its own row's, else its catalogue point's, read live; -1 without one. */
	float GlyphBaseEmission(const FGlyph& Glyph, const UHierarchicalInstancedStaticMeshComponent& Source)
	{
		if (Glyph.Row.IsValidIndex(3))
		{
			return Glyph.Row[3];
		}
		const int32 Address = Glyph.Index * Source.NumCustomDataFloats + 3;
		return Glyph.Index >= 0 && Source.PerInstanceSMCustomData.IsValidIndex(Address) ? Source.PerInstanceSMCustomData[Address] : -1.0f;
	}

	/**
	 * Rio 06.10 (aps.Stars.FarGlyphCrossfade): where a glyph's fade begins, as a share of aps.Stars.FarGlyphPixels: the
	 * approach point's own band (aps.Stars.ApproachPointFade, APSGalaxyGpuStars.cpp, found by name; 0.6 without it), clamped
	 * as the point clamps it, so a glyph and an approach point hand over to the sphere over the same stretch of disc.
	 */
	double GlyphFadeBandStart()
	{
		static IConsoleVariable* const Band = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Stars.ApproachPointFade"), false);
		return FMath::Clamp(Band ? static_cast<double>(Band->GetFloat()) : 0.6, 0.05, 1.0);
	}

	/** Rio 06.10 (aps.Stars.ApproachPilotView): the pilot's pixel tangent while the view was last the pilot's, per world. */
	struct FPilotEyesCache
	{
		TWeakObjectPtr<const UWorld> World;
		double PixelTangent = -1.0;
		bool bDetached = false;
	};
	FPilotEyesCache GPilotEyes;

	/**
	 * Rio 06.10 (star approach v2, stage B, change 12 "pilot's eyes"; aps.Stars.ApproachPilotView): who decides and who
	 * draws for the approach points this frame. The view is (Camera, PixelTangent), the stellar view's. It is not the
	 * pilot's while the F10 map is open (its camera holds the view) or while it stands farther than
	 * aps.Stars.ApproachPilotViewKm from the pawn's eyes; the pilot is then the pawn's view location with the pixel tangent
	 * last seen attached (the map camera's lens says nothing about the pilot's; the view's own until one was seen). Off,
	 * without a player controller or a pawn: the view itself, attached (stage A). O(1) a frame; a log line on a flip only.
	 */
	APSGalaxyGpuStars::FApproachEyes MakeApproachEyes(UWorld& World, const FVector& Camera, const double PixelTangent)
	{
		APSGalaxyGpuStars::FApproachEyes Result;
		Result.ViewCamera = Camera;
		Result.ViewPixelTangent = PixelTangent;
		Result.PilotCamera = Camera;
		Result.PilotPixelTangent = PixelTangent;
		Result.bDetached = false;
		if (GPilotEyes.World.Get() != &World)
		{
			GPilotEyes = FPilotEyesCache();
			GPilotEyes.World = &World;
		}
		const APlayerController* Player = CVarApproachPilotView.GetValueOnGameThread() != 0 ? World.GetFirstPlayerController() : nullptr;
		const APawn* Pilot = Player ? Player->GetPawn() : nullptr;
		bool bMapOpen = false;
		double ApartCm = 0.0;
		if (IsValid(Pilot))
		{
			const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(Player);
			bMapOpen = Gravity && Gravity->IsStrategicMapOpen();
			const FVector Eye = Pilot->GetPawnViewLocation();
			ApartCm = FVector::Distance(Camera, Eye);
			// Rio 06.10 (review): at least 1 km, so 0 or below cannot detach the pilot's own camera every frame.
			const double MaxApartCm = FMath::Max(static_cast<double>(CVarApproachPilotViewKm.GetValueOnGameThread()), 1.0) * 1.0e5;
			if (!Eye.ContainsNaN() && (bMapOpen || ApartCm > MaxApartCm))
			{
				Result.bDetached = true;
				Result.PilotCamera = Eye;
				Result.PilotPixelTangent = GPilotEyes.PixelTangent > 0.0 ? GPilotEyes.PixelTangent : PixelTangent;
			}
		}
		if (!Result.bDetached)
		{
			GPilotEyes.PixelTangent = PixelTangent;
		}
		if (Result.bDetached != GPilotEyes.bDetached)
		{
			GPilotEyes.bDetached = Result.bDetached;
			if (Result.bDetached)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach state from the pilot's eyes (%s), pixel tangent %.6g; fades and glyphs from the view"),
					bMapOpen ? TEXT("map open") : *FString::Printf(TEXT("view %.0f km from the pilot"), ApartCm / 1.0e5),
					Result.PilotPixelTangent);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Stars] approach state from the view again (pixel tangent %.6g)"), PixelTangent);
			}
		}
		return Result;
	}
}

void APSFarStarGlyphs::Update(UWorld* World, const TArray<AActor*>& Attached, const FVector& Camera, const double PixelTangent,
	const bool bDaylightHidden)
{
	using namespace APSFarStarGlyphsLocal;
	if (!World || !FMath::IsFinite(PixelTangent) || PixelTangent <= 0.0)
	{
		return;
	}
	// Rio 06.10 (aps.Stars.GlyphsFollowSky): from here on the glyphs follow every move of the sky in the same call.
	APSFarStarGlyphsLocal::EnsureGlyphsFollowSkyBound();
	// Rio 05.10 night: the GPU stars near the camera in a REAL SCALE world first (their approach points), so the galaxy
	// glyph below knows whether its star still needs it. After PresentGameplayFrame, in the same frame.
	// Rio 06.10 (star approach v2, stage B, change 12; aps.Stars.ApproachPilotView): they decide from the pilot's eyes and
	// draw for this view; the glyphs below keep the view (Camera, PixelTangent).
	// Rio 06.10 (review): the glyphs' crossfade snaps when the approach points' fade does (while the eyes are detached and on
	// a flip: the F10 map round trip), so a glyph eased from the map camera's view never lies over its sphere, rays and all,
	// for a quarter second after the map closes.
	const bool bEyesWereDetached = APSFarStarGlyphsLocal::GPilotEyes.World.Get() == World && APSFarStarGlyphsLocal::GPilotEyes.bDetached;
	const APSGalaxyGpuStars::FApproachEyes Eyes = APSFarStarGlyphsLocal::MakeApproachEyes(*World, Camera, PixelTangent);
	APSGalaxyGpuStars::UpdateApproachPoints(World, Eyes);
	const bool bGlyphFadeSnap = Eyes.bDetached || Eyes.bDetached != bEyesWereDetached;
	for (auto It = GGlyphs.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = GGlyphGlare.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	TArray<FGlyph>& Glyphs = GGlyphs.FindOrAdd(World);
	const AStarCluster* Cluster = nullptr;
	for (const AActor* Actor : Attached)
	{
		if ((Cluster = Cast<AStarCluster>(Actor)) != nullptr)
		{
			break;
		}
	}
	const UHierarchicalInstancedStaticMeshComponent* Source = IsValid(Cluster) ? Cluster->StarMeshInstances : nullptr;
	const float GlyphPixels = CVarGlyphPixels.GetValueOnGameThread();
	if (GlyphPixels <= 0.0f || !IsValid(Source) || !IsValid(Source->GetStaticMesh()))
	{
		for (FGlyph& Glyph : Glyphs) Destroy(Glyph);
		Glyphs.Reset();
		return;
	}

	// The stars that stand materialized now, each with its catalogue point.
	TSet<const AStar*> Present;
	{
		// Rio 06.10 (audit: trace scopes, instrumentation only).
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_FarGlyphRecords);
		for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
		{
			const AStarSystem* System = Record.bMaterialized ? Record.MaterializedSystem.Get() : nullptr;
			AStar* Star = System ? System->MainStar : nullptr;
			if (!IsValid(Star) || Record.InstanceIndex == INDEX_NONE)
			{
				continue;
			}
			Present.Add(Star);
			if (!Glyphs.ContainsByPredicate([Star](const FGlyph& Glyph) { return Glyph.Star.Get() == Star; }))
			{
				FGlyph& Glyph = Glyphs.AddDefaulted_GetRef();
				Glyph.Star = Star;
				Glyph.Index = Record.InstanceIndex;
			}
		}
	}
	// Rio 05.10 (real scale, stage 2): and the galaxy star that stands materialized in a REAL SCALE world.
	if (APSRealScale::IsActive(World))
	{
		AddGalaxyGlyph(*World, *Cluster, Source->NumCustomDataFloats, Glyphs, Present);
	}
	Glyphs.RemoveAll([&Present](FGlyph& Glyph)
	{
		const bool bGone = !Glyph.Star.IsValid() || !Present.Contains(Glyph.Star.Get());
		if (bGone) Destroy(Glyph);
		return bGone;
	});

	const double MeshRadius = FMath::Max(Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
	// Rio 06.10 (still ship): while a fast REAL SCALE ship owes its travel only the sky moves; a glyph stands where the sky
	// has its star (SkyPlace) and is sized from there.
	const double NowSeconds = FPlatformTime::Seconds();
	// Rio 06.10 (aps.Stars.FarGlyphHysteresis; 0, 1 and above: off, one threshold both ways as before).
	const float HysteresisSetting = CVarGlyphHysteresis.GetValueOnGameThread();
	const double Hysteresis = HysteresisSetting > 0.0f && HysteresisSetting < 1.0f
		? FMath::Clamp(static_cast<double>(HysteresisSetting), 0.5, 1.0) : 1.0;
	const bool bTrace = APSFarStarGlyphsLocal::GlyphTraceOn();
	// Rio 06.10 (star approach v2, stage B; aps.Stars.FarGlyphCrossfade): the glyph fades over the approach point's band.
	const bool bCrossfade = CVarGlyphCrossfade.GetValueOnGameThread() != 0;
	const double CrossfadeStart = bCrossfade ? APSFarStarGlyphsLocal::GlyphFadeBandStart() * GlyphPixels : 0.0;
	// Rio 06.10 (review: the hand-over in the approach point's time): the crossfade eases at most GlyphFadeRate a second over
	// the world's step, clamped as the approach points clamp theirs (paused: held).
	const float GlyphFadeSeconds = FMath::Clamp(World->GetDeltaSeconds(), 0.0f, 0.25f);
	// Rio 06.10 (aps.Stars.SystemGlare, aps.Stars.FarGlyphLinearFade): the stellar view's values for this frame, and whether a
	// glyph may have its own material: the shared one is the stellar view's gameplay material with the daylight term (the
	// one ApplyPointVisibility sets). Neither switch on: no own material, the shared one exactly as before.
	const APSFarStarGlyphsLocal::FGlyphGlare* GlareValues = APSFarStarGlyphsLocal::GGlyphGlare.Find(World);
	const bool bGlyphGlare = GlareValues && GlareValues->bEnabled;
	const float GlyphDayVisibility = GlareValues ? GlareValues->DayVisibility : 1.0f;
	const bool bGlyphLinearFade = bCrossfade && CVarGlyphLinearFade.GetValueOnGameThread() != 0;
	UMaterialInterface* GlyphSharedMaterial = Source->GetMaterial(0);
	float SharedVisibilityProbe = 1.0f;
	const bool bGlyphOwnMaterials = (bGlyphGlare || bGlyphLinearFade) && Cast<UMaterialInstanceDynamic>(GlyphSharedMaterial)
		&& GlyphSharedMaterial->GetScalarParameterValue(APSFarStarGlyphsLocal::GlyphVisibilityName(), SharedVisibilityProbe);
	for (FGlyph& Glyph : Glyphs)
	{
		const AStar* Star = Glyph.Star.Get();
		const FVector StarInSky = UAPSWorldOriginSubsystem::SkyPlace(*Star);
		const double RadiusCm = FMath::Max(static_cast<double>(Star->StarRadiusKM), 1.0) * 100000.0;
		const double PixelWorldRadius = FVector::Distance(Camera, StarInSky) * PixelTangent;
		const double ApparentPixels = PixelWorldRadius > 0.0 ? RadiusCm / PixelWorldRadius : TNumericLimits<double>::Max();
		// The glyph while the disc is smaller than it; from there on the sphere itself (photosphere and corona).
		// Rio 06.10 (aps.Stars.FarGlyphHysteresis): latched by the disc alone. The glyph's side (and a new glyph) keeps the
		// full threshold; the sphere's side comes back to the glyph only below its Hysteresis share (1: as before).
		// Rio 06.10 (stage B, aps.Stars.FarGlyphCrossfade): crossfading, one threshold both ways and no latch: the glyph's
		// light is down to nothing where it switches, so the switch itself is not seen (Alpha 1 off).
		// Rio 06.10 (review): the share eases toward the disc's at GlyphFadeRate, as the approach point's fade does, and the
		// glyph is drawn while it has any (the approach point too draws on until its fade is out); a new glyph takes the
		// disc's at once. Off: forgotten, so switching it on again starts from the disc's.
		if (!bCrossfade)
		{
			Glyph.FadeAlpha = -1.0f;
		}
		else
		{
			const float FadeTarget = static_cast<float>(1.0
				- FMath::SmoothStep(CrossfadeStart, static_cast<double>(GlyphPixels), ApparentPixels));
			Glyph.FadeAlpha = Glyph.FadeAlpha < 0.0f || bGlyphFadeSnap ? FadeTarget
				: FMath::Clamp(FMath::FInterpConstantTo(Glyph.FadeAlpha, FadeTarget, GlyphFadeSeconds, APSFarStarGlyphsLocal::GlyphFadeRate), 0.0f, 1.0f);
		}
		const double GlyphAlpha = bCrossfade ? static_cast<double>(Glyph.FadeAlpha) : 1.0;
		const double Threshold = !bCrossfade && Glyph.GlyphSide == 0 ? GlyphPixels * Hysteresis : static_cast<double>(GlyphPixels);
		const int32 PreviousSide = Glyph.GlyphSide;
		Glyph.GlyphSide = ApparentPixels < Threshold ? 1 : 0;
		const bool bShow = !bDaylightHidden && (bCrossfade ? GlyphAlpha > 0.0 : Glyph.GlyphSide == 1) && !Star->IsHidden();
		// Rio 06.10 (aps.Stars.SystemGlare): its own value if the stellar view lists its star, else the others'. Its own
		// material draws with day x glare (x the crossfade share with aps.Stars.FarGlyphLinearFade), one value; -1: none.
		float GlyphGlareValue = 1.0f;
		if (bGlyphGlare)
		{
			GlyphGlareValue = GlareValues->Others;
			for (const TPair<const AStar*, float>& GlareEntry : GlareValues->Own)
			{
				if (GlareEntry.Key == Star)
				{
					GlyphGlareValue = GlareEntry.Value;
					break;
				}
			}
		}
		const float GlyphVisibility = bGlyphOwnMaterials
			? GlyphDayVisibility * GlyphGlareValue * (bGlyphLinearFade ? static_cast<float>(GlyphAlpha) : 1.0f) : -1.0f;
		// Rio 06.10 (star approach v2, trace; aps.Stars.ApproachTrace): every glyph every frame, its switches as events.
		if (bTrace)
		{
			APSFarStarGlyphsLocal::TraceGlyphFrame(*World, *Star, StarInSky, bShow, PreviousSide, Glyph.GlyphSide,
				PixelWorldRadius / PixelTangent, ApparentPixels,
				PixelWorldRadius > 0.0 ? APSStellarOpticalSupport::CoreRadius(RadiusCm, PixelWorldRadius) / PixelWorldRadius : 0.0,
				Threshold, APSFarStarGlyphsLocal::GlyphBaseEmission(Glyph, *Source), GlyphAlpha, bCrossfade,
				bGlyphGlare && bGlyphOwnMaterials ? GlyphGlareValue : 1.0f);
		}
		// Rio 05.10 night: a galaxy star's glyph (the fallback without an approach point) logs its hand-over to the sphere.
		if (Glyph.bGalaxy && bShow != Glyph.bShown)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s: %s at %.4g AU (disc %.3f px, threshold %.2f px)"),
				*Star->AstroName.ToString(), bShow ? TEXT("sphere -> glyph") : TEXT("glyph -> sphere"),
				PixelWorldRadius / PixelTangent / 1.495978707e13, ApparentPixels, Threshold);
			Glyph.bShown = bShow;
		}
		if (!bShow)
		{
			if (UInstancedStaticMeshComponent* Mesh = Glyph.Mesh.Get(); Mesh && Mesh->IsVisible())
			{
				Mesh->SetVisibility(false);
			}
			continue;
		}
		if (!Glyph.Mesh.IsValid() && !Create(*World, Glyph, *Source, *Star, GlyphVisibility))
		{
			continue;
		}
		UInstancedStaticMeshComponent* Mesh = Glyph.Mesh.Get();
		// Rio 06.10 (aps.Stars.SystemGlare, aps.Stars.FarGlyphLinearFade): its own material, before it is shown: the value on a
		// relative 1% step and at the ends; a new shared material (a new build) gets a new child; with neither switch on, a
		// glyph that had one draws with the shared material again (as before).
		// Rio 06.10 (review): made once per shared material (GlareSource; its own parent is that material's parent, never the
		// shared dynamic instance itself), so a failed make is not retried every frame.
		if (GlyphVisibility >= 0.0f)
		{
			UMaterialInstanceDynamic* OwnGlyphMaterial = Glyph.GlareMaterial.Get();
			if (Glyph.GlareSource.Get() != GlyphSharedMaterial)
			{
				APSFarStarGlyphsLocal::InstallGlyphMaterial(Glyph, *Mesh, GlyphSharedMaterial, GlyphVisibility);
			}
			else if (OwnGlyphMaterial && APSFarStarGlyphsLocal::ShouldSendGlyphVisibility(Glyph.AppliedGlare, GlyphVisibility))
			{
				OwnGlyphMaterial->SetScalarParameterValue(APSFarStarGlyphsLocal::GlyphVisibilityName(), GlyphVisibility);
				Glyph.AppliedGlare = GlyphVisibility;
			}
		}
		else if (GlyphSharedMaterial && (Glyph.GlareMaterial.IsValid() || Glyph.AppliedGlare >= 0.0f || Glyph.GlareSource.IsValid()))
		{
			if (Glyph.GlareMaterial.IsValid() || Glyph.AppliedGlare >= 0.0f)
			{
				Mesh->SetMaterial(0, GlyphSharedMaterial);
			}
			Glyph.GlareMaterial.Reset();
			Glyph.AppliedGlare = -1.0f;
			Glyph.GlareSource.Reset();
		}
		// The crossfade share is in its own material (aps.Stars.FarGlyphLinearFade); the emission then stays full.
		const bool bFadeInMaterial = bGlyphLinearFade && GlyphVisibility >= 0.0f && Glyph.GlareMaterial.IsValid();
		// Sized as the stellar view sizes catalogue points: a compact core of at least a couple of pixels, rays for a
		// bright star, from the point's own data.
		// A galaxy star's glyph (Rio 05.10, real scale) reads its own row.
		const APSStellarOpticalSupport::FProfile Profile = Glyph.Row.IsEmpty()
			? APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData, Source->NumCustomDataFloats, Glyph.Index)
			: APSStellarOpticalSupport::Select(Glyph.Row, Glyph.Row.Num(), 0);
		const double Core = APSStellarOpticalSupport::CoreRadius(RadiusCm, PixelWorldRadius);
		const double Carrier = APSStellarOpticalSupport::CarrierRadius(RadiusCm, PixelWorldRadius, Profile);
		// Data and size reach the GPU through the instance's own updates (no render state rebuild); the size only when it
		// moved by a percent, a hundredth of a pixel at these sizes.
		APSStellarOpticalSupport::Publish(Mesh, 0, APSStellarOpticalSupport::CoreScale(Core, Carrier),
			APSStellarOpticalSupport::ResolvedRayStrength(Profile, RadiusCm, PixelWorldRadius));
		// Rio 06.10 (stage B, aps.Stars.FarGlyphCrossfade): its share of the emission (custom data 3; Select above still reads
		// the full row, so core and rays keep their size), sent on a 1% step and at either end; off, a faded glyph gets its
		// full emission back once and nothing is sent after that (stage A).
		// Rio 06.10 (aps.Stars.FarGlyphLinearFade): the share in its own material's visibility instead, with the glare in the
		// same value (never twice, never fighting over custom data 3); the emission gets its full value back once.
		const float TargetAlpha = bFadeInMaterial ? 1.0f : static_cast<float>(GlyphAlpha);
		if (TargetAlpha != Glyph.AppliedAlpha
			&& (!bCrossfade || FMath::Abs(TargetAlpha - Glyph.AppliedAlpha) > 0.01f || TargetAlpha <= 0.0f || TargetAlpha >= 1.0f))
		{
			const float FullEmission = APSFarStarGlyphsLocal::GlyphBaseEmission(Glyph, *Source);
			if (FullEmission >= 0.0f)
			{
				Mesh->SetCustomDataValue(0, 3, FullEmission * TargetAlpha, false);
				Glyph.AppliedAlpha = TargetAlpha;
			}
		}
		if (Glyph.bGalaxy && NowSeconds >= Glyph.NextLogSeconds)
		{
			Glyph.NextLogSeconds = NowSeconds + 0.5;
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] far glyph for %s (fallback): %.4g AU, disc %.3f px, core %.2f px, carrier %.2f px, emission %.4g"),
				*Star->AstroName.ToString(), PixelWorldRadius / PixelTangent / 1.495978707e13, ApparentPixels,
				Core / PixelWorldRadius, Carrier / PixelWorldRadius, Glyph.Row.IsValidIndex(3) ? Glyph.Row[3] : -1.0f);
		}
		if (!Glyph.Actor->GetActorLocation().Equals(StarInSky, 1.0))
		{
			Glyph.Actor->SetActorLocation(StarInSky, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (!FMath::IsNearlyEqual(Glyph.Carrier, Carrier, Carrier * 0.01))
		{
			Glyph.Carrier = Carrier;
			Mesh->UpdateInstanceTransform(0, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(Carrier / MeshRadius)),
				false, false, true);
		}
		if (!Mesh->IsVisible())
		{
			Mesh->SetVisibility(true);
		}
	}
}

void APSFarStarGlyphs::SetSystemGlare(const UWorld* World, const bool bEnabled, const float DayVisibility, const float Others,
	TConstArrayView<TPair<const AStar*, float>> OwnValues)
{
	using namespace APSFarStarGlyphsLocal;
	if (!World)
	{
		return;
	}
	// Rio 06.10 (aps.Stars.SystemGlare): kept for this frame's Update; a value that is not a number counts as 1 (no dimming).
	FGlyphGlare& WorldGlare = GGlyphGlare.FindOrAdd(World);
	WorldGlare.bEnabled = bEnabled;
	WorldGlare.DayVisibility = FMath::IsFinite(DayVisibility) ? FMath::Clamp(DayVisibility, 0.0f, 1.0f) : 1.0f;
	WorldGlare.Others = FMath::IsFinite(Others) ? FMath::Clamp(Others, 0.0f, 1.0f) : 1.0f;
	WorldGlare.Own.Reset();
	if (bEnabled)
	{
		for (const TPair<const AStar*, float>& GlareEntry : OwnValues)
		{
			WorldGlare.Own.Emplace(GlareEntry.Key, FMath::IsFinite(GlareEntry.Value) ? FMath::Clamp(GlareEntry.Value, 0.0f, 1.0f) : 1.0f);
		}
	}
}

void APSFarStarGlyphs::Reset(const UWorld* World)
{
	using namespace APSFarStarGlyphsLocal;
	if (TArray<FGlyph>* Glyphs = GGlyphs.Find(World))
	{
		for (FGlyph& Glyph : *Glyphs) Destroy(Glyph);
		GGlyphs.Remove(World);
	}
	// Rio 06.10 (aps.Stars.SystemGlare): the stellar view sets its values again before the next Update.
	GGlyphGlare.Remove(World);
}
