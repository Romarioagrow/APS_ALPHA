#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorldGenerationViewModel;
class SVerticalBox;
struct FAPSPreviewBodyEntry;

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
	FText GetContinueLabel() const;
	FReply CommitWorld();
	FReply RefreshPreview();
	FReply GoBack();
	FReply FocusPreview(uint8 FocusValue);
	FReply FocusPreviewUp();
	FReply FocusPreviewBody(TWeakObjectPtr<AActor> BodyActor);
	EActiveTimerReturnType RefreshBodyHierarchy(double CurrentTime, float DeltaTime);
	void RebuildBodyHierarchy(const TArray<FAPSPreviewBodyEntry>& Entries, uint32 Signature);

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	FSimpleDelegate OnBack;
	FSimpleDelegate OnContinue;
	TSharedPtr<SVerticalBox> BodyHierarchyBox;
	uint32 BodyHierarchySignature{0};
};
