#include "APSMissionTracker.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APS_ALPHA/UI/Hud/APSHudKit.h"

#include "APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncients.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncientsQuests.h"
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

	TAutoConsoleVariable<int32> CVarTrackerBrief(TEXT("aps.Quests.TrackerBrief"), 1,
		TEXT("1: the TASKS card shows how to do the tracked step (its brief) under the objective. 0: the objective alone."));

	FText TrackedBrief(const TWeakObjectPtr<UWorld>& World)
	{
		const FAPSMission* Mission = CVarTrackerBrief.GetValueOnGameThread() != 0 ? Tracked(World) : nullptr;
		return Mission ? Mission->Brief : FText::GetEmpty();
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
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Themed([]() { return P().TextSoft; })).AutoWrapText(true)
						.Text_Lambda([World]()
						{
							const FAPSMission* Mission = Tracked(World);
							return Mission ? SentenceCase(APSMissions::ObjectiveName(Mission->Objective)) : FText::GetEmpty();
						})
					]
					// Rio 09.10 ("the player does not understand the chains"): how to do the step, the department's brief.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(APSHud::TextFont(10)).ColorAndOpacity(Themed([]() { return P().TextQuiet; }))
						.AutoWrapText(true)
						.Text_Lambda([World]() { return TrackedBrief(World); })
						.Visibility_Lambda([World]() { return CollapsedIfEmpty(TrackedBrief(World)); })
					]
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

/**
 * Rio 09.10 ("the quest chains work, but the player does not understand them"): a notice at the top of the game view as
 * a step or mission starts or completes and as a chain starts or ends: what happened and the one line of what to do next.
 * It watches the board (no event of its own: the board, the ancients and their saves stay as they are), pairs a step's
 * end with the next step's start (the ancients put that a moment later), and moves the tracking to the chain that moved.
 */
namespace APSMissionTrackerPrivate
{
	TAutoConsoleVariable<int32> CVarToasts(TEXT("aps.Quests.Toasts"), 1,
		TEXT("1: a notice at the top of the game view when a quest step or mission starts or completes and when a chain ")
		TEXT("starts or ends, with what to do next. 0: none."));
	TAutoConsoleVariable<float> CVarToastSeconds(TEXT("aps.Quests.ToastSeconds"), 5.5f,
		TEXT("Seconds each quest notice stays in view (its clock waits under the terminal and the maps)."));
	TAutoConsoleVariable<int32> CVarAutoTrack(TEXT("aps.Quests.AutoTrack"), 1,
		TEXT("1: a chain whose step starts or moves becomes the tracked one, so the TASKS card shows what changed. ")
		TEXT("0: the tracking stays as chosen."));

	/** Over the HUDs and the tracker card (880), under the terminal (890), the F10 map (900) and the menu (1000). */
	constexpr int32 ToastZOrder = 885;
	constexpr float ToastWidth = 440.0f;
	/** A step's end and the next step's start (the ancients put it on their next look) are paired within this. */
	constexpr double SettleSeconds = 0.8;
	constexpr int32 MaxQueued = 4;
	/** After the board is first seen: the save (if any) lands then, and nothing is announced. */
	constexpr double QuietSeconds = 6.0;

	struct FToast
	{
		FText Kicker;
		FText Title;
		FText Body;
		FText Next;
		FLinearColor Accent{FLinearColor::White};
		double Age{0.0};
	};

	struct FSeen
	{
		APSMissions::EState State{APSMissions::EState::Offered};
		int32 Progress{0};
	};

	struct FToasts
	{
		TSharedPtr<SWidget> Widget;
		TWeakObjectPtr<UGameViewportClient> Viewport;
		TMap<FGuid, FSeen> Seen;
		bool bPrimed{false};
		double PrimedAt{0.0};
		uint32 Revision{0};
		TArray<FAPSMission> Completed;
		TArray<FAPSMission> Started;
		double FirstEventAt{0.0};
		TArray<FToast> Queue;
		double LastTick{0.0};
	};
	TMap<TWeakObjectPtr<UWorld>, FToasts> GToasts;

	/** Where a mission stands in a chain: an ancients step (its chain, step and length) or a department mission. */
	struct FChainStep
	{
		bool bAncient{false};
		FText Chain;
		int32 Step{INDEX_NONE};
		int32 Steps{0};
		FString SiteId;
	};

	FChainStep ChainStepOf(const UWorld* World, const FAPSMission& Mission)
	{
		FChainStep Out;
		Out.Chain = APSInfrastructure::DepartmentName(Mission.Department);
		const FAPSAncients* Ancients = APSAncientsQuests::IsAncientTemplate(Mission.Template) ? APSAncientsFind(World) : nullptr;
		int32 Step = INDEX_NONE;
		if (const FAPSAncients::FSite* Site = Ancients ? Ancients->FindByTemplate(Mission.Template, Step) : nullptr)
		{
			Out.bAncient = true;
			Out.Chain = APSAncients::ChainName(Site->Spec.Chain).ToUpper();
			Out.Step = Step;
			Out.Steps = APSAncientsQuests::ChainOf(Site->Spec.Chain).Steps.Num();
			Out.SiteId = Site->Spec.Id;
		}
		return Out;
	}

	bool IsFollowUp(const FAPSMission& Done, const FChainStep& DoneStep, const FAPSMission& New, const FChainStep& NewStep)
	{
		return (!Done.Next.IsNone() && New.Template == Done.Next)
			|| (DoneStep.bAncient && NewStep.bAncient && DoneStep.SiteId == NewStep.SiteId && NewStep.Step == DoneStep.Step + 1);
	}

	/** What to do next: an offer is accepted in the terminal; an active step says how (its brief). */
	FText NextLine(const FAPSMission& Mission)
	{
		if (Mission.State == APSMissions::EState::Offered)
		{
			return FText::Format(LOCTEXT("ToastAccept", "{0}. Accept it in the colony terminal: DIVISIONS, {1}."),
				SentenceCase(Mission.Title), APSInfrastructure::DepartmentName(Mission.Department));
		}
		const FText How = Mission.Brief.IsEmpty() ? SentenceCase(APSMissions::ObjectiveName(Mission.Objective)) : Mission.Brief;
		return FText::Format(LOCTEXT("ToastNext", "{0}. {1}"), SentenceCase(Mission.Title), How);
	}

	FText StepOf(const FChainStep& Step)
	{
		return FText::Format(LOCTEXT("ToastStepOf", "STEP {0} OF {1}"), FText::AsNumber(Step.Step + 1), FText::AsNumber(Step.Steps));
	}

	void Push(FToasts& Toasts, FToast&& Toast)
	{
		// The one in view finishes; the oldest waiting gives way.
		if (Toasts.Queue.Num() >= MaxQueued)
		{
			Toasts.Queue.RemoveAt(1);
		}
		Toasts.Queue.Add(MoveTemp(Toast));
	}

	/** The settled events as notices: each end with its follow-up, then the starts nobody's end explained. */
	void Settle(const UWorld* World, FToasts& Toasts)
	{
		TArray<bool> Used;
		Used.Init(false, Toasts.Started.Num());
		for (const FAPSMission& Done : Toasts.Completed)
		{
			const FChainStep DoneStep = ChainStepOf(World, Done);
			const FAPSMission* Follow = nullptr;
			for (int32 Index = 0; Index < Toasts.Started.Num() && !Follow; ++Index)
			{
				if (!Used[Index] && IsFollowUp(Done, DoneStep, Toasts.Started[Index], ChainStepOf(World, Toasts.Started[Index])))
				{
					Used[Index] = true;
					Follow = &Toasts.Started[Index];
				}
			}
			const bool bChainDone = DoneStep.bAncient && DoneStep.Step + 1 >= DoneStep.Steps;
			FToast Toast;
			Toast.Accent = APSInfrastructure::DepartmentColour(Done.Department);
			Toast.Kicker = bChainDone ? FText::Format(LOCTEXT("ToastChainDone", "QUEST CHAIN COMPLETE  ·  {0}"), DoneStep.Chain)
				: DoneStep.bAncient ? FText::Format(LOCTEXT("ToastStepDone", "{0} COMPLETE  ·  {1}"), StepOf(DoneStep), DoneStep.Chain)
				: Follow ? FText::Format(LOCTEXT("ToastChainStepDone", "CHAIN STEP COMPLETE  ·  {0}"), DoneStep.Chain)
				: FText::Format(LOCTEXT("ToastMissionDone", "MISSION COMPLETE  ·  {0}"), DoneStep.Chain);
			Toast.Title = SentenceCase(Done.Title);
			const FText Reward = RewardText(Done);
			Toast.Body = Reward.IsEmpty() ? LOCTEXT("ToastDone", "Done.")
				: FText::Format(LOCTEXT("ToastDoneReward", "Done. Reward: {0}"), Reward);
			Toast.Next = Follow ? NextLine(*Follow)
				: bChainDone ? LOCTEXT("ToastReadChain", "The whole chain and its story: colony terminal, JOURNAL.")
				: DoneStep.bAncient ? LOCTEXT("ToastNextSoon", "The next step comes to the DIVISIONS board shortly.")
				: FText::GetEmpty();
			Push(Toasts, MoveTemp(Toast));
		}
		for (int32 Index = 0; Index < Toasts.Started.Num(); ++Index)
		{
			if (Used[Index])
			{
				continue;
			}
			const FAPSMission& New = Toasts.Started[Index];
			const FChainStep NewStep = ChainStepOf(World, New);
			const bool bOffer = New.State == APSMissions::EState::Offered;
			FToast Toast;
			Toast.Accent = APSInfrastructure::DepartmentColour(New.Department);
			Toast.Kicker = !NewStep.bAncient ? FText::Format(LOCTEXT("ToastMissionActive", "MISSION ACTIVE  ·  {0}"), NewStep.Chain)
				: bOffer ? FText::Format(LOCTEXT("ToastStepOffer", "{0} OFFERED  ·  {1}"), StepOf(NewStep), NewStep.Chain)
				: NewStep.Step == 0 ? FText::Format(LOCTEXT("ToastChainStart", "QUEST CHAIN STARTED  ·  {0}"), NewStep.Chain)
				: FText::Format(LOCTEXT("ToastStepNew", "NEW {0}  ·  {1}"), StepOf(NewStep), NewStep.Chain);
			Toast.Title = SentenceCase(New.Title);
			Toast.Body = New.SubjectName;
			Toast.Next = NextLine(New);
			Push(Toasts, MoveTemp(Toast));
		}
		Toasts.Completed.Reset();
		Toasts.Started.Reset();
	}

	/** Reads the board when its revision moved: what started, completed and moved since the last look. */
	void Watch(FToasts& Toasts, FAPSMissionBoard& Board, const double Now)
	{
		if (Toasts.bPrimed && Board.GetRevision() == Toasts.Revision)
		{
			return;
		}
		Toasts.Revision = Board.GetRevision();
		TArray<FAPSMission> Completed;
		TArray<FAPSMission> Started;
		FGuid TrackTo;
		bool bRestored = false;
		TMap<FGuid, FSeen> Seen;
		for (const FAPSMission& Mission : Board.GetMissions())
		{
			Seen.Add(Mission.Id, FSeen{Mission.State, Mission.Progress});
			const FSeen* Old = Toasts.Seen.Find(Mission.Id);
			const bool bAncient = APSAncientsQuests::IsAncientTemplate(Mission.Template);
			if (Mission.State == APSMissions::EState::Completed)
			{
				if (Old && Old->State == APSMissions::EState::Active)
				{
					Completed.Add(Mission);
				}
				// Only a load brings a mission that was never seen already completed.
				bRestored |= !Old;
			}
			else if (Mission.State == APSMissions::EState::Active && (!Old || Old->State != APSMissions::EState::Active))
			{
				Started.Add(Mission);
				// The department's own follow-ups and accepts are tracked by the board; a Builders' step only when free.
				if (bAncient)
				{
					TrackTo = Mission.Id;
				}
			}
			else if (Mission.State == APSMissions::EState::Offered && !Old && bAncient)
			{
				Started.Add(Mission);
			}
			else if (Mission.State == APSMissions::EState::Active && Old && Mission.Progress > Old->Progress
				&& (bAncient || !Mission.Next.IsNone()))
			{
				TrackTo = Mission.Id;
			}
		}
		if (!Toasts.bPrimed)
		{
			Toasts.PrimedAt = Now;
		}
		Toasts.Seen = MoveTemp(Seen);
		Toasts.bPrimed = true;
		// The first look, the first seconds (a save is applied then) and a load bring many at once: nothing happened in play.
		if (Now - Toasts.PrimedAt < QuietSeconds || bRestored || (Completed.IsEmpty() && Started.Num() >= 3))
		{
			return;
		}
		if (CVarAutoTrack.GetValueOnGameThread() != 0 && TrackTo.IsValid())
		{
			const FAPSMission* Current = Board.GetTracked();
			if (!Current || Current->Id != TrackTo)
			{
				Board.SetTracked(TrackTo);
				Toasts.Revision = Board.GetRevision();
			}
		}
		if (CVarToasts.GetValueOnGameThread() == 0 || (Completed.IsEmpty() && Started.IsEmpty()))
		{
			return;
		}
		if (Toasts.Completed.IsEmpty() && Toasts.Started.IsEmpty())
		{
			Toasts.FirstEventAt = Now;
		}
		Toasts.Completed.Append(MoveTemp(Completed));
		Toasts.Started.Append(MoveTemp(Started));
	}

	const FToast* CurrentToast(const TWeakObjectPtr<UWorld>& World)
	{
		const FToasts* Toasts = GToasts.Find(World);
		return Toasts && !Toasts->Queue.IsEmpty() ? &Toasts->Queue[0] : nullptr;
	}

	/** Out of the terminal and the maps, as the card is; its clock waits there, so a notice is never missed. */
	bool IsToastRoomFree(const TWeakObjectPtr<UWorld>& World)
	{
		return !IsMapOpen(World) && !IsTerminalOpen(World);
	}

	bool IsToastShown(const TWeakObjectPtr<UWorld>& World)
	{
		return CVarToasts.GetValueOnGameThread() != 0 && CurrentToast(World) && IsToastRoomFree(World);
	}

	float ToastOpacity(const TWeakObjectPtr<UWorld>& World)
	{
		const FToast* Toast = CurrentToast(World);
		if (!Toast)
		{
			return 0.0f;
		}
		const double Duration = FMath::Max(1.0, double(CVarToastSeconds.GetValueOnGameThread()));
		return float(FMath::Clamp(FMath::Min(Toast->Age / 0.2, (Duration - Toast->Age) / 0.45), 0.0, 1.0));
	}

	/** The tracker card's family (APSHud): a dot and the kicker in the department's colour, the title, what happened,
	 * and under a rule NEXT with an action-coloured box and the line of what to do. */
	TSharedRef<SWidget> BuildToast(const TWeakObjectPtr<UWorld> World)
	{
		const auto Read = [World](FText FToast::* Field)
		{
			const FToast* Toast = CurrentToast(World);
			return Toast ? Toast->*Field : FText::GetEmpty();
		};
		const TAttribute<FSlateColor> Accent = TAttribute<FSlateColor>::CreateLambda([World]()
		{
			const FToast* Toast = CurrentToast(World);
			return FSlateColor(Toast ? Toast->Accent : APSChrome::Cyan());
		});
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.0f, 92.0f, 0.0f, 0.0f))
			.Visibility_Lambda([World]() { return VisibleIf(IsToastShown(World)); })
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.Padding(0.0f)
				.ColorAndOpacity_Lambda([World]() { return FLinearColor(1.0f, 1.0f, 1.0f, ToastOpacity(World)); })
				[
					SNew(SBox)
					.WidthOverride(ToastWidth)
					[
						APSHud::Card(
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
								[
									SNew(SBox).WidthOverride(6.0f).HeightOverride(6.0f)[SNew(SImage).Image(Dot()).ColorAndOpacity(Accent)]
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
								[
									SNew(STextBlock).Font(APSHud::LabelFont()).ColorAndOpacity(Accent)
									.Text_Lambda([Read]() { return Read(&FToast::Kicker); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Font(APSHud::TextFont(15, TEXT("Bold"))).ColorAndOpacity(Themed([]() { return P().Text; }))
								.AutoWrapText(true)
								.Text_Lambda([Read]() { return Read(&FToast::Title); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Themed([]() { return P().TextSoft; })).AutoWrapText(true)
								.Text_Lambda([Read]() { return Read(&FToast::Body); })
								.Visibility_Lambda([Read]() { return CollapsedIfEmpty(Read(&FToast::Body)); })
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([Read]() { return CollapsedIfEmpty(Read(&FToast::Next)); })
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f)[APSHud::Rule()]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 3.0f, 9.0f, 0.0f)
									[
										Box(8.0f, Themed([]() { return P().Action; }))
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()
										[
											APSHud::Label(LOCTEXT("ToastNextLabel", "NEXT"), Themed([]() { return P().TextQuiet; }))
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
										[
											SNew(STextBlock).Font(APSHud::TextFont(12)).ColorAndOpacity(Themed([]() { return P().Text; }))
											.AutoWrapText(true)
											.Text_Lambda([Read]() { return Read(&FToast::Next); })
										]
									]
								]
							],
							APSHud::EEdge::Centre)
					]
				]
			];
	}

	void TickToasts(UWorld* World)
	{
		FAPSMissionBoard* Board = APSMissionsFind(World);
		if (!Board)
		{
			return;
		}
		const double Now = FPlatformTime::Seconds();
		FToasts* Found = GToasts.Find(World);
		if (!Found)
		{
			for (auto It = GToasts.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
			Found = &GToasts.Add(World);
		}
		FToasts& Toasts = *Found;
		const double Delta = Toasts.LastTick > 0.0 ? FMath::Clamp(Now - Toasts.LastTick, 0.0, 0.25) : 0.0;
		Toasts.LastTick = Now;
		Watch(Toasts, *Board, Now);
		if (CVarToasts.GetValueOnGameThread() == 0)
		{
			Toasts.Queue.Reset();
			Toasts.Completed.Reset();
			Toasts.Started.Reset();
		}
		if ((!Toasts.Completed.IsEmpty() || !Toasts.Started.IsEmpty()) && Now - Toasts.FirstEventAt >= SettleSeconds)
		{
			Settle(World, Toasts);
		}
		if (!Toasts.Queue.IsEmpty() && IsToastRoomFree(World))
		{
			Toasts.Queue[0].Age += Delta;
			if (Toasts.Queue[0].Age >= FMath::Max(1.0, double(CVarToastSeconds.GetValueOnGameThread())))
			{
				Toasts.Queue.RemoveAt(0);
			}
		}
		// The widget joins the view with the first notice (never in worlds without one) and follows a new viewport.
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (!Viewport || (Toasts.Widget.IsValid() && Toasts.Viewport.Get() == Viewport) || (!Toasts.Widget.IsValid() && Toasts.Queue.IsEmpty()))
		{
			return;
		}
		if (Toasts.Widget.IsValid() && Toasts.Viewport.IsValid())
		{
			Toasts.Viewport->RemoveViewportWidgetContent(Toasts.Widget.ToSharedRef());
		}
		Toasts.Widget = BuildToast(World);
		Toasts.Viewport = Viewport;
		Viewport->AddViewportWidgetContent(Toasts.Widget.ToSharedRef(), ToastZOrder);
	}

	void RemoveToasts(UWorld* World)
	{
		FToasts Toasts;
		if (GToasts.RemoveAndCopyValue(World, Toasts) && Toasts.Widget.IsValid() && Toasts.Viewport.IsValid())
		{
			Toasts.Viewport->RemoveViewportWidgetContent(Toasts.Widget.ToSharedRef());
		}
	}

	FAutoConsoleCommandWithWorld ToastTestCommand(TEXT("aps.Quests.ToastTest"),
		TEXT("Shows a sample quest notice (the tracked mission's, when one is tracked)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld)
		{
			if (!InWorld || !APSMissionsFind(InWorld))
			{
				return;
			}
			FToasts& Toasts = GToasts.FindOrAdd(InWorld);
			const FAPSMission* Mission = Tracked(InWorld);
			FToast Toast;
			Toast.Accent = Mission ? APSInfrastructure::DepartmentColour(Mission->Department) : APSChrome::Cyan();
			Toast.Kicker = LOCTEXT("ToastTestKicker", "QUEST NOTICE  ·  TEST");
			Toast.Title = Mission ? SentenceCase(Mission->Title) : LOCTEXT("ToastTestTitle", "A shape on the horizon");
			Toast.Body = Mission ? Mission->SubjectName : LOCTEXT("ToastTestBody", "UNKNOWN STRUCTURE · RESO");
			Toast.Next = Mission ? NextLine(*Mission) : LOCTEXT("ToastTestNext", "Fly within 25 km of the marked site, or send any ship there.");
			Push(Toasts, MoveTemp(Toast));
		}));
}

void APSMissionTracker::Tick(UWorld* World)
{
	using namespace APSMissionTrackerPrivate;
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	TickToasts(World);
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
	RemoveToasts(World);
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
