#include "APSMissionTracker.h"

#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSMissionTracker"

namespace APSMissionTrackerPrivate
{
	TAutoConsoleVariable<int32> CVarTracker(TEXT("aps.UI.MissionTracker"), 1,
		TEXT("1: the tracked department mission shows in the top-left corner of the game view. 0: hidden."));

	/** Over the walking HUD (40), under the prompts (50), the ship HUD (60), the maps and the terminal. */
	constexpr int32 ZOrder = 45;

	struct FEntry
	{
		TSharedPtr<SWidget> Widget;
		TWeakObjectPtr<UGameViewportClient> Viewport;
	};
	TMap<TWeakObjectPtr<UWorld>, FEntry> GEntries;

	/** The F10 map holds the view with its own camera; the tracker stays out of it. */
	bool IsMapOpen(const TWeakObjectPtr<UWorld>& World)
	{
		const AGravityPlayerController* Controller = World.IsValid()
			? Cast<AGravityPlayerController>(World->GetFirstPlayerController()) : nullptr;
		return Controller && Controller->IsStrategicMapOpen();
	}

	const FAPSMission* Tracked(const TWeakObjectPtr<UWorld>& World)
	{
		const FAPSMissionBoard* Board = World.IsValid() ? APSMissionsFind(World.Get()) : nullptr;
		const FAPSMission* Mission = Board ? Board->GetTracked() : nullptr;
		return Mission && Mission->State == APSMissions::EState::Active ? Mission : nullptr;
	}

	FText RewardText(const FAPSMission& Mission)
	{
		FString Text = APSInfrastructure::DescribeAmounts(Mission.Reward).ToString();
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
		return Text.IsEmpty() ? FText::GetEmpty() : FText::FromString(TEXT("REWARD   ") + Text);
	}

	TSharedRef<SWidget> Build(const TWeakObjectPtr<UWorld> World)
	{
		const auto DepartmentColour = [World]() -> FSlateColor
		{
			const FAPSMission* Mission = Tracked(World);
			return Mission ? FSlateColor(APSInfrastructure::DepartmentColour(Mission->Department)) : FSlateColor(APSChrome::Cyan());
		};
		return SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			// Under the onboarding objective panel, which holds the top-left corner.
			.Padding(FMargin(20.0f, 150.0f, 0.0f, 0.0f))
			.Visibility_Lambda([World]()
			{
				return CVarTracker.GetValueOnGameThread() != 0 && Tracked(World) && !IsMapOpen(World)
					? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SBox)
				.WidthOverride(340.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.005f, 0.018f, 0.035f, 0.82f))
					.Padding(FMargin(14.0f, 10.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Font(APSChrome::Font("Bold", 9))
							.ColorAndOpacity_Lambda(DepartmentColour)
							.Text_Lambda([World]()
							{
								const FAPSMission* Mission = Tracked(World);
								return Mission ? FText::Format(LOCTEXT("Header", "TRACKED MISSION   /   {0}"),
									APSInfrastructure::DepartmentName(Mission->Department)) : FText::GetEmpty();
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Font(APSChrome::Font("Bold", 13))
							.ColorAndOpacity(FLinearColor(0.93f, 0.97f, 1.0f))
							.AutoWrapText(true)
							.Text_Lambda([World]()
							{
								const FAPSMission* Mission = Tracked(World);
								return Mission ? Mission->Title : FText::GetEmpty();
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Font(APSChrome::Font("Regular", 11))
							.ColorAndOpacity(FLinearColor(0.70f, 0.90f, 1.0f, 0.95f))
							.AutoWrapText(true)
							.Text_Lambda([World]()
							{
								const FAPSMission* Mission = Tracked(World);
								if (!Mission) return FText::GetEmpty();
								return Mission->SubjectName.IsEmpty() ? APSMissions::ObjectiveName(Mission->Objective)
									: FText::Format(LOCTEXT("Objective", "{0}: {1}"), APSMissions::ObjectiveName(Mission->Objective),
										Mission->SubjectName);
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(SBox)
								.HeightOverride(5.0f)
								[
									SNew(SProgressBar)
									.FillColorAndOpacity_Lambda(DepartmentColour)
									.Percent_Lambda([World]() -> TOptional<float>
									{
										const FAPSMission* Mission = Tracked(World);
										return Mission ? FMath::Clamp(float(Mission->Progress) / float(FMath::Max(Mission->Count, 1)), 0.0f, 1.0f)
											: 0.0f;
									})
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Font(APSChrome::Font("Bold", 11))
								.ColorAndOpacity(FLinearColor(0.93f, 0.97f, 1.0f))
								.Text_Lambda([World]()
								{
									const FAPSMission* Mission = Tracked(World);
									return Mission ? FText::Format(LOCTEXT("Count", "{0} / {1}"), APSUINumber::Number(Mission->Progress),
										APSUINumber::Number(Mission->Count)) : FText::GetEmpty();
								})
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Font(APSChrome::Font("Regular", 10))
							.ColorAndOpacity(FLinearColor(0.62f, 0.70f, 0.76f, 0.9f))
							.AutoWrapText(true)
							.Text_Lambda([World]()
							{
								const FAPSMission* Mission = Tracked(World);
								return Mission ? RewardText(*Mission) : FText::GetEmpty();
							})
						]
					]
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
		// Only once a mission is tracked: worlds without missions (the menu) never get the widget.
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

#undef LOCTEXT_NAMESPACE
