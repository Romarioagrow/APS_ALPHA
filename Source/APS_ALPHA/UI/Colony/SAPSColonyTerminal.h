#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class AAPSColonyModule;
class SAPSCivilizationMap;
class SAPSInfrastructurePanel;
class SAPSStarMapPanel;
class SAPSStarScheme;
class SAPSSystemScheme;
class SAPSSurfaceMap;
class ASpaceship;
class ASpaceShipyard;
class SVerticalBox;
class SWidgetSwitcher;
class UAPSColonyConstructionSubsystem;
struct FAPSColonyModuleSpec;
struct FAPSCivilizationJournalEntry;
struct FAPSProductionSnapshot;
struct FSlateBrush;

/**
 * Colony terminal (Rio, 29.09: one colony management menu out of atomic, reusable pieces). Tabs over live data only:
 * the civilization, the colony actors, construction at the base and on the headquarters, and the journal. Styled like
 * the main menu through UI/Style/APSMenuChrome.
 */
class SAPSColonyTerminal final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSColonyTerminal) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_EVENT(FSimpleDelegate, OnClose)
		SLATE_EVENT(FSimpleDelegate, OnOpenMap)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAPSColonyTerminal() override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	/** Rio 06.10 (audit: a focused child took Tab/K/F10): with aps.UI.TerminalButtonsNoFocus the terminal's hot keys are
	 * taken before any child sees them; Esc stays in OnKeyDown. */
	virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	/** 0 overview, 1 map, 2 infrastructure, 3 fleet orders, 4 divisions, 5 journal, 6 shipyard, 7 system scheme, 8 pilot,
	 * 9 surface map, 10 star map (test captures, K). Stars, map, scheme and surface are the modes of one MAP tab (Rio, 02.10). */
	void ShowTab(int32 TabIndex) { SelectTab(static_cast<ETab>(FMath::Clamp(TabIndex, 0, 10))); }
	bool IsShowingTab(int32 TabIndex) const { return static_cast<int32>(ActiveTab) == TabIndex; }
	/** Test captures: 1 the home planet's object page (INFRASTRUCTURE), 2 the construction catalogue, 0 neither. */
	void ShowTestOverlay(int32 Overlay);

private:
	enum class ETab : uint8
	{
		Overview,
		Map,
		Colony,
		Fleet,
		Divisions,
		Journal,
		Shipyard,
		Scheme,
		Pilot,
		Surface,
		/** Rio 05.10: the STAR level above the system map (appended: the indices above are the test captures'). */
		Stars
	};

	FReply SelectTab(ETab Tab);
	FReply Close();
	FReply OpenMap();
	/** Tab closes, K opens fleet command (and closes it from there), F10 opens the map; unset for any other key. Shared by
	 * OnKeyDown and OnPreviewKeyDown, so both act the same. */
	TOptional<FReply> HandleTerminalHotKey(const FKey& Key);
	TSharedRef<SWidget> BuildOverview();
	TSharedRef<SWidget> BuildColony();
	TSharedRef<SWidget> BuildConstruction();
	TSharedRef<SWidget> BuildJournal();
	/** Divisions: the six divisions of the civilization and their levels (the UMG civilization menu, 01.2026), what each
	 * changes in the game now and how it grows (Rio, 01.10). Cards: exploration, industry, science, civil affairs,
	 * military, fleet command; drawn by SAPSDivisionsPanel (Rio 02.10: gauges and chips instead of text lines). */
	TSharedRef<SWidget> BuildDivisions();
	/** Journal (Rio, 01.10: "more readable: fonts, icons"): newest first, each entry with its category's glyph and colour,
	 * and category chips with counts to filter. Built while the journal tab is shown. */
	void RebuildJournal();
	void HandleJournalEntry(const FAPSCivilizationJournalEntry& Entry);
	FReply SetJournalFilter(FName Category);
	/** The colony actors (base, pad, home ship), found once when the terminal opens. */
	void CacheColonyActors();
	FText DescribeColonyActor(int32 RoleIndex) const;
	/**
	 * Fleet command (K; Rio, 01.10): the civilization's ships by division, the map with their orders, and the order
	 * panel. A click on a ship card or a ship on the map picks it (Ctrl or Shift adds to the pick), a click on a world
	 * or station picks the target, and the order buttons light up for what the picked ships can do there. Cards are
	 * rebuilt when the units, their divisions or the filter change; their values are read live.
	 */
	TSharedRef<SWidget> BuildFleet();
	void RefreshFleet(bool bForceRebuild);
	TSharedRef<SWidget> BuildUnitCard(TWeakObjectPtr<ASpaceship> Ship);
	FReply PickUnit(TWeakObjectPtr<ASpaceship> Ship);
	FReply CycleUnitDivision(TWeakObjectPtr<ASpaceship> Ship);
	FReply SetDivisionFilter(int32 Filter);
	FReply PickAllShown();
	FReply GiveOrder(APSFleet::EOrder Order);
	FReply CancelPickedOrders();
	void HandleFleetMapSelection();
	TArray<ASpaceship*> GetPickedShips() const;
	/** Why no picked ship can take the order at the target; empty when at least one can. */
	FText OrderRefusal(APSFleet::EOrder Order) const;
	FAPSFleetCommand* GetFleet() const;
	/**
	 * B1 (Rio, 01.10: "studied planets and moons: their real look in the menu"): the fleet target's globe in the order
	 * panel. Rio 02.10 ("an empty mesh on a black background instead of the planet; remove the black"): a scene capture
	 * only saw the placeholder sphere (WorldScape builds the surface for the player's camera), so the globe is the
	 * surface map's own (SAPSSurfaceMap, globe only) on a transparent background, coarse while only surveyed.
	 */
	void UpdateBodyPreview();
	/**
	 * Shipyard (Rio, 01.10: "ships cannot be built in the game yet"; "on the right, which shipyard we build at"): the
	 * civilization's ship catalogue as cards with their baked thumbnails and class filters, and on the right the
	 * shipyards (the home one and those the construction ships built) with the slipway of the one picked. Each ship
	 * launches above its shipyard and joins fleet command; every shipyard builds at the same time.
	 */
	TSharedRef<SWidget> BuildShipyard();
	void RebuildShipyardCatalogue();
	/** Shipyard cards and the picked one's slipway rows, rebuilt when they change; progress is read live. */
	void RefreshShipyard(bool bForceRebuild);
	FReply OrderShipyardShip(int32 OptionIndex);
	FReply SelectShipyard(TWeakObjectPtr<ASpaceShipyard> Yard);
	FReply SetShipyardClassFilter(int32 Filter);
	/** The picked shipyard, else the home one. */
	ASpaceShipyard* GetSelectedYard() const;
	/** The main menu's baked thumbnail of a ship Blueprint, or null. */
	const FSlateBrush* ShipThumbnail(TSubclassOf<ASpaceship> ShipClass);

	UAPSColonyConstructionSubsystem* GetConstruction() const;
	FReply SelectSite(EAPSSpawnSite Site);
	FReply OrderModule(FName ModuleId);
	TSharedRef<SWidget> BuildModuleCard(const FAPSColonyModuleSpec& Spec);
	FText DescribeSite() const;
	/** The catalogue of the selected site. */
	void RebuildCatalogue();
	/** Queue and standing modules, rebuilt only when their contents change; progress bars read live values. */
	void RefreshConstruction();
	void RebuildQueue(const FAPSProductionSnapshot* Snapshot);
	void RebuildBuilt(const TArray<AAPSColonyModule*>& Modules);
	float JobProgress(const FGuid& JobId) const;

	/** Map: the system with the civilization's ships, stations and colony (SAPSCivilizationMap), the selected
	 * object with its course, and the objects of the current view as a list. A course goes to the piloted ship, or
	 * to the home ship when the pilot is on foot. */
	TSharedRef<SWidget> BuildMap();
	void RefreshMap();
	/** The mode bar shared by the MAP tab's modes: system map, system scheme, surface (fleet orders have their own tab). */
	TSharedRef<SWidget> BuildMapModes();
	bool IsMapTab() const
	{
		return ActiveTab == ETab::Map || ActiveTab == ETab::Scheme || ActiveTab == ETab::Surface || ActiveTab == ETab::Stars;
	}
	/** C6 (Rio, 02.10): the surface map of a world (SAPSSurfaceMap) with the worlds of the system to step through. */
	TSharedRef<SWidget> BuildSurface();
	FReply StepSurfaceBody(int32 Step);
	TSharedPtr<SAPSSurfaceMap> SurfaceMap;
	/** C2 (Rio, 02.10): the system in order, bodies to scale of size without distances (SAPSSystemScheme). */
	TSharedRef<SWidget> BuildScheme();
	/** C3 (Rio, 02.10): every infrastructure object, the home colony among stations, shipyards and outposts; each row
	 * opens its object page. */
	void RefreshInfrastructure(bool bForceRebuild);
	/** The INFRASTRUCTURE tab v2 (Rio 02.10): stocks, the network map, the construction catalogue, the holdings above and
	 * the object page every OPEN lands on (SAPSInfrastructurePanel). */
	TSharedPtr<SAPSInfrastructurePanel> InfrastructurePanel;
	/** P1 (Rio, 02.10): the old personal widget (WBP_CurrentStatus_UI) as the PILOT tab. Rio 04.10: a dashboard like the
	 * overview, read live four times a second while shown (SAPSPilotDashboard). */
	TSharedRef<SWidget> BuildPilot();
	/** C5 (Rio, 02.10): construction as a catalogue window over the fleet map: cards with what each structure needs,
	 * what it gives, whether the picked ships can raise it at the target, and the order. */
	TSharedRef<SWidget> BuildConstructionCatalogue();
	/** C4 (Rio, 02.10): the object picked on the map in a window: what it is, what the civilization knows and holds
	 * there, and what can be done (course, construction, shipyard, fleet orders). Rio 02.10 ("OPEN should open a full
	 * page"): now the object's page in the INFRASTRUCTURE tab; the window is no longer opened. */
	void OpenObjectWindow(AActor* Actor);
	void RebuildObjectWindow();
	FReply SetCourseToSelected();
	FReply ShowSystemView();
	FReply OpenLocalView();
	ASpaceship* GetCourseShip() const;

	TWeakObjectPtr<UWorld> World;
	FSimpleDelegate OnClose;
	FSimpleDelegate OnOpenMap;
	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<SVerticalBox> JournalList;
	FDelegateHandle JournalHandle;
	/** None: every category. */
	FName JournalFilter;
	TWeakObjectPtr<AActor> ColonyActors[3];
	ETab ActiveTab{ETab::Overview};
	TSharedPtr<SVerticalBox> FleetList;
	FString FleetSignature;
	TSharedPtr<SAPSCivilizationMap> FleetMap;
	TArray<TWeakObjectPtr<ASpaceship>> PickedUnits;
	TWeakObjectPtr<AActor> FleetTarget;
	/** -1 every division, else an APSFleet::EDivision. */
	int32 DivisionFilter{-1};
	FText FleetMessage;
	bool bFleetMessageIsError{false};
	/** The fleet map opens on the home planet's neighbourhood once, where the ships and moons are. */
	bool bFleetMapFocused{false};
	/** The fleet target's globe in the order panel (B1). */
	TSharedPtr<SAPSSurfaceMap> OrdersGlobe;

	TSharedPtr<SVerticalBox> ShipyardCatalogueBox;
	TSharedPtr<SVerticalBox> ShipyardQueueBox;
	TSharedPtr<SVerticalBox> ShipyardYardsBox;
	TArray<FAPSShipyardOption> ShipyardOptions;
	FString ShipyardSignature;
	FString ShipyardYardsSignature;
	TWeakObjectPtr<ASpaceShipyard> SelectedYard;
	/** -1 every class, else an ESpaceshipSizeClass. */
	int32 ShipyardClassFilter{-1};
	FText ShipyardMessage;
	bool bShipyardMessageIsError{false};
	bool bShipyardPresent{false};
	/** Thumbnails by Blueprint package; their textures are held while the terminal is open. */
	TMap<FString, TSharedPtr<FSlateBrush>> ShipThumbnails;
	TArray<TStrongObjectPtr<UObject>> ShipThumbnailTextures;

	/** Construction: the base by default is the headquarters (Rio, 29.09); the surface base when the pilot stands at it. */
	EAPSSpawnSite SelectedSite{EAPSSpawnSite::Orbit};
	TSharedPtr<SVerticalBox> CatalogueBox;
	TSharedPtr<SVerticalBox> QueueBox;
	TSharedPtr<SVerticalBox> BuiltBox;
	TMap<FGuid, float> JobProgressById;
	FString QueueSignature;
	int32 BuiltCount{-1};
	FText StatusMessage;
	bool bStatusIsError{false};
	float RefreshAccumulator{0.0f};

	TSharedPtr<SAPSCivilizationMap> Map;
	TSharedPtr<SAPSSystemScheme> Scheme;
	TSharedPtr<SVerticalBox> InfrastructureList;
	bool bConstructionOpen{false};
	TSharedPtr<SVerticalBox> ObjectWindowBox;
	TWeakObjectPtr<AActor> ObjectWindowActor;
	bool bObjectWindowOpen{false};
	FString InfrastructureSignature;
	TSharedPtr<SVerticalBox> MapList;
	FString MapListSignature;
	FText CourseMessage;
	bool bCourseIsError{false};

	/** Rio 05.10 (star map): MAP > STAR MAP (SAPSStarMapPanel), above the system map. */
	TSharedRef<SWidget> BuildStars();
	/** The star map's drill-down: the system scheme pinned to a system that stands (home, the materialized one). */
	void OpenSchemeOf(AActor* Star);
	/** FLEET ORDERS with a star system's anchor as the target and the map on STARS. */
	void OrderToSystem(const FGuid& SystemId);
	TSharedPtr<SAPSStarMapPanel> StarMapPanel;
	/** FLEET ORDERS' target map: the system (SYSTEM) or the stars around (STARS, the star scheme's picker style). */
	TSharedPtr<SAPSStarScheme> FleetStars;
	bool bFleetStars{false};
	/** The MAP tab reopens the mode used last. */
	ETab LastMapTab{ETab::Map};
	/** The drill-down's pin survives its own SelectTab(Scheme); the mode button shows the player's system again. */
	bool bKeepSchemePin{false};
};
