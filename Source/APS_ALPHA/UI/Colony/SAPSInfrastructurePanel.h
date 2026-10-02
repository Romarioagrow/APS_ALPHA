#pragma once

#include "CoreMinimal.h"
#include "SAPSObjectPage.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class AActor;
class SVerticalBox;
class SWidgetSwitcher;
class SWrapBox;
class UWorld;

/**
 * The INFRASTRUCTURE screen's network map (Rio 02.10: "a separate interaction map for the infrastructure, transport
 * first"). The home system in the middle (its star, the planets on a logarithmic scale to the system's edge, the moons
 * beside their planets) and around it the star systems the civilization knows or holds, with the nearest uncharted ones
 * faint (the next probes); what stands at each place as small icons in its department's colour (a shape per category),
 * the relay links of the network (FAPSStarSystems::GetNetwork) with traffic running along them, the home network's reach,
 * the cargo lanes of the cargo hubs, transport and relays ringed. Wheel zooms, a drag pans, a click opens the page of the
 * place or of the structure under the cursor.
 */
class SAPSInfrastructureMap final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSInfrastructureMap) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_EVENT(FAPSOnOpenObject, OnOpenObject)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	/** Re-reads the places, structures and links (the panel calls it twice a second while the map is shown). */
	void Refresh();
	/** One line on what the map shows: structures, systems held, links. */
	FText GetSummary() const { return Summary; }

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(640.0, 520.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	enum class ENodeKind : uint8
	{
		Home,
		Planet,
		Moon,
		System
	};

	/** A place: the home star, a planet or moon of the home system, or a star system of the cluster. */
	struct FNode
	{
		ENodeKind Kind{ENodeKind::Planet};
		/** The body or the home star; star systems resolve their anchor when clicked. */
		TWeakObjectPtr<AActor> Actor;
		FGuid SystemId;
		FText Name;
		FText Detail;
		FLinearColor Colour{FLinearColor::White};
		/** Map units: the home system's edge at 1. */
		FVector2D Position{FVector2D::ZeroVector};
		/** Moons: their place beside the planet in pixels (growing slowly with the zoom), so they stay readable. */
		FVector2D Fan{FVector2D::ZeroVector};
		/** Star systems: what the civilization knows there (APSStars::EKnowledge). */
		uint8 Knowledge{0};
		bool bClaimed{false};
		bool bInReach{false};
		/** Something of the civilization stands there; a transport structure or a relay does. */
		bool bHeld{false};
		bool bTransport{false};
		/** A claimed system's relay reach as a ring around it, map units. */
		double ReachUnits{0.0};
	};

	/** A structure at a place: a catalogue structure, a fleet station, an outpost or a settlement. */
	struct FIcon
	{
		int32 Node{INDEX_NONE};
		TWeakObjectPtr<AActor> Actor;
		FText Name;
		FText Detail;
		FLinearColor Colour{FLinearColor::White};
		/** APSInfrastructure::ECategory, or a fleet structure's or settlement's own shape (see the .cpp). */
		uint8 Shape{0};
		bool bTransport{false};
		bool bLarge{false};
	};

	/** A relay link between two systems, or a cargo hub's lane to another world of its system. */
	struct FLane
	{
		int32 From{INDEX_NONE};
		int32 To{INDEX_NONE};
		bool bRelay{true};
	};

	/** Pixels per map unit for the widget's size and the zoom. */
	double UnitPixels(const FVector2D& Size) const;
	/** The icon or node under a point (icons first); false when nothing is. */
	bool HitTest(const FVector2D& Local, int32& OutNode, int32& OutIcon) const;
	void OpenAt(const FVector2D& Local);

	TWeakObjectPtr<UWorld> World;
	FAPSOnOpenObject OnOpenObject;
	TArray<FNode> Nodes;
	TArray<FIcon> Icons;
	TArray<FLane> Lanes;
	/** The home network's reach (map units), 0 when it is not known. */
	double HomeReach{0.0};
	FText Summary;
	/** Wheel zoom (1 = everything) and the view centre's offset from the widget centre, pixels. */
	double MapZoom{1.0};
	FVector2D MapOffset{FVector2D::ZeroVector};
	bool bPressed{false};
	bool bPanning{false};
	FVector2D PressPosition{FVector2D::ZeroVector};
	FVector2D PressOffset{FVector2D::ZeroVector};
	int32 HoverNode{INDEX_NONE};
	int32 HoverIcon{INDEX_NONE};
	/** Where nodes and icons were painted last (local space), for picking. */
	mutable TArray<FVector2D> NodePixels;
	mutable TArray<FVector2D> IconPixels;
};

/**
 * The colony terminal's INFRASTRUCTURE tab (Rio 02.10: "greatly expand the infrastructure and the interaction: a separate
 * interaction map, transport first; many structures, megastructures, relays, outposts of different types, buildings for
 * each division; manage and build all of it from all our screens. Now it is just a list and it is unclear what to do
 * with it"). Over everything the civilization's stocks with their rates per minute; then three sections and the object
 * page:
 *  - NETWORK MAP: SAPSInfrastructureMap, and beside it the claimed systems, the expansion targets and the transport.
 *  - CONSTRUCTION CATALOGUE: every structure type by department with what it needs, costs, yields and changes, how many
 *    known places take it now; BUILD... picks the place (each with the runtime's reason) and runs the construction
 *    provider's own action there (APSObjectActions "Build.<type>"), so the fleet's rules and ship pick stay in one place.
 *  - HOLDINGS: the terminal's lists (home colony, stations, shipyards, outposts) and the colony's module construction.
 *  - The object page (SAPSObjectPage): OPEN on any screen lands here; BACK returns to the section or the previous page.
 */
class SAPSInfrastructurePanel final : public SCompoundWidget
{
public:
	enum class ESection : uint8
	{
		Network,
		Catalogue,
		Holdings
	};

	SLATE_BEGIN_ARGS(SAPSInfrastructurePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		/** The terminal's lists and the colony's module construction (the HOLDINGS section). */
		SLATE_ARGUMENT(TSharedPtr<SWidget>, Holdings)
		/** The terminal's own actions for an object on its page (its tabs). */
		SLATE_ARGUMENT(SAPSObjectPage::FExtraActions, ExtraActions)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	/** OPEN from another screen: the object's page in this tab; BACK returns to the section last shown here. */
	void ShowObject(AActor* Object);
	/** OPEN within this tab (the map, the lists, a page's rows): the page before it is kept for BACK. */
	void OpenObject(AActor* Object);
	void ShowSection(ESection Section);

private:
	/** The switcher's views: the three sections, then the object page. */
	static constexpr int32 PageView = 3;

	TSharedRef<SWidget> BuildStocks();
	TSharedRef<SWidget> BuildSections();
	TSharedRef<SWidget> BuildNetwork();
	TSharedRef<SWidget> BuildCatalogue();
	TSharedRef<SWidget> BuildPicker();
	TSharedRef<SWidget> BuildCard(FName TypeId);
	void RebuildCards();
	FReply SetDepartmentFilter(int32 Filter);
	void SetView(int32 View);
	void Back();
	FText BackLabel() const;

	/** The places construction can aim at: the home system's worlds, its star and the known or held star systems. */
	void CollectSites(TArray<AActor*>& OutSites) const;
	/** How many places take each type now, the departments' levels and the builds under way (once a second). */
	void RecountPlaces();
	/** The summary over the sections, from the runtime's counts. */
	void RefreshSummary();
	/** The lists beside the network map: claimed systems, expansion targets, transport and relays. */
	void RefreshNetworkLists(bool bForce);
	/** A star system's place for its page and orders: the home star for the home, else its anchor. */
	AActor* SystemSite(const FGuid& SystemId) const;

	FReply OpenPicker(FName TypeId);
	FReply ClosePicker();
	void RefreshPicker(bool bForce);
	/** Runs the construction provider's own action for this type at this place. */
	FReply BuildAt(FName TypeId, TWeakObjectPtr<AActor> Site);

	TWeakObjectPtr<UWorld> World;
	SAPSObjectPage::FExtraActions ExtraActions;
	TSharedPtr<SWidgetSwitcher> Views;
	int32 ActiveView{0};
	ESection ReturnSection{ESection::Network};
	/** The pages opened from a page, for BACK. */
	TArray<TWeakObjectPtr<AActor>> History;
	TSharedPtr<SAPSObjectPage> Page;
	TSharedPtr<SAPSInfrastructureMap> NetworkMap;
	TSharedPtr<SVerticalBox> NetworkLists;
	FString NetworkListsSignature;
	FText SummaryText;

	TSharedPtr<SWrapBox> CardsBox;
	/** -1 every department, else an APSInfrastructure::EDepartment. */
	int32 DepartmentFilter{-1};
	TMap<FName, int32> PlacesNow;
	TMap<FName, FString> PlacesNames;
	TMap<FName, int32> BuildsUnderWay;
	int32 DepartmentLevels[static_cast<int32>(APSInfrastructure::EDepartment::Count)]{};
	FText CatalogueMessage;
	bool bCatalogueMessageIsError{false};

	/** The site picker over the catalogue: the type being placed (None: closed). */
	FName PickerType;
	TSharedPtr<SVerticalBox> PickerRows;
	FString PickerSignature;
	FText PickerMessage;
	bool bPickerMessageIsError{false};

	float RefreshClock{0.0f};
	int32 RefreshCount{0};
	double LastTickSeconds{0.0};
};
