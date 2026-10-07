#pragma once

#include "CoreMinimal.h"

class AActor;
class AStar;
class UWorld;

/**
 * B7 (Rio 01.10: "tidy the stars' material for distance: when a star is far, it should look beautiful too"). A catalogue
 * star far away is drawn as its glyph (a compact core with rays, at least a couple of pixels, APSStellarOpticalSupport);
 * a materialized star (the home sun seen from its planets, the star of a visited system) is a real sphere whose catalogue
 * point is suppressed, so from a few AU it shrank below a pixel and vanished: the home sun was not to be seen from its
 * own planet's orbit (02.10 test shots). Such a star keeps its catalogue glyph while its disc is smaller than the glyph:
 * the same mesh, material (day fade included) and per-star data as its catalogue point, sized by the same optics.
 */
namespace APSFarStarGlyphs
{
	/** Each frame from the gameplay stellar view, with the generator's attached actors (its galaxy and cluster). */
	void Update(UWorld* World, const TArray<AActor*>& Attached, const FVector& Camera, double PixelTangent, bool bDaylightHidden);
	/**
	 * Rio 06.10 (aps.Stars.SystemGlare): set by the stellar view each frame before Update. A listed star's glyph draws at
	 * DayVisibility x its value, any other at DayVisibility x Others (values already carry the day blend), through the glyph's
	 * own material (GameplayPointVisibility, one value with its crossfade share). bEnabled false: the shared catalogue
	 * material, exactly as before (unless aps.Stars.FarGlyphLinearFade fades it through its own).
	 */
	void SetSystemGlare(const UWorld* World, bool bEnabled, float DayVisibility, float Others,
		TConstArrayView<TPair<const AStar*, float>> OwnValues);
	/** Removes a world's glyphs (the stellar view resets). */
	void Reset(const UWorld* World);
}
