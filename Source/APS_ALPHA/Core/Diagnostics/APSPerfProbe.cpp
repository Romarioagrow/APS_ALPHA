#include "APSPerfProbe.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "Stats/Stats.h"

namespace APSPerfProbePrivate
{
	TAutoConsoleVariable<float> CVarLogSeconds(TEXT("aps.Perf.LogSeconds"), 5.0f,
		TEXT("Seconds between [APS.Perf] lines in a game world (0 = off)."));
	TAutoConsoleVariable<float> CVarHitchMs(TEXT("aps.Perf.HitchMs"), 50.0f,
		TEXT("A frame whose game, render or GPU time is longer than this logs its own [APS.Perf] hitch line (0 = off)."));
	// 13.10 (Rio's 11.10 log: the three dumps went to the menu and the boarding; the 200-360 ms render frames near planets
	// never got one): a minimum busiest-thread time for a dump to start, and more dumps per world.
	TAutoConsoleVariable<float> CVarDumpMinMs(TEXT("aps.Perf.DumpMinMs"), 45.0f,
		TEXT("A hitch frame starts an automatic 'stat dumphitches' only when its busiest thread took at least this many ms ")
		TEXT("(0: any hitch, as before 13.10)."));
	TAutoConsoleVariable<int32> CVarAutoDump(TEXT("aps.Perf.AutoDumpHitches"), 12,
		TEXT("How many times per world a hitch (after the world's first 20 s, and 30 s after the last dump) turns on ")
		TEXT("'stat dumphitches' for aps.Perf.DumpSeconds, so the log names what each hitch frame spent its time on. The ")
		TEXT("editor going to the background or the world ending ends a dump at once. 0: off; set 0 when using ")
		TEXT("'stat dumphitches' by hand (the probe's -stop ends a manual dump)."));
	TAutoConsoleVariable<float> CVarDumpSeconds(TEXT("aps.Perf.DumpSeconds"), 20.0f,
		TEXT("How long the automatic 'stat dumphitches' stays on."));
	// Rio 14.10 (0.6.5.2: 40-60 fps in CRUISE between a planet and its many moons; his log: game 9-12 ms, render 12-19 ms,
	// GPU 7-9 ms, the bench world showed 2.5 ms game in the same trip): a window of a gameplay world below this frame rate
	// records an Unreal Insights trace (cpu, frame, bookmark, log) of aps.Perf.AutoTraceSeconds into Saved/Profiling, at
	// most aps.Perf.AutoTraces times per session, so his next log comes with the frames that are slow.
	TAutoConsoleVariable<float> CVarAutoTraceFps(TEXT("aps.Perf.AutoTraceFps"), 60.0f,
		TEXT("A [APS.Perf] window in a gameplay world (after its first 30 s) below this many fps starts an Insights trace; 0: off."));
	TAutoConsoleVariable<float> CVarAutoTraceSeconds(TEXT("aps.Perf.AutoTraceSeconds"), 8.0f,
		TEXT("Length of the automatic Insights trace, s."));
	TAutoConsoleVariable<int32> CVarAutoTraces(TEXT("aps.Perf.AutoTraces"), 2,
		TEXT("How many automatic Insights traces per session at most."));
	int32 GTracesUsed = 0;
	double GTraceEnd = 0.0;
	bool bGTracing = false;

	/** The editor out of focus throttles itself to a few frames per second with idle threads (Rio's log 02.10: frames of
	 * 333 ms at 5 ms of work). Such a frame is neither a hitch nor part of the averages. */
	bool IsBackgroundFrame(const double FrameMs, const double BusiestThreadMs)
	{
		return FrameMs > 150.0 && BusiestThreadMs < 0.25 * FrameMs;
	}

	struct FChannel
	{
		double Sum{0.0};
		double Worst{0.0};
		int32 Samples{0};

		void Add(const double Ms)
		{
			if (Ms <= 0.0) return;
			Sum += Ms;
			Worst = FMath::Max(Worst, Ms);
			++Samples;
		}
		double Average() const { return Samples > 0 ? Sum / Samples : 0.0; }
	};

	struct FWindow
	{
		TWeakObjectPtr<UWorld> World;
		double Start{0.0};
		FChannel Frame;
		FChannel Game;
		FChannel Render;
		FChannel Gpu;
		int32 Hitches{0};
		double BackgroundSeconds{0.0};
	};

	FWindow GWindow;
	double GLastHitchLog = 0.0;
	/** The world the probe has followed since WorldStart, how many automatic dumps it used, and when the last one ends(ed). */
	TWeakObjectPtr<UWorld> GDumpWorld;
	double GWorldStart = 0.0;
	int32 GDumpsUsed = 0;
	bool bGDumpRunning = false;
	double GDumpEnd = 0.0;
	/** The engine's AI logging flag before the dump: while stats collect, the engine draws a red "PROFILING WITH AI
	 * LOGGING ON!" over the HUD's objective panel unless AI logging is off (UnrealEngine.cpp, DrawStatsHUD). */
	bool bGAILoggingOffBefore = false;

	void SetHitchDump(const bool bOn, const TCHAR* Reason)
	{
#if STATS
		if (bOn == bGDumpRunning) return;
		// Rio 06.10 (audit: the blind toggle inverted a dump started by hand): UE 5.4's StatsCommand takes -start / -stop,
		// which do nothing when the dump already is in that state. The probe still only stops what it started itself.
		DirectStatsCommand(bOn ? TEXT("stat dumphitches -start") : TEXT("stat dumphitches -stop"), true);
		bGDumpRunning = bOn;
		if (GEngine)
		{
			if (bOn)
			{
				bGAILoggingOffBefore = GEngine->bDisableAILogging != 0;
				GEngine->bDisableAILogging = true;
			}
			else
			{
				GEngine->bDisableAILogging = bGAILoggingOffBefore;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[APS.Perf] stat dumphitches %s (%s)"), bOn ? TEXT("ON") : TEXT("OFF"), Reason);
#endif
	}
}

void APSPerfProbe::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSPerfProbePrivate;
	if (bGTracing && GEngine && FPlatformTime::Seconds() >= GTraceEnd)
	{
		bGTracing = false;
		GEngine->Exec(World, TEXT("Trace.Stop"));
		UE_LOG(LogTemp, Warning, TEXT("[APS.Perf] Insights trace stopped"));
	}
	using namespace APSPerfProbePrivate;
	const double Interval = CVarLogSeconds.GetValueOnGameThread();
	if (!World) return;
	const double Now = FPlatformTime::Seconds();
	if (GDumpWorld.Get() != World)
	{
		SetHitchDump(false, TEXT("new world"));
		GDumpWorld = World;
		GWorldStart = Now;
		GDumpsUsed = 0;
		GDumpEnd = 0.0;
	}
	if (bGDumpRunning && Now >= GDumpEnd)
	{
		SetHitchDump(false, TEXT("time is up"));
	}
	// Rio 06.10 (audit: aps.Perf.LogSeconds 0 mid-dump left the dump on for good): the two blocks above run with the
	// log off too; without a running dump they only note the world.
	if (Interval <= 0.0) return;
	if (GWindow.World.Get() != World)
	{
		GWindow = FWindow();
		GWindow.World = World;
		GWindow.Start = Now;
		return;
	}
	// The thread times are those of the last finished frame; the delta is this frame's wall time.
	const double FrameMs = DeltaSeconds * 1000.0;
	const double GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
	const double RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
	const double GpuMs = FPlatformTime::ToMilliseconds(GGPUFrameTime);
	const double BusiestMs = FMath::Max3(GameMs, RenderMs, GpuMs);
	if (IsBackgroundFrame(FrameMs, BusiestMs))
	{
		GWindow.BackgroundSeconds += DeltaSeconds;
		// Rio 05.10: out of focus the editor's idle 333 ms frames filled the dump; the flight's hitches never got in.
		if (bGDumpRunning)
		{
			SetHitchDump(false, TEXT("editor in the background"));
			GDumpEnd = Now;
		}
	}
	else
	{
		GWindow.Frame.Add(FrameMs);
		GWindow.Game.Add(GameMs);
		GWindow.Render.Add(RenderMs);
		GWindow.Gpu.Add(GpuMs);
		const float HitchMs = CVarHitchMs.GetValueOnGameThread();
		if (HitchMs > 0.0f && BusiestMs > HitchMs)
		{
			++GWindow.Hitches;
			if (Now - GLastHitchLog > 0.25)
			{
				GLastHitchLog = Now;
				UE_LOG(LogTemp, Warning, TEXT("[APS.Perf] hitch %.0f ms: game %.1f, render %.1f, gpu %.1f"),
					FrameMs, GameMs, RenderMs, GpuMs);
			}
			// Rio 02.10 (freezes in flight): past a world's loading, the next stretch of frames is dumped with the stat
			// tree of every hitch frame, so the log says which system spent the time. Rio 05.10 (the one dump was spent in
			// the menu): up to aps.Perf.AutoDumpHitches times per world, 30 s apart.
			const int32 MaxDumps = CVarAutoDump.GetValueOnGameThread();
			if (MaxDumps > GDumpsUsed && !bGDumpRunning && Now - GWorldStart > 20.0 && Now - GDumpEnd > 30.0
				// The wall-time frame, not the thread stat: a screenshot's read-back makes the last frame's game time read 500 ms.
				&& FrameMs >= CVarDumpMinMs.GetValueOnGameThread())
			{
				++GDumpsUsed;
				GDumpEnd = Now + FMath::Max(CVarDumpSeconds.GetValueOnGameThread(), 5.0f);
				SetHitchDump(true, *FString::Printf(TEXT("hitch, dump %d of %d in this world"), GDumpsUsed, MaxDumps));
			}
		}
	}
	if (Now - GWindow.Start < Interval) return;
	const double Seconds = Now - GWindow.Start;
	const double ActiveSeconds = FMath::Max(Seconds - GWindow.BackgroundSeconds, 0.001);
	if (GWindow.Frame.Samples > 0)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Perf] %.0f fps over %.1f s | frame %.1f ms (worst %.1f) | game %.1f (%.1f) | render %.1f (%.1f) | gpu %.1f (%.1f) | hitches %d%s"),
			GWindow.Frame.Samples / ActiveSeconds, ActiveSeconds, GWindow.Frame.Average(), GWindow.Frame.Worst,
			GWindow.Game.Average(), GWindow.Game.Worst, GWindow.Render.Average(), GWindow.Render.Worst,
			GWindow.Gpu.Average(), GWindow.Gpu.Worst, GWindow.Hitches,
			GWindow.BackgroundSeconds > 0.5 ? *FString::Printf(TEXT(" | %.0f s in background"), GWindow.BackgroundSeconds)
				: TEXT(""));
	}
	// Rio 14.10 (aps.Perf.AutoTraceFps): a slow window in a gameplay world records an Insights trace.
	if (GWindow.Frame.Samples > 0 && World && GEngine && !bGTracing && GTracesUsed < CVarAutoTraces.GetValueOnGameThread()
		&& Now - GWorldStart > 30.0 && GWindow.BackgroundSeconds < 0.5
		&& GWindow.Frame.Samples / ActiveSeconds < CVarAutoTraceFps.GetValueOnGameThread()
		// Rio 15.10 (his 0.6.5.4 log: a "3 fps" window of 74.6 s with 8.3 ms frames took the first trace): an ordinary window
		// whose frames are slow, not a stretched one (a pause, a load).
		&& Seconds <= 3.0 * Interval && GWindow.Frame.Average() > 1000.0 / FMath::Max(CVarAutoTraceFps.GetValueOnGameThread(), 1.0f)
		&& !World->GetMapName().Contains(TEXT("MainMenu")))
	{
		++GTracesUsed;
		bGTracing = true;
		GTraceEnd = Now + FMath::Clamp(static_cast<double>(CVarAutoTraceSeconds.GetValueOnGameThread()), 2.0, 30.0);
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProfilingDir() / FString::Printf(TEXT("APS_lowfps_%s.utrace"),
			*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
		GEngine->Exec(World, *FString::Printf(TEXT("Trace.File %s cpu,frame,bookmark,log"), *File));
		UE_LOG(LogTemp, Warning, TEXT("[APS.Perf] %.0f fps window: Insights trace %d of %d for %.0f s -> %s"),
			GWindow.Frame.Samples / ActiveSeconds, GTracesUsed, CVarAutoTraces.GetValueOnGameThread(), GTraceEnd - Now, *File);
	}
	const TWeakObjectPtr<UWorld> Kept = GWindow.World;
	GWindow = FWindow();
	GWindow.World = Kept;
	GWindow.Start = Now;
}

void APSPerfProbe::WorldEnded(UWorld* World)
{
	using namespace APSPerfProbePrivate;
	// Rio 06.10 (audit: a dump started in a world that ended kept running into the menu or the next load, with nothing
	// left to tick it off).
	if (bGDumpRunning && (!GDumpWorld.IsValid() || GDumpWorld.Get() == World))
	{
		SetHitchDump(false, TEXT("world end"));
		GDumpEnd = FPlatformTime::Seconds();
	}
}
