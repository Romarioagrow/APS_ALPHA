#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class AAPSColonyModule;
class SAPSCivilizationMap;
class ASpaceship;
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
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	/** 0 overview, 1 map, 2 colony, 3 fleet command, 4 divisions, 5 journal, 6 shipyard (test captures, K). */
	void ShowTab(int32 TabIndex) { SelectTab(static_cast<ETab>(FMath::Clamp(TabIndex, 0, 6))); }
	bool IsShowingTab(int32 TabIndex) const { return static_cast<int32>(ActiveTab) == TabIndex; }

private:
	enum class ETab : uint8
	{
		Overview,
		Map,
		Colony,
		Fleet,
		Divisions,
		Journal,
		Shipyard
	};

	FReply SelectTab(ETab Tab);
	FReply Close();
	FReply OpenMap();
	TSharedRef<SWidget> BuildOverview();
	TSharedRef<SWidget> BuildColony();
	TSharedRef<SWidget> BuildConstruction();
	TSharedRef<SWidget> BuildJournal();
	/** Divisions: the six divisions of the civilization and their levels (the UMG civilization menu, 01.2026). */
	TSharedRef<SWidget> BuildDivisions();
	void RebuildJournal();
	void HandleJournalEntry(const FAPSCivilizationJournalEntry& Entry);
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
	 * Shipyard (Rio, 01.10: "ships cannot be built in the game yet"): the civilization's ship catalogue as cards with
	 * their baked thumbnails, and the slipway. Each ship launches above the shipyard and joins fleet command.
	 */
	TSharedRef<SWidget> BuildShipyard();
	void RebuildShipyardCatalogue();
	/** Slipway rows, rebuilt when the queue changes; their progress is read live. */
	void RefreshShipyard(bool bForceRebuild);
	FReply OrderShipyardShip(int32 OptionIndex);
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

	TSharedPtr<SVerticalBox> ShipyardCatalogueBox;
	TSharedPtr<SVerticalBox> ShipyardQueueBox;
	TArray<FAPSShipyardOption> ShipyardOptions;
	FString ShipyardSignature;
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
	TSharedPtr<SVerticalBox> MapList;
	FString MapListSignature;
	FText CourseMessage;
	bool bCourseIsError{false};
};
