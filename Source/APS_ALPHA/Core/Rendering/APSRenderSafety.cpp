#include "APSRenderSafety.h"
#include "APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DelayedAutoRegister.h"
#include "Tickable.h"

/**
 * Rio 02.10: the editor crashed while he turned the strategic map ("Assertion failed: IntFitsIn ... Loss of data caused
 * by narrowing conversion, In = -27088503488" in the renderer). UE 5.4's virtual shadow map cache pans a directional
 * light's clipmap by the camera's move since the last frame, in pages of about a centimetre at the finest level, and
 * narrows that offset to int32 (FVirtualShadowMapCacheEntry::UpdateClipmapLevel): a camera that moves more than about
 * 21,000 km sideways to the sun in one frame (the map orbiting at AU distances, a star drive cruise) overflows it.
 * While the map is open, and while the camera moves over 1,000 km a frame and a second after, panning is turned off
 * (r.Shadow.Virtual.Cache.ClipmapPanning 0): the cache is then rebuilt instead of panned, which such moves need anyway.
 */
namespace APSRenderSafetyLocal
{
	TAutoConsoleVariable<int32> CVarGuard(TEXT("aps.Render.ClipmapPanningGuard"), 1,
		TEXT("1: switch the shadow cache's clipmap panning off while the strategic map is open or the camera moves over ")
		TEXT("1,000 km a frame (a UE 5.4 int32 overflow crash). 0: leave it alone."));

	constexpr double FastMoveCm = 1.0e8;
	constexpr double HoldSeconds = 1.0;

	/** The engine's panning switch, looked up once (Rio 04.10 log: 500 lookups flagged by the console manager). */
	IConsoleVariable* PanningVariable()
	{
		static IConsoleVariable* const Variable =
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Cache.ClipmapPanning"));
		return Variable;
	}

	class FGuard final : public FTickableGameObject
	{
	public:
		virtual TStatId GetStatId() const override
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSRenderSafety, STATGROUP_Tickables);
		}
		virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
		virtual bool IsTickableWhenPaused() const override { return true; }
		// PIE worlds live in the editor process: tick there too (only game and PIE worlds are looked at).
		virtual bool IsTickableInEditor() const override { return true; }

		/** A jump this frame (APSRenderSafety::MarkCameraJump): panning off now, held as for a fast camera. */
		void MarkRisky(const TCHAR* Why)
		{
			RiskySeconds = FPlatformTime::Seconds();
			IConsoleVariable* Panning = PanningVariable();
			if (Panning && !bForcedOff && CVarGuard.GetValueOnGameThread() != 0)
			{
				Saved = Panning->GetInt();
				if (Saved != 0)
				{
					Panning->Set(0, ECVF_SetByCode);
					bForcedOff = true;
					UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow clipmap panning off (%s)"), Why);
				}
			}
		}

		virtual void Tick(float) override
		{
			IConsoleVariable* Panning = PanningVariable();
			if (!Panning || !GEngine)
			{
				return;
			}
			const APlayerController* Controller = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				const UWorld* World = Context.World();
				if (World && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
				{
					Controller = World->GetFirstPlayerController();
					if (Controller)
					{
						break;
					}
				}
			}
			const double Now = FPlatformTime::Seconds();
			bool bRisky = false;
			FString Why;
			if (const APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager : nullptr)
			{
				const FVector Location = Camera->GetCameraLocation();
				const double Moved = bHasLast ? FVector::Distance(Location, Last) : 0.0;
				Last = Location;
				bHasLast = true;
				const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(Controller);
				if (Gravity && Gravity->IsStrategicMapOpen())
				{
					bRisky = true;
					Why = TEXT("strategic map");
				}
				else if (Moved > FastMoveCm)
				{
					bRisky = true;
					Why = FString::Printf(TEXT("camera moved %.0f km in a frame"), Moved * 1.0e-5);
				}
			}
			else
			{
				bHasLast = false;
			}
			if (bRisky)
			{
				RiskySeconds = Now;
			}
			const bool bOff = CVarGuard.GetValueOnGameThread() != 0 && (bRisky || Now - RiskySeconds < HoldSeconds);
			if (bOff && !bForcedOff)
			{
				Saved = Panning->GetInt();
				if (Saved != 0)
				{
					Panning->Set(0, ECVF_SetByCode);
					bForcedOff = true;
					UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow clipmap panning off (%s)"), *Why);
				}
			}
			else if (!bOff && bForcedOff)
			{
				Panning->Set(Saved, ECVF_SetByCode);
				bForcedOff = false;
				UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow clipmap panning back on"));
			}
		}

	private:
		FVector Last{FVector::ZeroVector};
		bool bHasLast{false};
		double RiskySeconds{-1000.0};
		bool bForcedOff{false};
		int32 Saved{1};
	};

	/**
	 * Rio 06.10 ("a dark sphere around the ship", frames 186/187 low over KYPHOTHEA's night side): the star key's virtual
	 * shadow map is a clipmap around the camera that ends at r.Shadow.Virtual.Clipmap.LastLevel (UE 5.4 default 22, the
	 * project keeps it). Level L reaches 2^(L+1) cm, less the renderer's resolution LOD bias (scalability, FOV and render
	 * width; about a level at 1080p), so the last level ends some 40-80 km out, the "84 km sphere". Past it every pixel
	 * counts as unshadowed (VirtualShadowMapProjectionDirectional.ush, GetMappedClipmapId): the terrain stops shadowing the
	 * key, and at night, at the terminator or under a low sun the far ground lights up around a dark disc that moves with
	 * the camera. Below 150 km over a body with the sun under 30 degrees the last level is raised until the clipmap reaches
	 * the horizon, at most to aps.Render.SurfaceShadowMaxLevel. The renderer reads the level every frame, but a new level
	 * count drops the light's whole shadow cache once (FVirtualShadowMapArrayCacheManager::FindCreateLightCacheEntry):
	 * the level goes up at once and down only after 3 s of a lower want. Checked four times a second; bodies are listed
	 * every 2 s. aps.Render.SurfaceShadowReach 0 puts the base level back.
	 */
	TAutoConsoleVariable<int32> CVarSurfaceShadowReach(TEXT("aps.Render.SurfaceShadowReach"), 1,
		TEXT("1: below 150 km over a planet or moon with the sun under 30 degrees (night, terminator, low sun), raise ")
		TEXT("r.Shadow.Virtual.Clipmap.LastLevel until the star's shadow map reaches the horizon (no dark disc around the ")
		TEXT("camera). 0: leave the level at its base (the engine's 22)."));

	TAutoConsoleVariable<int32> CVarSurfaceShadowMaxLevel(TEXT("aps.Render.SurfaceShadowMaxLevel"), 26,
		TEXT("The highest r.Shadow.Virtual.Clipmap.LastLevel aps.Render.SurfaceShadowReach may set. Each level doubles the ")
		TEXT("reach and may add shadow pages. Hard cap 28."));

	constexpr double ReachCheckSeconds = 0.25;
	constexpr double ReachBodyListSeconds = 2.0;
	constexpr double ReachLowerAfterSeconds = 3.0;
	constexpr double ReachAltitudeCm = 1.5e7;
	constexpr double ReachSunSine = 0.5;
	/** Once raised, the level stays up to 10% past either threshold, so hovering at one does not flap it. */
	constexpr double ReachKeepFactor = 1.1;
	/**
	 * Rio 06.10 (review): a raised level comes down only once the horizon sits a quarter level (about 19% of reach)
	 * inside the lower one, so altitude, FOV or window-size jitter on a level boundary does not drop the shadow cache
	 * every few seconds.
	 */
	constexpr double ReachLowerMarginLevels = 0.25;
	constexpr double ReachHorizonScale = 1.15;
	/** Hills past the geometric horizon. */
	constexpr double ReachHorizonMarginCm = 2.0e6;
	/** The renderer keeps the last level's radius, 2^(LastLevel + 1) cm, in an int (VirtualShadowMapClipmap.cpp). */
	constexpr int32 ReachHardMaxLevel = 28;
	/** FVirtualShadowMap::VirtualMaxResolutionXY: 128 pages of 128 texels. */
	constexpr double ReachVirtualResolution = 16384.0;

	/** The engine variables the reach reads, looked up once. */
	struct FReachVariables
	{
		IConsoleVariable* LastLevel{nullptr};
		IConsoleVariable* LodBias{nullptr};
		IConsoleVariable* LodBiasMoving{nullptr};
		IConsoleVariable* ScreenPercentage{nullptr};
		IConsoleVariable* DefaultMode{nullptr};
		IConsoleVariable* DefaultPercentage{nullptr};
		IConsoleVariable* AutoPixelCountMultiplier{nullptr};
	};

	const FReachVariables& ReachVariables()
	{
		static const FReachVariables Variables = []
		{
			IConsoleManager& Manager = IConsoleManager::Get();
			FReachVariables Found;
			Found.LastLevel = Manager.FindConsoleVariable(TEXT("r.Shadow.Virtual.Clipmap.LastLevel"));
			Found.LodBias = Manager.FindConsoleVariable(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"));
			Found.LodBiasMoving = Manager.FindConsoleVariable(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving"));
			Found.ScreenPercentage = Manager.FindConsoleVariable(TEXT("r.ScreenPercentage"));
			Found.DefaultMode = Manager.FindConsoleVariable(TEXT("r.ScreenPercentage.Default.Desktop.Mode"));
			Found.DefaultPercentage = Manager.FindConsoleVariable(TEXT("r.ScreenPercentage.Default"));
			Found.AutoPixelCountMultiplier = Manager.FindConsoleVariable(TEXT("r.ScreenPercentage.Auto.PixelCountMultiplier"));
			return Found;
		}();
		return Variables;
	}

	/**
	 * The share of the viewport width the scene renders at (LegacyScreenPercentageDriver.cpp): r.ScreenPercentage when
	 * set, otherwise the desktop default, which follows the display with BaseEngine.ini's [Rendering.AutoScreenPercentage]
	 * (720p shown: 720p rendered, 2160p: 1080p, 4320p: 1440p; the project keeps those). Dynamic resolution and the
	 * editor's own viewport percentages are not followed: a few tenths of a level at most.
	 */
	double ReachRenderFraction(const FReachVariables& Variables, double Width, double Height)
	{
		const double Manual = Variables.ScreenPercentage ? Variables.ScreenPercentage->GetFloat() : 0.0;
		if (Manual > 0.0)
		{
			return FMath::Clamp(Manual / 100.0, 0.1, 4.0);
		}
		const int32 Mode = Variables.DefaultMode ? Variables.DefaultMode->GetInt() : 1;
		if (Mode == 0)
		{
			const double Default = Variables.DefaultPercentage ? Variables.DefaultPercentage->GetFloat() : 100.0;
			return FMath::Clamp(Default / 100.0, 0.1, 4.0);
		}
		if (Mode != 1)
		{
			return 1.0;
		}
		const auto Pixels = [](double Height169) { return Height169 * Height169 * (1920.0 / 1080.0); };
		const double Shown = FMath::Max(Width * Height, 1.0);
		double Rendered = Shown;
		if (Shown > Pixels(4320.0))
		{
			Rendered = Shown * Pixels(1440.0) / Pixels(4320.0);
		}
		else if (Shown > Pixels(2160.0))
		{
			Rendered = FMath::Lerp(Pixels(1080.0), Pixels(1440.0),
				(Shown - Pixels(2160.0)) / (Pixels(4320.0) - Pixels(2160.0)));
		}
		else if (Shown > Pixels(720.0))
		{
			Rendered = FMath::Lerp(Pixels(720.0), Pixels(1080.0),
				(Shown - Pixels(720.0)) / (Pixels(2160.0) - Pixels(720.0)));
		}
		const double Multiplier = Variables.AutoPixelCountMultiplier
			? FMath::Max(static_cast<double>(Variables.AutoPixelCountMultiplier->GetFloat()), 0.01) : 1.0;
		return FMath::Clamp(FMath::Sqrt(Multiplier * Rendered / Shown), 0.1, 4.0);
	}

	/**
	 * The renderer's resolution LOD bias for a directional clipmap (FVirtualShadowMapClipmap's constructor): a pixel
	 * samples level log2(distance) + bias, so the last level reaches 2^(LastLevel + 1 - bias) cm. The larger of the still
	 * and moving light biases is taken (scalability sets them equal).
	 */
	double ReachLodBias(const APlayerController& Controller, const APlayerCameraManager& Camera)
	{
		const FReachVariables& Variables = ReachVariables();
		int32 SizeX = 0;
		int32 SizeY = 0;
		Controller.GetViewportSize(SizeX, SizeY);
		const double Width = SizeX > 0 ? SizeX : 1920.0;
		const double Height = SizeY > 0 ? SizeY : 1080.0;
		const double TanHalfFov = FMath::Tan(FMath::DegreesToRadians(
			FMath::Clamp(static_cast<double>(Camera.GetFOVAngle()), 1.0, 170.0) * 0.5));
		const double RenderWidth = FMath::Max(Width * ReachRenderFraction(Variables, Width, Height), 64.0);
		const double LodScale = 0.5 * TanHalfFov * ReachVirtualResolution / RenderWidth;
		const double Bias = FMath::Max(Variables.LodBias ? Variables.LodBias->GetFloat() : -0.5f,
			Variables.LodBiasMoving ? Variables.LodBiasMoving->GetFloat() : 0.5f);
		return FMath::Max(0.0, Bias + FMath::Log2(LodScale));
	}

	class FShadowReach final : public FTickableGameObject
	{
	public:
		virtual TStatId GetStatId() const override
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSShadowReach, STATGROUP_Tickables);
		}
		virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
		virtual bool IsTickableWhenPaused() const override { return true; }
		// PIE worlds live in the editor process; ending PIE must also bring the base level back.
		virtual bool IsTickableInEditor() const override { return true; }

		virtual void Tick(float) override
		{
			const double Now = FPlatformTime::Seconds();
			IConsoleVariable* LastLevel = ReachVariables().LastLevel;
			if (Now - LastCheck < ReachCheckSeconds || !LastLevel || !GEngine)
			{
				return;
			}
			LastCheck = Now;

			// A console A/B (r.Shadow.Virtual.Clipmap.LastLevel 25) outranks code: the engine would refuse every later
			// code write with a warning, so the reach stands aside for the rest of the session.
			const int32 Current = LastLevel->GetInt();
			const uint32 SetBy = static_cast<uint32>(LastLevel->GetFlags()) & static_cast<uint32>(ECVF_SetByMask);
			if (SetBy > static_cast<uint32>(ECVF_SetByCode))
			{
				if (!bStoodAside)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow reach stands aside: r.Shadow.Virtual.Clipmap.LastLevel %d ")
						TEXT("was set from the console"), Current);
				}
				bStoodAside = true;
				bRaised = false;
				LowerSince = -1.0;
				return;
			}
			bStoodAside = false;
			if (!bRaised)
			{
				Saved = Current;
			}
			else if (Current != Applied)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow reach: r.Shadow.Virtual.Clipmap.LastLevel changed to %d ")
					TEXT("elsewhere, kept as the base"), Current);
				Saved = Current;
				bRaised = false;
			}
			const int32 Holding = bRaised ? Applied : Saved;

			int32 Wanted = Saved;
			FString Why;
			if (!Want(Now, Holding, Wanted, Why))
			{
				LowerSince = -1.0;
				if (bRaised)
				{
					Apply(*LastLevel, Saved, Why);
				}
			}
			else if (Wanted > Holding)
			{
				LowerSince = -1.0;
				Apply(*LastLevel, Wanted, Why);
			}
			else if (Wanted < Holding)
			{
				if (LowerSince < 0.0)
				{
					LowerSince = Now;
				}
				else if (Now - LowerSince >= ReachLowerAfterSeconds)
				{
					LowerSince = -1.0;
					Apply(*LastLevel, Wanted, Why);
				}
			}
			else
			{
				LowerSince = -1.0;
			}
		}

	private:
		struct FReachBody
		{
			TWeakObjectPtr<const APlanetaryBody> Body;
			double RadiusCm{0.0};
		};

		/** The level this check wants (OutLevel); false when the base level must come back at once. */
		bool Want(double Now, int32 Holding, int32& OutLevel, FString& OutWhy)
		{
			OutLevel = Saved;
			if (CVarSurfaceShadowReach.GetValueOnGameThread() == 0)
			{
				OutWhy = TEXT("aps.Render.SurfaceShadowReach 0");
				return false;
			}
			UWorld* World = nullptr;
			const APlayerController* Controller = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* Candidate = Context.World();
				// A world ending PIE or a travel is torn down: no bodies listed from it, the base level comes back.
				if (Candidate && !Candidate->bIsTearingDown
					&& (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
				{
					Controller = Candidate->GetFirstPlayerController();
					if (Controller)
					{
						World = Candidate;
						break;
					}
				}
			}
			const APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
			if (!World || !Camera)
			{
				OutWhy = TEXT("no game view");
				return false;
			}
			const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(Controller);
			if (Gravity && Gravity->IsStrategicMapOpen())
			{
				OutWhy = TEXT("strategic map");
				return false;
			}

			RefreshBodies(*World, Now);
			const FVector Eye = Camera->GetCameraLocation();
			const APlanetaryBody* Nearest = nullptr;
			double AltitudeCm = TNumericLimits<double>::Max();
			double RadiusCm = 0.0;
			for (const FReachBody& Entry : Bodies)
			{
				const APlanetaryBody* Body = Entry.Body.Get();
				const double Altitude = Body ? FVector::Distance(Eye, Body->GetActorLocation()) - Entry.RadiusCm
					: TNumericLimits<double>::Max();
				if (Altitude < AltitudeCm)
				{
					Nearest = Body;
					AltitudeCm = Altitude;
					RadiusCm = Entry.RadiusCm;
				}
			}
			const double Keep = bRaised ? ReachKeepFactor : 1.0;
			if (!Nearest || AltitudeCm >= ReachAltitudeCm * Keep)
			{
				OutWhy = TEXT("far from bodies");
				return true;
			}
			const UAPSStellarVisualSubsystem* Stellar = World->GetSubsystem<UAPSStellarVisualSubsystem>();
			FVector Star;
			FString StarIdentity;
			if (!Stellar || !Stellar->GetActiveStellarTarget(Star, StarIdentity))
			{
				OutWhy = TEXT("no star");
				return true;
			}
			const FVector Up = (Eye - Nearest->GetActorLocation()).GetSafeNormal();
			const double SunSine = FVector::DotProduct(Up, (Star - Eye).GetSafeNormal());
			const double SunDegrees = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(SunSine, -1.0, 1.0)));
			const double Height = FMath::Max(AltitudeCm, 0.0);
			const double HorizonCm = FMath::Sqrt(Height * (2.0 * RadiusCm + Height)) * ReachHorizonScale
				+ ReachHorizonMarginCm;
			if (SunSine >= ReachSunSine * Keep)
			{
				OutWhy = FString::Printf(TEXT("horizon %.0f km, sun %.0f deg"), HorizonCm * 1.0e-5, SunDegrees);
				return true;
			}
			const double Bias = ReachLodBias(*Controller, *Camera);
			const int32 MaxLevel = FMath::Max(
				FMath::Clamp(CVarSurfaceShadowMaxLevel.GetValueOnGameThread(), 0, ReachHardMaxLevel), Saved);
			// A pixel at distance d samples level floor(log2(d) + bias) and is unshadowed past LastLevel, so the horizon
			// needs LastLevel >= floor(log2(Horizon) + bias) (VirtualShadowMapProjectionDirectional.ush, GetMappedClipmapId).
			const double Exact = FMath::Log2(HorizonCm) + Bias;
			int32 Level = FMath::FloorToInt32(Exact);
			if (Level < Holding && Exact + ReachLowerMarginLevels >= static_cast<double>(Holding))
			{
				Level = Holding;
			}
			OutLevel = FMath::Clamp(Level, Saved, MaxLevel);
			OutWhy = FString::Printf(TEXT("horizon %.0f km, sun %.0f deg, lod bias %.1f"),
				HorizonCm * 1.0e-5, SunDegrees, Bias);
			return true;
		}

		/** The bodies and their radii, listed every 2 s (the checks between only measure distances). */
		void RefreshBodies(UWorld& World, double Now)
		{
			if (BodiesWorld.Get() == &World && Now - BodiesSeconds < ReachBodyListSeconds)
			{
				return;
			}
			BodiesWorld = &World;
			BodiesSeconds = Now;
			Bodies.Reset();
			for (TActorIterator<APlanetaryBody> It(&World); It; ++It)
			{
				const double RadiusCm = IsValid(*It) ? It->GetWorldScapeBodyRadiusCm() : 0.0;
				if (FMath::IsFinite(RadiusCm) && RadiusCm > 1.0)
				{
					FReachBody Entry;
					Entry.Body = *It;
					Entry.RadiusCm = RadiusCm;
					Bodies.Add(Entry);
				}
			}
		}

		void Apply(IConsoleVariable& LastLevel, int32 Level, const FString& Why)
		{
			LastLevel.Set(Level, ECVF_SetByCode);
			Applied = Level;
			bRaised = Level != Saved;
			UE_LOG(LogTemp, Log, TEXT("[APS.Render] shadow reach level %d (%s)"), Level, *Why);
		}

		TArray<FReachBody> Bodies;
		TWeakObjectPtr<UWorld> BodiesWorld;
		double BodiesSeconds{-1000.0};
		double LastCheck{-1000.0};
		double LowerSince{-1.0};
		int32 Saved{22};
		int32 Applied{22};
		bool bRaised{false};
		bool bStoodAside{false};
	};

	/**
	 * Rio 09.10 (0.6.4.2, 4K TV at RENDER SCALE AUTO = 1080p + TSR): stars blink while the camera turns, a still camera is
	 * perfect. The catalogue ISM stars and far glyphs are translucent and reach the screen through TSR (UE 5.4 composites
	 * the separate translucency inside TSRUpdateHistory at input-pixel spacing); below native resolution a 1-2 px star is
	 * hit only in some jitter phases. Still, TSR averages 32 frames (r.TSR.History.SampleCount); once output pixels move
	 * over r.TSR.Velocity.WeightClampingPixelSpeed (1 px a frame) it keeps only r.TSR.Velocity.WeightClampingSampleCount
	 * (engine 4), so each frame's hit or miss shows. Until 09.10 EPIC/CINEMATIC forced native 4K (one sample per pixel).
	 * Below native resolution that count is raised to aps.Render.TsrMotionSamples; at native, or 0, the engine value stays.
	 * A still camera is unchanged (no velocity, no clamp). The GPU points (APS.Stars) are drawn after TSR: not affected.
	 */
	TAutoConsoleVariable<float> CVarTsrMotionSamples(TEXT("aps.Render.TsrMotionSamples"), 16.0f,
		TEXT("Rio 09.10: TSR history samples kept on camera motion while the scene renders below the shown resolution ")
		TEXT("(r.TSR.Velocity.WeightClampingSampleCount, engine 4): sub-pixel stars stop blinking while the camera turns. ")
		TEXT("32 = as still (no clamp), 0 = off (engine value). At native resolution nothing changes."));

	class FTsrMotionSamples final : public FTickableGameObject
	{
	public:
		virtual TStatId GetStatId() const override
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSTsrMotionSamples, STATGROUP_Tickables);
		}
		virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
		virtual bool IsTickableWhenPaused() const override { return true; }
		virtual bool IsTickableInEditor() const override { return true; }

		virtual void Tick(float) override
		{
			const double Now = FPlatformTime::Seconds();
			if (bStoodAside || Now - LastCheck < 0.5 || !GEngine)
			{
				return;
			}
			LastCheck = Now;
			static IConsoleVariable* const Clamp =
				IConsoleManager::Get().FindConsoleVariable(TEXT("r.TSR.Velocity.WeightClampingSampleCount"));
			if (!Clamp)
			{
				return;
			}
			// A console A/B (r.TSR.Velocity.WeightClampingSampleCount 4) outranks code: stand aside for the session.
			const uint32 SetBy = static_cast<uint32>(Clamp->GetFlags()) & static_cast<uint32>(ECVF_SetByMask);
			if (SetBy > static_cast<uint32>(ECVF_SetByCode))
			{
				bStoodAside = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Render] TSR motion samples: set from the console, standing aside"));
				return;
			}
			const float Current = Clamp->GetFloat();
			if (!bHaveSaved)
			{
				Saved = Current;
				bHaveSaved = true;
			}
			double Fraction = 1.0;
			FVector2D Size(0.0, 0.0);
			if (GEngine->GameViewport)
			{
				GEngine->GameViewport->GetViewportSize(Size);
			}
			if (Size.X >= 1.0 && Size.Y >= 1.0)
			{
				Fraction = ReachRenderFraction(ReachVariables(), Size.X, Size.Y);
			}
			const float Target = CVarTsrMotionSamples.GetValueOnGameThread();
			const float Desired = (Target > 0.0f && Fraction < 0.99) ? FMath::Max(Target, Saved) : Saved;
			if (!FMath::IsNearlyEqual(Current, Desired))
			{
				Clamp->Set(Desired, ECVF_SetByCode);
				UE_LOG(LogTemp, Log, TEXT("[APS.Render] TSR motion samples %.1f (render %.0f%% of %.0fx%.0f)"),
					Desired, Fraction * 100.0, Size.X, Size.Y);
			}
		}

	private:
		double LastCheck{-1000.0};
		float Saved{4.0f};
		bool bHaveSaved{false};
		bool bStoodAside{false};
	};

	// Rio 06.10 (audit: static-destruction order): namespace-static FTickableGameObjects would unregister from
	// FTickableStatics at exit after that singleton is gone in a monolithic exe; they live until the process ends instead
	// (nothing is restored at exit, as before: neither had a destructor).
	FGuard* GGuard = nullptr; // never freed: FTickableStatics' singleton dies before namespace statics in a monolithic exe
	FShadowReach* GShadowReach = nullptr; // never freed, as GGuard
	FTsrMotionSamples* GTsrMotionSamples = nullptr; // never freed, as GGuard

	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (!GGuard)
		{
			GGuard = new FGuard();
		}
		if (!GShadowReach)
		{
			GShadowReach = new FShadowReach();
		}
		if (!GTsrMotionSamples)
		{
			GTsrMotionSamples = new FTsrMotionSamples();
		}
	});
}

void APSRenderSafety::MarkCameraJump(const TCHAR* Why)
{
	if (APSRenderSafetyLocal::GGuard)
	{
		APSRenderSafetyLocal::GGuard->MarkRisky(Why);
	}
}
