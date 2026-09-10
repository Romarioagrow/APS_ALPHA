#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

class AMainMenuController;
class SWorldGenerationPanel;
struct FStreamableHandle;
class SBox;
class SButton;
class SWidgetSwitcher;
class UClass;
class UGameSave;
class UUserWidget;

enum class EAPSMenuPage : uint8
{
	Landing,
	ChoosePath,
	ExistingWorlds,
	AstronomicalGeneration,
	Civilization,
	Profile,
	Settings
};

/** Code-native visual language for the Choose Your Path cards.  Keeping the
 * motif explicit avoids coupling navigation semantics to localized card text. */
enum class EAPSPathVisual : uint8
{
	LiveSystem,
	WorldArchive,
	CivilizationNetwork,
	GalaxySynthesis,
	PlanetLaboratory,
	StoryArchive
};

enum class EAPSGenerationSurfaceControl : uint8;

struct FAPSExistingWorldEntry
{
	FString SaveFileName;
	FString DisplayName;
	FString SystemType;
	FString StarType;
	FString PlanetType;
	FString Habitability{TEXT("UNKNOWN")};
	FString Environment;
	int32 TotalPlanets{0};
	int32 InhabitedPlanets{0};
	int64 FileTimestamp{0};
	int64 FileSizeBytes{0};
	bool bMetadataLoaded{false};
	bool bFavorite{false};
};

enum class EAPSWorldCollection : uint8
{
	All,
	MyWorlds,
	Favorites,
	Recent
};

enum class EAPSWorldSortMode : uint8
{
	LastPlayed,
	Name,
	SaveSize
};

enum class EAPSWorldFilterKind : uint8
{
	StarType,
	WorldType,
	Inhabited,
	Environment
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
	void ApplyExistingWorldMetadata(const FString& SlotName, const UGameSave* Save);

#if WITH_DEV_AUTOMATION_TESTS
	/** Opens and inspects the real Choose Your Path page for rendered UI tests. */
	void OpenChoosePathForAutomation();
	void GetChoosePathDiagnosticsForAutomation(int32& OutCardCount,
		int32& OutProceduralVisualCount, int32& OutStaticTextureResourceCount) const;
	bool FocusChoosePathCardForAutomation(int32 CardIndex);
	bool HoverChoosePathCardForAutomation(int32 CardIndex);
	bool ClearChoosePathCardInteractionsForAutomation();
	bool GetChoosePathCardInteractionForAutomation(int32 CardIndex,
		bool& bOutHovered, bool& bOutFocused) const;
	bool GetChoosePathCardNormalizedRectForAutomation(int32 CardIndex,
		FSlateRect& OutRect) const;

	/** Opens the real generation page for a rendered automation smoke test. */
	void OpenAstronomicalGenerationForAutomation(
		EAstroPreviewFocus Focus, EAPSGenerationRoute Route);
	bool CommitSurfaceControlForAutomation(
		EAPSGenerationSurfaceControl Control, double Value);
	double GetSurfaceControlValueForAutomation(
		EAPSGenerationSurfaceControl Control) const;
#endif

private:
	void Navigate(EAPSMenuPage NewPage);
	TSharedRef<SWidget> BuildLandingPage();
	TSharedRef<SWidget> BuildChoosePathPage();
	TSharedRef<SWidget> BuildExistingWorldsPage();
	TSharedRef<SWidget> BuildCivilizationPage();
	TSharedRef<SWidget> BuildProfilePage();
	TSharedRef<SWidget> BuildSettingsPage();
	TSharedRef<SWidget> BuildAuxiliaryPage(const FText& SectionTitle,
		TSoftClassPtr<UUserWidget>& WidgetClass, TWeakObjectPtr<UUserWidget>& WidgetInstance,
		const FText& LoadingText);
	TSharedRef<SWidget> BuildHeader(const FText& SectionTitle, bool bShowBack = true);
	TSharedRef<SWidget> BuildPathCard(const FText& Title, const FText& Description,
		EAPSPathVisual Visual, const FLinearColor& Accent, FSimpleDelegate Action,
		bool bLarge = false, bool bEnabled = true);
	TSharedRef<SWidget> BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label);

	void LoadVisualResources();
	void BeginAuxiliaryMenuLoad();
	void OnAuxiliaryMenuLoaded();
	void LoadExistingWorlds();
	void RebuildExistingWorldGrid();
	void RebuildExistingWorldDetails();
	void BeginExistingWorldMetadataLoad();
	bool PassesExistingWorldFilters(const FAPSExistingWorldEntry& Entry) const;
	FText GetWorldCollectionLabel(EAPSWorldCollection Collection) const;
	FText GetWorldSortLabel() const;
	FText GetWorldFilterLabel(EAPSWorldFilterKind Kind) const;
	void DiscoverSpawnClassOptions();
	void SynchronizeSpawnClassOptions();
	void ApplySpawnClassSelection(EAPSStartAssetSlot Slot);
	void OnSpawnClassSelectionLoaded(EAPSStartAssetSlot Slot, FSoftObjectPath RequestedPath);
	FText GetSpawnClassName(EAPSStartAssetSlot Slot) const;
	FText GetSpawnClassOptionName(EAPSStartAssetSlot Slot, int32 OptionIndex) const;
	FReply CycleSpawnClass(EAPSStartAssetSlot Slot, int32 Direction);
	FReply SelectSpawnClass(EAPSStartAssetSlot Slot, int32 OptionIndex);

	FReply Back();
	FReply OpenChoosePath();
	FReply StartSingleGame();
	FReply OpenExistingWorlds();
	FReply OpenAstronomicalGeneration(EAstroPreviewFocus Focus, EAPSGenerationRoute Route);
	void ContinueAstronomicalGeneration();
	FReply OpenCivilization();
	FReply OpenProfile();
	FReply OpenSettings();
	FReply CommitCivilization();
	FReply ContinueExistingWorld();
	FReply SelectExistingWorld(TSharedPtr<FAPSExistingWorldEntry> Entry);
	FReply ChangeExistingWorldPage(int32 Delta);
	FReply SetWorldCollection(EAPSWorldCollection Collection);
	FReply CycleWorldSort();
	FReply ToggleWorldView();
	FReply CycleWorldFilter(EAPSWorldFilterKind Kind);
	FReply ToggleWorldFavorite(TSharedPtr<FAPSExistingWorldEntry> Entry);
	FReply ToggleWorldDetails();
	FReply QuitGame();
	void OnWorldSearchChanged(const FText& SearchText);

	TWeakObjectPtr<AMainMenuController> Controller;
	TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	EAPSMenuPage CurrentPage{EAPSMenuPage::Landing};
	EAPSMenuPage PreviousPage{EAPSMenuPage::Landing};
	/** The enum defaults to Landing before the first widget tree is populated. */
	bool bHasBuiltCurrentPage{false};

	TSharedPtr<SBox> ContentHost;
	TSharedPtr<SWorldGenerationPanel> WorldGenerationPanel;
	TSharedPtr<SBox> ExistingWorldGridHost;
	TSharedPtr<SBox> ExistingWorldDetailsHost;
	TArray<TSharedPtr<FAPSExistingWorldEntry>> ExistingWorlds;
	TSharedPtr<FAPSExistingWorldEntry> SelectedWorld;
	FString WorldSearch;
	int32 ExistingWorldPage{0};
	EAPSWorldCollection WorldCollection{EAPSWorldCollection::All};
	EAPSWorldSortMode WorldSortMode{EAPSWorldSortMode::LastPlayed};
	TMap<EAPSWorldFilterKind, int32> WorldFilterIndices;
	bool bShowTechnicalWorldDetails{false};
	bool bCompactWorldList{false};

	TMap<EAPSStartAssetSlot, TArray<TSoftClassPtr<AActor>>> SpawnClassOptions;
	TMap<EAPSStartAssetSlot, int32> SpawnClassIndices;
	TMap<EAPSStartAssetSlot, TSharedPtr<FStreamableHandle>> SpawnSelectionLoadHandles;
	/** Identity guard for async picker loads; stale callbacks must not clear a newer slot request. */
	TMap<EAPSStartAssetSlot, FSoftObjectPath> SpawnSelectionRequestedPaths;
	bool bSpawnClassOptionsDiscovered{false};
	TSharedPtr<SWidgetSwitcher> CivilizationEditorSwitcher;
	int32 CivilizationEditorSection{0};

	TSoftClassPtr<UUserWidget> SettingsPanelClass;
	TSoftClassPtr<UUserWidget> ProfilePanelClass;
	TWeakObjectPtr<UUserWidget> SettingsPanelInstance;
	TWeakObjectPtr<UUserWidget> ProfilePanelInstance;
	TSharedPtr<FStreamableHandle> AuxiliaryMenuLoadHandle;

	FSlateBrush SystemImage;
	FSlateBrush PlanetImage;
	FSlateBrush GalaxyImage;
	FSlateBrush ClusterImage;
	FSlateBrush CivilizationImage;
	FSlateBrush BackgroundImage;

	FButtonStyle PrimaryButtonStyle;
	FButtonStyle SecondaryButtonStyle;
	FButtonStyle CardButtonStyle;
	FButtonStyle DisabledCardButtonStyle;
	FScrollBarStyle ScrollBarStyle;

#if WITH_DEV_AUTOMATION_TESTS
	TArray<TWeakPtr<SButton>> ChoosePathCardButtons;
	int32 ChoosePathProceduralVisualCount{0};
	int32 ChoosePathStaticTextureResourceCount{0};
#endif
};
