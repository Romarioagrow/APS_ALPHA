#include "GravityGameModeBase.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/SMENU_HUD.h"
#include "APS_ALPHA/UI/SMENU_PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AGravityGameModeBase::AGravityGameModeBase()
{
	PlayerControllerClass = ASMENU_PlayerController::StaticClass();
	HUDClass = ASMENU_HUD::StaticClass();
}

void AGravityGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGeneratedWorld* NewGeneratedWorld = GameInstance->GetSubsystem<UMainGameplayInstance>()->NewGeneratedWorld)
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
