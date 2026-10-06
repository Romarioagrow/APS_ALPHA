#include "SAPSInfrastructurePanel.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSInfrastructurePanel"

namespace APSInfrastructurePanelPrivate
{
	using namespace APSChrome;

	/** Shapes of the map's icons past the catalogue's categories (APSInfrastructure::ECategory, 0..4). */
	constexpr uint8 ShapeFleetStation = 10;
	constexpr uint8 ShapeFleetShipyard = 11;
	constexpr uint8 ShapeFleetHeadquarters = 12;
	constexpr uint8 ShapeSettlement = 13;
	constexpr uint8 ShapeFleetOutpost = 14;

	/** Objects not painted keep this position and cannot be picked. */
	const FVector2D Unpainted(-1.0e9, -1.0e9);

	bool IsPainted(const FVector2D& Position)
	{
		return Position.X > -1.0e8;
	}

	/** Transport's colour: relay links, cargo lanes, the network's reach. */
	FLinearColor LinkColour()
	{
		return APSInfrastructure::DepartmentColour(APSInfrastructure::EDepartment::Transport);
	}

	const FSlateBrush* TileBrush()
	{
		// The terminal's metric tile: a dark inset with a quiet rim.
		// Rio 06.10: the shared tile follows the interface theme.
		return APSChrome::MetricTileBrush();
	}

	const FSlateBrush* Disc()
	{
		// The default half-height rounding makes a circle of a square box.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	bool IsTransport(const APSInfrastructure::FType& Type)
	{
		using APSInfrastructure::ECategory;
		return Type.Department == APSInfrastructure::EDepartment::Transport || Type.Category == ECategory::Transport
			|| Type.Category == ECategory::Relay;
	}

	/** "+4.0", "-1.5", "0.0": a rate per minute. */
	FText SignedRate(const float Rate)
	{
		FNumberFormattingOptions One;
		One.SetMinimumFractionalDigits(1);
		One.SetMaximumFractionalDigits(1);
		const FText Value = APSUINumber::Number(FMath::Abs(Rate), &One);
		return Rate > 0.05f ? FText::Format(LOCTEXT("Plus", "+{0}"), Value)
			: Rate < -0.05f ? FText::Format(LOCTEXT("Minus", "-{0}"), Value) : Value;
	}

	/** "+15%": a rule change. */
	FText SignedPercent(const float Share)
	{
		FNumberFormattingOptions Whole;
		Whole.SetMaximumFractionalDigits(0);
		return FText::Format(LOCTEXT("PlusPercent", "+{0}"), APSUINumber::Percent(Share, &Whole));
	}

	FText Au(const double Cm)
	{
		FNumberFormattingOptions Two;
		Two.SetMaximumFractionalDigits(2);
		return APSUINumber::Number(Cm / APSStars::AstronomicalUnitCm, &Two);
	}

	/** "90 S", "6 MIN". */
	FText Duration(const float Seconds)
	{
		return Seconds >= 120.0f
			? FText::Format(LOCTEXT("Minutes", "{0} MIN"), APSUINumber::Number(FMath::RoundToInt(Seconds / 60.0f)))
			: FText::Format(LOCTEXT("Seconds", "{0} S"), APSUINumber::Number(FMath::RoundToInt(Seconds)));
	}

	/** A place with its designation in front ("A3  NAME"), so lists read in the system's order. */
	FText SiteLabel(const AActor* Site)
	{
		const FString Designation = APSBodyDesignation::Of(Site);
		const FText Name = APSObjectActions::NameOf(Site);
		return Designation.IsEmpty() ? Name : FText::FromString(Designation + TEXT("  ") + Name.ToString());
	}

	FText BodyDetail(const APlanetaryBody* Body, const FAPSFleetCommand* Fleet)
	{
		FString Type = UEnum::GetDisplayValueAsText(Body->PlanetType).ToString().ToUpper();
		Type.RemoveFromEnd(TEXT(" PLANET"));
		return FText::Format(LOCTEXT("BodyDetail", "{0}  /  {1}  /  {2}"), Body->IsA<AMoon>() ? LOCTEXT("Moon", "MOON") : LOCTEXT("Planet", "PLANET"),
			FText::FromString(Type), Fleet ? APSFleet::SurveyName(Fleet->GetSurvey(Body)) : LOCTEXT("Known", "KNOWN"));
	}

	void Lines(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const TArray<FVector2D>& Points,
		const FLinearColor& Colour, const float Thickness)
	{
		TArray<FVector2f> Converted;
		Converted.Reserve(Points.Num());
		for (const FVector2D& Point : Points)
		{
			Converted.Add(FVector2f(Point));
		}
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), MoveTemp(Converted), ESlateDrawEffect::None, Colour,
			true, Thickness);
	}

	void Circle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 1.0)
		{
			return;
		}
		const int32 Segments = FMath::Clamp(FMath::RoundToInt(Radius * 0.6), 20, 160);
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 Step = 0; Step <= Segments; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Segments;
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		Lines(Out, Layer, Geometry, Points, Colour, Thickness);
	}

	/** A dashed circle: an edge or a reach, not an orbit. */
	void DashedCircle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 1.0)
		{
			return;
		}
		const int32 Count = FMath::Clamp(FMath::RoundToInt(Radius * 0.25), 16, 120);
		for (int32 Dash = 0; Dash < Count; ++Dash)
		{
			const double From = UE_TWO_PI * Dash / Count;
			const double To = From + UE_TWO_PI * 0.55 / Count;
			TArray<FVector2D> Points;
			for (int32 Step = 0; Step <= 3; ++Step)
			{
				const double Angle = FMath::Lerp(From, To, Step / 3.0);
				Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			Lines(Out, Layer, Geometry, Points, Colour, Thickness);
		}
	}

	void Dot(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour)
	{
		const float Size = static_cast<float>(Radius * 2.0);
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size, Size),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Centre.X - Radius), static_cast<float>(Centre.Y - Radius)))),
			Disc(), ESlateDrawEffect::None, Colour);
	}

	/** A dashed line: a cargo lane. */
	void Dashes(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& From,
		const FVector2D& To, const FLinearColor& Colour, const float Thickness)
	{
		const double Length = FVector2D::Distance(From, To);
		if (Length < 2.0)
		{
			return;
		}
		const FVector2D Along = (To - From) / Length;
		for (double Start = 0.0; Start < Length; Start += 10.0)
		{
			Lines(Out, Layer, Geometry, {From + Along * Start, From + Along * FMath::Min(Start + 6.0, Length)}, Colour, Thickness);
		}
	}

	void Label(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Position,
		const FText& Text, const FSlateFontInfo& FontInfo, const FLinearColor& Colour)
	{
		// Rio 03.10 ("everywhere the text strictly centred"): the capitals' middle where the line box's middle was.
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(FVector2f(320.0f, 20.0f),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y) + CapsCenterOffset(FontInfo)))),
			Text, FontInfo, ESlateDrawEffect::None, Colour);
	}

	/** An icon's outline by its shape, with a dot in its middle. */
	void DrawShape(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const uint8 Kind, const FLinearColor& Colour, const double Half)
	{
		using APSInfrastructure::ECategory;
		const auto Polygon = [&](const int32 Sides, const double Rotation)
		{
			TArray<FVector2D> Points;
			for (int32 Corner = 0; Corner <= Sides; ++Corner)
			{
				const double Angle = Rotation + UE_TWO_PI * Corner / Sides;
				Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Half);
			}
			Lines(Out, Layer, Geometry, Points, Colour, 1.8f);
		};
		switch (Kind)
		{
		case static_cast<uint8>(ECategory::Outpost):
		case ShapeFleetOutpost:
			Polygon(4, 0.0);
			break;
		case static_cast<uint8>(ECategory::Station):
		case ShapeFleetStation:
			Polygon(4, UE_PI * 0.25);
			break;
		case static_cast<uint8>(ECategory::Relay):
			Polygon(3, -UE_HALF_PI);
			break;
		case static_cast<uint8>(ECategory::Megastructure):
			Polygon(6, 0.0);
			break;
		case static_cast<uint8>(ECategory::Hub):
			// Rio 03.10: the huge hubs, an octagon round a ring.
			Polygon(8, UE_PI / 8.0);
			Circle(Out, Layer, Geometry, Centre, Half * 0.62, Colour, 1.2f);
			break;
		case ShapeFleetShipyard:
			Polygon(4, UE_PI * 0.25);
			Lines(Out, Layer, Geometry, {Centre + FVector2D(-Half * 0.7, 0.0), Centre + FVector2D(Half * 0.7, 0.0)}, Colour, 1.4f);
			break;
		case ShapeFleetHeadquarters:
			Polygon(5, -UE_HALF_PI);
			break;
		default:
			// Transport and settlements: a ring.
			Circle(Out, Layer, Geometry, Centre, Half, Colour, 1.8f);
			break;
		}
		Dot(Out, Layer, Geometry, Centre, FMath::Max(Half * 0.36, 1.4), Colour);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The network map

void SAPSInfrastructureMap::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	OnOpenObject = InArgs._OnOpenObject;
	// Icons and labels at the rim stay on the map, not on the lists beside it.
	SetClipping(EWidgetClipping::ClipToBounds);
	Refresh();
}

void SAPSInfrastructureMap::Refresh()
{
	using namespace APSInfrastructurePanelPrivate;
	Nodes.Reset();
	Icons.Reset();
	Lanes.Reset();
	HomeReach = 0.0;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		Summary = FText::GetEmpty();
		return;
	}
	AActor* Star = APSInfrastructureUI::HomeStar(LiveWorld);
	FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	const FAPSStarSystemInfo* HomeInfo = Stars && Stars->IsReady() ? Stars->GetHome() : nullptr;

	// The home system's planets; their orbital plane is the map's, as on the system map.
	TArray<APlanet*> Planets;
	FVector Normal = FVector::UpVector;
	bool bNormal = false;
	for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		Planets.Add(*It);
		if (const AActor* Orbit = It->GetAttachParentActor(); !bNormal && Orbit && Orbit->IsA<APlanetOrbit>())
		{
			Normal = Orbit->GetActorUpVector();
			bNormal = true;
		}
	}
	FVector PlaneU = FVector::VectorPlaneProject(FVector::ForwardVector, Normal).GetSafeNormal();
	if (PlaneU.IsNearlyZero())
	{
		PlaneU = FVector::VectorPlaneProject(FVector::RightVector, Normal).GetSafeNormal();
	}
	const FVector PlaneV = FVector::CrossProduct(Normal, PlaneU).GetSafeNormal();
	const auto InPlane = [&PlaneU, &PlaneV](const FVector& Offset)
	{
		return FVector2D(FVector::DotProduct(Offset, PlaneU), -FVector::DotProduct(Offset, PlaneV));
	};
	const FVector Origin = Star ? Star->GetActorLocation() : HomeInfo ? HomeInfo->Location : FVector::ZeroVector;

	// Inside the home system: logarithmic from the star to the system's edge at 1 (the system map's scale).
	double Nearest = TNumericLimits<double>::Max();
	double Farthest = 0.0;
	for (const APlanet* Planet : Planets)
	{
		const double Cm = FVector::Dist(Planet->GetActorLocation(), Origin);
		Nearest = FMath::Min(Nearest, Cm);
		Farthest = FMath::Max(Farthest, Cm);
	}
	const double Room = HomeInfo && HomeInfo->RoomCm > 0.0 ? HomeInfo->RoomCm : FMath::Max(Farthest * 1.25, 1.0e12);
	const double Knee = FMath::Max((Planets.IsEmpty() ? Room * 0.1 : Nearest) * 0.35, 1.0e9);
	const auto InnerRadius = [Room, Knee](const double DistanceCm)
	{
		return FMath::Min(0.12 + 0.82 * FMath::Loge(1.0 + DistanceCm / Knee) / FMath::Max(FMath::Loge(1.0 + Room / Knee), 1.0e-6), 0.95);
	};

	TMap<const AActor*, int32> NodeOfActor;
	TMap<FString, int32> NodeOfKey;
	{
		FNode Home;
		Home.Kind = ENodeKind::Home;
		Home.Actor = Star;
		Home.SystemId = HomeInfo ? HomeInfo->Id : FGuid();
		Home.Name = Star ? APSObjectActions::NameOf(Star) : HomeInfo ? FText::FromString(HomeInfo->Name) : LOCTEXT("HomeSystem", "HOME SYSTEM");
		Home.Detail = LOCTEXT("HomeDetail", "HOME SYSTEM  /  CLAIMED  /  THE NETWORK'S CENTRE");
		Home.Colour = FLinearColor(1.0f, 0.8f, 0.36f);
		Home.Knowledge = static_cast<uint8>(APSStars::EKnowledge::Surveyed);
		Home.bClaimed = true;
		Home.bInReach = true;
		Nodes.Add(Home);
		if (Star)
		{
			NodeOfActor.Add(Star, 0);
		}
	}
	for (APlanet* Planet : Planets)
	{
		const FVector Offset = Planet->GetActorLocation() - Origin;
		FVector2D Direction = InPlane(Offset);
		if (!Direction.Normalize())
		{
			Direction = FVector2D(1.0, 0.0);
		}
		FNode Node;
		Node.Kind = ENodeKind::Planet;
		Node.Actor = Planet;
		Node.Name = SiteLabel(Planet);
		Node.Detail = BodyDetail(Planet, Fleet);
		Node.Colour = APSInfrastructureUI::ColourOf(Planet);
		Node.Position = Direction * InnerRadius(Offset.Size());
		Node.Knowledge = static_cast<uint8>(Fleet ? Fleet->GetSurvey(Planet) : APSFleet::ESurvey::Studied);
		const int32 Added = Nodes.Add(Node);
		NodeOfActor.Add(Planet, Added);
		NodeOfKey.Add(FAPSFleetCommand::KeyOf(Planet), Added);
	}
	// Moons beside their planets, in their own directions: a schematic, their orbits are far below the map's scale.
	TArray<AMoon*> Moons;
	for (TActorIterator<AMoon> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && It->ParentPlanet && NodeOfActor.Contains(It->ParentPlanet))
		{
			Moons.Add(*It);
		}
	}
	Moons.Sort([](const AMoon& A, const AMoon& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), A.ParentPlanet->GetActorLocation())
			< FVector::DistSquared(B.GetActorLocation(), B.ParentPlanet->GetActorLocation());
	});
	TMap<const AActor*, int32> MoonsAround;
	for (AMoon* Moon : Moons)
	{
		const int32 Parent = NodeOfActor.FindChecked(Moon->ParentPlanet);
		const int32 Ordinal = MoonsAround.FindOrAdd(Moon->ParentPlanet)++;
		FVector2D Direction = InPlane(Moon->GetActorLocation() - Moon->ParentPlanet->GetActorLocation());
		if (!Direction.Normalize())
		{
			Direction = FVector2D(FMath::Cos(Ordinal * 1.3), FMath::Sin(Ordinal * 1.3));
		}
		FNode Node;
		Node.Kind = ENodeKind::Moon;
		Node.Actor = Moon;
		Node.Name = SiteLabel(Moon);
		Node.Detail = BodyDetail(Moon, Fleet);
		Node.Colour = APSInfrastructureUI::ColourOf(Moon);
		Node.Position = Nodes[Parent].Position;
		Node.Fan = Direction * (15.0 + 7.0 * Ordinal);
		Node.Knowledge = static_cast<uint8>(Fleet ? Fleet->GetSurvey(Moon) : APSFleet::ESurvey::Studied);
		const int32 Added = Nodes.Add(Node);
		NodeOfActor.Add(Moon, Added);
		NodeOfKey.Add(FAPSFleetCommand::KeyOf(Moon), Added);
	}

	// Around the home system: the systems the civilization knows or holds, and the nearest uncharted ones (the next
	// probes), logarithmic by their distance from home.
	TMap<int32, int32> NodeOfSystem;
	if (Stars && HomeInfo)
	{
		const int32 HomeIndex = Stars->GetHomeIndex();
		NodeOfSystem.Add(HomeIndex, 0);
		TArray<int32> Shown;
		Stars->GetKnown(Shown);
		TArray<int32> Neighbours;
		Stars->FindNearest(HomeInfo->Location, 10, Neighbours);
		int32 Uncharted = 0;
		for (const int32 Neighbour : Neighbours)
		{
			if (Uncharted < 6 && Neighbour != HomeIndex && !Shown.Contains(Neighbour))
			{
				Shown.Add(Neighbour);
				++Uncharted;
			}
		}
		Shown.Remove(HomeIndex);
		double MaxDistance = Room * 2.0;
		for (const int32 Each : Shown)
		{
			if (const FAPSStarSystemInfo* Info = Stars->Get(Each))
			{
				MaxDistance = FMath::Max(MaxDistance, Info->HomeDistanceCm * 1.08);
			}
		}
		const auto OuterRadius = [Room, MaxDistance](const double DistanceCm)
		{
			return 1.14 + 0.92 * FMath::Loge(FMath::Max(DistanceCm, Room) / Room) / FMath::Max(FMath::Loge(MaxDistance / Room), 1.0e-6);
		};
		const int32 FirstSystem = Nodes.Num();
		for (const int32 Each : Shown)
		{
			const FAPSStarSystemInfo* Info = Stars->Get(Each);
			if (!Info)
			{
				continue;
			}
			const FAPSStarSystemState State = Stars->GetState(Info->Id);
			FVector2D Direction = InPlane(Info->Location - HomeInfo->Location);
			if (!Direction.Normalize())
			{
				Direction = FVector2D(0.0, -1.0);
			}
			FNode Node;
			Node.Kind = ENodeKind::System;
			Node.SystemId = Info->Id;
			Node.Name = FText::FromString(Info->Name);
			Node.Colour = Info->Colour;
			Node.Knowledge = static_cast<uint8>(State.Knowledge);
			Node.bClaimed = State.bClaimed;
			Node.bInReach = Stars->IsInReach(Info->Id);
			Node.Position = Direction * OuterRadius(Info->HomeDistanceCm);
			Node.Detail = FText::Format(LOCTEXT("SystemDetail", "{0}  /  {1}{2}  /  {3} AU FROM HOME{4}"),
				FText::FromString(Info->Spectral.IsEmpty() ? FString(TEXT("STAR")) : Info->Spectral), APSStars::KnowledgeName(State.Knowledge),
				State.bClaimed ? LOCTEXT("SystemClaimed", "  /  CLAIMED") : FText::GetEmpty(), Au(Info->HomeDistanceCm),
				Node.bInReach ? LOCTEXT("SystemInReach", "  /  IN THE NETWORK'S REACH") : FText::GetEmpty());
			// A claimed system's relay reach as a ring (approximate: the map is radial from home, not from it).
			if (const double ReachCm = State.bClaimed ? Stars->ReachCm(Each) : 0.0; ReachCm > 0.0)
			{
				Node.ReachUnits = 0.5 * (OuterRadius(Info->HomeDistanceCm + ReachCm)
					- OuterRadius(FMath::Max(Info->HomeDistanceCm - ReachCm, Room)));
			}
			NodeOfSystem.Add(Each, Nodes.Add(Node));
		}
		// Systems close on the map are pushed apart, so each stays readable and clickable.
		for (int32 Pass = 0; Pass < 8; ++Pass)
		{
			for (int32 First = FirstSystem; First < Nodes.Num(); ++First)
			{
				for (int32 Second = First + 1; Second < Nodes.Num(); ++Second)
				{
					FVector2D Apart = Nodes[Second].Position - Nodes[First].Position;
					const double Gap = Apart.Size();
					if (Gap >= 0.14)
					{
						continue;
					}
					Apart = Gap > 1.0e-6 ? Apart / Gap : FVector2D(0.0, 1.0);
					const FVector2D Push = Apart * (0.14 - Gap) * 0.5;
					Nodes[First].Position -= Push;
					Nodes[Second].Position += Push;
				}
			}
		}
		// The home network's reach: exact, the map is radial from home.
		const double HomeReachCm = Stars->ReachCm(HomeIndex);
		HomeReach = HomeReachCm <= 0.0 ? 0.0 : HomeReachCm <= Room ? InnerRadius(HomeReachCm) : OuterRadius(HomeReachCm);
		TArray<TPair<int32, int32>> Links;
		Stars->GetNetwork(Links);
		for (const TPair<int32, int32>& Link : Links)
		{
			const int32* From = NodeOfSystem.Find(Link.Key);
			const int32* To = NodeOfSystem.Find(Link.Value);
			if (From && To)
			{
				Lanes.Add(FLane{*From, *To, true});
			}
		}
	}

	// What stands at each place: the catalogue's structures in their departments' colours, then the fleet's stations,
	// shipyards, HQs and outposts, and the settlements on the ground.
	const auto Mark = [this](const int32 At, const bool bTransport)
	{
		Nodes[At].bHeld = true;
		Nodes[At].bTransport = Nodes[At].bTransport || bTransport;
	};
	TArray<int32> CargoHubs;
	int32 StructureCount = 0;
	int32 TransportCount = 0;
	if (Infrastructure)
	{
		for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built.Type);
			int32 At = INDEX_NONE;
			if (Built.SystemId.IsValid())
			{
				const int32* InSystem = Stars ? NodeOfSystem.Find(Stars->IndexOf(Built.SystemId)) : nullptr;
				At = InSystem ? *InSystem : HomeInfo && Built.SystemId == HomeInfo->Id ? 0 : INDEX_NONE;
			}
			else if (const int32* AtBody = NodeOfKey.Find(Built.SiteKey))
			{
				At = *AtBody;
			}
			if (!Type || At == INDEX_NONE)
			{
				continue;
			}
			FIcon Icon;
			Icon.Node = At;
			Icon.Actor = Built.Actor;
			Icon.Name = Built.Actor.IsValid() ? APSObjectActions::NameOf(Built.Actor.Get()) : Type->Name;
			Icon.Detail = FText::Format(LOCTEXT("IconDetail", "{0}  /  {1}"), APSInfrastructure::CategoryName(Type->Category),
				APSInfrastructure::DepartmentName(Type->Department));
			Icon.Colour = APSInfrastructure::DepartmentColour(Type->Department);
			Icon.Shape = static_cast<uint8>(Type->Category);
			Icon.bTransport = IsTransport(*Type);
			Icon.bLarge = Type->bMegastructure || Type->Category == APSInfrastructure::ECategory::Hub;
			Icons.Add(Icon);
			Mark(At, Icon.bTransport);
			++StructureCount;
			TransportCount += Icon.bTransport ? 1 : 0;
			// A cargo hub moves goods between the worlds of its system.
			if (Type->Category == APSInfrastructure::ECategory::Transport && Type->Placement == APSInfrastructure::EPlacement::Orbit)
			{
				CargoHubs.AddUnique(At);
			}
		}
	}
	if (Fleet)
	{
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			AActor* Actor = Structure.Actor.Get();
			const int32* At = NodeOfActor.Find(Structure.Body.Get());
			// Catalogue structures stand above with their department.
			if (!Actor || !At || (Infrastructure && Infrastructure->FindByActor(Actor)))
			{
				continue;
			}
			FIcon Icon;
			Icon.Node = *At;
			Icon.Actor = Actor;
			Icon.Name = APSObjectActions::NameOf(Actor);
			Icon.Detail = FText::Format(LOCTEXT("FleetIconDetail", "{0}  /  {1}"), APSFleet::StructureName(Structure.Kind),
				Structure.bBuilt ? LOCTEXT("BuiltByFleet", "BUILT BY THE FLEET") : LOCTEXT("Founded", "FOUNDED WITH THE CIVILIZATION"));
			Icon.Colour = Cyan();
			Icon.Shape = Structure.Kind == APSFleet::EStructure::Shipyard ? ShapeFleetShipyard
				: Structure.Kind == APSFleet::EStructure::Headquarters ? ShapeFleetHeadquarters : ShapeFleetStation;
			Icons.Add(Icon);
			Mark(*At, false);
			++StructureCount;
		}
		for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
		{
			const int32* At = NodeOfActor.Find(Record.Body.Get());
			if (!At)
			{
				continue;
			}
			for (const TWeakObjectPtr<AActor>& Outpost : Record.Outposts)
			{
				if (!Outpost.IsValid())
				{
					continue;
				}
				FIcon Icon;
				Icon.Node = *At;
				Icon.Actor = Outpost;
				Icon.Name = APSObjectActions::NameOf(Outpost.Get());
				Icon.Detail = LOCTEXT("OutpostIconDetail", "OUTPOST  /  BUILT BY THE FLEET");
				Icon.Colour = Amber();
				Icon.Shape = ShapeFleetOutpost;
				Icons.Add(Icon);
				Mark(*At, false);
				++StructureCount;
			}
		}
	}
	for (TActorIterator<AColony> It(LiveWorld); It; ++It)
	{
		const int32* At = IsValid(*It) ? NodeOfActor.Find(APSInfrastructureUI::BodyOf(*It)) : nullptr;
		if (!At)
		{
			continue;
		}
		const bool bHomeColony = It->ActorHasTag(TEXT("APS.Civilization.Materialized"));
		FIcon Icon;
		Icon.Node = *At;
		Icon.Actor = *It;
		Icon.Name = bHomeColony ? LOCTEXT("HomeColony", "HOME COLONY") : APSObjectActions::NameOf(*It);
		Icon.Detail = bHomeColony ? LOCTEXT("HomeColonyDetail", "THE COLONY  /  ON THE GROUND") : LOCTEXT("SettlementDetail", "SETTLEMENT  /  ON THE GROUND");
		Icon.Colour = FLinearColor(0.36f, 1.0f, 0.58f);
		Icon.Shape = ShapeSettlement;
		Icons.Add(Icon);
		Mark(*At, false);
	}
	for (const int32 Hub : CargoHubs)
	{
		for (int32 Other = 0; Other < Nodes.Num(); ++Other)
		{
			if (Other != Hub && Nodes[Other].Kind != ENodeKind::System && Nodes[Other].bHeld)
			{
				Lanes.Add(FLane{Hub, Other, false});
			}
		}
	}

	int32 Known = 0;
	int32 Claimed = 0;
	int32 RelayLinks = 0;
	for (const FNode& Node : Nodes)
	{
		if (Node.Kind == ENodeKind::System && (Node.Knowledge > 0 || Node.bClaimed))
		{
			++Known;
			Claimed += Node.bClaimed ? 1 : 0;
		}
	}
	for (const FLane& Lane : Lanes)
	{
		RelayLinks += Lane.bRelay ? 1 : 0;
	}
	Summary = FText::Format(LOCTEXT("MapSummary", "STRUCTURES {0}  /  CLAIMED {1} OF {2} KNOWN SYSTEMS  /  RELAY LINKS {3}  /  TRANSPORT AND RELAYS {4}"),
		APSUINumber::Number(StructureCount), APSUINumber::Number(Claimed), APSUINumber::Number(Known), APSUINumber::Number(RelayLinks),
		APSUINumber::Number(TransportCount));
}

double SAPSInfrastructureMap::UnitPixels(const FVector2D& Size) const
{
	// The farthest systems sit near 2.06 units; the whole map fits at x1.
	return FMath::Max(FMath::Min(Size.X, Size.Y) * 0.5 - 34.0, 40.0) / 2.12 * MapZoom;
}

int32 SAPSInfrastructureMap::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace APSInfrastructurePanelPrivate;
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const double Unit = UnitPixels(Size);
	const FVector2D Centre = Size * 0.5 + MapOffset;
	const double Spread = FMath::Sqrt(MapZoom);
	const FSlateFontInfo LabelFont = Font(TEXT("Bold"), 9);
	const FSlateFontInfo SmallFont = Font(TEXT("Regular"), 9);
	const FLinearColor Link = LinkColour();
	const FLinearColor Edge = APSUITheme::RetintHighlight(FLinearColor(0.26f, 0.84f, 0.93f, 0.28f));
	NodePixels.SetNum(Nodes.Num());
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		NodePixels[Index] = Centre + Nodes[Index].Position * Unit + Nodes[Index].Fan * Spread;
	}
	IconPixels.Init(Unpainted, Icons.Num());

	// The home system's edge, and how far the network reaches from home.
	DashedCircle(OutDrawElements, LayerId, AllottedGeometry, Centre, Unit, Edge, 1.0f);
	Label(OutDrawElements, LayerId + 1, AllottedGeometry, Centre + FVector2D(0.70710678, -0.70710678) * Unit + FVector2D(6.0, -14.0),
		LOCTEXT("HomeEdge", "EDGE OF THE HOME SYSTEM"), SmallFont, Edge * FLinearColor(1.0f, 1.0f, 1.0f, 2.2f));
	if (HomeReach > 0.0)
	{
		DashedCircle(OutDrawElements, LayerId, AllottedGeometry, Centre, Unit * HomeReach, Link.CopyWithNewOpacity(0.32f), 1.4f);
		Label(OutDrawElements, LayerId + 1, AllottedGeometry, Centre + FVector2D(-0.70710678, 0.70710678) * Unit * HomeReach + FVector2D(-150.0, 4.0),
			LOCTEXT("HomeReach", "THE NETWORK'S REACH"), SmallFont, Link.CopyWithNewOpacity(0.7f));
	}
	// Planet orbits around the star, and the relay reach of the other claimed systems.
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FNode& Node = Nodes[Index];
		if (Node.Kind == ENodeKind::Planet)
		{
			Circle(OutDrawElements, LayerId, AllottedGeometry, Centre, Node.Position.Size() * Unit,
				FLinearColor(Node.Colour.R, Node.Colour.G, Node.Colour.B, 0.14f), 1.0f);
		}
		else if (Node.Kind == ENodeKind::System && Node.ReachUnits > 0.0)
		{
			DashedCircle(OutDrawElements, LayerId, AllottedGeometry, NodePixels[Index], Node.ReachUnits * Unit,
				Link.CopyWithNewOpacity(0.2f), 1.0f);
		}
	}

	// Relay links with traffic running along them; a cargo hub's lanes dashed.
	const double Now = FSlateApplication::Get().GetCurrentTime();
	for (const FLane& Lane : Lanes)
	{
		if (!NodePixels.IsValidIndex(Lane.From) || !NodePixels.IsValidIndex(Lane.To))
		{
			continue;
		}
		const FVector2D From = NodePixels[Lane.From];
		const FVector2D To = NodePixels[Lane.To];
		if (Lane.bRelay)
		{
			Lines(OutDrawElements, LayerId + 2, AllottedGeometry, {From, To}, Link.CopyWithNewOpacity(0.55f), 2.4f);
			for (int32 Pulse = 0; Pulse < 3; ++Pulse)
			{
				const double Along = FMath::Frac(Now * 0.18 + Pulse / 3.0);
				Dot(OutDrawElements, LayerId + 3, AllottedGeometry, FMath::Lerp(From, To, Along), 2.6, Link);
			}
		}
		else
		{
			Dashes(OutDrawElements, LayerId + 2, AllottedGeometry, From, To, Link.CopyWithNewOpacity(0.42f), 1.2f);
		}
	}

	// The places.
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FNode& Node = Nodes[Index];
		const FVector2D At = NodePixels[Index];
		switch (Node.Kind)
		{
		case ENodeKind::Home:
			Dot(OutDrawElements, LayerId + 3, AllottedGeometry, At, 15.0, Node.Colour.CopyWithNewOpacity(0.22f));
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, At, 8.0, Node.Colour);
			break;
		case ENodeKind::Planet:
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, At, 6.5, Node.Colour);
			break;
		case ENodeKind::Moon:
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, At, 4.0, Node.Colour);
			break;
		case ENodeKind::System:
		{
			const bool bCharted = Node.Knowledge > 0 || Node.bClaimed;
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, At, bCharted ? 6.0 : 3.5,
				bCharted ? Node.Colour : Node.Colour.CopyWithNewOpacity(0.45f));
			if (bCharted)
			{
				Circle(OutDrawElements, LayerId + 4, AllottedGeometry, At, 10.0,
					APSStars::KnowledgeColour(static_cast<APSStars::EKnowledge>(Node.Knowledge)), 1.4f);
			}
			else
			{
				DashedCircle(OutDrawElements, LayerId + 4, AllottedGeometry, At, 9.0, Muted().CopyWithNewOpacity(0.4f), 1.0f);
			}
			if (Node.bClaimed)
			{
				// The civilization's beacon burns there.
				DrawShape(OutDrawElements, LayerId + 5, AllottedGeometry, At + FVector2D(12.0, -12.0),
					static_cast<uint8>(APSInfrastructure::ECategory::Relay), Link, 4.5);
			}
			break;
		}
		default:
			break;
		}
		// Unsurveyed worlds read dim; transport and relays are ringed.
		if ((Node.Kind == ENodeKind::Planet || Node.Kind == ENodeKind::Moon) && Node.Knowledge == 0)
		{
			Circle(OutDrawElements, LayerId + 4, AllottedGeometry, At, Node.Kind == ENodeKind::Moon ? 6.5 : 9.5,
				Muted().CopyWithNewOpacity(0.45f), 1.0f);
		}
		if (Node.bTransport)
		{
			Circle(OutDrawElements, LayerId + 4, AllottedGeometry, At, Node.Kind == ENodeKind::Home ? 19.0 : 13.0, Link, 2.0f);
		}
	}

	// What stands at each place, around it: ten on the first ring, the rest on a wider one.
	TArray<int32> Placed;
	Placed.Init(0, Nodes.Num());
	for (int32 Index = 0; Index < Icons.Num(); ++Index)
	{
		const FIcon& Icon = Icons[Index];
		if (!NodePixels.IsValidIndex(Icon.Node))
		{
			continue;
		}
		const int32 Ordinal = Placed[Icon.Node]++;
		const ENodeKind Kind = Nodes[Icon.Node].Kind;
		const double Base = Kind == ENodeKind::Home ? 23.0 : Kind == ENodeKind::Moon ? 11.0 : 15.0;
		const int32 Ring = Ordinal / 10;
		const double Angle = -UE_PI * 0.85 + (Ordinal % 10) * 0.36 + Ring * 0.18;
		const FVector2D At = NodePixels[Icon.Node] + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Base + Ring * 11.0);
		IconPixels[Index] = At;
		const double Half = Icon.bLarge ? 6.5 : 4.5;
		if (Icon.bTransport)
		{
			Circle(OutDrawElements, LayerId + 5, AllottedGeometry, At, Half + 3.5, Link.CopyWithNewOpacity(0.75f), 1.2f);
		}
		DrawShape(OutDrawElements, LayerId + 5, AllottedGeometry, At, Icon.Shape, Icon.Colour, Half);
	}

	// Names: the star, planets and systems always, moons once zoomed in; a name that would overlap one is left to the
	// hover.
	TArray<FSlateRect> PlacedLabels;
	const auto PlaceLabel = [&PlacedLabels](const FVector2D& At, const FText& Text)
	{
		const FSlateRect Rect(At.X, At.Y, At.X + 7.0 * FMath::Max(Text.ToString().Len(), 4), At.Y + 13.0);
		for (const FSlateRect& Other : PlacedLabels)
		{
			if (Rect.Left < Other.Right + 4.0 && Other.Left < Rect.Right + 4.0 && Rect.Top < Other.Bottom && Other.Top < Rect.Bottom)
			{
				return false;
			}
		}
		PlacedLabels.Add(Rect);
		return true;
	};
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FNode& Node = Nodes[Index];
		if (Node.Kind == ENodeKind::Moon && MapZoom < 2.5)
		{
			continue;
		}
		const FVector2D At = NodePixels[Index] + FVector2D(-28.0, Node.Kind == ENodeKind::Home ? 17.0 : 11.0);
		if (!PlaceLabel(At, Node.Name))
		{
			continue;
		}
		const bool bFaint = Node.Kind == ENodeKind::System && Node.Knowledge == 0 && !Node.bClaimed;
		Label(OutDrawElements, LayerId + 6, AllottedGeometry, At, Node.Name, LabelFont,
			bFaint ? Muted().CopyWithNewOpacity(0.65f) : FLinearColor(Node.Colour.R, Node.Colour.G, Node.Colour.B, 0.95f));
	}

	// The hovered place or structure: a ring and what it is, with how to open it.
	FVector2D TipAt = Unpainted;
	FText TipName;
	FText TipDetail;
	if (Icons.IsValidIndex(HoverIcon) && IsPainted(IconPixels[HoverIcon]))
	{
		TipAt = IconPixels[HoverIcon];
		TipName = Icons[HoverIcon].Name;
		TipDetail = Icons[HoverIcon].Detail;
		Circle(OutDrawElements, LayerId + 7, AllottedGeometry, TipAt, 9.0, White(), 1.4f);
	}
	else if (Nodes.IsValidIndex(HoverNode) && NodePixels.IsValidIndex(HoverNode))
	{
		TipAt = NodePixels[HoverNode];
		TipName = Nodes[HoverNode].Name;
		TipDetail = Nodes[HoverNode].Detail;
		Circle(OutDrawElements, LayerId + 7, AllottedGeometry, TipAt, 15.0, Cyan(), 1.4f);
	}
	if (IsPainted(TipAt))
	{
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FSlateFontInfo TipFont = Font(TEXT("Bold"), 10);
		const FText TipHint = LOCTEXT("TipHint", "CLICK: ITS PAGE");
		const FVector2D NameSize = Measure->Measure(TipName, TipFont);
		const FVector2D DetailSize = Measure->Measure(TipDetail, SmallFont);
		const FVector2D HintSize = Measure->Measure(TipHint, LabelFont);
		const FVector2D BoxSize(FMath::Max3(NameSize.X, DetailSize.X, HintSize.X) + 18.0, NameSize.Y + DetailSize.Y + HintSize.Y + 16.0);
		FVector2D BoxAt = TipAt + FVector2D(18.0, 10.0);
		BoxAt.X = FMath::Max(4.0, FMath::Min(BoxAt.X, Size.X - BoxSize.X - 4.0));
		BoxAt.Y = FMath::Max(4.0, FMath::Min(BoxAt.Y, Size.Y - BoxSize.Y - 4.0));
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 8, AllottedGeometry.ToPaintGeometry(FVector2f(BoxSize),
			FSlateLayoutTransform(FVector2f(BoxAt))), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
			APSUITheme::Retint(FLinearColor(0.01f, 0.035f, 0.05f, 0.94f)));
		// Each line by its capitals' middle on the plate (Rio 03.10), not by Slate's line box.
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 9, AllottedGeometry.ToPaintGeometry(FVector2f(NameSize),
			FSlateLayoutTransform(FVector2f(BoxAt + FVector2D(9.0, 5.0 + CapsCenterOffset(TipFont))))), TipName, TipFont,
			ESlateDrawEffect::None, White());
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 9, AllottedGeometry.ToPaintGeometry(FVector2f(DetailSize),
			FSlateLayoutTransform(FVector2f(BoxAt + FVector2D(9.0, 7.0 + NameSize.Y + CapsCenterOffset(SmallFont))))), TipDetail,
			SmallFont, ESlateDrawEffect::None, Muted());
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 9, AllottedGeometry.ToPaintGeometry(FVector2f(HintSize),
			FSlateLayoutTransform(FVector2f(BoxAt + FVector2D(9.0, 9.0 + NameSize.Y + DetailSize.Y + CapsCenterOffset(LabelFont))))),
			TipHint, LabelFont, ESlateDrawEffect::None, Amber());
	}

	// Legend: the departments' colours, the shapes, the network's marks.
	{
		const int32 Count = static_cast<int32>(APSInfrastructure::EDepartment::Count);
		FVector2D Row(12.0, Size.Y - 17.0 * Count - 10.0);
		for (int32 Department = 0; Department < Count; ++Department)
		{
			const APSInfrastructure::EDepartment Kind = static_cast<APSInfrastructure::EDepartment>(Department);
			Dot(OutDrawElements, LayerId + 8, AllottedGeometry, Row + FVector2D(4.0, 8.0), 3.5, APSInfrastructure::DepartmentColour(Kind));
			Label(OutDrawElements, LayerId + 8, AllottedGeometry, Row + FVector2D(14.0, 0.0), APSInfrastructure::DepartmentName(Kind),
				SmallFont, Muted());
			Row.Y += 17.0;
		}
		FVector2D Shapes(140.0, Size.Y - 17.0 * 7 - 10.0);
		struct FShapeName
		{
			uint8 Kind;
			FText Name;
		};
		const FShapeName ShapeNames[] = {
			{static_cast<uint8>(APSInfrastructure::ECategory::Outpost), LOCTEXT("LegendOutpost", "OUTPOST")},
			{static_cast<uint8>(APSInfrastructure::ECategory::Station), LOCTEXT("LegendStation", "STATION")},
			{static_cast<uint8>(APSInfrastructure::ECategory::Hub), LOCTEXT("LegendHub", "HUB")},
			{static_cast<uint8>(APSInfrastructure::ECategory::Relay), LOCTEXT("LegendRelay", "RELAY / BEACON")},
			{static_cast<uint8>(APSInfrastructure::ECategory::Transport), LOCTEXT("LegendTransport", "TRANSPORT")},
			{static_cast<uint8>(APSInfrastructure::ECategory::Megastructure), LOCTEXT("LegendMegastructure", "MEGASTRUCTURE")},
			{ShapeFleetHeadquarters, LOCTEXT("LegendFleet", "FLEET STATION / HQ")}};
		for (const FShapeName& Entry : ShapeNames)
		{
			DrawShape(OutDrawElements, LayerId + 8, AllottedGeometry, Shapes + FVector2D(5.0, 8.0), Entry.Kind, Muted(), 4.5);
			Label(OutDrawElements, LayerId + 8, AllottedGeometry, Shapes + FVector2D(16.0, 0.0), Entry.Name, SmallFont, Muted());
			Shapes.Y += 17.0;
		}
		FVector2D Marks(300.0, Size.Y - 17.0 * 4 - 10.0);
		Lines(OutDrawElements, LayerId + 8, AllottedGeometry, {Marks + FVector2D(0.0, 8.0), Marks + FVector2D(16.0, 8.0)},
			Link.CopyWithNewOpacity(0.7f), 2.4f);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(22.0, 0.0), LOCTEXT("LegendLink", "RELAY LINK"), SmallFont, Muted());
		Marks.Y += 17.0;
		Dashes(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(0.0, 8.0), Marks + FVector2D(16.0, 8.0),
			Link.CopyWithNewOpacity(0.7f), 1.2f);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(22.0, 0.0), LOCTEXT("LegendLane", "CARGO LANE"), SmallFont, Muted());
		Marks.Y += 17.0;
		Circle(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(8.0, 8.0), 6.0, Link, 1.6f);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(22.0, 0.0), LOCTEXT("LegendRinged", "TRANSPORT HERE"), SmallFont, Muted());
		Marks.Y += 17.0;
		DashedCircle(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(8.0, 8.0), 6.0, Muted(), 1.0f);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Marks + FVector2D(22.0, 0.0), LOCTEXT("LegendUncharted", "UNCHARTED: PROBE IT"), SmallFont, Muted());
	}
	Label(OutDrawElements, LayerId + 8, AllottedGeometry, FVector2D(Size.X - 300.0, Size.Y - 24.0),
		LOCTEXT("MapHint", "Wheel zooms, drag pans, click opens"), SmallFont, Muted());
	return LayerId + 9;
}

bool SAPSInfrastructureMap::HitTest(const FVector2D& Local, int32& OutNode, int32& OutIcon) const
{
	OutNode = INDEX_NONE;
	OutIcon = INDEX_NONE;
	double Best = 9.0;
	for (int32 Index = 0; Index < IconPixels.Num() && Index < Icons.Num(); ++Index)
	{
		const double Distance = FVector2D::Distance(IconPixels[Index], Local);
		if (Distance < Best)
		{
			Best = Distance;
			OutIcon = Index;
		}
	}
	if (OutIcon != INDEX_NONE)
	{
		return true;
	}
	Best = 14.0;
	for (int32 Index = 0; Index < NodePixels.Num() && Index < Nodes.Num(); ++Index)
	{
		const double Distance = FVector2D::Distance(NodePixels[Index], Local);
		if (Distance < Best)
		{
			Best = Distance;
			OutNode = Index;
		}
	}
	return OutNode != INDEX_NONE;
}

void SAPSInfrastructureMap::OpenAt(const FVector2D& Local)
{
	int32 NodeIndex = INDEX_NONE;
	int32 IconIndex = INDEX_NONE;
	if (!HitTest(Local, NodeIndex, IconIndex))
	{
		return;
	}
	AActor* Target = nullptr;
	if (Icons.IsValidIndex(IconIndex))
	{
		Target = Icons[IconIndex].Actor.Get();
	}
	else if (Nodes.IsValidIndex(NodeIndex))
	{
		Target = Nodes[NodeIndex].Actor.Get();
		if (!Target && Nodes[NodeIndex].SystemId.IsValid())
		{
			// A star system's anchor (spawned on first use): its page, fleet orders and construction aim at it.
			if (FAPSStarSystems* Stars = APSStarSystemsFind(World.Get()))
			{
				Target = Stars->GetAnchor(Nodes[NodeIndex].SystemId);
			}
		}
	}
	if (Target)
	{
		OnOpenObject.ExecuteIfBound(Target);
	}
}

FReply SAPSInfrastructureMap::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (bPressed)
	{
		bPanning |= FVector2D::Distance(Local, PressPosition) > 4.0;
		if (bPanning)
		{
			MapOffset = PressOffset + (Local - PressPosition);
			return FReply::Handled();
		}
	}
	HitTest(Local, HoverNode, HoverIcon);
	return FReply::Unhandled();
}

FReply SAPSInfrastructureMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton && MouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = true;
	bPanning = false;
	PressPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	PressOffset = MapOffset;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAPSInfrastructureMap::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	if (!bPanning && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OpenAt(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	}
	bPanning = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAPSInfrastructureMap::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Zoom about the cursor; back at x1 the whole map is centred again.
	const FVector2D ScreenCentre = MyGeometry.GetLocalSize() * 0.5;
	const FVector2D Anchor = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const double Previous = MapZoom;
	MapZoom = FMath::Clamp(MapZoom * FMath::Pow(1.25, MouseEvent.GetWheelDelta()), 1.0, 40.0);
	if (MapZoom <= 1.0001)
	{
		MapOffset = FVector2D::ZeroVector;
		return FReply::Handled();
	}
	const FVector2D CentreNow = ScreenCentre + MapOffset;
	MapOffset = Anchor - (Anchor - CentreNow) * (MapZoom / Previous) - ScreenCentre;
	return FReply::Handled();
}

void SAPSInfrastructureMap::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseLeave(MouseEvent);
	HoverNode = INDEX_NONE;
	HoverIcon = INDEX_NONE;
}

FCursorReply SAPSInfrastructureMap::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return HoverNode != INDEX_NONE || HoverIcon != INDEX_NONE ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled();
}

// ---------------------------------------------------------------------------------------------------------------------
// The panel

void SAPSInfrastructurePanel::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	ExtraActions = InArgs._ExtraActions;
	const TSharedRef<SWidget> HoldingsWidget = InArgs._Holdings.IsValid() ? InArgs._Holdings.ToSharedRef() : SNullWidget::NullWidget;
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildStocks()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 12.0f)
		[
			BuildSections()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(Views, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()
			[
				BuildNetwork()
			]
			+ SWidgetSwitcher::Slot()
			[
				BuildCatalogue()
			]
			+ SWidgetSwitcher::Slot()
			[
				HoldingsWidget
			]
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(Page, SAPSObjectPage)
				.World(World)
				.ExtraActions(ExtraActions)
				.OnBack(FSimpleDelegate::CreateSP(this, &SAPSInfrastructurePanel::Back))
				.BackLabel(TAttribute<FText>::CreateSP(this, &SAPSInfrastructurePanel::BackLabel))
				.OnOpenObject(FAPSOnOpenObject::CreateSP(this, &SAPSInfrastructurePanel::OpenObject))
			]
		]
	];
	RecountPlaces();
	RebuildCards();
	SetView(0);
}

void SAPSInfrastructurePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Shown again after another tab: read at once.
	const bool bResumed = InCurrentTime - LastTickSeconds > 0.5;
	LastTickSeconds = InCurrentTime;
	RefreshClock += InDeltaTime;
	if (!bResumed && RefreshClock < 0.5f)
	{
		return;
	}
	RefreshClock = 0.0f;
	++RefreshCount;
	RefreshSummary();
	if (ActiveView == 0)
	{
		if (NetworkMap.IsValid())
		{
			NetworkMap->Refresh();
		}
		RefreshNetworkLists(bResumed);
	}
	else if (ActiveView == 1)
	{
		// The places and the stocks change with time. Reading every place costs a few milliseconds, so the counts and
		// the open picker take turns, each every 1.5 s.
		if (bResumed || RefreshCount % 3 == 0)
		{
			RecountPlaces();
		}
		if (!PickerType.IsNone() && (bResumed || RefreshCount % 3 == 1))
		{
			RefreshPicker(false);
		}
	}
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildStocks()
{
	using namespace APSInfrastructurePanelPrivate;
	const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < static_cast<int32>(APSInfrastructure::EResource::Count); ++Index)
	{
		const APSInfrastructure::EResource Resource = static_cast<APSInfrastructure::EResource>(Index);
		const FLinearColor Colour = APSInfrastructure::ResourceColour(Resource);
		Row->AddSlot().FillWidth(1.0f).Padding(Index == 0 ? 0.0f : 6.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBorder).BorderImage(TileBrush()).Padding(FMargin(1.0f))
			.ToolTipText(FText::Format(LOCTEXT("StockTip", "{0}: what the civilization holds, and what everything standing yields a minute, the home's own work included. Construction takes its cost when the ships set out."),
				APSInfrastructure::ResourceName(Resource)))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(3.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10.0f, 6.0f, 10.0f, 7.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(APSInfrastructure::ResourceName(Resource)).Font(Font("Bold", 9)).ColorAndOpacity(Colour)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
						[
							SNew(STextBlock).Font(Font("Bold", 16)).ColorAndOpacity(White())
							.Text_Lambda([this, Resource]()
							{
								const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
								return Infrastructure ? APSUINumber::Number(FMath::FloorToInt(Infrastructure->GetStock(Resource)))
									: FText::FromString(TEXT("-"));
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(8.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(STextBlock).Font(Font("Bold", 10))
							.Text_Lambda([this, Resource]()
							{
								const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
								return Infrastructure ? FText::Format(LOCTEXT("PerMinute", "{0} / MIN"), SignedRate(Infrastructure->GetRate(Resource)))
									: FText::GetEmpty();
							})
							.ColorAndOpacity_Lambda([this, Resource]()
							{
								const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
								const float Rate = Infrastructure ? Infrastructure->GetRate(Resource) : 0.0f;
								return FSlateColor(Rate > 0.05f ? Success() : Rate < -0.05f ? Amber() : Muted());
							})
						]
					]
				]
			]
		];
	}
	// What everything standing changes for the whole civilization, summed.
	Row->AddSlot().FillWidth(1.4f).Padding(6.0f, 0.0f, 0.0f, 0.0f)
	[
		SNew(SBorder).BorderImage(TileBrush()).Padding(FMargin(12.0f, 6.0f, 10.0f, 7.0f))
		.ToolTipText(LOCTEXT("BonusTip", "What the structures change for the whole civilization, summed: surveys and studies, construction work, the fleet's speed."))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("Bonuses", "STRUCTURES GIVE")).Font(Font("Bold", 9)).ColorAndOpacity(Cyan())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(White()).AutoWrapText(true)
				.Text_Lambda([this]()
				{
					const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
					return Infrastructure ? FText::Format(LOCTEXT("BonusValues", "SURVEYS {0}  /  BUILDING {1}  /  FLEET {2}"),
							SignedPercent(Infrastructure->SurveySpeedBonus()), SignedPercent(Infrastructure->BuildSpeedBonus()),
							SignedPercent(Infrastructure->ShipSpeedBonus()))
						: LOCTEXT("NoInfrastructure", "NO INFRASTRUCTURE IN THIS WORLD");
				})
			]
		]
	];
	return Row;
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildSections()
{
	using namespace APSInfrastructurePanelPrivate;
	const auto Section = [this](const FText& Caption, const EAPSChromeGlyph Glyph, const int32 View)
	{
		return APSInfrastructureUI::FrameButton(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(18.0f).HeightOverride(18.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Glyph).Color(Cyan()).StrokeWidth(1.25f)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Caption).Font(Font("Bold", 10)).ColorAndOpacity(White()).RenderTransform(CapsCenterShift(Font("Bold", 10)))
			],
			FOnClicked::CreateLambda([this, View]()
			{
				History.Reset();
				SetView(View);
				return FReply::Handled();
			}),
			TAttribute<bool>::CreateLambda([this, View]() { return ActiveView == View; }), Cyan());
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Section(LOCTEXT("SectionNetwork", "NETWORK MAP"), EAPSChromeGlyph::System, 0)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Section(LOCTEXT("SectionCatalogue", "CONSTRUCTION CATALOGUE"), EAPSChromeGlyph::Infrastructure, 1)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Section(LOCTEXT("SectionHoldings", "HOLDINGS AND COLONY"), EAPSChromeGlyph::Station, 2)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(14.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Muted()).AutoWrapText(true).Justification(ETextJustify::Right)
			.Text_Lambda([this]() { return SummaryText; })
		];
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildNetwork()
{
	using namespace APSInfrastructurePanelPrivate;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(NetworkMap, SAPSInfrastructureMap)
				.World(World)
				.OnOpenObject(FAPSOnOpenObject::CreateSP(this, &SAPSInfrastructurePanel::OpenObject))
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f)
			[
				SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("NetworkTitle", "INFRASTRUCTURE NETWORK")).Font(Font("Bold", 12)).ColorAndOpacity(Cyan())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Muted())
					.Text_Lambda([this]() { return NetworkMap.IsValid() ? NetworkMap->GetSummary() : FText::GetEmpty(); })
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(330.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(NetworkLists, SVerticalBox)
				]
			]
		];
}

void SAPSInfrastructurePanel::RefreshNetworkLists(const bool bForce)
{
	using namespace APSInfrastructurePanelPrivate;
	if (!NetworkLists.IsValid())
	{
		return;
	}
	UWorld* LiveWorld = World.Get();
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	struct FListRow
	{
		FGuid SystemId;
		TWeakObjectPtr<AActor> Actor;
		FText Name;
		FText Detail;
		FLinearColor Colour{FLinearColor::White};
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::System};
	};
	TArray<FListRow> Claimed;
	TArray<FListRow> Targets;
	TArray<FListRow> Transport;
	if (Stars && Stars->IsReady())
	{
		TArray<TPair<int32, int32>> Links;
		Stars->GetNetwork(Links);
		TArray<int32> Known;
		Stars->GetKnown(Known);
		for (const int32 Each : Known)
		{
			const FAPSStarSystemInfo* Info = Stars->Get(Each);
			if (!Info)
			{
				continue;
			}
			const FAPSStarSystemState State = Stars->GetState(Info->Id);
			int32 LinkCount = 0;
			for (const TPair<int32, int32>& Link : Links)
			{
				LinkCount += Link.Key == Each || Link.Value == Each ? 1 : 0;
			}
			FListRow Row;
			Row.SystemId = Info->Id;
			Row.Name = FText::FromString(Info->Name);
			Row.Colour = Info->Colour;
			if (State.bClaimed)
			{
				Row.Detail = FText::Format(LOCTEXT("ClaimedRow", "{0}  /  REACH {1} AU  /  {2} LINKS"),
					Info->bHome ? LOCTEXT("HomeRow", "HOME") : APSStars::KnowledgeName(State.Knowledge), Au(Stars->ReachCm(Each)),
					APSUINumber::Number(LinkCount));
				Claimed.Add(Row);
			}
			else
			{
				Row.Detail = FText::Format(LOCTEXT("TargetRow", "{0}  /  {1} AU FROM HOME  /  {2}"), APSStars::KnowledgeName(State.Knowledge),
					Au(Info->HomeDistanceCm), Stars->IsInReach(Info->Id) ? LOCTEXT("InReachRow", "IN REACH") : LOCTEXT("OutOfReachRow", "OUT OF REACH"));
				Targets.Add(Row);
			}
		}
	}
	if (Infrastructure)
	{
		for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built.Type);
			AActor* Actor = Built.Actor.Get();
			if (!Type || !Actor || !IsTransport(*Type))
			{
				continue;
			}
			FListRow Row;
			Row.Actor = Actor;
			Row.Name = APSObjectActions::NameOf(Actor);
			Row.Detail = Type->RelayReachAu > 0.0f
				? FText::Format(LOCTEXT("RelayRow", "{0}  /  CARRIES THE NETWORK {1} AU"), APSInfrastructure::CategoryName(Type->Category),
					Au(Type->RelayReachAu * APSStars::AstronomicalUnitCm))
				: FText::Format(LOCTEXT("TransportRow", "{0}  /  AT {1}"), APSInfrastructure::CategoryName(Type->Category),
					APSObjectActions::NameOf(Actor->GetAttachParentActor()));
			Row.Colour = APSInfrastructure::DepartmentColour(Type->Department);
			Row.Glyph = APSInfrastructureUI::CategoryGlyph(Type->Category);
			Transport.Add(Row);
		}
	}
	FString Signature;
	for (const TArray<FListRow>* List : {&Claimed, &Targets, &Transport})
	{
		for (const FListRow& Row : *List)
		{
			Signature += Row.Name.ToString() + TEXT("~") + Row.Detail.ToString() + TEXT(";");
		}
		Signature += TEXT("|");
	}
	if (!bForce && Signature == NetworkListsSignature)
	{
		return;
	}
	NetworkListsSignature = Signature;
	NetworkLists->ClearChildren();
	const auto AddRows = [this](const FText& Title, const FLinearColor& TitleColour, const TArray<FListRow>& Rows, const FText& Empty)
	{
		NetworkLists->AddSlot().AutoHeight().Padding(0.0f, NetworkLists->NumSlots() == 0 ? 0.0f : 16.0f, 0.0f, 8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Title).Font(Font("Bold", 10)).ColorAndOpacity(TitleColour)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
				]
			]
		];
		if (Rows.IsEmpty())
		{
			NetworkLists->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(Empty).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
			];
			return;
		}
		for (const FListRow& Row : Rows)
		{
			const FGuid SystemId = Row.SystemId;
			const TWeakObjectPtr<AActor> Actor = Row.Actor;
			NetworkLists->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				APSInfrastructureUI::FrameButton(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						IconBadge(Row.Glyph, Row.Colour, 26.0f)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(Row.Name).Font(Font("Bold", 10)).ColorAndOpacity(White())
							.RenderTransform(CapsCenterShift(Font("Bold", 10)))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(Row.Detail).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
						]
					],
					FOnClicked::CreateLambda([this, SystemId, Actor]()
					{
						AActor* Target = Actor.IsValid() ? Actor.Get() : SystemId.IsValid() ? SystemSite(SystemId) : nullptr;
						if (Target)
						{
							OpenObject(Target);
						}
						return FReply::Handled();
					}),
					TAttribute<bool>(false), Row.Colour)
			];
		}
	};
	AddRows(LOCTEXT("ClaimedTitle", "CLAIMED SYSTEMS"), LinkColour(), Claimed,
		LOCTEXT("ClaimedNone", "Only the home system so far: a survey beacon claims a scanned star system."));
	AddRows(LOCTEXT("TargetsTitle", "EXPANSION TARGETS"), Cyan(), Targets,
		LOCTEXT("TargetsNone", "No other star system known yet: click a faint star on the map and send a probe from its page."));
	AddRows(LOCTEXT("TransportTitle", "TRANSPORT AND RELAYS"), LinkColour(), Transport,
		LOCTEXT("TransportNone", "None built yet: cargo hubs, mass drivers, relays and the jump gate are in the CONSTRUCTION CATALOGUE."));
	NetworkLists->AddSlot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
		.Text(LOCTEXT("NetworkHelp", "Relays claim star systems and link them into the network: a beacon, an administration hub, a hyperspace relay, a jump gate. Systems in reach are linked; the home reaches 3 AU on its own."))
	];
}

AActor* SAPSInfrastructurePanel::SystemSite(const FGuid& SystemId) const
{
	UWorld* LiveWorld = World.Get();
	FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
	if (!Info)
	{
		return nullptr;
	}
	if (Info->bHome)
	{
		if (AActor* Star = APSInfrastructureUI::HomeStar(LiveWorld))
		{
			return Star;
		}
	}
	return Stars->GetAnchor(SystemId);
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildCatalogue()
{
	using namespace APSInfrastructurePanelPrivate;
	// Department chips with how many types each has: ALL, then the six divisions and transport in their colours.
	const TSharedRef<SWrapBox> Filters = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.0f, 6.0f));
	for (int32 Filter = -1; Filter < static_cast<int32>(APSInfrastructure::EDepartment::Count); ++Filter)
	{
		const FLinearColor Accent = Filter < 0 ? Cyan() : APSInfrastructure::DepartmentColour(static_cast<APSInfrastructure::EDepartment>(Filter));
		int32 Count = 0;
		for (const APSInfrastructure::FType& Type : APSInfrastructure::Types())
		{
			Count += Filter < 0 || static_cast<int32>(Type.Department) == Filter ? 1 : 0;
		}
		const FText Department = Filter < 0 ? LOCTEXT("FilterAll", "ALL")
			: APSInfrastructure::DepartmentName(static_cast<APSInfrastructure::EDepartment>(Filter));
		Filters->AddSlot()
		[
			APSInfrastructureUI::FrameButton(
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Accent)
				.Text(FText::Format(LOCTEXT("FilterChip", "{0}  {1}"), Department, APSUINumber::Number(Count))),
				FOnClicked::CreateSP(this, &SAPSInfrastructurePanel::SetDepartmentFilter, Filter),
				TAttribute<bool>::CreateLambda([this, Filter]() { return DepartmentFilter == Filter; }), Accent)
		];
	}
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Filters
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
				.Text(LOCTEXT("CatalogueHelp", "Construction ships raise these at the places the civilization knows. BUILD... lists every place that fits with what it still lacks there; the nearest free construction ship goes and the cost is taken when it sets out."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
				.Text_Lambda([this]() { return CatalogueMessage; })
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(bCatalogueMessageIsError ? Amber() : Success()); })
				.Visibility_Lambda([this]() { return CatalogueMessage.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(CardsBox, SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.0f, 10.0f))
				]
			]
		]
		// The site picker over the catalogue, the catalogue dimmed under it: a click beside the window closes it.
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return PickerType.IsNone() ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "NoBorder").ContentPadding(0.0f)
				.OnClicked(this, &SAPSInfrastructurePanel::ClosePicker)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Scrim())
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24.0f)
		[
			SNew(SBox).WidthOverride(760.0f)
			.Visibility_Lambda([this]() { return PickerType.IsNone() ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				BuildPicker()
			]
		];
}

void SAPSInfrastructurePanel::RebuildCards()
{
	if (!CardsBox.IsValid())
	{
		return;
	}
	CardsBox->ClearChildren();
	for (const APSInfrastructure::FType& Type : APSInfrastructure::Types())
	{
		if (DepartmentFilter < 0 || static_cast<int32>(Type.Department) == DepartmentFilter)
		{
			CardsBox->AddSlot()
			[
				BuildCard(Type.Id)
			];
		}
	}
}

FReply SAPSInfrastructurePanel::SetDepartmentFilter(const int32 Filter)
{
	DepartmentFilter = Filter;
	RebuildCards();
	return FReply::Handled();
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildCard(const FName TypeId)
{
	using namespace APSInfrastructurePanelPrivate;
	using APSInfrastructureUI::Chip;
	const APSInfrastructure::FType* Type = APSInfrastructure::Find(TypeId);
	if (!Type)
	{
		return SNullWidget::NullWidget;
	}
	const FLinearColor Accent = APSInfrastructure::DepartmentColour(Type->Department);
	const APSInfrastructure::EDepartment Department = Type->Department;
	const int32 RequiredLevel = Type->RequiredLevel;
	const FLinearColor UnlockColour = APSInfrastructure::DepartmentColour(APSInfrastructure::EDepartment::Science);
	const auto Unlocked = [this, TypeId]()
	{
		const FAPSMissionBoard* Missions = APSMissionsFind(World.Get());
		return Missions && Missions->IsUnlocked(TypeId);
	};

	// Rio 04.10 ("too much text, everything runs into each other; the same information, but readable, like OVERVIEW and
	// PILOT"): the place and the tags as chips under the name, the build time large in the corner, the needs as a list,
	// what it costs and what it gives as two columns of lines instead of chip clouds, the chain as dotted steps, and how
	// many places take it beside BUILD. Lines in the readable face; the long rules stay as tooltips.
	const FSlateFontInfo LineFont = APSUITheme::BodyFont("Regular", 12);
	const FSlateFontInfo StrongFont = APSUITheme::BodyFont("Bold", 12);
	const FSlateFontInfo LabelFont = APSUITheme::BodyFont("Bold", 10);
	const auto CardDot = [](const TAttribute<FSlateColor>& Colour, const float Size)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Colour)
			];
	};
	const auto CardLabel = [&LabelFont](const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(LabelFont).ColorAndOpacity(Muted());
	};

	// Where it stands first, then the category, a megastructure, a mission's unlock, giants only, a claim.
	const TSharedRef<SWrapBox> Badges = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5.0f, 5.0f));
	Badges->AddSlot()[Chip(APSInfrastructure::PlacementName(Type->Placement), FSlateColor(Cyan()), Cyan())];
	Badges->AddSlot()[Chip(APSInfrastructure::CategoryName(Type->Category), FSlateColor(Accent), Accent)];
	if (Type->bMegastructure)
	{
		Badges->AddSlot()[Chip(LOCTEXT("Megastructure", "MEGASTRUCTURE: BUILT IN STAGES"), FSlateColor(Amber()), Amber())];
	}
	else if (Type->Category == APSInfrastructure::ECategory::Hub)
	{
		Badges->AddSlot()[Chip(LOCTEXT("HubBadge", "HUB: BUILT IN STAGES"), FSlateColor(Amber()), Amber())];
	}
	if (Type->bNeedsUnlock)
	{
		Badges->AddSlot()
		[
			Chip(TAttribute<FText>::CreateLambda([Unlocked]()
				{
					return Unlocked() ? LOCTEXT("UnlockedBadge", "UNLOCKED BY A MISSION") : LOCTEXT("LockedBadge", "NEEDS A MISSION'S UNLOCK");
				}),
				TAttribute<FSlateColor>::CreateLambda([Unlocked]() { return FSlateColor(Unlocked() ? Success() : Amber()); }), UnlockColour)
		];
	}
	if (Type->bGiantOnly)
	{
		Badges->AddSlot()[Chip(LOCTEXT("GiantsOnly", "GAS AND ICE GIANTS ONLY"), FSlateColor(Cyan()), Cyan())];
	}
	if (Type->bClaims)
	{
		Badges->AddSlot()[Chip(LOCTEXT("ClaimsBadge", "CLAIMS THE STAR SYSTEM"), FSlateColor(LinkColour()), LinkColour())];
	}

	// What it needs: what the place must be known to, the ground, a structure there first, the department's level
	// against its level now, a mission's unlock.
	const TSharedRef<SVerticalBox> Needs = SNew(SVerticalBox);
	const auto Need = [&Needs, &LineFont, &CardDot](const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Colour)
	{
		Needs->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 6.0f, 9.0f, 0.0f)
			[
				CardDot(Colour, 6.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(Text).AutoWrapText(true).Font(LineFont).ColorAndOpacity(Colour)
			]
		];
	};
	const bool bStarSystem = Type->Placement == APSInfrastructure::EPlacement::StarSystem;
	Need(bStarSystem
		? (Type->RequiredKnowledge >= 2 ? LOCTEXT("NeedSystemSurveyed", "A surveyed star system (a visit or a survey)")
			: Type->RequiredKnowledge == 1 ? LOCTEXT("NeedSystemScanned", "A scanned star system (a probe or a scanner)")
			: LOCTEXT("NeedSystemAny", "Any star system"))
		: (Type->RequiredKnowledge >= 2 ? LOCTEXT("NeedWorldStudied", "A studied world (a science ship)")
			: Type->RequiredKnowledge == 1 ? LOCTEXT("NeedWorldSurveyed", "A surveyed world (an exploration ship)")
			: LOCTEXT("NeedWorldAny", "Any planet or moon")),
		FSlateColor(White()));
	if (Type->Placement == APSInfrastructure::EPlacement::Surface)
	{
		Need(LOCTEXT("NeedGround", "Solid ground: not a gas or ice giant"), FSlateColor(White()));
	}
	// The structures it stands on (Rio 03.10, chains): a station over the world, the step before it, its system's ring.
	TArray<FText> Requirements;
	APSInfrastructure::DescribeRequirements(*Type, Requirements);
	for (const FText& Requirement : Requirements)
	{
		Need(Requirement, FSlateColor(White()));
	}
	if (RequiredLevel > 0)
	{
		Need(TAttribute<FText>::CreateLambda([this, Department, RequiredLevel]()
			{
				return FText::Format(LOCTEXT("NeedLevel", "{0} level {1}  (now {2})"), APSInfrastructure::DepartmentName(Department),
					APSUINumber::Number(RequiredLevel), APSUINumber::Number(DepartmentLevels[static_cast<int32>(Department)]));
			}),
			TAttribute<FSlateColor>::CreateLambda([this, Department, RequiredLevel]()
			{
				return FSlateColor(DepartmentLevels[static_cast<int32>(Department)] >= RequiredLevel ? Success() : Amber());
			}));
	}
	if (Type->bNeedsUnlock)
	{
		Need(TAttribute<FText>::CreateLambda([Unlocked, Department]()
			{
				return Unlocked() ? LOCTEXT("NeedUnlocked", "Unlocked by a mission")
					: FText::Format(LOCTEXT("NeedUnlock", "A {0} mission's unlock (DIVISIONS)"), APSInfrastructure::DepartmentName(Department));
			}),
			TAttribute<FSlateColor>::CreateLambda([Unlocked]() { return FSlateColor(Unlocked() ? Success() : Amber()); }));
	}

	// What it costs (amber while the stocks do not cover it) and what it yields and changes while it stands: one line
	// each, a swatch in the resource's or the effect's colour in front.
	const auto CardLine = [&StrongFont, &CardDot](const TSharedRef<SVerticalBox>& Box, const TAttribute<FText>& Text,
		const TAttribute<FSlateColor>& Colour, const FLinearColor& Swatch)
	{
		Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 5.0f, 8.0f, 0.0f)
			[
				CardDot(FSlateColor(Swatch), 8.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(Text).AutoWrapText(true).Font(StrongFont).ColorAndOpacity(Colour)
			]
		];
	};
	const TSharedRef<SVerticalBox> Costs = SNew(SVerticalBox);
	for (const APSInfrastructure::FAmount& Amount : Type->Cost)
	{
		const APSInfrastructure::EResource Resource = Amount.Resource;
		const float Value = Amount.Value;
		CardLine(Costs, FText::Format(LOCTEXT("CostAmount", "{0} {1}"), APSUINumber::Number(FMath::RoundToInt(Value)), APSInfrastructure::ResourceName(Resource)),
			TAttribute<FSlateColor>::CreateLambda([this, Resource, Value]()
			{
				const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
				return FSlateColor(!Infrastructure || Infrastructure->GetStock(Resource) + 0.001f >= Value
					? APSInfrastructure::ResourceColour(Resource) : Amber());
			}),
			APSInfrastructure::ResourceColour(Resource));
	}
	if (Type->Cost.IsEmpty())
	{
		CardLine(Costs, LOCTEXT("CostNothing", "NOTHING"), FSlateColor(Muted()), Muted());
	}
	const TSharedRef<SVerticalBox> Gives = SNew(SVerticalBox);
	int32 GiveCount = 0;
	for (const APSInfrastructure::FAmount& Amount : Type->Yield)
	{
		const FLinearColor Colour = APSInfrastructure::ResourceColour(Amount.Resource);
		CardLine(Gives, FText::Format(LOCTEXT("YieldAmount", "+{0} {1} / MIN"), APSUINumber::Number(FMath::RoundToInt(Amount.Value)),
			APSInfrastructure::ResourceName(Amount.Resource)), FSlateColor(Colour), Colour);
		++GiveCount;
	}
	const auto Effect = [&CardLine, &Gives, &GiveCount](const FText& Text)
	{
		CardLine(Gives, Text, FSlateColor(APSChrome::Cyan()), APSChrome::Cyan());
		++GiveCount;
	};
	FNumberFormattingOptions OneDecimal;
	OneDecimal.SetMaximumFractionalDigits(1);
	if (Type->RelayReachAu > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectRelay", "NETWORK {0} AU"), APSUINumber::Number(Type->RelayReachAu, &OneDecimal)));
	}
	if (Type->ScanReachAu > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectScan", "SCANS {0} AU"), APSUINumber::Number(Type->ScanReachAu, &OneDecimal)));
	}
	if (Type->SurveySpeed > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectSurvey", "SURVEYS {0}"), SignedPercent(Type->SurveySpeed)));
	}
	if (Type->BuildSpeed > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectBuild", "BUILDING {0}"), SignedPercent(Type->BuildSpeed)));
	}
	if (Type->ShipSpeed > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectFleet", "FLEET SPEED {0}"), SignedPercent(Type->ShipSpeed)));
	}
	if (Type->LocalWorkSpeed > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectLocal", "WORK THERE {0}"), SignedPercent(Type->LocalWorkSpeed)));
	}
	if (Type->SystemWorkSpeed > 0.0f)
	{
		Effect(FText::Format(LOCTEXT("EffectSystemWork", "WORK IN THE SYSTEM {0}"), SignedPercent(Type->SystemWorkSpeed)));
	}
	if (Type->HubBerths > 0)
	{
		Effect(FText::Format(LOCTEXT("EffectBerths", "+{0} STATION BERTHS AT ITS WORLD"), APSUINumber::Number(Type->HubBerths)));
	}
	if (GiveCount == 0)
	{
		CardLine(Gives, LOCTEXT("GivesGuard", "GUARDS THE PLACE"), FSlateColor(Muted()), Muted());
	}
	if (Type->LimitPerSite > 1)
	{
		CardLine(Gives, FText::Format(LOCTEXT("EffectLimit", "UP TO {0} PER PLACE"), APSUINumber::Number(Type->LimitPerSite)),
			FSlateColor(Muted()), Muted());
	}

	// The chain it belongs to (Rio 03.10: "buildable in chains, a space elevator, then a space ring, one after another";
	// the locked steps say what they require): the station it starts from, then every step with how far the civilization
	// is: standing, under construction, ready somewhere, waiting (its needs stand, an unlock, a level or the stocks do
	// not), or locked and why. A dot in the step's state colour in front; this card's own step in bold.
	TArray<FName> ChainSteps;
	const bool bInChain = APSInfrastructure::GetChain(TypeId, ChainSteps);
	const TSharedRef<SVerticalBox> Chain = SNew(SVerticalBox);
	if (bInChain)
	{
		const TWeakObjectPtr<UWorld> ChainWorld = World;
		// How many of a type stand: anywhere, and the most at one place.
		const auto Standing = [ChainWorld](const FName Id, int32& OutMostAtOnePlace)
		{
			OutMostAtOnePlace = 0;
			const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(ChainWorld.Get());
			if (!Infrastructure)
			{
				return 0;
			}
			int32 Count = 0;
			TMap<FString, int32> PerPlace;
			for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
			{
				if (Built.Type == Id)
				{
					++Count;
					OutMostAtOnePlace = FMath::Max(OutMostAtOnePlace, ++PerPlace.FindOrAdd(Built.SiteKey));
				}
			}
			return Count;
		};
		const auto Step = [&Chain, &LineFont, &StrongFont, &CardDot](const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Colour,
			const bool bCurrent)
		{
			Chain->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 6.0f, 9.0f, 0.0f)
				[
					CardDot(Colour, 6.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(Text).AutoWrapText(true).Font(bCurrent ? StrongFont : LineFont).ColorAndOpacity(Colour)
				]
			];
		};
		Step(LOCTEXT("ChainStation", "1. An orbital station over the world (the home complex counts)"), FSlateColor(Success()), false);
		for (int32 Index = 0; Index < ChainSteps.Num(); ++Index)
		{
			const FName StepId = ChainSteps[Index];
			const APSInfrastructure::FType* StepType = APSInfrastructure::Find(StepId);
			if (!StepType)
			{
				continue;
			}
			const FText StepLabel = FText::Format(LOCTEXT("ChainStepName", "{0}. {1}"), APSUINumber::Number(Index + 2), StepType->Name);
			TArray<FText> StepNeeds;
			APSInfrastructure::DescribeRequirements(*StepType, StepNeeds);
			const FText Lock = StepNeeds.IsEmpty() ? LOCTEXT("ChainLockedPlace", "no known place takes it yet") : StepNeeds[0];
			// What it stands on: the step before at one place (as many as it wants), or the ring somewhere in the system.
			const FName Before = !StepType->RequiresAtSite.IsNone() ? StepType->RequiresAtSite : StepType->RequiresInSystem;
			const int32 BeforeWanted = !StepType->RequiresAtSite.IsNone() ? FMath::Max(StepType->RequiresAtSiteCount, 1) : 1;
			enum class EState : uint8 { Standing, Building, Ready, Waiting, Locked };
			const auto StateOf = [this, Standing, StepId, Before, BeforeWanted](int32& OutCount)
			{
				int32 Most = 0;
				OutCount = Standing(StepId, Most);
				if (OutCount > 0)
				{
					return EState::Standing;
				}
				if (BuildsUnderWay.FindRef(StepId) > 0)
				{
					return EState::Building;
				}
				if (PlacesNow.FindRef(StepId) > 0)
				{
					return EState::Ready;
				}
				int32 BeforeMost = 0;
				const bool bBeforeStands = Before.IsNone() || (Standing(Before, BeforeMost) > 0 && BeforeMost >= BeforeWanted);
				return bBeforeStands ? EState::Waiting : EState::Locked;
			};
			Step(TAttribute<FText>::CreateLambda([StateOf, StepLabel, Lock]()
				{
					int32 Count = 0;
					switch (StateOf(Count))
					{
					case EState::Standing: return FText::Format(LOCTEXT("ChainStanding", "{0}: stands ({1})"), StepLabel, APSUINumber::Number(Count));
					case EState::Building: return FText::Format(LOCTEXT("ChainBuilding", "{0}: under construction"), StepLabel);
					case EState::Ready: return FText::Format(LOCTEXT("ChainReady", "{0}: ready to build"), StepLabel);
					case EState::Waiting: return FText::Format(LOCTEXT("ChainWaiting", "{0}: its needs stand; an unlock, a level or the stocks are missing (BUILD... says which)"), StepLabel);
					default: return FText::Format(LOCTEXT("ChainLocked", "{0}: locked, {1}"), StepLabel, Lock);
					}
				}),
				TAttribute<FSlateColor>::CreateLambda([StateOf]()
				{
					int32 Count = 0;
					switch (StateOf(Count))
					{
					case EState::Standing: return FSlateColor(Success());
					case EState::Building:
					case EState::Waiting: return FSlateColor(Amber());
					case EState::Ready: return FSlateColor(Cyan());
					default: return FSlateColor(Muted());
					}
				}),
				StepId == TypeId);
		}
	}

	const FText WorkRule = FText::Format(LOCTEXT("WorkTime", "{0} for a construction ship; less with its level and the building bonuses"),
		Duration(Type->BuildSeconds));
	return SNew(SBox).WidthOverride(440.0f)
	[
		ChamferPanel(
			SNew(SVerticalBox)
			// The name with its department, and the build time in the corner.
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(APSInfrastructureUI::CategoryGlyph(Type->Category), Accent, 42.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Type->Name).Font(Font("Bold", 14)).ColorAndOpacity(White()).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(APSInfrastructure::DepartmentName(Department)).Font(LabelFont).ColorAndOpacity(Accent)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SVerticalBox).ToolTipText(WorkRule)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						CardLabel(LOCTEXT("BuildTime", "BUILD TIME"))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Duration(Type->BuildSeconds)).Font(Font("Bold", 15)).ColorAndOpacity(White())
					]
				]
			]
			// What it does, then where and what kind as chips.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Type->Role).AutoWrapText(true).Font(LineFont).ColorAndOpacity(White().CopyWithNewOpacity(0.9f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				Badges
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
			[
				CardLabel(LOCTEXT("RowNeeds", "NEEDS"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Needs
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox).Visibility(bInChain ? EVisibility::Visible : EVisibility::Collapsed)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					CardLabel(LOCTEXT("RowChain", "CHAIN"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Chain
				]
			]
			// Costs and gives side by side.
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						CardLabel(LOCTEXT("RowCosts", "COSTS"))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						Costs
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(1.0f)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.3f).Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						CardLabel(LOCTEXT("RowGives", "GIVES"))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						Gives
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 12.0f)
			[
				SNew(SBox).HeightOverride(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim())
				]
			]
			// How many known places take it now (the tooltip names them), and BUILD.
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 9.0f, 0.0f)
				[
					CardDot(TAttribute<FSlateColor>::CreateLambda([this, TypeId]()
					{
						return FSlateColor(PlacesNow.FindRef(TypeId) > 0 ? Success() : Amber());
					}), 8.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(StrongFont).AutoWrapText(true)
						.Text_Lambda([this, TypeId]()
						{
							const int32 Places = PlacesNow.FindRef(TypeId);
							return Places == 0 ? LOCTEXT("PlacesNone", "NO KNOWN PLACE TAKES IT NOW")
								: Places == 1 ? LOCTEXT("PlacesOne", "1 PLACE TAKES IT NOW")
								: FText::Format(LOCTEXT("PlacesMany", "{0} PLACES TAKE IT NOW"), APSUINumber::Number(Places));
						})
						.ColorAndOpacity_Lambda([this, TypeId]() { return FSlateColor(PlacesNow.FindRef(TypeId) > 0 ? Success() : Amber()); })
						.ToolTipText_Lambda([this, TypeId]()
						{
							const FString* Names = PlacesNames.Find(TypeId);
							return Names ? FText::FromString(*Names) : LOCTEXT("PlacesNoneTip", "BUILD... shows every place that fits and what each still lacks.");
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Font(APSUITheme::BodyFont("Regular", 11)).ColorAndOpacity(Amber())
						.Text_Lambda([this, TypeId]()
						{
							return FText::Format(LOCTEXT("UnderWayCount", "{0} under way"), APSUINumber::Number(BuildsUnderWay.FindRef(TypeId)));
						})
						.Visibility_Lambda([this, TypeId]() { return BuildsUnderWay.FindRef(TypeId) > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(140.0f)
					[
						APSInfrastructureUI::FilledButton(LOCTEXT("BuildEllipsis", "BUILD..."),
							FOnClicked::CreateSP(this, &SAPSInfrastructurePanel::OpenPicker, TypeId), TAttribute<bool>(true), Accent)
					]
				]
			],
			FMargin(18.0f, 16.0f), Accent.CopyWithNewOpacity(0.5f))
	];
}

TSharedRef<SWidget> SAPSInfrastructurePanel::BuildPicker()
{
	using namespace APSInfrastructurePanelPrivate;
	const auto Picked = [this]() { return APSInfrastructure::Find(PickerType); };
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Panel()).Padding(FMargin(18.0f, 16.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Font("Bold", 15)).ColorAndOpacity(White()).AutoWrapText(true)
				.Text_Lambda([Picked]()
				{
					const APSInfrastructure::FType* Type = Picked();
					return Type ? FText::Format(LOCTEXT("PickerTitle", "BUILD {0}: WHERE?"), Type->Name) : FText::GetEmpty();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				APSInfrastructureUI::FrameButton(SNew(STextBlock).Text(LOCTEXT("PickerClose", "CLOSE")).Font(Font("Bold", 10)).ColorAndOpacity(Amber()),
					FOnClicked::CreateSP(this, &SAPSInfrastructurePanel::ClosePicker), TAttribute<bool>(false), Amber())
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
			.Text_Lambda([Picked]()
			{
				const APSInfrastructure::FType* Type = Picked();
				return Type ? FText::Format(LOCTEXT("PickerRole", "{0}  /  {1}  /  costs {2}"), Type->Role,
					APSInfrastructure::PlacementName(Type->Placement), APSInfrastructure::DescribeAmounts(Type->Cost)) : FText::GetEmpty();
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Cyan())
			.Text(LOCTEXT("PickerHelp", "Every place the civilization knows that fits, ready ones first. BUILD HERE sends the nearest free construction ship, as the place's own page would."))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).MaxDesiredHeight(460.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(PickerRows, SVerticalBox)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Bold", 10))
			.Text_Lambda([this]() { return PickerMessage; })
			.ColorAndOpacity_Lambda([this]() { return FSlateColor(bPickerMessageIsError ? Amber() : Success()); })
		]
	];
}

FReply SAPSInfrastructurePanel::OpenPicker(const FName TypeId)
{
	PickerType = TypeId;
	PickerMessage = FText::GetEmpty();
	RefreshPicker(true);
	return FReply::Handled();
}

FReply SAPSInfrastructurePanel::ClosePicker()
{
	PickerType = NAME_None;
	return FReply::Handled();
}

void SAPSInfrastructurePanel::RefreshPicker(const bool bForce)
{
	using namespace APSInfrastructurePanelPrivate;
	if (!PickerRows.IsValid())
	{
		return;
	}
	const APSInfrastructure::FType* Type = APSInfrastructure::Find(PickerType);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	struct FSiteRow
	{
		TWeakObjectPtr<AActor> Site;
		FText Name;
		FText Kind;
		FText Reason;
		bool bReady{false};
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::Planet};
		FLinearColor Colour{FLinearColor::White};
	};
	TArray<FSiteRow> Rows;
	if (Type && Infrastructure)
	{
		TArray<AActor*> Sites;
		CollectSites(Sites);
		TArray<TPair<FName, FText>> Options;
		for (AActor* Site : Sites)
		{
			// The runtime's own list of what suits this place (placement, giants) with its reason for each.
			Infrastructure->GetOptions(Site, Options);
			const TPair<FName, FText>* Option = Options.FindByPredicate([Type](const TPair<FName, FText>& Each) { return Each.Key == Type->Id; });
			if (!Option)
			{
				continue;
			}
			FSiteRow Row;
			Row.Site = Site;
			Row.Name = SiteLabel(Site);
			Row.Kind = Site->IsA<AStar>() ? LOCTEXT("HomeSystemKind", "THE HOME STAR SYSTEM") : APSObjectActions::KindOf(Site);
			Row.Glyph = APSInfrastructureUI::GlyphOf(Site);
			Row.Colour = APSInfrastructureUI::ColourOf(Site);
			Row.Reason = Option->Value;
			if (Row.Reason.IsEmpty())
			{
				// The fleet's own pick: which ship goes, or why none can.
				FText Refusal;
				const ASpaceship* Ship = Fleet ? Fleet->PickShipFor(APSFleet::EOrder::BuildStructure, Site, Type->Id, Refusal) : nullptr;
				const FAPSFleetUnit* Unit = Ship ? Fleet->FindUnit(Ship) : nullptr;
				Row.bReady = Ship != nullptr;
				Row.Reason = Unit ? FText::Format(LOCTEXT("PickerShip", "READY: {0} ({1}) GOES"), FText::FromString(Unit->CallSign),
						APSFleet::DivisionName(Unit->Division))
					: Ship ? LOCTEXT("PickerReady", "READY")
					: Refusal.IsEmpty() ? LOCTEXT("PickerNoShip", "No free construction ship.") : Refusal;
			}
			Rows.Add(Row);
		}
	}
	Rows.StableSort([](const FSiteRow& A, const FSiteRow& B)
	{
		return A.bReady != B.bReady ? A.bReady : A.Name.ToString() < B.Name.ToString();
	});
	FString Signature = PickerType.ToString() + TEXT(":");
	for (const FSiteRow& Row : Rows)
	{
		Signature += Row.Name.ToString() + (Row.bReady ? TEXT("+") : TEXT("-")) + Row.Reason.ToString() + TEXT(";");
	}
	if (!bForce && Signature == PickerSignature)
	{
		return;
	}
	PickerSignature = Signature;
	PickerRows->ClearChildren();
	if (Rows.IsEmpty())
	{
		PickerRows->AddSlot().AutoHeight()
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Amber())
			.Text(Type && Type->Placement == APSInfrastructure::EPlacement::StarSystem
				? LOCTEXT("PickerNoSystem", "No known star system fits it: scan or survey one first (the NETWORK MAP's faint stars).")
				: LOCTEXT("PickerNoWorld", "No world of the home system fits it."))
		];
		return;
	}
	const FLinearColor Accent = Type ? APSInfrastructure::DepartmentColour(Type->Department) : Cyan();
	const FName TypeId = PickerType;
	for (const FSiteRow& Row : Rows)
	{
		PickerRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			ChamferPanel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					IconBadge(Row.Glyph, Row.Colour, 30.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Row.Name).Font(Font("Bold", 11)).ColorAndOpacity(White())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Row.Kind).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(Row.Reason).AutoWrapText(true).Font(Font("Bold", 9))
						.ColorAndOpacity(Row.bReady ? Success() : Amber())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f)
					[
						APSInfrastructureUI::FilledButton(LOCTEXT("BuildHere", "BUILD HERE"),
							FOnClicked::CreateSP(this, &SAPSInfrastructurePanel::BuildAt, TypeId, Row.Site), TAttribute<bool>(Row.bReady), Accent)
					]
				],
				FMargin(12.0f, 9.0f), Row.bReady ? Accent.CopyWithNewOpacity(0.6f) : CyanDim())
		];
	}
}

FReply SAPSInfrastructurePanel::BuildAt(const FName TypeId, const TWeakObjectPtr<AActor> Site)
{
	UWorld* LiveWorld = World.Get();
	AActor* Place = Site.Get();
	bPickerMessageIsError = true;
	if (!LiveWorld || !Place)
	{
		PickerMessage = LOCTEXT("PlaceGone", "That place is gone.");
		return FReply::Handled();
	}
	// The construction provider's own action at this place (APSObjectActions "Build.<type>"): the rules, the cost and the
	// ship pick are the fleet's, the same as on the place's page and every other screen.
	TArray<FAPSObjectAction> Actions;
	APSObjectActions::Gather(LiveWorld, Place, Actions);
	const FName Wanted(*(TEXT("Build.") + TypeId.ToString()));
	const FAPSObjectAction* Action = Actions.FindByPredicate([Wanted](const FAPSObjectAction& Each) { return Each.Id == Wanted; });
	if (!Action)
	{
		PickerMessage = LOCTEXT("NotOffered", "Not offered at this place.");
	}
	else if (!Action->bEnabled || !Action->Execute)
	{
		PickerMessage = Action->Detail;
	}
	else
	{
		const FText Answer = Action->Execute();
		// Done when a construction ship now carries this order to this place.
		bool bUnderWay = false;
		if (const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld))
		{
			for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
			{
				bUnderWay = bUnderWay || (Unit.Order == APSFleet::EOrder::BuildStructure && Unit.StructureType == TypeId
					&& Unit.Target.Get() == Place);
			}
		}
		bPickerMessageIsError = !bUnderWay;
		PickerMessage = Answer;
		if (bUnderWay)
		{
			CatalogueMessage = FText::Format(LOCTEXT("BuildOrdered", "{0}  Follow it on the page of {1} (NETWORK MAP) or in FLEET ORDERS."),
				Answer, APSObjectActions::NameOf(Place));
			bCatalogueMessageIsError = false;
			PickerType = NAME_None;
		}
	}
	RecountPlaces();
	PickerSignature.Reset();
	return FReply::Handled();
}

void SAPSInfrastructurePanel::CollectSites(TArray<AActor*>& OutSites) const
{
	OutSites.Reset();
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	// The home system's worlds in the system's order (only the home system is materialized in a game).
	TArray<AActor*> Bodies;
	for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It))
		{
			Bodies.Add(*It);
		}
	}
	Bodies.Sort([](const AActor& A, const AActor& B) { return APSBodyDesignation::Of(&A) < APSBodyDesignation::Of(&B); });
	OutSites.Append(Bodies);
	AActor* Star = APSInfrastructureUI::HomeStar(LiveWorld);
	if (Star)
	{
		OutSites.Add(Star);
	}
	// The star systems the civilization knows or holds, nearest first; their anchors stand for them.
	if (FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld); Stars && Stars->IsReady())
	{
		TArray<int32> Known;
		Stars->GetKnown(Known);
		for (const int32 Each : Known)
		{
			const FAPSStarSystemInfo* Info = Stars->Get(Each);
			if (!Info || (Info->bHome && Star))
			{
				continue;
			}
			if (AActor* Anchor = Stars->GetAnchor(Info->Id))
			{
				OutSites.Add(Anchor);
			}
		}
	}
}

void SAPSInfrastructurePanel::RecountPlaces()
{
	PlacesNow.Reset();
	PlacesNames.Reset();
	BuildsUnderWay.Reset();
	UWorld* LiveWorld = World.Get();
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld))
	{
		TArray<AActor*> Sites;
		CollectSites(Sites);
		TArray<TPair<FName, FText>> Options;
		for (const AActor* Site : Sites)
		{
			Infrastructure->GetOptions(Site, Options);
			for (const TPair<FName, FText>& Option : Options)
			{
				if (!Option.Value.IsEmpty())
				{
					continue;
				}
				++PlacesNow.FindOrAdd(Option.Key);
				FString& Names = PlacesNames.FindOrAdd(Option.Key);
				Names += (Names.IsEmpty() ? TEXT("") : TEXT(", ")) + APSObjectActions::NameOf(Site).ToString();
			}
		}
	}
	if (const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld))
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (Unit.Order == APSFleet::EOrder::BuildStructure && Unit.Phase != APSFleet::EPhase::Holding && !Unit.StructureType.IsNone())
			{
				++BuildsUnderWay.FindOrAdd(Unit.StructureType);
			}
		}
	}
	for (int32 Index = 0; Index < static_cast<int32>(APSInfrastructure::EDepartment::Count); ++Index)
	{
		DepartmentLevels[Index] = FAPSInfrastructure::DepartmentLevel(LiveWorld, static_cast<APSInfrastructure::EDepartment>(Index));
	}
}

void SAPSInfrastructurePanel::RefreshSummary()
{
	UWorld* LiveWorld = World.Get();
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	int32 Held = 0;
	int32 Links = 0;
	int32 UnderWay = 0;
	if (Stars && Stars->IsReady())
	{
		TArray<int32> Known;
		Stars->GetKnown(Known);
		for (const int32 Each : Known)
		{
			const FAPSStarSystemInfo* Info = Stars->Get(Each);
			Held += Info && Stars->IsClaimed(Info->Id) ? 1 : 0;
		}
		TArray<TPair<int32, int32>> Network;
		Stars->GetNetwork(Network);
		Links = Network.Num();
	}
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			UnderWay += Unit.Order == APSFleet::EOrder::BuildStructure && Unit.Phase != APSFleet::EPhase::Holding ? 1 : 0;
		}
	}
	SummaryText = FText::Format(LOCTEXT("Summary", "BUILT {0}  /  SYSTEMS HELD {1}  /  RELAY LINKS {2}  /  BUILDS UNDER WAY {3}"),
		APSUINumber::Number(Infrastructure ? Infrastructure->GetStructures().Num() : 0), APSUINumber::Number(Held),
		APSUINumber::Number(Links), APSUINumber::Number(UnderWay));
}

void SAPSInfrastructurePanel::SetView(const int32 View)
{
	ActiveView = FMath::Clamp(View, 0, PageView);
	if (Views.IsValid())
	{
		Views->SetActiveWidgetIndex(ActiveView);
	}
	if (ActiveView != 1)
	{
		PickerType = NAME_None;
	}
	if (ActiveView == 0)
	{
		if (NetworkMap.IsValid())
		{
			NetworkMap->Refresh();
		}
		RefreshNetworkLists(true);
	}
	else if (ActiveView == 1)
	{
		RecountPlaces();
	}
	RefreshSummary();
}

void SAPSInfrastructurePanel::ShowSection(const ESection Section)
{
	History.Reset();
	SetView(static_cast<int32>(Section));
}

void SAPSInfrastructurePanel::ShowObject(AActor* Object)
{
	if (!Object || !Page.IsValid())
	{
		return;
	}
	// From another screen: a fresh trail, BACK leads to the section this tab showed last.
	History.Reset();
	if (ActiveView != PageView)
	{
		ReturnSection = static_cast<ESection>(ActiveView);
	}
	Page->SetPageObject(Object);
	SetView(PageView);
}

void SAPSInfrastructurePanel::OpenObject(AActor* Object)
{
	if (!Object || !Page.IsValid())
	{
		return;
	}
	if (ActiveView == PageView)
	{
		// From one page to another: BACK returns to this one.
		AActor* Current = Page->GetPageObject();
		if (Current && Current != Object)
		{
			History.Add(Current);
			if (History.Num() > 16)
			{
				History.RemoveAt(0);
			}
		}
	}
	else
	{
		ReturnSection = static_cast<ESection>(ActiveView);
		History.Reset();
	}
	Page->SetPageObject(Object);
	SetView(PageView);
}

void SAPSInfrastructurePanel::Back()
{
	while (!History.IsEmpty())
	{
		const TWeakObjectPtr<AActor> Previous = History.Pop();
		if (Previous.IsValid() && Page.IsValid())
		{
			Page->SetPageObject(Previous.Get());
			return;
		}
	}
	SetView(static_cast<int32>(ReturnSection));
}

FText SAPSInfrastructurePanel::BackLabel() const
{
	for (int32 Index = History.Num() - 1; Index >= 0; --Index)
	{
		if (const AActor* Previous = History[Index].Get())
		{
			return APSObjectActions::NameOf(Previous);
		}
	}
	return ReturnSection == ESection::Catalogue ? LOCTEXT("BackCatalogue", "THE CATALOGUE")
		: ReturnSection == ESection::Holdings ? LOCTEXT("BackHoldings", "THE HOLDINGS") : LOCTEXT("BackNetwork", "THE NETWORK MAP");
}

#undef LOCTEXT_NAMESPACE
