#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Rendering/APSGameplayStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSStellarViewOptics.h"
#include <limits>

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
	// Optical carrier identity is explicit data, not a screen derivative. Both
	// warm and white luminous stars can sparkle; ordinary points retain their core.
	using namespace APSStellarOpticalSupport;
	TArray<float> White{1.0f, 0.94f, 0.85f, 20.0f, 0.034f, 0.0f};
	TArray<float> Warm{1.0f, 0.30f, 0.18f, 20.0f, 0.034f, 0.0f};
	const FProfile WhiteProfile = Select(White, 6, 0);
	const FProfile WarmProfile = Select(Warm, 6, 0);
	TestTrue(TEXT("A bright white point can receive a long carrier"), WhiteProfile.SupportPixels >= 8.0);
	TestTrue(TEXT("A bright red point is not categorically denied rays"), WarmProfile.SupportPixels >= 8.0);
	TArray<float> Faint = Warm;
	Faint[3] = 0.001f;
	TestEqual(TEXT("A faint cool point retains compact support"), Select(Faint, 6, 0).SupportPixels, CompactSupportPixels);
	TestEqual(TEXT("Malformed data cannot allocate an oversized carrier"), Select(Faint, 5, 0).SupportPixels, CompactSupportPixels);
	int32 Selected = 0;
	for (int32 Seed = 0; Seed < 1000; ++Seed)
	{
		Warm[4] = Seed / 1000.0f;
		Selected += Select(Warm, 6, 0).RayStrength > 0.0f;
	}
	TestTrue(TEXT("Crosses form a sparse stable subset rather than every star"), Selected > 20 && Selected < 350);
	const double PixelWorld = 100.0;
	const double UnresolvedCore = CoreRadius(1.0, PixelWorld);
	const double UnresolvedCarrier = CarrierRadius(1.0, PixelWorld, WhiteProfile);
	TestTrue(TEXT("Explicit ratio preserves unresolved core while enlarging ray coverage"),
		FMath::IsNearlyEqual(UnresolvedCarrier / CoreScale(UnresolvedCore, UnresolvedCarrier), UnresolvedCore, 1.0e-3));
	const double ResolvedPhysicalRadius = 3000.0;
	const double ResolvedCore = CoreRadius(ResolvedPhysicalRadius, PixelWorld);
	const double ResolvedCarrier = CarrierRadius(ResolvedPhysicalRadius, PixelWorld, WhiteProfile);
	TestEqual(TEXT("A physically resolved sphere keeps its real radius"), ResolvedCore, ResolvedPhysicalRadius);
	TestEqual(TEXT("A large physical sphere is never inferred to be a long-ray carrier"), CoreScale(ResolvedCore, ResolvedCarrier), 1.0f);
	TestEqual(TEXT("Close resolved stars do not carry optical crosses"), ResolvedRayStrength(WhiteProfile, ResolvedPhysicalRadius, PixelWorld), 0.0f);
	TestEqual(TEXT("A fully handed-off point has a safe neutral scale"), CoreScale(0.0, 0.0), 1.0f);
	UInstancedStaticMeshComponent* Legacy = NewObject<UInstancedStaticMeshComponent>();
	Legacy->SetNumCustomDataFloats(6);
	Legacy->AddInstance(FTransform::Identity);
	Legacy->SetCustomData(0, TArrayView<const float>(White), false);
	TestTrue(TEXT("Legacy six-float data can be upgraded"), EnsureLayout(Legacy));
	for (int32 Field = 0; Field < 6; ++Field)
		TestEqual(TEXT("Optical layout preserves each legacy field exactly"), Legacy->PerInstanceSMCustomData[Field], White[Field]);
	TestEqual(TEXT("Legacy luminosity is neutral"), Legacy->PerInstanceSMCustomData[6], 1.0f);
	TestEqual(TEXT("Legacy core scale is neutral"), Legacy->PerInstanceSMCustomData[CoreScaleIndex], 1.0f);
	TestEqual(TEXT("Legacy ray strength starts safely disabled"), Legacy->PerInstanceSMCustomData[RayStrengthIndex], 0.0f);
	TestTrue(TEXT("Explicit profile is published"), Publish(Legacy, 0, 4.0f, 0.7f));
	TestTrue(TEXT("Rechecking layout does not reset current optical data"), EnsureLayout(Legacy));
	TestEqual(TEXT("Published core ratio survives"), Legacy->PerInstanceSMCustomData[CoreScaleIndex], 4.0f);
	UInstancedStaticMeshComponent* ProjectedLegacy = NewObject<UInstancedStaticMeshComponent>();
	ProjectedLegacy->SetNumCustomDataFloats(11);
	ProjectedLegacy->AddInstance(FTransform::Identity);
	TArray<float> ProjectedRow{1.0f, 0.6f, 0.3f, 12.0f, 0.034f, 0.0f, 0.75f, -0.3f, 0.2f, 71.0f, 86.0f};
	ProjectedLegacy->SetCustomData(0, TArrayView<const float>(ProjectedRow), false);
	TestTrue(TEXT("Existing projection channels can be upgraded"), EnsureLayout(ProjectedLegacy));
	for (int32 Field = 0; Field < ProjectedRow.Num(); ++Field)
		TestEqual(TEXT("Projection and luminosity channels survive exactly"),
			ProjectedLegacy->PerInstanceSMCustomData[Field], ProjectedRow[Field]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSStellarProjectionPixelOpticsTest,
	"APS.Gameplay.Stellar.ProjectionPixelOptics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSStellarProjectionPixelOpticsTest::RunTest(const FString& Parameters)
{
	const double AuthoredAspect = 16.0 / 9.0;
	const double TanHalfHorizontal = FMath::Tan(FMath::DegreesToRadians(55.0 * 0.5));
	const double TanHalfVertical = TanHalfHorizontal / AuthoredAspect;
	const auto MakePerspective = [](const FIntRect& Rect, double TanHalfY)
	{
		FMatrix Matrix = FMatrix::Identity;
		Matrix.M[0][0] = 1.0 / (TanHalfY * Rect.Width() / Rect.Height());
		Matrix.M[1][1] = 1.0 / TanHalfY;
		Matrix.M[2][3] = 1.0;
		Matrix.M[3][3] = 0.0;
		return Matrix;
	};
	struct FFixture { const TCHAR* Name; FIntRect Rect; };
	const FFixture Fixtures[] = {
		{TEXT("16:9"), FIntRect(0, 0, 1920, 1080)},
		{TEXT("Ultrawide same height"), FIntRect(0, 0, 3440, 1080)},
		{TEXT("Actual ultrawide viewport"), FIntRect(0, 0, 3432, 1332)},
		{TEXT("4:3 same height"), FIntRect(0, 0, 1440, 1080)},
		{TEXT("Letterboxed constrained rectangle"), FIntRect(440, 0, 3000, 1440)},
		{TEXT("Constrained sub-view with nonzero origin"), FIntRect(120, 180, 2040, 1260)}
	};
	for (const FFixture& Fixture : Fixtures)
	{
		const FMatrix Projection = MakePerspective(Fixture.Rect, TanHalfVertical);
		double Tangent = 0.0;
		TestTrue(FString::Printf(TEXT("%s accepts the rendered perspective"), Fixture.Name),
			APSStellarViewOptics::TryPixelTangent(Projection, Fixture.Rect, Tangent));
		const double Expected = 2.0 * TanHalfVertical / Fixture.Rect.Height();
		TestTrue(FString::Printf(TEXT("%s honours MaintainYFOV and the constrained height"), Fixture.Name),
			FMath::IsNearlyEqual(Tangent, Expected, 1.0e-12));
		const double FocalX = 0.5 * Fixture.Rect.Width() * Projection.M[0][0];
		const double FocalY = 0.5 * Fixture.Rect.Height() * Projection.M[1][1];
		for (double Support : {2.2, 9.5, 14.0})
		{
			const double OpticalRadius = APSGameplayStellarProjection::GetFullScalePointRadius(
				1.0e10, 1.0, Tangent, Support);
			TestTrue(TEXT("Carrier retains its requested native pixels on both axes"),
				FMath::IsNearlyEqual(OpticalRadius / 1.0e10 * FocalX, Support, 1.0e-9)
				&& FMath::IsNearlyEqual(OpticalRadius / 1.0e10 * FocalY, Support, 1.0e-9));
		}
		FAPSPreviewProjectedSphere Sphere;
		double OpticalRadius = 0.0;
		TestTrue(TEXT("Projection optics retain the same physical sphere"),
			APSGameplayStellarProjection::Project(FVector(1.0e10, 0, 0), 1000.0,
				Tangent, Sphere, OpticalRadius)
			&& FMath::IsNearlyEqual(Sphere.Radius / Sphere.Center.Size(), 1.0e-7, 1.0e-16));
	}
	const FIntRect WideRect(0, 0, 3432, 1332);
	FMatrix WideProjection = MakePerspective(WideRect, TanHalfVertical);
	double WideTangent = 0.0;
	APSStellarViewOptics::TryPixelTangent(WideProjection, WideRect, WideTangent);
	const double LegacyTangent = 2.0 * TanHalfHorizontal / WideRect.Width();
	TestTrue(TEXT("Ultrawide regression: horizontal-FOV shortcut undersized the carrier by 31 percent"),
		FMath::IsNearlyEqual(LegacyTangent / WideTangent,
			WideRect.Height() * AuthoredAspect / WideRect.Width(), 1.0e-12)
		&& LegacyTangent / WideTangent < 0.70);
	// MaintainXFOV cameras are equally valid; the helper makes no policy guess.
	const FMatrix HorizontalProjection = MakePerspective(WideRect,
		TanHalfHorizontal / (double(WideRect.Width()) / WideRect.Height()));
	double HorizontalTangent = 0.0;
	TestTrue(TEXT("Actual horizontal-FOV projection preserves the old tangent when appropriate"),
		APSStellarViewOptics::TryPixelTangent(HorizontalProjection, WideRect, HorizontalTangent)
		&& FMath::IsNearlyEqual(HorizontalTangent, LegacyTangent, 1.0e-12));
	WideProjection.M[2][0] = 0.17;
	WideProjection.M[2][1] = -0.12;
	double ShiftedTangent = 0.0;
	TestTrue(TEXT("An off-centre projection does not change optical pixel scale"),
		APSStellarViewOptics::TryPixelTangent(WideProjection, WideRect, ShiftedTangent)
		&& FMath::IsNearlyEqual(ShiftedTangent, WideTangent, 1.0e-12));
	TestFalse(TEXT("Empty constrained view is rejected"),
		APSStellarViewOptics::TryPixelTangent(WideProjection, FIntRect(0, 0, 0, 0), ShiftedTangent));
	TestFalse(TEXT("Orthographic projection is not silently treated as perspective"),
		APSStellarViewOptics::TryPixelTangent(FMatrix::Identity, WideRect, ShiftedTangent));
	WideProjection.M[0][0] = 0.0;
	TestFalse(TEXT("Zero focal length is rejected"),
		APSStellarViewOptics::TryPixelTangent(WideProjection, WideRect, ShiftedTangent));
	WideProjection.M[0][0] = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Nonfinite projection is rejected"),
		APSStellarViewOptics::TryPixelTangent(WideProjection, WideRect, ShiftedTangent));
	TestEqual(TEXT("No local view preserves the caller's previous valid fallback"),
		APSStellarViewOptics::PixelTangent(nullptr, LegacyTangent), LegacyTangent);
	TestEqual(TEXT("Invalid fallback cannot inject nonfinite instance scales"),
		APSStellarViewOptics::PixelTangent(nullptr, std::numeric_limits<double>::quiet_NaN()), 0.0);
	return true;
}
#endif
