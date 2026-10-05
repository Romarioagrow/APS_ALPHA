#include "APSColonyTerminalSubsystem.h"

#include "APSMissionTracker.h"
#include "SAPSColonyTerminal.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyOnboardingSubsystem.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/SWeakWidget.h"

#define LOCTEXT_NAMESPACE "APSColonyTerminal"

namespace APSColonyTerminal
{
	/** The Tab hint sits just under the terminal (the objective too, in the mission tracker's card at the same layer). */
	constexpr int32 OverlayZOrder = 880;
	constexpr double HintSeconds = 14.0;

	/** Above the HUD (40-60), below the strategic map (900) and the main menu (1000). */
	constexpr int32 ViewportZOrder = 890;

	TAutoConsoleVariable<float> CVarTerminalShot(
		TEXT("aps.Colony.TerminalShot"), 0.0f,
		TEXT(">0: that many seconds after the terminal key is bound, open the colony terminal, save a screenshot of each ")
		TEXT("tab with the UI to Saved/Screenshots/ColonyTerminal and close it again (test runs)."));
}

bool UAPSColonyTerminalSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSColonyTerminalSubsystem::Tick(float DeltaTime)
{
	if (bBound)
	{
		// Test capture: the game view, then every tab (overview to shipyard), 1.5 s apart, each with the UI.
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		if (ShotStage < 0 || Now < ShotStageSeconds)
		{
			return;
		}
		if (ShotStage > 29)
		{
			CloseTerminal();
			ShotStage = -1;
			UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Terminal] test captures saved"));
			return;
		}
		if (ShotStage == 0)
		{
			// The game view first, with the objective panel and the HUD.
			FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / TEXT("ColonyTerminal")
				/ FString::Printf(TEXT("%s_hud.png"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))), true, false);
		}
		else if (ShotStage == 1)
		{
			if (!TerminalWidget.IsValid())
			{
				ToggleTerminal();
			}
		}
		else if (TerminalWidget.IsValid())
		{
			// Two steps a tab: show it, then shoot it a moment later. A shot is taken with the next frame, so switching
			// in the same tick as the request caught the next tab (01.10: the journal's shot showed the shipyard).
			// Then the object window over the map and the construction catalogue (02.10, C4/C5).
			// Rio 05.10 (star map): then MAP > STAR MAP and FLEET ORDERS with the STARS target picker.
			static const TCHAR* TabNames[] = {TEXT("overview"), TEXT("map"), TEXT("colony"), TEXT("fleet"),
				TEXT("divisions"), TEXT("journal"), TEXT("shipyard"), TEXT("scheme"), TEXT("pilot"), TEXT("surface"),
				TEXT("object"), TEXT("construction"), TEXT("starmap"), TEXT("fleetstars")};
			const int32 Tab = (ShotStage - 2) / 2;
			if ((ShotStage - 2) % 2 == 0)
			{
				if (Tab < 10)
				{
					TerminalWidget->ShowTab(Tab);
				}
				else if (Tab == 12)
				{
					TerminalWidget->ShowTab(10);
				}
				else
				{
					TerminalWidget->ShowTestOverlay(Tab == 13 ? 3 : Tab - 9);
				}
			}
			else
			{
				FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / TEXT("ColonyTerminal")
					/ FString::Printf(TEXT("%s_%s.png"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")), TabNames[Tab]),
					true, false);
				if (Tab == 11 || Tab == 13)
				{
					TerminalWidget->ShowTestOverlay(0);
				}
			}
		}
		++ShotStage;
		ShotStageSeconds = Now + 1.2;
		return;
	}
	// Only the generated game's controller: other levels keep their own Tab (the authored single-play menu).
	AGravityPlayerController* Controller = GetWorld()
		? Cast<AGravityPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
	if (!IsValid(Controller) || !Controller->InputComponent || !Controller->IsLocalController())
	{
		return;
	}
	Controller->InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &UAPSColonyTerminalSubsystem::ToggleTerminal);
	// K: fleet command (Rio, 01.10). Bound above the pawn, like Tab, so it wins over the M3's legacy SwitchRamp mapping.
	Controller->InputComponent->BindKey(EKeys::K, IE_Pressed, this, &UAPSColonyTerminalSubsystem::ToggleFleetCommand);
	BoundController = Controller;
	bBound = true;
	if (UAPSColonyConstructionSubsystem* Construction = GetWorld()->GetSubsystem<UAPSColonyConstructionSubsystem>())
	{
		Construction->OnTestShotsFinished().AddUObject(this, &UAPSColonyTerminalSubsystem::StartTestCaptures);
	}
	HintUntilSeconds = GetWorld()->GetTimeSeconds() + APSColonyTerminal::HintSeconds;
	ShowOverlays();
	if (const float ShotDelay = APSColonyTerminal::CVarTerminalShot.GetValueOnGameThread(); ShotDelay > 0.0f)
	{
		ShotStage = 0;
		ShotStageSeconds = GetWorld()->GetTimeSeconds() + ShotDelay;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Terminal] Tab opens the colony terminal (%s)"), *GetNameSafe(Controller));
}

TStatId UAPSColonyTerminalSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSColonyTerminalSubsystem, STATGROUP_Tickables);
}

void UAPSColonyTerminalSubsystem::Deinitialize()
{
	HideOverlays();
	CloseTerminal();
	Super::Deinitialize();
}

void UAPSColonyTerminalSubsystem::StartTestCaptures()
{
	if (bBound && GetWorld() && ShotStage < 0)
	{
		ShotStage = 0;
		ShotStageSeconds = GetWorld()->GetTimeSeconds() + 0.5;
	}
}

void UAPSColonyTerminalSubsystem::ToggleFleetCommand()
{
	constexpr int32 FleetTab = 3;
	if (TerminalWidget.IsValid())
	{
		if (TerminalWidget->IsShowingTab(FleetTab))
		{
			CloseTerminal();
		}
		else
		{
			TerminalWidget->ShowTab(FleetTab);
		}
		return;
	}
	ToggleTerminal();
	if (TerminalWidget.IsValid())
	{
		TerminalWidget->ShowTab(FleetTab);
	}
}

void UAPSColonyTerminalSubsystem::OpenTerminalTab(const int32 Tab)
{
	if (!TerminalWidget.IsValid())
	{
		ToggleTerminal();
	}
	if (TerminalWidget.IsValid())
	{
		TerminalWidget->ShowTab(Tab);
	}
}

void UAPSColonyTerminalSubsystem::ToggleTerminal()
{
	if (TerminalWidget.IsValid())
	{
		CloseTerminal();
		return;
	}
	APlayerController* Controller = BoundController.Get();
	if (!IsValid(Controller) || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	TerminalWidget = SNew(SAPSColonyTerminal)
		.World(GetWorld())
		.OnClose(FSimpleDelegate::CreateUObject(this, &UAPSColonyTerminalSubsystem::CloseTerminal))
		.OnOpenMap(FSimpleDelegate::CreateUObject(this, &UAPSColonyTerminalSubsystem::OpenStrategicMap));
	TerminalContainer = SNew(SWeakWidget).PossiblyNullContent(TerminalWidget.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(TerminalContainer.ToSharedRef(), APSColonyTerminal::ViewportZOrder);
	bTerminalEverOpened = true;
	if (UAPSColonyOnboardingSubsystem* Onboarding = GetWorld()->GetSubsystem<UAPSColonyOnboardingSubsystem>())
	{
		Onboarding->NotifyTerminalOpened();
	}

	Controller->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetWidgetToFocus(TerminalWidget);
	Controller->SetInputMode(InputMode);
	UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Terminal] opened"));
}

void UAPSColonyTerminalSubsystem::CloseTerminal()
{
	if (!TerminalWidget.IsValid())
	{
		return;
	}
	if (TerminalContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(TerminalContainer.ToSharedRef());
	}
	TerminalContainer.Reset();
	TerminalWidget.Reset();
	if (APlayerController* Controller = BoundController.Get())
	{
		Controller->bShowMouseCursor = false;
		Controller->SetInputMode(FInputModeGameOnly());
	}
}

void UAPSColonyTerminalSubsystem::ShowOverlays()
{
	using namespace APSChrome;
	if (OverlayContainer.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	const TWeakObjectPtr<UAPSColonyTerminalSubsystem> WeakThis(this);
	// Top left: the ship HUD holds the top right (navigation) and the bottom left (flight), the pilot's HUD the bottom
	// left. In the top right the objective covered the ship's navigation block (u4-hud-ship-1, 30.09).
	// Rio 03.10: the objective heads the mission tracker's card there (APSMissionTracker), with the tracked mission under
	// it in the same style; as two panels at fixed offsets the tracker ran over the objective.
	APSMissionTracker::SetObjective(GetWorld(), [WeakThis](FText& OutTitle, FText& OutBody)
	{
		const UAPSColonyTerminalSubsystem* Self = WeakThis.Get();
		// Under the F10 map too: its 3D view between the panels let the objective show through (02.10 test shots).
		const AGravityPlayerController* Controller = Self ? Cast<AGravityPlayerController>(Self->BoundController.Get()) : nullptr;
		const UAPSColonyOnboardingSubsystem* Onboarding = Self && Self->GetWorld()
			? Self->GetWorld()->GetSubsystem<UAPSColonyOnboardingSubsystem>() : nullptr;
		return Self && Onboarding && !Self->IsTerminalOpen() && !(Controller && Controller->IsStrategicMapOpen())
			&& Onboarding->GetObjective(OutTitle, OutBody) && !OutTitle.IsEmpty();
	});
	OverlayWidget = SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0.0f, 28.0f, 0.0f, 0.0f))
		[
			SNew(SBox).WidthOverride(380.0f)
			.Visibility_Lambda([WeakThis]()
			{
				const UAPSColonyTerminalSubsystem* Self = WeakThis.Get();
				return Self && Self->GetWorld() && !Self->bTerminalEverOpened
					&& Self->GetWorld()->GetTimeSeconds() < Self->HintUntilSeconds
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				ChamferPanel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						KeyChip(LOCTEXT("HintKey", "TAB"), Cyan())
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(LOCTEXT("HintTitle", "COLONY TERMINAL")).Font(Font("Bold", 12))
							.ColorAndOpacity(White())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(LOCTEXT("HintBody", "Construction, the system and a course, the journal"))
							.Font(Font("Regular", 9)).ColorAndOpacity(Muted())
						]
					],
					FMargin(14.0f, 9.0f), CyanDim())
			]
		];
	OverlayContainer = SNew(SWeakWidget).PossiblyNullContent(OverlayWidget.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(OverlayContainer.ToSharedRef(), APSColonyTerminal::OverlayZOrder);
}

void UAPSColonyTerminalSubsystem::HideOverlays()
{
	APSMissionTracker::SetObjective(GetWorld(), nullptr);
	if (OverlayContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(OverlayContainer.ToSharedRef());
	}
	OverlayContainer.Reset();
	OverlayWidget.Reset();
}

void UAPSColonyTerminalSubsystem::OpenStrategicMap()
{
	CloseTerminal();
	if (AGravityPlayerController* Controller = Cast<AGravityPlayerController>(BoundController.Get()))
	{
		Controller->ToggleStrategicMap();
	}
}

#undef LOCTEXT_NAMESPACE
