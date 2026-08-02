#include "MainMenuController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWeakWidget.h"

void AMainMenuController::BeginPlay()
{
	Super::BeginPlay();
	InstallSlateMenuTimer = GetWorldTimerManager().SetTimerForNextTick(
		this, &AMainMenuController::InstallSlateMenu);
}

void AMainMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InstallSlateMenuTimer);
	RemoveSlateMenu();
	if (WorldGenerationViewModel)
	{
		WorldGenerationViewModel->Shutdown();
	}
	Super::EndPlay(EndPlayReason);
}

void AMainMenuController::InstallSlateMenu()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	// Blueprint menu assets remain available as a fallback, but the runtime path
	// is owned by code so every screen shares one data model and one visual system.
	UWidgetLayoutLibrary::RemoveAllWidgets(this);
	if (!MenuGeneratedWorld)
	{
		MenuGeneratedWorld = NewObject<UGeneratedWorld>(this);
	}
	if (!WorldGenerationViewModel)
	{
		WorldGenerationViewModel = NewObject<UWorldGenerationViewModel>(this);
		WorldGenerationViewModel->Initialize(this, MenuGeneratedWorld);
	}

	SlateMenuRoot = SNew(SAPSMainMenuRoot)
		.Controller(this)
		.ViewModel(WorldGenerationViewModel);
	SlateMenuContainer = SNew(SWeakWidget).PossiblyNullContent(SlateMenuRoot.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(SlateMenuContainer.ToSharedRef(), 1000);

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(SlateMenuRoot);
	SetInputMode(InputMode);
}

void AMainMenuController::RemoveSlateMenu()
{
	if (SlateMenuContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SlateMenuContainer.ToSharedRef());
	}
	SlateMenuContainer.Reset();
	SlateMenuRoot.Reset();
}

void AMainMenuController::LaunchSingleGame()
{
	if (UMainGameplayInstance* GameplayInstance = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
	{
		GameplayInstance->bIsLoadingMode = false;
	}
	UGameplayStatics::OpenLevel(this, TEXT("L_APS_SinglePlay_StartLocation"));
}

void AMainMenuController::LoadWorldSlot(const FString& SaveFileName)
{
	SetSaveSlotName(SaveFileName);
	SetLoadingModeTrue();
	UGameplayStatics::OpenLevel(this, TEXT("L_WorldGeneration"));
}

void AMainMenuController::HoldSlateResource(UObject* Resource)
{
	if (Resource)
	{
		SlateResources.AddUnique(Resource);
	}
}

void AMainMenuController::SetSaveSlotName(FString OutSaveSlotName) 
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
			{
				MainGameplayInstance->SaveSlotName = OutSaveSlotName;
				UE_LOG(LogTemp, Log, TEXT("SaveSlotName set to: %s"), *MainGameplayInstance->SaveSlotName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MainGameplayInstance is null"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GameInstance is null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("World is null"));
	}
}

void AMainMenuController::SetLoadingModeTrue()
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UMainGameplayInstance* MainGameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
			{
				MainGameplayInstance->bIsLoadingMode = true;
			}
		}
	}
}
