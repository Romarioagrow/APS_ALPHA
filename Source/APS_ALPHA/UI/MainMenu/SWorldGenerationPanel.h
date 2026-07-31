#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorldGenerationViewModel;

class SWorldGenerationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldGenerationPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText GetPreviewStatus() const;
	FReply CommitWorld();
	FReply RefreshPreview();

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
};
