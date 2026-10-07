#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/UI/MainMenu/APSWorldBrowserMetadata.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

class AMainMenuController;
class SWorldGenerationPanel;
struct FStreamableHandle;
class SBox;
class SButton;
class SMenuAnchor;
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

/** Rio 06.10: NEW WORLD's paths (Docs/Design/MAIN_MENU_OBSERVATORY.md section 3.4); PLANET waits for Planet Lab. */
enum class EAPSNewWorldPath : uint8
{
	SingleGame,
	Civilization,
	Space,
	Planet
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
	/** Rio 03.10: the true home system from a version 2 sidecar. Older records only echo the menu's editor buffer (a G
	 * star and one frozen planet in almost every save), so the browser does not present their system fields as facts. */
	bool bSystemRecorded{false};
	FAPSWorldSystemRecord System;
};

/** Rio 03.10: MY WORLDS was dropped. It listed exactly ALL WORLDS: every save is made by this game on this machine, and
 * no record tells "mine" from anything else. */
enum class EAPSWorldCollection : uint8
{
	All,
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

/** One choice of a world browser filter: built from the scanned worlds, with how many the choice would show. */
struct FAPSWorldFilterOption
{
	/** Empty for ANY. */
	FString Key;
	FText Label;
	int32 Count{0};
};

enum class EAPSSettingsTab : uint8;

class SAPSMainMenuRoot final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSMainMenuRoot) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AMainMenuController>, Controller)
		SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
	SLATE_END_ARGS()

	SAPSMainMenuRoot();
	virtual ~SAPSMainMenuRoot() override;
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	/** Rio 06.10: the Observatory landing page's letter keys (C N W P S, Q twice, Enter). */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	void ApplyExistingWorldMetadata(const FString& SlotName, const UGameSave* Save);
	/** Rio 06.10 checks (aps.Menu.Open, aps.Menu.ThemeShots): opens a page by name (Landing, NewWorld, Worlds, Settings,
	 * Profile) and, for Settings, a tab (Video, Graphics, Audio, Interface). False for an unknown name. */
	bool OpenPageByName(const FString& Page, const FString& Tab);

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
	/** Rio 06.10: the button styles from the active interface theme (APSMenu::ApplyTheme first). */
	void BuildButtonStyles();
	/** Rio 06.10: a theme switch (SETTINGS / INTERFACE) recolours the menu at once: palette, styles, this page. */
	void HandleThemeChanged();
	void RebuildCurrentPage();
	FDelegateHandle ThemeChangedHandle;
	/** Rio 06.10: SINGLE GAME's maps as a grid of cards with a rendered preview (Content/Slate/MapPreviews). */
	TSharedRef<SWidget> BuildAuthoredMapGrid();
	TSharedPtr<FSlateBrush> MapPreviewBrush;
	/** Rio 06.10: the SETTINGS page's open tab, kept while a theme switch rebuilds the page. */
	EAPSSettingsTab SettingsTab{};
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
	TSharedRef<SWidget> BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label);
	/** Rio 06.10: a card's classes as a grid of large cards over the page (the CLASS button opens it, Esc closes). */
	void OpenSpawnPicker(EAPSStartAssetSlot Slot);
	void CloseSpawnPicker();
	TSharedRef<SWidget> BuildSpawnPicker(EAPSStartAssetSlot Slot);
	TSharedPtr<SBox> SpawnPickerHost;
	bool bSpawnPickerOpen{false};

	void LoadVisualResources();
	void BeginAuxiliaryMenuLoad();
	void OnAuxiliaryMenuLoaded();
	void LoadExistingWorlds();
	void RefreshExistingWorlds();
	uint32 ComputeExistingWorldDirectoryFingerprint() const;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime,
		float InDeltaTime) override;
	void RebuildExistingWorldGrid();
	void RebuildExistingWorldDetails();
	/** Rio 03.10: the confirmation modal of DELETE WORLD, shown over the browser while PendingDeleteWorld is set. */
	TSharedRef<SWidget> BuildDeleteWorldDialog();
	/** False for non-world slots and for a world a running game holds (OutReason says why). */
	bool CanDeleteExistingWorld(const FAPSExistingWorldEntry& Entry, FText* OutReason = nullptr) const;
	/** A load of the slot being handed to gameplay here, or another game world in this process playing it. */
	bool IsWorldSlotInUse(const FString& SlotName) const;
	FReply RequestDeleteExistingWorld();
	FReply CancelDeleteExistingWorld();
	FReply ConfirmDeleteExistingWorld();
	/** Removes one world's save and sidecar (the in-use and non-world checks first); OutFailure says why not. */
	bool DeleteWorldFiles(const FAPSExistingWorldEntry& Entry, FString& OutFailure, bool bLogEach);
	/** Rio 03.10: DELETE ALL removes every world the collection, search and filters list (ListedWorlds), after a
	 * confirmation whose button arms only after a countdown. */
	TSharedRef<SWidget> BuildDeleteAllDialog();
	FReply RequestDeleteListedWorlds();
	FReply CancelDeleteListedWorlds();
	FReply ConfirmDeleteListedWorlds();
	void BeginExistingWorldMetadataLoad();
	/** Collection, search and every filter. */
	bool PassesExistingWorldFilters(const FAPSExistingWorldEntry& Entry) const;
	bool PassesWorldCollectionAndSearch(const FAPSExistingWorldEntry& Entry) const;
	bool MatchesWorldFilter(const FAPSExistingWorldEntry& Entry, EAPSWorldFilterKind Kind) const;
	/** True when the collection, the search or a filter narrows the list. */
	bool HasWorldNarrowing() const;
	/** Rebuilds the options of every filter from the scanned worlds, counted under the other filters. */
	void RebuildWorldFilterOptions();
	/** Grid, then the selection (dropped if no longer listed), then the details. */
	void ApplyExistingWorldView();
	TSharedRef<SWidget> BuildWorldFilterMenu(EAPSWorldFilterKind Kind);
	FReply ToggleWorldFilterMenu(EAPSWorldFilterKind Kind);
	FReply SelectWorldFilter(EAPSWorldFilterKind Kind, FString Key);
	FReply ClearWorldFilters();
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
	/** Station, headquarters and shipyard cards: start the pilot in orbit aboard that station. */
	FReply SetStartHere(EAPSStartAssetSlot Slot);
	bool IsStartHere(EAPSStartAssetSlot Slot) const;
	void RefreshSpawnThumbnails();
	const FSlateBrush* GetSpawnClassThumbnail(EAPSStartAssetSlot Slot, int32 OptionIndex) const;
	const FSlateBrush* GetSelectedSpawnThumbnail(EAPSStartAssetSlot Slot) const;

	FReply Back();
	FReply OpenChoosePath();
	/** CONTINUE: opens the newest world (LatestWorld). */
	FReply ContinueLatestWorld();
	/** NEW WORLD: picks a path card (PLANET is not ready and is never picked), and runs the picked path. */
	FReply PickNewWorldPath(EAPSNewWorldPath Path);
	FReply RunNewWorldPath();
	bool IsSplashVisible() const;
	EActiveTimerReturnType TickSplash(double InCurrentTime, float InDeltaTime);
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
	/** Rio 06.10: the newest world, CONTINUE's target; set whenever the landing page is built. */
	TSharedPtr<FAPSExistingWorldEntry> LatestWorld;
	/** The start-up title screen over the menu, and its lift (TickSplash). */
	TSharedPtr<SWidget> Splash;
	double SplashShownAt{-1.0};
	double SplashLiftStarted{-1.0};
	int32 SplashCalmFrames{0};
	/** The landing page's Q: the first press arms it, a second one before this time quits. */
	double QuitArmedUntil{0.0};
	/** NEW WORLD's picked path, kept while the menu stays open. */
	EAPSNewWorldPath NewWorldPath{EAPSNewWorldPath::Civilization};
	/** The world the delete confirmation asks about; the modal is open while it is set. */
	TSharedPtr<FAPSExistingWorldEntry> PendingDeleteWorld;
	/** One line in the details panel: "... was deleted", or why a deletion failed; cleared by the next selection. */
	FText WorldBrowserNotice;
	/** The worlds DELETE ALL asks about; its modal is open while this is not empty. */
	TArray<TSharedPtr<FAPSExistingWorldEntry>> PendingBulkDelete;
	/** Application time at which the DELETE ALL confirmation arms. */
	double BulkDeleteArmTime{0.0};
	/** What the grid lists now: collection, search and filters applied, before paging. */
	TArray<TSharedPtr<FAPSExistingWorldEntry>> ListedWorlds;
	FString WorldSearch;
	int32 ExistingWorldPage{0};
	EAPSWorldCollection WorldCollection{EAPSWorldCollection::All};
	EAPSWorldSortMode WorldSortMode{EAPSWorldSortMode::LastPlayed};
	/** The chosen option key of each filter; absent = ANY. Survives rescans. */
	TMap<EAPSWorldFilterKind, FString> WorldFilterChoices;
	TMap<EAPSWorldFilterKind, TArray<FAPSWorldFilterOption>> WorldFilterOptions;
	TMap<EAPSWorldFilterKind, TSharedPtr<SMenuAnchor>> WorldFilterAnchors;
	uint32 ExistingWorldDirectoryFingerprint{0};
	double NextExistingWorldRefreshTime{0.0};
	bool bShowTechnicalWorldDetails{false};
	bool bCompactWorldList{false};

	TMap<EAPSStartAssetSlot, TArray<TSoftClassPtr<AActor>>> SpawnClassOptions;
	TMap<EAPSStartAssetSlot, int32> SpawnClassIndices;
	/** Option captions aligned with SpawnClassOptions, fixed when the list is built: loading a class while picking no
	 * longer renames it or its twins (Rio 02.10). */
	mutable TMap<EAPSStartAssetSlot, TArray<FText>> SpawnClassCaptions;
	/** Baked Blueprint thumbnails aligned with SpawnClassOptions; null where none was baked. */
	TMap<EAPSStartAssetSlot, TArray<TSharedPtr<FSlateBrush>>> SpawnClassThumbnails;
	/** One brush per Blueprint package, so each thumbnail texture loads once. */
	TMap<FName, TSharedPtr<FSlateBrush>> SpawnThumbnailBrushCache;
	/** Start Blueprints left out by the basic mesh rule or UAPSStartAssetFilter; never re-added as defaults. */
	TSet<FSoftObjectPath> HiddenStartClasses;
	/** Hidden legacy duplicate -> the listed Blueprint with the same name, for default selection. */
	TMap<FSoftObjectPath, FSoftObjectPath> DuplicateStartClasses;
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
	/** Secondary at rest, muted red under the pointer (DELETE WORLD). */
	FButtonStyle DangerButtonStyle;
	/** The confirmation's DELETE: red at rest. */
	FButtonStyle DangerConfirmButtonStyle;
	/** A row of a filter dropdown: bare at rest, lit under the pointer. */
	FButtonStyle DropdownOptionStyle;
	FScrollBarStyle ScrollBarStyle;

#if WITH_DEV_AUTOMATION_TESTS
	TArray<TWeakPtr<SButton>> ChoosePathCardButtons;
	int32 ChoosePathProceduralVisualCount{0};
	int32 ChoosePathStaticTextureResourceCount{0};
#endif
};
