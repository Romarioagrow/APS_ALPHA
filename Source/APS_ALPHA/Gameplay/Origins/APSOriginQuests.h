#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"

class UWorld;

/**
 * Rio 10.10 / 13.10 ("происхождения должны давать контент: отдельная цепочка на отдельном экране, не строки в общем
 * журнале"; concept section 4, task cards T-10): the origin's own story as a chain of steps, one per origin (ARK, ADRIFT,
 * UNDER THE RING), each with its story line, its objective and what it opens. Progress is kept as tokens on the mission
 * board (ORIGIN_<Key>_<n>, saved with the civilization); the runtime checks the current step once a second against the
 * game's own systems (the terminal, the colony's modules, the ladder's tokens, the Ancients' chains, the fleet, the
 * pilot's place) and tells each step in the journal (category Origin) and on the HUD. The colony terminal's ORIGIN page
 * reads View(). Runs only when the world's rules ask for the ladder.
 */
namespace APSOriginQuests
{
	enum class ETrigger : uint8
	{
		None,
		/** The colony terminal (Tab) is open. */
		TerminalOpened,
		/** The pilot on foot, Number metres or more from the colony. */
		OnFootAway,
		/** A colony module of id Param stands. */
		ModuleBuilt,
		/** A ladder token Param is open on the mission board. */
		Token,
		/** An infrastructure structure of type Param stands. */
		StructureBuilt,
		/** An Ancients chain of key Param (Echoes, QuietHull, ...) is on the board: offered, active or done. */
		AncientsChain,
		/** The fleet has launched Number hulls or more. */
		ShipsLaunched,
		/** The pilot sits in a ship Number kilometres or more above the home world's surface. */
		InShipAboveKm,
		/** Number worlds of the home system or more are surveyed. */
		WorldsSurveyed,
		/** The pilot on foot on the home world. */
		OnHomeGround
	};

	struct FStep
	{
		const TCHAR* Id{TEXT("")};
		FText Title;
		/** Told when the step opens (the journal, the HUD). */
		FText Story;
		/** One line: what to do. */
		FText Objective;
		/** Told when the step completes. */
		FText Done;
		/** What it opens ("THE LAUNCH"); empty: nothing in particular. */
		FText Opens;
		ETrigger Trigger{ETrigger::None};
		FName Param;
		float Number{0.0f};
		/** The rung of the ladder the step belongs to (0..4). */
		uint8 Rung{0};
	};

	struct FChain
	{
		APSWorldRules::EOrigin Origin{APSWorldRules::EOrigin::None};
		const TCHAR* Key{TEXT("")};
		FText Name;
		TArray<FStep> Steps;
	};

	APS_ALPHA_API const FChain* ChainOf(APSWorldRules::EOrigin Origin);
	/** The token that records step Index done: ORIGIN_<Key>_<Index + 1>. */
	APS_ALPHA_API FName StepToken(const FChain& Chain, int32 Index);
	/** The five rungs' names (0..4). */
	APS_ALPHA_API FText RungName(int32 Rung);

	/** What the ORIGIN page shows: the chain and how far it got (from the board's tokens). */
	struct FView
	{
		const FChain* Chain{nullptr};
		int32 Done{0};
		bool IsValid() const { return Chain != nullptr; }
		bool IsComplete() const { return Chain && Done >= Chain->Steps.Num(); }
		const FStep* Current() const { return Chain && Chain->Steps.IsValidIndex(Done) ? &Chain->Steps[Done] : nullptr; }
	};
	APS_ALPHA_API FView View(const UWorld* World);

	/** Once a frame from the fleet command subsystem: the world's origin story, when the ladder is on. */
	APS_ALPHA_API void Tick(UWorld* World, float DeltaSeconds);
	/** Test: completes the current step by hand (aps.Origin.Advance). */
	APS_ALPHA_API bool Advance(UWorld* World);
}
