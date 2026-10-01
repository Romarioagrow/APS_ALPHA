#include "APSFleetCommand.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipCatalog.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "APSFleetCommand"

namespace APSFleetPrivate
{
	using namespace APSFleet;

	const FName HomeShipTag(TEXT("APS.Fleet.HomeShip"));
	const FName EscortTag(TEXT("APS.Fleet.Escort"));
	const FName MaterializedTag(TEXT("APS.Civilization.Materialized"));
	const FName UnitTag(TEXT("APS.Fleet.Unit"));
	const FName ParkedTag(TEXT("APS.Civilization.SurfaceParked"));
	const FName OutpostTag(TEXT("APS.Fleet.Outpost"));

	TAutoConsoleVariable<float> CVarSpeedScale(TEXT("aps.Fleet.SpeedScale"), 1.0f,
		TEXT("Multiplies the fleet autopilot's speed (tests)."));
	TAutoConsoleVariable<float> CVarWorkScale(TEXT("aps.Fleet.WorkScale"), 1.0f,
		TEXT("Multiplies how fast surveys and outposts progress at the target (tests)."));
	TAutoConsoleVariable<int32> CVarLog(TEXT("aps.Fleet.Log"), 0,
		TEXT("1 logs every unit under orders once a second: phase, distance, speed."));
	TAutoConsoleVariable<float> CVarBuildScale(TEXT("aps.Fleet.BuildScale"), 1.0f,
		TEXT("Multiplies how fast the shipyard builds (tests)."));
	const FName BuiltTag(TEXT("APS.Fleet.Built"));

	/** Seconds on the slipway by size, before the Industry level shortens them. */
	float ClassBuildSeconds(const ESpaceshipSizeClass SizeClass)
	{
		switch (SizeClass)
		{
		case ESpaceshipSizeClass::XXS: return 20.0f;
		case ESpaceshipSizeClass::XS: return 30.0f;
		case ESpaceshipSizeClass::S: return 45.0f;
		case ESpaceshipSizeClass::M: return 60.0f;
		case ESpaceshipSizeClass::L: return 90.0f;
		case ESpaceshipSizeClass::XL: return 120.0f;
		case ESpaceshipSizeClass::XXL: return 150.0f;
		default: return 240.0f;
		}
	}

	TMap<const UWorld*, FAPSFleetCommand*>& Registry()
	{
		static TMap<const UWorld*, FAPSFleetCommand*> Fleets;
		return Fleets;
	}

	bool IsFleetShip(const ASpaceship* Ship)
	{
		return IsValid(Ship) && (Ship->ActorHasTag(HomeShipTag) || Ship->ActorHasTag(EscortTag)
			|| Ship->ActorHasTag(MaterializedTag) || Ship->ActorHasTag(UnitTag));
	}

	/** Planets and moons by their WorldScape radius, stars by their catalogue radius (cm); 0 for anything else. */
	double BodyRadiusCm(const AActor* Actor)
	{
		if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor))
		{
			return Body->GetWorldScapeBodyRadiusCm();
		}
		if (const AStar* Star = Cast<AStar>(Actor))
		{
			return Star->StarRadiusKM * 100000.0;
		}
		return 0.0;
	}

	template <typename TEnum>
	FText EnumText(const TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? FText::FromString(Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString().ToUpper())
			: FText::GetEmpty();
	}

	FText NameOf(const AActor* Actor)
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

	/** Three words for a 0..1 value, low to high. */
	FText Grade(const float Value, const FText& Low, const FText& Mid, const FText& High, const float LowBelow = 0.34f,
		const float HighFrom = 0.67f)
	{
		return Value < LowBelow ? Low : Value >= HighFrom ? High : Mid;
	}

	/** What a survey reveals from the body's generated surface profile, and what a study adds. */
	void AddFindings(const APlanetaryBody* Body, const ESurvey From, const ESurvey To, TArray<FText>& OutLines)
	{
		const FAPSResolvedPlanetSurfaceProfile Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body);
		if (From < ESurvey::Surveyed && To >= ESurvey::Surveyed)
		{
			OutLines.Add(FText::Format(LOCTEXT("FindSurface", "Surface: {0}"), EnumText(Profile.Archetype)));
			OutLines.Add(FText::Format(LOCTEXT("FindLiquid", "Liquid: {0}"), Profile.LiquidType == EAPSPlanetLiquidType::None
				? LOCTEXT("FindNoLiquid", "NONE") : EnumText(Profile.LiquidType)));
			OutLines.Add(FText::Format(LOCTEXT("FindTemperature", "Temperature: {0}"), Grade(Profile.Temperature,
				LOCTEXT("Cold", "COLD"), LOCTEXT("Temperate", "TEMPERATE"), LOCTEXT("Hot", "HOT"), 0.3f, 0.7f)));
			// The body's air from the atmosphere model: the generated profile carries one default pressure for all.
			OutLines.Add(FText::Format(LOCTEXT("FindAtmosphere", "Atmosphere: {0}"),
				APSAtmosphereModel::Describe(APSAtmosphereModel::Density(Body))));
			OutLines.Add(FText::Format(LOCTEXT("FindHabitability", "Habitability: {0}"), EnumText(Body->PlanetHabitability)));
		}
		if (From < ESurvey::Studied && To >= ESurvey::Studied)
		{
			OutLines.Add(FText::Format(LOCTEXT("FindLife", "Biosphere: {0}"), Profile.Biomass < 0.02f
				? LOCTEXT("NoLife", "NONE") : Grade(Profile.Biomass, LOCTEXT("SparseLife", "SPARSE"),
					LOCTEXT("LivingLife", "LIVING"), LOCTEXT("RichLife", "RICH"))));
			OutLines.Add(FText::Format(LOCTEXT("FindGeology", "Geology: {0}"), Grade(Profile.SeismicActivity,
				LOCTEXT("Calm", "CALM"), LOCTEXT("Active", "ACTIVE"), LOCTEXT("Violent", "VIOLENT"))));
			OutLines.Add(FText::Format(LOCTEXT("FindMetals", "Metals: {0}"), Grade(Profile.Metallic,
				LOCTEXT("PoorMetal", "POOR"), LOCTEXT("SomeMetal", "WORKABLE"), LOCTEXT("RichMetal", "RICH"))));
		}
	}

	/**
	 * The actor rotation that points the ship's own nose and roof (hulls author them; ShipForward/ShipUp are their world
	 * directions now) along Forward and UpHint.
	 */
	FQuat ShipFacing(const FQuat& Actor, const FVector& ShipForward, const FVector& ShipUp, const FVector& Forward,
		FVector UpHint)
	{
		if (FMath::Abs(FVector::DotProduct(Forward, UpHint)) > 0.98)
		{
			UpHint = ShipUp;
			if (FMath::Abs(FVector::DotProduct(Forward, UpHint)) > 0.98)
			{
				return Actor;
			}
		}
		const FQuat LocalBasis = FRotationMatrix::MakeFromXZ(Actor.UnrotateVector(ShipForward),
			Actor.UnrotateVector(ShipUp)).ToQuat();
		return FRotationMatrix::MakeFromXZ(Forward, UpHint).ToQuat() * LocalBasis.Inverse();
	}

	int32 AtLeastOne(const int32 Value)
	{
		return FMath::Max(Value, 1);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Rules

FText APSFleet::DivisionName(const EDivision Division)
{
	switch (Division)
	{
	case EDivision::MainFleet: return LOCTEXT("MainFleet", "MAIN FLEET");
	case EDivision::Exploration: return LOCTEXT("Exploration", "EXPLORATION");
	case EDivision::Science: return LOCTEXT("Science", "SCIENCE");
	case EDivision::Construction: return LOCTEXT("Construction", "CONSTRUCTION");
	default: return FText::GetEmpty();
	}
}

FText APSFleet::DivisionRole(const EDivision Division)
{
	switch (Division)
	{
	case EDivision::MainFleet: return LOCTEXT("MainFleetRole", "The line: escorts and holds positions. Military level.");
	case EDivision::Exploration: return LOCTEXT("ExplorationRole", "Surveys planets and moons. Exploration level speeds it up.");
	case EDivision::Science: return LOCTEXT("ScienceRole", "Studies worlds in depth: life, geology, metals. Science level.");
	case EDivision::Construction: return LOCTEXT("ConstructionRole", "Builds outposts at surveyed worlds. Industry level.");
	default: return FText::GetEmpty();
	}
}

FLinearColor APSFleet::DivisionColour(const EDivision Division)
{
	switch (Division)
	{
	case EDivision::MainFleet: return FLinearColor(1.0f, 0.46f, 0.36f);
	case EDivision::Exploration: return FLinearColor(0.36f, 0.86f, 1.0f);
	case EDivision::Science: return FLinearColor(0.72f, 0.56f, 1.0f);
	case EDivision::Construction: return FLinearColor(1.0f, 0.76f, 0.3f);
	default: return FLinearColor::White;
	}
}

FText APSFleet::OrderName(const EOrder Order)
{
	switch (Order)
	{
	case EOrder::Move: return LOCTEXT("OrderMove", "MOVE");
	case EOrder::Survey: return LOCTEXT("OrderSurvey", "SURVEY");
	case EOrder::BuildOutpost: return LOCTEXT("OrderOutpost", "BUILD OUTPOST");
	case EOrder::Return: return LOCTEXT("OrderReturn", "RETURN");
	default: return LOCTEXT("OrderNone", "NO ORDERS");
	}
}

FText APSFleet::SurveyName(const ESurvey Survey)
{
	switch (Survey)
	{
	case ESurvey::Surveyed: return LOCTEXT("Surveyed", "SURVEYED");
	case ESurvey::Studied: return LOCTEXT("Studied", "STUDIED");
	default: return LOCTEXT("Unknown", "NO DATA");
	}
}

APSFleet::EDivision APSFleet::DefaultDivision(const ESpaceshipSizeClass SizeClass, const bool bFlagship)
{
	if (bFlagship)
	{
		return EDivision::MainFleet;
	}
	switch (SizeClass)
	{
	case ESpaceshipSizeClass::XXS:
	case ESpaceshipSizeClass::XS:
		return EDivision::Exploration;
	case ESpaceshipSizeClass::S:
		return EDivision::Science;
	case ESpaceshipSizeClass::M:
		return EDivision::Construction;
	default:
		return EDivision::MainFleet;
	}
}

bool APSFleet::DivisionCan(const EDivision Division, const EOrder Order)
{
	switch (Order)
	{
	case EOrder::Move:
	case EOrder::Return:
		return true;
	case EOrder::Survey:
		return Division == EDivision::Exploration || Division == EDivision::Science;
	case EOrder::BuildOutpost:
		return Division == EDivision::Construction;
	default:
		return false;
	}
}

APSFleet::ESurvey APSFleet::SurveyBy(const EDivision Division)
{
	return Division == EDivision::Science ? ESurvey::Studied
		: Division == EDivision::Exploration ? ESurvey::Surveyed : ESurvey::Unknown;
}

APSFleet::EDivision APSFleet::NextDivision(const EDivision Division)
{
	return static_cast<EDivision>((static_cast<uint8>(Division) + 1) % static_cast<uint8>(EDivision::Count));
}

double APSFleet::StepSpeed(const double CurrentSpeed, const double DistanceCm, const double SurfaceDistanceCm,
	const double ClassCapCm, const double DeltaSeconds)
{
	const double Approach = 0.8 * FMath::Max(DistanceCm, 0.0);
	const double NearBody = FMath::Max(200000.0, FMath::Max(SurfaceDistanceCm, 0.0));
	const double Desired = FMath::Min3(Approach, NearBody, FMath::Max(ClassCapCm, 1.0));
	const double Gain = FMath::Max(CurrentSpeed * FMath::Pow(2.5, DeltaSeconds), CurrentSpeed + 5000.0 * DeltaSeconds);
	return FMath::Max(0.0, FMath::Min(Desired, Gain));
}

double APSFleet::ClassCap(const FSpaceshipClassPreset& Preset)
{
	// Without SpaceWrap 20 000 km/s, so a moon ~25 000 km out is ~20 s away (at 100 km/s it took 3.5 minutes, e10-fleet-1);
	// with it the CRUISE band's scale (about 330 c), so a planet 1 AU away is ~30 s.
	return Preset.bSupportsSpaceWrap ? 1.0e13 : 2.0e9;
}

double APSFleet::WorkSeconds(const EOrder Order, const EDivision Division, const int32 DivisionLevel)
{
	const double Base = Order == EOrder::BuildOutpost ? 36.0 : Division == EDivision::Science ? 30.0 : 20.0;
	return Base / (1.0 + 0.2 * FMath::Max(DivisionLevel, 0));
}

double APSFleet::SlotRadius(const double BodyRadiusCm)
{
	return BodyRadiusCm + FMath::Max(BodyRadiusCm * 0.25, 30000000.0);
}

bool APSFleet::Detour(const FVector& Start, const FVector& End, const FVector& Centre, const double Radius,
	FVector& OutPoint)
{
	if (Radius <= 0.0 || FVector::DistSquared(End, Centre) < FMath::Square(Radius))
	{
		return false;
	}
	const FVector Closest = FMath::ClosestPointOnSegment(Centre, Start, End);
	// Clear of the body, or closest only at the end: an approach from above to a low berth (a shipyard at 1.01 radii).
	if (FVector::DistSquared(Closest, Centre) >= FMath::Square(Radius * 1.1)
		|| FVector::DistSquared(Closest, End) < FMath::Square(Radius * 0.05))
	{
		return false;
	}
	FVector Out = Closest - Centre;
	if (Out.SizeSquared() < 1.0)
	{
		// Straight through the centre: go round on any side across the way.
		Out = FVector::CrossProduct(End - Start, FVector::UpVector);
		if (Out.SizeSquared() < 1.0)
		{
			Out = FVector::CrossProduct(End - Start, FVector::ForwardVector);
		}
	}
	OutPoint = Centre + Out.GetSafeNormal() * Radius * 1.35;
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// Registry

FAPSFleetCommand* APSFleetFind(const UWorld* World)
{
	return World ? APSFleetPrivate::Registry().FindRef(World) : nullptr;
}

void APSFleetRegister(const UWorld* World, FAPSFleetCommand* Fleet)
{
	if (!World)
	{
		return;
	}
	if (Fleet)
	{
		APSFleetPrivate::Registry().Add(World, Fleet);
	}
	else
	{
		APSFleetPrivate::Registry().Remove(World);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Fleet command

FAPSFleetCommand::FAPSFleetCommand(UWorld* InWorld)
	: World(InWorld)
{
}

FAPSFleetCommand::~FAPSFleetCommand() = default;

UCivilization* FAPSFleetCommand::Civilization() const
{
	// The gameplay state is a game-instance subsystem (as the terminal reads it).
	const UWorld* LiveWorld = World.Get();
	const UGameInstance* GameInstance = LiveWorld ? LiveWorld->GetGameInstance() : nullptr;
	const UMainGameplayInstance* State = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	return State ? State->CurrentCivilization.Get() : nullptr;
}

int32 FAPSFleetCommand::DivisionLevel(const APSFleet::EDivision Division) const
{
	const UCivilization* Civ = Civilization();
	if (!Civ)
	{
		return 1;
	}
	switch (Division)
	{
	case APSFleet::EDivision::MainFleet: return Civ->Divisions.Military;
	case APSFleet::EDivision::Exploration: return Civ->Divisions.Exploration;
	case APSFleet::EDivision::Science: return Civ->Divisions.Science;
	case APSFleet::EDivision::Construction: return Civ->Divisions.Industry;
	default: return 1;
	}
}

double FAPSFleetCommand::SpeedScale() const
{
	const UCivilization* Civ = Civilization();
	return FMath::Max(APSFleetPrivate::CVarSpeedScale.GetValueOnGameThread(), 0.01f)
		* (1.0 + 0.1 * (Civ ? FMath::Max(Civ->Divisions.FleetCommand, 0) : 0));
}

void FAPSFleetCommand::Post(const FText& Text) const
{
	UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Fleet"), Text);
}

FText FAPSFleetCommand::UnitName(const FAPSFleetUnit& Unit) const
{
	return FText::FromString(Unit.CallSign);
}

const FAPSFleetUnit* FAPSFleetCommand::FindUnit(const ASpaceship* Ship) const
{
	return Ship ? Units.FindByPredicate([Ship](const FAPSFleetUnit& Unit) { return Unit.Ship.Get() == Ship; }) : nullptr;
}

FAPSFleetUnit* FAPSFleetCommand::FindUnitMutable(const ASpaceship* Ship)
{
	return Ship ? Units.FindByPredicate([Ship](const FAPSFleetUnit& Unit) { return Unit.Ship.Get() == Ship; }) : nullptr;
}

void FAPSFleetCommand::SetDivision(const ASpaceship* Ship, const APSFleet::EDivision Division)
{
	FAPSFleetUnit* Unit = FindUnitMutable(Ship);
	if (!Unit || Unit->Division == Division)
	{
		return;
	}
	Unit->Division = Division;
	// An order the new division cannot carry out stops where the ship is.
	if (Unit->Order != APSFleet::EOrder::None && !APSFleet::DivisionCan(Division, Unit->Order))
	{
		CancelOrder(Ship);
	}
	++Revision;
	Post(FText::Format(LOCTEXT("Reassigned", "{0} joins {1}."), UnitName(*Unit), APSFleet::DivisionName(Division)));
}

APSFleet::ESurvey FAPSFleetCommand::GetSurvey(const AActor* Body) const
{
	const FAPSFleetBodyRecord* Record = FindBody(Body);
	return Record ? Record->Survey : APSFleet::ESurvey::Unknown;
}

const FAPSFleetBodyRecord* FAPSFleetCommand::FindBody(const AActor* Body) const
{
	return Body ? Bodies.FindByPredicate([Body](const FAPSFleetBodyRecord& Record) { return Record.Body.Get() == Body; })
		: nullptr;
}

int32 FAPSFleetCommand::CountOutposts(const AActor* Body) const
{
	const FAPSFleetBodyRecord* Record = FindBody(Body);
	if (!Record)
	{
		return 0;
	}
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Outpost : Record->Outposts)
	{
		Count += Outpost.IsValid() ? 1 : 0;
	}
	return Count;
}

FAPSFleetBodyRecord& FAPSFleetCommand::BodyRecord(APlanetaryBody* Body)
{
	if (FAPSFleetBodyRecord* Record = Bodies.FindByPredicate([Body](const FAPSFleetBodyRecord& Entry)
		{
			return Entry.Body.Get() == Body;
		}))
	{
		return *Record;
	}
	FAPSFleetBodyRecord& Record = Bodies.AddDefaulted_GetRef();
	Record.Body = Body;
	return Record;
}

AActor* FAPSFleetCommand::NearestBody(const FVector& Location, double& OutSurfaceDistanceCm, double& OutRadiusCm) const
{
	AActor* Best = nullptr;
	OutSurfaceDistanceCm = TNumericLimits<double>::Max();
	OutRadiusCm = 0.0;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return nullptr;
	}
	for (TActorIterator<ACelestialBody> It(LiveWorld); It; ++It)
	{
		const double Radius = APSFleetPrivate::BodyRadiusCm(*It);
		if (Radius <= 0.0)
		{
			continue;
		}
		const double Surface = FVector::Distance(Location, It->GetActorLocation()) - Radius;
		if (Surface < OutSurfaceDistanceCm)
		{
			OutSurfaceDistanceCm = Surface;
			OutRadiusCm = Radius;
			Best = *It;
		}
	}
	return Best;
}

APlanetaryBody* FAPSFleetCommand::HomeBodyOf(const FVector& Location) const
{
	// The planet whose neighbourhood it is (a moon's or station's planet): the nearest in its own radii, as the map does.
	APlanet* Best = nullptr;
	double BestRatio = TNumericLimits<double>::Max();
	if (UWorld* LiveWorld = World.Get())
	{
		for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
		{
			const double Ratio = FVector::Distance(Location, It->GetActorLocation())
				/ FMath::Max(It->GetWorldScapeBodyRadiusCm(), 1.0);
			if (Ratio < BestRatio)
			{
				BestRatio = Ratio;
				Best = *It;
			}
		}
	}
	return BestRatio <= 400.0 ? Best : nullptr;
}

void FAPSFleetCommand::RefreshUnits()
{
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	const int32 Before = Units.Num();
	Units.RemoveAll([](const FAPSFleetUnit& Unit) { return !Unit.Ship.IsValid(); });
	bool bChanged = Units.Num() != Before;
	for (TActorIterator<ASpaceship> It(LiveWorld); It; ++It)
	{
		ASpaceship* Ship = *It;
		if (!IsFleetShip(Ship) || FindUnit(Ship))
		{
			continue;
		}
		FAPSFleetUnit& Unit = Units.AddDefaulted_GetRef();
		Unit.Ship = Ship;
		Unit.bFlagship = Ship->ActorHasTag(HomeShipTag);
		Unit.Division = APSFleet::DefaultDivision(Ship->SizeClass, Unit.bFlagship);
		Unit.CallSign = Unit.bFlagship ? TEXT("FLAGSHIP")
			: FString::Printf(TEXT("%s-%02d"), *Ship->GetSizeClassName(), ++RegisteredCount);
		bChanged = true;
	}
	if (bChanged)
	{
		Units.Sort([](const FAPSFleetUnit& A, const FAPSFleetUnit& B)
		{
			if (A.bFlagship != B.bFlagship)
			{
				return A.bFlagship;
			}
			return A.CallSign < B.CallSign;
		});
		++Revision;
	}
	if (PendingRestore.IsSet())
	{
		ApplyPendingRestore();
	}
	// The home world is known: the colony stands on it.
	if (!bHomeKnown)
	{
		const FAPSFleetUnit* Flagship = Units.FindByPredicate([](const FAPSFleetUnit& Unit) { return Unit.bFlagship; });
		if (const ASpaceship* Ship = Flagship ? Flagship->Ship.Get() : nullptr)
		{
			if (APlanetaryBody* Home = HomeBodyOf(Ship->GetActorLocation()))
			{
				FAPSFleetBodyRecord& Record = BodyRecord(Home);
				AddFindings(Home, APSFleet::ESurvey::Unknown, APSFleet::ESurvey::Studied, Record.Findings);
				Record.Survey = APSFleet::ESurvey::Studied;
				bHomeKnown = true;
				++Revision;
			}
		}
	}
}

FText FAPSFleetCommand::CheckOrder(const ASpaceship* Ship, const APSFleet::EOrder Order, const AActor* Target) const
{
	using namespace APSFleet;
	const FAPSFleetUnit* Unit = FindUnit(Ship);
	if (!Unit)
	{
		return LOCTEXT("NotOurs", "Not a ship of the civilization.");
	}
	if (Ship->HasPilot())
	{
		return LOCTEXT("Piloted", "Someone is at the helm: orders go to crewless ships.");
	}
	if (Order == EOrder::None)
	{
		return LOCTEXT("NoOrder", "Choose an order.");
	}
	if (Order == EOrder::Return)
	{
		return Unit->bHasBerth || Unit->Order != EOrder::None ? FText::GetEmpty()
			: LOCTEXT("AtBerth", "Already at its berth.");
	}
	if (!IsValid(Target))
	{
		return LOCTEXT("PickTarget", "Pick a target on the map.");
	}
	if (!DivisionCan(Unit->Division, Order))
	{
		return Order == EOrder::Survey ? LOCTEXT("SurveyDivision", "Surveys need an exploration or science ship.")
			: LOCTEXT("OutpostDivision", "Outposts need a construction ship.");
	}
	const bool bBody = Target->IsA<APlanetaryBody>();
	if (Order == EOrder::Move && !bBody && !Target->IsA<ATechActor>())
	{
		return LOCTEXT("MoveTarget", "Ships fly to planets, moons, stations and outposts.");
	}
	if ((Order == EOrder::Survey || Order == EOrder::BuildOutpost) && !bBody)
	{
		return LOCTEXT("BodyTarget", "Pick a planet or a moon.");
	}
	if (Order == EOrder::Survey && GetSurvey(Target) >= SurveyBy(Unit->Division))
	{
		return Unit->Division == EDivision::Science ? LOCTEXT("AlreadyStudied", "Already studied.")
			: LOCTEXT("AlreadySurveyed", "Already surveyed: a science ship can study it deeper.");
	}
	if (Order == EOrder::BuildOutpost)
	{
		if (GetSurvey(Target) == ESurvey::Unknown)
		{
			return LOCTEXT("NeedSurvey", "No data on this world: survey it first.");
		}
		if (CountOutposts(Target) >= 3)
		{
			return LOCTEXT("OutpostLimit", "Three outposts already orbit it.");
		}
	}
	if (!Ship->ActiveClassPreset.bSupportsSpaceWrap && HomeBodyOf(Target->GetActorLocation()) != HomeBodyOf(Ship->GetActorLocation()))
	{
		return FText::Format(LOCTEXT("NoWrap", "Class {0} has no SpaceWrap: it only flies near its own planet."),
			FText::FromString(Ship->GetSizeClassName()));
	}
	if (Unit->Target.Get() == Target && Unit->Order == Order)
	{
		return LOCTEXT("SameOrder", "Already under this order.");
	}
	return FText::GetEmpty();
}

int32 FAPSFleetCommand::IssueOrder(const TArray<ASpaceship*>& Ships, const APSFleet::EOrder Order, AActor* Target,
	FText& OutRefusal)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	OutRefusal = FText::GetEmpty();
	int32 Issued = 0;
	FString Names;
	for (ASpaceship* Ship : Ships)
	{
		const FText Refusal = CheckOrder(Ship, Order, Target);
		FAPSFleetUnit* Unit = FindUnitMutable(Ship);
		if (!Refusal.IsEmpty() || !Unit)
		{
			if (OutRefusal.IsEmpty())
			{
				OutRefusal = Unit ? FText::Format(LOCTEXT("Refused", "{0}: {1}"), UnitName(*Unit), Refusal) : Refusal;
			}
			continue;
		}
		// The berth is where the ship waited before its first order: its parent then, or where it stood.
		if (!Unit->bHasBerth)
		{
			Unit->Berth = Ship->GetAttachParentActor();
			Unit->BerthRelative = Unit->Berth.IsValid()
				? Ship->GetActorTransform().GetRelativeTransform(Unit->Berth->GetActorTransform()) : Ship->GetActorTransform();
			Unit->bHasBerth = true;
			Unit->bBerthParked = Ship->ActorHasTag(ParkedTag);
		}
		Ship->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Ship->Tags.Remove(ParkedTag);
		// Clear the berth first: straight out from the nearest body, a few ship lengths.
		double Surface = 0.0, Radius = 0.0;
		AActor* Near = NearestBody(Ship->GetActorLocation(), Surface, Radius);
		const FVector Out = Near ? (Ship->GetActorLocation() - Near->GetActorLocation()).GetSafeNormal() : Ship->GetShipUpVector();
		const double Clearance = FMath::Max(300000.0, Ship->GetComponentsBoundingBox().GetExtent().GetMax() * 6.0);
		Unit->DepartFrom = Near;
		Unit->DepartOffset = Ship->GetActorLocation() + Out * Clearance - (Near ? Near->GetActorLocation() : FVector::ZeroVector);
		Unit->Order = Order;
		Unit->Target = Order == EOrder::Return ? nullptr : Target;
		Unit->Phase = EPhase::Departing;
		Unit->Progress = 0.0f;
		Unit->Speed = 0.0;
		Unit->Heading = Ship->GetShipForwardVector();
		// A slot on the side of the target the ship comes from, fanned out so a group does not stack.
		if (Order != EOrder::Return)
		{
			FVector Side = (Ship->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
			if (Side.IsNearlyZero())
			{
				Side = FVector::UpVector;
			}
			Unit->SlotDirection = FQuat(Target->GetActorUpVector(), FMath::DegreesToRadians(9.0 * Issued)).RotateVector(Side);
		}
		++Issued;
		Names += (Names.IsEmpty() ? TEXT("") : TEXT(", ")) + Unit->CallSign;
	}
	if (Issued > 0)
	{
		++Revision;
		Post(Order == EOrder::Return
			? FText::Format(LOCTEXT("PostReturn", "Order to {0}: return to the berth."), FText::FromString(Names))
			: FText::Format(LOCTEXT("PostOrder", "Order to {0}: {1} {2}."), FText::FromString(Names),
				OrderName(Order), NameOf(Target)));
	}
	return Issued;
}

void FAPSFleetCommand::CancelOrder(const ASpaceship* Ship)
{
	FAPSFleetUnit* Unit = FindUnitMutable(Ship);
	if (!Unit || Unit->Order == APSFleet::EOrder::None)
	{
		return;
	}
	Unit->Order = APSFleet::EOrder::None;
	Unit->Phase = APSFleet::EPhase::Idle;
	Unit->Target = nullptr;
	Unit->Speed = 0.0;
	Unit->Progress = 0.0f;
	++Revision;
	Post(FText::Format(LOCTEXT("Cancelled", "{0}: orders cancelled, holding position."), UnitName(*Unit)));
}

FVector FAPSFleetCommand::SlotLocation(const FAPSFleetUnit& Unit) const
{
	if (Unit.Order == APSFleet::EOrder::Return)
	{
		const AActor* Berth = Unit.Berth.Get();
		return Berth ? (Unit.BerthRelative * Berth->GetActorTransform()).GetLocation() : Unit.BerthRelative.GetLocation();
	}
	const AActor* Target = Unit.Target.Get();
	if (!Target)
	{
		return Unit.Ship.IsValid() ? Unit.Ship->GetActorLocation() : FVector::ZeroVector;
	}
	const double Radius = APSFleetPrivate::BodyRadiusCm(Target);
	return Target->GetActorLocation() + Unit.SlotDirection * (Radius > 0.0 ? APSFleet::SlotRadius(Radius) : 150000.0);
}

void FAPSFleetCommand::Tick(const float DeltaSeconds)
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || DeltaSeconds <= 0.0f)
	{
		return;
	}
	const double Now = LiveWorld->GetTimeSeconds();
	if (Now >= RefreshSeconds)
	{
		RefreshSeconds = Now + 1.0;
		RefreshUnits();
		if (APSFleetPrivate::CVarLog.GetValueOnGameThread() != 0)
		{
			for (const FAPSFleetUnit& Unit : Units)
			{
				if (Unit.Order != APSFleet::EOrder::None)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s %s phase=%d remaining=%.1f km speed=%.2f km/s progress=%.2f"),
						*Unit.CallSign, *APSFleet::OrderName(Unit.Order).ToString(), static_cast<int32>(Unit.Phase),
						Unit.RemainingCm / 100000.0, Unit.Speed / 100000.0, Unit.Progress);
				}
			}
		}
	}
	for (FAPSFleetUnit& Unit : Units)
	{
		TickUnit(Unit, DeltaSeconds);
	}
	TickShipyard(DeltaSeconds);
}

void FAPSFleetCommand::TickUnit(FAPSFleetUnit& Unit, const float DeltaSeconds)
{
	using namespace APSFleet;
	ASpaceship* Ship = Unit.Ship.Get();
	if (!Ship || Unit.Order == EOrder::None)
	{
		return;
	}
	if (Ship->HasPilot())
	{
		// The pilot took the helm: the order ends where the ship is.
		Post(FText::Format(LOCTEXT("TookHelm", "{0}: a pilot took the helm, orders cancelled."), UnitName(Unit)));
		Unit.Order = EOrder::None;
		Unit.Phase = EPhase::Idle;
		Unit.Target = nullptr;
		++Revision;
		return;
	}
	if (Unit.Order != EOrder::Return && !Unit.Target.IsValid())
	{
		CancelOrder(Ship);
		return;
	}
	switch (Unit.Phase)
	{
	case EPhase::Departing:
	{
		const AActor* From = Unit.DepartFrom.Get();
		const FVector Goal = (From ? From->GetActorLocation() : FVector::ZeroVector) + Unit.DepartOffset;
		double Remaining = 0.0;
		Fly(Unit, Ship, Goal, 0.0, DeltaSeconds, Remaining);
		Unit.RemainingCm = FVector::Distance(Ship->GetActorLocation(), SlotLocation(Unit));
		if (Remaining < 20000.0)
		{
			Unit.Phase = EPhase::Transit;
		}
		break;
	}
	case EPhase::Transit:
	{
		const FVector Slot = SlotLocation(Unit);
		// Round any planet, moon or star in the way: head for the first detour point, else the slot.
		FVector Goal = Slot;
		double BestDistance = TNumericLimits<double>::Max();
		if (UWorld* LiveWorld = World.Get())
		{
			for (TActorIterator<ACelestialBody> It(LiveWorld); It; ++It)
			{
				FVector Point;
				if (APSFleet::Detour(Ship->GetActorLocation(), Slot, It->GetActorLocation(), APSFleetPrivate::BodyRadiusCm(*It), Point))
				{
					const double Distance = FVector::DistSquared(Ship->GetActorLocation(), Point);
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						Goal = Point;
					}
				}
			}
		}
		double Remaining = 0.0;
		Fly(Unit, Ship, Goal, FVector::Distance(Goal, Slot), DeltaSeconds, Remaining);
		Unit.RemainingCm = Remaining;
		if (Goal.Equals(Slot) && Remaining < FMath::Max(10000.0, Unit.Speed * DeltaSeconds * 1.5))
		{
			Arrive(Unit, Ship);
		}
		break;
	}
	case EPhase::Working:
		Unit.Progress = FMath::Min(1.0f, Unit.Progress + DeltaSeconds * FMath::Max(APSFleetPrivate::CVarWorkScale.GetValueOnGameThread(), 0.0f)
			/ FMath::Max(Unit.WorkLength, 0.1f));
		if (Unit.Progress >= 1.0f)
		{
			FinishWork(Unit);
		}
		break;
	default:
		break;
	}
}

void FAPSFleetCommand::Fly(FAPSFleetUnit& Unit, ASpaceship* Ship, const FVector& Goal, const double BeyondGoalCm,
	const float DeltaSeconds, double& OutRemainingCm)
{
	const FVector Location = Ship->GetActorLocation();
	const FVector ToGoal = Goal - Location;
	const double Distance = ToGoal.Size();
	OutRemainingCm = Distance + BeyondGoalCm;
	double Surface = 0.0, Radius = 0.0;
	const AActor* Near = NearestBody(Location, Surface, Radius);
	const double Cap = Unit.Phase == APSFleet::EPhase::Departing ? 100000.0
		: APSFleet::ClassCap(Ship->ActiveClassPreset) * SpeedScale();
	Unit.Speed = APSFleet::StepSpeed(Unit.Speed, OutRemainingCm, Surface, Cap, DeltaSeconds);
	if (Distance < 1.0)
	{
		return;
	}
	// Turn the heading toward the goal over about a third of a second, then step no further than the goal.
	const FVector Wanted = ToGoal / Distance;
	Unit.Heading = Unit.Heading.IsNearlyZero() ? Wanted
		: (Unit.Heading + (Wanted - Unit.Heading) * FMath::Min(1.0f, DeltaSeconds * 3.0f)).GetSafeNormal();
	if (Unit.Heading.IsNearlyZero())
	{
		Unit.Heading = Wanted;
	}
	const double Step = FMath::Min(Unit.Speed * DeltaSeconds, Distance);
	const FVector UpHint = Near ? (Location - Near->GetActorLocation()).GetSafeNormal() : Ship->GetShipUpVector();
	const FQuat Facing = APSFleetPrivate::ShipFacing(Ship->GetActorQuat(), Ship->GetShipForwardVector(),
		Ship->GetShipUpVector(), Unit.Heading, UpHint);
	Ship->SetActorLocationAndRotation(Location + Unit.Heading * Step,
		FMath::QInterpTo(Ship->GetActorQuat(), Facing, DeltaSeconds, 2.5f), false, nullptr, ETeleportType::TeleportPhysics);
}

void FAPSFleetCommand::Arrive(FAPSFleetUnit& Unit, ASpaceship* Ship)
{
	using namespace APSFleet;
	Unit.Speed = 0.0;
	Unit.RemainingCm = 0.0;
	if (Unit.Order == EOrder::Return)
	{
		AActor* Berth = Unit.Berth.Get();
		const FTransform Home = Berth ? Unit.BerthRelative * Berth->GetActorTransform() : Unit.BerthRelative;
		Ship->SetActorTransform(Home, false, nullptr, ETeleportType::TeleportPhysics);
		if (Berth)
		{
			Ship->AttachToActor(Berth, FAttachmentTransformRules::KeepWorldTransform);
		}
		if (Unit.bBerthParked)
		{
			Ship->Tags.AddUnique(TEXT("APS.Civilization.SurfaceParked"));
		}
		Unit.Order = EOrder::None;
		Unit.Phase = EPhase::Idle;
		Unit.bHasBerth = false;
		++Revision;
		Post(FText::Format(LOCTEXT("Berthed", "{0} is back at its berth."), UnitName(Unit)));
		return;
	}
	AActor* Target = Unit.Target.Get();
	Ship->SetActorLocation(SlotLocation(Unit), false, nullptr, ETeleportType::TeleportPhysics);
	if (Target)
	{
		Ship->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
	}
	if (Unit.Order == EOrder::Move)
	{
		Unit.Phase = EPhase::Holding;
		Post(FText::Format(LOCTEXT("Arrived", "{0} arrived at {1}."), UnitName(Unit), APSFleetPrivate::NameOf(Target)));
	}
	else
	{
		Unit.Phase = EPhase::Working;
		Unit.Progress = 0.0f;
		Unit.WorkLength = static_cast<float>(WorkSeconds(Unit.Order, Unit.Division, DivisionLevel(Unit.Division)));
		Post(FText::Format(Unit.Order == EOrder::Survey ? LOCTEXT("SurveyStarted", "{0} began surveying {1}.")
			: LOCTEXT("BuildStarted", "{0} began building an outpost at {1}."), UnitName(Unit), APSFleetPrivate::NameOf(Target)));
	}
	++Revision;
}

void FAPSFleetCommand::FinishWork(FAPSFleetUnit& Unit)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	APlanetaryBody* Body = Cast<APlanetaryBody>(Unit.Target.Get());
	Unit.Phase = EPhase::Holding;
	Unit.Progress = 1.0f;
	++Revision;
	if (!Body)
	{
		return;
	}
	FAPSFleetBodyRecord& Record = BodyRecord(Body);
	if (Unit.Order == EOrder::Survey)
	{
		const ESurvey Reached = SurveyBy(Unit.Division);
		if (Reached <= Record.Survey)
		{
			return;
		}
		TArray<FText> Lines;
		AddFindings(Body, Record.Survey, Reached, Lines);
		Record.Findings.Append(Lines);
		Record.Survey = Reached;
		Post(FText::Format(LOCTEXT("SurveyDone", "{0} {1} {2}: {3}."), UnitName(Unit), Reached == ESurvey::Studied
			? LOCTEXT("StudiedVerb", "studied") : LOCTEXT("SurveyedVerb", "surveyed"), NameOf(Body),
			FText::Join(FText::FromString(TEXT("; ")), Lines)));
		// Ships still working on what is now known are done too.
		for (FAPSFleetUnit& Other : Units)
		{
			if (&Other != &Unit && Other.Order == EOrder::Survey && Other.Target.Get() == Body
				&& Other.Phase == EPhase::Working && SurveyBy(Other.Division) <= Record.Survey)
			{
				Other.Phase = EPhase::Holding;
				Other.Progress = 1.0f;
			}
		}
		return;
	}
	if (Unit.Order == EOrder::BuildOutpost)
	{
		UWorld* LiveWorld = World.Get();
		const ASpaceship* Ship = Unit.Ship.Get();
		if (!LiveWorld || !Ship)
		{
			return;
		}
		// Beside the builder in its orbit, a few kilometres along, turned to the body.
		const FVector Radial = Unit.SlotDirection.GetSafeNormal();
		const FVector Along = FVector::CrossProduct(Body->GetActorUpVector(), Radial).GetSafeNormal();
		const FVector Location = Ship->GetActorLocation() + (Along.IsNearlyZero() ? FVector::ForwardVector : Along) * 400000.0;
		const FText Name = FText::Format(LOCTEXT("OutpostName", "Outpost {0} {1}"),
			FText::FromString(NameOf(Body).ToString()), FText::AsNumber(CountOutposts(Body) + 1));
		if (!SpawnOutpost(Body, Location, FRotationMatrix::MakeFromZ(Radial).ToQuat(), Name))
		{
			Post(FText::Format(LOCTEXT("BuildFailed", "{0} could not place the outpost at {1}."), UnitName(Unit), NameOf(Body)));
			return;
		}
		Post(FText::Format(LOCTEXT("OutpostBuilt", "{0} built {1}."), UnitName(Unit), Name));
	}
}

FText FAPSFleetCommand::DescribeState(const FAPSFleetUnit& Unit) const
{
	using namespace APSFleet;
	const ASpaceship* Ship = Unit.Ship.Get();
	if (Ship && Ship->HasPilot())
	{
		return LOCTEXT("StatePiloted", "PILOTED");
	}
	const FText Target = APSFleetPrivate::NameOf(Unit.Target.Get());
	switch (Unit.Phase)
	{
	case EPhase::Departing:
		return FText::Format(LOCTEXT("StateDeparting", "{0} {1}: LEAVING THE BERTH"), OrderName(Unit.Order), Target);
	case EPhase::Transit:
		return FText::Format(LOCTEXT("StateTransit", "{0} {1}: {2} KM TO GO"), OrderName(Unit.Order),
			Unit.Order == EOrder::Return ? LOCTEXT("ToBerth", "TO THE BERTH") : Target,
			FText::AsNumber(FMath::RoundToInt64(Unit.RemainingCm / 100000.0)));
	case EPhase::Working:
		return FText::Format(LOCTEXT("StateWorking", "{0} {1}: {2}%"), OrderName(Unit.Order), Target,
			FText::AsNumber(FMath::RoundToInt(Unit.Progress * 100.0f)));
	case EPhase::Holding:
		return Unit.Order == EOrder::Move ? FText::Format(LOCTEXT("StateHolding", "HOLDING AT {0}"), Target)
			: FText::Format(LOCTEXT("StateDone", "{0} {1}: DONE, HOLDING"), OrderName(Unit.Order), Target);
	default:
		return Unit.bHasBerth ? LOCTEXT("StateIdleSpace", "IDLE, HOLDING POSITION") : LOCTEXT("StateIdle", "IDLE AT THE BERTH");
	}
}

void FAPSFleetCommand::LogUnits() const
{
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		const FAPSFleetUnit& Unit = Units[Index];
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %d %s class=%s division=%s ship=%s | %s"), Index, *Unit.CallSign,
			Unit.Ship.IsValid() ? *Unit.Ship->GetSizeClassName() : TEXT("?"), *APSFleet::DivisionName(Unit.Division).ToString(),
			*GetNameSafe(Unit.Ship.Get()), *DescribeState(Unit).ToString());
	}
	for (const FAPSFleetBodyRecord& Record : Bodies)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] body %s %s outposts=%d"), *APSFleetPrivate::NameOf(Record.Body.Get()).ToString(),
			*APSFleet::SurveyName(Record.Survey).ToString(), CountOutposts(Record.Body.Get()));
	}
}

int32 FAPSFleetCommand::ConsoleOrder(const FString& Who, const APSFleet::EOrder Order, const FString& TargetName,
	FText& OutRefusal)
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return 0;
	}
	RefreshUnits();
	TArray<ASpaceship*> Ships;
	for (const FAPSFleetUnit& Unit : Units)
	{
		const bool bPick = Who.Equals(TEXT("all"), ESearchCase::IgnoreCase) || Unit.CallSign.Equals(Who, ESearchCase::IgnoreCase)
			|| APSFleet::DivisionName(Unit.Division).ToString().Replace(TEXT(" "), TEXT("")).Equals(Who, ESearchCase::IgnoreCase);
		if (bPick && Unit.Ship.IsValid())
		{
			Ships.Add(Unit.Ship.Get());
		}
	}
	// The target: the body or station whose name contains the text, nearest to the first ship; "moon" or "planet" alone
	// picks the nearest of that kind.
	AActor* Target = nullptr;
	if (!TargetName.IsEmpty() && Order != APSFleet::EOrder::Return)
	{
		const FVector From = Ships.Num() > 0 ? Ships[0]->GetActorLocation() : FVector::ZeroVector;
		double Best = TNumericLimits<double>::Max();
		for (TActorIterator<AActor> It(LiveWorld); It; ++It)
		{
			AActor* Actor = *It;
			const bool bKind = TargetName.Equals(TEXT("moon"), ESearchCase::IgnoreCase) ? Actor->IsA<AMoon>()
				: TargetName.Equals(TEXT("planet"), ESearchCase::IgnoreCase) ? Actor->IsA<APlanet>() && !Actor->IsA<AMoon>()
				: TargetName.Equals(TEXT("station"), ESearchCase::IgnoreCase) ? Actor->IsA<ASpaceStation>()
				: (Actor->IsA<APlanetaryBody>() || Actor->IsA<ATechActor>())
					&& APSFleetPrivate::NameOf(Actor).ToString().Contains(TargetName, ESearchCase::IgnoreCase);
			if (!bKind)
			{
				continue;
			}
			// A moon or planet other than the one the ships wait at.
			const double Distance = FVector::DistSquared(From, Actor->GetActorLocation());
			const double Radius = APSFleetPrivate::BodyRadiusCm(Actor);
			if (Radius > 0.0 && Distance < FMath::Square(Radius * 3.0) && TargetName.Equals(TEXT("planet"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (Distance < Best)
			{
				Best = Distance;
				Target = Actor;
			}
		}
	}
	if (Order == APSFleet::EOrder::None)
	{
		for (ASpaceship* Ship : Ships)
		{
			CancelOrder(Ship);
		}
		return Ships.Num();
	}
	return IssueOrder(Ships, Order, Target, OutRefusal);
}

// ---------------------------------------------------------------------------------------------------------------------
// Saves

FArchive& operator<<(FArchive& Ar, FAPSFleetSaveData& Data)
{
	int32 UnitCount = Data.Units.Num();
	Ar << UnitCount;
	if (Ar.IsLoading())
	{
		Data.Units.SetNum(FMath::Clamp(UnitCount, 0, 4096));
	}
	for (FAPSFleetUnitRecord& Unit : Data.Units)
	{
		Ar << Unit.CallSign << Unit.Division << Unit.Order << Unit.Phase << Unit.TargetKey << Unit.SlotDirection
			<< Unit.Progress << Unit.WorkLength << Unit.bHasBerth << Unit.bBerthParked << Unit.BerthKey
			<< Unit.BerthRelative << Unit.ReferenceKey << Unit.RelativeTransform << Unit.bPiloted << Unit.SpawnClassPath;
	}
	int32 SurveyCount = Data.Surveys.Num();
	Ar << SurveyCount;
	if (Ar.IsLoading())
	{
		Data.Surveys.SetNum(FMath::Clamp(SurveyCount, 0, 4096));
	}
	for (TPair<FString, uint8>& Survey : Data.Surveys)
	{
		Ar << Survey.Key << Survey.Value;
	}
	int32 OutpostCount = Data.Outposts.Num();
	Ar << OutpostCount;
	if (Ar.IsLoading())
	{
		Data.Outposts.SetNum(FMath::Clamp(OutpostCount, 0, 4096));
	}
	for (FAPSFleetSaveData::FOutpost& Outpost : Data.Outposts)
	{
		Ar << Outpost.BodyKey << Outpost.RelativeTransform << Outpost.Name;
	}
	return Ar;
}

FString FAPSFleetCommand::KeyOf(const AActor* Actor)
{
	if (!Actor)
	{
		return FString();
	}
	if (const ACelestialBody* Body = Cast<ACelestialBody>(Actor); Body && !Body->AstroName.IsNone())
	{
		return TEXT("BODY:") + Body->AstroName.ToString();
	}
	return TEXT("ACTOR:") + Actor->GetName();
}

AActor* FAPSFleetCommand::FindByKey(const FString& Key) const
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || Key.IsEmpty())
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(LiveWorld); It; ++It)
	{
		if (KeyOf(*It) == Key)
		{
			return *It;
		}
	}
	return nullptr;
}

AAutonomousOutpost* FAPSFleetCommand::SpawnOutpost(APlanetaryBody* Body, const FVector& Location, const FQuat& Rotation,
	const FText& Name)
{
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || !Body)
	{
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAutonomousOutpost* Outpost = LiveWorld->SpawnActor<AAutonomousOutpost>(AAutonomousOutpost::StaticClass(), Location,
		Rotation.Rotator(), Parameters);
	if (!Outpost)
	{
		return nullptr;
	}
	Outpost->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	Outpost->Tags.Add(TEXT("APS.GeneratedCivilization"));
	Outpost->Tags.Add(TEXT("APS.Infrastructure.PlanetOutpost"));
	Outpost->Tags.Add(OutpostTag);
	// The world actor keeps its display name protected; it is a reflected property, so set it through reflection.
	if (FTextProperty* NameProperty = FindFProperty<FTextProperty>(Outpost->GetClass(), TEXT("InGameName")))
	{
		NameProperty->SetPropertyValue_InContainer(Outpost, Name);
	}
	BodyRecord(Body).Outposts.Add(Outpost);
	if (UCivilization* Civ = Civilization())
	{
		++Civ->Infrastructure.PlanetOutposts;
	}
	++Revision;
	return Outpost;
}

void FAPSFleetCommand::CaptureSave(FAPSFleetSaveData& OutData) const
{
	using namespace APSFleet;
	OutData = FAPSFleetSaveData();
	for (const FAPSFleetUnit& Unit : Units)
	{
		const ASpaceship* Ship = Unit.Ship.Get();
		if (!Ship)
		{
			continue;
		}
		FAPSFleetUnitRecord& Record = OutData.Units.AddDefaulted_GetRef();
		Record.CallSign = Unit.CallSign;
		Record.Division = static_cast<uint8>(Unit.Division);
		Record.Order = static_cast<uint8>(Unit.Order);
		Record.Phase = static_cast<uint8>(Unit.Phase);
		Record.TargetKey = KeyOf(Unit.Target.Get());
		Record.SlotDirection = Unit.SlotDirection;
		Record.Progress = Unit.Progress;
		Record.WorkLength = Unit.WorkLength;
		Record.bHasBerth = Unit.bHasBerth;
		Record.bBerthParked = Unit.bBerthParked;
		Record.BerthKey = KeyOf(Unit.Berth.Get());
		Record.BerthRelative = Unit.BerthRelative;
		Record.bPiloted = Ship->HasPilot();
		Record.SpawnClassPath = Ship->ActorHasTag(APSFleetPrivate::BuiltTag) ? Ship->GetClass()->GetPathName() : FString();
		// Where it stands, relative to its target or the nearest body: the world origin moves between sessions.
		const AActor* Reference = Unit.Target.Get();
		if (!Reference)
		{
			double Surface = 0.0, Radius = 0.0;
			Reference = NearestBody(Ship->GetActorLocation(), Surface, Radius);
		}
		Record.ReferenceKey = KeyOf(Reference);
		Record.RelativeTransform = Reference
			? Ship->GetActorTransform().GetRelativeTransform(Reference->GetActorTransform()) : Ship->GetActorTransform();
	}
	for (const FAPSFleetBodyRecord& Body : Bodies)
	{
		const APlanetaryBody* Planet = Body.Body.Get();
		if (!Planet)
		{
			continue;
		}
		if (Body.Survey != ESurvey::Unknown)
		{
			OutData.Surveys.Emplace(KeyOf(Planet), static_cast<uint8>(Body.Survey));
		}
		for (const TWeakObjectPtr<AActor>& Outpost : Body.Outposts)
		{
			if (Outpost.IsValid())
			{
				FAPSFleetSaveData::FOutpost& Record = OutData.Outposts.AddDefaulted_GetRef();
				Record.BodyKey = KeyOf(Planet);
				Record.RelativeTransform = Outpost->GetActorTransform().GetRelativeTransform(Planet->GetActorTransform());
				Record.Name = APSFleetPrivate::NameOf(Outpost.Get()).ToString();
			}
		}
	}
}

void FAPSFleetCommand::SetPendingRestore(FAPSFleetSaveData&& Data)
{
	PendingRestore = MoveTemp(Data);
	PendingRestoreSince = World.IsValid() ? World->GetTimeSeconds() : 0.0;
	RefreshSeconds = 0.0;
}

void FAPSFleetCommand::ApplyPendingRestore()
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || !PendingRestore.IsSet())
	{
		return;
	}
	// The saved units come back with the generated hierarchy; wait for them, but not forever.
	const FAPSFleetSaveData Data = PendingRestore.GetValue();
	if (Units.Num() < Data.Units.Num() && LiveWorld->GetTimeSeconds() - PendingRestoreSince < 20.0)
	{
		return;
	}
	PendingRestore.Reset();
	for (const TPair<FString, uint8>& Survey : Data.Surveys)
	{
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(FindByKey(Survey.Key)))
		{
			FAPSFleetBodyRecord& Record = BodyRecord(Body);
			const ESurvey Level = static_cast<ESurvey>(FMath::Min<uint8>(Survey.Value, static_cast<uint8>(ESurvey::Studied)));
			if (Level > Record.Survey)
			{
				AddFindings(Body, Record.Survey, Level, Record.Findings);
				Record.Survey = Level;
			}
		}
	}
	int32 Outposts = 0;
	for (const FAPSFleetSaveData::FOutpost& Saved : Data.Outposts)
	{
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(FindByKey(Saved.BodyKey)))
		{
			const FTransform Transform = Saved.RelativeTransform * Body->GetActorTransform();
			Outposts += SpawnOutpost(Body, Transform.GetLocation(), Transform.GetRotation(), FText::FromString(Saved.Name)) ? 1 : 0;
		}
	}
	// Ships the shipyard built come back first: the generator only respawns its own fleet.
	for (const FAPSFleetUnitRecord& Saved : Data.Units)
	{
		if (Saved.SpawnClassPath.IsEmpty() || Units.ContainsByPredicate([&Saved](const FAPSFleetUnit& Unit)
			{
				return Unit.CallSign == Saved.CallSign;
			}))
		{
			continue;
		}
		const AActor* Reference = FindByKey(Saved.ReferenceKey);
		UClass* Class = LoadClass<ASpaceship>(nullptr, *Saved.SpawnClassPath);
		ASpaceship* Ship = Class && Reference ? LaunchShip(Class, Saved.RelativeTransform * Reference->GetActorTransform()) : nullptr;
		if (!Ship)
		{
			continue;
		}
		FAPSFleetUnit& Unit = Units.AddDefaulted_GetRef();
		Unit.Ship = Ship;
		Unit.CallSign = Saved.CallSign;
		int32 Number = 0;
		FString Digits;
		if (Saved.CallSign.Split(TEXT("-"), nullptr, &Digits, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			Number = FCString::Atoi(*Digits);
		}
		RegisteredCount = FMath::Max(RegisteredCount, Number);
	}
	int32 Restored = 0;
	for (const FAPSFleetUnitRecord& Saved : Data.Units)
	{
		FAPSFleetUnit* Unit = Units.FindByPredicate([&Saved](const FAPSFleetUnit& Candidate)
		{
			return Candidate.CallSign == Saved.CallSign;
		});
		ASpaceship* Ship = Unit ? Unit->Ship.Get() : nullptr;
		if (!Ship)
		{
			continue;
		}
		++Restored;
		Unit->Division = static_cast<EDivision>(FMath::Min<uint8>(Saved.Division, static_cast<uint8>(EDivision::Count) - 1));
		Unit->bHasBerth = Saved.bHasBerth;
		Unit->bBerthParked = Saved.bBerthParked;
		Unit->Berth = FindByKey(Saved.BerthKey);
		Unit->BerthRelative = Saved.BerthRelative;
		// A ship that left its berth stands where it was saved; one still at its berth is already there.
		const bool bAway = Saved.bHasBerth || Saved.Order != static_cast<uint8>(EOrder::None);
		if (bAway && !Saved.bPiloted && !Ship->HasPilot())
		{
			if (const AActor* Reference = FindByKey(Saved.ReferenceKey))
			{
				Ship->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				Ship->Tags.Remove(ParkedTag);
				Ship->SetActorTransform(Saved.RelativeTransform * Reference->GetActorTransform(), false, nullptr,
					ETeleportType::TeleportPhysics);
			}
		}
		Unit->Order = static_cast<EOrder>(FMath::Min<uint8>(Saved.Order, static_cast<uint8>(EOrder::Return)));
		Unit->Phase = static_cast<EPhase>(FMath::Min<uint8>(Saved.Phase, static_cast<uint8>(EPhase::Holding)));
		Unit->Target = FindByKey(Saved.TargetKey);
		Unit->SlotDirection = Saved.SlotDirection;
		Unit->Progress = Saved.Progress;
		Unit->WorkLength = Saved.WorkLength;
		Unit->Speed = 0.0;
		Unit->Heading = FVector::ZeroVector;
		if (Unit->Order != EOrder::None && Unit->Order != EOrder::Return && !Unit->Target.IsValid())
		{
			Unit->Order = EOrder::None;
			Unit->Phase = EPhase::Idle;
		}
		// Leaving the berth needs the point it was clearing; after a load it simply sets course.
		if (Unit->Phase == EPhase::Departing)
		{
			Unit->Phase = EPhase::Transit;
		}
		if ((Unit->Phase == EPhase::Working || Unit->Phase == EPhase::Holding) && Unit->Target.IsValid() && !Ship->HasPilot())
		{
			Ship->AttachToActor(Unit->Target.Get(), FAttachmentTransformRules::KeepWorldTransform);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] restored %d of %d units, %d surveys, %d outposts"), Restored, Data.Units.Num(),
		Data.Surveys.Num(), Outposts);
	++Revision;
}

// ---------------------------------------------------------------------------------------------------------------------
// Shipyard

ASpaceShipyard* FAPSFleetCommand::FindShipyard() const
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return nullptr;
	}
	// The civilization's own shipyard first (the escorts wait there), else any.
	ASpaceShipyard* Any = nullptr;
	for (TActorIterator<ASpaceShipyard> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		if (It->ActorHasTag(TEXT("APS.GeneratedCivilization")))
		{
			return *It;
		}
		Any = Any ? Any : *It;
	}
	return Any;
}

void FAPSFleetCommand::GetShipyardOptions(TArray<FAPSShipyardOption>& OutOptions) const
{
	OutOptions.Reset();
	UWorld* LiveWorld = World.Get();
	const UAPSShipCatalog* Catalog = nullptr;
	if (LiveWorld)
	{
		for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It) && It->ShipCatalog)
			{
				Catalog = It->ShipCatalog;
				break;
			}
		}
	}
	if (!Catalog)
	{
		return;
	}
	const float Industry = 1.0f + 0.2f * FMath::Max(DivisionLevel(APSFleet::EDivision::Construction), 0);
	for (const FAPSShipCatalogEntry& Entry : Catalog->Ships)
	{
		if (!Entry.ShipClass)
		{
			continue;
		}
		FAPSShipyardOption& Option = OutOptions.AddDefaulted_GetRef();
		Option.ShipClass = Entry.ShipClass;
		Option.SizeClass = Entry.SizeClass;
		FString Name = Entry.ShipClass->GetName();
		Name.RemoveFromStart(TEXT("BP_Spaceship_"));
		Name.RemoveFromEnd(TEXT("_C"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		Option.Name = FText::FromString(Name.ToUpper());
		Option.BuildSeconds = APSFleetPrivate::ClassBuildSeconds(Entry.SizeClass) / Industry;
	}
	OutOptions.Sort([](const FAPSShipyardOption& A, const FAPSShipyardOption& B)
	{
		return A.SizeClass != B.SizeClass ? A.SizeClass < B.SizeClass : A.Name.ToString() < B.Name.ToString();
	});
}

FText FAPSFleetCommand::OrderShip(const FAPSShipyardOption& Option)
{
	if (!Option.ShipClass)
	{
		return LOCTEXT("NoShipClass", "Pick a ship.");
	}
	if (!FindShipyard())
	{
		return LOCTEXT("NoShipyard", "No shipyard in this world.");
	}
	if (ShipyardQueue.Num() >= ShipyardQueueLimit)
	{
		return FText::Format(LOCTEXT("SlipwayFull", "The slipway is full: {0} ships queued."), FText::AsNumber(ShipyardQueueLimit));
	}
	FAPSShipyardJob& Job = ShipyardQueue.AddDefaulted_GetRef();
	Job.ShipClass = Option.ShipClass;
	Job.SizeClass = Option.SizeClass;
	Job.Name = Option.Name;
	Job.Length = FMath::Max(Option.BuildSeconds, 1.0f);
	++Revision;
	Post(FText::Format(LOCTEXT("ShipOrdered", "Shipyard: {0} (class {1}) laid down, about {2} s."), Option.Name,
		APSFleetPrivate::EnumText(Option.SizeClass), FText::AsNumber(FMath::RoundToInt(Job.Length))));
	return FText::GetEmpty();
}

void FAPSFleetCommand::TickShipyard(const float DeltaSeconds)
{
	if (ShipyardQueue.IsEmpty())
	{
		return;
	}
	FAPSShipyardJob& Job = ShipyardQueue[0];
	Job.Progress = FMath::Min(1.0f, Job.Progress + DeltaSeconds
		* FMath::Max(APSFleetPrivate::CVarBuildScale.GetValueOnGameThread(), 0.0f) / FMath::Max(Job.Length, 0.1f));
	if (Job.Progress < 1.0f)
	{
		return;
	}
	ASpaceShipyard* Shipyard = FindShipyard();
	if (!Shipyard)
	{
		return;
	}
	// Above the shipyard, four abreast, a row per four ships.
	const int32 Slot = LaunchedCount;
	constexpr double Spacing = 30000.0;
	const FVector Base = Shipyard->SpawnPoint ? Shipyard->SpawnPoint->GetComponentLocation() : Shipyard->GetActorLocation();
	const FVector Location = Base + Shipyard->GetActorRightVector() * ((Slot % 4) - 1.5) * Spacing
		+ Shipyard->GetActorUpVector() * (Slot / 4 + 1) * Spacing;
	const FAPSShipyardJob Launched = Job;
	ShipyardQueue.RemoveAt(0);
	ASpaceship* Ship = LaunchShip(Launched.ShipClass, FTransform(Shipyard->GetActorQuat(), Location));
	if (!Ship)
	{
		Post(FText::Format(LOCTEXT("LaunchFailed", "Shipyard: {0} could not be launched."), Launched.Name));
		return;
	}
	++LaunchedCount;
	RefreshUnits();
	const FAPSFleetUnit* Unit = FindUnit(Ship);
	Post(FText::Format(LOCTEXT("ShipLaunched", "Shipyard launched {0}: {1}, {2}."), FText::FromString(Unit ? Unit->CallSign : Ship->GetName()),
		Launched.Name, Unit ? APSFleet::DivisionName(Unit->Division) : FText::GetEmpty()));
}

ASpaceship* FAPSFleetCommand::LaunchShip(const TSubclassOf<ASpaceship> ShipClass, const FTransform& Transform)
{
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	ASpaceShipyard* Shipyard = FindShipyard();
	if (!LiveWorld || !ShipClass)
	{
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpaceship* Ship = LiveWorld->SpawnActor<ASpaceship>(ShipClass, Transform.GetLocation(), Transform.Rotator(), Parameters);
	if (!Ship)
	{
		return nullptr;
	}
	// As the generator readies an escort: the star system that moves the ship's frame, the civilization's tags.
	for (const FAPSFleetUnit& Unit : Units)
	{
		if (Unit.Ship.IsValid() && Unit.Ship->OffsetSystem)
		{
			Ship->OffsetSystem = Unit.Ship->OffsetSystem;
			break;
		}
	}
	Ship->Tags.AddUnique(TEXT("APS.GeneratedCivilization"));
	Ship->Tags.AddUnique(UnitTag);
	Ship->Tags.AddUnique(BuiltTag);
	if (Shipyard && FVector::Dist(Ship->GetActorLocation(), Shipyard->GetActorLocation()) < 2000000.0)
	{
		Ship->AttachToActor(Shipyard, FAttachmentTransformRules::KeepWorldTransform);
	}
	++Revision;
	return Ship;
}

// ---------------------------------------------------------------------------------------------------------------------
// Console

namespace APSFleetPrivate
{
	FAutoConsoleCommandWithWorld ListCommand(TEXT("aps.Fleet.List"), TEXT("Logs the fleet's units and known worlds."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (const FAPSFleetCommand* Fleet = APSFleetFind(World))
			{
				Fleet->LogUnits();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] no fleet command in this world"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs OrderCommand(TEXT("aps.Fleet.Order"),
		TEXT("aps.Fleet.Order <all|call sign|division> <move|survey|outpost|return|cancel> [target name, or moon/planet/station]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			FAPSFleetCommand* Fleet = APSFleetFind(World);
			if (!Fleet || Args.Num() < 2)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] usage: aps.Fleet.Order <who> <order> [target] (fleet %s)"),
					Fleet ? TEXT("ok") : TEXT("missing"));
				return;
			}
			const FString& Verb = Args[1];
			const APSFleet::EOrder Order = Verb.Equals(TEXT("move"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::Move
				: Verb.Equals(TEXT("survey"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::Survey
				: Verb.Equals(TEXT("outpost"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::BuildOutpost
				: Verb.Equals(TEXT("return"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::Return : APSFleet::EOrder::None;
			FText Refusal;
			const int32 Issued = Fleet->ConsoleOrder(Args[0], Order, Args.Num() > 2 ? Args[2] : FString(), Refusal);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] order %s %s -> %d ship(s)%s%s"), *Args[0], *Verb, Issued,
				Refusal.IsEmpty() ? TEXT("") : TEXT("; refused: "), *Refusal.ToString());
		}));

	/**
	 * aps.Fleet.AutoTest <label> [quit]: fleet command end to end in a generated game (a run starts it from the menu).
	 * Once the fleet has ships and the world has settled: exploration surveys the nearest moon (else a planet), science
	 * studies the nearest other planet, construction is refused at the moon for want of data; when the survey is in,
	 * construction builds an outpost there. The terminal is shot on fleet command every eight seconds
	 * (Saved/Screenshots/.../Fleet/<label>_NN.png); the run quits when the outpost stands, or after eight minutes.
	 */
	struct FAutoTest
	{
		bool bActive{false};
		bool bQuit{false};
		FString Label;
		int32 Stage{0};
		double Started{0.0};
		double StageSeconds{0.0};
		double NextShot{0.0};
		int32 Shot{0};
		TWeakObjectPtr<AActor> SurveyTarget;
		FTSTicker::FDelegateHandle Ticker;
	};
	FAutoTest GAutoTest;

	void AutoTestShot()
	{
		const FString File = FPaths::ScreenShotDir() / TEXT("Fleet") / FString::Printf(TEXT("%s_%02d.png"), *GAutoTest.Label,
			GAutoTest.Shot++);
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest shot %s"), *File);
	}

	bool TickAutoTest(float)
	{
		if (!GAutoTest.bActive || !GEngine)
		{
			return false;
		}
		const double Now = FPlatformTime::Seconds();
		const auto Finish = [](const TCHAR* Why)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest ended: %s"), Why);
			GAutoTest.bActive = false;
			if (GAutoTest.bQuit)
			{
				FPlatformMisc::RequestExit(false, TEXT("aps.Fleet.AutoTest"));
			}
			return false;
		};
		if (Now - GAutoTest.Started > 480.0)
		{
			return Finish(TEXT("timeout"));
		}
		UWorld* World = nullptr;
		FAPSFleetCommand* Fleet = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				if (FAPSFleetCommand* Found = APSFleetFind(Context.World()); Found && Found->GetUnits().Num() >= 2)
				{
					World = Context.World();
					Fleet = Found;
					break;
				}
			}
		}
		if (!Fleet)
		{
			return true;
		}
		FText Refusal;
		switch (GAutoTest.Stage)
		{
		case 0:
			if (GAutoTest.StageSeconds <= 0.0)
			{
				GAutoTest.StageSeconds = Now + 20.0;
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: %d units, settling 20 s"), Fleet->GetUnits().Num());
				return true;
			}
			if (Now < GAutoTest.StageSeconds)
			{
				return true;
			}
			Fleet->LogUnits();
			{
				int32 Issued = Fleet->ConsoleOrder(TEXT("exploration"), EOrder::Survey, TEXT("moon"), Refusal);
				if (Issued == 0)
				{
					Issued = Fleet->ConsoleOrder(TEXT("exploration"), EOrder::Survey, TEXT("planet"), Refusal);
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: exploration survey -> %d ship(s) %s"), Issued, *Refusal.ToString());
				for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
				{
					if (Unit.Division == EDivision::Exploration && Unit.Order == EOrder::Survey)
					{
						GAutoTest.SurveyTarget = Unit.Target;
						break;
					}
				}
				Issued = Fleet->ConsoleOrder(TEXT("science"), EOrder::Survey, TEXT("planet"), Refusal);
				if (Issued == 0)
				{
					// A system of one planet: science studies the moon in depth instead.
					Issued = Fleet->ConsoleOrder(TEXT("science"), EOrder::Survey, TEXT("moon"), Refusal);
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: science study -> %d ship(s) %s"), Issued, *Refusal.ToString());
				Issued = Fleet->ConsoleOrder(TEXT("construction"), EOrder::BuildOutpost, TEXT("moon"), Refusal);
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: construction before the survey -> %d ship(s), refused: %s"),
					Issued, *Refusal.ToString());
			}
			if (UAPSColonyTerminalSubsystem* Terminal = World->GetSubsystem<UAPSColonyTerminalSubsystem>();
				Terminal && !Terminal->IsTerminalOpen())
			{
				Terminal->ToggleFleetCommand();
			}
			GAutoTest.Stage = 1;
			GAutoTest.NextShot = Now + 3.0;
			break;
		case 1:
			if (!GAutoTest.SurveyTarget.IsValid())
			{
				return Finish(TEXT("no survey target"));
			}
			if (Fleet->GetSurvey(GAutoTest.SurveyTarget.Get()) != ESurvey::Unknown)
			{
				TArray<ASpaceship*> Builders;
				for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
				{
					if (Unit.Division == EDivision::Construction && Unit.Ship.IsValid())
					{
						Builders.Add(Unit.Ship.Get());
						break;
					}
				}
				const int32 Issued = Fleet->IssueOrder(Builders, EOrder::BuildOutpost, GAutoTest.SurveyTarget.Get(), Refusal);
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: surveyed, construction outpost -> %d ship(s) %s"), Issued,
					*Refusal.ToString());
				GAutoTest.Stage = Issued > 0 ? 2 : 3;
				GAutoTest.StageSeconds = Now + 6.0;
			}
			break;
		case 2:
			if (Fleet->CountOutposts(GAutoTest.SurveyTarget.Get()) > 0)
			{
				GAutoTest.Stage = 3;
				GAutoTest.StageSeconds = Now + 6.0;
			}
			break;
		default:
			if (Now >= GAutoTest.StageSeconds)
			{
				Fleet->LogUnits();
				AutoTestShot();
				return Finish(TEXT("outpost built"));
			}
			break;
		}
		if (GAutoTest.Stage > 0 && Now >= GAutoTest.NextShot)
		{
			GAutoTest.NextShot = Now + 8.0;
			AutoTestShot();
		}
		return true;
	}

	FAutoConsoleCommandWithWorldAndArgs AutoTestCommand(TEXT("aps.Fleet.AutoTest"),
		TEXT("aps.Fleet.AutoTest <label> [quit]: survey, study and build an outpost with the fleet, shooting fleet command."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			if (GAutoTest.bActive)
			{
				FTSTicker::GetCoreTicker().RemoveTicker(GAutoTest.Ticker);
			}
			GAutoTest = FAutoTest();
			GAutoTest.bActive = true;
			GAutoTest.Label = Args.Num() > 0 ? Args[0] : FString(TEXT("fleet"));
			GAutoTest.bQuit = Args.Num() > 1 && Args[1].Equals(TEXT("quit"), ESearchCase::IgnoreCase);
			GAutoTest.Started = FPlatformTime::Seconds();
			GAutoTest.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAutoTest), 0.25f);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest armed: %s%s"), *GAutoTest.Label, GAutoTest.bQuit ? TEXT(" (quit)") : TEXT(""));
		}));
}

#undef LOCTEXT_NAMESPACE
