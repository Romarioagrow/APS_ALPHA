#include "APSWorldOriginSubsystem.h"

#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace APSWorldOrigin
{
	TAutoConsoleVariable<int32> CVarRebaseOnSpawn(
		TEXT("aps.WorldOrigin.RebaseOnSpawn"), 1,
		TEXT("1 shifts the whole world so the player's spawn point becomes 0,0,0 (engine world origin). ")
		TEXT("0 keeps the generation frame with the headquarters at 0,0,0."));

	TAutoConsoleVariable<float> CVarMinShiftMeters(
		TEXT("aps.WorldOrigin.MinShiftMeters"), 100.0f,
		TEXT("No shift when the target is already closer to 0,0,0 than this, m."));

	/** UWorld::OriginLocation is an FIntVector in cm: about 21 474 km per axis. */
	constexpr double MaximumOriginCm = 2147000000.0;

	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	APawn* PlayerPawn(const UWorld* World)
	{
		const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	FAutoConsoleCommandWithWorld RebaseHereCommand(
		TEXT("aps.WorldOrigin.RebaseHere"),
		TEXT("Shifts the world so the player pawn becomes 0,0,0."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UWorld* GameWorld = FindGameWorld(World);
			APawn* Pawn = PlayerPawn(GameWorld);
			UAPSWorldOriginSubsystem* Origin = GameWorld ? GameWorld->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
			if (Pawn && Origin)
			{
				Origin->RebaseOnto(Pawn->GetActorLocation(), TEXT("console"));
			}
		}));

	FAutoConsoleCommandWithWorld ReportCommand(
		TEXT("aps.WorldOrigin.Report"),
		TEXT("Logs the engine world origin and how far the player pawn is from 0,0,0."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UWorld* GameWorld = FindGameWorld(World);
			const APawn* Pawn = PlayerPawn(GameWorld);
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] report origin=%s pawn=%s (%.3f km from 0,0,0)"),
				GameWorld ? *GameWorld->OriginLocation.ToString() : TEXT("none"),
				Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("none"),
				Pawn ? Pawn->GetActorLocation().Size() / 100000.0 : -1.0);
		}));
}

bool UAPSWorldOriginSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FVector UAPSWorldOriginSubsystem::GetOriginOffset() const
{
	const UWorld* World = GetWorld();
	return World ? FVector(World->OriginLocation) : FVector::ZeroVector;
}

bool UAPSWorldOriginSubsystem::IsStellarCatalogueSettling() const
{
	for (TActorIterator<AAstroGenerator> It(GetWorld()); It; ++It)
	{
		if (!It->GetCanonicalStellarProjectionDescriptor().bFinalized)
		{
			return true;
		}
	}
	return false;
}

bool UAPSWorldOriginSubsystem::RebaseOnto(const FVector& WorldLocation, const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (IsStellarCatalogueSettling())
	{
		// The canonical star catalogue validates its proxy bounds in the frame it was generated in, at the end of level
		// initialisation. The spawn shift used to come first; a surface start moves ~2 500 km, failed those bounds and
		// left the catalogue unused: no stars after a surface spawn (regression of b21445f9, 29.09). The shift now waits
		// for the catalogue (the same frame's initialisation) and then centres on the player where they stand.
		if (!bRebaseDeferred)
		{
			bRebaseDeferred = true;
			DeferredRebaseTicks = 0;
			DeferredRebaseReason = FString::Printf(TEXT("%s, after the star catalogue"), Reason);
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UAPSWorldOriginSubsystem::RetryDeferredRebase));
			UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] defer reason=%s until the star catalogue is final"), Reason);
		}
		return false;
	}
	return RebaseNow(WorldLocation, Reason);
}

void UAPSWorldOriginSubsystem::RetryDeferredRebase()
{
	UWorld* World = GetWorld();
	if (!World || !bRebaseDeferred)
	{
		return;
	}
	// A catalogue that never finalizes (rejected dataset) must not keep the player off-centre: give up after ~2 s.
	if (IsStellarCatalogueSettling() && ++DeferredRebaseTicks < 120)
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UAPSWorldOriginSubsystem::RetryDeferredRebase));
		return;
	}
	bRebaseDeferred = false;
	if (const APawn* Pawn = APSWorldOrigin::PlayerPawn(World))
	{
		RebaseNow(Pawn->GetActorLocation(), *DeferredRebaseReason);
	}
}

bool UAPSWorldOriginSubsystem::RebaseNow(const FVector& WorldLocation, const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const double DistanceCm = WorldLocation.Size();
	if (APSWorldOrigin::CVarRebaseOnSpawn.GetValueOnGameThread() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] skip reason=%s: aps.WorldOrigin.RebaseOnSpawn 0, target %.1f km from 0,0,0"),
			Reason, DistanceCm / 100000.0);
		return false;
	}
	if (DistanceCm < APSWorldOrigin::CVarMinShiftMeters.GetValueOnGameThread() * 100.0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldOrigin] skip reason=%s: target already %.1f m from 0,0,0"),
			Reason, DistanceCm / 100.0);
		return false;
	}

	const FIntVector OldOrigin = World->OriginLocation;
	const FVector Target = FVector(OldOrigin) + WorldLocation;
	if (FMath::Abs(Target.X) > APSWorldOrigin::MaximumOriginCm || FMath::Abs(Target.Y) > APSWorldOrigin::MaximumOriginCm
		|| FMath::Abs(Target.Z) > APSWorldOrigin::MaximumOriginCm)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.WorldOrigin] skip reason=%s: new origin %s is outside the engine's int32 range"),
			Reason, *Target.ToCompactString());
		return false;
	}
	const FIntVector NewOrigin(
		static_cast<int32>(FMath::RoundToDouble(Target.X)),
		static_cast<int32>(FMath::RoundToDouble(Target.Y)),
		static_cast<int32>(FMath::RoundToDouble(Target.Z)));

	const APawn* Pawn = APSWorldOrigin::PlayerPawn(World);
	const FVector PawnBefore = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	const double StartSeconds = FPlatformTime::Seconds();
	if (!World->SetNewWorldOrigin(NewOrigin))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.WorldOrigin] engine refused the shift reason=%s (level visibility request pending)"),
			Reason);
		return false;
	}
	const double ShiftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	const FVector PawnAfter = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldOrigin] rebase reason=%s shift=%.3f km origin %s -> %s | pawn %.3f km -> %.2f m from 0,0,0 (%s) | %.1f ms"),
		Reason, DistanceCm / 100000.0, *OldOrigin.ToString(), *NewOrigin.ToString(), PawnBefore.Size() / 100000.0,
		PawnAfter.Size() / 100.0, *PawnAfter.ToCompactString(), ShiftMs);
	return true;
}
