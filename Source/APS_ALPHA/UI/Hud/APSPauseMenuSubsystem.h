#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPauseMenuSubsystem.generated.h"

class APlayerController;
class SAPSPauseMenu;
class SWeakWidget;

/**
 * Rio 10.10: the native pause menu (SAPSPauseMenu) over the paused game. AGravityPlayerController binds the
 * Toggle_PauseMenu action (Escape / Shift+Escape) to Toggle; the level Blueprint's old WBP_PauseMenu never sees the
 * key while aps.UI.PauseMenu is 1.
 */
UCLASS()
class APS_ALPHA_API UAPSPauseMenuSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	void Toggle(APlayerController* Controller);
	void Close();
	bool IsOpen() const { return Menu.IsValid(); }
	/** For the HUD: true while the pause menu of that world is open. */
	static bool IsOpenIn(const UWorld* World);

private:
	void LeaveGame();
	void QuitGame();
	void SaveBeforeLeaving();

	TSharedPtr<SAPSPauseMenu> Menu;
	TSharedPtr<SWeakWidget> Container;
	TWeakObjectPtr<APlayerController> BoundController;
	bool bPausedByMenu = false;
};
