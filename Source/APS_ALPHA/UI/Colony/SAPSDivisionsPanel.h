#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UCivilization;
class UWorld;

/**
 * DIVISIONS tab of the colony terminal (Rio 02.10: "still very unreadable; turn the text into visual parameters, so the
 * eye has something to catch on"). The civilization's six divisions as cards in two columns, each in its own colour:
 * a ring gauge with the level (founding levels amber, levels earned by work green, room to the cap dim), what the level
 * changes in the game as stat chips (icon, big number, short label), the growth toward the next earned level as a
 * segmented bar, and the division's ships as hull icons, filled while under orders. The former text lines stay as the
 * tooltips of their parts.
 *
 * Read live from the civilization (founding levels) and fleet command (work tally, earned levels, ships, sector HQs)
 * with the fleet's own rules (APSFleet::WorkSeconds): once a frame, and only while the tab is shown.
 */
class APS_ALPHA_API SAPSDivisionsPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSDivisionsPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Exploration, industry, science, civil affairs, military, fleet command. */
	static constexpr int32 CardCount = 6;

	/** One card's live values. */
	struct FCardState
	{
		/** The civilization's level from the founding, and the levels the work earned on top (fleet command). */
		int32 Founding{0};
		int32 Earned{0};
		/** Work counted toward the earned levels (science: worlds studied plus anomaly points). */
		int32 Work{0};
		/** Science: worlds studied, and anomaly points (an expedition 1, the pilot on foot 2). */
		int32 Studied{0};
		int32 Anomalies{0};
		/** The division's ships (fleet command: every ship) and those under orders. */
		int32 Ships{0};
		int32 UnderOrders{0};
		/** Fleet command: sector HQs the fleet built, each speeding every ship up. */
		int32 Headquarters{0};

		int32 Level() const { return Founding + Earned; }
	};

	TSharedRef<SWidget> BuildCard(int32 Card);
	/** The ring gauge with the level number inside. */
	TSharedRef<SWidget> BuildLevel(int32 Card, const FLinearColor& Accent);
	/** Founding, earned and still to earn, in the ring's colours. */
	TSharedRef<SWidget> BuildLegend(int32 Card);
	/** What the level changes in the game, one chip per number. */
	TSharedRef<SWidget> BuildStats(int32 Card, const FLinearColor& Accent);
	TSharedRef<SWidget> BuildGrowth(int32 Card);
	TSharedRef<SWidget> BuildShips(int32 Card, const FLinearColor& Accent);

	/** The card's values; all six are read again at most once a frame. */
	const FCardState& State(int32 Card) const;
	void ReadStates() const;
	const UCivilization* GetCivilization() const;
	/** Work done toward the next earned level, of the work one level takes (all of it once every level is earned). */
	int32 GrowthDone(int32 Card) const;
	/** The former text lines, kept as tooltips: the level, the effect in the game, the growth rule, the ships. */
	FText LevelText(int32 Card) const;
	FText EffectText(int32 Card) const;
	FText GrowthText(int32 Card) const;
	FText GrowthDetail(int32 Card) const;
	FText ShipsText(int32 Card) const;

	/** Rio 02.10: each division's missions under its card, rebuilt when the board changes. */
	void RebuildMissions();
	TSharedRef<SWidget> BuildMission(const struct FAPSMission& Mission) const;

	TWeakObjectPtr<UWorld> World;
	mutable FCardState States[CardCount];
	mutable uint64 StatesFrame{MAX_uint64};
	TSharedPtr<class SVerticalBox> MissionBoxes[CardCount];
	uint32 MissionRevision{0};
};
