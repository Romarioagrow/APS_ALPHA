#pragma once

#include "CoreMinimal.h"
#include "APSStrategicMapTypes.h"
#include "InputCoreTypes.h"
#include "Widgets/SLeafWidget.h"

class APlayerController;
class FAPSStrategicMapCamera;
class FAPSStrategicMapScene;

DECLARE_DELEGATE_OneParam(FOnAPSStrategicMapPick, const APSStrategicMap::FSelection&);

/**
 * The strategic map's view over the real 3D world (Rio 02.10: "like the 2D labels of the generation menu, but here in
 * the full 3D game world"): every object projected through the live camera and marked by its limb, plates in the
 * generation menu's style placed without overlap (APSPreviewAnnotationLayout), orbits, fleet routes and the pilot's
 * course, catalogue star systems with the relay network and the selected system's sphere. Markers that land on one
 * another merge into a "+N" so dense places stay readable; the selection is always labelled, and an arrow at the edge
 * points to it when it is off the view. Everything is projected once per paint from one camera snapshot.
 *
 * It is also the map's input surface: RMB (or LMB) drag orbits, MMB or Shift+RMB pans, the wheel zooms, a click selects
 * the nearest object within 12 px, a double click flies to it.
 */
class SAPSStrategicMapView final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSStrategicMapView) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, Controller)
		SLATE_ARGUMENT(TSharedPtr<FAPSStrategicMapScene>, Scene)
		SLATE_ARGUMENT(TSharedPtr<FAPSStrategicMapCamera>, Camera)
		SLATE_EVENT(FOnAPSStrategicMapPick, OnSelect)
		SLATE_EVENT(FOnAPSStrategicMapPick, OnFocus)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(320.0, 240.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	/** Objects and labels painted last frame, for the panel's status line. */
	int32 GetPaintedCount() const { return Painted.Num(); }
	int32 GetLabelledCount() const { return LabelledCount; }

private:
	/** What was painted where (panel space), for picking. */
	struct FPainted
	{
		APSStrategicMap::FSelection Target;
		FVector2D Position{FVector2D::ZeroVector};
		/** A body's projected disc; small markers pick within the 12 px reach. */
		float Radius{0.0f};
	};

	enum class EDrag : uint8
	{
		None,
		Pending,
		Orbit,
		Pan
	};

	bool Pick(const FVector2D& LocalPosition, APSStrategicMap::FSelection& OutTarget) const;
	/** Away from every mark: the galaxy star the click points at, made a star system (Rio 04.10, far courses). */
	bool PickGalaxyStar(const FGeometry& Geometry, const FVector2D& LocalPosition, APSStrategicMap::FSelection& OutTarget) const;

	TWeakObjectPtr<APlayerController> Controller;
	TSharedPtr<FAPSStrategicMapScene> Scene;
	TSharedPtr<FAPSStrategicMapCamera> Camera;
	FOnAPSStrategicMapPick OnSelect;
	FOnAPSStrategicMapPick OnFocus;
	mutable TArray<FPainted> Painted;
	mutable int32 LabelledCount{0};
	mutable double ViewportWidthPixels{1920.0};
	EDrag Drag{EDrag::None};
	FKey DragButton;
	FVector2D PressPosition{FVector2D::ZeroVector};
};
