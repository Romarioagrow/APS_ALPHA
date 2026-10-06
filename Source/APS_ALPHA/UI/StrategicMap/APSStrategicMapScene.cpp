#include "APSStrategicMapScene.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Megastructures/APSMegastructures.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "APSStrategicMapScene"

namespace APSStrategicMapSceneLocal
{
	using APSStrategicMap::FObject;

	/** Catalogue systems drawn around the view focus besides every known one (Rio's brief: the nearest ~60). */
	constexpr int32 NearbySystems = 60;
	/** A mesh this far from its actor is a stray component, not the structure (the HQ Alpha asset, 02.10). */
	constexpr double StrayMeshCm = 10000000.0;
	constexpr double MaximumStructureRadiusCm = 5000000.0;

	/** "ROCKY", "FROZEN", "GAS GIANT": the enum's " Planet" word repeats the kind (as the menu's plates). */
	FString TypeName(const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return Name;
	}

	FText StarType(const AStar* Star)
	{
		const FString Class = UEnum::GetDisplayValueAsText(Star->StellarClass).ToString().ToUpper();
		return FText::FromString(Star->FullSpectralName.IsNone() ? Class
			: FString::Printf(TEXT("%s  /  %s"), *Star->FullSpectralName.ToString().ToUpper(), *Class));
	}

	FText BodyType(const APlanetaryBody* Body)
	{
		FString Type = TypeName(Body->PlanetType);
		if (const AMoon* Moon = Cast<AMoon>(Body); Moon && Body->PlanetType == EPlanetType::Unknown)
		{
			Type = UEnum::GetDisplayValueAsText(Moon->MoonType).ToString().ToUpper();
		}
		return FText::FromString(FString::Printf(TEXT("%s  /  %s KM"), *Type,
			*APSUINumber::Number(FMath::RoundToInt64(Body->GetWorldScapeBodyRadiusCm() / 100000.0)).ToString()));
	}

	/** The generation menu's marker colours by surface type. */
	FLinearColor TypeColour(const EPlanetType Type, const FLinearColor& Fallback)
	{
		switch (Type)
		{
		case EPlanetType::Melted:
		case EPlanetType::Volcanic:
		case EPlanetType::Lava:
		case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.25f, 0.08f, 1.0f);
		case EPlanetType::GasGiant:
			return FLinearColor(1.0f, 0.66f, 0.18f, 1.0f);
		case EPlanetType::IceGiant:
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
			return FLinearColor(0.42f, 0.82f, 1.0f, 1.0f);
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return FLinearColor(0.12f, 0.56f, 1.0f, 1.0f);
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Oasis:
			return FLinearColor(0.20f, 0.92f, 0.55f, 1.0f);
		case EPlanetType::Desert:
		case EPlanetType::Sand:
			return FLinearColor(0.96f, 0.72f, 0.28f, 1.0f);
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
		case EPlanetType::Carbon:
			return FLinearColor(0.74f, 0.64f, 0.92f, 1.0f);
		default:
			return Fallback;
		}
	}

	FLinearColor StarColour(const AStar* Star)
	{
		if (Star->SurfaceTemperature <= 0)
		{
			return APSChrome::Amber();
		}
		// The star's own light, kept bright enough for a marker on dark space.
		const FLinearColor Light = FLinearColor::MakeFromColorTemperature(
			FMath::Clamp(static_cast<float>(Star->SurfaceTemperature), 2000.0f, 15000.0f));
		const float Peak = FMath::Max3(Light.R, Light.G, Light.B);
		return Peak > 0.0f ? FLinearColor(Light.R / Peak, Light.G / Peak, Light.B / Peak, 1.0f) : APSChrome::Amber();
	}

	FText DesignationOf(const AActor* Actor)
	{
		const FString Designation = APSBodyDesignation::Of(Actor);
		return Designation.IsEmpty() ? FText::GetEmpty() : FText::FromString(Designation);
	}

	/** A structure's or hull's size from its meshes: gravity spheres and other shapes are not its size. */
	double MeshRadius(const AActor* Actor)
	{
		FBox Box(ForceInit);
		const FVector Centre = Actor->GetActorLocation();
		Actor->ForEachComponent<UMeshComponent>(false, [&Box, &Centre](const UMeshComponent* Component)
		{
			if (Component && Component->IsRegistered() && Component->IsVisible()
				&& FVector::DistSquared(Component->Bounds.Origin, Centre) < FMath::Square(StrayMeshCm))
			{
				Box += Component->Bounds.GetBox();
			}
		});
		return Box.IsValid ? FMath::Min(Box.GetExtent().Size(), MaximumStructureRadiusCm) : 0.0;
	}

	/** The orbit through the body round its centre, in the plane of its orbit actor (as the menu draws them). */
	void SetOrbit(FObject& Object, const AActor* Body, AActor* Centre, const FVector& FallbackNormal)
	{
		if (!IsValid(Centre))
		{
			return;
		}
		const FVector Relative = Body->GetActorLocation() - Centre->GetActorLocation();
		const double Radius = Relative.Size();
		if (!FMath::IsFinite(Radius) || Radius < 1000.0)
		{
			return;
		}
		const FVector Direction = Relative / Radius;
		FVector Normal = FallbackNormal;
		if (const APlanetOrbit* Orbit = Cast<APlanetOrbit>(Body->GetAttachParentActor()))
		{
			Normal = Orbit->GetActorQuat().GetAxisZ();
		}
		// The body lies on its orbit: the plane must hold its direction.
		Normal = (Normal - Direction * FVector::DotProduct(Normal, Direction)).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = FVector::CrossProduct(Direction, FMath::Abs(Direction.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector)
				.GetSafeNormal();
		}
		Object.OrbitCentre = Centre;
		Object.OrbitAxisX = Direction;
		Object.OrbitAxisY = FVector::CrossProduct(Normal, Direction).GetSafeNormal();
		Object.OrbitRadiusCm = Radius;
	}
}

FSlateFontInfo APSStrategicMap::PlateTypeFont()
{
	return APSUITheme::BodyFont("Bold", 9);
}

FSlateFontInfo APSStrategicMap::PlateNameFont()
{
	return APSUITheme::BodyFont("Bold", 11);
}

FSlateFontInfo APSStrategicMap::PlateDesignationFont()
{
	return APSChrome::Font("Bold", 10);
}

APSStrategicMap::FPlate APSStrategicMap::LayoutPlate(const FText& Type, const FText& Name, const FText& Designation)
{
	FPlate Plate;
	if (!FSlateApplication::IsInitialized())
	{
		Plate.Size = FVector2D(160.0, 36.0);
		return Plate;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo TypeFont = PlateTypeFont();
	const FSlateFontInfo NameFont = PlateNameFont();
	const FSlateFontInfo DesignationFont = PlateDesignationFont();
	// The plates are capitals: the visual height of a line is its cap height (about 0.71 em), not the line box.
	const auto CapHeight = [](const FSlateFontInfo& Info) { return Info.Size * (96.0f / 72.0f) * 0.71f; };
	const auto TopToBaseline = [&Measure](const FSlateFontInfo& Info)
	{
		return static_cast<float>(Measure->GetMaxCharacterHeight(Info) + Measure->GetBaseline(Info));
	};
	Plate.NameWidth = static_cast<float>(Measure->Measure(Name, NameFont).X);
	const float DesignationWidth = Designation.IsEmpty() ? 0.0f
		: PlateDesignationGap + static_cast<float>(Measure->Measure(Designation, DesignationFont).X);
	const float TypeWidth = Type.IsEmpty() ? 0.0f : static_cast<float>(Measure->Measure(Type, TypeFont).X);
	float Baseline = PlatePadY;
	if (!Type.IsEmpty())
	{
		Baseline += CapHeight(TypeFont);
		Plate.TypeTop = Baseline - TopToBaseline(TypeFont);
		Baseline += PlateLineGap;
	}
	Baseline += CapHeight(NameFont);
	Plate.NameTop = Baseline - TopToBaseline(NameFont);
	Plate.DesignationTop = Baseline - TopToBaseline(DesignationFont);
	Plate.Size = FVector2D(PlateBar + PlatePadX + FMath::Max(TypeWidth, Plate.NameWidth + DesignationWidth) + PlatePadX,
		Baseline + PlatePadY);
	return Plate;
}

FAPSStrategicMapScene::FAPSStrategicMapScene(UWorld* InWorld, AAstroGenerator* InGenerator)
	: World(InWorld)
	, Generator(InGenerator)
{
}

void FAPSStrategicMapScene::Update(const float DeltaSeconds, const FVector& ViewFocus, const double ViewDistance)
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	ObjectsClock -= DeltaSeconds;
	SystemsClock -= DeltaSeconds;
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const uint32 FleetRevision = Fleet ? Fleet->GetRevision() : 0;
	const uint32 InfrastructureRevision = Infrastructure ? Infrastructure->GetRevision() : 0;
	const uint32 StarsRevision = Stars ? Stars->GetRevision() : 0;
	if (FleetRevision != SeenFleetRevision || InfrastructureRevision != SeenInfrastructureRevision)
	{
		SeenFleetRevision = FleetRevision;
		SeenInfrastructureRevision = InfrastructureRevision;
		bObjectsDirty = true;
		// Relays change the network's reach.
		bSystemsDirty = true;
	}
	if (StarsRevision != SeenStarsRevision)
	{
		// Rio 04.10 (hitches with the map open in flight): the galaxy's systems register around a flying pilot every half
		// second and each re-pick of the catalogue cost ~50 ms; that news waits up to two seconds (orders above do not).
		SeenStarsRevision = StarsRevision;
		bStarsNews = true;
	}
	if (bObjectsDirty || ObjectsClock <= 0.0f)
	{
		RefreshObjects();
		bObjectsDirty = false;
		// Rio 03.10 (FPS on the map): a full re-read walks the actors, meshes and names (1-3 ms). Launches, builds and orders
		// bump a revision and re-read at once; positions are read live by the view, so the timed pass can be rare.
		ObjectsClock = 2.0f;
		++ObjectsSerial;
	}
	// The catalogue systems follow what is looked at: re-picked when the focus moved a fifth of the view.
	const bool bFocusMoved = FVector::Dist(ViewFocus, SystemsFocus) > FMath::Max(ViewDistance * 0.2, 1.0e9);
	if (bSystemsDirty || (bStarsNews && SystemsClock <= -1.6f) || (bFocusMoved && SystemsClock <= 0.0f) || SystemsClock <= -2.0f)
	{
		RefreshSystems(ViewFocus);
		bSystemsDirty = false;
		bStarsNews = false;
		SystemsClock = 0.4f;
	}
}

APSStrategicMap::FPlate FAPSStrategicMapScene::CachedPlate(const FText& Type, const FText& Name, const FText& Designation)
{
	const FString Key = Type.ToString() + TEXT("\n") + Name.ToString() + TEXT("\n") + Designation.ToString();
	if (const APSStrategicMap::FPlate* Found = PlateCache.Find(Key))
	{
		return *Found;
	}
	if (PlateCache.Num() > 4096)
	{
		PlateCache.Reset();
	}
	return PlateCache.Add(Key, APSStrategicMap::LayoutPlate(Type, Name, Designation));
}

void FAPSStrategicMapScene::RefreshObjects()
{
	using APSStrategicMap::EKind;
	using APSStrategicMap::FObject;
	using APSStrategicMapSceneLocal::BodyType;
	using APSStrategicMapSceneLocal::DesignationOf;
	using APSStrategicMapSceneLocal::MeshRadius;
	using APSStrategicMapSceneLocal::SetOrbit;
	using APSStrategicMapSceneLocal::StarColour;
	using APSStrategicMapSceneLocal::StarType;
	using APSStrategicMapSceneLocal::TypeColour;
	Objects.Reset();
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	const FVector FrameUp = GetFrameUp();
	APawn* Pilot = GetPilot();
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld);
	TSet<const AActor*> Seen;
	const auto Add = [this, &Seen](AActor* Actor, const EKind Kind, const uint8 Priority) -> FObject*
	{
		if (!IsValid(Actor) || Seen.Contains(Actor))
		{
			return nullptr;
		}
		Seen.Add(Actor);
		FObject& Object = Objects.AddDefaulted_GetRef();
		Object.Actor = Actor;
		Object.Kind = Kind;
		Object.Priority = Priority;
		Object.Name = APSObjectActions::NameOf(Actor);
		return &Object;
	};
	// Built infrastructure takes its department's colour and its catalogue kind.
	const auto Built = [Infrastructure](const AActor* Actor) -> const APSInfrastructure::FType*
	{
		const FAPSBuiltStructure* Structure = Infrastructure ? Infrastructure->FindByActor(Actor) : nullptr;
		return Structure ? APSInfrastructure::Find(Structure->Type) : nullptr;
	};

	for (TActorIterator<AStar> It(LiveWorld); It; ++It)
	{
		AStar* Star = *It;
		if (FObject* Object = Add(Star, EKind::Star, 1))
		{
			Object->Type = StarType(Star);
			Object->Designation = DesignationOf(Star);
			Object->Colour = StarColour(Star);
			Object->RadiusCm = Star->StarRadiusKM > 0 ? Star->StarRadiusKM * 100000.0 : MeshRadius(Star);
		}
	}
	for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
	{
		APlanet* Planet = *It;
		if (FObject* Object = Add(Planet, EKind::Planet, 2))
		{
			Object->Type = BodyType(Planet);
			Object->Designation = DesignationOf(Planet);
			Object->Colour = TypeColour(Planet->PlanetType, APSChrome::Cyan());
			Object->RadiusCm = Planet->GetWorldScapeBodyRadiusCm();
			SetOrbit(*Object, Planet, Planet->ParentStar, FrameUp);
		}
	}
	for (TActorIterator<AMoon> It(LiveWorld); It; ++It)
	{
		AMoon* Moon = *It;
		if (FObject* Object = Add(Moon, EKind::Moon, 4))
		{
			Object->Type = BodyType(Moon);
			Object->Designation = DesignationOf(Moon);
			Object->Colour = TypeColour(Moon->PlanetType, FLinearColor(0.56f, 0.76f, 1.0f, 1.0f));
			Object->RadiusCm = Moon->GetWorldScapeBodyRadiusCm();
			SetOrbit(*Object, Moon, Moon->ParentPlanet, FrameUp);
		}
	}
	for (TActorIterator<ASpaceStation> It(LiveWorld); It; ++It)
	{
		ASpaceStation* Station = *It;
		if (FObject* Object = Add(Station, EKind::Station, 3))
		{
			const APSInfrastructure::FType* Type = Built(Station);
			Object->Type = APSObjectActions::KindOf(Station);
			Object->Colour = Type ? APSInfrastructure::DepartmentColour(Type->Department)
				: Station->IsA<ASpaceHeadquarters>() ? APSChrome::Amber() : APSChrome::Cyan();
			Object->RadiusCm = MeshRadius(Station);
			Object->bOwn = true;
		}
	}
	for (TActorIterator<AAutonomousOutpost> It(LiveWorld); It; ++It)
	{
		AAutonomousOutpost* Outpost = *It;
		if (FObject* Object = Add(Outpost, EKind::Outpost, 5))
		{
			const APSInfrastructure::FType* Type = Built(Outpost);
			Object->Type = APSObjectActions::KindOf(Outpost);
			Object->Colour = Type ? APSInfrastructure::DepartmentColour(Type->Department) : APSChrome::Amber();
			Object->RadiusCm = MeshRadius(Outpost);
			Object->bOwn = true;
		}
	}
	for (TActorIterator<AColony> It(LiveWorld); It; ++It)
	{
		AColony* Colony = *It;
		const UAPSCivilizationIdentityComponent* Identity = IsValid(Colony)
			? Colony->FindComponentByClass<UAPSCivilizationIdentityComponent>() : nullptr;
		const bool bHomeColony = IsValid(Colony) && Colony->ActorHasTag(TEXT("APS.Civilization.Materialized")) && Identity
			&& Identity->Role == EAPSCivilizationEntityRole::BaseModule;
		if (FObject* Object = Add(Colony, EKind::Colony, bHomeColony ? 2 : 4))
		{
			Object->Type = bHomeColony ? LOCTEXT("HomeColonyType", "SETTLEMENT  /  HOME COLONY") : APSObjectActions::KindOf(Colony);
			Object->Colour = bHomeColony ? APSChrome::Success() : APSChrome::Muted();
			Object->RadiusCm = MeshRadius(Colony);
			Object->bOwn = bHomeColony;
		}
	}
	// Built structures of other kinds (beacons and relays at system anchors keep the generator's families).
	if (Infrastructure)
	{
		for (const FAPSBuiltStructure& Structure : Infrastructure->GetStructures())
		{
			AActor* Actor = Structure.Actor.Get();
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure.Type);
			// Rio 03.10: the huge hubs are stations (of a higher tier) on the map.
			const bool bHub = Type && Type->Category == APSInfrastructure::ECategory::Hub;
			if (FObject* Object = Add(Actor, bHub ? EKind::Station : EKind::Outpost, bHub ? 3 : 5))
			{
				Object->Type = APSObjectActions::KindOf(Actor);
				Object->Colour = Type ? APSInfrastructure::DepartmentColour(Type->Department) : APSChrome::Amber();
				Object->RadiusCm = MeshRadius(Actor);
				Object->bOwn = true;
				// The orbital ring: its circle round its world on the orbits layer, through its marker where the elevator's
				// tether crosses it.
				AActor* Body = Actor->GetAttachParentActor();
				APSMegastructures::FWorldLayout Layout;
				if (Type && Type->Visual == APSInfrastructure::EVisual::OrbitalRing && Body && APSMegastructures::LayoutAt(Body, Layout))
				{
					const FVector North = Body->GetActorUpVector();
					Object->OrbitCentre = Body;
					Object->OrbitAxisX = APSMegastructures::EquatorDirection(North, Actor->GetActorLocation() - Body->GetActorLocation(), 0.0,
						Body->GetActorForwardVector());
					Object->OrbitAxisY = FVector::CrossProduct(North, Object->OrbitAxisX).GetSafeNormal();
					Object->OrbitRadiusCm = Layout.RingRadiusCm;
				}
			}
		}
	}
	if (Fleet)
	{
		// Located anomaly sites: their navigation beacons.
		for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
		{
			AActor* Beacon = Record.AnomalyBeacon.Get();
			if (FObject* Object = Add(Beacon, EKind::Anomaly, 5))
			{
				Object->Name = APSFleet::AnomalyName(Record.AnomalyKind);
				Object->Type = Record.Anomaly == APSFleet::EAnomalyState::Investigated
					? FText::Format(LOCTEXT("AnomalyDone", "ANOMALY  /  INVESTIGATED  /  {0}"),
						APSObjectActions::NameOf(Record.Body.Get()))
					: FText::Format(LOCTEXT("AnomalySite", "ANOMALY SITE  /  {0}"), APSObjectActions::NameOf(Record.Body.Get()));
				Object->Colour = FLinearColor(1.0f, 0.62f, 0.2f, 1.0f);
				Object->RadiusCm = 2000.0;
				Object->bOwn = true;
			}
		}
		// The ancient sites (Gameplay/Ancients): km-scale ruins of an unknown race, marked like anomalies in pale cyan.
		for (TActorIterator<AActor> It(LiveWorld); It; ++It)
		{
			if (!IsValid(*It) || !It->ActorHasTag(TEXT("APS.Ancient.Site")))
			{
				continue;
			}
			if (FObject* Object = Add(*It, EKind::Anomaly, 4))
			{
				Object->Type = LOCTEXT("AncientSiteType", "ANCIENT SITE");
				Object->Colour = FLinearColor(0.25f, 0.9f, 1.0f, 1.0f);
				Object->RadiusCm = 200000.0;
			}
		}
		// The fleet in its divisions' colours, named by call sign.
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			ASpaceship* Ship = Unit.Ship.Get();
			const bool bPiloted = Ship && Ship == Pilot;
			if (FObject* Object = Add(Ship, EKind::Ship, bPiloted ? 0 : Unit.bFlagship ? 5 : 6))
			{
				Object->Type = bPiloted
					? FText::Format(LOCTEXT("YourUnitType", "YOUR SHIP  /  {0}  /  CLASS {1}"), APSFleet::DivisionName(Unit.Division),
						FText::FromString(Ship->GetSizeClassName()))
					: FText::Format(LOCTEXT("UnitType", "SHIP  /  {0}  /  CLASS {1}"), APSFleet::DivisionName(Unit.Division),
						FText::FromString(Ship->GetSizeClassName()));
				Object->Colour = bPiloted ? APSChrome::White() : APSFleet::DivisionColour(Unit.Division);
				Object->RadiusCm = MeshRadius(Ship);
				Object->bOwn = true;
				Object->bPlayer = bPiloted;
			}
		}
	}
	if (ASpaceship* PilotedShip = Cast<ASpaceship>(Pilot))
	{
		if (FObject* Object = Add(PilotedShip, EKind::Ship, 0))
		{
			Object->Type = FText::Format(LOCTEXT("YourShipType", "YOUR SHIP  /  CLASS {0}"),
				FText::FromString(PilotedShip->GetSizeClassName()));
			Object->Colour = APSChrome::White();
			Object->RadiusCm = MeshRadius(PilotedShip);
			Object->bOwn = true;
			Object->bPlayer = true;
		}
	}
	else if (FObject* Object = Add(Pilot, EKind::Pilot, 0))
	{
		Object->Name = LOCTEXT("You", "YOU");
		Object->Type = LOCTEXT("PilotType", "PILOT");
		Object->Colour = APSChrome::White();
		Object->RadiusCm = 100.0;
		Object->bOwn = true;
		Object->bPlayer = true;
	}

	Objects.StableSort([](const FObject& A, const FObject& B) { return A.Priority < B.Priority; });
	for (FObject& Object : Objects)
	{
		Object.Plate = CachedPlate(Object.Type, Object.Name, Object.Designation);
	}
}

void FAPSStrategicMapScene::RefreshSystems(const FVector& Focus)
{
	using APSStrategicMap::FSystemMark;
	Systems.Reset();
	Links.Reset();
	SystemsFocus = Focus;
	FAPSStarSystems* Stars = GetStars();
	if (!Stars || !Stars->IsReady())
	{
		return;
	}
	// Home, then what the civilization holds and knows, then the neighbours of the view.
	TArray<int32> Wanted;
	if (Stars->GetHomeIndex() != INDEX_NONE)
	{
		Wanted.Add(Stars->GetHomeIndex());
	}
	if (Selection.IsSystem())
	{
		Wanted.AddUnique(Selection.SystemIndex);
	}
	TArray<int32> Known;
	Stars->GetKnown(Known);
	for (const int32 Index : Known)
	{
		Wanted.AddUnique(Index);
	}
	TArray<int32> Nearby;
	Stars->FindNearest(Focus, APSStrategicMapSceneLocal::NearbySystems, Nearby);
	for (const int32 Index : Nearby)
	{
		Wanted.AddUnique(Index);
	}
	for (const int32 Index : Wanted)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		if (!Info)
		{
			continue;
		}
		const FAPSStarSystemState State = Stars->GetState(Info->Id);
		FSystemMark& Mark = Systems.AddDefaulted_GetRef();
		Mark.Index = Index;
		Mark.Name = FText::FromString(Info->Name);
		Mark.Knowledge = State.Knowledge;
		Mark.bClaimed = State.bClaimed;
		Mark.bHome = Info->bHome;
		Mark.StarCount = Info->StarCount;
		Mark.RoomCm = Info->RoomCm;
		Mark.ReachCm = State.bClaimed ? Stars->ReachCm(Index) : 0.0;
		const FText Spectral = FText::FromString(Info->Spectral.IsEmpty() ? TEXT("STAR") : Info->Spectral.ToUpper());
		if (Info->bHome)
		{
			Mark.Type = FText::Format(LOCTEXT("HomeSystemType", "HOME SYSTEM  /  {0}"), Spectral);
			Mark.Colour = APSChrome::Amber();
		}
		else if (State.bClaimed)
		{
			// The marker keeps its knowledge colour; the claim is the lit beacon round it (the view).
			Mark.Type = FText::Format(LOCTEXT("ClaimedSystemType", "{0}  /  CLAIMED"), Spectral);
			Mark.Colour = APSStars::KnowledgeColour(State.Knowledge);
		}
		else
		{
			Mark.Type = Info->StarCount > 1
				? FText::Format(LOCTEXT("MultipleSystemType", "{0}  /  {1}  /  {2} STARS"), Spectral,
					APSStars::KnowledgeName(State.Knowledge), APSUINumber::Number(Info->StarCount))
				: FText::Format(LOCTEXT("SystemType", "{0}  /  {1}"), Spectral, APSStars::KnowledgeName(State.Knowledge));
			Mark.Colour = APSStars::KnowledgeColour(State.Knowledge);
		}
		// A deep-space anomaly the civilization detected or located there (FAPSStarSystems): its own marker and line.
		Mark.Anomaly = State.Anomaly;
		if (State.Anomaly == 1 || State.Anomaly == 2)
		{
			Mark.Type = FText::Format(LOCTEXT("SystemAnomalyType", "{0}  /  {1}"), Mark.Type, State.Anomaly == 2
				? FAPSStarSystems::AnomalyName(Stars->AnomalyKindOf(Info->Id)) : LOCTEXT("AnomalyDetected", "ANOMALY DETECTED"));
		}
		Mark.Plate = CachedPlate(Mark.Type, Mark.Name, FText::GetEmpty());
	}
	Stars->GetNetwork(Links);
}

int32 FAPSStrategicMapScene::FindObject(const AActor* Actor) const
{
	return Actor ? Objects.IndexOfByPredicate([Actor](const APSStrategicMap::FObject& Object)
	{
		return Object.Actor.Get() == Actor;
	}) : INDEX_NONE;
}

int32 FAPSStrategicMapScene::FindSystem(const int32 CatalogueIndex) const
{
	return Systems.IndexOfByPredicate([CatalogueIndex](const APSStrategicMap::FSystemMark& Mark)
	{
		return Mark.Index == CatalogueIndex;
	});
}

void FAPSStrategicMapScene::GetObstacles(TArray<TPair<TWeakObjectPtr<AActor>, double>>& OutObstacles) const
{
	OutObstacles.Reset();
	for (const APSStrategicMap::FObject& Object : Objects)
	{
		if (Object.RadiusCm <= 0.0)
		{
			continue;
		}
		if (Object.Kind == APSStrategicMap::EKind::Star)
		{
			OutObstacles.Emplace(Object.Actor, Object.RadiusCm * 1.03);
		}
		else if (Object.Kind == APSStrategicMap::EKind::Planet || Object.Kind == APSStrategicMap::EKind::Moon)
		{
			// Just under sea level: the map may come down to a colony or a landed ship, never inside the world.
			OutObstacles.Emplace(Object.Actor, Object.RadiusCm * 0.999);
		}
	}
}

bool FAPSStrategicMapScene::SurfaceUp(const APSStrategicMap::FSelection& Target, FVector& OutUp,
	double& OutBodyRadiusCm) const
{
	FVector Location;
	if (Target.IsSystem() || !Locate(Target, Location))
	{
		return false;
	}
	const AActor* Self = Target.Actor.Get();
	double BestHeight = TNumericLimits<double>::Max();
	for (const APSStrategicMap::FObject& Object : Objects)
	{
		const AActor* Body = Object.Actor.Get();
		if (!Body || Body == Self || Object.RadiusCm <= 0.0
			|| (Object.Kind != APSStrategicMap::EKind::Planet && Object.Kind != APSStrategicMap::EKind::Moon))
		{
			continue;
		}
		const FVector Offset = Location - Body->GetActorLocation();
		const double Height = Offset.Size() - Object.RadiusCm;
		// On the ground or just over it (stations orbit a quarter of a radius up and more).
		if (Height < FMath::Max(Object.RadiusCm * 0.03, 50000.0) && Height < BestHeight)
		{
			BestHeight = Height;
			OutUp = Offset.GetSafeNormal();
			OutBodyRadiusCm = Object.RadiusCm;
		}
	}
	return BestHeight < TNumericLimits<double>::Max() && !OutUp.IsNearlyZero();
}

void FAPSStrategicMapScene::Select(const APSStrategicMap::FSelection& NewSelection)
{
	APSStrategicMap::FSelection Normalised = NewSelection;
	FGuid SystemId;
	if (const AActor* Actor = Normalised.Actor.Get(); Actor && FAPSStarSystems::AnchorSystem(Actor, SystemId))
	{
		// A system's anchor stands for the system: the list, the sphere and the object page all treat it as one.
		if (const FAPSStarSystems* Stars = GetStars(); Stars && Stars->IndexOf(SystemId) != INDEX_NONE)
		{
			Normalised = APSStrategicMap::FSelection::OfSystem(Stars->IndexOf(SystemId));
		}
	}
	if (Normalised == Selection)
	{
		return;
	}
	Selection = Normalised;
	++SelectionSerial;
	// The selected system is always drawn, whatever the view's neighbourhood.
	bSystemsDirty = true;
}

void FAPSStrategicMapScene::SetLayer(const APSStrategicMap::ELayer Layer, const bool bOn)
{
	const uint32 Bit = 1u << static_cast<uint32>(Layer);
	LayerMask = bOn ? LayerMask | Bit : LayerMask & ~Bit;
}

AActor* FAPSStrategicMapScene::ResolveActor(const APSStrategicMap::FSelection& Target) const
{
	if (AActor* Actor = Target.Actor.Get())
	{
		return Actor;
	}
	FAPSStarSystems* Stars = GetStars();
	const FAPSStarSystemInfo* Info = Stars && Target.IsSystem() ? Stars->Get(Target.SystemIndex) : nullptr;
	return Info ? Stars->GetAnchor(Info->Id) : nullptr;
}

bool FAPSStrategicMapScene::LocateSystem(const int32 CatalogueIndex, FVector& OutLocation) const
{
	const FAPSStarSystems* Stars = GetStars();
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(CatalogueIndex) : nullptr;
	if (!Info)
	{
		return false;
	}
	OutLocation = Info->Location;
	return true;
}

bool FAPSStrategicMapScene::Locate(const APSStrategicMap::FSelection& Target, FVector& OutLocation) const
{
	if (const AActor* Actor = Target.Actor.Get())
	{
		OutLocation = Actor->GetActorLocation();
		return true;
	}
	return Target.IsSystem() && LocateSystem(Target.SystemIndex, OutLocation);
}

double FAPSStrategicMapScene::FrameRadius(const APSStrategicMap::FSelection& Target) const
{
	using APSStrategicMap::EKind;
	using APSStrategicMap::FObject;
	if (Target.IsSystem())
	{
		const FAPSStarSystems* Stars = GetStars();
		const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Target.SystemIndex) : nullptr;
		return Info ? (Info->bHome ? GetHomeRoomCm() : FMath::Max(Info->RoomCm, APSStars::AstronomicalUnitCm * 0.05))
			: APSStars::AstronomicalUnitCm;
	}
	const int32 Index = FindObject(Target.Actor.Get());
	if (!Objects.IsValidIndex(Index))
	{
		return 1000000.0;
	}
	const FObject& Object = Objects[Index];
	switch (Object.Kind)
	{
	case EKind::Star:
		return Object.RadiusCm * 5.0;
	case EKind::Planet:
		// The planet with its near orbits: stations and ships at their slots (1.25 radii and more).
		return Object.RadiusCm * 2.6;
	case EKind::Moon:
		return Object.RadiusCm * 2.2;
	case EKind::Anomaly:
		return 3000000.0;
	case EKind::Pilot:
		return 20000.0;
	default:
		return FMath::Max(Object.RadiusCm * 3.0, 15000.0);
	}
}

double FAPSStrategicMapScene::FocusRadius(const APSStrategicMap::FSelection& Target) const
{
	if (Target.IsSystem())
	{
		return FrameRadius(Target) * 0.01;
	}
	const int32 Index = FindObject(Target.Actor.Get());
	return Objects.IsValidIndex(Index) ? Objects[Index].RadiusCm : 0.0;
}

FText FAPSStrategicMapScene::NameOf(const APSStrategicMap::FSelection& Target) const
{
	if (const AActor* Actor = Target.Actor.Get())
	{
		const int32 Index = FindObject(Actor);
		return Objects.IsValidIndex(Index) ? Objects[Index].Name : APSObjectActions::NameOf(Actor);
	}
	const FAPSStarSystems* Stars = GetStars();
	const FAPSStarSystemInfo* Info = Stars && Target.IsSystem() ? Stars->Get(Target.SystemIndex) : nullptr;
	return Info ? FText::FromString(Info->Name) : FText::GetEmpty();
}

FAPSStarSystems* FAPSStrategicMapScene::GetStars() const
{
	return APSStarSystemsFind(World.Get());
}

AActor* FAPSStrategicMapScene::GetHomeStar() const
{
	if (const AAstroGenerator* Astro = Generator.Get(); Astro && IsValid(Astro->HomeStar))
	{
		return Astro->HomeStar;
	}
	if (const APlanet* Planet = GetHomePlanet(); Planet && IsValid(Planet->ParentStar))
	{
		return Planet->ParentStar;
	}
	if (UWorld* LiveWorld = World.Get())
	{
		for (TActorIterator<AStar> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

AActor* FAPSStrategicMapScene::GetReference() const
{
	if (AActor* Star = GetHomeStar())
	{
		return Star;
	}
	const AAstroGenerator* Astro = Generator.Get();
	return Astro ? Astro->GetPreviewHomeSystem() : nullptr;
}

APlanet* FAPSStrategicMapScene::GetHomePlanet() const
{
	if (const AAstroGenerator* Astro = Generator.Get(); Astro && IsValid(Astro->HomePlanet))
	{
		return Astro->HomePlanet;
	}
	// Without a generator: the planet nearest the pilot.
	UWorld* LiveWorld = World.Get();
	const APawn* Pilot = GetPilot();
	APlanet* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	if (LiveWorld)
	{
		for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
		{
			const double Distance = Pilot ? FVector::DistSquared(Pilot->GetActorLocation(), It->GetActorLocation()) : 0.0;
			if (IsValid(*It) && Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
	}
	return Best;
}

APawn* FAPSStrategicMapScene::GetPilot() const
{
	const UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

ASpaceship* FAPSStrategicMapScene::GetMyShip() const
{
	if (ASpaceship* Piloted = Cast<ASpaceship>(GetPilot()))
	{
		return Piloted;
	}
	if (const FAPSFleetCommand* Fleet = APSFleetFind(World.Get()))
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (Unit.bFlagship && Unit.Ship.IsValid())
			{
				return Unit.Ship.Get();
			}
		}
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (Unit.Division == APSFleet::EDivision::MainFleet && Unit.Ship.IsValid())
			{
				return Unit.Ship.Get();
			}
		}
	}
	return nullptr;
}

FVector FAPSStrategicMapScene::GetFrameUp() const
{
	// The plane of the home planets' orbits, so the system lies flat on the screen.
	const auto OrbitNormal = [](const APlanet* Planet, FVector& OutNormal)
	{
		const APlanetOrbit* Orbit = IsValid(Planet) ? Cast<APlanetOrbit>(Planet->GetAttachParentActor()) : nullptr;
		if (!Orbit)
		{
			return false;
		}
		OutNormal = Orbit->GetActorQuat().GetAxisZ();
		return !OutNormal.IsNearlyZero();
	};
	FVector Normal = FVector::UpVector;
	if (OrbitNormal(GetHomePlanet(), Normal))
	{
		return Normal;
	}
	if (UWorld* LiveWorld = World.Get())
	{
		for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
		{
			if (OrbitNormal(*It, Normal))
			{
				return Normal;
			}
		}
	}
	return FVector::UpVector;
}

double FAPSStrategicMapScene::GetHomeRoomCm() const
{
	if (const FAPSStarSystems* Stars = GetStars())
	{
		if (const FAPSStarSystemInfo* Home = Stars->GetHome(); Home && Home->RoomCm > 0.0)
		{
			return Home->RoomCm;
		}
	}
	// Before the catalogue is read: the outermost planet's orbit round the home star.
	const AActor* Star = GetHomeStar();
	double Outermost = 0.0;
	if (UWorld* LiveWorld = World.Get(); LiveWorld && Star)
	{
		for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It) && It->ParentStar == Star)
			{
				Outermost = FMath::Max(Outermost, FVector::Dist(It->GetActorLocation(), Star->GetActorLocation()));
			}
		}
	}
	return Outermost > 0.0 ? Outermost * 1.15 : 5.0 * APSStars::AstronomicalUnitCm;
}

double FAPSStrategicMapScene::GetClusterFrameRadius() const
{
	const FAPSStarSystems* Stars = GetStars();
	const FAPSStarSystemInfo* Home = Stars ? Stars->GetHome() : nullptr;
	if (!Home)
	{
		return 40.0 * APSStars::AstronomicalUnitCm;
	}
	// The few dozen nearest neighbours round home fill the view.
	TArray<int32> Nearest;
	Stars->FindNearest(Home->Location, 40, Nearest);
	double Farthest = 0.0;
	for (const int32 Index : Nearest)
	{
		if (const FAPSStarSystemInfo* Info = Stars->Get(Index))
		{
			Farthest = FMath::Max(Farthest, FVector::Dist(Info->Location, Home->Location));
		}
	}
	return FMath::Clamp(Farthest * 1.1, 5.0 * APSStars::AstronomicalUnitCm, 150.0 * APSStars::AstronomicalUnitCm);
}

#undef LOCTEXT_NAMESPACE
