#include "SAPSObjectPage.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Widgets/Layout/SGridPanel.h"

#include "SAPSSurfaceMap.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSObjectPage"

namespace APSObjectPagePrivate
{
	using namespace APSChrome;

	const FSlateBrush* ChipBrush()
	{
		// The terminal's metric tile: a dark inset with a quiet rim.
		// Rio 06.10: the shared tile follows the interface theme.
		return APSChrome::MetricTileBrush();
	}

	const FSlateBrush* RoundBrush()
	{
		// The default half-height rounding makes a circle of a square box.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	/**
	 * Rio 06.10 (audit: a clicked object page button kept keyboard focus): aps.UI.TerminalButtonsNoFocus is registered with
	 * the terminal's buttons (SAPSColonyTerminal.cpp) and looked up by name here; when it is not found or 0 the buttons stay
	 * focusable (the previous path).
	 */
	bool TerminalButtonsNoFocus()
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.UI.TerminalButtonsNoFocus"));
		return Variable && Variable->GetInt() != 0;
	}

	/** A part's title: small and bold in its colour, a hairline under it. */
	TSharedRef<SWidget> PartTitle(const FText& Title, const FLinearColor& Colour)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Title).Font(Font("Bold", 10)).ColorAndOpacity(Colour)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 8.0f)
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
				]
			];
	}

	/** A body with its designation in front ("A3  NAME"), so the lists read in the system's order. */
	FText BodyLabel(const AActor* Actor)
	{
		const FString Designation = APSBodyDesignation::Of(Actor);
		const FText Name = APSObjectActions::NameOf(Actor);
		return Designation.IsEmpty() ? Name : FText::FromString(Designation + TEXT("  ") + Name.ToString());
	}

	/** The palette the system map draws worlds with (SAPSCivilizationMap), by type. */
	FLinearColor BodyColour(const APlanetaryBody* Body)
	{
		if (Body->IsA<AMoon>())
		{
			return FLinearColor(0.70f, 0.76f, 0.82f);
		}
		switch (Body->PlanetType)
		{
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Nordic:
		case EPlanetType::Tundra:
		case EPlanetType::IceGiant:
		case EPlanetType::Ammonia:
			return FLinearColor(0.72f, 0.88f, 1.0f);
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return FLinearColor(0.22f, 0.6f, 1.0f);
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::SuperEarth:
		case EPlanetType::Pangea:
		case EPlanetType::Savanna:
		case EPlanetType::Oasis:
			return FLinearColor(0.38f, 0.86f, 0.5f);
		case EPlanetType::Desert:
		case EPlanetType::Sand:
			return FLinearColor(0.92f, 0.76f, 0.46f);
		case EPlanetType::Volcanic:
		case EPlanetType::Lava:
		case EPlanetType::Melted:
		case EPlanetType::Sulfur:
			return FLinearColor(1.0f, 0.42f, 0.22f);
		case EPlanetType::GasGiant:
		case EPlanetType::HotGiant:
			return FLinearColor(0.95f, 0.8f, 0.6f);
		default:
			return FLinearColor(0.66f, 0.74f, 0.78f);
		}
	}
}

TSharedRef<SWidget> APSInfrastructureUI::FrameButton(TSharedRef<SWidget> Content, FOnClicked OnClicked,
	TAttribute<bool> IsSelected, const FLinearColor& Accent)
{
	using namespace APSChrome;
	// The button exists before its content, so the frame can watch its hover state.
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		// Rio 06.10 (audit: focus stayed on the clicked button): aps.UI.TerminalButtonsNoFocus, 0 = focusable as before.
		.IsFocusable(!APSObjectPagePrivate::TerminalButtonsNoFocus())
		.OnClicked(OnClicked);
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(
		SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface)
			.Brush(FAppStyle::GetBrush("WhiteBrush"))
			.Tint_Lambda([IsSelected]()
			{
				return IsSelected.Get(false) ? APSUITheme::Retint(FLinearColor(0.02f, 0.13f, 0.17f, 0.97f)) : Panel();
			})
			.ChamferTop(true)
			.ChamferBottom(true)
		]
		+ SOverlay::Slot().Padding(FMargin(14.0f, 9.0f))
		[
			Content
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame)
			.Thickness(1.0f)
			.Color_Lambda([WeakButton, IsSelected, Accent]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				return IsSelected.Get(false) || (Pinned && Pinned->IsHovered()) ? Accent : CyanDim();
			})
		]);
	return Button;
}

TSharedRef<SWidget> APSInfrastructureUI::FrameButton(const TSharedRef<STextBlock>& Label, FOnClicked OnClicked,
	TAttribute<bool> IsSelected, const FLinearColor& Accent)
{
	// As the terminal's ChromeButton: centred justification, and the capitals' middle on the box's middle.
	Label->SetJustification(ETextJustify::Center);
	if (!Label->GetRenderTransform().IsSet())
	{
		Label->SetRenderTransform(APSChrome::CapsCenterShift(Label->GetFont()));
	}
	return FrameButton(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[Label], MoveTemp(OnClicked), MoveTemp(IsSelected),
		Accent);
}

TSharedRef<SWidget> APSInfrastructureUI::FilledButton(const FText& Label, FOnClicked OnClicked, TAttribute<bool> CanClick,
	const FLinearColor& Accent)
{
	using namespace APSChrome;
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		.IsEnabled(CanClick)
		// Rio 06.10 (audit: focus stayed on the clicked button): aps.UI.TerminalButtonsNoFocus, 0 = focusable as before.
		.IsFocusable(!APSObjectPagePrivate::TerminalButtonsNoFocus())
		.OnClicked(OnClicked);
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(
		SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface)
			.Brush(FAppStyle::GetBrush("WhiteBrush"))
			.Tint_Lambda([WeakButton, CanClick]()
			{
				if (!CanClick.Get(true))
				{
					return APSUITheme::Retint(FLinearColor(0.07f, 0.13f, 0.16f, 0.95f));
				}
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				return Pinned && Pinned->IsHovered() ? APSChrome::AmberBright() : Amber();
			})
			.ChamferTop(true)
			.ChamferBottom(true)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(14.0f, 8.0f))
		[
			// Rio 06.10 ("some buttons have two lines"): one line on every button. A centred auto-wrapped label got no
			// width on its first frame and stayed broken word by word.
			SNew(STextBlock).Text(Label).Font(Font("Bold", 10)).Justification(ETextJustify::Center)
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			.RenderTransform(CapsCenterShift(Font("Bold", 10)))
			.ColorAndOpacity_Lambda([CanClick]()
			{
				return FSlateColor(CanClick.Get(true) ? APSChrome::OnAmber() : Muted());
			})
		]
		// The action's own colour on the edge: navigation blue, the fleet's gold, a department's colour.
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame)
			.Thickness(1.5f)
			.Color_Lambda([CanClick, Accent]() { return CanClick.Get(true) ? Accent : Accent.CopyWithNewOpacity(0.45f); })
		]);
	return Button;
}

TSharedRef<SWidget> APSInfrastructureUI::ActionCell(const FAPSObjectAction& Action, FOnClicked OnClicked, const bool bCanRun,
	const float Width)
{
	using namespace APSChrome;
	// What is under way is read once a frame and shared by the cell's parts.
	struct FUnderwayState
	{
		uint64 Frame{MAX_uint64};
		bool bUnderway{false};
		float Progress{0.0f};
		FText Status;
		FText Who;
	};
	const TSharedRef<FUnderwayState> State = MakeShared<FUnderwayState>();
	const TFunction<bool(float&, FText&, FText&)> Underway = Action.Underway;
	const auto Read = [State, Underway]() -> const FUnderwayState&
	{
		if (State->Frame != GFrameCounter)
		{
			State->Frame = GFrameCounter;
			State->bUnderway = Underway && Underway(State->Progress, State->Status, State->Who);
		}
		return *State;
	};
	const FText Label = Action.Label;
	const FText Detail = Action.Detail;
	const FLinearColor Accent = Action.Colour;
	const FSlateFontInfo LabelFont = Font("Bold", 10);
	// An order under way keeps the button lit (no disabled greying) but ignores clicks: the fill is the state.
	const TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(0.0f)
		// Rio 06.10 (audit: focus stayed on the clicked button): aps.UI.TerminalButtonsNoFocus, 0 = focusable as before.
		.IsFocusable(!APSObjectPagePrivate::TerminalButtonsNoFocus())
		.IsEnabled_Lambda([Read, bCanRun]() { return bCanRun || Read().bUnderway; })
		.OnClicked_Lambda([Read, OnClicked]()
		{
			return Read().bUnderway || !OnClicked.IsBound() ? FReply::Handled() : OnClicked.Execute();
		});
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(
		SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface)
			.Brush(FAppStyle::GetBrush("WhiteBrush"))
			.Tint_Lambda([WeakButton, Read, bCanRun]()
			{
				if (Read().bUnderway)
				{
					return APSUITheme::Retint(FLinearColor(0.03f, 0.08f, 0.10f, 0.97f));
				}
				if (!bCanRun)
				{
					return APSUITheme::Retint(FLinearColor(0.07f, 0.13f, 0.16f, 0.95f));
				}
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				return Pinned && Pinned->IsHovered() ? APSChrome::AmberBright() : Amber();
			})
			.ChamferTop(true)
			.ChamferBottom(true)
		]
		// The part done, in the action's colour, from the left.
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride_Lambda([Read, Width]()
			{
				return FOptionalSize(Read().bUnderway ? FMath::Max(Width * Read().Progress, 0.0f) : 0.0f);
			})
			.Visibility_Lambda([Read]() { return Read().bUnderway ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SAPSChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint(Accent.CopyWithNewOpacity(0.62f))
				.ChamferTop(true)
				.ChamferBottom(true)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(14.0f, 8.0f))
		[
			// Rio 06.10 (audit: the ellipsis never drew under centred justification, and the % and 'YOURSELF' were cut
			// mid-letter): the label takes what is left and ends in an ellipsis; the percentage has its own block, so it is
			// never cut. Short labels stay centred (the box is only as wide as its text then).
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label).Font(LabelFont).Justification(ETextJustify::Left)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.RenderTransform(CapsCenterShift(LabelFont))
				.ColorAndOpacity_Lambda([Read, bCanRun]()
				{
					return FSlateColor(Read().bUnderway ? FLinearColor(0.96f, 0.98f, 1.0f, 1.0f)
						: bCanRun ? APSChrome::OnAmber() : Muted());
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(LabelFont)
				.RenderTransform(CapsCenterShift(LabelFont))
				.Text_Lambda([Read]()
				{
					return Read().bUnderway ? FText::Format(LOCTEXT("UnderwayPercent", "{0}%"),
						APSUINumber::Number(FMath::RoundToInt(Read().Progress * 100.0f))) : FText::GetEmpty();
				})
				.ColorAndOpacity_Lambda([Read, bCanRun]()
				{
					return FSlateColor(Read().bUnderway ? FLinearColor(0.96f, 0.98f, 1.0f, 1.0f)
						: bCanRun ? APSChrome::OnAmber() : Muted());
				})
				.Visibility_Lambda([Read]() { return Read().bUnderway ? EVisibility::Visible : EVisibility::Collapsed; })
			]
		]
		// The action's own colour on the edge: navigation blue, the fleet's gold, a department's colour.
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame)
			.Thickness(1.5f)
			.Color_Lambda([Read, bCanRun, Accent]() { return Read().bUnderway || bCanRun ? Accent : Accent.CopyWithNewOpacity(0.45f); })
		]);
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Button
		]
		// A dim button says why under it; an order under way says who is at it.
		+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11))
			.Text_Lambda([Read, Detail]()
			{
				return Read().bUnderway ? FText::Format(LOCTEXT("UnderwayDetail", "{0}: {1}."), Read().Who, Read().Status) : Detail;
			})
			.ColorAndOpacity_Lambda([Read, Accent]() { return FSlateColor(Read().bUnderway ? Accent : Muted()); })
			.Visibility_Lambda([Read, bCanRun, Detail]()
			{
				return Read().bUnderway || (!bCanRun && !Detail.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
			})
		];
}

TSharedRef<SWidget> APSInfrastructureUI::Chip(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& TextColour,
	const FLinearColor& Swatch)
{
	return SNew(SBorder).BorderImage(APSObjectPagePrivate::ChipBrush()).Padding(FMargin(7.0f, 3.0f, 9.0f, 3.0f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(7.0f).HeightOverride(7.0f)
			[
				SNew(SBorder).BorderImage(APSObjectPagePrivate::RoundBrush()).BorderBackgroundColor(Swatch)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			// The readable face: chips carry numbers and units, small.
			SNew(STextBlock).Text(Text).Font(APSUITheme::BodyFont("Bold", 10)).ColorAndOpacity(TextColour)
		]
	];
}

EAPSChromeGlyph APSInfrastructureUI::CategoryGlyph(const APSInfrastructure::ECategory Category)
{
	using APSInfrastructure::ECategory;
	switch (Category)
	{
	case ECategory::Station: return EAPSChromeGlyph::Station;
	case ECategory::Relay: return EAPSChromeGlyph::Compass;
	case ECategory::Transport: return EAPSChromeGlyph::Fleet;
	case ECategory::Megastructure: return EAPSChromeGlyph::System;
	// Rio 03.10: the huge hubs read as headquarters-class stations.
	case ECategory::Hub: return EAPSChromeGlyph::Headquarters;
	default: return EAPSChromeGlyph::Infrastructure;
	}
}

EAPSChromeGlyph APSInfrastructureUI::GlyphOf(const AActor* Object)
{
	if (!Object)
	{
		return EAPSChromeGlyph::Infrastructure;
	}
	if (FGuid SystemId; Object->IsA<AStar>() || FAPSStarSystems::AnchorSystem(Object, SystemId))
	{
		return EAPSChromeGlyph::System;
	}
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(Object->GetWorld()))
	{
		if (const FAPSBuiltStructure* Built = Infrastructure->FindByActor(Object))
		{
			if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type))
			{
				return CategoryGlyph(Type->Category);
			}
		}
	}
	if (Object->IsA<APlanetaryBody>()) return EAPSChromeGlyph::Planet;
	if (Object->IsA<ASpaceShipyard>()) return EAPSChromeGlyph::Shipyard;
	if (Object->IsA<ASpaceHeadquarters>()) return EAPSChromeGlyph::Headquarters;
	if (Object->IsA<ASpaceStation>()) return EAPSChromeGlyph::Station;
	if (Object->IsA<ASpaceship>()) return EAPSChromeGlyph::Ship;
	if (Object->IsA<AColony>()) return EAPSChromeGlyph::Headquarters;
	return EAPSChromeGlyph::Infrastructure;
}

FLinearColor APSInfrastructureUI::ColourOf(const AActor* Object)
{
	using namespace APSChrome;
	if (!Object)
	{
		return Muted();
	}
	const UWorld* ObjectWorld = Object->GetWorld();
	if (FGuid SystemId; FAPSStarSystems::AnchorSystem(Object, SystemId))
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(ObjectWorld);
		const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
		return Info ? Info->Colour : Amber();
	}
	if (Object->IsA<AStar>())
	{
		return FLinearColor(1.0f, 0.8f, 0.36f);
	}
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(ObjectWorld))
	{
		if (const FAPSBuiltStructure* Built = Infrastructure->FindByActor(Object))
		{
			if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type))
			{
				return APSInfrastructure::DepartmentColour(Type->Department);
			}
		}
	}
	if (const ASpaceship* Ship = Cast<ASpaceship>(Object))
	{
		const FAPSFleetCommand* Fleet = APSFleetFind(ObjectWorld);
		const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
		return Unit ? APSFleet::DivisionColour(Unit->Division) : White();
	}
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Object))
	{
		return APSObjectPagePrivate::BodyColour(Body);
	}
	if (Object->IsA<AColony>()) return FLinearColor(0.36f, 1.0f, 0.58f);
	if (Object->IsA<AAutonomousOutpost>()) return Amber();
	return Cyan();
}

AActor* APSInfrastructureUI::HomeStar(const UWorld* InWorld)
{
	if (!InWorld)
	{
		return nullptr;
	}
	for (TActorIterator<AAstroGenerator> It(InWorld); It; ++It)
	{
		if (IsValid(*It) && IsValid(It->HomeStar))
		{
			return It->HomeStar;
		}
	}
	return nullptr;
}

APlanetaryBody* APSInfrastructureUI::BodyOf(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (APlanetaryBody* Orbited = FAPSFleetCommand::OrbitedBody(Actor))
	{
		return Orbited;
	}
	// Not attached (a colony set on the ground): the world whose surface it stands on.
	APlanetaryBody* Best = nullptr;
	double BestGap = TNumericLimits<double>::Max();
	if (const UWorld* ActorWorld = Actor->GetWorld())
	{
		for (TActorIterator<APlanetaryBody> It(ActorWorld); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			const double Radius = It->GetWorldScapeBodyRadiusCm();
			const double Gap = FMath::Abs(FVector::Dist(It->GetActorLocation(), Actor->GetActorLocation()) - Radius);
			if (Gap < FMath::Max(Radius * 0.05, 200000.0) && Gap < BestGap)
			{
				BestGap = Gap;
				Best = *It;
			}
		}
	}
	return Best;
}

void SAPSObjectPage::Construct(const FArguments& InArgs)
{
	using namespace APSObjectPagePrivate;
	using namespace APSInfrastructureUI;
	World = InArgs._World;
	ExtraActions = InArgs._ExtraActions;
	OnBack = InArgs._OnBack;
	BackLabel = InArgs._BackLabel;
	OnOpenObject = InArgs._OnOpenObject;
	// Worlds and star systems are places: they list what stands there.
	const auto IsPlace = [this]()
	{
		const AActor* Actor = PageActor.Get();
		FGuid SystemId;
		return Actor && (Actor->IsA<APlanetaryBody>() || FAPSInfrastructure::SiteSystem(Actor->GetWorld(), Actor, SystemId));
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		// BACK, and where the page sits.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				FrameButton(
					SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
					.Text_Lambda([this]()
					{
						return FText::Format(LOCTEXT("Back", "<  BACK TO {0}"), BackLabel.Get(LOCTEXT("BackInfrastructure", "INFRASTRUCTURE")));
					}),
					FOnClicked::CreateLambda([this]()
					{
						OnBack.ExecuteIfBound();
						return FReply::Handled();
					}),
					TAttribute<bool>(false), Cyan())
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("Crumb", "INFRASTRUCTURE  /  OBJECT PAGE")).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SHorizontalBox)
				// What it is: the kind and name, the status lines, what stands here and what it belongs to, the fleet there.
				+ SHorizontalBox::Slot().FillWidth(0.46f).Padding(0.0f, 0.0f, 24.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Font("Bold", 11)).AutoWrapText(true)
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(ColourOf(PageActor.Get())); })
						.Text_Lambda([this]()
						{
							const AActor* Actor = PageActor.Get();
							if (!Actor)
							{
								return LOCTEXT("Gone", "THE OBJECT IS GONE");
							}
							const FString Designation = APSBodyDesignation::Of(Actor);
							return Designation.IsEmpty() ? APSObjectActions::KindOf(Actor)
								: FText::Format(LOCTEXT("KindDesignation", "{0}  /  {1}"), APSObjectActions::KindOf(Actor),
									FText::FromString(Designation));
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(Font("Bold", 22)).ColorAndOpacity(White()).AutoWrapText(true)
						.Text_Lambda([this]()
						{
							const AActor* Actor = PageActor.Get();
							return Actor ? APSObjectActions::NameOf(Actor) : FText::FromString(TEXT("-"));
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
					[
						PartTitle(LOCTEXT("Status", "STATUS"), Cyan())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(FieldsBox, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)
						.Visibility_Lambda([IsPlace]() { return IsPlace() ? EVisibility::Visible : EVisibility::Collapsed; })
						+ SVerticalBox::Slot().AutoHeight()
						[
							PartTitle(LOCTEXT("StandsHere", "STANDS HERE"), Success())
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(HereBox, SVerticalBox)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						PartTitle(LOCTEXT("Linked", "BELONGS TO  /  AROUND IT"), Cyan())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(LinkedBox, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						PartTitle(LOCTEXT("UnderWay", "UNDER WAY HERE"), Amber())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(UnderWayBox, SVerticalBox)
					]
				]
				// What can be done: the preview at the side, then every action by group and what the last one answered.
				+ SHorizontalBox::Slot().FillWidth(0.54f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBox).WidthOverride(240.0f).HeightOverride(240.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SAssignNew(Globe, SAPSSurfaceMap).World(World).GlobeOnly(true)
								.Visibility_Lambda([this]() { return IsGlobeShown() ? EVisibility::Visible : EVisibility::Collapsed; })
							]
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SAssignNew(GlyphBox, SBox).WidthOverride(170.0f).HeightOverride(170.0f)
								.Visibility_Lambda([this]() { return IsGlobeShown() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Muted()).Justification(ETextJustify::Center)
						.Text_Lambda([this]()
						{
							const APlanetaryBody* Body = Cast<APlanetaryBody>(PageActor.Get());
							if (!Body)
							{
								return FText::GetEmpty();
							}
							if (!Globe.IsValid() || !Globe->HasSurface())
							{
								return LOCTEXT("CaptionGas", "A GAS WORLD: NO SOLID SURFACE TO SHOW");
							}
							const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
							const APSFleet::ESurvey Survey = Fleet ? Fleet->GetSurvey(Body) : APSFleet::ESurvey::Studied;
							return Survey == APSFleet::ESurvey::Unknown ? LOCTEXT("CaptionUnknown", "NOT SURVEYED: A SCANNER'S NOISE ONLY")
								: Survey == APSFleet::ESurvey::Surveyed ? LOCTEXT("CaptionSurveyed", "SURVEYED: COARSE  /  DRAG TO TURN")
								: LOCTEXT("CaptionStudied", "STUDIED  /  DRAG TO TURN");
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						PartTitle(LOCTEXT("Actions", "WHAT CAN BE DONE HERE"), Amber())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
						.Text_Lambda([this]() { return Message; })
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(MessageColour); })
						.Visibility_Lambda([this]() { return Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(ActionsBox, SVerticalBox)
					]
				]
			]
		]
	];
}

void SAPSObjectPage::SetPageObject(AActor* Object)
{
	PageActor = Object;
	Message = FText::GetEmpty();
	// Never a signature of a read part: everything is built again.
	FieldsSignature = RelatedSignature = UnderWaySignature = ActionsSignature = TEXT("-");
	UpdatePreview();
	Refresh(true);
}

void SAPSObjectPage::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// A page shown again after a while (another tab, another section) is read at once.
	const bool bResumed = InCurrentTime - LastTickSeconds > 0.5;
	LastTickSeconds = InCurrentTime;
	if (bResumed)
	{
		Refresh(true);
		return;
	}
	FastClock += InDeltaTime;
	SlowClock += InDeltaTime;
	if (FastClock >= 0.5f)
	{
		FastClock = 0.0f;
		RefreshFields();
		RefreshUnderWay();
	}
	if (SlowClock >= 1.0f)
	{
		SlowClock = 0.0f;
		RefreshRelated();
		RefreshActions();
		if (IsGlobeShown())
		{
			Globe->RefreshMarkers();
		}
	}
}

void SAPSObjectPage::Refresh(const bool bForce)
{
	if (bForce)
	{
		FastClock = 0.0f;
		SlowClock = 0.0f;
	}
	RefreshFields();
	RefreshUnderWay();
	RefreshRelated();
	RefreshActions();
}

bool SAPSObjectPage::IsGlobeShown() const
{
	const APlanetaryBody* Body = Cast<APlanetaryBody>(PageActor.Get());
	return Body && Globe.IsValid() && Globe->GetBody() == Body && Globe->HasSurface();
}

void SAPSObjectPage::UpdatePreview()
{
	using namespace APSObjectPagePrivate;
	AActor* Actor = PageActor.Get();
	APlanetaryBody* Body = Cast<APlanetaryBody>(Actor);
	if (Globe.IsValid() && Globe->GetBody() != Body)
	{
		Globe->SetBody(Body);
	}
	if (!GlyphBox.IsValid())
	{
		return;
	}
	// Everything without a globe (a gas world, a star system, a station, a ship): its glyph large in its colour.
	const FLinearColor Colour = APSInfrastructureUI::ColourOf(Actor);
	// Rio 02.10 ("instead of this icon, the real look of our building, a snapshot like the menu's icons"): a station, a
	// headquarters, a shipyard or a ship with a baked thumbnail shows it.
	if (const FSlateBrush* Snapshot = Actor ? APSUIThumbnails::FindBrush(Actor->GetClass()) : nullptr)
	{
		GlyphBox->SetWidthOverride(236.0f);
		GlyphBox->SetHeightOverride(236.0f);
		GlyphBox->SetContent(
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(RoundBrush()).BorderBackgroundColor(Colour.CopyWithNewOpacity(0.07f))
			]
			// A thumbnail may be wide (a long hull baked with -Wide): fit it, never stretch it.
			+ SOverlay::Slot()
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage).Image(Snapshot)
				]
			]);
		return;
	}
	GlyphBox->SetWidthOverride(170.0f);
	GlyphBox->SetHeightOverride(170.0f);
	GlyphBox->SetContent(
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(RoundBrush()).BorderBackgroundColor(Colour.CopyWithNewOpacity(0.10f))
		]
		+ SOverlay::Slot().Padding(28.0f)
		[
			SNew(SAPSVectorGlyph).Glyph(APSInfrastructureUI::GlyphOf(Actor)).Color(Colour).StrokeWidth(3.0f)
		]);
}

void SAPSObjectPage::RefreshFields()
{
	using namespace APSObjectPagePrivate;
	if (!FieldsBox.IsValid())
	{
		return;
	}
	Fields.Reset();
	if (const AActor* Actor = PageActor.Get())
	{
		APSObjectActions::Describe(World.Get(), Actor, Fields);
	}
	// The rows stay while their labels do; the values are read by index.
	FString Signature = TEXT("F:");
	for (const TPair<FText, FText>& Field : Fields)
	{
		Signature += Field.Key.ToString() + TEXT("|");
	}
	if (Signature == FieldsSignature)
	{
		return;
	}
	FieldsSignature = Signature;
	FieldsBox->ClearChildren();
	if (Fields.IsEmpty())
	{
		FieldsBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoFields", "Nothing more is known of it.")).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	// Rio 06.10 ("use the space, not always down in a column"): the label over its value, three to a row.
	constexpr int32 Columns = 3;
	const TSharedRef<SGridPanel> Grid = SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f).FillColumn(2, 1.0f);
	for (int32 Index = 0; Index < Fields.Num(); ++Index)
	{
		Grid->AddSlot(Index % Columns, Index / Columns).Padding(FMargin(0.0f, 0.0f, 16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Fields[Index].Key).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 12)).ColorAndOpacity(White())
				.Text_Lambda([this, Index]() { return Fields.IsValidIndex(Index) ? Fields[Index].Value : FText::GetEmpty(); })
			]
		];
	}
	FieldsBox->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[Grid];
}

void SAPSObjectPage::CollectHere(const AActor* Object, TArray<FRelated>& OutRows) const
{
	UWorld* LiveWorld = World.Get();
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	const auto Add = [&OutRows](AActor* Actor, const FText& Detail)
	{
		if (!Actor || OutRows.ContainsByPredicate([Actor](const FRelated& Row) { return Row.Actor.Get() == Actor; }))
		{
			return;
		}
		FRelated& Row = OutRows.AddDefaulted_GetRef();
		Row.Actor = Actor;
		Row.Name = APSObjectActions::NameOf(Actor);
		Row.Detail = Detail;
		Row.Glyph = APSInfrastructureUI::GlyphOf(Actor);
		Row.Colour = APSInfrastructureUI::ColourOf(Actor);
	};
	// The catalogue's structures at this place: a world's, or a star system's around its star.
	if (Infrastructure)
	{
		TArray<const FAPSBuiltStructure*> Here;
		Infrastructure->GetAt(Object, Here);
		for (const FAPSBuiltStructure* Built : Here)
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type);
			Add(Built->Actor.Get(), Type ? FText::Format(LOCTEXT("HereBuilt", "{0}  /  {1}"),
				APSInfrastructure::CategoryName(Type->Category), APSInfrastructure::DepartmentName(Type->Department)) : FText::GetEmpty());
		}
	}
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Object);
	if (!Body)
	{
		return;
	}
	// The fleet's stations, shipyards and HQs in orbit, its outposts and the anomaly's beacon.
	if (Fleet)
	{
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			if (Structure.Body.Get() == Body)
			{
				Add(Structure.Actor.Get(), FText::Format(LOCTEXT("HereFleetStructure", "{0}  /  {1}"), APSFleet::StructureName(Structure.Kind),
					Structure.bBuilt ? LOCTEXT("HereBuiltByFleet", "BUILT BY THE FLEET") : LOCTEXT("HereFounded", "FOUNDED WITH THE CIVILIZATION")));
			}
		}
		if (const FAPSFleetBodyRecord* Record = Fleet->FindBody(Body))
		{
			for (const TWeakObjectPtr<AActor>& Outpost : Record->Outposts)
			{
				Add(Outpost.Get(), LOCTEXT("HereOutpost", "OUTPOST  /  BUILT BY THE FLEET"));
			}
			if (Record->bHasAnomaly)
			{
				Add(Record->AnomalyBeacon.Get(), FText::Format(LOCTEXT("HereAnomaly", "ANOMALY SITE  /  {0}"), APSFleet::AnomalyName(Record->AnomalyKind)));
			}
		}
	}
	// The colony and the settlements on the ground.
	if (LiveWorld)
	{
		for (TActorIterator<AColony> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It) && APSInfrastructureUI::BodyOf(*It) == Body)
			{
				Add(*It, It->ActorHasTag(TEXT("APS.Civilization.Materialized")) ? LOCTEXT("HereColony", "HOME COLONY")
					: LOCTEXT("HereSettlement", "SETTLEMENT"));
			}
		}
	}
}

void SAPSObjectPage::CollectLinked(const AActor* Object, TArray<FRelated>& OutRows) const
{
	using namespace APSObjectPagePrivate;
	UWorld* LiveWorld = World.Get();
	const auto Add = [&OutRows](AActor* Actor, const FText& Detail)
	{
		if (!Actor || OutRows.ContainsByPredicate([Actor](const FRelated& Row) { return Row.Actor.Get() == Actor; }))
		{
			return;
		}
		FRelated& Row = OutRows.AddDefaulted_GetRef();
		Row.Actor = Actor;
		Row.Name = Actor->IsA<APlanetaryBody>() || Actor->IsA<AStar>() ? BodyLabel(Actor) : APSObjectActions::NameOf(Actor);
		Row.Detail = Detail;
		Row.Glyph = APSInfrastructureUI::GlyphOf(Actor);
		Row.Colour = APSInfrastructureUI::ColourOf(Actor);
	};
	if (const AMoon* Moon = Cast<AMoon>(Object))
	{
		Add(Moon->ParentPlanet, LOCTEXT("LinkedPlanet", "ITS PLANET"));
	}
	else if (const APlanet* Planet = Cast<APlanet>(Object))
	{
		Add(Planet->ParentStar, LOCTEXT("LinkedStar", "ITS STAR: THE STAR SYSTEM"));
		if (LiveWorld)
		{
			for (TActorIterator<AMoon> It(LiveWorld); It; ++It)
			{
				if (IsValid(*It) && It->ParentPlanet == Planet)
				{
					Add(*It, LOCTEXT("LinkedMoon", "MOON"));
				}
			}
		}
	}
	else if (const AStar* Star = Cast<AStar>(Object))
	{
		if (LiveWorld)
		{
			for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
			{
				if (IsValid(*It) && It->ParentStar == Star)
				{
					Add(*It, LOCTEXT("LinkedWorld", "PLANET OF THE SYSTEM"));
				}
			}
		}
	}
	else if (const ASpaceship* Ship = Cast<ASpaceship>(Object))
	{
		const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
		if (const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr)
		{
			Add(Unit->Target.Get(), FText::Format(LOCTEXT("LinkedTarget", "ORDER  /  {0}"), APSFleet::OrderName(Unit->Order)));
			Add(Unit->Berth.Get(), LOCTEXT("LinkedBerth", "ITS BERTH"));
		}
	}
	else if (Object)
	{
		// A structure, a colony or a beacon: the world or the star system it stands at.
		AActor* Site = APSInfrastructureUI::BodyOf(Object);
		if (!Site)
		{
			Site = Object->GetAttachParentActor();
		}
		Add(Site, LOCTEXT("LinkedSite", "WHERE IT STANDS"));
	}
	OutRows.Sort([](const FRelated& A, const FRelated& B) { return A.Name.ToString() < B.Name.ToString(); });
}

void SAPSObjectPage::FillRows(const TSharedPtr<SVerticalBox>& Box, const TArray<FRelated>& Rows, const FText& Empty)
{
	using namespace APSObjectPagePrivate;
	if (!Box.IsValid())
	{
		return;
	}
	Box->ClearChildren();
	if (Rows.IsEmpty())
	{
		Box->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(Empty).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	for (const FRelated& Row : Rows)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			APSInfrastructureUI::FrameButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(Row.Glyph, Row.Colour, 28.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Row.Name).Font(Font("Bold", 11)).ColorAndOpacity(White())
						.RenderTransform(APSChrome::CapsCenterShift(Font("Bold", 11)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Row.Detail).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("OpenRow", "OPEN")).Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
				],
				FOnClicked::CreateSP(this, &SAPSObjectPage::OpenRelated, Row.Actor), TAttribute<bool>(false), Row.Colour)
		];
	}
}

void SAPSObjectPage::RefreshRelated()
{
	TArray<FRelated> Here;
	TArray<FRelated> Linked;
	if (const AActor* Actor = PageActor.Get())
	{
		CollectHere(Actor, Here);
		CollectLinked(Actor, Linked);
	}
	FString Signature = TEXT("R:");
	for (const TArray<FRelated>* Rows : {&Here, &Linked})
	{
		for (const FRelated& Row : *Rows)
		{
			Signature += Row.Name.ToString() + TEXT("~") + Row.Detail.ToString() + TEXT("~") + GetNameSafe(Row.Actor.Get()) + TEXT(";");
		}
		Signature += TEXT("|");
	}
	if (Signature == RelatedSignature)
	{
		return;
	}
	RelatedSignature = Signature;
	FillRows(HereBox, Here, LOCTEXT("HereNone", "Nothing of the civilization stands here yet: build from the actions."));
	FillRows(LinkedBox, Linked, LOCTEXT("LinkedNone", "Not linked to anything else on the maps."));
}

void SAPSObjectPage::RefreshUnderWay()
{
	using namespace APSObjectPagePrivate;
	if (!UnderWayBox.IsValid())
	{
		return;
	}
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	const AActor* Actor = PageActor.Get();
	TArray<TWeakObjectPtr<ASpaceship>> Ships;
	FString Signature = TEXT("U:");
	if (Fleet && Actor)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (Unit.Ship.IsValid() && Unit.Order != APSFleet::EOrder::None && Unit.Target.Get() == Actor)
			{
				Ships.Add(Unit.Ship);
				Signature += Unit.CallSign + TEXT(";");
			}
		}
	}
	if (Signature == UnderWaySignature)
	{
		return;
	}
	UnderWaySignature = Signature;
	UnderWayBox->ClearChildren();
	if (Ships.IsEmpty())
	{
		UnderWayBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("UnderWayNone", "No ship of the fleet is on its way here or at work here."))
			.AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	for (const TWeakObjectPtr<ASpaceship>& Ship : Ships)
	{
		// Read live: the state line follows the order's phase and progress.
		const auto Unit = [this, Ship]() -> const FAPSFleetUnit*
		{
			const FAPSFleetCommand* Live = APSFleetFind(World.Get());
			return Live ? Live->FindUnit(Ship.Get()) : nullptr;
		};
		const auto DivisionColour = [Unit]()
		{
			const FAPSFleetUnit* Found = Unit();
			return Found ? APSFleet::DivisionColour(Found->Division) : Muted();
		};
		UnderWayBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			APSInfrastructureUI::FrameButton(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(4.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([DivisionColour]() { return FSlateColor(DivisionColour()); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(White())
						.RenderTransform(APSChrome::CapsCenterShift(Font("Bold", 11)))
						.Text_Lambda([Unit]()
						{
							const FAPSFleetUnit* Found = Unit();
							return Found ? FText::Format(LOCTEXT("UnderWayUnit", "{0}  /  {1}"), FText::FromString(Found->CallSign),
								APSFleet::DivisionName(Found->Division)) : LOCTEXT("UnderWayGone", "GONE");
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 9)).ColorAndOpacity(Amber())
						.Text_Lambda([this, Unit]()
						{
							const FAPSFleetUnit* Found = Unit();
							const FAPSFleetCommand* Live = APSFleetFind(World.Get());
							if (!Found || !Live)
							{
								return FText::GetEmpty();
							}
							// What a construction order raises, by the catalogue's name.
							const APSInfrastructure::FType* Type = Found->Order == APSFleet::EOrder::BuildStructure
								? APSInfrastructure::Find(Found->StructureType) : nullptr;
							return Type ? FText::Format(LOCTEXT("UnderWayBuild", "{0}  /  {1}"), Live->DescribeState(*Found), Type->Name)
								: Live->DescribeState(*Found);
						})
					]
				],
				FOnClicked::CreateLambda([this, Ship]()
				{
					if (Ship.IsValid())
					{
						OnOpenObject.ExecuteIfBound(Ship.Get());
					}
					return FReply::Handled();
				}),
				TAttribute<bool>(false), Cyan())
		];
	}
}

void SAPSObjectPage::GatherActions(TArray<FAPSObjectAction>& OutActions) const
{
	OutActions.Reset();
	AActor* Actor = PageActor.Get();
	UWorld* LiveWorld = World.Get();
	if (!Actor || !LiveWorld)
	{
		return;
	}
	APSObjectActions::Gather(LiveWorld, Actor, OutActions);
	if (ExtraActions)
	{
		ExtraActions(Actor, OutActions);
	}
}

void SAPSObjectPage::RefreshActions()
{
	using namespace APSObjectPagePrivate;
	if (!ActionsBox.IsValid())
	{
		return;
	}
	TArray<FAPSObjectAction> Actions;
	GatherActions(Actions);
	FString Signature = TEXT("A:");
	for (const FAPSObjectAction& Action : Actions)
	{
		Signature += Action.Id.ToString() + (Action.bEnabled ? TEXT("+") : TEXT("-")) + Action.Label.ToString() + TEXT("~")
			+ Action.Detail.ToString() + TEXT(";");
	}
	if (Signature == ActionsSignature)
	{
		return;
	}
	ActionsSignature = Signature;
	ActionsBox->ClearChildren();
	if (Actions.IsEmpty())
	{
		ActionsBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoActions", "Nothing can be done with this object from here yet."))
			.AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		];
		return;
	}
	// Groups in the providers' order: navigation, the fleet, construction by department, then the screens.
	TArray<FString> Groups;
	for (const FAPSObjectAction& Action : Actions)
	{
		Groups.AddUnique(Action.Group.ToString());
	}
	for (const FString& Group : Groups)
	{
		const TSharedRef<SWrapBox> Cells = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8.0f, 8.0f));
		FLinearColor GroupColour = Cyan();
		FText GroupName;
		int32 Ready = 0;
		int32 Total = 0;
		for (const FAPSObjectAction& Action : Actions)
		{
			if (Action.Group.ToString() != Group)
			{
				continue;
			}
			if (Total == 0)
			{
				GroupColour = Action.Colour;
				GroupName = Action.Group;
			}
			++Total;
			const bool bCanRun = Action.bEnabled && static_cast<bool>(Action.Execute);
			Ready += bCanRun ? 1 : 0;
			// A dim button says why under it, one under way who is at it; every button's tooltip says what it does.
			// Rio 06.10 (audit: a long label ends in an ellipsis): the tooltip starts with the whole label.
			constexpr float CellWidth = 236.0f;
			const FText CellTip = Action.Detail.IsEmpty() ? Action.Label
				: FText::Format(LOCTEXT("ActionCellTip", "{0}\n{1}"), Action.Label, Action.Detail);
			Cells->AddSlot()
			[
				SNew(SBox).WidthOverride(CellWidth).ToolTipText(CellTip)
				[
					APSInfrastructureUI::ActionCell(Action, FOnClicked::CreateSP(this, &SAPSObjectPage::RunAction, Action.Id), bCanRun,
						CellWidth)
				]
			];
		}
		ActionsBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
				[
					SNew(SBorder).BorderImage(RoundBrush()).BorderBackgroundColor(GroupColour)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(GroupName.IsEmpty() ? LOCTEXT("OtherGroup", "OTHER") : GroupName)
				.Font(Font("Bold", 10)).ColorAndOpacity(GroupColour)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Ready > 0 ? Success() : Muted())
				.Text(FText::Format(LOCTEXT("GroupReady", "{0} OF {1} POSSIBLE NOW"), APSUINumber::Number(Ready), APSUINumber::Number(Total)))
			]
		];
		ActionsBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			Cells
		];
	}
}

FReply SAPSObjectPage::RunAction(const FName ActionId)
{
	using namespace APSChrome;
	// Gathered again: the ship a fleet action sends is picked now, by the same rules as every other screen.
	TArray<FAPSObjectAction> Actions;
	GatherActions(Actions);
	const FAPSObjectAction* Action = Actions.FindByPredicate([ActionId](const FAPSObjectAction& Each) { return Each.Id == ActionId; });
	if (!Action)
	{
		Message = LOCTEXT("ActionGone", "This action is not offered here any more.");
		MessageColour = Amber();
	}
	else if (!Action->bEnabled || !Action->Execute)
	{
		Message = Action->Detail.IsEmpty() ? LOCTEXT("ActionUnavailable", "Not possible now.") : Action->Detail;
		MessageColour = Amber();
	}
	else
	{
		// An answer that changed the fleet or the infrastructure reads as done; any other reads neutral.
		const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
		const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
		const uint32 FleetBefore = Fleet ? Fleet->GetRevision() : 0;
		const uint32 InfrastructureBefore = Infrastructure ? Infrastructure->GetRevision() : 0;
		const FText Answer = Action->Execute();
		const bool bChanged = (Fleet && Fleet->GetRevision() != FleetBefore)
			|| (Infrastructure && Infrastructure->GetRevision() != InfrastructureBefore);
		Message = Answer.IsEmpty() ? FText::Format(LOCTEXT("ActionDone", "{0}: done."), Action->Label) : Answer;
		MessageColour = bChanged ? Success() : Cyan();
	}
	// What the action changed is read on the next frame.
	FastClock = 10.0f;
	SlowClock = 10.0f;
	return FReply::Handled();
}

FReply SAPSObjectPage::OpenRelated(const TWeakObjectPtr<AActor> Actor)
{
	if (Actor.IsValid())
	{
		OnOpenObject.ExecuteIfBound(Actor.Get());
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
