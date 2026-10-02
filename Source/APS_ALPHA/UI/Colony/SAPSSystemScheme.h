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

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(640.0, 420.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const override;

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
	double Zoom{1.0};
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
};
