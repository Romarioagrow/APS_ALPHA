// Rio 09.10 (4K on the RTX 5080, "maximum performance without a visual regression"): the GPU cost of a ship's own lights.
// Packaged 0.6.3 at 4K (TSR 67 %): Rio's L flagship BP_Spaceship_L_P1_08 carries 130 local lights, 12 of them shadowed (rect
// lights M5Light_42..53); flying on its autopilot with the walker inside, ShadowDepths took 21.6 ms (non-Nanite virtual
// shadow map raster 20.9 ms, 695 draws): a moving shadowed local light invalidates all its virtual shadow map pages every frame
// and the (non-Nanite) hull is drawn into them again. Seated, with the chase camera ~800 m behind, ~3 ms.
// Everything here is runtime state of components in game/PIE worlds (no asset is touched), each behind its own switch, and the
// defaults are the build Rio tested:
//   aps.Ship.LightShadowsWhileMoving 0 - a moving ship's own lights stop casting shadows, except the nearest to the camera.
//   aps.Ship.LightCullDistanceM N      - a ship's lights without a max draw distance get one (camera far outside the hull).
//   aps.Ship.LightsReport              - every local light of the piloted or boarded ship.

#include "Spaceship.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DelayedAutoRegister.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/ObjectKey.h"

namespace APSShipLightBudget
{
	// Rio 09.10 evening ("these lamp shadows eat performance: delete or hide them, sacrifice them for stable flights"):
	// default 0 with KeepNearest 0 = no ship-light shadows while the ship moves; they come back at rest.
	TAutoConsoleVariable<int32> CVarLightShadowsWhileMoving(
		TEXT("aps.Ship.LightShadowsWhileMoving"), 0,
		TEXT("Rio 09.10 (4K: the L flagship's 12 shadowed interior lights cost ~21 ms of virtual shadow map raster while it flies ")
		TEXT("with the walker aboard; a moving shadowed local light redraws all its shadow pages every frame). 1: ship ")
		TEXT("lights keep their authored shadows, as before. 0: while a ship moves (the hull collision's notion: faster than ")
		TEXT("aps.Ship.HullRestoreMaxSpeedCm, its autopilot, a world flow, owed travel or a fleet order) its own movable local lights ")
		TEXT("(point/spot/rect components of the ship and of the actors attached to it; never a walker's, another ship's, a ")
		TEXT("directional or a sky light) stop casting shadows, except the aps.Ship.LightShadowsKeepNearest nearest to the player's ")
		TEXT("camera; each gets its authored shadows back once the ship has rested aps.Ship.HullRestoreRestSeconds."));
	TAutoConsoleVariable<float> CVarLightShadowsHoldSeconds(
		TEXT("aps.Ship.LightShadowsHoldSeconds"), 60.0f,
		TEXT("Rio 09.10 late: once a moving ship's lights lost their shadows, they stay off this many seconds after its last motion ")
		TEXT("(no off/on flicker while the motion notion flickers in the world flow). 0: back after aps.Ship.HullRestoreRestSeconds."));
	TAutoConsoleVariable<int32> CVarLightShadowsKeepNearest(
		TEXT("aps.Ship.LightShadowsKeepNearest"), 0,
		TEXT("Rio 09.10 (aps.Ship.LightShadowsWhileMoving 0): this many of a moving ship's authored-shadowed, visible lights nearest ")
		TEXT("to the player's camera keep their shadows (ranked again every 0.25 s; a kept light gives way only to one ~16 % nearer)."));
	TAutoConsoleVariable<float> CVarLightCullDistanceM(
		TEXT("aps.Ship.LightCullDistanceM"), 0.0f,
		TEXT("Rio 09.10 (4K: the seated chase camera is ~800 m behind the L flagship): > 0 gives every movable local light of a ship ")
		TEXT("that has no max draw distance of its own this one, in metres, fading out over its last 20 %, so interior lights stop ")
		TEXT("costing when the camera is far outside the hull (authored distances and the pilot's camera fills stay as they are). ")
		TEXT("0 (default): none, and the authored values come back."));

	/** Real seconds between two passes over the ships, and between two refreshes of one ship's light list. */
	constexpr float TickSeconds = 0.25f;
	constexpr double RefreshSeconds = 2.0;
	/** Squared camera distance of a light kept last pass is scaled by this: another must be ~16 % nearer to take its place. */
	constexpr double KeptBias = 0.7;
	constexpr float CullFadeFraction = 0.2f;
}

/** A friend of ASpaceship (its motion notion GetHullMotionReason, its hull and its on-foot test). Game thread only. */
class FAPSShipLightBudget
{
public:
	static bool Tick(float DeltaTime);
	static void Report(const TArray<FString>& Args, UWorld* World);

private:
	struct FLight
	{
		TWeakObjectPtr<ULocalLightComponent> Light;
		float AuthoredMaxDrawDistance{0.0f};
		float AuthoredFadeRange{0.0f};
		bool bAuthoredShadows{false};
		bool bMovable{false};
		/** The pilot's camera fills (ASpaceship::PilotFillLight/PointLight): never culled. */
		bool bPilotFill{false};
		/** This budget switched its shadows off / gave it a max draw distance. */
		bool bShadowCut{false};
		bool bCullApplied{false};
		/** Among the nearest kept shadowed on the last pass. */
		bool bKept{false};
	};
	struct FShip
	{
		TWeakObjectPtr<ASpaceship> Ship;
		TArray<FLight> Lights;
		double NextRefreshSeconds{0.0};
		double LastMotionWorldSeconds{-1.0e9};
		bool bMoving{false};
		float AppliedCullCm{0.0f};
	};

	static TMap<FObjectKey, FShip>& Ships()
	{
		static TMap<FObjectKey, FShip> Map;
		return Map;
	}
	static bool IsShipsOwn(const ASpaceship& Ship, const AActor& Actor);
	static void Refresh(ASpaceship& Ship, FShip& State);
	static void Update(ASpaceship& Ship, FShip& State, bool bHasCamera, const FVector& Camera, double WorldSeconds,
		bool bCutFeature, float CullCm);
	static void CutShadows(FShip& State, bool bHasCamera, const FVector& Camera, int32 KeepNearest);
	static void RestoreShadows(FShip& State);
	static void ApplyCull(const ASpaceship& Ship, FShip& State, float CullCm);
	static void RestoreCull(FLight& Entry, ULocalLightComponent& Light);
	static int32 CountShadowed(const FShip& State);
	static int32 CountAuthoredShadowed(const FShip& State, int32* OutFixed = nullptr);
	static bool HasChanges(const FShip& State);
	static UWorld* FindPlayWorld(UWorld* Hint);
	static ASpaceship* FindPlayerShip(UWorld& World);
};

bool FAPSShipLightBudget::IsShipsOwn(const ASpaceship& Ship, const AActor& Actor)
{
	// A walker (the player's character), a vehicle carried in a bay or another ship keeps its lights (and what hangs on it).
	for (const AActor* Link = &Actor; Link && Link != &Ship; Link = Link->GetAttachParentActor())
	{
		if (Link->IsA<APawn>())
		{
			return false;
		}
	}
	return true;
}

void FAPSShipLightBudget::Refresh(ASpaceship& Ship, FShip& State)
{
	// The same set as the bench's [APS.ShipReport] lights= count: the ship and every actor attached to it, recursively.
	TArray<AActor*> Actors{&Ship};
	Ship.GetAttachedActors(Actors, false, true);
	TSet<ULocalLightComponent*> Found;
	for (const AActor* Actor : Actors)
	{
		if (!IsValid(Actor) || !IsShipsOwn(Ship, *Actor))
		{
			continue;
		}
		TInlineComponentArray<ULocalLightComponent*> Lights(Actor);
		for (ULocalLightComponent* Light : Lights)
		{
			if (IsValid(Light))
			{
				Found.Add(Light);
			}
		}
	}
	for (int32 Index = State.Lights.Num() - 1; Index >= 0; --Index)
	{
		FLight& Entry = State.Lights[Index];
		ULocalLightComponent* Light = Entry.Light.Get();
		if (IsValid(Light) && Found.Remove(Light) > 0)
		{
			continue;
		}
		if (IsValid(Light))
		{
			// No longer the ship's (detached, or now on a walker): what was changed on it goes back.
			if (Entry.bShadowCut)
			{
				Light->SetCastShadows(Entry.bAuthoredShadows);
			}
			if (Entry.bCullApplied)
			{
				RestoreCull(Entry, *Light);
			}
		}
		State.Lights.RemoveAtSwap(Index);
	}
	for (ULocalLightComponent* Light : Found)
	{
		FLight& Entry = State.Lights.AddDefaulted_GetRef();
		Entry.Light = Light;
		Entry.bAuthoredShadows = Light->CastShadows != 0;
		Entry.AuthoredMaxDrawDistance = Light->MaxDrawDistance;
		Entry.AuthoredFadeRange = Light->MaxDistanceFadeRange;
		Entry.bMovable = Light->Mobility == EComponentMobility::Movable;
		Entry.bPilotFill = Light == Ship.PilotFillLight || Light == Ship.PilotFillPointLight;
	}
}

int32 FAPSShipLightBudget::CountShadowed(const FShip& State)
{
	// As [APS.ShipReport] shadowed=: visible lights casting shadows.
	int32 Count = 0;
	for (const FLight& Entry : State.Lights)
	{
		const ULocalLightComponent* Light = Entry.Light.Get();
		Count += IsValid(Light) && Light->IsVisible() && Light->CastShadows ? 1 : 0;
	}
	return Count;
}

int32 FAPSShipLightBudget::CountAuthoredShadowed(const FShip& State, int32* OutFixed)
{
	int32 Count = 0;
	int32 Fixed = 0;
	for (const FLight& Entry : State.Lights)
	{
		if (Entry.bAuthoredShadows && Entry.Light.IsValid())
		{
			++Count;
			Fixed += Entry.bMovable ? 0 : 1;
		}
	}
	if (OutFixed)
	{
		*OutFixed = Fixed;
	}
	return Count;
}

bool FAPSShipLightBudget::HasChanges(const FShip& State)
{
	if (State.bMoving || State.AppliedCullCm > 0.0f)
	{
		return true;
	}
	for (const FLight& Entry : State.Lights)
	{
		if (Entry.bShadowCut || Entry.bCullApplied)
		{
			return true;
		}
	}
	return false;
}

void FAPSShipLightBudget::CutShadows(FShip& State, const bool bHasCamera, const FVector& Camera, const int32 KeepNearest)
{
	struct FCandidate
	{
		int32 Index;
		double Score;
	};
	TArray<FCandidate, TInlineAllocator<32>> Candidates;
	if (bHasCamera && KeepNearest > 0)
	{
		for (int32 Index = 0; Index < State.Lights.Num(); ++Index)
		{
			const FLight& Entry = State.Lights[Index];
			const ULocalLightComponent* Light = Entry.Light.Get();
			if (Entry.bAuthoredShadows && Entry.bMovable && IsValid(Light) && Light->IsVisible() && Light->bAffectsWorld)
			{
				Candidates.Add({Index, FVector::DistSquared(Camera, Light->GetComponentLocation()) * (Entry.bKept ? APSShipLightBudget::KeptBias : 1.0)});
			}
		}
		Candidates.Sort([](const FCandidate& Left, const FCandidate& Right) { return Left.Score < Right.Score; });
	}
	TBitArray<> Keep(false, State.Lights.Num());
	for (int32 Rank = 0; Rank < FMath::Min(KeepNearest, Candidates.Num()); ++Rank)
	{
		Keep[Candidates[Rank].Index] = true;
	}
	for (int32 Index = 0; Index < State.Lights.Num(); ++Index)
	{
		FLight& Entry = State.Lights[Index];
		ULocalLightComponent* Light = Entry.Light.Get();
		// Static or stationary lights (none on the ships scanned) are left as they are: their shadows are not the moving kind.
		if (!Entry.bAuthoredShadows || !Entry.bMovable || !IsValid(Light))
		{
			continue;
		}
		Entry.bKept = Keep[Index];
		if (Entry.bKept)
		{
			if (Entry.bShadowCut)
			{
				Light->SetCastShadows(true);
				Entry.bShadowCut = false;
			}
		}
		else if (!Entry.bShadowCut && Light->CastShadows)
		{
			Light->SetCastShadows(false);
			Entry.bShadowCut = true;
		}
	}
}

void FAPSShipLightBudget::RestoreShadows(FShip& State)
{
	for (FLight& Entry : State.Lights)
	{
		ULocalLightComponent* Light = Entry.Light.Get();
		if (Entry.bShadowCut && IsValid(Light))
		{
			Light->SetCastShadows(Entry.bAuthoredShadows);
		}
		Entry.bShadowCut = false;
		Entry.bKept = false;
	}
}

void FAPSShipLightBudget::RestoreCull(FLight& Entry, ULocalLightComponent& Light)
{
	Light.MaxDrawDistance = Entry.AuthoredMaxDrawDistance;
	Light.MaxDistanceFadeRange = Entry.AuthoredFadeRange;
	Light.MarkRenderStateDirty();
	Entry.bCullApplied = false;
}

void FAPSShipLightBudget::ApplyCull(const ASpaceship& Ship, FShip& State, const float CullCm)
{
	// The renderer reads both values when it builds the light's scene proxy (FLightSceneProxy), hence the dirty mark. A
	// fade range of 0 would divide by zero in GetLightFadeFactor.
	const float FadeCm = FMath::Max(CullCm * APSShipLightBudget::CullFadeFraction, 100.0f);
	int32 Applied = 0;
	for (FLight& Entry : State.Lights)
	{
		ULocalLightComponent* Light = Entry.Light.Get();
		if (!IsValid(Light))
		{
			continue;
		}
		if (CullCm > 0.0f && Entry.bMovable && !Entry.bPilotFill && Entry.AuthoredMaxDrawDistance <= 0.0f)
		{
			if (!Entry.bCullApplied || Light->MaxDrawDistance != CullCm || Light->MaxDistanceFadeRange != FadeCm)
			{
				Light->MaxDrawDistance = CullCm;
				Light->MaxDistanceFadeRange = FadeCm;
				Light->MarkRenderStateDirty();
				Entry.bCullApplied = true;
			}
			++Applied;
		}
		else if (Entry.bCullApplied)
		{
			RestoreCull(Entry, *Light);
		}
	}
	if (State.AppliedCullCm != CullCm)
	{
		if (CullCm > 0.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipLights] %s: %d lights, %d get a max draw distance of %.0f m (fade over the last %.0f m)"),
				*Ship.GetName(), State.Lights.Num(), Applied, CullCm / 100.0, FadeCm / 100.0);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipLights] %s: %d lights, authored max draw distances restored"), *Ship.GetName(),
				State.Lights.Num());
		}
		State.AppliedCullCm = CullCm;
	}
}

void FAPSShipLightBudget::Update(ASpaceship& Ship, FShip& State, const bool bHasCamera, const FVector& Camera,
	const double WorldSeconds, const bool bCutFeature, const float CullCm)
{
	const double Now = FPlatformTime::Seconds();
	if ((bCutFeature || CullCm > 0.0f) && Now >= State.NextRefreshSeconds)
	{
		// Components can be added later (a fitted module, a spawned fixture): the list is gathered again now and then,
		// spread over the ships.
		Refresh(Ship, State);
		State.NextRefreshSeconds = Now + APSShipLightBudget::RefreshSeconds * FMath::FRandRange(0.75, 1.25);
	}

	const TCHAR* Motion = bCutFeature ? Ship.GetHullMotionReason() : nullptr;
	if (Motion)
	{
		State.LastMotionWorldSeconds = WorldSeconds;
	}
	static const IConsoleVariable* const RestCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.HullRestoreRestSeconds"));
	// Rio 09.10 late (0.6.4.3: "the material glitches back and forth at medium speed"): the motion notion flickers in the
	// world flow, so the shadows went off and on; once cut they now stay cut for aps.Ship.LightShadowsHoldSeconds.
	static IConsoleVariable* HoldCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Ship.LightShadowsHoldSeconds"));
	const double RestSeconds = FMath::Max(RestCVar ? FMath::Max(RestCVar->GetFloat(), 0.0f) : 1.0f,
		HoldCVar ? FMath::Max(HoldCVar->GetFloat(), 0.0f) : 0.0f);
	const bool bMoving = bCutFeature
		&& (Motion || (State.bMoving && WorldSeconds - State.LastMotionWorldSeconds < RestSeconds));
	if (bMoving)
	{
		const int32 Before = CountShadowed(State);
		CutShadows(State, bHasCamera, Camera, FMath::Max(APSShipLightBudget::CVarLightShadowsKeepNearest.GetValueOnGameThread(), 0));
		int32 Fixed = 0;
		if (!State.bMoving && CountAuthoredShadowed(State, &Fixed) > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipLights] %s: %d lights, %d shadowed -> %d shadowed while moving (%s; keep nearest %d%s)"),
				*Ship.GetName(), State.Lights.Num(), Before, CountShadowed(State), Motion ? Motion : TEXT("moving"),
				APSShipLightBudget::CVarLightShadowsKeepNearest.GetValueOnGameThread(),
				Fixed > 0 ? *FString::Printf(TEXT(", %d fixed lights left as authored"), Fixed) : TEXT(""));
		}
		State.bMoving = true;
	}
	else if (State.bMoving)
	{
		const int32 Before = CountShadowed(State);
		RestoreShadows(State);
		State.bMoving = false;
		if (CountAuthoredShadowed(State) > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipLights] %s: %d lights, %d shadowed -> %d shadowed, restored %s"), *Ship.GetName(),
				State.Lights.Num(), Before, CountShadowed(State),
				bCutFeature ? TEXT("at rest") : TEXT("(aps.Ship.LightShadowsWhileMoving 1)"));
		}
	}

	if (CullCm > 0.0f || State.AppliedCullCm > 0.0f)
	{
		ApplyCull(Ship, State, CullCm);
	}
}

bool FAPSShipLightBudget::Tick(float)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_ShipLightBudget);
	const bool bCutFeature = APSShipLightBudget::CVarLightShadowsWhileMoving.GetValueOnGameThread() == 0;
	const float CullCm = FMath::Max(APSShipLightBudget::CVarLightCullDistanceM.GetValueOnGameThread(), 0.0f) * 100.0f;
	const bool bActive = bCutFeature || CullCm > 0.0f;
	TMap<FObjectKey, FShip>& States = Ships();
	if ((!bActive && States.IsEmpty()) || !GEngine)
	{
		// The defaults with nothing left to give back: today's behaviour, no work.
		return true;
	}
	TSet<FObjectKey> Seen;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World || (Context.WorldType != EWorldType::Game && Context.WorldType != EWorldType::PIE) || World->bIsTearingDown
			|| !World->HasBegunPlay())
		{
			continue;
		}
		FVector Camera = FVector::ZeroVector;
		bool bHasCamera = false;
		if (const APlayerController* Player = World->GetFirstPlayerController(); Player && Player->PlayerCameraManager)
		{
			Camera = Player->PlayerCameraManager->GetCameraLocation();
			bHasCamera = true;
		}
		for (TActorIterator<ASpaceship> It(World); It; ++It)
		{
			ASpaceship* Ship = *It;
			if (!IsValid(Ship) || Ship->IsActorBeingDestroyed())
			{
				continue;
			}
			const FObjectKey Key(Ship);
			FShip* State = States.Find(Key);
			if (!State)
			{
				if (!bActive)
				{
					continue;
				}
				State = &States.Add(Key);
				State->Ship = Ship;
			}
			Seen.Add(Key);
			Update(*Ship, *State, bHasCamera, Camera, World->GetTimeSeconds(), bCutFeature, CullCm);
		}
	}
	for (auto It = States.CreateIterator(); It; ++It)
	{
		// Ships gone (destroyed, their world torn down) have nothing left to give back; with both switches at their defaults a
		// ship whose lights are all back as authored is forgotten.
		if (!Seen.Contains(It.Key()) || !It.Value().Ship.IsValid() || (!bActive && !HasChanges(It.Value())))
		{
			It.RemoveCurrent();
		}
	}
	return true;
}

UWorld* FAPSShipLightBudget::FindPlayWorld(UWorld* Hint)
{
	// The console may pass the editor world while the ship flies in PIE (as aps.Ship.Benchmark).
	if (Hint && (Hint->WorldType == EWorldType::Game || Hint->WorldType == EWorldType::PIE))
	{
		return Hint;
	}
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				return Context.World();
			}
		}
	}
	return nullptr;
}

ASpaceship* FAPSShipLightBudget::FindPlayerShip(UWorld& World)
{
	const APlayerController* Player = World.GetFirstPlayerController();
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}
	// Piloted, or a walker attached aboard.
	for (AActor* Link = Pawn; Link; Link = Link->GetAttachParentActor())
	{
		if (ASpaceship* Ship = Cast<ASpaceship>(Link))
		{
			return Ship;
		}
	}
	// On foot and not attached (a parked ship's deck): the ship whose hull sphere holds him, the nearest if several.
	ASpaceship* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<ASpaceship> It(&World); It; ++It)
	{
		const UPrimitiveComponent* Hull = It->GetPrimaryHullComponent();
		if (!Hull || !It->IsPlayerOnFootNear(0.0))
		{
			continue;
		}
		const double Distance = FVector::DistSquared(Pawn->GetActorLocation(), Hull->Bounds.Origin);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	return Best;
}

void FAPSShipLightBudget::Report(const TArray<FString>& Args, UWorld* InWorld)
{
	UWorld* World = FindPlayWorld(InWorld);
	ASpaceship* Ship = World ? FindPlayerShip(*World) : nullptr;
	if (!Ship)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.ShipLights] report: no piloted or boarded ship (fly one, or stand aboard)."));
		return;
	}
	FVector Camera = Ship->GetActorLocation();
	if (const APlayerController* Player = World->GetFirstPlayerController(); Player && Player->PlayerCameraManager)
	{
		Camera = Player->PlayerCameraManager->GetCameraLocation();
	}
	// The budget's own list when it tracks the ship (its authored values and what it changed), a fresh one otherwise.
	FShip Fresh;
	const FShip* State = Ships().Find(FObjectKey(Ship));
	if (!State)
	{
		Refresh(*Ship, Fresh);
		State = &Fresh;
	}

	TArray<int32> Order;
	int32 Points = 0, Spots = 0, Rects = 0, Visible = 0, Shadowed = 0, Authored = 0, Cut = 0, Culled = 0;
	for (int32 Index = 0; Index < State->Lights.Num(); ++Index)
	{
		const FLight& Entry = State->Lights[Index];
		const ULocalLightComponent* Light = Entry.Light.Get();
		if (!IsValid(Light))
		{
			continue;
		}
		Order.Add(Index);
		Spots += Light->IsA<USpotLightComponent>() ? 1 : 0;
		Points += Light->IsA<UPointLightComponent>() && !Light->IsA<USpotLightComponent>() ? 1 : 0;
		Rects += Light->IsA<URectLightComponent>() ? 1 : 0;
		Visible += Light->IsVisible() ? 1 : 0;
		Shadowed += Light->IsVisible() && Light->CastShadows ? 1 : 0;
		Authored += Entry.bAuthoredShadows ? 1 : 0;
		Cut += Entry.bShadowCut ? 1 : 0;
		Culled += Entry.bCullApplied ? 1 : 0;
	}
	// Authored-shadowed lights first, then by distance to the camera.
	Order.Sort([State, &Camera](const int32 Left, const int32 Right)
	{
		const FLight& A = State->Lights[Left];
		const FLight& B = State->Lights[Right];
		if (A.bAuthoredShadows != B.bAuthoredShadows)
		{
			return A.bAuthoredShadows;
		}
		return FVector::DistSquared(Camera, A.Light->GetComponentLocation()) < FVector::DistSquared(Camera, B.Light->GetComponentLocation());
	});
	const TCHAR* Motion = Ship->GetHullMotionReason();
	UE_LOG(LogTemp, Log,
		TEXT("[APS.ShipLights] report %s (%s): %d local lights (point %d, spot %d, rect %d), visible %d, shadowed %d now / %d ")
		TEXT("authored, %d cut while moving, %d with a budget draw distance; motion: %s; LightShadowsWhileMoving=%d ")
		TEXT("LightShadowsKeepNearest=%d LightCullDistanceM=%.0f"),
		*Ship->GetName(), *Ship->GetClass()->GetName(), Order.Num(), Points, Spots, Rects, Visible, Shadowed, Authored, Cut,
		Culled, Motion ? Motion : TEXT("at rest"), APSShipLightBudget::CVarLightShadowsWhileMoving.GetValueOnGameThread(),
		APSShipLightBudget::CVarLightShadowsKeepNearest.GetValueOnGameThread(),
		APSShipLightBudget::CVarLightCullDistanceM.GetValueOnGameThread());
	for (const int32 Index : Order)
	{
		const FLight& Entry = State->Lights[Index];
		const ULocalLightComponent* Light = Entry.Light.Get();
		const TCHAR* Kind = Light->IsA<USpotLightComponent>() ? TEXT("Spot")
			: Light->IsA<UPointLightComponent>() ? TEXT("Point")
			: Light->IsA<URectLightComponent>() ? TEXT("Rect") : TEXT("Local");
		const TCHAR* Units = TEXT("?");
		switch (Light->IntensityUnits)
		{
		case ELightUnits::Unitless: Units = TEXT("unitless"); break;
		case ELightUnits::Candelas: Units = TEXT("cd"); break;
		case ELightUnits::Lumens: Units = TEXT("lm"); break;
		case ELightUnits::EV: Units = TEXT("EV"); break;
		default: break;
		}
		const TCHAR* Mobility = Light->Mobility == EComponentMobility::Movable ? TEXT("Movable")
			: Light->Mobility == EComponentMobility::Stationary ? TEXT("Stationary") : TEXT("Static");
		const AActor* Owner = Light->GetOwner();
		UE_LOG(LogTemp, Log,
			TEXT("[APS.ShipLights]   %s %s.%s (%s) intensity=%.2f %s radius=%.1f m shadows=%d (authored %d%s%s) visible=%d ")
			TEXT("maxDraw=%.0f m fade=%.0f m%s mobility=%s dist=%.1f m"),
			Kind, Owner ? *Owner->GetName() : TEXT("?"), *Light->GetName(), *Light->GetClass()->GetName(), Light->Intensity, Units,
			Light->AttenuationRadius / 100.0, Light->CastShadows ? 1 : 0, Entry.bAuthoredShadows ? 1 : 0,
			Entry.bShadowCut ? TEXT(", cut while moving") : TEXT(""), Entry.bKept ? TEXT(", kept nearest") : TEXT(""),
			Light->IsVisible() ? 1 : 0, Light->MaxDrawDistance / 100.0, Light->MaxDistanceFadeRange / 100.0,
			Entry.bPilotFill ? TEXT(" pilotFill") : TEXT(""), Mobility,
			FVector::Dist(Camera, Light->GetComponentLocation()) / 100.0);
	}
}

namespace APSShipLightBudget
{
	FDelayedAutoRegisterHelper GLightBudgetTickerRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&FAPSShipLightBudget::Tick), TickSeconds);
	});
	FAutoConsoleCommandWithWorldAndArgs GLightsReportCommand(TEXT("aps.Ship.LightsReport"),
		TEXT("Rio 09.10: logs every local light of the piloted or boarded ship ([APS.ShipLights]): class, component, intensity, ")
		TEXT("attenuation radius, shadows now and authored, what aps.Ship.LightShadowsWhileMoving / LightCullDistanceM changed, ")
		TEXT("max draw distance, mobility and distance to the camera."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FAPSShipLightBudget::Report));
}
