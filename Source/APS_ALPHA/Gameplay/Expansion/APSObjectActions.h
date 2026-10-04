#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * What can be done with an object (Rio 02.10: "for every object its own options of what can be done with it; it must
 * be extensible"): a planet, moon, star system, station, outpost, shipyard, HQ, ship, colony or anomaly site. Providers
 * add actions for the objects they understand (navigation, fleet orders, construction, surveys, the surface map, the
 * strategic map); every screen (the F10 map, the terminal's object page, the HUD) shows the same list.
 */
struct APS_ALPHA_API FAPSObjectAction
{
	/** Stable id, e.g. "Nav.SetCourse", "Fleet.Survey", "Build.MiningOutpost". */
	FName Id;
	FText Label;
	/** What it does, or why it cannot be done now. */
	FText Detail;
	/** Grouping on screens: NAVIGATION, FLEET, CONSTRUCTION, SCIENCE, ... */
	FText Group;
	FLinearColor Colour{FLinearColor::White};
	bool bEnabled{true};
	/** Runs the action; the screens refresh from the runtime revisions afterwards. Returns a message for the screen. */
	TFunction<FText()> Execute;
	/**
	 * Rio 04.10 ("a button whose order is under way must show it, with a fill"): the order of this kind under way at the
	 * object, read by the screens every frame. False when there is none; else the part done, 0..1 (the flight there, then
	 * the work), what is happening ("EN ROUTE", "BUILDING") and who does it ("M-10 (CONSTRUCTION)").
	 */
	TFunction<bool(float& OutProgress, FText& OutStatus, FText& OutWho)> Underway;
};

namespace APSObjectActions
{
	/** Adds actions for an object (ignore objects the provider does not understand). */
	using FProvider = TFunction<void(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions)>;

	/** Registers (or replaces) a provider by name; the built-in ones register on first use. */
	APS_ALPHA_API void RegisterProvider(FName Name, FProvider Provider);
	/** Every action for the object, in provider order. */
	APS_ALPHA_API void Gather(UWorld* World, AActor* Object, TArray<FAPSObjectAction>& OutActions);
	/** A one-line kind for the object ("PLANET", "STAR SYSTEM", "STATION", "SHIP S-04"...). */
	APS_ALPHA_API FText KindOf(const AActor* Object);
	/** The object's display name (catalogue names for bodies and systems). */
	APS_ALPHA_API FText NameOf(const AActor* Object);
	/** Status lines for the object page: what is known, what stands there, what is under way. */
	APS_ALPHA_API void Describe(UWorld* World, const AActor* Object, TArray<TPair<FText, FText>>& OutFields);
}
