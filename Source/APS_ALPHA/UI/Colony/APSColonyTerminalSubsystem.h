#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSColonyTerminalSubsystem.generated.h"

class APlayerController;
class SAPSColonyTerminal;
class SWeakWidget;

/**
 * Opens the colony terminal (SAPSColonyTerminal) with Tab in a generated game, the way F10 opens the strategic map:
 * a viewport widget above the HUD, game-and-UI input, the cursor on. The key is bound on the gameplay controller's
 * input component at runtime, so the shared controller class stays untouched.
 */
UCLASS()
class APS_ALPHA_API UAPSColonyTerminalSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate() && (!bBound || ShotStage >= 0); }
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	void ToggleTerminal();
	/** K: the terminal on fleet command; K again closes it. */
	void ToggleFleetCommand();
	bool IsTerminalOpen() const { return TerminalWidget.IsValid(); }
	/** Test runs: open the terminal, capture every tab with the UI, close it (after aps.Colony.ModuleShots too). */
	void StartTestCaptures();

private:
	void CloseTerminal();
	void OpenStrategicMap();
	/** The current onboarding objective (top right) and, for the first seconds, the Tab hint (top centre). */
	void ShowOverlays();
	void HideOverlays();

	TWeakObjectPtr<APlayerController> BoundController;
	TSharedPtr<SAPSColonyTerminal> TerminalWidget;
	TSharedPtr<SWeakWidget> TerminalContainer;
	TSharedPtr<SWidget> OverlayWidget;
	TSharedPtr<SWeakWidget> OverlayContainer;
	double HintUntilSeconds{0.0};
	bool bTerminalEverOpened{false};
	/** aps.Colony.TerminalShot: -1 idle; 0 the game view, 1 open, 2..6 each tab with the UI, then close. */
	int32 ShotStage{-1};
	double ShotStageSeconds{0.0};
	bool bBound{false};
};
