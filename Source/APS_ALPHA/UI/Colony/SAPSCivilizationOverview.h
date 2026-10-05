#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWorld;

DECLARE_DELEGATE_OneParam(FAPSOverviewOpenTab, int32);

/**
 * OVERVIEW tab of the colony terminal (Rio 04.10: "unreadable, ugly: improve all of it"). It replaces sixteen equal
 * label tiles with:
 * - the civilization's name and identity in one line;
 * - four headline numbers with icons and gauges: population, credits, tech level out of ten, and the fleet by division;
 * - three cards, each with a button to the tab that goes deeper: the home system drawn as its star with the planet and
 *   moons, the exploration of the system's worlds with the fleet at work, and the infrastructure as counts with pips.
 * Live values are read twice a second, and only while the tab is shown.
 */
class APS_ALPHA_API SAPSCivilizationOverview final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSCivilizationOverview) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		/** Opens a tab of the terminal by its index (SAPSColonyTerminal::ShowTab). */
		SLATE_EVENT(FAPSOverviewOpenTab, OnOpenTab)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** The fleet divisions of APSFleet::EDivision: main fleet, exploration, science, construction. */
	static constexpr int32 DivisionCount = 4;

	struct FLiveState
	{
		bool bCivilization{false};
		int32 Population{0};
		int64 Credits{0};
		int32 TechLevel{0};
		/** Orbital stations, ground settlements, planet outposts, star outposts. */
		int32 Infrastructure[4]{};
		/** Ships by fleet division, and those of them under orders. */
		int32 Ships[DivisionCount]{};
		int32 Busy[DivisionCount]{};
		/** The worlds in the system, and those surveyed or studied (studied is not counted as surveyed). */
		int32 Worlds{0};
		int32 Surveyed{0};
		int32 Studied{0};

		int32 ShipCount() const;
		int32 InfrastructureCount() const;
	};

	void ReadState();
	TSharedRef<SWidget> BuildHero();
	TSharedRef<SWidget> BuildHeadlines();
	TSharedRef<SWidget> BuildHome();
	TSharedRef<SWidget> BuildExploration();
	TSharedRef<SWidget> BuildInfrastructure();
	FReply OpenTab(int32 Tab);

	TWeakObjectPtr<UWorld> World;
	FAPSOverviewOpenTab OnOpenTab;
	FLiveState State;
	float ReadAccumulator{0.0f};
};
