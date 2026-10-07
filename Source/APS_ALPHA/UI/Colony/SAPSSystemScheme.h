#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AActor;
class UWorld;

/**
 * System scheme of the civilization menu (Rio, 02.10: "2D schemes in order and to scale of size, without distances;
 * a hypergiant shows its edge"). Each star of the current system has a lane: the star at the left, its planets in
 * orbital order with even gaps, moons in a column under their planet. Discs share one kilometre scale, so a gas giant
 * dwarfs a rocky world and a hypergiant shows only its limb. Wheel zooms, a drag pans, a click picks a body.
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

	/** Lays the scheme out for the current size and zoom: disc centres and radii in local space. */
	void Layout(const FVector2D& Size) const;
	int32 HitTest(const FVector2D& Local) const;

	TWeakObjectPtr<UWorld> World;
	FOnPicked OnPicked;
	TArray<FBody> Bodies;
	int32 Lanes{0};
	TWeakObjectPtr<AActor> Picked;
	/** Rio 06.10: set by Layout to the zoom that shows every star whole (the default view), then by the wheel. */
	mutable double Zoom{1.0};
	/** A new system opens at the fitting zoom (Layout resolves it once it knows the view's size). */
	mutable bool bFitPending{true};
	/** Rio 06.10 (audit: the fit was judged with the planets at their smallest dots): the fit's passes left; the second one
	 * re-judges it at the candidate zoom with the lanes' real largest discs. Set to 2 with bFitPending. */
	mutable int32 FitPasses{2};
	FVector2D Pan{FVector2D::ZeroVector};
	bool bDragging{false};
	bool bDragged{false};
	FVector2D DragStart{FVector2D::ZeroVector};
	FVector2D PanStart{FVector2D::ZeroVector};
	mutable FVector2D LaidOutSize{FVector2D::ZeroVector};
	mutable TArray<FVector2D> Centres;
	mutable TArray<float> Radii;
	/** Pixels per kilometre of the last layout, for the scale bar. */
	mutable double PixelsPerKm{0.0};
	/** Per lane: where the planets' labels start (one row under the largest disc, Rio 02.10: "even paddings"). */
	mutable TArray<double> LabelRows;
	/** Rio 04.10 evening ("zoomed in they lie on each other, zoomed out the star never shows whole"): per lane its top and
	 * height in the last layout. A lane is at least its share of the widget and grows with its largest disc, labels and
	 * moon column, so zooming in never stacks one lane onto the next; the scheme then also pans vertically. */
	mutable TArray<double> LaneTops;
	mutable TArray<double> LaneHeights;
	/** The size of everything laid out, for the pan limits, and the pan the last layout used (within those limits). */
	mutable FVector2D ContentSize{FVector2D::ZeroVector};
	mutable FVector2D LaidOutPan{FVector2D::ZeroVector};
	/** The wheel's limits: zoomed out, the system's largest star fits its lane whole and to scale; zoomed in, the largest
	 * planet still fits the view. */
	mutable double MinZoom{0.25};
	mutable double MaxZoom{60.0};
	/** Pan clamped so the scheme never leaves the view: not right of its left edge, never all of it off to the left or up. */
	FVector2D ClampPan(const FVector2D& Wanted, const FVector2D& Size) const;
	/** Rio 05.10 (star map): the star whose system the scheme shows instead of the one nearest the player. */
	TWeakObjectPtr<AActor> PinnedStar;
};
