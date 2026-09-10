#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Rendering/APSContinuousPreviewFrame.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewVisibility.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAddressedSystemEditTest,
	"APS.Preview.Coherence.AddressedSystemEdits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAddressedSystemEditTest::RunTest(const FString& Parameters)
{
	FAPSPreviewSystemEditOverride Edit;
	FStarSystemModel Original;
	Original.StableId = FGuid(1, 2, 3, 4);
	Original.GenerationSeed = 271828;
	Original.Location = FVector(1.0e16, -2.0e16, 3.0e16);
	Original.AmountOfStars = 3;
	Original.StarSystemType = EStarType::TripleStar;
	Original.PotentialPlanetCount = 7;
	Original.bHasPlanetarySystem = true;
	FStarSystemModel Model = Original;
	Edit.bOverrideOrbitDistribution = true;
	Edit.OrbitDistribution = EOrbitDistributionType::Dense;
	Edit.ApplyToSystem(Model);
	TestTrue(TEXT("Orbit-only edit cannot change a system's physical catalog record"),
		FStarSystemModel::StaticStruct()->CompareScriptStruct(&Original, &Model, 0));
	for (int32 Count = 1; Count <= 6; ++Count)
	{
		for (int32 Total = 0; Total <= 120; ++Total)
		{
			Edit.TotalPlanets = Total;
			int32 ResolvedTotal = 0;
			for (int32 Star = 0; Star < Count; ++Star)
			{
				FPlanetarySystemModel Family;
				Family.PlanetarySystemType = EPlanetarySystemType::SinglePlanetSystem;
				Family.AmountOfPlanets = 99;
				Edit.ApplyToFamily(Family, Star, Count);
				ResolvedTotal += Family.AmountOfPlanets;
				TestEqual(TEXT("Every addressed family receives the edited orbit distribution"), Family.OrbitDistributionType, EOrbitDistributionType::Dense);
				TestTrue(TEXT("A zero-planet allocation cannot grow a phantom planet"),
					Family.AmountOfPlanets != 0 || Family.PlanetarySystemType == EPlanetarySystemType::NoPlanetSystem);
				TestTrue(TEXT("The single-planet generator cannot truncate an explicit larger count"),
					Family.AmountOfPlanets <= 1 || Family.PlanetarySystemType != EPlanetarySystemType::SinglePlanetSystem);
			}
			TestEqual(TEXT("Family allocation conserves the exact requested total"), ResolvedTotal, Total);
		}
	}
	Edit.StarCount = 2;
	Edit.StarType = EStarType::DoubleStar;
	Edit.TotalPlanets = 8;
	Edit.ApplyToSystem(Model);
	TestEqual(TEXT("Multiplicity is explicit"), Model.AmountOfStars, 2);
	TestEqual(TEXT("Catalog planet count matches the requested total"), Model.PotentialPlanetCount, 8);
	TestEqual(TEXT("System edit retains stable identity"), Model.StableId, Original.StableId);
	TestEqual(TEXT("System edit retains physical position"), Model.Location, Original.Location);
	TestEqual(TEXT("System edit retains owned generation seed"), Model.GenerationSeed, Original.GenerationSeed);
	UGeneratedWorld* World = NewObject<UGeneratedWorld>();
	UGeneratedWorld* Reordered = NewObject<UGeneratedWorld>();
	World->SetPreviewSystemEditOverride(TEXT("SYS0"), Edit);
	World->SetPreviewSystemEditOverride(TEXT("SYS-remote"), Edit);
	Reordered->SetPreviewSystemEditOverride(TEXT("SYS-remote"), Edit);
	Reordered->SetPreviewSystemEditOverride(TEXT("SYS0"), Edit);
	TestEqual(TEXT("System edit hash is insertion-order independent"), World->GetPreviewSystemEditHash(), Reordered->GetPreviewSystemEditHash());
	UGeneratedWorld* Copy = DuplicateObject<UGeneratedWorld>(World, GetTransientPackage());
	TestEqual(TEXT("System edits survive reflection transport"), Copy->GetPreviewSystemEditHash(), World->GetPreviewSystemEditHash());
	TestTrue(TEXT("An unedited system has no override"), World->FindPreviewSystemEditOverride(TEXT("SYS-neighbor")) == nullptr);
	World->ClearPreviewSystemEditOverrides();
	TestEqual(TEXT("Explicit regenerate clears system edits"), World->GetPreviewSystemEditHash(), 0u);
	TestTrue(TEXT("Clearing preview cannot mutate transported system edits"), Copy->FindPreviewSystemEditOverride(TEXT("SYS0")) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAddressedStellarEditTest,
	"APS.Preview.Coherence.AddressedStellarEdits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAddressedStellarEditTest::RunTest(const FString& Parameters)
{
	UGeneratedWorld* World = NewObject<UGeneratedWorld>();
	UGeneratedWorld* Reordered = NewObject<UGeneratedWorld>();
	UStarGenerator* Generator = NewObject<UStarGenerator>();
	Generator->SetGenerationSeed(271828);
	TSharedPtr<FStarModel> Base = MakeShared<FStarModel>();
	Base->StellarType = EStellarType::MainSequence;
	Base->SpectralClass = ESpectralClass::G;
	Generator->GenerateStarModel(Base);
	FAPSPreviewStarEditOverride Edit;
	Edit.AutomaticModel = Edit.Model = *Base;
	Edit.RadiusOverrideSolar = 2.25;
	Generator->ApplyRadiusOverrideSolar(Edit.Model, Edit.RadiusOverrideSolar);
	World->SetPreviewStarEditOverride(TEXT("SYS0/S1"), Edit);
	World->SetPreviewStarEditOverride(TEXT("SYS-remote/S0"), Edit);
	Reordered->SetPreviewStarEditOverride(TEXT("SYS-remote/S0"), Edit);
	Reordered->SetPreviewStarEditOverride(TEXT("SYS0/S1"), Edit);
	TestEqual(TEXT("Stellar authoring hash is independent of insertion order"),
		World->GetPreviewStarEditHash(), Reordered->GetPreviewStarEditHash());
	FStarModel Neighbor = *Base;
	TestFalse(TEXT("Companion edit cannot alias the home primary"), World->ApplyPreviewStarEditOverride(TEXT("SYS0/S0"), Neighbor));
	TestTrue(TEXT("Unselected star retains every model field"), FStarModel::StaticStruct()->CompareScriptStruct(&Neighbor, Base.Get(), 0));
	Neighbor.Location = FVector(1.0e18, 2.0e18, 3.0e18);
	TestTrue(TEXT("Addressed star resolves its own override"), World->ApplyPreviewStarEditOverride(TEXT("SYS0/S1"), Neighbor));
	TestEqual(TEXT("Editor never replaces the canonical position"), Neighbor.Location, FVector(1.0e18, 2.0e18, 3.0e18));
	TestEqual(TEXT("Edited radius is the physical model radius"), Neighbor.RadiusKM, Edit.Model.RadiusKM);
	UGeneratedWorld* Transport = DuplicateObject<UGeneratedWorld>(World, GetTransientPackage());
	TestEqual(TEXT("Gameplay duplication preserves exact stellar edit hash"), Transport->GetPreviewStarEditHash(), World->GetPreviewStarEditHash());
	const FAPSPreviewStarEditOverride* Copied = Transport->FindPreviewStarEditOverride(TEXT("SYS0/S1"));
	if (TestNotNull(TEXT("Gameplay duplication retains the addressed edit"), Copied))
	{
		TestTrue(TEXT("AUTO baseline survives reflection transport"), FStarModel::StaticStruct()->CompareScriptStruct(&Copied->AutomaticModel, Base.Get(), 0));
		TestTrue(TEXT("Resolved physical star survives reflection transport"), FStarModel::StaticStruct()->CompareScriptStruct(&Copied->Model, &Edit.Model, 0));
	}
	World->ClearPreviewStarEditOverrides();
	TestEqual(TEXT("Explicit regenerate clears stellar authoring"), World->GetPreviewStarEditHash(), 0u);
	TestTrue(TEXT("Clearing source cannot mutate committed copy"), Transport->FindPreviewStarEditOverride(TEXT("SYS0/S1")) != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSContinuousPreviewFrameTest,
	"APS.Preview.Coherence.PhysicalViewFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSContinuousPreviewFrameTest::RunTest(const FString& Parameters)
{
	constexpr double SolarRadiusCm = 6.957e10;
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	const FVector StarCenter = FVector::ZeroVector;
	const FVector Observer(0.0, AstronomicalUnitCm, 0.0);
	const double PhysicalAngularRadius = FMath::Asin(SolarRadiusCm / AstronomicalUnitCm);
	TestTrue(TEXT("solar disc at one AU has its physical angular radius"),
		FMath::IsNearlyEqual(FMath::RadiansToDegrees(PhysicalAngularRadius), 0.26645, 0.0001));
	for (const double Scale : {1.0e-12, 1.0e-8, 0.002, 1.0, 1000.0})
	{
		FAPSContinuousPreviewFrame Frame;
		Frame.ObserverCm = Observer;
		Frame.RenderCmPerPhysicalCm = Scale;
		FAPSPreviewProjectedSphere Star;
		TestTrue(TEXT("physical star projects at every numerical scale"),
			Frame.ProjectSphere(StarCenter, SolarRadiusCm, Star));
		TestTrue(TEXT("far compression preserves exact stellar angular radius"),
			FMath::IsNearlyEqual(FMath::Asin(Star.Radius / Star.Center.Size()),
				PhysicalAngularRadius, 1.0e-12));
		TestTrue(TEXT("render center remains inside finite envelope"),
			Star.Center.Size() < Frame.FarEnvelopeCm);
		TestTrue(TEXT("star direction remains tied to observer and canonical position"),
			Star.Center.GetSafeNormal().Equals(FVector(0.0, -1.0, 0.0), 1.0e-12));

		FAPSPreviewProjectedSphere Companion;
		Frame.ProjectSphere(StarCenter, SolarRadiusCm * 0.2, Companion);
		TestTrue(TEXT("different stellar physical radii retain their ratio"),
			FMath::IsNearlyEqual(Companion.Radius / Star.Radius, 0.2, 1.0e-12));

		const FVector OriginShift(1.0e17, -2.0e17, 3.0e17);
		FAPSContinuousPreviewFrame Shifted = Frame;
		Shifted.ObserverCm += OriginShift;
		FAPSPreviewProjectedSphere ShiftedStar;
		Shifted.ProjectSphere(StarCenter + OriginShift, SolarRadiusCm, ShiftedStar);
		TestTrue(TEXT("changing canonical origin does not change the view"),
			ShiftedStar.Center.Equals(Star.Center, FMath::Max(1.0e-6, Star.Center.Size() * 1.0e-10)));
	}

	FAPSContinuousPreviewFrame Frame;
	Frame.ObserverCm = FVector(2.0e13, -1.0e13, 3.0e12);
	const FVector A(3.0e17, 5.0e16, 2.0e16);
	const FVector B(-1.0e16, 2.0e16, 4.0e17);
	FVector RenderA, RenderB;
	Frame.ProjectPosition(A, RenderA);
	Frame.ProjectPosition(B, RenderB);
	const double PhysicalPairAngle = FVector::DotProduct(
		(A - Frame.ObserverCm).GetSafeNormal(), (B - Frame.ObserverCm).GetSafeNormal());
	TestTrue(TEXT("galaxy and cluster keep their relative sky directions"),
		FMath::IsNearlyEqual(FVector::DotProduct(RenderA.GetSafeNormal(), RenderB.GetSafeNormal()),
			PhysicalPairAngle, 1.0e-12));

	FAPSContinuousPreviewOrbit Galaxy;
	Galaxy.CenterCm = FVector(3.0e16, -2.0e16, 0.0);
	Galaxy.DistanceCm = 2.0e17;
	FAPSContinuousPreviewOrbit Planet;
	Planet.CenterCm = Observer;
	Planet.DistanceCm = 2.7e9;
	double LastDistance = Galaxy.DistanceCm;
	for (int32 Index = 0; Index <= 300; ++Index)
	{
		const auto Pose = FAPSContinuousPreviewOrbit::Interpolate(Galaxy, Planet, Index / 300.0);
		TestTrue(TEXT("all transition frames are finite canonical views"), Pose.IsValid());
		TestTrue(TEXT("zoom progresses monotonically through physical scales"),
			Pose.DistanceCm <= LastDistance * (1.0 + 1.0e-12));
		LastDistance = Pose.DistanceCm;
		TestTrue(TEXT("translation converges before zoom exposes a remote off-axis center"),
			(Pose.CenterCm - Planet.CenterCm).Size() / Pose.DistanceCm
				<= (Galaxy.CenterCm - Planet.CenterCm).Size() / Galaxy.DistanceCm + 1.0e-8);
	}
	const auto MidMove = FAPSContinuousPreviewOrbit::Interpolate(Galaxy, Planet, 0.37);
	const auto Interrupted = FAPSContinuousPreviewOrbit::Interpolate(MidMove, Galaxy, 0.0);
	TestTrue(TEXT("interrupting a transition preserves the exact displayed observer"),
		Interrupted.ObserverCm().Equals(MidMove.ObserverCm(), 0.0));
	const auto Finished = FAPSContinuousPreviewOrbit::Interpolate(Galaxy, Planet, 1.0);
	TestTrue(TEXT("transition ends at the exact requested physical observer"),
		Finished.ObserverCm().Equals(Planet.ObserverCm(), 0.0));
	const auto Repeat = FAPSContinuousPreviewOrbit::Interpolate(Planet, Planet, 0.5);
	TestTrue(TEXT("repeated focus is stationary"),
		Repeat.ObserverCm().Equals(Planet.ObserverCm(), 1.0));
	FAPSContinuousPreviewOrbit StarCloseup;
	StarCloseup.DistanceCm = SolarRadiusCm * 4.3;
	const auto Transfer = FAPSContinuousPreviewOrbit::Interpolate(StarCloseup, Planet, 0.5);
	TestTrue(TEXT("star-to-planet transfer pulls back enough to show their shared spatial context"),
		Transfer.DistanceCm >= FVector::Distance(StarCloseup.CenterCm, Planet.CenterCm) * 2.5);
	const auto BeforeTransferMidpoint = FAPSContinuousPreviewOrbit::Interpolate(StarCloseup, Planet, 0.5 - 1.0e-6);
	const auto AfterTransferMidpoint = FAPSContinuousPreviewOrbit::Interpolate(StarCloseup, Planet, 0.5 + 1.0e-6);
	TestTrue(TEXT("two close-up transfer legs join continuously"),
		FVector::Distance(BeforeTransferMidpoint.ObserverCm(), AfterTransferMidpoint.ObserverCm())
			< Transfer.DistanceCm * 1.0e-8);
	Frame.RenderCmPerPhysicalCm = 0.0;
	FAPSPreviewProjectedSphere Invalid;
	TestFalse(TEXT("invalid scale cannot publish a collapsed scene"),
		Frame.ProjectSphere(A, SolarRadiusCm, Invalid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPreviewAnnotationVisibilityTest,
	"APS.Preview.Coherence.AnnotationVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewAnnotationVisibilityTest::RunTest(const FString& Parameters)
{
	const FAPSPreviewOccluder Planet{FVector(0.0, 0.0, 10.0), 2.0};
	TestFalse(TEXT("An orbit in front of the planet remains visible"), Planet.Occludes(FVector(0.0, 0.0, 7.0)));
	TestFalse(TEXT("The nearest surface is not behind itself"), Planet.Occludes(FVector(0.0, 0.0, 8.0)));
	TestTrue(TEXT("The rear surface is hidden"), Planet.Occludes(FVector(0.0, 0.0, 12.0)));
	TestFalse(TEXT("A point outside the actual perspective silhouette remains visible"), Planet.Occludes(FVector(5.0, 0.0, 20.0)));
	TArray<FVector2D> Intervals;
	for (const double Scale : {1.0e-6, 1.0, 1.0e6, 1.0e12})
	{
		const TArray<FAPSPreviewOccluder> Occluders{{Planet.Center * Scale, Planet.Radius * Scale}};
		APSPreviewVisibility::VisibleIntervals(FVector(-6.0, 0.0, 20.0) * Scale,
			FVector(6.0, 0.0, 20.0) * Scale, Occluders, Intervals);
		TestEqual(TEXT("Two visible endpoints cannot bridge the hidden middle"), Intervals.Num(), 2);
		if (Intervals.Num() == 2)
		{
			const double LimbX = 40.0 / FMath::Sqrt(96.0);
			TestTrue(TEXT("Rear guide ends at the actual tangent cone"),
				FMath::IsNearlyEqual(Intervals[0].Y, (6.0 - LimbX) / 12.0, 1.0e-10));
			TestTrue(TEXT("Rear guide restarts at the opposite limb"),
				FMath::IsNearlyEqual(Intervals[1].X, (6.0 + LimbX) / 12.0, 1.0e-10));
		}
		APSPreviewVisibility::VisibleIntervals(FVector(-6.0, 0.0, 5.0) * Scale,
			FVector(6.0, 0.0, 5.0) * Scale, Occluders, Intervals);
		TestTrue(TEXT("Front guide is continuous across the apparent body disc"),
			Intervals.Num() == 1 && Intervals[0].Equals(FVector2D(0.0, 1.0), 1.0e-10));
		APSPreviewVisibility::VisibleIntervals(FVector(0.0, 0.0, 1.0) * Scale,
			FVector(0.0, 0.0, 20.0) * Scale, Occluders, Intervals);
		TestTrue(TEXT("A guide entering the sphere ends at its front surface"),
			Intervals.Num() == 1 && FMath::IsNearlyEqual(Intervals[0].Y, 7.0 / 19.0, 1.0e-10));
	}
	const TArray<FAPSPreviewOccluder> TwoBodies{{FVector(-3.0, 0.0, 10.0), 1.0}, {FVector(3.0, 0.0, 12.0), 1.0}};
	APSPreviewVisibility::VisibleIntervals(FVector(-12.0, 0.0, 20.0), FVector(12.0, 0.0, 20.0), TwoBodies, Intervals);
	TestEqual(TEXT("Two bodies split a crossing guide into three visible intervals"), Intervals.Num(), 3);
	FRandomStream Random(42791);
	for (int32 Case = 0; Case < 250; ++Case)
	{
		const FVector From(Random.FRandRange(-12.0, 12.0), Random.FRandRange(-3.0, 3.0), Random.FRandRange(1.0, 30.0));
		const FVector To(Random.FRandRange(-12.0, 12.0), Random.FRandRange(-3.0, 3.0), Random.FRandRange(1.0, 30.0));
		APSPreviewVisibility::VisibleIntervals(From, To, TwoBodies, Intervals);
		for (int32 Sample = 0; Sample < 41; ++Sample)
		{
			const double T = (Sample + 0.371) / 41.0;
			const FVector Point = FMath::Lerp(From, To, T);
			const double Length = Point.Size();
			const FVector Ray = Point / Length;
			bool bHidden = false;
			for (const FAPSPreviewOccluder& Body : TwoBodies)
			{
				const double Along = FVector::DotProduct(Body.Center, Ray);
				const double PerpendicularSquared = (Body.Center - Along * Ray).SizeSquared();
				if (Along > 0.0 && PerpendicularSquared < Body.Radius * Body.Radius
					&& Along - FMath::Sqrt(Body.Radius * Body.Radius - PerpendicularSquared) < Length) bHidden = true;
			}
			const bool bVisible = Intervals.ContainsByPredicate([T](const FVector2D& Interval) { return T >= Interval.X && T <= Interval.Y; });
			TestEqual(TEXT("Analytic clipping agrees with independent ray/sphere intersections"), bVisible, !bHidden);
		}
	}
	FVector2D A(-50.0, 5.0), B(50.0, 5.0);
	TestTrue(TEXT("A guide crossing both panel edges is retained"), APSPreviewVisibility::ClipToPanel(A, B, FVector2D(10.0)));
	TestTrue(TEXT("The left panel cannot receive an orbit segment"), A.Equals(FVector2D(0.0, 5.0), 1.0e-10));
	TestTrue(TEXT("The right panel cannot receive an orbit segment"), B.Equals(FVector2D(10.0, 5.0), 1.0e-10));
	A = FVector2D(-50.0, -5.0); B = FVector2D(50.0, -5.0);
	TestFalse(TEXT("A guide above the central viewport is discarded"), APSPreviewVisibility::ClipToPanel(A, B, FVector2D(10.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPreviewResolvedFrustumTest,
	"APS.Preview.Coherence.ResolvedFrustum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewResolvedFrustumTest::RunTest(const FString& Parameters)
{
	using APSPreviewVisibility::SphereIntersectsView;
	for (const double Scale : {1.0e-6, 1.0, 1.0e9, 1.0e15})
	{
		TestTrue(TEXT("A front-facing physical disc remains allocated"), SphereIntersectsView(FVector(10.0, 0.0, 0.0) * Scale, Scale, 0.5, 0.3));
		TestFalse(TEXT("A disc fully behind the observer needs no render geometry"), SphereIntersectsView(FVector(-10.0, 0.0, 0.0) * Scale, Scale, 0.5, 0.3));
		TestFalse(TEXT("An off-screen disc is rejected beyond its entire silhouette"), SphereIntersectsView(FVector(10.0, 6.2, 0.0) * Scale, Scale, 0.5, 0.3));
		TestTrue(TEXT("A disc with its center outside the right edge still crosses the view"), SphereIntersectsView(FVector(10.0, 6.0, 0.0) * Scale, Scale, 0.5, 0.3));
		TestTrue(TEXT("An optical guard keeps the star before its disc enters the view"), SphereIntersectsView(FVector(10.0, 6.2, 0.0) * Scale, Scale * 1.2, 0.5, 0.3));
		TestTrue(TEXT("The top edge uses the viewport's vertical field of view"), SphereIntersectsView(FVector(10.0, 0.0, 4.0) * Scale, Scale, 0.5, 0.3));
		TestFalse(TEXT("A disc entirely above the top edge is rejected"), SphereIntersectsView(FVector(10.0, 0.0, 4.2) * Scale, Scale, 0.5, 0.3));
		TestTrue(TEXT("A sphere containing the observer cannot disappear due to its center behind the near plane"), SphereIntersectsView(FVector(-0.5, 0.0, 0.0) * Scale, Scale, 0.5, 0.3));
	}
	TestFalse(TEXT("An invalid frustum cannot claim visibility"), SphereIntersectsView(FVector(10.0, 0.0, 0.0), 1.0, 0.0, 0.3));
	return true;
}

#endif
