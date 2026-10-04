#pragma once

#include "CoreMinimal.h"

/**
 * Ancient structures (Rio 02.10: "a separate concept: structures left by an unknown race, monumental ones seen from orbit
 * and smaller ones; quest chains tied to them; spawn them with some chance, but guarantee some in the start system and
 * something interesting in the nearest systems"). The Builders' sites are data until their world stands: a kind, a size,
 * a seed and a body, all derived from the world seed and stable keys (body catalogue names, star system ids). Nothing of
 * a site is saved: every session derives the same sites again, and the quest progress lives in the department mission
 * board's own save (APSAncientsQuests). Design: Docs/Design/ANCIENT_STRUCTURES.md.
 */
namespace APSAncients
{
	enum class EKind : uint8
	{
		/** A leaning black obelisk kilometres tall with a ring of standing stones (monumental). */
		Monolith,
		/** A ring kilometres across standing on its edge, half sunk, an arc of it fallen (monumental). */
		BrokenRing,
		/** A grid of stepped towers around one tall spire (monumental or large). */
		SpireField,
		/** The corner of a buried cube breaking the surface (large). */
		SunkenCube,
		/** A ring of standing stones around a plinth, walkable (small). */
		StoneCircle,
		/** A broken hull kilometres long in a world's high orbit. */
		Derelict,
		Count
	};

	enum class ESize : uint8
	{
		Small,
		Large,
		Monumental
	};

	/** The quest chain a site carries (APSAncientsQuests). Appended only: mission templates carry the chain's key. */
	enum class EChain : uint8
	{
		/** The monument on the home planet: signal, survey, study, contact; the chart to the next system. */
		Echoes,
		/** The derelict in the home planet's high orbit: approach, scan, board. */
		QuietHull,
		/** The small circle on a home moon (or the home planet). */
		Circle,
		/** A chance site on another world of the start system. */
		LostWorks,
		/** A site in a nearby star system, one after another along the Builders' charts. */
		Road,
		Count
	};

	/** One site as data: everything here follows from the seeds, so a load derives it again. */
	struct FSiteSpec
	{
		/** Stable id: H_MONUMENT, H_CIRCLE, H_HULL, H_LOST_<body>, N_<system digits>. Actor names and mission templates use it. */
		FString Id;
		EKind Kind{EKind::Monolith};
		ESize Size{ESize::Monumental};
		EChain Chain{EChain::Echoes};
		/** Drives the shape, the candidate places and the turn; never rolled again. */
		uint32 Seed{0};
		/** 1 for the monumental shapes, about 0.4 for their large variants. */
		float Scale{1.0f};
		bool bHomeSystem{true};
		/** The body's fleet key (FAPSFleetCommand::KeyOf); in a nearby system filled once its worlds stand. */
		FString BodyKey;
		/** Nearby systems: the catalogue system, its index and its rank from home (0 is the nearest). */
		FGuid SystemId;
		int32 SystemIndex{INDEX_NONE};
		int32 Rank{INDEX_NONE};
		/** Nearby systems: which of the system's eligible worlds carries it (modulo their number). */
		uint32 Pick{0};

		bool IsOrbital() const { return Kind == EKind::Derelict; }
	};

	/** What a built site measures (cm), for the quests, the texts and the test teleport. */
	struct FMetrics
	{
		/** Top above the site's centre. */
		double HeightCm{0.0};
		/** Radius of what stands on the ground (or of the hull). */
		double FootprintCm{0.0};
		/** The navigation point stands this far above the centre, so fleet slots around it stay clear of the ground. */
		double NavHeightCm{150000.0};
		/** On foot within this of the centre counts as standing at the site. */
		double OnFootCm{30000.0};
		/** The size the texts quote: height, diameter, edge or length. */
		double SizeCm{0.0};
		/** Towers of a spire field, stones of a circle: for the texts. */
		int32 PartCount{0};
	};

	FText KindName(EKind Kind);
	/** One sentence on what stands there; {Size} and {Count} are filled from the metrics. */
	FText KindDescription(EKind Kind, const FMetrics& Metrics);
	/** What the contact finds: the last line of a chain. */
	FText KindStory(EKind Kind);
	FText SizeName(ESize Size);
	FText ChainName(EChain Chain);
	/** "MONOLITH", for logs. */
	const TCHAR* KindLabel(EKind Kind);
	/** "3.4 km" or "38 m". */
	FText LengthText(double Cm);
	/** CRC32 of the parts joined by '|': the one hash every roll uses (the same on every platform and session). */
	uint32 Hash(const FString& A, const FString& B = FString(), const FString& C = FString());
	/** "23.4 N, 41.2 E" for a body-local direction. */
	FText WhereText(const FVector& LocalUp);
}
