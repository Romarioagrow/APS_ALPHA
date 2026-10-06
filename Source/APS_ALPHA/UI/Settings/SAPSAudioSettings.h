#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class UWorld;
enum class EAPSAudioChannel : uint8;

class SAPSAudioSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSAudioSettings) : _CreditsOnly(false), _ShowTitle(true) {}
		SLATE_ARGUMENT(UWorld*, World)
		SLATE_ARGUMENT(bool, CreditsOnly)
		/** Rio 06.10: the SETTINGS page gives the tab its own heading. */
		SLATE_ARGUMENT(bool, ShowTitle)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual ~SAPSAudioSettings() override;
private:
	TSharedRef<SWidget> MakeRow(EAPSAudioChannel Channel, const FText& Label);
	void Save() const;
	TWeakObjectPtr<UWorld> World;
	FSliderStyle SliderStyle;
};
