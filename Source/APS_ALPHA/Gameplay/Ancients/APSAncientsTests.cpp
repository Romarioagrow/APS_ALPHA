#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APSAncientsGeometry.h"
#include "APSAncientsQuests.h"
#include "APSAncientsSites.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

/**
 * Ancient structures (Docs/Design/ANCIENT_STRUCTURES.md): the shapes face the engine's way and stand within what they
 * measure, everything follows from the seed alone, and the chains sit on the mission board without disturbing it.
 * No world needed: APS.Ancients.*
 */
namespace APSAncientsTestsLocal
{
	using namespace APSAncients;

	struct FFlatGround final : FAPSAncientGround
	{
		virtual double Z(const double, const double) const override
		{
			return 0.0;
		}
	};

	/** Rolling ground (a few tens of metres over kilometres): the parts must be seated on it one by one. */
	struct FHillyGround final : FAPSAncientGround
	{
		virtual double Z(const double X, const double Y) const override
		{
			return 2000.0 * FMath::Sin(X / 50000.0) + 1500.0 * FMath::Cos(Y / 37000.0);
		}
	};

	FSiteSpec MakeSpec(const EKind Kind, const uint32 Seed, const float Scale = 1.0f)
	{
		FSiteSpec Spec;
		Spec.Id = TEXT("TEST");
		Spec.Kind = Kind;
		Spec.Seed = Seed;
		Spec.Scale = Scale;
		Spec.Size = Kind == EKind::StoneCircle ? ESize::Small : ESize::Monumental;
		return Spec;
	}

	/** Triangles whose winding disagrees with their normals by the engine's front-face rule. */
	int32 CountMisfacing(const FAPSAncientMesh& Mesh, int32& OutDegenerate)
	{
		int32 Misfacing = 0;
		OutDegenerate = 0;
		for (int32 Index = 0; Index + 2 < Mesh.Triangles.Num(); Index += 3)
		{
			const FVector& A = Mesh.Vertices[Mesh.Triangles[Index]];
			const FVector& B = Mesh.Vertices[Mesh.Triangles[Index + 1]];
			const FVector& C = Mesh.Vertices[Mesh.Triangles[Index + 2]];
			const FVector Facing = FVector::CrossProduct(B - A, C - A);
			if (Facing.SizeSquared() < 1.0e-6)
			{
				++OutDegenerate;
				continue;
			}
			const FVector Normal = Mesh.Normals[Mesh.Triangles[Index]] + Mesh.Normals[Mesh.Triangles[Index + 1]]
				+ Mesh.Normals[Mesh.Triangles[Index + 2]];
			Misfacing += FVector::DotProduct(Facing, Normal) > 0.0 ? 1 : 0;
		}
		return Misfacing;
	}

	bool IsWellFormed(const FAPSAncientMesh& Mesh)
	{
		if (Mesh.Triangles.Num() % 3 != 0 || Mesh.Normals.Num() != Mesh.Vertices.Num() || Mesh.UVs.Num() != Mesh.Vertices.Num()
			|| Mesh.Tangents.Num() != Mesh.Vertices.Num())
		{
			return false;
		}
		for (const int32 Index : Mesh.Triangles)
		{
			if (!Mesh.Vertices.IsValidIndex(Index))
			{
				return false;
			}
		}
		for (const FVector& Vertex : Mesh.Vertices)
		{
			if (Vertex.ContainsNaN())
			{
				return false;
			}
		}
		return true;
	}

	FBox BoundsOf(const FAPSAncientMesh& Mesh)
	{
		return Mesh.Vertices.IsEmpty() ? FBox(ForceInit) : FBox(Mesh.Vertices);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAncientsGeometryTest, "APS.Ancients.Geometry.Shapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAncientsGeometryTest::RunTest(const FString& Parameters)
{
	using namespace APSAncientsTestsLocal;
	const FFlatGround Flat;
	const FHillyGround Hills;
	for (int32 KindIndex = 0; KindIndex < static_cast<int32>(EKind::Count); ++KindIndex)
	{
		const EKind Kind = static_cast<EKind>(KindIndex);
		for (const uint32 Seed : {1u, 77u, 4242u, 900001u, 0xdeadbeefu})
		{
			const FSiteSpec Spec = MakeSpec(Kind, Seed);
			const FString What = FString::Printf(TEXT("%s seed %u"), KindLabel(Kind), Seed);
			FAPSAncientShape Measure;
			APSAncientsGeometry::Build(Spec, nullptr, Measure);
			FAPSAncientShape Shape;
			APSAncientsGeometry::Build(Spec, &Flat, Shape);
			FAPSAncientShape Again;
			APSAncientsGeometry::Build(Spec, &Flat, Again);
			FAPSAncientShape Rolling;
			APSAncientsGeometry::Build(Spec, &Hills, Rolling);

			TestTrue(What + TEXT(": stone"), !Shape.Stone.IsEmpty() && IsWellFormed(Shape.Stone) && IsWellFormed(Rolling.Stone));
			TestTrue(What + TEXT(": glow"), (!Shape.Glow.IsEmpty() || !Shape.Beacon.IsEmpty()) && IsWellFormed(Shape.Glow)
				&& IsWellFormed(Shape.Beacon));
			TestTrue(What + TEXT(": a few thousand triangles at most"), Shape.Stone.NumTriangles() + Shape.Glow.NumTriangles()
				+ Shape.Beacon.NumTriangles() < 20000);
			int32 Degenerate = 0;
			TestEqual(What + TEXT(": stone faces outward"), CountMisfacing(Shape.Stone, Degenerate), 0);
			TestEqual(What + TEXT(": glow faces outward"), CountMisfacing(Shape.Glow, Degenerate), 0);
			TestEqual(What + TEXT(": beacon faces outward"), CountMisfacing(Shape.Beacon, Degenerate), 0);
			// The site search uses the measure: it must be the build's own.
			TestEqual(What + TEXT(": measured footprint"), Measure.Metrics.FootprintCm, Shape.Metrics.FootprintCm);
			TestEqual(What + TEXT(": measured height"), Measure.Metrics.HeightCm, Shape.Metrics.HeightCm);
			TestEqual(What + TEXT(": measured navigation point"), Measure.Metrics.NavHeightCm, Shape.Metrics.NavHeightCm);
			TestTrue(What + TEXT(": the same seed builds the same shape"), Shape.Stone.Vertices == Again.Stone.Vertices
				&& Shape.Glow.Vertices == Again.Glow.Vertices);
			TestTrue(What + TEXT(": the ground moves parts, never the draws"), Shape.Stone.NumTriangles() == Rolling.Stone.NumTriangles()
				|| Kind == EKind::BrokenRing);
			TestTrue(What + TEXT(": metrics"), Shape.Metrics.HeightCm > 0.0 && Shape.Metrics.FootprintCm > 0.0 && Shape.Metrics.SizeCm > 0.0
				&& Shape.Metrics.NavHeightCm > Shape.Metrics.HeightCm);

			const FBox Stone = BoundsOf(Shape.Stone);
			if (Kind == EKind::Derelict)
			{
				// Along X, centred on the hull, about its length.
				TestTrue(What + TEXT(": hull along X"), Stone.Max.X <= Shape.Metrics.SizeCm * 0.62 && Stone.Min.X >= -Shape.Metrics.SizeCm * 0.62
					&& Stone.GetExtent().X > Stone.GetExtent().Y && Stone.GetExtent().X > Stone.GetExtent().Z);
				// The navigation point clears the hull and its rings from any side by the fleet's 1.5 km slot.
				TestTrue(What + TEXT(": slot clear of the hull"), Shape.Metrics.NavHeightCm - 150000.0 > Stone.GetExtent().Z
					&& Shape.Metrics.NavHeightCm - 150000.0 > Stone.GetExtent().Y);
			}
			else
			{
				// It reaches about its measured height on flat ground and no higher; its base is buried.
				TestTrue(What + TEXT(": top within the measure"), Stone.Max.Z <= Shape.Metrics.HeightCm * 1.03 + 100.0);
				TestTrue(What + TEXT(": stands its height"), Stone.Max.Z >= Shape.Metrics.HeightCm * (Kind == EKind::StoneCircle ? 0.25 : 0.8));
				TestTrue(What + TEXT(": buried, not hanging"), Stone.Min.Z < 0.0);
				TestTrue(What + TEXT(": within its footprint"), FMath::Max(FMath::Abs(Stone.Min.X), FMath::Abs(Stone.Max.X))
					<= Shape.Metrics.FootprintCm * 1.6 && FMath::Max(FMath::Abs(Stone.Min.Y), FMath::Abs(Stone.Max.Y))
					<= Shape.Metrics.FootprintCm * 1.6);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAncientsPlacementTest, "APS.Ancients.Sites.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAncientsPlacementTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("one hash"), APSAncients::Hash(TEXT("a"), TEXT("b"), TEXT("c")), FCrc::StrCrc32(TEXT("a|b|c")));
	for (const uint32 Seed : {3u, 1234567u})
	{
		TSet<FString> Seen;
		for (int32 Index = 0; Index < 48; ++Index)
		{
			const FVector Direction = APSAncientsSites::CandidateDirection(Seed, Index);
			TestTrue(TEXT("unit direction"), FMath::IsNearlyEqual(Direction.Size(), 1.0, 1.0e-6));
			TestTrue(TEXT("away from the poles"), FMath::Abs(FMath::RadiansToDegrees(FMath::Asin(Direction.Z))) <= 55.0 + 1.0e-3);
			TestTrue(TEXT("the same every time"), Direction.Equals(APSAncientsSites::CandidateDirection(Seed, Index), 0.0));
			Seen.Add(Direction.ToString());
		}
		TestTrue(TEXT("candidates differ"), Seen.Num() > 40);
		const double Yaw = APSAncientsSites::Yaw(Seed);
		TestTrue(TEXT("yaw"), Yaw >= 0.0 && Yaw <= UE_TWO_PI && Yaw == APSAncientsSites::Yaw(Seed));
		FVector Direction;
		double Radii = 0.0;
		APSAncientsSites::OrbitOf(Seed, Direction, Radii);
		TestTrue(TEXT("high orbit"), Radii >= 1.9 && Radii <= 2.6 && FMath::IsNearlyEqual(Direction.Size(), 1.0, 1.0e-6));
		TestTrue(TEXT("orbital turn"), APSAncientsSites::OrbitalTurn(Seed).IsNormalized());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAncientsChainsTest, "APS.Ancients.Quests.Chains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAncientsChainsTest::RunTest(const FString& Parameters)
{
	using namespace APSAncientsTestsLocal;
	using APSMissions::EObjective;
	TSet<FName> Templates;
	TSet<FString> Subjects;
	for (int32 ChainIndex = 0; ChainIndex < static_cast<int32>(EChain::Count); ++ChainIndex)
	{
		const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(static_cast<EChain>(ChainIndex));
		TestEqual(TEXT("chain table order"), static_cast<int32>(Chain.Id), ChainIndex);
		TestTrue(TEXT("chain has steps and a signal"), !Chain.Steps.IsEmpty() && !Chain.Signal.IsEmpty());
		FSiteSpec Spec = MakeSpec(EKind::Monolith, 5);
		Spec.Chain = Chain.Id;
		for (const FString& Id : {FString(TEXT("H_MONUMENT")), FString(TEXT("N_0123456789ABCDEF0123456789ABCDEF"))})
		{
			Spec.Id = Id;
			for (int32 Step = 0; Step < Chain.Steps.Num(); ++Step)
			{
				const APSAncientsQuests::FStep& Definition = Chain.Steps[Step];
				TestTrue(TEXT("step texts and conditions"), !Definition.Title.IsEmpty() && !Definition.Brief.IsEmpty()
					&& Definition.Conditions != 0);
				// Only objectives no count-of-any or subjectless board template listens to (APSMissionsLocal::Matches).
				TestTrue(TEXT("safe objective"), Definition.Objective == EObjective::LocateAnomaly
					|| Definition.Objective == EObjective::StudyWorld || Definition.Objective == EObjective::InvestigateAnomaly
					|| Definition.Objective == EObjective::SurveySystem);
				if (Definition.Unlocks)
				{
					const APSInfrastructure::FType* Type = APSInfrastructure::Find(FName(Definition.Unlocks));
					TestTrue(FString::Printf(TEXT("unlock %s is a locked catalogue type"), Definition.Unlocks), Type && Type->bNeedsUnlock);
				}
				const FName Template = APSAncientsQuests::TemplateOf(Spec, Step);
				TestTrue(TEXT("ours"), APSAncientsQuests::IsAncientTemplate(Template));
				TestFalse(TEXT("unique template"), Templates.Contains(Template));
				Templates.Add(Template);
				const FString Subject = APSAncientsQuests::SubjectOf(Spec, Step) + Chain.Key;
				TestFalse(TEXT("unique subject"), Subjects.Contains(Subject));
				Subjects.Add(Subject);
			}
		}
	}
	TestFalse(TEXT("the board's own templates are not ours"), APSAncientsQuests::IsAncientTemplate(TEXT("ScienceLocate")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAncientsBoardTest, "APS.Ancients.Quests.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAncientsBoardTest::RunTest(const FString& Parameters)
{
	using namespace APSAncientsTestsLocal;
	FSiteSpec Spec = MakeSpec(EKind::Monolith, 11);
	Spec.Id = TEXT("H_MONUMENT");
	Spec.Chain = EChain::Echoes;
	FFormatNamedArguments Args;
	Args.Add(TEXT("Site"), FText::FromString(TEXT("THE NEEDLE")));
	Args.Add(TEXT("Body"), FText::FromString(TEXT("HOME")));
	const FText SubjectName = FText::FromString(TEXT("THE NEEDLE  /  HOME"));

	FAPSMissionBoard Board(nullptr);
	const FAPSMission First = APSAncientsQuests::MakeMission(Spec, 0, Args, SubjectName, 10.0, APSMissions::EState::Active);
	TestFalse(TEXT("reward from the chain"), First.Reward.IsEmpty());
	APSAncientsQuests::Inject(Board, First, true);
	TestEqual(TEXT("on the board"), Board.GetMissions().Num(), 1);
	TestTrue(TEXT("tracked when nothing else is"), Board.GetTracked() && Board.GetTracked()->Id == First.Id);

	// Only its own subject moves it.
	Board.Notify(First.Objective, TEXT("BODY:Elsewhere"), 1);
	TestEqual(TEXT("another subject"), Board.GetMissions()[0].Progress, 0);
	Board.Notify(First.Objective, First.Subject, 1);
	TestEqual(TEXT("its subject"), Board.GetMissions()[0].Progress, 1);

	APSAncientsQuests::FBoardView View;
	TestTrue(TEXT("view reads"), View.Refresh(Board));
	TestTrue(TEXT("open in the view"), View.Open.Contains(First.Template));
	TestFalse(TEXT("unchanged board, no reread"), View.Refresh(Board));

	// The player's tracked mission stays tracked when another of ours comes.
	const FAPSMission Second = APSAncientsQuests::MakeMission(Spec, 1, Args, SubjectName, 20.0, APSMissions::EState::Active);
	APSAncientsQuests::Inject(Board, Second, true);
	TestTrue(TEXT("tracking kept"), Board.GetTracked() && Board.GetTracked()->Id == First.Id);
	TestEqual(TEXT("both on the board"), Board.GetMissions().Num(), 2);

	// A save keeps our templates but not the rewards (the board fills those from its own templates): Edit puts them back.
	FAPSMissionSaveData Saved;
	Board.CaptureSave(Saved);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	Writer << Saved;
	FAPSMissionSaveData Loaded;
	FMemoryReader Reader(Bytes, true);
	Reader << Loaded;
	TestEqual(TEXT("missions saved"), Loaded.Missions.Num(), 2);
	TestTrue(TEXT("template kept"), Loaded.Missions.Num() == 2 && Loaded.Missions[0].Template == First.Template
		&& Loaded.Missions[0].Subject == First.Subject);
	TestTrue(TEXT("reward lost in the save"), Loaded.Missions.Num() == 2 && Loaded.Missions[0].Reward.IsEmpty());
	FAPSMissionBoard Restored(nullptr);
	Restored.RestoreSave(MoveTemp(Loaded));
	const bool bEdited = APSAncientsQuests::Edit(Restored, [&First](FAPSMission& Mission)
	{
		if (Mission.Template != First.Template || !Mission.Reward.IsEmpty())
		{
			return false;
		}
		Mission.Reward = First.Reward;
		return true;
	});
	TestTrue(TEXT("edited"), bEdited);
	TestTrue(TEXT("reward back"), Restored.GetMissions().Num() == 2 && !Restored.GetMissions()[0].Reward.IsEmpty());
	TestFalse(TEXT("nothing more to edit"), APSAncientsQuests::Edit(Restored, [](FAPSMission&) { return false; }));
	return true;
}

#endif
