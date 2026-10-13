#include "APSPauseMenuSubsystem.h"

#include "SAPSPauseMenu.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SWeakWidget.h"

#define LOCTEXT_NAMESPACE "APSPauseMenu"

namespace APSPauseMenu
{
	// Above the colony terminal (890) and the HUD cards: the pause menu covers everything.
	constexpr int32 ViewportZOrder = 950;
	const TCHAR* MainMenuMap = TEXT("/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_MainMenu_Alpha");
}

bool UAPSPauseMenuSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSPauseMenuSubsystem::Deinitialize()
{
	Close();
	Super::Deinitialize();
}

bool UAPSPauseMenuSubsystem::IsOpenIn(const UWorld* World)
{
	const UAPSPauseMenuSubsystem* Subsystem = World ? World->GetSubsystem<UAPSPauseMenuSubsystem>() : nullptr;
	return Subsystem && Subsystem->IsOpen();
}

void UAPSPauseMenuSubsystem::Toggle(APlayerController* Controller)
{
	if (Menu.IsValid())
	{
		Close();
		return;
	}
	if (!IsValid(Controller) || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	BoundController = Controller;
	const FText WorldName = LOCTEXT("PausedHint", "The game is paused. ESC resumes it.");
	Menu = SNew(SAPSPauseMenu)
		.World(GetWorld())
		.WorldName(WorldName)
		.OnContinue(FSimpleDelegate::CreateUObject(this, &UAPSPauseMenuSubsystem::Close))
		.OnLeave(FSimpleDelegate::CreateUObject(this, &UAPSPauseMenuSubsystem::LeaveGame))
		.OnQuit(FSimpleDelegate::CreateUObject(this, &UAPSPauseMenuSubsystem::QuitGame));
	Container = SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(Container.ToSharedRef(), APSPauseMenu::ViewportZOrder);

	bPausedByMenu = !GetWorld()->IsPaused() && UGameplayStatics::SetGamePaused(GetWorld(), true);
	Controller->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(Menu);
	Controller->SetInputMode(InputMode);
	UE_LOG(LogTemp, Log, TEXT("[APS.PauseMenu] opened (paused=%d)"), bPausedByMenu ? 1 : 0);
}

void UAPSPauseMenuSubsystem::Close()
{
	if (!Menu.IsValid())
	{
		return;
	}
	if (Container.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Container.ToSharedRef());
	}
	Container.Reset();
	Menu.Reset();
	if (bPausedByMenu && GetWorld())
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
	bPausedByMenu = false;
	if (APlayerController* Controller = BoundController.Get())
	{
		Controller->bShowMouseCursor = false;
		Controller->SetInputMode(FInputModeGameOnly());
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.PauseMenu] closed"));
}

void UAPSPauseMenuSubsystem::SaveBeforeLeaving()
{
	if (AGravityPlayerController* Controller = Cast<AGravityPlayerController>(BoundController.Get()))
	{
		Controller->SaveCurrentWorld();
		Controller->bSavedForExit = true;
	}
}

void UAPSPauseMenuSubsystem::LeaveGame()
{
	UWorld* World = GetWorld();
	Close();
	SaveBeforeLeaving();
	UE_LOG(LogTemp, Log, TEXT("[APS.PauseMenu] leave: saved, to the main menu"));
	UGameplayStatics::OpenLevel(World, FName(APSPauseMenu::MainMenuMap));
}

void UAPSPauseMenuSubsystem::QuitGame()
{
	UWorld* World = GetWorld();
	APlayerController* Controller = BoundController.Get();
	Close();
	SaveBeforeLeaving();
	UE_LOG(LogTemp, Log, TEXT("[APS.PauseMenu] quit: saved, closing the game"));
	UKismetSystemLibrary::QuitGame(World, Controller, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
