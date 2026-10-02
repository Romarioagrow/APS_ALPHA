#pragma once

#include "CoreMinimal.h"

class APlanetaryBody;

/**
 * Rio 02.10 ("no planet surfaces, grey balls"): a body whose WorldScape surface is not on screen showed the authored
 * globe of its Blueprint, a StarterContent sphere with the dark prototype grid. That happens while another family is
 * resident, and while its own first build settles. The globe now wears a flat colour from the body's own surface
 * palette and liquid, so a distant world reads as itself until its relief takes over.
 */
namespace APSPlaceholderGlobe
{
	/**
	 * Tints the body's authored globe meshes once. Idempotent and cheap after the first call.
	 * Only gameplay bodies are tinted: menu previews are skipped. aps.Surface.PlaceholderTint 0 keeps the authored look.
	 */
	APS_ALPHA_API void Apply(APlanetaryBody* Body);

	/** The flat colour a body's globe wears: the palette's land tones, blended with its liquid by the ocean share. */
	APS_ALPHA_API FLinearColor ColorOf(const APlanetaryBody* Body);
}
