#include "GravityGameModeBase.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "APS_ALPHA/Pawns/Spectator/APSSpectatorPawn.h"
#include "APS_ALPHA/UI/SMENU_HUD.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGravityGameModeBase::AGravityGameModeBase()
{
	PlayerControllerClass = AGravityPlayerController::StaticClass();
	HUDClass = ASMENU_HUD::StaticClass();
}

void AGravityGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		// L_WorldGeneration still contains a legacy one-shot PrintString in its
		// Level Blueprint. Clear only the messages emitted during BeginPlay on the
		// following tick; later engine diagnostics (for example texture-pool
		// pressure) remain visible instead of being globally disabled.
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this, &AGravityGameModeBase::ClearLegacyLevelScreenMessages));
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			UMainGameplayInstance* GameplayState =
				GameInstance->GetSubsystem<UMainGameplayInstance>();
			if (GameplayState && GameplayState->bUseAuthoredSinglePlayWorld)
			{
				UE_LOG(LogTemp, Log,
					TEXT("[APS.WorldGeneration] Authored SinglePlay route: using only actors serialized in the level"));
				return;
			}

			if (UGeneratedWorld* NewGeneratedWorld = GameplayState ? GameplayState->NewGeneratedWorld : nullptr)
			{
				TSubclassOf<AAstroGenerator> RuntimeGeneratorClass = BP_AstroGeneratorClass;
				if (!RuntimeGeneratorClass)
				{
					// Existing APS_GM_Single_Gravity assets predate this native property and
					// therefore serialize it as null until someone resaves the Blueprint.
					// The generated deployment route must still work in packaged builds and
					// clean workspaces, so resolve the same proven generator asset explicitly.
					RuntimeGeneratorClass = LoadClass<AAstroGenerator>(nullptr,
						TEXT("/Game/APS/APS_ALPHA/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
				}
				if (!RuntimeGeneratorClass)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[APS.WorldGeneration] Runtime generator class is unavailable on the gameplay mode and fallback asset"));
					return;
				}
				APlayerController* PlayerController = World->GetFirstPlayerController();
				AActor* OwnerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
				const FTransform GeneratorTransform(FRotator::ZeroRotator, FVector::ZeroVector);
				AAstroGenerator* AstroGenerator = World->SpawnActorDeferred<AAstroGenerator>(
					RuntimeGeneratorClass, GeneratorTransform, OwnerActor, nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (AstroGenerator)
				{
					// Blueprint CDOs used by the authored map may still serialize automatic
					// BeginPlay generation. Configure the independent runtime instance before
					// FinishSpawning so it cannot build a default hierarchy and then stack the
					// committed hierarchy on top of it (the former black/inside-planet handoff).
					AstroGenerator->bAutoGeneration = false;
					// This actor deploys a committed generated model into the empty runtime
					// map.  BP_AstroGenerator is also used by authored SinglePlay and its CDO
					// may serialize integration references from that level; those references
					// cannot survive OpenLevel and prevented the generated HomePlanet from
					// being selected, leaving only the lightweight placeholder sphere.
					AstroGenerator->bIntegrateStartPlanet = false;
					AstroGenerator->WSR_StartHomePlanet = nullptr;
					AstroGenerator->SetGeneratedWorld(NewGeneratedWorld);
					UGameplayStatics::FinishSpawningActor(AstroGenerator, GeneratorTransform);
					AstroGenerator->DisplayNewGeneratedWorld();
					AstroGenerator->GenerateWorldByModel();
					const bool bStarterCommitPending = AstroGenerator->IsGeneratedStarterCommitPending();
					const bool bSavedWorldReplay = GameplayState->bPendingSavedWorldReplay;
					if (bSavedWorldReplay)
					{
						GameplayState->bSavedWorldHierarchyReady = false;
						const TWeakObjectPtr<AAstroGenerator> WeakGenerator(AstroGenerator);
						const TWeakObjectPtr<UGeneratedWorld> ExpectedModel(NewGeneratedWorld);
						const uint64 CommitSerial = AstroGenerator->GetGeneratedStarterCommitSerial();
						const double RequestTime = FPlatformTime::Seconds();
						if (bStarterCommitPending)
						{
							World->GetTimerManager().SetTimer(SavedWorldReplayTimer,
								FTimerDelegate::CreateWeakLambda(this,
									[this, WeakGenerator, ExpectedModel, CommitSerial, RequestTime]()
									{
										TryFinalizeSavedWorldReplay(WeakGenerator, ExpectedModel,
											CommitSerial, RequestTime);
									}), 0.1f, true);
						}
						// Preserve the same-frame overlay when generation completed synchronously.
						TryFinalizeSavedWorldReplay(WeakGenerator, ExpectedModel, CommitSerial, RequestTime);
					}
					if (bStarterCommitPending)
					{
						UE_LOG(LogTemp, Log,
							TEXT("[APS.WorldGeneration] Generated world starter commit queued; saved replay deferred=%d"),
							bSavedWorldReplay ? 1 : 0);
					}
					else if (!AstroGenerator->HasGeneratedStarterCommitFailed())
					{
						UE_LOG(LogTemp, Log,
							TEXT("[APS.WorldGeneration] Generated committed world from isolated runtime generator"));
					}
					else if (!bSavedWorldReplay)
					{
						UE_LOG(LogTemp, Error,
							TEXT("[APS.WorldGeneration] Generated world starter commit failed"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error,
						TEXT("[APS.WorldGeneration] Failed to spawn runtime generator class %s"),
						*GetNameSafe(BP_AstroGeneratorClass.Get()));
				}
			}
		}
	}
}

void AGravityGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SavedWorldReplayTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AGravityGameModeBase::TryFinalizeSavedWorldReplay(
	TWeakObjectPtr<AAstroGenerator> WeakGenerator, TWeakObjectPtr<UGeneratedWorld> ExpectedModel,
	const uint64 CommitSerial, const double RequestTime)
{
	UWorld* World = GetWorld();
	AAstroGenerator* Generator = WeakGenerator.Get();
	UMainGameplayInstance* GameplayState = IsValid(World) && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const auto AbortReplay = [World](FTimerHandle& Timer, const TCHAR* Reason)
	{
		if (IsValid(World)) World->GetTimerManager().ClearTimer(Timer);
		UE_LOG(LogTemp, Error, TEXT("[APS.Save] Deferred generated hierarchy replay aborted: %s"), Reason);
	};
	if (!IsValid(Generator) || !IsValid(World) || Generator->GetWorld() != World
		|| !ExpectedModel.IsValid() || Generator->GetGeneratedWorldModel() != ExpectedModel.Get()
		|| Generator->GetGeneratedStarterCommitSerial() != CommitSerial
		|| !GameplayState || GameplayState->NewGeneratedWorld != ExpectedModel.Get()
		|| !GameplayState->bPendingSavedWorldReplay || GameplayState->bUseAuthoredSinglePlayWorld)
	{
		AbortReplay(SavedWorldReplayTimer, TEXT("request cancelled or generator/world model replaced"));
		return;
	}
	if (Generator->HasGeneratedStarterCommitFailed())
	{
		AbortReplay(SavedWorldReplayTimer, TEXT("starter hierarchy commit failed"));
		return;
	}
	// Slightly exceeds the generator's 180-second material deadline; never wait indefinitely.
	if (FPlatformTime::Seconds() - RequestTime > 185.0)
	{
		AbortReplay(SavedWorldReplayTimer, TEXT("starter hierarchy wait exceeded 185 seconds"));
		return;
	}
	if (Generator->IsGeneratedStarterCommitPending()) return;

	// Clear before LoadWorld, which may alter replay state or initiate world teardown.
	World->GetTimerManager().ClearTimer(SavedWorldReplayTimer);
	GameplayState->bSavedWorldHierarchyReady = true;
	if (AGravityPlayerController* GravityController =
		Cast<AGravityPlayerController>(World->GetFirstPlayerController()))
	{
		GravityController->LoadWorld();
	}
}

void AGravityGameModeBase::ClearLegacyLevelScreenMessages()
{
	if (GEngine)
	{
		GEngine->ClearOnScreenDebugMessages();
	}
}

UClass* AGravityGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// Generated-civilization travel already carries an immutable class selection
	// in the GameInstance subsystem.  Use it for the initial spawn as well as the
	// later station placement so the map never flashes/possesses the GameMode's
	// unrelated default pawn.
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UMainGameplayInstance* GameplayState =
				GameInstance->GetSubsystem<UMainGameplayInstance>())
			{
				if (GameplayState->bSpawnGeneratedCivilization
					&& IsValid(GameplayState->SpawnParameters)
					&& GameplayState->SpawnParameters->BP_CharacterClass)
				{
					return GameplayState->SpawnParameters->BP_CharacterClass.Get();
				}
			}
		}
	}
	// Rio 03.10: Generate Space (the astronomical model without a civilization) flies a free camera, not the walker.
	// A save of such a world replays the same route; LoadWorld still restores the pawn class the save holds.
	if (Cast<APlayerController>(InController) && AAPSSpectatorPawn::IsGeneratedSpaceRoute(GetWorld()))
	{
		return AAPSSpectatorPawn::StaticClass();
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

