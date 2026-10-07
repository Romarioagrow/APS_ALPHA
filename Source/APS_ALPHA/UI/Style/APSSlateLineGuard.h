#pragma once

#include "CoreMinimal.h"

/**
 * Rio 06.10 (offscreen q10 crash, "Assertion failed: RenderBatch.NumIndices > 0", SlateRHIRenderingPolicy.cpp:913): UE 5.4
 * Slate's antialiased line builder (ElementBatcher.cpp, FLineBuilder::MakeStartCap/MakeEndCap) builds no geometry for a
 * segment of length <= SMALL_NUMBER or a non-finite one. A FSlateDrawElement::MakeLines element that builds nothing and
 * lands alone in its render batch (its own layer, thickness, clip or draw effects) reaches the renderer with no indices and
 * asserts, in the editor and in a packaged game. Such a polyline is invisible anyway: it is not drawn.
 *
 * The rule of APSShipPerf::IsDrawableHudLine (Spaceship.cpp), checked on the float points Slate receives: every point
 * finite after the cast to float, and at least one segment longer than 0.01 px. Fewer than two points is not drawable
 * either (MakeLines drops those itself).
 */
namespace APSSlateLineGuard
{
	inline bool IsDrawable(const TArray<FVector2f>& Points)
	{
		bool bHasLength = false;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2f& Point = Points[Index];
			if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y))
			{
				return false;
			}
			bHasLength = bHasLength || (Index > 0 && (Point - Points[Index - 1]).SizeSquared() > 1.0e-4f);
		}
		return bHasLength;
	}

	/** The same for double points: Slate casts them to float, where a finite double past FLT_MAX turns infinite and
	 * points millions of pixels out collapse onto one value. */
	inline bool IsDrawable(const TArray<FVector2D>& Points)
	{
		bool bHasLength = false;
		FVector2f Previous = FVector2f::ZeroVector;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2f Point(Points[Index]);
			if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y))
			{
				return false;
			}
			bHasLength = bHasLength || (Index > 0 && (Point - Previous).SizeSquared() > 1.0e-4f);
			Previous = Point;
		}
		return bHasLength;
	}
}
