#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APSMegastructures.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

/**
 * Hubs and megastructures (Docs/Design/MEGASTRUCTURES.md): the chains hold together in the catalogue, every step's rule
 * refuses until the step before it stands, the world layout keeps the parts clear of the neighbours, the mesh fits put the
 * authored meshes where they belong, and the save format did not change. No world needed: APS.Megastructures.*
 */
namespace APSMegastructuresTestsLocal
{
	using namespace APSInfrastructure;

	const FType* Type(const TCHAR* Id)
	{
		return Find(FName(Id));
	}

	bool Contains(const FText& Text, const TCHAR* Part)
	{
		return Text.ToString().Contains(Part);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMegastructuresChainCatalogueTest, "APS.Megastructures.Chains.Catalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSMegastructuresChainCatalogueTest::RunTest(const FString& Parameters)
{
	using namespace APSMegastructuresTestsLocal;
	// The ids saves and missions name are all still there, every id once.
	for (const TCHAR* Id : {TEXT("OrbitalRing"), TEXT("SpaceElevator"), TEXT("DysonSwarm"), TEXT("JumpGate"), TEXT("SpaceHub"),
		TEXT("GrandHub"), TEXT("DysonSphere"), TEXT("MiningOutpost"), TEXT("AdministrationHub")})
	{
		TestNotNull(FString::Printf(TEXT("%s is in the catalogue"), Id), Type(Id));
	}
	TSet<FName> Ids;
	for (const FType& Each : Types())
	{
		TestFalse(FString::Printf(TEXT("%s appears once"), *Each.Id.ToString()), Ids.Contains(Each.Id));
		Ids.Add(Each.Id);
	}

	TArray<FName> Steps;
	TestTrue(TEXT("the elevator is in a chain"), GetChain(FName(TEXT("SpaceElevator")), Steps));
	TestTrue(TEXT("elevator > ring > swarm > sphere"), Steps
		== TArray<FName>{FName(TEXT("SpaceElevator")), FName(TEXT("OrbitalRing")), FName(TEXT("DysonSwarm")), FName(TEXT("DysonSphere"))});
	TArray<FName> Same;
	TestTrue(TEXT("the sphere finds the same chain"), GetChain(FName(TEXT("DysonSphere")), Same) && Same == Steps);
	TestTrue(TEXT("the hubs are a chain"), GetChain(FName(TEXT("GrandHub")), Steps));
	TestTrue(TEXT("hub > grand hub"), Steps == TArray<FName>{FName(TEXT("SpaceHub")), FName(TEXT("GrandHub"))});
	TestFalse(TEXT("a mining outpost is in no chain"), GetChain(FName(TEXT("MiningOutpost")), Steps));

	// Each step stands on the one before it; the first ones on a station over the world.
	for (const TArray<const TCHAR*>& Chain : {TArray<const TCHAR*>{TEXT("SpaceElevator"), TEXT("OrbitalRing"), TEXT("DysonSwarm"),
		TEXT("DysonSphere")}, TArray<const TCHAR*>{TEXT("SpaceHub"), TEXT("GrandHub")}})
	{
		for (int32 Index = 0; Index < Chain.Num(); ++Index)
		{
			const FType* Step = Type(Chain[Index]);
			if (!TestNotNull(Chain[Index], Step))
			{
				continue;
			}
			if (Index == 0)
			{
				TestTrue(FString::Printf(TEXT("%s needs a station over the world"), Chain[Index]), Step->bNeedsStationHere);
			}
			else
			{
				const FName Before(Chain[Index - 1]);
				TestTrue(FString::Printf(TEXT("%s stands on %s"), Chain[Index], Chain[Index - 1]),
					Step->RequiresAtSite == Before || Step->RequiresInSystem == Before);
			}
			// Raised by construction ships only, with a look of their own; never offered to build mode (orbit or surface).
			TestTrue(FString::Printf(TEXT("%s is fleet-only"), Chain[Index]), Step->bFleetOnly);
			TestTrue(FString::Printf(TEXT("%s has a megastructure look"), Chain[Index]), IsMegaVisual(Step->Visual));
			TestTrue(FString::Printf(TEXT("%s is not an orbit or surface type"), Chain[Index]),
				Step->Placement != EPlacement::Orbit && Step->Placement != EPlacement::Surface);
			TArray<FText> Lines;
			DescribeRequirements(*Step, Lines);
			TestTrue(FString::Printf(TEXT("%s says what it requires"), Chain[Index]), !Lines.IsEmpty());
		}
	}
	// The sphere wants five swarm rings at its star, and one star takes five.
	const FType* Sphere = Type(TEXT("DysonSphere"));
	const FType* Swarm = Type(TEXT("DysonSwarm"));
	if (TestNotNull(TEXT("sphere"), Sphere) && TestNotNull(TEXT("swarm"), Swarm))
	{
		TestEqual(TEXT("the sphere wants 5 segments"), Sphere->RequiresAtSiteCount, 5);
		TestTrue(TEXT("one star takes them"), Swarm->LimitPerSite >= Sphere->RequiresAtSiteCount);
		TestTrue(TEXT("both around the star"), Sphere->Placement == EPlacement::StarSystem && Swarm->Placement == EPlacement::StarSystem);
	}
	// Hubs are their own tier on the maps.
	const FType* Hub = Type(TEXT("SpaceHub"));
	if (TestNotNull(TEXT("hub"), Hub))
	{
		TestTrue(TEXT("a hub is a hub"), Hub->Category == ECategory::Hub);
		TestTrue(TEXT("a hub gives berths"), Hub->HubBerths > 0);
		TestTrue(TEXT("a hub speeds its system"), Hub->SystemWorkSpeed > 0.0f);
		TestTrue(TEXT("in a high orbit"), Hub->Placement == EPlacement::HighOrbit);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMegastructuresChainRulesTest, "APS.Megastructures.Chains.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSMegastructuresChainRulesTest::RunTest(const FString& Parameters)
{
	using namespace APSMegastructuresTestsLocal;
	const FType* Elevator = Type(TEXT("SpaceElevator"));
	const FType* Ring = Type(TEXT("OrbitalRing"));
	const FType* Swarm = Type(TEXT("DysonSwarm"));
	const FType* Sphere = Type(TEXT("DysonSphere"));
	const FType* Hub = Type(TEXT("SpaceHub"));
	const FType* GrandHub = Type(TEXT("GrandHub"));
	if (!TestNotNull(TEXT("elevator"), Elevator) || !TestNotNull(TEXT("ring"), Ring) || !TestNotNull(TEXT("swarm"), Swarm)
		|| !TestNotNull(TEXT("sphere"), Sphere) || !TestNotNull(TEXT("hub"), Hub) || !TestNotNull(TEXT("grand hub"), GrandHub))
	{
		return false;
	}
	FChainState Empty;
	FChainState Station;
	Station.bStationHere = true;
	// The elevator and the hub: refused without a station over the world, free with one.
	TestTrue(TEXT("no elevator without a station"), Contains(ChainRefusal(*Elevator, Empty), TEXT("station")));
	TestTrue(TEXT("an elevator over a station"), ChainRefusal(*Elevator, Station).IsEmpty());
	TestTrue(TEXT("no hub without a station"), !ChainRefusal(*Hub, Empty).IsEmpty());
	TestTrue(TEXT("a hub over a station"), ChainRefusal(*Hub, Station).IsEmpty());
	// The ring: refused until the elevator stands on that world; the station alone is not enough.
	const FText NoElevator = ChainRefusal(*Ring, Station);
	TestTrue(TEXT("the ring requires the elevator"), Contains(NoElevator, TEXT("Requires")) && Contains(NoElevator, *Elevator->Name.ToString()));
	FChainState WithElevator;
	WithElevator.RequiredHere = 1;
	TestTrue(TEXT("a ring over the elevator"), ChainRefusal(*Ring, WithElevator).IsEmpty());
	// The grand hub stands on a hub at the same world.
	TestTrue(TEXT("the grand hub requires a hub"), Contains(ChainRefusal(*GrandHub, Empty), *Hub->Name.ToString()));
	TestTrue(TEXT("a grand hub over a hub"), ChainRefusal(*GrandHub, WithElevator).IsEmpty());
	// The swarm wants a ring anywhere in its star system.
	TestTrue(TEXT("the swarm requires a ring in the system"), Contains(ChainRefusal(*Swarm, Empty), *Ring->Name.ToString()));
	FChainState RingInSystem;
	RingInSystem.RequiredInSystem = 1;
	TestTrue(TEXT("a swarm with a ring in the system"), ChainRefusal(*Swarm, RingInSystem).IsEmpty());
	// The sphere counts its segments.
	FChainState Four;
	Four.RequiredHere = 4;
	const FText Short = ChainRefusal(*Sphere, Four);
	TestTrue(TEXT("four segments are not enough"), Contains(Short, TEXT("5")) && Contains(Short, TEXT("4")));
	FChainState Five;
	Five.RequiredHere = 5;
	TestTrue(TEXT("five segments close the sphere"), ChainRefusal(*Sphere, Five).IsEmpty());

	// Berths: a hub takes more of the ordinary orbital stations at its world, never more hubs or megastructures.
	const FType* Research = Type(TEXT("ResearchStation"));
	if (TestNotNull(TEXT("research station"), Research))
	{
		const TArray<FName> HubHere = {FName(TEXT("SpaceHub"))};
		TestEqual(TEXT("a hub gives a station its berths"), HubBerthsFor(*Research, HubHere), Hub->HubBerths);
		TestEqual(TEXT("both hubs add up"), HubBerthsFor(*Research, {FName(TEXT("SpaceHub")), FName(TEXT("GrandHub"))}),
			Hub->HubBerths + GrandHub->HubBerths);
		TestEqual(TEXT("no berths without a hub"), HubBerthsFor(*Research, {FName(TEXT("ResearchStation"))}), 0);
		TestEqual(TEXT("no extra hubs"), HubBerthsFor(*Hub, HubHere), 0);
		TestEqual(TEXT("no extra elevators"), HubBerthsFor(*Elevator, HubHere), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMegastructuresLayoutTest, "APS.Megastructures.Geometry.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSMegastructuresLayoutTest::RunTest(const FString& Parameters)
{
	using namespace APSMegastructures;
	// Kepler without the radius: Earth's stationary orbit is 6.62 radii, Mars's 6.03.
	TestTrue(TEXT("Earth 6.62 radii"), FMath::IsNearlyEqual(StationaryOrbitRatio(5.51, 23.934), 6.62, 0.05));
	TestTrue(TEXT("Mars 6.03 radii"), FMath::IsNearlyEqual(StationaryOrbitRatio(3.93, 24.623), 6.03, 0.1));
	TestTrue(TEXT("a longer day lifts it"), StationaryOrbitRatio(5.5, 36.0) > StationaryOrbitRatio(5.5, 18.0));

	const double Radius = 200000000.0;
	// Nothing near: the ring at the hand-made level's 1.884 radii, the counterweight at the stationary orbit above it.
	const FWorldLayout Free = ComputeLayout(Radius, 6.6, 0.0);
	TestTrue(TEXT("free: the ring fits"), Free.bRingFits);
	TestTrue(TEXT("free: ring at 1.884 radii"), FMath::IsNearlyEqual(Free.RingRadiusCm / Radius, Assets::RingRadiusRatio, 1.0e-6));
	TestTrue(TEXT("free: the elevator fits"), Free.bElevatorFits);
	TestTrue(TEXT("free: counterweight at 6.6 radii"), FMath::IsNearlyEqual(Free.CounterweightRadiusCm / Radius, 6.6, 1.0e-6));
	TestTrue(TEXT("free: counterweight over the ring"), Free.CounterweightRadiusCm > Free.RingRadiusCm * 1.19);
	TestTrue(TEXT("free: hubs fit above the air"), Free.bHubFits && Free.HubRadiusCm > Radius * 1.15);
	// A moon whose surface comes within 4 radii: everything stays under it.
	const FWorldLayout Moon = ComputeLayout(Radius, 6.6, Radius * 4.0);
	TestTrue(TEXT("moon at 4 R: ring fits"), Moon.bRingFits);
	TestTrue(TEXT("moon at 4 R: counterweight below 3 radii"), Moon.bElevatorFits && Moon.CounterweightRadiusCm <= Radius * 3.0 + 1.0);
	TestTrue(TEXT("moon at 4 R: hub below it"), Moon.HubRadiusCm <= Radius * 3.0 + 1.0);
	// Closer: room for a ring (at 1.6) but not for a stationary orbit above it.
	const FWorldLayout Tight = ComputeLayout(Radius, 6.6, Radius * 2.0);
	TestTrue(TEXT("moon at 2 R: ring at 1.6"), Tight.bRingFits && FMath::IsNearlyEqual(Tight.RingRadiusCm / Radius, 1.6, 1.0e-6));
	TestFalse(TEXT("moon at 2 R: no elevator"), Tight.bElevatorFits);
	// Closer still: no ring either.
	TestFalse(TEXT("moon at 1.5 R: no ring"), ComputeLayout(Radius, 6.6, Radius * 1.5).bRingFits);
	// A slow-spinning, light world: the counterweight never beyond 9 radii.
	TestTrue(TEXT("capped at 9 radii"), ComputeLayout(Radius, 20.0, 0.0).CounterweightRadiusCm <= Radius * 9.0 + 1.0);

	// Construction steps.
	TestEqual(TEXT("nothing before the work"), StageOf(0.0f, 48), 0);
	TestEqual(TEXT("the first segment at once"), StageOf(0.001f, 48), 1);
	TestEqual(TEXT("all at the end"), StageOf(1.0f, 48), 48);
	TestEqual(TEXT("half way"), StageOf(0.5f, 48), 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMegastructuresFitTest, "APS.Megastructures.Geometry.Fits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSMegastructuresFitTest::RunTest(const FString& Parameters)
{
	using namespace APSMegastructures;
	// Every mesh axis onto every world axis: a proper rotation, the next axis toward the reference.
	const FVector Worlds[] = {FVector(0.3, -0.5, 0.81).GetSafeNormal(), FVector::UpVector, FVector::ForwardVector};
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (const FVector& World : Worlds)
		{
			const FVector Reference(0.2, 0.9, -0.1);
			const FQuat Rotation = AxisRotation(Axis, World, Reference);
			FVector MeshAxis = FVector::ZeroVector;
			MeshAxis[Axis] = 1.0;
			FVector Next = FVector::ZeroVector;
			Next[(Axis + 1) % 3] = 1.0;
			TestTrue(FString::Printf(TEXT("axis %d lands on the world axis"), Axis), Rotation.RotateVector(MeshAxis).Equals(World, 1.0e-6));
			TestTrue(FString::Printf(TEXT("axis %d's next one toward the reference"), Axis),
				FVector::DotProduct(Rotation.RotateVector(Next), FVector::VectorPlaneProject(Reference, World).GetSafeNormal()) > 0.999999);
			TestTrue(FString::Printf(TEXT("axis %d: a unit rotation"), Axis), FMath::IsNearlyEqual(Rotation.Size(), 1.0, 1.0e-9));
		}
	}

	// The authored ring (a hoop 10 cm thick round X, 60 cm across, its pivot under it) round a world 3,770 km out.
	const FVector RingOrigin(0.0, 0.15, 30.0);
	const FVector RingExtent(5.05, 29.85, 29.95);
	const FVector Centre(1.0e9, -2.0e8, 5.0e7);
	const FVector North = FVector(0.1, 0.2, 0.97).GetSafeNormal();
	const double Radius = 3.768e8;
	const FTransform Ring = FitRing(RingOrigin, RingExtent, Centre, North, FVector::ForwardVector, Radius, Assets::RingThicknessRatio);
	TestTrue(TEXT("the hoop's centre on the world's"), Ring.TransformPosition(RingOrigin).Equals(Centre, 1.0));
	const FVector Rim = Ring.TransformPosition(RingOrigin + FVector(0.0, 0.0, RingExtent.Z));
	TestTrue(TEXT("its rim at the ring's radius"), FMath::IsNearlyEqual(FVector::Dist(Rim, Centre), Radius, Radius * 1.0e-6));
	TestTrue(TEXT("its rim in the equatorial plane"), FMath::Abs(FVector::DotProduct(Rim - Centre, North)) < Radius * 1.0e-6);
	const FVector Face = Ring.TransformPosition(RingOrigin + FVector(RingExtent.X, 0.0, 0.0)) - Centre;
	TestTrue(TEXT("its thickness along the axis, as authored"), Face.GetSafeNormal().Equals(North, 1.0e-6)
		&& FMath::IsNearlyEqual(Face.Size(), RingExtent.X * Radius / RingExtent.Z * Assets::RingThicknessRatio, 10.0));

	// A cable from the tower's collar to the counterweight.
	const FVector From(0.0, 0.0, 540000.0);
	const FVector To(0.0, 0.0, 1.1e9);
	const FTransform Cable = FitSpan(FVector::ZeroVector, FVector(50.0), 2, From, To, FVector::ForwardVector, 240000.0);
	TestTrue(TEXT("the cable starts at the collar"), Cable.TransformPosition(FVector(0.0, 0.0, -50.0)).Equals(From, 1.0));
	TestTrue(TEXT("and ends at the counterweight"), Cable.TransformPosition(FVector(0.0, 0.0, 50.0)).Equals(To, 1.0));
	TestTrue(TEXT("240 m thick"), FMath::IsNearlyEqual(FVector::Dist(Cable.TransformPosition(FVector(-50.0, 0.0, 0.0)),
		Cable.TransformPosition(FVector(50.0, 0.0, 0.0))), 240000.0, 1.0));

	// The tower stands on its base; the hub sits on its centre with its spindle toward the world.
	const FVector TowerOrigin(0.0, 0.0, 29.85);
	const FVector TowerExtent(13.8, 13.8, 29.95);
	const FTransform Tower = FitStanding(TowerOrigin, TowerExtent, FVector::ZeroVector, FVector::UpVector, FVector::ForwardVector, 10000.0);
	TestTrue(TEXT("the tower's foot on the ground"), Tower.TransformPosition(FVector(0.0, 0.0, -0.1)).Equals(FVector::ZeroVector, 1.0));
	TestTrue(TEXT("6 km tall"), FMath::IsNearlyEqual(Tower.TransformPosition(FVector(0.0, 0.0, 59.8)).Z, 599000.0, 1.0));
	const FTransform Hub = FitCentred(FVector(0.0, 0.0, 28.2), FVector(57.6, 57.6, 28.3), FVector::ZeroVector, -FVector::UpVector,
		FVector::ForwardVector, 10000.0);
	TestTrue(TEXT("the hub centred"), Hub.TransformPosition(FVector(0.0, 0.0, 28.2)).Equals(FVector::ZeroVector, 1.0));
	TestTrue(TEXT("its spindle (mesh +Z) toward the world"), Hub.TransformPosition(FVector(0.0, 0.0, 56.5)).Z < -250000.0);

	// Ring points: on the circle, in its plane, from the start both ways, none twice.
	TArray<FVector> Points;
	RingPoints(Centre, North, Centre + FVector::ForwardVector * 10.0, Radius, 48, Points);
	TestEqual(TEXT("48 points"), Points.Num(), 48);
	const FVector First = FVector::VectorPlaneProject(FVector::ForwardVector, North).GetSafeNormal();
	TestTrue(TEXT("the first toward the start"), ((Points[0] - Centre).GetSafeNormal()).Equals(First, 1.0e-6));
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		TestTrue(TEXT("on the circle"), FMath::IsNearlyEqual(FVector::Dist(Points[Index], Centre), Radius, Radius * 1.0e-6));
		TestTrue(TEXT("in the plane"), FMath::Abs(FVector::DotProduct(Points[Index] - Centre, North)) < Radius * 1.0e-6);
		for (int32 Other = 0; Other < Index; ++Other)
		{
			TestTrue(TEXT("no point twice"), FVector::Dist(Points[Index], Points[Other]) > Radius * 0.05);
		}
	}
	TestTrue(TEXT("the second and third either side of the first"),
		FMath::IsNearlyEqual(FVector::Dist(Points[1], Points[0]), FVector::Dist(Points[2], Points[0]), Radius * 1.0e-6));

	// The equator's direction: off the reference's latitude, turned east by the offset.
	const FVector Up = FVector::UpVector;
	TestTrue(TEXT("on the equator"), FMath::Abs(EquatorDirection(Up, FVector(1.0, 0.0, 0.8), 0.0, FVector::ForwardVector).Z) < 1.0e-9);
	TestTrue(TEXT("5 degrees east"), FMath::IsNearlyEqual(FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(
		EquatorDirection(Up, FVector::ForwardVector, 5.0, FVector::ForwardVector), FVector::ForwardVector))), 5.0, 1.0e-6));
	TestTrue(TEXT("a reference along the axis falls back"),
		EquatorDirection(Up, Up, 0.0, FVector::RightVector).Equals(FVector::RightVector, 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMegastructuresSaveTest, "APS.Megastructures.SaveFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSMegastructuresSaveTest::RunTest(const FString& Parameters)
{
	// Nothing new is saved: a hub, an elevator, a ring and a swarm go through the infrastructure's version 2 blob as any
	// structure (type id, site, actor name, the root's transform), and come back the same.
	FAPSInfrastructureSaveData Saved;
	const TCHAR* Ids[] = {TEXT("SpaceHub"), TEXT("SpaceElevator"), TEXT("OrbitalRing"), TEXT("DysonSwarm")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ids); ++Index)
	{
		FAPSInfrastructureSaveData::FStructure& Structure = Saved.Structures.AddDefaulted_GetRef();
		Structure.Type = FName(Ids[Index]);
		Structure.SiteKey = Index == 3 ? TEXT("SYSTEM:0123") : TEXT("HOME-A");
		Structure.ActorName = FString::Printf(TEXT("APS_Infra_%s_%d"), Ids[Index], Index + 1);
		Structure.RelativeTransform = FTransform(FQuat(FVector::UpVector, 0.3 * Index), FVector(2.0e8 + Index, -3.0e7, 1.5e8), FVector::OneVector);
		Structure.BuiltSeconds = 100.0 * Index;
	}
	Saved.Stocks = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	Writer << Saved;
	TestEqual(TEXT("still version 2"), Bytes.Num() > 0 ? static_cast<int32>(Bytes[0]) : -1, 2);
	FAPSInfrastructureSaveData Loaded;
	FMemoryReader Reader(Bytes);
	Reader << Loaded;
	TestFalse(TEXT("reads back"), Reader.IsError());
	if (TestEqual(TEXT("four structures"), Loaded.Structures.Num(), 4))
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			TestTrue(TEXT("type"), Loaded.Structures[Index].Type == Saved.Structures[Index].Type);
			TestEqual(TEXT("site"), Loaded.Structures[Index].SiteKey, Saved.Structures[Index].SiteKey);
			TestTrue(TEXT("root"), Loaded.Structures[Index].RelativeTransform.Equals(Saved.Structures[Index].RelativeTransform, 1.0e-6));
		}
	}
	return true;
}

#endif
