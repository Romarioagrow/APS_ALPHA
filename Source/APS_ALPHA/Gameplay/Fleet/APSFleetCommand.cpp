#include "APSFleetCommand.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "APSShipPlacement.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Gameplay/Megastructures/APSMegastructures.h"
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
	TAutoConsoleVariable<int32> CVarHoldLaunchInFlight(TEXT("aps.Fleet.HoldLaunchInFlight"), 1,
		TEXT("1: a finished slipway job waits at 100% while the world flows past the player's fast ship or its travel is ")
		TEXT("owed to the sky, and launches once that has been calm for aps.Fleet.HoldLaunchCalmSeconds (the hull spawn froze ")
		TEXT("single frames mid-flight). 0 restores the launch on the very frame the job finishes."));
	TAutoConsoleVariable<float> CVarHoldLaunchCalmSeconds(TEXT("aps.Fleet.HoldLaunchCalmSeconds"), 1.0f,
		TEXT("Seconds without a world flow before a held slipway launch goes ahead (aps.Fleet.HoldLaunchInFlight)."));
	/** Rio 06.10 (audit: galaxy-reach systems register ~1-3 s after the load, so their orders were cleared on the first tick). */
	TAutoConsoleVariable<int32> CVarRestoreWaitForSystems(TEXT("aps.Save.RestoreWaitForSystems"), 1,
		TEXT("1: the fleet restore waits (at most the existing 20 s) until the star registry is read and every SYSTEM: key in the ")
		TEXT("saved units resolves, so orders to star systems and ships referenced to them are not cleared or skipped on the first ")
		TEXT("tick after load; a save taken while it waits writes the loaded fleet data, not the half-restored fleet. 0: as before."));
	/** Rio 06.10 (flight FPS): per fleet command, the shipyards whose finished job waits for a calm flight and since when
	 * (world seconds), so the hold is logged once per job and the launch says how long it waited. */
	TMap<const FAPSFleetCommand*, TMap<TWeakObjectPtr<ASpaceShipyard>, double>>& HeldLaunches()
	{
		static TMap<const FAPSFleetCommand*, TMap<TWeakObjectPtr<ASpaceShipyard>, double>> Held;
		return Held;
	}
	const FName BuiltTag(TEXT("APS.Fleet.Built"));
	const FName CivilizationTag(TEXT("APS.GeneratedCivilization"));
	/** Stations, shipyards and HQs the construction ships raised (the generated home complex has no such tag). */
	const FName StructureTag(TEXT("APS.Fleet.Structure"));

	/** "a station" for the journal's sentences; StructureName is the upper-case label. */
	FText StructureNoun(const EStructure Structure)
	{
		switch (Structure)
		{
		case EStructure::Shipyard: return LOCTEXT("NounShipyard", "a shipyard");
		case EStructure::Headquarters: return LOCTEXT("NounHeadquarters", "a sector headquarters");
		default: return LOCTEXT("NounStation", "a station");
		}
	}

	/** What the structure gives, said once it stands. */
	FText StructureEffect(const EStructure Structure, const FText& Body)
	{
		switch (Structure)
		{
		case EStructure::Shipyard: return LOCTEXT("EffectShipyard", "Its slipway is open in the SHIPYARD tab.");
		case EStructure::Headquarters: return LOCTEXT("EffectHeadquarters", "The whole fleet flies 10% faster.");
		default: return FText::Format(LOCTEXT("EffectStation", "Work at {0} now goes 25% faster."), Body);
		}
	}

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
		// Ground vehicles (rover, hover, drone; Rio 02.10) are the colony's, never fleet units or orders.
		return IsValid(Ship) && !Ship->IsGroundVehicle() && (Ship->ActorHasTag(HomeShipTag) || Ship->ActorHasTag(EscortTag)
			|| Ship->ActorHasTag(MaterializedTag) || Ship->ActorHasTag(UnitTag));
	}

	/** Rio 07.10: held structures and outposts raised per second once their world stands again (a station is a blueprint). */
	constexpr int32 HeldStructureRaisesPerSecond = 2;
	constexpr int32 HeldOutpostRaisesPerSecond = 4;

	/**
	 * Rio 07.10: the foreign star system the materializer stands up now (invalid when none) and, when OutBodies is given,
	 * its own planets and moons by fleet key: what that system held when it was released stands again only on these, never
	 * on a body of the same name elsewhere (names are syllable words; a home world may share one).
	 */
	FGuid FleetHostBodies(const UWorld* World, TMap<FString, APlanetaryBody*>* OutBodies)
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		const FAPSSystemMaterializer* Materializer = Stars ? Stars->GetMaterializer() : nullptr;
		const FAPSStarSystemInfo* Info = Materializer ? Stars->Get(Materializer->GetActiveIndex()) : nullptr;
		if (!Info || Info->bHome)
		{
			return FGuid();
		}
		if (OutBodies)
		{
			TArray<APlanet*> Planets;
			Materializer->GetPlanets(Planets);
			for (APlanet* Planet : Planets)
			{
				const FString PlanetKey = FAPSFleetCommand::KeyOf(Planet);
				if (!OutBodies->Contains(PlanetKey))
				{
					OutBodies->Add(PlanetKey, Planet);
				}
				for (AMoon* Moon : Planet->Moons)
				{
					const FString MoonKey = IsValid(Moon) ? FAPSFleetCommand::KeyOf(Moon) : FString();
					if (!MoonKey.IsEmpty() && !OutBodies->Contains(MoonKey))
					{
						OutBodies->Add(MoonKey, Moon);
					}
				}
			}
		}
		return Info->Id;
	}

	/**
	 * Rio 07.10 (review: a ship saved by the beacon of a galaxy system without a saved state was lost at load, that system not
	 * being registered again, and held the fleet restore for its whole wait): a load registers this star system again by
	 * itself (a cluster system with the generated world, a galaxy system by its saved state).
	 */
	bool SystemReturnsAtLoad(const UWorld* World, const FGuid& Id)
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		const FAPSStarSystemInfo* Info = Stars ? Stars->Find(Id) : nullptr;
		if (!Info)
		{
			return false;
		}
		if (Info->GalaxyIndex == INDEX_NONE)
		{
			return true;
		}
		const FAPSStarSystemState State = Stars->GetState(Id);
		return State.Knowledge != APSStars::EKnowledge::Catalogued || State.bClaimed || !State.Structures.IsEmpty();
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
		if (FGuid SystemId; FAPSStarSystems::AnchorSystem(Actor, SystemId))
		{
			const FAPSStarSystems* Stars = APSStarSystemsFind(Actor->GetWorld());
			const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
			return Info ? FText::FromString(Info->Name) : FText::FromString(TEXT("STAR SYSTEM"));
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

	/** Rio 05.10 (star map): a distance as the maps write it: km near the worlds, AU between the stars, light years beyond. */
	FText DistanceText(const double Cm)
	{
		constexpr double LightYearCm = 9.4607304725808e17;
		FNumberFormattingOptions Two;
		Two.SetMaximumFractionalDigits(2);
		if (Cm >= 0.1 * LightYearCm)
		{
			return FText::Format(LOCTEXT("TransitLy", "{0} LY"), APSUINumber::Number(Cm / LightYearCm, &Two));
		}
		if (Cm >= 0.01 * APSStars::AstronomicalUnitCm)
		{
			return FText::Format(LOCTEXT("TransitAu", "{0} AU"), APSUINumber::Number(Cm / APSStars::AstronomicalUnitCm, &Two));
		}
		return FText::Format(LOCTEXT("TransitKm", "{0} KM"), APSUINumber::Number(FMath::RoundToInt64(Cm / 100000.0)));
	}

	/**
	 * Rio 05.10 (star map): seconds to fly DistanceCm from Speed by StepSpeed's profile: the speed grows 2.5 times a second
	 * up to the cap, then 0.8 of the way left each second closes in until 100 m are left (Arrive). Departure and the
	 * near-body limit are left out: an estimate for the screens.
	 */
	double FlightSeconds(const double DistanceCm, const double SpeedCm, const double CapCm)
	{
		constexpr double Approach = 0.8;
		constexpr double ArriveCm = 10000.0;
		const double LnGain = FMath::Loge(2.5);
		if (DistanceCm <= ArriveCm)
		{
			return 0.0;
		}
		const double Cap = FMath::Max(CapCm, 5000.0);
		// From rest StepSpeed first adds 50 m/s a second, then the growth takes over.
		const double Start = FMath::Clamp(SpeedCm, 5000.0, Cap);
		if (Start >= Approach * DistanceCm)
		{
			// Already as fast as the approach allows: only the closing in is left.
			return FMath::Loge(DistanceCm / ArriveCm) / Approach;
		}
		const double RampCm = (Cap - Start) / LnGain;
		const double ApproachCm = Cap / Approach;
		if (RampCm + ApproachCm <= DistanceCm)
		{
			return FMath::Loge(Cap / Start) / LnGain + (DistanceCm - RampCm - ApproachCm) / Cap
				+ FMath::Loge(ApproachCm / ArriveCm) / Approach;
		}
		// The cap is never reached: the growth meets the approach at Peak, (Peak - Start) / ln 2.5 + Peak / 0.8 = Distance.
		const double Peak = FMath::Clamp((DistanceCm + Start / LnGain) / (1.0 / LnGain + 1.0 / Approach), Start, Cap);
		return FMath::Loge(Peak / Start) / LnGain + FMath::Loge(FMath::Max(Peak / Approach, ArriveCm) / ArriveCm) / Approach;
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
	case EOrder::BuildStation: return LOCTEXT("OrderStation", "BUILD STATION");
	case EOrder::BuildShipyard: return LOCTEXT("OrderShipyard", "BUILD SHIPYARD");
	case EOrder::BuildHeadquarters: return LOCTEXT("OrderHeadquarters", "BUILD SECTOR HQ");
	case EOrder::Expedition: return LOCTEXT("OrderExpedition", "EXPEDITION");
	case EOrder::Probe: return LOCTEXT("OrderProbe", "PROBE");
	case EOrder::SurveySystem: return LOCTEXT("OrderSurveySystem", "SURVEY SYSTEM");
	case EOrder::BuildStructure: return LOCTEXT("OrderBuildStructure", "BUILD");
	default: return LOCTEXT("OrderNone", "NO ORDERS");
	}
}

FText APSFleet::StructureName(const EStructure Structure)
{
	switch (Structure)
	{
	case EStructure::Station: return LOCTEXT("StructureStation", "STATION");
	case EStructure::Shipyard: return LOCTEXT("StructureShipyard", "SHIPYARD");
	case EStructure::Headquarters: return LOCTEXT("StructureHeadquarters", "SECTOR HQ");
	default: return FText::GetEmpty();
	}
}

bool APSFleet::StructureOf(const EOrder Order, EStructure& OutStructure)
{
	switch (Order)
	{
	case EOrder::BuildStation: OutStructure = EStructure::Station; return true;
	case EOrder::BuildShipyard: OutStructure = EStructure::Shipyard; return true;
	case EOrder::BuildHeadquarters: OutStructure = EStructure::Headquarters; return true;
	default: return false;
	}
}

bool APSFleet::NeedsStation(const EStructure Structure)
{
	return Structure != EStructure::Station;
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
	case EOrder::Expedition:
	case EOrder::Probe:
	case EOrder::SurveySystem:
		return Division == EDivision::Exploration || Division == EDivision::Science;
	case EOrder::BuildStructure:
		return Division == EDivision::Construction;
	case EOrder::BuildOutpost:
	case EOrder::BuildStation:
	case EOrder::BuildShipyard:
	case EOrder::BuildHeadquarters:
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

double APSFleet::WorkSeconds(const EOrder Order, const EDivision Division, const int32 DivisionLevel,
	const bool bStationThere)
{
	double Base = Division == EDivision::Science ? 30.0 : 20.0;
	switch (Order)
	{
	case EOrder::BuildOutpost: Base = 36.0; break;
	case EOrder::BuildStation: Base = 60.0; break;
	case EOrder::BuildShipyard: Base = 75.0; break;
	case EOrder::BuildHeadquarters: Base = 90.0; break;
	case EOrder::Expedition: Base = 45.0; break;
	case EOrder::Probe: Base = 8.0; break;
	case EOrder::SurveySystem: Base = Division == EDivision::Science ? 30.0 : 25.0; break;
	default: break;
	}
	return Base / (1.0 + 0.2 * FMath::Max(DivisionLevel, 0)) / (bStationThere ? 1.25 : 1.0);
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

FVector APSFleet::RoundBody(const FVector& Ship, const FVector& Slot, const FVector& Centre, const double Radius)
{
	const FVector FromCentre = Ship - Centre;
	const double Distance = FromCentre.Size();
	if (Radius <= 0.0 || Distance < 1.0)
	{
		return Slot;
	}
	const FVector Up = FromCentre / Distance;
	if (Distance < Radius * 1.3)
	{
		// Climb clear first: from a berth on or near the surface every way round starts by going up.
		return Centre + Up * (Radius * 1.4);
	}
	const FVector ToSlot = (Slot - Centre).GetSafeNormal();
	const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Up, ToSlot), -1.0, 1.0));
	FVector Axis = FVector::CrossProduct(Up, ToSlot).GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		// The slot straight behind the body: any way round will do.
		Axis = FVector::CrossProduct(Up, FMath::Abs(Up.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
	}
	const double Step = FMath::Min(Angle, FMath::DegreesToRadians(30.0));
	return Centre + Up.RotateAngleAxisRad(Step, Axis) * FMath::Max(Distance, Radius * 1.35);
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

FAPSFleetCommand::~FAPSFleetCommand()
{
	// Rio 06.10: a later fleet command at the same address must not inherit this one's held launches.
	APSFleetPrivate::HeldLaunches().Remove(this);
}

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
	int32 Base = 1;
	if (Civ)
	{
		switch (Division)
		{
		case APSFleet::EDivision::MainFleet: Base = Civ->Divisions.Military; break;
		case APSFleet::EDivision::Exploration: Base = Civ->Divisions.Exploration; break;
		case APSFleet::EDivision::Science: Base = Civ->Divisions.Science; break;
		case APSFleet::EDivision::Construction: Base = Civ->Divisions.Industry; break;
		default: break;
		}
	}
	return Base + GetEarnedLevel(Division);
}

int32 FAPSFleetCommand::GetDivisionLevel(const APSFleet::EDivision Division) const
{
	return DivisionLevel(Division);
}

FAPSFleetCommand::FWorkTally FAPSFleetCommand::GetWorkTally() const
{
	FWorkTally Tally;
	for (const FAPSFleetBodyRecord& Record : Bodies)
	{
		Tally.Surveyed += Record.Survey >= APSFleet::ESurvey::Surveyed ? 1 : 0;
		Tally.Studied += Record.Survey == APSFleet::ESurvey::Studied ? 1 : 0;
		Tally.Built += CountOutposts(Record.Body.Get());
		// Rio 07.10 (review): a record kept from a save for a world away at load has not rolled its anomaly yet; its saved
		// investigation counts until it does (RebindBody applies it and clears HeldInvestigation, 0 for every other record).
		Tally.Investigated += Record.bHasAnomaly && Record.Anomaly == APSFleet::EAnomalyState::Investigated
			? (Record.bAnomalyInPerson ? 2 : 1) : Record.HeldInvestigation;
	}
	// The home world is known from the start: nobody surveyed it.
	if (bHomeKnown)
	{
		Tally.Surveyed = FMath::Max(Tally.Surveyed - 1, 0);
		Tally.Studied = FMath::Max(Tally.Studied - 1, 0);
	}
	for (const FAPSFleetStructure& Structure : Structures)
	{
		Tally.Built += Structure.bBuilt && Structure.Actor.IsValid() ? 1 : 0;
	}
	// Rio 07.10: what the fleet built on worlds that are away (a released star system) still counts: the levels it earned
	// do not fall while the pilot is elsewhere (aps.Stars.HoldReleasedStructures).
	Tally.Built += HeldStructures.Num() + HeldOutposts.Num();
	for (const FAPSFleetUnit& Unit : Units)
	{
		Tally.Launched += Unit.Ship.IsValid() && Unit.Ship->ActorHasTag(APSFleetPrivate::BuiltTag) ? 1 : 0;
	}
	return Tally;
}

int32 FAPSFleetCommand::GetEarnedLevel(const APSFleet::EDivision Division) const
{
	const FWorkTally Tally = GetWorkTally();
	switch (Division)
	{
	case APSFleet::EDivision::Exploration: return FMath::Clamp(Tally.Surveyed / 3, 0, 3);
	case APSFleet::EDivision::Science: return FMath::Clamp((Tally.Studied + Tally.Investigated) / 2, 0, 3);
	case APSFleet::EDivision::Construction: return FMath::Clamp(Tally.Built / 3, 0, 3);
	default: return 0;
	}
}

int32 FAPSFleetCommand::GetEarnedFleetCommandLevel() const
{
	return FMath::Clamp(GetWorkTally().Launched / 4, 0, 3);
}

double FAPSFleetCommand::GetDivisionSpeedFactor(const APSFleet::EDivision Division) const
{
	// The line answers first: each military level speeds the main fleet's ships up by 15%.
	return Division == APSFleet::EDivision::MainFleet ? 1.0 + 0.15 * FMath::Max(DivisionLevel(Division), 0) : 1.0;
}

float FAPSFleetCommand::GetShipyardRate() const
{
	return 1.0f + 0.2f * FMath::Max(DivisionLevel(APSFleet::EDivision::Construction), 0);
}

void FAPSFleetCommand::AnnouncePromotions()
{
	using namespace APSFleet;
	const int32 Earned[4] = {GetEarnedLevel(EDivision::Exploration), GetEarnedLevel(EDivision::Science),
		GetEarnedLevel(EDivision::Construction), GetEarnedFleetCommandLevel()};
	if (!bEarnedPrimed)
	{
		FMemory::Memcpy(AnnouncedEarned, Earned, sizeof(Earned));
		bEarnedPrimed = true;
		return;
	}
	const UCivilization* Civ = Civilization();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (Earned[Index] > AnnouncedEarned[Index])
		{
			const FText Name = Index == 0 ? LOCTEXT("PromoteExploration", "Exploration")
				: Index == 1 ? LOCTEXT("PromoteScience", "Science")
				: Index == 2 ? LOCTEXT("PromoteIndustry", "Industry") : LOCTEXT("PromoteFleetCommand", "Fleet command");
			const int32 Level = Index == 0 ? DivisionLevel(EDivision::Exploration)
				: Index == 1 ? DivisionLevel(EDivision::Science)
				: Index == 2 ? DivisionLevel(EDivision::Construction)
				: (Civ ? Civ->Divisions.FleetCommand : 0) + Earned[Index];
			Post(FText::Format(LOCTEXT("Promoted", "{0} division learned from its work: level {1} (+{2} earned). See DIVISIONS."),
				Name, APSUINumber::Number(Level), APSUINumber::Number(Earned[Index])));
		}
		AnnouncedEarned[Index] = Earned[Index];
	}
}

double FAPSFleetCommand::SpeedScale() const
{
	const UCivilization* Civ = Civilization();
	// Each Fleet Command level (the civilization's and the earned) and each sector HQ the fleet built: 10%.
	return FMath::Max(APSFleetPrivate::CVarSpeedScale.GetValueOnGameThread(), 0.01f)
		* (1.0 + 0.1 * ((Civ ? FMath::Max(Civ->Divisions.FleetCommand, 0) : 0) + GetEarnedFleetCommandLevel()
			+ CountBuiltHeadquarters()));
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
	// Rio 07.10 (aps.Stars.HoldReleasedStructures: a revisited world got a second record, Unknown again): the record kept
	// while this world was away is its record again; one a released system kept, only for that system's own world.
	if (Body && FAPSInfrastructure::HoldsReleased() && Bodies.ContainsByPredicate([](const FAPSFleetBodyRecord& Entry)
		{
			return !Entry.Body.IsValid() && !Entry.Key.IsEmpty();
		}))
	{
		const FString Key = KeyOf(Body);
		TMap<FString, APlanetaryBody*> HostBodies;
		const FGuid ActiveHost = APSFleetPrivate::FleetHostBodies(World.Get(), &HostBodies);
		const FGuid Host = HostBodies.FindRef(Key) == Body ? ActiveHost : FGuid();
		if (FAPSFleetBodyRecord* Kept = Bodies.FindByPredicate([&Key, &Host](const FAPSFleetBodyRecord& Entry)
			{
				return !Entry.Body.IsValid() && Entry.Key == Key && (!Entry.HostSystemId.IsValid() || Entry.HostSystemId == Host);
			}))
		{
			RebindBody(*Kept, Body);
			return *Kept;
		}
	}
	FAPSFleetBodyRecord& Record = Bodies.AddDefaulted_GetRef();
	Record.Body = Body;
	RollAnomaly(Record, Body);
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
		const double Surface = FVector::Distance(Location, UAPSWorldOriginSubsystem::WorldPlace(**It)) - Radius;
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
			const double Ratio = FVector::Distance(Location, UAPSWorldOriginSubsystem::WorldPlace(**It))
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
	RefreshStructures();
	if (PendingRestore.IsSet())
	{
		ApplyPendingRestore();
	}
	if (!PendingRestore.IsSet())
	{
		// Rio 07.10: what waited for its world to stand again (aps.Stars.HoldReleasedStructures; nothing when none waits).
		RaiseHeldWorlds();
		AnnouncePromotions();
		TickAnomalies();
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
				// Anomalies are what ships find: the home world, known from the start, hides none.
				Record.bHasAnomaly = false;
				bHomeKnown = true;
				++Revision;
			}
		}
	}
}

FText FAPSFleetCommand::CheckOrder(const ASpaceship* Ship, const APSFleet::EOrder Order, const AActor* Target,
	const bool bByPilot) const
{
	using namespace APSFleet;
	const FAPSFleetUnit* Unit = FindUnit(Ship);
	if (!Unit)
	{
		return LOCTEXT("NotOurs", "Not a ship of the civilization.");
	}
	if (Ship->HasPilot() && !bByPilot)
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
	EStructure Structure = EStructure::Station;
	const bool bStructure = StructureOf(Order, Structure);
	if (!DivisionCan(Unit->Division, Order))
	{
		return Order == EOrder::Survey ? LOCTEXT("SurveyDivision", "Surveys need an exploration or science ship.")
			: Order == EOrder::Expedition ? LOCTEXT("ExpeditionDivision", "Expeditions need an exploration or science ship.")
			: bStructure ? LOCTEXT("BuildDivision", "Building needs a construction ship.")
			: LOCTEXT("OutpostDivision", "Outposts need a construction ship.");
	}
	FGuid SystemId;
	const bool bSystem = FAPSInfrastructure::SiteSystem(World.Get(), Target, SystemId) && !Target->IsA<APlanetaryBody>();
	if (Order == EOrder::BuildStructure)
	{
		return LOCTEXT("BuildNeedsType", "Pick what to build from the construction catalogue.");
	}
	if (Order == EOrder::Probe || Order == EOrder::SurveySystem || ((Order == EOrder::Move || Order == EOrder::Expedition) && bSystem))
	{
		if (!bSystem)
		{
			return LOCTEXT("SystemTarget", "Pick a star system.");
		}
		if (!Ship->ActiveClassPreset.bSupportsSpaceWrap && !bByPilot)
		{
			return FText::Format(LOCTEXT("NoWrapStars", "Class {0} has no SpaceWrap: it cannot reach other stars."),
				FText::FromString(Ship->GetSizeClassName()));
		}
		const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		const APSStars::EKnowledge Known = Stars ? Stars->GetKnowledge(SystemId) : APSStars::EKnowledge::Catalogued;
		if (Order == EOrder::Probe && Known >= APSStars::EKnowledge::Scanned)
		{
			return LOCTEXT("AlreadyScanned", "Already scanned: a survey of the system charts it further.");
		}
		if (Order == EOrder::SurveySystem && Known >= APSStars::EKnowledge::Surveyed)
		{
			return LOCTEXT("SystemAlreadySurveyed", "This star system is surveyed already.");
		}
		if (Order == EOrder::Expedition && (!Stars || Stars->AnomalyKindOf(SystemId) == INDEX_NONE || Stars->GetState(SystemId).Anomaly != 2))
		{
			return LOCTEXT("SystemNoLocatedAnomaly", "No located anomaly here: a scan detects one, a survey of the system locates it.");
		}
		if (Unit->Target.Get() == Target && Unit->Order == Order)
		{
			return LOCTEXT("SameOrder", "Already under this order.");
		}
		return FText::GetEmpty();
	}
	const bool bBody = Target->IsA<APlanetaryBody>();
	if (Order == EOrder::Move && !bBody && !Target->IsA<ATechActor>())
	{
		return LOCTEXT("MoveTarget", "Ships fly to planets, moons, stations and outposts.");
	}
	if ((Order == EOrder::Survey || Order == EOrder::BuildOutpost || Order == EOrder::Expedition || bStructure) && !bBody)
	{
		return LOCTEXT("BodyTarget", "Pick a planet or a moon.");
	}
	if (Order == EOrder::Expedition)
	{
		const FAPSFleetBodyRecord* Record = FindBody(Target);
		if (!Record || !Record->bHasAnomaly || Record->Anomaly < EAnomalyState::Located)
		{
			return Record && Record->bHasAnomaly && Record->Anomaly == EAnomalyState::Detected
				? LOCTEXT("ExpeditionNeedsSite", "The anomaly's site is unknown: a science ship's study locates it.")
				: LOCTEXT("ExpeditionNoAnomaly", "No anomaly known here: surveys find them, studies locate them.");
		}
		if (Record->Anomaly == EAnomalyState::Investigated)
		{
			return LOCTEXT("ExpeditionDone", "Its anomaly is investigated already.");
		}
		for (const FAPSFleetUnit& Other : Units)
		{
			if (&Other != Unit && Other.Order == Order && Other.Target.Get() == Target)
			{
				return FText::Format(LOCTEXT("ExpeditionUnderWay", "{0} is on that expedition already."), UnitName(Other));
			}
		}
	}
	if (bStructure)
	{
		// An outpost is the foothold, a station the base for the rest (Rio, 01.10: "after an outpost, a station, a HQ").
		const int32 Stations = CountStructures(Target, EStructure::Station);
		if (GetSurvey(Target) == ESurvey::Unknown)
		{
			return LOCTEXT("StructureNeedsSurvey", "No data on this world: survey it first.");
		}
		if (CountOutposts(Target) == 0 && Stations == 0)
		{
			return LOCTEXT("NeedOutpost", "Build an outpost there first: it is the foothold.");
		}
		if (NeedsStation(Structure) && Stations == 0)
		{
			return LOCTEXT("NeedStation", "Build a station there first.");
		}
		// The limit counts the fleet's own structures: a hub over the world is not one of its two stations.
		int32 FleetOwn = 0;
		for (const FAPSFleetStructure& Standing : Structures)
		{
			FleetOwn += Standing.Kind == Structure && Standing.Actor.IsValid() && Standing.Body.Get() == Target ? 1 : 0;
		}
		if (FleetOwn >= (Structure == EStructure::Station ? 2 : 1))
		{
			return Structure == EStructure::Station ? LOCTEXT("StationLimit", "Two stations already orbit it.")
				: Structure == EStructure::Shipyard ? LOCTEXT("ShipyardLimit", "A shipyard already orbits it.")
				: LOCTEXT("HeadquartersLimit", "A headquarters already orbits it.");
		}
		for (const FAPSFleetUnit& Other : Units)
		{
			if (&Other != Unit && Other.Order == Order && Other.Target.Get() == Target)
			{
				return FText::Format(LOCTEXT("AlreadyBuilding", "{0} is already building it there."), UnitName(Other));
			}
		}
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
	if (!bByPilot && !Ship->ActiveClassPreset.bSupportsSpaceWrap
		&& HomeBodyOf(Target->GetActorLocation()) != HomeBodyOf(Ship->GetActorLocation()))
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

bool FAPSFleetCommand::PilotWorkRange(const ASpaceship* Ship, const APSFleet::EOrder Order, const AActor* Target,
	double& OutDistanceCm, double& OutRangeCm) const
{
	using namespace APSFleet;
	if (!Ship || !IsValid(Target))
	{
		return false;
	}
	OutDistanceCm = FVector::Dist(Ship->GetActorLocation(), UAPSWorldOriginSubsystem::WorldPlace(*Target));
	if (Target->IsA<APlanetaryBody>())
	{
		// A survey reads the world from a near approach (four of its radii and 20,000 km); building and an expedition's
		// landing need a near orbit (twice the crews' slot), and what is built rises beside the ship.
		const double Radius = APSFleetPrivate::BodyRadiusCm(Target);
		OutRangeCm = Order == EOrder::Survey ? Radius * 4.0 + 2.0e9 : SlotRadius(Radius) * 2.0;
		return true;
	}
	FGuid SystemId;
	if (FAPSInfrastructure::SiteSystem(World.Get(), Target, SystemId))
	{
		// A star system: anywhere inside its room among its neighbours.
		const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
		if (Info)
		{
			OutDistanceCm = FVector::Dist(Ship->GetActorLocation(), Info->Location - UAPSWorldOriginSubsystem::SkyOffsetOf(World.Get()));
		}
		OutRangeCm = FMath::Max(Info ? Info->RoomCm : 0.0, APSStars::AstronomicalUnitCm);
		return true;
	}
	// A station, an outpost: alongside, within 100 km.
	OutRangeCm = 1.0e7;
	return true;
}

FText FAPSFleetCommand::CheckPilotOrder(const ASpaceship* Ship, const APSFleet::EOrder Order, const AActor* Target,
	const FName StructureType) const
{
	using namespace APSFleet;
	if (!Ship || !Ship->HasPilot())
	{
		return LOCTEXT("PilotNotAboard", "Take the helm of the ship first.");
	}
	if (Order == EOrder::Move || Order == EOrder::Return || Order == EOrder::None)
	{
		return LOCTEXT("PilotFliesHimself", "You fly there yourself.");
	}
	if (const FAPSFleetUnit* Unit = FindUnit(Ship); Unit && Unit->bPilotWork && Unit->Order != EOrder::None)
	{
		return FText::Format(LOCTEXT("PilotBusy", "Your ship is at work already: {0}. Flying far off stops it."),
			DescribeState(*Unit));
	}
	const FText Refusal = Order == EOrder::BuildStructure ? CheckBuildOrder(Ship, Target, StructureType, true)
		: CheckOrder(Ship, Order, Target, true);
	if (!Refusal.IsEmpty())
	{
		return Refusal;
	}
	double Distance = 0.0;
	double Range = 0.0;
	if (PilotWorkRange(Ship, Order, Target, Distance, Range) && Distance > Range)
	{
		const auto Text = [](const double Cm)
		{
			FNumberFormattingOptions Two;
			Two.SetMaximumFractionalDigits(2);
			return Cm >= 0.01 * APSStars::AstronomicalUnitCm
				? FText::Format(LOCTEXT("PilotAu", "{0} AU"), APSUINumber::Number(Cm / APSStars::AstronomicalUnitCm, &Two))
				: FText::Format(LOCTEXT("PilotKm", "{0} km"), APSUINumber::Number(FMath::RoundToInt64(Cm / 100000.0)));
		};
		return FText::Format(LOCTEXT("PilotTooFar", "Fly within {0} of it first (now {1})."), Text(Range), Text(Distance));
	}
	return FText::GetEmpty();
}

FText FAPSFleetCommand::IssuePilotOrder(ASpaceship* Ship, const APSFleet::EOrder Order, AActor* Target, const FName StructureType)
{
	using namespace APSFleet;
	FText Refusal = CheckPilotOrder(Ship, Order, Target, StructureType);
	FAPSFleetUnit* Unit = Refusal.IsEmpty() ? FindUnitMutable(Ship) : nullptr;
	if (Refusal.IsEmpty() && !Unit)
	{
		Refusal = LOCTEXT("NotOurs", "Not a ship of the civilization.");
	}
	if (Refusal.IsEmpty() && Order == EOrder::BuildStructure)
	{
		FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
		if (!Infrastructure || !Infrastructure->Reserve(StructureType))
		{
			Refusal = LOCTEXT("CostNotTaken", "The stocks do not cover it.");
		}
	}
	if (!Refusal.IsEmpty())
	{
		return Refusal;
	}
	// The same order as a crew would take, flown by the pilot: no slot, no autopilot, the work starts at once.
	Unit->Order = Order;
	Unit->StructureType = Order == EOrder::BuildStructure ? StructureType : NAME_None;
	Unit->Target = Target;
	Unit->bPilotWork = true;
	Unit->SlotDirection = (Ship->GetActorLocation() - UAPSWorldOriginSubsystem::WorldPlace(*Target)).GetSafeNormal();
	if (Unit->SlotDirection.IsNearlyZero()) Unit->SlotDirection = FVector::UpVector;
	Unit->Speed = 0.0;
	Unit->RemainingCm = 0.0;
	Unit->TransitStartCm = 0.0;
	BeginWork(*Unit);
	return FText::GetEmpty();
}

void FAPSFleetCommand::TickPilotWork(FAPSFleetUnit& Unit, const float DeltaSeconds)
{
	using namespace APSFleet;
	ASpaceship* Ship = Unit.Ship.Get();
	AActor* Target = Unit.Target.Get();
	const auto End = [this, &Unit](const FText& Why)
	{
		if (Unit.Order == EOrder::BuildStructure && Unit.Phase == EPhase::Working)
		{
			if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get())) Infrastructure->Refund(Unit.StructureType);
		}
		if (!Why.IsEmpty()) Post(FText::Format(LOCTEXT("PilotWorkEnded", "{0}: {1}"), UnitName(Unit), Why));
		Unit.bPilotWork = false;
		Unit.StructureType = NAME_None;
		Unit.Order = EOrder::None;
		Unit.Phase = EPhase::Idle;
		Unit.Target = nullptr;
		Unit.Progress = 0.0f;
		++Revision;
	};
	if (!Ship || !IsValid(Target) || Unit.Phase != EPhase::Working)
	{
		End(FText::GetEmpty());
		return;
	}
	// The ship's own instruments do the work: it goes on while the pilot walks the decks, as long as the ship stays.
	double Distance = 0.0;
	double Range = 0.0;
	PilotWorkRange(Ship, Unit.Order, Target, Distance, Range);
	if (Distance > Range * 3.0)
	{
		End(LOCTEXT("PilotFlewOff", "flew too far away, the work stopped."));
		return;
	}
	// Out of reach the work waits (the ship is coming back); within it, it goes on as a crew's does.
	if (Distance <= Range)
	{
		Unit.Progress = FMath::Min(1.0f, Unit.Progress + DeltaSeconds
			* FMath::Max(APSFleetPrivate::CVarWorkScale.GetValueOnGameThread(), 0.0f) / FMath::Max(Unit.WorkLength, 0.1f));
	}
	if (Unit.Progress >= 1.0f)
	{
		FinishWork(Unit);
		// The pilot keeps the ship: no holding at a slot, no return to a berth.
		Unit.bPilotWork = false;
		Unit.StructureType = NAME_None;
		Unit.Order = EOrder::None;
		Unit.Phase = EPhase::Idle;
		Unit.Target = nullptr;
		++Revision;
	}
}

FText FAPSFleetCommand::CheckBuildOrder(const ASpaceship* Ship, const AActor* Target, const FName StructureType,
	const bool bByPilot) const
{
	using namespace APSFleet;
	const FAPSFleetUnit* Unit = FindUnit(Ship);
	if (!Unit)
	{
		return LOCTEXT("NotOurs", "Not a ship of the civilization.");
	}
	if (Ship->HasPilot() && !bByPilot)
	{
		return LOCTEXT("Piloted", "Someone is at the helm: orders go to crewless ships.");
	}
	if (!IsValid(Target))
	{
		return LOCTEXT("PickTarget", "Pick a target on the map.");
	}
	if (!DivisionCan(Unit->Division, EOrder::BuildStructure))
	{
		return LOCTEXT("BuildDivision", "Building needs a construction ship.");
	}
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
	if (!Infrastructure)
	{
		return LOCTEXT("NoInfrastructure", "No civilization infrastructure in this world.");
	}
	if (const FText Refusal = Infrastructure->CheckBuild(StructureType, Target); !Refusal.IsEmpty())
	{
		return Refusal;
	}
	FGuid SystemId;
	const bool bSystem = FAPSInfrastructure::SiteSystem(World.Get(), Target, SystemId) && !Target->IsA<APlanetaryBody>();
	if (!bByPilot && !Ship->ActiveClassPreset.bSupportsSpaceWrap
		&& (bSystem || HomeBodyOf(Target->GetActorLocation()) != HomeBodyOf(Ship->GetActorLocation())))
	{
		return FText::Format(LOCTEXT("NoWrap", "Class {0} has no SpaceWrap: it only flies near its own planet."),
			FText::FromString(Ship->GetSizeClassName()));
	}
	for (const FAPSFleetUnit& Other : Units)
	{
		if (&Other != Unit && Other.Order == EOrder::BuildStructure && Other.Target.Get() == Target
			&& Other.StructureType == StructureType)
		{
			return FText::Format(LOCTEXT("AlreadyBuildingType", "{0} is already building it there."), UnitName(Other));
		}
	}
	return FText::GetEmpty();
}

ASpaceship* FAPSFleetCommand::PickShipFor(const APSFleet::EOrder Order, const AActor* Target, const FName StructureType,
	FText& OutRefusal) const
{
	using namespace APSFleet;
	OutRefusal = FText::GetEmpty();
	ASpaceship* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (const FAPSFleetUnit& Unit : Units)
	{
		ASpaceship* Ship = Unit.Ship.Get();
		// Idle or holding where its last order left it; a ship under way keeps its order.
		if (!Ship || (Unit.Order != EOrder::None && Unit.Phase != EPhase::Holding)) continue;
		const FText Refusal = Order == EOrder::BuildStructure ? CheckBuildOrder(Ship, Target, StructureType)
			: CheckOrder(Ship, Order, Target);
		if (!Refusal.IsEmpty())
		{
			if (OutRefusal.IsEmpty()) OutRefusal = FText::Format(LOCTEXT("PickRefused", "{0}: {1}"), UnitName(Unit), Refusal);
			continue;
		}
		const double Distance = Target ? FVector::DistSquared(Ship->GetActorLocation(), UAPSWorldOriginSubsystem::WorldPlace(*Target)) : 0.0;
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Ship;
		}
	}
	if (Best) OutRefusal = FText::GetEmpty();
	else if (OutRefusal.IsEmpty()) OutRefusal = LOCTEXT("NoShipFree", "No free ship of the right division.");
	return Best;
}

int32 FAPSFleetCommand::IssueOrder(const TArray<ASpaceship*>& Ships, const APSFleet::EOrder Order, AActor* Target,
	FText& OutRefusal, const FName StructureType)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	OutRefusal = FText::GetEmpty();
	int32 Issued = 0;
	FString Names;
	for (ASpaceship* Ship : Ships)
	{
		// One builder raises one structure; its cost is taken when it sets out.
		if (Order == EOrder::BuildStructure && Issued > 0) break;
		FText Refusal = Order == EOrder::BuildStructure ? CheckBuildOrder(Ship, Target, StructureType)
			: CheckOrder(Ship, Order, Target);
		if (Refusal.IsEmpty() && Order == EOrder::BuildStructure)
		{
			FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
			if (!Infrastructure || !Infrastructure->Reserve(StructureType))
			{
				Refusal = LOCTEXT("CostNotTaken", "The stocks do not cover it.");
			}
		}
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
		// Rio 06.10 (still ship, the review): a unit berthed on something the still frame holds (a system riding with the
		// sky, a catalogue anchor) leaves it at its world place: once detached it lives in the world's frame.
		const FVector UnitWorldPlace = UAPSWorldOriginSubsystem::WorldPlace(*Ship);
		Ship->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (!UnitWorldPlace.Equals(Ship->GetActorLocation(), 1.0))
		{
			Ship->SetActorLocation(UnitWorldPlace, false, nullptr, ETeleportType::TeleportPhysics);
		}
		Ship->Tags.Remove(ParkedTag);
		// Clear the berth first: straight out from the nearest body, a few ship lengths.
		double Surface = 0.0, Radius = 0.0;
		AActor* Near = NearestBody(Ship->GetActorLocation(), Surface, Radius);
		const FVector Out = Near ? (Ship->GetActorLocation() - UAPSWorldOriginSubsystem::WorldPlace(*Near)).GetSafeNormal() : Ship->GetShipUpVector();
		const double Clearance = FMath::Max(300000.0, Ship->GetComponentsBoundingBox().GetExtent().GetMax() * 6.0);
		Unit->DepartFrom = Near;
		Unit->DepartOffset = Ship->GetActorLocation() + Out * Clearance - (Near ? UAPSWorldOriginSubsystem::WorldPlace(*Near) : FVector::ZeroVector);
		if (Unit->bPilotWork && Unit->Order == EOrder::BuildStructure && Unit->Phase == EPhase::Working)
		{
			if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get())) Infrastructure->Refund(Unit->StructureType);
		}
		Unit->bPilotWork = false;
		Unit->Order = Order;
		Unit->StructureType = Order == EOrder::BuildStructure ? StructureType : NAME_None;
		Unit->Target = Order == EOrder::Return ? nullptr : Target;
		Unit->Phase = EPhase::Departing;
		Unit->Progress = 0.0f;
		Unit->TransitStartCm = 0.0;
		Unit->Speed = 0.0;
		Unit->Heading = Ship->GetShipForwardVector();
		// A slot on the side of the target the ship comes from, fanned out so a group does not stack.
		if (Order != EOrder::Return)
		{
			FVector Side = (Ship->GetActorLocation() - UAPSWorldOriginSubsystem::WorldPlace(*Target)).GetSafeNormal();
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
		const APSInfrastructure::FType* Type = Order == EOrder::BuildStructure ? APSInfrastructure::Find(StructureType) : nullptr;
		Post(Order == EOrder::Return
			? FText::Format(LOCTEXT("PostReturn", "Order to {0}: return to the berth."), FText::FromString(Names))
			: Type ? FText::Format(LOCTEXT("PostBuildOrder", "Order to {0}: build {1} at {2}."), FText::FromString(Names),
				Type->Name, NameOf(Target))
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
	if (Unit->Order == APSFleet::EOrder::BuildStructure && Unit->Phase != APSFleet::EPhase::Holding)
	{
		if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get())) Infrastructure->Refund(Unit->StructureType);
	}
	Unit->StructureType = NAME_None;
	Unit->Order = APSFleet::EOrder::None;
	Unit->Phase = APSFleet::EPhase::Idle;
	Unit->Target = nullptr;
	Unit->Speed = 0.0;
	Unit->Progress = 0.0f;
	Unit->bPilotWork = false;
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
	// Rio 03.10: a hub's or megastructure's construction ship works by its scaffold (APSMegastructures::WorkSlot), not at
	// the world's generic slot.
	if (Unit.Order == APSFleet::EOrder::BuildStructure)
	{
		const APSInfrastructure::FType* Type = APSInfrastructure::Find(Unit.StructureType);
		const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
		FVector WorkSlot;
		if (Type && Infrastructure && APSInfrastructure::IsMegaVisual(Type->Visual)
			&& APSMegastructures::WorkSlot(*Type, *Target, *Infrastructure, WorkSlot))
		{
			return WorkSlot;
		}
	}
	const double Radius = APSFleetPrivate::BodyRadiusCm(Target);
	if (FGuid SystemId; Target->IsA<AStar>() || FAPSStarSystems::AnchorSystem(Target, SystemId))
	{
		// In a star system the ships hold at its edge of glare, not a few hundred kilometres over the photosphere.
		return UAPSWorldOriginSubsystem::WorldPlace(*Target) + Unit.SlotDirection * FMath::Max(0.06 * APSStars::AstronomicalUnitCm, Radius * 40.0);
	}
	return UAPSWorldOriginSubsystem::WorldPlace(*Target) + Unit.SlotDirection * (Radius > 0.0 ? APSFleet::SlotRadius(Radius) : 150000.0);
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
	if (Unit.bPilotWork)
	{
		TickPilotWork(Unit, DeltaSeconds);
		return;
	}
	if (Ship->HasPilot())
	{
		// The pilot took the helm: the order ends where the ship is.
		Post(FText::Format(LOCTEXT("TookHelm", "{0}: a pilot took the helm, orders cancelled."), UnitName(Unit)));
		if (Unit.Order == EOrder::BuildStructure && Unit.Phase != EPhase::Holding)
		{
			if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get())) Infrastructure->Refund(Unit.StructureType);
		}
		Unit.StructureType = NAME_None;
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
		const FVector Goal = (From ? UAPSWorldOriginSubsystem::WorldPlace(*From) : FVector::ZeroVector) + Unit.DepartOffset;
		double Remaining = 0.0;
		Fly(Unit, Ship, Goal, 0.0, DeltaSeconds, Remaining);
		Unit.RemainingCm = FVector::Distance(Ship->GetActorLocation(), SlotLocation(Unit));
		Unit.TransitStartCm = FMath::Max(Unit.TransitStartCm, Unit.RemainingCm);
		if (Remaining < 20000.0)
		{
			Unit.Phase = EPhase::Transit;
		}
		break;
	}
	case EPhase::Transit:
	{
		const FVector Slot = SlotLocation(Unit);
		// Round any planet, moon or star in the way, the nearest such first: along its sphere (RoundBody), never
		// through it; else straight for the slot.
		FVector Goal = Slot;
		double BestDistance = TNumericLimits<double>::Max();
		if (UWorld* LiveWorld = World.Get())
		{
			for (TActorIterator<ACelestialBody> It(LiveWorld); It; ++It)
			{
				FVector Point;
				const double Radius = APSFleetPrivate::BodyRadiusCm(*It);
				if (APSFleet::Detour(Ship->GetActorLocation(), Slot, UAPSWorldOriginSubsystem::WorldPlace(**It), Radius, Point))
				{
					const double Distance = FVector::DistSquared(Ship->GetActorLocation(), Point);
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						Goal = APSFleet::RoundBody(Ship->GetActorLocation(), Slot, UAPSWorldOriginSubsystem::WorldPlace(**It), Radius);
					}
				}
			}
		}
		double Remaining = 0.0;
		Fly(Unit, Ship, Goal, FVector::Distance(Goal, Slot), DeltaSeconds, Remaining);
		Unit.RemainingCm = Remaining;
		Unit.TransitStartCm = FMath::Max(Unit.TransitStartCm, Unit.RemainingCm);
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
		: APSFleet::ClassCap(Ship->ActiveClassPreset) * SpeedScale() * GetDivisionSpeedFactor(Unit.Division);
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
	const FVector UpHint = Near ? (Location - UAPSWorldOriginSubsystem::WorldPlace(*Near)).GetSafeNormal() : Ship->GetShipUpVector();
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
		++Revision;
		return;
	}
	BeginWork(Unit);
}

void FAPSFleetCommand::BeginWork(FAPSFleetUnit& Unit)
{
	using namespace APSFleet;
	AActor* Target = Unit.Target.Get();
	{
		Unit.Phase = EPhase::Working;
		Unit.Progress = 0.0f;
		Unit.WorkLength = static_cast<float>(WorkSeconds(Unit.Order, Unit.Division, DivisionLevel(Unit.Division),
			CountStructures(Target, EStructure::Station) > 0));
		const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
		if (Unit.Order == EOrder::BuildStructure)
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Unit.StructureType);
			const double Speed = 1.0 + 0.2 * FMath::Max(DivisionLevel(Unit.Division), 0)
				+ (Infrastructure ? Infrastructure->BuildSpeedBonus() + Infrastructure->LocalWorkBonus(Target) : 0.0);
			Unit.WorkLength = static_cast<float>((Type ? Type->BuildSeconds : 60.0f) / FMath::Max(Speed, 0.1));
			Post(FText::Format(LOCTEXT("BuildStructureStarted", "{0} began building {1} at {2}."), UnitName(Unit),
				Type ? Type->Name : FText::FromName(Unit.StructureType), APSFleetPrivate::NameOf(Target)));
			++Revision;
			return;
		}
		if (Unit.Order == EOrder::Survey && Infrastructure)
		{
			// Probe bays and civic services everywhere, survey outposts and laboratories at this world.
			Unit.WorkLength /= 1.0f + Infrastructure->SurveySpeedBonus() + Infrastructure->LocalWorkBonus(Target);
		}
		if (Unit.Order == EOrder::Probe || Unit.Order == EOrder::SurveySystem)
		{
			Post(FText::Format(Unit.Order == EOrder::Probe
				? LOCTEXT("ProbeStarted", "{0} launched a probe into {1}.")
				: LOCTEXT("SystemSurveyStarted", "{0} began charting the star system {1}."), UnitName(Unit),
				APSFleetPrivate::NameOf(Target)));
			++Revision;
			return;
		}
		EStructure Structure = EStructure::Station;
		Post(Unit.Order == EOrder::Survey
			? FText::Format(LOCTEXT("SurveyStarted", "{0} began surveying {1}."), UnitName(Unit), APSFleetPrivate::NameOf(Target))
			: Unit.Order == EOrder::Expedition
			? FText::Format(LOCTEXT("ExpeditionStarted", "{0} sent its crew down to the anomaly on {1}."), UnitName(Unit),
				APSFleetPrivate::NameOf(Target))
			: StructureOf(Unit.Order, Structure)
			? FText::Format(LOCTEXT("StructureStarted", "{0} began building {1} at {2}."), UnitName(Unit),
				APSFleetPrivate::StructureNoun(Structure), APSFleetPrivate::NameOf(Target))
			: FText::Format(LOCTEXT("BuildStarted", "{0} began building an outpost at {1}."), UnitName(Unit),
				APSFleetPrivate::NameOf(Target)));
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
	if (Unit.Order == EOrder::Expedition && !Body)
	{
		FGuid SystemId;
		FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		if (Stars && FAPSInfrastructure::SiteSystem(World.Get(), Unit.Target.Get(), SystemId))
		{
			Stars->InvestigateAnomaly(SystemId, FText::Format(LOCTEXT("ExpeditionOf", "The expedition of {0}"), UnitName(Unit)));
		}
		return;
	}
	if (Unit.Order == EOrder::Probe || Unit.Order == EOrder::SurveySystem)
	{
		FGuid SystemId;
		FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		if (Stars && FAPSInfrastructure::SiteSystem(World.Get(), Unit.Target.Get(), SystemId))
		{
			Stars->Learn(SystemId, Unit.Order == EOrder::Probe ? APSStars::EKnowledge::Scanned : APSStars::EKnowledge::Surveyed,
				FText::Format(Unit.Order == EOrder::Probe ? LOCTEXT("ProbedBy", "the probe of {0}")
					: LOCTEXT("ChartedBy", "charted by {0}"), UnitName(Unit)));
		}
		return;
	}
	if (Unit.Order == EOrder::BuildStructure)
	{
		AActor* Site = Unit.Target.Get();
		FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get());
		const APSInfrastructure::FType* Type = APSInfrastructure::Find(Unit.StructureType);
		const FVector Near = Unit.Ship.IsValid() ? Unit.Ship->GetActorLocation() : (Site ? Site->GetActorLocation() : FVector::ZeroVector);
		AActor* Built = Infrastructure && Site ? Infrastructure->Complete(Unit.StructureType, Site, &Near) : nullptr;
		if (!Built && Infrastructure) Infrastructure->Refund(Unit.StructureType);
		Post(Built ? FText::Format(LOCTEXT("StructureRaised", "{0} built {1} at {2}."), UnitName(Unit),
				Type ? Type->Name : FText::FromName(Unit.StructureType), NameOf(Site))
			: FText::Format(LOCTEXT("StructureNotRaised", "{0} could not raise {1} at {2}; the stocks are returned."),
				UnitName(Unit), Type ? Type->Name : FText::FromName(Unit.StructureType), NameOf(Site)));
		Unit.StructureType = NAME_None;
		return;
	}
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
		const ESurvey Before = Record.Survey;
		Record.Survey = Reached;
		RevealAnomaly(Record, Reached, UnitName(Unit), true);
		if (Before < ESurvey::Surveyed) APSMissionsNotify(World.Get(), APSMissions::EObjective::SurveyWorld, KeyOf(Body));
		if (Reached == ESurvey::Studied) APSMissionsNotify(World.Get(), APSMissions::EObjective::StudyWorld, KeyOf(Body));
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
	if (Unit.Order == EOrder::Expedition)
	{
		InvestigateAnomaly(Record, UnitName(Unit), false, true);
		return;
	}
	if (EStructure Structure = EStructure::Station; StructureOf(Unit.Order, Structure))
	{
		const ASpaceship* Ship = Unit.Ship.Get();
		if (!Ship)
		{
			return;
		}
		// Beside the builder in its orbit, on the other side from the outposts, spaced by what already stands there.
		const FVector Radial = Unit.SlotDirection.GetSafeNormal();
		FVector Along = FVector::CrossProduct(Body->GetActorUpVector(), Radial).GetSafeNormal();
		Along = Along.IsNearlyZero() ? FVector::ForwardVector : Along;
		int32 Standing = 0;
		for (int32 Kind = 0; Kind < static_cast<int32>(EStructure::Count); ++Kind)
		{
			Standing += CountStructures(Body, static_cast<EStructure>(Kind));
		}
		const FVector Location = Ship->GetActorLocation() - Along * (800000.0 + 600000.0 * Standing);
		const FText Name = FText::Format(LOCTEXT("StructureTitle", "{0} {1}"), StructureName(Structure), NameOf(Body));
		const FString ActorName = FString::Printf(TEXT("APS_Fleet_%s_%d"),
			Structure == EStructure::Shipyard ? TEXT("Shipyard") : Structure == EStructure::Headquarters ? TEXT("HQ")
				: TEXT("Station"), ++StructureSerial);
		if (!SpawnStructure(Structure, Body, FTransform(FRotationMatrix::MakeFromZ(Radial).ToQuat(), Location), Name,
			ActorName))
		{
			Post(FText::Format(LOCTEXT("StructureFailed", "{0} could not raise {1} at {2}."), UnitName(Unit),
				StructureNoun(Structure), NameOf(Body)));
			return;
		}
		Post(FText::Format(LOCTEXT("StructureBuilt", "{0} built {1}. {2}"), UnitName(Unit), Name,
			StructureEffect(Structure, NameOf(Body))));
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
			FText::FromString(NameOf(Body).ToString()), APSUINumber::Number(CountOutposts(Body) + 1));
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
	const FText Target = APSFleetPrivate::NameOf(Unit.Target.Get());
	if (Ship && Ship->HasPilot())
	{
		return Unit.bPilotWork && Unit.Phase == EPhase::Working
			? FText::Format(LOCTEXT("StatePilotWork", "PILOTED, {0} {1}: {2}%"), OrderName(Unit.Order), Target,
				APSUINumber::Number(FMath::RoundToInt(Unit.Progress * 100.0f)))
			: LOCTEXT("StatePiloted", "PILOTED");
	}
	switch (Unit.Phase)
	{
	case EPhase::Departing:
		return FText::Format(LOCTEXT("StateDeparting", "{0} {1}: LEAVING THE BERTH"), OrderName(Unit.Order), Target);
	case EPhase::Transit:
		// Rio 05.10 (star map): the way left in AU between the stars (light years beyond), in km only near a world.
		return FText::Format(LOCTEXT("StateTransit", "{0} {1}: {2} TO GO"), OrderName(Unit.Order),
			Unit.Order == EOrder::Return ? LOCTEXT("ToBerth", "TO THE BERTH") : Target,
			APSFleetPrivate::DistanceText(Unit.RemainingCm));
	case EPhase::Working:
		return FText::Format(LOCTEXT("StateWorking", "{0} {1}: {2}%"), OrderName(Unit.Order), Target,
			APSUINumber::Number(FMath::RoundToInt(Unit.Progress * 100.0f)));
	case EPhase::Holding:
		return Unit.Order == EOrder::Move ? FText::Format(LOCTEXT("StateHolding", "HOLDING AT {0}"), Target)
			: FText::Format(LOCTEXT("StateDone", "{0} {1}: DONE, HOLDING"), OrderName(Unit.Order), Target);
	default:
		return Unit.bHasBerth ? LOCTEXT("StateIdleSpace", "IDLE, HOLDING POSITION") : LOCTEXT("StateIdle", "IDLE AT THE BERTH");
	}
}

double FAPSFleetCommand::EstimateArrivalSeconds(const ASpaceship* Ship, const AActor* Target) const
{
	using namespace APSFleet;
	const FAPSFleetUnit* Unit = FindUnit(Ship);
	if (!Unit || !IsValid(Target))
	{
		return -1.0;
	}
	// The reach CheckOrder allows: without SpaceWrap neither another star nor another planet's neighbourhood.
	FGuid SystemId;
	const bool bSystem = FAPSInfrastructure::SiteSystem(World.Get(), Target, SystemId) && !Target->IsA<APlanetaryBody>();
	if (!Ship->ActiveClassPreset.bSupportsSpaceWrap
		&& (bSystem || HomeBodyOf(Target->GetActorLocation()) != HomeBodyOf(Ship->GetActorLocation())))
	{
		return -1.0;
	}
	const double Cap = ClassCap(Ship->ActiveClassPreset) * SpeedScale() * GetDivisionSpeedFactor(Unit->Division);
	if (Unit->Target.Get() == Target && Unit->Order != EOrder::None && Unit->Order != EOrder::Return)
	{
		if (Unit->Phase == EPhase::Transit)
		{
			return APSFleetPrivate::FlightSeconds(Unit->RemainingCm, Unit->Speed, Cap);
		}
		if (Unit->Phase == EPhase::Working || Unit->Phase == EPhase::Holding)
		{
			return 0.0;
		}
	}
	// From rest where it is: a few seconds to clear the berth, then to the slot SlotLocation gives (a star system's edge of
	// glare, a world's orbit).
	const double Radius = APSFleetPrivate::BodyRadiusCm(Target);
	const double Slot = bSystem ? FMath::Max(0.06 * APSStars::AstronomicalUnitCm, Radius * 40.0)
		: Radius > 0.0 ? SlotRadius(Radius) : 150000.0;
	constexpr double DepartSeconds = 4.0;
	return DepartSeconds + APSFleetPrivate::FlightSeconds(
		FMath::Max(FVector::Dist(Ship->GetActorLocation(), UAPSWorldOriginSubsystem::WorldPlace(*Target)) - Slot, 0.0), 0.0, Cap);
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
		// Rio 07.10: a world that is away (its star system released) by its key.
		const FString Name = Record.Body.IsValid() || Record.Key.IsEmpty() ? APSFleetPrivate::NameOf(Record.Body.Get()).ToString()
			: Record.Key + TEXT(" (away)");
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] body %s %s outposts=%d"), *Name,
			*APSFleet::SurveyName(Record.Survey).ToString(), CountOutposts(Record.Body.Get()));
	}
	for (const FAPSFleetStructure& Structure : Structures)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] structure %s %s at %s%s"), *GetNameSafe(Structure.Actor.Get()),
			*APSFleet::StructureName(Structure.Kind).ToString(), *APSFleetPrivate::NameOf(Structure.Body.Get()).ToString(),
			Structure.bBuilt ? TEXT(" (built by the fleet)") : TEXT(""));
	}
	if (!HeldStructures.IsEmpty() || !HeldOutposts.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] held while their worlds are away: %d structure(s), %d outpost(s)"),
			HeldStructures.Num(), HeldOutposts.Num());
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

void FAPSFleetSaveData::SerializeExtras(FArchive& Ar, FAPSFleetSaveData& Data)
{
	int32 StructureCount = Data.Structures.Num();
	Ar << StructureCount;
	if (Ar.IsLoading())
	{
		Data.Structures.SetNum(FMath::Clamp(StructureCount, 0, 4096));
	}
	for (FStructure& Structure : Data.Structures)
	{
		Ar << Structure.Kind << Structure.BodyKey << Structure.RelativeTransform << Structure.Name << Structure.ActorName;
	}
	int32 JobCount = Data.ShipyardJobs.Num();
	Ar << JobCount;
	if (Ar.IsLoading())
	{
		Data.ShipyardJobs.SetNum(FMath::Clamp(JobCount, 0, 4096));
	}
	for (FShipyardJob& Job : Data.ShipyardJobs)
	{
		Ar << Job.ClassPath << Job.SizeClass << Job.Name << Job.Length << Job.Progress << Job.YardKey;
	}
	int32 InvestigationCount = Data.Investigations.Num();
	Ar << InvestigationCount;
	if (Ar.IsLoading())
	{
		Data.Investigations.SetNum(FMath::Clamp(InvestigationCount, 0, 4096));
	}
	for (TPair<FString, uint8>& Investigation : Data.Investigations)
	{
		Ar << Investigation.Key << Investigation.Value;
	}
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
	if (FGuid SystemId; FAPSStarSystems::AnchorSystem(Actor, SystemId))
	{
		return TEXT("SYSTEM:") + SystemId.ToString(EGuidFormats::Digits);
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
	if (Key.StartsWith(TEXT("SYSTEM:")))
	{
		// A star system's anchor is spawned on demand, so a load finds its orders' targets again.
		FGuid SystemId;
		FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
		return Stars && FGuid::Parse(Key.RightChop(7), SystemId) ? Stars->GetAnchor(SystemId) : nullptr;
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
	const FText& Name, const bool bCountCivilization)
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
	// Rio 07.10: one standing again after its world was away was counted when it was built.
	if (UCivilization* Civ = bCountCivilization ? Civilization() : nullptr)
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
	// Rio 06.10 (audit: a save taken before the fleet restore ran wrote the half-restored fleet: the generator's ships without
	// their orders, no surveys, outposts, built structures or slipway jobs): while the loaded data still waits, it is what is
	// saved (aps.Save.RestoreWaitForSystems, whose wait can hold it a little longer; 0 keeps the previous capture).
	if (PendingRestore.IsSet() && APSFleetPrivate::CVarRestoreWaitForSystems.GetValueOnAnyThread() != 0)
	{
		OutData = PendingRestore.GetValue();
		return;
	}
	for (const FAPSFleetUnit& Unit : Units)
	{
		const ASpaceship* Ship = Unit.Ship.Get();
		if (!Ship)
		{
			continue;
		}
		if (Unit.Order == APSFleet::EOrder::BuildStructure && !Unit.StructureType.IsNone())
		{
			OutData.UnitStructureTypes.Emplace(Unit.CallSign, Unit.StructureType.ToString());
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
		// Rio 07.10 (aps.Stars.HoldReleasedStructures): a ship waiting at a star system's beacon (its world went with the
		// system) is kept by that beacon, which a load finds again (a SYSTEM: key), not by whatever body is nearest. Only when
		// the load registers that system again; else by the nearest body, as before.
		if (FGuid AnchorId; !Reference && FAPSInfrastructure::HoldsReleased() && Ship->GetAttachParentActor()
			&& FAPSStarSystems::AnchorSystem(Ship->GetAttachParentActor(), AnchorId)
			&& APSFleetPrivate::SystemReturnsAtLoad(World.Get(), AnchorId))
		{
			Reference = Ship->GetAttachParentActor();
		}
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
			// Rio 07.10: a world that is away (its star system released, or not here at load) is saved by its key.
			if (!Body.Key.IsEmpty() && Body.Survey != ESurvey::Unknown)
			{
				OutData.Surveys.Emplace(Body.Key, static_cast<uint8>(Body.Survey));
			}
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
	// Rio 07.10: and the outposts held while their worlds are away, as they were.
	for (const TPair<FGuid, FAPSFleetSaveData::FOutpost>& Held : HeldOutposts)
	{
		OutData.Outposts.Add(Held.Value);
	}
	for (const FAPSFleetBodyRecord& Body : Bodies)
	{
		if (Body.Body.IsValid() && Body.bHasAnomaly && Body.Anomaly == EAnomalyState::Investigated)
		{
			OutData.Investigations.Emplace(KeyOf(Body.Body.Get()), Body.bAnomalyInPerson ? 2 : 1);
		}
		else if (!Body.Body.IsValid() && !Body.Key.IsEmpty())
		{
			// Rio 07.10: a world that is away, by its key (one kept from a save has not rolled its anomaly yet).
			const uint8 Investigated = Body.bHasAnomaly && Body.Anomaly == EAnomalyState::Investigated
				? static_cast<uint8>(Body.bAnomalyInPerson ? 2 : 1) : Body.HeldInvestigation;
			if (Investigated > 0)
			{
				OutData.Investigations.Emplace(Body.Key, Investigated);
			}
		}
	}
	// Only what the fleet built: the generator raises the home complex again by itself.
	for (const FAPSFleetStructure& Structure : Structures)
	{
		const AActor* Actor = Structure.Actor.Get();
		const APlanetaryBody* Body = Structure.Body.Get();
		if (!Structure.bBuilt || !Actor || !Body)
		{
			continue;
		}
		FAPSFleetSaveData::FStructure& Record = OutData.Structures.AddDefaulted_GetRef();
		Record.Kind = static_cast<uint8>(Structure.Kind);
		Record.BodyKey = KeyOf(Body);
		Record.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Body->GetActorTransform());
		Record.Name = IItemInfoInterface::Execute_GetInGameName(Actor).ToString();
		Record.ActorName = Actor->GetName();
	}
	// Rio 07.10: and those held while their worlds are away, as they were (a load holds them again until the world stands).
	for (const TPair<FGuid, FAPSFleetSaveData::FStructure>& Held : HeldStructures)
	{
		OutData.Structures.Add(Held.Value);
	}
	for (const FAPSShipyardJob& Job : ShipyardQueue)
	{
		if (!Job.ShipClass)
		{
			continue;
		}
		FAPSFleetSaveData::FShipyardJob& Record = OutData.ShipyardJobs.AddDefaulted_GetRef();
		Record.ClassPath = Job.ShipClass->GetPathName();
		Record.SizeClass = static_cast<uint8>(Job.SizeClass);
		Record.Name = Job.Name.ToString();
		Record.Length = Job.Length;
		Record.Progress = Job.Progress;
		Record.YardKey = KeyOf(Job.Yard.Get());
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
	// Rio 06.10 (audit: galaxy-reach systems register ~1-3 s after the load; FindByKey of their SYSTEM: keys was null on the
	// first tick, so orders to them were cleared and ships referenced to them stayed where the generator put them): within
	// the same 20 s, wait for the star registry and for every saved SYSTEM: key. A save without such keys does not wait
	// (worlds whose registry never reads, as the authored route may be, restore as before).
	if (CVarRestoreWaitForSystems.GetValueOnGameThread() != 0 && LiveWorld->GetTimeSeconds() - PendingRestoreSince < 20.0)
	{
		const auto IsSystemKey = [](const FString& Key) { return Key.StartsWith(TEXT("SYSTEM:")); };
		const bool bAnySystemKey = Data.Units.ContainsByPredicate([&IsSystemKey](const FAPSFleetUnitRecord& Saved)
		{
			return IsSystemKey(Saved.TargetKey) || IsSystemKey(Saved.ReferenceKey) || IsSystemKey(Saved.BerthKey);
		});
		if (bAnySystemKey)
		{
			const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
			if (!Stars || !Stars->IsReady())
			{
				return;
			}
			for (const FAPSFleetUnitRecord& Saved : Data.Units)
			{
				for (const FString* Key : {&Saved.TargetKey, &Saved.ReferenceKey, &Saved.BerthKey})
				{
					if (IsSystemKey(*Key) && !FindByKey(*Key))
					{
						return;
					}
				}
			}
		}
	}
	PendingRestore.Reset();
	// Rio 07.10 (aps.Stars.HoldReleasedStructures): what the fleet has on a world that is not here at load (another star
	// system's, which stands only while the pilot is there) is kept until that world stands, not dropped: its survey and
	// investigation as a record by its key, its structures and outposts held (raised by the world's name, RaiseHeldWorlds).
	const bool bHoldReleased = FAPSInfrastructure::HoldsReleased();
	const auto IsBodyKey = [](const FString& Key) { return Key.StartsWith(TEXT("BODY:")); };
	const auto KeptRecord = [this](const FString& Key) -> FAPSFleetBodyRecord&
	{
		if (FAPSFleetBodyRecord* Kept = Bodies.FindByPredicate([&Key](const FAPSFleetBodyRecord& Record)
			{
				return !Record.Body.IsValid() && Record.Key == Key;
			}))
		{
			return *Kept;
		}
		FAPSFleetBodyRecord& Record = Bodies.AddDefaulted_GetRef();
		Record.Key = Key;
		return Record;
	};
	int32 KeptWorlds = 0;
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
			RevealAnomaly(Record, Record.Survey, FText::GetEmpty(), false);
		}
		else if (bHoldReleased && IsBodyKey(Survey.Key))
		{
			FAPSFleetBodyRecord& Record = KeptRecord(Survey.Key);
			const ESurvey Level = static_cast<ESurvey>(FMath::Min<uint8>(Survey.Value, static_cast<uint8>(ESurvey::Studied)));
			if (Level > Record.Survey)
			{
				Record.Survey = Level;
			}
			++KeptWorlds;
		}
	}
	// The fleet's stations, shipyards and HQs first, under their saved names: units and slipway jobs refer to them.
	int32 StructuresBack = 0;
	for (const FAPSFleetSaveData::FStructure& Saved : Data.Structures)
	{
		APlanetaryBody* Body = Cast<APlanetaryBody>(FindByKey(Saved.BodyKey));
		// Rio 07.10: one on a world not here is held (its name kept from a new build's) until a world of that name stands.
		if (!Body && bHoldReleased && IsBodyKey(Saved.BodyKey) && Saved.Kind < static_cast<uint8>(EStructure::Count))
		{
			FString Digits;
			if (Saved.ActorName.Split(TEXT("_"), nullptr, &Digits, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				StructureSerial = FMath::Max(StructureSerial, FCString::Atoi(*Digits));
			}
			HeldStructures.Emplace(FGuid(), Saved);
			continue;
		}
		if (!Body || Saved.Kind >= static_cast<uint8>(EStructure::Count)
			|| (!Saved.ActorName.IsEmpty() && FindByKey(TEXT("ACTOR:") + Saved.ActorName)))
		{
			continue;
		}
		FString Digits;
		if (Saved.ActorName.Split(TEXT("_"), nullptr, &Digits, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			StructureSerial = FMath::Max(StructureSerial, FCString::Atoi(*Digits));
		}
		StructuresBack += SpawnStructure(static_cast<EStructure>(Saved.Kind), Body,
			Saved.RelativeTransform * Body->GetActorTransform(), FText::FromString(Saved.Name), Saved.ActorName) ? 1 : 0;
	}
	int32 Outposts = 0;
	for (const FAPSFleetSaveData::FOutpost& Saved : Data.Outposts)
	{
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(FindByKey(Saved.BodyKey)))
		{
			const FTransform Transform = Saved.RelativeTransform * Body->GetActorTransform();
			Outposts += SpawnOutpost(Body, Transform.GetLocation(), Transform.GetRotation(), FText::FromString(Saved.Name)) ? 1 : 0;
		}
		else if (bHoldReleased && IsBodyKey(Saved.BodyKey))
		{
			// Rio 07.10: held until a world of that name stands.
			HeldOutposts.Emplace(FGuid(), Saved);
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
		ASpaceship* Ship = Class && Reference ? LaunchShip(Class, Saved.RelativeTransform * Reference->GetActorTransform(), nullptr)
			: nullptr;
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
		Unit->Order = static_cast<EOrder>(FMath::Min<uint8>(Saved.Order, static_cast<uint8>(LastOrder)));
		Unit->Phase = static_cast<EPhase>(FMath::Min<uint8>(Saved.Phase, static_cast<uint8>(EPhase::Holding)));
		Unit->StructureType = NAME_None;
		for (const TPair<FString, FString>& Building : Data.UnitStructureTypes)
		{
			if (Building.Key == Saved.CallSign) Unit->StructureType = FName(*Building.Value);
		}
		if (Unit->Order == EOrder::BuildStructure && Unit->StructureType.IsNone())
		{
			Unit->Order = EOrder::None;
		}
		Unit->Target = FindByKey(Saved.TargetKey);
		Unit->SlotDirection = Saved.SlotDirection;
		Unit->Progress = Saved.Progress;
		Unit->WorkLength = Saved.WorkLength;
		Unit->Speed = 0.0;
		Unit->Heading = FVector::ZeroVector;
		if (Unit->Order != EOrder::None && Unit->Order != EOrder::Return && !Unit->Target.IsValid())
		{
			// Rio 07.10 (aps.Stars.HoldReleasedStructures): a build at a world not here gives back the cost taken when it was
			// ordered, as a lost target does in play (CancelOrder); before, the stocks were simply gone.
			if (bHoldReleased && Unit->Order == EOrder::BuildStructure && Unit->Phase != EPhase::Holding)
			{
				if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(LiveWorld))
				{
					Infrastructure->Refund(Unit->StructureType);
					UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s: its build of %s at %s ended at load (the world is not here); cost returned"),
						*Unit->CallSign, *Unit->StructureType.ToString(), *Saved.TargetKey);
				}
			}
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
	for (const TPair<FString, uint8>& Saved : Data.Investigations)
	{
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(FindByKey(Saved.Key)))
		{
			InvestigateAnomaly(BodyRecord(Body), FText::GetEmpty(), Saved.Value == 2, false);
		}
		else if (bHoldReleased && IsBodyKey(Saved.Key))
		{
			// Rio 07.10: kept with its world's record until the world stands (its anomaly is rolled then).
			KeptRecord(Saved.Key).HeldInvestigation = static_cast<uint8>(Saved.Value == 2 ? 2 : 1);
		}
	}
	// The slipways: a job whose shipyard did not come back moves to the home one (TickShipyard).
	for (const FAPSFleetSaveData::FShipyardJob& Saved : Data.ShipyardJobs)
	{
		UClass* Class = LoadClass<ASpaceship>(nullptr, *Saved.ClassPath);
		if (!Class)
		{
			continue;
		}
		FAPSShipyardJob& Job = ShipyardQueue.AddDefaulted_GetRef();
		Job.ShipClass = Class;
		Job.SizeClass = static_cast<ESpaceshipSizeClass>(Saved.SizeClass);
		Job.Name = FText::FromString(Saved.Name);
		Job.Length = FMath::Max(Saved.Length, 1.0f);
		Job.Progress = FMath::Clamp(Saved.Progress, 0.0f, 0.999f);
		Job.Yard = Cast<ASpaceShipyard>(FindByKey(Saved.YardKey));
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] restored %d of %d units, %d surveys, %d outposts, %d of %d structures, %d slipway jobs"),
		Restored, Data.Units.Num(), Data.Surveys.Num(), Outposts, StructuresBack, Data.Structures.Num(), Data.ShipyardJobs.Num());
	if (KeptWorlds + HeldStructures.Num() + HeldOutposts.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] held until their worlds stand: %d world record(s), %d structure(s), %d outpost(s)"),
			KeptWorlds, HeldStructures.Num(), HeldOutposts.Num());
	}
	// The levels the restored work earned were announced in the saved journal already.
	bEarnedPrimed = false;
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
	// The home shipyard (the escorts wait there): GetShipyards lists it first.
	TArray<ASpaceShipyard*> Yards;
	GetShipyards(Yards);
	return Yards.IsEmpty() ? nullptr : Yards[0];
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
	const float Industry = GetShipyardRate();
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

void FAPSFleetCommand::GetShipyards(TArray<ASpaceShipyard*>& OutYards) const
{
	using namespace APSFleetPrivate;
	OutYards.Reset();
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	// One civilization per world: every shipyard is its own. The generator does not tag the home complex, the
	// construction ships tag what they raise (StructureTag).
	for (TActorIterator<ASpaceShipyard> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It))
		{
			OutYards.Add(*It);
		}
	}
	// The home shipyard first, then those the construction ships raised, in the order they went up.
	OutYards.StableSort([](const ASpaceShipyard& A, const ASpaceShipyard& B)
	{
		return !A.ActorHasTag(StructureTag) && B.ActorHasTag(StructureTag);
	});
}

int32 FAPSFleetCommand::CountQueued(const ASpaceShipyard* Yard) const
{
	int32 Count = 0;
	for (const FAPSShipyardJob& Job : ShipyardQueue)
	{
		Count += Job.Yard.Get() == Yard ? 1 : 0;
	}
	return Count;
}

int32 FAPSFleetCommand::GetLaunchedCount(const ASpaceShipyard* Yard) const
{
	for (const TPair<TWeakObjectPtr<ASpaceShipyard>, int32>& Entry : LaunchedAt)
	{
		if (Entry.Key.Get() == Yard)
		{
			return Entry.Value;
		}
	}
	return 0;
}

FText FAPSFleetCommand::OrderShip(const FAPSShipyardOption& Option, ASpaceShipyard* Yard)
{
	if (!Option.ShipClass)
	{
		return LOCTEXT("NoShipClass", "Pick a ship.");
	}
	Yard = IsValid(Yard) ? Yard : FindShipyard();
	if (!Yard)
	{
		return LOCTEXT("NoShipyard", "No shipyard in this world.");
	}
	const FText YardName = DisplayName(Yard);
	if (CountQueued(Yard) >= ShipyardQueueLimit)
	{
		return FText::Format(LOCTEXT("SlipwayFull", "The slipway of {0} is full: {1} ships queued. Pick another shipyard."),
			YardName, APSUINumber::Number(ShipyardQueueLimit));
	}
	FAPSShipyardJob& Job = ShipyardQueue.AddDefaulted_GetRef();
	Job.ShipClass = Option.ShipClass;
	Job.SizeClass = Option.SizeClass;
	Job.Name = Option.Name;
	Job.Length = FMath::Max(Option.BuildSeconds, 1.0f);
	Job.Yard = Yard;
	++Revision;
	Post(FText::Format(LOCTEXT("ShipOrdered", "{0}: {1} (class {2}) laid down, about {3} s."), YardName, Option.Name,
		APSFleetPrivate::EnumText(Option.SizeClass), APSUINumber::Number(FMath::RoundToInt(Job.Length))));
	return FText::GetEmpty();
}

void FAPSFleetCommand::TickShipyard(const float DeltaSeconds)
{
	if (ShipyardQueue.IsEmpty())
	{
		return;
	}
	// A job whose shipyard is gone (or did not come back with a load) moves to the home one.
	ASpaceShipyard* Home = nullptr;
	bool bHomeFound = false;
	for (FAPSShipyardJob& Job : ShipyardQueue)
	{
		if (!Job.Yard.IsValid())
		{
			if (!bHomeFound)
			{
				Home = FindShipyard();
				bHomeFound = true;
			}
			Job.Yard = Home;
		}
	}
	// Rio 06.10 (flight FPS: single frames of 923, 216 and 76 ms in his log while he flew): a finished job spawns its hull,
	// sweeps a clear spot for it and refreshes the units on the game thread, all in one frame. While the world flows past
	// his fast ship, or its travel is owed to the sky, the job waits at 100% on the slipway (it stays in the queue, so a
	// save keeps it) and launches once the flight has been calm for aps.Fleet.HoldLaunchCalmSeconds. Parked, on foot or
	// at a speed without a flow nothing waits, as before.
	UWorld* LiveWorld = World.Get();
	const UAPSWorldOriginSubsystem* Origin = LiveWorld ? LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	bool bHoldLaunch = false;
	if (Origin && APSFleetPrivate::CVarHoldLaunchInFlight.GetValueOnGameThread() != 0)
	{
		bHoldLaunch = (Origin->IsTravelDeferred()
			|| Origin->IsWorldFlowing(FMath::Max(APSFleetPrivate::CVarHoldLaunchCalmSeconds.GetValueOnGameThread(), 0.0f)));
	}
	// Every shipyard builds the first ship of its own queue.
	const float Rate = FMath::Max(APSFleetPrivate::CVarBuildScale.GetValueOnGameThread(), 0.0f);
	TArray<const ASpaceShipyard*, TInlineAllocator<8>> Busy;
	for (int32 Index = 0; Index < ShipyardQueue.Num(); ++Index)
	{
		FAPSShipyardJob& Job = ShipyardQueue[Index];
		ASpaceShipyard* Shipyard = Job.Yard.Get();
		if (!Shipyard || Busy.Contains(Shipyard))
		{
			continue;
		}
		Busy.Add(Shipyard);
		Job.Progress = FMath::Min(1.0f, Job.Progress + DeltaSeconds * Rate / FMath::Max(Job.Length, 0.1f));
		if (Job.Progress < 1.0f)
		{
			continue;
		}
		if (bHoldLaunch)
		{
			TMap<TWeakObjectPtr<ASpaceShipyard>, double>& Held = APSFleetPrivate::HeldLaunches().FindOrAdd(this);
			if (!Held.Contains(Shipyard))
			{
				Held.Add(Shipyard, LiveWorld->GetTimeSeconds());
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s: %s finished, launch held while the world flows past the player ")
					TEXT("(aps.Fleet.HoldLaunchInFlight, travel deferred=%d)"), *DisplayName(Shipyard).ToString(), *Job.Name.ToString(),
					Origin->IsTravelDeferred() ? 1 : 0);
			}
			continue;
		}
		if (TMap<TWeakObjectPtr<ASpaceShipyard>, double>* Held = APSFleetPrivate::HeldLaunches().Find(this))
		{
			double HeldSince = 0.0;
			if (Held->RemoveAndCopyValue(Shipyard, HeldSince))
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s: launching %s, held %.1f s for a calm flight"),
					*DisplayName(Shipyard).ToString(), *Job.Name.ToString(), LiveWorld ? LiveWorld->GetTimeSeconds() - HeldSince : 0.0);
			}
		}
		// Above the shipyard, four abreast, a row per four ships.
		const int32 Slot = LaunchedAt.FindRef(Shipyard);
		constexpr double Spacing = 30000.0;
		const FVector Base = Shipyard->SpawnPoint ? Shipyard->SpawnPoint->GetComponentLocation() : Shipyard->GetActorLocation();
		const FVector Location = Base + Shipyard->GetActorRightVector() * ((Slot % 4) - 1.5) * Spacing
			+ Shipyard->GetActorUpVector() * (Slot / 4 + 1) * Spacing;
		const FAPSShipyardJob Launched = Job;
		ShipyardQueue.RemoveAt(Index--);
		ASpaceship* Ship = LaunchShip(Launched.ShipClass, FTransform(Shipyard->GetActorQuat(), Location), Shipyard);
		if (!Ship)
		{
			Post(FText::Format(LOCTEXT("LaunchFailed", "{0}: {1} could not be launched."), DisplayName(Shipyard), Launched.Name));
			continue;
		}
		// Rio 02.10: fixed 300 m slots put large builds into each other; the hull's own size decides.
		APSShipPlacement::PlaceClear(*Ship, Base, Shipyard->GetActorQuat());
		LaunchedAt.FindOrAdd(Shipyard) = Slot + 1;
		++LaunchedCount;
		RefreshUnits();
		const FAPSFleetUnit* Unit = FindUnit(Ship);
		Post(FText::Format(LOCTEXT("ShipLaunched", "{0} launched {1}: {2}, {3}."), DisplayName(Shipyard),
			FText::FromString(Unit ? Unit->CallSign : Ship->GetName()), Launched.Name,
			Unit ? APSFleet::DivisionName(Unit->Division) : FText::GetEmpty()));
	}
}

ASpaceship* FAPSFleetCommand::LaunchShip(const TSubclassOf<ASpaceship> ShipClass, const FTransform& Transform,
	ASpaceShipyard* Shipyard)
{
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || !ShipClass)
	{
		return nullptr;
	}
	const bool bLaunchedAtShipyard = Shipyard != nullptr;
	if (!Shipyard)
	{
		// A ship back from a save: the shipyard it waits at, if any is near.
		TArray<ASpaceShipyard*> Yards;
		GetShipyards(Yards);
		double Best = TNumericLimits<double>::Max();
		for (ASpaceShipyard* Candidate : Yards)
		{
			const double Distance = FVector::DistSquared(Candidate->GetActorLocation(), Transform.GetLocation());
			if (Distance < Best)
			{
				Best = Distance;
				Shipyard = Candidate;
			}
		}
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpaceship* Ship = LiveWorld->SpawnActor<ASpaceship>(ShipClass, Transform.GetLocation(), Transform.Rotator(), Parameters);
	if (!Ship)
	{
		return nullptr;
	}
	if (!bLaunchedAtShipyard)
	{
		// A ship back from a save keeps its place when that is clear; one saved inside another hull moves to the
		// nearest clear spot (Rio, 02.10: ships spawned into each other).
		// The anchor is where its hull stands now, so a clear save does not move at all.
		const FBox Hull = APSShipPlacement::HullBox(Ship);
		APSShipPlacement::PlaceClear(*Ship, Hull.IsValid ? Hull.GetCenter() : Ship->GetActorLocation(), Transform.GetRotation());
	}
	// As the generator readies an escort: the star system that moves the ship's frame, the civilization's tags.
	for (const FAPSFleetUnit& Unit : Units)
	{
		if (Unit.Ship.IsValid() && Unit.Ship->OffsetSystem)
		{
			Ship->OffsetSystem = Unit.Ship->OffsetSystem;
			if (Ship->OnboardComputer)
			{
				Ship->OnboardComputer->OffsetSystem = Unit.Ship->OffsetSystem;
			}
			break;
		}
	}
	Ship->Tags.AddUnique(CivilizationTag);
	Ship->Tags.AddUnique(UnitTag);
	Ship->Tags.AddUnique(BuiltTag);
	if (bLaunchedAtShipyard)
	{
		APSMissionsNotify(LiveWorld, APSMissions::EObjective::LaunchShip, ShipClass ? ShipClass->GetPathName() : FString());
	}
	if (Shipyard && FVector::Dist(Ship->GetActorLocation(), Shipyard->GetActorLocation()) < 2000000.0)
	{
		Ship->AttachToActor(Shipyard, FAttachmentTransformRules::KeepWorldTransform);
	}
	++Revision;
	return Ship;
}

// ---------------------------------------------------------------------------------------------------------------------
// Structures

ASpaceStation* FAPSFleetCommand::SpawnStructure(const APSFleet::EStructure Kind, APlanetaryBody* Body,
	const FTransform& Transform, const FText& Name, const FString& ActorName, const bool bCountCivilization)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || !Body)
	{
		return nullptr;
	}
	// The classes the home complex was raised with (the menu's station, shipyard and headquarters choices).
	TSubclassOf<ASpaceStation> Class;
	for (TActorIterator<AAstroGenerator> It(LiveWorld); It && !Class; ++It)
	{
		if (IsValid(*It))
		{
			Class = Kind == EStructure::Shipyard ? TSubclassOf<ASpaceStation>(It->BP_HomeSpaceShipyard)
				: Kind == EStructure::Headquarters ? TSubclassOf<ASpaceStation>(It->BP_HomeSpaceHeadquarters)
				: It->BP_HomeSpaceStation;
		}
	}
	if (!Class)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] no %s class in this world (no generator)"), *StructureName(Kind).ToString());
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!ActorName.IsEmpty())
	{
		Parameters.Name = FName(*ActorName);
		Parameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	}
	ASpaceStation* Station = LiveWorld->SpawnActor<ASpaceStation>(Class, Transform.GetLocation(), Transform.Rotator(), Parameters);
	if (!Station)
	{
		return nullptr;
	}
	Station->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	Station->CalculateAffectionRadius();
	Station->Tags.AddUnique(CivilizationTag);
	Station->Tags.AddUnique(StructureTag);
	Station->Tags.AddUnique(Kind == EStructure::Shipyard ? FName(TEXT("APS.Infrastructure.Shipyard"))
		: Kind == EStructure::Headquarters ? FName(TEXT("APS.Infrastructure.Headquarters"))
		: FName(TEXT("APS.Infrastructure.OrbitalStation")));
	if (ASpaceHeadquarters* Headquarters = Cast<ASpaceHeadquarters>(Station))
	{
		Headquarters->Civilization = Civilization();
	}
	// The world actor keeps its display name protected; it is a reflected property, so set it through reflection.
	if (FTextProperty* NameProperty = FindFProperty<FTextProperty>(Station->GetClass(), TEXT("InGameName")))
	{
		NameProperty->SetPropertyValue_InContainer(Station, Name);
	}
	// Rio 07.10: one standing again after its world was away was counted when it was built.
	if (UCivilization* Civ = bCountCivilization ? Civilization() : nullptr)
	{
		++Civ->Infrastructure.OrbitalStations;
	}
	FAPSFleetStructure& Record = Structures.AddDefaulted_GetRef();
	Record.Actor = Station;
	Record.Body = Body;
	Record.Kind = Kind;
	Record.bBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] structure %s (%s) at %s, %.0f km from its centre"), *Station->GetName(),
		*StructureName(Kind).ToString(), *NameOf(Body).ToString(),
		FVector::Dist(Station->GetActorLocation(), Body->GetActorLocation()) / 100000.0);
	++Revision;
	return Station;
}

void FAPSFleetCommand::RefreshStructures()
{
	using namespace APSFleet;
	Structures.Reset();
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	for (TActorIterator<ASpaceStation> It(LiveWorld); It; ++It)
	{
		APlanetaryBody* Body = IsValid(*It) ? OrbitedBody(*It) : nullptr;
		if (!Body)
		{
			continue;
		}
		FAPSFleetStructure& Record = Structures.AddDefaulted_GetRef();
		Record.Actor = *It;
		Record.Body = Body;
		Record.Kind = It->IsA<ASpaceShipyard>() ? EStructure::Shipyard
			: It->IsA<ASpaceHeadquarters>() ? EStructure::Headquarters : EStructure::Station;
		Record.bBuilt = It->ActorHasTag(APSFleetPrivate::StructureTag);
	}
}

int32 FAPSFleetCommand::CountStructures(const AActor* Body, const APSFleet::EStructure Kind) const
{
	int32 Count = 0;
	for (const FAPSFleetStructure& Structure : Structures)
	{
		Count += Body && Structure.Kind == Kind && Structure.Actor.IsValid() && Structure.Body.Get() == Body ? 1 : 0;
	}
	// Rio 03.10: a catalogue hub over the world is a station of a higher tier (the foothold, the base for a shipyard or a
	// HQ, a faster crew there).
	if (Kind == APSFleet::EStructure::Station && Body)
	{
		if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get()))
		{
			TArray<const FAPSBuiltStructure*> Here;
			Infrastructure->GetAt(Body, Here);
			for (const FAPSBuiltStructure* Built : Here)
			{
				const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built->Type);
				Count += Type && Type->Category == APSInfrastructure::ECategory::Hub && Built->Actor.IsValid() ? 1 : 0;
			}
		}
	}
	return Count;
}

int32 FAPSFleetCommand::CountBuiltHeadquarters() const
{
	int32 Count = 0;
	for (const FAPSFleetStructure& Structure : Structures)
	{
		Count += Structure.bBuilt && Structure.Kind == APSFleet::EStructure::Headquarters && Structure.Actor.IsValid() ? 1 : 0;
	}
	// Rio 07.10: an HQ on a world that is away (its star system released) still speeds the fleet up.
	for (const TPair<FGuid, FAPSFleetSaveData::FStructure>& Held : HeldStructures)
	{
		Count += Held.Value.Kind == static_cast<uint8>(APSFleet::EStructure::Headquarters) ? 1 : 0;
	}
	return Count;
}

APlanetaryBody* FAPSFleetCommand::OrbitedBody(const AActor* Actor)
{
	for (AActor* Parent = Actor ? Actor->GetAttachParentActor() : nullptr; Parent; Parent = Parent->GetAttachParentActor())
	{
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(Parent))
		{
			return Body;
		}
	}
	return nullptr;
}

void FAPSFleetCommand::HoldOnRelease(const TArray<APlanetaryBody*>& ReleasedBodies, const FGuid& HostSystemId,
	FAPSStarSystems& Stars, const AActor* SystemRoot)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	if (!FAPSInfrastructure::HoldsReleased() || (ReleasedBodies.IsEmpty() && !SystemRoot) || !HostSystemId.IsValid())
	{
		return;
	}
	TSet<const AActor*> Released;
	for (const APlanetaryBody* Body : ReleasedBodies)
	{
		if (Body)
		{
			Released.Add(Body);
		}
	}
	const FAPSStarSystemInfo* Info = Stars.Find(HostSystemId);
	const FString SystemName = Info ? Info->Name : HostSystemId.ToString(EGuidFormats::Digits);
	int32 Changed = 0;
	// The ships there (attached to the world through their target, a station or the shipyard that launched them): to the
	// system's beacon where they are, before the tree goes. Their order then ends next tick as for any lost target
	// (CancelOrder: a build's cost is returned). The beacon is spawned only when a ship needs it (navigation charts it).
	// Rio 07.10 (review: a ship holding or working at the star itself, or at an orbit of the tree, has no world in its attach
	// chain and was destroyed with the tree, its order never cancelled, its build's cost gone): any ship in the tree too.
	AActor* Anchor = nullptr;
	for (FAPSFleetUnit& Unit : Units)
	{
		ASpaceship* Ship = Unit.Ship.Get();
		const APlanetaryBody* AtWorld = Ship ? OrbitedBody(Ship) : nullptr;
		if (!Ship || (!Released.Contains(AtWorld) && !(SystemRoot && Ship->IsAttachedTo(SystemRoot))))
		{
			continue;
		}
		Anchor = Anchor ? Anchor : Stars.GetAnchor(HostSystemId);
		if (!Anchor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] %s has no beacon: %s goes with its world"), *SystemName, *Unit.CallSign);
			continue;
		}
		Ship->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Ship->AttachToActor(Anchor, FAttachmentTransformRules::KeepWorldTransform);
		++Changed;
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s held: its %s went with %s; it waits at the system's beacon"), *Unit.CallSign,
			AtWorld ? TEXT("world") : TEXT("star"), *SystemName);
	}
	// The stations, shipyards and HQs the fleet built there, as a save writes them.
	for (const FAPSFleetStructure& Structure : Structures)
	{
		const AActor* Actor = Structure.Actor.Get();
		const APlanetaryBody* Body = Structure.Body.Get();
		if (!Structure.bBuilt || !Actor || !Body || !Released.Contains(Body))
		{
			continue;
		}
		FAPSFleetSaveData::FStructure Saved;
		Saved.Kind = static_cast<uint8>(Structure.Kind);
		Saved.BodyKey = KeyOf(Body);
		Saved.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Body->GetActorTransform());
		Saved.Name = IItemInfoInterface::Execute_GetInGameName(Actor).ToString();
		Saved.ActorName = Actor->GetName();
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %s held: its world went with %s (%s)"), *Saved.ActorName, *SystemName, *Saved.BodyKey);
		HeldStructures.Emplace(HostSystemId, MoveTemp(Saved));
		++Changed;
	}
	// The worlds' records (surveys, anomalies) stay, by key and system, with their outposts held.
	int32 Records = 0;
	for (FAPSFleetBodyRecord& Record : Bodies)
	{
		const APlanetaryBody* Body = Record.Body.Get();
		if (!Body || !Released.Contains(Body))
		{
			continue;
		}
		for (const TWeakObjectPtr<AActor>& Outpost : Record.Outposts)
		{
			if (const AActor* Standing = Outpost.Get())
			{
				FAPSFleetSaveData::FOutpost Saved;
				Saved.BodyKey = KeyOf(Body);
				Saved.RelativeTransform = Standing->GetActorTransform().GetRelativeTransform(Body->GetActorTransform());
				Saved.Name = NameOf(Standing).ToString();
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] outpost %s held: its world went with %s (%s)"), *Saved.Name, *SystemName,
					*Saved.BodyKey);
				HeldOutposts.Emplace(HostSystemId, MoveTemp(Saved));
			}
		}
		Record.Outposts.Reset();
		Record.Key = KeyOf(Body);
		Record.HostSystemId = HostSystemId;
		++Records;
	}
	if (Records > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] %d world record(s) held: their worlds went with %s"), Records, *SystemName);
	}
	if (Changed + Records > 0)
	{
		++Revision;
	}
}

void FAPSFleetCommand::RaiseHeldWorlds()
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	UWorld* LiveWorld = World.Get();
	const auto IsKept = [](const FAPSFleetBodyRecord& Record) { return !Record.Body.IsValid() && !Record.Key.IsEmpty(); };
	if (!LiveWorld || !FAPSInfrastructure::HoldsReleased()
		|| (HeldStructures.IsEmpty() && HeldOutposts.IsEmpty() && !Bodies.ContainsByPredicate(IsKept)))
	{
		return;
	}
	// A released system's: only on its own bodies while it stands. Held at load (no system): by the world's name, the first
	// body of a key, as the restore takes it.
	TMap<FString, APlanetaryBody*> HostBodies;
	const FGuid ActiveHost = FleetHostBodies(LiveWorld, &HostBodies);
	const bool bAnyUnscoped = Bodies.ContainsByPredicate([&IsKept](const FAPSFleetBodyRecord& Record)
		{
			return IsKept(Record) && !Record.HostSystemId.IsValid();
		})
		|| HeldStructures.ContainsByPredicate([](const TPair<FGuid, FAPSFleetSaveData::FStructure>& Held) { return !Held.Key.IsValid(); })
		|| HeldOutposts.ContainsByPredicate([](const TPair<FGuid, FAPSFleetSaveData::FOutpost>& Held) { return !Held.Key.IsValid(); });
	TMap<FString, APlanetaryBody*> AllBodies;
	if (bAnyUnscoped)
	{
		for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
		{
			const FString Key = IsValid(*It) ? KeyOf(*It) : FString();
			if (!Key.IsEmpty() && !AllBodies.Contains(Key))
			{
				AllBodies.Add(Key, *It);
			}
		}
	}
	const auto Resolve = [&HostBodies, &AllBodies, &ActiveHost](const FGuid& Host, const FString& Key) -> APlanetaryBody*
	{
		if (Host.IsValid())
		{
			return ActiveHost.IsValid() && Host == ActiveHost ? HostBodies.FindRef(Key) : nullptr;
		}
		return AllBodies.FindRef(Key);
	};
	// The worlds' records first: the outposts below go into them.
	for (FAPSFleetBodyRecord& Record : Bodies)
	{
		if (!IsKept(Record))
		{
			continue;
		}
		APlanetaryBody* Body = Resolve(Record.HostSystemId, Record.Key);
		if (Body && !FindBody(Body))
		{
			RebindBody(Record, Body);
		}
	}
	int32 StructuresRaised = 0;
	for (int32 Index = 0; Index < HeldStructures.Num() && StructuresRaised < HeldStructureRaisesPerSecond; ++Index)
	{
		APlanetaryBody* Body = Resolve(HeldStructures[Index].Key, HeldStructures[Index].Value.BodyKey);
		if (!Body)
		{
			continue;
		}
		const FAPSFleetSaveData::FStructure Saved = HeldStructures[Index].Value;
		// Rio 07.10 (review): the civilization counted one held in this session when it was built; one held at load (no
		// system) was not (its counters are not saved: the restore counts what it raises), so it is counted now.
		const bool bCount = !HeldStructures[Index].Key.IsValid();
		HeldStructures.RemoveAt(Index--);
		++StructuresRaised;
		const ASpaceStation* Station = Saved.Kind < static_cast<uint8>(EStructure::Count)
			? SpawnStructure(static_cast<EStructure>(Saved.Kind), Body, Saved.RelativeTransform * Body->GetActorTransform(),
				FText::FromString(Saved.Name), Saved.ActorName, bCount)
			: nullptr;
		if (Station)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] held structure %s raised at %s"), *Station->GetName(), *Saved.BodyKey);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] held structure %s could not be raised at %s"), *Saved.ActorName, *Saved.BodyKey);
		}
	}
	int32 OutpostsRaised = 0;
	for (int32 Index = 0; Index < HeldOutposts.Num() && OutpostsRaised < HeldOutpostRaisesPerSecond; ++Index)
	{
		APlanetaryBody* Body = Resolve(HeldOutposts[Index].Key, HeldOutposts[Index].Value.BodyKey);
		if (!Body)
		{
			continue;
		}
		const FAPSFleetSaveData::FOutpost Saved = HeldOutposts[Index].Value;
		const bool bCount = !HeldOutposts[Index].Key.IsValid();
		HeldOutposts.RemoveAt(Index--);
		++OutpostsRaised;
		const FTransform Transform = Saved.RelativeTransform * Body->GetActorTransform();
		if (const AActor* Outpost = SpawnOutpost(Body, Transform.GetLocation(), Transform.GetRotation(), FText::FromString(Saved.Name), bCount))
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] held outpost %s raised at %s"), *Outpost->GetName(), *Saved.BodyKey);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] held outpost %s could not be raised at %s"), *Saved.Name, *Saved.BodyKey);
		}
	}
}

void FAPSFleetCommand::RebindBody(FAPSFleetBodyRecord& Record, APlanetaryBody* Body)
{
	using namespace APSFleet;
	using namespace APSFleetPrivate;
	if (!Body)
	{
		return;
	}
	Record.Body = Body;
	if (!Record.HostSystemId.IsValid())
	{
		// Kept from a save while its world was away: the restore's steps now (its findings, its anomaly rolled and revealed
		// to the saved level, its investigation); from now on an ordinary record.
		Record.Findings.Reset();
		if (Record.Survey > ESurvey::Unknown)
		{
			AddFindings(Body, ESurvey::Unknown, Record.Survey, Record.Findings);
		}
		RollAnomaly(Record, Body);
		RevealAnomaly(Record, Record.Survey, FText::GetEmpty(), false);
		if (Record.HeldInvestigation > 0)
		{
			InvestigateAnomaly(Record, FText::GetEmpty(), Record.HeldInvestigation == 2, false);
		}
		Record.HeldInvestigation = 0;
		Record.Key.Reset();
	}
	else if (Record.bHasAnomaly && Record.Anomaly >= EAnomalyState::Located)
	{
		// Its beacon went with the world: at the site again, named as it was.
		SpawnAnomalyBeacon(Record);
		AActor* Beacon = Record.AnomalyBeacon.Get();
		FTextProperty* NameProperty = Beacon && Record.Anomaly == EAnomalyState::Investigated
			? FindFProperty<FTextProperty>(Beacon->GetClass(), TEXT("InGameName")) : nullptr;
		if (NameProperty)
		{
			NameProperty->SetPropertyValue_InContainer(Beacon, FText::Format(LOCTEXT("BeaconInvestigated", "INVESTIGATED: {0}"),
				AnomalyName(Record.AnomalyKind)));
		}
	}
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] world %s raised: its record is back (%s)"), *KeyOf(Body),
		*SurveyName(Record.Survey).ToString());
}

FText FAPSFleetCommand::DisplayName(const AActor* Actor)
{
	return APSFleetPrivate::NameOf(Actor);
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
		TEXT("aps.Fleet.Order <all|call sign|division> <move|survey|expedition|outpost|station|shipyard|hq|return|cancel> [target name, or moon/planet/station]"),
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
				: Verb.Equals(TEXT("station"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::BuildStation
				: Verb.Equals(TEXT("shipyard"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::BuildShipyard
				: Verb.Equals(TEXT("hq"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::BuildHeadquarters
				: Verb.Equals(TEXT("expedition"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::Expedition
				: Verb.Equals(TEXT("return"), ESearchCase::IgnoreCase) ? APSFleet::EOrder::Return : APSFleet::EOrder::None;
			FText Refusal;
			const int32 Issued = Fleet->ConsoleOrder(Args[0], Order, Args.Num() > 2 ? Args[2] : FString(), Refusal);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] order %s %s -> %d ship(s)%s%s"), *Args[0], *Verb, Issued,
				Refusal.IsEmpty() ? TEXT("") : TEXT("; refused: "), *Refusal.ToString());
		}));

	FAutoConsoleCommandWithWorld AnomaliesCommand(TEXT("aps.Fleet.Anomalies"),
		TEXT("Logs the anomaly every planet and moon hides (kind, state, site) and its beacon."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (FAPSFleetCommand* Fleet = APSFleetFind(World))
			{
				Fleet->LogAnomalies();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs ShipyardCommand(TEXT("aps.Fleet.Shipyard"),
		TEXT("aps.Fleet.Shipyard [ship name] [shipyard number]: lists the shipyards and their slipways, or lays a catalogue ")
		TEXT("ship down at one (1 = the home shipyard; underscores stand for spaces)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			FAPSFleetCommand* Fleet = APSFleetFind(World);
			if (!Fleet)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Fleet] no fleet command in this world"));
				return;
			}
			TArray<ASpaceShipyard*> Yards;
			Fleet->GetShipyards(Yards);
			if (Args.Num() > 0)
			{
				TArray<FAPSShipyardOption> Options;
				Fleet->GetShipyardOptions(Options);
				const FString Wanted = Args[0].Replace(TEXT("_"), TEXT(" "));
				const FAPSShipyardOption* Option = Options.FindByPredicate([&Wanted](const FAPSShipyardOption& Candidate)
				{
					return Candidate.Name.ToString().Contains(Wanted, ESearchCase::IgnoreCase);
				});
				const int32 YardIndex = Args.Num() > 1 ? FCString::Atoi(*Args[1]) - 1 : 0;
				const FText Refusal = Option
					? Fleet->OrderShip(*Option, Yards.IsValidIndex(YardIndex) ? Yards[YardIndex] : nullptr)
					: FText::FromString(TEXT("no such ship in the catalogue"));
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] shipyard order %s at %d: %s"), *Args[0], YardIndex + 1,
					Refusal.IsEmpty() ? TEXT("laid down") : *Refusal.ToString());
			}
			for (int32 Index = 0; Index < Yards.Num(); ++Index)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] shipyard %d %s at %s: %d queued, %d launched"), Index + 1,
					*FAPSFleetCommand::DisplayName(Yards[Index]).ToString(),
					*FAPSFleetCommand::DisplayName(FAPSFleetCommand::OrbitedBody(Yards[Index])).ToString(),
					Fleet->CountQueued(Yards[Index]), Fleet->GetLaunchedCount(Yards[Index]));
			}
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
		/** build: after the outpost a station, then a shipyard there, then a ship laid down and launched at it. */
		bool bBuild{false};
		/** anomaly: science studies a world that hides an anomaly (its site gets located), then an expedition goes down. */
		bool bAnomaly{false};
		TWeakObjectPtr<ASpaceShipyard> Yard;
		int32 LaunchedBefore{0};
		/** build: every terminal tab is shot once at the end (shipyards, journal, divisions with the chain's results). */
		bool bTabsShot{false};
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
			if (GAutoTest.bAnomaly)
			{
				// The first world (not home) that hides an anomaly; the science ships study it, which locates the site.
				Fleet->LogAnomalies();
				APlanetaryBody* Hidden = nullptr;
				for (TActorIterator<APlanetaryBody> It(World); It && !Hidden; ++It)
				{
					const FAPSFleetBodyRecord* Record = Fleet->FindBody(*It);
					Hidden = Record && Record->bHasAnomaly && Record->Anomaly == EAnomalyState::Hidden ? *It : nullptr;
				}
				if (!Hidden)
				{
					return Finish(TEXT("no world hides an anomaly"));
				}
				TArray<ASpaceship*> Science;
				for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
				{
					if (Unit.Division == EDivision::Science && Unit.Ship.IsValid())
					{
						Science.Add(Unit.Ship.Get());
					}
				}
				const int32 Issued = Fleet->IssueOrder(Science, EOrder::Survey, Hidden, Refusal);
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: science study of %s for its anomaly -> %d ship(s) %s"),
					*Hidden->GetName(), Issued, *Refusal.ToString());
				GAutoTest.SurveyTarget = Hidden;
				GAutoTest.Stage = Issued > 0 ? 10 : 3;
				GAutoTest.StageSeconds = Now + 6.0;
				if (UAPSColonyTerminalSubsystem* Terminal = World->GetSubsystem<UAPSColonyTerminalSubsystem>();
					Terminal && !Terminal->IsTerminalOpen())
				{
					Terminal->ToggleFleetCommand();
				}
				GAutoTest.NextShot = Now + 3.0;
				break;
			}
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
				GAutoTest.Stage = GAutoTest.bBuild ? 4 : 3;
				GAutoTest.StageSeconds = Now + 6.0;
			}
			break;
		case 4:
		case 5:
		{
			// The builder holds where it raised the outpost: a station next, then (once it stands) a shipyard.
			if (GAutoTest.Stage == 5 && Fleet->CountStructures(GAutoTest.SurveyTarget.Get(), EStructure::Station) == 0)
			{
				break;
			}
			const EOrder Next = GAutoTest.Stage == 4 ? EOrder::BuildStation : EOrder::BuildShipyard;
			TArray<ASpaceship*> Builders;
			for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
			{
				if (Unit.Division == EDivision::Construction && Unit.Ship.IsValid()
					&& (Unit.Order == EOrder::None || Unit.Phase == EPhase::Holding))
				{
					Builders.Add(Unit.Ship.Get());
					break;
				}
			}
			const int32 Issued = Fleet->IssueOrder(Builders, Next, GAutoTest.SurveyTarget.Get(), Refusal);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: %s -> %d ship(s) %s"), *OrderName(Next).ToString(), Issued,
				*Refusal.ToString());
			if (Issued == 0)
			{
				return Finish(TEXT("construction refused"));
			}
			GAutoTest.Stage = GAutoTest.Stage == 4 ? 5 : 6;
			break;
		}
		case 6:
			for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
			{
				if (Structure.Kind == EStructure::Shipyard && Structure.bBuilt
					&& Structure.Body.Get() == GAutoTest.SurveyTarget.Get())
				{
					GAutoTest.Yard = Cast<ASpaceShipyard>(Structure.Actor.Get());
				}
			}
			if (ASpaceShipyard* Yard = GAutoTest.Yard.Get())
			{
				TArray<FAPSShipyardOption> Options;
				Fleet->GetShipyardOptions(Options);
				GAutoTest.LaunchedBefore = Fleet->GetLaunchedCount(Yard);
				const FText Laid = Options.IsEmpty() ? FText::FromString(TEXT("no catalogue")) : Fleet->OrderShip(Options[0], Yard);
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: ship at the new shipyard %s: %s"), *Yard->GetName(),
					Laid.IsEmpty() ? TEXT("laid down") : *Laid.ToString());
				if (!Laid.IsEmpty())
				{
					return Finish(TEXT("the new shipyard refused the ship"));
				}
				GAutoTest.Stage = 7;
			}
			break;
		case 7:
			if (Fleet->GetLaunchedCount(GAutoTest.Yard.Get()) > GAutoTest.LaunchedBefore)
			{
				GAutoTest.Stage = 8;
				GAutoTest.StageSeconds = Now + 6.0;
				Fleet->LogAnomalies();
			}
			break;
		case 8:
		{
			// An anomaly located on the surveyed world (the science ship studied it): an expedition investigates it.
			const FAPSFleetBodyRecord* Record = Fleet->FindBody(GAutoTest.SurveyTarget.Get());
			if (!Record || !Record->bHasAnomaly || Record->Anomaly != EAnomalyState::Located)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: no located anomaly on %s (state %d), no expedition"),
					*GetNameSafe(GAutoTest.SurveyTarget.Get()), Record ? static_cast<int32>(Record->Anomaly) : -1);
				GAutoTest.Stage = 3;
				break;
			}
			TArray<ASpaceship*> Crew;
			for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
			{
				if ((Unit.Division == EDivision::Science || Unit.Division == EDivision::Exploration) && Unit.Ship.IsValid()
					&& (Unit.Order == EOrder::None || Unit.Phase == EPhase::Holding))
				{
					Crew.Add(Unit.Ship.Get());
					break;
				}
			}
			const int32 Issued = Fleet->IssueOrder(Crew, EOrder::Expedition, GAutoTest.SurveyTarget.Get(), Refusal);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest: expedition -> %d ship(s) %s"), Issued, *Refusal.ToString());
			GAutoTest.Stage = Issued > 0 ? 9 : 3;
			break;
		}
		case 9:
			if (const FAPSFleetBodyRecord* Record = Fleet->FindBody(GAutoTest.SurveyTarget.Get());
				Record && Record->Anomaly == EAnomalyState::Investigated)
			{
				Fleet->LogAnomalies();
				GAutoTest.Stage = 3;
				GAutoTest.StageSeconds = Now + 6.0;
			}
			break;
		case 10:
			// The study located the site: the expedition next (stage 8 picks a free exploration or science ship).
			if (const FAPSFleetBodyRecord* Record = Fleet->FindBody(GAutoTest.SurveyTarget.Get());
				Record && Record->Anomaly == EAnomalyState::Located)
			{
				Fleet->LogAnomalies();
				GAutoTest.Stage = 8;
			}
			break;
		default:
			if (Now >= GAutoTest.StageSeconds)
			{
				if ((GAutoTest.bBuild || GAutoTest.bAnomaly) && !GAutoTest.bTabsShot)
				{
					GAutoTest.bTabsShot = true;
					if (UAPSColonyTerminalSubsystem* Terminal = World->GetSubsystem<UAPSColonyTerminalSubsystem>())
					{
						Terminal->StartTestCaptures();
					}
					GAutoTest.StageSeconds = Now + 22.0;
					break;
				}
				Fleet->LogUnits();
				AutoTestShot();
				return Finish(GAutoTest.bAnomaly ? TEXT("anomaly located and investigated")
					: GAutoTest.bBuild ? TEXT("station, shipyard and its first ship built") : TEXT("outpost built"));
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
		TEXT("aps.Fleet.AutoTest <label> [quit] [build]: survey, study and build an outpost with the fleet, shooting fleet ")
		TEXT("command; build goes on to a station, a shipyard there and a ship launched from it; anomaly has science study a ")
		TEXT("world that hides an anomaly and sends an expedition to its site."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			if (GAutoTest.bActive)
			{
				FTSTicker::GetCoreTicker().RemoveTicker(GAutoTest.Ticker);
			}
			GAutoTest = FAutoTest();
			GAutoTest.bActive = true;
			GAutoTest.Label = Args.Num() > 0 ? Args[0] : FString(TEXT("fleet"));
			for (int32 Index = 1; Index < Args.Num(); ++Index)
			{
				GAutoTest.bQuit |= Args[Index].Equals(TEXT("quit"), ESearchCase::IgnoreCase);
				GAutoTest.bBuild |= Args[Index].Equals(TEXT("build"), ESearchCase::IgnoreCase);
				GAutoTest.bAnomaly |= Args[Index].Equals(TEXT("anomaly"), ESearchCase::IgnoreCase);
			}
			GAutoTest.Started = FPlatformTime::Seconds();
			GAutoTest.Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickAutoTest), 0.25f);
			UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] autotest armed: %s%s"), *GAutoTest.Label, GAutoTest.bQuit ? TEXT(" (quit)") : TEXT(""));
		}));
}

#undef LOCTEXT_NAMESPACE
