#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FAPSConstructionMode;
class SBorder;
class SWrapBox;

/**
 * Build mode's palette (Rio 02.10: "a separate menu where we pick what to spawn"): a chamfered bar docked at the bottom
 * of the screen in the menu's style (APSMenuChrome). Two sections: PROPS (APSConstruction's catalogue, free) and
 * INFRASTRUCTURE (the infrastructure catalogue's types for this place, with their cost, greyed with the runtime's own
 * refusal). Cards are clicked or picked with 1-9 / 0; the status line shows what the ghost is doing. Everything is read
 * from FAPSConstructionMode, which owns the palette; the bar takes the clicks over it, the rest of the screen passes
 * them to the game.
 */
class SAPSBuildPalette final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSBuildPalette) {}
		SLATE_ARGUMENT(TWeakPtr<FAPSConstructionMode>, Mode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	/** The cursor is over the bar (the ghost hides and a click there is the palette's). */
	bool IsPointerOverBar() const;

private:
	void RebuildCards();

	TWeakPtr<FAPSConstructionMode> Mode;
	TSharedPtr<SBorder> Bar;
	TSharedPtr<SWrapBox> Cards;
	uint32 BuiltRevision{0};
	int32 BuiltSection{-1};
};
