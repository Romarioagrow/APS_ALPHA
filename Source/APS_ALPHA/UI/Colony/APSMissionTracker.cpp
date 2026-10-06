#include "APSMissionTracker.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APS_ALPHA/UI/Hud/APSHudKit.h"

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
	/** Fixed, as both former panels were: auto-wrapped text gives a self-sizing card no stable width. Rio 06.10: 30 %
	 * smaller than the 420 it was. */
	constexpr float CardWidth = 300.0f;

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
		static FSlateColorBrush Brush(FLinearColor::White);
		Brush.TintColor = APSChrome::CyanDim();
		return &Brush;
	}

	const FSlateBrush* BarFill()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	/**
	 * Rio 06.10 ("Flight HUD v4", the TASKS card): the HUD's instrument family (APSHud). A small TASKS header with the
	 * TAB key, the objective with an action-coloured box, then the tracked mission: a dot in the department's colour,
	 * the count, the title, its step and its place as a chip. No reward line (Rio: "+20 research" did not belong there).
	 */
	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	TAttribute<FSlateColor> Themed(TFunction<FLinearColor()> Pick)
	{
		return TAttribute<FSlateColor>::CreateLambda([Pick]() { return FSlateColor(Pick()); });
	}

	FSlateFontInfo TitleFont()
	{
		return APSHud::TextFont(13, TEXT("Bold"));
	}

	FSlateFontInfo BodyFont()
	{
		return APSHud::TextFont(11);
	}

	/** A checklist box outlined in its colour. */
	const FSlateBrush* StepBox()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, 1.0f, FLinearColor::White, 1.5f);
		return &Brush;
	}

	const FSlateBrush* Dot()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 3.0f);
		return &Brush;
	}

	EVisibility CollapsedIfEmpty(const FText& Text)
	{
		return Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
	}

	TSharedRef<SWidget> Box(const float Size, const TAttribute<FSlateColor>& Colour)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		[
			SNew(SImage).Image(StepBox()).ColorAndOpacity(Colour)
		];
	}

	/** The onboarding objective: an action-coloured box, the task as the title and how to do it under it. */
	TSharedRef<SWidget> ObjectiveSection(const TWeakObjectPtr<UWorld>& World)
	{
		const auto Read = [World](const bool bTitle)
		{
			FText ObjectiveTitle;
			FText ObjectiveBody;
			ReadObjective(World, ObjectiveTitle, ObjectiveBody);
			return bTitle ? ObjectiveTitle : ObjectiveBody;
		};
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 3.0f, 10.0f, 0.0f)
			[
				Box(10.0f, Themed([]() { return P().Action; }))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(Themed([]() { return P().Text; })).AutoWrapText(true)
					.Text_Lambda([Read]() { return Read(true); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Themed([]() { return P().TextSoft; })).AutoWrapText(true)
					.Text_Lambda([Read]() { return Read(false); })
					.Visibility_Lambda([Read]() { return CollapsedIfEmpty(Read(false)); })
				]
			];
	}

	/** The tracked mission: department dot and name with the count, the title, the step and its place as a chip. */
	TSharedRef<SWidget> MissionSection(const TWeakObjectPtr<UWorld>& World)
	{
		const TAttribute<FSlateColor> Accent = TAttribute<FSlateColor>::CreateLambda([World]()
		{
			return FSlateColor(DepartmentColour(World));
		});
		const auto Subject = [World]()
		{
			const FAPSMission* Mission = Tracked(World);
			return Mission ? Mission->SubjectName : FText::GetEmpty();
		};
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(6.0f).HeightOverride(6.0f)[SNew(SImage).Image(Dot()).ColorAndOpacity(Accent)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(APSHud::LabelFont()).ColorAndOpacity(Themed([]() { return P().TextQuiet; }))
					.Text_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission ? FText::Format(LOCTEXT("MissionLabel", "{0} MISSION"),
							APSInfrastructure::DepartmentName(Mission->Department)) : FText::GetEmpty();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(APSHud::ValueFont(10)).ColorAndOpacity(Themed([]() { return P().Text; }))
					.Text_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission ? FText::Format(LOCTEXT("Count", "{0} / {1}"), APSUINumber::Number(Mission->Progress),
							APSUINumber::Number(Mission->Count)) : FText::GetEmpty();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(Themed([]() { return P().Text; })).AutoWrapText(true)
				.Text_Lambda([World]()
				{
					const FAPSMission* Mission = Tracked(World);
					return Mission ? SentenceCase(Mission->Title) : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 3.0f, 9.0f, 0.0f)
				[
					Box(8.0f, Themed([]() { return P().Highlight; }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Themed([]() { return P().TextSoft; })).AutoWrapText(true)
					.Text_Lambda([World]()
					{
						const FAPSMission* Mission = Tracked(World);
						return Mission ? SentenceCase(APSMissions::ObjectiveName(Mission->Objective)) : FText::GetEmpty();
					})
				]
			]
			// Where: the site and its world, as the catalogue writes them, on a chip.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(17.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(1.0f)
				.BorderBackgroundColor(Themed([]() { return APSUITheme::Fade(P().Frame, 1.2f); }))
				.Visibility_Lambda([Subject]() { return CollapsedIfEmpty(Subject()); })
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(FMargin(7.0f, 2.0f))
					.BorderBackgroundColor(Themed([]() { return P().Panel.CopyWithNewOpacity(1.0f); }))
					[
						SNew(STextBlock).Font(APSHud::LabelFont(9)).ColorAndOpacity(Themed([]() { return P().Highlight; }))
						.Text_Lambda([Subject]() { return Subject(); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				// A bar only where it tells something: a single target is either done or not, the count says it.
				SNew(SBox)
				.HeightOverride(3.0f)
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
			];
	}

	/**
	 * Rio 03.10: the objective and the tracked mission as one card from one corner, so they stack and never overlap
	 * (two panels at fixed offsets did once the objective wrapped). A rule shows only while both sections do.
	 */
	TSharedRef<SWidget> Build(const TWeakObjectPtr<UWorld> World)
	{
		return SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			// The objective panel's corner: the ship HUD holds the top right (navigation) and the bottom (flight).
			.Padding(FMargin(22.0f, 22.0f, 0.0f, 0.0f))
			.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World) || IsMissionShown(World)); })
			[
				SNew(SBox)
				.WidthOverride(CardWidth)
				[
					APSHud::Card(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[APSHud::Label(LOCTEXT("Tasks", "TASKS"), Themed([]() { return P().TextQuiet; }))]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[APSChrome::KeyChip(LOCTEXT("TasksKey", "TAB"), P().TextQuiet)]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 10.0f)[APSHud::Rule()]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox)
							.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World)); })
							[
								ObjectiveSection(World)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)
						[
							SNew(SBox)
							.Visibility_Lambda([World]() { return VisibleIf(IsObjectiveShown(World) && IsMissionShown(World)); })
							[
								APSHud::Rule()
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
						APSHud::EEdge::Left)
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
