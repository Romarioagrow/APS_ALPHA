#pragma once

#include "CoreMinimal.h"
#include "SAPSCivilizationOverview.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Widgets/SCompoundWidget.h"

class AActor;
class ASpaceship;
class SVerticalBox;
class UWorld;

/**
 * PILOT tab of the colony terminal (Rio 04.10: "like the overview dashboard: the current character's status, all the main
 * parameters, and something interesting: what goes on around, nearby"). It replaces the former label/value rows with:
 * - who the pilot is and where, in one line;
 * - four headline numbers: altitude over the nearest world, speed with the flight mode, gravity in g and its source, and
 *   the sky (day, twilight or night from the star's height over the horizon; in space the star's distance);
 * - three cards, each with a link deeper: where you are (the world and its star drawn, coordinates as the quests give
 *   them), around you (the nearest ships, stations, colony, worlds and found ancient sites with distances), and right
 *   now (the course, the objective and the tracked mission, the latest events).
 * Live values are read four times a second, and only while the tab is shown.
 */
class APS_ALPHA_API SAPSPilotDashboard final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSPilotDashboard) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		/** The ship a course goes to: the piloted one, else the home ship (SAPSColonyTerminal::GetCourseShip). */
		SLATE_ARGUMENT(TFunction<ASpaceship*()>, CourseShip)
		/** The colony's actors by role: 0 the surface base, 1 the landing pad, 2 the home ship (may be null). */
		SLATE_ARGUMENT(TFunction<AActor*(int32)>, ColonyActor)
		/** Opens a tab of the terminal by its index (SAPSColonyTerminal::ShowTab). */
		SLATE_EVENT(FAPSOverviewOpenTab, OnOpenTab)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	struct FNearby
	{
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::Planet};
		FLinearColor Colour{FLinearColor::White};
		FText Name;
		FText Detail;
		double DistanceCm{0.0};
	};

	struct FJournalLine
	{
		FLinearColor Colour{FLinearColor::White};
		FText Category;
		FText Text;
	};

	struct FState
	{
		bool bPawn{false};
		FText Title;
		FText Environment;
		FText Mode;
		double SpeedCm{0.0};
		/** cm/s2; zero when weightless or in free flight. */
		double GravityCm{0.0};
		FText GravitySource;

		bool bBody{false};
		FText BodyName;
		FText BodyDetail;
		FLinearColor BodyColour{FLinearColor::White};
		int32 Moons{0};
		double AltitudeCm{0.0};
		FText StarName;
		FText StarDetail;
		FLinearColor StarColour{FLinearColor::White};
		/** Near the world (within its radius of the ground): the coordinates and the star's height over the horizon. */
		bool bNear{false};
		FText Coordinates;
		double SunElevation{0.0};
		double StarDistanceCm{0.0};

		FText Course;
		bool bCourse{false};
		FText CourseDetail;

		FText ObjectiveTitle;
		FText ObjectiveBody;
		bool bMission{false};
		FText MissionLabel;
		FText MissionTitle;
		FText MissionStep;
		FText MissionSubject;
		FText MissionCount;
		FLinearColor MissionColour{FLinearColor::White};

		TArray<FNearby> Nearby;
		TArray<FJournalLine> Journal;
	};

	void ReadState();
	void RebuildNearby();
	void RebuildJournal();
	TSharedRef<SWidget> BuildHero();
	TSharedRef<SWidget> BuildHeadlines();
	TSharedRef<SWidget> BuildWhere();
	TSharedRef<SWidget> BuildAround();
	TSharedRef<SWidget> BuildNow();
	FReply OpenTab(int32 Tab);

	TWeakObjectPtr<UWorld> World;
	TFunction<ASpaceship*()> CourseShip;
	TFunction<AActor*(int32)> ColonyActor;
	FAPSOverviewOpenTab OnOpenTab;
	FState State;
	float ReadAccumulator{0.0f};
	TSharedPtr<SVerticalBox> NearbyBox;
	FString NearbySignature;
	TSharedPtr<SVerticalBox> JournalBox;
	FString JournalSignature;
};
