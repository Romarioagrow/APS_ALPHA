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

/**
 * Rio 13.10 (CURRENT_WORK.md, "подача авторских миров и происхождений"): the generation screen as a CARD. A ready world
 * (an authored world) or an origin is shown and never edited: the live preview with its levels on the left, the card's
 * texts and the world's facts on the right, START. The knob column, REGENERATE and renaming are not built at all.
 */
struct FAPSWorldCardInfo
{
	struct FLine
	{
		FText Label;
		FText Value;
	};
	struct FRung
	{
		FText Number;
		FText Title;
		FText Line;
		/** What opens the rung ("THE LAUNCH  ·  the monument"); empty for the rung that is open. */
		FText Key;
		bool bOpen{false};
	};
	/** Under the title instead of ASTRONOMICAL GENERATION. */
	FText HeaderSubtitle;
	FText Kicker;
	FText Title;
	FText Subtitle;
	FText Lore;
	TArray<FText> Tags;
	/** WHERE YOU START rows. */
	TArray<FLine> Start;
	/** THE LADDER rows (origins). */
	TArray<FRung> Ladder;
	/** RULES chips. */
	TArray<FLine> Rules;
	FText StartLabel;
	/** The line under the preview's level buttons. */
	FText Hint;
};

class SWorldGenerationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldGenerationPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_EVENT(FSimpleDelegate, OnContinue)
		/** Rio 13.10: set, the screen is a card (FAPSWorldCardInfo); null, the editor as before. */
		SLATE_ARGUMENT(TSharedPtr<FAPSWorldCardInfo>, Card)
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
	/** Rio 13.10: the card's right column (texts, start, ladder, the live facts, rules). */
	TSharedRef<SWidget> BuildCardColumn();

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	FSimpleDelegate OnBack;
	FSimpleDelegate OnContinue;
	TSharedPtr<FAPSWorldCardInfo> CardInfo;
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
