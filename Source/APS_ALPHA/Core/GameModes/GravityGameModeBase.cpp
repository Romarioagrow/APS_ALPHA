#include "GravityGameModeBase.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/SMENU_HUD.h"
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
				// Spawn AAstroGenerator and pass UGeneratedWorld
				FActorSpawnParameters SpawnParams;
				if (AActor* OwnerActor = World->GetFirstPlayerController()->GetPawn())
				{
					SpawnParams.Owner = OwnerActor;
				}

				if (AAstroGenerator* AstroGenerator = World->SpawnActor<AAstroGenerator>(
					BP_AstroGeneratorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams))
				{
					AstroGenerator->SetGeneratedWorld(NewGeneratedWorld);
					AstroGenerator->DisplayNewGeneratedWorld();
					AstroGenerator->GenerateWorldByModel();
				}
			}
		}
	}
}

