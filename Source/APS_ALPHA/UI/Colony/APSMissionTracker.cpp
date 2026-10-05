#include "APSMissionTracker.h"

#include "APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSMissionTracker"

namespace APSMissionTrackerPrivate
{
	TAutoConsoleVariable<int32> CVarTracker(TEXT("aps.UI.MissionTracker"), 1,
		TEXT("1: the tracked department mission shows in the top-left corner of the game view. 0: hidden."));

	/**
	 * One card with the onboarding objective (Rio 03.10), so at the objective's layer: over the HUDs (40-60) and the
	 * ship's markers, under the colony terminal (890), the F10 map (900) and the menu (1000).
	 */
	constexpr int32 ZOrder = 880;
	/** Fixed, as both former panels were: auto-wrapped text gives a self-sizing card no stable width. */
	constexpr float CardWidth = 420.0f;

	struct FEntry
	{
		TSharedPtr<SWidget> Widget;
		TWeakObjectPtr<UGameViewportClient> Viewport;
		/** The colony terminal subsystem's objective (APSMissionTracker::SetObjective); unbound: none. */
		TFunction<bool(FText&, FText&)> Objective;
	};
	TMap<TWeakObjectPtr<UWorld>, FEntry> GEntries;

	EVisibility VisibleIf(const bool bVisible)
	{
		return bVisible ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
	}

	/** The F10 map holds the view with its own camera; the tracker stays out of it. */
	bool IsMapOpen(const TWeakObjectPtr<UWorld>& World)
	{
		const AGravityPlayerController* Controller = World.IsValid()
			? Cast<AGravityPlayerController>(World->GetFirstPlayerController()) : nullptr;
		return Controller && Controller->IsStrategicMapOpen();
	}

	/** The terminal's scrim and the gaps between its panels let the HUD show through: the card steps aside, as the
	 * objective always did. */
	bool IsTerminalOpen(const TWeakObjectPtr<UWorld>& World)
	{
		const UAPSColonyTerminalSubsystem* Terminal = World.IsValid()
			? World->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		return Terminal && Terminal->IsTerminalOpen();
	}

	const FAPSMission* Tracked(const TWeakObjectPtr<UWorld>& World)
	{
		const FAPSMissionBoard* Board = World.IsValid() ? APSMissionsFind(World.Get()) : nullptr;
		const FAPSMission* Mission = Board ? Board->GetTracked() : nullptr;
		return Mission && Mission->State == APSMissions::EState::Active ? Mission : nullptr;
	}

	bool IsMissionShown(const TWeakObjectPtr<UWorld>& World)
	{
		return CVarTracker.GetValueOnGameThread() != 0 && Tracked(World) && !IsMapOpen(World) && !IsTerminalOpen(World);
	}

	bool ReadObjective(const TWeakObjectPtr<UWorld>& World, FText& OutTitle, FText& OutBody)
	{
		const FEntry* Entry = GEntries.Find(World);
		return Entry && Entry->Objective && Entry->Objective(OutTitle, OutBody);
	}

	bool IsObjectiveShown(const TWeakObjectPtr<UWorld>& World)
	{
		FText IgnoredTitle;
		FText IgnoredBody;
		return ReadObjective(World, IgnoredTitle, IgnoredBody);
	}

	FLinearColor DepartmentColour(const TWeakObjectPtr<UWorld>& World)
	{
		const FAPSMission* Mission = Tracked(World);
		return Mission ? APSInfrastructure::DepartmentColour(Mission->Department) : APSChrome::Cyan();
	}

	/** Rio 03.10: the catalogue writes titles in capitals; the card shows them in sentence case, like the objective.
	 * Text already in mixed case is kept as written. */
	FText SentenceCase(const FText& Text)
	{
		const FString Source = Text.ToString();
		if (Source.IsEmpty() || !Source.Equals(Text.ToUpper().ToString(), ESearchCase::CaseSensitive))
		{
			return Text;
		}
		const FString Lower = Text.ToLower().ToString();
		return FText::AsCultureInvariant(FText::AsCultureInvariant(Lower.Left(1)).ToUpper().ToString() + Lower.Mid(1));
	}

	/** Stocks, earned levels and an unlocked structure type; the REWARD label is drawn beside it. */
	FText RewardText(const FAPSMission& Mission)
	{
		// No stocks is no line ("REWARD NOTHING" read as a broken mission); levels or an unlock still show.
		FString Text = Mission.Reward.IsEmpty() ? FString() : APSInfrastructure::DescribeAmounts(Mission.Reward).ToString();
		const auto Append = [&Text](const FString& Part)
		{
			Text += Text.IsEmpty() ? Part : TEXT("   /   ") + Part;
		};
		if (Mission.RewardLevels > 0)
		{
			Append(FString::Printf(TEXT("+%d LEVEL%s"), Mission.RewardLevels, Mission.RewardLevels > 1 ? TEXT("S") : TEXT("")));
		}
		if (!Mission.Unlocks.IsNone())
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Mission.Unlocks);
			Append(TEXT("UNLOCKS ") + (Type ? Type->Name.ToString() : Mission.Unlocks.ToString()).ToUpper());
		}
		return Text.IsEmpty() ? FText::GetEmpty() : FText::FromString(Text);
	}

	/** The progress bar: a thin cyan line on a dim track (the default style's black trough read as a hole). */
	const FSlateBrush* BarTrack()
	{
		static const FSlateColorBrush Brush(APSChrome::CyanDim());
		return &Brush;
	}

	const FSlateBrush* BarFill()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	/**
	 * Rio 04.10 ("the design is off, too much plain text, readability"): no round icon badges (their glyphs read as
	 * crosses) and fewer sentences. A section is a coloured bar down its left side, a small spaced label, a title in the
	 * readable face, and short lines under it.
	 */
	FSlateFontInfo LabelFont()
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		Font.LetterSpacing = 70;
		return Font;
	}

	FSlateFontInfo TitleFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", 14);
	}

	FSlateFontInfo BodyFont()
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", 11);
	}

	/** The section's colour as a thin bar the height of its text. */
	TSharedRef<SWidget> AccentBar(const TAttribute<FSlateColor>& Colour)
	{
		return SNew(SBox)
			.WidthOverride(3.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
			];
	}

	/** The mission's step as a checklist box, outlined in the department's colour. */
	const FSlateBrush* StepBox()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, 2.0f, FLinearColor::White, 1.5f);
		return &Brush;
	}

	EVisibility CollapsedIfEmpty(const FText& Text)
	{
		return Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
	}

	/** The onboarding objective: amber bar, OBJECTIVE, the task as the title and how to do it under it. */
	TSharedRef<SWidget> ObjectiveSection(const TWeakObjectPtr<UWorld>& World)
	{
		using namespace APSChrome;
		const auto Read = [World](const bool bTitle)
		{
			FText ObjectiveTitle;
			FText ObjectiveBody;
			ReadObjective(World, ObjectiveTitle, ObjectiveBody);
			return bTitle ? ObjectiveTitle : ObjectiveBody;
		};
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				AccentBar(Amber())
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12.0f, 1.0f, 0.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("ObjectiveLabel", "OBJECTIVE")).Font(LabelFont()).ColorAndOpacity(Amber())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(White()).AutoWrapText(true)
					.Text_Lambda([Read]() { return Read(true); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted()).AutoWrapText(true)
					.Text_Lambda([Read]() { return Read(false); })
					.Visibility_Lambda([Read]() { return CollapsedIfEmpty(Read(false)); })
				]
			];
	}

	/**
	 * The tracked mission: a bar and "SCIENCE MISSION" in the department's colour with the count on the right, the title,
	 * the objective as a checklist step with its place under it in spaced capitals, a bar only for counts above one, and
	 * the reward.
	 */
	TSharedRef<SWidget> MissionSection(const TWeakObjectPtr<UWorld>& World)
	{
		using namespace APSChrome;
		const TAttribute<FSlateColor> Accent = TAttribute<FSlateColor>::CreateLambda([World]()
		{
			return FSlateColor(DepartmentColour(World));
		});
		const auto Subject = [World]()
		{
			const FAPSMission* Mission = Tracked(World);
			return Mission ? Mission->SubjectName : FText::GetEmpty();
		};
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				AccentBar(Accent)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12.0f, 1.0f, 0.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(LabelFont()).ColorAndOpacity(Accent)
						.Text_Lambda([World]()
						{
							const FAPSMission* Mission = Tracked(World);
							return Mission ? FText::Format(LOCTEXT("MissionLabel", "{0} MISSION"),
								APSInfrastructure::DepartmentName(Mission->Department)) : FText::GetEmpty();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(White())
						.Text_Lambda([World]()
						{
							const FAPSMission* Mission = Tracked(World);
							return Mission ? FText::Format(LOCTEXT("Count", "{0} / {1}"), APSUINumber::Number(Mission->Progress),
								APSUINumber::Number(Mission->Count)) : FText::GetEmpty();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(White()).AutoWrapText(true)
					.Text_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission ? SentenceCase(Mission->Title) : FText::GetEmpty();
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(1.0f, 4.0f, 9.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
						[
							SNew(SImage).Image(StepBox()).ColorAndOpacity(Accent)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(White())
							.AutoWrapText(true)
							.Text_Lambda([World]()
							{
								const FAPSMission* Mission = Tracked(World);
								return Mission ? SentenceCase(APSMissions::ObjectiveName(Mission->Objective)) : FText::GetEmpty();
							})
						]
						// Where: the site and its world, as the catalogue writes them, in the label's capitals.
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Font(LabelFont()).ColorAndOpacity(Cyan()).AutoWrapText(true)
							.Text_Lambda([Subject]() { return Subject(); })
							.Visibility_Lambda([Subject]() { return CollapsedIfEmpty(Subject()); })
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					// A bar only where it tells something: a single target is either done or not, the count says it.
					SNew(SBox)
					.HeightOverride(4.0f)
					.Visibility_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission && Mission->Count > 1 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					[
						SNew(SProgressBar)
						.BackgroundImage(BarTrack())
						.FillImage(BarFill())
						.FillColorAndOpacity(Accent)
						.Percent_Lambda([World]() -> TOptional<float>
						{
							const FAPSMission* Mission = Tracked(World);
							return Mission ? FMath::Clamp(float(Mission->Progress) / float(FMath::Max(Mission->Count, 1)), 0.0f, 1.0f)
								: 0.0f;
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission && !RewardText(*Mission).IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("RewardLabel", "REWARD")).Font(LabelFont()).ColorAndOpacity(Muted())
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(9.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Amber()).AutoWrapText(true)
						.Text_Lambda([World]()
						{
							const FAPSMission* Mission = Tracked(World);
							return Mission ? RewardText(*Mission) : FText::GetEmpty();
						})
					]
				]
			];
	}

	/**
	 * Rio 03.10: the objective and the tracked mission as one card from one corner, so they stack and never overlap
	 * (two panels at fixed offsets did once the objective wrapped). A divider shows only while both sections do.
	 */
	TSharedRef<SWidget> Build(const TWeakObjectPtr<UWorld> World)
	{
		using namespace APSChrome;
		return SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			// The objective panel's corner: the ship HUD holds the top right (navigation) and the bottom left (flight), the
			// pilot's HUD the bottom left.
			.Padding(FMargin(24.0f, 96.0f, 0.0f, 0.0f))
			.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World) || IsMissionShown(World)); })
			[
				SNew(SBox)
				.WidthOverride(CardWidth)
				[
					ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World)); })
							[
								ObjectiveSection(World)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f)
						[
							SNew(SBox)
							.HeightOverride(1.0f)
							.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World) && IsMissionShown(World)); })
							[
								SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.Visibility_Lambda([World]() { return VisibleIf(IsMissionShown(World)); })
							[
								MissionSection(World)
							]
						],
						FMargin(14.0f, 12.0f), CyanDim())
				]
			];
	}
}

void APSMissionTracker::Tick(UWorld* World)
{
	using namespace APSMissionTrackerPrivate;
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	UGameViewportClient* Viewport = World->GetGameViewport();
	const FEntry* Existing = GEntries.Find(World);
	if (!Viewport || (Existing && Existing->Widget.IsValid() && Existing->Viewport.Get() == Viewport)
		// Only once a mission is tracked or the objective is set (that adds the entry): worlds without either (the menu)
		// never get the widget.
		|| (!Existing && !Tracked(World)))
	{
		return;
	}
	for (auto It = GEntries.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) It.RemoveCurrent();
	}
	FEntry& Entry = GEntries.FindOrAdd(World);
	if (Entry.Widget.IsValid() && Entry.Viewport.IsValid())
	{
		Entry.Viewport->RemoveViewportWidgetContent(Entry.Widget.ToSharedRef());
	}
	Entry.Widget = Build(World);
	Entry.Viewport = Viewport;
	Viewport->AddViewportWidgetContent(Entry.Widget.ToSharedRef(), ZOrder);
}

void APSMissionTracker::Remove(UWorld* World)
{
	using namespace APSMissionTrackerPrivate;
	FEntry Entry;
	if (!GEntries.RemoveAndCopyValue(World, Entry))
	{
		return;
	}
	if (Entry.Widget.IsValid() && Entry.Viewport.IsValid())
	{
		Entry.Viewport->RemoveViewportWidgetContent(Entry.Widget.ToSharedRef());
	}
}

void APSMissionTracker::SetObjective(UWorld* World, TFunction<bool(FText& OutTitle, FText& OutBody)> Objective)
{
	using namespace APSMissionTrackerPrivate;
	if (!World)
	{
		return;
	}
	if (!Objective)
	{
		// Only drops it: the world may be ending, and the card must not come back for it.
		if (FEntry* Entry = GEntries.Find(World))
		{
			Entry->Objective.Reset();
		}
		return;
	}
	GEntries.FindOrAdd(World).Objective = MoveTemp(Objective);
	// The card reads the objective every frame, so one already in the view shows it at once; otherwise add it now.
	Tick(World);
}

#undef LOCTEXT_NAMESPACE
