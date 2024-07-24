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
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			UGeneratedWorld* NewGeneratedWorld = GameInstance->GetSubsystem<UMainGameplayInstance>()->NewGeneratedWorld;
			if (NewGeneratedWorld)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Spawning AstroGenerator")));

				// Spawn AAstroGenerator and pass UGeneratedWorld
				FActorSpawnParameters SpawnParams;
				AActor* OwnerActor = World->GetFirstPlayerController()->GetPawn();
				if (OwnerActor)
				{
					SpawnParams.Owner = OwnerActor;
				}

				AAstroGenerator* AstroGenerator = World->SpawnActor<AAstroGenerator>(AAstroGenerator::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				if (AstroGenerator)
				{
					AstroGenerator->SetGeneratedWorld(NewGeneratedWorld);
					AstroGenerator->DisplayNewGeneratedWorld();
				}
			}
		}
	}
}
