#include "SAPSStarMapPanel.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APSStarMapModel.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Colony/SAPSObjectPage.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSStarMapPanel"

namespace APSStarMapPanelPrivate
{
	using namespace APSChrome;

	constexpr double TwoPi = 6.28318530717958647692;

	FLinearColor TealColour() { return FLinearColor(0.35f, 0.95f, 0.95f, 1.0f); }
	FLinearColor RowColour() { return APSUITheme::Retint(FLinearColor(0.0f, 0.016f, 0.026f, 0.6f)); }
	FLinearColor PickedRowColour() { return APSUITheme::Retint(FLinearColor(0.02f, 0.13f, 0.17f, 0.97f)); }

	/** A star's dot with its knowledge ring, as the map draws it (the ring dashed while only catalogued); also the LEGEND dot. */
	class SAPSStarDot final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAPSStarDot)
			: _DotRadius(3.4f)
			, _RingRadius(6.4f)
		{}
			SLATE_ATTRIBUTE(FLinearColor, DotColour)
			SLATE_ATTRIBUTE(FLinearColor, RingColour)
			SLATE_ATTRIBUTE(bool, Dashed)
			SLATE_ARGUMENT(float, DotRadius)
			SLATE_ARGUMENT(float, RingRadius)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			DotColour = InArgs._DotColour;
			RingColour = InArgs._RingColour;
			Dashed = InArgs._Dashed;
			DotRadius = InArgs._DotRadius;
			RingRadius = InArgs._RingRadius;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(2.0 * RingRadius + 4.0, 2.0 * RingRadius + 4.0);
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out,
			const int32 LayerId, const FWidgetStyle&, const bool) const override
		{
			static const FSlateRoundedBoxBrush Disc(FLinearColor::White);
			const FVector2D Middle = FVector2D(Geometry.GetLocalSize()) * 0.5;
			const FLinearColor Fill = DotColour.Get(FLinearColor::Transparent);
			if (Fill.A > 0.0f)
			{
				FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2f(2.0f * DotRadius, 2.0f * DotRadius),
					FSlateLayoutTransform(FVector2f(static_cast<float>(Middle.X) - DotRadius, static_cast<float>(Middle.Y) - DotRadius))),
					&Disc, ESlateDrawEffect::None, Fill);
			}
			const FLinearColor Edge = RingColour.Get(FLinearColor::Transparent);
			if (Edge.A > 0.0f)
			{
				const bool bDashed = Dashed.Get(false);
				const int32 Pieces = bDashed ? 10 : 1;
				for (int32 Piece = 0; Piece < Pieces; ++Piece)
				{
					const double From = TwoPi * Piece / Pieces;
					const double Span = bDashed ? TwoPi * 0.55 / Pieces : TwoPi;
					TArray<FVector2D> Points;
					constexpr int32 Steps = 6;
					const int32 Count = bDashed ? Steps : Steps * 6;
					for (int32 Step = 0; Step <= Count; ++Step)
					{
						const double Angle = From + Span * Step / Count;
						Points.Add(Middle + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RingRadius);
					}
					FSlateDrawElement::MakeLines(Out, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Edge, true, 1.2f);
				}
			}
			return LayerId;
		}

	private:
		TAttribute<FLinearColor> DotColour;
		TAttribute<FLinearColor> RingColour;
		TAttribute<bool> Dashed;
		float DotRadius{3.4f};
		float RingRadius{6.4f};
	};

	/** The one obvious action: a filled amber button with dark text, dim when it cannot be done (the terminal's PrimaryButton). */
	TSharedRef<SWidget> AmberButton(const TAttribute<FText>& Label, FOnClicked OnClicked, TAttribute<bool> CanClick)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsEnabled(CanClick)
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
				SNew(STextBlock).Text(Label).Font(Font("Bold", 11))
				.Justification(ETextJustify::Center).RenderTransform(CapsCenterShift(Font("Bold", 11)))
				.ColorAndOpacity_Lambda([CanClick]()
				{
					return FSlateColor(CanClick.Get(true) ? APSChrome::OnAmber() : Muted());
				})
			]);
		return Button;
	}

	/** A chrome button with a centred label in its accent (the terminal's ChromeButton). */
	TSharedRef<SWidget> LabelButton(const TAttribute<FText>& Label, const TAttribute<FSlateColor>& Colour, FOnClicked OnClicked,
		const FLinearColor& Accent)
	{
		return APSInfrastructureUI::FrameButton(SNew(STextBlock).Text(Label).Font(Font("Bold", 10)).ColorAndOpacity(Colour),
			MoveTemp(OnClicked), TAttribute<bool>(false), Accent);
	}

	FText Ordinal(const int32 Value)
	{
		const int32 Tens = Value % 100;
		const TCHAR* Suffix = Tens >= 11 && Tens <= 13 ? TEXT("th") : Value % 10 == 1 ? TEXT("st") : Value % 10 == 2 ? TEXT("nd")
			: Value % 10 == 3 ? TEXT("rd") : TEXT("th");
		return FText::FromString(FString::Printf(TEXT("%d%s"), Value, Suffix));
	}

	FText DistanceText(const double Cm)
	{
		return FText::FromString(UShipNavigationComponent::FormatDistance(Cm));
	}

	FText OrderLabel(const APSFleet::EOrder Order, const FText& Anomaly)
	{
		switch (Order)
		{
		case APSFleet::EOrder::Probe: return LOCTEXT("OrderProbe", "SEND A PROBE");
		case APSFleet::EOrder::SurveySystem: return LOCTEXT("OrderSurvey", "SURVEY THE SYSTEM");
		case APSFleet::EOrder::Expedition: return FText::Format(LOCTEXT("OrderExpedition", "EXPEDITION TO THE {0}"), Anomaly);
		default: return LOCTEXT("OrderMove", "SEND THE MAIN FLEET");
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Shared pieces

TSharedRef<SWidget> APSStarMapUI::LegendToggle()
{
	using namespace APSChrome;
	using namespace APSStarMapPanelPrivate;
	// Rio 05.10: the legend and hint lines under every star map hide with one switch (and the map takes their room).
	return APSInfrastructureUI::FrameButton(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(12.0f).HeightOverride(12.0f)
			[
				SNew(SAPSStarDot).DotRadius(2.6f).RingRadius(5.0f)
				.DotColour_Lambda([]() { return SAPSStarScheme::IsLegendShown() ? Cyan() : FLinearColor::Transparent; })
				.RingColour_Lambda([]() { return SAPSStarScheme::IsLegendShown() ? Cyan() : Muted(); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("Legend", "LEGEND")).Font(Font("Bold", 9))
			.ColorAndOpacity_Lambda([]() { return FSlateColor(SAPSStarScheme::IsLegendShown() ? Cyan() : Muted()); })
		],
		FOnClicked::CreateLambda([]()
		{
			SAPSStarScheme::SetLegendShown(!SAPSStarScheme::IsLegendShown());
			return FReply::Handled();
		}),
		TAttribute<bool>::CreateLambda([]() { return SAPSStarScheme::IsLegendShown(); }), Cyan());
}

FGuid APSStarMapUI::SystemOf(const AActor* Actor)
{
	FGuid Id;
	return Actor && FAPSStarSystems::AnchorSystem(Actor, Id) ? Id : FGuid();
}

bool APSStarMapUI::DescribeSystemTarget(UWorld* World, const AActor* Target, FText& OutKind, FText& OutBody, FText& OutAnomaly)
{
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	const FGuid Id = SystemOf(Target);
	const FAPSStarSystemInfo* Info = Stars && Id.IsValid() ? Stars->Find(Id) : nullptr;
	if (!Info)
	{
		return false;
	}
	const FAPSStarSystemState State = Stars->GetState(Id);
	const APSStars::EKnowledge Known = Info->bHome ? APSStars::EKnowledge::Surveyed : State.Knowledge;
	OutKind = FText::Format(LOCTEXT("TargetKind", "STAR SYSTEM  /  {0}  /  {1}"),
		FText::FromString(Info->Spectral.IsEmpty() ? FString(TEXT("STAR")) : Info->Spectral),
		Info->bHome ? LOCTEXT("TargetHome", "HOME") : State.bClaimed ? LOCTEXT("TargetClaimed", "CLAIMED") : APSStars::KnowledgeName(Known));
	const FText Worlds = Known == APSStars::EKnowledge::Catalogued ? LOCTEXT("TargetWorldsUnknown", "Its worlds are unknown")
		: Info->PotentialPlanets == 0 ? LOCTEXT("TargetNoWorlds", "No worlds")
		: FText::Format(LOCTEXT("TargetWorlds", "Up to {0} worlds"), APSUINumber::Number(Info->PotentialPlanets));
	const FText Network = State.bClaimed ? LOCTEXT("TargetOnNetwork", "on the relay network")
		: Stars->IsInReach(Id) ? LOCTEXT("TargetInReach", "in the relays' reach") : LOCTEXT("TargetOutOfReach", "out of the relays' reach");
	OutBody = Info->bHome ? LOCTEXT("TargetHomeBody", "The civilization's home system.")
		: FText::Format(LOCTEXT("TargetBody", "{0}. {1} from home; {2}."), Worlds,
			APSStarMapPanelPrivate::DistanceText(Info->HomeDistanceCm), Network);
	const int32 Kind = Stars->AnomalyKindOf(Id);
	OutAnomaly = Kind == INDEX_NONE || State.Anomaly == 0 || State.Anomaly >= 3 ? FText::GetEmpty()
		: State.Anomaly == 1
		? FText::Format(LOCTEXT("TargetAnomalyDetected", "{0} detected: survey the system to locate it."), FAPSStarSystems::AnomalyName(Kind))
		: FText::Format(LOCTEXT("TargetAnomalyLocated", "{0} located: an expedition or a visit in person investigates it."),
			FAPSStarSystems::AnomalyName(Kind));
	return true;
}

TArray<SAPSStarScheme::FPlannedRoute> APSStarMapUI::PlannedRoutes(UWorld* World, const TArray<ASpaceship*>& Ships,
	const AActor* Target)
{
	TArray<SAPSStarScheme::FPlannedRoute> Routes;
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	if (!Fleet || !SystemOf(Target).IsValid())
	{
		return Routes;
	}
	for (ASpaceship* Ship : Ships)
	{
		const FAPSFleetUnit* Unit = Fleet->FindUnit(Ship);
		const FString Eta = Unit ? APSStarMap::FormatEta(Fleet->EstimateArrivalSeconds(Ship, Target)) : FString();
		// A ship that cannot fly there (no SpaceWrap) gets no route.
		if (Eta.IsEmpty())
		{
			continue;
		}
		SAPSStarScheme::FPlannedRoute& Route = Routes.AddDefaulted_GetRef();
		Route.Ship = Ship;
		Route.Label = FText::Format(LOCTEXT("PlannedEta", "{0}  ETA ~{1}"), FText::FromString(Unit->CallSign), FText::FromString(Eta));
	}
	return Routes;
}

// ---------------------------------------------------------------------------------------------------------------------
// MAP > STAR MAP

void SAPSStarMapPanel::Construct(const FArguments& InArgs)
{
	using namespace APSChrome;
	using namespace APSStarMapPanelPrivate;
	World = InArgs._World;
	CourseShip = InArgs._CourseShip;
	OnOpenScheme = InArgs._OnOpenScheme;
	OnFleetOrders = InArgs._OnFleetOrders;

	const auto FilterChip = [this](const APSStarMap::EFilter Wanted, const FText& Label)
	{
		return APSInfrastructureUI::FrameButton(
			SNew(STextBlock).Font(Font("Bold", 9))
			.Text_Lambda([this, Wanted, Label]()
			{
				const int32 Count = Scheme.IsValid() ? Scheme->GetSnapshot().FilterCounts[static_cast<int32>(Wanted)] : 0;
				return FText::Format(LOCTEXT("FilterChip", "{0}  {1}"), Label, APSUINumber::Number(Count));
			})
			.ColorAndOpacity_Lambda([this, Wanted]()
			{
				return FSlateColor(Scheme.IsValid() && Scheme->GetFilter() == Wanted ? Cyan() : Muted());
			}),
			FOnClicked::CreateLambda([this, Wanted]()
			{
				if (Scheme.IsValid())
				{
					Scheme->SetFilter(Wanted);
					ListSignature.Reset();
					RebuildList();
				}
				return FReply::Handled();
			}),
			TAttribute<bool>::CreateLambda([this, Wanted]() { return Scheme.IsValid() && Scheme->GetFilter() == Wanted; }), Cyan());
	};
	// One line of the card: a label column and its value in its own colour.
	const auto Field = [](const FText& Label, TFunction<FText()> Value, TFunction<FLinearColor()> Colour)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(78.0f)
				[
					SNew(STextBlock).Text(Label).Font(APSUITheme::BodyFont("Bold", 9)).ColorAndOpacity(Muted())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Top)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10))
				.Text_Lambda([Value]() { return Value(); })
				.ColorAndOpacity_Lambda([Colour]() { return FSlateColor(Colour()); })
			];
	};

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(Scheme, SAPSStarScheme)
				.World(World)
				.Style(SAPSStarScheme::EStyle::Full)
				.OnPicked_Lambda([this](const FGuid& Id) { Select(Id); })
				.OnOpened_Lambda([this](const FGuid& Id) { HandleOpened(Id); })
			]
			// The map's title over it: around which system, how many of the catalogue's are shown and known.
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f, 8.0f)
			[
				SNew(SVerticalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(Cyan())
					.Text_Lambda([this]() { return Scheme.IsValid() ? Scheme->GetTitle() : FText::GetEmpty(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
					.Text_Lambda([this]() { return Scheme.IsValid() ? Scheme->GetSubtitle() : FText::GetEmpty(); })
				]
			]
			// The filters with their counts, and the legend's switch.
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					FilterChip(APSStarMap::EFilter::All, LOCTEXT("FilterAll", "ALL"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					FilterChip(APSStarMap::EFilter::Known, LOCTEXT("FilterKnown", "KNOWN"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					FilterChip(APSStarMap::EFilter::Claimed, LOCTEXT("FilterClaimed", "CLAIMED"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					FilterChip(APSStarMap::EFilter::Uncharted, LOCTEXT("FilterUncharted", "UNCHARTED"))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					APSStarMapUI::LegendToggle()
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(330.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					IconSectionHeading(EAPSChromeGlyph::Stars, LOCTEXT("Section", "STAR MAP"),
						LOCTEXT("SectionSubtitle", "Neighbouring systems in order of distance"))
				]
				// The map around home, or around the system the pilot is in now.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([this]()
					{
						return PilotSystemId.IsValid() || (Scheme.IsValid() && Scheme->GetCentre().IsValid())
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Around", "AROUND")).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						APSInfrastructureUI::FrameButton(
							SNew(STextBlock).Text(LOCTEXT("AroundHome", "HOME")).Font(Font("Bold", 9))
							.ColorAndOpacity_Lambda([this]()
							{
								return FSlateColor(Scheme.IsValid() && !Scheme->GetCentre().IsValid() ? Cyan() : Muted());
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (Scheme.IsValid())
								{
									Scheme->SetCentre(FGuid());
									ListSignature.Reset();
								}
								return FReply::Handled();
							}),
							TAttribute<bool>::CreateLambda([this]() { return Scheme.IsValid() && !Scheme->GetCentre().IsValid(); }), Cyan())
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return PilotSystemId.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							APSInfrastructureUI::FrameButton(
								SNew(STextBlock).Font(Font("Bold", 9))
								.Text_Lambda([this]() { return FText::Format(LOCTEXT("AroundYou", "YOU: {0}"), PilotSystemName); })
								.ColorAndOpacity_Lambda([this]()
								{
									return FSlateColor(Scheme.IsValid() && Scheme->GetCentre() == PilotSystemId ? Cyan() : Muted());
								}),
								FOnClicked::CreateLambda([this]()
								{
									if (Scheme.IsValid() && PilotSystemId.IsValid())
									{
										Scheme->SetCentre(PilotSystemId);
										ListSignature.Reset();
									}
									return FReply::Handled();
								}),
								TAttribute<bool>::CreateLambda([this]()
								{
									return Scheme.IsValid() && PilotSystemId.IsValid() && Scheme->GetCentre() == PilotSystemId;
								}), Cyan())
						]
					]
				]
				// The pick's card.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(STextBlock).Font(Font("Bold", 14)).ColorAndOpacity(White())
								.Text_Lambda([this]() { return Card.Name; })
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(8.0f, 0.0f, 0.0f, 2.0f)
							[
								SNew(STextBlock).Font(Font("Bold", 10))
								.Text_Lambda([this]() { return Card.Designation; })
								.ColorAndOpacity_Lambda([this]() { return FSlateColor(Card.Colour); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Cyan())
							.Text_Lambda([this]() { return Card.Kind; })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
						[
							SNew(SVerticalBox)
							.Visibility_Lambda([this]() { return Card.bValid ? EVisibility::Visible : EVisibility::Collapsed; })
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
							[
								Field(LOCTEXT("FieldDistance", "DISTANCE"), [this]() { return Card.Distance; }, []() { return White(); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
							[
								Field(LOCTEXT("FieldKnown", "KNOWN"), [this]() { return Card.Known; }, [this]() { return Card.KnownColour; })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
							[
								Field(LOCTEXT("FieldWorlds", "WORLDS"), [this]() { return Card.Worlds; }, []() { return White(); })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
							[
								Field(LOCTEXT("FieldNetwork", "NETWORK"), [this]() { return Card.Network; }, [this]() { return Card.NetworkColour; })
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
							[
								Field(LOCTEXT("FieldAnomaly", "ANOMALY"), [this]() { return Card.Anomaly; }, [this]() { return Card.AnomalyColour; })
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								Field(LOCTEXT("FieldBestShip", "BEST SHIP"), [this]() { return Card.BestShip; }, [this]() { return Card.BestShipColour; })
							]
						],
						FMargin(14.0f, 12.0f), CyanDim())
				]
				// The next order for it (a probe, a survey of the system, an expedition, the main fleet) and who goes.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return Card.bHasOrder ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						AmberButton(TAttribute<FText>::CreateLambda([this]() { return Card.OrderLabel; }),
							FOnClicked::CreateSP(this, &SAPSStarMapPanel::RunOrder),
							TAttribute<bool>::CreateLambda([this]() { return Card.bOrderReady; }))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
					.Text_Lambda([this]() { return Card.OrderDetail; })
					.Visibility_Lambda([this]() { return Card.bHasOrder ? EVisibility::Visible : EVisibility::Collapsed; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						LabelButton(LOCTEXT("SetCourse", "SET COURSE"), FSlateColor(Cyan()),
							FOnClicked::CreateSP(this, &SAPSStarMapPanel::SetCourse), Cyan())
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						LabelButton(LOCTEXT("FleetOrders", "FLEET ORDERS  >"), FSlateColor(Cyan()),
							FOnClicked::CreateSP(this, &SAPSStarMapPanel::FleetOrders), Cyan())
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					LabelButton(LOCTEXT("OpenScheme", "OPEN THE SYSTEM SCHEME"),
						TAttribute<FSlateColor>::CreateLambda([this]()
						{
							return FSlateColor(Card.bSchemeReady ? Cyan() : FLinearColor(Muted().R, Muted().G, Muted().B, 0.6f));
						}),
						FOnClicked::CreateSP(this, &SAPSStarMapPanel::OpenScheme), Cyan())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
					.Text_Lambda([this]() { return Card.SchemeDetail; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
					.Text_Lambda([this]() { return Message; })
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(bMessageIsError ? Amber() : Success()); })
					.Visibility_Lambda([this]() { return Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("InView", "IN THIS VIEW  /  NEAREST FIRST")).Font(Font("Bold", 10)).ColorAndOpacity(Muted())
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(List, SVerticalBox)
					]
				]
			]
		]
	];
	ReadCard();
	RebuildList();
}

void SAPSStarMapPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	CardClock -= InDeltaTime;
	if (CardClock <= 0.0f)
	{
		CardClock = 0.5f;
		ReadCard();
		RebuildList();
	}
}

void SAPSStarMapPanel::Select(const FGuid& Id)
{
	if (Scheme.IsValid())
	{
		Scheme->SetSelected(Id);
	}
	Message = FText::GetEmpty();
	ReadCard();
}

AActor* SAPSStarMapPanel::PickedSite(const bool bAnchor) const
{
	FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FGuid Picked = Scheme.IsValid() ? Scheme->GetSelected() : FGuid();
	const FAPSStarSystemInfo* Info = Stars && Picked.IsValid() ? Stars->Find(Picked) : nullptr;
	if (!Info)
	{
		return nullptr;
	}
	// Home stands for itself (its star); every other system for its anchor (spawned on first use).
	if (Info->bHome && !bAnchor)
	{
		if (AActor* Star = APSStarMap::FindStarActor(World.Get(), *Stars, Stars->IndexOf(Picked)))
		{
			return Star;
		}
	}
	return Stars->GetAnchor(Picked);
}

void SAPSStarMapPanel::ReadCard()
{
	using namespace APSChrome;
	using namespace APSStarMapPanelPrivate;
	Card = FCard();
	NextOrder = APSFleet::EOrder::None;
	NextShip = nullptr;
	TArray<SAPSStarScheme::FPlannedRoute> Planned;
	UWorld* LiveWorld = World.Get();
	FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	// The system the pilot is in, when it is not home: the map can be centred on it.
	PilotSystemId.Invalidate();
	PilotSystemName = FText::GetEmpty();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (const FAPSStarSystemInfo* Here = Stars && Pawn ? Stars->Get(Stars->FindContaining(Pawn->GetActorLocation())) : nullptr;
		Here && !Here->bHome)
	{
		PilotSystemId = Here->Id;
		PilotSystemName = FText::FromString(Here->Name);
	}
	const FGuid Picked = Scheme.IsValid() ? Scheme->GetSelected() : FGuid();
	const FAPSStarSystemInfo* Info = Stars && Picked.IsValid() ? Stars->Find(Picked) : nullptr;
	if (!Info || !Scheme.IsValid())
	{
		Card.Name = LOCTEXT("CardNone", "NO SYSTEM PICKED");
		Card.Kind = LOCTEXT("CardHelp", "Click a star on the map or a row below; a double click opens the scheme of a system that stands.");
		Card.SchemeDetail = LOCTEXT("SchemeHelp", "The home system's scheme is always drawn; another system's while you are there.");
		if (Scheme.IsValid())
		{
			Scheme->SetPlannedRoutes(Planned);
		}
		return;
	}
	const APSStarMap::FSnapshot& Snap = Scheme->GetSnapshot();
	const int32 Shown = Scheme->GetModel().IndexOf(Picked);
	const APSStarMap::FSystem* System = Snap.Systems.IsValidIndex(Shown) ? &Snap.Systems[Shown] : nullptr;
	const FAPSStarSystemState State = Stars->GetState(Picked);
	const APSStars::EKnowledge Known = Info->bHome ? APSStars::EKnowledge::Surveyed : State.Knowledge;
	Card.bValid = true;
	Card.Name = FText::FromString(Info->Name);
	Card.Designation = FText::FromString(System ? System->Designation : FString(TEXT("A")));
	Card.Colour = System ? System->Colour : White();
	Card.Kind = FText::Format(LOCTEXT("CardKind", "{0}  /  {1}  /  {2}"),
		FText::FromString(Info->Spectral.IsEmpty() ? FString(TEXT("STAR")) : Info->Spectral),
		System ? System->Kind : LOCTEXT("KindStar", "STAR"),
		Info->StarCount <= 1 ? LOCTEXT("OneStar", "1 STAR") : FText::Format(LOCTEXT("SomeStars", "{0} STARS"), APSUINumber::Number(Info->StarCount)));

	// Where it is: how far from the map's centre and, among the 25 nearest, its rank.
	const FAPSStarSystemInfo* CentreInfo = Snap.Systems.Num() > 0 ? Stars->Find(Snap.Systems[0].Id) : nullptr;
	if (System && System->bCentre)
	{
		Card.Distance = Info->bHome ? LOCTEXT("DistanceHome", "home: the middle of the map") : LOCTEXT("DistanceCentre", "the middle of the map");
	}
	else
	{
		const double Cm = System ? System->DistanceCm : CentreInfo ? FVector::Dist(Info->Location, CentreInfo->Location) : Info->HomeDistanceCm;
		const FText From = CentreInfo && !CentreInfo->bHome ? FText::FromString(CentreInfo->Name) : LOCTEXT("FromHome", "home");
		Card.Distance = System && System->Ring < APSStarMap::OuterRing
			? FText::Format(LOCTEXT("DistanceRank", "{0} from {1}  /  {2} nearest"), DistanceText(Cm), From, Ordinal(System->Rank))
			: FText::Format(LOCTEXT("DistanceFar", "{0} from {1}"), DistanceText(Cm), From);
	}
	Card.Known = Known == APSStars::EKnowledge::Catalogued ? LOCTEXT("KnownCatalogued", "CATALOGUED: only its light is known")
		: Known == APSStars::EKnowledge::Scanned ? LOCTEXT("KnownScanned", "SCANNED: its worlds are counted")
		: LOCTEXT("KnownSurveyed", "SURVEYED: charted");
	Card.KnownColour = Known == APSStars::EKnowledge::Catalogued ? Muted() : APSStars::KnowledgeColour(Known);
	// Its worlds: unknown until a scan, counted (the catalogue's "up to") after it, typed once the system stands and is surveyed.
	if (!System || Known == APSStars::EKnowledge::Catalogued)
	{
		Card.Worlds = LOCTEXT("WorldsUnknown", "unknown: a probe counts them");
	}
	else if (System->Worlds == 0)
	{
		Card.Worlds = LOCTEXT("WorldsNone", "none");
	}
	else if (!System->WorldTypes.IsEmpty())
	{
		TArray<FText> Types;
		for (int32 Each = 0; Each < System->WorldTypes.Num() && Each < 3; ++Each)
		{
			Types.Add(System->WorldTypes[Each]);
		}
		Card.Worlds = FText::Format(System->WorldTypes.Num() > 3 ? LOCTEXT("WorldsTypedMore", "{0}: {1}...") : LOCTEXT("WorldsTyped", "{0}: {1}"),
			APSUINumber::Number(System->Worlds), FText::Join(FText::FromString(TEXT(", ")), Types));
	}
	else if (System->bWorldsExact)
	{
		Card.Worlds = FText::Format(LOCTEXT("WorldsCounted", "{0}, types unknown: survey it"), APSUINumber::Number(System->Worlds));
	}
	else
	{
		Card.Worlds = Known == APSStars::EKnowledge::Scanned
			? FText::Format(LOCTEXT("WorldsUpTo", "up to {0}, types unknown: survey it"), APSUINumber::Number(System->Worlds))
			: FText::Format(LOCTEXT("WorldsUpToSurveyed", "up to {0}: fly there to see them"), APSUINumber::Number(System->Worlds));
	}
	const bool bClaimed = State.bClaimed || Info->bHome;
	const bool bReach = !bClaimed && Stars->IsInReach(Picked);
	Card.Network = bClaimed ? LOCTEXT("NetworkClaimed", "CLAIMED: on the relay network")
		: bReach ? LOCTEXT("NetworkReach", "in the relays' reach") : LOCTEXT("NetworkOut", "out of the relays' reach");
	Card.NetworkColour = bClaimed ? TealColour() : bReach ? Cyan() : Muted();
	const int32 AnomalyKind = Stars->AnomalyKindOf(Picked);
	if (AnomalyKind == INDEX_NONE || State.Anomaly == 0)
	{
		Card.Anomaly = LOCTEXT("AnomalyNone", "none known");
		Card.AnomalyColour = Muted();
	}
	else
	{
		Card.Anomaly = FText::Format(LOCTEXT("AnomalyState", "{0}  /  {1}"), FAPSStarSystems::AnomalyName(AnomalyKind),
			State.Anomaly == 1 ? LOCTEXT("AnomalyDetected", "DETECTED") : State.Anomaly == 2 ? LOCTEXT("AnomalyLocated", "LOCATED")
			: LOCTEXT("AnomalyInvestigated", "INVESTIGATED"));
		Card.AnomalyColour = State.Anomaly >= 3 ? Muted() : Amber();
	}

	// The next order and the ship it would send: a probe for an uncharted system, a survey for a scanned one, an
	// expedition to a located anomaly, else the main fleet. Its route is drawn on the map in amber.
	if (Info->bHome)
	{
		Card.BestShip = LOCTEXT("BestShipHome", "the fleet's berths are here");
		Card.BestShipColour = Muted();
	}
	else
	{
		NextOrder = Known == APSStars::EKnowledge::Catalogued ? APSFleet::EOrder::Probe
			: Known == APSStars::EKnowledge::Scanned ? APSFleet::EOrder::SurveySystem
			: State.Anomaly == 2 && AnomalyKind != INDEX_NONE ? APSFleet::EOrder::Expedition : APSFleet::EOrder::Move;
		Card.bHasOrder = true;
		Card.OrderLabel = OrderLabel(NextOrder, AnomalyKind != INDEX_NONE ? FAPSStarSystems::AnomalyName(AnomalyKind) : FText::GetEmpty());
		const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
		AActor* Anchor = Stars->GetAnchor(Picked);
		FText Refusal = Fleet ? FText::GetEmpty() : LOCTEXT("NoFleet", "Fleet command is not running in this world.");
		ASpaceship* Ship = Fleet && Anchor ? APSStarMap::PickShip(*Fleet, NextOrder, Anchor, Refusal) : nullptr;
		const FAPSFleetUnit* Unit = Ship ? Fleet->FindUnit(Ship) : nullptr;
		if (Unit)
		{
			NextShip = Ship;
			Card.bOrderReady = true;
			const FString Eta = APSStarMap::FormatEta(Fleet->EstimateArrivalSeconds(Ship, Anchor));
			const FText Call = FText::FromString(Unit->CallSign);
			Card.BestShip = Eta.IsEmpty() ? FText::Format(LOCTEXT("BestShip", "{0} {1}"), Call, APSFleet::DivisionName(Unit->Division))
				: FText::Format(LOCTEXT("BestShipEta", "{0} {1}  /  ETA ~{2}"), Call, APSFleet::DivisionName(Unit->Division), FText::FromString(Eta));
			Card.BestShipColour = APSFleet::DivisionColour(Unit->Division);
			Card.OrderDetail = FText::Format(LOCTEXT("OrderGoes", "{0} ({1}) goes; its route is in amber."), Call, APSFleet::DivisionName(Unit->Division));
			SAPSStarScheme::FPlannedRoute& Route = Planned.AddDefaulted_GetRef();
			Route.Ship = Ship;
			Route.Label = Eta.IsEmpty() ? FText::Format(LOCTEXT("PlannedWord", "{0}  {1}"), Call, APSStarMap::OrderWord(NextOrder))
				: FText::Format(LOCTEXT("PlannedWordEta", "{0}  {1}  ETA ~{2}"), Call, APSStarMap::OrderWord(NextOrder), FText::FromString(Eta));
		}
		else
		{
			Card.BestShip = Refusal.IsEmpty() ? LOCTEXT("NoShip", "no free ship can go") : Refusal;
			Card.BestShipColour = Muted();
			Card.OrderDetail = Card.BestShip;
		}
	}
	Scheme->SetPlannedRoutes(Planned);
	Card.bSchemeReady = APSStarMap::FindStarActor(LiveWorld, *Stars, Stars->IndexOf(Picked)) != nullptr;
	Card.SchemeDetail = Card.bSchemeReady
		? (Info->bHome ? LOCTEXT("SchemeHome", "The home system: its stars, worlds and moons in order.")
			: LOCTEXT("SchemeHere", "It stands now: its stars, worlds and moons in order."))
		: LOCTEXT("SchemeAway", "Drawn from the system itself: fly there in person (home is always drawn).");
}

void SAPSStarMapPanel::RebuildList()
{
	using namespace APSChrome;
	using namespace APSStarMapPanelPrivate;
	if (!List.IsValid() || !Scheme.IsValid())
	{
		return;
	}
	const APSStarMap::FSnapshot& Snap = Scheme->GetSnapshot();
	// Rows change only with what is shown and known; the pick's highlight is read live.
	FString Wanted = FString::Printf(TEXT("%d|"), static_cast<int32>(Scheme->GetFilter()));
	TArray<int32> Rows;
	for (int32 Index = 1; Index < Snap.Systems.Num(); ++Index)
	{
		const APSStarMap::FSystem& System = Snap.Systems[Index];
		if (APSStarMap::PassesFilter(System, Scheme->GetFilter()))
		{
			Rows.Add(Index);
			Wanted += FString::Printf(TEXT("%s:%d%d;"), *System.Id.ToString(EGuidFormats::Short), static_cast<int32>(System.Knowledge),
				System.bClaimed ? 1 : 0);
		}
	}
	if (Wanted == ListSignature)
	{
		return;
	}
	ListSignature = Wanted;
	List->ClearChildren();
	for (const int32 Index : Rows)
	{
		const APSStarMap::FSystem& System = Snap.Systems[Index];
		const FGuid Id = System.Id;
		const bool bUncharted = System.Knowledge == APSStars::EKnowledge::Catalogued && !System.bClaimed && !System.bHome;
		const FString Spectral = (System.Spectral.IsEmpty() ? FString(TEXT("STAR")) : System.Spectral)
			+ (System.StarCount > 1 ? FString::Printf(TEXT(" +%d"), System.StarCount - 1) : FString());
		const auto IsPicked = [this, Id]() { return Scheme.IsValid() && Scheme->GetSelected() == Id; };
		List->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.OnClicked_Lambda([this, Id]()
			{
				Select(Id);
				return FReply::Handled();
			})
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(0.0f)
				.BorderBackgroundColor_Lambda([IsPicked]() { return FSlateColor(IsPicked() ? PickedRowColour() : RowColour()); })
				[
					SNew(SBox).HeightOverride(24.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(3.0f)
							[
								SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
								.BorderBackgroundColor_Lambda([IsPicked]() { return FSlateColor(IsPicked() ? Amber() : FLinearColor::Transparent); })
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
							[
								SNew(SAPSStarDot)
								.DotColour(bUncharted ? FLinearColor(System.Colour.R, System.Colour.G, System.Colour.B, 0.6f) : System.Colour)
								.RingColour(bUncharted ? FLinearColor(Muted().R, Muted().G, Muted().B, 0.55f) : APSStars::KnowledgeColour(System.Knowledge))
								.Dashed(bUncharted)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(0.36f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(System.Name)).Font(APSUITheme::BodyFont("Bold", 10))
							.ColorAndOpacity_Lambda([IsPicked]() { return FSlateColor(IsPicked() ? Amber() : White()); })
						]
						+ SHorizontalBox::Slot().FillWidth(0.2f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(Spectral)).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
						]
						+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(APSStarMap::StateName(System)).Font(Font("Regular", 9))
							.ColorAndOpacity(APSStarMap::StateColour(System))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 10.0f, 0.0f)
						[
							SNew(STextBlock).Text(DistanceText(System.DistanceCm)).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
						]
					]
				]
			]
		];
	}
}

FReply SAPSStarMapPanel::RunOrder()
{
	FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	ASpaceship* Ship = NextShip.Get();
	AActor* Anchor = PickedSite(true);
	FText Refusal;
	const int32 Issued = Fleet && Ship && Anchor ? Fleet->IssueOrder({Ship}, NextOrder, Anchor, Refusal) : 0;
	const FAPSFleetUnit* Unit = Fleet && Ship ? Fleet->FindUnit(Ship) : nullptr;
	bMessageIsError = Issued == 0;
	Message = Issued > 0 && Unit
		? FText::Format(LOCTEXT("OrderGiven", "ORDER GIVEN: {0} {1} {2}."), FText::FromString(Unit->CallSign), APSFleet::OrderName(NextOrder),
			Card.Name)
		: Refusal.IsEmpty() ? LOCTEXT("OrderNoShip", "No ship can take it now.") : Refusal;
	ReadCard();
	return FReply::Handled();
}

FReply SAPSStarMapPanel::SetCourse()
{
	ASpaceship* Ship = CourseShip ? CourseShip() : nullptr;
	AActor* Site = PickedSite(false);
	bMessageIsError = true;
	if (!Site)
	{
		Message = LOCTEXT("CoursePick", "Pick a system first.");
	}
	else if (!Ship || !Ship->ShipNavigation)
	{
		Message = LOCTEXT("CourseNoShip", "No ship to set a course for.");
	}
	else if (!Ship->ShipNavigation->SetCourse(Site->GetPathName()))
	{
		Message = LOCTEXT("CourseNotCharted", "This system is not charted for navigation yet.");
	}
	else
	{
		Message = FText::Format(LOCTEXT("CourseSet", "COURSE SET: {0}"), Card.Name);
		bMessageIsError = false;
	}
	return FReply::Handled();
}

FReply SAPSStarMapPanel::OpenScheme()
{
	FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FGuid Picked = Scheme.IsValid() ? Scheme->GetSelected() : FGuid();
	AActor* Star = Stars && Picked.IsValid() ? APSStarMap::FindStarActor(World.Get(), *Stars, Stars->IndexOf(Picked)) : nullptr;
	if (Star)
	{
		Message = FText::GetEmpty();
		OnOpenScheme.ExecuteIfBound(Star);
	}
	else
	{
		bMessageIsError = true;
		Message = Picked.IsValid() ? LOCTEXT("SchemeNotHere", "Its scheme is drawn from the system itself: fly there in person.")
			: LOCTEXT("SchemePick", "Pick a system first.");
	}
	return FReply::Handled();
}

FReply SAPSStarMapPanel::FleetOrders()
{
	const FGuid Picked = Scheme.IsValid() ? Scheme->GetSelected() : FGuid();
	if (Picked.IsValid())
	{
		OnFleetOrders.ExecuteIfBound(Picked);
	}
	else
	{
		bMessageIsError = true;
		Message = LOCTEXT("FleetPick", "Pick a system first.");
	}
	return FReply::Handled();
}

void SAPSStarMapPanel::HandleOpened(const FGuid& Id)
{
	Select(Id);
	OpenScheme();
}

// ---------------------------------------------------------------------------------------------------------------------
// FLEET ORDERS' ROUTES

void SAPSStarRoutes::Construct(const FArguments& InArgs)
{
	using namespace APSChrome;
	World = InArgs._World;
	Ships = InArgs._Ships;
	Target = InArgs._Target;
	ChildSlot
	[
		SNew(SVerticalBox)
		.Visibility_Lambda([this]() { return Rows.IsValid() && Rows->NumSlots() > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("Routes", "ROUTES")).Font(Font("Bold", 10)).ColorAndOpacity(Muted())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SAssignNew(Rows, SVerticalBox)
		]
	];
}

void SAPSStarRoutes::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Rebuild();
}

void SAPSStarRoutes::Rebuild()
{
	using namespace APSChrome;
	if (!Rows.IsValid())
	{
		return;
	}
	const AActor* Goal = Target ? Target() : nullptr;
	const bool bSystem = APSStarMapUI::SystemOf(Goal).IsValid();
	const TArray<ASpaceship*> Picked = bSystem && Ships ? Ships() : TArray<ASpaceship*>();
	FString Wanted = GetNameSafe(Goal);
	for (const ASpaceship* Ship : Picked)
	{
		Wanted += TEXT("|") + GetNameSafe(Ship);
	}
	if (Wanted == Signature)
	{
		return;
	}
	Signature = Wanted;
	Rows->ClearChildren();
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	for (ASpaceship* Ship : Picked)
	{
		const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
		if (!Unit)
		{
			continue;
		}
		const TWeakObjectPtr<ASpaceship> WeakShip = Ship;
		const TWeakObjectPtr<const AActor> WeakGoal = Goal;
		const TWeakObjectPtr<UWorld> WeakWorld = World;
		const FLinearColor Colour = APSFleet::DivisionColour(Unit->Division);
		Rows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(0.0f)
			.BorderBackgroundColor(APSStarMapPanelPrivate::RowColour())
			[
				SNew(SBox).HeightOverride(24.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(3.0f)
						[
							SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(54.0f)
						[
							SNew(STextBlock).Text(FText::FromString(Unit->CallSign)).Font(APSUITheme::BodyFont("Bold", 10))
							.ColorAndOpacity(Colour)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Font("Regular", 9)).ColorAndOpacity(Muted())
						.Text_Lambda([WeakShip, WeakGoal, WeakWorld]()
						{
							const ASpaceship* Live = WeakShip.Get();
							const AActor* Live2 = WeakGoal.Get();
							const FAPSStarSystems* Stars = APSStarSystemsFind(WeakWorld.Get());
							if (!Live || !Live2 || !Stars)
							{
								return FText::GetEmpty();
							}
							const FAPSStarSystemInfo* Here = Stars->Get(Stars->FindContaining(Live->GetActorLocation()));
							return FText::Format(LOCTEXT("RouteFrom", "from {0}  /  {1}"),
								Here ? FText::FromString(Here->Name) : LOCTEXT("DeepSpace", "DEEP SPACE"),
								APSStarMapPanelPrivate::DistanceText(FVector::Dist(Live->GetActorLocation(), Live2->GetActorLocation())));
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(STextBlock).Font(APSUITheme::BodyFont("Bold", 10))
						.Text_Lambda([WeakShip, WeakGoal, WeakWorld]()
						{
							const FAPSFleetCommand* Command = APSFleetFind(WeakWorld.Get());
							const FString Eta = Command ? APSStarMap::FormatEta(Command->EstimateArrivalSeconds(WeakShip.Get(), WeakGoal.Get()))
								: FString();
							return Eta.IsEmpty() ? LOCTEXT("RouteNoReach", "CANNOT REACH")
								: FText::Format(LOCTEXT("RouteEta", "ETA ~{0}"), FText::FromString(Eta));
						})
						.ColorAndOpacity_Lambda([WeakShip, WeakGoal, WeakWorld]()
						{
							const FAPSFleetCommand* Command = APSFleetFind(WeakWorld.Get());
							return FSlateColor(Command && Command->EstimateArrivalSeconds(WeakShip.Get(), WeakGoal.Get()) >= 0.0 ? Amber() : Muted());
						})
					]
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
