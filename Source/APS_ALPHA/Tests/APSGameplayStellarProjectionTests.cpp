#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Rendering/APSGameplayStellarProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGameplayStellarProjectionTest,
	"APS.Gameplay.Stellar.PhysicalObserverProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGameplayStellarProjectionTest::RunTest(const FString& Parameters)
{
	const double PixelTangent = 2.0 / 2048.0;
	for (double Distance : {1.0e10, 1.0e13, 1.0e16, 1.0e20})
	{
		const FVector Position = FVector(1.0, 0.25, -0.15).GetSafeNormal() * Distance;
		FAPSPreviewProjectedSphere Sphere;
		double OpticalRadius = 0.0;
		TestTrue(TEXT("Physical star can be projected"), APSGameplayStellarProjection::Project(
			Position, 1000.0, PixelTangent, Sphere, OpticalRadius));
		TestTrue(TEXT("Catalog direction and formation are preserved"),
			Sphere.Center.GetSafeNormal().Equals(Position.GetSafeNormal(), 1.0e-12));
		TestTrue(TEXT("Projected centers stay inside the render envelope"),
			Sphere.Center.Size() <= APSGameplayStellarProjection::FarEnvelopeCm);
		TestTrue(TEXT("Physical angular radius is preserved"),
			FMath::IsNearlyEqual(Sphere.Radius / Sphere.Center.Size(), 1000.0 / Distance, 1.0e-18));
		TestTrue(TEXT("An unresolved star has the same bounded optical support at every distance"),
			FMath::IsNearlyEqual(OpticalRadius / (Sphere.Center.Size() * PixelTangent), 2.2, 1.0e-9));
	}
	FAPSPreviewOccluder Star;
	Star.Center = FVector(1.0e13, 0, 0);
	Star.Radius = 1.0e11;
	TArray<FAPSPreviewOccluder> Occluders{Star};
	TestTrue(TEXT("An opaque star blocks a physical background point even when the render proxy is closer"),
		APSGameplayStellarProjection::IsOccluded(FVector(1.0e17, 0, 0), Occluders));
	TestFalse(TEXT("A point physically in front of the star stays visible"),
		APSGameplayStellarProjection::IsOccluded(FVector(1.0e12, 0, 0), Occluders));
	TestFalse(TEXT("A point outside the stellar limb stays visible"),
		APSGameplayStellarProjection::IsOccluded(FVector(1.0e17, 2.0e15, 0), Occluders));
	const APSGameplayStellarProjection::FPreparedOccluder Prepared(Star);
	FRandomStream Random(417);
	for (int32 Index = 0; Index < 512; ++Index)
	{
		const FVector Point(FMath::Pow(10.0, Random.FRandRange(10.0, 18.0)),
			Random.FRandRange(-1.0e12, 1.0e12), Random.FRandRange(-1.0e12, 1.0e12));
		TestEqual(TEXT("Prepared occlusion retains physical foreground/background classification"),
			Prepared.Occludes(Point.GetSafeNormal(), Point.Size()), Star.Occludes(Point));
	}
	TestFalse(TEXT("An uninitialized view must be projected"),
		APSGameplayStellarProjection::CanReuseProjection(0.0, 0.0, PixelTangent));
	TestTrue(TEXT("Walking does not rebuild a distant stellar catalog"),
		APSGameplayStellarProjection::CanReuseProjection(1.0e6, 1.0e17, PixelTangent));
	TestFalse(TEXT("Observable interstellar parallax invalidates the cache"),
		APSGameplayStellarProjection::CanReuseProjection(1.0e14, 1.0e17, PixelTangent));
	TestFalse(TEXT("Approaching or crossing the closest point forces a rebuild"),
		APSGameplayStellarProjection::CanReuseProjection(1.0e17, 1.0e17, PixelTangent));
	TArray<FAPSPreviewOccluder> Moved = Occluders;
	Moved[0].Center.Y += 1.0e6;
	TestTrue(TEXT("Sub-pixel limb movement reuses a distant occlusion mask"),
		APSGameplayStellarProjection::CanReuseOcclusion(Occluders, Moved, 1.0e17, PixelTangent));
	TestFalse(TEXT("A nearby foreground star prevents approximate occlusion reuse"),
		APSGameplayStellarProjection::CanReuseOcclusion(Occluders, Moved, 1.0e12, PixelTangent));
	Moved[0].Center.Y += 1.0e10;
	TestFalse(TEXT("Observable limb movement invalidates occlusion"),
		APSGameplayStellarProjection::CanReuseOcclusion(Occluders, Moved, 1.0e17, PixelTangent));
	Moved = Occluders;
	Moved[0].Radius *= 1.1;
	TestFalse(TEXT("A changed angular radius invalidates occlusion"),
		APSGameplayStellarProjection::CanReuseOcclusion(Occluders, Moved, 1.0e17, PixelTangent));
	Moved.Reset();
	TestFalse(TEXT("Removed bodies invalidate occlusion"),
		APSGameplayStellarProjection::CanReuseOcclusion(Occluders, Moved, 1.0e17, PixelTangent));
	TestTrue(TEXT("A sub-pixel camera FOV change does not rebuild the catalog"),
		APSGameplayStellarProjection::CanReuseOptics(PixelTangent, PixelTangent * 1.001));
	TestFalse(TEXT("A meaningful camera FOV change refreshes point support"),
		APSGameplayStellarProjection::CanReuseOptics(PixelTangent, PixelTangent * 1.1));
	TestTrue(TEXT("A 100-metre movement can retain the bounded render anchor"),
		APSGameplayStellarProjection::CanReuseProjection(1.0e4, 1.0e9, PixelTangent));
	TestFalse(TEXT("Accumulated render-anchor error eventually forces recentering"),
		APSGameplayStellarProjection::CanReuseProjection(1.0e6, 1.0e9, PixelTangent));
	TestFalse(TEXT("An unchanged visible point is not uploaded on an occlusion-only refresh"),
		APSGameplayStellarProjection::NeedsInstanceUpload(false, false, false));
	TestFalse(TEXT("An unchanged hidden point is not uploaded even on reprojection"),
		APSGameplayStellarProjection::NeedsInstanceUpload(true, true, true));
	TestTrue(TEXT("A point disappearing behind a limb is uploaded"),
		APSGameplayStellarProjection::NeedsInstanceUpload(false, false, true));
	TestTrue(TEXT("A point emerging from behind a limb is uploaded"),
		APSGameplayStellarProjection::NeedsInstanceUpload(false, true, false));
	TestTrue(TEXT("A reprojected visible point is uploaded"),
		APSGameplayStellarProjection::NeedsInstanceUpload(true, false, false));
	FAPSPreviewProjectedSphere Sphere;
	double OpticalRadius = 0.0;
	TestFalse(TEXT("Invalid optics are rejected"),
		APSGameplayStellarProjection::Project(FVector(100,0,0), 1.0, 0.0, Sphere, OpticalRadius));
	return true;
}
#endif
