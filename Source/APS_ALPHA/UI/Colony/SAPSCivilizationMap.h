#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AActor;
class UWorld;

/**
 * Top-down map of the current star system for the civilization menu (Rio, 30.09: "one menu, make the map"): planets
 * on their orbits, moons, stations, the colony, the civilization's ships, the pilot and the course target. Radii are
 * logarithmic, so a station beside its planet and the outermost planet share one view. The system view shows the star
 * and planets with a badge where the civilization is present; a double click on a planet opens its local view with
 * moons, stations, ships and the colony.
 */
class APS_ALPHA_API SAPSCivilizationMap final : public SLeafWidget
{
public:
	enum class EKind : uint8
	{
		Star,
		Planet,
		Moon,
		Station,
		Colony,
		Settlement,
		Ship,
		Pilot,
		/** An autonomous outpost in orbit (built by a construction ship, or founded with the civilization). */
		Outpost
	};

	struct FObject
	{
		TWeakObjectPtr<AActor> Actor;
		EKind Kind{EKind::Planet};
		/** The actor's path, the id the ship navigation uses for a course. */
		FString StableId;
		FText Name;
		FText Detail;
		FLinearColor Color{FLinearColor::White};
		FVector Location{FVector::ZeroVector};
		double RadiusCm{0.0};
		/** The planet a moon, station, colony or ship belongs to (its local view); unset in deep space. */
		TWeakObjectPtr<AActor> Anchor;
		bool bOwn{false};
	};

	SLATE_BEGIN_ARGS(SAPSCivilizationMap) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_EVENT(FSimpleDelegate, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Re-reads the world: the terminal calls it twice a second while the map is shown. */
	void Refresh();
	const FObject* GetSelected() const;
	/** The objects drawn in the current view, in map order (the side list of the menu). */
	void GetShownObjects(TArray<const FObject*>& OutObjects) const;
	void SelectById(const FString& StableId);
	/** Opens the local view of this planet; nullptr returns to the system view. */
	void Focus(AActor* Planet);
	/**
	 * Rio 09.10 (playtest 0.6.4): the system map opens where the player is: the local view of the planet he stands on or
	 * orbits (a moon's planet for a moon, whose neighbourhood that view holds), else the system. Only the first call per
	 * map decides (the terminal makes a new map each time it opens), so a view picked in the same session is kept.
	 */
	void OpenAtPilot();
	bool IsLocalView() const { return FocusPlanet.IsValid(); }
	FText GetViewTitle() const;
	void SetCourseTargetId(const FString& StableId) { CourseTargetId = StableId; }
	/** Ships ringed as selected for orders (fleet command). */
	void SetHighlightedShips(const TArray<TWeakObjectPtr<AActor>>& Ships) { HighlightedShips = Ships; }

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(640.0, 520.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	/** Rio 06.10 (audit: Alt-Tab during a drag left the press armed): a lost capture ends the press and the pan. */
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	/** Whether the object is drawn (and can be picked) in the current view. */
	bool IsShown(const FObject& Object) const;
	/** Map position of a world location in the widget's local space, logarithmic in distance from the view centre. */
	FVector2D Project(const FVector& WorldLocation, const FVector2D& Centre, double PixelRadius) const;
	double ProjectRadius(double DistanceCm, double PixelRadius) const;
	int32 HitTest(const FVector2D& LocalPosition) const;
	/**
	 * Rio 08.10 (playtest item 34): the wheel past a zoom limit changes the level, as the buttons do: out of a planet's
	 * view back to the system, into the picked (else the hovered) planet's view. Returns whether the view changed.
	 */
	bool SwitchLevelByWheel(bool bInward, const FVector2D& CursorAt, const FVector2D& Size);

	TWeakObjectPtr<UWorld> World;
	FSimpleDelegate OnSelectionChanged;
	TArray<FObject> Objects;
	TWeakObjectPtr<AActor> Star;
	TWeakObjectPtr<AActor> FocusPlanet;
	FVector PlaneU{FVector::ForwardVector};
	FVector PlaneV{FVector::RightVector};
	/** The view's outer radius and the knee of its logarithmic scale (cm). */
	double RangeCm{1.0};
	double KneeCm{1.0};
	FString SelectedId;
	FString CourseTargetId;
	TArray<TWeakObjectPtr<AActor>> HighlightedShips;
	int32 HoverIndex{INDEX_NONE};
	/** Where each object was painted last (local space), for picking; far off-screen when it was not drawn. */
	mutable TArray<FVector2D> PaintedPositions;
	/** Radius of the focused planet's disc in the local view, for picking. */
	mutable double PaintedDiscRadius{0.0};
	/** Wheel zoom (1 = the whole view) and the view centre's offset from the widget centre, pixels (Rio 02.10). */
	double MapZoom{1.0};
	FVector2D MapOffset{FVector2D::ZeroVector};
	/** A press that moves past a few pixels pans the map; one that does not is a click (selection). */
	bool bPressed{false};
	bool bPanning{false};
	FVector2D PressPosition{FVector2D::ZeroVector};
	FVector2D PressOffset{FVector2D::ZeroVector};
	/** OpenAtPilot ran for this map. */
	bool bOpenedAtPilot{false};
	/** Wheel turned past a zoom limit (notches, one way), and when the wheel last changed the level (hysteresis). */
	double LimitPush{0.0};
	bool bLimitPushInward{false};
	double LastLevelSwitchSeconds{-1.0e9};
};
