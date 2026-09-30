#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSBandFlightModelTest,
	"APS.Gameplay.Vehicle.BandFlightModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSBandFlightModelTest::RunTest(const FString& Parameters)
{
	using namespace APSFlightBandModel;
	constexpr double Dt = 1.0 / 60.0;
	constexpr double DistanceFloorCm = 50000.0;
	constexpr double SurfaceFactor = 2.0;
	constexpr double SurfaceFloorCm = 25000.0;
	const FAPSFlightBandSettings Maneuver = UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand::Maneuver);
	const FAPSFlightBandSettings Flight = UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand::Flight);
	const FAPSFlightBandSettings Orbital = UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand::Orbital);
	const FAPSFlightBandSettings Cruise = UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand::Cruise);
	const FAPSFlightBandSettings Stellar = UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand::Stellar);

	TestTrue(TEXT("bands step up drastically"), Maneuver.MaxSpeed < Flight.MaxSpeed && Flight.MaxSpeed < Orbital.MaxSpeed
		&& Orbital.MaxSpeed < Cruise.MaxSpeed && Cruise.MaxSpeed < Stellar.MaxSpeed);
	TestTrue(TEXT("every band has its own boost"), Maneuver.BoostMultiplier > 1.0 && Stellar.BoostMultiplier > 1.0);
	TestTrue(TEXT("bands use hover, drive, assist and cruise control"), Maneuver.Control == EAPSFlightBandControl::Hover
		&& Flight.Control == EAPSFlightBandControl::Drive && Orbital.Control == EAPSFlightBandControl::Assist
		&& Cruise.Control == EAPSFlightBandControl::Cruise && Stellar.Control == EAPSFlightBandControl::Cruise);
	TestTrue(TEXT("small hulls are nimbler than titans"), ClassAgility(0) > ClassAgility(3) && ClassAgility(3) > ClassAgility(7));

	// Hover: held keys reach the limit; released keys stop the ship.
	const double HoverLimit = Maneuver.MaxSpeed * 100.0;
	const double HoverAcceleration = Maneuver.Acceleration * 100.0;
	FVector Hover = FVector::ZeroVector;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		Hover = HoverStep(Hover, FVector::ForwardVector * HoverLimit, HoverAcceleration, Dt);
	}
	TestTrue(TEXT("hover reaches the Maneuver limit"), FMath::IsNearlyEqual(Hover.X, HoverLimit, 1.0));
	for (int32 Frame = 0; Frame < 180; ++Frame)
	{
		Hover = HoverStep(Hover, FVector::ZeroVector, HoverAcceleration, Dt);
	}
	TestTrue(TEXT("released hover keys stop the ship within 3 s"), Hover.IsNearlyZero(1.0));

	// Drive: full thrust from rest, less near the limit; releasing W slows the ship like a car.
	const double FlightLimit = Flight.MaxSpeed * 100.0;
	const double FlightAcceleration = Flight.Acceleration * 100.0;
	double DriveSpeed = 0.0;
	DriveSpeed = TaperedSpeedStep(DriveSpeed, FlightLimit, FlightAcceleration, Dt);
	TestTrue(TEXT("drive starts with the full thrust"), FMath::IsNearlyEqual(DriveSpeed / Dt, FlightAcceleration, FlightAcceleration * 0.01));
	const double HalfSpeedGain = TaperedSpeedStep(FlightLimit * 0.5, FlightLimit, FlightAcceleration, Dt) - FlightLimit * 0.5;
	TestTrue(TEXT("drive keeps most of its thrust at half speed"), HalfSpeedGain > FlightAcceleration * Dt * 0.7);
	for (int32 Frame = 1; Frame < 300; ++Frame)
	{
		DriveSpeed = TaperedSpeedStep(DriveSpeed, FlightLimit, FlightAcceleration, Dt);
	}
	TestTrue(TEXT("drive reaches 90% of the Flight limit within 5 s"), DriveSpeed > FlightLimit * 0.9);
	for (int32 Frame = 300; Frame < 600; ++Frame)
	{
		DriveSpeed = TaperedSpeedStep(DriveSpeed, FlightLimit, FlightAcceleration, Dt);
	}
	const double NearLimitGain = TaperedSpeedStep(DriveSpeed, FlightLimit, FlightAcceleration, Dt) - DriveSpeed;
	TestTrue(TEXT("drive thrust fades near the limit"), NearLimitGain < FlightAcceleration * Dt * 0.2);
	TestTrue(TEXT("drive stays below the limit"), DriveSpeed < FlightLimit && DriveSpeed > FlightLimit * 0.8);
	TestTrue(TEXT("a speed above the target is kept (shed separately)"),
		FMath::IsNearlyEqual(TaperedSpeedStep(FlightLimit * 2.0, FlightLimit, FlightAcceleration, Dt), FlightLimit * 2.0));
	TestTrue(TEXT("W against a backward drift pushes with the full thrust"),
		FMath::IsNearlyEqual(TaperedSpeedStep(-1000.0, FlightLimit, FlightAcceleration, Dt), -1000.0 + FlightAcceleration * Dt));
	{
		const double OrbitalLimit = Orbital.MaxSpeed * 100.0;
		double OrbitalSpeed = 0.0;
		for (int32 Frame = 0; Frame < 330; ++Frame)
		{
			OrbitalSpeed = TaperedSpeedStep(OrbitalSpeed, OrbitalLimit, Orbital.Acceleration * 100.0, Dt);
		}
		TestTrue(TEXT("Orbital reaches 90% of its open-space limit within 5.5 s"), OrbitalSpeed > OrbitalLimit * 0.9);
	}
	const double Released = DriveSpeed * FMath::Exp(-Flight.ReleaseDrag);
	TestTrue(TEXT("one second without W loses the release drag share"), Released < DriveSpeed * 0.6);

	// Assist: the velocity turns toward the nose and keeps its magnitude; backward motion aligns with -nose.
	const FVector Sideways(0.0, 50000.0, 0.0);
	FVector Aligned = Sideways;
	for (int32 Frame = 0; Frame < 180; ++Frame)
	{
		Aligned = AlignToNose(Aligned, FVector::ForwardVector, Orbital.AssistRate, Dt);
	}
	TestTrue(TEXT("assist keeps the speed while turning it"), FMath::IsNearlyEqual(Aligned.Size(), Sideways.Size(), 1.0));
	TestTrue(TEXT("assist turns the velocity to the nose within 3 s"),
		FVector::DotProduct(Aligned.GetSafeNormal(), FVector::ForwardVector) > 0.95);
	const FVector Reversing = AlignToNose(FVector(-1000.0, 300.0, 0.0), FVector::ForwardVector, 2.0, Dt);
	TestTrue(TEXT("backward motion aligns with the tail, not through a turn"), Reversing.X < 0.0);

	// Limits.
	TestTrue(TEXT("unknown distance keeps the band limit"),
		FMath::IsNearlyEqual(SpeedLimitCm(Orbital, 1.0, -1.0, -1.0, DistanceFloorCm, SurfaceFactor, SurfaceFloorCm),
			Orbital.MaxSpeed * 100.0));
	TestTrue(TEXT("Orbital at 100 km = 50 km/s"),
		FMath::IsNearlyEqual(SpeedLimitCm(Orbital, 1.0, 1.0e7, -1.0, DistanceFloorCm, SurfaceFactor, SurfaceFloorCm), 5.0e6));
	TestTrue(TEXT("distance limit keeps its floor on the ground"),
		FMath::IsNearlyEqual(SpeedLimitCm(Orbital, 1.0, 1000.0, 1000.0, DistanceFloorCm, SurfaceFactor, SurfaceFloorCm),
			DistanceFloorCm));
	TestTrue(TEXT("Flight at 100 m clearance = surface floor"),
		FMath::IsNearlyEqual(SpeedLimitCm(Flight, 1.0, -1.0, 10000.0, DistanceFloorCm, SurfaceFactor, SurfaceFloorCm),
			SurfaceFloorCm));
	TestTrue(TEXT("Flight high above the ground = band limit"),
		FMath::IsNearlyEqual(SpeedLimitCm(Flight, 1.0, -1.0, 1.0e7, DistanceFloorCm, SurfaceFactor, SurfaceFloorCm),
			Flight.MaxSpeed * 100.0));
	TestTrue(TEXT("boost raises the band limit"),
		FMath::IsNearlyEqual(SpeedLimitCm(Maneuver, Maneuver.BoostMultiplier, -1.0, -1.0, DistanceFloorCm, 0.0, SurfaceFloorCm),
			Maneuver.MaxSpeed * 100.0 * Maneuver.BoostMultiplier));

	// Band drop: from cruise speed to Maneuver in about 1.5 s, without a one-frame cut.
	double Speed = 1.0e11;
	TestTrue(TEXT("band drop is not a one-frame cut"), ShedOverspeed(Speed, 6000.0, 5.0, Dt) > 6000.0 * 2.0);
	for (int32 Frame = 0; Frame < 90; ++Frame)
	{
		Speed = ShedOverspeed(Speed, 6000.0, 5.0, Dt);
	}
	TestTrue(TEXT("band drop is within 2% of the new limit after 1.5 s"), Speed >= 6000.0 && Speed < 6120.0);

	// Course geometry: STELLAR sees a star only when its course runs into the star's system.
	{
		const FVector Ahead = FVector::ForwardVector;
		TestTrue(TEXT("inside a system the course clearance is zero"),
			CourseClearanceCm(FVector(50.0, 0.0, 0.0), Ahead, 100.0, 1000.0) == 0.0);
		TestTrue(TEXT("a system behind does not limit"), CourseClearanceCm(FVector(-500.0, 0.0, 0.0), Ahead, 100.0, 1000.0) < 0.0);
		TestTrue(TEXT("a system dead ahead is reached at its edge"),
			FMath::IsNearlyEqual(CourseClearanceCm(FVector(1000.0, 0.0, 0.0), Ahead, 100.0, 1000.0), 900.0, 1.0e-6));
		TestTrue(TEXT("the course clearance is continuous at the rim of the system"),
			FMath::Abs(CourseClearanceCm(FVector(1000.0, 100.0 - 1.0e-7, 0.0), Ahead, 100.0, 1000.0)
				- CourseClearanceCm(FVector(1000.0, 100.0 + 1.0e-7, 0.0), Ahead, 100.0, 1000.0)) < 0.01);
		TestTrue(TEXT("a system well off the course hardly counts"),
			CourseClearanceCm(FVector(1000.0, 300.0, 0.0), Ahead, 100.0, 1000.0) > 100000.0);
		TestTrue(TEXT("the course leaves charted space at its far side"),
			FMath::IsNearlyEqual(CourseExitCm(FVector(50.0, 0.0, 0.0), Ahead, 100.0), 50.0, 1.0e-6));
		TestTrue(TEXT("beyond the edge nothing is left"), CourseExitCm(FVector(150.0, 0.0, 0.0), Ahead, 100.0) == 0.0);
		TestTrue(TEXT("cruise control grows the speed by a bounded factor"),
			FMath::IsNearlyEqual(CapSpeedGrowth(1.0e6, 1.0e9, 0.6, 3.0e6, 1.0), 4.0e6)
			&& FMath::IsNearlyEqual(CapSpeedGrowth(1.0e8, 1.0e12, 0.6, 3.0e6, 1.0), 1.0e8 * FMath::Exp(0.6)));
		TestTrue(TEXT("slowing down is never capped"), CapSpeedGrowth(1.0e9, 1.0e6, 0.6, 3.0e6, 1.0) == 1.0e6);
	}

	// CRUISE at the scale of a star system, around a planet 1 AU from its sun. The steps are the ones ApplyTranslation
	// takes: cruise control, the growth cap, the shed and the closing guard.
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	constexpr double SpeedOfLightCm = 2.99792458e10;
	const double Departure = GetDefault<UAPSShipFlightModel>()->DepartureSpeedFactor;
	TestTrue(TEXT("leaving a body is faster than approaching it"), Departure > 1.0);
	const auto CruiseLimit = [&](double Boost, double SurfaceCm, double StarCm)
	{
		FSpeedLimitInputs Inputs;
		Inputs.SurfaceCm = SurfaceCm;
		Inputs.StarCm = StarCm;
		Inputs.DistanceFloorCm = DistanceFloorCm;
		return BandSpeedLimitCm(Cruise, Boost, Inputs, -1.0);
	};
	const auto CruiseStep = [&](double Speed, double Limit, double Boost, double Step)
	{
		const double Next = CapSpeedGrowth(Speed, CruiseSpeedStep(Speed, 1.0, Limit, Cruise.CruiseResponse * Boost, 1.5, 0.0, Step),
			Cruise.MaxLogAcceleration * FMath::Sqrt(Boost), Cruise.Acceleration * 100.0 * Boost, Step);
		return ShedOverspeed(Next, Limit, ShedRate(5.0, Cruise.DistanceSpeedFactor * Boost), Step);
	};
	TestTrue(TEXT("cruise flies about 60 c one AU from the sun"),
		FMath::IsWithin(CruiseLimit(1.0, -1.0, AstronomicalUnitCm) / SpeedOfLightCm, 50.0, 70.0));
	{
		// Leaving orbit (Rio, 30.09: the ship shot out of the system): the speed grows by a bounded factor per second.
		const auto Leave = [&](double Boost, double& OutMaxLogRate)
		{
			double Distance = 1.0e8;
			double Speed = 0.0;
			double Seconds = 0.0;
			OutMaxLogRate = 0.0;
			while (Distance < AstronomicalUnitCm && Seconds < 600.0)
			{
				const double Next = CruiseStep(Speed, CruiseLimit(Boost, Distance * Departure, AstronomicalUnitCm + Distance),
					Boost, Dt);
				if (Speed > 1.0e7 && Next > Speed)
				{
					OutMaxLogRate = FMath::Max(OutMaxLogRate, FMath::Loge(Next / Speed) / Dt);
				}
				Speed = Next;
				Distance += Speed * Dt;
				Seconds += Dt;
			}
			return Seconds;
		};
		double MaxLogRate = 0.0;
		double BoostedMaxLogRate = 0.0;
		const double Seconds = Leave(1.0, MaxLogRate);
		const double BoostedSeconds = Leave(Cruise.BoostMultiplier, BoostedMaxLogRate);
		TestTrue(TEXT("leaving orbit for 1 AU takes 15-40 s: no shot out of the system"), Seconds > 15.0 && Seconds < 40.0);
		TestTrue(TEXT("boost leaves for 1 AU in under 20 s"), BoostedSeconds < 20.0);
		TestTrue(TEXT("cruise speed grows no faster than its cap"),
			MaxLogRate <= Cruise.MaxLogAcceleration * 1.001
			&& BoostedMaxLogRate <= Cruise.MaxLogAcceleration * FMath::Sqrt(Cruise.BoostMultiplier) * 1.001);
		AddInfo(FString::Printf(TEXT("cruise 1000 km -> 1 AU: %.1f s, boosted %.1f s"), Seconds, BoostedSeconds));
	}
	{
		// Arriving at a planet from 1 AU and from beyond the planets (5 AU, where STELLAR hands over at home).
		const auto Arrive = [&](double Boost, double StartCm, bool bFromSystemEdge, bool& bOutPassedSurface)
		{
			double Distance = StartCm;
			const auto StarCm = [&]() { return bFromSystemEdge ? Distance + AstronomicalUnitCm : AstronomicalUnitCm; };
			double Speed = CruiseLimit(Boost, Distance, StarCm());
			double Seconds = 0.0;
			bOutPassedSurface = false;
			while (Distance > 1.0e8 && Seconds < 600.0)
			{
				Speed = GuardClosingSpeed(CruiseStep(Speed, CruiseLimit(Boost, Distance, StarCm()), Boost, Dt), Distance, Dt);
				Distance -= Speed * Dt;
				Seconds += Dt;
				bOutPassedSurface |= Distance < 0.0;
			}
			return Seconds;
		};
		bool bPassedSurface = false;
		const double Seconds = Arrive(1.0, AstronomicalUnitCm, false, bPassedSurface);
		TestFalse(TEXT("cruise never passes the surface"), bPassedSurface);
		TestTrue(TEXT("cruise covers 1 AU to 1000 km in under 40 s"), Seconds < 40.0);
		const double FromEdge = Arrive(1.0, 5.0 * AstronomicalUnitCm, true, bPassedSurface);
		const double BoostedFromEdge = Arrive(Cruise.BoostMultiplier, 5.0 * AstronomicalUnitCm, true, bPassedSurface);
		TestFalse(TEXT("boosted cruise from the edge never passes the surface"), bPassedSurface);
		TestTrue(TEXT("beyond the planets to a planet in under 40 s (12 s with boost)"),
			FromEdge < 40.0 && BoostedFromEdge < 12.0);
		AddInfo(FString::Printf(TEXT("cruise 1 AU -> 1000 km: %.1f s; 5 AU -> planet %.1f s, boosted %.1f s"), Seconds,
			FromEdge, BoostedFromEdge));
	}

	// STELLAR between the stars of the generated cluster, about 1 AU apart (30.09). Only the target system on the course
	// slows the ship, down to the CRUISE speed at its edge, so AUTO hands over without a jolt; CRUISE there is the floor.
	const double SystemCm = GetDefault<UAPSShipFlightModel>()->InterstellarDistanceAU * AstronomicalUnitCm;
	const auto StellarLimits = [&](double Alpha, double FromStarCm, double ToStarCm, double EdgeCm, double CourseCm,
		double& OutBelow)
	{
		// CRUISE sees every surface, the edge of charted space included; STELLAR only the course.
		const double StarCm = FMath::Min(FromStarCm, ToStarCm);
		OutBelow = CruiseLimit(1.0 + (Cruise.BoostMultiplier - 1.0) * Alpha,
			FMath::Min3(FromStarCm * Departure, ToStarCm, EdgeCm), StarCm);
		FSpeedLimitInputs Inputs;
		Inputs.StarCm = StarCm;
		Inputs.CourseCm = CourseCm;
		Inputs.DistanceFloorCm = DistanceFloorCm;
		return BandSpeedLimitCm(Stellar, 1.0 + (Stellar.BoostMultiplier - 1.0) * Alpha, Inputs, OutBelow);
	};
	const auto StellarStep = [&](double Speed, double Limit, double Alpha, double GapCm, double Step)
	{
		const double Boost = 1.0 + (Stellar.BoostMultiplier - 1.0) * Alpha;
		const double Next = CapSpeedGrowth(Speed, CruiseSpeedStep(Speed, 1.0, Limit, Stellar.CruiseResponse * Boost, 1.5, 0.0, Step),
			Stellar.MaxLogAcceleration * FMath::Sqrt(Boost), Stellar.Acceleration * 100.0 * Boost, Step);
		return GuardClosingSpeed(ShedOverspeed(Next, Limit, ShedRate(5.0, Stellar.DistanceSpeedFactor * Boost), Step), GapCm, Step);
	};
	const auto StellarTrip = [&](double Alpha, double Step, double GapAU, bool& bOutPassedStar, double& OutEntryRatio,
		double& OutMaxLogRate)
	{
		constexpr double StarRadiusCm = 6.957e10;
		const double Gap = GapAU * AstronomicalUnitCm;
		double Travelled = SystemCm;
		constexpr double NoEdge = TNumericLimits<double>::Max();
		double Below = 0.0;
		StellarLimits(Alpha, Travelled, Gap - Travelled, NoEdge, Gap - Travelled - SystemCm, Below);
		double Speed = Below;
		double Seconds = 0.0;
		bOutPassedStar = false;
		OutMaxLogRate = 0.0;
		while (Gap - Travelled > SystemCm && Seconds < 600.0)
		{
			const double Limit = StellarLimits(Alpha, Travelled, Gap - Travelled, NoEdge, Gap - Travelled - SystemCm, Below);
			const double Next = StellarStep(Speed, Limit, Alpha, Gap - Travelled - StarRadiusCm, Step);
			if (Next > Speed && Speed > 0.0)
			{
				OutMaxLogRate = FMath::Max(OutMaxLogRate, FMath::Loge(Next / Speed) / Step);
			}
			Speed = Next;
			Travelled += Speed * Step;
			Seconds += Step;
			bOutPassedStar |= Travelled > Gap;
		}
		StellarLimits(Alpha, Travelled, Gap - Travelled, NoEdge, 0.0, Below);
		OutEntryRatio = Speed / Below;
		return Seconds;
	};
	{
		bool bPassedStar = false;
		double EntryRatio = 0.0;
		double MaxLogRate = 0.0;
		const double Hop = StellarTrip(0.0, Dt, 1.0, bPassedStar, EntryRatio, MaxLogRate);
		TestFalse(TEXT("a hop to the next star never passes it"), bPassedStar);
		TestTrue(TEXT("stellar enters the target system at the cruise speed there"), EntryRatio < 1.1);
		TestTrue(TEXT("stellar speed grows no faster than its cap"), MaxLogRate <= Stellar.MaxLogAcceleration * 1.001);
		const double Across = StellarTrip(0.0, Dt, 20.0, bPassedStar, EntryRatio, MaxLogRate);
		TestFalse(TEXT("20 AU across the cluster never pass the target star"), bPassedStar);
		TestTrue(TEXT("a hop of 1 AU in under 20 s, 20 AU in under 30 s"), Hop < 20.0 && Across < 30.0);
		const double BoostedHop = StellarTrip(1.0, Dt, 1.0, bPassedStar, EntryRatio, MaxLogRate);
		const double BoostedAcross = StellarTrip(1.0, Dt, 20.0, bPassedStar, EntryRatio, MaxLogRate);
		TestFalse(TEXT("boosted stellar flight never passes the target star"), bPassedStar);
		TestTrue(TEXT("with boost 1 AU in under 7 s and 20 AU in under 10 s, entering at the cruise speed"),
			BoostedHop < 7.0 && BoostedAcross < 10.0 && EntryRatio < 1.1);
		StellarTrip(1.0, 0.4, 20.0, bPassedStar, EntryRatio, MaxLogRate);
		TestFalse(TEXT("0.4 s frames (the engine's longest) never carry a boosted ship through the star"), bPassedStar);
		TestTrue(TEXT("0.4 s frames still enter the system close to the cruise speed"), EntryRatio < 1.3);
		AddInfo(FString::Printf(TEXT("stellar 1 AU: %.1f s, 20 AU: %.1f s; boosted %.1f s, %.1f s"), Hop, Across, BoostedHop,
			BoostedAcross));
	}
	{
		// Rio, 30.09: at the edge of the cluster the ship ran up to 230 000 c and broke through. With no star ahead the
		// speed grows by a bounded factor and the edge of charted space (a little beyond the cluster) stops it.
		const auto ToEdge = [&](double Alpha, double Step, double& OutPeak)
		{
			constexpr double NoStar = TNumericLimits<double>::Max();
			const double Edge = 150.0 * AstronomicalUnitCm;
			double Travelled = SystemCm;
			double Below = 0.0;
			StellarLimits(Alpha, Travelled, NoStar, Edge - Travelled, Edge - Travelled, Below);
			double Speed = Below;
			OutPeak = Speed;
			for (double Seconds = 0.0; Seconds < 300.0; Seconds += Step)
			{
				const double Limit = StellarLimits(Alpha, Travelled, NoStar, Edge - Travelled, Edge - Travelled, Below);
				Speed = StellarStep(Speed, Limit, Alpha, Edge - Travelled, Step);
				Travelled += Speed * Step;
				OutPeak = FMath::Max(OutPeak, Speed);
			}
			return Edge - Travelled;
		};
		double Peak = 0.0;
		const double Left = ToEdge(0.0, Dt, Peak);
		TestTrue(TEXT("stellar stops at the edge of charted space"), Left >= 0.0 && Left < 1.0e9);
		TestTrue(TEXT("stellar stays below its band limit"), Peak <= Stellar.MaxSpeed * 100.0 * 1.001);
		double BoostedPeak = 0.0;
		TestTrue(TEXT("boosted 0.4 s frames never cross the edge"), ToEdge(1.0, 0.4, BoostedPeak) >= 0.0);
		TestTrue(TEXT("boosted stellar stays below its boosted limit"),
			BoostedPeak <= Stellar.MaxSpeed * 100.0 * Stellar.BoostMultiplier * 1.001);
	}
	TestTrue(TEXT("the shed keeps up with a boosted approach (rate above e x factor)"),
		ShedRate(5.0, Stellar.DistanceSpeedFactor * Stellar.BoostMultiplier) > UE_EULERS_NUMBER * Stellar.DistanceSpeedFactor
			* Stellar.BoostMultiplier);
	TestTrue(TEXT("the closing guard lets a frame take a quarter of the gap"),
		FMath::IsNearlyEqual(GuardClosingSpeed(1.0e20, 1000.0, 0.5), 500.0));

	// Cruise holds speed in space, drags in atmosphere, S bleeds it.
	TestTrue(TEXT("cruise holds speed without input in space"),
		FMath::IsNearlyEqual(CruiseSpeedStep(5.0e6, 0.0, 1.0e7, 0.8, 1.5, 0.0, Dt), 5.0e6));
	TestTrue(TEXT("cruise drags without input in atmosphere"), CruiseSpeedStep(5.0e6, 0.0, 1.0e7, 0.8, 1.5, 0.65, Dt) < 5.0e6);
	TestTrue(TEXT("S slows cruise"), CruiseSpeedStep(5.0e6, -1.0, 1.0e7, 0.8, 1.5, 0.0, Dt) < 5.0e6);

	// Brake: one second of Left Ctrl takes most of the speed.
	FVector Braked(100000.0, 0.0, 0.0);
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Braked = BrakeStep(Braked, 5000.0, 1.5, Dt);
	}
	TestTrue(TEXT("brake removes most speed in a second"), Braked.X < 30000.0 && Braked.X >= 0.0);
	return true;
}

#endif
