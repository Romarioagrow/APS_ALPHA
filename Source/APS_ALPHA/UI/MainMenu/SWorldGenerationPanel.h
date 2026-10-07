#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorldGenerationViewModel;
class SAPSGenerationRangeSlider;
class SVerticalBox;
class SEditableTextBox;
struct FAPSPreviewBodyEntry;
struct FAPSModelCard;

enum class EAPSGenerationSurfaceControl : uint8
{
	Seed,
	FeatureScale,
	ReliefScale,
	LandCoverage,
	Mountains,
	Craters,
	Roughness
};

class SWorldGenerationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldGenerationPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_EVENT(FSimpleDelegate, OnContinue)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

#if WITH_DEV_AUTOMATION_TESTS
	/** Commits through the real Slate slider delegate used by the rendered panel. */
	bool CommitSurfaceControlForAutomation(EAPSGenerationSurfaceControl Control, double Value);
	double GetSurfaceControlValueForAutomation(EAPSGenerationSurfaceControl Control) const;
#endif

private:
	FText GetPreviewStatus() const;
	FText GetContinueLabel() const;
	FReply CommitWorld();
	FReply RefreshPreview();
	FReply GoBack();
	FReply FocusPreview(uint8 FocusValue);
	FReply FocusPreviewUp();
	FReply FocusPreviewBody(TWeakObjectPtr<AActor> BodyActor);
	FReply FocusPreviewHierarchyEntry(TWeakObjectPtr<AActor> BodyActor,
		int32 ClusterSystemInstanceIndex, int32 PreviewFocusValue);
	EActiveTimerReturnType RefreshBodyHierarchy(double CurrentTime, float DeltaTime);
	void RebuildBodyHierarchy(const TArray<FAPSPreviewBodyEntry>& Entries, uint32 Signature);
	FString GetHierarchyEntryKey(const FAPSPreviewBodyEntry& Entry) const;
	FReply ToggleHierarchyChildren(FString EntryKey);
	/** LIVE MODEL card, rebuilt only when its content changes (Rio 02.10: readable facts, not one text block). */
	void RefreshModelCard();
	void RebuildModelCard(const FAPSModelCard& Card);
	/** The name box follows the selected body or scope while it has no keyboard focus. */
	void RefreshNameBox();

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	FSimpleDelegate OnBack;
	FSimpleDelegate OnContinue;
	TSharedPtr<SVerticalBox> BodyHierarchyBox;
	TMap<EAPSGenerationSurfaceControl, TSharedPtr<SAPSGenerationRangeSlider>> SurfaceControlSliders;
	uint32 BodyHierarchySignature{0};
	TSet<FString> CollapsedHierarchyEntries;
	uint32 HierarchyExpansionRevision{0};
	FString PendingRenameBodyKey;
	TSharedPtr<SEditableTextBox> BodyNameTextBox;
	TWeakObjectPtr<AActor> LastHierarchySelection;
	TSharedPtr<SVerticalBox> ModelCardBox;
	FString ModelCardSignature;
	FString NameBoxKey;
	FString NameBoxValue;
};
