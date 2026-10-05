#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/UI/Colony/APSStarMapModel.h"
#include "Math/RandomStream.h"

namespace APSStarMapTests
{
	constexpr double TwoPi = 6.28318530717958647692;
	constexpr double HalfPi = 1.57079632679489661923;
	/** One light year in astronomical units: the real-scale map multiplies the compressed distances by about this. */
	constexpr double LightYearInAu = 63241.0;

	double Wrap(const double Angle)
	{
		const double Wrapped = FMath::Fmod(Angle, TwoPi);
		return Wrapped < 0.0 ? Wrapped + TwoPi : Wrapped;
	}

	/** A cluster's neighbours: random directions, 1.5 to 20 AU, nearest first, with their rank rings. */
	void MakeNeighbours(const int32 Seed, const int32 Count, TArray<APSStarMap::FLayoutItem>& OutItems)
	{
		FRandomStream Stream(Seed);
		TArray<FVector> Offsets;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Offsets.Add(Stream.VRand() * Stream.FRandRange(1.5, 20.0) * APSStars::AstronomicalUnitCm);
		}
		Offsets.Sort([](const FVector& A, const FVector& B) { return A.SizeSquared() < B.SizeSquared(); });
		TArray<int32> Rings;
		APSStarMap::AssignRings(Count, FMath::Min(Count, APSStarMap::NearestCount), TArray<int32>(), Rings);
		OutItems.Reset();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			OutItems.Add({Offsets[Index], Rings[Index]});
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStarMapLayoutScaleTest,
	"APS.UI.StarMap.LayoutUnchangedByScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStarMapLayoutScaleTest::RunTest(const FString& Parameters)
{
	using namespace APSStarMapTests;
	// Rio 05.10: the map draws ranks and directions, not distances, so the compressed (AU) world and the real-scale one
	// (light years) give the same picture; only the rings' captions change.
	const FVector TiltedU = FVector(1.0, 0.3, 0.2).GetSafeNormal();
	const FVector TiltedNormal = FVector::CrossProduct(TiltedU, FVector(0.1, 0.2, 1.0)).GetSafeNormal();
	const FVector TiltedV = FVector::CrossProduct(TiltedNormal, TiltedU).GetSafeNormal();
	const TPair<FVector, FVector> Planes[] = {TPair<FVector, FVector>(FVector::ForwardVector, FVector::RightVector),
		TPair<FVector, FVector>(TiltedU, TiltedV)};
	for (const int32 Seed : {7, 20261005, 31337})
	{
		TArray<APSStarMap::FLayoutItem> Items;
		MakeNeighbours(Seed, 60, Items);
		TArray<APSStarMap::FLayoutItem> Scaled = Items;
		for (APSStarMap::FLayoutItem& Item : Scaled)
		{
			Item.Offset *= LightYearInAu;
		}
		for (const TPair<FVector, FVector>& Plane : Planes)
		{
			TArray<APSStarMap::FLayoutSlot> Compressed;
			TArray<APSStarMap::FLayoutSlot> Real;
			APSStarMap::Layout(Items, Plane.Key, Plane.Value, Compressed);
			APSStarMap::Layout(Scaled, Plane.Key, Plane.Value, Real);
			TestEqual(TEXT("Every system has its place"), Real.Num(), Compressed.Num());
			double Worst = 0.0;
			for (int32 Index = 0; Index < Compressed.Num() && Index < Real.Num(); ++Index)
			{
				Worst = FMath::Max(Worst, FVector2D::Distance(Compressed[Index].Position, Real[Index].Position));
				Worst = FMath::Max(Worst, FMath::Abs(Compressed[Index].Radius - Real[Index].Radius));
				Worst = FMath::Max(Worst, FMath::Abs(Compressed[Index].Elevation - Real[Index].Elevation));
			}
			TestTrue(FString::Printf(TEXT("The layout is the same at AU and light-year distances (seed %d, worst %.3g)"), Seed, Worst),
				Worst < 1.0e-7);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStarMapRingsTest,
	"APS.UI.StarMap.Rings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStarMapRingsTest::RunTest(const FString& Parameters)
{
	using namespace APSStarMapTests;
	// Ring membership by rank: 6 nearest, the next 10, the next 9, the rest outside.
	TArray<int32> Rings;
	APSStarMap::AssignRings(30, 25, TArray<int32>(), Rings);
	int32 Counts[4] = {0, 0, 0, 0};
	for (const int32 Ring : Rings)
	{
		++Counts[FMath::Clamp(Ring, 0, 3)];
	}
	TestEqual(TEXT("6 on the first ring"), Counts[0], 6);
	TestEqual(TEXT("10 on the second"), Counts[1], 10);
	TestEqual(TEXT("9 on the third"), Counts[2], 9);
	TestEqual(TEXT("The rest on the outer ring"), Counts[3], 5);
	for (int32 Rank = 1; Rank < Rings.Num(); ++Rank)
	{
		TestTrue(TEXT("Rings follow the rank"), Rings[Rank] >= Rings[Rank - 1]);
	}

	// Hysteresis: a newcomer at rank 2 pushes everyone after it one place out; nobody changes ring for that.
	TArray<int32> Previous;
	Previous.Init(INDEX_NONE, 25);
	for (int32 Rank = 3; Rank < 25; ++Rank)
	{
		Previous[Rank] = Rings[Rank - 1];
	}
	TArray<int32> After;
	APSStarMap::AssignRings(25, 25, Previous, After);
	TestEqual(TEXT("The sixth nearest, pushed to seventh, keeps the first ring"), After[6], 0);
	TestEqual(TEXT("The sixteenth, pushed to seventeenth, keeps the second ring"), After[16], 1);
	TestEqual(TEXT("A newcomer takes its rank's ring"), After[2], 0);
	// Two places past the edge it moves on.
	Previous.Init(INDEX_NONE, 25);
	Previous[7] = 0;
	APSStarMap::AssignRings(25, 25, Previous, After);
	TestEqual(TEXT("Two places past its ring a system takes the next one"), After[7], 1);

	// Spreading along the rings: the order of the real directions is kept, neighbours are spaced, the captions' band is free.
	TArray<APSStarMap::FLayoutItem> Items;
	MakeNeighbours(20261005, 60, Items);
	TArray<APSStarMap::FLayoutSlot> Slots;
	APSStarMap::Layout(Items, FVector::ForwardVector, FVector::RightVector, Slots);
	for (int32 Ring = 0; Ring <= APSStarMap::OuterRing; ++Ring)
	{
		TArray<int32> Members;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index].Ring == Ring)
			{
				Members.Add(Index);
			}
		}
		if (Members.IsEmpty())
		{
			continue;
		}
		TArray<int32> ByBearing = Members;
		ByBearing.StableSort([&Slots](const int32 A, const int32 B)
		{
			return Wrap(Slots[A].Bearing - HalfPi) < Wrap(Slots[B].Bearing - HalfPi);
		});
		TArray<int32> ByAngle = Members;
		ByAngle.StableSort([&Slots](const int32 A, const int32 B)
		{
			return Wrap(Slots[A].Angle - HalfPi) < Wrap(Slots[B].Angle - HalfPi);
		});
		TestTrue(FString::Printf(TEXT("Ring %d keeps the stars' order"), Ring), ByBearing == ByAngle);
		const double Separation = APSStarMap::RingSeparation(Ring, Members.Num());
		const double Band = APSStarMap::CaptionBand(Ring);
		bool bSpaced = true;
		for (int32 Each = 1; Each < ByAngle.Num(); ++Each)
		{
			bSpaced &= Wrap(Slots[ByAngle[Each]].Angle - HalfPi) - Wrap(Slots[ByAngle[Each - 1]].Angle - HalfPi) >= Separation - 1.0e-9;
		}
		TestTrue(FString::Printf(TEXT("Ring %d: neighbours at least %.3f rad apart"), Ring, Separation), bSpaced);
		TestTrue(FString::Printf(TEXT("Ring %d: the caption's band at the top is free"), Ring),
			Wrap(Slots[ByAngle[0]].Angle - HalfPi) >= Band - 1.0e-9 && Wrap(Slots[ByAngle.Last()].Angle - HalfPi) <= TwoPi - Band + 1.0e-9);
		for (const int32 Member : Members)
		{
			const double Off = FMath::Abs(Slots[Member].Radius - APSStarMap::RingRadius[Ring]);
			TestTrue(TEXT("A star stays by its ring"), Off <= APSStarMap::RingSpread * 0.5 + 1.0e-9);
			for (const int32 Other : Members)
			{
				if (Items[Other].Offset.SizeSquared() > Items[Member].Offset.SizeSquared())
				{
					TestTrue(TEXT("A farther star of the ring is not drawn inside a nearer one"), Slots[Other].Radius >= Slots[Member].Radius - 1.0e-12);
				}
			}
		}
	}
	// Above the plane: the elevation's sine.
	TArray<APSStarMap::FLayoutSlot> Up;
	APSStarMap::Layout({{FVector(0.0, 0.0, 5.0), 0}, {FVector(3.0, 0.0, -3.0), 0}}, FVector::ForwardVector, FVector::RightVector, Up);
	TestTrue(TEXT("Straight above the plane"), FMath::IsNearlyEqual(Up[0].Elevation, 1.0, 1.0e-9));
	TestTrue(TEXT("45 degrees below it"), FMath::IsNearlyEqual(Up[1].Elevation, -FMath::Sqrt(0.5), 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStarMapLimitsTest,
	"APS.UI.StarMap.Limits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStarMapLimitsTest::RunTest(const FString& Parameters)
{
	using APSStarMap::FCandidate;
	// 25 nearest, then the extras: 2 fleet's, 1 pick, 10 claimed, 40 known; at most 60.
	TArray<FCandidate> Near;
	for (int32 Index = 0; Index < 25; ++Index)
	{
		Near.Add({Index, 1.0 + Index * 0.1, 4});
	}
	TArray<FCandidate> Extras;
	int32 Next = 100;
	Extras.Add({Next++, 90.0, 1});
	Extras.Add({Next++, 95.0, 1});
	Extras.Add({Next++, 99.0, 0});
	for (int32 Each = 0; Each < 10; ++Each)
	{
		Extras.Add({Next++, 50.0 + Each, 2});
	}
	for (int32 Each = 0; Each < 40; ++Each)
	{
		Extras.Add({Next++, 10.0 + Each, 3});
	}
	TArray<FCandidate> Shown;
	int32 NearCount = 0;
	APSStarMap::SelectShown(Near, Extras, APSStarMap::MaxShown, Shown, NearCount);
	TestEqual(TEXT("The 25 nearest make the rank rings"), NearCount, 25);
	TestEqual(TEXT("At most 60 shown"), Shown.Num(), APSStarMap::MaxShown);
	const auto Has = [&Shown](const int32 Index) { return Shown.ContainsByPredicate([Index](const FCandidate& C) { return C.Index == Index; }); };
	TestTrue(TEXT("The fleet's systems stay"), Has(100) && Has(101));
	TestTrue(TEXT("The pick stays"), Has(102));
	int32 Claimed = 0;
	int32 Known = 0;
	for (const FCandidate& Candidate : Shown)
	{
		Claimed += Candidate.Priority == 2 ? 1 : 0;
		Known += Candidate.Priority == 3 ? 1 : 0;
	}
	TestEqual(TEXT("Every claimed system stays"), Claimed, 10);
	TestEqual(TEXT("The known fill the rest"), Known, 60 - 25 - 3 - 10);
	TestTrue(TEXT("The nearer known ones stay"), Has(113) && !Has(152));
	bool bOrdered = true;
	for (int32 Index = 1; Index < Shown.Num(); ++Index)
	{
		bOrdered &= Index == NearCount || Shown[Index].DistanceCm >= Shown[Index - 1].DistanceCm;
	}
	TestTrue(TEXT("Nearest first, the outer ring's after the rank rings"), bOrdered);

	// The fleet's and the pick are never dropped, even past the limit.
	TArray<FCandidate> Busy;
	for (int32 Each = 0; Each < 40; ++Each)
	{
		Busy.Add({200 + Each, 30.0 + Each, 1});
	}
	APSStarMap::SelectShown(Near, Busy, APSStarMap::MaxShown, Shown, NearCount);
	TestEqual(TEXT("Every system in use stays"), Shown.Num(), 65);

	// The chips' times.
	TestEqual(TEXT("Seconds"), APSStarMap::FormatEta(38.0), FString(TEXT("0:38")));
	TestEqual(TEXT("Minutes"), APSStarMap::FormatEta(125.2), FString(TEXT("2:06")));
	TestEqual(TEXT("Hours"), APSStarMap::FormatEta(3725.0), FString(TEXT("1:02:05")));
	TestTrue(TEXT("Unknown"), APSStarMap::FormatEta(-1.0).IsEmpty());
	return true;
}

#endif
