#include "SAPSCivilizationMap.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSCivilizationMap"

namespace APSCivilizationMapPrivate
{
	using namespace APSChrome;

	const FSlateBrush* Disc()
	{
		// The brush's default half-height rounding makes a circle of any square box.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	FLinearColor PlanetColour(const APlanet* Planet)
	{
		switch (Planet ? Planet->PlanetType : EPlanetType::Unknown)
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

	template <typename TEnum>
	FText EnumText(const TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? FText::FromString(Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString().ToUpper())
			: FText::GetEmpty();
	}

	FText ActorName(const AActor* Actor)
	{
		if (!Actor)
		{
			return FText::GetEmpty();
		}
		if (const ACelestialBody* Body = Cast<ACelestialBody>(Actor); Body && !Body->AstroName.IsNone())
		{
			return FText::FromString(Body->AstroName.ToString().ToUpper());
		}
		if (Actor->GetClass()->ImplementsInterface(UItemInfoInterface::StaticClass()))
		{
			const FText Name = IItemInfoInterface::Execute_GetInGameName(Actor);
			if (!Name.IsEmpty())
			{
				return FText::FromString(Name.ToString().ToUpper());
			}
		}
		FString Name = Actor->GetName();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.ReplaceInline(TEXT("_C_"), TEXT(" "));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return FText::FromString(Name.ToUpper());
	}

	void Circle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 1.0)
		{
			return;
		}
		const int32 Segments = FMath::Clamp(FMath::RoundToInt(Radius * 0.6), 24, 160);
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / Segments;
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour,
			true, Thickness);
	}

	/** A dashed circle: a boundary, not an orbit. */
	void DashedCircle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 1.0)
		{
			return;
		}
		const int32 Dashes = FMath::Clamp(FMath::RoundToInt(Radius * 0.25), 24, 96);
		for (int32 Index = 0; Index < Dashes; ++Index)
		{
			const double From = UE_TWO_PI * Index / Dashes;
			const double To = From + UE_TWO_PI * 0.55 / Dashes;
			TArray<FVector2D> Points;
			for (int32 Step = 0; Step <= 3; ++Step)
			{
				const double Angle = FMath::Lerp(From, To, Step / 3.0);
				Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour,
				true, Thickness);
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

	void Polyline(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry,
		const TArray<FVector2D>& Points, const FLinearColor& Colour, const float Thickness)
	{
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Colour,
			true, Thickness);
	}

	void Label(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Position,
		const FText& Text, const FSlateFontInfo& FontInfo, const FLinearColor& Colour)
	{
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(FVector2f(260.0f, 18.0f),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y)))),
			Text, FontInfo, ESlateDrawEffect::None, Colour);
	}

	/** A dark rounded plate under a map name, so it reads over the orbits and the star's glow (Rio 02.10). */
	void Plate(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& TopLeft,
		const FVector2D& Size)
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 3.0f);
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(TopLeft.X), static_cast<float>(TopLeft.Y)))),
			&Brush, ESlateDrawEffect::None, FLinearColor(0.0f, 0.014f, 0.024f, 0.74f));
	}

	/** Four corner brackets around a point, as the ship HUD marks its course target. */
	void Brackets(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Half, const FLinearColor& Colour)
	{
		const double Arm = FMath::Clamp(Half * 0.45, 4.0, 10.0);
		for (const FVector2D& Corner : {FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0),
			FVector2D(-1.0, 1.0)})
		{
			const FVector2D Tip = Centre + Corner * Half;
			Polyline(Out, Layer, Geometry, {Tip - FVector2D(Corner.X * Arm, 0.0), Tip, Tip - FVector2D(0.0, Corner.Y * Arm)},
				Colour, 1.6f);
		}
	}

	/** A dashed line, the look of an order under way. */
	void Dashes(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& From,
		const FVector2D& To, const FLinearColor& Colour)
	{
		const double Length = FVector2D::Distance(From, To);
		if (Length < 2.0)
		{
			return;
		}
		const FVector2D Along = (To - From) / Length;
		for (double Start = 0.0; Start < Length; Start += 10.0)
		{
			Polyline(Out, Layer, Geometry, {From + Along * Start, From + Along * FMath::Min(Start + 6.0, Length)}, Colour, 1.4f);
		}
	}

	/** An arc from the top, clockwise, for a share of a full turn (work progress). */
	void Arc(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const double Share, const FLinearColor& Colour)
	{
		const int32 Segments = FMath::Max(2, FMath::RoundToInt(48.0 * FMath::Clamp(Share, 0.0, 1.0)));
		TArray<FVector2D> Points;
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const double Angle = UE_TWO_PI * FMath::Clamp(Share, 0.0, 1.0) * Index / Segments - UE_HALF_PI;
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		Polyline(Out, Layer, Geometry, Points, Colour, 2.0f);
	}

	/** Objects not drawn in the current view keep this position and cannot be picked. */
	const FVector2D Unpainted(-1.0e9, -1.0e9);

	bool IsPainted(const FVector2D& Position)
	{
		return Position.X > -1.0e8;
	}
}

void SAPSCivilizationMap::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	// Markers at the rim (the star's direction) stay on the map, not on the panels beside it.
	SetClipping(EWidgetClipping::ClipToBounds);
	Refresh();
}

void SAPSCivilizationMap::Refresh()
{
	using namespace APSCivilizationMapPrivate;
	UWorld* LiveWorld = World.Get();
	Objects.Reset();
	if (!LiveWorld)
	{
		return;
	}
	const APlayerController* Controller = LiveWorld->GetFirstPlayerController();
	APawn* Pilot = Controller ? Controller->GetPawn() : nullptr;
	const FVector PilotLocation = Pilot ? Pilot->GetActorLocation() : FVector::ZeroVector;

	// The system the pilot is in: the nearest materialized star and its planets.
	AStar* SystemStar = nullptr;
	double BestStarDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AStar> It(LiveWorld); It; ++It)
	{
		const double Distance = IsValid(*It) ? FVector::DistSquared(It->GetActorLocation(), PilotLocation)
			: TNumericLimits<double>::Max();
		if (Distance < BestStarDistance)
		{
			BestStarDistance = Distance;
			SystemStar = *It;
		}
	}
	Star = SystemStar;
	if (!SystemStar)
	{
		return;
	}
	TArray<APlanet*> Planets;
	for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && It->ParentStar == SystemStar)
		{
			Planets.Add(*It);
		}
	}

	// The map plane is the planets' orbital plane.
	FVector Normal = FVector::UpVector;
	for (const APlanet* Planet : Planets)
	{
		if (const AActor* Orbit = Planet->GetAttachParentActor(); Orbit && Orbit->IsA<APlanetOrbit>())
		{
			Normal = Orbit->GetActorUpVector();
			break;
		}
	}
	PlaneU = FVector::VectorPlaneProject(FVector::ForwardVector, Normal).GetSafeNormal();
	if (PlaneU.IsNearlyZero())
	{
		PlaneU = FVector::VectorPlaneProject(FVector::RightVector, Normal).GetSafeNormal();
	}
	PlaneV = FVector::CrossProduct(Normal, PlaneU).GetSafeNormal();

	const auto Add = [this](AActor* Actor, const EKind Kind, const FText& Detail, const FLinearColor& Colour,
		const double RadiusCm, AActor* Anchor, const bool bOwn) -> FObject&
	{
		FObject& Object = Objects.AddDefaulted_GetRef();
		Object.Actor = Actor;
		Object.Kind = Kind;
		Object.StableId = Actor->GetPathName();
		Object.Name = APSCivilizationMapPrivate::ActorName(Actor);
		Object.Detail = Detail;
		Object.Color = Colour;
		Object.Location = Actor->GetActorLocation();
		Object.RadiusCm = RadiusCm;
		Object.Anchor = Anchor;
		Object.bOwn = bOwn;
		return Object;
	};
	// A moon's, station's or ship's planet: the nearest one, counted in its own radii (moons reach ~60 of them).
	const auto AnchorOf = [&Planets](const FVector& Location) -> APlanet*
	{
		APlanet* Best = nullptr;
		double BestRatio = TNumericLimits<double>::Max();
		for (APlanet* Planet : Planets)
		{
			const double Ratio = FVector::Distance(Location, Planet->GetActorLocation())
				/ FMath::Max(Planet->GetWorldScapeBodyRadiusCm(), 1.0);
			if (Ratio < BestRatio)
			{
				BestRatio = Ratio;
				Best = Planet;
			}
		}
		return BestRatio <= 400.0 ? Best : nullptr;
	};

	Add(SystemStar, EKind::Star, SystemStar->FullSpectralClass.IsNone()
		? LOCTEXT("StarDetail", "STAR") : FText::FromString(SystemStar->FullSpectralClass.ToString().ToUpper()),
		FLinearColor(1.0f, 0.8f, 0.36f), SystemStar->StarRadiusKM * 100000.0, nullptr, false);
	for (APlanet* Planet : Planets)
	{
		Add(Planet, EKind::Planet, EnumText(Planet->PlanetType),
			PlanetColour(Planet), Planet->GetWorldScapeBodyRadiusCm(), Planet, false);
	}
	for (TActorIterator<AMoon> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && Planets.Contains(It->ParentPlanet))
		{
			Add(*It, EKind::Moon, EnumText(It->MoonType),
				FLinearColor(0.7f, 0.76f, 0.82f), It->GetWorldScapeBodyRadiusCm(), It->ParentPlanet, false);
		}
	}
	for (TActorIterator<ASpaceStation> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		const FText Detail = It->IsA<ASpaceHeadquarters>() ? LOCTEXT("HeadquartersDetail", "HEADQUARTERS")
			: It->IsA<ASpaceShipyard>() ? LOCTEXT("ShipyardDetail", "SHIPYARD") : LOCTEXT("StationDetail", "STATION");
		// Rio 02.10: the headquarters stands out in gold (its own marker and legend entry); stations and shipyards cyan.
		Add(*It, EKind::Station, Detail, It->IsA<ASpaceHeadquarters>() ? FLinearColor(1.0f, 0.85f, 0.38f) : Cyan(), 0.0,
			AnchorOf(It->GetActorLocation()), true);
	}
	for (TActorIterator<AColony> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		const UAPSCivilizationIdentityComponent* Identity = It->FindComponentByClass<UAPSCivilizationIdentityComponent>();
		const bool bOwnColony = It->ActorHasTag(TEXT("APS.Civilization.Materialized")) && Identity
			&& Identity->Role == EAPSCivilizationEntityRole::BaseModule;
		FObject& Colony = Add(*It, bOwnColony ? EKind::Colony : EKind::Settlement,
			bOwnColony ? LOCTEXT("ColonyDetail", "HOME COLONY") : LOCTEXT("SettlementDetail", "SETTLEMENT"),
			bOwnColony ? FLinearColor(0.36f, 1.0f, 0.58f) : Muted(), 0.0, AnchorOf(It->GetActorLocation()), bOwnColony);
		if (bOwnColony)
		{
			Colony.Name = LOCTEXT("ColonyName", "HOME COLONY");
		}
	}
	// Anomaly beacons at located sites (fleet command, Rio 01.10): a course can be set to them.
	for (TActorIterator<AActor> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(TEXT("APS.Fleet.Anomaly")))
		{
			// The ancient sites (Gameplay/Ancients) carry the anomaly tag too; they read as what they are, in pale cyan.
			const bool bAncient = It->ActorHasTag(TEXT("APS.Ancient.Site"));
			Add(*It, EKind::Outpost, bAncient ? LOCTEXT("AncientDetail", "ANCIENT SITE") : LOCTEXT("AnomalyDetail", "ANOMALY SITE"),
				bAncient ? FLinearColor(0.25f, 0.9f, 1.0f) : FLinearColor(1.0f, 0.62f, 0.2f), 0.0,
				AnchorOf(It->GetActorLocation()), true);
		}
	}
	for (TActorIterator<AAutonomousOutpost> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It))
		{
			Add(*It, EKind::Outpost, LOCTEXT("OutpostDetail", "AUTONOMOUS OUTPOST"), Amber(), 0.0,
				AnchorOf(It->GetActorLocation()), true);
		}
	}
	// Ships in their division's colour, named by their call sign, as the fleet command lists them.
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	for (TActorIterator<ASpaceship> It(LiveWorld); It; ++It)
	{
		ASpaceship* Ship = *It;
		const bool bOwn = IsValid(Ship) && (Ship->ActorHasTag(TEXT("APS.Fleet.HomeShip"))
			|| Ship->ActorHasTag(TEXT("APS.Fleet.Escort")) || Ship->ActorHasTag(TEXT("APS.Civilization.Materialized"))
			|| Ship->ActorHasTag(TEXT("APS.Fleet.Unit")));
		if (!IsValid(Ship) || (!bOwn && Ship != Pilot))
		{
			continue;
		}
		const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
		FObject& Object = Add(Ship, EKind::Ship, Unit
			? FText::Format(LOCTEXT("UnitDetail", "CLASS {0}  /  {1}"), EnumText(Ship->SizeClass), APSFleet::DivisionName(Unit->Division))
			: Ship->IsGroundVehicle() ? LOCTEXT("VehicleDetail", "GROUND VEHICLE")
			: FText::Format(LOCTEXT("ShipDetail", "CLASS {0} SHIP"), EnumText(Ship->SizeClass)),
			Unit ? APSFleet::DivisionColour(Unit->Division) : bOwn ? FLinearColor(0.36f, 1.0f, 0.58f) : White(), 0.0,
			AnchorOf(Ship->GetActorLocation()), bOwn);
		if (Unit)
		{
			Object.Name = FText::FromString(Unit->CallSign);
		}
	}
	if (Pilot)
	{
		FObject& You = Add(Pilot, EKind::Pilot, LOCTEXT("PilotDetail", "YOUR POSITION"), White(), 0.0,
			AnchorOf(PilotLocation), true);
		You.Name = LOCTEXT("You", "YOU");
	}
	// Keep a local view on its planet; otherwise reframe.
	Focus(FocusPlanet.Get());
}

void SAPSCivilizationMap::Focus(AActor* Planet)
{
	// Refresh() re-focuses the same view every few frames: only a new view (system or a planet's space) starts whole.
	if (FocusPlanet.Get() != Planet)
	{
		MapZoom = 1.0;
		MapOffset = FVector2D::ZeroVector;
	}
	FocusPlanet = Planet;
	const AActor* Centre = Planet ? Planet : Star.Get();
	RangeCm = 1.0;
	KneeCm = 1.0;
	if (!Centre)
	{
		return;
	}
	const FVector CentreLocation = Centre->GetActorLocation();
	const auto InPlane = [this, &CentreLocation](const FVector& Location)
	{
		const FVector Relative = Location - CentreLocation;
		return FVector2D(FVector::DotProduct(Relative, PlaneU), FVector::DotProduct(Relative, PlaneV)).Size();
	};
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Planet))
	{
		// Local view: the planet's neighbourhood, from its surface out to its farthest moon or station. Rio 02.10 ("a ship
		// leaving on an order must not zoom the planet out unless that one ship is picked"): ships widen the view only
		// when exactly one is picked and it is that ship.
		const double PlanetRadius = FMath::Max(Body->GetWorldScapeBodyRadiusCm(), 1.0);
		double Farthest = PlanetRadius * 5.0;
		const AActor* FollowedShip = HighlightedShips.Num() == 1 ? HighlightedShips[0].Get() : nullptr;
		for (const FObject& Object : Objects)
		{
			if (Object.Anchor.Get() == Planet && Object.Actor.Get() != Planet
				&& (Object.Kind != EKind::Ship || Object.Actor.Get() == FollowedShip))
			{
				Farthest = FMath::Max(Farthest, InPlane(Object.Location));
			}
		}
		RangeCm = Farthest * 1.12;
		KneeCm = PlanetRadius * 0.6;
	}
	else
	{
		// System view: from the star out to the outermost planet.
		double Nearest = TNumericLimits<double>::Max();
		double Farthest = 0.0;
		for (const FObject& Object : Objects)
		{
			if (Object.Kind == EKind::Planet)
			{
				const double Distance = InPlane(Object.Location);
				Nearest = FMath::Min(Nearest, Distance);
				Farthest = FMath::Max(Farthest, Distance);
			}
		}
		if (Farthest <= 0.0)
		{
			Farthest = 1.5e13;
			Nearest = 5.0e12;
		}
		RangeCm = Farthest * 1.08;
		KneeCm = FMath::Max(Nearest * 0.35, 1.0e9);
	}
}

const SAPSCivilizationMap::FObject* SAPSCivilizationMap::GetSelected() const
{
	return Objects.FindByPredicate([this](const FObject& Object) { return Object.StableId == SelectedId; });
}

void SAPSCivilizationMap::GetShownObjects(TArray<const FObject*>& OutObjects) const
{
	OutObjects.Reset();
	for (const FObject& Object : Objects)
	{
		if (IsShown(Object))
		{
			OutObjects.Add(&Object);
		}
	}
}

void SAPSCivilizationMap::SelectById(const FString& StableId)
{
	SelectedId = StableId;
	OnSelectionChanged.ExecuteIfBound();
}

FText SAPSCivilizationMap::GetViewTitle() const
{
	if (const AActor* Planet = FocusPlanet.Get())
	{
		return FText::Format(LOCTEXT("LocalTitle", "{0}  /  LOCAL VIEW"), APSCivilizationMapPrivate::ActorName(Planet));
	}
	return FText::Format(LOCTEXT("SystemTitle", "SYSTEM  /  {0}"), Star.IsValid()
		? APSCivilizationMapPrivate::ActorName(Star.Get()) : LOCTEXT("NoStar", "NO STAR CHARTED"));
}

bool SAPSCivilizationMap::IsShown(const FObject& Object) const
{
	if (const AActor* Planet = FocusPlanet.Get())
	{
		return Object.Anchor.Get() == Planet;
	}
	switch (Object.Kind)
	{
	case EKind::Star:
	case EKind::Planet:
	case EKind::Pilot:
		return true;
	case EKind::Ship:
		// At system scale a ship beside its planet sits in the planet's presence badge.
		return !Object.Anchor.IsValid();
	default:
		return false;
	}
}

double SAPSCivilizationMap::ProjectRadius(const double DistanceCm, const double PixelRadius) const
{
	const double Scale = FMath::Loge(1.0 + FMath::Max(DistanceCm, 0.0) / KneeCm)
		/ FMath::Max(FMath::Loge(1.0 + RangeCm / KneeCm), 1.0e-6);
	return PixelRadius * FMath::Min(Scale, 1.06);
}

FVector2D SAPSCivilizationMap::Project(const FVector& WorldLocation, const FVector2D& Centre,
	const double PixelRadius) const
{
	const AActor* ViewCentre = FocusPlanet.IsValid() ? FocusPlanet.Get() : Star.Get();
	if (!ViewCentre)
	{
		return Centre;
	}
	const FVector Relative = WorldLocation - ViewCentre->GetActorLocation();
	const FVector2D InPlane(FVector::DotProduct(Relative, PlaneU), FVector::DotProduct(Relative, PlaneV));
	const double Distance = InPlane.Size();
	if (Distance < 1.0)
	{
		return Centre;
	}
	return Centre + FVector2D(InPlane.X, -InPlane.Y) / Distance * ProjectRadius(Distance, PixelRadius);
}

int32 SAPSCivilizationMap::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace APSCivilizationMapPrivate;
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	// The whole projection scales with the radius, so the wheel zoom is a larger radius and the pan a moved centre.
	const FVector2D Centre = Size * 0.5 + MapOffset;
	const double PixelRadius = FMath::Max(FMath::Min(Size.X, Size.Y) * 0.5 - 38.0, 40.0) * MapZoom;
	const FSlateFontInfo LabelFont = Font(TEXT("Bold"), 9);
	const FSlateFontInfo SmallFont = Font(TEXT("Regular"), 9);
	PaintedPositions.Init(Unpainted, Objects.Num());
	PaintedDiscRadius = 0.0;
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());

	// Only real orbits are drawn as rings (Rio, 01.10: the quarter-view guide rings read as orbits nobody has); the edge
	// of the view, just past the outermost planet or the planet's farthest moon, station or ship, is a dashed boundary.
	const AActor* Planet = FocusPlanet.Get();
	{
		const FLinearColor EdgeColour(0.26f, 0.84f, 0.93f, 0.30f);
		DashedCircle(OutDrawElements, LayerId, AllottedGeometry, Centre, PixelRadius, EdgeColour, 1.0f);
		const FVector2D EdgeLabel = Centre + FVector2D(0.70710678, -0.70710678) * PixelRadius + FVector2D(6.0, -14.0);
		Label(OutDrawElements, LayerId + 1, AllottedGeometry, EdgeLabel,
			Planet ? LOCTEXT("PlanetSpaceEdge", "EDGE OF LOCAL SPACE") : LOCTEXT("SystemEdge", "EDGE OF THE SYSTEM"), SmallFont,
			EdgeColour * FLinearColor(1.0f, 1.0f, 1.0f, 2.2f));
	}
	const AActor* ViewCentre = Planet ? Planet : Star.Get();
	const auto InPlaneDistance = [this, ViewCentre](const FVector& Location)
	{
		const FVector Relative = Location - ViewCentre->GetActorLocation();
		return FVector2D(FVector::DotProduct(Relative, PlaneU), FVector::DotProduct(Relative, PlaneV)).Size();
	};
	if (ViewCentre)
	{
		// Orbits: planets around the star, or moons around the focused planet.
		for (const FObject& Object : Objects)
		{
			const bool bOrbit = Planet ? Object.Kind == EKind::Moon && Object.Anchor.Get() == Planet
				: Object.Kind == EKind::Planet;
			if (bOrbit)
			{
				Circle(OutDrawElements, LayerId + 1, AllottedGeometry, Centre,
					ProjectRadius(InPlaneDistance(Object.Location), PixelRadius),
					FLinearColor(Object.Color.R, Object.Color.G, Object.Color.B, 0.22f), 1.0f);
			}
		}
		if (Planet)
		{
			// The star is off the local view: a small sun on the rim shows its direction.
			if (const AActor* SystemStar = Star.Get())
			{
				const FVector Relative = SystemStar->GetActorLocation() - Planet->GetActorLocation();
				FVector2D Direction(FVector::DotProduct(Relative, PlaneU), -FVector::DotProduct(Relative, PlaneV));
				if (Direction.Normalize())
				{
					const FVector2D Sun = Centre + Direction * FMath::Min(PixelRadius + 18.0, FMath::Min(Size.X, Size.Y) * 0.5 - 16.0);
					Dot(OutDrawElements, LayerId + 2, AllottedGeometry, Sun, 5.0, FLinearColor(1.0f, 0.8f, 0.36f, 0.9f));
					Label(OutDrawElements, LayerId + 3, AllottedGeometry, Sun + FVector2D(8.0, -7.0),
						LOCTEXT("SunDirection", "STAR"), SmallFont, FLinearColor(1.0f, 0.8f, 0.36f, 0.8f));
				}
			}
		}
	}

	// Where the civilization is, at system scale: a badge beside each planet with its stations, colony and ships.
	TMap<const AActor*, int32> Presence;
	if (!Planet)
	{
		for (const FObject& Object : Objects)
		{
			if (Object.bOwn && Object.Kind != EKind::Pilot && Object.Anchor.IsValid())
			{
				++Presence.FindOrAdd(Object.Anchor.Get());
			}
		}
	}

	// Names placed so far: a name that would overlap one is left to the hover, so a shipyard with ten ships, the colony
	// and the outposts beside it stay readable (objects come planets and moons first, then stations, then ships).
	TArray<FSlateRect> PlacedLabels;
	const auto PlaceLabel = [&PlacedLabels](const FVector2D& At, const FText& Text, const double Lines)
	{
		const FSlateRect Rect(At.X, At.Y, At.X + 7.2 * FMath::Max(Text.ToString().Len(), 4), At.Y + 13.0 * Lines);
		for (const FSlateRect& Placed : PlacedLabels)
		{
			if (Rect.Left < Placed.Right + 4.0 && Placed.Left < Rect.Right + 4.0 && Rect.Top < Placed.Bottom
				&& Placed.Top < Rect.Bottom)
			{
				return false;
			}
		}
		PlacedLabels.Add(Rect);
		return true;
	};
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	// YOU is always written: its place is taken first.
	for (const FObject& Object : Objects)
	{
		if (Object.Kind == EKind::Pilot && IsShown(Object))
		{
			PlaceLabel(Project(Object.Location, Centre, PixelRadius) + FVector2D(14.0, -20.0), Object.Name, 1.0);
		}
	}
	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		const FObject& Object = Objects[Index];
		if (!IsShown(Object))
		{
			continue;
		}
		const FVector2D Position = Project(Object.Location, Centre, PixelRadius);
		PaintedPositions[Index] = Position;
		const FLinearColor& Colour = Object.Color;
		switch (Object.Kind)
		{
		case EKind::Star:
			Dot(OutDrawElements, LayerId + 2, AllottedGeometry, Position, 14.0,
				FLinearColor(Colour.R, Colour.G, Colour.B, 0.25f));
			Dot(OutDrawElements, LayerId + 3, AllottedGeometry, Position, 7.0, Colour);
			break;
		case EKind::Planet:
			if (Object.Actor.Get() == Planet)
			{
				// The focused planet as a disc of its real (logarithmic) size.
				PaintedDiscRadius = FMath::Max(ProjectRadius(Object.RadiusCm, PixelRadius), 18.0);
				Dot(OutDrawElements, LayerId + 2, AllottedGeometry, Position, PaintedDiscRadius,
					FLinearColor(Colour.R * 0.5f, Colour.G * 0.5f, Colour.B * 0.5f, 0.85f));
				Circle(OutDrawElements, LayerId + 3, AllottedGeometry, Position, PaintedDiscRadius, Colour, 1.2f);
			}
			else
			{
				Dot(OutDrawElements, LayerId + 3, AllottedGeometry, Position, 6.5, Colour);
			}
			break;
		case EKind::Moon:
			Dot(OutDrawElements, LayerId + 3, AllottedGeometry, Position, 4.5, Colour);
			break;
		case EKind::Station:
		{
			const AActor* Station = Object.Actor.Get();
			if (Station && Station->IsA<ASpaceHeadquarters>())
			{
				// Rio 02.10 ("where the HQ is"): the headquarters as a house in a soft halo, larger than a station.
				const double Half = 7.0;
				Dot(OutDrawElements, LayerId + 3, AllottedGeometry, Position, 13.0,
					FLinearColor(Colour.R, Colour.G, Colour.B, 0.16f));
				Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(-Half, -1.0),
					Position + FVector2D(0.0, -Half - 2.0), Position + FVector2D(Half, -1.0), Position + FVector2D(Half, Half),
					Position + FVector2D(-Half, Half), Position + FVector2D(-Half, -1.0)}, Colour, 1.8f);
				Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(-2.2, Half),
					Position + FVector2D(-2.2, 2.0), Position + FVector2D(2.2, 2.0), Position + FVector2D(2.2, Half)},
					Colour, 1.4f);
			}
			else if (Station && Station->IsA<ASpaceShipyard>())
			{
				// A shipyard: an open dock under its gantry.
				const double Half = 6.0;
				Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(-Half, -Half),
					Position + FVector2D(-Half, Half), Position + FVector2D(Half, Half), Position + FVector2D(Half, -Half)},
					Colour, 1.6f);
				Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(-Half - 2.5, -Half),
					Position + FVector2D(Half + 2.5, -Half)}, Colour, 1.6f);
			}
			else
			{
				const double Half = 5.0;
				Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(-Half, -Half),
					Position + FVector2D(Half, -Half), Position + FVector2D(Half, Half), Position + FVector2D(-Half, Half),
					Position + FVector2D(-Half, -Half)}, Colour, 1.6f);
				Dot(OutDrawElements, LayerId + 4, AllottedGeometry, Position, 1.8, Colour);
			}
			break;
		}
		case EKind::Colony:
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, Position, 5.5, Colour);
			Circle(OutDrawElements, LayerId + 4, AllottedGeometry, Position, 10.0, Colour, 1.2f);
			break;
		case EKind::Settlement:
			Dot(OutDrawElements, LayerId + 4, AllottedGeometry, Position, 4.0, Colour);
			break;
		case EKind::Ship:
		{
			const double Tip = 7.0;
			Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(0.0, -Tip),
				Position + FVector2D(Tip * 0.75, Tip * 0.7), Position + FVector2D(-Tip * 0.75, Tip * 0.7),
				Position + FVector2D(0.0, -Tip)}, Colour, 1.6f);
			break;
		}
		case EKind::Outpost:
		{
			const double Half = 4.5;
			Polyline(OutDrawElements, LayerId + 4, AllottedGeometry, {Position + FVector2D(0.0, -Half),
				Position + FVector2D(Half, 0.0), Position + FVector2D(0.0, Half), Position + FVector2D(-Half, 0.0),
				Position + FVector2D(0.0, -Half)}, Colour, 1.6f);
			break;
		}
		case EKind::Pilot:
			Circle(OutDrawElements, LayerId + 5, AllottedGeometry, Position, 12.0, White(), 1.4f);
			Label(OutDrawElements, LayerId + 6, AllottedGeometry, Position + FVector2D(14.0, -20.0), Object.Name,
				LabelFont, White());
			break;
		default:
			break;
		}
		if (const int32* Count = Presence.Find(Object.Actor.Get()))
		{
			const FVector2D Badge = Position + FVector2D(9.0, -9.0);
			Dot(OutDrawElements, LayerId + 5, AllottedGeometry, Badge, 4.0, FLinearColor(0.36f, 1.0f, 0.58f));
			Label(OutDrawElements, LayerId + 6, AllottedGeometry, Badge + FVector2D(5.0, -8.0),
				APSUINumber::Number(*Count), SmallFont, FLinearColor(0.36f, 1.0f, 0.58f));
		}
		// Names: always for the star and planets, and for everything in a local view.
		const bool bNamed = Object.Kind == EKind::Star || Object.Kind == EKind::Planet || Planet != nullptr;
		const bool bSurveyTag = Fleet && (Object.Kind == EKind::Planet || Object.Kind == EKind::Moon);
		const double Below = Object.Kind == EKind::Planet && Object.Actor.Get() == Planet ? PaintedDiscRadius : 9.0;
		if (bNamed && Object.Kind != EKind::Pilot
			&& PlaceLabel(Position + FVector2D(-30.0, Below + 3.0), Object.Name, bSurveyTag ? 2.0 : 1.0))
		{
			// Rio 02.10 ("labels hard to read"): a dark plate under the name and its survey line.
			const double PlateWidth = FMath::Max(FontMeasure->Measure(Object.Name, LabelFont).X, bSurveyTag
				? FontMeasure->Measure(APSFleet::SurveyName(Fleet->GetSurvey(Object.Actor.Get())), SmallFont).X : 0.0);
			Plate(OutDrawElements, LayerId + 5, AllottedGeometry, Position + FVector2D(-34.0, Below + 2.0),
				FVector2D(PlateWidth + 8.0, bSurveyTag ? 28.0 : 15.0));
			// Rio 03.10: each line sits on the plate by its capitals' middle (7.5 px from the plate's top; the survey
			// line 12.5 px under it), not by Slate's line box.
			const auto LineTop = [&FontMeasure](const FSlateFontInfo& LineFont, const double CapsMiddle)
			{
				return CapsMiddle - FontMeasure->GetMaxCharacterHeight(LineFont) * 0.5 + CapsCenterOffset(LineFont);
			};
			Label(OutDrawElements, LayerId + 6, AllottedGeometry,
				Position + FVector2D(-30.0, LineTop(LabelFont, Below + 9.5)), Object.Name,
				LabelFont, FLinearColor(Colour.R, Colour.G, Colour.B, 0.95f));
			// What the civilization knows of the world: its survey under the name.
			if (bSurveyTag)
			{
				const APSFleet::ESurvey Survey = Fleet->GetSurvey(Object.Actor.Get());
				Label(OutDrawElements, LayerId + 6, AllottedGeometry, Position + FVector2D(-30.0, LineTop(SmallFont, Below + 22.0)),
					APSFleet::SurveyName(Survey), SmallFont, Survey == APSFleet::ESurvey::Unknown
						? FLinearColor(0.55f, 0.62f, 0.66f, 0.7f) : Survey == APSFleet::ESurvey::Surveyed ? Cyan()
						: APSFleet::DivisionColour(APSFleet::EDivision::Science));
			}
		}
	}

	// Orders: a dashed line from each ship under way to its target, a progress arc round a ship at work, and a ring
	// round the ships picked for orders. At system scale a ship or target beside its planet stands at the planet.
	const auto PaintedOrAnchor = [this](const AActor* Actor) -> FVector2D
	{
		const int32 Index = Actor ? Objects.IndexOfByPredicate([Actor](const FObject& Object)
		{
			return Object.Actor.Get() == Actor;
		}) : INDEX_NONE;
		if (Index == INDEX_NONE)
		{
			return Unpainted;
		}
		if (IsPainted(PaintedPositions[Index]))
		{
			return PaintedPositions[Index];
		}
		const AActor* Anchor = Objects[Index].Anchor.Get();
		const int32 AnchorIndex = Anchor ? Objects.IndexOfByPredicate([Anchor](const FObject& Object)
		{
			return Object.Actor.Get() == Anchor && Object.Kind == EKind::Planet;
		}) : INDEX_NONE;
		return AnchorIndex != INDEX_NONE ? PaintedPositions[AnchorIndex] : Unpainted;
	};
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			const FVector2D From = PaintedOrAnchor(Unit.Ship.Get());
			if (!IsPainted(From))
			{
				continue;
			}
			const FLinearColor Colour = APSFleet::DivisionColour(Unit.Division);
			if (Unit.Phase == APSFleet::EPhase::Departing || Unit.Phase == APSFleet::EPhase::Transit)
			{
				const FVector2D To = PaintedOrAnchor(Unit.Order == APSFleet::EOrder::Return ? Unit.Berth.Get() : Unit.Target.Get());
				if (IsPainted(To))
				{
					const FLinearColor Line(Colour.R, Colour.G, Colour.B, 0.8f);
					Dashes(OutDrawElements, LayerId + 5, AllottedGeometry, From, To, Line);
					Circle(OutDrawElements, LayerId + 5, AllottedGeometry, To, 9.0, Line, 1.2f);
				}
			}
			else if (Unit.Phase == APSFleet::EPhase::Working)
			{
				Arc(OutDrawElements, LayerId + 5, AllottedGeometry, From, 11.0, Unit.Progress, Colour);
			}
		}
	}
	for (const TWeakObjectPtr<AActor>& Ship : HighlightedShips)
	{
		const FVector2D At = PaintedOrAnchor(Ship.Get());
		if (IsPainted(At))
		{
			Circle(OutDrawElements, LayerId + 6, AllottedGeometry, At, 15.0, White(), 1.4f);
		}
	}

	// The course target, the selection and the hovered object.
	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		const FObject& Object = Objects[Index];
		FVector2D Position = PaintedPositions[Index];
		if (!IsPainted(Position) && Object.StableId == CourseTargetId && Object.Anchor.IsValid())
		{
			// At system scale the target sits in its planet's badge: mark the planet.
			const int32 AnchorIndex = Objects.IndexOfByPredicate([&Object](const FObject& Candidate)
			{
				return Candidate.Actor == Object.Anchor && Candidate.Kind == EKind::Planet;
			});
			Position = AnchorIndex != INDEX_NONE ? PaintedPositions[AnchorIndex] : Position;
		}
		if (!IsPainted(Position))
		{
			continue;
		}
		const double Radius = Object.Kind == EKind::Planet && Object.Actor.Get() == Planet ? PaintedDiscRadius + 6.0
			: 13.0;
		if (Object.StableId == CourseTargetId)
		{
			Brackets(OutDrawElements, LayerId + 7, AllottedGeometry, Position, Radius + 3.0, Amber());
		}
		if (Object.StableId == SelectedId)
		{
			Circle(OutDrawElements, LayerId + 7, AllottedGeometry, Position, Radius, White(), 1.6f);
		}
		else if (Index == HoverIndex)
		{
			Circle(OutDrawElements, LayerId + 7, AllottedGeometry, Position, Radius, Cyan(), 1.2f);
			Label(OutDrawElements, LayerId + 8, AllottedGeometry, Position + FVector2D(Radius + 4.0, -8.0),
				Object.Name, LabelFont, Cyan());
		}
	}

	// Legend and scale note.
	struct FLegend
	{
		FText Text;
		FLinearColor Colour;
	};
	TArray<FLegend> Legend = {
		{LOCTEXT("LegendColony", "COLONY"), FLinearColor(0.36f, 1.0f, 0.58f)},
		{LOCTEXT("LegendHeadquarters", "HQ"), FLinearColor(1.0f, 0.85f, 0.38f)},
		{LOCTEXT("LegendStation", "STATION"), Cyan()},
		{LOCTEXT("LegendOutpost", "OUTPOST"), Amber()},
		{LOCTEXT("LegendCourse", "COURSE"), Amber()},
		{LOCTEXT("LegendYou", "YOU"), White()}};
	if (!Fleet)
	{
		Legend.Insert({LOCTEXT("LegendShip", "OUR SHIP"), FLinearColor(0.36f, 1.0f, 0.58f)}, 3);
	}
	FVector2D Row(12.0, Size.Y - 18.0 * Legend.Num() - 8.0);
	for (const FLegend& Entry : Legend)
	{
		Dot(OutDrawElements, LayerId + 8, AllottedGeometry, Row + FVector2D(4.0, 8.0), 3.5, Entry.Colour);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Row + FVector2D(14.0, 0.0), Entry.Text, SmallFont,
			Muted());
		Row.Y += 18.0;
	}
	// Ships take their division's colour.
	if (Fleet)
	{
		FVector2D Column(104.0, Size.Y - 18.0 * (static_cast<int32>(APSFleet::EDivision::Count) + 1) - 8.0);
		Label(OutDrawElements, LayerId + 8, AllottedGeometry, Column + FVector2D(14.0, 0.0), LOCTEXT("LegendShips", "SHIPS"),
			SmallFont, Muted());
		for (int32 Division = 0; Division < static_cast<int32>(APSFleet::EDivision::Count); ++Division)
		{
			Column.Y += 18.0;
			const APSFleet::EDivision Kind = static_cast<APSFleet::EDivision>(Division);
			Dot(OutDrawElements, LayerId + 8, AllottedGeometry, Column + FVector2D(4.0, 8.0), 3.5, APSFleet::DivisionColour(Kind));
			Label(OutDrawElements, LayerId + 8, AllottedGeometry, Column + FVector2D(14.0, 0.0), APSFleet::DivisionName(Kind),
				SmallFont, Muted());
		}
	}
	Label(OutDrawElements, LayerId + 8, AllottedGeometry, FVector2D(Size.X - 250.0, Size.Y - 24.0),
		Planet ? LOCTEXT("LocalScale", "Distances logarithmic  /  SYSTEM VIEW returns")
			: LOCTEXT("SystemScale", "Distances logarithmic  /  double-click a planet"),
		SmallFont, Muted());
	return LayerId + 8;
}

int32 SAPSCivilizationMap::HitTest(const FVector2D& LocalPosition) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = 16.0;
	for (int32 Index = 0; Index < PaintedPositions.Num() && Index < Objects.Num(); ++Index)
	{
		if (!APSCivilizationMapPrivate::IsPainted(PaintedPositions[Index]))
		{
			continue;
		}
		double Distance = FVector2D::Distance(PaintedPositions[Index], LocalPosition);
		if (Objects[Index].Actor.Get() == FocusPlanet.Get() && Objects[Index].Kind == EKind::Planet)
		{
			// The focused planet is a disc: anywhere on it picks it, unless a smaller object is closer.
			Distance = Distance <= PaintedDiscRadius ? 15.9 : Distance;
		}
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Index;
		}
	}
	return Best;
}

FReply SAPSCivilizationMap::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
	HoverIndex = HitTest(Local);
	return FReply::Unhandled();
}

FReply SAPSCivilizationMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SAPSCivilizationMap::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	if (!bPanning && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		// Rio 02.10 ("objects right on top of each other: a click opens a little list beside them, pick one there"): two
		// or more drawn objects under the cursor (the focused planet's disc aside) open a list of them by the cursor.
		TArray<int32> Stack;
		for (int32 Index = 0; Index < PaintedPositions.Num() && Index < Objects.Num(); ++Index)
		{
			const bool bFocusDisc = Objects[Index].Kind == EKind::Planet && Objects[Index].Actor.Get() == FocusPlanet.Get();
			if (!bFocusDisc && Objects[Index].Kind != EKind::Pilot && APSCivilizationMapPrivate::IsPainted(PaintedPositions[Index])
				&& FVector2D::Distance(PaintedPositions[Index], Local) <= 12.0)
			{
				Stack.Add(Index);
			}
		}
		if (Stack.Num() > 1)
		{
			const TWeakPtr<SAPSCivilizationMap> WeakMap = SharedThis(this);
			TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
			for (const int32 Index : Stack)
			{
				const FObject& Object = Objects[Index];
				const FString Id = Object.StableId;
				List->AddSlot().AutoHeight().Padding(0.0f, 1.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.03f, 0.10f, 0.13f, 1.0f))
					.ContentPadding(FMargin(14.0f, 5.0f))
					.OnClicked_Lambda([WeakMap, Id]()
					{
						if (const TSharedPtr<SAPSCivilizationMap> MapWidget = WeakMap.Pin())
						{
							MapWidget->SelectedId = Id;
							MapWidget->OnSelectionChanged.ExecuteIfBound();
						}
						FSlateApplication::Get().DismissAllMenus();
						return FReply::Handled();
					})
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(Object.Name).Font(APSChrome::Font(TEXT("Bold"), 10))
							.ColorAndOpacity(FSlateColor(Object.Color))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(Object.Detail).Font(APSChrome::Font(TEXT("Regular"), 9))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.64f, 0.75f, 0.80f, 1.0f)))
						]
					]
				];
			}
			FSlateApplication::Get().PushMenu(SharedThis(this), FWidgetPath(),
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.016f, 0.026f, 0.96f)).Padding(6.0f)
				[
					SNew(SBox).MinDesiredWidth(220.0f)
					[
						List
					]
				],
				MouseEvent.GetScreenSpacePosition() + FVector2D(14.0, -10.0),
				FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		}
		else
		{
			const int32 Index = HitTest(Local);
			SelectedId = Objects.IsValidIndex(Index) ? Objects[Index].StableId : FString();
			OnSelectionChanged.ExecuteIfBound();
		}
	}
	bPanning = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAPSCivilizationMap::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Size = MyGeometry.GetLocalSize();
	const FVector2D ScreenCentre = Size * 0.5;
	// Zoom about the selected object when one is drawn, else about the cursor.
	FVector2D Anchor = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	bool bSelectedAnchor = false;
	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		if (!SelectedId.IsEmpty() && Objects[Index].StableId == SelectedId && PaintedPositions.IsValidIndex(Index)
			&& PaintedPositions[Index].X > -1000.0 && PaintedPositions[Index].X < Size.X + 1000.0)
		{
			Anchor = PaintedPositions[Index];
			bSelectedAnchor = true;
			break;
		}
	}
	const double Previous = MapZoom;
	MapZoom = FMath::Clamp(MapZoom * FMath::Pow(1.25, MouseEvent.GetWheelDelta()), 1.0, 80.0);
	if (MapZoom <= 1.0001)
	{
		MapOffset = FVector2D::ZeroVector;
		return FReply::Handled();
	}
	const FVector2D CentreNow = ScreenCentre + MapOffset;
	FVector2D CentreNew = Anchor - (Anchor - CentreNow) * (MapZoom / Previous);
	if (bSelectedAnchor && MapZoom > Previous)
	{
		// Zooming in brings the selected object toward the middle of the view.
		CentreNew += (ScreenCentre - Anchor) * 0.35;
	}
	MapOffset = CentreNew - ScreenCentre;
	return FReply::Handled();
}

FReply SAPSCivilizationMap::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const int32 Index = HitTest(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
	if (Objects.IsValidIndex(Index) && Objects[Index].Kind == EKind::Planet && !FocusPlanet.IsValid())
	{
		Focus(Objects[Index].Actor.Get());
		SelectedId = Objects[Index].StableId;
		OnSelectionChanged.ExecuteIfBound();
	}
	return FReply::Handled();
}

void SAPSCivilizationMap::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseLeave(MouseEvent);
	HoverIndex = INDEX_NONE;
}

FCursorReply SAPSCivilizationMap::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return HoverIndex != INDEX_NONE ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
