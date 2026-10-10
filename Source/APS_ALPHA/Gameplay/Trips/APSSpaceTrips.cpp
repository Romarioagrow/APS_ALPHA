#include "APSSpaceTrips.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
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

	const FName GuideCategory(TEXT("Guide"));
	constexpr double LightYearCm = 9.4607e17;
	constexpr float AutoStartGraceSeconds = 6.0f;
	constexpr float ArrivingTimeoutSeconds = 60.0f;
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
			[](const FAPSStarSystemInfo& Info) { const FString L = SpectralLetters(Info.Spectral); return L == TEXT("M") || L == TEXT("K"); }},
		{TEXT("blue"), LOCTEXT("RouteBlue", "BLUE AND WHITE"), LOCTEXT("RouteBlueSub", "The hot young suns that burn bright and short."), 4,
			[](const FAPSStarSystemInfo& Info) { const FString L = SpectralLetters(Info.Spectral); return L == TEXT("O") || L == TEXT("B") || L == TEXT("A"); }},
		{TEXT("doubles"), LOCTEXT("RouteDoubles", "DOUBLES AND TRIPLES"), LOCTEXT("RouteDoublesSub", "Suns that share a sky."), 4,
			[](const FAPSStarSystemInfo& Info) { return Info.StarCount >= 2; }},
		{TEXT("dark"), LOCTEXT("RouteDark", "THE DARK"), LOCTEXT("RouteDarkSub", "What suns leave behind: dwarfs, neutron stars, the holes."), 3,
			[](const FAPSStarSystemInfo& Info) { const FString L = SpectralLetters(Info.Spectral); return L == TEXT("NS") || L == TEXT("BH") || L == TEXT("PS"); }},
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

void APSSpaceTrips::FTrip::Pause(const FText& Why)
{
	if (Phase == EPhase::Idle || Phase == EPhase::Done || Phase == EPhase::Paused)
	{
		return;
	}
	PausedFrom = Phase == EPhase::Hold ? EPhase::WorldLeg : Phase;
	Phase = EPhase::Paused;
	PhaseSeconds = 0.0f;
	Guide(FText::Format(LOCTEXT("Paused", "{0} aps.Trip.Resume brings the guide back, aps.Trip.Next moves on."), Why));
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
			Guide(LOCTEXT("Welcome", "Welcome aboard. This sky is for looking at: the guide flies, you watch. Routes: aps.Trip.Routes; a route: aps.Trip.Start home."));
		}
		if (!bAutoStarted && CVarAutoStart.GetValueOnGameThread() != 0)
		{
			PhaseSeconds += DeltaSeconds;
			if (PhaseSeconds >= AutoStartGraceSeconds && Ship && Ship->GetEngineRunning())
			{
				bAutoStarted = true;
				Start(TEXT("home"));
			}
		}
		return;
	}
	if (Phase == EPhase::Idle || Phase == EPhase::Done || Phase == EPhase::Paused)
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
		if (HoldSeconds >= FMath::Max(CVarHoldSeconds.GetValueOnGameThread(), 1.0f))
		{
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
	const bool bInside = Stars && Info && Stars->FindContaining(Ship.GetActorLocation()) == Stop.SystemIndex;
	Guide(FText::Format(LOCTEXT("StopBegin", "STOP {0} of {1}: {2}{3}"), FText::AsNumber(StopIndex + 1), FText::AsNumber(Route.Stops.Num()),
		FText::FromString(Stop.Name), Info && !bInside && !bHome
			? FText::Format(LOCTEXT("StopAway", ", {0} away."), DistanceText(FVector::Dist(Ship.GetActorLocation(), Info->Location)))
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
	const FVector ShipLocation = Ship.GetActorLocation();
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
	Guide(DescribeStar(Star, Stop));
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
	Guide(FText::Format(LOCTEXT("WorldNext", "Next: {0}, {1}, {2}."), Planet ? TypeName(Planet->PlanetType) : LOCTEXT("TypeBody", "A BODY"),
		NameOf(Ship, Body), DistanceText(Away)));
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
		Guide(DescribeWorld(Ship, Body));
		return;
	}
	Pause(LOCTEXT("HelmWorld", "You have the helm."));
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
	if (Info && Info->StarCount >= 2)
	{
		return FText::Format(LOCTEXT("StarMulti", "{Name}: {Suns} suns, the primary {Spectral}, {Worlds} worlds. {Wonder}"), Args);
	}
	return FText::Format(LOCTEXT("StarSingle", "{Name}: a {Spectral} sun with {Worlds} worlds. {Wonder}"), Args);
}

FText APSSpaceTrips::FTrip::DescribeWorld(const ASpaceship& Ship, const AActor* Body) const
{
	using namespace APSSpaceTripsLocal;
	const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body);
	if (!Planet)
	{
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
	if (Cast<AMoon>(Body))
	{
		return FText::Format(LOCTEXT("MoonLine", "{Name}, a moon: {Type}, {Radius} km across the middle. {Wonder}"), Args);
	}
	if (Moons > 0)
	{
		return FText::Format(LOCTEXT("PlanetMoonsLine", "{Name}: {Type}, {Radius} km, {Moons} moons. {Wonder}"), Args);
	}
	return FText::Format(LOCTEXT("PlanetLine", "{Name}: {Type}, {Radius} km. {Wonder}"), Args);
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
			if (FTrip* Trip = TripFor(World, false)) Trip->Pause(LOCTEXT("PausedByHand", "The helm is yours."));
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

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(TEXT("aps.Trip.Status"),
		TEXT("Logs the route, the stop and what the guide is doing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (FTrip* Trip = TripFor(World, false)) UE_LOG(LogTemp, Log, TEXT("[APS.Trip] %s"), *Trip->Describe());
		}));
}

#undef LOCTEXT_NAMESPACE
