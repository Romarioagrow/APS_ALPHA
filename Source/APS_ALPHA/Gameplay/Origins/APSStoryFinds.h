#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
class UWorld;

/**
 * Rio 19.10 ("start filling the planets with objects along the story"; "more depth and detail in everything"): the finds
 * around the colony of an ORIGIN world, real places to walk or drive to, built from the project's own cooked meshes and
 * procedural stone (no new art, no particles):
 *  - THE LANDER: a small hull broken on the ground (the ark's lander; in ADRIFT the expedition before ours), its engines and
 *    landing leg thrown aside, crates strewn, a red lamp on a mast;
 *  - THE SUPPLY POD: cargo crates around a scorched ring, a blue lamp;
 *  - THE CRYSTALS: an outcrop of glowing crystal on dark rock;
 *  - THE VENTS: basalt cones with glowing throats and a warm light;
 *  - in ARK, three Builders' waymarks on the line from the ark to the monument, their glyphs lit.
 * Four finds stand 1.5-5.5 km round the colony's site on dry, gentle ground, the same places for the same world (seed and
 * site); the radar marks them once the comms mast stands (ARK) or the ship is down (ADRIFT). F at a find takes what it
 * holds (energy, metals once they are open) and the journal tells what it was. Lamps unshadowed with a fading draw
 * distance, small parts without shadows, everything attached to the body; nothing outside ARK and ADRIFT (aps.Finds.Enable).
 */
namespace APSStoryFinds
{
	/** The finds run in this world (ARK or ADRIFT founded with its kit, aps.Finds.Enable). */
	APS_ALPHA_API bool IsActive(const UWorld* World);
	/** Once a frame (the fleet command subsystem): placing and building the finds, the waymarks, the markers. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);

	/** The walker's F: a find within reach that still holds something (null otherwise). */
	APS_ALPHA_API AActor* Candidate(const APawn* Pawn);
	APS_ALPHA_API FText ActionFor(const APawn* Pawn, const AActor* Candidate);
	APS_ALPHA_API FText SubjectFor(const AActor* Candidate);
	APS_ALPHA_API bool TryInteract(APawn* Pawn, AActor* Candidate);

	/** "FINDS 1 OF 4" under the story's step once the radar has marked them; empty otherwise. */
	APS_ALPHA_API FText FindsLine(const UWorld* World);
}
