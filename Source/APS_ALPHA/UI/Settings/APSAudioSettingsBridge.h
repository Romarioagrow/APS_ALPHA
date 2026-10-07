#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "APSAudioSettingsBridge.generated.h"

class UUserWidget;
class UWorld;

/** Connects the existing MBLS Audio tab to the game's persistent audio channels. */
UCLASS()
class APS_ALPHA_API UAPSAudioSettingsBridge : public UUserWidgetExtension
{
	GENERATED_BODY()
public:
	// Call after TakeWidget: the Blueprint's Construct has loaded its old settings.
	static bool Attach(UUserWidget* Panel);
	static void AttachWorldPanels(UWorld* World);
	static bool HasCompatibleControls(UUserWidget* Panel);
	virtual void Construct() override;
	virtual void Destruct() override;
	virtual bool RequiresTick() const override { return true; }
	virtual void Tick(const FGeometry& Geometry, float DeltaTime) override;

private:
	bool InitializeControls();
	void ApplyChanges();
	void Save();
	float LastValues[5] = {};
	float SaveDelay = 0.f;
	bool bInitialized = false;
	bool bDirty = false;
};
