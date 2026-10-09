#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AActor;
class AStar;
class UWorld;

/**
 * System scheme of the civilization menu (Rio, 02.10: "2D schemes in order and to scale of size, without distances").
 * Each star of the current system has a lane: the star at the left, its planets in orbital order with even gaps, moons
 * in a column under their planet. Planets and moons share one kilometre scale, so a gas giant dwarfs a rocky world
 * (Rio 09.10); the stars share one of their own, whole in their lanes, and a star drawn smaller than the planets' scale
 * says by how much. Wheel zooms around the cursor (one view transform for everything), a drag pans, a click picks.
 */
class APS_ALPHA_API SAPSSystemScheme final : public SLeafWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnPicked, AActor*);

	SLATE_BEGIN_ARGS(SAPSSystemScheme) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_EVENT(FOnPicked, OnPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	/** Re-reads the current system (the star nearest the player); the terminal calls it while the scheme is shown. */
	void Refresh();
	AActor* GetPicked() const { return Picked.Get(); }
	void SetPicked(AActor* Actor) { Picked = Actor; }
	/** Rio 05.10 (star map): pins the scheme to this star's system (the star map's drill-down); null follows the player again. */
	void ShowSystem(AActor* Star);
	bool IsPinned() const { return PinnedStar.IsValid(); }
	/** Rio 09.10 (item 33, "BHOUNKNOWN"): a star's class as the scheme and its card say it ("G6V", "BLACK HOLE"). */
	static FText StarClassText(const AStar& Star);
	/** Wheel steps around a point of the scheme (local): that point stays where it is. The wheel and the test runs. */
	void ZoomAt(double WheelSteps, const FVector2D& Local);
	/** Test runs: aps.Test.Scheme [open | zoom <wheel steps> [x y] | log] (x, y: fractions of the scheme's size). */
	static void RunTestCommand(const TArray<FString>& Args, UWorld* World);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(640.0, 420.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const override;
	/** Rio 06.10 (audit: a drag whose capture was taken away kept panning on the next hover): stops the drag. */
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	struct FBody
	{
		TWeakObjectPtr<AActor> Actor;
		FText Designation;
		FText Name;
		FText Detail;
		/** Moons: "A1.02  SIAX", beside the dot. */
		FText Label;
		FLinearColor Color{FLinearColor::White};
		double RadiusKm{0.0};
		bool bStar{false};
		bool bMoon{false};
		/** Index of the star lane (stars) or of the parent planet (moons) in Bodies. */
		int32 Lane{0};
		int32 Parent{INDEX_NONE};
	};

	/** Lays the scheme out for the current size (the default view, every star whole), then applies the zoom and the pan as
	 * one transform: disc centres and radii, lane tops and label rows in local space. */
	void Layout(const FVector2D& Size) const;
	int32 HitTest(const FVector2D& Local) const;

	TWeakObjectPtr<UWorld> World;
	FOnPicked OnPicked;
	TArray<FBody> Bodies;
	int32 Lanes{0};
	TWeakObjectPtr<AActor> Picked;
	/** Rio 09.10: the view's magnification of the laid-out scheme, 1 = the default view (the wheel's lower limit). */
	mutable double Zoom{1.0};
	/** The view's offset in local pixels, applied after the zoom. */
	FVector2D Pan{FVector2D::ZeroVector};
	bool bDragging{false};
	bool bDragged{false};
	FVector2D DragStart{FVector2D::ZeroVector};
	FVector2D PanStart{FVector2D::ZeroVector};
	mutable FVector2D LaidOutSize{FVector2D::ZeroVector};
	mutable TArray<FVector2D> Centres;
	mutable TArray<float> Radii;
	/** Pixels per kilometre of the planets and moons on screen (zoom included), for the scale bar and the stars' note. */
	mutable double PixelsPerKm{0.0};
	/** Per lane: where the planets' labels start (one row under the largest disc, Rio 02.10: "even paddings"). */
	mutable TArray<double> LabelRows;
	/** Rio 04.10 evening ("zoomed in they lie on each other, zoomed out the star never shows whole"): per lane its top and
	 * height on screen. A lane is at least its share of the widget and grows with its largest disc, labels and moon
	 * column, so lanes never overlap; the scheme then also pans vertically. */
	mutable TArray<double> LaneTops;
	mutable TArray<double> LaneHeights;
	/** The size of everything laid out at zoom 1, for the pan limits, and the pan the last layout used (within them). */
	mutable FVector2D ContentSize{FVector2D::ZeroVector};
	mutable FVector2D LaidOutPan{FVector2D::ZeroVector};
	/** The wheel's upper limit: the largest planet's disc up to about 40% of the view's height. */
	mutable double MaxZoom{60.0};
	/** Pan clamped so the scheme never leaves the view: not right of its left edge, never all of it off to the left or up. */
	FVector2D ClampPan(const FVector2D& Wanted, const FVector2D& Size) const;
	/** Rio 05.10 (star map): the star whose system the scheme shows instead of the one nearest the player. */
	TWeakObjectPtr<AActor> PinnedStar;
};
