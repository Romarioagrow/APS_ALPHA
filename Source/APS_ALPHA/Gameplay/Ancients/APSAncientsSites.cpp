#include "APSAncientsSites.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"
#include "Misc/Crc.h"

#define LOCTEXT_NAMESPACE "APSAncients"

namespace APSAncientsSitesLocal
{
	using namespace APSAncients;

	/** Nearby systems taken into account, nearest first, and the chance of a site by rank (per mille). */
	constexpr int32 NearbySystems = 16;
	/** About one other world of the start system in five hides a site (per mille). */
	constexpr uint32 LostWorksChance = 220u;

	/** A seed for FRandomStream from a hash. */
	int32 StreamSeed(const uint32 Hash)
	{
		return static_cast<int32>(Hash & 0x7fffffffu);
	}

	/** A world wholly or mostly under water is a poor place for a circle one has to walk to. */
	bool IsWet(const EPlanetType Type)
	{
		return Type == EPlanetType::Ocean || Type == EPlanetType::Water;
	}

	/** Upper-case letters and digits, the rest underscores: actor names and mission templates take it. */
	FString Sanitize(const FString& Text)
	{
		FString Out;
		Out.Reserve(Text.Len());
		for (const TCHAR Char : Text)
		{
			Out.AppendChar(FChar::IsAlnum(Char) ? FChar::ToUpper(Char) : TEXT('_'));
		}
		return Out;
	}

	/** The scale of a site's large variant: 0.40-0.55 of the monumental shape, from its seed. */
	float LargeScale(const uint32 Seed)
	{
		return 0.40f + 0.15f * static_cast<float>((Seed >> 9) % 100u) / 100.0f;
	}

	void MakeNearby(const FAPSStarSystemInfo& Info, const int32 Index, const int32 Rank, const FString& Cluster,
		TArray<FSiteSpec>& OutSpecs)
	{
		FSiteSpec& Spec = OutSpecs.AddDefaulted_GetRef();
		Spec.Id = TEXT("N_") + Info.Id.ToString(EGuidFormats::Digits);
		Spec.bHomeSystem = false;
		Spec.Chain = EChain::Road;
		Spec.SystemId = Info.Id;
		Spec.SystemIndex = Index;
		Spec.Rank = Rank;
		Spec.Seed = Hash(Cluster, Spec.Id, TEXT("SITE"));
		Spec.Pick = Hash(Cluster, Spec.Id, TEXT("WORLD"));
		// Monolith 20, ring 18, spires 18, cube 18, circle 10, derelict 16.
		const uint32 Roll = (Spec.Seed >> 5) % 100u;
		Spec.Kind = Roll < 20u ? EKind::Monolith : Roll < 38u ? EKind::BrokenRing : Roll < 56u ? EKind::SpireField
			: Roll < 74u ? EKind::SunkenCube : Roll < 84u ? EKind::StoneCircle : EKind::Derelict;
		const bool bMonumental = (Spec.Seed >> 13) % 100u < 60u;
		switch (Spec.Kind)
		{
		case EKind::StoneCircle:
			Spec.Size = ESize::Small;
			Spec.Scale = 1.0f + 0.4f * static_cast<float>((Spec.Seed >> 9) % 100u) / 100.0f;
			break;
		case EKind::SunkenCube:
			Spec.Size = ESize::Large;
			Spec.Scale = 0.7f + 0.3f * static_cast<float>((Spec.Seed >> 9) % 100u) / 100.0f;
			break;
		default:
			Spec.Size = bMonumental ? ESize::Monumental : ESize::Large;
			Spec.Scale = bMonumental ? 1.0f : LargeScale(Spec.Seed);
			break;
		}
	}
}

FText APSAncients::KindName(const EKind Kind)
{
	switch (Kind)
	{
	case EKind::Monolith: return LOCTEXT("KindMonolith", "THE NEEDLE");
	case EKind::BrokenRing: return LOCTEXT("KindRing", "THE BROKEN RING");
	case EKind::SpireField: return LOCTEXT("KindSpires", "THE SPIRE FOREST");
	case EKind::SunkenCube: return LOCTEXT("KindCube", "THE SUNKEN CUBE");
	case EKind::StoneCircle: return LOCTEXT("KindCircle", "THE STONE CIRCLE");
	case EKind::Derelict: return LOCTEXT("KindDerelict", "THE QUIET HULL");
	default: return LOCTEXT("KindUnknown", "UNKNOWN STRUCTURE");
	}
}

FText APSAncients::KindDescription(const EKind Kind, const FMetrics& Metrics)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Size"), LengthText(Metrics.SizeCm));
	Args.Add(TEXT("Count"), FText::AsNumber(Metrics.PartCount));
	switch (Kind)
	{
	case EKind::Monolith:
		return FText::Format(LOCTEXT("DescribeMonolith",
			"a single black obelisk {Size} tall, leaning a few degrees as if it listens, inside a ring of standing stones."), Args);
	case EKind::BrokenRing:
		return FText::Format(LOCTEXT("DescribeRing",
			"a ring {Size} across standing on its edge, half sunk in the ground, an arc of it fallen beside it."), Args);
	case EKind::SpireField:
		return FText::Format(LOCTEXT("DescribeSpires",
			"{Count} stepped towers around one spire {Size} tall, set out on a grid older than the hills around them."), Args);
	case EKind::SunkenCube:
		return FText::Format(LOCTEXT("DescribeCube",
			"the corner of a black cube {Size} on a side breaking the surface; the rest of it is underground."), Args);
	case EKind::StoneCircle:
		return FText::Format(LOCTEXT("DescribeCircle",
			"a ring of {Count} standing stones {Size} across around a plinth that is still warm."), Args);
	case EKind::Derelict:
		return FText::Format(LOCTEXT("DescribeDerelict",
			"a hull {Size} long, broken behind its middle, cold and dark, its rings half gone."), Args);
	default:
		return FText::GetEmpty();
	}
}

FText APSAncients::KindStory(const EKind Kind)
{
	switch (Kind)
	{
	case EKind::Monolith:
		return LOCTEXT("StoryMonolith", "The stone is warm and hums under a hand. It was a tether anchor: the Builders climbed to orbit along it.");
	case EKind::BrokenRing:
		return LOCTEXT("StoryRing", "The ring was the frame of a gate; its fallen arc still points at one star.");
	case EKind::SpireField:
		return LOCTEXT("StorySpires", "The spires are antennas tuned to one star; when the wind crosses them they sing the same seven notes.");
	case EKind::SunkenCube:
		return LOCTEXT("StoryCube", "The cube is hollow and dry inside, with shelves of glass that still hold light.");
	case EKind::StoneCircle:
		return LOCTEXT("StoryCircle", "A disc of glass in the plinth lights under a hand: a map of this system with one world more than we can find.");
	case EKind::Derelict:
		return LOCTEXT("StoryDerelict", "Inside the hull the air is stale but breathable, as if it had waited for a crew.");
	default:
		return FText::GetEmpty();
	}
}

FText APSAncients::SizeName(const ESize Size)
{
	switch (Size)
	{
	case ESize::Small: return LOCTEXT("SizeSmall", "SMALL");
	case ESize::Large: return LOCTEXT("SizeLarge", "LARGE");
	default: return LOCTEXT("SizeMonumental", "MONUMENTAL");
	}
}

FText APSAncients::ChainName(const EChain Chain)
{
	switch (Chain)
	{
	case EChain::Echoes: return LOCTEXT("ChainEchoes", "ECHOES OF THE BUILDERS");
	case EChain::QuietHull: return LOCTEXT("ChainHull", "THE QUIET HULL");
	case EChain::Circle: return LOCTEXT("ChainCircle", "THE CIRCLE");
	case EChain::LostWorks: return LOCTEXT("ChainLost", "LOST WORKS");
	case EChain::Road: return LOCTEXT("ChainRoad", "THE BUILDERS' ROAD");
	default: return FText::GetEmpty();
	}
}

const TCHAR* APSAncients::KindLabel(const EKind Kind)
{
	switch (Kind)
	{
	case EKind::Monolith: return TEXT("MONOLITH");
	case EKind::BrokenRing: return TEXT("BROKEN_RING");
	case EKind::SpireField: return TEXT("SPIRE_FIELD");
	case EKind::SunkenCube: return TEXT("SUNKEN_CUBE");
	case EKind::StoneCircle: return TEXT("STONE_CIRCLE");
	case EKind::Derelict: return TEXT("DERELICT");
	default: return TEXT("UNKNOWN");
	}
}

FText APSAncients::LengthText(const double Cm)
{
	if (Cm >= 100000.0)
	{
		FNumberFormattingOptions One;
		One.SetMinimumFractionalDigits(1);
		One.SetMaximumFractionalDigits(1);
		return FText::Format(LOCTEXT("LengthKm", "{0} km"), FText::AsNumber(Cm / 100000.0, &One));
	}
	return FText::Format(LOCTEXT("LengthM", "{0} m"), FText::AsNumber(FMath::RoundToInt(Cm / 100.0)));
}

uint32 APSAncients::Hash(const FString& A, const FString& B, const FString& C)
{
	const FString Joined = A + TEXT("|") + B + TEXT("|") + C;
	return FCrc::StrCrc32(*Joined);
}

FText APSAncients::WhereText(const FVector& LocalUp)
{
	const double Latitude = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(LocalUp.Z, -1.0, 1.0)));
	const double Longitude = FMath::RadiansToDegrees(FMath::Atan2(LocalUp.Y, LocalUp.X));
	return FText::FromString(FString::Printf(TEXT("%.1f %s, %.1f %s"), FMath::Abs(Latitude), Latitude >= 0.0 ? TEXT("N") : TEXT("S"),
		FMath::Abs(Longitude), Longitude >= 0.0 ? TEXT("E") : TEXT("W")));
}

int32 APSAncientsSites::WorldSeedOf(const UWorld* World, const AAstroGenerator* Generator)
{
	// The committed model is the one a new game and a load share (a load replays the saved model into it).
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (Gameplay && IsValid(Gameplay->NewGeneratedWorld))
	{
		return Gameplay->NewGeneratedWorld->GenerationSeed;
	}
	const UGeneratedWorld* Model = Generator ? Generator->GetGeneratedWorldModel() : nullptr;
	return IsValid(Model) ? Model->GenerationSeed : 0;
}

void APSAncientsSites::BuildHomeSpecs(const int32 WorldSeed, APlanet* HomePlanet, TArray<APSAncients::FSiteSpec>& OutSpecs,
	TMap<FString, TWeakObjectPtr<APlanetaryBody>>& OutBodies)
{
	using namespace APSAncients;
	using namespace APSAncientsSitesLocal;
	if (!IsValid(HomePlanet))
	{
		return;
	}
	const FString Seed = FString::FromInt(WorldSeed);
	const FString HomeKey = KeyOf(HomePlanet);

	// The monument: one of the three monumental shapes on the home planet, seen from orbit.
	{
		FSiteSpec& Spec = OutSpecs.AddDefaulted_GetRef();
		Spec.Id = TEXT("H_MONUMENT");
		Spec.Chain = EChain::Echoes;
		Spec.Size = ESize::Monumental;
		Spec.BodyKey = HomeKey;
		Spec.Seed = Hash(Seed, Spec.Id, HomeKey);
		static constexpr EKind Monuments[] = {EKind::Monolith, EKind::BrokenRing, EKind::SpireField};
		Spec.Kind = Monuments[(Spec.Seed >> 4) % UE_ARRAY_COUNT(Monuments)];
		OutBodies.Add(Spec.Id, HomePlanet);
	}

	// The small circle: on a home moon with a surface (a dry one when there is one), else on the home planet.
	{
		TArray<APlanetaryBody*> Moons;
		for (AMoon* Moon : HomePlanet->Moons)
		{
			if (IsValid(Moon) && !Moon->AstroName.IsNone() && UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Moon->PlanetType))
			{
				Moons.Add(Moon);
			}
		}
		// By name, not by spawn order: the same moon whatever order the generator listed them in.
		Moons.Sort([](const APlanetaryBody& A, const APlanetaryBody& B) { return A.AstroName.LexicalLess(B.AstroName); });
		const TArray<APlanetaryBody*> Dry = Moons.FilterByPredicate([](const APlanetaryBody* Body) { return !IsWet(Body->PlanetType); });
		const TArray<APlanetaryBody*>& Choice = Dry.IsEmpty() ? Moons : Dry;
		APlanetaryBody* Body = Choice.IsEmpty() ? static_cast<APlanetaryBody*>(HomePlanet)
			: Choice[static_cast<int32>(Hash(Seed, TEXT("H_CIRCLE/MOON"), HomeKey) % static_cast<uint32>(Choice.Num()))];
		FSiteSpec& Spec = OutSpecs.AddDefaulted_GetRef();
		Spec.Id = TEXT("H_CIRCLE");
		Spec.Kind = EKind::StoneCircle;
		Spec.Chain = EChain::Circle;
		Spec.Size = ESize::Small;
		Spec.BodyKey = KeyOf(Body);
		Spec.Seed = Hash(Seed, Spec.Id, Spec.BodyKey);
		Spec.Scale = 1.0f + 0.3f * static_cast<float>((Spec.Seed >> 9) % 100u) / 100.0f;
		OutBodies.Add(Spec.Id, Body);
	}

	// The derelict in the home planet's high orbit.
	{
		FSiteSpec& Spec = OutSpecs.AddDefaulted_GetRef();
		Spec.Id = TEXT("H_HULL");
		Spec.Kind = EKind::Derelict;
		Spec.Chain = EChain::QuietHull;
		Spec.Size = ESize::Monumental;
		Spec.BodyKey = HomeKey;
		Spec.Seed = Hash(Seed, Spec.Id, HomeKey);
		OutBodies.Add(Spec.Id, HomePlanet);
	}
}

bool APSAncientsSites::RollLostWorks(const int32 WorldSeed, const APlanetaryBody* Body, APSAncients::FSiteSpec& OutSpec)
{
	using namespace APSAncients;
	using namespace APSAncientsSitesLocal;
	if (!IsValid(Body) || Body->AstroName.IsNone() || !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		return false;
	}
	const FString Seed = FString::FromInt(WorldSeed);
	const FString Key = KeyOf(Body);
	if (Hash(Seed, TEXT("LOST"), Key) % 1000u >= LostWorksChance)
	{
		return false;
	}
	OutSpec = FSiteSpec();
	OutSpec.Id = TEXT("H_LOST_") + Sanitize(Body->AstroName.ToString());
	OutSpec.Chain = EChain::LostWorks;
	OutSpec.BodyKey = Key;
	OutSpec.Seed = Hash(Seed, OutSpec.Id, Key);
	// Cube 32, spires 26, a smaller needle 22, circle 20: large works, and now and then a small one.
	const uint32 Roll = (OutSpec.Seed >> 3) % 100u;
	if (Roll < 32u)
	{
		OutSpec.Kind = EKind::SunkenCube;
		OutSpec.Size = ESize::Large;
		OutSpec.Scale = 0.6f + 0.3f * static_cast<float>((OutSpec.Seed >> 9) % 100u) / 100.0f;
	}
	else if (Roll < 80u)
	{
		OutSpec.Kind = Roll < 58u ? EKind::SpireField : EKind::Monolith;
		OutSpec.Size = ESize::Large;
		OutSpec.Scale = LargeScale(OutSpec.Seed);
	}
	else
	{
		OutSpec.Kind = EKind::StoneCircle;
		OutSpec.Size = ESize::Small;
		OutSpec.Scale = 1.0f + 0.4f * static_cast<float>((OutSpec.Seed >> 9) % 100u) / 100.0f;
	}
	return true;
}

void APSAncientsSites::BuildNearbySpecs(const FAPSStarSystems& Stars, TArray<APSAncients::FSiteSpec>& OutSpecs)
{
	using namespace APSAncients;
	using namespace APSAncientsSitesLocal;
	const FAPSStarSystemInfo* Home = Stars.GetHome();
	if (!Home)
	{
		return;
	}
	TArray<int32> Nearest;
	Stars.FindNearest(Home->Location, NearbySystems + 8, Nearest);
	TArray<int32> Ranked;
	for (const int32 Index : Nearest)
	{
		const FAPSStarSystemInfo* Info = Stars.Get(Index);
		// Systems without worlds, the home and stars inside its sphere carry no site.
		if (!Info || Info->bHome || Info->bInsideHome || Info->PotentialPlanets <= 0)
		{
			continue;
		}
		Ranked.Add(Index);
		if (Ranked.Num() >= NearbySystems)
		{
			break;
		}
	}
	const FString Cluster = FString::FromInt(Stars.GetClusterSeed());
	const int32 First = OutSpecs.Num();
	bool bNearestHaveOne = false;
	for (int32 Rank = 0; Rank < Ranked.Num(); ++Rank)
	{
		const FAPSStarSystemInfo* Info = Stars.Get(Ranked[Rank]);
		const uint32 Chance = Rank < 3 ? 450u : Rank < 8 ? 280u : 160u;
		if (Hash(Cluster, TEXT("NEARBY"), Info->Id.ToString(EGuidFormats::Digits)) % 1000u < Chance)
		{
			MakeNearby(*Info, Ranked[Rank], Rank, Cluster, OutSpecs);
			bNearestHaveOne |= Rank < 3;
		}
	}
	// The guarantee (Rio: "something interesting in the nearest systems"): the nearest system gets one when none of the
	// three nearest rolled one.
	if (!bNearestHaveOne && !Ranked.IsEmpty())
	{
		MakeNearby(*Stars.Get(Ranked[0]), Ranked[0], 0, Cluster, OutSpecs);
	}
	TArray<FSiteSpec> Nearby(OutSpecs.GetData() + First, OutSpecs.Num() - First);
	Nearby.Sort([](const FSiteSpec& A, const FSiteSpec& B) { return A.Rank < B.Rank; });
	OutSpecs.SetNum(First);
	OutSpecs.Append(MoveTemp(Nearby));
}

FVector APSAncientsSites::CandidateDirection(const uint32 Seed, const int32 Index, const double MaxLatitudeDegrees)
{
	const FRandomStream Stream(APSAncientsSitesLocal::StreamSeed(APSAncients::Hash(FString::Printf(TEXT("%u"), Seed),
		TEXT("CANDIDATE"), FString::FromInt(Index))));
	const double Latitude = FMath::DegreesToRadians(Stream.FRandRange(-MaxLatitudeDegrees, MaxLatitudeDegrees));
	const double Longitude = FMath::DegreesToRadians(Stream.FRandRange(-180.0, 180.0));
	return FVector(FMath::Cos(Latitude) * FMath::Cos(Longitude), FMath::Cos(Latitude) * FMath::Sin(Longitude),
		FMath::Sin(Latitude));
}

double APSAncientsSites::Yaw(const uint32 Seed)
{
	const FRandomStream Stream(APSAncientsSitesLocal::StreamSeed(APSAncients::Hash(FString::Printf(TEXT("%u"), Seed), TEXT("YAW"))));
	return Stream.FRandRange(0.0, UE_TWO_PI);
}

void APSAncientsSites::OrbitOf(const uint32 Seed, FVector& OutLocalDirection, double& OutRadii)
{
	const FRandomStream Stream(APSAncientsSitesLocal::StreamSeed(APSAncients::Hash(FString::Printf(TEXT("%u"), Seed), TEXT("ORBIT"))));
	OutRadii = Stream.FRandRange(1.9, 2.6);
	OutLocalDirection = CandidateDirection(Seed, 0, 40.0);
}

FQuat APSAncientsSites::OrbitalTurn(const uint32 Seed)
{
	const FRandomStream Stream(APSAncientsSitesLocal::StreamSeed(APSAncients::Hash(FString::Printf(TEXT("%u"), Seed), TEXT("TURN"))));
	return FRotator(Stream.FRandRange(-55.0, 55.0), Stream.FRandRange(0.0, 360.0), Stream.FRandRange(-180.0, 180.0)).Quaternion();
}

FString APSAncientsSites::KeyOf(const AActor* Body)
{
	return FAPSFleetCommand::KeyOf(Body);
}

FText APSAncientsSites::BodyName(const AActor* Body)
{
	return FAPSFleetCommand::DisplayName(Body);
}

#undef LOCTEXT_NAMESPACE
