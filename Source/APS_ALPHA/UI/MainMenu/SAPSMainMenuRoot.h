#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

class AMainMenuController;
class SBox;
class UClass;

enum class EAPSMenuPage : uint8
{
	Landing,
	ChoosePath,
	ExistingWorlds,
	AstronomicalGeneration,
	Civilization
};

struct FAPSExistingWorldEntry
{
	FString SaveFileName;
	FString DisplayName;
	FString SystemType;
	FString StarType;
	FString PlanetType;
	FString Environment;
	int32 TotalPlanets{0};
	int32 InhabitedPlanets{0};
	int64 FileTimestamp{0};
};

class SAPSMainMenuRoot final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSMainMenuRoot) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AMainMenuController>, Controller)
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
	SLATE_END_ARGS()

	SAPSMainMenuRoot();
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	void Navigate(EAPSMenuPage NewPage);
	TSharedRef<SWidget> BuildLandingPage();
	TSharedRef<SWidget> BuildChoosePathPage();
	TSharedRef<SWidget> BuildExistingWorldsPage();
	TSharedRef<SWidget> BuildCivilizationPage();
	TSharedRef<SWidget> BuildHeader(const FText& SectionTitle, bool bShowBack = true);
	TSharedRef<SWidget> BuildPathCard(const FText& Title, const FText& Description,
		const FSlateBrush* Image, const FLinearColor& Accent, FSimpleDelegate Action,
		bool bLarge = false, bool bEnabled = true);
	TSharedRef<SWidget> BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label);

	void LoadVisualResources();
	void LoadExistingWorlds();
	void RebuildExistingWorldGrid();
	void RebuildExistingWorldDetails();
	void LoadSpawnClassOptions();
	void RefreshSpawnClassBrush(EAPSStartAssetSlot Slot);
	FText GetSpawnClassName(EAPSStartAssetSlot Slot) const;
	const FSlateBrush* GetSpawnClassBrush(EAPSStartAssetSlot Slot) const;
	FReply CycleSpawnClass(EAPSStartAssetSlot Slot, int32 Direction);

	FReply Back();
	FReply OpenChoosePath();
	FReply StartSingleGame();
	FReply OpenExistingWorlds();
	FReply OpenAstronomicalGeneration(EAstroPreviewFocus Focus);
	FReply OpenCivilization();
	FReply CommitCivilization();
	FReply ContinueExistingWorld();
	FReply SelectExistingWorld(TSharedPtr<FAPSExistingWorldEntry> Entry);
	FReply QuitGame();
	void OnWorldSearchChanged(const FText& SearchText);

	TWeakObjectPtr<AMainMenuController> Controller;
	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	EAPSMenuPage CurrentPage{EAPSMenuPage::Landing};
	EAPSMenuPage PreviousPage{EAPSMenuPage::Landing};

	TSharedPtr<SBox> ContentHost;
	TSharedPtr<SBox> ExistingWorldGridHost;
	TSharedPtr<SBox> ExistingWorldDetailsHost;
	TArray<TSharedPtr<FAPSExistingWorldEntry>> ExistingWorlds;
	TSharedPtr<FAPSExistingWorldEntry> SelectedWorld;
	FString WorldSearch;

	TMap<EAPSStartAssetSlot, TArray<TSubclassOf<AActor>>> SpawnClassOptions;
	TMap<EAPSStartAssetSlot, int32> SpawnClassIndices;
	TMap<EAPSStartAssetSlot, FSlateBrush> SpawnClassBrushes;

	FSlateBrush SystemImage;
	FSlateBrush PlanetImage;
	FSlateBrush GalaxyImage;
	FSlateBrush ClusterImage;
	FSlateBrush CivilizationImage;

	FButtonStyle PrimaryButtonStyle;
	FButtonStyle SecondaryButtonStyle;
	FButtonStyle CardButtonStyle;
	FButtonStyle DisabledCardButtonStyle;
	FScrollBarStyle ScrollBarStyle;
};
