#include "APSStarMapModel.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "APSStarMap"

namespace APSStarMapLocal
{
	using namespace APSStarMap;

	constexpr double TwoPi = 6.28318530717958647692;
	constexpr double HalfPi = 1.57079632679489661923;
	/** A fixed bearing for a system straight above or below the plane, by its place in the list. */
	constexpr double GoldenAngle = 2.39996322972865332;
	/** The approved mock's least arc between neighbours on each ring: 96, 92, 104 and 140 px at 126 px a unit. */
	constexpr double MinimumArc[4] = {96.0 / 126.0, 92.0 / 126.0, 104.0 / 126.0, 140.0 / 126.0};
	/** Each ring's distance caption sits on its top: that band (26 px deep) and 10 px beside it stay free of stars. */
	constexpr double CaptionDepth = 26.0 / 126.0;
	constexpr double CaptionMargin = 10.0 / 126.0;

	double Wrap(const double Angle)
	{
		const double Wrapped = FMath::Fmod(Angle, TwoPi);
		return Wrapped < 0.0 ? Wrapped + TwoPi : Wrapped;
	}

	/**
	 * Along a ring (the mock's relaxation, star_level_mock.py, made exact): neighbours at least RingSeparation apart, the
	 * caption's band at the top free, and the stars moved as little as that allows (least squares) without ever passing
	 * each other, so the order along the ring and the rough directions stay. The ring is cut at its top: the members in
	 * their order from there are spaced by isotonic regression (pool adjacent violators) of bearing - rank * separation,
	 * clamped into the band's limits.
	 */
	void Spread(const TArray<int32>& Members, const int32 Ring, TArray<FLayoutSlot>& Slots)
	{
		const int32 Count = Members.Num();
		if (Count == 0)
		{
			return;
		}
		const double Band = CaptionBand(Ring);
		const double Separation = RingSeparation(Ring, Count);
		struct FEntry
		{
			/** From the top, counter-clockwise, 0..2 pi. */
			double Along{0.0};
			int32 Member{INDEX_NONE};
		};
		TArray<FEntry> Entries;
		Entries.Reserve(Count);
		for (const int32 Member : Members)
		{
			Entries.Add({Wrap(Slots[Member].Angle - HalfPi), Member});
		}
		Entries.Sort([](const FEntry& A, const FEntry& B)
		{
			return A.Along != B.Along ? A.Along < B.Along : A.Member < B.Member;
		});
		// Pool adjacent violators over Along - rank * separation: blocks of equal values, each the mean of its members.
		struct FBlock
		{
			double Sum{0.0};
			int32 Size{0};
			double Mean() const { return Sum / Size; }
		};
		TArray<FBlock> Blocks;
		Blocks.Reserve(Count);
		for (int32 Rank = 0; Rank < Count; ++Rank)
		{
			Blocks.Add({Entries[Rank].Along - Rank * Separation, 1});
			while (Blocks.Num() >= 2 && Blocks[Blocks.Num() - 2].Mean() > Blocks.Last().Mean())
			{
				const FBlock Last = Blocks.Pop(EAllowShrinking::No);
				Blocks.Last().Sum += Last.Sum;
				Blocks.Last().Size += Last.Size;
			}
		}
		const double Lowest = Band;
		const double Highest = FMath::Max(TwoPi - Band - (Count - 1) * Separation, Lowest);
		int32 Rank = 0;
		for (const FBlock& Block : Blocks)
		{
			const double Value = FMath::Clamp(Block.Mean(), Lowest, Highest);
			for (int32 Each = 0; Each < Block.Size; ++Each, ++Rank)
			{
				Slots[Entries[Rank].Member].Angle = Wrap(Value + Rank * Separation + HalfPi);
			}
		}
	}

	/** The disc a class is drawn with and its kind: by the letter, larger for giants (which also glow). */
	struct FClassLook
	{
		float Disc{5.0f};
		bool bGiant{false};
		FText Kind;
	};

	FClassLook LookOf(const FString& Spectral)
	{
		FClassLook Look;
		Look.Kind = LOCTEXT("KindStar", "STAR");
		const FString Text = Spectral.TrimStartAndEnd();
		if (Text.IsEmpty() || Text.StartsWith(TEXT("Unknown")))
		{
			return Look;
		}
		if (Text.StartsWith(TEXT("NS")))
		{
			Look.Disc = 3.0f;
			Look.Kind = LOCTEXT("KindNeutron", "NEUTRON STAR");
			return Look;
		}
		if (Text.StartsWith(TEXT("BH")))
		{
			Look.Disc = 4.0f;
			Look.Kind = LOCTEXT("KindBlackHole", "BLACK HOLE");
			return Look;
		}
		if (Text.StartsWith(TEXT("PS")) || Text.StartsWith(TEXT("PROTO")))
		{
			Look.Disc = 6.0f;
			Look.Kind = LOCTEXT("KindProtostar", "PROTOSTAR");
			return Look;
		}
		// The cluster writes class, subclass and luminosity class ("K4III"); the galaxy's stars also "DA" and "sdK3".
		bool bWhiteDwarf = false;
		bool bSubdwarf = false;
		int32 At = 0;
		if (Text.StartsWith(TEXT("sd"), ESearchCase::CaseSensitive))
		{
			bSubdwarf = true;
			At = 2;
		}
		else if (Text.Len() >= 2 && Text[0] == TEXT('D') && FChar::IsAlpha(Text[1]))
		{
			bWhiteDwarf = true;
			At = 1;
		}
		const TCHAR Letter = At < Text.Len() ? FChar::ToUpper(Text[At]) : TEXT('G');
		++At;
		while (At < Text.Len() && (FChar::IsDigit(Text[At]) || Text[At] == TEXT('.')))
		{
			++At;
		}
		const FString Suffix = Text.Mid(At);
		float Base = 5.0f;
		switch (Letter)
		{
		case TEXT('O'): Base = 9.5f; break;
		case TEXT('B'): Base = 9.0f; break;
		case TEXT('A'): Base = 8.0f; break;
		case TEXT('F'): Base = 7.0f; break;
		case TEXT('G'): Base = 6.5f; break;
		case TEXT('K'): Base = 5.6f; break;
		case TEXT('M'): Base = 4.6f; break;
		case TEXT('L'): Base = 3.4f; break;
		case TEXT('T'): Base = 3.2f; break;
		case TEXT('Y'): Base = 3.0f; break;
		default: break;
		}
		Look.Disc = Base;
		if (bWhiteDwarf || Suffix == TEXT("VII"))
		{
			Look.Disc = 3.6f;
			Look.Kind = LOCTEXT("KindWhiteDwarf", "WHITE DWARF");
		}
		else if (Letter == TEXT('L') || Letter == TEXT('T') || Letter == TEXT('Y') || Suffix == TEXT("VIII"))
		{
			Look.Disc = FMath::Min(Base, 3.4f);
			Look.Kind = LOCTEXT("KindBrownDwarf", "BROWN DWARF");
		}
		else if (bSubdwarf || Suffix == TEXT("VI"))
		{
			Look.Disc = Base * 0.85f;
			Look.Kind = LOCTEXT("KindSubdwarf", "SUBDWARF");
		}
		else if (Suffix == TEXT("IV"))
		{
			Look.Disc = FMath::Max(Base * 1.3f, 7.0f);
			Look.Kind = LOCTEXT("KindSubgiant", "SUBGIANT");
		}
		else if (Suffix == TEXT("III"))
		{
			Look.Disc = 10.0f;
			Look.bGiant = true;
			Look.Kind = LOCTEXT("KindGiant", "GIANT");
		}
		else if (Suffix == TEXT("II"))
		{
			Look.Disc = 10.5f;
			Look.bGiant = true;
			Look.Kind = LOCTEXT("KindBrightGiant", "BRIGHT GIANT");
		}
		else if (Suffix == TEXT("Ia+") || Suffix == TEXT("O") || Suffix == TEXT("0"))
		{
			Look.Disc = 11.5f;
			Look.bGiant = true;
			Look.Kind = LOCTEXT("KindHypergiant", "HYPERGIANT");
		}
		else if (Suffix.StartsWith(TEXT("I"), ESearchCase::CaseSensitive))
		{
			Look.Disc = 11.0f;
			Look.bGiant = true;
			Look.Kind = LOCTEXT("KindSupergiant", "SUPERGIANT");
		}
		else
		{
			Look.Kind = LOCTEXT("KindMainSequence", "MAIN SEQUENCE");
		}
		return Look;
	}

	/** The worlds' colours, as the system scheme paints them (SAPSSystemScheme TypeColor). */
	FLinearColor WorldColour(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Melted: case EPlanetType::Volcanic: case EPlanetType::Lava: case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.32f, 0.12f);
		case EPlanetType::GasGiant: return FLinearColor(0.98f, 0.70f, 0.30f);
		case EPlanetType::IceGiant: case EPlanetType::Ice: case EPlanetType::Frozen: return FLinearColor(0.55f, 0.85f, 1.0f);
		case EPlanetType::Ocean: case EPlanetType::Water: case EPlanetType::Archipelago: return FLinearColor(0.18f, 0.55f, 1.0f);
		case EPlanetType::Terrestrial: case EPlanetType::Forest: case EPlanetType::Oasis: return FLinearColor(0.25f, 0.88f, 0.55f);
		case EPlanetType::Desert: case EPlanetType::Sand: return FLinearColor(0.95f, 0.74f, 0.36f);
		case EPlanetType::Metal: case EPlanetType::Metallic: case EPlanetType::Carbon: return FLinearColor(0.72f, 0.66f, 0.90f);
		default: return FLinearColor(0.70f, 0.72f, 0.76f);
		}
	}

	FText WorldTypeName(const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Name);
	}

	/** The star's light as a marker on dark space: its hue at full brightness (as the strategic map's StarColour). */
	FLinearColor MarkerColour(const FLinearColor& Light)
	{
		const float Peak = FMath::Max3(Light.R, Light.G, Light.B);
		return Peak > 0.0f ? FLinearColor(Light.R / Peak, Light.G / Peak, Light.B / Peak, 1.0f) : APSChrome::Amber();
	}

	/** The home system's planets' orbital plane, as on the system and network maps: (U, V), V = normal x U. */
	void PlaneOf(UWorld* World, const FAPSStarSystems& Stars, FVector& OutU, FVector& OutV)
	{
		FVector Normal = FVector::UpVector;
		const AAstroGenerator* Generator = Stars.GetGenerator();
		const AStar* HomeStar = Generator ? Generator->HomeStar : nullptr;
		for (TActorIterator<APlanet> It(World); It; ++It)
		{
			if (!IsValid(*It) || (HomeStar && It->ParentStar != HomeStar))
			{
				continue;
			}
			if (const AActor* Orbit = It->GetAttachParentActor(); Orbit && Orbit->IsA<APlanetOrbit>())
			{
				Normal = Orbit->GetActorUpVector();
				break;
			}
		}
		OutU = FVector::VectorPlaneProject(FVector::ForwardVector, Normal).GetSafeNormal();
		if (OutU.IsNearlyZero())
		{
			OutU = FVector::VectorPlaneProject(FVector::RightVector, Normal).GetSafeNormal();
		}
		OutV = FVector::CrossProduct(Normal, OutU).GetSafeNormal();
	}

	/** The system an actor stands for or is in: a system's anchor, else the system whose room holds it. */
	int32 SystemOf(const FAPSStarSystems& Stars, const AActor* Actor)
	{
		if (!Actor)
		{
			return INDEX_NONE;
		}
		if (FGuid Id; FAPSStarSystems::AnchorSystem(Actor, Id))
		{
			return Stars.IndexOf(Id);
		}
		return Stars.FindContaining(Actor->GetActorLocation());
	}

	FText WorkWord(const APSFleet::EOrder Order)
	{
		using APSFleet::EOrder;
		switch (Order)
		{
		case EOrder::BuildOutpost:
		case EOrder::BuildStation:
		case EOrder::BuildShipyard:
		case EOrder::BuildHeadquarters:
		case EOrder::BuildStructure: return LOCTEXT("WorkBuilding", "BUILDING");
		case EOrder::Survey:
		case EOrder::SurveySystem: return LOCTEXT("WorkSurveying", "SURVEYING");
		case EOrder::Probe: return LOCTEXT("WorkScanning", "SCANNING");
		case EOrder::Expedition: return LOCTEXT("WorkInvestigating", "INVESTIGATING");
		default: return LOCTEXT("WorkAtWork", "AT WORK");
		}
	}

	/** A route's live state from its unit: the share flown, the work, the time left and the chip. */
	void ReadRoute(const FAPSFleetCommand& Fleet, const FAPSFleetUnit& Unit, FRoute& Route)
	{
		using APSFleet::EPhase;
		Route.Phase = Unit.Phase;
		Route.Progress = FMath::Clamp(Unit.Progress, 0.0f, 1.0f);
		Route.Share = Unit.Phase == EPhase::Transit && Unit.TransitStartCm > 0.0
			? static_cast<float>(FMath::Clamp(1.0 - Unit.RemainingCm / Unit.TransitStartCm, 0.0, 1.0))
			: Unit.Phase == EPhase::Working || Unit.Phase == EPhase::Holding ? 1.0f : 0.0f;
		const AActor* Target = Unit.Order == APSFleet::EOrder::Return ? Unit.Berth.Get() : Unit.Target.Get();
		Route.EtaSeconds = Unit.Phase == EPhase::Departing || Unit.Phase == EPhase::Transit
			? Fleet.EstimateArrivalSeconds(Unit.Ship.Get(), Target) : -1.0;
		if (Unit.Phase == EPhase::Working)
		{
			Route.Chip = FText::Format(LOCTEXT("ChipWork", "{0}  {1}  {2}%"), FText::FromString(Unit.CallSign), WorkWord(Unit.Order),
				APSUINumber::Number(FMath::RoundToInt(Route.Progress * 100.0f)));
		}
		else
		{
			const FString Eta = FormatEta(Route.EtaSeconds);
			Route.Chip = Eta.IsEmpty()
				? FText::Format(LOCTEXT("ChipRoute", "{0}  {1}"), FText::FromString(Unit.CallSign), OrderWord(Unit.Order))
				: FText::Format(LOCTEXT("ChipRouteEta", "{0}  {1}  {2}"), FText::FromString(Unit.CallSign), OrderWord(Unit.Order),
					FText::FromString(Eta));
		}
	}

	FText DistanceCaption(const double Cm)
	{
		return FText::FromString(UShipNavigationComponent::FormatDistance(Cm));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Layout (no world)

double APSStarMap::CaptionBand(const int32 Ring)
{
	using namespace APSStarMapLocal;
	const double Radius = RingRadius[FMath::Clamp(Ring, 0, OuterRing)];
	return FMath::Acos(FMath::Clamp((Radius - CaptionDepth) / Radius, -1.0, 1.0)) + CaptionMargin / Radius;
}

double APSStarMap::RingSeparation(const int32 Ring, const int32 Count)
{
	using namespace APSStarMapLocal;
	const double Radius = RingRadius[FMath::Clamp(Ring, 0, OuterRing)];
	const double Room = TwoPi - 2.0 * CaptionBand(Ring);
	return FMath::Max(FMath::Min(MinimumArc[FMath::Clamp(Ring, 0, OuterRing)] / Radius, 0.92 * Room / FMath::Max(Count, 1)), 1.0e-4);
}

void APSStarMap::AssignRings(const int32 Count, const int32 NearCount, const TArray<int32>& PreviousRings,
	TArray<int32>& OutRings)
{
	OutRings.SetNumUninitialized(FMath::Max(Count, 0));
	int32 Starts[RankRings + 1];
	Starts[0] = 0;
	for (int32 Ring = 0; Ring < RankRings; ++Ring)
	{
		Starts[Ring + 1] = Starts[Ring] + RingSizes[Ring];
	}
	for (int32 Rank = 0; Rank < Count; ++Rank)
	{
		if (Rank >= NearCount)
		{
			OutRings[Rank] = OuterRing;
			continue;
		}
		int32 Ring = 0;
		while (Ring < RankRings - 1 && Rank >= Starts[Ring + 1])
		{
			++Ring;
		}
		// Hysteresis: one place past a ring's edge a system keeps the ring it had.
		const int32 Previous = PreviousRings.IsValidIndex(Rank) ? PreviousRings[Rank] : INDEX_NONE;
		if (Previous >= 0 && Previous < RankRings && Previous != Ring && Rank >= Starts[Previous] - 1
			&& Rank <= Starts[Previous + 1])
		{
			Ring = Previous;
		}
		OutRings[Rank] = Ring;
	}
}

void APSStarMap::Layout(const TArray<FLayoutItem>& Items, const FVector& PlaneU, const FVector& PlaneV,
	TArray<FLayoutSlot>& OutSlots)
{
	using namespace APSStarMapLocal;
	OutSlots.Reset();
	OutSlots.SetNum(Items.Num());
	const FVector Normal = FVector::CrossProduct(PlaneU, PlaneV).GetSafeNormal();
	TArray<double> Distances;
	Distances.SetNumZeroed(Items.Num());
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FVector& Offset = Items[Index].Offset;
		const double Distance = Offset.Size();
		const double X = FVector::DotProduct(Offset, PlaneU);
		const double Y = FVector::DotProduct(Offset, PlaneV);
		FLayoutSlot& Slot = OutSlots[Index];
		Distances[Index] = Distance;
		// Straight above or below the plane a system has no direction in it: a fixed one by its place in the list.
		Slot.Bearing = Distance > 0.0 && X * X + Y * Y > FMath::Square(Distance * 1.0e-9) ? Wrap(FMath::Atan2(Y, X))
			: Wrap(GoldenAngle * Index);
		Slot.Angle = Slot.Bearing;
		Slot.Elevation = Distance > 0.0 ? FMath::Clamp(FVector::DotProduct(Offset, Normal) / Distance, -1.0, 1.0) : 0.0;
	}
	for (int32 Ring = 0; Ring <= OuterRing; ++Ring)
	{
		TArray<int32> Members;
		double Nearest = TNumericLimits<double>::Max();
		double Farthest = 0.0;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index].Ring == Ring)
			{
				Members.Add(Index);
				Nearest = FMath::Min(Nearest, Distances[Index]);
				Farthest = FMath::Max(Farthest, Distances[Index]);
			}
		}
		if (Members.IsEmpty())
		{
			continue;
		}
		const double Radius = RingRadius[Ring];
		for (const int32 Member : Members)
		{
			// Nearer members a little inside their ring, farther ones outside: the order by distance shows.
			const double Share = Farthest > Nearest ? (Distances[Member] - Nearest) / (Farthest - Nearest) : 0.5;
			OutSlots[Member].Radius = Radius + (Ring < OuterRing ? (Share - 0.5) * RingSpread : 0.0);
		}
		Spread(Members, Ring, OutSlots);
	}
	for (FLayoutSlot& Slot : OutSlots)
	{
		Slot.Position = FVector2D(FMath::Cos(Slot.Angle), -FMath::Sin(Slot.Angle)) * Slot.Radius;
	}
}

void APSStarMap::SelectShown(const TArray<FCandidate>& Near, const TArray<FCandidate>& Extras, const int32 MaxCount,
	TArray<FCandidate>& OutShown, int32& OutNearCount)
{
	const auto ByDistance = [](const FCandidate& A, const FCandidate& B)
	{
		return A.DistanceCm != B.DistanceCm ? A.DistanceCm < B.DistanceCm : A.Index < B.Index;
	};
	OutShown = Near;
	OutShown.Sort(ByDistance);
	if (OutShown.Num() > MaxCount)
	{
		OutShown.SetNum(FMath::Max(MaxCount, 0));
	}
	OutNearCount = OutShown.Num();
	// Over the limit the extras go by priority, the farthest first; the pick and the fleet's always stay.
	TArray<FCandidate> Ranked = Extras;
	Ranked.Sort([](const FCandidate& A, const FCandidate& B)
	{
		return A.Priority != B.Priority ? A.Priority < B.Priority
			: A.DistanceCm != B.DistanceCm ? A.DistanceCm < B.DistanceCm : A.Index < B.Index;
	});
	const int32 Room = FMath::Max(MaxCount - OutNearCount, 0);
	TArray<FCandidate> Kept;
	for (const FCandidate& Candidate : Ranked)
	{
		if (Candidate.Priority <= 1 || Kept.Num() < Room)
		{
			Kept.Add(Candidate);
		}
	}
	Kept.Sort(ByDistance);
	OutShown.Append(Kept);
}

// ---------------------------------------------------------------------------------------------------------------------
// Words

FText APSStarMap::StateName(const FSystem& System)
{
	if (System.bHome)
	{
		return LOCTEXT("StateHome", "HOME");
	}
	return System.bClaimed ? LOCTEXT("StateClaimed", "CLAIMED") : APSStars::KnowledgeName(System.Knowledge);
}

FLinearColor APSStarMap::StateColour(const FSystem& System)
{
	if (System.bHome)
	{
		return APSChrome::Amber();
	}
	if (System.bClaimed)
	{
		// The relay network's teal (SAPSInfrastructurePanel).
		return FLinearColor(0.35f, 0.95f, 0.95f, 1.0f);
	}
	return System.Knowledge == APSStars::EKnowledge::Catalogued ? APSChrome::Muted().CopyWithNewOpacity(0.7f)
		: APSStars::KnowledgeColour(System.Knowledge);
}

FText APSStarMap::TypeLine(const FSystem& System)
{
	const FText Spectral = FText::FromString(System.Spectral.IsEmpty() ? FString(TEXT("STAR")) : System.Spectral);
	return System.Knowledge != APSStars::EKnowledge::Catalogued && System.Worlds == 0
		? FText::Format(LOCTEXT("TypeNoWorlds", "{0}  /  {1}  /  NO WORLDS"), Spectral, StateName(System))
		: FText::Format(LOCTEXT("TypeLine", "{0}  /  {1}"), Spectral, StateName(System));
}

FString APSStarMap::FormatEta(const double Seconds)
{
	if (!(Seconds >= 0.0) || !FMath::IsFinite(Seconds))
	{
		return FString();
	}
	const int64 Whole = static_cast<int64>(FMath::CeilToDouble(FMath::Min(Seconds, 359999.0)));
	return Whole >= 3600 ? FString::Printf(TEXT("%lld:%02lld:%02lld"), Whole / 3600, Whole / 60 % 60, Whole % 60)
		: FString::Printf(TEXT("%lld:%02lld"), Whole / 60, Whole % 60);
}

bool APSStarMap::PassesFilter(const FSystem& System, const EFilter Filter)
{
	const bool bKnown = System.bClaimed || System.Knowledge != APSStars::EKnowledge::Catalogued;
	switch (Filter)
	{
	case EFilter::Known: return bKnown || System.bCentre;
	case EFilter::Claimed: return System.bClaimed || System.bCentre;
	case EFilter::Uncharted: return !bKnown || System.bCentre;
	default: return true;
	}
}

FText APSStarMap::OrderWord(const APSFleet::EOrder Order)
{
	switch (Order)
	{
	case APSFleet::EOrder::SurveySystem: return LOCTEXT("WordSurveySystem", "SURVEY");
	case APSFleet::EOrder::BuildStructure: return LOCTEXT("WordBuild", "BUILD");
	case APSFleet::EOrder::BuildOutpost: return LOCTEXT("WordOutpost", "OUTPOST");
	default: return APSFleet::OrderName(Order);
	}
}

ASpaceship* APSStarMap::PickShip(const FAPSFleetCommand& Fleet, const APSFleet::EOrder Order, const AActor* Target,
	FText& OutRefusal)
{
	using namespace APSFleet;
	OutRefusal = FText::GetEmpty();
	if (!Target)
	{
		OutRefusal = LOCTEXT("PickNoTarget", "Pick a star system first.");
		return nullptr;
	}
	TArray<EDivision, TInlineAllocator<2>> Divisions;
	switch (Order)
	{
	case EOrder::Probe: Divisions = {EDivision::Exploration, EDivision::Science}; break;
	case EOrder::SurveySystem:
	case EOrder::Expedition: Divisions = {EDivision::Science, EDivision::Exploration}; break;
	default: Divisions = {EDivision::MainFleet}; break;
	}
	for (const EDivision Division : Divisions)
	{
		ASpaceship* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (const FAPSFleetUnit& Unit : Fleet.GetUnits())
		{
			ASpaceship* Ship = Unit.Ship.Get();
			if (!Ship || Unit.Division != Division || (Unit.Order != EOrder::None && Unit.Phase != EPhase::Holding))
			{
				continue;
			}
			const FText Refusal = Fleet.CheckOrder(Ship, Order, Target);
			if (!Refusal.IsEmpty())
			{
				if (OutRefusal.IsEmpty())
				{
					OutRefusal = Refusal;
				}
				continue;
			}
			const double Distance = FVector::DistSquared(Ship->GetActorLocation(), Target->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Ship;
			}
		}
		if (Best)
		{
			OutRefusal = FText::GetEmpty();
			return Best;
		}
	}
	if (OutRefusal.IsEmpty())
	{
		OutRefusal = FText::Format(LOCTEXT("PickNoShip", "No free {0} ship."), DivisionName(Divisions[0]));
	}
	return nullptr;
}

AActor* APSStarMap::FindStarActor(UWorld* World, const FAPSStarSystems& Stars, const int32 Index)
{
	const FAPSStarSystemInfo* Info = Stars.Get(Index);
	if (!World || !Info)
	{
		return nullptr;
	}
	if (Info->bHome)
	{
		const AAstroGenerator* Generator = Stars.GetGenerator();
		return Generator && IsValid(Generator->HomeStar) ? Generator->HomeStar : nullptr;
	}
	const FAPSSystemMaterializer* Materializer = Stars.GetMaterializer();
	if (!Materializer || Materializer->GetActiveIndex() != Index)
	{
		return nullptr;
	}
	// The materialized system's star stands at its catalogue place.
	AActor* Best = nullptr;
	double BestDistance = FMath::Square(FMath::Max(Info->RoomCm, APSStars::AstronomicalUnitCm));
	for (TActorIterator<AStar> It(World); It; ++It)
	{
		const double Distance = IsValid(*It) ? FVector::DistSquared(It->GetActorLocation(), Info->Location)
			: TNumericLimits<double>::Max();
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	return Best;
}

// ---------------------------------------------------------------------------------------------------------------------
// The model

bool FAPSStarMapModel::Update(UWorld* World, const FQuery& Query, const bool bForce)
{
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	const FAPSSystemMaterializer* Materializer = Stars ? Stars->GetMaterializer() : nullptr;
	const uint32 StarsRevision = Stars ? Stars->GetRevision() : 0;
	const uint32 FleetRevision = Fleet ? Fleet->GetRevision() : 0;
	const uint32 InfrastructureRevision = Infrastructure ? Infrastructure->GetRevision() : 0;
	const int32 Materialized = Materializer ? Materializer->GetActiveIndex() : INDEX_NONE;
	const bool bReady = Stars && Stars->IsReady();
	const bool bRebuild = bForce || !bBuilt || StarsRevision != SeenStars || FleetRevision != SeenFleet
		|| InfrastructureRevision != SeenInfrastructure || Materialized != SeenMaterialized || Query != SeenQuery
		|| bReady != Snapshot.bReady;
	if (bRebuild)
	{
		SeenStars = StarsRevision;
		SeenFleet = FleetRevision;
		SeenInfrastructure = InfrastructureRevision;
		SeenMaterialized = Materialized;
		SeenQuery = Query;
		bBuilt = true;
		Rebuild(World, Query);
	}
	UpdateLive(World);
	return bRebuild;
}

int32 FAPSStarMapModel::IndexOf(const FGuid& Id) const
{
	const int32* Found = SnapshotIndex.Find(Id);
	return Found ? *Found : INDEX_NONE;
}

double FAPSStarMapModel::RadiusAt(const double DistanceCm) const
{
	if (RadiusByDistance.IsEmpty())
	{
		return 0.0;
	}
	for (int32 Index = 1; Index < RadiusByDistance.Num(); ++Index)
	{
		const TPair<double, double>& Before = RadiusByDistance[Index - 1];
		const TPair<double, double>& After = RadiusByDistance[Index];
		if (DistanceCm <= After.Key)
		{
			const double Share = After.Key > Before.Key ? (DistanceCm - Before.Key) / (After.Key - Before.Key) : 1.0;
			return FMath::Lerp(Before.Value, After.Value, FMath::Clamp(Share, 0.0, 1.0));
		}
	}
	// Beyond the farthest shown: just outside it.
	return RadiusByDistance.Last().Value + 0.12;
}

FVector2D FAPSStarMapModel::PlaceAt(const FVector& WorldLocation) const
{
	const FVector Offset = WorldLocation - Snapshot.CentreLocation;
	const double X = FVector::DotProduct(Offset, Snapshot.PlaneU);
	const double Y = FVector::DotProduct(Offset, Snapshot.PlaneV);
	const double Angle = X * X + Y * Y > 1.0 ? FMath::Atan2(Y, X) : 0.0;
	return FVector2D(FMath::Cos(Angle), -FMath::Sin(Angle)) * RadiusAt(Offset.Size());
}

bool FAPSStarMapModel::LocateShip(UWorld* World, const AActor* Ship, int32& OutSystem, FVector2D& OutPosition) const
{
	OutSystem = INDEX_NONE;
	OutPosition = FVector2D::ZeroVector;
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	if (!Ship || !Stars || !Snapshot.bReady)
	{
		return false;
	}
	const int32 Containing = Stars->FindContaining(Ship->GetActorLocation());
	const FAPSStarSystemInfo* Info = Stars->Get(Containing);
	OutSystem = Info ? IndexOf(Info->Id) : INDEX_NONE;
	OutPosition = Snapshot.Systems.IsValidIndex(OutSystem) ? Snapshot.Systems[OutSystem].Slot.Position
		: PlaceAt(Ship->GetActorLocation());
	return true;
}

void FAPSStarMapModel::Rebuild(UWorld* World, const FQuery& Query)
{
	using namespace APSStarMap;
	using namespace APSStarMapLocal;
	FSnapshot Next;
	SnapshotIndex.Reset();
	RadiusByDistance.Reset();
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	const FAPSStarSystemInfo* CentreInfo = nullptr;
	int32 CentreIndex = INDEX_NONE;
	if (Stars && Stars->IsReady())
	{
		CentreIndex = Query.Centre.IsValid() ? Stars->IndexOf(Query.Centre) : INDEX_NONE;
		if (!Stars->Get(CentreIndex) || Stars->Get(CentreIndex)->bInsideHome)
		{
			CentreIndex = Stars->GetHomeIndex();
		}
		CentreInfo = Stars->Get(CentreIndex);
	}
	if (!CentreInfo)
	{
		Next.Status = Stars && Stars->IsReady() ? LOCTEXT("NoHome", "NO HOME SYSTEM IN THE CATALOGUE")
			: LOCTEXT("NotReady", "THE STAR CATALOGUE IS STILL BEING READ");
		Snapshot = MoveTemp(Next);
		LayoutHash = 0;
		++LayoutVersion;
		return;
	}
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	const FAPSSystemMaterializer* Materializer = Stars->GetMaterializer();
	const int32 MaterializedIndex = Materializer ? Materializer->GetActiveIndex() : INDEX_NONE;
	Next.bReady = true;
	Next.CentreId = CentreInfo->Id;
	Next.CentreLocation = CentreInfo->Location;
	PlaneOf(World, *Stars, Next.PlaneU, Next.PlaneV);
	const FVector CentreLocation = CentreInfo->Location;

	// The 25 nearest (one catalogue pass, only on a revision), then the known, the fleet's and the pinned.
	TArray<int32> NearestIndices;
	Stars->FindNearest(CentreLocation, NearestCount + 1, NearestIndices);
	TArray<FCandidate> Near;
	TSet<int32> InNear;
	for (const int32 Index : NearestIndices)
	{
		if (Index != CentreIndex && Near.Num() < NearestCount)
		{
			Near.Add({Index, FVector::Dist(Stars->Get(Index)->Location, CentreLocation), 4});
			InNear.Add(Index);
		}
	}
	TMap<int32, uint8> ExtraPriority;
	const auto AddExtra = [&](const int32 Index, const uint8 Priority)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		if (!Info || Info->bInsideHome || Index == CentreIndex || InNear.Contains(Index))
		{
			return;
		}
		uint8& Existing = ExtraPriority.FindOrAdd(Index, Priority);
		Existing = FMath::Min(Existing, Priority);
	};
	TArray<int32> Known;
	Stars->GetKnown(Known);
	for (const int32 Index : Known)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		AddExtra(Index, Info && Stars->IsClaimed(Info->Id) ? 2 : 3);
	}
	// The fleet: where each order goes and where its flight began (seen when it began), and where every ship is.
	struct FUnitRoute
	{
		const FAPSFleetUnit* Unit{nullptr};
		int32 From{INDEX_NONE};
		int32 To{INDEX_NONE};
		bool bSystemOrder{false};
	};
	TArray<FUnitRoute> UnitRoutes;
	TSet<int32> InUse;
	TMap<int32, TArray<int32>> ShipsBySystem;
	if (Fleet)
	{
		TSet<FString> LiveKeys;
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			const ASpaceship* Ship = Unit.Ship.Get();
			if (!Ship)
			{
				continue;
			}
			const int32 Place = Stars->FindContaining(Ship->GetActorLocation());
			const bool bUnderWay = Unit.Order != APSFleet::EOrder::None
				&& (Unit.Phase == APSFleet::EPhase::Departing || Unit.Phase == APSFleet::EPhase::Transit);
			if (Place != INDEX_NONE && !bUnderWay)
			{
				ShipsBySystem.FindOrAdd(Place).Add(static_cast<int32>(Unit.Division));
				InUse.Add(Place);
				AddExtra(Place, 1);
			}
			const AActor* TargetActor = Unit.Order == APSFleet::EOrder::Return ? Unit.Berth.Get() : Unit.Target.Get();
			if (Unit.Order == APSFleet::EOrder::None || Unit.Phase == APSFleet::EPhase::Holding || !TargetActor)
			{
				continue;
			}
			const int32 To = SystemOf(*Stars, TargetActor);
			if (To == INDEX_NONE)
			{
				continue;
			}
			const FString Key = FString::Printf(TEXT("%s|%d|%s"), *Unit.CallSign, static_cast<int32>(Unit.Order),
				*GetNameSafe(TargetActor));
			LiveKeys.Add(Key);
			int32 From = INDEX_NONE;
			if (const int32* Remembered = RouteOrigins.Find(Key))
			{
				From = *Remembered;
			}
			else
			{
				From = Place;
				if (From == INDEX_NONE && Unit.TransitStartCm > Unit.RemainingCm)
				{
					// First seen under way: where the flight began, back along the way it came.
					const FVector Away = (Ship->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
					From = Stars->FindContaining(Ship->GetActorLocation() + Away * (Unit.TransitStartCm - Unit.RemainingCm));
				}
				From = From == INDEX_NONE ? To : From;
				RouteOrigins.Add(Key, From);
			}
			FGuid AnchorId;
			const bool bSystemOrder = FAPSStarSystems::AnchorSystem(TargetActor, AnchorId);
			// Flights and work inside the home system belong to the system map.
			const FAPSStarSystemInfo* ToInfo = Stars->Get(To);
			if (From == To && !bSystemOrder && ToInfo && ToInfo->bHome)
			{
				continue;
			}
			UnitRoutes.Add({&Unit, From, To, bSystemOrder});
			for (const int32 Index : {From, To})
			{
				InUse.Add(Index);
				AddExtra(Index, 1);
			}
		}
		for (auto It = RouteOrigins.CreateIterator(); It; ++It)
		{
			if (!LiveKeys.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}
	for (const FGuid& Id : Query.Pinned)
	{
		AddExtra(Stars->IndexOf(Id), 0);
	}
	TArray<FCandidate> Extras;
	for (const TPair<int32, uint8>& Pair : ExtraPriority)
	{
		Extras.Add({Pair.Key, FVector::Dist(Stars->Get(Pair.Key)->Location, CentreLocation), Pair.Value});
	}
	TArray<FCandidate> Shown;
	int32 NearCount = 0;
	SelectShown(Near, Extras, MaxShown, Shown, NearCount);

	// Rings (the previous ones for the hysteresis) and the layout.
	TArray<int32> Previous;
	TArray<FLayoutItem> Items;
	for (const FCandidate& Candidate : Shown)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Candidate.Index);
		const int32* Ring = PreviousRings.Find(Info->Id);
		Previous.Add(Ring ? *Ring : INDEX_NONE);
		Items.Add({Info->Location - CentreLocation, 0});
	}
	TArray<int32> Rings;
	AssignRings(Shown.Num(), NearCount, Previous, Rings);
	for (int32 Each = 0; Each < Items.Num(); ++Each)
	{
		Items[Each].Ring = Rings[Each];
	}
	TArray<FLayoutSlot> Slots;
	Layout(Items, Next.PlaneU, Next.PlaneV, Slots);

	// What each place holds: outposts, headquarters (the catalogue's and the fleet's), ships.
	TMap<int32, int32> OutpostsBySystem;
	TSet<int32> HeadquartersSystems;
	const auto SystemOfStructure = [Stars](const FGuid& SystemId, const AActor* Actor)
	{
		return SystemId.IsValid() ? Stars->IndexOf(SystemId) : Actor ? Stars->FindContaining(Actor->GetActorLocation()) : INDEX_NONE;
	};
	if (Infrastructure)
	{
		for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built.Type);
			const int32 At = Type ? SystemOfStructure(Built.SystemId, Built.Actor.Get()) : INDEX_NONE;
			if (At == INDEX_NONE)
			{
				continue;
			}
			if (Type->Category == APSInfrastructure::ECategory::Outpost)
			{
				++OutpostsBySystem.FindOrAdd(At);
			}
			if (Type->Visual == APSInfrastructure::EVisual::Headquarters)
			{
				HeadquartersSystems.Add(At);
			}
		}
	}
	if (Fleet)
	{
		for (const FAPSFleetBodyRecord& Record : Fleet->GetBodies())
		{
			for (const TWeakObjectPtr<AActor>& Outpost : Record.Outposts)
			{
				if (const int32 At = Outpost.IsValid() ? Stars->FindContaining(Outpost->GetActorLocation()) : INDEX_NONE; At != INDEX_NONE)
				{
					++OutpostsBySystem.FindOrAdd(At);
				}
			}
		}
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			if (Structure.Kind == APSFleet::EStructure::Headquarters && Structure.Actor.IsValid())
			{
				if (const int32 At = Stars->FindContaining(Structure.Actor->GetActorLocation()); At != INDEX_NONE)
				{
					HeadquartersSystems.Add(At);
				}
			}
		}
	}

	const AAstroGenerator* Generator = Stars->GetGenerator();
	const AStar* HomeStar = Generator ? Generator->HomeStar : nullptr;
	const auto MakeSystem = [&](const int32 Index) -> FSystem
	{
		const FAPSStarSystemInfo& Info = *Stars->Get(Index);
		const FAPSStarSystemState State = Stars->GetState(Info.Id);
		FSystem System;
		System.Index = Index;
		System.Id = Info.Id;
		System.Name = Info.Name;
		System.Spectral = Info.Spectral;
		System.StarCount = FMath::Max(Info.StarCount, 1);
		for (int32 Letter = 0; Letter < FMath::Min(System.StarCount, 4); ++Letter)
		{
			System.Designation.AppendChar(static_cast<TCHAR>(TEXT('A') + Letter));
		}
		System.Colour = MarkerColour(Info.Colour);
		const FClassLook Look = LookOf(Info.Spectral);
		System.Kind = Look.Kind;
		System.DiscRadius = Look.Disc;
		System.PickerRadius = 0.5f + 0.8f * Look.Disc;
		System.bGiant = Look.bGiant;
		System.bHome = Info.bHome;
		System.Knowledge = Info.bHome ? APSStars::EKnowledge::Surveyed : State.Knowledge;
		System.bClaimed = State.bClaimed || Info.bHome;
		System.bInReach = Stars->IsInReach(Info.Id);
		System.Anomaly = State.Anomaly;
		System.AnomalyKind = Stars->AnomalyKindOf(Info.Id);
		System.bMaterialized = Info.bHome || Index == MaterializedIndex;
		System.bColony = Info.bHome;
		System.bHeadquarters = Info.bHome || HeadquartersSystems.Contains(Index);
		System.Outposts = OutpostsBySystem.FindRef(Index);
		System.bInUse = InUse.Contains(Index);
		if (const TArray<int32>* Divisions = ShipsBySystem.Find(Index))
		{
			System.Ships = Divisions->Num();
			int32 Counts[static_cast<int32>(APSFleet::EDivision::Count)] = {};
			for (const int32 Division : *Divisions)
			{
				++Counts[FMath::Clamp(Division, 0, static_cast<int32>(APSFleet::EDivision::Count) - 1)];
			}
			int32 Most = 0;
			for (int32 Division = 1; Division < static_cast<int32>(APSFleet::EDivision::Count); ++Division)
			{
				Most = Counts[Division] > Counts[Most] ? Division : Most;
			}
			System.ShipsDivision = static_cast<APSFleet::EDivision>(Most);
		}
		// The worlds: counted and typed where the system stands (home, materialized), else the catalogue's "up to".
		if (System.bMaterialized)
		{
			TArray<APlanet*> Planets;
			if (Info.bHome)
			{
				for (TActorIterator<APlanet> It(World); It; ++It)
				{
					if (IsValid(*It) && HomeStar && It->ParentStar == HomeStar)
					{
						Planets.Add(*It);
					}
				}
			}
			else if (Materializer)
			{
				Materializer->GetPlanets(Planets);
			}
			Planets.RemoveAll([](const APlanet* Planet) { return !IsValid(Planet); });
			Planets.Sort([](const APlanet& A, const APlanet& B)
			{
				const FVector StarA = A.ParentStar ? A.ParentStar->GetActorLocation() : FVector::ZeroVector;
				const FVector StarB = B.ParentStar ? B.ParentStar->GetActorLocation() : FVector::ZeroVector;
				return FVector::DistSquared(A.GetActorLocation(), StarA) < FVector::DistSquared(B.GetActorLocation(), StarB);
			});
			System.Worlds = Planets.Num();
			System.bWorldsExact = true;
			if (System.Knowledge == APSStars::EKnowledge::Surveyed)
			{
				for (const APlanet* Planet : Planets)
				{
					System.WorldColours.Add(WorldColour(Planet->PlanetType));
					System.WorldTypes.Add(WorldTypeName(Planet->PlanetType));
				}
			}
		}
		else if (System.Knowledge != APSStars::EKnowledge::Catalogued)
		{
			System.Worlds = Info.PotentialPlanets;
		}
		return System;
	};

	{
		FSystem& Centre = Next.Systems.Add_GetRef(MakeSystem(CentreIndex));
		Centre.bCentre = true;
		Centre.Ring = INDEX_NONE;
		SnapshotIndex.Add(Centre.Id, 0);
	}
	RadiusByDistance.Add(TPair<double, double>(0.0, 0.0));
	for (int32 Each = 0; Each < Shown.Num(); ++Each)
	{
		FSystem System = MakeSystem(Shown[Each].Index);
		System.DistanceCm = Shown[Each].DistanceCm;
		System.Rank = Each + 1;
		System.Ring = Rings[Each];
		System.Slot = Slots[Each];
		SnapshotIndex.Add(System.Id, Next.Systems.Num());
		RadiusByDistance.Add(TPair<double, double>(System.DistanceCm,
			FMath::Max(System.Slot.Radius, RadiusByDistance.Last().Value)));
		Next.Systems.Add(MoveTemp(System));
	}
	PreviousRings.Reset();
	for (int32 Index = 1; Index < Next.Systems.Num(); ++Index)
	{
		PreviousRings.Add(Next.Systems[Index].Id, Next.Systems[Index].Ring);
	}

	// Routes and work, the relay network, the core.
	const auto Shown2Snapshot = [&](const int32 CatalogueIndex)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(CatalogueIndex);
		const int32* Found = Info ? SnapshotIndex.Find(Info->Id) : nullptr;
		return Found ? *Found : INDEX_NONE;
	};
	for (const FUnitRoute& UnitRoute : UnitRoutes)
	{
		const int32 To = Shown2Snapshot(UnitRoute.To);
		const int32 From = Shown2Snapshot(UnitRoute.From);
		if (To == INDEX_NONE)
		{
			continue;
		}
		FRoute& Route = Next.Routes.AddDefaulted_GetRef();
		Route.CallSign = UnitRoute.Unit->CallSign;
		Route.Division = UnitRoute.Unit->Division;
		Route.Order = UnitRoute.Unit->Order;
		Route.Ship = UnitRoute.Unit->Ship;
		Route.To = To;
		Route.From = From == INDEX_NONE || UnitRoute.Unit->Phase == APSFleet::EPhase::Working ? To : From;
		ReadRoute(*Fleet, *UnitRoute.Unit, Route);
	}
	TArray<TPair<int32, int32>> Network;
	Stars->GetNetwork(Network);
	for (const TPair<int32, int32>& Link : Network)
	{
		const int32 A = Shown2Snapshot(Link.Key);
		const int32 B = Shown2Snapshot(Link.Value);
		if (A != INDEX_NONE && B != INDEX_NONE && A != B)
		{
			Next.Links.Add({A, B});
		}
	}
	FVector CoreCentre = FVector::ZeroVector;
	double CoreRadius = 0.0;
	if (APSGalaxyGpuStars::GetIndexedBounds(World, CoreCentre, CoreRadius))
	{
		const FVector ToCore = CoreCentre - CentreLocation;
		const double X = FVector::DotProduct(ToCore, Next.PlaneU);
		const double Y = FVector::DotProduct(ToCore, Next.PlaneV);
		if (X * X + Y * Y > FMath::Square(APSStars::AstronomicalUnitCm))
		{
			Next.bCore = true;
			Next.CoreAngle = FMath::Atan2(Y, X);
		}
	}

	// Ring captions: the map's only scale, in the world's own unit (ShipNavigationComponent's formatter).
	bool bOuterKnownOnly = true;
	for (int32 Index = 1; Index < Next.Systems.Num(); ++Index)
	{
		const FSystem& System = Next.Systems[Index];
		FRing& Ring = Next.Rings[FMath::Clamp(System.Ring, 0, OuterRing)];
		Ring.NearCm = Ring.Count == 0 ? System.DistanceCm : FMath::Min(Ring.NearCm, System.DistanceCm);
		Ring.FarCm = FMath::Max(Ring.FarCm, System.DistanceCm);
		++Ring.Count;
		if (System.Ring == OuterRing && !System.bClaimed && System.Knowledge == APSStars::EKnowledge::Catalogued)
		{
			bOuterKnownOnly = false;
		}
	}
	for (int32 RingIndex = 0; RingIndex <= OuterRing; ++RingIndex)
	{
		FRing& Ring = Next.Rings[RingIndex];
		if (Ring.Count == 0)
		{
			continue;
		}
		const FText Count = APSUINumber::Number(Ring.Count);
		if (RingIndex == 0)
		{
			Ring.Caption = FText::Format(LOCTEXT("RingNearest", "{0} NEAREST  /  UP TO {1}"), Count, DistanceCaption(Ring.FarCm));
		}
		else if (RingIndex < OuterRing)
		{
			Ring.Caption = FText::Format(LOCTEXT("RingNext", "NEXT {0}  /  UP TO {1}"), Count, DistanceCaption(Ring.FarCm));
		}
		else
		{
			const FText Which = bOuterKnownOnly ? LOCTEXT("RingFartherKnown", "FARTHER, KNOWN ONLY") : LOCTEXT("RingFarther", "FARTHER");
			Ring.Caption = Ring.Count == 1 || Ring.FarCm <= Ring.NearCm * 1.001
				? FText::Format(LOCTEXT("RingFartherOne", "{0}  /  {1}"), Which, DistanceCaption(Ring.FarCm))
				: FText::Format(LOCTEXT("RingFartherRange", "{0}  /  {1} - {2}"), Which, DistanceCaption(Ring.NearCm), DistanceCaption(Ring.FarCm));
		}
	}

	// Counts for the title and the filters.
	for (int32 Index = 0; Index < Stars->Num(); ++Index)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		Next.CatalogueCount += Info && !Info->bInsideHome && Index != CentreIndex ? 1 : 0;
	}
	Next.KnownCount = Known.Num() - (Known.Contains(CentreIndex) ? 1 : 0);
	for (int32 Index = 1; Index < Next.Systems.Num(); ++Index)
	{
		for (int32 Filter = 0; Filter < 4; ++Filter)
		{
			Next.FilterCounts[Filter] += PassesFilter(Next.Systems[Index], static_cast<EFilter>(Filter)) ? 1 : 0;
		}
	}

	// The labels' cache follows what is drawn where, not the ships' progress.
	uint32 Hash = GetTypeHash(Next.CentreId);
	for (const FSystem& System : Next.Systems)
	{
		Hash = HashCombine(Hash, GetTypeHash(System.Id));
		Hash = HashCombine(Hash, GetTypeHash(FIntPoint(FMath::RoundToInt(System.Slot.Position.X * 1000.0),
			FMath::RoundToInt(System.Slot.Position.Y * 1000.0))));
		Hash = HashCombine(Hash, GetTypeHash(FIntVector4(System.Ring, static_cast<int32>(System.Knowledge) * 4 + (System.bClaimed ? 2 : 0)
			+ (System.bInUse ? 1 : 0), System.Worlds * 64 + System.Anomaly * 16 + FMath::Min(System.Outposts, 3) * 4
			+ (System.bHeadquarters ? 2 : 0) + (System.bMaterialized ? 1 : 0), System.Ships)));
	}
	for (const FRoute& Route : Next.Routes)
	{
		Hash = HashCombine(Hash, GetTypeHash(FIntPoint(Route.From, Route.To)));
	}
	for (const FLink& Link : Next.Links)
	{
		Hash = HashCombine(Hash, GetTypeHash(FIntPoint(Link.A, Link.B)));
	}
	Hash = HashCombine(Hash, GetTypeHash(FMath::RoundToInt(Next.CoreAngle * 1000.0) + (Next.bCore ? 1 : 0)));
	if (Hash != LayoutHash)
	{
		LayoutHash = Hash;
		++LayoutVersion;
		UE_LOG(LogTemp, Log, TEXT("[APS.StarMap] around %s: %d of %d systems (rings %d/%d/%d, outer %d), %d known, %d routes, %d links"),
			*Next.Systems[0].Name, Next.Systems.Num() - 1, Next.CatalogueCount, Next.Rings[0].Count, Next.Rings[1].Count, Next.Rings[2].Count,
			Next.Rings[3].Count, Next.KnownCount, Next.Routes.Num(), Next.Links.Num());
	}
	Snapshot = MoveTemp(Next);
}

void FAPSStarMapModel::UpdateLive(UWorld* World)
{
	using namespace APSStarMap;
	if (!Snapshot.bReady)
	{
		return;
	}
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	if (Fleet)
	{
		for (FRoute& Route : Snapshot.Routes)
		{
			if (const FAPSFleetUnit* Unit = Fleet->FindUnit(Route.Ship.Get()))
			{
				APSStarMapLocal::ReadRoute(*Fleet, *Unit, Route);
			}
		}
	}
	// The pilot: in a shown system, else between them.
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	Snapshot.bPilot = Pawn && Stars;
	Snapshot.PilotSystem = INDEX_NONE;
	if (Snapshot.bPilot)
	{
		FVector2D Position;
		int32 System = INDEX_NONE;
		LocateShip(World, Pawn, System, Position);
		Snapshot.PilotSystem = System;
		Snapshot.PilotPosition = Position;
	}
}

#undef LOCTEXT_NAMESPACE
