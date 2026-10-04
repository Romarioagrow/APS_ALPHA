#include "APSWorldRoll.h"

#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetaryZoneType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetHabitability.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Generation/APSGalaxyMorphology.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/UI/MainMenu/APSAtmosphereControlBounds.h"
#include "CoreGlobals.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include <initializer_list>

namespace APSWorldRollPrivate
{
	constexpr double RollEarthRadiusKm = 6371.0;
	constexpr double RollSolarRadiusAu = 0.00465047;

	template <typename T>
	struct TRollOption
	{
		T Value;
		float Weight;
	};

	/** One weighted draw; the weights are relative and need not sum to one. */
	template <typename T>
	T PickWeighted(const FRandomStream& Random, std::initializer_list<TRollOption<T>> Options)
	{
		float Total = 0.0f;
		for (const TRollOption<T>& Option : Options) Total += FMath::Max(Option.Weight, 0.0f);
		float Remaining = Random.FRand() * Total;
		T Last = Options.begin()->Value;
		for (const TRollOption<T>& Option : Options)
		{
			if (Option.Weight <= 0.0f) continue;
			Last = Option.Value;
			Remaining -= Option.Weight;
			if (Remaining < 0.0f) return Option.Value;
		}
		return Last;
	}

	double RollBetween(const FRandomStream& Random, const double Min, const double Max)
	{
		return Min + (Max - Min) * Random.FRand();
	}

	/** The seed the panel's own star editor uses for an address, so a re-pick of the same class reproduces the star. */
	int32 StarEditSeed(const int32 WorldSeed, const FString& Address)
	{
		return static_cast<int32>(HashCombine(GetTypeHash(WorldSeed), FCrc::StrCrc32(*Address)) & 0x7fffffffu);
	}

	enum class EArchetype : uint8
	{
		None,
		HotJupiter,
		RedDwarfSwarm,
		GiantSun,
		MoonRichGiant,
		WaterWorld,
		BinarySuns,
		TripleSuns,
		CrowdedSystem,
		WhiteDwarfRemnant,
		BlueSun,
		LonelyWorld,
		TiltedSystem
	};

	const TCHAR* ArchetypeName(const EArchetype Archetype)
	{
		switch (Archetype)
		{
		case EArchetype::HotJupiter: return TEXT("hot-jupiter");
		case EArchetype::RedDwarfSwarm: return TEXT("red-dwarf-swarm");
		case EArchetype::GiantSun: return TEXT("giant-sun");
		case EArchetype::MoonRichGiant: return TEXT("moon-rich-giant");
		case EArchetype::WaterWorld: return TEXT("water-world");
		case EArchetype::BinarySuns: return TEXT("binary-suns");
		case EArchetype::TripleSuns: return TEXT("triple-suns");
		case EArchetype::CrowdedSystem: return TEXT("crowded-system");
		case EArchetype::WhiteDwarfRemnant: return TEXT("white-dwarf-remnant");
		case EArchetype::BlueSun: return TEXT("blue-sun");
		case EArchetype::LonelyWorld: return TEXT("lonely-world");
		case EArchetype::TiltedSystem: return TEXT("tilted-system");
		default: return TEXT("standard");
		}
	}

	struct FStarPick
	{
		EStellarType Type{EStellarType::MainSequence};
		ESpectralClass Class{ESpectralClass::G};
	};

	ESpectralClass RollMainSequenceClass(const FRandomStream& Random)
	{
		// Rio 03.10: mostly the common G/K/M suns, sometimes F/A, rarely O/B.
		return PickWeighted<ESpectralClass>(Random, {
			{ESpectralClass::M, 20.0f}, {ESpectralClass::K, 28.0f}, {ESpectralClass::G, 30.0f},
			{ESpectralClass::F, 13.0f}, {ESpectralClass::A, 6.0f}, {ESpectralClass::B, 2.2f},
			{ESpectralClass::O, 0.8f}});
	}

	/** A spectral class the stellar generator models for the type (remnants are never rolled). */
	ESpectralClass RollClassFor(const FRandomStream& Random, const EStellarType Type)
	{
		switch (Type)
		{
		case EStellarType::SubGiant:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::G, 45.0f}, {ESpectralClass::K, 35.0f}, {ESpectralClass::F, 20.0f}});
		case EStellarType::Giant:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::K, 50.0f}, {ESpectralClass::M, 35.0f}, {ESpectralClass::G, 15.0f}});
		case EStellarType::BrightGiant:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::K, 40.0f}, {ESpectralClass::M, 30.0f}, {ESpectralClass::G, 15.0f},
				{ESpectralClass::F, 15.0f}});
		case EStellarType::SuperGiant:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::M, 40.0f}, {ESpectralClass::B, 30.0f}, {ESpectralClass::A, 15.0f},
				{ESpectralClass::K, 15.0f}});
		case EStellarType::SubDwarf:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::K, 45.0f}, {ESpectralClass::M, 35.0f}, {ESpectralClass::G, 20.0f}});
		case EStellarType::WhiteDwarf:
			// The class the star editor gives a white dwarf.
			return ESpectralClass::A;
		case EStellarType::BrownDwarf:
			return PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::L, 50.0f}, {ESpectralClass::T, 35.0f}, {ESpectralClass::Y, 15.0f}});
		default:
			return RollMainSequenceClass(Random);
		}
	}

	FStarPick RollPrimary(const FRandomStream& Random)
	{
		// Evolved and degenerate suns stay rare; black holes, neutron stars and pulsars are never a home star.
		FStarPick Pick;
		Pick.Type = PickWeighted<EStellarType>(Random, {
			{EStellarType::MainSequence, 88.0f}, {EStellarType::SubGiant, 4.0f}, {EStellarType::Giant, 2.5f},
			{EStellarType::SubDwarf, 2.0f}, {EStellarType::WhiteDwarf, 1.5f}, {EStellarType::BrownDwarf, 0.8f},
			{EStellarType::BrightGiant, 0.7f}, {EStellarType::SuperGiant, 0.5f}});
		Pick.Class = RollClassFor(Random, Pick.Type);
		return Pick;
	}

	FStarPick RollCompanion(const FRandomStream& Random)
	{
		// Companions are mostly smaller, cooler suns, now and then a white or brown dwarf.
		FStarPick Pick;
		Pick.Type = PickWeighted<EStellarType>(Random, {
			{EStellarType::MainSequence, 84.0f}, {EStellarType::WhiteDwarf, 9.0f}, {EStellarType::BrownDwarf, 7.0f}});
		Pick.Class = Pick.Type != EStellarType::MainSequence ? RollClassFor(Random, Pick.Type)
			: PickWeighted<ESpectralClass>(Random, {
				{ESpectralClass::M, 45.0f}, {ESpectralClass::K, 27.0f}, {ESpectralClass::G, 15.0f},
				{ESpectralClass::F, 8.0f}, {ESpectralClass::A, 5.0f}});
		return Pick;
	}

	int32 RollStarCount(const FRandomStream& Random)
	{
		return PickWeighted<int32>(Random, {
			{1, 58.0f}, {2, 26.0f}, {3, 10.0f}, {4, 4.0f}, {5, 1.5f}, {6, 0.5f}});
	}

	EStarType StarTypeFor(const int32 StarCount)
	{
		return StarCount <= 1 ? EStarType::SingleStar : StarCount == 2 ? EStarType::DoubleStar
			: StarCount == 3 ? EStarType::TripleStar : EStarType::MultipleStar;
	}

	/** Planets in the whole system: Sun-like stars carry the richest families, evolved and dwarf suns fewer. */
	int32 RollPlanetTotal(const FRandomStream& Random, const FStarPick& Primary)
	{
		switch (Primary.Type)
		{
		case EStellarType::SubGiant:
			return PickWeighted<int32>(Random, {{1, 12.0f}, {2, 22.0f}, {3, 24.0f}, {4, 20.0f}, {5, 12.0f}, {6, 10.0f}});
		case EStellarType::Giant:
		case EStellarType::BrightGiant:
		case EStellarType::SuperGiant:
			return PickWeighted<int32>(Random, {{1, 22.0f}, {2, 30.0f}, {3, 26.0f}, {4, 14.0f}, {5, 8.0f}});
		case EStellarType::WhiteDwarf:
			return PickWeighted<int32>(Random, {{1, 30.0f}, {2, 32.0f}, {3, 24.0f}, {4, 14.0f}});
		case EStellarType::BrownDwarf:
			return PickWeighted<int32>(Random, {{1, 35.0f}, {2, 35.0f}, {3, 30.0f}});
		case EStellarType::SubDwarf:
			return PickWeighted<int32>(Random, {{1, 15.0f}, {2, 25.0f}, {3, 25.0f}, {4, 20.0f}, {5, 15.0f}});
		default:
			break;
		}
		if (Primary.Class == ESpectralClass::O || Primary.Class == ESpectralClass::B || Primary.Class == ESpectralClass::A)
			return PickWeighted<int32>(Random, {{1, 10.0f}, {2, 18.0f}, {3, 22.0f}, {4, 20.0f}, {5, 15.0f}, {6, 9.0f}, {7, 6.0f}});
		if (Primary.Class == ESpectralClass::M)
			return PickWeighted<int32>(Random, {
				{1, 5.0f}, {2, 9.0f}, {3, 14.0f}, {4, 16.0f}, {5, 16.0f}, {6, 14.0f}, {7, 11.0f}, {8, 8.0f}, {9, 5.0f},
				{10, 2.0f}});
		return PickWeighted<int32>(Random, {
			{1, 4.0f}, {2, 7.0f}, {3, 11.0f}, {4, 14.0f}, {5, 15.0f}, {6, 14.0f}, {7, 11.0f}, {8, 9.0f}, {9, 6.0f},
			{10, 4.0f}, {11, 3.0f}, {12, 2.0f}});
	}

	EOrbitDistributionType RollDistribution(const FRandomStream& Random, const FStarPick& Primary)
	{
		const bool bGiant = Primary.Type == EStellarType::Giant || Primary.Type == EStellarType::BrightGiant
			|| Primary.Type == EStellarType::SuperGiant || Primary.Type == EStellarType::SubGiant;
		const bool bDwarf = Primary.Type == EStellarType::WhiteDwarf || Primary.Type == EStellarType::BrownDwarf
			|| Primary.Type == EStellarType::SubDwarf || Primary.Class == ESpectralClass::M;
		if (bGiant)
			return PickWeighted<EOrbitDistributionType>(Random, {
				{EOrbitDistributionType::InnerOuter, 30.0f}, {EOrbitDistributionType::Chaotic, 30.0f},
				{EOrbitDistributionType::Gaussian, 20.0f}, {EOrbitDistributionType::Uniform, 20.0f}});
		if (bDwarf)
			return PickWeighted<EOrbitDistributionType>(Random, {
				{EOrbitDistributionType::Dense, 40.0f}, {EOrbitDistributionType::Uniform, 30.0f},
				{EOrbitDistributionType::Gaussian, 20.0f}, {EOrbitDistributionType::Chaotic, 10.0f}});
		return PickWeighted<EOrbitDistributionType>(Random, {
			{EOrbitDistributionType::Uniform, 32.0f}, {EOrbitDistributionType::InnerOuter, 23.0f},
			{EOrbitDistributionType::Gaussian, 20.0f}, {EOrbitDistributionType::Dense, 14.0f},
			{EOrbitDistributionType::Chaotic, 11.0f}});
	}

	double RollInclination(const FRandomStream& Random)
	{
		// Mostly a near-flat family like the default 8 degrees; seldom a visibly tilted one.
		const int32 Band = PickWeighted<int32>(Random, {{0, 15.0f}, {1, 70.0f}, {2, 12.0f}, {3, 3.0f}});
		return Band == 0 ? RollBetween(Random, 0.0, 2.0) : Band == 1 ? RollBetween(Random, 2.0, 8.0)
			: Band == 2 ? RollBetween(Random, 8.0, 18.0) : RollBetween(Random, 20.0, 40.0);
	}

	/** The start world's family by the orbital zone it lands in. Every entry has a WorldScape surface. */
	EPlanetType RollStartWorldType(const FRandomStream& Random, const EPlanetaryZoneType Zone)
	{
		switch (Zone)
		{
		case EPlanetaryZoneType::HabitableZone:
			return PickWeighted<EPlanetType>(Random, {
				{EPlanetType::Terrestrial, 20.0f}, {EPlanetType::Forest, 12.0f}, {EPlanetType::Ocean, 9.0f},
				{EPlanetType::Archipelago, 8.0f}, {EPlanetType::Pangea, 8.0f}, {EPlanetType::Savanna, 8.0f},
				{EPlanetType::SuperEarth, 8.0f}, {EPlanetType::Nordic, 6.0f}, {EPlanetType::Oasis, 5.0f},
				{EPlanetType::HighMountain, 5.0f}, {EPlanetType::Water, 4.0f}, {EPlanetType::Desert, 4.0f},
				{EPlanetType::Tundra, 3.0f}});
		case EPlanetaryZoneType::WarmZone:
			return PickWeighted<EPlanetType>(Random, {
				{EPlanetType::Desert, 22.0f}, {EPlanetType::Sand, 14.0f}, {EPlanetType::Savanna, 14.0f},
				{EPlanetType::Oasis, 10.0f}, {EPlanetType::Terrestrial, 10.0f}, {EPlanetType::Greenhouse, 8.0f},
				{EPlanetType::HighMountain, 6.0f}, {EPlanetType::Pangea, 6.0f}, {EPlanetType::Volcanic, 5.0f},
				{EPlanetType::Basalt, 5.0f}});
		case EPlanetaryZoneType::HotZone:
			// Scorched but standable: no lava or molten start worlds.
			return PickWeighted<EPlanetType>(Random, {
				{EPlanetType::Desert, 20.0f}, {EPlanetType::Volcanic, 18.0f}, {EPlanetType::Basalt, 16.0f},
				{EPlanetType::Sand, 14.0f}, {EPlanetType::Rocky, 12.0f}, {EPlanetType::Greenhouse, 8.0f},
				{EPlanetType::Metal, 6.0f}, {EPlanetType::Sulfur, 6.0f}});
		case EPlanetaryZoneType::ColdZone:
			return PickWeighted<EPlanetType>(Random, {
				{EPlanetType::Tundra, 24.0f}, {EPlanetType::Nordic, 20.0f}, {EPlanetType::Frozen, 16.0f},
				{EPlanetType::Ice, 10.0f}, {EPlanetType::HighMountain, 10.0f}, {EPlanetType::Rocky, 8.0f},
				{EPlanetType::Basalt, 6.0f}, {EPlanetType::Terrestrial, 6.0f}});
		default:
			return PickWeighted<EPlanetType>(Random, {
				{EPlanetType::Frozen, 34.0f}, {EPlanetType::Ice, 30.0f}, {EPlanetType::Tundra, 10.0f},
				{EPlanetType::Rocky, 10.0f}, {EPlanetType::Crystal, 6.0f}, {EPlanetType::Ammonia, 5.0f},
				{EPlanetType::Basalt, 5.0f}});
		}
	}

	/** The planetary generator's radius ranges in Earth radii; a start world is never smaller than half an Earth. */
	void RadiusRangeEarth(const EPlanetType Type, double& OutMin, double& OutMax)
	{
		switch (Type)
		{
		case EPlanetType::SuperEarth: OutMin = 1.3; OutMax = 2.0; return;
		case EPlanetType::Forest:
		case EPlanetType::Savanna: OutMin = 0.9; OutMax = 1.3; return;
		case EPlanetType::Terrestrial:
		case EPlanetType::Greenhouse: OutMin = 0.9; OutMax = 1.1; return;
		case EPlanetType::Ocean:
		case EPlanetType::Desert:
		case EPlanetType::Volcanic:
		case EPlanetType::Metal:
		case EPlanetType::Carbon:
		case EPlanetType::Lava: OutMin = 0.8; OutMax = 1.1; return;
		case EPlanetType::Metallic: OutMin = 0.8; OutMax = 1.3; return;
		case EPlanetType::Crystal: OutMin = 0.6; OutMax = 1.4; return;
		case EPlanetType::Ammonia:
		case EPlanetType::Sulfur: OutMin = 0.6; OutMax = 0.9; return;
		case EPlanetType::Rocky:
		case EPlanetType::Basalt: OutMin = 0.5; OutMax = 0.9; return;
		case EPlanetType::Ice:
		case EPlanetType::Frozen: OutMin = 0.5; OutMax = 0.8; return;
		case EPlanetType::HotGiant: OutMin = 9.4; OutMax = 12.6; return;
		case EPlanetType::GasGiant: OutMin = 7.9; OutMax = 11.0; return;
		case EPlanetType::IceGiant: OutMin = 3.1; OutMax = 7.9; return;
		// Water, Nordic, Tundra, HighMountain, Sand, Oasis, Archipelago, Pangea: Earth-like sizes.
		default: OutMin = 0.75; OutMax = 1.15; return;
		}
	}

	int32 RollStartWorldMoons(const FRandomStream& Random)
	{
		return PickWeighted<int32>(Random, {
			{0, 30.0f}, {1, 34.0f}, {2, 18.0f}, {3, 9.0f}, {4, 5.0f}, {5, 2.0f}, {6, 1.0f}, {7, 1.0f}});
	}

	/** Where the planetary generator puts a family's orbits and its zones around the same star model. */
	struct FFamilyForecast
	{
		TArray<double> OrbitsAu;
		double HabitableInnerAu{0.95};
		double HabitableOuterAu{1.37};
		double HotOuterAu{0.5};

		EPlanetaryZoneType ZoneOf(const double OrbitAu) const
		{
			if (OrbitAu < HotOuterAu) return EPlanetaryZoneType::HotZone;
			if (OrbitAu < HabitableInnerAu) return EPlanetaryZoneType::WarmZone;
			if (OrbitAu <= HabitableOuterAu) return EPlanetaryZoneType::HabitableZone;
			if (OrbitAu <= HabitableOuterAu * 2.0) return EPlanetaryZoneType::ColdZone;
			return EPlanetaryZoneType::IceZone;
		}
	};

	/** The compact range, layout and habitable-zone rule of GenerateCustomPlanetarySystemModel. The layout is exact
	 * for every recipe except Chaotic, whose jitter comes from the system's own stream. */
	FFamilyForecast ForecastFamily(const FStarModel& Star, const int32 PlanetCount,
		const EOrbitDistributionType Distribution, const int32 LayoutSeed)
	{
		FFamilyForecast Forecast;
		const double Mass = FMath::IsFinite(Star.Mass) && Star.Mass > 0.0 ? Star.Mass : 1.0;
		const double RadiusSolar = FMath::IsFinite(Star.Radius) && Star.Radius > 0.0 ? Star.Radius : 1.0;
		const double Luminosity = FMath::IsFinite(Star.Luminosity) && Star.Luminosity > 0.0 ? Star.Luminosity : 1.0;
		const double MaxScaling = Star.StellarType == EStellarType::HyperGiant ? 5.0
			: Star.StellarType == EStellarType::SuperGiant ? 6.0 : 10.0;
		double MinOrbit = Mass;
		double MaxOrbit = Mass * MaxScaling;
		UPlanetarySystemGenerator::CompactPlanetOrbitRange(MinOrbit, MaxOrbit, RadiusSolar);
		FRandomStream LayoutRandom(LayoutSeed);
		Forecast.OrbitsAu = UPlanetarySystemGenerator::BuildPlanetOrbitLayout(
			PlanetCount, Distribution, MinOrbit, MaxOrbit, RadiusSolar, LayoutRandom);
		Forecast.HabitableInnerAu = FMath::Sqrt(Luminosity / 1.1);
		Forecast.HabitableOuterAu = FMath::Sqrt(Luminosity / 0.53);
		const double StarAu = RadiusSolar * RollSolarRadiusAu;
		Forecast.HotOuterAu = StarAu + FMath::Max(Forecast.HabitableInnerAu - StarAu, 0.0) * 0.5;
		return Forecast;
	}

	/** The orbit nearest the middle of the habitable zone (log distance); with bAllowSecond, now and then the next one. */
	int32 ChooseStartSlot(const FRandomStream& Random, const FFamilyForecast& Forecast, const TSet<int32>& Reserved,
		const bool bAllowSecond)
	{
		const double Middle = FMath::Sqrt(Forecast.HabitableInnerAu * Forecast.HabitableOuterAu);
		TArray<TPair<double, int32>> Ranked;
		for (int32 Slot = 0; Slot < Forecast.OrbitsAu.Num(); ++Slot)
		{
			if (Reserved.Contains(Slot) || !FMath::IsFinite(Forecast.OrbitsAu[Slot]) || Forecast.OrbitsAu[Slot] <= 0.0)
				continue;
			Ranked.Emplace(FMath::Abs(FMath::Loge(Forecast.OrbitsAu[Slot] / FMath::Max(Middle, 1.0e-6))), Slot);
		}
		if (Ranked.IsEmpty()) return 0;
		Ranked.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });
		return bAllowSecond && Ranked.Num() > 1 && Random.FRand() < 0.25f ? Ranked[1].Value : Ranked[0].Value;
	}

	/** A planet the archetype shapes itself (type, size, moons), stored as an ordinary body edit of the panel. */
	void WriteBodyEdit(UGeneratedWorld& World, const int32 Slot, const EPlanetType Type, const double RadiusEarth,
		const int32 Moons, const EPlanetaryZoneType Zone)
	{
		FAPSPreviewBodyEditOverride Body;
		Body.PlanetType = Type;
		Body.RadiusKm = RadiusEarth * RollEarthRadiusKm;
		Body.MoonCount = FMath::Clamp(Moons, 0, 10);
		// Zero is the Auto surface seed, resolved from the world seed and this body's address.
		Body.SurfaceSeed = 0;
		// The generator's own atmosphere height for any planet: radius / 30.
		Body.AtmosphereHeight = APSAtmosphereControlBounds::Height(Body.RadiusKm / 30.0, Type);
		Body.AtmosphereRayleighScattering = APSAtmosphereControlBounds::Rayleigh(Body.AtmosphereRayleighScattering, Type);
		Body.PlanetHabitability = UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
			Type, Zone, Body.AtmosphereHeight);
		World.SetPreviewBodyEditOverride(FString::Printf(TEXT("SYS0/S0/P%d"), Slot), Body);
	}

	/** The start world: the panel's editor buffer, with surface, atmosphere and cloud defaults for its new type. */
	void WriteStartWorld(UGeneratedWorld& World, const FRandomStream& Random, EPlanetType Type,
		const EPlanetaryZoneType Zone, const int32 Moons)
	{
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Type) || !APSPlanetTypes::IsSelectable(Type))
		{
			Type = EPlanetType::Terrestrial;
		}
		const UGeneratedWorld* Defaults = GetDefault<UGeneratedWorld>();
		double MinRadius = 0.9;
		double MaxRadius = 1.1;
		RadiusRangeEarth(Type, MinRadius, MaxRadius);
		World.PlanetType = Type;
		World.PlanetRadius = FMath::RoundToDouble(RollBetween(Random, MinRadius, MaxRadius) * RollEarthRadiusKm);
		World.MoonsAmount = FMath::Clamp(Moons, 0, 10);
		World.MoonOrbitRadiusKm = 0.0;
		// Rio 03.10: a rolled world takes the Auto surface seed (zero), never the fixed default 1337.
		World.PlanetSurfaceSeed = 0;
		World.SurfaceFeatureScale = Defaults->SurfaceFeatureScale;
		World.SurfaceReliefScale = Defaults->SurfaceReliefScale;
		World.SurfaceLandCoverageScale = Defaults->SurfaceLandCoverageScale;
		World.SurfaceMountainScale = Defaults->SurfaceMountainScale;
		World.SurfaceCraterScale = Defaults->SurfaceCraterScale;
		World.SurfaceRoughnessScale = Defaults->SurfaceRoughnessScale;
		World.AtmosphereHeight = APSAtmosphereControlBounds::Height(Defaults->AtmosphereHeight, Type);
		World.AtmosphereOpacity = Defaults->AtmosphereOpacity;
		World.AtmosphereMultiScattering = Defaults->AtmosphereMultiScattering;
		World.AtmosphereRayleighScattering = APSAtmosphereControlBounds::Rayleigh(Defaults->AtmosphereRayleighScattering, Type);
		World.AtmosphereColor = Defaults->AtmosphereColor;
		World.CloudSettings = Defaults->CloudSettings.Sanitized();
		World.PlanetHabitability = UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
			Type, Zone, World.AtmosphereHeight);
	}

	EArchetype RollArchetype(const FRandomStream& Random)
	{
		return PickWeighted<EArchetype>(Random, {
			{EArchetype::HotJupiter, 11.0f}, {EArchetype::RedDwarfSwarm, 10.0f}, {EArchetype::GiantSun, 7.0f},
			{EArchetype::MoonRichGiant, 11.0f}, {EArchetype::WaterWorld, 11.0f}, {EArchetype::BinarySuns, 10.0f},
			{EArchetype::TripleSuns, 7.0f}, {EArchetype::CrowdedSystem, 8.0f}, {EArchetype::WhiteDwarfRemnant, 5.0f},
			{EArchetype::BlueSun, 6.0f}, {EArchetype::LonelyWorld, 7.0f}, {EArchetype::TiltedSystem, 7.0f}});
	}

	FString EnumName(const UEnum* Enum, const int64 Value)
	{
		return Enum ? Enum->GetNameStringByValue(Value) : FString::Printf(TEXT("%lld"), Value);
	}

	/** A star of the rolled class, as the panel's star editor would create it for this address. */
	FStarModel MakeStar(UStarGenerator& Stars, const FStarPick& Pick, const int32 WorldSeed, const FString& Address)
	{
		Stars.SetGenerationSeed(StarEditSeed(WorldSeed, Address));
		TSharedPtr<FStarModel> Model = MakeShared<FStarModel>();
		Model->StellarType = Pick.Type;
		Model->SpectralClass = Pick.Class;
		Stars.GenerateStarModel(Model);
		return *Model;
	}

	// ---- Galaxy and home cluster (Rio 03.10: "REGENERATE should roll the galaxy and clusters too, all
	// possible parameters"). Written to the same model fields the GALAXY and STAR CLUSTER pages edit. ----

	/** The galaxy recipe of one roll. */
	struct FSkyPick
	{
		EGalaxyType GalaxyType{EGalaxyType::Spiral};
		EGalaxyClass GalaxyClass{EGalaxyClass::SpiralSb};
		int32 GalaxySize{250};
		double GalaxyDensity{10.0};
		EStarClusterType ClusterType{EStarClusterType::RingArc};
		EStarClusterSize ClusterSize{EStarClusterSize::Giant};
		EStarClusterPopulation Population{EStarClusterPopulation::AllSequenses};
		EStarClusterComposition Composition{EStarClusterComposition::AllSpectral};
	};

	enum class ESkyArchetype : uint8
	{
		None,
		BarredGrandDesign,
		RingGalaxy,
		DwarfIrregular,
		GlobularRichElliptical,
		StarburstAssociation,
		CollidingGalaxies,
		MagellanicCloud
	};

	const TCHAR* SkyArchetypeName(const ESkyArchetype Archetype)
	{
		switch (Archetype)
		{
		case ESkyArchetype::BarredGrandDesign: return TEXT("barred-grand-design");
		case ESkyArchetype::RingGalaxy: return TEXT("ring-galaxy");
		case ESkyArchetype::DwarfIrregular: return TEXT("dwarf-irregular");
		case ESkyArchetype::GlobularRichElliptical: return TEXT("globular-rich-elliptical");
		case ESkyArchetype::StarburstAssociation: return TEXT("starburst-association");
		case ESkyArchetype::CollidingGalaxies: return TEXT("colliding-galaxies");
		case ESkyArchetype::MagellanicCloud: return TEXT("magellanic-cloud");
		default: return TEXT("standard");
		}
	}

	/** Relative weight of a per-type subclass; the classic Sb/SBb lead their types. Legacy classes are never rolled. */
	float GalaxySubclassWeight(const EGalaxyClass Class)
	{
		switch (Class)
		{
		case EGalaxyClass::E0: case EGalaxyClass::E1: case EGalaxyClass::E2: return 12.0f;
		case EGalaxyClass::E3: return 11.0f;
		case EGalaxyClass::E4: return 10.0f;
		case EGalaxyClass::E5: return 9.0f;
		case EGalaxyClass::E6: return 7.0f;
		case EGalaxyClass::E7: return 5.0f;
		case EGalaxyClass::cD: return 10.0f;
		case EGalaxyClass::dE: return 12.0f;
		case EGalaxyClass::S0: return 45.0f;
		case EGalaxyClass::S0a: return 30.0f;
		case EGalaxyClass::SB0: return 25.0f;
		case EGalaxyClass::SpiralSa: case EGalaxyClass::BarredSBa: return 8.0f;
		case EGalaxyClass::SpiralSab: case EGalaxyClass::BarredSBab: return 10.0f;
		case EGalaxyClass::SpiralSb: case EGalaxyClass::BarredSBb: return 20.0f;
		case EGalaxyClass::SpiralSbc: case EGalaxyClass::BarredSBbc: return 16.0f;
		case EGalaxyClass::SpiralSc: case EGalaxyClass::BarredSBc: return 18.0f;
		case EGalaxyClass::SpiralScd: case EGalaxyClass::BarredSBcd: return 12.0f;
		case EGalaxyClass::SpiralSd: case EGalaxyClass::BarredSBd: return 9.0f;
		case EGalaxyClass::SpiralSm: case EGalaxyClass::BarredSBm: return 7.0f;
		case EGalaxyClass::Irr: return 30.0f;
		case EGalaxyClass::Im: return 25.0f;
		case EGalaxyClass::IBm: return 15.0f;
		case EGalaxyClass::dIrr: return 20.0f;
		case EGalaxyClass::I0: return 10.0f;
		case EGalaxyClass::PecWarped: return 20.0f;
		case EGalaxyClass::PecRing: case EGalaxyClass::PecInteracting: return 22.0f;
		case EGalaxyClass::PecTidalTails: return 20.0f;
		case EGalaxyClass::PecPolarRing: return 16.0f;
		default: return 0.0f;
		}
	}

	EGalaxyClass RollGalaxyClass(const FRandomStream& Random, const EGalaxyType Type)
	{
		const TConstArrayView<EGalaxyClass> Subclasses = APSGalaxyMorphology::GetSubclasses(Type);
		float Total = 0.0f;
		for (const EGalaxyClass Class : Subclasses) Total += GalaxySubclassWeight(Class);
		float Remaining = Random.FRand() * Total;
		for (const EGalaxyClass Class : Subclasses)
		{
			Remaining -= GalaxySubclassWeight(Class);
			if (GalaxySubclassWeight(Class) > 0.0f && Remaining < 0.0f) return Class;
		}
		return APSGalaxyMorphology::GetDefaultSubclass(Type);
	}

	/** GALAXY SIZE (x 50,000 catalogue units of radius): dwarfs small, cD and grand designs large; 250 is the default. */
	int32 RollGalaxySize(const FRandomStream& Random, const EGalaxyClass Class)
	{
		double Min = 210.0;
		double Max = 340.0;
		switch (Class)
		{
		case EGalaxyClass::dE: case EGalaxyClass::dIrr: Min = 110.0; Max = 180.0; break;
		case EGalaxyClass::Im: case EGalaxyClass::IBm: case EGalaxyClass::SpiralSm: case EGalaxyClass::BarredSBm:
			Min = 140.0; Max = 220.0; break;
		case EGalaxyClass::Irr: case EGalaxyClass::I0: Min = 160.0; Max = 260.0; break;
		case EGalaxyClass::cD: Min = 380.0; Max = 520.0; break;
		case EGalaxyClass::E0: case EGalaxyClass::E1: case EGalaxyClass::E2: case EGalaxyClass::E3:
		case EGalaxyClass::E4: case EGalaxyClass::E5: case EGalaxyClass::E6: case EGalaxyClass::E7:
			Min = 240.0; Max = 420.0; break;
		case EGalaxyClass::SpiralSa: case EGalaxyClass::SpiralSab: case EGalaxyClass::SpiralSb:
		case EGalaxyClass::BarredSBa: case EGalaxyClass::BarredSBab: case EGalaxyClass::BarredSBb:
			Min = 240.0; Max = 380.0; break;
		default: break;
		}
		return FMath::RoundToInt(RollBetween(Random, Min, Max));
	}

	/** The home cluster's formation fits its galaxy: old ellipticals favour globulars, starbursts young associations. */
	EStarClusterType RollClusterType(const FRandomStream& Random, const EGalaxyType GalaxyType)
	{
		switch (GalaxyType)
		{
		case EGalaxyType::Elliptical:
		case EGalaxyType::Lenticular:
			return PickWeighted<EStarClusterType>(Random, {
				{EStarClusterType::GlobularCluster, 18.0f}, {EStarClusterType::Supercluster, 14.0f},
				{EStarClusterType::RingArc, 10.0f}, {EStarClusterType::ElongatedStream, 10.0f},
				{EStarClusterType::MovingGroup, 10.0f}, {EStarClusterType::OpenCluster, 6.0f},
				{EStarClusterType::DoubleCluster, 6.0f}, {EStarClusterType::Hourglass, 6.0f},
				{EStarClusterType::Nebula, 6.0f}, {EStarClusterType::SuperStarCluster, 6.0f},
				{EStarClusterType::YoungAssociation, 4.0f}, {EStarClusterType::EmbeddedCluster, 4.0f}});
		case EGalaxyType::Irregular:
			return PickWeighted<EStarClusterType>(Random, {
				{EStarClusterType::YoungAssociation, 16.0f}, {EStarClusterType::EmbeddedCluster, 14.0f},
				{EStarClusterType::SuperStarCluster, 12.0f}, {EStarClusterType::DoubleCluster, 9.0f},
				{EStarClusterType::MovingGroup, 8.0f}, {EStarClusterType::RingArc, 8.0f},
				{EStarClusterType::OpenCluster, 7.0f}, {EStarClusterType::Nebula, 7.0f},
				{EStarClusterType::ElongatedStream, 6.0f}, {EStarClusterType::Hourglass, 5.0f},
				{EStarClusterType::Supercluster, 4.0f}, {EStarClusterType::GlobularCluster, 4.0f}});
		case EGalaxyType::Peculiar:
			return PickWeighted<EStarClusterType>(Random, {
				{EStarClusterType::SuperStarCluster, 16.0f}, {EStarClusterType::EmbeddedCluster, 12.0f},
				{EStarClusterType::ElongatedStream, 12.0f}, {EStarClusterType::YoungAssociation, 12.0f},
				{EStarClusterType::RingArc, 9.0f}, {EStarClusterType::Hourglass, 8.0f},
				{EStarClusterType::Nebula, 8.0f}, {EStarClusterType::DoubleCluster, 7.0f},
				{EStarClusterType::MovingGroup, 6.0f}, {EStarClusterType::Supercluster, 5.0f},
				{EStarClusterType::OpenCluster, 3.0f}, {EStarClusterType::GlobularCluster, 3.0f}});
		default:
			// Spiral and barred spiral disks: the accepted Ring / Arc leads.
			return PickWeighted<EStarClusterType>(Random, {
				{EStarClusterType::RingArc, 14.0f}, {EStarClusterType::YoungAssociation, 12.0f},
				{EStarClusterType::Nebula, 11.0f}, {EStarClusterType::EmbeddedCluster, 9.0f},
				{EStarClusterType::DoubleCluster, 8.0f}, {EStarClusterType::ElongatedStream, 8.0f},
				{EStarClusterType::MovingGroup, 8.0f}, {EStarClusterType::OpenCluster, 7.0f},
				{EStarClusterType::SuperStarCluster, 7.0f}, {EStarClusterType::Supercluster, 6.0f},
				{EStarClusterType::Hourglass, 5.0f}, {EStarClusterType::GlobularCluster, 5.0f}});
		}
	}

	/**
	 * Small..Giant only. Tiny reads as an empty sky and Colossal stores 50-100k sealed systems in every save
	 * (~110-220 MB), so a random press never picks it; it stays a deliberate choice on the STAR CLUSTER page.
	 */
	EStarClusterSize RollClusterSize(const FRandomStream& Random, const EStarClusterType Type)
	{
		switch (Type)
		{
		case EStarClusterType::MovingGroup:
		case EStarClusterType::YoungAssociation:
		case EStarClusterType::DoubleCluster:
		case EStarClusterType::OpenCluster:
			return PickWeighted<EStarClusterSize>(Random, {
				{EStarClusterSize::Small, 22.0f}, {EStarClusterSize::Medium, 32.0f},
				{EStarClusterSize::Large, 28.0f}, {EStarClusterSize::Giant, 18.0f}});
		case EStarClusterType::SuperStarCluster:
		case EStarClusterType::GlobularCluster:
		case EStarClusterType::Supercluster:
			return PickWeighted<EStarClusterSize>(Random, {
				{EStarClusterSize::Small, 6.0f}, {EStarClusterSize::Medium, 20.0f},
				{EStarClusterSize::Large, 34.0f}, {EStarClusterSize::Giant, 40.0f}});
		default:
			return PickWeighted<EStarClusterSize>(Random, {
				{EStarClusterSize::Small, 15.0f}, {EStarClusterSize::Medium, 25.0f},
				{EStarClusterSize::Large, 30.0f}, {EStarClusterSize::Giant, 30.0f}});
		}
	}

	bool IsYoungFormation(const EStarClusterType Type)
	{
		return Type == EStarClusterType::YoungAssociation || Type == EStarClusterType::EmbeddedCluster
			|| Type == EStarClusterType::SuperStarCluster;
	}

	EStarClusterPopulation RollClusterPopulation(const FRandomStream& Random, const EStarClusterType Type)
	{
		if (Type == EStarClusterType::YoungAssociation || Type == EStarClusterType::EmbeddedCluster)
			return PickWeighted<EStarClusterPopulation>(Random, {
				{EStarClusterPopulation::MainSequence, 40.0f}, {EStarClusterPopulation::AllSequenses, 30.0f},
				{EStarClusterPopulation::Protostars, 30.0f}});
		if (Type == EStarClusterType::SuperStarCluster)
			return PickWeighted<EStarClusterPopulation>(Random, {
				{EStarClusterPopulation::MainSequence, 45.0f}, {EStarClusterPopulation::AllSequenses, 40.0f},
				{EStarClusterPopulation::Giants, 15.0f}});
		if (Type == EStarClusterType::GlobularCluster)
			return PickWeighted<EStarClusterPopulation>(Random, {
				{EStarClusterPopulation::AllSequenses, 40.0f}, {EStarClusterPopulation::Giants, 35.0f},
				{EStarClusterPopulation::Dwarfs, 25.0f}});
		return PickWeighted<EStarClusterPopulation>(Random, {
			{EStarClusterPopulation::AllSequenses, 50.0f}, {EStarClusterPopulation::MainSequence, 25.0f},
			{EStarClusterPopulation::Dwarfs, 10.0f}, {EStarClusterPopulation::Giants, 8.0f},
			{EStarClusterPopulation::Protostars, 7.0f}});
	}

	/** Mostly the full spectrum; young formations lean blue, globulars orange and red; a single colour stays rare. */
	EStarClusterComposition RollClusterComposition(const FRandomStream& Random, const EStarClusterType Type)
	{
		if (IsYoungFormation(Type))
			return PickWeighted<EStarClusterComposition>(Random, {
				{EStarClusterComposition::AllSpectral, 30.0f}, {EStarClusterComposition::BlueWhite, 22.0f},
				{EStarClusterComposition::MostlyBlue, 20.0f}, {EStarClusterComposition::MostlyWhite, 10.0f},
				{EStarClusterComposition::WhiteYellow, 6.0f}, {EStarClusterComposition::OnlyBlue, 4.0f},
				{EStarClusterComposition::OnlyWhite, 2.0f}});
		if (Type == EStarClusterType::GlobularCluster)
			return PickWeighted<EStarClusterComposition>(Random, {
				{EStarClusterComposition::AllSpectral, 25.0f}, {EStarClusterComposition::MostlyOrange, 20.0f},
				{EStarClusterComposition::OrangeRed, 18.0f}, {EStarClusterComposition::MostlyRed, 14.0f},
				{EStarClusterComposition::YellowOrange, 12.0f}, {EStarClusterComposition::OnlyRed, 3.0f},
				{EStarClusterComposition::OnlyOrange, 3.0f}});
		return PickWeighted<EStarClusterComposition>(Random, {
			{EStarClusterComposition::AllSpectral, 55.0f}, {EStarClusterComposition::MostlyBlue, 4.0f},
			{EStarClusterComposition::MostlyWhite, 4.0f}, {EStarClusterComposition::MostlyYellow, 4.0f},
			{EStarClusterComposition::MostlyOrange, 4.0f}, {EStarClusterComposition::MostlyRed, 4.0f},
			{EStarClusterComposition::BlueWhite, 3.0f}, {EStarClusterComposition::WhiteYellow, 3.0f},
			{EStarClusterComposition::YellowOrange, 3.0f}, {EStarClusterComposition::OrangeRed, 3.0f},
			{EStarClusterComposition::OnlyBlue, 1.0f}, {EStarClusterComposition::OnlyWhite, 1.0f},
			{EStarClusterComposition::OnlyYellow, 1.0f}, {EStarClusterComposition::OnlyOrange, 1.0f},
			{EStarClusterComposition::OnlyRed, 1.0f}});
	}

	/**
	 * Rio 03.10 ("REGENERATE changes everything except STARS"): the placed star count follows the rolled galaxy,
	 * log-normal around a modest median while the menu re-projects every point on the CPU (dwarfs ~6k, most types
	 * ~15k, cD ~24k, x sqrt(size / 250)), three significant digits like the slider, within 1,800..MaxPlacedStars.
	 */
	int32 RollPlacedStars(const FRandomStream& Random, const EGalaxyClass Class, const int32 GalaxySize)
	{
		double Median = 15000.0;
		switch (Class)
		{
		case EGalaxyClass::dE: case EGalaxyClass::dIrr: Median = 6000.0; break;
		case EGalaxyClass::Im: case EGalaxyClass::IBm: case EGalaxyClass::SpiralSm: case EGalaxyClass::BarredSBm:
			Median = 9000.0; break;
		case EGalaxyClass::Irr: case EGalaxyClass::I0: Median = 10000.0; break;
		case EGalaxyClass::cD: Median = 24000.0; break;
		default: break;
		}
		Median *= FMath::Sqrt(FMath::Max(GalaxySize, 1) / 250.0);
		// A bell from three draws (one per statement): about x0.64..x1.57 typically, x0.26..x3.9 at the extremes.
		const double A = Random.FRand();
		const double B = Random.FRand();
		const double C = Random.FRand();
		const double Raw = Median * FMath::Exp((A + B + C - 1.5) * 0.9);
		const double Step = FMath::Pow(10.0,
			FMath::Max(FMath::FloorToDouble(FMath::LogX(10.0, FMath::Max(Raw, 1.0))) - 2.0, 0.0));
		return FMath::Clamp(static_cast<int32>(FMath::RoundToDouble(Raw / Step) * Step),
			APSGalaxyMorphology::PreviewReferenceBudget, APSGalaxyMorphology::MaxPlacedStars);
	}

	/**
	 * Rio 03.10: the galaxy's POPULATION / COMPOSITION follow its type (ellipticals and lenticulars old, giant-rich and
	 * red; irregulars and peculiars young and blue; spirals mostly the historic mix); hand-made archetypes pick their
	 * own. Two draws (population, then composition), always.
	 */
	void RollGalaxyStarMix(const FRandomStream& Random, const EGalaxyType Type, const ESkyArchetype Archetype,
		EStarClusterPopulation& OutPopulation, EStarClusterComposition& OutComposition)
	{
		using EP = EStarClusterPopulation;
		using EC = EStarClusterComposition;
		switch (Archetype)
		{
		case ESkyArchetype::GlobularRichElliptical:
			OutPopulation = PickWeighted<EP>(Random, {{EP::Giants, 60.0f}, {EP::AllSequenses, 40.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::OrangeRed, 45.0f}, {EC::MostlyRed, 30.0f}, {EC::MostlyOrange, 25.0f}});
			return;
		case ESkyArchetype::StarburstAssociation:
		case ESkyArchetype::CollidingGalaxies:
			OutPopulation = PickWeighted<EP>(Random, {{EP::Protostars, 45.0f}, {EP::MainSequence, 35.0f}, {EP::AllSequenses, 20.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::BlueWhite, 45.0f}, {EC::MostlyBlue, 35.0f}, {EC::AllSpectral, 20.0f}});
			return;
		case ESkyArchetype::RingGalaxy:
			OutPopulation = PickWeighted<EP>(Random, {{EP::MainSequence, 50.0f}, {EP::AllSequenses, 50.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::BlueWhite, 50.0f}, {EC::AllSpectral, 50.0f}});
			return;
		case ESkyArchetype::DwarfIrregular:
		case ESkyArchetype::MagellanicCloud:
			OutPopulation = PickWeighted<EP>(Random, {{EP::MainSequence, 40.0f}, {EP::Dwarfs, 25.0f}, {EP::AllSequenses, 35.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::AllSpectral, 60.0f}, {EC::BlueWhite, 25.0f}, {EC::MostlyBlue, 15.0f}});
			return;
		default:
			break;
		}
		switch (Type)
		{
		case EGalaxyType::Elliptical:
		case EGalaxyType::Lenticular:
			OutPopulation = PickWeighted<EP>(Random, {{EP::AllSequenses, 50.0f}, {EP::Giants, 25.0f}, {EP::Dwarfs, 20.0f},
				{EP::MainSequence, 5.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::AllSpectral, 50.0f}, {EC::MostlyRed, 15.0f}, {EC::OrangeRed, 15.0f},
				{EC::MostlyOrange, 12.0f}, {EC::YellowOrange, 8.0f}});
			return;
		case EGalaxyType::Irregular:
			OutPopulation = PickWeighted<EP>(Random, {{EP::AllSequenses, 45.0f}, {EP::MainSequence, 20.0f},
				{EP::Protostars, 20.0f}, {EP::Giants, 10.0f}, {EP::Dwarfs, 5.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::AllSpectral, 50.0f}, {EC::MostlyBlue, 20.0f}, {EC::BlueWhite, 20.0f},
				{EC::MostlyWhite, 10.0f}});
			return;
		case EGalaxyType::Peculiar:
			OutPopulation = PickWeighted<EP>(Random, {{EP::AllSequenses, 50.0f}, {EP::Protostars, 20.0f}, {EP::Giants, 15.0f},
				{EP::MainSequence, 10.0f}, {EP::Dwarfs, 5.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::AllSpectral, 55.0f}, {EC::BlueWhite, 20.0f}, {EC::MostlyBlue, 15.0f},
				{EC::MostlyWhite, 10.0f}});
			return;
		default:
			// Spiral and barred spiral: mostly the accepted historic mix.
			OutPopulation = PickWeighted<EP>(Random, {{EP::AllSequenses, 60.0f}, {EP::MainSequence, 15.0f}, {EP::Giants, 15.0f},
				{EP::Dwarfs, 7.0f}, {EP::Protostars, 3.0f}});
			OutComposition = PickWeighted<EC>(Random, {{EC::AllSpectral, 70.0f}, {EC::MostlyYellow, 8.0f}, {EC::WhiteYellow, 8.0f},
				{EC::BlueWhite, 7.0f}, {EC::YellowOrange, 7.0f}});
			return;
		}
	}

	ESkyArchetype RollSkyArchetype(const FRandomStream& Random)
	{
		return PickWeighted<ESkyArchetype>(Random, {
			{ESkyArchetype::BarredGrandDesign, 18.0f}, {ESkyArchetype::DwarfIrregular, 16.0f},
			{ESkyArchetype::GlobularRichElliptical, 16.0f}, {ESkyArchetype::RingGalaxy, 14.0f},
			{ESkyArchetype::StarburstAssociation, 14.0f}, {ESkyArchetype::CollidingGalaxies, 12.0f},
			{ESkyArchetype::MagellanicCloud, 10.0f}});
	}

	/** A hand-made galaxy-and-cluster pairing; every value stays an ordinary, editable model setting. */
	void ApplySkyArchetype(const FRandomStream& Random, const ESkyArchetype Archetype, FSkyPick& Pick)
	{
		switch (Archetype)
		{
		case ESkyArchetype::BarredGrandDesign:
			Pick.GalaxyType = EGalaxyType::BarredSpiral;
			Pick.GalaxyClass = Random.FRand() < 0.55f ? EGalaxyClass::BarredSBb : EGalaxyClass::BarredSBbc;
			Pick.GalaxySize = Random.RandRange(340, 440);
			Pick.ClusterType = Random.FRand() < 0.6f ? EStarClusterType::RingArc : EStarClusterType::Nebula;
			Pick.ClusterSize = EStarClusterSize::Giant;
			break;
		case ESkyArchetype::RingGalaxy:
			Pick.GalaxyType = EGalaxyType::Peculiar;
			Pick.GalaxyClass = EGalaxyClass::PecRing;
			Pick.GalaxySize = Random.RandRange(260, 380);
			Pick.ClusterType = EStarClusterType::YoungAssociation;
			Pick.ClusterSize = EStarClusterSize::Large;
			Pick.Population = EStarClusterPopulation::MainSequence;
			Pick.Composition = Random.FRand() < 0.5f ? EStarClusterComposition::BlueWhite : EStarClusterComposition::MostlyBlue;
			break;
		case ESkyArchetype::DwarfIrregular:
			Pick.GalaxyType = EGalaxyType::Irregular;
			Pick.GalaxyClass = Random.FRand() < 0.7f ? EGalaxyClass::dIrr : EGalaxyClass::Im;
			Pick.GalaxySize = Random.RandRange(110, 170);
			Pick.ClusterType = PickWeighted<EStarClusterType>(Random, {
				{EStarClusterType::MovingGroup, 40.0f}, {EStarClusterType::OpenCluster, 30.0f},
				{EStarClusterType::YoungAssociation, 30.0f}});
			Pick.ClusterSize = Random.FRand() < 0.5f ? EStarClusterSize::Small : EStarClusterSize::Medium;
			Pick.Composition = Random.FRand() < 0.6f ? EStarClusterComposition::AllSpectral : EStarClusterComposition::MostlyBlue;
			break;
		case ESkyArchetype::GlobularRichElliptical:
			Pick.GalaxyType = EGalaxyType::Elliptical;
			Pick.GalaxyClass = PickWeighted<EGalaxyClass>(Random, {
				{EGalaxyClass::cD, 50.0f}, {EGalaxyClass::E1, 25.0f}, {EGalaxyClass::E2, 25.0f}});
			Pick.GalaxySize = Pick.GalaxyClass == EGalaxyClass::cD ? Random.RandRange(380, 520) : Random.RandRange(300, 420);
			Pick.ClusterType = EStarClusterType::GlobularCluster;
			Pick.ClusterSize = EStarClusterSize::Giant;
			Pick.Population = Random.FRand() < 0.6f ? EStarClusterPopulation::Giants : EStarClusterPopulation::AllSequenses;
			Pick.Composition = PickWeighted<EStarClusterComposition>(Random, {
				{EStarClusterComposition::MostlyOrange, 40.0f}, {EStarClusterComposition::OrangeRed, 35.0f},
				{EStarClusterComposition::MostlyRed, 25.0f}});
			break;
		case ESkyArchetype::StarburstAssociation:
			Pick.GalaxyType = EGalaxyType::Irregular;
			Pick.GalaxyClass = EGalaxyClass::I0;
			Pick.GalaxySize = Random.RandRange(160, 240);
			Pick.ClusterType = Random.FRand() < 0.6f ? EStarClusterType::YoungAssociation : EStarClusterType::EmbeddedCluster;
			Pick.ClusterSize = EStarClusterSize::Large;
			Pick.Population = Random.FRand() < 0.5f ? EStarClusterPopulation::Protostars : EStarClusterPopulation::MainSequence;
			Pick.Composition = PickWeighted<EStarClusterComposition>(Random, {
				{EStarClusterComposition::MostlyBlue, 50.0f}, {EStarClusterComposition::BlueWhite, 40.0f},
				{EStarClusterComposition::OnlyBlue, 10.0f}});
			break;
		case ESkyArchetype::CollidingGalaxies:
			Pick.GalaxyType = EGalaxyType::Peculiar;
			Pick.GalaxyClass = Random.FRand() < 0.5f ? EGalaxyClass::PecInteracting : EGalaxyClass::PecTidalTails;
			Pick.GalaxySize = Random.RandRange(300, 440);
			Pick.ClusterType = EStarClusterType::SuperStarCluster;
			Pick.ClusterSize = EStarClusterSize::Giant;
			Pick.Population = EStarClusterPopulation::MainSequence;
			Pick.Composition = Random.FRand() < 0.6f ? EStarClusterComposition::BlueWhite : EStarClusterComposition::MostlyBlue;
			break;
		case ESkyArchetype::MagellanicCloud:
			Pick.GalaxyType = EGalaxyType::Irregular;
			Pick.GalaxyClass = Random.FRand() < 0.6f ? EGalaxyClass::Im : EGalaxyClass::IBm;
			Pick.GalaxySize = Random.RandRange(140, 200);
			Pick.ClusterType = Random.FRand() < 0.6f ? EStarClusterType::DoubleCluster : EStarClusterType::YoungAssociation;
			Pick.ClusterSize = Random.FRand() < 0.5f ? EStarClusterSize::Medium : EStarClusterSize::Large;
			Pick.Composition = Random.FRand() < 0.5f ? EStarClusterComposition::AllSpectral : EStarClusterComposition::BlueWhite;
			break;
		default:
			break;
		}
	}

	/**
	 * Rolls the galaxy and the home cluster into the model. A stream of their own keeps every home-system draw of the
	 * same roll seed exactly where it was. Returns the log fragment.
	 */
	FString RollSky(UGeneratedWorld& World, const int32 RollSeed)
	{
		const FRandomStream Random(static_cast<int32>(HashCombineFast(GetTypeHash(RollSeed), 0x534b5952u) & 0x7fffffffu) | 1);
		FSkyPick Pick;
		// About one press in eight pairs the galaxy and its cluster into a hand-made archetype.
		const ESkyArchetype Archetype = Random.FRand() < 0.12f ? RollSkyArchetype(Random) : ESkyArchetype::None;
		if (Archetype != ESkyArchetype::None)
		{
			ApplySkyArchetype(Random, Archetype, Pick);
		}
		else
		{
			Pick.GalaxyType = PickWeighted<EGalaxyType>(Random, {
				{EGalaxyType::Spiral, 30.0f}, {EGalaxyType::BarredSpiral, 25.0f}, {EGalaxyType::Elliptical, 15.0f},
				{EGalaxyType::Irregular, 12.0f}, {EGalaxyType::Lenticular, 10.0f}, {EGalaxyType::Peculiar, 8.0f}});
			Pick.GalaxyClass = RollGalaxyClass(Random, Pick.GalaxyType);
			Pick.GalaxySize = RollGalaxySize(Random, Pick.GalaxyClass);
			Pick.ClusterType = RollClusterType(Random, Pick.GalaxyType);
			Pick.ClusterSize = RollClusterSize(Random, Pick.ClusterType);
			Pick.Population = RollClusterPopulation(Random, Pick.ClusterType);
			Pick.Composition = RollClusterComposition(Random, Pick.ClusterType);
		}
		// STAR DENSITY only rescales the catalogue radius; keep it near the default 10.
		Pick.GalaxyDensity = FMath::RoundToDouble(RollBetween(Random, 7.0, 14.0) * 10.0) / 10.0;
		// The class always belongs to its type: the menu's CLASS list never shows a legacy value after a roll.
		Pick.GalaxyClass = APSGalaxyMorphology::CoerceSubclass(Pick.GalaxyType, Pick.GalaxyClass);
		const int32 PlacedStars = RollPlacedStars(Random, Pick.GalaxyClass, Pick.GalaxySize);
		EStarClusterPopulation GalaxyPopulation = EStarClusterPopulation::AllSequenses;
		EStarClusterComposition GalaxyComposition = EStarClusterComposition::AllSpectral;
		RollGalaxyStarMix(Random, Pick.GalaxyType, Archetype, GalaxyPopulation, GalaxyComposition);

		World.GalaxyType = Pick.GalaxyType;
		World.GalaxyClass = Pick.GalaxyClass;
		World.GalaxySize = FMath::Clamp(Pick.GalaxySize, APSGalaxyMorphology::MinGalaxySize, 100000);
		World.GalaxyStarDensity = Pick.GalaxyDensity;
		World.GalaxyPlacedStarCount = PlacedStars;
		World.GalaxyStarPopulation = GalaxyPopulation;
		World.GalaxyStarComposition = GalaxyComposition;
		World.StarClusterType = Pick.ClusterType;
		World.StarClusterSize = Pick.ClusterSize;
		World.StarClusterPopulation = Pick.Population;
		World.StarClusterComposition = Pick.Composition;
		// Names follow the new world seed (APSBodyNames); REGENERATE has already cleared any typed name.
		return FString::Printf(TEXT("galaxy=%s/%s size=%d density=%.1f stars=%d gpop=%s gcomp=%s \"%s\" cluster=%s/%s pop=%s comp=%s \"%s\" sky=%s"),
			*EnumName(StaticEnum<EGalaxyType>(), static_cast<int64>(World.GalaxyType)),
			*EnumName(StaticEnum<EGalaxyClass>(), static_cast<int64>(World.GalaxyClass)),
			World.GalaxySize, World.GalaxyStarDensity, World.GalaxyPlacedStarCount,
			*EnumName(StaticEnum<EStarClusterPopulation>(), static_cast<int64>(World.GalaxyStarPopulation)),
			*EnumName(StaticEnum<EStarClusterComposition>(), static_cast<int64>(World.GalaxyStarComposition)),
			*World.GetGalaxyName(),
			*EnumName(StaticEnum<EStarClusterType>(), static_cast<int64>(World.StarClusterType)),
			*EnumName(StaticEnum<EStarClusterSize>(), static_cast<int64>(World.StarClusterSize)),
			*EnumName(StaticEnum<EStarClusterPopulation>(), static_cast<int64>(World.StarClusterPopulation)),
			*EnumName(StaticEnum<EStarClusterComposition>(), static_cast<int64>(World.StarClusterComposition)),
			*World.GetClusterName(), SkyArchetypeName(Archetype));
	}
}

int32 APSWorldRoll::MakeFreshSeed()
{
	// The preview reseeds FMath::Rand with its world seed, so the global stream alone would repeat across presses.
	static uint32 PressCounter = 0;
	uint32 Hash = HashCombineFast(GetTypeHash(FDateTime::UtcNow().GetTicks()), GetTypeHash(FPlatformTime::Cycles64()));
	Hash = HashCombineFast(Hash, GetTypeHash(++PressCounter));
	Hash = HashCombineFast(Hash, static_cast<uint32>(FMath::Rand()));
	return static_cast<int32>(Hash & 0x7fffffffu) | 1;
}

bool APSWorldRoll::IsInteractiveSession()
{
	// The night bench and the diagnostics rely on the default one-planet world plus -Planets/-Moons.
	const TCHAR* CommandLine = FCommandLine::Get();
	return !FApp::IsUnattended() && !IsRunningCommandlet() && !GIsAutomationTesting
		&& !FParse::Param(CommandLine, TEXT("APSNoWorldRoll"))
		&& FCString::Stristr(CommandLine, TEXT("ExecCmds")) == nullptr
		&& FCString::Stristr(CommandLine, TEXT("APSBench")) == nullptr
		&& FCString::Stristr(CommandLine, TEXT("APSDiagnostic")) == nullptr
		&& FCString::Stristr(CommandLine, TEXT("APSProbe")) == nullptr;
}

APSWorldRoll::FResult APSWorldRoll::Apply(UGeneratedWorld& World, const int32 RollSeed, const EScope Scope)
{
	using namespace APSWorldRollPrivate;
	const FRandomStream Random(RollSeed);
	FResult Result;
	Result.RollSeed = RollSeed;
	// A new canonical world: galaxy, cluster, home record and every body identity follow this seed.
	Result.WorldSeed = 1 + static_cast<int32>(HashCombineFast(GetTypeHash(RollSeed), 0x574f524cu)
		% static_cast<uint32>(MAX_int32 - 1));
	World.GenerationSeed = Result.WorldSeed;
	// The recipe is explicit: the legacy RANDOM switches would replace parts of it with the generator's own draws.
	World.bRandomHomeSystem = false;
	World.bRandomHomeSystemType = false;
	World.bRandomHomeStar = false;
	World.bRandomStartPlanetNumber = false;
	World.HomeStarRadiusOverrideSolar = 0.0;

	if (Scope == EScope::PlanetOnly)
	{
		const EPlanetaryZoneType Zone = PickWeighted<EPlanetaryZoneType>(Random, {
			{EPlanetaryZoneType::HabitableZone, 50.0f}, {EPlanetaryZoneType::WarmZone, 15.0f},
			{EPlanetaryZoneType::ColdZone, 20.0f}, {EPlanetaryZoneType::IceZone, 10.0f},
			{EPlanetaryZoneType::HotZone, 5.0f}});
		WriteStartWorld(World, Random, RollStartWorldType(Random, Zone), Zone, RollStartWorldMoons(Random));
		Result.Archetype = TEXT("planet");
		Result.Summary = FString::Printf(TEXT("seed=%d world=%d planet=%s %.0fkm (%s) moons=%d archetype=%s"),
			RollSeed, Result.WorldSeed, *EnumName(StaticEnum<EPlanetType>(), static_cast<int64>(World.PlanetType)),
			World.PlanetRadius, *EnumName(StaticEnum<EPlanetaryZoneType>(), static_cast<int64>(Zone)),
			World.MoonsAmount, *Result.Archetype);
		return Result;
	}

	// About one press in ten rolls a hand-made archetype instead of the standard distributions.
	const EArchetype Archetype = Random.FRand() < 0.10f ? RollArchetype(Random) : EArchetype::None;
	FStarPick Primary = RollPrimary(Random);
	int32 StarCount = RollStarCount(Random);
	int32 TotalPlanets = RollPlanetTotal(Random, Primary);
	EOrbitDistributionType Distribution = RollDistribution(Random, Primary);
	double Inclination = RollInclination(Random);
	TOptional<EPlanetType> StartWorldType;
	TOptional<int32> StartWorldMoons;
	switch (Archetype)
	{
	case EArchetype::HotJupiter:
		Primary = {EStellarType::MainSequence, PickWeighted<ESpectralClass>(Random, {
			{ESpectralClass::G, 45.0f}, {ESpectralClass::K, 35.0f}, {ESpectralClass::F, 20.0f}})};
		StarCount = 1;
		TotalPlanets = Random.RandRange(4, 8);
		// A tight family keeps a temperate orbit free next to the giant.
		Distribution = EOrbitDistributionType::Dense;
		break;
	case EArchetype::RedDwarfSwarm:
		Primary = {EStellarType::MainSequence, ESpectralClass::M};
		StarCount = 1;
		TotalPlanets = Random.RandRange(7, 11);
		Distribution = EOrbitDistributionType::Dense;
		Inclination = RollBetween(Random, 0.0, 3.0);
		break;
	case EArchetype::GiantSun:
		Primary.Type = Random.FRand() < 0.6f ? EStellarType::Giant : EStellarType::SubGiant;
		Primary.Class = RollClassFor(Random, Primary.Type);
		StarCount = 1;
		TotalPlanets = Random.RandRange(2, 4);
		Distribution = Random.FRand() < 0.5f ? EOrbitDistributionType::InnerOuter : EOrbitDistributionType::Uniform;
		break;
	case EArchetype::MoonRichGiant:
		Primary = {EStellarType::MainSequence, PickWeighted<ESpectralClass>(Random, {
			{ESpectralClass::G, 50.0f}, {ESpectralClass::K, 35.0f}, {ESpectralClass::F, 15.0f}})};
		StarCount = 1;
		TotalPlanets = Random.RandRange(4, 8);
		break;
	case EArchetype::WaterWorld:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.55f ? ESpectralClass::G : ESpectralClass::K};
		StartWorldType = PickWeighted<EPlanetType>(Random, {
			{EPlanetType::Water, 40.0f}, {EPlanetType::Ocean, 35.0f}, {EPlanetType::Archipelago, 25.0f}});
		StartWorldMoons = Random.RandRange(1, 2);
		break;
	case EArchetype::BinarySuns:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.6f ? ESpectralClass::G : ESpectralClass::K};
		StarCount = 2;
		TotalPlanets = Random.RandRange(3, 7);
		StartWorldType = PickWeighted<EPlanetType>(Random, {
			{EPlanetType::Desert, 45.0f}, {EPlanetType::Sand, 30.0f}, {EPlanetType::Savanna, 25.0f}});
		break;
	case EArchetype::TripleSuns:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.5f ? ESpectralClass::G : ESpectralClass::K};
		StarCount = 3;
		TotalPlanets = Random.RandRange(5, 9);
		break;
	case EArchetype::CrowdedSystem:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.5f ? ESpectralClass::G : ESpectralClass::K};
		StarCount = 1;
		TotalPlanets = Random.RandRange(13, 15);
		Distribution = Random.FRand() < 0.5f ? EOrbitDistributionType::Uniform : EOrbitDistributionType::Gaussian;
		break;
	case EArchetype::WhiteDwarfRemnant:
		Primary = {EStellarType::WhiteDwarf, ESpectralClass::A};
		StarCount = 1;
		TotalPlanets = Random.RandRange(2, 4);
		Distribution = EOrbitDistributionType::Dense;
		break;
	case EArchetype::BlueSun:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.7f ? ESpectralClass::A : ESpectralClass::B};
		TotalPlanets = Random.RandRange(3, 6);
		Distribution = EOrbitDistributionType::Uniform;
		break;
	case EArchetype::LonelyWorld:
		Primary = {EStellarType::MainSequence, Random.FRand() < 0.6f ? ESpectralClass::G : ESpectralClass::K};
		StarCount = 1;
		TotalPlanets = 1;
		StartWorldMoons = Random.RandRange(1, 3);
		break;
	case EArchetype::TiltedSystem:
		Inclination = RollBetween(Random, 25.0, 45.0);
		TotalPlanets = FMath::Max(TotalPlanets, 4);
		break;
	default:
		break;
	}
	StarCount = FMath::Clamp(StarCount, 1, 6);
	// Every route starts on a world of the home star: at least one planet, within the system editor's range.
	TotalPlanets = FMath::Clamp(TotalPlanets, 1, 15);

	// The primary is pinned as the panel's own star edit: the forecast below then sees the very star the preview builds.
	UStarGenerator* Stars = NewObject<UStarGenerator>(GetTransientPackage());
	const FStarModel PrimaryModel = MakeStar(*Stars, Primary, Result.WorldSeed, TEXT("SYS0/S0"));

	// The home star's share of the total, exactly as FAPSPreviewSystemEditOverride::ApplyToFamily splits it.
	const int32 HomeFamilyCount = TotalPlanets / StarCount + (TotalPlanets % StarCount > 0 ? 1 : 0);
	const FFamilyForecast Forecast = ForecastFamily(PrimaryModel, HomeFamilyCount, Distribution,
		static_cast<int32>(HashCombineFast(GetTypeHash(RollSeed), 0x4c41594fu) & 0x7fffffffu));

	// Archetype planets other than the start world, by orbit slot.
	TSet<int32> ReservedSlots;
	TArray<TPair<int32, EPlanetType>> ArchetypeBodies;
	if (Archetype == EArchetype::HotJupiter && Forecast.OrbitsAu.Num() >= 2)
	{
		ReservedSlots.Add(0);
		ArchetypeBodies.Emplace(0, EPlanetType::HotGiant);
	}
	if (Archetype == EArchetype::MoonRichGiant && Forecast.OrbitsAu.Num() >= 2)
	{
		const int32 Outer = Forecast.OrbitsAu.Num() - 1;
		ReservedSlots.Add(Outer);
		ArchetypeBodies.Emplace(Outer, EPlanetType::GasGiant);
	}
	// Rio 03.10 ("adequate, yet sometimes really interesting"): the compact layouts put the habitable zone of most
	// red and orange suns inside their first orbit, so left alone six starts in ten were frozen. A little under half
	// of the rolls (and every forced ocean or desert world) move the start world's orbit into the zone; the rest stay
	// where the layout put them: cold, icy or scorched.
	const bool bWantsHabitableOrbit = StartWorldType.IsSet() || Random.FRand() < 0.45f;
	const int32 StartSlot = FMath::Clamp(ChooseStartSlot(Random, Forecast, ReservedSlots, !bWantsHabitableOrbit), 0,
		FMath::Max(HomeFamilyCount - 1, 0));
	EPlanetaryZoneType StartZone = Forecast.OrbitsAu.IsValidIndex(StartSlot)
		? Forecast.ZoneOf(Forecast.OrbitsAu[StartSlot]) : EPlanetaryZoneType::HabitableZone;
	double HabitableOrbitAu = 0.0;
	if (bWantsHabitableOrbit && StartZone != EPlanetaryZoneType::HabitableZone)
	{
		// The slot nearest the zone moves into it, so it never crosses a neighbouring orbit. Suns whose zone lies
		// beyond the editable orbit range (blue and bright giants) keep a hot start world.
		const double TargetAu = FMath::Sqrt(Forecast.HabitableInnerAu * Forecast.HabitableOuterAu)
			* RollBetween(Random, 0.92, 1.08);
		if (TargetAu >= UGeneratedWorld::MinimumPlanetOrbitAu(PrimaryModel.Radius, 2.0 * RollEarthRadiusKm) * 1.5
			&& TargetAu <= UGeneratedWorld::MaximumPlanetOrbitAu(PrimaryModel.Radius))
		{
			HabitableOrbitAu = TargetAu;
			StartZone = EPlanetaryZoneType::HabitableZone;
		}
	}
	WriteStartWorld(World, Random, StartWorldType.IsSet() ? StartWorldType.GetValue() : RollStartWorldType(Random, StartZone),
		StartZone, StartWorldMoons.IsSet() ? StartWorldMoons.GetValue() : RollStartWorldMoons(Random));
	if (HabitableOrbitAu > 0.0)
	{
		// An ordinary orbit edit of the panel: RESET on the PLANET page returns the world to its generated slot.
		FAPSPreviewPlanetOrbitEdit Orbit;
		Orbit.bOverrideDistance = true;
		Orbit.DistanceAu = HabitableOrbitAu;
		World.SetPlanetOrbitEdit(FString::Printf(TEXT("SYS0/S0/P%d"), StartSlot), Orbit);
	}

	for (const TPair<int32, EPlanetType>& Body : ArchetypeBodies)
	{
		double MinRadius = 1.0;
		double MaxRadius = 1.0;
		RadiusRangeEarth(Body.Value, MinRadius, MaxRadius);
		const double RadiusEarth = RollBetween(Random, MinRadius, MaxRadius);
		const bool bHotGiant = Body.Value == EPlanetType::HotGiant;
		WriteBodyEdit(World, Body.Key, Body.Value, RadiusEarth, bHotGiant ? Random.RandRange(0, 1) : Random.RandRange(8, 10),
			bHotGiant ? EPlanetaryZoneType::HotZone : Forecast.ZoneOf(Forecast.OrbitsAu[Body.Key]));
		if (bHotGiant)
		{
			// A true hot Jupiter hugs its sun: a few hundredths of an AU, scaled with the star's light.
			FAPSPreviewPlanetOrbitEdit Orbit;
			Orbit.bOverrideDistance = true;
			const double Luminosity = FMath::IsFinite(PrimaryModel.Luminosity) && PrimaryModel.Luminosity > 0.0
				? PrimaryModel.Luminosity : 1.0;
			Orbit.DistanceAu = FMath::Max(RollBetween(Random, 0.035, 0.08) * FMath::Sqrt(Luminosity),
				UGeneratedWorld::MinimumPlanetOrbitAu(PrimaryModel.Radius, RadiusEarth * RollEarthRadiusKm) * 1.6);
			World.SetPlanetOrbitEdit(FString::Printf(TEXT("SYS0/S0/P%d"), Body.Key), Orbit);
		}
	}

	FAPSPreviewStarEditOverride PrimaryEdit;
	PrimaryEdit.AutomaticModel = PrimaryEdit.Model = PrimaryModel;
	World.SetPreviewStarEditOverride(TEXT("SYS0/S0"), PrimaryEdit);
	// Companion suns are star edits too, so each keeps its own class (the home recipe would copy the primary).
	FString CompanionText;
	for (int32 StarIndex = 1; StarIndex < StarCount; ++StarIndex)
	{
		FStarPick Companion = RollCompanion(Random);
		if (Archetype == EArchetype::BinarySuns || Archetype == EArchetype::TripleSuns)
		{
			Companion = {EStellarType::MainSequence, StarIndex == 1 ? ESpectralClass::K : ESpectralClass::M};
		}
		const FString Address = FString::Printf(TEXT("SYS0/S%d"), StarIndex);
		FAPSPreviewStarEditOverride Edit;
		Edit.AutomaticModel = Edit.Model = MakeStar(*Stars, Companion, Result.WorldSeed, Address);
		World.SetPreviewStarEditOverride(Address, Edit);
		CompanionText += TEXT("+") + EnumName(StaticEnum<ESpectralClass>(), static_cast<int64>(Edit.Model.SpectralClass));
	}

	// The home recipe as the SYSTEM and STAR pages show and edit it.
	const EStarType SystemType = StarTypeFor(StarCount);
	World.StarType = SystemType;
	World.StellarType = PrimaryModel.StellarType;
	World.SpectralClass = PrimaryModel.SpectralClass;
	World.PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
	World.OrbitDistributionType = Distribution;
	World.PlanetsAmount = FMath::Clamp(HomeFamilyCount, 1, 20);
	World.StartPlanetIndex = StartSlot + 1;
	FAPSPreviewSystemEditOverride SystemEdit;
	SystemEdit.StarCount = StarCount;
	SystemEdit.StarType = SystemType;
	SystemEdit.TotalPlanets = TotalPlanets;
	SystemEdit.bOverrideOrbitDistribution = true;
	SystemEdit.OrbitDistribution = Distribution;
	SystemEdit.bOverrideOrbitInclination = true;
	SystemEdit.MaxOrbitInclinationDegrees = FMath::RoundToDouble(Inclination * 10.0) / 10.0;
	World.SetPreviewSystemEditOverride(TEXT("SYS0"), SystemEdit);
	// Rio 03.10: the galaxy and the home cluster are rolled too (own stream: the draws above are unchanged).
	const FString SkySummary = RollSky(World, RollSeed);

	Result.Archetype = ArchetypeName(Archetype);
	Result.Summary = FString::Printf(
		TEXT("seed=%d world=%d star=%s %s x%d%s planets=%d home=P%d/%d %s %.0fkm (%s%s) moons=%d dist=%s incl=%.1f archetype=%s %s"),
		RollSeed, Result.WorldSeed, *EnumName(StaticEnum<EStellarType>(), static_cast<int64>(PrimaryModel.StellarType)),
		*EnumName(StaticEnum<ESpectralClass>(), static_cast<int64>(PrimaryModel.SpectralClass)), StarCount, *CompanionText,
		TotalPlanets, StartSlot + 1, HomeFamilyCount, *EnumName(StaticEnum<EPlanetType>(), static_cast<int64>(World.PlanetType)),
		World.PlanetRadius, *EnumName(StaticEnum<EPlanetaryZoneType>(), static_cast<int64>(StartZone)),
		HabitableOrbitAu > 0.0 ? *FString::Printf(TEXT(", orbit moved to %.2f AU"), HabitableOrbitAu) : TEXT(""),
		World.MoonsAmount,
		*EnumName(StaticEnum<EOrbitDistributionType>(), static_cast<int64>(Distribution)),
		SystemEdit.MaxOrbitInclinationDegrees, *Result.Archetype, *SkySummary);
	return Result;
}
