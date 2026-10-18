#include "APSSpaceTrips.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Enums/PlanetHabitability.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncients.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncientsTypes.h"
#include "APS_ALPHA/Gameplay/Trips/APSSpaceTripsHud.h"
#include "APS_ALPHA/UI/Colony/APSMissionTracker.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightBenchmark.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "APSSpaceTrips"

namespace APSSpaceTripsLocal
{
	using namespace APSSpaceTrips;

	TAutoConsoleVariable<int32> CVarEnable(TEXT("aps.Trip.Enable"), 1,
		TEXT("1: SPACE TRIPS runs in a world without civilization goals (the guide, the routes, the journal). 0: asleep; the ")
		TEXT("aps.Trip.* commands still work in any world."));
	TAutoConsoleVariable<int32> CVarAutoStart(TEXT("aps.Trip.AutoStart"), 1,
		TEXT("1: in a SPACE TRIPS world the first route (HOME WATERS) starts by itself once the pilot sits in a ship with the ")
		TEXT("engine on. 0: aps.Trip.Start only."));
	TAutoConsoleVariable<float> CVarHoldSeconds(TEXT("aps.Trip.HoldSeconds"), 25.0f,
		TEXT("How long the guide holds the ship near each world for the view, seconds."));
	TAutoConsoleVariable<float> CVarTurnRate(TEXT("aps.Trip.TurnRate"), 15.0f,
		TEXT("How fast the guide turns the nose onto the next star with the star drive on, degrees per second."));
	TAutoConsoleVariable<float> CVarFoldAfterSeconds(TEXT("aps.Trip.FoldAfterSeconds"), 120.0f,
		TEXT("A star leg that has not arrived after this many seconds (a hull without a star drive, a drive that keeps dropping) ")
		TEXT("folds the distance: the ship is set at the system's edge, as aps.Stars.Visit does. 0: never."));
	TAutoConsoleVariable<int32> CVarWorldsPerStop(TEXT("aps.Trip.WorldsPerStop"), 3,
		TEXT("How many worlds of a foreign star system a route shows (the home system shows all)."));
	TAutoConsoleVariable<float> CVarResumeAfterSeconds(TEXT("aps.Trip.ResumeAfterSeconds"), 6.0f,
		TEXT("A route the pilot's hand paused goes on by itself once the pilot sits with the engine on, no course of their ")
		TEXT("own, and the helm still for this many seconds. 0: aps.Trip.Resume only (aps.Trip.Pause always waits for it)."));

	const FName GuideCategory(TEXT("Guide"));
	constexpr double LightYearCm = 9.4607e17;
	constexpr float AutoStartGraceSeconds = 6.0f;
	/** The helm counts as still while the nose turns less than this between looks, degrees (a drifting hull wobbles). */
	constexpr float HelmStillDegrees = 1.5f;
	constexpr float ArrivingTimeoutSeconds = 60.0f;
	/** The route card left open this long at the start: the home route begins by itself. */
	constexpr float CardAutoStartSeconds = 45.0f;
	constexpr double HelmTurnDegrees = 2.0;

	struct FWorldTrip
	{
		TWeakObjectPtr<UWorld> World;
		TUniquePtr<FTrip> Trip;
	};

	TArray<FWorldTrip>& Trips()
	{
		static TArray<FWorldTrip> Entries;
		return Entries;
	}

	void ForgetWorld(UWorld* World, bool, bool)
	{
		Trips().RemoveAll([World](const FWorldTrip& Entry) { return !Entry.World.IsValid() || Entry.World.Get() == World; });
	}

	/** The leading letters of a catalogue spectral string ("K4V" -> "K", "BH" -> "BH"). */
	FString SpectralLetters(const FString& Spectral)
	{
		FString Letters;
		for (const TCHAR Char : Spectral)
		{
			if (!FChar::IsAlpha(Char))
			{
				break;
			}
			Letters.AppendChar(Char);
		}
		return Letters;
	}

	/** The primary's model in the cluster's record (null for a galaxy star, which carries its string). */
	const FStarModel* ClusterModel(const FAPSStarSystems& Stars, const FAPSStarSystemInfo& Info)
	{
		const AStarCluster* Cluster = Stars.GetCluster();
		return Info.GalaxyIndex == INDEX_NONE && Cluster && Cluster->PotentialStarSystems.IsValidIndex(Info.Record)
			? &Cluster->PotentialStarSystems[Info.Record].PrimaryStarModel : nullptr;
	}

	/** Rio 19.10 (ark-t/trip review: RED, BLUE and THE DARK were nearly empty): the class letters of any star of the catalogue. */
	FString SpectralOf(const FAPSStarSystems& Stars, const FAPSStarSystemInfo& Info)
	{
		const FString Letters = SpectralLetters(Info.Spectral);
		if (!Letters.IsEmpty())
		{
			return Letters;
		}
		const FStarModel* Model = ClusterModel(Stars, Info);
		if (!Model)
		{
			return Letters;
		}
		switch (Model->StellarType)
		{
		case EStellarType::BlackHole: return TEXT("BH");
		case EStellarType::Neutron:
		case EStellarType::Pulsar: return TEXT("NS");
		case EStellarType::WhiteDwarf: return TEXT("D");
		default: break;
		}
		const FString Name = StaticEnum<ESpectralClass>()->GetNameStringByValue(static_cast<int64>(Model->SpectralClass));
		return Name == TEXT("Unknown") ? FString() : Name;
	}

	bool IsGiantStar(const FAPSStarSystems& Stars, const FAPSStarSystemInfo& Info)
	{
		const FStarModel* Model = ClusterModel(Stars, Info);
		return Model && (Model->StellarType == EStellarType::Giant || Model->StellarType == EStellarType::BrightGiant
			|| Model->StellarType == EStellarType::SuperGiant || Model->StellarType == EStellarType::HyperGiant);
	}

	FText TypeName(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Rocky: return LOCTEXT("TypeRocky", "ROCKY WORLD");
		case EPlanetType::Terrestrial: return LOCTEXT("TypeTerrestrial", "TERRESTRIAL WORLD");
		case EPlanetType::Greenhouse: return LOCTEXT("TypeGreenhouse", "GREENHOUSE WORLD");
		case EPlanetType::Melted: return LOCTEXT("TypeMelted", "MOLTEN WORLD");
		case EPlanetType::HotGiant: return LOCTEXT("TypeHotGiant", "HOT GIANT");
		case EPlanetType::GasGiant: return LOCTEXT("TypeGasGiant", "GAS GIANT");
		case EPlanetType::IceGiant: return LOCTEXT("TypeIceGiant", "ICE GIANT");
		case EPlanetType::Dwarf: return LOCTEXT("TypeDwarf", "DWARF WORLD");
		case EPlanetType::Ocean:
		case EPlanetType::Water: return LOCTEXT("TypeOcean", "OCEAN WORLD");
		case EPlanetType::Desert: return LOCTEXT("TypeDesert", "DESERT WORLD");
		case EPlanetType::Forest: return LOCTEXT("TypeForest", "FOREST WORLD");
		case EPlanetType::Volcanic: return LOCTEXT("TypeVolcanic", "VOLCANIC WORLD");
		case EPlanetType::Ice:
		case EPlanetType::Frozen: return LOCTEXT("TypeIce", "ICE WORLD");
		case EPlanetType::Ammonia: return LOCTEXT("TypeAmmonia", "AMMONIA WORLD");
		case EPlanetType::Metal:
		case EPlanetType::Metallic: return LOCTEXT("TypeMetal", "METAL WORLD");
		case EPlanetType::Carbon: return LOCTEXT("TypeCarbon", "CARBON WORLD");
		case EPlanetType::SuperEarth: return LOCTEXT("TypeSuperEarth", "SUPER-EARTH");
		case EPlanetType::Lava: return LOCTEXT("TypeLava", "LAVA WORLD");
		case EPlanetType::Nordic: return LOCTEXT("TypeNordic", "NORDIC WORLD");
		case EPlanetType::Tundra: return LOCTEXT("TypeTundra", "TUNDRA WORLD");
		case EPlanetType::HighMountain: return LOCTEXT("TypeHighMountain", "MOUNTAIN WORLD");
		case EPlanetType::Sand: return LOCTEXT("TypeSand", "SAND WORLD");
		case EPlanetType::Oasis: return LOCTEXT("TypeOasis", "OASIS WORLD");
		case EPlanetType::Archipelago: return LOCTEXT("TypeArchipelago", "ARCHIPELAGO WORLD");
		case EPlanetType::Pangea: return LOCTEXT("TypePangea", "PANGEA WORLD");
		case EPlanetType::Rogue: return LOCTEXT("TypeRogue", "ROGUE WORLD");
		case EPlanetType::Basalt: return LOCTEXT("TypeBasalt", "BASALT WORLD");
		case EPlanetType::Savanna: return LOCTEXT("TypeSavanna", "SAVANNA WORLD");
		case EPlanetType::Sulfur: return LOCTEXT("TypeSulfur", "SULFUR WORLD");
		case EPlanetType::Crystal: return LOCTEXT("TypeCrystal", "CRYSTAL WORLD");
		default: return LOCTEXT("TypeWorld", "WORLD");
		}
	}

	/** One line of the guidebook per kind of world (Rio 09.10: meditative, no numbers to beat). */
	FText Wonder(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::HotGiant:
		case EPlanetType::GasGiant:
			return LOCTEXT("WonderGiant", "No ground anywhere. The bands are storms older than any colony.");
		case EPlanetType::IceGiant:
			return LOCTEXT("WonderIceGiant", "Blue from methane, cold enough to freeze the air. Its winds are the fastest in the system.");
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return LOCTEXT("WonderOcean", "Water from pole to pole. Look for the storms; they are the size of countries.");
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Savanna:
		case EPlanetType::Pangea:
		case EPlanetType::Oasis:
			return LOCTEXT("WonderLiving", "Green under the clouds. A world that would take us in.");
		case EPlanetType::Desert:
		case EPlanetType::Sand:
		case EPlanetType::Tundra:
		case EPlanetType::Nordic:
		case EPlanetType::HighMountain:
			return LOCTEXT("WonderDry", "Dry, old and quiet. The wind has had this place to itself.");
		case EPlanetType::Volcanic:
		case EPlanetType::Lava:
		case EPlanetType::Melted:
		case EPlanetType::Sulfur:
		case EPlanetType::Basalt:
			return LOCTEXT("WonderFire", "The crust has not set. Rivers of light where the night side is.");
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Ammonia:
			return LOCTEXT("WonderIce", "A frozen sea with a sky. Nothing here hurries.");
		case EPlanetType::Greenhouse:
			return LOCTEXT("WonderGreenhouse", "Clouds that never part, and under them a heat that would melt lead.");
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
			return LOCTEXT("WonderMetal", "The core of a world that lost its mantle. It rings when struck.");
		case EPlanetType::Carbon:
			return LOCTEXT("WonderCarbon", "Graphite plains and, deeper, diamond. Black in daylight.");
		case EPlanetType::Crystal:
			return LOCTEXT("WonderCrystal", "The ground grows in facets. At dawn the whole hemisphere lights up.");
		case EPlanetType::SuperEarth:
			return LOCTEXT("WonderSuperEarth", "Twice our gravity. The mountains are low because they have to be.");
		case EPlanetType::Dwarf:
			return LOCTEXT("WonderDwarf", "Too small to keep an air. Every crater is as old as the system.");
		case EPlanetType::Rogue:
			return LOCTEXT("WonderRogue", "A world without a sun, lit only by what it keeps inside.");
		default:
			return LOCTEXT("WonderWorld", "A world. Someone will name its mountains.");
		}
	}

	FText LookFor(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::HotGiant:
		case EPlanetType::GasGiant:
			return LOCTEXT("LookGiant", "Look along the terminator: the bands fold into storms there, and the moons cast black dots on the clouds.");
		case EPlanetType::IceGiant:
			return LOCTEXT("LookIceGiant", "Against the blue, look for a moon's shadow crossing the cloud deck.");
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return LOCTEXT("LookOcean", "Watch the sunlit limb: the sea throws the star back like a mirror.");
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Savanna:
		case EPlanetType::Pangea:
		case EPlanetType::Oasis:
			return LOCTEXT("LookLiving", "On the night side, look for lightning under the clouds; at the limb, the thin blue shell of the air.");
		case EPlanetType::Desert:
		case EPlanetType::Sand:
		case EPlanetType::Tundra:
		case EPlanetType::Nordic:
		case EPlanetType::HighMountain:
			return LOCTEXT("LookDry", "At the terminator every ridge throws a shadow kilometres long.");
		case EPlanetType::Volcanic:
		case EPlanetType::Lava:
		case EPlanetType::Melted:
		case EPlanetType::Sulfur:
		case EPlanetType::Basalt:
			return LOCTEXT("LookFire", "Wait for the night side: the cracks glow, and the glow moves.");
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Ammonia:
			return LOCTEXT("LookIce", "Ice catches the low sun: the whole limb glitters.");
		case EPlanetType::Greenhouse:
			return LOCTEXT("LookGreenhouse", "Nothing of the ground shows; the cloud deck itself is the view, lit gold from inside.");
		default:
			return LOCTEXT("LookDefault", "Fly the terminator line: the light changes fastest there.");
		}
	}

	/** The colour of a sun's light, by its surface temperature. */
	FText StarLight(const int32 Kelvin)
	{
		if (Kelvin <= 0) return FText::GetEmpty();
		if (Kelvin < 3700) return LOCTEXT("LightRed", "Its light is the colour of embers: noon under it looks like dusk at home.");
		if (Kelvin < 5200) return LOCTEXT("LightOrange", "Orange light, warm and patient; its worlds see a gold sky.");
		if (Kelvin < 6000) return LOCTEXT("LightYellow", "Yellow-white, like our own: shadows here would look familiar.");
		if (Kelvin < 7500) return LOCTEXT("LightWhite", "White light, a little harsher than ours.");
		if (Kelvin < 10000) return LOCTEXT("LightHot", "White-hot: the sky it lights stays deep blue even at noon.");
		return LOCTEXT("LightBlue", "Blue-white and furious: it will burn out in a few million years.");
	}

	FText StarWonder(const EStellarType Type)
	{
		switch (Type)
		{
		case EStellarType::MainSequence: return LOCTEXT("StarMain", "A steady sun. Everything that lives, lives by one of these.");
		case EStellarType::SubDwarf: return LOCTEXT("StarSubDwarf", "An old, thin-metalled sun from the galaxy's first generation.");
		case EStellarType::SubGiant: return LOCTEXT("StarSubGiant", "A sun leaving its long middle age; it has begun to swell.");
		case EStellarType::Giant:
		case EStellarType::BrightGiant: return LOCTEXT("StarGiant", "A sun in its last age, swollen past where its inner planets were.");
		case EStellarType::SuperGiant:
		case EStellarType::HyperGiant: return LOCTEXT("StarSuperGiant", "A sun too large to last. When it goes, the whole cluster will see it.");
		case EStellarType::WhiteDwarf: return LOCTEXT("StarWhiteDwarf", "The ember of a sun that is finished. It will cool for longer than the galaxy has existed.");
		case EStellarType::BrownDwarf: return LOCTEXT("StarBrownDwarf", "A star that never caught: warm, dim, patient.");
		case EStellarType::Neutron:
		case EStellarType::Pulsar: return LOCTEXT("StarNeutron", "A sun's mass in a city's width, spun to a blur. The guide keeps its distance.");
		case EStellarType::Protostar: return LOCTEXT("StarProtostar", "A sun still being born, wrapped in the cloud it came from.");
		case EStellarType::BlackHole: return LOCTEXT("StarBlackHole", "The light bends around what you cannot see.");
		default: return FText::GetEmpty();
		}
	}

	FText DistanceText(const double Cm)
	{
		if (Cm >= 0.05 * LightYearCm)
		{
			return FText::Format(LOCTEXT("DistanceLy", "{0} ly"), FText::AsNumber(FMath::RoundToDouble(Cm / LightYearCm * 100.0) / 100.0));
		}
		return FText::FromString(UShipNavigationComponent::FormatDistance(Cm));
	}

	AAstroGenerator* Generator(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				return *It;
			}
		}
		return nullptr;
	}

	/** Greedy chain: from the start, always the nearest remaining; the order the eye travels in. */
	template <typename T>
	void ChainByDistance(const FVector& Start, TArray<T>& Items, TFunctionRef<FVector(const T&)> PlaceOf)
	{
		TArray<T> Ordered;
		FVector At = Start;
		while (!Items.IsEmpty())
		{
			int32 Best = 0;
			double BestDistance = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				const double D = FVector::DistSquared(At, PlaceOf(Items[Index]));
				if (D < BestDistance)
				{
					BestDistance = D;
					Best = Index;
				}
			}
			At = PlaceOf(Items[Best]);
			Ordered.Add(Items[Best]);
			Items.RemoveAt(Best);
		}
		Items = MoveTemp(Ordered);
	}
}

// Registry

APSSpaceTrips::FTrip* APSSpaceTrips::Find(const UWorld* World, const bool bCreate)
{
	using namespace APSSpaceTripsLocal;
	if (!World)
	{
		return nullptr;
	}
	for (FWorldTrip& Entry : Trips())
	{
		if (Entry.World.Get() == World)
		{
			return Entry.Trip.Get();
		}
	}
	if (!bCreate)
	{
		return nullptr;
	}
	static bool bCleanupBound = false;
	if (!bCleanupBound)
	{
		bCleanupBound = true;
		FWorldDelegates::OnWorldCleanup.AddStatic(&ForgetWorld);
	}
	Trips().RemoveAll([](const FWorldTrip& Entry) { return !Entry.World.IsValid(); });
	FWorldTrip& Added = Trips().AddDefaulted_GetRef();
	Added.World = const_cast<UWorld*>(World);
	Added.Trip = MakeUnique<FTrip>(const_cast<UWorld*>(World));
	return Added.Trip.Get();
}

void APSSpaceTrips::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSSpaceTripsLocal;
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	// A SPACE TRIPS world wakes the guide by itself; any other world only through aps.Trip.Start.
	const bool bTripWorld = CVarEnable.GetValueOnGameThread() != 0 && APSWorldRules::IsTrip(World);
	if (FTrip* Trip = Find(World, bTripWorld))
	{
		Trip->Tick(DeltaSeconds);
	}
	if (bTripWorld)
	{
		// Rio 19.10: the route card (O) and the photo (P) without the console.
		APSSpaceTripsHud::Tick(World);
	}
}

// The trip

APSSpaceTrips::FTrip::FTrip(UWorld* InWorld)
	: World(InWorld)
{
}

ASpaceship* APSSpaceTrips::FTrip::PilotShip() const
{
	const UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
	return Ship && !Ship->IsGroundVehicle() ? Ship : nullptr;
}

void APSSpaceTrips::FTrip::Guide(const FText& Text) const
{
	UAPSCivilizationJournalSubsystem::Post(World.Get(), APSSpaceTripsLocal::GuideCategory, Text);
	// Rio 19.10 (SPACE TRIPS without the console): the guide speaks on screen, in the notices' own card.
	const bool bRoute = Phase != EPhase::Idle && !Route.Id.IsEmpty();
	APSMissionTracker::PushNotice(World.Get(), bRoute ? FText::Format(LOCTEXT("GuideKickerRoute", "SPACE TRIPS  ·  {0}"), Route.Name)
		: LOCTEXT("GuideKicker", "SPACE TRIPS  ·  THE GUIDE"),
		Route.Stops.IsValidIndex(StopIndex) && bRoute ? FText::FromString(Route.Stops[StopIndex].Name.ToUpper()) : LOCTEXT("GuideTitle", "THE GUIDE"),
		Text, LOCTEXT("GuideKeys", "O routes  ·  P photo"), FLinearColor(0.55f, 0.72f, 0.90f));
	UE_LOG(LogTemp, Log, TEXT("[APS.Trip] %s"), *Text.ToString());
}

FString APSSpaceTrips::FTrip::Describe() const
{
	const TCHAR* PhaseName = Phase == EPhase::Idle ? TEXT("idle") : Phase == EPhase::Waiting ? TEXT("waiting for the pilot")
		: Phase == EPhase::StarLeg ? TEXT("star leg") : Phase == EPhase::Arriving ? TEXT("arriving") : Phase == EPhase::WorldLeg ? TEXT("world leg")
		: Phase == EPhase::Hold ? TEXT("hold") : Phase == EPhase::Paused ? TEXT("paused") : TEXT("done");
	return FString::Printf(TEXT("route %s, stop %d/%d, world %d/%d, %s (%.0f s)"), *Route.Id, StopIndex + 1, Route.Stops.Num(),
		WorldIndex + 1, Worlds.Num(), PhaseName, PhaseSeconds);
}

void APSSpaceTrips::FTrip::Objective(FText& OutTitle, FText& OutBody) const
{
	using namespace APSSpaceTripsLocal;
	const bool bRoute = Phase != EPhase::Idle && Phase != EPhase::Done;
	OutTitle = bRoute ? FText::Format(LOCTEXT("ObjTitle", "{0}   {1}/{2}"), Route.Name, FText::AsNumber(FMath::Max(StopIndex, 0) + 1),
		FText::AsNumber(Route.Stops.Num())) : LOCTEXT("ObjTitleIdle", "SPACE TRIPS");
	const ASpaceship* Ship = PilotShip();
	const FText Stop = Route.Stops.IsValidIndex(StopIndex) ? FText::FromString(Route.Stops[StopIndex].Name) : FText::GetEmpty();
	const FText Body = Ship && Target.IsValid() ? NameOf(*Ship, Target.Get()) : Stop;
	switch (Phase)
	{
	case EPhase::Idle: OutBody = LOCTEXT("ObjIdle", "Choose a route: O. The guide flies, you watch; P takes a photo."); break;
	case EPhase::Done: OutBody = LOCTEXT("ObjDone", "The route is done. O for another, or the sky is yours."); break;
	case EPhase::Waiting: OutBody = LOCTEXT("ObjWaiting", "Take the seat and start the engine (G): the route begins."); break;
	case EPhase::StarLeg: OutBody = FText::Format(LOCTEXT("ObjStarLeg", "Star drive to {0}."), Stop); break;
	case EPhase::Arriving: OutBody = FText::Format(LOCTEXT("ObjArriving", "Arriving at {0}: its worlds come into view."), Stop); break;
	case EPhase::WorldLeg: OutBody = FText::Format(LOCTEXT("ObjWorldLeg", "Flying to {0}."), Body); break;
	case EPhase::Hold: OutBody = FText::Format(LOCTEXT("ObjHold", "At {0}. Look around; P for a photo."), Body); break;
	case EPhase::Paused: OutBody = LOCTEXT("ObjPaused", "Paused: leave the helm still and the guide takes it back; O for routes."); break;
	default: OutBody = FText::GetEmpty(); break;
	}
}

bool APSSpaceTrips::GetObjective(const UWorld* World, FText& OutTitle, FText& OutBody)
{
	const FTrip* Trip = World && APSWorldRules::IsTrip(World) ? Find(World, false) : nullptr;
	if (!Trip)
	{
		return false;
	}
	Trip->Objective(OutTitle, OutBody);
	return !OutTitle.IsEmpty();
}

void APSSpaceTrips::FTrip::BuildRoutes(TArray<FRoute>& OutRoutes) const
{
	using namespace APSSpaceTripsLocal;
	OutRoutes.Reset();
	UWorld* LiveWorld = World.Get();
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const int32 HomeIndex = Stars ? Stars->GetHomeIndex() : INDEX_NONE;
	const int32 Foreign = FMath::Max(CVarWorldsPerStop.GetValueOnGameThread(), 1);
	{
		FRoute& Added = OutRoutes.AddDefaulted_GetRef();
		Added.Id = TEXT("home");
		Added.Name = LOCTEXT("RouteHome", "HOME WATERS");
		Added.Subtitle = LOCTEXT("RouteHomeSub", "The worlds of our own sun, one by one.");
		FStop& Stop = Added.Stops.AddDefaulted_GetRef();
		Stop.SystemIndex = HomeIndex;
		Stop.Name = Stars && Stars->GetHome() ? Stars->GetHome()->Name : TEXT("home");
	}
	// Rio 19.10: what someone left near home, when the Builders' sites stand (SPACE TRIPS keeps their traces).
	if (const FAPSAncients* Ancients = APSAncientsFind(LiveWorld))
	{
		int32 Sites = 0;
		for (const FAPSAncients::FSite& Site : Ancients->GetSites())
		{
			Sites += Site.Spec.bHomeSystem && Site.Actor.IsValid() ? 1 : 0;
		}
		if (Sites > 0)
		{
			FRoute& Added = OutRoutes.AddDefaulted_GetRef();
			Added.Id = TEXT("builders");
			Added.Name = LOCTEXT("RouteBuilders", "THE BUILDERS' TRAIL");
			Added.Subtitle = LOCTEXT("RouteBuildersSub", "What someone left here before us: stone kilometres high, a hull in a high orbit, a ring of standing stones.");
			FStop& Stop = Added.Stops.AddDefaulted_GetRef();
			Stop.SystemIndex = HomeIndex;
			Stop.Name = Stars && Stars->GetHome() ? Stars->GetHome()->Name : TEXT("home");
			Stop.bBuilders = true;
		}
	}
	if (!Stars || !Stars->IsReady())
	{
		return;
	}
	const ASpaceship* Ship = PilotShip();
	const FVector From = Ship ? Ship->GetActorLocation() : Stars->GetHome() ? Stars->GetHome()->Location : FVector::ZeroVector;
	TArray<int32> Nearest;
	Stars->FindNearest(From, 48, Nearest);
	Nearest.RemoveAll([Stars, HomeIndex](const int32 Index)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		return !Info || Index == HomeIndex || Info->bInsideHome;
	});
	struct FTheme
	{
		const TCHAR* Id;
		FText Name;
		FText Subtitle;
		int32 Count;
		TFunction<bool(const FAPSStarSystemInfo&)> Fits;
	};
	const TArray<FTheme> Themes = {
		{TEXT("nearest"), LOCTEXT("RouteNearest", "THE NEAREST SUNS"), LOCTEXT("RouteNearestSub", "Four stars within reach, as the crow flies."), 4,
			[](const FAPSStarSystemInfo&) { return true; }},
		{TEXT("red"), LOCTEXT("RouteRed", "RED AND COLD"), LOCTEXT("RouteRedSub", "The small red suns that outlive everything else."), 4,
			[Stars](const FAPSStarSystemInfo& Info) { const FString L = SpectralOf(*Stars, Info); return L == TEXT("M") || L == TEXT("K"); }},
		{TEXT("blue"), LOCTEXT("RouteBlue", "BLUE AND WHITE"), LOCTEXT("RouteBlueSub", "The hot young suns that burn bright and short."), 4,
			[Stars](const FAPSStarSystemInfo& Info) { const FString L = SpectralOf(*Stars, Info); return L == TEXT("O") || L == TEXT("B") || L == TEXT("A"); }},
		{TEXT("doubles"), LOCTEXT("RouteDoubles", "DOUBLES AND TRIPLES"), LOCTEXT("RouteDoublesSub", "Suns that share a sky."), 4,
			[](const FAPSStarSystemInfo& Info) { return Info.StarCount >= 2; }},
		{TEXT("giants"), LOCTEXT("RouteGiants", "THE OLD GIANTS"), LOCTEXT("RouteGiantsSub", "Suns in their last age, swollen to the size of orbits."), 3,
			[Stars](const FAPSStarSystemInfo& Info) { return IsGiantStar(*Stars, Info); }},
		{TEXT("dark"), LOCTEXT("RouteDark", "THE DARK"), LOCTEXT("RouteDarkSub", "What suns leave behind: dwarfs, neutron stars, the holes."), 3,
			[Stars](const FAPSStarSystemInfo& Info) { const FString L = SpectralOf(*Stars, Info); return L == TEXT("NS") || L == TEXT("BH") || L == TEXT("PS") || L == TEXT("D"); }},
		{TEXT("crowded"), LOCTEXT("RouteCrowded", "THE CROWDED SKIES"), LOCTEXT("RouteCrowdedSub", "Systems with the most worlds to see."), 4,
			[](const FAPSStarSystemInfo& Info) { return Info.PotentialPlanets >= 6; }}};
	for (const FTheme& Theme : Themes)
	{
		TArray<int32> Picked;
		for (const int32 Index : Nearest)
		{
			if (Picked.Num() >= Theme.Count)
			{
				break;
			}
			if (Theme.Fits(*Stars->Get(Index)))
			{
				Picked.Add(Index);
			}
		}
		if (Picked.IsEmpty())
		{
			continue;
		}
		ChainByDistance<int32>(From, Picked, [Stars](const int32& Index) { return Stars->Get(Index)->Location; });
		FRoute& Added = OutRoutes.AddDefaulted_GetRef();
		Added.Id = Theme.Id;
		Added.Name = Theme.Name;
		Added.Subtitle = Theme.Subtitle;
		for (const int32 Index : Picked)
		{
			FStop& Stop = Added.Stops.AddDefaulted_GetRef();
			Stop.SystemIndex = Index;
			Stop.Name = Stars->Get(Index)->Name;
			Stop.WorldsToShow = Foreign;
		}
	}
	// Rio 19.10 ("the nearest wonders"): one of each of the strangest suns within reach, nearest first.
	{
		TArray<int32> Picked;
		const TFunction<bool(const FAPSStarSystemInfo&)> Kinds[] = {
			[Stars](const FAPSStarSystemInfo& Info) { const FString L = SpectralOf(*Stars, Info); return L == TEXT("BH") || L == TEXT("NS"); },
			[Stars](const FAPSStarSystemInfo& Info) { return IsGiantStar(*Stars, Info); },
			[](const FAPSStarSystemInfo& Info) { return Info.StarCount >= 3; },
			[Stars](const FAPSStarSystemInfo& Info) { const FString L = SpectralOf(*Stars, Info); return L == TEXT("O") || L == TEXT("B"); }};
		for (const TFunction<bool(const FAPSStarSystemInfo&)>& Kind : Kinds)
		{
			for (const int32 Index : Nearest)
			{
				if (!Picked.Contains(Index) && Kind(*Stars->Get(Index)))
				{
					Picked.Add(Index);
					break;
				}
			}
		}
		if (Picked.Num() >= 2)
		{
			ChainByDistance<int32>(From, Picked, [Stars](const int32& Index) { return Stars->Get(Index)->Location; });
			FRoute& Added = OutRoutes.AddDefaulted_GetRef();
			Added.Id = TEXT("wonders");
			Added.Name = LOCTEXT("RouteWonders", "THE NEAREST WONDERS");
			Added.Subtitle = LOCTEXT("RouteWondersSub", "One of each of the strangest suns within reach: the remnant, the giant, the many-sunned, the blue.");
			for (const int32 Index : Picked)
			{
				FStop& Stop = Added.Stops.AddDefaulted_GetRef();
				Stop.SystemIndex = Index;
				Stop.Name = Stars->Get(Index)->Name;
				Stop.WorldsToShow = Foreign;
			}
		}
	}
}

bool APSSpaceTrips::FTrip::Start(const FString& IdOrIndex)
{
	TArray<FRoute> Routes;
	BuildRoutes(Routes);
	const FRoute* Found = Routes.FindByPredicate([&IdOrIndex](const FRoute& Candidate) { return Candidate.Id.Equals(IdOrIndex, ESearchCase::IgnoreCase); });
	if (!Found && IdOrIndex.IsNumeric())
	{
		const int32 Index = FCString::Atoi(*IdOrIndex);
		Found = Routes.IsValidIndex(Index) ? &Routes[Index] : nullptr;
	}
	if (!Found)
	{
		Guide(FText::Format(LOCTEXT("NoRoute", "No route '{0}'. aps.Trip.Routes lists them."), FText::FromString(IdOrIndex)));
		return false;
	}
	if (Phase != EPhase::Idle && Phase != EPhase::Done)
	{
		Stop(LOCTEXT("StopForNew", "another route"));
	}
	Route = *Found;
	bAutoStarted = true;
	APSSpaceTripsHud::CloseCard(World.Get());
	StopIndex = 0;
	WorldIndex = INDEX_NONE;
	Worlds.Reset();
	Target = nullptr;
	Phase = EPhase::Waiting;
	PhaseSeconds = 0.0f;
	bToldToBoard = false;
	Guide(FText::Format(LOCTEXT("RouteStart", "ROUTE {0}: {1} {2} stops. The guide has the helm; take it any time, and aps.Trip.Resume hands it back. aps.Trip.Next skips ahead, aps.Trip.Photo takes a still."),
		Route.Name, Route.Subtitle, FText::AsNumber(Route.Stops.Num())));
	return true;
}

void APSSpaceTrips::FTrip::Stop(const FText& Why)
{
	if (Phase == EPhase::Idle)
	{
		return;
	}
	if (ASpaceship* Ship = PilotShip(); Ship && Ship->FlightModel && Ship->FlightModel->IsAutopilotEngaged()
		&& Ship->FlightModel->GetAutopilotTarget() == Target.Get())
	{
		Ship->FlightModel->DisengageAutopilot(TEXT("the guide stepped back"));
	}
	Phase = EPhase::Idle;
	Target = nullptr;
	Worlds.Reset();
	Guide(FText::Format(LOCTEXT("Stopped", "The guide steps back: {0}. The helm is yours."), Why));
}

void APSSpaceTrips::FTrip::Pause(const FText& Why, const bool bByHand)
{
	if (Phase == EPhase::Idle || Phase == EPhase::Done || Phase == EPhase::Paused)
	{
		return;
	}
	PausedFrom = Phase == EPhase::Hold ? EPhase::WorldLeg : Phase;
	Phase = EPhase::Paused;
	PhaseSeconds = 0.0f;
	bPausedByHand = bByHand;
	HelmStillSeconds = 0.0f;
	if (const ASpaceship* Ship = PilotShip())
	{
		HelmStillRotation = Ship->GetActorQuat();
	}
	Guide(bByHand || APSSpaceTripsLocal::CVarResumeAfterSeconds.GetValueOnGameThread() <= 0.0f
		? FText::Format(LOCTEXT("Paused", "{0} aps.Trip.Resume brings the guide back, aps.Trip.Next moves on."), Why)
		: FText::Format(LOCTEXT("PausedForAWhile", "{0} Leave the helm still a few seconds and the guide takes it back; aps.Trip.Next moves on."), Why));
}

void APSSpaceTrips::FTrip::Resume()
{
	if (Phase != EPhase::Paused)
	{
		return;
	}
	ASpaceship* Ship = PilotShip();
	if (!Ship)
	{
		Phase = EPhase::Waiting;
		PhaseSeconds = 0.0f;
		return;
	}
	Guide(LOCTEXT("Resumed", "The guide takes the helm again."));
	if (PausedFrom == EPhase::WorldLeg && Worlds.IsValidIndex(WorldIndex))
	{
		BeginWorldLeg(*Ship);
	}
	else if (PausedFrom == EPhase::Arriving)
	{
		Phase = EPhase::Arriving;
		PhaseSeconds = 0.0f;
	}
	else
	{
		BeginStop(*Ship);
	}
}

void APSSpaceTrips::FTrip::Next()
{
	ASpaceship* Ship = PilotShip();
	if (Phase == EPhase::Idle || Phase == EPhase::Done || !Ship)
	{
		return;
	}
	if ((Phase == EPhase::WorldLeg || Phase == EPhase::Hold || (Phase == EPhase::Paused && PausedFrom == EPhase::WorldLeg))
		&& Worlds.IsValidIndex(WorldIndex))
	{
		NextWorld(*Ship);
		return;
	}
	++StopIndex;
	BeginStop(*Ship);
}

void APSSpaceTrips::FTrip::Photo(const FString& Name)
{
	const FString Stem = Name.IsEmpty() ? (Route.Stops.IsValidIndex(StopIndex) ? Route.Stops[StopIndex].Name : FString(TEXT("Sky"))) : Name;
	FString Clean;
	for (const TCHAR Char : Stem)
	{
		Clean.AppendChar(FChar::IsAlnum(Char) ? Char : TEXT('_'));
	}
	const FString File = FPaths::ScreenShotDir() / TEXT("Trips") / FString::Printf(TEXT("Trip_%s_%02d.png"), *Clean, ++PhotoSerial);
	FScreenshotRequest::RequestScreenshot(File, false, false);
	Guide(FText::Format(LOCTEXT("Photo", "A still of the view, without the instruments: {0}"), FText::FromString(File)));
}

void APSSpaceTrips::FTrip::TickPaused(ASpaceship* Ship, const float DeltaSeconds)
{
	using namespace APSSpaceTripsLocal;
	// APS DEV 14.10 (audit): a pause the pilot's hand caused (the helm, a course of their own, the seat left) ends by
	// itself: the pilot sits with the engine on, no autopilot of their own, and the helm has been still for a while.
	// aps.Trip.Pause is a wish: it waits for aps.Trip.Resume.
	const float After = CVarResumeAfterSeconds.GetValueOnGameThread();
	if (bPausedByHand || After <= 0.0f || !Ship || !Ship->FlightModel || !Ship->GetEngineRunning()
		|| Ship->FlightModel->IsAutopilotEngaged())
	{
		HelmStillSeconds = 0.0f;
		if (Ship)
		{
			HelmStillRotation = Ship->GetActorQuat();
		}
		return;
	}
	const FQuat Now = Ship->GetActorQuat();
	if (Now.AngularDistance(HelmStillRotation) > FMath::DegreesToRadians(HelmStillDegrees))
	{
		HelmStillSeconds = 0.0f;
		HelmStillRotation = Now;
		return;
	}
	HelmStillSeconds += DeltaSeconds;
	if (HelmStillSeconds >= After)
	{
		HelmStillSeconds = 0.0f;
		Resume();
	}
}

void APSSpaceTrips::FTrip::Tick(const float DeltaSeconds)
{
	using namespace APSSpaceTripsLocal;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	ASpaceship* Ship = PilotShip();
	// A SPACE TRIPS world: a welcome once, and the first route by itself once the pilot sits with the engine on.
	if (Phase == EPhase::Idle && APSWorldRules::IsTrip(LiveWorld) && CVarEnable.GetValueOnGameThread() != 0)
	{
		if (!bWelcomed)
		{
			bWelcomed = true;
			Guide(LOCTEXT("Welcome", "Welcome aboard. This sky is for looking at: the guide flies, you watch. O opens the routes, P takes a photo."));
		}
		if (!bCardOffered && Ship)
		{
			PhaseSeconds += DeltaSeconds;
			if (PhaseSeconds >= AutoStartGraceSeconds)
			{
				// Rio 19.10: the routes to choose from, on screen; the home route by itself only if the card stays untouched.
				bCardOffered = true;
				PhaseSeconds = 0.0f;
				APSSpaceTripsHud::OpenCard(LiveWorld);
			}
		}
		else if (bCardOffered && !bAutoStarted && CVarAutoStart.GetValueOnGameThread() != 0)
		{
			PhaseSeconds += DeltaSeconds;
			if (!APSSpaceTripsHud::IsCardOpen(LiveWorld))
			{
				bAutoStarted = true;
			}
			else if (PhaseSeconds >= CardAutoStartSeconds)
			{
				bAutoStarted = true;
				Start(TEXT("home"));
			}
		}
		return;
	}
	if (Phase == EPhase::Paused)
	{
		TickPaused(Ship, DeltaSeconds);
		return;
	}
	if (Phase == EPhase::Idle || Phase == EPhase::Done)
	{
		return;
	}
	if (!Ship || !Ship->FlightModel)
	{
		if (Phase != EPhase::Waiting)
		{
			Pause(LOCTEXT("PilotLeft", "The pilot left the seat: the route waits."));
		}
		return;
	}
	if (!Ship->GetEngineRunning())
	{
		if (!bToldToBoard)
		{
			bToldToBoard = true;
			Guide(LOCTEXT("StartEngine", "Start the engine (G): the tour begins when it runs."));
		}
		return;
	}
	PhaseSeconds += DeltaSeconds;
	switch (Phase)
	{
	case EPhase::Waiting:
		BeginStop(*Ship);
		break;
	case EPhase::StarLeg:
		TickStarLeg(*Ship, DeltaSeconds);
		break;
	case EPhase::Arriving:
		TickArriving(*Ship);
		break;
	case EPhase::WorldLeg:
		TickWorldLeg(*Ship);
		break;
	case EPhase::Hold:
		HoldSeconds += DeltaSeconds;
		if (bRotated && Ship->GetActorQuat().AngularDistance(LastSetRotation) > FMath::DegreesToRadians(HelmTurnDegrees))
		{
			// The pilot's hand: the guide lets the view be theirs.
			bRotated = false;
			Pause(LOCTEXT("HelmHold", "You have the helm."));
			break;
		}
		if (const AActor* Sight = Target.Get())
		{
			TurnNoseTo(*Ship, SightOf(*Ship, *Sight), DeltaSeconds);
		}
		if (HoldSeconds >= FMath::Max(CVarHoldSeconds.GetValueOnGameThread(), 1.0f))
		{
			bRotated = false;
			NextWorld(*Ship);
		}
		break;
	default:
		break;
	}
}

void APSSpaceTrips::FTrip::BeginStop(ASpaceship& Ship)
{
	using namespace APSSpaceTripsLocal;
	Worlds.Reset();
	WorldIndex = INDEX_NONE;
	Target = nullptr;
	if (!Route.Stops.IsValidIndex(StopIndex))
	{
		Phase = EPhase::Done;
		Guide(LOCTEXT("RouteEnd", "The route ends here. The sky is yours: aps.Trip.Routes for another."));
		return;
	}
	const FStop& Stop = Route.Stops[StopIndex];
	const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stop.SystemIndex) : nullptr;
	const bool bHome = !Info || Info->bHome;
	// APS DEV 14.10: the catalogue's places are the sky's (the world place plus the sky offset), as the fleet measures them.
	const FVector SkyOffset = UAPSWorldOriginSubsystem::SkyOffsetOf(World.Get());
	const bool bInside = Stars && Info && Stars->FindContaining(Ship.GetActorLocation() + SkyOffset) == Stop.SystemIndex;
	Guide(FText::Format(LOCTEXT("StopBegin", "STOP {0} of {1}: {2}{3}"), FText::AsNumber(StopIndex + 1), FText::AsNumber(Route.Stops.Num()),
		FText::FromString(Stop.Name), Info && !bInside && !bHome
			? FText::Format(LOCTEXT("StopAway", ", {0} away."), DistanceText(FVector::Dist(Ship.GetActorLocation() + SkyOffset, Info->Location)))
			: LOCTEXT("StopHere", ".")));
	if (bHome || bInside)
	{
		Phase = EPhase::Arriving;
		PhaseSeconds = 0.0f;
		return;
	}
	BeginStarLeg(Ship);
}

void APSSpaceTrips::FTrip::BeginStarLeg(ASpaceship& Ship)
{
	Phase = EPhase::StarLeg;
	PhaseSeconds = 0.0f;
	bDriveWasOn = false;
	bRotated = false;
	EngageAttempts = 0;
	NextEngageSeconds = 0.0f;
	if (Ship.FlightModel && Ship.FlightModel->IsAutopilotEngaged())
	{
		Ship.FlightModel->DisengageAutopilot(TEXT("star leg"));
	}
}

void APSSpaceTrips::FTrip::TickStarLeg(ASpaceship& Ship, const float DeltaSeconds)
{
	using namespace APSSpaceTripsLocal;
	UAPSShipFlightModel* FlightModel = Ship.FlightModel;
	FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FStop& Stop = Route.Stops[StopIndex];
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stop.SystemIndex) : nullptr;
	if (!FlightModel || !Info)
	{
		++StopIndex;
		BeginStop(Ship);
		return;
	}
	// The catalogue's places are the sky's: the ship's place is read in the same frame (APS DEV 14.10).
	const FVector ShipLocation = Ship.GetActorLocation() + UAPSWorldOriginSubsystem::SkyOffsetOf(World.Get());
	// There: inside the system's room (the drive drops by itself on entering it).
	if (Stars->FindContaining(ShipLocation) == Stop.SystemIndex)
	{
		Phase = EPhase::Arriving;
		PhaseSeconds = 0.0f;
		return;
	}
	// The pilot's hand on the helm: the nose turned since the guide set it.
	if (FlightModel->IsStarDriveActive() && bRotated && Ship.GetActorQuat().AngularDistance(LastSetRotation) > FMath::DegreesToRadians(HelmTurnDegrees))
	{
		Pause(LOCTEXT("HelmStar", "You have the helm."));
		return;
	}
	if (FlightModel->IsStarDriveActive())
	{
		bDriveWasOn = true;
		// The nose comes round onto the star, a few degrees a second; the drive's course follows the nose.
		const FVector Direction = (Info->Location - ShipLocation).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			return;
		}
		FVector Up = Ship.GetActorUpVector();
		Up = (Up - Direction * FVector::DotProduct(Up, Direction)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		const FQuat Wanted = FAPSShipFlightBenchmark::GetRotationForFlightAxes(Ship, Direction, Up);
		const FQuat Current = Ship.GetActorQuat();
		const double Angle = Current.AngularDistance(Wanted);
		const double MaxStep = FMath::DegreesToRadians(FMath::Max(CVarTurnRate.GetValueOnGameThread(), 1.0f)) * DeltaSeconds;
		const double Alpha = Angle > UE_SMALL_NUMBER ? FMath::Min(1.0, MaxStep / Angle) : 1.0;
		Ship.SetActorRotation(FQuat::Slerp(Current, Wanted, Alpha), ETeleportType::TeleportPhysics);
		LastSetRotation = Ship.GetActorQuat();
		bRotated = true;
		return;
	}
	bRotated = false;
	// Fold the distance when the drive will not carry this hull, or has been dropping for too long.
	const float FoldAfter = CVarFoldAfterSeconds.GetValueOnGameThread();
	const bool bFold = (EngageAttempts >= 3 && !bDriveWasOn) || (FoldAfter > 0.0f && PhaseSeconds >= FoldAfter);
	if (bFold)
	{
		FAPSSystemMaterializer* Materializer = Stars->GetMaterializer();
		if (Materializer && Materializer->VisitForTest(*Stars, Stop.SystemIndex, false))
		{
			Guide(FText::Format(LOCTEXT("Folded", "The drive will not carry this hull so far: the guide folds the distance to {0}."), FText::FromString(Stop.Name)));
			Phase = EPhase::Arriving;
			PhaseSeconds = 0.0f;
		}
		else
		{
			Guide(FText::Format(LOCTEXT("FoldFailed", "{0} keeps its distance; on to the next."), FText::FromString(Stop.Name)));
			++StopIndex;
			BeginStop(Ship);
		}
		return;
	}
	// The drive again (its own arrival at a system on the way, a brake, a refusal): a few seconds between tries.
	if (PhaseSeconds >= NextEngageSeconds)
	{
		NextEngageSeconds = PhaseSeconds + 3.0f;
		++EngageAttempts;
		if (FlightModel->EngageStarDrive())
		{
			if (EngageAttempts == 1)
			{
				Guide(FText::Format(LOCTEXT("DriveOn", "Star drive on: the nose comes round to {0}."), FText::FromString(Stop.Name)));
			}
		}
	}
}

void APSSpaceTrips::FTrip::TickArriving(ASpaceship& Ship)
{
	using namespace APSSpaceTripsLocal;
	const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FStop& Stop = Route.Stops[StopIndex];
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stop.SystemIndex) : nullptr;
	const bool bHome = !Info || Info->bHome;
	if (bHome)
	{
		ListWorlds(Ship);
		return;
	}
	const FAPSSystemMaterializer* Materializer = Stars->GetMaterializer();
	if (Materializer && Materializer->GetActiveIndex() == Stop.SystemIndex)
	{
		TArray<APlanet*> Planets;
		Materializer->GetPlanets(Planets);
		// The planets come one by one over a few seconds; wait for the first, then a moment for the rest.
		if (Planets.Num() > 0 && PhaseSeconds >= 4.0f)
		{
			ListWorlds(Ship);
			return;
		}
	}
	if (PhaseSeconds >= ArrivingTimeoutSeconds)
	{
		Guide(FText::Format(LOCTEXT("ArriveTimeout", "{0} does not come into view; on to the next."), FText::FromString(Stop.Name)));
		++StopIndex;
		BeginStop(Ship);
	}
}

void APSSpaceTrips::FTrip::ListWorlds(ASpaceship& Ship)
{
	using namespace APSSpaceTripsLocal;
	Worlds.Reset();
	const FStop& Stop = Route.Stops[StopIndex];
	const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stop.SystemIndex) : nullptr;
	const bool bHome = !Info || Info->bHome;
	const AStar* Star = nullptr;
	TArray<AActor*> Bodies;
	if (bHome)
	{
		const AAstroGenerator* Gen = Generator(World.Get());
		Star = Gen ? Gen->HomeStar : nullptr;
		if (Star)
		{
			for (APlanet* Planet : Star->Planets)
			{
				if (!IsValid(Planet))
				{
					continue;
				}
				Bodies.Add(Planet);
				for (AMoon* Moon : Planet->Moons)
				{
					if (IsValid(Moon))
					{
						Bodies.Add(Moon);
						break;
					}
				}
			}
		}
	}
	else if (const FAPSSystemMaterializer* Materializer = Stars->GetMaterializer())
	{
		Star = Materializer->GetStar();
		TArray<APlanet*> Planets;
		Materializer->GetPlanets(Planets);
		for (APlanet* Planet : Planets)
		{
			if (IsValid(Planet))
			{
				Bodies.Add(Planet);
			}
		}
	}
	if (Stop.bBuilders)
	{
		Bodies.Reset();
		if (const FAPSAncients* Ancients = APSAncientsFind(World.Get()))
		{
			for (const FAPSAncients::FSite& Site : Ancients->GetSites())
			{
				if (Site.Spec.bHomeSystem && Site.Actor.IsValid())
				{
					Bodies.Add(Site.Actor.Get());
				}
			}
		}
		Guide(LOCTEXT("BuildersIntro", "THE BUILDERS' TRAIL: nobody knows who they were. They built in stone that does not weather, at sizes nothing alive needs, and left the lights on."));
	}
	else
	{
		Guide(DescribeStar(Star, Stop));
	}
	ChainByDistance<AActor*>(Ship.GetActorLocation(), Bodies, [&Ship](AActor* const& Body) { return UAPSWorldOriginSubsystem::PlaceFor(&Ship, *Body); });
	if (Stop.WorldsToShow > 0 && Bodies.Num() > Stop.WorldsToShow)
	{
		Bodies.SetNum(Stop.WorldsToShow);
	}
	for (AActor* Body : Bodies)
	{
		Worlds.Add(Body);
	}
	if (Worlds.IsEmpty())
	{
		Guide(LOCTEXT("NoWorlds", "Nothing stands near this sun; on to the next."));
		++StopIndex;
		BeginStop(Ship);
		return;
	}
	WorldIndex = 0;
	BeginWorldLeg(Ship);
}

void APSSpaceTrips::FTrip::BeginWorldLeg(ASpaceship& Ship)
{
	using namespace APSSpaceTripsLocal;
	AActor* Body = Worlds.IsValidIndex(WorldIndex) ? Worlds[WorldIndex].Get() : nullptr;
	if (!Body || !Ship.FlightModel)
	{
		NextWorld(Ship);
		return;
	}
	Target = Body;
	Phase = EPhase::WorldLeg;
	PhaseSeconds = 0.0f;
	HoldSeconds = 0.0f;
	Ship.FlightModel->EngageAutopilot(Body);
	const double Away = FVector::Dist(Ship.GetActorLocation(), UAPSWorldOriginSubsystem::PlaceFor(&Ship, *Body));
	const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body);
	FText Kind = Planet ? TypeName(Planet->PlanetType) : LOCTEXT("TypeBody", "A BODY");
	if (const FAPSAncients* Ancients = Planet ? nullptr : APSAncientsFind(World.Get()))
	{
		if (const FAPSAncients::FSite* Site = Ancients->FindByActor(Body))
		{
			Kind = FText::FromString(FString(APSAncients::KindLabel(Site->Spec.Kind)).Replace(TEXT("_"), TEXT(" ")).ToUpper());
		}
	}
	Guide(FText::Format(LOCTEXT("WorldNext", "Next: {0}, {1}, {2}."), Kind, NameOf(Ship, Body), DistanceText(Away)));
}

void APSSpaceTrips::FTrip::TickWorldLeg(ASpaceship& Ship)
{
	using namespace APSSpaceTripsLocal;
	UAPSShipFlightModel* FlightModel = Ship.FlightModel;
	AActor* Body = Target.Get();
	if (!FlightModel || !Body)
	{
		NextWorld(Ship);
		return;
	}
	if (FlightModel->IsAutopilotEngaged())
	{
		if (FlightModel->GetAutopilotTarget() != Body)
		{
			// The pilot picked another target: the guide waits.
			Pause(LOCTEXT("HelmRetarget", "A course of your own."));
		}
		return;
	}
	// The autopilot let go: arrived (near the stop distance it chose) or the pilot's hand.
	double RadiusCm = 10000.0;
	double ArrivalCm = 100000.0;
	if (const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body))
	{
		RadiusCm = Planet->GetWorldScapeBodyRadiusCm();
		ArrivalCm = FMath::Max(RadiusCm * 0.08, 20000000.0);
	}
	const double Away = FVector::Dist(Ship.GetActorLocation(), UAPSWorldOriginSubsystem::PlaceFor(&Ship, *Body)) - RadiusCm;
	if (Away <= ArrivalCm * 2.5 || PhaseSeconds < 0.5f)
	{
		if (PhaseSeconds < 0.5f)
		{
			// Engaged and let go within the frame: the autopilot refused (a hull it will not fly, a target too close).
			if (Away > ArrivalCm * 2.5)
			{
				NextWorld(Ship);
				return;
			}
		}
		Phase = EPhase::Hold;
		HoldSeconds = 0.0f;
		bRotated = false;
		Guide(DescribeWorld(Ship, Body));
		return;
	}
	Pause(LOCTEXT("HelmWorld", "You have the helm."));
}

void APSSpaceTrips::FTrip::TurnNoseTo(ASpaceship& Ship, const FVector& Direction, const float DeltaSeconds)
{
	using namespace APSSpaceTripsLocal;
	if (Direction.IsNearlyZero())
	{
		return;
	}
	FVector Up = Ship.GetActorUpVector();
	Up = (Up - Direction * FVector::DotProduct(Up, Direction)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FQuat Wanted = FAPSShipFlightBenchmark::GetRotationForFlightAxes(Ship, Direction, Up);
	const FQuat Current = Ship.GetActorQuat();
	const double Angle = Current.AngularDistance(Wanted);
	// Half the star leg's rate: a slow look round, not a snap.
	const double MaxStep = FMath::DegreesToRadians(FMath::Max(CVarTurnRate.GetValueOnGameThread() * 0.5f, 1.0f)) * DeltaSeconds;
	const double Alpha = Angle > UE_SMALL_NUMBER ? FMath::Min(1.0, MaxStep / Angle) : 1.0;
	Ship.SetActorRotation(FQuat::Slerp(Current, Wanted, Alpha), ETeleportType::TeleportPhysics);
	LastSetRotation = Ship.GetActorQuat();
	bRotated = true;
}

FVector APSSpaceTrips::FTrip::SightOf(const ASpaceship& Ship, const AActor& Body) const
{
	const FVector From = Ship.GetActorLocation();
	const FVector Centre = UAPSWorldOriginSubsystem::PlaceFor(&Ship, Body);
	FVector Toward = (Centre - From).GetSafeNormal();
	const APlanetaryBody* Planet = Cast<APlanetaryBody>(&Body);
	if (!Planet || Toward.IsNearlyZero())
	{
		if (!Planet)
		{
			// A site: the middle of what was built (its actor stands at a navigation point beside it).
			FVector Origin;
			FVector Extent;
			Body.GetActorBounds(false, Origin, Extent);
			const FVector Middle = UAPSWorldOriginSubsystem::PlaceFor(&Ship, Body) + (Origin - Body.GetActorLocation());
			const FVector ToMiddle = (Middle - From).GetSafeNormal();
			return ToMiddle.IsNearlyZero() ? Toward : ToMiddle;
		}
		return Toward;
	}
	// A world: its limb on the sunlit side, so the terminator, the air's shell and the sun share the view.
	const double Distance = FVector::Dist(Centre, From);
	const double Radius = FMath::Max(Planet->GetWorldScapeBodyRadiusCm(), 1.0);
	const double Limb = Distance > Radius ? FMath::Asin(FMath::Clamp(Radius / Distance, 0.0, 1.0)) : UE_HALF_PI;
	const APlanet* AsPlanet = Cast<APlanet>(&Body);
	const AMoon* AsMoon = Cast<AMoon>(&Body);
	const AStar* Star = AsPlanet ? AsPlanet->ParentStar : AsMoon && IsValid(AsMoon->ParentPlanet) ? AsMoon->ParentPlanet->ParentStar : nullptr;
	FVector Side = IsValid(Star) ? FVector::VectorPlaneProject(UAPSWorldOriginSubsystem::PlaceFor(&Ship, *Star) - From, Toward).GetSafeNormal()
		: FVector::ZeroVector;
	if (Side.IsNearlyZero())
	{
		FVector Unused;
		Toward.FindBestAxisVectors(Side, Unused);
	}
	// Toward the limb, 80 % of the way out from the centre.
	return (Toward * FMath::Cos(Limb * 0.8) + Side * FMath::Sin(Limb * 0.8)).GetSafeNormal();
}

void APSSpaceTrips::FTrip::NextWorld(ASpaceship& Ship)
{
	++WorldIndex;
	if (Worlds.IsValidIndex(WorldIndex))
	{
		BeginWorldLeg(Ship);
		return;
	}
	++StopIndex;
	BeginStop(Ship);
}

FText APSSpaceTrips::FTrip::NameOf(const ASpaceship& Ship, const AActor* Body) const
{
	if (!Body)
	{
		return FText::GetEmpty();
	}
	if (const FAPSAncients* Ancients = Cast<APlanetaryBody>(Body) ? nullptr : APSAncientsFind(World.Get()))
	{
		if (const FAPSAncients::FSite* Site = Ancients->FindByActor(Body))
		{
			return Ancients->DisplayName(*Site);
		}
	}
	if (Ship.ShipNavigation)
	{
		for (const FShipNavigationContact& Contact : Ship.ShipNavigation->GetContacts())
		{
			if (Contact.Actor.Get() == Body && !Contact.DisplayName.IsEmpty())
			{
				return FText::FromString(Contact.DisplayName);
			}
		}
	}
	FString Name = Body->GetName();
	Name.RemoveFromStart(TEXT("BP_"));
	return FText::FromString(Name);
}

FText APSSpaceTrips::FTrip::DescribeStar(const AStar* Star, const FStop& Stop) const
{
	using namespace APSSpaceTripsLocal;
	const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stop.SystemIndex) : nullptr;
	FFormatNamedArguments Args;
	Args.Add(TEXT("Name"), FText::FromString(Stop.Name));
	Args.Add(TEXT("Spectral"), Star && !Star->FullSpectralClass.IsNone() ? FText::FromName(Star->FullSpectralClass)
		: Info ? FText::FromString(Info->Spectral) : FText::GetEmpty());
	Args.Add(TEXT("Suns"), FText::AsNumber(Info ? FMath::Max(Info->StarCount, 1) : 1));
	Args.Add(TEXT("Worlds"), FText::AsNumber(Star ? Star->Planets.Num() : Info ? Info->PotentialPlanets : 0));
	Args.Add(TEXT("Wonder"), Star ? StarWonder(Star->StellarClass) : FText::GetEmpty());
	// Rio 19.10 ("more depth and detail"): the sun in numbers, and the colour of its light.
	FText Numbers = FText::GetEmpty();
	if (Star && Star->SurfaceTemperature > 0 && Star->StellarClass != EStellarType::BlackHole)
	{
		FNumberFormattingOptions Two;
		Two.SetMaximumFractionalDigits(2);
		Numbers = FText::Format(LOCTEXT("StarNumbers", " {0} K at the surface, {1} times the Sun's width, {2} times its light."),
			FText::AsNumber(Star->SurfaceTemperature), FText::AsNumber(Star->StarRadiusKM / 696340.0, &Two),
			FText::AsNumber(Star->Luminosity, &Two));
	}
	Args.Add(TEXT("Numbers"), Numbers);
	Args.Add(TEXT("Light"), Star ? StarLight(Star->SurfaceTemperature) : FText::GetEmpty());
	if (Info && Info->StarCount >= 2)
	{
		return FText::Format(LOCTEXT("StarMultiDeep", "{Name}: {Suns} suns, the primary {Spectral}, {Worlds} worlds.{Numbers} {Wonder} {Light}"), Args);
	}
	return FText::Format(LOCTEXT("StarSingleDeep", "{Name}: a {Spectral} sun with {Worlds} worlds.{Numbers} {Wonder} {Light}"), Args);
}

FText APSSpaceTrips::FTrip::DescribeWorld(const ASpaceship& Ship, const AActor* Body) const
{
	using namespace APSSpaceTripsLocal;
	const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body);
	if (!Planet)
	{
		// A Builders' site: its own description (the kind, its size), then the trail's line.
		if (const FAPSAncients* Ancients = APSAncientsFind(World.Get()))
		{
			if (const FAPSAncients::FSite* Site = Ancients->FindByActor(Body))
			{
				return FText::Format(LOCTEXT("SiteLine", "{0}: {1} Nobody has seen it lit by anything but its own glyphs and our sun."),
					Ancients->DisplayName(*Site), APSAncients::KindDescription(Site->Spec.Kind, Site->Metrics));
			}
		}
		return FText::Format(LOCTEXT("BodyPlain", "{0}. Look."), NameOf(Ship, Body));
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("Name"), NameOf(Ship, Body));
	Args.Add(TEXT("Type"), TypeName(Planet->PlanetType));
	Args.Add(TEXT("Radius"), FText::AsNumber(Planet->PlanetRadiusKM));
	Args.Add(TEXT("Wonder"), Wonder(Planet->PlanetType));
	const APlanet* AsPlanet = Cast<APlanet>(Body);
	const int32 Moons = AsPlanet ? AsPlanet->Moons.Num() : 0;
	Args.Add(TEXT("Moons"), FText::AsNumber(Moons));
	// Rio 19.10 ("more depth and detail"): the weight, the heat and the air of it, and where to look.
	FNumberFormattingOptions One;
	One.SetMaximumFractionalDigits(1);
	FText Facts = FText::GetEmpty();
	if (Planet->PlanetGravityStrength > 0.01)
	{
		Facts = FText::Format(LOCTEXT("FactGravity", ", {0} g"), FText::AsNumber(Planet->PlanetGravityStrength, &One));
	}
	if (Planet->Temperature > 20 && Planet->Temperature < 3000)
	{
		Facts = FText::Format(LOCTEXT("FactHeat", "{0}, {1} °C"), Facts, FText::AsNumber(Planet->Temperature - 273));
	}
	if (Planet->PlanetHabitability == EPlanetHabitability::Habitable)
	{
		Facts = FText::Format(LOCTEXT("FactAir", "{0}, air we could breathe"), Facts);
	}
	Args.Add(TEXT("Facts"), Facts);
	Args.Add(TEXT("Look"), LookFor(Planet->PlanetType));
	if (Cast<AMoon>(Body))
	{
		return FText::Format(LOCTEXT("MoonLineDeep", "{Name}, a moon: {Type}, {Radius} km across{Facts}. {Wonder} {Look}"), Args);
	}
	if (Moons > 0)
	{
		return FText::Format(LOCTEXT("PlanetMoonsLineDeep", "{Name}: {Type}, {Radius} km{Facts}, {Moons} moons. {Wonder} {Look}"), Args);
	}
	return FText::Format(LOCTEXT("PlanetLineDeep", "{Name}: {Type}, {Radius} km{Facts}. {Wonder} {Look}"), Args);
}

// Console

namespace APSSpaceTripsLocal
{
	FTrip* TripFor(UWorld* World, const bool bCreate)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Trip] no game world"));
			return nullptr;
		}
		FTrip* Trip = APSSpaceTrips::Find(World, bCreate);
		if (!Trip)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Trip] no route running (aps.Trip.Start <id|n>)"));
		}
		return Trip;
	}

	FAutoConsoleCommandWithWorldAndArgs RoutesCommand(TEXT("aps.Trip.Routes"),
		TEXT("Lists the excursion routes this world offers (SPACE TRIPS): number, id, name, stops."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, true))
			{
				TArray<FRoute> Routes;
				Trip->BuildRoutes(Routes);
				for (int32 Index = 0; Index < Routes.Num(); ++Index)
				{
					const FRoute& Route = Routes[Index];
					FString Stops;
					for (const FStop& Stop : Route.Stops)
					{
						Stops += (Stops.IsEmpty() ? TEXT("") : TEXT(" > ")) + Stop.Name;
					}
					UE_LOG(LogTemp, Log, TEXT("[APS.Trip] %d %-8s %s: %s (%d stops: %s)"), Index, *Route.Id, *Route.Name.ToString(),
						*Route.Subtitle.ToString(), Route.Stops.Num(), *Stops);
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.Trip] %s"), *Trip->Describe());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs StartCommand(TEXT("aps.Trip.Start"),
		TEXT("Starts an excursion route by id or number (aps.Trip.Routes): the guide takes the helm."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, true))
			{
				Trip->Start(Args.Num() > 0 ? Args[0] : TEXT("home"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs NextCommand(TEXT("aps.Trip.Next"),
		TEXT("Skips to the route's next world, or the next stop."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) Trip->Next();
		}));

	FAutoConsoleCommandWithWorldAndArgs PauseCommand(TEXT("aps.Trip.Pause"),
		TEXT("The guide lets go of the helm; aps.Trip.Resume gives it back."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) Trip->Pause(LOCTEXT("PausedByHand", "The helm is yours."), true);
		}));

	FAutoConsoleCommandWithWorldAndArgs ResumeCommand(TEXT("aps.Trip.Resume"),
		TEXT("The guide takes the helm again after a pause."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) Trip->Resume();
		}));

	FAutoConsoleCommandWithWorldAndArgs StopCommand(TEXT("aps.Trip.Stop"),
		TEXT("Ends the route; the helm is the pilot's."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) Trip->Stop(LOCTEXT("StoppedByHand", "by your word"));
		}));

	FAutoConsoleCommandWithWorldAndArgs PhotoCommand(TEXT("aps.Trip.Photo"),
		TEXT("A still of the view without the HUD into Saved/Screenshots/Trips: aps.Trip.Photo [name]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, true)) Trip->Photo(Args.Num() > 0 ? Args[0] : FString());
		}));

	FAutoConsoleCommandWithWorldAndArgs EngineCommand(TEXT("aps.Trip.Engine"),
		TEXT("Test: the piloted ship's engine on, as G does (offscreen runs have no keys)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr;
			if (Ship && !Ship->GetEngineRunning())
			{
				Ship->SwitchEngines();
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Trip] engine %s"), Ship ? (Ship->GetEngineRunning() ? TEXT("on") : TEXT("off")) : TEXT("(no piloted ship)"));
		}));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(TEXT("aps.Trip.Status"),
		TEXT("Logs the route, the stop and what the guide is doing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) UE_LOG(LogTemp, Log, TEXT("[APS.Trip] %s"), *Trip->Describe());
		}));
}

#undef LOCTEXT_NAMESPACE
