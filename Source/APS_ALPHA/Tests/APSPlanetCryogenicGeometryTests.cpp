#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "Engine/World.h"

namespace APSPlanetCryogenicGeometryTests
{
	constexpr EPlanetType Types[] = {
		EPlanetType::Ice, EPlanetType::Frozen, EPlanetType::Tundra, EPlanetType::Rogue};
	constexpr int32 Seeds[] = {73531, 11021, 90379};
	constexpr double RadiiKm[] = {679.0638, 6750.0};
	constexpr int32 GlobeSampleCount = 256;
	constexpr double ParityToleranceCm = 1.e-6;

	// Identity/regression floors, not a claim that the visual distinction is sufficient.
	constexpr double MinimumPairRmsCm = 100.0;
	constexpr double ChangedPointToleranceCm = 1.0;
	constexpr double MinimumChangedFraction = 0.25;
	constexpr double MinimumUnitShapeRms = 0.001;

	struct FFixture
	{
		UWorld* World = nullptr;
		APlanet* Planet = nullptr;
		UAPSWorldScapePlanetNoise* Noise = nullptr;

		FFixture()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false,
				ERHIFeatureLevel::Num, &Values);
			if (!World) return;
			Planet = World->SpawnActor<APlanet>();
			Noise = NewObject<UAPSWorldScapePlanetNoise>(World);
			if (!Planet) return;
			Planet->SetActorTickEnabled(false);
			Planet->Temperature = 288;
			Planet->PlanetAtmosphere.Humidity = 42.0f;
			Planet->PlanetAtmosphere.AtmosphericPressure = 101325.0f;
			Planet->PlanetGeosphere.SeismicActivity = 2.0f;
			Planet->PlanetGeosphere.CrustThickness = 38.0f;
			Planet->WorldScapePresentationScale = 1.0;
			Planet->SurfaceFeatureScale = Planet->SurfaceReliefScale = 1.0;
			Planet->SurfaceLandCoverageScale = Planet->SurfaceMountainScale = 1.0;
			Planet->SurfaceCraterScale = Planet->SurfaceRoughnessScale = 1.0;
		}

		~FFixture()
		{
			if (World) World->DestroyWorld(false);
		}

		bool IsValid() const { return World && Planet && Noise; }

		void SetPhysicalBody(int32 Seed, double RadiusKm) const
		{
			Planet->WorldScapeSeed = Seed;
			Planet->RadiusKM = RadiusKm;
			Planet->PlanetRadiusKM = FMath::FloorToInt(RadiusKm);
		}

		FAPSResolvedPlanetSurfaceProfile Resolve(EPlanetType Type,
			const UAPSPlanetSurfaceCatalog* Catalog) const
		{
			// Reuse the same body and generator. No type-specific seed changes.
			Planet->PlanetType = Type;
			const auto Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Catalog);
			Noise->Configure(Profile);
			return Profile;
		}
	};

	TArray<FVector> MakeDirections(double RadiusCm)
	{
		TArray<FVector> Result;
		Result.Reserve(GlobeSampleCount + 75);
		constexpr double GoldenAngle = 2.39996322972865332;
		for (int32 Index = 0; Index < GlobeSampleCount; ++Index)
		{
			const double Z = 1.0 - 2.0 * (Index + 0.5) / GlobeSampleCount;
			const double Ring = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			Result.Add(FVector(Ring * FMath::Cos(GoldenAngle * Index),
				Ring * FMath::Sin(GoldenAngle * Index), Z));
		}
		// Include three 1 km local windows, not just distant points on the globe.
		for (FVector Center : {FVector(0.3, 0.4, 0.866), FVector(-0.4, 0.8, 0.44),
			FVector(0.75, -0.4, -0.52)})
		{
			Center.Normalize();
			const FVector East = FVector::CrossProduct(FVector::UpVector, Center).GetSafeNormal();
			const FVector North = FVector::CrossProduct(Center, East).GetSafeNormal();
			for (int32 Y = -2; Y <= 2; ++Y)
				for (int32 X = -2; X <= 2; ++X)
					Result.Add((Center * RadiusCm + (East * X + North * Y) * 25000.0).GetSafeNormal());
		}
		return Result;
	}

	struct FHeights
	{
		TArray<double> Values;
		bool bFinite = true;
		double MaxParityErrorCm = 0.0;
	};

	FHeights Sample(const FFixture& Fixture, const FAPSResolvedPlanetSurfaceProfile& Profile,
		const TArray<FVector>& Directions, double RadiusCm, bool bCheckParity)
	{
		CustomNoise NativeNoise(Fixture.Planet->WorldScapeSeed);
		// Native constructor has a seed guard; explicitly initialize its permutation.
		NativeNoise.SetSeed(Fixture.Planet->WorldScapeSeed + 1);
		NativeNoise.SetSeed(Fixture.Planet->WorldScapeSeed);
		const double Scale = FMath::Max(1.0, double(FMath::RoundToInt(Profile.NoiseScale)));
		const double Intensity = FMath::Max(1.0, double(FMath::RoundToInt(Profile.NoiseIntensity)));
		FHeights Result;
		Result.Values.Reserve(Directions.Num());
		for (const FVector& Direction : Directions)
		{
			const DVector Position(Direction * RadiusCm);
			DVector NoisePosition;
			const double Height = Fixture.Noise->GetNoise(NativeNoise, Position, DVector(0.0),
				Scale, Intensity, RadiusCm, false, Direction.Z, NoisePosition, FNoiseData(), true).Height;
			Result.Values.Add(Height);
			Result.bFinite &= FMath::IsFinite(Height);
			if (!bCheckParity) continue;
			const double InstanceHeight = Fixture.Noise->SampleResolved(NativeNoise, Position,
				DVector(0.0), Scale, Intensity, RadiusCm, Direction.Z, NoisePosition).Height;
			const double SnapshotHeight = UAPSWorldScapePlanetNoise::SampleResolvedProfile(Profile,
				NativeNoise, Position, DVector(0.0), Scale, Intensity, RadiusCm, Direction.Z, NoisePosition).Height;
			const double HeightOnly = UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Profile,
				NativeNoise, Position, DVector(0.0), Scale, Intensity, RadiusCm, Direction.Z);
			const double CollisionHeight = Fixture.Noise->SampleCollisionHeight(NativeNoise, Position,
				DVector(0.0), Scale, Intensity, RadiusCm, Direction.Z);
			for (double Other : {InstanceHeight, SnapshotHeight, HeightOnly, CollisionHeight})
			{
				Result.bFinite &= FMath::IsFinite(Other);
				Result.MaxParityErrorCm = FMath::Max(Result.MaxParityErrorCm, FMath::Abs(Height - Other));
			}
		}
		// Only signed physical Height enters this test. No HeightNormalize, palette,
		// material, climate, water/foliage masks or profile/mesh signatures are compared.
		return Result;
	}

	void CheckPair(FAutomationTestBase& Test, const FString& Label,
		const FHeights& A, const FHeights& B)
	{
		if (!Test.TestTrue(Label + TEXT(" finite matching samples"),
			A.bFinite && B.bFinite && A.Values.Num() == B.Values.Num() && !A.Values.IsEmpty())) return;
		double SumA = 0.0, SumB = 0.0, SquaredDifference = 0.0;
		int32 Changed = 0;
		for (int32 I = 0; I < A.Values.Num(); ++I)
		{
			SumA += A.Values[I];
			SumB += B.Values[I];
			const double Difference = A.Values[I] - B.Values[I];
			SquaredDifference += Difference * Difference;
			Changed += FMath::Abs(Difference) > ChangedPointToleranceCm ? 1 : 0;
		}
		const double MeanA = SumA / A.Values.Num(), MeanB = SumB / B.Values.Num();
		double VarianceA = 0.0, VarianceB = 0.0;
		for (int32 I = 0; I < A.Values.Num(); ++I)
		{
			VarianceA += FMath::Square(A.Values[I] - MeanA);
			VarianceB += FMath::Square(B.Values[I] - MeanB);
		}
		const double DeviationA = FMath::Sqrt(VarianceA / A.Values.Num());
		const double DeviationB = FMath::Sqrt(VarianceB / B.Values.Num());
		if (!Test.TestTrue(Label + TEXT(" both fields have physical relief"),
			DeviationA > ChangedPointToleranceCm && DeviationB > ChangedPointToleranceCm)) return;
		double UnitShapeDifference = 0.0;
		for (int32 I = 0; I < A.Values.Num(); ++I)
			UnitShapeDifference += FMath::Square((A.Values[I] - MeanA) / DeviationA
				- (B.Values[I] - MeanB) / DeviationB);
		const double Rms = FMath::Sqrt(SquaredDifference / A.Values.Num());
		const double ChangedFraction = double(Changed) / A.Values.Num();
		const double ShapeRms = FMath::Sqrt(UnitShapeDifference / A.Values.Num());
		Test.AddInfo(FString::Printf(TEXT("%s: height RMS=%.6g cm, changed=%.3f, unit-shape RMS=%.6g"),
			*Label, Rms, ChangedFraction, ShapeRms));
		Test.TestTrue(Label + TEXT(" physical height RMS differs by at least 1 m"), Rms >= MinimumPairRmsCm);
		Test.TestTrue(Label + TEXT(" at least 25 percent differ by more than 1 cm"), ChangedFraction >= MinimumChangedFraction);
		Test.TestTrue(Label + TEXT(" shape is not merely an offset or amplitude change"), ShapeRms >= MinimumUnitShapeRms);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCryogenicPhysicalTypeSeparation,
	"APS.Gameplay.World.PlanetSurface.CryogenicGeometry.PhysicalTypeSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCryogenicPhysicalTypeSeparation::RunTest(const FString& Parameters)
{
	using namespace APSPlanetCryogenicGeometryTests;
	FFixture Fixture;
	if (!TestTrue(TEXT("Isolated CPU fixture"), Fixture.IsValid())) return false;
	const auto* SavedCatalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	if (!TestNotNull(TEXT("Saved surface catalog"), SavedCatalog)) return false;
	const UAPSPlanetSurfaceCatalog* Catalogs[] = {nullptr, SavedCatalog};
	for (const auto* Catalog : Catalogs)
	for (int32 Seed : Seeds)
	for (double RadiusKm : RadiiKm)
	{
		Fixture.SetPhysicalBody(Seed, RadiusKm);
		const TArray<FVector> Directions = MakeDirections(RadiusKm * 100000.0);
		TArray<FHeights> Samples;
		for (EPlanetType Type : Types)
		{
			const auto Profile = Fixture.Resolve(Type, Catalog);
			Samples.Add(Sample(Fixture, Profile, Directions, RadiusKm * 100000.0, false));
		}
		for (int32 A = 0; A < UE_ARRAY_COUNT(Types); ++A)
		for (int32 B = A + 1; B < UE_ARRAY_COUNT(Types); ++B)
		{
			const FString Label = FString::Printf(TEXT("%s seed=%d radius=%.4f km %s/%s"),
				Catalog ? TEXT("catalog") : TEXT("native"), Seed, RadiusKm,
				*UEnum::GetValueAsString(Types[A]), *UEnum::GetValueAsString(Types[B]));
			CheckPair(*this, Label, Samples[A], Samples[B]);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCryogenicReconfigureHeightParity,
	"APS.Gameplay.World.PlanetSurface.CryogenicGeometry.ReconfigureHeightParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCryogenicReconfigureHeightParity::RunTest(const FString& Parameters)
{
	using namespace APSPlanetCryogenicGeometryTests;
	FFixture Fixture;
	if (!TestTrue(TEXT("Isolated CPU fixture"), Fixture.IsValid())) return false;
	const auto* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
	if (!TestNotNull(TEXT("Saved surface catalog"), Catalog)) return false;
	for (int32 Seed : Seeds)
	for (double RadiusKm : RadiiKm)
	{
		Fixture.SetPhysicalBody(Seed, RadiusKm);
		const TArray<FVector> Directions = MakeDirections(RadiusKm * 100000.0);
		auto SampleCurrentType = [&](EPlanetType Type)
		{
			const auto Profile = Fixture.Resolve(Type, Catalog);
			FHeights Heights = Sample(Fixture, Profile, Directions, RadiusKm * 100000.0, true);
			const FString Label = FString::Printf(TEXT("seed=%d radius=%.4f km %s"),
				Seed, RadiusKm, *UEnum::GetValueAsString(Type));
			TestTrue(Label + TEXT(" all height paths are finite"), Heights.bFinite);
			TestTrue(FString::Printf(TEXT("%s full/resolved/height-only/collision parity <= 1e-6 cm (%.9g)"),
				*Label, Heights.MaxParityErrorCm), Heights.MaxParityErrorCm <= ParityToleranceCm);
			return Heights;
		};
		const FHeights InitialIce = SampleCurrentType(EPlanetType::Ice);
		for (EPlanetType Other : {EPlanetType::Frozen, EPlanetType::Tundra, EPlanetType::Rogue})
		{
			const FHeights Changed = SampleCurrentType(Other);
			const FString Label = FString::Printf(TEXT("seed=%d radius=%.4f km Ice -> %s -> Ice"),
				Seed, RadiusKm, *UEnum::GetValueAsString(Other));
			CheckPair(*this, Label, InitialIce, Changed);
			const FHeights ReturnedIce = SampleCurrentType(EPlanetType::Ice);
			TestTrue(Label + TEXT(" restores every signed physical height exactly"),
				InitialIce.bFinite && ReturnedIce.bFinite && InitialIce.Values == ReturnedIce.Values);
			TestEqual(Label + TEXT(" keeps body seed"), Fixture.Planet->WorldScapeSeed, Seed);
			TestEqual(Label + TEXT(" keeps physical body radius"), Fixture.Planet->RadiusKM, RadiusKm);
		}
	}
	// Deliberately no render/UI publication claim: this fixture never ticks a
	// WorldScape root, loads a menu map, cooks collision or inspects a live editor.
	return true;
}

#endif
