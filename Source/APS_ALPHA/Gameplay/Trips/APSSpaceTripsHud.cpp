#include "APSSpaceTripsHud.h"

#include "APSSpaceTrips.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/UI/Hud/APSHudKit.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSSpaceTripsHud"

namespace APSSpaceTripsHudLocal
{
	TAutoConsoleVariable<int32> CVarKeys(TEXT("aps.Trip.Keys"), 1,
		TEXT("Rio 19.10 (SPACE TRIPS without the console): 1 binds O (the route card: Up / Down, Enter, O or Escape) and P (a still ")
		TEXT("without the instruments) in a trip world. 0: the console only, as before."));

	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<UInputComponent> Input;
		TSharedPtr<SWidget> Widget;
		TSharedPtr<SVerticalBox> List;
		TArray<APSSpaceTrips::FRoute> Routes;
		int32 Selected{0};
		bool bOpen{false};
	};

	TArray<FState>& States()
	{
		static TArray<FState> Entries;
		return Entries;
	}

	FState* Find(const UWorld* World)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	/** The world's end: its card leaves the view (the viewport outlives the level). */
	void ForgetWorld(UWorld* World, bool, bool)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World && Entry.Widget.IsValid() && GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(Entry.Widget.ToSharedRef());
			}
		}
		States().RemoveAll([World](const FState& Entry) { return Entry.World.Get() == World || !Entry.World.IsValid(); });
	}

	FState& StateFor(UWorld* World)
	{
		if (FState* Existing = Find(World))
		{
			return *Existing;
		}
		static bool bCleanupBound = false;
		if (!bCleanupBound)
		{
			bCleanupBound = true;
			FWorldDelegates::OnWorldCleanup.AddStatic(&ForgetWorld);
		}
		States().RemoveAll([](const FState& Entry) { return !Entry.World.IsValid(); });
		FState& Added = States().AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	FText DistanceLine(UWorld* World, const APSSpaceTrips::FRoute& Route)
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Stars || !Pawn || Route.Stops.IsEmpty())
		{
			return FText::GetEmpty();
		}
		const FAPSStarSystemInfo* First = Stars->Get(Route.Stops[0].SystemIndex);
		if (!First || First->bHome)
		{
			return LOCTEXT("HereLine", "starts here, in the home system");
		}
		const double Cm = FVector::Dist(Pawn->GetActorLocation() + UAPSWorldOriginSubsystem::SkyOffsetOf(World), First->Location);
		constexpr double LightYearCm = 9.4607e17;
		return FText::Format(LOCTEXT("FirstStopLine", "first stop {0} ly away"),
			FText::AsNumber(FMath::RoundToDouble(Cm / LightYearCm * 100.0) / 100.0));
	}

	void Fill(FState& State)
	{
		if (!State.List.IsValid())
		{
			return;
		}
		State.List->ClearChildren();
		UWorld* World = State.World.Get();
		const APSSpaceTrips::FTrip* Trip = APSSpaceTrips::Find(World, false);
		const FString Running = Trip && Trip->GetPhase() != APSSpaceTrips::EPhase::Idle && Trip->GetPhase() != APSSpaceTrips::EPhase::Done
			? Trip->GetRoute().Id : FString();
		for (int32 Index = 0; Index < State.Routes.Num(); ++Index)
		{
			const APSSpaceTrips::FRoute& Route = State.Routes[Index];
			const bool bSelected = Index == State.Selected;
			const FText Meta = FText::Format(LOCTEXT("RouteMeta", "{0} {0}|plural(one=stop,other=stops) · {1}{2}"),
				FText::AsNumber(Route.Stops.Num()), DistanceLine(World, Route),
				Route.Id == Running ? LOCTEXT("Running", " · NOW") : FText::GetEmpty());
			State.List->AddSlot().AutoHeight().Padding(0.0f, Index == 0 ? 2.0f : 8.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("RouteName", "{0}{1}"), bSelected ? FText::FromString(TEXT("> ")) : FText::FromString(TEXT("  ")),
						Route.Name))
					.Font(APSHud::ValueFont(bSelected ? 15 : 13))
					.ColorAndOpacity(FSlateColor(bSelected ? P().Action : P().Text))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(18.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Route.Subtitle).Font(APSHud::TextFont(11)).AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(bSelected ? P().Text : P().TextSoft))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(18.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Meta).Font(APSHud::TextFont(10)).ColorAndOpacity(FSlateColor(P().TextQuiet))
				]
			];
		}
		if (State.Routes.IsEmpty())
		{
			State.List->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("NoRoutes", "The star catalogue is still being read: a moment.")).Font(APSHud::TextFont(11))
				.ColorAndOpacity(FSlateColor(P().TextSoft))
			];
		}
	}

	void EnsureWidget(FState& State)
	{
		if (State.Widget.IsValid() || !GEngine || !GEngine->GameViewport)
		{
			return;
		}
		const TWeakObjectPtr<UWorld> WeakWorld = State.World;
		SAssignNew(State.List, SVerticalBox);
		State.Widget = SNew(SOverlay)
			.Visibility_Lambda([WeakWorld]()
			{
				const FState* Live = Find(WeakWorld.Get());
				return Live && Live->bOpen ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
			})
			// trip-t2 (19.10): on the left it ran into the TASKS card; a choice in the middle of the view, the sky around it.
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(430.0f)
				[
					APSHud::Card(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							APSHud::Label(LOCTEXT("CardTitle", "SPACE TRIPS  ·  ROUTES"),
								TAttribute<FSlateColor>::CreateLambda([]() { return FSlateColor(P().TextQuiet); }))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							State.List.ToSharedRef()
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CardKeys", "UP / DOWN choose    ENTER set out    O close    P photo"))
							.Font(APSHud::LabelFont(9)).ColorAndOpacity(FSlateColor(P().TextQuiet))
						],
						APSHud::EEdge::Left)
				]
			];
		GEngine->GameViewport->AddViewportWidgetContent(State.Widget.ToSharedRef(), 40);
	}

	void Open(FState& State)
	{
		UWorld* World = State.World.Get();
		APSSpaceTrips::FTrip* Trip = APSSpaceTrips::Find(World, true);
		if (!Trip)
		{
			return;
		}
		State.Routes.Reset();
		Trip->BuildRoutes(State.Routes);
		const FString Running = Trip->GetRoute().Id;
		const int32 RunningIndex = State.Routes.IndexOfByPredicate([&Running](const APSSpaceTrips::FRoute& Route) { return Route.Id == Running; });
		State.Selected = State.Routes.IsValidIndex(RunningIndex) ? RunningIndex : 0;
		EnsureWidget(State);
		Fill(State);
		State.bOpen = true;
		UE_LOG(LogTemp, Log, TEXT("[APS.Trip] route card open: %d routes"), State.Routes.Num());
	}

	void Close(FState& State)
	{
		State.bOpen = false;
	}

	void Move(FState& State, const int32 Step)
	{
		if (!State.bOpen || State.Routes.IsEmpty())
		{
			return;
		}
		State.Selected = (State.Selected + Step + State.Routes.Num()) % State.Routes.Num();
		Fill(State);
	}

	void Confirm(FState& State)
	{
		if (!State.bOpen || !State.Routes.IsValidIndex(State.Selected))
		{
			return;
		}
		if (APSSpaceTrips::FTrip* Trip = APSSpaceTrips::Find(State.World.Get(), true))
		{
			Trip->Start(State.Routes[State.Selected].Id);
		}
		Close(State);
	}

	void Bind(FState& State, APlayerController& Controller)
	{
		UInputComponent* Input = NewObject<UInputComponent>(&Controller, TEXT("APSTripsInput"));
		Input->RegisterComponent();
		Input->bBlockInput = false;
		const auto Add = [Input](const FKey& Key, const bool bConsume, TFunction<void(FState&)> Do)
		{
			FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
			Binding.bConsumeInput = bConsume;
			Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([Do, Input]()
			{
				if (FState* Live = Find(Input->GetWorld()))
				{
					Do(*Live);
				}
			});
			Input->KeyBindings.Add(Binding);
		};
		Add(EKeys::O, true, [](FState& Live) { Live.bOpen ? Close(Live) : Open(Live); });
		Add(EKeys::Up, false, [](FState& Live) { Move(Live, -1); });
		Add(EKeys::Down, false, [](FState& Live) { Move(Live, 1); });
		Add(EKeys::Enter, false, [](FState& Live) { Confirm(Live); });
		Add(EKeys::P, true, [](FState& Live)
		{
			if (APSSpaceTrips::FTrip* Trip = APSSpaceTrips::Find(Live.World.Get(), true))
			{
				Trip->Photo(FString());
			}
		});
		Controller.PushInputComponent(Input);
		State.Controller = &Controller;
		State.Input = Input;
		UE_LOG(LogTemp, Log, TEXT("[APS.Trip] keys bound: O routes, P photo"));
	}
}

void APSSpaceTripsHud::Tick(UWorld* World)
{
	using namespace APSSpaceTripsHudLocal;
	if (!World || CVarKeys.GetValueOnGameThread() == 0)
	{
		return;
	}
	APlayerController* Controller = World->GetFirstPlayerController();
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}
	FState& State = StateFor(World);
	if (State.Controller.Get() != Controller || !State.Input.IsValid())
	{
		Bind(State, *Controller);
	}
}

void APSSpaceTripsHud::CloseCard(UWorld* World)
{
	if (APSSpaceTripsHudLocal::FState* State = APSSpaceTripsHudLocal::Find(World))
	{
		APSSpaceTripsHudLocal::Close(*State);
	}
}

bool APSSpaceTripsHud::IsCardOpen(const UWorld* World)
{
	const APSSpaceTripsHudLocal::FState* State = APSSpaceTripsHudLocal::Find(World);
	return State && State->bOpen;
}

void APSSpaceTripsHud::OpenCard(UWorld* World)
{
	using namespace APSSpaceTripsHudLocal;
	if (World && CVarKeys.GetValueOnGameThread() != 0)
	{
		Open(StateFor(World));
	}
}

namespace APSSpaceTripsHudConsole
{
	// Offscreen runs (no keys there): the card and its choice by console.
	FAutoConsoleCommandWithWorldAndArgs CardCommand(TEXT("aps.Trip.Card"),
		TEXT("Test: the route card: aps.Trip.Card open | close | up | down | go."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			using namespace APSSpaceTripsHudLocal;
			if (!World || Args.IsEmpty())
			{
				return;
			}
			FState& State = StateFor(World);
			const FString& Verb = Args[0];
			if (Verb.Equals(TEXT("open"), ESearchCase::IgnoreCase)) Open(State);
			else if (Verb.Equals(TEXT("close"), ESearchCase::IgnoreCase)) Close(State);
			else if (Verb.Equals(TEXT("up"), ESearchCase::IgnoreCase)) Move(State, -1);
			else if (Verb.Equals(TEXT("down"), ESearchCase::IgnoreCase)) Move(State, 1);
			else if (Verb.Equals(TEXT("go"), ESearchCase::IgnoreCase)) Confirm(State);
			UE_LOG(LogTemp, Log, TEXT("[APS.Trip] card %s: open=%d selected=%d of %d"), *Verb, State.bOpen ? 1 : 0, State.Selected,
				State.Routes.Num());
		}));
}

#undef LOCTEXT_NAMESPACE
