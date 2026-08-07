#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorldGenerationViewModel;
class SAPSGenerationRangeSlider;
class SVerticalBox;
struct FAPSPreviewBodyEntry;

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

	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	FSimpleDelegate OnBack;
	FSimpleDelegate OnContinue;
	TSharedPtr<SVerticalBox> BodyHierarchyBox;
	TMap<EAPSGenerationSurfaceControl, TSharedPtr<SAPSGenerationRangeSlider>> SurfaceControlSliders;
	uint32 BodyHierarchySignature{0};
};
