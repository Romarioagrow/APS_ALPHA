#pragma once

#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SOverlay.h"

namespace APSChamfer
{
	inline float Cut(const FVector2D& Size)
	{
		return FMath::Min(12.0f, FMath::Min(Size.X, Size.Y) * 0.24f);
	}
}

/** Clips the entire card, including rectangular child brushes and hover art.
 * A rectangle intersected with this diamond is exactly the frame's octagon.
 * Both zones are hard barriers, so a nested scroll box cannot discard them. */
class SAPSChamferedOverlay final : public SOverlay
{
public:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
		const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		if (Size.X <= 0.0 || Size.Y <= 0.0) return LayerId;
		const float Cut = APSChamfer::Cut(Size);
		FSlateClippingZone Bounds(Geometry);
		Bounds.SetAlwaysClip(true);
		Elements.PushClip(Bounds);
		FSlateClippingZone Corners(
			Geometry.LocalToAbsolute(FVector2D(Size.X * 0.5, Cut - Size.X * 0.5)),
			Geometry.LocalToAbsolute(FVector2D(Size.X + Size.Y * 0.5 - Cut, Size.Y * 0.5)),
			Geometry.LocalToAbsolute(FVector2D(Cut - Size.Y * 0.5, Size.Y * 0.5)),
			Geometry.LocalToAbsolute(FVector2D(Size.X * 0.5, Size.Y + Size.X * 0.5 - Cut)));
		Corners.SetAlwaysClip(true);
		Elements.PushClip(Corners);
		const int32 LastLayer = SOverlay::OnPaint(Args, Geometry, CullingRect,
			Elements, LayerId, Style, bParentEnabled);
		Elements.PopClip();
		Elements.PopClip();
		return LastLayer;
	}
};
