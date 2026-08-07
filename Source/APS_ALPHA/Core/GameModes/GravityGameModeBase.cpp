#include "GravityGameModeBase.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "APS_ALPHA/UI/SMENU_HUD.h"
#include "Engine/Engine.h"
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
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			const UMainGameplayInstance* GameplayState =
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
					UE_LOG(LogTemp, Log,
						TEXT("[APS.WorldGeneration] Generated committed world from isolated runtime generator"));
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
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

