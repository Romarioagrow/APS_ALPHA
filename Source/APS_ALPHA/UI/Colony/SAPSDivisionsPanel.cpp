#include "SAPSDivisionsPanel.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleDefaults.h"
#include "Textures/SlateShaderResource.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSDivisionsPanel"

namespace APSDivisionsPanelPrivate
{
	using namespace APSChrome;

	constexpr int32 ExplorationCard = 0;
	constexpr int32 IndustryCard = 1;
	constexpr int32 ScienceCard = 2;
	constexpr int32 CivilAffairsCard = 3;
	constexpr int32 MilitaryCard = 4;
	constexpr int32 FleetCommandCard = 5;

	/** Levels the work can earn per division (FAPSFleetCommand::GetEarnedLevel). */
	constexpr int32 MaxEarned = 3;
	constexpr float RingSize = 88.0f;
	constexpr float RingWidth = 7.0f;
	/** "GROWTH" and "SHIPS" share one column, so the bar and the hulls start under each other. */
	constexpr float RowLabelWidth = 66.0f;
	constexpr int32 MaxHulls = 12;
	constexpr float HullWidth = 11.0f;
	constexpr float HullHeight = 14.0f;
	constexpr float HullPitch = 15.0f;

	/** Work per earned level, as FAPSFleetCommand::GetEarnedLevel counts it (and the former growth lines said); 0: the
	 * level is set at the founding only. */
	int32 WorkPerLevel(const int32 Card)
	{
		switch (Card)
		{
		case ExplorationCard: return 3;
		case IndustryCard: return 3;
		case ScienceCard: return 2;
		case FleetCommandCard: return 4;
		default: return 0;
		}
	}

	/** The card a fleet division's ships count on. */
	int32 CardOf(const APSFleet::EDivision Division)
	{
		switch (Division)
		{
		case APSFleet::EDivision::Exploration: return ExplorationCard;
		case APSFleet::EDivision::Construction: return IndustryCard;
		case APSFleet::EDivision::Science: return ScienceCard;
		case APSFleet::EDivision::MainFleet: return MilitaryCard;
		default: return INDEX_NONE;
		}
	}

	struct FCardSpec
	{
		FText Name;
		FText Role;
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::Divisions};
		/** The division's colour: its fleet division's (as in fleet orders), the journal's colony green for civil
		 * affairs, the terminal's cyan for fleet command (every ship, as the journal's fleet entries). */
		FLinearColor Accent{FLinearColor::White};
		/** After "2 / 3" on the growth bar. */
		FText WorkUnit;
	};

	FCardSpec CardSpec(const int32 Card)
	{
		using APSFleet::EDivision;
		switch (Card)
		{
		case ExplorationCard:
			return {LOCTEXT("Exploration", "EXPLORATION"),
				LOCTEXT("ExplorationRole", "Surveys planets, systems and clusters, charts routes and finds anomalies."),
				EAPSChromeGlyph::Compass, APSFleet::DivisionColour(EDivision::Exploration),
				LOCTEXT("ExplorationWork", "WORLDS SURVEYED")};
		case IndustryCard:
			return {LOCTEXT("Industry", "INDUSTRY"),
				LOCTEXT("IndustryRole", "Production, mining, construction and the shipyards: everything the colony builds."),
				EAPSChromeGlyph::Infrastructure, APSFleet::DivisionColour(EDivision::Construction), LOCTEXT("IndustryWork", "BUILT")};
		case ScienceCard:
			return {LOCTEXT("Science", "SCIENCE"),
				LOCTEXT("ScienceRole", "Research and new technologies; investigates what exploration finds."),
				EAPSChromeGlyph::Planet, APSFleet::DivisionColour(EDivision::Science),
				LOCTEXT("ScienceWork", "STUDIES OR ANOMALIES")};
		case CivilAffairsCard:
			return {LOCTEXT("CivilAffairs", "CIVIL AFFAIRS"),
				LOCTEXT("CivilAffairsRole", "Claims star systems, runs public services, law and population growth."),
				EAPSChromeGlyph::Civilization, FLinearColor(0.36f, 1.0f, 0.58f), FText::GetEmpty()};
		case MilitaryCard:
			return {LOCTEXT("Military", "MILITARY"), LOCTEXT("MilitaryRole", "Forces, defence protocols and training."),
				EAPSChromeGlyph::Lock, APSFleet::DivisionColour(EDivision::MainFleet), FText::GetEmpty()};
		default:
			return {LOCTEXT("FleetCommand", "FLEET COMMAND"), LOCTEXT("FleetCommandRole", "Deploys, repairs and upgrades the fleet."),
				EAPSChromeGlyph::Fleet, Cyan(), LOCTEXT("FleetCommandWork", "SHIPS LAUNCHED")};
		}
	}

	void Arc(FSlateWindowElementList& Elements, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const float Radius, const float From, const float To, const FLinearColor& Colour, const float Thickness)
	{
		const int32 Steps = FMath::Clamp(FMath::CeilToInt((To - From) * Radius / 3.0f), 2, 160);
		TArray<FVector2D> Points;
		Points.Reserve(Steps + 1);
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float Angle = FMath::Lerp(From, To, static_cast<float>(Index) / Steps);
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour,
			true, Thickness);
	}

	/**
	 * The level gauge: one segment per level up to the division's cap, clockwise from the top: founding levels amber,
	 * earned levels green, the levels still to earn dim. A hairline in the division's colour rings it; a division
	 * without levels shows the bare track.
	 */
	class SLevelRing final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLevelRing) {}
			SLATE_ATTRIBUTE(int32, Founding)
			SLATE_ATTRIBUTE(int32, Earned)
			SLATE_ATTRIBUTE(int32, Cap)
			SLATE_ARGUMENT(FLinearColor, Accent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Founding = InArgs._Founding;
			Earned = InArgs._Earned;
			Cap = InArgs._Cap;
			Accent = InArgs._Accent;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(RingSize, RingSize); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, const int32 LayerId, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const float Radius = static_cast<float>(FMath::Min(Size.X, Size.Y)) * 0.5f - RingWidth * 0.5f - 5.0f;
			if (Radius < 8.0f)
			{
				return LayerId;
			}
			const FVector2D Centre = Size * 0.5;
			// Line widths are screen pixels: scaled with the UI, so the ring keeps its weight at any resolution.
			const float PixelScale = Geometry.GetAccumulatedLayoutTransform().GetScale();
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const int32 FromFounding = FMath::Max(Founding.Get(0), 0);
			const int32 FromWork = FMath::Max(Earned.Get(0), 0);
			const int32 Segments = FMath::Max(Cap.Get(0), FromFounding + FromWork);
			Arc(Elements, LayerId, Geometry, Centre, Radius + RingWidth * 0.5f + 3.0f, 0.0f, UE_TWO_PI,
				Accent.CopyWithNewOpacity(0.6f) * Tint, PixelScale);
			if (Segments <= 0)
			{
				Arc(Elements, LayerId, Geometry, Centre, Radius, 0.0f, UE_TWO_PI, CyanDim().CopyWithNewOpacity(0.35f) * Tint,
					RingWidth * PixelScale);
				return LayerId + 1;
			}
			const float Step = UE_TWO_PI / Segments;
			// Gaps wider than the lines' soft ends; a single level is a closed ring.
			const float Gap = Segments > 1 ? FMath::Min(5.0f / Radius, Step * 0.4f) : 0.0f;
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const FLinearColor Colour = Segment < FromFounding ? Amber()
					: Segment < FromFounding + FromWork ? Success() : CyanDim();
				const float From = -UE_HALF_PI + Segment * Step + Gap * 0.5f;
				Arc(Elements, LayerId + 1, Geometry, Centre, Radius, From, From + Step - Gap, Colour * Tint,
					RingWidth * PixelScale);
			}
			return LayerId + 1;
		}

	private:
		TAttribute<int32> Founding;
		TAttribute<int32> Earned;
		TAttribute<int32> Cap;
		FLinearColor Accent{FLinearColor::White};
	};

	/** A division's ships as hull icons: those under orders first and filled, the rest outlined; dots past twelve. */
	class SHullRow final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SHullRow) {}
			SLATE_ATTRIBUTE(int32, Ships)
			SLATE_ATTRIBUTE(int32, UnderOrders)
			SLATE_ARGUMENT(FLinearColor, Accent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Ships = InArgs._Ships;
			UnderOrders = InArgs._UnderOrders;
			Accent = InArgs._Accent;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			const int32 Count = FMath::Max(Ships.Get(0), 0);
			const int32 Shown = FMath::Min(Count, MaxHulls);
			return FVector2D(Shown * HullPitch + (Count > Shown ? 14.0f : 0.0f), HullHeight + 4.0f);
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, const int32 LayerId, const FWidgetStyle& Style, bool) const override
		{
			const int32 Count = FMath::Max(Ships.Get(0), 0);
			const int32 Shown = FMath::Min(Count, MaxHulls);
			if (Shown <= 0)
			{
				return LayerId;
			}
			const int32 Lit = FMath::Clamp(UnderOrders.Get(0), 0, Shown);
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const float PixelScale = Geometry.GetAccumulatedLayoutTransform().GetScale();
			const float Top = (static_cast<float>(Geometry.GetLocalSize().Y) - HullHeight) * 0.5f;
			const FSlateBrush* Solid = FAppStyle::GetBrush("WhiteBrush");
			const FSlateResourceHandle Handle = Solid->GetRenderingResource();
			const FSlateShaderResourceProxy* Proxy = Handle.GetResourceProxy();
			const FVector2f UV = Proxy ? Proxy->StartUV + Proxy->SizeUV * 0.5f : FVector2f(0.5f, 0.5f);
			const FColor Fill = (Accent * Tint).ToFColor(true);
			const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			for (int32 Hull = 0; Hull < Shown; ++Hull)
			{
				const float Middle = Hull * HullPitch + HullWidth * 0.5f;
				const FVector2f Nose(Middle, Top);
				const FVector2f Starboard(Middle + HullWidth * 0.5f, Top + HullHeight);
				const FVector2f Notch(Middle, Top + HullHeight * 0.7f);
				const FVector2f Port(Middle - HullWidth * 0.5f, Top + HullHeight);
				const bool bUnderOrders = Hull < Lit;
				if (bUnderOrders)
				{
					// The arrowhead as two triangles; the notch stays inside both.
					const SlateIndex Base = static_cast<SlateIndex>(Vertices.Num());
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Nose, UV, Fill));
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Starboard, UV, Fill));
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Notch, UV, Fill));
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Port, UV, Fill));
					for (const uint32 Corner : {0u, 1u, 2u, 0u, 2u, 3u})
					{
						Indices.Add(static_cast<SlateIndex>(Base + Corner));
					}
				}
				FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
					TArray<FVector2f>{Nose, Starboard, Notch, Port, Nose}, ESlateDrawEffect::None,
					(bUnderOrders ? Accent : Accent.CopyWithNewOpacity(Accent.A * 0.5f)) * Tint, true, 1.25f * PixelScale);
			}
			if (Vertices.Num() > 0)
			{
				FSlateDrawElement::MakeCustomVerts(Elements, LayerId, Handle, Vertices, Indices, nullptr, 0, 0);
			}
			if (Count > Shown)
			{
				for (int32 Dot = 0; Dot < 3; ++Dot)
				{
					FSlateDrawElement::MakeBox(Elements, LayerId + 1, Geometry.ToPaintGeometry(FVector2f(2.5f, 2.5f),
						FSlateLayoutTransform(FVector2f(Shown * HullPitch + 1.0f + Dot * 4.5f, Top + HullHeight - 2.5f))),
						Solid, ESlateDrawEffect::None, Accent * Tint);
				}
			}
			return LayerId + 1;
		}

	private:
		TAttribute<int32> Ships;
		TAttribute<int32> UnderOrders;
		FLinearColor Accent{FLinearColor::White};
	};

	const FSlateBrush* ChipBrush()
	{
		// The overview's metric tile (APSChrome::MetricTile): a dark inset with a quiet rim.
		// Rio 06.10: the shared tile follows the interface theme.
		return APSChrome::MetricTileBrush();
	}

	TSharedRef<SWidget> RowLabel(const FText& Label)
	{
		return SNew(SBox).WidthOverride(RowLabelWidth).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Label).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
		];
	}

	/** One effect of the level: its icon, the number large with its unit, a short label and an optional quiet note; the
	 * former sentence is the tooltip. */
	TSharedRef<SWidget> StatChip(const EAPSChromeGlyph Glyph, const FLinearColor& Accent, const TAttribute<FText>& Value,
		const FText& Unit, const FText& Label, const TAttribute<FText>& Note, const TAttribute<FText>& Sentence)
	{
		return SNew(SBorder).BorderImage(ChipBrush()).Padding(FMargin(10.0f, 7.0f, 14.0f, 8.0f)).ToolTipText(Sentence)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 5.0f, 9.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Glyph).Color(Accent).StrokeWidth(1.5f)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
					[
						SNew(STextBlock).Text(Value).Font(Font("Bold", 17)).ColorAndOpacity(White())
					]
					// The unit in the readable face: the display font is for digits and capitals.
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(3.0f, 0.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock).Text(Unit).Font(APSUITheme::BodyFont("Bold", 12)).ColorAndOpacity(Accent)
						.Visibility(Unit.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Label).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Note).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
						.Visibility(Note.IsBound() || !Note.Get().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed)
					]
				]
			]
		];
	}
}

void SAPSDivisionsPanel::Construct(const FArguments& InArgs)
{
	using namespace APSDivisionsPanelPrivate;
	World = InArgs._World;
	StatesFrame = GFrameCounter;
	ReadStates();
	const TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.0f));
	for (int32 Card = 0; Card < CardCount; ++Card)
	{
		Grid->AddSlot(Card % 2, Card / 2)
		[
			BuildCard(Card)
		];
	}
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			IconSectionHeading(EAPSChromeGlyph::Divisions, LOCTEXT("DivisionsSection", "DIVISIONS"),
				GetCivilization()
					? LOCTEXT("DivisionsSubtitleRing", "Rings: founding levels (amber), earned by work (green), still to earn (dim). Chips: what each level changes in the game, live")
					: LOCTEXT("DivisionsNoCivilization", "No civilization in this world yet"))
		]
		+ SScrollBox::Slot()
		[
			Grid
		]
	];
}

void SAPSDivisionsPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const FAPSMissionBoard* Board = APSMissionsFind(World.Get());
	if (const uint32 Revision = Board ? Board->GetRevision() : 0; Revision != MissionRevision)
	{
		MissionRevision = Revision;
		RebuildMissions();
	}
}

void SAPSDivisionsPanel::RebuildMissions()
{
	using namespace APSDivisionsPanelPrivate;
	using APSInfrastructure::EDepartment;
	const FAPSMissionBoard* Board = APSMissionsFind(World.Get());
	for (int32 Card = 0; Card < CardCount; ++Card)
	{
		const TSharedPtr<SVerticalBox> Box = MissionBoxes[Card];
		if (!Box.IsValid()) continue;
		Box->ClearChildren();
		if (!Board) continue;
		// Transport missions sit with industry, which raises the hubs and relays.
		TArray<EDepartment> Departments;
		switch (Card)
		{
		case ExplorationCard: Departments = {EDepartment::Exploration}; break;
		case IndustryCard: Departments = {EDepartment::Industry, EDepartment::Transport}; break;
		case ScienceCard: Departments = {EDepartment::Science}; break;
		case CivilAffairsCard: Departments = {EDepartment::CivilAffairs}; break;
		case MilitaryCard: Departments = {EDepartment::Military}; break;
		default: Departments = {EDepartment::FleetCommand}; break;
		}
		TArray<const FAPSMission*> Shown;
		for (const EDepartment Department : Departments)
		{
			TArray<const FAPSMission*> Each;
			Board->GetFor(Department, Each);
			for (const FAPSMission* Mission : Each)
			{
				if (Mission->State == APSMissions::EState::Offered || Mission->State == APSMissions::EState::Active) Shown.Add(Mission);
			}
		}
		Shown.StableSort([](const FAPSMission& A, const FAPSMission& B)
			{ return A.State == APSMissions::EState::Active && B.State != APSMissions::EState::Active; });
		if (Shown.IsEmpty()) continue;
		Box->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("MissionsHeading", "MISSIONS")).Font(Font("Bold", 11)).ColorAndOpacity(Cyan())
		];
		for (const FAPSMission* Mission : Shown)
		{
			Box->AddSlot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				BuildMission(*Mission)
			];
		}
	}
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildMission(const FAPSMission& Mission) const
{
	using namespace APSDivisionsPanelPrivate;
	const bool bActive = Mission.State == APSMissions::EState::Active;
	const FLinearColor Accent = APSInfrastructure::DepartmentColour(Mission.Department);
	const FGuid Id = Mission.Id;
	const TWeakObjectPtr<UWorld> WeakWorld = World;
	const auto Button = [](const FText& Label, const FLinearColor& Colour, TFunction<void()> OnClick)
	{
		// Rio 03.10: the label centred both ways, 14 each side.
		return SNew(SButton).ContentPadding(FMargin(14.0f, 4.0f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				CenteredLabel(Label, Font("Bold", 10), Colour)
			];
	};
	FString Rewards = APSInfrastructure::DescribeAmounts(Mission.Reward).ToString();
	if (Mission.RewardLevels > 0)
	{
		Rewards += FString::Printf(TEXT("  /  +%d LEVEL"), Mission.RewardLevels);
	}
	if (const APSInfrastructure::FType* Unlock = Mission.Unlocks.IsNone() ? nullptr : APSInfrastructure::Find(Mission.Unlocks))
	{
		Rewards += TEXT("  /  UNLOCKS ") + Unlock->Name.ToString();
	}
	const float Progress = Mission.Count > 0 ? FMath::Clamp(float(Mission.Progress) / float(Mission.Count), 0.0f, 1.0f) : 0.0f;
	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	if (bActive)
	{
		Buttons->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			Button(LOCTEXT("Track", "TRACK"), Cyan(), [WeakWorld, Id]() { if (FAPSMissionBoard* Board = APSMissionsFind(WeakWorld.Get())) Board->SetTracked(Id); })
		];
		Buttons->AddSlot().AutoWidth()
		[
			Button(LOCTEXT("Drop", "DROP"), Muted(), [WeakWorld, Id]() { if (FAPSMissionBoard* Board = APSMissionsFind(WeakWorld.Get())) Board->Decline(Id); })
		];
	}
	else
	{
		Buttons->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			Button(LOCTEXT("Accept", "ACCEPT"), Amber(), [WeakWorld, Id]() { if (FAPSMissionBoard* Board = APSMissionsFind(WeakWorld.Get())) Board->Accept(Id); })
		];
		Buttons->AddSlot().AutoWidth()
		[
			Button(LOCTEXT("Decline", "DECLINE"), Muted(), [WeakWorld, Id]() { if (FAPSMissionBoard* Board = APSMissionsFind(WeakWorld.Get())) Board->Decline(Id); })
		];
	}
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSUITheme::Retint(FLinearColor(0.012f, 0.035f, 0.05f, bActive ? 0.95f : 0.75f))).Padding(0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(bActive ? Accent : Accent.CopyWithNewOpacity(0.45f))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10.0f, 7.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(Mission.Title).Font(Font("Bold", 12)).ColorAndOpacity(bActive ? Amber() : White())
						.RenderTransform(CapsCenterShift(Font("Bold", 12)))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::Format(LOCTEXT("MissionState", "{0}  /  {1}"),
							APSInfrastructure::DepartmentName(Mission.Department), APSMissions::StateName(Mission.State)))
						.Font(Font("Bold", 9)).ColorAndOpacity(Accent)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Mission.Brief).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::Format(LOCTEXT("MissionObjective", "{0}  {1} / {2}"),
							APSMissions::ObjectiveName(Mission.Objective), APSUINumber::Number(Mission.Progress), APSUINumber::Number(Mission.Count)))
						.Font(Font("Bold", 10)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 10)))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(4.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
							]
							+ SOverlay::Slot().HAlign(HAlign_Left)
							[
								SNew(SBox).WidthOverride(TAttribute<FOptionalSize>::CreateLambda([Progress]() { return FOptionalSize(Progress * 200.0f); }))
								[
									SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Accent)
								]
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::Format(LOCTEXT("MissionReward", "REWARD  {0}"), FText::FromString(Rewards)))
					.AutoWrapText(true).Font(Font("Bold", 10)).ColorAndOpacity(Success())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					Buttons
				]
			]
		];
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildCard(const int32 Card)
{
	using namespace APSDivisionsPanelPrivate;
	const FCardSpec Spec = CardSpec(Card);
	return ChamferPanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
			[
				BuildLevel(Card, Spec.Accent)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						IconBadge(Spec.Glyph, Spec.Accent, 28.0f)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Spec.Name).Font(Font("Bold", 14)).ColorAndOpacity(White())
						.RenderTransform(CapsCenterShift(Font("Bold", 14)))
					]
				]
				// The description stays one quiet line; the numbers below carry the card.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Spec.Role).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
				[
					BuildLegend(Card)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			BuildStats(Card, Spec.Accent)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
		[
			SNew(SBox).HeightOverride(1.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			BuildGrowth(Card)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			BuildShips(Card, Spec.Accent)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SAssignNew(MissionBoxes[Card], SVerticalBox)
		],
		FMargin(16.0f, 14.0f), Spec.Accent.CopyWithNewOpacity(0.55f));
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildLevel(const int32 Card, const FLinearColor& Accent)
{
	using namespace APSDivisionsPanelPrivate;
	const bool bGrows = WorkPerLevel(Card) > 0;
	return SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
		.ToolTipText_Lambda([this, Card]() { return LevelText(Card); })
		[
			SNew(SBox).WidthOverride(RingSize).HeightOverride(RingSize)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SLevelRing).Accent(Accent)
					.Founding_Lambda([this, Card]() { return State(Card).Founding; })
					.Earned_Lambda([this, Card]() { return State(Card).Earned; })
					// A division that grows has room for every level its work can earn; the others end at the founding.
					.Cap_Lambda([this, Card, bGrows]()
					{
						const FCardState& Each = State(Card);
						return Each.Founding + (bGrows ? FMath::Max(MaxEarned, Each.Earned) : Each.Earned);
					})
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Font(Font("Bold", 24)).ColorAndOpacity(White())
						.Text_Lambda([this, Card]() { return APSUINumber::Number(State(Card).Level()); })
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, -3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("RingLevel", "LEVEL")).Font(Font("Bold", 9)).ColorAndOpacity(Accent)
					]
				]
			]
		];
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildLegend(const int32 Card)
{
	using namespace APSDivisionsPanelPrivate;
	const auto Item = [](const FLinearColor& Swatch, const TAttribute<FText>& Value, const FLinearColor& ValueColour,
		const FText& Label) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(8.0f).HeightOverride(8.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Swatch)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Value).Font(Font("Bold", 11)).ColorAndOpacity(ValueColour)
				.RenderTransform(CapsCenterShift(Font("Bold", 11)))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Label).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
			];
	};
	const TSharedRef<SHorizontalBox> Legend = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			Item(Amber(), TAttribute<FText>::CreateLambda([this, Card]() { return APSUINumber::Number(State(Card).Founding); }),
				Amber(), LOCTEXT("LegendFounding", "FOUNDING"))
		];
	if (WorkPerLevel(Card) > 0)
	{
		Legend->AddSlot().AutoWidth().Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			Item(Success(), TAttribute<FText>::CreateLambda([this, Card]()
			{
				return FText::Format(LOCTEXT("PlusValue", "+{0}"), APSUINumber::Number(State(Card).Earned));
			}), Success(), LOCTEXT("LegendEarned", "EARNED"))
		];
		Legend->AddSlot().AutoWidth().Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			Item(CyanDim(), TAttribute<FText>::CreateLambda([this, Card]()
			{
				return FText::Format(LOCTEXT("PlusValue", "+{0}"), APSUINumber::Number(FMath::Max(MaxEarned - State(Card).Earned, 0)));
			}), Muted(), LOCTEXT("LegendToEarn", "TO EARN"))
		];
	}
	return Legend;
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildStats(const int32 Card, const FLinearColor& Accent)
{
	using namespace APSDivisionsPanelPrivate;
	using APSFleet::EDivision;
	using APSFleet::EOrder;
	const TSharedRef<SWrapBox> Chips = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8.0f, 8.0f));
	const TAttribute<FText> Sentence = TAttribute<FText>::CreateLambda([this, Card]() { return EffectText(Card); });
	const auto Add = [&Chips, &Accent, &Sentence](const EAPSChromeGlyph Glyph, const TAttribute<FText>& Value,
		const FText& Unit, const FText& Label, const TAttribute<FText>& Note = TAttribute<FText>())
	{
		Chips->AddSlot().VAlign(VAlign_Fill)
		[
			StatChip(Glyph, Accent, Value, Unit, Label, Note, Sentence)
		];
	};
	// Seconds of work at the target at the card's level, by the fleet's own rule (as the former line counted them).
	const auto Seconds = [this, Card](const EOrder Order, const EDivision Division, const bool bStation)
	{
		return TAttribute<FText>::CreateLambda([this, Card, Order, Division, bStation]()
		{
			return APSUINumber::Number(FMath::RoundToInt(APSFleet::WorkSeconds(Order, Division, State(Card).Level(), bStation)));
		});
	};
	// "+N": a percent per level of the card (fleet command: per level and per sector HQ).
	const auto PerLevel = [this, Card](const int32 PercentEach, const bool bWithHeadquarters)
	{
		return TAttribute<FText>::CreateLambda([this, Card, PercentEach, bWithHeadquarters]()
		{
			const FCardState& Each = State(Card);
			return FText::Format(LOCTEXT("PlusValue", "+{0}"), APSUINumber::Number(
				PercentEach * (FMath::Max(Each.Level(), 0) + (bWithHeadquarters ? Each.Headquarters : 0))));
		});
	};
	const FText SecondsUnit = LOCTEXT("SecondsUnit", "s");
	const FText PercentUnit = LOCTEXT("PercentUnit", "%");
	switch (Card)
	{
	case ExplorationCard:
		Add(EAPSChromeGlyph::Compass, Seconds(EOrder::Survey, EDivision::Exploration, false), SecondsUnit,
			LOCTEXT("ChipSurvey", "SURVEY"));
		Add(EAPSChromeGlyph::Station, Seconds(EOrder::Survey, EDivision::Exploration, true), SecondsUnit,
			LOCTEXT("ChipWithStation", "WITH STATION"));
		break;
	case IndustryCard:
		Add(EAPSChromeGlyph::World, Seconds(EOrder::BuildOutpost, EDivision::Construction, false), SecondsUnit,
			LOCTEXT("ChipOutpost", "OUTPOST"));
		Add(EAPSChromeGlyph::Station, Seconds(EOrder::BuildStation, EDivision::Construction, false), SecondsUnit,
			LOCTEXT("ChipStation", "STATION"));
		Add(EAPSChromeGlyph::Shipyard, Seconds(EOrder::BuildShipyard, EDivision::Construction, false), SecondsUnit,
			LOCTEXT("ChipShipyard", "SHIPYARD"));
		Add(EAPSChromeGlyph::Headquarters, Seconds(EOrder::BuildHeadquarters, EDivision::Construction, false), SecondsUnit,
			LOCTEXT("ChipSectorHq", "SECTOR HQ"));
		Add(EAPSChromeGlyph::Ship, PerLevel(20, false), PercentUnit, LOCTEXT("ChipSlipways", "SLIPWAY SPEED"));
		break;
	case ScienceCard:
		Add(EAPSChromeGlyph::Planet, Seconds(EOrder::Survey, EDivision::Science, false), SecondsUnit,
			LOCTEXT("ChipStudy", "STUDY"), LOCTEXT("ChipStudyNote", "life, geology, metals"));
		Add(EAPSChromeGlyph::Station, Seconds(EOrder::Survey, EDivision::Science, true), SecondsUnit,
			LOCTEXT("ChipWithStation", "WITH STATION"));
		break;
	case CivilAffairsCard:
		Add(EAPSChromeGlyph::Infrastructure, PerLevel(15, false), PercentUnit, LOCTEXT("ChipModules", "BUILD SPEED"),
			LOCTEXT("ChipModulesNote", "colony modules"));
		break;
	case MilitaryCard:
		Add(EAPSChromeGlyph::Ship, PerLevel(15, false), PercentUnit, LOCTEXT("ChipMainFleet", "MAIN FLEET SPEED"),
			LOCTEXT("ChipMainFleetNote", "the line answers first"));
		break;
	default:
		Add(EAPSChromeGlyph::Fleet, PerLevel(10, true), PercentUnit, LOCTEXT("ChipShipSpeed", "SHIP SPEED"),
			LOCTEXT("ChipShipSpeedNote", "every ship"));
		Add(EAPSChromeGlyph::Headquarters,
			TAttribute<FText>::CreateLambda([this, Card]() { return APSUINumber::Number(State(Card).Headquarters); }),
			FText::GetEmpty(), LOCTEXT("ChipHeadquarters", "SECTOR HQS"),
			TAttribute<FText>::CreateLambda([this, Card]()
			{
				return FText::Format(LOCTEXT("ChipHeadquartersNote", "+{0}% of the speed"),
					APSUINumber::Number(10 * State(Card).Headquarters));
			}));
		break;
	}
	return Chips;
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildGrowth(const int32 Card)
{
	using namespace APSDivisionsPanelPrivate;
	const int32 Step = WorkPerLevel(Card);
	const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			RowLabel(LOCTEXT("GrowthRow", "GROWTH"))
		];
	if (Step <= 0)
	{
		Row->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("FixedLevel", "FIXED: SET WHEN THE CIVILIZATION WAS FOUNDED"))
			.Font(Font("Bold", 9)).ColorAndOpacity(Muted())
		];
	}
	else
	{
		// One segment per piece of work a level takes, lit as the work is done.
		const TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
		for (int32 Segment = 0; Segment < Step; ++Segment)
		{
			Bar->AddSlot().FillWidth(1.0f).Padding(Segment > 0 ? 3.0f : 0.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this, Card, Segment]()
				{
					return FSlateColor(Segment < GrowthDone(Card) ? Success() : CyanDim());
				})
			];
		}
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(96.0f).HeightOverride(8.0f)
			[
				Bar
			]
		];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 13)))
			.Text_Lambda([this, Card, Step]()
			{
				return FText::Format(LOCTEXT("GrowthFraction", "{0} / {1}"), APSUINumber::Number(GrowthDone(Card)),
					APSUINumber::Number(Step));
			})
		];
		Row->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(CardSpec(Card).WorkUnit).Font(Font("Bold", 9)).ColorAndOpacity(White())
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				.Text_Lambda([this, Card]() { return GrowthDetail(Card); })
			]
		];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Success())
			.Text_Lambda([this, Card]()
			{
				const FCardState& Each = State(Card);
				return Each.Earned >= MaxEarned
					? FText::Format(LOCTEXT("GrowthAllEarned", "ALL +{0} EARNED"), APSUINumber::Number(MaxEarned))
					: FText::Format(LOCTEXT("GrowthNext", "NEXT: LEVEL {0}"), APSUINumber::Number(Each.Level() + 1));
			})
		];
	}
	return SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
		.ToolTipText_Lambda([this, Card]() { return GrowthText(Card); })
		[
			Row
		];
}

TSharedRef<SWidget> SAPSDivisionsPanel::BuildShips(const int32 Card, const FLinearColor& Accent)
{
	using namespace APSDivisionsPanelPrivate;
	if (Card == CivilAffairsCard)
	{
		// Civil affairs commands no ships.
		return SNew(SBox).Visibility(EVisibility::Collapsed);
	}
	return SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(0.0f)
		.ToolTipText_Lambda([this, Card]() { return ShipsText(Card); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				RowLabel(LOCTEXT("ShipsRow", "SHIPS"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SHullRow).Accent(Accent)
				.Ships_Lambda([this, Card]() { return State(Card).Ships; })
				.UnderOrders_Lambda([this, Card]() { return State(Card).UnderOrders; })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 13)))
				.Text_Lambda([this, Card]() { return APSUINumber::Number(State(Card).Ships); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
				.Text(Card == FleetCommandCard ? LOCTEXT("ShipsInFleet", "IN THE FLEET") : LOCTEXT("ShipsAssigned", "ASSIGNED"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 13)).ColorAndOpacity(Accent).RenderTransform(CapsCenterShift(Font("Bold", 13)))
				.Text_Lambda([this, Card]() { return APSUINumber::Number(State(Card).UnderOrders); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("ShipsUnderOrders", "UNDER ORDERS")).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
			]
		];
}

const SAPSDivisionsPanel::FCardState& SAPSDivisionsPanel::State(const int32 Card) const
{
	// Every gauge, chip and bar reads here: the six cards are read again once a frame, while the tab is painted.
	if (StatesFrame != GFrameCounter)
	{
		StatesFrame = GFrameCounter;
		ReadStates();
	}
	return States[FMath::Clamp(Card, 0, CardCount - 1)];
}

const UCivilization* SAPSDivisionsPanel::GetCivilization() const
{
	const UWorld* LiveWorld = World.Get();
	const UGameInstance* GameInstance = LiveWorld ? LiveWorld->GetGameInstance() : nullptr;
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	return Gameplay ? Gameplay->CurrentCivilization.Get() : nullptr;
}

void SAPSDivisionsPanel::ReadStates() const
{
	using namespace APSDivisionsPanelPrivate;
	using APSFleet::EDivision;
	for (FCardState& Each : States)
	{
		Each = FCardState();
	}
	const UCivilization* Civ = GetCivilization();
	const FAPSCivilizationDivisions Levels = Civ ? Civ->Divisions : FAPSCivilizationDivisions();
	States[ExplorationCard].Founding = Levels.Exploration;
	States[IndustryCard].Founding = Levels.Industry;
	States[ScienceCard].Founding = Levels.Science;
	States[CivilAffairsCard].Founding = Levels.CivilAffairs;
	States[MilitaryCard].Founding = Levels.Military;
	States[FleetCommandCard].Founding = Levels.FleetCommand;

	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	if (!Fleet)
	{
		return;
	}
	// The earned levels as the fleet's rules count them, and the tally they come from for the growth bars.
	const FAPSFleetCommand::FWorkTally Tally = Fleet->GetWorkTally();
	States[ExplorationCard].Earned = Fleet->GetEarnedLevel(EDivision::Exploration);
	States[ExplorationCard].Work = Tally.Surveyed;
	States[IndustryCard].Earned = Fleet->GetEarnedLevel(EDivision::Construction);
	States[IndustryCard].Work = Tally.Built;
	States[ScienceCard].Earned = Fleet->GetEarnedLevel(EDivision::Science);
	States[ScienceCard].Work = Tally.Studied + Tally.Investigated;
	States[ScienceCard].Studied = Tally.Studied;
	States[ScienceCard].Anomalies = Tally.Investigated;
	States[FleetCommandCard].Earned = Fleet->GetEarnedFleetCommandLevel();
	States[FleetCommandCard].Work = Tally.Launched;
	States[FleetCommandCard].Headquarters = Fleet->CountBuiltHeadquarters();
	// Ships by division as fleet orders (K) has them; fleet command counts every ship.
	for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
	{
		const int32 Busy = Unit.Order != APSFleet::EOrder::None ? 1 : 0;
		const int32 Owner = CardOf(Unit.Division);
		if (Owner != INDEX_NONE)
		{
			++States[Owner].Ships;
			States[Owner].UnderOrders += Busy;
		}
		++States[FleetCommandCard].Ships;
		States[FleetCommandCard].UnderOrders += Busy;
	}
}

int32 SAPSDivisionsPanel::GrowthDone(const int32 Card) const
{
	using namespace APSDivisionsPanelPrivate;
	const int32 Step = WorkPerLevel(Card);
	if (Step <= 0)
	{
		return 0;
	}
	const FCardState& Each = State(Card);
	return Each.Earned >= MaxEarned ? Step : FMath::Clamp(Each.Work - Each.Earned * Step, 0, Step);
}

FText SAPSDivisionsPanel::LevelText(const int32 Card) const
{
	const FCardState& Each = State(Card);
	return Each.Earned > 0
		? FText::Format(LOCTEXT("DivisionLevelEarned", "LEVEL {0}  (+{1} EARNED)"), APSUINumber::Number(Each.Level()),
			APSUINumber::Number(Each.Earned))
		: FText::Format(LOCTEXT("DivisionLevel", "LEVEL {0}"), APSUINumber::Number(Each.Level()));
}

FText SAPSDivisionsPanel::EffectText(const int32 Card) const
{
	using namespace APSDivisionsPanelPrivate;
	using APSFleet::EDivision;
	using APSFleet::EOrder;
	using APSFleet::WorkSeconds;
	const FCardState& Each = State(Card);
	const int32 Level = Each.Level();
	const auto Seconds = [](const double Value) { return APSUINumber::Number(FMath::RoundToInt(Value)); };
	switch (Card)
	{
	case ExplorationCard:
		return FText::Format(LOCTEXT("ExplorationEffect", "A SURVEY TAKES {0} S AT THE WORLD, {1} S WHERE A STATION STANDS"),
			Seconds(WorkSeconds(EOrder::Survey, EDivision::Exploration, Level)),
			Seconds(WorkSeconds(EOrder::Survey, EDivision::Exploration, Level, true)));
	case IndustryCard:
		return FText::Format(LOCTEXT("IndustryEffect", "OUTPOST {0} S  /  STATION {1} S  /  SHIPYARD {2} S  /  SECTOR HQ {3} S;  SLIPWAYS BUILD {4}% FASTER"),
			Seconds(WorkSeconds(EOrder::BuildOutpost, EDivision::Construction, Level)),
			Seconds(WorkSeconds(EOrder::BuildStation, EDivision::Construction, Level)),
			Seconds(WorkSeconds(EOrder::BuildShipyard, EDivision::Construction, Level)),
			Seconds(WorkSeconds(EOrder::BuildHeadquarters, EDivision::Construction, Level)),
			APSUINumber::Number(FMath::RoundToInt(20.0f * FMath::Max(Level, 0))));
	case ScienceCard:
		return FText::Format(LOCTEXT("ScienceEffect", "A STUDY (LIFE, GEOLOGY, METALS) TAKES {0} S, {1} S WHERE A STATION STANDS"),
			Seconds(WorkSeconds(EOrder::Survey, EDivision::Science, Level)),
			Seconds(WorkSeconds(EOrder::Survey, EDivision::Science, Level, true)));
	case CivilAffairsCard:
		return FText::Format(LOCTEXT("CivilEffect", "COLONY MODULES BUILD {0}% FASTER"),
			APSUINumber::Number(15 * FMath::Max(Level, 0)));
	case MilitaryCard:
		return FText::Format(LOCTEXT("MilitaryEffect", "MAIN FLEET SHIPS FLY {0}% FASTER: THE LINE ANSWERS FIRST"),
			APSUINumber::Number(15 * FMath::Max(Level, 0)));
	default:
		return FText::Format(LOCTEXT("FleetCommandEffect", "EVERY SHIP FLIES {0}% FASTER ({1}% OF IT FROM {2} SECTOR HQS)"),
			APSUINumber::Number(10 * (FMath::Max(Level, 0) + Each.Headquarters)), APSUINumber::Number(10 * Each.Headquarters),
			APSUINumber::Number(Each.Headquarters));
	}
}

FText SAPSDivisionsPanel::GrowthText(const int32 Card) const
{
	using namespace APSDivisionsPanelPrivate;
	const FCardState& Each = State(Card);
	switch (Card)
	{
	case ExplorationCard:
		return FText::Format(LOCTEXT("ExplorationGrowth", "GROWS: +1 per 3 worlds surveyed, up to +3. Surveyed so far: {0}."),
			APSUINumber::Number(Each.Work));
	case IndustryCard:
		return FText::Format(LOCTEXT("IndustryGrowth", "GROWS: +1 per 3 outposts, stations, shipyards or HQs built, up to +3. Built so far: {0}."),
			APSUINumber::Number(Each.Work));
	case ScienceCard:
		return FText::Format(LOCTEXT("ScienceGrowthAnomalies", "GROWS: +1 per 2 worlds studied or anomalies investigated (on foot counts twice), up to +3. So far: {0} studied, {1} from anomalies."),
			APSUINumber::Number(Each.Studied), APSUINumber::Number(Each.Anomalies));
	case FleetCommandCard:
		return FText::Format(LOCTEXT("FleetCommandGrowth", "GROWS: +1 per 4 ships launched from the slipways, up to +3. Launched so far: {0}."),
			APSUINumber::Number(Each.Work));
	default:
		return LOCTEXT("FixedGrowth", "Set when the civilization was founded.");
	}
}

FText SAPSDivisionsPanel::GrowthDetail(const int32 Card) const
{
	using namespace APSDivisionsPanelPrivate;
	const FCardState& Each = State(Card);
	switch (Card)
	{
	case ExplorationCard:
		return FText::Format(LOCTEXT("ExplorationSoFar", "{0} surveyed so far"), APSUINumber::Number(Each.Work));
	case IndustryCard:
		return FText::Format(LOCTEXT("IndustrySoFar", "Outposts, stations, shipyards or HQs: {0} so far"),
			APSUINumber::Number(Each.Work));
	case ScienceCard:
		return FText::Format(LOCTEXT("ScienceSoFar", "{0} worlds studied, {1} from anomalies (on foot counts twice)"),
			APSUINumber::Number(Each.Studied), APSUINumber::Number(Each.Anomalies));
	case FleetCommandCard:
		return FText::Format(LOCTEXT("FleetCommandSoFar", "From the slipways: {0} so far"), APSUINumber::Number(Each.Work));
	default:
		return FText::GetEmpty();
	}
}

FText SAPSDivisionsPanel::ShipsText(const int32 Card) const
{
	const FCardState& Each = State(Card);
	return FText::Format(LOCTEXT("DivisionShips", "SHIPS {0}  /  UNDER ORDERS {1}"), APSUINumber::Number(Each.Ships),
		APSUINumber::Number(Each.UnderOrders));
}

#undef LOCTEXT_NAMESPACE
