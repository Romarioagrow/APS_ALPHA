#pragma once

#include "CoreMinimal.h"

/**
 * Names of generated galaxies, clusters, stars, planets and moons. Rio, 02.10: human, varied names without the
 * PLANET/MOON suffix; moons sound like their planet. Every name follows from the world seed and the body's generation
 * address ("SYS0/S1/P2/M0"), so a world keeps its names on every load.
 */
namespace APSBodyNames
{
	/** The original "Lonesobo Planet" names. Worlds saved before 02.10 keep them (UGeneratedWorld::NameStyle). */
	constexpr int32 LegacyStyle = 0;
	/** Seven syllable families (Latin, Greek, Nordic, Slavic, Japanese-like, stellar, Arabic-like), no suffix. */
	constexpr int32 CurrentStyle = 1;

	enum class EKind : uint8
	{
		Galaxy,
		Cluster,
		Star,
		Planet,
		Moon
	};

	APS_ALPHA_API FString Generate(int32 WorldSeed, const FString& Address, EKind Kind);
	/** The original generator: an alternating consonant/vowel word plus " " + LegacyKind. */
	APS_ALPHA_API FString Legacy(int32 WorldSeed, const FString& Address, const FString& LegacyKind);
	/** For generation paths without a canonical address; not reproducible. */
	APS_ALPHA_API FString Random(EKind Kind);
	/** "Planet" and "Moon" name those kinds; any other legacy suffix is a star's spectral class. */
	APS_ALPHA_API EKind KindFromLegacy(const FString& LegacyKind);
	/** Legacy for LegacyStyle, Generate for any other style. */
	APS_ALPHA_API FName ForStyle(int32 Style, int32 WorldSeed, const FString& Address, const FString& LegacyKind);
}
