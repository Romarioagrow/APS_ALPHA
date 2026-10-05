#pragma once

#include "CoreMinimal.h"
#include "APSStarMapModel.h"
#include "APS_ALPHA/UI/StrategicMap/APSStrategicMapTypes.h"
#include "Widgets/SLeafWidget.h"

class ASpaceship;
class UWorld;

/**
 * Rio 05.10 (star map): the STAR level of the maps, above the system map (approved mock-ups star_level_mock.png and
 * fleet_target_mock.png). The neighbouring systems on rings by rank around the centre system (FAPSStarMapModel): each star
 * a disc in its spectral colour, sized by class, a companion dot for a multiple system, its knowledge ring (dashed
 * catalogued, blue scanned, green surveyed), an F10-style plate ("M5V  /  SCANNED", name, designation) with its worlds as
 * dots, and the civilization's marks above it (claimed, colony, HQ, outposts, anomaly, ships); routes of the fleet in
 * division colours with each ship on its share of the way, work as a progress arc, the relay network in teal, the planned
 * route to the pick in amber. Labels go by priority (the pick, home, claimed, the fleet's, the known) into free room; an
 * uncharted star gets only its name, or shows it on hover when crowded. Wheel zooms, a drag pans, a click picks, a double
 * click opens. Two styles: Full (MAP > STAR MAP) and Picker (FLEET ORDERS' target, names only but the target's plate).
 * The model is re-read twice a second while the widget is shown; the labels are laid out again only when what is drawn
 * where changes.
 */
class APS_ALPHA_API SAPSStarScheme final : public SLeafWidget
{
public:
	enum class EStyle : uint8
	{
		Full,
		Picker
	};
	DECLARE_DELEGATE_OneParam(FOnSystem, const FGuid&);

	/** A planned order: the ship's way from where it is to the pick, amber, with its label ("E-07  ETA ~0:38"). */
	struct FPlannedRoute
	{
		TWeakObjectPtr<ASpaceship> Ship;
		FText Label;
	};

	SLATE_BEGIN_ARGS(SAPSStarScheme)
		: _Style(EStyle::Full)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_ARGUMENT(EStyle, Style)
		/** A click on a star picks it. */
		SLATE_EVENT(FOnSystem, OnPicked)
		/** A double click opens it. */
		SLATE_EVENT(FOnSystem, OnOpened)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	/** Re-reads the model now (it refreshes itself twice a second while shown). */
	void Refresh(bool bForce = false);

	/** The pick: kept on the map wherever it is, and its plate first. */
	void SetSelected(const FGuid& Id);
	const FGuid& GetSelected() const { return Selected; }
	/** The system in the middle (invalid: home). */
	void SetCentre(const FGuid& Id);
	const FGuid& GetCentre() const { return Centre; }
	void SetFilter(APSStarMap::EFilter InFilter) { Filter = InFilter; }
	APSStarMap::EFilter GetFilter() const { return Filter; }
	void SetPlannedRoutes(const TArray<FPlannedRoute>& Routes);
	/** Ships ringed in white where they are (the fleet's pick). */
	void SetHighlightedShips(const TArray<TWeakObjectPtr<ASpaceship>>& Ships);
	const APSStarMap::FSnapshot& GetSnapshot() const { return Model.Get(); }
	const FAPSStarMapModel& GetModel() const { return Model; }
	/** "STARS  /  AROUND AVELIN" and "27 OF 2,847 SYSTEMS  /  10 KNOWN", for the overlay over the map. */
	FText GetTitle() const;
	FText GetSubtitle() const;

	/** Rio 05.10: the legend and hint lines under the star maps, on or off for all of them, remembered in GameUserSettings. */
	static bool IsLegendShown();
	static void SetLegendShown(bool bShown);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(640.0, 520.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void OnMouseLeave(const FPointerEvent& Event) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const override;

private:
	/** Where things land in the widget for a size, zoom and pan. */
	struct FFrame
	{
		FVector2D Size{FVector2D::ZeroVector};
		FVector2D Centre{FVector2D::ZeroVector};
		double PixelsPerUnit{126.0};
		double TopBand{0.0};
		double BottomBand{0.0};
		/** The outermost ring drawn (3 the outer ring when it has members). */
		int32 OuterRing{2};
	};

	enum class ELabel : uint8
	{
		Plate,
		Name
	};

	struct FLabel
	{
		ELabel Kind{ELabel::Name};
		int32 System{INDEX_NONE};
		FBox2D Rect{ForceInit};
		/** A plate's lines (F10's layout) and the row of its worlds' dots under them. */
		APSStrategicMap::FPlate Plate;
		double StripHeight{0.0};
		/** Set off from its star: a thin leader to it. */
		bool bLeader{false};
	};

	struct FShipPlace
	{
		int32 System{INDEX_NONE};
		FVector2D Position{FVector2D::ZeroVector};
		bool bValid{false};
	};

	FFrame MakeFrame(const FVector2D& Size) const;
	FVector2D ToScreen(const FFrame& Frame, const FVector2D& LayoutPosition) const;
	float DiscOf(const APSStarMap::FSystem& System) const;
	bool IsDrawn(const APSStarMap::FSystem& System) const;
	/** The star under a point, or INDEX_NONE. */
	int32 HitTest(const FVector2D& Local) const;
	/** Places plates and names for the frame (cached until what is drawn, the frame or the pick changes). */
	void PlaceLabels(const FFrame& Frame) const;
	void UpdateShipPlaces();

	TWeakObjectPtr<UWorld> World;
	EStyle Style{EStyle::Full};
	FOnSystem OnPicked;
	FOnSystem OnOpened;
	FAPSStarMapModel Model;
	FGuid Selected;
	FGuid Centre;
	APSStarMap::EFilter Filter{APSStarMap::EFilter::All};
	TArray<FPlannedRoute> PlannedRoutes;
	TArray<FShipPlace> PlannedPlaces;
	TArray<TWeakObjectPtr<ASpaceship>> HighlightedShips;
	TArray<FShipPlace> HighlightedPlaces;
	float RefreshClock{0.0f};

	double Zoom{1.0};
	FVector2D Pan{FVector2D::ZeroVector};
	bool bPressed{false};
	bool bDragged{false};
	FVector2D PressPosition{FVector2D::ZeroVector};
	FVector2D PressPan{FVector2D::ZeroVector};
	int32 Hovered{INDEX_NONE};

	/** The last paint: the frame and every system's place on screen (for picking). */
	mutable FFrame LastFrame;
	mutable TArray<FVector2D> Screen;
	/** The labels' cache and what it was made for. */
	mutable TArray<FLabel> Labels;
	mutable TArray<FBox2D> LabelObstacles;
	mutable TArray<FBox2D> SoftObstacles;
	mutable TArray<bool> Named;
	mutable uint32 LabelKey{0};
	/** Where each system's plate or name stood last, from its star, so a new layout tries that first (no jumping labels). */
	mutable TMap<FGuid, FVector2D> LastLabelOffsets;
};
