// Scripted keyboard input for the player's gravity character and per-frame motion metrics, for comparing how smoothly
// pilots turn and run (the "jerky turns at speed" report of 29.09).
//
// aps.Char.Probe <run|slalom|reverse> [duration=12] [sprint=1] [mode=0|1|2|3] [period=0.7]
//   Presses the real keys through the PlayerController (W, A/D, S, LeftShift, 1-3), so Enhanced Input, the legacy
//   sprint action and the character's own handlers run exactly as for a player. Logs facing yaw rate/acceleration,
//   ground speed dips, camera acceleration and the locomotion values the animation blueprint reads, and writes a
//   CSV to Saved/Diagnostics/CharacterProbe.
// aps.Char.AutoProbe <pattern> [map=Substring] [warmup=30] [quit] [probe options]: waits for the player's gravity
//   character (in that map, released from any spawn hold), lets streaming settle, runs the probe, optionally quits.
//   ("duration", not "seconds": the engine reads any "seconds=" on the command line, -ExecCmds included, as -benchmark
//   run time and exits.)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

namespace APSCharacterProbe
{
	struct FSample
	{
		double Time{0.0};
		double YawDegrees{0.0};
		double GroundSpeed{0.0};
		FVector Camera{FVector::ZeroVector};
		double CameraLag{0.0};
		double AnimGroundSpeed{-1.0};
		int32 AnimShouldMove{-1};
		double PlayRate{1.0};
		bool bTurnPhase{false};
	};

	struct FProbe
	{
		bool bActive{false};
		TWeakObjectPtr<ACustomGravityCharacter> Character;
		TWeakObjectPtr<APlayerController> Controller;
		FString Pattern{TEXT("slalom")};
		double Seconds{12.0};
		bool bSprint{true};
		int32 Mode{0};
		double Period{0.7};
		/** look=: controller yaw input per second, alternating direction every 2 s (mouse look while moving). */
		double LookRate{0.0};
		/** shot=1: in the last second, tilt the view one way and the other and capture both (the sky check). */
		bool bShots{false};
		int32 ShotsTaken{0};
		double Elapsed{0.0};
		TArray<FKey> Held;
		TArray<FSample> Samples;
		/** Every frame's time, ms: the stutter report is about uneven frames, not the average. */
		TArray<double> FrameMs;
		/** Unreal Insights region per second (CharProbe_000...), for per-second timer maxima. */
		FString Region;
		int32 RegionSecond{-1};
		FVector ReferenceUp{FVector::UpVector};
		FVector ReferenceForward{FVector::ForwardVector};
		double LastRawYaw{0.0};
		double YawUnwrap{0.0};
		bool bHasYaw{false};
		FTSTicker::FDelegateHandle Ticker;
	};
	FProbe GProbe;

	struct FAutoProbe
	{
		bool bActive{false};
		FString Map;
		double WarmupSeconds{30.0};
		bool bQuit{false};
		TArray<FString> ProbeArgs;
		double StartSeconds{0.0};
		double ReadySeconds{0.0};
		bool bStarted{false};
		FTSTicker::FDelegateHandle Ticker;
	};
	FAutoProbe GAutoProbe;

	APlayerController* FindPlayerController()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
			{
				if (APlayerController* Controller = Context.World()->GetFirstPlayerController())
				{
					return Controller;
				}
			}
		}
		return nullptr;
	}

	double Percentile(TArray<double> Values, double Fraction)
	{
		if (Values.IsEmpty())
		{
			return 0.0;
		}
		Values.Sort();
		return Values[FMath::Clamp(FMath::CeilToInt(Fraction * Values.Num()) - 1, 0, Values.Num() - 1)];
	}

	void SetHeld(APlayerController& Controller, const TArray<FKey>& Wanted)
	{
		for (int32 Index = GProbe.Held.Num() - 1; Index >= 0; --Index)
		{
			if (!Wanted.Contains(GProbe.Held[Index]))
			{
				Controller.InputKey(FInputKeyParams(GProbe.Held[Index], IE_Released, 0.0));
				GProbe.Held.RemoveAt(Index);
			}
		}
		for (const FKey& Key : Wanted)
		{
			if (!GProbe.Held.Contains(Key))
			{
				Controller.InputKey(FInputKeyParams(Key, IE_Pressed, 1.0));
				GProbe.Held.Add(Key);
			}
		}
	}

	/** Keys held at time T of the pattern; the turn phase is what the turn metrics are computed over. */
	TArray<FKey> KeysAt(double Time, bool& bOutTurnPhase)
	{
		TArray<FKey> Keys;
		bOutTurnPhase = false;
		if (Time < 0.5)
		{
			return Keys;
		}
		if (GProbe.bSprint)
		{
			Keys.Add(EKeys::LeftShift);
		}
		if (GProbe.Pattern.Equals(TEXT("reverse"), ESearchCase::IgnoreCase))
		{
			// Forward, then a full reversal every period*4 once up to speed.
			const double Phase = Time - 3.0;
			const bool bBackwards = Phase > 0.0 && FMath::FloorToInt(Phase / (GProbe.Period * 4.0)) % 2 == 0;
			Keys.Add(bBackwards ? EKeys::S : EKeys::W);
			bOutTurnPhase = Phase > 0.0;
			return Keys;
		}
		Keys.Add(EKeys::W);
		// A straight run is measured once up to speed.
		bOutTurnPhase = Time >= 3.0;
		if (GProbe.Pattern.Equals(TEXT("slalom"), ESearchCase::IgnoreCase) && Time >= 3.0)
		{
			// W+A / W+D alternate: the input direction swings 90 degrees every period, as when weaving at speed.
			const int32 Leg = FMath::FloorToInt((Time - 3.0) / GProbe.Period);
			Keys.Add(Leg % 2 == 0 ? EKeys::A : EKeys::D);
			bOutTurnPhase = true;
		}
		return Keys;
	}

	void ReadAnimation(ACustomGravityCharacter& Character, FSample& Sample)
	{
		USkeletalMeshComponent* Mesh = Character.GetMesh();
		UAnimInstance* Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
		if (!Anim)
		{
			return;
		}
		Sample.PlayRate = Mesh->GlobalAnimRateScale;
		if (const FNumericProperty* Speed = CastField<FNumericProperty>(FindFProperty<FProperty>(Anim->GetClass(), TEXT("GroundSpeed"))))
		{
			const void* Address = Speed->ContainerPtrToValuePtr<void>(Anim);
			Sample.AnimGroundSpeed = Speed->IsFloatingPoint()
				? Speed->GetFloatingPointPropertyValue(Address)
				: static_cast<double>(Speed->GetSignedIntPropertyValue(Address));
		}
		if (const FBoolProperty* ShouldMove = CastField<FBoolProperty>(FindFProperty<FProperty>(Anim->GetClass(), TEXT("ShouldMove"))))
		{
			Sample.AnimShouldMove = ShouldMove->GetPropertyValue_InContainer(Anim) ? 1 : 0;
		}
	}

	void Finish(const TCHAR* Reason)
	{
		if (!GProbe.bActive)
		{
			return;
		}
		if (APlayerController* Controller = GProbe.Controller.Get())
		{
			SetHeld(*Controller, TArray<FKey>());
		}
		FTSTicker::GetCoreTicker().RemoveTicker(GProbe.Ticker);
		GProbe.bActive = false;
		if (!GProbe.Region.IsEmpty())
		{
			TRACE_END_REGION(*GProbe.Region);
			GProbe.Region.Reset();
		}

		// Rates from consecutive samples; only the turn phase counts for the turn metrics.
		TArray<double> YawAcceleration, YawRate, CameraAcceleration, AnimSpeedRate, TurnSpeeds;
		int32 ShouldMoveDrops = 0;
		double PlayRateMin = TNumericLimits<double>::Max();
		double PlayRateMax = 0.0;
		double LagMax = 0.0;
		const TArray<FSample>& S = GProbe.Samples;
		for (int32 Index = 2; Index < S.Num(); ++Index)
		{
			if (!S[Index].bTurnPhase)
			{
				continue;
			}
			const double Dt1 = S[Index].Time - S[Index - 1].Time;
			const double Dt0 = S[Index - 1].Time - S[Index - 2].Time;
			if (Dt1 <= UE_SMALL_NUMBER || Dt0 <= UE_SMALL_NUMBER)
			{
				continue;
			}
			const double Rate1 = (S[Index].YawDegrees - S[Index - 1].YawDegrees) / Dt1;
			const double Rate0 = (S[Index - 1].YawDegrees - S[Index - 2].YawDegrees) / Dt0;
			YawRate.Add(FMath::Abs(Rate1));
			YawAcceleration.Add(FMath::Abs(Rate1 - Rate0) / (0.5 * (Dt0 + Dt1)));
			const FVector Velocity1 = (S[Index].Camera - S[Index - 1].Camera) / Dt1;
			const FVector Velocity0 = (S[Index - 1].Camera - S[Index - 2].Camera) / Dt0;
			CameraAcceleration.Add((Velocity1 - Velocity0).Size() / (0.5 * (Dt0 + Dt1)));
			if (S[Index].AnimGroundSpeed >= 0.0 && S[Index - 1].AnimGroundSpeed >= 0.0)
			{
				AnimSpeedRate.Add(FMath::Abs(S[Index].AnimGroundSpeed - S[Index - 1].AnimGroundSpeed) / Dt1);
			}
			TurnSpeeds.Add(S[Index].GroundSpeed);
			ShouldMoveDrops += S[Index].AnimShouldMove == 0 && S[Index].GroundSpeed > 20.0 ? 1 : 0;
			PlayRateMin = FMath::Min(PlayRateMin, S[Index].PlayRate);
			PlayRateMax = FMath::Max(PlayRateMax, S[Index].PlayRate);
			LagMax = FMath::Max(LagMax, S[Index].CameraLag);
		}
		double SpeedMean = 0.0;
		for (const double Speed : TurnSpeeds)
		{
			SpeedMean += Speed;
		}
		SpeedMean = TurnSpeeds.IsEmpty() ? 0.0 : SpeedMean / TurnSpeeds.Num();
		const ACustomGravityCharacter* Character = GProbe.Character.Get();
		const USpringArmComponent* Boom = Character ? Character->CameraBoom : nullptr;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.CharProbe] finished (%s) character=%s pattern=%s sprint=%d mode=%d period=%.2f frames=%d turnFrames=%d | ")
			TEXT("yaw rate p95 %.0f max %.0f deg/s, yaw accel p95 %.0f max %.0f deg/s2 | speed mean %.0f min %.0f cm/s (dip %.0f%%) | ")
			TEXT("camera accel p95 %.0f max %.0f cm/s2, lag max %.0f of %.0f cm | anim GroundSpeed change p95 %.0f max %.0f cm/s2, ")
			TEXT("ShouldMove drops %d, play rate %.2f-%.2f"),
			Reason, *GetNameSafe(Character), *GProbe.Pattern, GProbe.bSprint ? 1 : 0, GProbe.Mode, GProbe.Period, S.Num(),
			YawRate.Num(), Percentile(YawRate, 0.95), Percentile(YawRate, 1.0), Percentile(YawAcceleration, 0.95),
			Percentile(YawAcceleration, 1.0), SpeedMean, Percentile(TurnSpeeds, 0.0),
			SpeedMean > 1.0 ? 100.0 * (1.0 - Percentile(TurnSpeeds, 0.0) / SpeedMean) : 0.0,
			Percentile(CameraAcceleration, 0.95), Percentile(CameraAcceleration, 1.0), LagMax,
			Boom ? Boom->CameraLagMaxDistance : 0.0, Percentile(AnimSpeedRate, 0.95), Percentile(AnimSpeedRate, 1.0),
			ShouldMoveDrops, PlayRateMin == TNumericLimits<double>::Max() ? 0.0 : PlayRateMin, PlayRateMax);
		{
			// Frame pacing: a steady 120 FPS with a 30 ms frame every second reads fine as FPS and feels like a stutter.
			const double Median = Percentile(GProbe.FrameMs, 0.5);
			int32 Spikes = 0;
			int32 Hitches = 0;
			double Sum = 0.0;
			for (const double Ms : GProbe.FrameMs)
			{
				Spikes += Ms > Median * 1.5 ? 1 : 0;
				Hitches += Ms > 33.4 ? 1 : 0;
				Sum += Ms;
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.CharProbe] frames look=%.0f | frame avg %.2f median %.2f p95 %.2f p99 %.2f max %.2f ms | spikes >1.5x median %d of %d, hitches >33 ms %d"),
				GProbe.LookRate, GProbe.FrameMs.IsEmpty() ? 0.0 : Sum / GProbe.FrameMs.Num(), Median, Percentile(GProbe.FrameMs, 0.95),
				Percentile(GProbe.FrameMs, 0.99), Percentile(GProbe.FrameMs, 1.0), Spikes, GProbe.FrameMs.Num(), Hitches);
		}

		FString Csv = TEXT("time,yaw_deg,ground_speed,camera_x,camera_y,camera_z,camera_lag,anim_ground_speed,anim_should_move,play_rate,turn_phase\n");
		for (const FSample& Sample : S)
		{
			Csv += FString::Printf(TEXT("%.4f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%.3f,%d\n"), Sample.Time, Sample.YawDegrees,
				Sample.GroundSpeed, Sample.Camera.X, Sample.Camera.Y, Sample.Camera.Z, Sample.CameraLag, Sample.AnimGroundSpeed,
				Sample.AnimShouldMove, Sample.PlayRate, Sample.bTurnPhase ? 1 : 0);
		}
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Diagnostics/CharacterProbe")
			/ FString::Printf(TEXT("%s_%s_%s_m%d.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),
				*GetNameSafe(Character ? Character->GetClass() : nullptr), *GProbe.Pattern, GProbe.Mode);
		FFileHelper::SaveStringToFile(Csv, *Path);
		UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] csv=%s"), *FPaths::ConvertRelativePathToFull(Path));
	}

	bool TickProbe(float DeltaTime)
	{
		ACustomGravityCharacter* Character = GProbe.Character.Get();
		APlayerController* Controller = GProbe.Controller.Get();
		if (!GProbe.bActive || !Character || !Controller || Controller->GetPawn() != Character)
		{
			Finish(TEXT("character or controller lost"));
			return false;
		}
		GProbe.Elapsed += DeltaTime;
		if (GProbe.Elapsed >= GProbe.Seconds)
		{
			Finish(TEXT("completed"));
			return false;
		}
		GProbe.FrameMs.Add(DeltaTime * 1000.0);
		if (const int32 Second = FMath::FloorToInt(GProbe.Elapsed); Second != GProbe.RegionSecond)
		{
			if (!GProbe.Region.IsEmpty())
			{
				TRACE_END_REGION(*GProbe.Region);
			}
			GProbe.RegionSecond = Second;
			GProbe.Region = FString::Printf(TEXT("CharProbe_%03d"), Second);
			TRACE_BEGIN_REGION(*GProbe.Region);
		}
		if (GProbe.LookRate != 0.0 && GProbe.Elapsed >= 1.0)
		{
			const double Direction = FMath::FloorToInt(GProbe.Elapsed / 2.0) % 2 == 0 ? 1.0 : -1.0;
			Character->AddControllerYawInput(static_cast<float>(GProbe.LookRate * Direction * DeltaTime));
		}
		if (GProbe.bShots)
		{
			// Two captures a few frames apart with the view tilted each way: one of them shows the sky.
			const double Left = GProbe.Seconds - GProbe.Elapsed;
			const int32 Wanted = Left < 0.3 ? 4 : (Left < 0.6 ? 3 : (Left < 0.9 ? 2 : (Left < 1.2 ? 1 : 0)));
			while (GProbe.ShotsTaken < Wanted)
			{
				++GProbe.ShotsTaken;
				if (GProbe.ShotsTaken == 1 || GProbe.ShotsTaken == 3)
				{
					Character->AddControllerPitchInput(GProbe.ShotsTaken == 1 ? -12.0f : 24.0f);
				}
				else
				{
					const FString File = FPaths::ScreenShotDir() / TEXT("CharProbe") / FString::Printf(TEXT("%s_%s_shot%d.png"),
						*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")), *GProbe.Pattern, GProbe.ShotsTaken / 2);
					FScreenshotRequest::RequestScreenshot(File, false, false);
					UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] shot %s"), *FPaths::ConvertRelativePathToFull(File));
				}
			}
		}

		FSample Sample;
		Sample.Time = GProbe.Elapsed;
		const FVector Up = Character->GetGravityUpVector();
		const FVector Facing = FVector::VectorPlaneProject(Character->GetActorForwardVector(), Up).GetSafeNormal();
		const FVector ReferenceRight = FVector::CrossProduct(GProbe.ReferenceUp, GProbe.ReferenceForward).GetSafeNormal();
		const double RawYaw = FMath::RadiansToDegrees(FMath::Atan2(
			FVector::DotProduct(Facing, ReferenceRight), FVector::DotProduct(Facing, GProbe.ReferenceForward)));
		if (GProbe.bHasYaw)
		{
			const double Step = RawYaw - GProbe.LastRawYaw;
			GProbe.YawUnwrap += Step > 180.0 ? -360.0 : (Step < -180.0 ? 360.0 : 0.0);
		}
		GProbe.LastRawYaw = RawYaw;
		GProbe.bHasYaw = true;
		Sample.YawDegrees = RawYaw + GProbe.YawUnwrap;
		Sample.GroundSpeed = FVector::VectorPlaneProject(Character->GetVelocity(), Up).Size();
		if (Character->FollowCamera)
		{
			Sample.Camera = Character->FollowCamera->GetComponentLocation();
		}
		if (Character->CameraBoom)
		{
			// Distance of the lagged boom origin from the character: what CameraLagMaxDistance clamps.
			const FVector BoomOrigin = Sample.Camera + Character->CameraBoom->GetForwardVector() * Character->CameraBoom->TargetArmLength;
			Sample.CameraLag = FVector::Distance(BoomOrigin, Character->CameraBoom->GetComponentLocation());
		}
		ReadAnimation(*Character, Sample);

		bool bTurnPhase = false;
		const TArray<FKey> Keys = KeysAt(GProbe.Elapsed, bTurnPhase);
		Sample.bTurnPhase = bTurnPhase;
		GProbe.Samples.Add(Sample);
		SetHeld(*Controller, Keys);
		return true;
	}

	void Start(const TArray<FString>& Args, UWorld*)
	{
		if (GProbe.bActive)
		{
			Finish(TEXT("restarted"));
		}
		APlayerController* Controller = FindPlayerController();
		ACustomGravityCharacter* Character = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.CharProbe] the player must control a gravity character (ACustomGravityCharacter)."));
			return;
		}
		GProbe = FProbe();
		GProbe.Pattern = Args.Num() > 0 ? Args[0] : FString(TEXT("slalom"));
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			FString Key, Value;
			if (!Args[Index].Split(TEXT("="), &Key, &Value))
			{
				continue;
			}
			if (Key.Equals(TEXT("duration"), ESearchCase::IgnoreCase)) GProbe.Seconds = FMath::Clamp(FCString::Atod(*Value), 2.0, 120.0);
			else if (Key.Equals(TEXT("sprint"), ESearchCase::IgnoreCase)) GProbe.bSprint = FCString::Atoi(*Value) != 0;
			else if (Key.Equals(TEXT("mode"), ESearchCase::IgnoreCase)) GProbe.Mode = FMath::Clamp(FCString::Atoi(*Value), 0, 3);
			else if (Key.Equals(TEXT("period"), ESearchCase::IgnoreCase)) GProbe.Period = FMath::Clamp(FCString::Atod(*Value), 0.1, 5.0);
			else if (Key.Equals(TEXT("look"), ESearchCase::IgnoreCase)) GProbe.LookRate = FMath::Clamp(FCString::Atod(*Value), -720.0, 720.0);
			else if (Key.Equals(TEXT("shot"), ESearchCase::IgnoreCase)) GProbe.bShots = FCString::Atoi(*Value) != 0;
		}
		GProbe.Character = Character;
		GProbe.Controller = Controller;
		GProbe.ReferenceUp = Character->GetGravityUpVector();
		GProbe.ReferenceForward = FVector::VectorPlaneProject(Character->GetActorForwardVector(), GProbe.ReferenceUp).GetSafeNormal();
		GProbe.bActive = true;
		if (GProbe.Mode > 0)
		{
			// The pace keys of APSSpeedModeCharacter; other characters ignore them.
			const FKey ModeKey = GProbe.Mode == 1 ? EKeys::One : (GProbe.Mode == 2 ? EKeys::Two : EKeys::Three);
			Controller->InputKey(FInputKeyParams(ModeKey, IE_Pressed, 1.0));
			Controller->InputKey(FInputKeyParams(ModeKey, IE_Released, 0.0));
		}
		GProbe.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickProbe));
		UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] start character=%s class=%s pattern=%s duration=%.0f sprint=%d mode=%d period=%.2f"),
			*Character->GetName(), *Character->GetClass()->GetName(), *GProbe.Pattern, GProbe.Seconds, GProbe.bSprint ? 1 : 0,
			GProbe.Mode, GProbe.Period);
	}

	void EndAutoProbe(const TCHAR* Reason)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] auto probe ended: %s"), Reason);
		GAutoProbe.bActive = false;
		if (GAutoProbe.bQuit)
		{
			FPlatformMisc::RequestExit(false, TEXT("aps.Char.AutoProbe"));
		}
	}

	bool TickAutoProbe(float)
	{
		if (!GAutoProbe.bActive)
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		if (Now - GAutoProbe.StartSeconds > 900.0)
		{
			EndAutoProbe(TEXT("timeout after 15 min"));
			return false;
		}
		if (GAutoProbe.bStarted)
		{
			if (!GProbe.bActive)
			{
				EndAutoProbe(TEXT("probe finished"));
				return false;
			}
			return true;
		}
		APlayerController* Controller = FindPlayerController();
		const ACustomGravityCharacter* Character = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
		if (!Character || Character->IsSurfaceHandoffSuspended()
			|| (!GAutoProbe.Map.IsEmpty() && !Character->GetWorld()->GetMapName().Contains(GAutoProbe.Map)))
		{
			GAutoProbe.ReadySeconds = 0.0;
			return true;
		}
		if (GAutoProbe.ReadySeconds <= 0.0)
		{
			GAutoProbe.ReadySeconds = Now;
			UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] auto probe: %s ready, warming up %.0f s"), *Character->GetName(),
				GAutoProbe.WarmupSeconds);
		}
		if (Now - GAutoProbe.ReadySeconds < GAutoProbe.WarmupSeconds)
		{
			return true;
		}
		Start(GAutoProbe.ProbeArgs, nullptr);
		if (!GProbe.bActive)
		{
			EndAutoProbe(TEXT("probe did not start"));
			return false;
		}
		GAutoProbe.bStarted = true;
		return true;
	}

	void AutoProbe(const TArray<FString>& Args, UWorld*)
	{
		if (GAutoProbe.bActive)
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GAutoProbe.Ticker);
		}
		GAutoProbe = FAutoProbe();
		GAutoProbe.bActive = true;
		for (const FString& Arg : Args)
		{
			if (Arg.StartsWith(TEXT("map="), ESearchCase::IgnoreCase)) GAutoProbe.Map = Arg.RightChop(4);
			else if (Arg.StartsWith(TEXT("warmup="), ESearchCase::IgnoreCase)) GAutoProbe.WarmupSeconds = FMath::Clamp(FCString::Atod(*Arg.RightChop(7)), 0.0, 300.0);
			else if (Arg.Equals(TEXT("quit"), ESearchCase::IgnoreCase)) GAutoProbe.bQuit = true;
			else GAutoProbe.ProbeArgs.Add(Arg);
		}
		GAutoProbe.StartSeconds = FPlatformTime::Seconds();
		GAutoProbe.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAutoProbe), 0.25f);
		UE_LOG(LogTemp, Log, TEXT("[APS.CharProbe] auto probe armed: map='%s' warmup=%.0f s quit=%d args=%s"), *GAutoProbe.Map,
			GAutoProbe.WarmupSeconds, GAutoProbe.bQuit ? 1 : 0, *FString::Join(GAutoProbe.ProbeArgs, TEXT(" ")));
	}

	FAutoConsoleCommandWithWorldAndArgs ProbeCommand(
		TEXT("aps.Char.Probe"),
		TEXT("aps.Char.Probe <run|slalom|reverse> [duration=12] [sprint=1] [mode=0-3] [period=0.7]: presses W/A/D/S/Shift through the ")
		TEXT("PlayerController and logs how smoothly the gravity character turns, runs and animates."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Start));

	FAutoConsoleCommandWithWorldAndArgs AutoProbeCommand(
		TEXT("aps.Char.AutoProbe"),
		TEXT("aps.Char.AutoProbe <pattern> [map=Substring] [warmup=30] [quit] [probe options]: waits for the player's gravity ")
		TEXT("character, lets streaming settle, runs aps.Char.Probe and optionally quits (for -game runs)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AutoProbe));
}

#endif
