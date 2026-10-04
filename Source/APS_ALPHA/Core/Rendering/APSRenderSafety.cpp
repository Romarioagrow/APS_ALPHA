#include "APSRenderSafety.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
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

	TUniquePtr<FGuard> GGuard;

	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		GGuard = MakeUnique<FGuard>();
	});
}

void APSRenderSafety::MarkCameraJump(const TCHAR* Why)
{
	if (APSRenderSafetyLocal::GGuard)
	{
		APSRenderSafetyLocal::GGuard->MarkRisky(Why);
	}
}
