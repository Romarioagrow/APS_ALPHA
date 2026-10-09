// aps.Menu.GenerationShots: opens the generation menu and saves a UI screenshot plus the LIVE MODEL card and hierarchy
// rows of every scope, so readability (Rio, 02.10) can be checked without clicking through the menu.

#include "APSGenerationModelCard.h"
#include "WorldGenerationViewModel.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace APSGenerationShotsPrivate
{
	struct FShot
	{
		EAstroPreviewFocus Focus;
		const TCHAR* Name;
		bool bMoon;
	};

	const FShot Shots[] = {
		{EAstroPreviewFocus::Overview, TEXT("overview"), false},
		{EAstroPreviewFocus::Galaxy, TEXT("galaxy"), false},
		{EAstroPreviewFocus::StarCluster, TEXT("cluster"), false},
		{EAstroPreviewFocus::HomeSystem, TEXT("system"), false},
		{EAstroPreviewFocus::HomeStar, TEXT("star"), false},
		{EAstroPreviewFocus::HomePlanet, TEXT("planet"), false},
		{EAstroPreviewFocus::HomePlanet, TEXT("moon"), true},
	};

	struct FState
	{
		bool bActive{false};
		bool bOpened{false};
		bool bQuit{false};
		int32 Planets{-1};
		int32 Moons{-1};
		bool bPlanetsApplied{false};
		bool bMoonsApplied{false};
		/** Rio 03.10 (fps A/B in a heavy world): optional stars= csize= ctype= cpop= arguments, applied once. */
		double GalaxyStars{-1.0};
		FString ClusterSize;
		FString ClusterType;
		FString ClusterPopulation;
		/** gpop= gcomp=: the galaxy's POPULATION / COMPOSITION rows (EStarClusterPopulation / Composition names). */
		FString GalaxyPopulation;
		FString GalaxyComposition;
		/** Rio 03.10 (white glow in some generated worlds): gtype= gclass= (EGalaxyType / EGalaxyClass names), gsize= gdens=. */
		FString GalaxyType;
		FString GalaxyClass;
		double GalaxySize{-1.0};
		double GalaxyDensity{-1.0};
		/** Rio 05.10: realscale=1 turns the REAL SCALE (EXPERIMENTAL) row on before the shots. */
		bool bRealScale{false};
		/** Rio 09.10 (playtest 17): drawn= moves the GALAXY STARS slider (every star drawn: placed + GPU points). */
		double DrawnStars{-1.0};
		/** Rio 09.10 (playtest 32): orbit= moves the home planet's ORBIT DISTANCE / AU before the shots, then logs where it
		 * stands after the rebuild (it snapped back under REAL SCALE). */
		double OrbitAu{-1.0};
		int32 OrbitStage{0};
		bool bWorldApplied{false};
		int32 Step{0};
		bool bStepRequested{false};
		/** Rio 02.10: FPS fell while the cluster camera turned. After the cluster shot the camera orbits for a few
		 * seconds and the frame times of that turn are logged. */
		double OrbitUntil{0.0};
		bool bOrbitDone{false};
		int32 OrbitFrames{0};
		double OrbitWorstMs{0.0};
		double OrbitStartSeconds{0.0};
		/** Inside the orbit call (it applies the preview frame), and the thread times of the turn, ms summed. */
		double OrbitCallMs{0.0};
		double OrbitCallWorstMs{0.0};
		double OrbitGameMs{0.0};
		double OrbitRenderMs{0.0};
		double OrbitRhiMs{0.0};
		double LastTickSeconds{0.0};
		double StartSeconds{0.0};
		double ReadySeconds{0.0};
		/** Rio 03.10 ("after REGENERATE the orbits vanish, nothing highlights"): after the scopes, two REGENERATE presses,
		 * each shot at SYSTEM with its overlay. */
		int32 RegenShots{0};
		bool bRegenRequested{false};
		bool bRegenFocused{false};
		FString Label;
		FTSTicker::FDelegateHandle Ticker;
	};
	FState GShots;

	AMainMenuController* FindMenuController()
	{
		if (!GEngine) return nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
			{
				if (AMainMenuController* Controller = Cast<AMainMenuController>(Context.World()->GetFirstPlayerController()))
				{
					return Controller;
				}
			}
		}
		return nullptr;
	}

	void LogScope(const UWorldGenerationViewModel& VM, const TCHAR* Name)
	{
		FAPSModelCard Card;
		VM.GetPreviewModelCard(Card);
		UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] %s card: %s [%s] %s / %s"), Name, *Card.Kind.ToString(),
			*Card.Designation.ToString(), *Card.Title.ToString(), *Card.Subtitle.ToString());
		for (const FAPSModelFact& Fact : Card.Facts)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots]   fact %s = %s %s%s%s"), *Fact.Label.ToString(), *Fact.Value.ToString(),
				*Fact.Unit.ToString(), Fact.Note.IsEmpty() ? TEXT("") : TEXT(" / "), *Fact.Note.ToString());
		}
		TArray<FAPSPreviewBodyEntry> Rows;
		VM.GetPreviewHierarchyEntries(Rows);
		for (const FAPSPreviewBodyEntry& Row : Rows)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots]   row %d [%s] %s / %s"), Row.Depth,
				*APSBodyDesignation::Of(Row.Actor.Get()), *Row.Label.ToString(), *Row.Details.ToString());
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots]   name box %s: %s (rename %d)"), *VM.GetCurrentScopeNameTitle().ToString(),
			*VM.GetCurrentScopeName().ToString(), VM.CanRenameCurrentScope() ? 1 : 0);
	}

	void Finish(const TCHAR* Reason)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] finished: %s"), Reason);
		GShots.bActive = false;
		if (GShots.bQuit)
		{
			FPlatformMisc::RequestExit(false, TEXT("aps.Menu.GenerationShots"));
		}
	}

	bool Tick(float)
	{
		if (!GShots.bActive) return false;
		const double Now = FPlatformTime::Seconds();
		if (Now - GShots.StartSeconds > 600.0)
		{
			Finish(TEXT("timed out"));
			return false;
		}
		AMainMenuController* Controller = FindMenuController();
		UWorldGenerationViewModel* VM = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
		if (!VM) return true;
		if (!GShots.bOpened)
		{
			// Fails until the Slate menu is installed; the ticker retries.
			GShots.bOpened = Controller->OpenAstronomicalGenerationForAutomation(
				EAstroPreviewFocus::HomeSystem, EAPSGenerationRoute::Civilization);
			return true;
		}
		if (!VM->bPreviewReady)
		{
			GShots.ReadySeconds = 0.0;
			return true;
		}
		if (GShots.ReadySeconds <= 0.0) GShots.ReadySeconds = Now;
		if (Now - GShots.ReadySeconds < 4.0) return true;
		if (GShots.Planets > 0 && !GShots.bPlanetsApplied)
		{
			GShots.bPlanetsApplied = true;
			VM->SetPlanetsAmount(GShots.Planets);
			GShots.ReadySeconds = 0.0;
			return true;
		}
		if (GShots.Moons >= 0 && !GShots.bMoonsApplied)
		{
			GShots.bMoonsApplied = true;
			VM->SetMoonsAmount(GShots.Moons);
			GShots.ReadySeconds = 0.0;
			return true;
		}
		if (!GShots.bWorldApplied)
		{
			GShots.bWorldApplied = true;
			bool bChanged = false;
			if (GShots.GalaxyStars > 0.0)
			{
				VM->SetGalaxyPlacedStarCount(GShots.GalaxyStars);
				bChanged = true;
			}
			const auto ApplyEnum = [VM, &bChanged](const UEnum* Enum, const FString& Name)
			{
				if (Name.IsEmpty() || !Enum) return;
				const int64 Value = Enum->GetValueByNameString(Name);
				if (Value == INDEX_NONE)
				{
					UE_LOG(LogTemp, Warning, TEXT("[APS.MenuShots] no %s value '%s'"), *Enum->GetName(), *Name);
					return;
				}
				VM->SetEnumValue(Enum, static_cast<int32>(Value));
				bChanged = true;
			};
			ApplyEnum(StaticEnum<EStarClusterSize>(), GShots.ClusterSize);
			ApplyEnum(StaticEnum<EStarClusterType>(), GShots.ClusterType);
			ApplyEnum(StaticEnum<EStarClusterPopulation>(), GShots.ClusterPopulation);
			ApplyEnum(StaticEnum<EGalaxyType>(), GShots.GalaxyType);
			ApplyEnum(StaticEnum<EGalaxyClass>(), GShots.GalaxyClass);
			if (GShots.GalaxySize > 0.0)
			{
				VM->SetGalaxySize(GShots.GalaxySize);
				bChanged = true;
			}
			if (GShots.GalaxyDensity > 0.0)
			{
				VM->SetGalaxyStarDensity(GShots.GalaxyDensity);
				bChanged = true;
			}
			const auto GalaxyValue = [](const UEnum* Enum, const FString& Name)
			{
				return Enum && !Name.IsEmpty() ? Enum->GetValueByNameString(Name) : INDEX_NONE;
			};
			if (const int64 Value = GalaxyValue(StaticEnum<EStarClusterPopulation>(), GShots.GalaxyPopulation); Value != INDEX_NONE)
			{
				VM->SetGalaxyStarPopulation(static_cast<int32>(Value));
				bChanged = true;
			}
			if (const int64 Value = GalaxyValue(StaticEnum<EStarClusterComposition>(), GShots.GalaxyComposition); Value != INDEX_NONE)
			{
				VM->SetGalaxyStarComposition(static_cast<int32>(Value));
				bChanged = true;
			}
			if (GShots.bRealScale)
			{
				VM->SetRealScale(true);
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] REAL SCALE on (active=%d)"), VM->IsRealScaleActive() ? 1 : 0);
				bChanged = true;
			}
			if (GShots.DrawnStars > 0.0)
			{
				VM->SetGalaxyDrawnStars(GShots.DrawnStars);
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] STARS slider -> %.0f: target %lld (ceiling %d)"),
					GShots.DrawnStars, VM->GetGalaxyDrawnStarTarget(), VM->GetGalaxyDrawnStarCeiling());
				bChanged = true;
			}
			if (bChanged)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] world: stars=%.0f cluster size=%s type=%s population=%s; galaxy population=%s composition=%s"),
					GShots.GalaxyStars, *GShots.ClusterSize, *GShots.ClusterType, *GShots.ClusterPopulation,
					*GShots.GalaxyPopulation, *GShots.GalaxyComposition);
				GShots.ReadySeconds = 0.0;
				return true;
			}
		}
		if (GShots.OrbitAu > 0.0 && GShots.OrbitStage < 3)
		{
			// Rio 09.10 (playtest 32): PLANET of the home world, the slider's value, then the value after the rebuild.
			GShots.ReadySeconds = 0.0;
			if (GShots.OrbitStage == 0)
			{
				// Selected the way a click on it selects it (the SELECTED PLANET panel needs a selected planet).
				const AAstroGenerator* Generator = VM->GetPreviewGenerator();
				if (!Generator || !IsValid(Generator->HomePlanet) || !VM->FocusPreviewBody(Generator->HomePlanet))
				{
					VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
				}
			}
			else if (GShots.OrbitStage == 1)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] orbit: editable=%d before %.4f AU, asking %.4f AU (real scale %d)"),
					VM->CanEditSelectedPlanetOrbit() ? 1 : 0, VM->GetSelectedPlanetOrbitDistanceAu(), GShots.OrbitAu,
					VM->IsRealScaleActive() ? 1 : 0);
				VM->SetSelectedPlanetOrbitDistanceAu(GShots.OrbitAu);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] orbit: after the rebuild %.4f AU (asked %.4f), manual edit %d"),
					VM->GetSelectedPlanetOrbitDistanceAu(), GShots.OrbitAu, VM->HasSelectedPlanetOrbitEdit() ? 1 : 0);
			}
			++GShots.OrbitStage;
			return true;
		}
		if (GShots.Step >= UE_ARRAY_COUNT(Shots))
		{
			if (GShots.RegenShots < 2)
			{
				// Each wait above (4 s after the preview is ready) lets the regenerated system and the camera settle.
				if (!GShots.bRegenRequested)
				{
					GShots.bRegenRequested = true;
					GShots.ReadySeconds = 0.0;
					UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] REGENERATE %d"), GShots.RegenShots + 1);
					VM->RegeneratePreviewVariant();
					return true;
				}
				if (!GShots.bRegenFocused)
				{
					GShots.bRegenFocused = true;
					GShots.ReadySeconds = 0.0;
					VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
					return true;
				}
				const FString RegenName = FString::Printf(TEXT("regen%d_system"), GShots.RegenShots + 1);
				LogScope(*VM, *RegenName);
				const FString RegenFile = FPaths::ScreenShotDir() / TEXT("GenerationMenu")
					/ FString::Printf(TEXT("%s_%d_%s.png"), *GShots.Label, GShots.Step + GShots.RegenShots, *RegenName);
				FScreenshotRequest::RequestScreenshot(RegenFile, true, false);
				UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] shot %s"), *FPaths::GetCleanFilename(RegenFile));
				++GShots.RegenShots;
				GShots.bRegenRequested = false;
				GShots.bRegenFocused = false;
				return true;
			}
			Finish(TEXT("all scopes shot"));
			return false;
		}
		const FShot& Shot = Shots[GShots.Step];
		if (!GShots.bStepRequested)
		{
			GShots.bStepRequested = true;
			GShots.ReadySeconds = 0.0;
			if (Shot.bMoon)
			{
				// The first moon of the system hierarchy, focused the way a click on it does.
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
				TArray<FAPSPreviewBodyEntry> Rows;
				VM->GetPreviewBodyEntries(Rows);
				const FAPSPreviewBodyEntry* MoonRow = Rows.FindByPredicate(
					[](const FAPSPreviewBodyEntry& Row) { return Row.Actor.IsValid() && Row.Actor->IsA<AMoon>(); });
				if (!MoonRow || !VM->FocusPreviewBody(MoonRow->Actor))
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] moon: none in this system, skipped"));
					++GShots.Step;
					GShots.bStepRequested = false;
				}
			}
			else
			{
				VM->SetPreviewFocus(Shot.Focus);
			}
			return true;
		}
		if (GShots.OrbitUntil > 0.0)
		{
			// Turning the camera like a drag: the frame times of the turn, then the next scope.
			const double FrameMs = (Now - GShots.LastTickSeconds) * 1000.0;
			GShots.LastTickSeconds = Now;
			++GShots.OrbitFrames;
			GShots.OrbitWorstMs = FMath::Max(GShots.OrbitWorstMs, FrameMs);
			const double CallStart = FPlatformTime::Seconds();
			VM->OrbitPreview(FVector2D(6.0, 1.5));
			const double CallMs = (FPlatformTime::Seconds() - CallStart) * 1000.0;
			GShots.OrbitCallMs += CallMs;
			GShots.OrbitCallWorstMs = FMath::Max(GShots.OrbitCallWorstMs, CallMs);
			GShots.OrbitGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
			GShots.OrbitRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
			GShots.OrbitRhiMs += FPlatformTime::ToMilliseconds(GRHIThreadTime);
			if (Now < GShots.OrbitUntil) return true;
			VM->EndPreviewOrbit();
			const double Frames = FMath::Max(GShots.OrbitFrames, 1);
			UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] %s orbit: %d frames in %.1f s, avg %.1f ms, worst %.1f ms; orbit call avg %.2f ms worst %.2f ms; game %.1f / render %.1f / rhi %.1f ms"),
				Shot.Name, GShots.OrbitFrames, Now - GShots.OrbitStartSeconds,
				(Now - GShots.OrbitStartSeconds) * 1000.0 / Frames, GShots.OrbitWorstMs, GShots.OrbitCallMs / Frames,
				GShots.OrbitCallWorstMs, GShots.OrbitGameMs / Frames, GShots.OrbitRenderMs / Frames, GShots.OrbitRhiMs / Frames);
			GShots.OrbitUntil = 0.0;
			GShots.bOrbitDone = true;
			++GShots.Step;
			GShots.bStepRequested = false;
			return true;
		}
		LogScope(*VM, Shot.Name);
		const FString File = FPaths::ScreenShotDir() / TEXT("GenerationMenu")
			/ FString::Printf(TEXT("%s_%d_%s.png"), *GShots.Label, GShots.Step, Shot.Name);
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] shot %s"), *FPaths::GetCleanFilename(File));
		// Rio 03.10: the galaxy turns as well; each scope's orbit is timed on its own line.
		if ((Shot.Focus == EAstroPreviewFocus::StarCluster || Shot.Focus == EAstroPreviewFocus::Galaxy) && !Shot.bMoon)
		{
			VM->BeginPreviewOrbit();
			GShots.OrbitStartSeconds = Now;
			GShots.LastTickSeconds = Now;
			GShots.OrbitUntil = Now + 4.0;
			GShots.OrbitFrames = 0;
			GShots.OrbitWorstMs = 0.0;
			GShots.OrbitCallMs = GShots.OrbitCallWorstMs = 0.0;
			GShots.OrbitGameMs = GShots.OrbitRenderMs = GShots.OrbitRhiMs = 0.0;
			return true;
		}
		++GShots.Step;
		GShots.bStepRequested = false;
		return true;
	}
}

static FAutoConsoleCommand GAPSGenerationShotsCommand(
	TEXT("aps.Menu.GenerationShots"),
	TEXT("aps.Menu.GenerationShots <label> [planets] [moons] [quit]: opens the generation menu (Civilization route) and ")
	TEXT("saves a UI screenshot of every scope to Saved/Screenshots/GenerationMenu, logging each LIVE MODEL card ")
	TEXT("and hierarchy row as [APS.MenuShots]."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		using namespace APSGenerationShotsPrivate;
		if (GShots.Ticker.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(GShots.Ticker);
		GShots = FState();
		GShots.bActive = true;
		GShots.StartSeconds = FPlatformTime::Seconds();
		GShots.Label = Args.IsValidIndex(0) ? Args[0] : TEXT("menu");
		GShots.Planets = Args.IsValidIndex(1) ? FCString::Atoi(*Args[1]) : -1;
		GShots.Moons = Args.IsValidIndex(2) ? FCString::Atoi(*Args[2]) : -1;
		GShots.bQuit = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase); });
		for (const FString& Arg : Args)
		{
			FString Key;
			FString Value;
			if (!Arg.Split(TEXT("="), &Key, &Value)) continue;
			if (Key.Equals(TEXT("stars"), ESearchCase::IgnoreCase)) GShots.GalaxyStars = FCString::Atod(*Value);
			else if (Key.Equals(TEXT("csize"), ESearchCase::IgnoreCase)) GShots.ClusterSize = Value;
			else if (Key.Equals(TEXT("ctype"), ESearchCase::IgnoreCase)) GShots.ClusterType = Value;
			else if (Key.Equals(TEXT("cpop"), ESearchCase::IgnoreCase)) GShots.ClusterPopulation = Value;
			else if (Key.Equals(TEXT("gpop"), ESearchCase::IgnoreCase)) GShots.GalaxyPopulation = Value;
			else if (Key.Equals(TEXT("gtype"), ESearchCase::IgnoreCase)) GShots.GalaxyType = Value;
			else if (Key.Equals(TEXT("gclass"), ESearchCase::IgnoreCase)) GShots.GalaxyClass = Value;
			else if (Key.Equals(TEXT("gsize"), ESearchCase::IgnoreCase)) GShots.GalaxySize = FCString::Atod(*Value);
			else if (Key.Equals(TEXT("gdens"), ESearchCase::IgnoreCase)) GShots.GalaxyDensity = FCString::Atod(*Value);
			else if (Key.Equals(TEXT("gcomp"), ESearchCase::IgnoreCase)) GShots.GalaxyComposition = Value;
			else if (Key.Equals(TEXT("realscale"), ESearchCase::IgnoreCase)) GShots.bRealScale = FCString::Atoi(*Value) != 0;
			else if (Key.Equals(TEXT("drawn"), ESearchCase::IgnoreCase)) GShots.DrawnStars = FCString::Atod(*Value);
			else if (Key.Equals(TEXT("orbit"), ESearchCase::IgnoreCase)) GShots.OrbitAu = FCString::Atod(*Value);
		}
		GShots.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), 0.0f);
		UE_LOG(LogTemp, Log, TEXT("[APS.MenuShots] armed label=%s planets=%d moons=%d quit=%d"), *GShots.Label,
			GShots.Planets, GShots.Moons, GShots.bQuit ? 1 : 0);
	}));
#endif
