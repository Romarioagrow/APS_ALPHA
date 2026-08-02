#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorldGenerationViewModel;

class SWorldGenerationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldGenerationPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_EVENT(FSimpleDelegate, OnContinue)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText GetPreviewStatus() const;
	FReply CommitWorld();
	FReply RefreshPreview();
	FReply GoBack();
	FReply FocusPreview(uint8 FocusValue);

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	FSimpleDelegate OnBack;
	FSimpleDelegate OnContinue;
};
