#include "SAPSShipHud.h"

#include "APSHudKit.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSShipHud"

namespace APSShipHudPrivate
{
	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	TAttribute<FSlateColor> Colour(TFunction<FLinearColor()> Pick)
	{
		return TAttribute<FSlateColor>::CreateLambda([Pick]() { return FSlateColor(Pick()); });
	}

	const FSlateBrush* Dot()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 4.0f);
		return &Brush;
	}

	/** The speed against the band's limit: the track runs to 125 % so the limit has a mark and an overshoot shows. */
	class SSpeedBar final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSpeedBar) {}
			SLATE_ATTRIBUTE(float, Fraction)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Fraction = InArgs._Fraction;
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(120.0f, 10.0f); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
			constexpr float Span = 1.25f;
			const float Value = FMath::Clamp(Fraction.Get(0.0f), 0.0f, Span);
			const double TrackY = (Size.Y - 3.0) * 0.5;
			const auto Box = [&](const double X, const double Y, const double W, const double H, const FLinearColor& Colour,
				const int32 Layer)
			{
				if (W <= 0.0 || H <= 0.0)
				{
					return;
				}
				FSlateDrawElement::MakeBox(OutDrawElements, Layer,
					Geometry.ToPaintGeometry(FVector2D(W, H), FSlateLayoutTransform(FVector2D(X, Y))),
					White, ESlateDrawEffect::None, Colour);
			};
			Box(0.0, TrackY, Size.X, 3.0, APSUITheme::Fade(P().Frame, 0.9f), LayerId);
			const double LimitX = Size.X / Span;
			const double FillX = Size.X * Value / Span;
			Box(0.0, TrackY, FMath::Min(FillX, LimitX), 3.0, P().Highlight, LayerId + 1);
			Box(LimitX, TrackY, FMath::Max(FillX - LimitX, 0.0), 3.0, P().Action, LayerId + 1);
			Box(LimitX - 1.0, 0.0, 2.0, Size.Y, P().Action, LayerId + 2);
			return LayerId + 2;
		}

	private:
		TAttribute<float> Fraction;
	};

	FString FormatEta(const double Seconds)
	{
		if (Seconds < 120.0) return FString::Printf(TEXT("%.0f s"), Seconds);
		if (Seconds < 7200.0) return FString::Printf(TEXT("%.1f min"), Seconds / 60.0);
		if (Seconds < 172800.0) return FString::Printf(TEXT("%.1f h"), Seconds / 3600.0);
		// Rio 06.10 (audit: a far star at low speed printed a day count wider than the tile): display only, bounded. The unit
		// stays after the last space for SplitValueUnit; NaN and infinity end in the last line.
		const double Days = Seconds / 86400.0;
		if (Days < 1000.0) return FString::Printf(TEXT("%.1f d"), Days);
		const double Years = Seconds / 31557600.0;
		if (Years < 1000.0) return FString::Printf(TEXT("%.0f y"), Years);
		return FString(TEXT("> 999 y"));
	}

	/** A metric well: a small label over a large value with its unit. */
	TSharedRef<SWidget> Tile(const FText& Label, const TAttribute<FString>& Value)
	{
		return SNew(SBorder).BorderImage(APSChrome::MetricTileBrush()).Padding(FMargin(10.0f, 7.0f, 10.0f, 8.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[APSHud::Label(Label, Colour([]() { return P().TextQuiet; }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[APSHud::ValueWithUnit(Value, 16)]
		];
	}

	/** One flight-bar column: a label, a main line and a quiet line under it. */
	TSharedRef<SWidget> Column(const TAttribute<FText>& Label, const TSharedRef<SWidget>& Main, const TAttribute<FText>& Sub,
		const TAttribute<FSlateColor>& SubColour)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[APSHud::Label(Label, Colour([]() { return P().TextQuiet; }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[Main]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				// Rio 06.10 (audit: a wrapped sub-line grew the flight bar by a line and made it jump): one line, as in 61532ed6.
				SNew(STextBlock).Text(Sub).Font(APSHud::TextFont(11)).ColorAndOpacity(SubColour)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			];
	}
}

void SAPSShipHud::Construct(const FArguments& InArgs)
{
	Ship = InArgs._Ship;
	ShowNavigation = InArgs._ShowNavigation;
	StatusText = InArgs._StatusText;

	const TAttribute<FText> HintText = InArgs._HintText;
	NavigationCard = BuildNavigationCard();
	FlightBar = BuildFlightBar();
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 22.0f, 22.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(312.0f)
			.Visibility_Lambda([this]() { return ShowNavigation.Get(true) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[NavigationCard.ToSharedRef()]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 48.0f)
		[
			SNew(SBox).WidthOverride(820.0f).Visibility(EVisibility::HitTestInvisible)[FlightBar.ToSharedRef()]
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(60.0f, 0.0f, 60.0f, 14.0f)
		[
			SNew(SAPSHudKeyHints).Text(HintText)
		]
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
	Refresh(0.0, 0.0f);
	RegisterActiveTimer(0.05f, FWidgetActiveTimerDelegate::CreateSP(this, &SAPSShipHud::Refresh));
}

EActiveTimerReturnType SAPSShipHud::Refresh(double, float)
{
	const ASpaceship* Pinned = Ship.Get();
	Readout = Pinned && Pinned->FlightModel ? Pinned->FlightModel->GetHudReadout() : FAPSFlightReadout();
	const UShipNavigationComponent* Navigation = Pinned ? Pinned->ShipNavigation : nullptr;
	FString Signature;
	if (Navigation)
	{
		const TArray<FShipNavigationContact>& Contacts = Navigation->GetContacts();
		const int32 Count = FMath::Min(Contacts.Num(), 5);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Signature += Contacts[Index].StableId + TEXT(";");
		}
	}
	if (Signature != NearestSignature)
	{
		NearestSignature = Signature;
		RebuildNearest();
	}
	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SAPSShipHud::BuildNavigationCard()
{
	using namespace APSShipHudPrivate;
	const TWeakObjectPtr<ASpaceship> WeakShip = Ship;
	const auto Selected = [WeakShip]() -> const FShipNavigationContact*
	{
		const ASpaceship* Pinned = WeakShip.Get();
		return Pinned && Pinned->ShipNavigation ? Pinned->ShipNavigation->GetSelectedContact() : nullptr;
	};
	const auto HasTarget = [Selected]() { return Selected() != nullptr; };

	return APSHud::Card(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[APSHud::Label(LOCTEXT("Navigation", "NAVIGATION"), Colour([]() { return P().TextQuiet; }))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSHud::TextFont(11)).ColorAndOpacity(Colour([]() { return P().TextQuiet; }))
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.Text_Lambda([WeakShip]()
				{
					const ASpaceship* Pinned = WeakShip.Get();
					if (!Pinned || !Pinned->ShipNavigation)
					{
						return LOCTEXT("NavigationOffline", "Offline");
					}
					return FText::Format(LOCTEXT("NavigationSummary", "{0} · {1} contacts"),
						FText::FromString(FText::FromString(Pinned->GetFlightEnvironmentName()).ToLower().ToString()),
						FText::AsNumber(Pinned->ShipNavigation->GetDiscoveredContactCount()));
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)[APSHud::Rule()]
		// The active target.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([HasTarget]() { return HasTarget() ? EVisibility::Visible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight()
			[APSHud::Label(LOCTEXT("ActiveTarget", "ACTIVE TARGET"), Colour([]() { return P().Action; }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSHud::ValueFont(18)).ColorAndOpacity(Colour([]() { return P().Text; }))
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.Text_Lambda([Selected]()
				{
					const FShipNavigationContact* Contact = Selected();
					return Contact ? FText::FromString(Contact->DisplayName.ToUpper()) : FText::GetEmpty();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSHud::TextFont(12)).ColorAndOpacity(Colour([]() { return P().TextSoft; }))
				.AutoWrapText(true)
				.Text_Lambda([Selected]()
				{
					const FShipNavigationContact* Contact = Selected();
					if (!Contact)
					{
						return FText::GetEmpty();
					}
					FString Line = Contact->Detail.IsEmpty() ? Contact->TypeLabel : Contact->Detail;
					Line = Line.ToLower();
					if (Line.Len() > 0)
					{
						Line[0] = FChar::ToUpper(Line[0]);
					}
					if (!Contact->HierarchyLabel.IsEmpty())
					{
						Line += TEXT(" · ") + Contact->HierarchyLabel;
					}
					return FText::FromString(Line);
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 5.0f, 0.0f)
				[
					Tile(LOCTEXT("Range", "RANGE"), TAttribute<FString>::CreateLambda([Selected]()
					{
						const FShipNavigationContact* Contact = Selected();
						return Contact ? UShipNavigationComponent::FormatDistance(Contact->DistanceCentimeters) : FString();
					}))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f, 0.0f, 0.0f)
				[
					Tile(LOCTEXT("Arrival", "ARRIVAL"), TAttribute<FString>::CreateLambda([WeakShip, Selected]()
					{
						const FShipNavigationContact* Contact = Selected();
						const ASpaceship* Pinned = WeakShip.Get();
						const double SpeedCm = Pinned ? Pinned->GetShipSpeedMetersPerSecond() * 100.0 : 0.0;
						return Contact && SpeedCm > 1.0 ? FormatEta(Contact->DistanceCentimeters / SpeedCm) : FString(TEXT("--"));
					}))
				]
			]
		]
		// No target yet.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([HasTarget]() { return HasTarget() ? EVisibility::Collapsed : EVisibility::Visible; })
			+ SVerticalBox::Slot().AutoHeight()
			[APSHud::Label(LOCTEXT("NoTarget", "NO TARGET"), Colour([]() { return P().TextQuiet; }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoTargetHint", "T picks the nearest body")).Font(APSHud::TextFont(12))
				.ColorAndOpacity(Colour([]() { return P().TextSoft; }))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[APSHud::Label(LOCTEXT("Nearest", "NEAREST"), Colour([]() { return P().TextQuiet; }))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[SAssignNew(NearestList, SVerticalBox)],
		APSHud::EEdge::Right);
}

void SAPSShipHud::RebuildNearest()
{
	using namespace APSShipHudPrivate;
	if (!NearestList.IsValid())
	{
		return;
	}
	NearestList->ClearChildren();
	const ASpaceship* Pinned = Ship.Get();
	const int32 Count = Pinned && Pinned->ShipNavigation ? FMath::Min(Pinned->ShipNavigation->GetContacts().Num(), 5) : 0;
	const TWeakObjectPtr<ASpaceship> WeakShip = Ship;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const auto Contact = [WeakShip, Index]() -> const FShipNavigationContact*
		{
			const ASpaceship* Current = WeakShip.Get();
			return Current && Current->ShipNavigation ? Current->ShipNavigation->GetContact(Index) : nullptr;
		};
		const auto Picked = [WeakShip, Index]()
		{
			const ASpaceship* Current = WeakShip.Get();
			return Current && Current->ShipNavigation && Current->ShipNavigation->GetSelectedContactIndex() == Index;
		};
		NearestList->AddSlot().AutoHeight().Padding(0.0f, 1.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).Padding(FMargin(7.0f, 4.0f))
			.BorderBackgroundColor_Lambda([Picked]()
			{
				return FSlateColor(Picked() ? APSUITheme::Fade(P().Text, 0.07f) : FLinearColor::Transparent);
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(2.0f).HeightOverride(13.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([Picked]() { return FSlateColor(Picked() ? P().Action : FLinearColor::Transparent); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(APSHud::TextFont(12, TEXT("SemiBold"))).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.ColorAndOpacity_Lambda([Picked]() { return FSlateColor(Picked() ? P().Text : P().TextSoft); })
					.Text_Lambda([Contact]()
					{
						const FShipNavigationContact* Current = Contact();
						return Current ? FText::FromString(Current->DisplayName) : FText::GetEmpty();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(74.0f)
					[
						SNew(STextBlock).Font(APSHud::TextFont(11)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.ColorAndOpacity(Colour([]() { return P().TextQuiet; }))
						.Text_Lambda([Contact]()
						{
							const FShipNavigationContact* Current = Contact();
							if (!Current)
							{
								return FText::GetEmpty();
							}
							FString Type = Current->TypeLabel.ToLower();
							if (Type.Len() > 0)
							{
								Type[0] = FChar::ToUpper(Type[0]);
							}
							return FText::FromString(Type);
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(APSHud::ValueFont(11)).ColorAndOpacity(Colour([]() { return P().Text; }))
					.Text_Lambda([Contact]()
					{
						const FShipNavigationContact* Current = Contact();
						return Current ? FText::FromString(UShipNavigationComponent::FormatDistance(Current->DistanceCentimeters))
							: FText::GetEmpty();
					})
				]
			]
		];
	}
}

TSharedRef<SWidget> SAPSShipHud::BuildFlightBar()
{
	using namespace APSShipHudPrivate;
	const auto Valid = [this]() { return Readout.bValid; };

	// 1. Autopilot, or the star drive while it runs.
	const TSharedRef<SWidget> PilotMain =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(8.0f).HeightOverride(8.0f)
			[
				SNew(SBorder).BorderImage(Dot())
				.BorderBackgroundColor_Lambda([this]()
				{
					const bool bOn = Readout.bStarDrive || Readout.bAutopilot;
					return FSlateColor(bOn ? P().Success : P().TextDim);
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(APSHud::ValueFont(17)).ColorAndOpacity(Colour([]() { return P().Text; }))
			.Text_Lambda([this]()
			{
				if (Readout.bStarDrive)
				{
					return Readout.DriveSpool < 1.0f
						? FText::Format(LOCTEXT("DriveSpooling", "{0}%"), FText::AsNumber(FMath::RoundToInt(Readout.DriveSpool * 100.0f)))
						: LOCTEXT("DriveCruise", "CRUISE");
				}
				return Readout.bAutopilot ? LOCTEXT("AutopilotOn", "ON") : LOCTEXT("AutopilotOff", "OFF");
			})
		];
	const TSharedRef<SWidget> PilotColumn = Column(
		TAttribute<FText>::CreateLambda([this]()
		{
			return Readout.bStarDrive ? LOCTEXT("StarDrive", "STAR DRIVE") : LOCTEXT("Autopilot", "AUTOPILOT");
		}),
		PilotMain,
		TAttribute<FText>::CreateLambda([this]()
		{
			if (Readout.bStarDrive)
			{
				if (Readout.DriveHeldBy.IsEmpty())
				{
					return FText::Format(LOCTEXT("DriveSet", "Set {0}"), FText::FromString(Readout.DriveSet));
				}
				// Rio 06.10 (audit: "Held back: INSIDE A STAR SYSTEM" did not fit the column): short words here only; the
				// flight model's literals stay as they are, the logs and run_0610_saw.ps1 parse them.
				const FString& H = Readout.DriveHeldBy;
				const FText Why = H == TEXT("INSIDE A STAR SYSTEM") ? LOCTEXT("HeldInSystem", "in a system")
					: H == TEXT("STAR SYSTEM AHEAD") ? LOCTEXT("HeldSystemAhead", "system ahead")
					: H == TEXT("BODY NEAR") ? LOCTEXT("HeldBody", "body near")
					: FText::FromString(H.ToLower());
				return FText::Format(LOCTEXT("DriveHeldShort", "Held: {0}"), Why);
			}
			return Readout.bAutopilot
				? FText::Format(LOCTEXT("AutopilotTo", "To {0}"), FText::FromString(Readout.AutopilotTarget))
				: LOCTEXT("AutopilotHint", "Z engages it");
		}),
		Colour([]() { return P().TextSoft; }));

	// 2. Speed against the limit.
	const TSharedRef<SWidget> SpeedColumn =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			// Rio 06.10: the label and the limit on one line, in the same face, so they share a baseline.
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[APSHud::Label(LOCTEXT("Speed", "SPEED"), Colour([]() { return P().TextQuiet; }))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(APSHud::LabelFont(9)).ColorAndOpacity(Colour([]() { return P().TextQuiet; }))
				.Text_Lambda([this]()
				{
					// Rio 06.10 (audit: "MM/S" read as millimetres): the units keep their spelling (Mm/s), as in the value row.
					if (Readout.bStarDrive)
					{
						return FText::Format(LOCTEXT("CruiseLimit", "CRUISE {0}"), FText::FromString(Readout.DriveCruise));
					}
					return Readout.Boost > 0.0f
						? FText::Format(LOCTEXT("BoostLimit", "BOOST x{0}   LIMIT {1}"), FText::AsNumber(Readout.Boost), FText::FromString(Readout.SpeedLimit))
						: FText::Format(LOCTEXT("SpeedLimit", "LIMIT {0}"), FText::FromString(Readout.SpeedLimit));
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
		[APSHud::ValueWithUnit(TAttribute<FString>::CreateLambda([this]() { return Readout.Speed; }), 20, 104.0f)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(SSpeedBar).Fraction_Lambda([this]() { return Readout.SpeedFraction; })
		];

	// 3. What is next: the autopilot's remaining way, or the nearest surface.
	const TSharedRef<SWidget> NextColumn = Column(
		TAttribute<FText>::CreateLambda([this]()
		{
			return Readout.bAutopilot ? LOCTEXT("ToTarget", "TO TARGET") : LOCTEXT("NearestSurface", "NEAREST");
		}),
		APSHud::ValueWithUnit(TAttribute<FString>::CreateLambda([this]()
		{
			if (Readout.bAutopilot)
			{
				return Readout.AutopilotRemaining;
			}
			return Readout.NearestDistance.IsEmpty() ? FString(TEXT("--")) : Readout.NearestDistance;
		}), 17, 88.0f),
		TAttribute<FText>::CreateLambda([this]()
		{
			if (Readout.bAutopilot)
			{
				return FText::FromString(Readout.AutopilotTarget);
			}
			return Readout.NearestBody.IsEmpty() ? LOCTEXT("OpenSpace", "Open space") : FText::FromString(Readout.NearestBody);
		}),
		Colour([]() { return P().TextSoft; }));

	// 4. The band (pace) or the engine's state.
	const TSharedRef<SWidget> BandColumn = Column(
		TAttribute<FText>::CreateLambda([this]()
		{
			if (Readout.bStarDrive)
			{
				return LOCTEXT("Pace", "PACE");
			}
			return Readout.bAutoBand ? LOCTEXT("PaceAuto", "PACE · AUTO") : LOCTEXT("PaceManual", "PACE · MANUAL");
		}),
		// Rio 06.10 (audit: the bar keeps a constant height): one line with an ellipsis, as in 61532ed6.
		SNew(STextBlock).Font(APSHud::ValueFont(14)).ColorAndOpacity(Colour([]() { return P().Highlight; }))
		.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
		.Text_Lambda([this]()
		{
			if (Readout.bStarDrive)
			{
				return LOCTEXT("PaceDrive", "STAR DRIVE");
			}
			return FText::FromString(Readout.BandName.ToUpper());
		}),
		TAttribute<FText>::CreateLambda([this]()
		{
			if (!Readout.bEngineRunning)
			{
				return LOCTEXT("EngineOff", "Engine off: G starts it");
			}
			FString Control = Readout.Control.ToLower();
			if (Control.Len() > 0)
			{
				Control[0] = FChar::ToUpper(Control[0]);
			}
			return FText::FromString(Control);
		}),
		TAttribute<FSlateColor>::CreateLambda([this]() { return FSlateColor(Readout.bEngineRunning ? P().TextSoft : P().Action); }));

	const TSharedRef<SWidget> Columns =
		SNew(SHorizontalBox)
		.Visibility_Lambda([Valid]() { return Valid() ? EVisibility::Visible : EVisibility::Collapsed; })
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(170.0f)[PilotColumn]]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14.0f, 0.0f)[APSHud::Rule(true)]
		+ SHorizontalBox::Slot().FillWidth(1.0f)[SpeedColumn]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14.0f, 0.0f)[APSHud::Rule(true)]
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(170.0f)[NextColumn]]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14.0f, 0.0f)[APSHud::Rule(true)]
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(190.0f)[BandColumn]];

	return APSHud::Card(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock).Font(APSHud::TextFont(12, TEXT("SemiBold"))).ColorAndOpacity(Colour([]() { return P().Action; }))
			.Visibility_Lambda([this]() { return Readout.Notice.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			.Text_Lambda([this]() { return FText::FromString(Readout.Notice); })
		]
		+ SVerticalBox::Slot().AutoHeight()[Columns]
		// Outside band flight (vehicles, the legacy model) the status line itself.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(APSHud::TextFont(12)).ColorAndOpacity(Colour([]() { return P().TextSoft; }))
			.AutoWrapText(true)
			.Visibility_Lambda([Valid]() { return Valid() ? EVisibility::Collapsed : EVisibility::Visible; })
			.Text(StatusText)
		],
		APSHud::EEdge::Centre, FMargin(18.0f, 12.0f, 18.0f, 12.0f));
}

#undef LOCTEXT_NAMESPACE
