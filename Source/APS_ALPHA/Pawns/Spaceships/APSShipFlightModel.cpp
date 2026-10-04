#include "APSShipFlightModel.h"
#include "APSShipFlightBenchmark.h"
#include "Components/MeshComponent.h"

#include "Spaceship.h"
#include "ShipNavigationComponent.h"
#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Construction/APSShipBuildComponent.h"
#include "APS_ALPHA/Gameplay/Vehicles/APSGroundVehicleTypes.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace APSShipFlightModelLocal
{
	constexpr double SolarRadiusCm = 6.957e10;
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	constexpr double LightYearCm = 9.4607e17;
	/** Above this the base sphere is close enough to the terrain for the speed limits. */
	constexpr double TerrainQueryAltitudeCm = 5.0e7;
	/** Bands without a sweep still sweep frames shorter than this (10 km): stations, docking, ground. */
	constexpr double ShortMoveSweepCm = 1.0e6;
	constexpr int32 NearestGeneratedStars = 8;
	/** Rio 03.10: the drawn galaxy stars a scan looks at (the nearest few and the course's among them). */
	constexpr int32 GalaxyScanStars = 32;
	constexpr int32 BandCount = static_cast<int32>(EAPSFlightBand::Stellar) + 1;
	/** A course turned by more than 0.25 degrees since the last catalogue scan is scanned again. */
	constexpr double CourseRescanCosine = 0.99999048;
	/** Charted space: the catalogue's bounding sphere times this, plus the margin. */
	constexpr double ChartedSpaceScale = 1.25;
	constexpr double ChartedSpaceMarginCm = 50.0 * AstronomicalUnitCm;

	TAutoConsoleVariable<int32> CVarFlightModel(
		TEXT("aps.Ship.FlightModel"), 1,
		TEXT("1: every ship flies with the band model (keys 1-5: Maneuver, Flight, Orbital, Cruise, Stellar; ")
		TEXT("Right Shift/Ctrl step). 0: the previous power steps x engine modes (the experimental ship keeps the bands)."));

	TAutoConsoleVariable<float> CVarDistanceFactorScale(
		TEXT("aps.Ship.DistanceFactorScale"), 1.0f,
		TEXT("Multiplies every band's distance speed factor (tuning without a rebuild)."));
	// F2 (Rio, 02.10): the cluster is compact (stars about 1 AU apart), so past its edge the nearest star only recedes
	// and a limit proportional to it ran away exponentially until the edge of charted space.
	TAutoConsoleVariable<float> CVarStarSpacingCap(
		TEXT("aps.Ship.StarSpacingCap"), 0.0f,
		TEXT("The star term of the speed limit counts at most this many median star spacings (0 = uncapped)."));
	TAutoConsoleVariable<float> CVarLimitCreep(
		TEXT("aps.Ship.LimitCreep"), 0.04f,
		TEXT("No ceiling: held at the limit, the limit grows by this fraction of itself per second (0 = a hard limit)."));
	TAutoConsoleVariable<int32> CVarGalaxyCharted(
		TEXT("aps.Ship.GalaxyCharted"), 1,
		TEXT("Rio 03.10: 1 makes the whole galaxy charted space (its drawn stars are reachable systems); 0 keeps the cluster's edge."));
	TAutoConsoleVariable<float> CVarOutsideClusterSeconds(
		TEXT("aps.Ship.OutsideClusterSeconds"), 0.0f,
		TEXT("Outside the star cluster a band flies at most the cluster's radius in this many seconds (0 = unbounded)."));

	TAutoConsoleVariable<float> CVarSurfaceSpeedFactor(
		TEXT("aps.Ship.SurfaceSpeedFactor"), -1.0f,
		TEXT(">= 0 overrides the ships' SurfaceSpeedFactor (1/s); 0 lifts the low-altitude limit of the Maneuver and ")
		TEXT("Flight bands. -1 keeps each ship's value."));

	TAutoConsoleVariable<int32> CVarLog(
		TEXT("aps.Ship.FlightLog"), 0,
		TEXT("1 logs the piloted ship's band, limit, nearest surface and speed every second."));

	// Star drive (Rio 02.10, key J).
	TAutoConsoleVariable<float> CVarDriveCrossSeconds(
		TEXT("aps.Ship.StarDrive.CrossSeconds"), 10.0f,
		TEXT("Star drive: without input the ship crosses the median gap between neighbouring stars in this many seconds."));
	TAutoConsoleVariable<float> CVarDriveSpoolSeconds(
		TEXT("aps.Ship.StarDrive.SpoolSeconds"), 2.0f,
		TEXT("Star drive: seconds from engaging to the cruise speed."));
	TAutoConsoleVariable<float> CVarDriveThrottleRate(
		TEXT("aps.Ship.StarDrive.ThrottleRate"), 0.5f,
		TEXT("Star drive: held W multiplies the set speed by e^rate per second, S divides it; Shift doubles the rate."));
	TAutoConsoleVariable<float> CVarDriveMaxMultiple(
		TEXT("aps.Ship.StarDrive.MaxMultiple"), 40.0f,
		TEXT("Star drive: W raises the set speed up to this many times the cruise speed."));
	TAutoConsoleVariable<float> CVarDriveResponse(
		TEXT("aps.Ship.StarDrive.Response"), 1.2f,
		TEXT("Star drive: how fast the speed follows the set speed, 1/s in log space (heavier hulls follow slower)."));
	TAutoConsoleVariable<float> CVarDriveAlignRate(
		TEXT("aps.Ship.StarDrive.AlignRate"), 0.8f,
		TEXT("Star drive: how fast the course follows the nose, 1/s (lower: the ship floats wider through a turn)."));
	TAutoConsoleVariable<int32> CVarFeel(
		TEXT("aps.Ship.Feel"), 1,
		TEXT("1 (Rio 02.10): calmer flight near the ground - AUTO keeps FLIGHT low over the ground, half the ground speed ")
		TEXT("limit, less FLIGHT thrust, a lazier velocity follow, a gentle speed bleed in the air unless closing on the ")
		TEXT("ground, wider AUTO hysteresis and a heavier steering feel. 0: the previous tuning (A/B)."));
	/** aps.Ship.Feel: AUTO keeps FLIGHT below this ground clearance (and shifts up only above it), cm. */
	constexpr double LowFlightCeilingCm = 350000.0;

	/**
	 * Rio 04.10 ("in AUTO flight the ship is too twitchy on the mouse: a touch swings it left and right with inertia, I
	 * cannot hold it steady to aim"): in space the passive damping was 0.22/s (a turn ran on ~4.5 s after the mouse
	 * stopped) and the mouse went straight through. Space bands now settle like the air does, smooth the deltas a little
	 * and answer small moves finely (a power curve on the normalised input), large ones fully.
	 */
	TAutoConsoleVariable<float> CVarSpaceSteerDamping(TEXT("aps.Ship.SpaceSteerDamping"), 14.0f,
		TEXT("Space band flight: multiplier on the passive turn damping (1 = the old drift, ~4.5 s to settle)."));
	TAutoConsoleVariable<float> CVarSpaceSteerCurve(TEXT("aps.Ship.SpaceSteerCurve"), 1.5f,
		TEXT("Space band flight: exponent on the normalised mouse steering (1 = linear; larger = finer small moves)."));
	TAutoConsoleVariable<float> CVarSpaceSteerSmoothing(TEXT("aps.Ship.SpaceSteerSmoothing"), 0.05f,
		TEXT("Space band flight: low-pass time constant of the mouse steering, seconds (0 = none)."));
	TAutoConsoleVariable<float> CVarCloseInHoldSeconds(TEXT("aps.Ship.CloseInHoldSeconds"), 5.0f,
		TEXT("Rio 04.10: after AUTO shifts down closing in on a body, how long it keeps from shifting up again (0 = at once)."));
	TAutoConsoleVariable<float> CVarSpaceKeyTurnScale(TEXT("aps.Ship.SpaceKeyTurnScale"), 0.45f,
		TEXT("Rio 04.10: space band flight with the mouse on the camera: the share of the full turn rate A/D reach (1 = full)."));
	TAutoConsoleVariable<float> CVarSpaceKeyTurnEase(TEXT("aps.Ship.SpaceKeyTurnEase"), 0.25f,
		TEXT("Rio 04.10: space band flight with the mouse on the camera: how long A/D ease in and out, seconds (0 = at once)."));

	/**
	 * Rio 04.10: the autopilot ("it flies round the stars in jerks, up, then left, like Tetris; let it lead with the nose";
	 * "on arrival it spun like mad round the object and would not stop").
	 */
	TAutoConsoleVariable<float> CVarAutopilotApproachSeconds(TEXT("aps.Autopilot.ApproachSeconds"), 2.5f,
		TEXT("The autopilot's speed is held to the way left to its stop over this many seconds (an even, unhurried arrival)."));
	TAutoConsoleVariable<float> CVarAutopilotTurnRate(TEXT("aps.Autopilot.TurnRate"), 35.0f,
		TEXT("The fastest the autopilot turns the nose, degrees a second."));
	TAutoConsoleVariable<float> CVarAutopilotAimEase(TEXT("aps.Autopilot.AimEase"), 0.6f,
		TEXT("How long the autopilot's course takes to swing to a new aim (a detour round a world appearing or ending), s."));
	TAutoConsoleVariable<float> CVarAutopilotBank(TEXT("aps.Autopilot.Bank"), 25.0f,
		TEXT("How far the autopilot banks into a turn, degrees (0 = level turns)."));

	bool FeelEnabled()
	{
		return CVarFeel.GetValueOnGameThread() != 0;
	}

	FString FormatSpeed(double CmPerSecond)
	{
		const double MetersPerSecond = CmPerSecond / 100.0;
		if (MetersPerSecond < 1000.0) return FString::Printf(TEXT("%.0f m/s"), MetersPerSecond);
		if (MetersPerSecond < 1000000.0) return FString::Printf(TEXT("%.2f km/s"), MetersPerSecond / 1000.0);
		if (MetersPerSecond < 299792458.0) return FString::Printf(TEXT("%.2f Mm/s"), MetersPerSecond / 1000000.0);
		return FString::Printf(TEXT("%.1f c"), MetersPerSecond / 299792458.0);
	}

	FString BodyName(const AActor* Actor)
	{
		if (const ACelestialBody* Body = Cast<ACelestialBody>(Actor); Body && !Body->AstroName.IsNone())
		{
			return Body->AstroName.ToString().ToUpper();
		}
		FString Name = Actor ? Actor->GetName() : FString();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.ReplaceInline(TEXT("_C_"), TEXT("_"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return Name.ToUpper();
	}

	const TCHAR* ControlLabel(EAPSFlightBandControl Control, bool bInAtmosphere)
	{
		switch (Control)
		{
		case EAPSFlightBandControl::Hover: return TEXT("HOVER: LET GO TO STOP");
		case EAPSFlightBandControl::Drive: return TEXT("LET GO OF W TO SLOW DOWN");
		case EAPSFlightBandControl::Cruise: return TEXT("SPEED HOLDS, S SLOWS DOWN");
		case EAPSFlightBandControl::Assist:
		default:
			return bInAtmosphere ? TEXT("AIR DRAG SLOWS YOU") : TEXT("SPEED HOLDS, S SLOWS DOWN");
		}
	}
}

double APSFlightBandModel::SpeedLimitCm(const FAPSFlightBandSettings& Band, double Boost, double NearestSurfaceCm,
	double GroundClearanceCm, double DistanceFloorCm, double SurfaceFactor, double SurfaceFloorCm)
{
	const double SafeBoost = FMath::Max(Boost, 1.0);
	double Limit = FMath::Max(Band.MaxSpeed, 0.1) * 100.0 * SafeBoost;
	if (Band.DistanceSpeedFactor > 0.0)
	{
		if (NearestSurfaceCm >= 0.0)
		{
			Limit = FMath::Min(Limit,
				FMath::Max(Band.DistanceSpeedFactor * SafeBoost * NearestSurfaceCm, DistanceFloorCm));
		}
	}
	else if (SurfaceFactor > 0.0 && GroundClearanceCm >= 0.0)
	{
		Limit = FMath::Min(Limit, FMath::Max(SurfaceFactor * GroundClearanceCm, SurfaceFloorCm));
	}
	return Limit;
}

double APSFlightBandModel::BandSpeedLimitCm(const FAPSFlightBandSettings& Band, double Boost,
	const FSpeedLimitInputs& Inputs, double BelowLimitCm)
{
	const double SafeBoost = FMath::Max(Boost, 1.0);
	double Limit = SpeedLimitCm(Band, SafeBoost, Band.bCourseLimit ? Inputs.LocalSurfaceCm : Inputs.SurfaceCm,
		Inputs.GroundClearanceCm, Inputs.DistanceFloorCm, Inputs.SurfaceFactor, Inputs.SurfaceFloorCm);
	if (Band.StarDistanceSpeedFactor > 0.0 && Inputs.StarCm >= 0.0)
	{
		// The scale of a star system: a share of the distance to the sun per second.
		Limit = FMath::Min(Limit, FMath::Max(Band.StarDistanceSpeedFactor * SafeBoost * Inputs.StarCm, Inputs.DistanceFloorCm));
	}
	if (Band.bCourseLimit)
	{
		if (Inputs.CourseCm >= 0.0)
		{
			Limit = FMath::Min(Limit, Band.DistanceSpeedFactor * SafeBoost * Inputs.CourseCm);
		}
		// Inside a system, and arriving at one, the band is as fast as the band below: AUTO hands over without a jolt.
		Limit = FMath::Max(Limit, BelowLimitCm);
	}
	return Limit;
}

double APSFlightBandModel::CapSpeedGrowth(double PreviousSpeed, double Speed, double MaxLogRate, double MinAcceleration,
	double DeltaTime)
{
	if (Speed <= PreviousSpeed || MaxLogRate <= 0.0 || DeltaTime <= 0.0)
	{
		return Speed;
	}
	const double Previous = FMath::Max(PreviousSpeed, 0.0);
	return FMath::Min(Speed, FMath::Max(Previous * FMath::Exp(MaxLogRate * DeltaTime),
		Previous + FMath::Max(MinAcceleration, 0.0) * DeltaTime));
}

double APSFlightBandModel::CourseClearanceCm(const FVector& ToCenter, const FVector& Direction, double RadiusCm,
	double MissScale)
{
	const double RadiusSquared = RadiusCm * RadiusCm;
	const double DistanceSquared = ToCenter.SizeSquared();
	if (DistanceSquared <= RadiusSquared)
	{
		return 0.0;
	}
	const double Along = FVector::DotProduct(ToCenter, Direction);
	if (Along <= 0.0)
	{
		return -1.0;
	}
	const double MissSquared = FMath::Max(DistanceSquared - Along * Along, 0.0);
	if (MissSquared < RadiusSquared)
	{
		return Along - FMath::Sqrt(RadiusSquared - MissSquared);
	}
	// Continuous with the hit at a miss of one radius; a star a few radii off the course hardly counts.
	return Along + (FMath::Sqrt(MissSquared) - RadiusCm) * FMath::Max(MissScale, 1.0);
}

double APSFlightBandModel::CourseExitCm(const FVector& FromCenter, const FVector& Direction, double RadiusCm)
{
	const double Outside = FromCenter.SizeSquared() - RadiusCm * RadiusCm;
	if (Outside >= 0.0)
	{
		return 0.0;
	}
	const double Along = FVector::DotProduct(FromCenter, Direction);
	return -Along + FMath::Sqrt(Along * Along - Outside);
}

double APSFlightBandModel::ShedOverspeed(double Speed, double Limit, double Rate, double DeltaTime)
{
	if (Limit <= UE_DOUBLE_SMALL_NUMBER)
	{
		return 0.0;
	}
	if (Speed <= Limit)
	{
		return Speed;
	}
	const double KeptLogExcess = FMath::Exp(-FMath::Max(Rate, 0.0) * FMath::Max(DeltaTime, 0.0));
	return Limit * FMath::Pow(Speed / Limit, KeptLogExcess);
}

double APSFlightBandModel::ShedRate(double BaseRate, double DistanceFactor)
{
	return FMath::Max(BaseRate, 4.0 * FMath::Max(DistanceFactor, 0.0));
}

double APSFlightBandModel::GuardClosingSpeed(double ClosingSpeed, double DistanceCm, double DeltaTime)
{
	if (DeltaTime <= 0.0 || DistanceCm < 0.0)
	{
		return ClosingSpeed;
	}
	return FMath::Min(ClosingSpeed, 0.25 * DistanceCm / DeltaTime);
}

FVector APSFlightBandModel::HoverStep(const FVector& Velocity, const FVector& Target, double Acceleration, double DeltaTime)
{
	if (DeltaTime <= 0.0)
	{
		return Velocity;
	}
	return Velocity + (Target - Velocity).GetClampedToMaxSize(FMath::Max(Acceleration, 0.0) * DeltaTime);
}

double APSFlightBandModel::TaperedSpeedStep(double Speed, double Target, double Acceleration, double DeltaTime)
{
	if (DeltaTime <= 0.0 || Target <= 0.0 || Speed >= Target)
	{
		return Speed;
	}
	const double Thrust = FMath::Max(Acceleration, 0.0);
	if (Speed < 0.0)
	{
		return FMath::Min(Speed + Thrust * DeltaTime, 0.0);
	}
	// dv/dt = a (1 - (v/T)^2) integrates exactly to v/T = tanh(atanh(v/T) + a t / T): full thrust from rest,
	// fading only near the target, like a car reaching its top speed.
	const double Ratio = FMath::Min(Speed / Target, 0.999999);
	return Target * FMath::Min(FMath::Tanh(0.5 * FMath::Loge((1.0 + Ratio) / (1.0 - Ratio)) + Thrust / Target * DeltaTime),
		1.0);
}

FVector APSFlightBandModel::AlignToNose(const FVector& Velocity, const FVector& Forward, double Rate, double DeltaTime)
{
	const double Speed = Velocity.Size();
	if (Speed <= UE_DOUBLE_SMALL_NUMBER || Rate <= 0.0 || DeltaTime <= 0.0 || Forward.IsNearlyZero())
	{
		return Velocity;
	}
	const FVector Direction = Velocity / Speed;
	const FVector Goal = FVector::DotProduct(Direction, Forward) >= 0.0 ? Forward : -Forward;
	const double Alpha = 1.0 - FMath::Exp(-Rate * DeltaTime);
	const FVector Turned = (Direction + (Goal - Direction) * Alpha).GetSafeNormal();
	return (Turned.IsNearlyZero() ? Goal : Turned) * Speed;
}

double APSFlightBandModel::CruiseSpeedStep(double Speed, double ForwardInput, double Limit, double Response,
	double BrakeRate, double Drag, double DeltaTime)
{
	if (DeltaTime <= 0.0)
	{
		return Speed;
	}
	if (ForwardInput > UE_KINDA_SMALL_NUMBER)
	{
		const double Target = Limit * FMath::Min(ForwardInput, 1.0);
		return Speed < Target ? Target - (Target - Speed) * FMath::Exp(-FMath::Max(Response, 0.0) * DeltaTime) : Speed;
	}
	if (ForwardInput < -UE_KINDA_SMALL_NUMBER)
	{
		const double Next = Speed * FMath::Exp(-FMath::Max(BrakeRate, 0.0) * FMath::Min(-ForwardInput, 1.0) * DeltaTime);
		return Next < 100.0 ? 0.0 : Next;
	}
	return Drag > 0.0 ? Speed * FMath::Exp(-Drag * DeltaTime) : Speed;
}

FVector APSFlightBandModel::BrakeStep(const FVector& Velocity, double BrakeAcceleration, double BrakeRate, double DeltaTime)
{
	const double Speed = Velocity.Size();
	if (Speed <= UE_DOUBLE_SMALL_NUMBER || DeltaTime <= 0.0)
	{
		return DeltaTime > 0.0 ? FVector::ZeroVector : Velocity;
	}
	const double Reduction = FMath::Max(BrakeAcceleration, Speed * BrakeRate) * DeltaTime;
	return Speed <= Reduction ? FVector::ZeroVector : Velocity * ((Speed - Reduction) / Speed);
}

double APSFlightBandModel::ClassAgility(uint8 SizeClass)
{
	// XXS, XS, S, M, L, XL, XXL, Titan.
	static constexpr double Agility[] = {1.6, 1.4, 1.2, 1.0, 0.8, 0.65, 0.5, 0.4};
	return Agility[FMath::Min(static_cast<int32>(SizeClass), 7)];
}

UAPSShipFlightModel::UAPSShipFlightModel()
{
	PrimaryComponentTick.bCanEverTick = false;
	for (int32 Index = 0; Index < APSShipFlightModelLocal::BandCount; ++Index)
	{
		Bands.Add(MakeDefaultBand(static_cast<EAPSFlightBand>(Index)));
	}
}

FAPSFlightBandSettings UAPSShipFlightModel::MakeDefaultBand(EAPSFlightBand Band)
{
	FAPSFlightBandSettings Settings;
	switch (Band)
	{
	case EAPSFlightBand::Maneuver:
		Settings.Name = TEXT("MANEUVER");
		Settings.Control = EAPSFlightBandControl::Hover;
		Settings.MaxSpeed = 60.0;
		Settings.Acceleration = 25.0;
		Settings.BoostMultiplier = 3.0;
		Settings.LateralFraction = 1.0;
		break;
	case EAPSFlightBand::Flight:
		Settings.Name = TEXT("FLIGHT");
		Settings.Control = EAPSFlightBandControl::Drive;
		// 90% of the limit in about 4.4 s, like every band below Cruise (Rio: long spool-ups are unpleasant).
		Settings.MaxSpeed = 1200.0;
		Settings.Acceleration = 400.0;
		Settings.BoostMultiplier = 3.0;
		Settings.AssistRate = 2.5;
		Settings.ReleaseDrag = 0.6;
		break;
	case EAPSFlightBand::Orbital:
		Settings.Name = TEXT("ORBITAL");
		Settings.Control = EAPSFlightBandControl::Assist;
		Settings.MaxSpeed = 100000.0;
		Settings.Acceleration = 30000.0;
		Settings.BoostMultiplier = 4.0;
		Settings.AssistRate = 1.5;
		Settings.DistanceSpeedFactor = 0.5;
		break;
	case EAPSFlightBand::Cruise:
		Settings.Name = TEXT("CRUISE");
		Settings.Control = EAPSFlightBandControl::Cruise;
		// The scale of one star system (Rio, 30.09): 12% of the distance to the sun per second, 60 c at 1 AU and the
		// 10 000 c cap at the system's edge, so the planets drift past and the stars stay put. The speed grows at most
		// 1.8x per second: leaving orbit no longer shoots the ship out of the system. Near bodies the distance limit rules.
		Settings.MaxSpeed = 3.0e12;
		Settings.Acceleration = 30000.0;
		Settings.BoostMultiplier = 3.0;
		Settings.CruiseResponse = 1.5;
		Settings.AssistRate = 3.0;
		Settings.DistanceSpeedFactor = 0.5;
		Settings.StarDistanceSpeedFactor = 0.12;
		Settings.MaxLogAcceleration = 0.6;
		Settings.bSweepCollision = false;
		break;
	case EAPSFlightBand::Stellar:
	default:
		Settings.Name = TEXT("STELLAR");
		Settings.Control = EAPSFlightBandControl::Cruise;
		// ~3e7 c: 5 light years in about half a minute (10 s with boost). Only the system the course runs into slows the
		// ship, down to the CRUISE speed at that system's edge, and the speed grows at most 2.2x per second (5x with
		// boost): no more jumps to 230 000 c at the edge of the cluster, no pumping while passing stars (Rio, 30.09).
		Settings.MaxSpeed = 1.0e16;
		Settings.Acceleration = 30000.0;
		Settings.BoostMultiplier = 4.0;
		Settings.CruiseResponse = 1.5;
		Settings.AssistRate = 3.0;
		Settings.DistanceSpeedFactor = 0.5;
		Settings.MaxLogAcceleration = 0.8;
		Settings.bCourseLimit = true;
		Settings.bSweepCollision = false;
		break;
	}
	return Settings;
}

void UAPSShipFlightModel::BeginPlay()
{
	Super::BeginPlay();
	EnsureBandSettings();
	FlightBand = IsBandAvailable(InitialBand) ? InitialBand : EAPSFlightBand::Maneuver;
}

ASpaceship* UAPSShipFlightModel::GetShip() const
{
	return Cast<ASpaceship>(GetOwner());
}

bool UAPSShipFlightModel::IsBandFlightActive() const
{
	return bAlwaysUseBands || APSShipFlightModelLocal::CVarFlightModel.GetValueOnGameThread() != 0;
}

const FAPSFlightBandSettings& UAPSShipFlightModel::GetBandSettings(EAPSFlightBand Band) const
{
	const int32 Index = static_cast<int32>(Band);
	if (Bands.IsValidIndex(Index))
	{
		return Bands[Index];
	}
	static const FAPSFlightBandSettings Fallback = MakeDefaultBand(EAPSFlightBand::Maneuver);
	return Fallback;
}

bool UAPSShipFlightModel::IsBandAvailable(EAPSFlightBand Band) const
{
	const ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return false;
	}
	switch (Band)
	{
	case EAPSFlightBand::Cruise: return Ship->ActiveClassPreset.bSupportsSpaceWrap;
	case EAPSFlightBand::Stellar: return Ship->ActiveClassPreset.bSupportsOffset;
	default: return true;
	}
}

void UAPSShipFlightModel::SetFlightBand(EAPSFlightBand NewBand)
{
	const ASpaceship* Ship = GetShip();
	if (Ship && Ship->IsGroundVehicle())
	{
		// Rio 02.10: a ground vehicle never uses the space flight bands.
		VehicleNotice(TEXT("FLIGHT MODES (1-5) ARE FOR SHIPS"));
		return;
	}
	if (!IsBandAvailable(NewBand))
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Flight] %s: band %s needs a %s engine this %s hull lacks"), *GetNameSafe(Ship),
			*GetBandSettings(NewBand).Name, NewBand == EAPSFlightBand::Stellar ? TEXT("Offset") : TEXT("SpaceWrap"),
			Ship ? *Ship->GetSizeClassName() : TEXT("?"));
		return;
	}
	bManualBand = true;
	ApplyBand(NewBand, TEXT("manual"));
}

void UAPSShipFlightModel::SetAutoBands()
{
	if (const ASpaceship* Ship = GetShip(); Ship && Ship->IsGroundVehicle())
	{
		return;
	}
	if (bManualBand)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Flight] %s band AUTO from %s"), *GetNameSafe(GetOwner()),
			*GetBandSettings(FlightBand).Name);
	}
	bManualBand = false;
	AutoShiftHold = 0.0f;
}

void UAPSShipFlightModel::ApplyBand(EAPSFlightBand NewBand, const TCHAR* Reason)
{
	if (NewBand == FlightBand)
	{
		return;
	}
	const ASpaceship* Ship = GetShip();
	const EAPSFlightBand PreviousBand = FlightBand;
	FlightBand = NewBand;
	AutoShiftHold = 0.0f;
	BodyRefreshElapsed = TNumericLimits<float>::Max();
	CatalogueScanElapsed = TNumericLimits<float>::Max();
	UE_LOG(LogTemp, Log, TEXT("[APS.Flight] %s band %s -> %s at %s (%s)"), *GetNameSafe(Ship),
		*GetBandSettings(PreviousBand).Name, *GetBandSettings(NewBand).Name,
		Ship ? *APSShipFlightModelLocal::FormatSpeed(Ship->KinematicVelocity.Size()) : TEXT("?"), Reason);
	// The journal keeps the milestones of the player's flight: leaving the star systems and arriving at one.
	if (Ship && Ship->IsPlayerControlled())
	{
		if (NewBand == EAPSFlightBand::Stellar)
		{
			UAPSCivilizationJournalSubsystem::Post(Ship, TEXT("Flight"), NSLOCTEXT("APSCivilizationJournal",
				"Interstellar", "The home ship left the star systems behind: interstellar flight."));
		}
		else if (PreviousBand == EAPSFlightBand::Stellar)
		{
			UAPSCivilizationJournalSubsystem::Post(Ship, TEXT("Flight"), NSLOCTEXT("APSCivilizationJournal",
				"Arrived", "The home ship slowed down at a star system."));
		}
	}
}

EAPSFlightBand UAPSShipFlightModel::NeighbourBand(EAPSFlightBand Band, int32 Direction) const
{
	for (int32 Index = static_cast<int32>(Band) + (Direction > 0 ? 1 : -1);
		Index >= 0 && Index < APSShipFlightModelLocal::BandCount; Index += Direction > 0 ? 1 : -1)
	{
		if (IsBandAvailable(static_cast<EAPSFlightBand>(Index)))
		{
			return static_cast<EAPSFlightBand>(Index);
		}
	}
	return Band;
}

void UAPSShipFlightModel::StepFlightBand(int32 Direction)
{
	const EAPSFlightBand Next = NeighbourBand(FlightBand, Direction);
	if (Next != FlightBand)
	{
		SetFlightBand(Next);
	}
}

void UAPSShipFlightModel::EngageAutopilot(AActor* Target)
{
	ASpaceship* Ship = GetShip();
	if (!Ship || !IsValid(Target) || Target == Ship)
	{
		return;
	}
	if (Ship->IsGroundVehicle())
	{
		VehicleNotice(TEXT("THE AUTOPILOT IS FOR SHIPS"));
		return;
	}
	DisengageStarDrive(TEXT("autopilot"));
	// Where to stop: in a low orbit of a world, beside a station, well clear of a star.
	double RadiusCm = 10000.0;
	double ArrivalCm = 100000.0;
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Target))
	{
		RadiusCm = Body->GetWorldScapeBodyRadiusCm();
		ArrivalCm = FMath::Max(RadiusCm * 0.08, 20000000.0);
	}
	else if (const AStar* Star = Cast<AStar>(Target))
	{
		RadiusCm = (Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM)) * 100000.0;
		ArrivalCm = RadiusCm * 3.0;
	}
	else if (Target->IsA<ASpaceStation>())
	{
		RadiusCm = 50000.0;
		ArrivalCm = 150000.0;
	}
	AutopilotTarget = Target;
	AutopilotArrivalCm = ArrivalCm;
	AutopilotRemainingCm = -1.0;
	bAutopilotRotated = false;
	AutopilotCourse = FVector::ZeroVector;
	AutopilotLevelUp = Ship->GetActorUpVector();
	AutopilotBankDegrees = 0.0;
	AutopilotSpeedCapCm = TNumericLimits<double>::Max();
	SetAutoBands();
	UE_LOG(LogTemp, Log, TEXT("[APS.Autopilot] %s engaged for %s (stops %.0f km from its surface)"), *GetNameSafe(Ship),
		*GetNameSafe(Target), ArrivalCm / 100000.0);
}

void UAPSShipFlightModel::ApplyWorldShift(const FVector& Offset)
{
	// Planets, moons and stations are read from their actors, which moved; generated stars keep a fixed world centre.
	for (FFlightBody& Body : FlightBodies)
	{
		if (Body.bFixedLocation)
		{
			Body.Center += Offset;
		}
	}
}

void UAPSShipFlightModel::DisengageAutopilot(const TCHAR* Reason)
{
	if (!AutopilotTarget.IsValid() && !bDebugDrive)
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Autopilot] %s off: %s"), *GetNameSafe(GetShip()), Reason);
	AutopilotTarget.Reset();
	bAutopilotRotated = false;
	AutopilotCourse = FVector::ZeroVector;
	AutopilotBankDegrees = 0.0;
	AutopilotSpeedCapCm = TNumericLimits<double>::Max();
	bDebugDrive = false;
	DebugForwardInput = 0.0f;
	bDebugBoost = false;
}

void UAPSShipFlightModel::UpdateAutopilot(const float DeltaTime)
{
	ASpaceship* Ship = GetShip();
	AActor* Target = AutopilotTarget.Get();
	if (!Ship || !Target)
	{
		if (bAutopilotRotated)
		{
			DisengageAutopilot(TEXT("the target is gone"));
		}
		return;
	}
	// The helm takes the ship back: thrust, strafe, the brake, or a turn the autopilot did not make.
	if (FMath::Abs(Ship->ForwardInput) > 0.1f || FMath::Abs(Ship->SideInput) > 0.1f || FMath::Abs(Ship->VerticalInput) > 0.1f
		|| Ship->bIsDecelerating
		// A turn of more than half a degree since the autopilot's own: the pilot steered (small drifts are not).
		|| (bAutopilotRotated && Ship->GetActorQuat().AngularDistance(AutopilotLastRotation) > FMath::DegreesToRadians(0.5)))
	{
		DisengageAutopilot(TEXT("helm input"));
		return;
	}
	double RadiusCm = 0.0;
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Target))
	{
		RadiusCm = Body->GetWorldScapeBodyRadiusCm();
	}
	else if (const AStar* Star = Cast<AStar>(Target))
	{
		RadiusCm = (Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM)) * 100000.0;
	}
	else if (Target->IsA<ASpaceStation>())
	{
		RadiusCm = 50000.0;
	}
	const FVector ToTarget = Target->GetActorLocation() - Ship->GetActorLocation();
	const double Distance = ToTarget.Size();
	AutopilotRemainingCm = FMath::Max(Distance - RadiusCm - AutopilotArrivalCm, 0.0);
	const double Speed = Ship->KinematicVelocity.Size();
	// Rio 04.10 ("on arrival it spun like mad round the object and would not stop"): the bands slow a ship near worlds
	// and stations only, so a small target (an ancient site) was met at full band speed, its 1 km stop crossed within a
	// frame and every turn back overshot again. The speed is held to what covers the way left to the stop in a few seconds.
	constexpr double ArrivalSpeedCm = 3000.0;
	AutopilotSpeedCapCm = FMath::Max((Distance - RadiusCm - AutopilotArrivalCm * 0.5)
		/ FMath::Max(APSShipFlightModelLocal::CVarAutopilotApproachSeconds.GetValueOnGameThread(), 0.5f), ArrivalSpeedCm);
	if (Distance - RadiusCm <= AutopilotArrivalCm)
	{
		// There: brake to a stop, then hand the ship back. (Not through SetDebugDrive: its log line came every frame, 278
		// lines in 4 s of Rio's 04.10 arrival.)
		AutopilotSpeedCapCm = ArrivalSpeedCm;
		if (Speed < 2000.0)
		{
			DisengageAutopilot(TEXT("arrived"));
			return;
		}
		bDebugDrive = true;
		DebugForwardInput = -1.0f;
		bDebugBoost = false;
		return;
	}
	// Rio 02.10 ("an object behind a planet: the autopilot tries to fly straight through it"): a world on the way is flown
	// round through a point beside its limb, clear of its air, recomputed every frame until the target is in sight.
	const FVector ShipLocation = Ship->GetActorLocation();
	const FVector Segment = Target->GetActorLocation() - ShipLocation;
	const double SegmentSquared = Segment.SizeSquared();
	FVector Aim = Target->GetActorLocation();
	double NearestBlock = TNumericLimits<double>::Max();
	for (const FFlightBody& Body : FlightBodies)
	{
		const AActor* BodyActor = Body.Actor.Get();
		if (!Body.bSolid || SegmentSquared < 1.0 || (BodyActor && BodyActor == Target) || (!Body.bFixedLocation && !BodyActor))
		{
			continue;
		}
		const FVector Centre = Body.bFixedLocation ? Body.Center : BodyActor->GetActorTransform().TransformPosition(Body.Center);
		const double Along = FVector::DotProduct(Centre - ShipLocation, Segment) / SegmentSquared;
		const FVector Closest = ShipLocation + Segment * FMath::Clamp(Along, 0.0, 1.0);
		// Through the solid sphere a hair inside its surface: a target on a world's near side is not behind it.
		if (Along <= 0.0 || Along >= 1.0 || FVector::Distance(Closest, Centre) >= Body.RadiusCm * 0.995
			|| Along * FMath::Sqrt(SegmentSquared) >= NearestBlock)
		{
			continue;
		}
		NearestBlock = Along * FMath::Sqrt(SegmentSquared);
		FVector Out = (Closest - Centre).GetSafeNormal();
		if (Out.IsNearlyZero())
		{
			Out = FVector::VectorPlaneProject(Ship->GetActorUpVector(), Segment.GetSafeNormal()).GetSafeNormal(
				UE_SMALL_NUMBER, FVector::UpVector);
		}
		double Clearance = Body.RadiusCm * 0.2;
		if (const APlanetaryBody* World = Cast<APlanetaryBody>(BodyActor))
		{
			Clearance = FMath::Max(Clearance, World->AtmosphereHeight * 100000.0 * 1.2);
		}
		Aim = Centre + Out * (Body.RadiusCm + Clearance);
		// Rio 04.10 ("on the surface, with the target behind the world, it flies straight into the ground and sticks in
		// it; let it go up to orbit first"): from low down the point beside the limb lies below the horizon, and the way to
		// it runs through the world. Then the ship climbs first, steeply and leaning toward the target, until that way is
		// clear (from orbit), and only then turns for the limb.
		const FVector Leg = Aim - ShipLocation;
		const double LegSquared = Leg.SizeSquared();
		const double LegAlong = LegSquared > 1.0
			? FMath::Clamp(FVector::DotProduct(Centre - ShipLocation, Leg) / LegSquared, 0.0, 1.0) : 0.0;
		if (FVector::Distance(ShipLocation + Leg * LegAlong, Centre) < Body.RadiusCm * 1.01)
		{
			const FVector Radial = (ShipLocation - Centre).GetSafeNormal(UE_SMALL_NUMBER, Out);
			FVector Lean = FVector::VectorPlaneProject(Target->GetActorLocation() - ShipLocation, Radial).GetSafeNormal();
			if (Lean.IsNearlyZero())
			{
				Lean = FVector::VectorPlaneProject(Ship->GetShipForwardVector(), Radial).GetSafeNormal();
			}
			const double Climb = FMath::Max(Body.RadiusCm + Clearance - FVector::Distance(ShipLocation, Centre), Clearance * 0.25);
			Aim = ShipLocation + (Radial * 0.8 + Lean * 0.6).GetSafeNormal(UE_SMALL_NUMBER, Radial) * Climb * 1.5;
		}
	}
	const FVector ToAim = Aim - ShipLocation;
	const FVector AimDirection = ToAim.SizeSquared() > 1.0 ? ToAim.GetSafeNormal() : Ship->GetShipForwardVector();
	// Rio 04.10 ("it flies round the stars in jerks, up, then left, like Tetris; let it lead with the nose"): the course
	// swings toward a new aim (a detour appearing or ending) over a moment instead of jumping to it, the nose follows at a
	// bounded rate, and the hull banks into the turn and levels out on course, as a flyer does.
	const double Ease = 1.0 - FMath::Exp(-DeltaTime
		/ FMath::Max(APSShipFlightModelLocal::CVarAutopilotAimEase.GetValueOnGameThread(), 0.01f));
	AutopilotCourse = AutopilotCourse.IsNearlyZero() ? AimDirection
		: (AutopilotCourse + (AimDirection - AutopilotCourse) * Ease).GetSafeNormal(UE_SMALL_NUMBER, AimDirection);
	const FVector Direction = AutopilotCourse;
	FVector LevelUp = FVector::VectorPlaneProject(AutopilotLevelUp.IsNearlyZero() ? Ship->GetActorUpVector() : AutopilotLevelUp,
		Direction).GetSafeNormal();
	if (LevelUp.IsNearlyZero())
	{
		LevelUp = FVector::VectorPlaneProject(FVector::UpVector, Direction).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}
	AutopilotLevelUp = LevelUp;
	// The bank leans to the side the nose still has to turn to, more for a wider turn, and eases back to level on course.
	const FVector Forward = Ship->GetShipForwardVector();
	const double TurnDegrees = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(Direction, FVector::CrossProduct(LevelUp, Forward).GetSafeNormal()),
		FVector::DotProduct(Direction, Forward)));
	const double MaxBank = FMath::Clamp(static_cast<double>(APSShipFlightModelLocal::CVarAutopilotBank.GetValueOnGameThread()), 0.0, 60.0);
	AutopilotBankDegrees = FMath::FInterpTo(AutopilotBankDegrees, FMath::Clamp(TurnDegrees * 0.8, -MaxBank, MaxBank),
		static_cast<double>(DeltaTime), 1.5);
	const double Bank = FMath::DegreesToRadians(AutopilotBankDegrees);
	const FVector LevelRight = FVector::CrossProduct(LevelUp, Direction).GetSafeNormal();
	const FVector Up = (LevelUp * FMath::Cos(Bank) + LevelRight * FMath::Sin(Bank)).GetSafeNormal(UE_SMALL_NUMBER, LevelUp);
	const FQuat Wanted = FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Direction, Up);
	const FQuat Current = Ship->GetActorQuat();
	const double Angle = Current.AngularDistance(Wanted);
	const double MaxStep = FMath::DegreesToRadians(FMath::Max(APSShipFlightModelLocal::CVarAutopilotTurnRate.GetValueOnGameThread(), 1.0f))
		* DeltaTime;
	const double Alpha = FMath::Min(FMath::Clamp(DeltaTime * 1.6, 0.0, 1.0), Angle > UE_SMALL_NUMBER ? MaxStep / Angle : 1.0);
	Ship->SetActorRotation(FQuat::Slerp(Current, Wanted, Alpha), ETeleportType::TeleportPhysics);
	AutopilotLastRotation = Ship->GetActorQuat();
	bAutopilotRotated = true;
	// Thrust once the nose is on the target; the bands slow the ship near bodies as for a pilot.
	const double Alignment = FVector::DotProduct(Ship->GetShipForwardVector(), Direction);
	bDebugDrive = true;
	DebugForwardInput = Alignment > 0.97 ? 1.0f : Alignment > 0.7 ? 0.3f : 0.0f;
	// Rio 02.10 ("the autopilot crawls"): on course it flies boosted while more than three seconds of the way are left,
	// out of the air or still far off; the bands' distance limits slow it near worlds and stations as before.
	// Rio 04.10 ("let me make a trip longer"): a band picked with 1-5 under the autopilot is the pace, unboosted.
	bDebugBoost = !bManualBand && Alignment > 0.97 && AutopilotRemainingCm > FMath::Max(Speed * 3.0, 50000.0)
		&& (!IsInAtmosphere() || AutopilotRemainingCm > 20000000.0);
}

void UAPSShipFlightModel::SetDebugDrive(bool bEnabled, float Forward, bool bBoost)
{
	bDebugDrive = bEnabled;
	DebugForwardInput = FMath::Clamp(Forward, -1.0f, 1.0f);
	bDebugBoost = bBoost;
	UE_LOG(LogTemp, Log, TEXT("[APS.Flight] %s test drive %s: band %s forward %.2f boost %d"), *GetNameSafe(GetOwner()),
		bEnabled ? TEXT("on") : TEXT("off"), *GetBandSettings(FlightBand).Name, DebugForwardInput, bDebugBoost ? 1 : 0);
}

void UAPSShipFlightModel::OnPossessed()
{
	EnsureBandSettings();
	if (const ASpaceship* Ship = GetShip(); Ship && Ship->IsGroundVehicle())
	{
		// Rio 02.10: a ground vehicle never flies the bands; it starts from its pose on the ground.
		EnsureKinematicHull();
		ResetVehicleState();
		return;
	}
	if (IsBandFlightActive())
	{
		EnsureKinematicHull();
	}
	if (!IsBandAvailable(FlightBand))
	{
		FlightBand = EAPSFlightBand::Maneuver;
	}
	// Every boarding starts in AUTO; keys 1-5 are an override for this flight.
	bManualBand = false;
	AutoShiftHold = 0.0f;
	BodyRefreshElapsed = TNumericLimits<float>::Max();
	GroundProbeElapsed = TNumericLimits<float>::Max();
	CatalogueScanElapsed = TNumericLimits<float>::Max();
}

void UAPSShipFlightModel::EnsureBandSettings()
{
	if (Bands.Num() == APSShipFlightModelLocal::BandCount)
	{
		return;
	}
	Bands.Reset();
	for (int32 Index = 0; Index < APSShipFlightModelLocal::BandCount; ++Index)
	{
		Bands.Add(MakeDefaultBand(static_cast<EAPSFlightBand>(Index)));
	}
}

void UAPSShipFlightModel::EnsureKinematicHull()
{
	ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return;
	}
	// The band model owns translation; the power-step model would otherwise hand XXS-M hulls to rigid-body physics.
	Ship->ActiveClassPreset.bUsesPhysicalImpulse = false;
	if (Ship->SpaceshipHull && Ship->SpaceshipHull->IsSimulatingPhysics())
	{
		Ship->KinematicVelocity = Ship->SpaceshipHull->GetPhysicsLinearVelocity();
		Ship->SpaceshipHull->SetSimulatePhysics(false);
	}
}

bool UAPSShipFlightModel::IsInAtmosphere() const
{
	const ASpaceship* Ship = GetShip();
	return Ship && (Ship->CurrentFlightEnvironment == EShipFlightEnvironment::Atmosphere
		|| Ship->CurrentFlightEnvironment == EShipFlightEnvironment::Surface);
}

void UAPSShipFlightModel::RefreshFlightBodies()
{
	FlightBodies.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ACelestialBody> It(World); It; ++It)
	{
		ACelestialBody* Body = *It;
		if (!IsValid(Body))
		{
			continue;
		}
		double RadiusCm = 0.0;
		if (const APlanetaryBody* Planetary = Cast<APlanetaryBody>(Body))
		{
			RadiusCm = Planetary->GetWorldScapeBodyRadiusCm();
		}
		else if (const AStar* Star = Cast<AStar>(Body))
		{
			RadiusCm = FMath::Max(static_cast<double>(Star->StarRadiusKM), Star->RadiusKM) * 100000.0;
		}
		else
		{
			RadiusCm = Body->RadiusKM * 100000.0;
		}
		if (RadiusCm <= 0.0)
		{
			continue;
		}
		FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
		Entry.Actor = Body;
		Entry.RadiusCm = RadiusCm;
		Entry.Name = APSShipFlightModelLocal::BodyName(Body);
		Entry.bStar = Body->IsA<AStar>();
	}
	// A star's system reaches a quarter beyond its outermost planet; a bare star is InterstellarDistanceAU across.
	const double BareSystemCm = InterstellarDistanceAU * APSShipFlightModelLocal::AstronomicalUnitCm;
	for (FFlightBody& Entry : FlightBodies)
	{
		if (Entry.bStar)
		{
			Entry.SystemRadiusCm = FMath::Max(BareSystemCm, Entry.RadiusCm);
		}
	}
	for (TActorIterator<APlanet> It(World); It; ++It)
	{
		const APlanet* Planet = *It;
		if (!IsValid(Planet) || !IsValid(Planet->ParentStar))
		{
			continue;
		}
		for (FFlightBody& Entry : FlightBodies)
		{
			if (Entry.bStar && Entry.Actor.Get() == Planet->ParentStar)
			{
				Entry.SystemRadiusCm = FMath::Max(Entry.SystemRadiusCm,
					1.25 * FVector::Dist(Planet->GetActorLocation(), Planet->ParentStar->GetActorLocation()));
			}
		}
	}
	for (TActorIterator<ASpaceStation> It(World); It; ++It)
	{
		ASpaceStation* Station = *It;
		if (!IsValid(Station))
		{
			continue;
		}
		const TPair<FVector, double>* Bounds = StationBoundsCache.Find(Station);
		if (!Bounds)
		{
			// The hull's bounds once per station (stations do not change shape in flight): visible meshes only. Colliding
			// bounds took in gravity and interaction volumes, and the new HQ Alpha's volume put a ship far outside it at
			// "0 m" from the station, so every band sat at the 500 m/s floor (Rio 02.10).
			FBox Hull(ForceInit);
			const FVector StationLocation = Station->GetActorLocation();
			Station->ForEachComponent<UMeshComponent>(false, [&Hull, StationLocation](const UMeshComponent* Mesh)
			{
				// A component left far away (one at the world origin in HQ Alpha) is not part of the hull.
				if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0
					&& FVector::Dist(Mesh->Bounds.Origin, StationLocation) < 10000000.0)
				{
					Hull += Mesh->Bounds.GetBox();
				}
			});
			const FVector Origin = Hull.IsValid ? Hull.GetCenter() : Station->GetActorLocation();
			// A station is never more than a few kilometres across.
			const double Radius = Hull.IsValid ? FMath::Min(Hull.GetExtent().Size(), 1000000.0) : 0.0;
			Bounds = &StationBoundsCache.Add(Station, TPair<FVector, double>(
				Station->GetActorTransform().InverseTransformPosition(Origin), Radius));
		}
		if (Bounds->Value <= 0.0)
		{
			continue;
		}
		FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
		Entry.Actor = Station;
		Entry.Center = Bounds->Key;
		Entry.RadiusCm = Bounds->Value;
		Entry.Name = APSShipFlightModelLocal::BodyName(Station);
		Entry.bSolid = false;
	}
}

bool UAPSShipFlightModel::RebuildStarCatalogue()
{
	UWorld* World = GetWorld();
	AAstroGenerator* Generator = CatalogueGenerator.Get();
	if (!IsValid(Generator) && World)
	{
		// The generator the stellar view draws (UAPSStellarVisualSubsystem::UpdateGameplayStellarView).
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("WorldGenerationPreview")) && !It->UsesContinuousPreviewFrame()
				&& It->GetCanonicalStellarProjectionDescriptor().bFinalized && IsValid(It->GetPreviewHomeSystem()))
			{
				Generator = *It;
				CatalogueGenerator = Generator;
				break;
			}
		}
	}
	if (!IsValid(Generator))
	{
		return false;
	}
	const FAPSCanonicalStellarProjectionDescriptor& Descriptor = Generator->GetCanonicalStellarProjectionDescriptor();
	AActor* Home = Generator->GetPreviewHomeSystem();
	if (!Descriptor.bFinalized || !IsValid(Home))
	{
		return false;
	}
	if (CatalogueHome.Get() == Home && CatalogueBuildSerial == Descriptor.ProxyBuildSerial
		&& CatalogueMutationSerial == Descriptor.TransformMutationSerial && !CatalogueFromHome.IsEmpty())
	{
		return true;
	}
	AStarCluster* Cluster = nullptr;
	TArray<AActor*> Attached;
	Generator->GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached)
	{
		Cluster = Cast<AStarCluster>(Actor);
		if (Cluster)
		{
			break;
		}
	}
	if (!IsValid(Cluster) || !IsValid(Cluster->StarMeshInstances))
	{
		return false;
	}

	const FVector HomeLocation = Home->GetActorLocation();
	TArray<FVector> FromHome;
	FromHome.Reserve(Cluster->PotentialStarSystems.Num());
	if (Descriptor.bConsumedFinalizedDataset)
	{
		// A committed game draws the immutable proxy transforms under a generator root scaled back to physical size:
		// read them exactly as drawn, once the stellar view has applied that scale.
		const double RequiredScale = Descriptor.Galaxy.PositionScale > 0.0 ? 1.0 / Descriptor.Galaxy.PositionScale : 0.0;
		const double RootScale = Generator->GetActorScale3D().GetAbsMax();
		if (!FMath::IsFinite(RequiredScale) || RequiredScale <= 0.0 || FMath::Abs(RootScale / RequiredScale - 1.0) > 1.0e-3)
		{
			return false;
		}
		const FTransform ComponentTransform = Cluster->StarMeshInstances->GetComponentTransform();
		for (int32 Index = 0; Index < Cluster->PotentialStarSystems.Num(); ++Index)
		{
			// A materialized system (the home) has star actors of its own; proxies overlapping it are not drawn.
			if (!Cluster->PotentialStarSystems[Index].bMaterialized && Cluster->SystemProxyBaseTransforms.IsValidIndex(Index)
				&& Generator->GetGameplayStellarSuppression(
					Generator->MakeGameplayStellarKey(Cluster->StarMeshInstances, Index)) == 0)
			{
				FromHome.Add(ComponentTransform.TransformPosition(Cluster->SystemProxyBaseTransforms[Index].GetLocation())
					- HomeLocation);
			}
		}
	}
	else
	{
		// Physical observer layers place every star at its canonical offset from the home system.
		const FAPSCanonicalStellarProjectionFrame& Frame = Descriptor.StarCluster;
		for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
		{
			if (!Record.bMaterialized && Generator->GetGameplayStellarSuppression(
				Generator->MakeGameplayStellarKey(Cluster->StarMeshInstances, Record.InstanceIndex)) == 0)
			{
				FromHome.Add(Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm);
			}
		}
	}
	// The catalogue's extent and spacing set the scale of star flight: charted space ends a little beyond its bounding
	// sphere, and the log records how far apart the stars really are (a generated cluster spans hundreds of AU).
	double NearestCm = TNumericLimits<double>::Max();
	FVector Centroid = FVector::ZeroVector;
	for (const FVector& Offset : FromHome)
	{
		NearestCm = FMath::Min(NearestCm, Offset.Size());
		Centroid += Offset;
	}
	Centroid = FromHome.IsEmpty() ? FVector::ZeroVector : Centroid / static_cast<double>(FromHome.Num());
	double RadiusCm = 0.0;
	for (const FVector& Offset : FromHome)
	{
		RadiusCm = FMath::Max(RadiusCm, FVector::Dist(Offset, Centroid));
	}
	TArray<double> Spacings;
	const int32 SampleCount = FMath::Min(FromHome.Num(), 64);
	for (int32 Sample = 0; Sample < SampleCount; ++Sample)
	{
		const FVector& Star = FromHome[(static_cast<int64>(Sample) * 7919) % FromHome.Num()];
		double BestSquared = TNumericLimits<double>::Max();
		for (const FVector& Other : FromHome)
		{
			const double DistanceSquared = FVector::DistSquared(Star, Other);
			BestSquared = DistanceSquared > 0.0 ? FMath::Min(BestSquared, DistanceSquared) : BestSquared;
		}
		Spacings.Add(FMath::Sqrt(BestSquared));
	}
	Spacings.Sort();
	double OutermostOrbitCm = 0.0;
	for (TActorIterator<APlanet> It(World); It; ++It)
	{
		if (IsValid(*It) && IsValid(It->ParentStar))
		{
			OutermostOrbitCm = FMath::Max(OutermostOrbitCm, FVector::Dist(It->GetActorLocation(), It->ParentStar->GetActorLocation()));
		}
	}
	constexpr double AU = APSShipFlightModelLocal::AstronomicalUnitCm;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Flight] star catalogue: %d systems (%s): nearest %.1f AU from home, cluster radius %.0f AU around a centre %.0f AU from home, star spacing median %.1f AU (10%% %.1f, 90%% %.1f); outermost planet orbit %.1f AU; system sphere %.0f AU"),
		FromHome.Num(), Descriptor.bConsumedFinalizedDataset ? TEXT("as drawn") : TEXT("canonical"),
		FromHome.IsEmpty() ? 0.0 : NearestCm / AU, RadiusCm / AU, Centroid.Size() / AU,
		Spacings.IsEmpty() ? 0.0 : Spacings[Spacings.Num() / 2] / AU,
		Spacings.IsEmpty() ? 0.0 : Spacings[Spacings.Num() / 10] / AU,
		Spacings.IsEmpty() ? 0.0 : Spacings[Spacings.Num() * 9 / 10] / AU, OutermostOrbitCm / AU, InterstellarDistanceAU);
	CatalogueCenterFromHome = Centroid;
	CatalogueRadiusCm = RadiusCm;
	CatalogueSpacingMedianCm = Spacings.IsEmpty() ? 0.0 : Spacings[Spacings.Num() / 2];
	CatalogueFromHome = MoveTemp(FromHome);
	CatalogueHome = Home;
	CatalogueBuildSerial = Descriptor.ProxyBuildSerial;
	CatalogueMutationSerial = Descriptor.TransformMutationSerial;
	NearestCatalogueStars.Reset();
	CourseCatalogueStar = INDEX_NONE;
	return !CatalogueFromHome.IsEmpty();
}

void UAPSShipFlightModel::ScanStarCatalogue(const FVector& Location, const FVector& Heading)
{
	NearestCatalogueStars.Reset();
	CourseCatalogueStar = INDEX_NONE;
	CatalogueScanHeading = Heading;
	if (!RebuildStarCatalogue())
	{
		if (!bCatalogueMissingLogged && CatalogueGenerator.IsValid())
		{
			bCatalogueMissingLogged = true;
			UE_LOG(LogTemp, Log, TEXT("[APS.Flight] star catalogue not ready yet (stellar view has not placed it); stars as actors only"));
		}
		return;
	}
	// One pass over the catalogue: the nearest few stars and the system the course runs into. No allocation, no sort.
	const FVector ObserverFromHome = Location - CatalogueHome->GetActorLocation();
	const double SystemRadiusCm = InterstellarDistanceAU * APSShipFlightModelLocal::AstronomicalUnitCm;
	const bool bCourse = !Heading.IsNearlyZero();
	double BestCourseCm = TNumericLimits<double>::Max();
	TArray<TPair<double, int32>, TInlineAllocator<APSShipFlightModelLocal::NearestGeneratedStars>> Nearest;
	int32 Farthest = INDEX_NONE;
	for (int32 Index = 0; Index < CatalogueFromHome.Num(); ++Index)
	{
		const FVector ToStar = CatalogueFromHome[Index] - ObserverFromHome;
		const double DistanceSquared = ToStar.SizeSquared();
		if (Nearest.Num() < APSShipFlightModelLocal::NearestGeneratedStars
			|| DistanceSquared < Nearest[Farthest].Key)
		{
			if (Nearest.Num() < APSShipFlightModelLocal::NearestGeneratedStars)
			{
				Nearest.Emplace(DistanceSquared, Index);
			}
			else
			{
				Nearest[Farthest] = TPair<double, int32>(DistanceSquared, Index);
			}
			Farthest = 0;
			for (int32 Slot = 1; Slot < Nearest.Num(); ++Slot)
			{
				Farthest = Nearest[Slot].Key > Nearest[Farthest].Key ? Slot : Farthest;
			}
		}
		if (bCourse)
		{
			const double Clearance = APSFlightBandModel::CourseClearanceCm(ToStar, Heading, SystemRadiusCm, CourseMissScale);
			if (Clearance >= 0.0 && Clearance < BestCourseCm)
			{
				BestCourseCm = Clearance;
				CourseCatalogueStar = Index;
			}
		}
	}
	for (const TPair<double, int32>& Star : Nearest)
	{
		NearestCatalogueStars.Add(Star.Value);
	}
	ScanGalaxyStars(Location, Heading);
}

void UAPSShipFlightModel::ScanGalaxyStars(const FVector& Location, const FVector& Heading)
{
	// Rio 03.10 ("every star must be reachable"): the drawn galaxy stars are stars to fly to as the cluster's are: the
	// nearest few limit the speed, and the one the course runs into slows the drive for its arrival.
	NearestGalaxyStars.Reset();
	GalaxySpacingCm = 0.0;
	const AActor* Home = CatalogueHome.Get();
	TArray<APSGalaxyGpuStars::FNearStar> Near;
	if (!Home || !APSGalaxyGpuStars::FindNearStars(GetWorld(), Location, APSShipFlightModelLocal::GalaxyScanStars, 1.0e30, Near))
	{
		return;
	}
	const AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(GetWorld());
	const FVector HomeLocation = Home->GetActorLocation();
	const double SystemRadiusCm = InterstellarDistanceAU * APSShipFlightModelLocal::AstronomicalUnitCm;
	const auto RadiusOf = [Galaxy](const APSGalaxyGpuStars::FNearStar& Star)
	{
		FGalaxyCatalogStarRecord Record;
		if (!Galaxy || !Galaxy->StarCatalog.ResolveStar(Star.CatalogIndex, Record)) return APSShipFlightModelLocal::SolarRadiusCm;
		return APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass)
			* FMath::Max(static_cast<double>(Record.RadiusScale), 0.0) * APSCanonicalStellarProjection::SolarRadiusCm;
	};
	int32 Course = INDEX_NONE;
	double BestCourseCm = TNumericLimits<double>::Max();
	if (!Heading.IsNearlyZero())
	{
		for (int32 Index = 0; Index < Near.Num(); ++Index)
		{
			const double Clearance = APSFlightBandModel::CourseClearanceCm(Near[Index].WorldLocation - Location, Heading,
				SystemRadiusCm, CourseMissScale);
			if (Clearance >= 0.0 && Clearance < BestCourseCm)
			{
				BestCourseCm = Clearance;
				Course = Index;
			}
		}
	}
	for (int32 Index = 0; Index < Near.Num(); ++Index)
	{
		if (Index < APSShipFlightModelLocal::NearestGeneratedStars || Index == Course)
		{
			NearestGalaxyStars.Add(FVector4(Near[Index].WorldLocation - HomeLocation, RadiusOf(Near[Index])));
		}
	}
	// The field's local spacing: the median of the nearest few stars' distances to their own nearest neighbour.
	TArray<double, TInlineAllocator<APSShipFlightModelLocal::NearestGeneratedStars>> Gaps;
	for (int32 Index = 0; Index < Near.Num() && Index < APSShipFlightModelLocal::NearestGeneratedStars; ++Index)
	{
		double BestSquared = TNumericLimits<double>::Max();
		for (int32 Other = 0; Other < Near.Num(); ++Other)
		{
			if (Other != Index)
			{
				BestSquared = FMath::Min(BestSquared, FVector::DistSquared(Near[Index].WorldLocation, Near[Other].WorldLocation));
			}
		}
		if (BestSquared < TNumericLimits<double>::Max())
		{
			Gaps.Add(FMath::Sqrt(BestSquared));
		}
	}
	Gaps.Sort();
	GalaxySpacingCm = Gaps.IsEmpty() ? 0.0 : Gaps[Gaps.Num() / 2];
}

void UAPSShipFlightModel::UpdateGroundProbe(float DeltaTime)
{
	ASpaceship* Ship = GetShip();
	TimeSinceGroundProbe += DeltaTime;
	GroundProbeElapsed += DeltaTime;
	const FVector Down = Ship ? Ship->ActiveGravityDirection.GetSafeNormal() : FVector::ZeroVector;
	if (GroundProbeElapsed >= GroundProbeInterval)
	{
		GroundProbeElapsed = 0.0f;
		TimeSinceGroundProbe = 0.0f;
		ProbedGroundClearanceCm = -1.0;
		if (Ship && GetWorld())
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipGroundProbe), false, Ship);
			if (IsValid(Ship->Pilot))
			{
				Params.AddIgnoredActor(Ship->Pilot);
			}
			FHitResult Hit;
			const FVector Start = Ship->GetActorLocation();
			const double HullReach = Ship->SpaceshipHull ? Ship->SpaceshipHull->Bounds.SphereRadius * 0.5 : 0.0;
			if (!Down.IsNearlyZero()
				&& GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Down * GroundProbeLength, ECC_Visibility, Params))
			{
				ProbedGroundClearanceCm = FMath::Max(static_cast<double>(Hit.Distance) - HullReach, 0.0);
			}
			else if (const double TerrainClearance = MeasureTerrainClearanceCm(Start); TerrainClearance >= 0.0)
			{
				ProbedGroundClearanceCm = FMath::Max(TerrainClearance - HullReach, 0.0);
			}
		}
	}
	// Between probes the clearance follows the vertical speed, so a fast descent never reads a stale probe.
	GroundClearanceCm = ProbedGroundClearanceCm < 0.0 || !Ship
		? -1.0
		: FMath::Max(ProbedGroundClearanceCm - FVector::DotProduct(Ship->KinematicVelocity, Down) * TimeSinceGroundProbe, 0.0);
}

double UAPSShipFlightModel::MeasureTerrainClearanceCm(const FVector& Location) const
{
	// WorldScape builds collision only in the last ~140 m above the ground, so higher up the line trace sees nothing.
	// Its height noise (the canonical surface the placement resolver also samples) gives the terrain, and the sea
	// level of a liquid world, at any altitude.
	double Best = -1.0;
	for (const FFlightBody& Body : FlightBodies)
	{
		const APlanetaryBody* Planet = Cast<APlanetaryBody>(Body.Actor.Get());
		APlanetarySurfaceGenerator* Surface = Planet ? Planet->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0)
		{
			continue;
		}
		const FVector Center = Root->GetActorLocation();
		const FVector Offset = Location - Center;
		const double Distance = Offset.Size();
		if (Distance <= UE_DOUBLE_SMALL_NUMBER
			|| Distance - Root->PlanetScale > APSShipFlightModelLocal::TerrainQueryAltitudeCm
			// A shared root may be carrying another body's surface.
			|| !Surface->IsSurfaceProfileCurrent(Planet))
		{
			continue;
		}
		const FVector Direction = Offset / Distance;
		double SurfaceRadius = Root->PlanetScale + Root->GetGroundHeight(Center + Direction * Root->PlanetScale, false);
		if (Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None)
		{
			SurfaceRadius = FMath::Max(SurfaceRadius, Root->PlanetScale
				+ static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity);
		}
		const double Clearance = FMath::Max(Distance - SurfaceRadius, 0.0);
		Best = Best < 0.0 ? Clearance : FMath::Min(Best, Clearance);
	}
	return Best;
}

void UAPSShipFlightModel::UpdateNearestSurface(float DeltaTime)
{
	BodyRefreshElapsed += DeltaTime;
	if (BodyRefreshElapsed >= BodyRefreshInterval)
	{
		BodyRefreshElapsed = 0.0f;
		RefreshFlightBodies();
	}
	UpdateGroundProbe(DeltaTime);

	const ASpaceship* Ship = GetShip();
	const FVector Location = Ship ? Ship->GetActorLocation() : FVector::ZeroVector;
	// Where the ship is going: its velocity, or the nose (tail when reversing) from rest.
	FVector Heading = Ship ? Ship->KinematicVelocity.GetSafeNormal() : FVector::ZeroVector;
	if (Heading.IsNearlyZero() && Ship)
	{
		Heading = Ship->GetShipForwardVector() * (Ship->ForwardInput < 0.0f ? -1.0 : 1.0);
	}

	// The star catalogue: every 0.1 s in the fast bands, and in STELLAR soon after the course turns, since it slows
	// for the system its course runs into. A scan is one pass over ~36k points.
	CatalogueScanElapsed += DeltaTime;
	const bool bFastBand = FlightBand >= EAPSFlightBand::Cruise || bStarDrive;
	const bool bCourseTurned = (FlightBand == EAPSFlightBand::Stellar || bStarDrive) && CatalogueScanElapsed >= 0.05f
		&& !Heading.IsNearlyZero()
		&& FVector::DotProduct(Heading, CatalogueScanHeading) < APSShipFlightModelLocal::CourseRescanCosine;
	if (CatalogueScanElapsed >= (bFastBand ? CatalogueScanInterval : 1.0f) || bCourseTurned)
	{
		CatalogueScanElapsed = 0.0f;
		ScanStarCatalogue(Location, Heading);
	}

	// The limit of every body grows with the departure factor while the ship heads away from it. Taking the smallest
	// scaled distance keeps the limit continuous when the nearest body changes on the way from one star to the next.
	const double ExtraDeparture = FMath::Max(DepartureSpeedFactor, 1.0) - 1.0;
	const auto DepartureScale = [&Heading, ExtraDeparture](const FVector& Outward)
	{
		return 1.0 + ExtraDeparture * FMath::Clamp(FVector::DotProduct(Heading, Outward), 0.0, 1.0);
	};
	const double BareSystemCm = InterstellarDistanceAU * APSShipFlightModelLocal::AstronomicalUnitCm;
	double Nearest = -1.0;
	double LimitDistance = -1.0;
	double LimitLocal = -1.0;
	double Solid = -1.0;
	double NearestStar = -1.0;
	double SystemGap = TNumericLimits<double>::Max();
	double Course = -1.0;
	FVector SolidOutward = FVector::ZeroVector;
	const FString* Name = nullptr;
	const auto AddBody = [&](const FVector& Center, double RadiusCm, const FString& BodyName, bool bSolid, bool bStar,
		double SystemRadiusCm)
	{
		const FVector Offset = Location - Center;
		const double Distance = FMath::Max(Offset.Size() - RadiusCm, 0.0);
		if (Nearest < 0.0 || Distance < Nearest)
		{
			Nearest = Distance;
			Name = &BodyName;
		}
		const FVector Outward = Offset.GetSafeNormal();
		const double Scaled = Distance * DepartureScale(Outward);
		LimitDistance = LimitDistance < 0.0 ? Scaled : FMath::Min(LimitDistance, Scaled);
		if (!bStar)
		{
			LimitLocal = LimitLocal < 0.0 ? Scaled : FMath::Min(LimitLocal, Scaled);
		}
		if (bSolid && (Solid < 0.0 || Distance < Solid))
		{
			Solid = Distance;
			SolidOutward = Outward;
		}
		if (bStar)
		{
			NearestStar = NearestStar < 0.0 ? Distance : FMath::Min(NearestStar, Distance);
			SystemGap = FMath::Min(SystemGap, Offset.Size() - SystemRadiusCm);
			const double Clearance = Heading.IsNearlyZero() ? -1.0
				: APSFlightBandModel::CourseClearanceCm(-Offset, Heading, SystemRadiusCm, CourseMissScale);
			if (Clearance >= 0.0)
			{
				Course = Course < 0.0 ? Clearance : FMath::Min(Course, Clearance);
			}
		}
	};
	for (const FFlightBody& Body : FlightBodies)
	{
		FVector Center = Body.Center;
		if (!Body.bFixedLocation)
		{
			const AActor* Actor = Body.Actor.Get();
			if (!Actor)
			{
				continue;
			}
			Center = Actor->GetActorTransform().TransformPosition(Body.Center);
		}
		AddBody(Center, Body.RadiusCm, Body.Name, Body.bSolid, Body.bStar, Body.SystemRadiusCm);
	}
	if (const AActor* Home = CatalogueHome.Get())
	{
		// Catalogue stars from the last scan, placed from the home system's current location.
		static const FString StarName(TEXT("STAR"));
		const FVector HomeLocation = Home->GetActorLocation();
		for (const int32 Index : NearestCatalogueStars)
		{
			if (CatalogueFromHome.IsValidIndex(Index))
			{
				AddBody(HomeLocation + CatalogueFromHome[Index], APSShipFlightModelLocal::SolarRadiusCm, StarName, true, true,
					BareSystemCm);
			}
		}
		if (CatalogueFromHome.IsValidIndex(CourseCatalogueStar) && !NearestCatalogueStars.Contains(CourseCatalogueStar))
		{
			AddBody(HomeLocation + CatalogueFromHome[CourseCatalogueStar], APSShipFlightModelLocal::SolarRadiusCm, StarName,
				true, true, BareSystemCm);
		}
		// The galaxy's drawn stars from the last scan (a giant's system starts a few of its radii out).
		for (const FVector4& Star : NearestGalaxyStars)
		{
			AddBody(HomeLocation + FVector(Star), Star.W, StarName, true, true, FMath::Max(BareSystemCm, Star.W * 3.0));
		}
	}
	// The edge of charted space is a surface too: the star field ends a little beyond the catalogue's bounding sphere
	// (Rio, 30.09: past the edge of the cluster the ship ran off into the void), and beyond ~42 light years from the
	// world origin the renderer loses precision (the DoubleFloat ensure, 29.09). The ship slows for it and stops at it.
	static const FString EdgeName(TEXT("EDGE OF CHARTED SPACE"));
	FVector EdgeCenter = FVector::ZeroVector;
	double EdgeRadiusCm = MaxTravelRadiusLightYears * APSShipFlightModelLocal::LightYearCm;
	if (const AActor* Home = CatalogueHome.Get(); Home && CatalogueRadiusCm > 0.0)
	{
		FVector ChartedCenter = Home->GetActorLocation() + CatalogueCenterFromHome;
		double ChartedRadiusCm = CatalogueRadiusCm * APSShipFlightModelLocal::ChartedSpaceScale
			+ APSShipFlightModelLocal::ChartedSpaceMarginCm;
		// Rio 03.10 ("every star must be reachable"): with the galaxy's stars charted, charted space is the galaxy.
		FVector GalaxyCenter;
		double GalaxyRadiusCm = 0.0;
		if (APSShipFlightModelLocal::CVarGalaxyCharted.GetValueOnGameThread() != 0
			&& APSGalaxyGpuStars::GetIndexedBounds(GetWorld(), GalaxyCenter, GalaxyRadiusCm))
		{
			// (Grown to hold the cluster's own charted sphere, should the cluster stand at the galaxy's rim.)
			ChartedRadiusCm = FMath::Max(GalaxyRadiusCm * APSShipFlightModelLocal::ChartedSpaceScale
				+ APSShipFlightModelLocal::ChartedSpaceMarginCm, FVector::Dist(GalaxyCenter, ChartedCenter) + ChartedRadiusCm);
			ChartedCenter = GalaxyCenter;
		}
		if (EdgeRadiusCm <= 0.0 || ChartedCenter.Size() + ChartedRadiusCm < EdgeRadiusCm)
		{
			EdgeCenter = ChartedCenter;
			EdgeRadiusCm = ChartedRadiusCm;
		}
	}
	CatalogueGapCm = 0.0;
	if (const AActor* Home = CatalogueHome.Get(); Home && CatalogueRadiusCm > 0.0)
	{
		CatalogueGapCm = FMath::Max(FVector::Dist(Location, Home->GetActorLocation() + CatalogueCenterFromHome)
			- CatalogueRadiusCm, 0.0);
	}
	const FVector FromEdgeCenter = Location - EdgeCenter;
	const double FromCenter = FromEdgeCenter.Size();
	if (EdgeRadiusCm > 0.0 && FromCenter > 1.0)
	{
		const double Gap = FMath::Max(EdgeRadiusCm - FromCenter, 0.0);
		const FVector Inward = -FromEdgeCenter / FromCenter;
		// Outside the edge (a spawn or save out there) the way back in is always open.
		const bool bReturning = Gap <= 0.0 && FVector::DotProduct(Heading, Inward) > 0.0;
		if (!bReturning)
		{
			const double Scaled = Gap * DepartureScale(Inward);
			LimitDistance = LimitDistance < 0.0 ? Scaled : FMath::Min(LimitDistance, Scaled);
			if (Solid < 0.0 || Gap < Solid)
			{
				Solid = Gap;
				SolidOutward = Inward;
			}
			if (!Heading.IsNearlyZero())
			{
				const double Exit = APSFlightBandModel::CourseExitCm(FromEdgeCenter, Heading, EdgeRadiusCm);
				Course = Course < 0.0 ? Exit : FMath::Min(Course, Exit);
			}
		}
		if (Nearest < 0.0 || Gap < Nearest)
		{
			Nearest = Gap;
			Name = &EdgeName;
		}
	}
	NearestStarDistanceCm = NearestStar;
	bSystemGapKnown = SystemGap < TNumericLimits<double>::Max();
	NearestSystemGapCm = bSystemGapKnown ? SystemGap : 0.0;
	// Terrain above the base sphere is only known from the ground probe.
	GroundLimitClearanceCm = -1.0;
	if (GroundClearanceCm >= 0.0)
	{
		Nearest = Nearest < 0.0 ? GroundClearanceCm : FMath::Min(Nearest, GroundClearanceCm);
		const FVector Up = Ship ? -Ship->ActiveGravityDirection.GetSafeNormal() : FVector::ZeroVector;
		GroundLimitClearanceCm = GroundClearanceCm * DepartureScale(Up);
		LimitDistance = LimitDistance < 0.0 ? GroundLimitClearanceCm : FMath::Min(LimitDistance, GroundLimitClearanceCm);
		LimitLocal = LimitLocal < 0.0 ? GroundLimitClearanceCm : FMath::Min(LimitLocal, GroundLimitClearanceCm);
		if (!Up.IsNearlyZero() && (Solid < 0.0 || GroundClearanceCm < Solid))
		{
			Solid = GroundClearanceCm;
			SolidOutward = Up;
		}
	}
	NearestSurfaceDistanceCm = Nearest;
	LimitSurfaceDistanceCm = LimitDistance;
	LimitLocalDistanceCm = LimitLocal;
	CourseClearanceCm = Course;
	SolidSurfaceDistanceCm = Solid;
	SolidSurfaceOutward = SolidOutward;
	NearestBodyName = Name ? *Name : FString();
}

double UAPSShipFlightModel::BandBoost(EAPSFlightBand InBand, double Alpha) const
{
	return 1.0 + (FMath::Max(GetBandSettings(InBand).BoostMultiplier, 1.0) - 1.0) * FMath::Clamp(Alpha, 0.0, 1.0);
}

double UAPSShipFlightModel::BandLimitCm(EAPSFlightBand InBand, double Alpha) const
{
	FAPSFlightBandSettings Settings = GetBandSettings(InBand);
	const double FactorScale = FMath::Max(APSShipFlightModelLocal::CVarDistanceFactorScale.GetValueOnGameThread(), 0.0f);
	Settings.DistanceSpeedFactor *= FactorScale;
	Settings.StarDistanceSpeedFactor *= FactorScale;
	const float SurfaceFactorOverride = APSShipFlightModelLocal::CVarSurfaceSpeedFactor.GetValueOnGameThread();
	APSFlightBandModel::FSpeedLimitInputs Inputs;
	Inputs.SurfaceCm = LimitSurfaceDistanceCm;
	Inputs.LocalSurfaceCm = LimitLocalDistanceCm;
	Inputs.GroundClearanceCm = GroundLimitClearanceCm;
	Inputs.StarCm = NearestStarDistanceCm;
	const double SpacingCap = APSShipFlightModelLocal::CVarStarSpacingCap.GetValueOnGameThread();
	if (SpacingCap > 0.0 && CatalogueSpacingMedianCm > 0.0 && Inputs.StarCm >= 0.0)
	{
		Inputs.StarCm = FMath::Min(Inputs.StarCm, SpacingCap * CatalogueSpacingMedianCm);
	}
	Inputs.CourseCm = CourseClearanceCm;
	Inputs.DistanceFloorCm = DistanceLimitFloor * 100.0;
	// aps.Ship.Feel (Rio 02.10, "AUTO flies too fast near the surface"): half the ground limit and a lower floor, so low
	// flight is slower; climbing away still doubles it (DepartureSpeedFactor), so the way out stays quick.
	const bool bFeel = APSShipFlightModelLocal::FeelEnabled();
	Inputs.SurfaceFactor = SurfaceFactorOverride >= 0.0f ? SurfaceFactorOverride : SurfaceSpeedFactor * (bFeel ? 0.5 : 1.0);
	Inputs.SurfaceFloorCm = SurfaceSpeedFloor * 100.0 * (bFeel ? 0.6 : 1.0);
	const EAPSFlightBand Below = Settings.bCourseLimit ? NeighbourBand(InBand, -1) : InBand;
	double Limit = APSFlightBandModel::BandSpeedLimitCm(Settings, BandBoost(InBand, Alpha), Inputs,
		Below != InBand ? BandLimitCm(Below, Alpha) : -1.0);
	const double OutsideSeconds = APSShipFlightModelLocal::CVarOutsideClusterSeconds.GetValueOnGameThread();
	if (OutsideSeconds > 0.0 && CatalogueGapCm > 0.0 && CatalogueRadiusCm > 0.0)
	{
		// Past the cluster's edge the speed stays bounded: the way back is as quick as the way out.
		Limit = FMath::Min(Limit, FMath::Max(CatalogueRadiusCm / OutsideSeconds, Inputs.DistanceFloorCm));
	}
	return Limit;
}

void UAPSShipFlightModel::UpdateAutoBand(double SpeedCm, double Throttle, float DeltaTime)
{
	const ASpaceship* Ship = GetShip();
	UpShiftBlockSeconds = FMath::Max(UpShiftBlockSeconds - DeltaTime, 0.0f);
	if (!Ship || !bAutoBands || bManualBand)
	{
		return;
	}
	// Surroundings: which bands make sense here at all.
	const bool bStation = Cast<ASpaceStation>(Ship->ActiveGravitySource.Get()) != nullptr;
	const bool bInAir = IsInAtmosphere() && !bStation;
	// Outside every star system: a quarter beyond its planets, or InterstellarDistanceAU around a bare star.
	const bool bInterstellar = !bSystemGapKnown || NearestSystemGapCm > 0.0;
	// Rio 02.10 (AUTO flew 500 m/s a hundred metres up): ORBITAL's distance floor beat FLIGHT's ground limit, so AUTO
	// shifted up right above the ground, and over an airless moon it never left ORBITAL. Low over any ground FLIGHT is
	// the top band and the landing hover is offered; above the ceiling AUTO may shift up, and it drops back below 70% of it.
	const bool bFeel = APSShipFlightModelLocal::FeelEnabled();
	const bool bLowFlight = bFeel && !bStation && GroundClearanceCm >= 0.0
		&& GroundClearanceCm < (FlightBand > EAPSFlightBand::Flight ? 0.7 : 1.0) * APSShipFlightModelLocal::LowFlightCeilingCm;
	EAPSFlightBand Lowest = bStation || bInAir || bLowFlight ? EAPSFlightBand::Flight : EAPSFlightBand::Orbital;
	EAPSFlightBand Highest = bInterstellar ? EAPSFlightBand::Stellar : EAPSFlightBand::Cruise;
	while (Highest > Lowest && !IsBandAvailable(Highest))
	{
		Highest = static_cast<EAPSFlightBand>(static_cast<uint8>(Highest) - 1);
	}
	if (bLowFlight)
	{
		Highest = EAPSFlightBand::Flight;
	}
	const bool bHover = Throttle <= 0.1 && SpeedCm < HoverSpeed * 100.0
		&& (bStation || ((bInAir || bLowFlight) && GroundClearanceCm >= 0.0 && GroundClearanceCm < LandingClearance * 100.0));

	EAPSFlightBand Wanted = FlightBand;
	const TCHAR* Reason = TEXT("auto");
	if (Wanted > Highest)
	{
		// Out of interstellar space, into a gravity well or an atmosphere: drop at once and shed the speed.
		Wanted = Highest;
		Reason = TEXT("auto: surroundings");
	}
	else if (Wanted == EAPSFlightBand::Maneuver)
	{
		// Leave the hover when W runs into its limit, or once no station or ground is near.
		if (!(bStation || bInAir || bLowFlight) || (Throttle > 0.5 && SpeedCm >= 0.85 * BandLimitCm(Wanted, BoostAlpha)))
		{
			Wanted = Lowest;
			Reason = TEXT("auto: leaving the hover");
		}
	}
	else if (Wanted < Lowest)
	{
		Wanted = Lowest;
		Reason = TEXT("auto: surroundings");
	}
	else
	{
		const double Here = BandLimitCm(Wanted, BoostAlpha);
		const EAPSFlightBand Up = NeighbourBand(Wanted, 1);
		const EAPSFlightBand Down = NeighbourBand(Wanted, -1);
		if (Up != Wanted && Up <= Highest && Throttle > 0.5 && SpeedCm >= 0.85 * Here && UpShiftBlockSeconds <= 0.0f
			&& BandLimitCm(Up, BoostAlpha) >= (bFeel ? 1.4 : 1.25) * Here)
		{
			// Like an automatic gearbox: at the top of this band, and the next one would go faster here.
			Wanted = Up;
			Reason = TEXT("auto: shift up");
		}
		else if (Down != Wanted && Down >= Lowest)
		{
			// aps.Ship.Feel: a wider gap between shifting up (1.4x) and down (1.0x), held longer, so AUTO stops hunting
			// between two bands (Rio's log 02.10: ORBITAL <-> FLIGHT every few seconds near the ground).
			const bool bNoGain = Here <= (bFeel ? 1.0 : 1.05) * BandLimitCm(Down, BoostAlpha);
			const bool bSlowedDown = Throttle <= 0.1 && SpeedCm < 0.4 * GetBandSettings(Down).MaxSpeed * 100.0;
			AutoShiftHold = bNoGain || bSlowedDown ? AutoShiftHold + DeltaTime : 0.0f;
			if (AutoShiftHold >= (bFeel ? 0.8f : 0.4f))
			{
				// Closing on a body (this band is no faster than the one below) or coasting slow: shift down.
				Wanted = Down;
				Reason = bNoGain ? TEXT("auto: closing in") : TEXT("auto: slowed down");
				// Rio 04.10 (the approach to HQ: ORBITAL <-> CRUISE 8 times in 80 s with W held, a 50-85 ms hitch at every
				// change, each re-scanning the bodies and the catalogue and restarting the arrival forecast): closing in,
				// the band below stays a while before AUTO shifts up again.
				if (bNoGain)
				{
					UpShiftBlockSeconds = FMath::Max(APSShipFlightModelLocal::CVarCloseInHoldSeconds.GetValueOnGameThread(), 0.0f);
				}
			}
		}
		else if (bHover && Down == EAPSFlightBand::Maneuver)
		{
			AutoShiftHold += DeltaTime;
			if (AutoShiftHold >= 0.5f)
			{
				Wanted = EAPSFlightBand::Maneuver;
				Reason = bStation ? TEXT("auto: docking hover") : TEXT("auto: landing hover");
			}
		}
		else
		{
			AutoShiftHold = 0.0f;
		}
	}
	if (Wanted != FlightBand && IsBandAvailable(Wanted))
	{
		ApplyBand(Wanted, Reason);
	}
}

FVector UAPSShipFlightModel::StepVelocity(const FAPSFlightBandSettings& Band, const FVector& LocalInput, double Limit,
	double Boost, double Drag, float DeltaTime) const
{
	const ASpaceship* Ship = GetShip();
	const FVector Forward = Ship->GetShipForwardVector();
	const FVector Right = Ship->GetShipRightVector();
	const FVector Up = Ship->GetShipUpVector();
	const double Acceleration = Band.Acceleration * 100.0 * Boost
		* APSFlightBandModel::ClassAgility(static_cast<uint8>(Ship->SizeClass));
	FVector Velocity = Ship->KinematicVelocity;

	switch (Band.Control)
	{
	case EAPSFlightBandControl::Hover:
		return APSFlightBandModel::HoverStep(Velocity,
			Forward * LocalInput.X * Limit + (Right * LocalInput.Y + Up * LocalInput.Z) * Limit * Band.LateralFraction,
			Acceleration, DeltaTime);

	case EAPSFlightBandControl::Cruise:
	{
		const double Speed = APSFlightBandModel::CruiseSpeedStep(Velocity.Size(), LocalInput.X, Limit,
			Band.CruiseResponse * Boost, BrakeRate, Drag, DeltaTime);
		// Cruise speed is a magnitude along the nose; the velocity always turns toward +Forward.
		const FVector Heading = Velocity.IsNearlyZero() ? Forward : Velocity.GetSafeNormal();
		const double Align = 1.0 - FMath::Exp(-FMath::Max(Band.AssistRate, 0.0) * DeltaTime);
		const FVector Direction = (Heading + (Forward - Heading) * Align).GetSafeNormal();
		return (Direction.IsNearlyZero() ? Forward : Direction) * Speed;
	}

	case EAPSFlightBandControl::Drive:
	case EAPSFlightBandControl::Assist:
	default:
	{
		// The engines cancel drift: the velocity turns toward the nose and keeps its magnitude.
		Velocity = APSFlightBandModel::AlignToNose(Velocity, Forward, Band.AssistRate, DeltaTime);
		double ForwardSpeed = FVector::DotProduct(Velocity, Forward);
		FVector Lateral = Velocity - Forward * ForwardSpeed;
		if (LocalInput.X > UE_KINDA_SMALL_NUMBER)
		{
			ForwardSpeed = APSFlightBandModel::TaperedSpeedStep(ForwardSpeed, Limit * LocalInput.X, Acceleration, DeltaTime);
		}
		else if (LocalInput.X < -UE_KINDA_SMALL_NUMBER)
		{
			// S brakes, then reverses gently.
			const double ReverseTarget = -Limit * Band.LateralFraction * -LocalInput.X;
			if (ForwardSpeed > ReverseTarget)
			{
				ForwardSpeed = FMath::Max(ReverseTarget, ForwardSpeed - Acceleration * -LocalInput.X * DeltaTime);
			}
		}
		else
		{
			// Released: Drive slows like a car, Assist keeps the speed in space; air drags both.
			const double ReleaseDrag = (Band.Control == EAPSFlightBandControl::Drive ? Band.ReleaseDrag : 0.0) + Drag;
			if (ReleaseDrag > 0.0)
			{
				ForwardSpeed *= FMath::Exp(-ReleaseDrag * DeltaTime);
			}
		}
		// Strafe and vertical keys set a velocity across the nose; without them the remaining drift is cancelled.
		Lateral = APSFlightBandModel::HoverStep(Lateral, (Right * LocalInput.Y + Up * LocalInput.Z) * Limit * Band.LateralFraction,
			Acceleration, DeltaTime);
		return Forward * ForwardSpeed + Lateral;
	}
	}
}

bool UAPSShipFlightModel::ApplyTranslation(float DeltaTime)
{
	ASpaceship* Ship = GetShip();
	if (Ship && Ship->IsGroundVehicle())
	{
		// Rio 02.10: rover, hover and drone drive in the local gravity frame; the bands (and AUTO) never move them.
		return ApplyVehicleTranslation(DeltaTime);
	}
	if (!Ship || !IsBandFlightActive() || !Ship->bEngineRunning || !Ship->SpaceshipHull || DeltaTime <= 0.0f)
	{
		// Engine off or the previous model selected: ASpaceship keeps its own behaviour.
		if (DeltaTime > 0.0f)
		{
			DisengageStarDrive(TEXT("engine off"));
		}
		return false;
	}
	EnsureKinematicHull();
	EnsureBandSettings();
	UpdateNearestSurface(DeltaTime);
	UpdateAutopilot(DeltaTime);

	const bool bBoostHeld = Ship->bIsAccelerating || (bDebugDrive && bDebugBoost);
	BoostAlpha = FMath::FInterpConstantTo(BoostAlpha, bBoostHeld ? 1.0 : 0.0, static_cast<double>(DeltaTime), 2.5);
	// With the mouse on the camera (C) A/D turn the hull (ASpaceship::ApplyRotationInput) instead of strafing.
	const FVector LocalInput = FVector(bDebugDrive ? DebugForwardInput : Ship->ForwardInput,
		Ship->IsMouseLookActive() ? 0.0f : Ship->SideInput, Ship->VerticalInput).GetClampedToMaxSize(1.0);
	if (bStarDrive && ApplyStarDrive(LocalInput, DeltaTime))
	{
		return true;
	}
	UpdateAutoBand(Ship->KinematicVelocity.Size(), LocalInput.X, DeltaTime);

	FAPSFlightBandSettings Band = GetBandSettings(FlightBand);
	if (FlightBand == EAPSFlightBand::Flight && APSShipFlightModelLocal::FeelEnabled()
		&& (IsInAtmosphere() || (GroundClearanceCm >= 0.0 && GroundClearanceCm < APSShipFlightModelLocal::LowFlightCeilingCm)))
	{
		// aps.Ship.Feel (Rio 02.10, "the weight is not felt"): less thrust, a lazier velocity follow (the hull carries its
		// momentum through a turn) and a longer coast after W. Still 90% of a 300 m/s ground limit in about 2 s.
		Band.Acceleration *= 0.6;
		Band.AssistRate *= 0.65;
		Band.ReleaseDrag *= 0.75;
	}
	const double Boost = BandBoost(FlightBand, BoostAlpha);
	// What held W reaches now (boost included), crept up while W stays held at it (Rio 02.10: no ceiling, the ship
	// keeps accelerating slowly). Releasing W or braking lets the creep fall back; a band change starts over.
	const double BaseLimit = BandLimitCm(FlightBand, BoostAlpha);
	if (CreepBand != FlightBand)
	{
		CreepBand = FlightBand;
		LimitCreep = 1.0;
	}
	// The autopilot flies at the plain limit, so it stops where it means to.
	// Hover bands (docking, landing) keep their limit: the creep is for flight, not for a ship easing into a hangar.
	const bool bThrottleHeld = LocalInput.X > 0.5 && !Ship->bIsDecelerating && !IsAutopilotEngaged()
		&& Band.Control != EAPSFlightBandControl::Hover;
	if (bThrottleHeld && Ship->KinematicVelocity.Size() >= BaseLimit * LimitCreep * 0.97)
	{
		LimitCreep += FMath::Max(APSShipFlightModelLocal::CVarLimitCreep.GetValueOnGameThread(), 0.0f) * DeltaTime;
	}
	else if (!bThrottleHeld)
	{
		LimitCreep = 1.0 + (LimitCreep - 1.0) * FMath::Exp(-1.5 * DeltaTime);
	}
	// The autopilot's even arrival (UpdateAutopilot) caps whatever the band allows.
	const double AutopilotCap = IsAutopilotEngaged() ? AutopilotSpeedCapCm : TNumericLimits<double>::Max();
	const double Limit = FMath::Min(BaseLimit * LimitCreep, AutopilotCap);
	CurrentSpeedLimitCm = Limit;
	// In space the Assist and Cruise bands keep the speed a boost built up (Rio, 29.09: letting go of Shift must not
	// slow the ship; S does). Only the full-boost limit, which still shrinks near bodies, and a band drop shed it.
	// Hover and drive, and anything in an atmosphere, settle back to the held limit.
	const bool bKeepsSpeed = !IsInAtmosphere()
		&& (Band.Control == EAPSFlightBandControl::Assist || Band.Control == EAPSFlightBandControl::Cruise);
	// Only as much boost as was actually used: without Shift the ship slows for a body or the edge at the plain limit
	// (30.09: it came in at four times that and braked hard), and a speed back under the limit forgets the boost.
	KeptBoostAlpha = Ship->KinematicVelocity.Size() <= Limit ? BoostAlpha : FMath::Max(KeptBoostAlpha, BoostAlpha);
	const double KeptBoost = bKeepsSpeed ? BandBoost(FlightBand, KeptBoostAlpha) : Boost;
	const double KeptLimit = FMath::Min((bKeepsSpeed ? BandLimitCm(FlightBand, KeptBoostAlpha) : BaseLimit) * LimitCreep,
		AutopilotCap);

	const double Drag = IsInAtmosphere() ? Ship->GetEnvironmentDrag() : 0.0;
	FVector Velocity = StepVelocity(Band, LocalInput, Limit, Boost, Drag, DeltaTime);
	if (Band.MaxLogAcceleration > 0.0)
	{
		// Whatever the limit ahead allows, cruise control multiplies the speed by a bounded factor per second.
		const double PreviousSpeed = Ship->KinematicVelocity.Size();
		const double NextSpeed = Velocity.Size();
		const double CappedSpeed = APSFlightBandModel::CapSpeedGrowth(PreviousSpeed, NextSpeed,
			Band.MaxLogAcceleration * FMath::Sqrt(Boost), Band.Acceleration * 100.0 * Boost
				* APSFlightBandModel::ClassAgility(static_cast<uint8>(Ship->SizeClass)), DeltaTime);
		if (CappedSpeed < NextSpeed && NextSpeed > UE_DOUBLE_SMALL_NUMBER)
		{
			Velocity *= CappedSpeed / NextSpeed;
		}
	}
	if (Ship->bIsDecelerating)
	{
		Velocity = APSFlightBandModel::BrakeStep(Velocity, Band.Acceleration * 100.0 * Boost * BrakeAccelerationScale,
			BrakeRate, DeltaTime);
	}
	const double Speed = Velocity.Size();
	if (Speed > KeptLimit && Speed > UE_DOUBLE_SMALL_NUMBER)
	{
		// Band drop or closing on a body: most of the excess goes within half a second.
		const double Factor = Band.DistanceSpeedFactor
			* FMath::Max(APSShipFlightModelLocal::CVarDistanceFactorScale.GetValueOnGameThread(), 0.0f) * KeptBoost;
		double ShedBase = OverspeedShedRate;
		if (IsInAtmosphere() && APSShipFlightModelLocal::FeelEnabled())
		{
			// aps.Ship.Feel (Rio 02.10, the air felt jerky): after a band drop or a turn away from the ground the excess
			// bleeds off over a few seconds; heading into the ground or a body sheds it at the full rate.
			const double Closing = SolidSurfaceDistanceCm >= 0.0 && !SolidSurfaceOutward.IsNearlyZero()
				? -FVector::DotProduct(Velocity / Speed, SolidSurfaceOutward) : 1.0;
			ShedBase = FMath::Lerp(1.2, OverspeedShedRate, FMath::Clamp(2.0 * Closing, 0.0, 1.0));
		}
		Velocity *= APSFlightBandModel::ShedOverspeed(Speed, KeptLimit,
			APSFlightBandModel::ShedRate(ShedBase, Factor), DeltaTime) / Speed;
	}
	MoveShip(Velocity, Band.bSweepCollision, DeltaTime);

	if (APSShipFlightModelLocal::CVarLog.GetValueOnGameThread() != 0)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastLogSeconds >= 1.0)
		{
			LastLogSeconds = Now;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Flight] %s band=%s env=%s speed=%s limit=%s nearest=%s %.0f m ground=%.0f m star=%.4g AU system=%.4g AU course=%.4g AU boost=%.2f drag=%.2f"),
				*Ship->GetName(), *Band.Name, *Ship->GetFlightEnvironmentName(),
				*APSShipFlightModelLocal::FormatSpeed(Ship->KinematicVelocity.Size()),
				*APSShipFlightModelLocal::FormatSpeed(Limit), *NearestBodyName, NearestSurfaceDistanceCm / 100.0,
				GroundClearanceCm / 100.0, NearestStarDistanceCm / APSShipFlightModelLocal::AstronomicalUnitCm,
				NearestSystemGapCm / APSShipFlightModelLocal::AstronomicalUnitCm,
				CourseClearanceCm / APSShipFlightModelLocal::AstronomicalUnitCm, Boost, Drag);
		}
	}
	return true;
}

void UAPSShipFlightModel::MoveShip(FVector Velocity, const bool bSweepBand, const float DeltaTime)
{
	ASpaceship* Ship = GetShip();
	if (SolidSurfaceDistanceCm >= 0.0 && !SolidSurfaceOutward.IsNearlyZero())
	{
		// Whatever the limits and the frame time, a frame closes at most a quarter of the gap to a planet, moon, star
		// or the ground: a hitch at interstellar speed cannot carry the ship through a body. Sideways motion is free.
		const double Closing = -FVector::DotProduct(Velocity, SolidSurfaceOutward);
		const double Kept = APSFlightBandModel::GuardClosingSpeed(Closing, SolidSurfaceDistanceCm, DeltaTime);
		if (Kept < Closing)
		{
			Velocity += SolidSurfaceOutward * (Closing - Kept);
		}
	}
	Ship->KinematicVelocity = Velocity;

	// Fast bands skip the sweep, but a slow frame in them (near a station) still collides.
	const bool bSweep = bSweepBand || Velocity.Size() * DeltaTime < APSShipFlightModelLocal::ShortMoveSweepCm;
	FHitResult Hit;
	Ship->MoveShipKinematic(Velocity * DeltaTime, bSweep, Hit);
	if (Hit.bBlockingHit)
	{
		// Same contact rule as the power-step model: drop only the velocity into the obstacle.
		const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
		const double VelocityIntoSurface = FVector::DotProduct(Ship->KinematicVelocity, SurfaceNormal);
		if (!SurfaceNormal.IsNearlyZero() && VelocityIntoSurface < 0.0)
		{
			Ship->KinematicVelocity -= SurfaceNormal * VelocityIntoSurface;
		}
		if (APSShipFlightModelLocal::CVarLog.GetValueOnGameThread() != 0)
		{
			const double Now = FPlatformTime::Seconds();
			if (Now - LastContactLogSeconds >= 1.0)
			{
				LastContactLogSeconds = Now;
				UE_LOG(LogTemp, Log, TEXT("[APS.Flight] %s contact %s.%s (%s) normal=%s start-penetrating=%d depth=%.0f cm"),
					*Ship->GetName(), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()),
					Hit.GetComponent() ? *Hit.GetComponent()->GetCollisionProfileName().ToString() : TEXT("?"),
					*SurfaceNormal.ToCompactString(), Hit.bStartPenetrating ? 1 : 0, Hit.PenetrationDepth);
			}
		}
	}
}

void UAPSShipFlightModel::ToggleStarDrive()
{
	if (bStarDrive)
	{
		DisengageStarDrive(TEXT("switched off"));
	}
	else
	{
		EngageStarDrive();
	}
}

bool UAPSShipFlightModel::EngageStarDrive()
{
	ASpaceship* Ship = GetShip();
	if (!Ship || bStarDrive)
	{
		return bStarDrive;
	}
	// Rio 02.10: never in a ground vehicle (rover, hover, drone).
	const TCHAR* Refusal = Ship->IsGroundVehicle() ? TEXT("NOT IN A GROUND VEHICLE")
		: !IsBandFlightActive() ? TEXT("THE BAND FLIGHT MODEL IS OFF")
		: !Ship->bEngineRunning ? TEXT("ENGINE OFF (G)")
		: !IsBandAvailable(EAPSFlightBand::Stellar) ? TEXT("NEEDS AN OFFSET ENGINE (CLASS M AND UP)")
		: IsInAtmosphere() ? TEXT("LEAVE THE ATMOSPHERE FIRST")
		: nullptr;
	if (Refusal)
	{
		StarDriveNotice = FString::Printf(TEXT("STAR DRIVE: %s"), Refusal);
		StarDriveNoticeUntil = FPlatformTime::Seconds() + 4.0;
		UE_LOG(LogTemp, Log, TEXT("[APS.Drive] %s (%s hull) star drive refused: %s"), *GetNameSafe(Ship),
			*Ship->GetSizeClassName(), Refusal);
		return false;
	}
	DisengageAutopilot(TEXT("star drive"));
	bStarDrive = true;
	StarDriveSeconds = 0.0f;
	StarDriveIdleSeconds = 0.0f;
	StarDriveFromCm = Ship->KinematicVelocity.Size();
	StarDriveSetCm = StarDriveCruiseCm();
	StarDriveHeldBy = nullptr;
	// Engaged between the systems, the next one entered is an arrival; engaged in one, it has to be left first.
	bStarDriveLeftSystem = !(bSystemGapKnown && NearestSystemGapCm <= 0.0);
	StarDriveNotice.Reset();
	CatalogueScanElapsed = TNumericLimits<float>::Max();
	UE_LOG(LogTemp, Log, TEXT("[APS.Drive] %s star drive on at %s: cruise %s (star spacing median %.2f AU), %s"),
		*GetNameSafe(Ship), *APSShipFlightModelLocal::FormatSpeed(StarDriveFromCm),
		*APSShipFlightModelLocal::FormatSpeed(StarDriveSetCm),
		CatalogueSpacingMedianCm / APSShipFlightModelLocal::AstronomicalUnitCm,
		bStarDriveLeftSystem ? TEXT("between the systems") : TEXT("inside a star system"));
	return true;
}

void UAPSShipFlightModel::DisengageStarDrive(const TCHAR* Reason)
{
	if (!bStarDrive)
	{
		return;
	}
	bStarDrive = false;
	StarDriveHeldBy = nullptr;
	const ASpaceship* Ship = GetShip();
	UE_LOG(LogTemp, Log, TEXT("[APS.Drive] %s star drive off at %s: %s"), *GetNameSafe(Ship),
		Ship ? *APSShipFlightModelLocal::FormatSpeed(Ship->KinematicVelocity.Size()) : TEXT("?"), Reason);
	StarDriveNotice = FString::Printf(TEXT("STAR DRIVE OFF: %s"), *FString(Reason).ToUpper());
	StarDriveNoticeUntil = FPlatformTime::Seconds() + 4.0;
	LimitCreep = 1.0;
	KeptBoostAlpha = 0.0;
	if (Ship && !bManualBand && !IsInAtmosphere())
	{
		// The bands take over at the speed the drive left: the fastest band here (STELLAR between the systems, CRUISE in
		// one), and AUTO shifts down from there as the ship slows or closes on a body.
		EAPSFlightBand Band = !bSystemGapKnown || NearestSystemGapCm > 0.0 ? EAPSFlightBand::Stellar : EAPSFlightBand::Cruise;
		while (Band > EAPSFlightBand::Orbital && !IsBandAvailable(Band))
		{
			Band = static_cast<EAPSFlightBand>(static_cast<uint8>(Band) - 1);
		}
		ApplyBand(Band, TEXT("star drive off"));
	}
}

double UAPSShipFlightModel::StarDriveCruiseCm() const
{
	// Neighbouring stars pass every CrossSeconds: about 1 AU in 10 s in the generated cluster (spacing median 1.0-1.6 AU).
	constexpr double AU = APSShipFlightModelLocal::AstronomicalUnitCm;
	// Rio 03.10 ("every star must be reachable"): past the cluster's edge the neighbours are the galaxy's stars.
	const double Spacing = CatalogueGapCm > 0.0 && GalaxySpacingCm > 0.0 ? GalaxySpacingCm
		: CatalogueSpacingMedianCm > 0.0 ? CatalogueSpacingMedianCm : AU;
	return FMath::Max(Spacing, 0.1 * AU)
		/ FMath::Max(APSShipFlightModelLocal::CVarDriveCrossSeconds.GetValueOnGameThread(), 0.5f);
}

bool UAPSShipFlightModel::ApplyStarDrive(const FVector& LocalInput, const float DeltaTime)
{
	ASpaceship* Ship = GetShip();
	if (!Ship->IsPlayerControlled())
	{
		DisengageStarDrive(TEXT("no pilot"));
		return false;
	}
	if (IsInAtmosphere())
	{
		DisengageStarDrive(TEXT("atmosphere"));
		return false;
	}
	if (Ship->bIsDecelerating)
	{
		// Ctrl brakes out of the drive: the band takes over this frame, with its brake.
		DisengageStarDrive(TEXT("brake"));
		return false;
	}
	// Arrival: once outside every star system, entering one hands the ship to the bands at about the CRUISE speed there.
	const bool bInsideSystem = bSystemGapKnown && NearestSystemGapCm <= 0.0;
	if (!bInsideSystem)
	{
		bStarDriveLeftSystem = true;
	}
	else if (bStarDriveLeftSystem)
	{
		UAPSCivilizationJournalSubsystem::Post(Ship, TEXT("Flight"), NSLOCTEXT("APSCivilizationJournal", "DriveArrived",
			"The home ship dropped out of the star drive at a star system."));
		DisengageStarDrive(TEXT("arrived at a star system"));
		return false;
	}

	StarDriveSeconds += DeltaTime;
	const double Cruise = StarDriveCruiseCm();
	const double Agility = APSFlightBandModel::ClassAgility(static_cast<uint8>(Ship->SizeClass));
	// W raises the set speed and S lowers it, in log space so a press feels alike at any speed; Shift doubles the rate.
	const double Rate = FMath::Max(APSShipFlightModelLocal::CVarDriveThrottleRate.GetValueOnGameThread(), 0.0f)
		* (Ship->bIsAccelerating ? 2.0 : 1.0);
	const double MinimumSet = 0.02 * Cruise;
	const double MaximumSet = Cruise * FMath::Max(APSShipFlightModelLocal::CVarDriveMaxMultiple.GetValueOnGameThread(), 1.0f);
	StarDriveSetCm = FMath::Clamp(StarDriveSetCm * FMath::Exp(Rate * LocalInput.X * DeltaTime), MinimumSet, MaximumSet);
	// S held at the bottom of the range for a second winds the drive down.
	StarDriveIdleSeconds = LocalInput.X < -0.5 && StarDriveSetCm <= 1.01 * MinimumSet ? StarDriveIdleSeconds + DeltaTime : 0.0f;
	if (StarDriveIdleSeconds >= 1.0f)
	{
		DisengageStarDrive(TEXT("slowed down"));
		return false;
	}

	// What the surroundings allow, as in CRUISE: planets, moons and stations slow the ship by their distance, and a star
	// system on the course (or the edge of charted space) slows it to the CRUISE speed at its edge. Inside a system the
	// drive flies no faster than CRUISE does there.
	const double Factor = GetBandSettings(EAPSFlightBand::Cruise).DistanceSpeedFactor
		* FMath::Max(APSShipFlightModelLocal::CVarDistanceFactorScale.GetValueOnGameThread(), 0.0f);
	double Allowed = TNumericLimits<double>::Max();
	const TCHAR* HeldBy = nullptr;
	if (LimitLocalDistanceCm >= 0.0)
	{
		Allowed = FMath::Max(Factor * LimitLocalDistanceCm, DistanceLimitFloor * 100.0);
		HeldBy = TEXT("BODY NEAR");
	}
	if (CourseClearanceCm >= 0.0)
	{
		const double CourseLimit = FMath::Max(Factor * CourseClearanceCm, BandLimitCm(EAPSFlightBand::Cruise, 0.0));
		if (CourseLimit < Allowed)
		{
			Allowed = CourseLimit;
			HeldBy = bInsideSystem ? TEXT("INSIDE A STAR SYSTEM") : TEXT("STAR SYSTEM AHEAD");
		}
	}
	const double Target = FMath::Max(FMath::Min(StarDriveSetCm, Allowed), 100.0);
	StarDriveHeldBy = Allowed < StarDriveSetCm ? HeldBy : nullptr;
	CurrentSpeedLimitCm = Target;

	const FVector Velocity = Ship->KinematicVelocity;
	const double Speed = Velocity.Size();
	const double Spool = FMath::Max(APSShipFlightModelLocal::CVarDriveSpoolSeconds.GetValueOnGameThread(), 0.1f);
	const bool bSpooling = StarDriveSeconds < Spool;
	double NextSpeed = Target;
	if (bSpooling)
	{
		// The spool: from the speed at engaging to the target along an S-curve in log space, so the stars start to move
		// at once and the ship reaches the cruise speed in two seconds.
		const double Alpha = FMath::SmoothStep(0.0, 1.0, static_cast<double>(StarDriveSeconds) / Spool);
		NextSpeed = FMath::Exp(FMath::Lerp(FMath::Loge(FMath::Max(StarDriveFromCm, 1000.0)), FMath::Loge(Target), Alpha));
	}
	else
	{
		// Momentum: the speed follows the target in log space, slower for heavy hulls; a body or a system ahead is
		// followed at the bands' overspeed rate, so the drive never overshoots it.
		const double FollowRate = StarDriveHeldBy && Target < Speed
			? APSFlightBandModel::ShedRate(OverspeedShedRate, Factor)
			: FMath::Max(APSShipFlightModelLocal::CVarDriveResponse.GetValueOnGameThread(), 0.05f) * Agility;
		const double LogTarget = FMath::Loge(Target);
		NextSpeed = FMath::Exp(LogTarget
			+ (FMath::Loge(FMath::Max(Speed, 1.0)) - LogTarget) * FMath::Exp(-FollowRate * DeltaTime));
	}
	// The ship floats: the course follows the nose lazily, so a turn carries it wide (during the spool it lines up quickly).
	const FVector Forward = Ship->GetShipForwardVector();
	const FVector Heading = Speed > 1.0 ? Velocity / Speed : Forward;
	const double AlignRate = bSpooling ? 4.0
		: FMath::Max(APSShipFlightModelLocal::CVarDriveAlignRate.GetValueOnGameThread(), 0.0f) * FMath::Sqrt(Agility);
	FVector Direction = (Heading + (Forward - Heading) * (1.0 - FMath::Exp(-AlignRate * DeltaTime))).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = Forward;
	}
	MoveShip(Direction * NextSpeed, false, DeltaTime);

	if (APSShipFlightModelLocal::CVarLog.GetValueOnGameThread() != 0)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastLogSeconds >= 1.0)
		{
			LastLogSeconds = Now;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Drive] %s t=%.1f s speed=%s set=%s allowed=%s held=%s system=%.4g AU course=%.4g AU slip=%.1f deg"),
				*Ship->GetName(), StarDriveSeconds, *APSShipFlightModelLocal::FormatSpeed(Ship->KinematicVelocity.Size()),
				*APSShipFlightModelLocal::FormatSpeed(StarDriveSetCm), *APSShipFlightModelLocal::FormatSpeed(Target),
				StarDriveHeldBy ? StarDriveHeldBy : TEXT("-"), NearestSystemGapCm / APSShipFlightModelLocal::AstronomicalUnitCm,
				CourseClearanceCm / APSShipFlightModelLocal::AstronomicalUnitCm,
				FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Direction, Forward), -1.0, 1.0))));
		}
	}
	return true;
}

void UAPSShipFlightModel::GetSteeringFeel(double& OutRateScale, double& OutResponseScale, double& OutDampingScale) const
{
	OutRateScale = OutResponseScale = OutDampingScale = 1.0;
	if (bStarDrive)
	{
		// A yoke (Rio 02.10): gentler turns that build up and settle with a lag, so the ship floats through them.
		OutRateScale = 0.5;
		OutResponseScale = 0.35;
		OutDampingScale = 4.0;
	}
	else if (IsInAtmosphere() && APSShipFlightModelLocal::FeelEnabled())
	{
		// In the air the hull turns with its weight and stops turning soon after the mouse does, instead of tossing on.
		OutRateScale = 0.85;
		OutResponseScale = 0.7;
		OutDampingScale = 12.0;
	}
	else
	{
		// In space too the turn stops soon after the mouse does (it drifted on for seconds).
		OutResponseScale = 0.85;
		OutDampingScale = FMath::Max(APSShipFlightModelLocal::CVarSpaceSteerDamping.GetValueOnGameThread(), 1.0f);
	}
}

FVector UAPSShipFlightModel::SmoothSteeringInput(const FVector& RawPitchYawRoll, const float DeltaTime)
{
	// Mouse deltas arrive unevenly from frame to frame; a short low-pass takes the jerk out of a turn.
	const bool bSpace = !bStarDrive && !(IsInAtmosphere() && APSShipFlightModelLocal::FeelEnabled());
	const double TimeConstant = bStarDrive ? 0.15
		: !bSpace ? 0.07 : FMath::Max(APSShipFlightModelLocal::CVarSpaceSteerSmoothing.GetValueOnGameThread(), 0.0f);
	if (TimeConstant <= 0.0 || DeltaTime <= 0.0f)
	{
		SmoothedSteering = RawPitchYawRoll;
	}
	else
	{
		SmoothedSteering += (RawPitchYawRoll - SmoothedSteering) * (1.0 - FMath::Exp(-DeltaTime / TimeConstant));
	}
	if (!bSpace)
	{
		return SmoothedSteering;
	}
	// In space small moves aim finely: the input as a share of the ship's full steering, raised to a power.
	const ASpaceship* Ship = GetShip();
	const double Limit = Ship ? FMath::Max(static_cast<double>(Ship->SteeringInputLimit), 0.05) : 1.0;
	const double Curve = FMath::Clamp(static_cast<double>(APSShipFlightModelLocal::CVarSpaceSteerCurve.GetValueOnGameThread()), 1.0, 3.0);
	const auto Shape = [Limit, Curve](const double Value)
	{
		const double Share = FMath::Clamp(Value / Limit, -1.0, 1.0);
		return FMath::Sign(Share) * FMath::Pow(FMath::Abs(Share), Curve) * Limit;
	};
	FVector Shaped(Shape(SmoothedSteering.X), Shape(SmoothedSteering.Y), Shape(SmoothedSteering.Z));
	if (Ship && Ship->IsMouseLookActive())
	{
		// Rio 04.10 ("in cruise A and D turn far too sharply"): with the mouse on the camera the keys turn the hull. A key
		// is all or nothing, so it eases in over a few tenths of a second (a tap nudges the nose) to a calmer full rate.
		const double KeyTimeConstant = FMath::Max(APSShipFlightModelLocal::CVarSpaceKeyTurnEase.GetValueOnGameThread(), 0.0f);
		KeySteeringYaw = KeyTimeConstant <= 0.0 || DeltaTime <= 0.0f ? RawPitchYawRoll.Y
			: KeySteeringYaw + (RawPitchYawRoll.Y - KeySteeringYaw) * (1.0 - FMath::Exp(-DeltaTime / KeyTimeConstant));
		Shaped.Y = KeySteeringYaw * FMath::Clamp(APSShipFlightModelLocal::CVarSpaceKeyTurnScale.GetValueOnGameThread(), 0.05f, 1.0f);
	}
	else
	{
		KeySteeringYaw = 0.0;
	}
	return Shaped;
}

FString UAPSShipFlightModel::GetStatusText() const
{
	if (const ASpaceship* Owner = GetShip(); Owner && Owner->IsGroundVehicle())
	{
		return GetVehicleStatusText();
	}
	if (!IsBandFlightActive())
	{
		return FString();
	}
	const ASpaceship* Ship = GetShip();
	const FAPSFlightBandSettings& Band = GetBandSettings(FlightBand);
	const FString Notice = FPlatformTime::Seconds() < StarDriveNoticeUntil && !StarDriveNotice.IsEmpty()
		? TEXT("   |   ") + StarDriveNotice : FString();
	if (bStarDrive)
	{
		const double Speed = Ship ? Ship->KinematicVelocity.Size() : 0.0;
		const float Spool = FMath::Max(APSShipFlightModelLocal::CVarDriveSpoolSeconds.GetValueOnGameThread(), 0.1f);
		FString DriveLine = StarDriveSeconds < Spool
			? FString::Printf(TEXT("STAR DRIVE   SPOOLING %d%%   %s"), FMath::RoundToInt(100.0f * StarDriveSeconds / Spool),
				*APSShipFlightModelLocal::FormatSpeed(Speed))
			: FString::Printf(TEXT("STAR DRIVE   %s   (SET %s)"), *APSShipFlightModelLocal::FormatSpeed(Speed),
				*APSShipFlightModelLocal::FormatSpeed(StarDriveSetCm));
		if (StarDriveHeldBy)
		{
			DriveLine += FString::Printf(TEXT("   HELD BACK: %s"), StarDriveHeldBy);
		}
		const FString Near = NearestSurfaceDistanceCm >= 0.0
			? FString::Printf(TEXT("%s %s"), NearestBodyName.IsEmpty() ? TEXT("SURFACE") : *NearestBodyName,
				*UShipNavigationComponent::FormatDistance(NearestSurfaceDistanceCm))
			: FString(TEXT("OPEN SPACE"));
		// The keys are on the hint line below; this one says what the set speed is measured against.
		return FString::Printf(TEXT("%s%s\n%s   |   CRUISE %s"), *DriveLine, *Notice, *Near,
			*APSShipFlightModelLocal::FormatSpeed(StarDriveCruiseCm()));
	}
	// Two short lines: what the ship does now, and what is near.
	FString Line = FString::Printf(TEXT("%s %s   %s   (up to %s)"), IsAutoBandActive() ? TEXT("AUTO") : TEXT("MANUAL"),
		*Band.Name, *APSShipFlightModelLocal::FormatSpeed(Ship ? Ship->KinematicVelocity.Size() : 0.0),
		*APSShipFlightModelLocal::FormatSpeed(CurrentSpeedLimitCm));
	if (const AActor* Target = AutopilotTarget.Get())
	{
		Line = FString::Printf(TEXT("AUTOPILOT > %s   %s left   |   %s"), *APSShipFlightModelLocal::BodyName(Target),
			*UShipNavigationComponent::FormatDistance(FMath::Max(AutopilotRemainingCm, 0.0)), *Line);
	}
	if (BoostAlpha > 0.05)
	{
		Line += FString::Printf(TEXT("   BOOST x%.1f"), BandBoost(FlightBand, BoostAlpha));
	}
	if (Ship && !Ship->bEngineRunning)
	{
		Line += TEXT("   ENGINE OFF (G)");
	}
	Line += Notice;
	const FString Nearest = NearestSurfaceDistanceCm >= 0.0
		? FString::Printf(TEXT("%s %s"), NearestBodyName.IsEmpty() ? TEXT("SURFACE") : *NearestBodyName,
			*UShipNavigationComponent::FormatDistance(NearestSurfaceDistanceCm))
		: FString(TEXT("OPEN SPACE"));
	return FString::Printf(TEXT("%s\n%s   |   %s"), *Line, *Nearest,
		APSShipFlightModelLocal::ControlLabel(Band.Control, IsInAtmosphere()));
}

FString UAPSShipFlightModel::GetHintText() const
{
	if (const ASpaceship* Owner = GetShip(); Owner && Owner->IsGroundVehicle())
	{
		return GetVehicleHintText();
	}
	if (!IsBandFlightActive())
	{
		return FString();
	}
	const ASpaceship* HintShip = GetShip();
	// Rio 02.10: the autopilot holds until Z or a flight key and the mouse only looks around meanwhile; C switches the
	// mouse between steering and the camera.
	if (IsAutopilotEngaged())
	{
		return FString(TEXT("AUTOPILOT ON   |   Z OFF   1-5 PACE   0 AUTO   |   W/S A/D CTRL TAKE THE HELM   |   MOUSE LOOKS AROUND   |   G ENGINE   F EXIT   |   N M T V Y NAV"));
	}
	const bool bMouseLook = HintShip && HintShip->IsMouseLookActive();
	if (bStarDrive)
	{
		return FString::Printf(TEXT("W FASTER   S SLOWER   SHIFT QUICKER   CTRL BRAKE OUT   |   J DRIVE OFF   |   %s"),
			bMouseLook ? TEXT("MOUSE: CAMERA (C: STEER)") : TEXT("MOUSE STEERS LIKE A YOKE   C MOUSE: CAMERA"));
	}
	// One line: flying needs W/S, Shift and Ctrl; the band keys are an override, 0 hands the choice back.
	FString Drive = IsBandAvailable(EAPSFlightBand::Stellar) ? TEXT("   J STAR DRIVE") : TEXT("");
	// C14: B where the orbital build mode can start (or why it just could not).
	if (const UAPSShipBuildComponent* Build = HintShip ? HintShip->FindComponentByClass<UAPSShipBuildComponent>() : nullptr)
	{
		if (const FString BuildHint = Build->GetHintText(); !BuildHint.IsEmpty())
		{
			Drive += TEXT("   ") + BuildHint;
		}
	}
	Drive += bMouseLook ? TEXT("   A/D TURN   C MOUSE: CAMERA") : TEXT("   C MOUSE: STEER");
	return IsAutoBandActive()
		? FString::Printf(TEXT("W/S THRUST   SHIFT BOOST   CTRL BRAKE   |   1-5 MANUAL MODE   |   Z AUTOPILOT%s   G ENGINE   F EXIT   |   N M T V Y NAV"), *Drive)
		: FString::Printf(TEXT("W/S THRUST   SHIFT BOOST   CTRL BRAKE   |   0 AUTO   1-5 MODE   |   Z AUTOPILOT%s   G ENGINE   F EXIT   |   N M T V Y NAV"), *Drive);
}

namespace APSShipFlightModelLocal
{
	// Until a key is bound everywhere: toggles the star drive of the piloted ship.
	FAutoConsoleCommandWithWorld CmdStarDrive(
		TEXT("aps.Ship.StarDrive"),
		TEXT("Toggles the star drive of the piloted ship (key J): spool, cruise, W/S faster/slower."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (!World)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				const APawn* Pawn = It->IsValid() ? (*It)->GetPawn() : nullptr;
				if (UAPSShipFlightModel* Model = Pawn ? Pawn->FindComponentByClass<UAPSShipFlightModel>() : nullptr)
				{
					Model->ToggleStarDrive();
					return;
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Drive] aps.Ship.StarDrive: no piloted ship"));
		}));
}

// ---------------------------------------------------------------------------------------------------------------------
// Ground vehicles (Rio 02.10: "a system of ground transport: a wheeled one, a hover one, and one that flies like a drone
// or a small helicopter, but only up to the top of the atmosphere, or a little into space"; it should look good and be
// pleasant to drive). ApplyTranslation hands a vehicle to ApplyVehicleTranslation: it drives in the local gravity frame,
// never the bands, sweeps against the world and turns the hull itself (ASpaceship's rotation input stays out). All
// kinematic, like every ship here: Chaos vehicles assume world -Z gravity, and these planets are full-scale spheres.

namespace APSShipFlightModelVehicle
{
	TAutoConsoleVariable<float> CVarMouseSteer(
		TEXT("aps.Vehicle.MouseSteer"), 1.0f,
		TEXT("Ground vehicles: how strongly the mouse steers the rover and the hover and turns the drone (A/D steer fully at any value)."));
	TAutoConsoleVariable<float> CVarSpeedScale(
		TEXT("aps.Vehicle.SpeedScale"), 1.0f,
		TEXT("Ground vehicles: multiplies every top speed and thrust (tuning without a rebuild)."));
	TAutoConsoleVariable<int32> CVarVehicleLog(
		TEXT("aps.Vehicle.Log"), 0,
		TEXT("1 logs the driven vehicle's speed, ground height and source, slope, altitude and steering every second."));

	/** A long frame is split into steps of at most this, MaxSteps of them; a longer hitch slows the vehicle's time. */
	constexpr double MaxStepSeconds = 0.025;
	constexpr int32 MaxSteps = 6;
	constexpr double MaxFrameSeconds = 0.15;
	/** Probes start this far above a tyre centre or a hull corner, along the gravity up. */
	constexpr double ProbeAboveCm = 150.0;
	/** Rover: within this of the ground (plus what the step closes) the tyres hold it; farther up, it is airborne. */
	constexpr double RoverSnapCm = 35.0;
	/** Rover: ground steeper than about 60 degrees does not carry the tyres. */
	constexpr double RoverMinGroundCosine = 0.5;
	/** Hover: its ride height over the ground or a liquid while it runs. Parked hovers and drones rest this high. */
	constexpr double HoverRideCm = 175.0;
	/** Hover (Rio 03.10): its ride rises with speed by up to HoverSpeedRiseCm; Space lifts it HoverLiftCm over the ride
	 * while held (a kick on the press, then held there), and on release it sinks back on a soft field for this long. */
	constexpr double HoverSpeedRiseCm = 50.0;
	constexpr double HoverLiftCm = 350.0;
	constexpr double HoverLiftKick = 650.0;
	constexpr double HoverSoftSeconds = 2.0;
	constexpr double RestHeightCm = 3.0;
	/** Drone: its speed limits grow with the height over the ground beyond this, so its ceiling is minutes away. */
	constexpr double DroneReferenceHeightCm = 150000.0;
	/** Drone: the ceiling over an airless world's sea level; a world with air: its atmosphere's top plus 10%. */
	constexpr double AirlessCeilingCm = 400000.0;
	constexpr double CeilingMargin = 1.1;
	/** WorldScape heights are sampled at most this often and this far apart; between samples the last one stands. */
	constexpr double NoiseSampleSeconds = 0.1;
	constexpr double NoiseSampleDistanceCm = 1000.0;

	/** How a rover or a hover drives, cm and seconds. */
	struct FDriveTuning
	{
		double TopSpeed;
		double BoostTopSpeed;
		double Thrust;
		double BoostThrust;
		double Brake;
		double ReverseSpeed;
		/** Rolling (or air) resistance without throttle: a constant plus a share of the speed, per second. */
		double CoastDecel;
		double CoastDrag;
		/** How fast sideways slip is taken out (1/s), and the most sideways force the tyres or the field give. */
		double Grip;
		double MaxLateralAccel;
		/** Turn rate (deg/s): its limit, what is left at a standstill and how fast it builds (deg/s^2); tightest turn (cm). */
		double MaxYawRate;
		double StandstillYawRate;
		double YawAccel;
		double MinTurnRadius;
	};
	// Rio 02.10: 32-41 m/s with weight (90% of the top speed in about five seconds, a turn builds up and settles); the
	// hover is twice as fast and floats: a long coast and little grip, so it drifts through a turn. Both about 18% faster
	// and quicker than the first cut ("add 15-20%").
	constexpr FDriveTuning RoverTuning{3200.0, 4150.0, 1000.0, 1300.0, 1500.0, 900.0, 60.0, 0.035, 9.0, 1300.0, 75.0,
		22.0, 260.0, 550.0};
	constexpr FDriveTuning HoverTuning{6500.0, 8250.0, 1530.0, 2000.0, 1800.0, 1200.0, 25.0, 0.012, 1.7, 1500.0, 90.0,
		55.0, 300.0, 300.0};
	/** Drone: speed along the nose and up or down near the ground (80 and 25 m/s), and how fast it gets there. */
	constexpr double DroneSpeed = 8000.0;
	constexpr double DroneClimbSpeed = 2500.0;
	constexpr double DroneThrust = 1800.0;
	constexpr double DroneBoost = 1.8;
	constexpr double DroneMaxYawRate = 120.0;
	/** Drone: degrees of yaw and look pitch per unit of mouse input (the ThrustYaw and ThrustPitch axes). */
	constexpr double DroneMouseDegrees = 1.6;

	FVector OnPlane(const FVector& Vector, const FVector& Normal)
	{
		return Vector - Normal * FVector::DotProduct(Vector, Normal);
	}

	/** Moves a speed toward zero by Amount, never through it. */
	double TowardZero(const double Speed, const double Amount)
	{
		return Speed > 0.0 ? FMath::Max(Speed - Amount, 0.0) : FMath::Min(Speed + Amount, 0.0);
	}

	/** The world a vehicle drives on: the planet or moon pulling it, else the world it belongs to. */
	const APlanetaryBody* GroundBody(const ASpaceship& Ship)
	{
		const APlanetaryBody* Body = Cast<APlanetaryBody>(Ship.ActiveGravitySource.Get());
		return Body ? Body : Cast<APlanetaryBody>(Ship.GetGroundVehicleHomeBody());
	}

	/** A body's current WorldScape surface: its root and centre, and the liquid's radius on a sea world (else -1). */
	AWorldScapeRoot* SurfaceRoot(const APlanetaryBody* Body, FVector& OutCenter, double& OutLiquidRadiusCm)
	{
		APlanetarySurfaceGenerator* Surface = Body ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0
			|| !Surface->IsSurfaceProfileCurrent(Body))
		{
			return nullptr;
		}
		OutCenter = Root->GetActorLocation();
		OutLiquidRadiusCm = Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			? Root->PlanetScale + static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity
			: -1.0;
		return Root;
	}

	/** The terrain's radius under a point from the WorldScape height (the canonical surface); -1 when unknown. */
	double TerrainRadius(AWorldScapeRoot* Root, const FVector& Center, const FVector& Location)
	{
		const FVector Direction = (Location - Center).GetSafeNormal();
		if (!Root || Direction.IsNearlyZero())
		{
			return -1.0;
		}
		const double Radius = Root->PlanetScale + Root->GetGroundHeight(Center + Direction * Root->PlanetScale, false);
		return FMath::IsFinite(Radius) ? Radius : -1.0;
	}
}

FVector UAPSShipFlightModel::VehicleDown() const
{
	const ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return FVector::DownVector;
	}
	// Toward the centre of the world under the vehicle: a parked ship's artificial gravity nearby does not count here.
	if (const APlanetaryBody* Body = APSShipFlightModelVehicle::GroundBody(*Ship))
	{
		const FVector Down = (Body->GetActorLocation() - Ship->GetActorLocation()).GetSafeNormal();
		if (!Down.IsNearlyZero())
		{
			return Down;
		}
	}
	return Ship->ActiveGravityDirection.IsNearlyZero() ? -Ship->GetActorUpVector() : Ship->ActiveGravityDirection.GetSafeNormal();
}

double UAPSShipFlightModel::VehicleGravity() const
{
	const ASpaceship* Ship = GetShip();
	return Ship && Ship->ActiveGravityAcceleration > 50.0 ? Ship->ActiveGravityAcceleration : 980.0;
}

void UAPSShipFlightModel::VehicleNotice(const TCHAR* Text)
{
	StarDriveNotice = Text;
	StarDriveNoticeUntil = FPlatformTime::Seconds() + 3.0;
}

void UAPSShipFlightModel::ResetVehicleState()
{
	using namespace APSShipFlightModelVehicle;
	const double LastLog = Vehicle.LastLogSeconds;
	Vehicle = FVehicleState();
	Vehicle.LastLogSeconds = LastLog;
	const ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return;
	}
	const FVector Up = -VehicleDown();
	FVector Heading = OnPlane(Ship->GetShipForwardVector(), Up).GetSafeNormal();
	if (Heading.IsNearlyZero())
	{
		// The nose straight up or down: the hull's underside gives the heading.
		Heading = OnPlane(-Ship->GetShipUpVector(), Up).GetSafeNormal();
	}
	if (Heading.IsNearlyZero())
	{
		Heading = OnPlane(FVector::ForwardVector, Up).GetSafeNormal();
		if (Heading.IsNearlyZero())
		{
			Heading = OnPlane(FVector::RightVector, Up).GetSafeNormal();
		}
	}
	Vehicle.Heading = Heading;
	Vehicle.BodyUp = Ship->GetShipUpVector();
	Vehicle.GroundNormal = Up;
	Vehicle.LastVelocity = Ship->KinematicVelocity;
	Vehicle.bInitialized = true;
}

UAPSShipFlightModel::FVehicleControls UAPSShipFlightModel::ReadVehicleControls(const float DeltaTime) const
{
	FVehicleControls Controls;
	const ASpaceship* Ship = GetShip();
	// Nobody at the controls, or the engine off (G): nothing drives it; it comes to rest by itself.
	if (!Ship || !Ship->bEngineRunning || !IsValid(Ship->Pilot))
	{
		return Controls;
	}
	Controls.bPowered = true;
	Controls.Throttle = FMath::Clamp(static_cast<double>(Ship->ForwardInput), -1.0, 1.0);
	Controls.Strafe = FMath::Clamp(static_cast<double>(Ship->SideInput), -1.0, 1.0);
	// Space up, Alt down (the vertical thrust keys); for the drone Ctrl also takes it down.
	Controls.Vertical = FMath::Clamp(static_cast<double>(Ship->VerticalInput) - (Ship->bIsDecelerating ? 1.0 : 0.0), -1.0, 1.0);
	Controls.MouseYaw = Ship->YawInput;
	Controls.MousePitch = Ship->PitchInput;
	// Mouse steering reads the mouse's speed, not its per-frame step, so it feels the same at any frame rate: 80 units of
	// the yaw axis a second turn the wheel fully at aps.Vehicle.MouseSteer 1. A/D add to it.
	const double MouseScale = FMath::Max(APSShipFlightModelVehicle::CVarMouseSteer.GetValueOnGameThread(), 0.0f);
	const double MouseSteer = DeltaTime > 0.0f ? Ship->YawInput / DeltaTime / 80.0 * MouseScale : 0.0;
	Controls.Steer = FMath::Clamp(static_cast<double>(Ship->SideInput) + MouseSteer, -1.0, 1.0);
	Controls.bBoost = Ship->bIsAccelerating;
	Controls.bBrake = Ship->bIsDecelerating;
	// Rio 02.10: with the mouse on the camera (C) the drone turns on A/D, 90 degrees a second, instead of strafing.
	if (Ship->IsMouseLookActive() && Ship->GetGroundVehicleKind() == EAPSGroundVehicleKind::Drone && DeltaTime > 0.0f)
	{
		Controls.MouseYaw = Ship->SideInput * 90.0 * DeltaTime / APSShipFlightModelVehicle::DroneMouseDegrees;
		Controls.Strafe = 0.0;
	}
	return Controls;
}

bool UAPSShipFlightModel::ProbeVehicleGround(const FVector& Up, const double AboveCm, const double BelowCm,
	FVehicleGround& OutGround, const bool bTrace)
{
	using namespace APSShipFlightModelVehicle;
	OutGround = FVehicleGround();
	ASpaceship* Ship = GetShip();
	UWorld* World = GetWorld();
	if (!Ship || !World || !Ship->SpaceshipHull)
	{
		return false;
	}
	// Probe points, front-left, front-right, back-left, back-right: a rover's tyre centres (each contact lies a radius
	// below its centre), else the corners of the hull's underside.
	const FTransform HullTransform = Ship->SpaceshipHull->GetComponentTransform();
	FVector Points[4];
	double Reach[4];
	const TArray<ASpaceship::FGroundVehicleWheel>& Wheels = Ship->GetGroundVehicleWheels();
	if (Wheels.Num() == 4)
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Points[Index] = HullTransform.TransformPosition(Wheels[Index].LocalCenter);
			Reach[Index] = Wheels[Index].LocalRadius * HullTransform.GetScale3D().GetAbsMax() + BelowCm;
		}
	}
	else
	{
		FVector LocalMin;
		FVector LocalMax;
		if (!Ship->GetPrimaryHullLocalBounds(Ship->SpaceshipHull, LocalMin, LocalMax))
		{
			return false;
		}
		const FVector Center = (LocalMin + LocalMax) * 0.5;
		const FVector Extent = (LocalMax - LocalMin) * 0.5;
		const FVector Forward = Ship->FlightForwardLocalAxis;
		const FVector Right = FVector::CrossProduct(Ship->FlightUpLocalAxis, Forward).GetSafeNormal();
		const double HalfLength = FVector::DotProduct(Extent, Forward.GetAbs()) * 0.42;
		const double HalfWidth = FVector::DotProduct(Extent, Right.GetAbs()) * 0.38;
		const double Signs[4][2] = {{1.0, -1.0}, {1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			FVector Local = Center + Forward * (Signs[Index][0] * HalfLength) + Right * (Signs[Index][1] * HalfWidth);
			Local.Z = LocalMin.Z;
			Points[Index] = HullTransform.TransformPosition(Local);
			Reach[Index] = BelowCm;
		}
	}

	// The world-static channel: terrain, structures and hulls answer it; interaction boxes and gravity spheres do not.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSVehicleGround), false, Ship);
	if (IsValid(Ship->Pilot))
	{
		Params.AddIgnoredActor(Ship->Pilot);
	}
	FVector PointSum = FVector::ZeroVector;
	FVector NormalSum = FVector::ZeroVector;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FHitResult Hit;
		OutGround.Contact[Index] = Points[Index] - Up * Reach[Index];
		if (bTrace && World->LineTraceSingleByChannel(Hit, Points[Index] + Up * AboveCm, OutGround.Contact[Index],
			ECC_WorldStatic, Params))
		{
			OutGround.bHit[Index] = true;
			OutGround.Contact[Index] = Hit.ImpactPoint;
			PointSum += Hit.ImpactPoint;
			NormalSum += Hit.ImpactNormal;
			++OutGround.Hits;
		}
	}
	const FVector Location = Ship->GetActorLocation();
	if (OutGround.Hits > 0)
	{
		FVector Normal = FVector::ZeroVector;
		if (OutGround.Hits == 4)
		{
			// The plane of the four contacts (across its diagonals): the ground under the tyres, not one terrain facet.
			Normal = FVector::CrossProduct(OutGround.Contact[1] - OutGround.Contact[2],
				OutGround.Contact[0] - OutGround.Contact[3]);
		}
		else if (OutGround.Hits == 3)
		{
			FVector Three[3];
			int32 Count = 0;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				if (OutGround.bHit[Index])
				{
					Three[Count++] = OutGround.Contact[Index];
				}
			}
			Normal = FVector::CrossProduct(Three[1] - Three[0], Three[2] - Three[0]);
		}
		Normal = Normal.GetSafeNormal();
		if (FVector::DotProduct(Normal, Up) < 0.0)
		{
			Normal = -Normal;
		}
		if (Normal.IsNearlyZero())
		{
			// One or two contacts: their own normals, half way to the gravity up.
			Normal = (NormalSum.GetSafeNormal() + Up).GetSafeNormal();
		}
		OutGround.Normal = Normal.IsNearlyZero() ? Up : Normal;
		OutGround.Point = PointSum / OutGround.Hits;
		const double NormalUp = FVector::DotProduct(OutGround.Normal, Up);
		// The origin's height over that plane, measured along the gravity up.
		OutGround.Height = NormalUp > 0.05
			? FVector::DotProduct(Location - OutGround.Point, OutGround.Normal) / NormalUp
			: FVector::DotProduct(Location - OutGround.Point, Up);
		OutGround.bValid = true;
		OutGround.bCollision = true;
	}

	FVector Center = FVector::ZeroVector;
	double LiquidRadius = -1.0;
	if (AWorldScapeRoot* Root = SurfaceRoot(GroundBody(*Ship), Center, LiquidRadius))
	{
		const double Radial = FVector::Distance(Location, Center);
		if (LiquidRadius > 0.0)
		{
			OutGround.LiquidHeight = Radial - LiquidRadius;
		}
		if (!OutGround.bValid)
		{
			// No collision here (far from the player, or faster than WorldScape builds it): the canonical height.
			const double Now = World->GetTimeSeconds();
			if (Vehicle.NoiseGroundRadiusCm < 0.0 || Now - Vehicle.NoiseSampleSeconds > NoiseSampleSeconds
				|| FVector::DistSquared(Location, Vehicle.NoiseSampleLocation) > FMath::Square(NoiseSampleDistanceCm))
			{
				Vehicle.NoiseGroundRadiusCm = TerrainRadius(Root, Center, Location);
				Vehicle.NoiseSampleSeconds = Now;
				Vehicle.NoiseSampleLocation = Location;
			}
			if (Vehicle.NoiseGroundRadiusCm > 0.0)
			{
				OutGround.Height = Radial - Vehicle.NoiseGroundRadiusCm;
				OutGround.Normal = Up;
				OutGround.Point = Location - Up * OutGround.Height;
				OutGround.bValid = true;
			}
		}
	}
	return OutGround.bValid;
}

void UAPSShipFlightModel::StepRover(const FVehicleControls& Controls, const FVector& Up, const double Gravity,
	const double DeltaTime)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	const FDriveTuning& Tuning = RoverTuning;
	const double SpeedScale = FMath::Max(CVarSpeedScale.GetValueOnGameThread(), 0.05f);
	FVector Velocity = Ship->KinematicVelocity;
	// The steering wheel follows the controls with a short lag; the turn below follows the wheel with the rover's weight.
	Vehicle.Steer += (Controls.Steer - Vehicle.Steer) * (1.0 - FMath::Exp(-DeltaTime / 0.09));
	const bool bHandbrake = !Controls.bPowered || Controls.bBrake;

	FVehicleGround Ground;
	ProbeVehicleGround(Up, ProbeAboveCm, 400.0, Ground);
	const double NormalUp = FVector::DotProduct(Ground.Normal, Up);
	const double Leaving = FVector::DotProduct(Velocity, Ground.Normal);
	const bool bOnGround = Ground.bValid && NormalUp >= RoverMinGroundCosine && Leaving < 400.0
		&& Ground.Height <= RoverSnapCm + FMath::Max(-Leaving, 0.0) * DeltaTime;
	Vehicle.bCollisionGround = Ground.bCollision;
	Vehicle.GroundHeightCm = Ground.bValid ? Ground.Height : -1.0;
	Vehicle.SlopeDegrees = Ground.bValid ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(NormalUp, -1.0, 1.0))) : 0.0;
	Vehicle.bOnLiquid = Ground.LiquidHeight < 0.0;
	Vehicle.GroundNormal = bOnGround ? Ground.Normal : Up;
	Vehicle.bGrounded = bOnGround;

	// Suspension: each tyre reaches for the ground under it, whatever the plane the body rests on.
	const TArray<ASpaceship::FGroundVehicleWheel>& Wheels = Ship->GetGroundVehicleWheels();
	if (Wheels.Num() == 4)
	{
		const FTransform HullTransform = Ship->SpaceshipHull->GetComponentTransform();
		const FVector HullUp = HullTransform.TransformVectorNoScale(FVector::UpVector);
		const double Scale = HullTransform.GetScale3D().GetAbsMax();
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector Bottom = HullTransform.TransformPosition(Wheels[Index].LocalCenter)
				- HullUp * (Wheels[Index].LocalRadius * Scale);
			const double Drop = Ground.bHit[Index] && bOnGround
				? FMath::Clamp(FVector::DotProduct(Bottom - Ground.Contact[Index], HullUp), -15.0, 18.0) : 18.0;
			Vehicle.WheelDrop[Index] += (Drop - Vehicle.WheelDrop[Index]) * (1.0 - FMath::Exp(-25.0 * DeltaTime));
		}
	}

	FVector Snap = FVector::ZeroVector;
	if (bOnGround)
	{
		const FVector Normal = Ground.Normal;
		// On the ground: held on the plane of its tyres' contacts and moving along it. Down onto it with a short
		// suspension lag (no jitter over the terrain's facets), up out of it at once (never sinking in).
		Snap = -Up * (Ground.Height < 0.0 ? Ground.Height : Ground.Height * (1.0 - FMath::Exp(-30.0 * DeltaTime)));
		Velocity = OnPlane(Velocity, Normal);
		const FVector Forward = OnPlane(Vehicle.Heading, Normal).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Normal, Forward).GetSafeNormal();
		double Along = FVector::DotProduct(Velocity, Forward);
		double Across = FVector::DotProduct(Velocity, Right);
		const double Top = (Controls.bBoost ? Tuning.BoostTopSpeed : Tuning.TopSpeed) * SpeedScale;
		const double Thrust = (Controls.bBoost ? Tuning.BoostThrust : Tuning.Thrust) * SpeedScale;
		const double Throttle = Controls.Throttle;
		if (Throttle > 0.05)
		{
			// W: brakes a backward roll first, then pulls toward the top speed, with less thrust near it.
			Along = Along < -100.0 ? FMath::Min(Along + Tuning.Brake * DeltaTime, 0.0)
				: APSFlightBandModel::TaperedSpeedStep(Along, Top * Throttle, Thrust, DeltaTime);
		}
		else if (Throttle < -0.05)
		{
			// S: brakes, and from a stop reverses gently.
			const double Reverse = Tuning.ReverseSpeed * SpeedScale * -Throttle;
			if (Along > 100.0)
			{
				Along = FMath::Max(Along - Tuning.Brake * -Throttle * DeltaTime, 0.0);
			}
			else if (Along > -Reverse)
			{
				Along = FMath::Max(Along - Thrust * 0.6 * -Throttle * DeltaTime, -Reverse);
			}
		}
		else
		{
			Along = TowardZero(Along, (Tuning.CoastDecel + Tuning.CoastDrag * FMath::Abs(Along)) * DeltaTime);
		}
		if (Along > Top)
		{
			// Past the top speed (boost let go, downhill): the extra fades, like engine braking.
			Along = FMath::Max(Top, Along - 250.0 * DeltaTime);
		}
		if (bHandbrake)
		{
			Along = TowardZero(Along, Tuning.Brake * 0.8 * DeltaTime);
		}
		// The slope pulls along the ground; stopped without throttle, the brakes hold it on anything but steep ground.
		const FVector SlopePull = OnPlane(-Up * Gravity, Normal);
		if (FMath::Abs(Throttle) < 0.05 && FMath::Abs(Along) < 80.0 && Vehicle.SlopeDegrees < 32.0)
		{
			Along = TowardZero(Along, 400.0 * DeltaTime);
		}
		else
		{
			Along += FVector::DotProduct(SlopePull, Forward) * DeltaTime;
			Across += FVector::DotProduct(SlopePull, Right) * DeltaTime;
		}
		if (Along > Top * 1.03)
		{
			// Downhill on a heavy world (02.10 test: 40 m/s against "up to 27" at 3 g): past the top speed the drive
			// holds it with up to its brakes, as a hill descent control would.
			Along = FMath::Max(Top * 1.03, Along - Tuning.Brake * DeltaTime);
		}
		// The tyres take the sideways slip out, up to their grip; the handbrake lets the rover slide.
		const double Grip = bHandbrake && Controls.bPowered ? 1.6 : Tuning.Grip;
		const double Slip = Across * FMath::Exp(-Grip * DeltaTime) - Across;
		Across += FMath::Clamp(Slip, -Tuning.MaxLateralAccel * DeltaTime, Tuning.MaxLateralAccel * DeltaTime);
		Velocity = Forward * Along + Right * Across;

		// Turning: the rate follows the wheel with the rover's weight. The tightest turn and the tyres' grip limit it;
		// at a standstill it still creeps round (all-wheel steering). Reversing turns the other way, like a car.
		const double Speed = FMath::Abs(Along);
		double MaxRate = FMath::Min(Tuning.MaxYawRate, FMath::RadiansToDegrees(Speed / Tuning.MinTurnRadius));
		MaxRate = FMath::Min(MaxRate, FMath::RadiansToDegrees(Tuning.MaxLateralAccel * 1.1 / FMath::Max(Speed, 1.0)));
		MaxRate = FMath::Max(MaxRate, Tuning.StandstillYawRate * (1.0 - FMath::Clamp(Speed / 400.0, 0.0, 1.0)));
		const double TargetRate = Vehicle.Steer * MaxRate * (Along < -50.0 ? -1.0 : 1.0);
		Vehicle.YawRate = FMath::FInterpConstantTo(Vehicle.YawRate, TargetRate, DeltaTime,
			FMath::Abs(TargetRate) > FMath::Abs(Vehicle.YawRate) ? Tuning.YawAccel : Tuning.YawAccel * 1.8);
		Vehicle.Heading = FQuat(Normal, FMath::DegreesToRadians(Vehicle.YawRate * DeltaTime)).RotateVector(Vehicle.Heading);
	}
	else
	{
		// In the air (off a crest at speed, down a drop): gravity pulls and the tyres have nothing to steer with.
		Velocity -= Up * Gravity * DeltaTime;
		Vehicle.YawRate *= FMath::Exp(-2.0 * DeltaTime);
		Vehicle.Heading = FQuat(Up, FMath::DegreesToRadians(Vehicle.YawRate * DeltaTime)).RotateVector(Vehicle.Heading);
		if (Ground.bValid && Ground.Height < 0.0)
		{
			// Under the ground with nothing to stand on (terrain steeper than its tyres take): back out on top of it.
			Snap = -Up * Ground.Height;
			Velocity -= Up * FMath::Min(FVector::DotProduct(Velocity, Up), 0.0);
		}
	}
	if (!Snap.IsNearlyZero())
	{
		// The tyres follow the ground at once; no sweep for this, the plane is the terrain the body rides over.
		Ship->AddActorWorldOffset(Snap, false, nullptr, ETeleportType::None);
	}
	MoveVehicle(Velocity, Velocity * DeltaTime, Up);
	Ship->KinematicVelocity = Velocity;
}

void UAPSShipFlightModel::StepHover(const FVehicleControls& Controls, const FVector& Up, const double Gravity,
	const double DeltaTime)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	const FDriveTuning& Tuning = HoverTuning;
	const double SpeedScale = FMath::Max(CVarSpeedScale.GetValueOnGameThread(), 0.05f);
	FVector Velocity = Ship->KinematicVelocity;
	Vehicle.Steer += (Controls.Steer - Vehicle.Steer) * (1.0 - FMath::Exp(-DeltaTime / 0.12));

	FVehicleGround Ground;
	ProbeVehicleGround(Up, ProbeAboveCm, HoverRideCm + 600.0, Ground);
	// It floats over whichever is higher under it: the ground or a sea (of water, lava, ...).
	bool bSupport = Ground.bValid;
	double Support = Ground.bValid ? Ground.Height : 0.0;
	Vehicle.bOnLiquid = false;
	if (Ground.LiquidHeight < TNumericLimits<double>::Max() && (!bSupport || Ground.LiquidHeight < Support))
	{
		Support = Ground.LiquidHeight;
		bSupport = true;
		Vehicle.bOnLiquid = true;
	}
	const double NormalUp = FVector::DotProduct(Ground.Normal, Up);
	Vehicle.bCollisionGround = Ground.bCollision;
	Vehicle.GroundHeightCm = bSupport ? Support : -1.0;
	Vehicle.SlopeDegrees = Ground.bValid && !Vehicle.bOnLiquid
		? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(NormalUp, -1.0, 1.0))) : 0.0;
	Vehicle.GroundNormal = Ground.bValid && !Vehicle.bOnLiquid ? Ground.Normal : Up;

	// Height: a damped spring to the ride height while it runs, with a slow bob; parked, it settles onto its underside.
	Vehicle.BobSeconds += DeltaTime;
	// Rio 03.10: the ride rises half a metre with speed; Space lifts it while held (a kick on the press, then held at the
	// lift height) and on release it sinks softly back to the ride.
	const bool bLiftKey = Controls.bPowered && Controls.Vertical > 0.5;
	const double SpeedShare = FMath::Clamp(OnPlane(Velocity, Up).Size() / FMath::Max(Tuning.TopSpeed * SpeedScale, 1.0), 0.0, 1.0);
	const double Ride = Controls.bPowered
		? HoverRideCm + HoverSpeedRiseCm * SpeedShare + (bLiftKey ? HoverLiftCm : 0.0) + 4.0 * FMath::Sin(Vehicle.BobSeconds * 2.6)
		: RestHeightCm;
	double Vertical = FVector::DotProduct(Velocity, Up);
	if (bLiftKey && !Vehicle.bHopHeld && Vehicle.bGrounded)
	{
		Vertical = FMath::Max(Vertical, 0.0) + HoverLiftKick;
	}
	if (!bLiftKey && Vehicle.bHopHeld)
	{
		Vehicle.HopSeconds = HoverSoftSeconds;
	}
	Vehicle.bHopHeld = bLiftKey;
	Vehicle.HopSeconds = FMath::Max(Vehicle.HopSeconds - DeltaTime, 0.0);
	FVector Snap = FVector::ZeroVector;
	if (bSupport)
	{
		// Held up by the lift the field is firm; sinking back after it, soft, so it settles without a bounce.
		const double Omega = bLiftKey ? 4.0 : Vehicle.HopSeconds > 0.0 ? 2.0 : 5.5;
		const double Damping = bLiftKey ? 0.9 : Vehicle.HopSeconds > 0.0 ? 1.0 : 0.8;
		const double Spring = FMath::Clamp(Omega * Omega * (Ride - Support) - 2.0 * Damping * Omega * Vertical,
			-1500.0, 2500.0);
		// High over the ground (off a cliff) the field lets go: it sinks at about half the gravity.
		const double Release = FMath::Clamp((Support - Ride - 200.0) / 600.0, 0.0, 1.0);
		Vertical += FMath::Lerp(Spring, -0.55 * Gravity, Release) * DeltaTime;
		if (Support < 0.0)
		{
			// Never under the ground or the sea.
			Snap = -Up * Support;
			Vertical = FMath::Max(Vertical, 0.0);
		}
	}
	else
	{
		Vertical -= 0.55 * Gravity * DeltaTime;
	}
	Vehicle.bGrounded = bSupport && Support <= Ride + 25.0;

	const FVector Forward = OnPlane(Vehicle.Heading, Up).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	double Along = FVector::DotProduct(Velocity, Forward);
	double Across = FVector::DotProduct(Velocity, Right);
	const double Top = (Controls.bBoost ? Tuning.BoostTopSpeed : Tuning.TopSpeed) * SpeedScale;
	const double Thrust = (Controls.bBoost ? Tuning.BoostThrust : Tuning.Thrust) * SpeedScale;
	const double Throttle = Controls.Throttle;
	if (Throttle > 0.05)
	{
		Along = Along < -100.0 ? FMath::Min(Along + Tuning.Brake * DeltaTime, 0.0)
			: APSFlightBandModel::TaperedSpeedStep(Along, Top * Throttle, Thrust, DeltaTime);
	}
	else if (Throttle < -0.05)
	{
		const double Reverse = Tuning.ReverseSpeed * SpeedScale * -Throttle;
		if (Along > 100.0)
		{
			Along = FMath::Max(Along - Tuning.Brake * -Throttle * DeltaTime, 0.0);
		}
		else if (Along > -Reverse)
		{
			Along = FMath::Max(Along - Thrust * 0.6 * -Throttle * DeltaTime, -Reverse);
		}
	}
	else
	{
		// It glides: a long coast without throttle.
		Along = TowardZero(Along, (Tuning.CoastDecel + Tuning.CoastDrag * FMath::Abs(Along)) * DeltaTime);
	}
	if (Along > Top)
	{
		Along = FMath::Max(Top, Along - 300.0 * DeltaTime);
	}
	const bool bAirBrake = Controls.bBrake || !Controls.bPowered;
	if (bAirBrake)
	{
		Along = TowardZero(Along, Tuning.Brake * DeltaTime);
	}
	if (Vehicle.bGrounded && Ground.bValid && !Vehicle.bOnLiquid)
	{
		// A slope under it pushes it gently downhill.
		const FVector SlopePush = OnPlane(OnPlane(-Up * Gravity, Ground.Normal), Up) * 0.25;
		Along += FVector::DotProduct(SlopePush, Forward) * DeltaTime;
		Across += FVector::DotProduct(SlopePush, Right) * DeltaTime;
	}
	// Little grip: the sideways drift dies away slowly, so it slides through a turn (the air brake catches it).
	const double Grip = bAirBrake ? 6.0 : Tuning.Grip;
	const double Slip = Across * FMath::Exp(-Grip * DeltaTime) - Across;
	Across += FMath::Clamp(Slip, -Tuning.MaxLateralAccel * DeltaTime, Tuning.MaxLateralAccel * DeltaTime);
	Velocity = Forward * Along + Right * Across + Up * Vertical;

	// Turning like the rover, but it also turns on the spot (and the same way when reversing: it spins, it does not steer).
	const double Speed = FMath::Abs(Along);
	double MaxRate = FMath::Min(Tuning.MaxYawRate, FMath::RadiansToDegrees(Tuning.MaxLateralAccel * 1.2 / FMath::Max(Speed, 1.0)));
	MaxRate = FMath::Max(MaxRate, Tuning.StandstillYawRate);
	const double TargetRate = Controls.bPowered ? Vehicle.Steer * MaxRate : 0.0;
	Vehicle.YawRate = FMath::FInterpConstantTo(Vehicle.YawRate, TargetRate, DeltaTime,
		FMath::Abs(TargetRate) > FMath::Abs(Vehicle.YawRate) ? Tuning.YawAccel : Tuning.YawAccel * 1.5);
	Vehicle.Heading = FQuat(Up, FMath::DegreesToRadians(Vehicle.YawRate * DeltaTime)).RotateVector(Vehicle.Heading);

	if (!Snap.IsNearlyZero())
	{
		Ship->AddActorWorldOffset(Snap, false, nullptr, ETeleportType::None);
	}
	MoveVehicle(Velocity, Velocity * DeltaTime, Up);
	Ship->KinematicVelocity = Velocity;
}

void UAPSShipFlightModel::StepDrone(const FVehicleControls& Controls, const FVector& Up, const double Gravity,
	const double DeltaTime)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	const double SpeedScale = FMath::Max(CVarSpeedScale.GetValueOnGameThread(), 0.05f);
	FVector Velocity = Ship->KinematicVelocity;

	// Yaw: the heading catches up with the mouse within a tenth of a second, at most DroneMaxYawRate.
	const double YawStep = FMath::Clamp(Vehicle.PendingYaw * (1.0 - FMath::Exp(-DeltaTime / 0.1)),
		-DroneMaxYawRate * DeltaTime, DroneMaxYawRate * DeltaTime);
	Vehicle.PendingYaw -= YawStep;
	Vehicle.YawRate = DeltaTime > 0.0 ? YawStep / DeltaTime : 0.0;
	Vehicle.Heading = FQuat(Up, FMath::DegreesToRadians(YawStep)).RotateVector(Vehicle.Heading);

	FVehicleGround Ground;
	// Collision exists only in the last ~140 m over the ground: higher up the WorldScape height alone does.
	ProbeVehicleGround(Up, ProbeAboveCm, 20000.0, Ground, Vehicle.GroundHeightCm < 25000.0);
	// The floor: the ground, or a sea over it (the drone does not dive into it).
	bool bFloor = Ground.bValid;
	double Floor = Ground.bValid ? Ground.Height : 0.0;
	Vehicle.bOnLiquid = false;
	if (Ground.LiquidHeight < TNumericLimits<double>::Max() && (!bFloor || Ground.LiquidHeight < Floor))
	{
		Floor = Ground.LiquidHeight;
		bFloor = true;
		Vehicle.bOnLiquid = true;
	}
	Vehicle.bCollisionGround = Ground.bCollision;
	Vehicle.GroundHeightCm = bFloor ? Floor : -1.0;
	Vehicle.GroundNormal = Ground.bValid && !Vehicle.bOnLiquid ? Ground.Normal : Up;

	// Altitude over the world's sea level, and the ceiling: the top of its air plus 10% (a few km on an airless world).
	double Altitude = -1.0;
	double Ceiling = AirlessCeilingCm;
	if (const APlanetaryBody* Body = GroundBody(*Ship))
	{
		Altitude = FVector::Distance(Ship->GetActorLocation(), Body->GetActorLocation()) - Body->GetWorldScapeBodyRadiusCm();
		Ceiling = Body->AtmosphereHeight > 0.0 ? Body->AtmosphereHeight * 100000.0 * CeilingMargin : AirlessCeilingCm;
	}
	Vehicle.AltitudeCm = Altitude;
	Vehicle.CeilingCm = Ceiling;

	// Rio 02.10: up to the top of the air. Near the ground 80 m/s; higher up the limits grow with the height over the
	// ground, so the ceiling is a few minutes away (about one with boost and the nose up), not hours.
	const double Height = FMath::Max(bFloor ? Floor : Altitude, 0.0);
	const double AltitudeScale = FMath::Max(1.0, Height / DroneReferenceHeightCm);
	const double Horizontal = DroneSpeed * SpeedScale * AltitudeScale * (Controls.bBoost ? DroneBoost : 1.0);
	const double Climb = DroneClimbSpeed * SpeedScale * AltitudeScale * (Controls.bBoost ? 1.6 : 1.0);
	const double Pitch = FMath::DegreesToRadians(Vehicle.LookPitch);
	const FVector Forward = OnPlane(Vehicle.Heading, Up).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	const FVector Nose = Forward * FMath::Cos(Pitch) + Up * FMath::Sin(Pitch);
	// W/S along the nose (it flies where it looks), A/D sideways, Space/Alt/Ctrl straight up and down.
	FVector Wanted = (Nose * Controls.Throttle + Right * (Controls.Strafe * 0.75)) * Horizontal + Up * (Controls.Vertical * Climb);
	if (!Controls.bPowered)
	{
		// Nobody at the controls (or the engine off): it comes down by itself, quickly from high up, and lands.
		Wanted = -Up * FMath::Clamp(Height * 0.4, 300.0, 6000.0);
	}
	if (Altitude >= 0.0)
	{
		// The ceiling: climbing fades out over its last 3%; above it the drone eases back down.
		const double Rising = FVector::DotProduct(Wanted, Up);
		if (Rising > 0.0)
		{
			const double Room = FMath::Clamp((Ceiling - Altitude) / (0.03 * Ceiling), 0.0, 1.0);
			Wanted -= Up * (Rising * (1.0 - Room));
		}
		if (Altitude > Ceiling)
		{
			Wanted = OnPlane(Wanted, Up) - Up * FMath::Min(Climb, (Altitude - Ceiling) * 0.6 + 200.0);
		}
	}
	if (bFloor)
	{
		// Near the ground it slows its descent and touches down softly.
		const double Sinking = -FVector::DotProduct(Wanted, Up);
		const double MaxSinking = FMath::Max(150.0, Floor * 0.9);
		if (Sinking > MaxSinking)
		{
			Wanted += Up * (Sinking - MaxSinking);
		}
	}
	const double Thrust = DroneThrust * SpeedScale * AltitudeScale * (Controls.bBoost ? 1.4 : 1.0);
	Velocity = APSFlightBandModel::HoverStep(Velocity, Wanted, Thrust, DeltaTime);

	FVector Snap = FVector::ZeroVector;
	Vehicle.bGrounded = false;
	if (bFloor && Floor <= RestHeightCm + 8.0 && FVector::DotProduct(Wanted, Up) <= 1.0)
	{
		// Landed: it rests on its underside; only a climb (Space, or the nose up with W) lifts it off again.
		Vehicle.bGrounded = true;
		Velocity = OnPlane(Velocity, Up) * FMath::Exp(-6.0 * DeltaTime);
		Snap = Up * (RestHeightCm - Floor);
	}
	else if (bFloor && Floor < 0.0)
	{
		Snap = -Up * Floor;
		Velocity -= Up * FMath::Min(FVector::DotProduct(Velocity, Up), 0.0);
	}
	if (!Snap.IsNearlyZero())
	{
		Ship->AddActorWorldOffset(Snap, false, nullptr, ETeleportType::None);
	}
	MoveVehicle(Velocity, Velocity * DeltaTime, Up);
	Ship->KinematicVelocity = Velocity;
}

void UAPSShipFlightModel::MoveVehicle(FVector& Velocity, const FVector& Delta, const FVector& Up)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	if (!Ship || Delta.IsNearlyZero())
	{
		return;
	}
	const auto ContactNormal = [](const FHitResult& Hit)
	{
		return (Hit.Normal.IsNearlyZero() ? FVector(Hit.ImpactNormal) : FVector(Hit.Normal)).GetSafeNormal();
	};
	FHitResult Hit;
	Ship->MoveShipKinematic(Delta, true, Hit);
	if (!Hit.bBlockingHit)
	{
		return;
	}
	FVector Normal = ContactNormal(Hit);
	if (Hit.bStartPenetrating)
	{
		// It started inside something (a snap onto a ledge, a structure raised over a parked spot): out first, then on.
		Ship->AddActorWorldOffset(Normal * (FMath::Max(static_cast<double>(Hit.PenetrationDepth), 0.0) + 1.0), false,
			nullptr, ETeleportType::None);
		FHitResult Retry;
		Ship->MoveShipKinematic(Delta, true, Retry);
		if (!Retry.bBlockingHit)
		{
			return;
		}
		if (Retry.bStartPenetrating)
		{
			// Rio 03.10 ("by the base the hover will not go forward, then it drives fine"): it stands inside something the
			// push could not clear (a rock in its belly, a slab's edge); this move ignores that one component, so it drives
			// out in any direction, still stopped by everything else. Named in the log, a few seconds apart.
			UPrimitiveComponent* Stuck = Retry.GetComponent();
			UWorld* StuckWorld = Ship->GetWorld();
			if (StuckWorld && StuckWorld->GetTimeSeconds() - Vehicle.LastLogSeconds > 3.0)
			{
				Vehicle.LastLogSeconds = StuckWorld->GetTimeSeconds();
				UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicle] %s stuck in %s / %s (%.0f cm deep): drives out of it"),
					*Ship->GetGroundVehicleName(), *GetNameSafe(Retry.GetActor()), *GetNameSafe(Stuck),
					static_cast<double>(Retry.PenetrationDepth));
			}
			if (Stuck && Ship->SpaceshipHull)
			{
				Ship->SpaceshipHull->IgnoreComponentWhenMoving(Stuck, true);
				FHitResult Free;
				Ship->MoveShipKinematic(Delta, true, Free);
				Ship->SpaceshipHull->IgnoreComponentWhenMoving(Stuck, false);
			}
			return;
		}
		Hit = Retry;
		Normal = ContactNormal(Hit);
	}
	const FVector Remaining = Delta * (1.0 - Hit.Time);
	const bool bDrone = Ship->GetGroundVehicleKind() == EAPSGroundVehicleKind::Drone;
	if (!bDrone && FVector::DotProduct(Normal, Up) > 0.65 && FVector::DotProduct(Delta.GetSafeNormal(), Up) > -0.3)
	{
		// A ground-like contact while driving along (a bump, the foot of a ramp): drive on over it; the probes lift the
		// vehicle onto it in the next step.
		Ship->AddActorWorldOffset(OnPlane(Remaining, Up) + Up * 2.0, false, nullptr, ETeleportType::None);
		return;
	}
	// A wall (or the ground under a landing vehicle): the speed into it goes and the rest slides along it. Against a wall
	// a rover or hover loses only its level speed, so a steep bank stops it without throwing it up.
	const auto Against = [bDrone, &Up](const FVector& ContactNormalIn)
	{
		if (bDrone || FVector::DotProduct(ContactNormalIn, Up) >= 0.65)
		{
			return ContactNormalIn;
		}
		const FVector Level = OnPlane(ContactNormalIn, Up).GetSafeNormal();
		return Level.IsNearlyZero() ? ContactNormalIn : Level;
	};
	const FVector Wall = Against(Normal);
	const double Into = FVector::DotProduct(Velocity, Wall);
	if (Into < 0.0)
	{
		Velocity -= Wall * Into;
	}
	const FVector Slide = OnPlane(Remaining, Wall);
	if (Slide.SizeSquared() > 1.0)
	{
		FHitResult SlideHit;
		Ship->MoveShipKinematic(Slide, true, SlideHit);
		if (SlideHit.bBlockingHit && !SlideHit.bStartPenetrating)
		{
			const FVector SlideWall = Against(ContactNormal(SlideHit));
			const double SlideInto = FVector::DotProduct(Velocity, SlideWall);
			if (SlideInto < 0.0)
			{
				Velocity -= SlideWall * SlideInto;
			}
		}
	}
}

void UAPSShipFlightModel::PoseVehicle(const FVector& Up, const double DeltaTime)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	if (!Ship || !Ship->SpaceshipHull || DeltaTime <= 0.0)
	{
		return;
	}
	const EAPSGroundVehicleKind Kind = Ship->GetGroundVehicleKind();
	const FVector Velocity = Ship->KinematicVelocity;
	const FVector Acceleration = (Velocity - Vehicle.LastVelocity) / DeltaTime;
	Vehicle.LastVelocity = Velocity;

	// The body's up: the rover lies on its ground, the hover half follows it (level over a sea), the drone stays level
	// with the gravity and settles onto the ground when it lands.
	FVector TargetUp = Up;
	double UpRate = 5.0;
	switch (Kind)
	{
	case EAPSGroundVehicleKind::Rover:
		TargetUp = Vehicle.GroundNormal;
		UpRate = Vehicle.bGrounded ? 12.0 : 1.5;
		break;
	case EAPSGroundVehicleKind::Hover:
		TargetUp = (Up + Vehicle.GroundNormal).GetSafeNormal();
		UpRate = 4.0;
		break;
	default:
		TargetUp = Vehicle.bGrounded ? Vehicle.GroundNormal : Up;
		break;
	}
	Vehicle.BodyUp = FMath::Lerp(Vehicle.BodyUp, TargetUp, 1.0 - FMath::Exp(-UpRate * DeltaTime)).GetSafeNormal();
	if (Vehicle.BodyUp.IsNearlyZero() || FVector::DotProduct(Vehicle.BodyUp, Up) < 0.2)
	{
		Vehicle.BodyUp = Up;
	}
	const FVector Forward = OnPlane(Vehicle.Heading, Vehicle.BodyUp).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		return;
	}
	const FVector Right = FVector::CrossProduct(Vehicle.BodyUp, Forward).GetSafeNormal();

	// Lean, in degrees (roll lifts the right side, pitch the nose): the rover rolls out of a turn and squats under
	// thrust, the hover banks into its turns, the drone tilts into its motion and toward where it looks.
	const double Lateral = FVector::DotProduct(Acceleration, Right);
	const double Longitudinal = FVector::DotProduct(Acceleration, Forward);
	double TargetRoll = 0.0;
	double TargetPitch = 0.0;
	switch (Kind)
	{
	case EAPSGroundVehicleKind::Rover:
		TargetRoll = Vehicle.bGrounded ? FMath::Clamp(Lateral * 0.0038, -5.0, 5.0) : 0.0;
		TargetPitch = Vehicle.bGrounded ? FMath::Clamp(Longitudinal * 0.0035, -4.0, 4.0) : 0.0;
		break;
	case EAPSGroundVehicleKind::Hover:
		TargetRoll = FMath::Clamp(-Lateral * 0.008, -14.0, 14.0);
		TargetPitch = FMath::Clamp(Longitudinal * 0.002, -4.0, 4.0);
		break;
	default:
		TargetRoll = Vehicle.bGrounded ? 0.0 : FMath::Clamp(-Lateral * 0.01, -20.0, 20.0);
		TargetPitch = Vehicle.bGrounded ? 0.0
			: FMath::Clamp(-Longitudinal * 0.008, -15.0, 15.0) + Vehicle.LookPitch * 0.6;
		break;
	}
	Vehicle.Roll += (TargetRoll - Vehicle.Roll) * (1.0 - FMath::Exp(-6.0 * DeltaTime));
	Vehicle.Pitch += (TargetPitch - Vehicle.Pitch) * (1.0 - FMath::Exp(-6.0 * DeltaTime));
	const FQuat Body = FRotationMatrix::MakeFromXZ(Forward, Vehicle.BodyUp).ToQuat();
	const FQuat Lean = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Vehicle.Roll))
		* FQuat(FVector::RightVector, FMath::DegreesToRadians(-Vehicle.Pitch));
	const FQuat Pose = (Body * Lean).GetNormalized();
	Ship->SetActorRotation(Ship->GetActorRotationForFlightAxes(Pose.GetAxisX(), Pose.GetAxisZ()), ETeleportType::None);

	// The rover's tyres roll with its speed, the front ones steer and each reaches for its ground.
	const TArray<ASpaceship::FGroundVehicleWheel>& Wheels = Ship->GetGroundVehicleWheels();
	if (Wheels.Num() > 0)
	{
		const double Scale = FMath::Max(Ship->SpaceshipHull->GetComponentScale().GetAbsMax(), 0.01);
		const double Radius = FMath::Max(Wheels[0].LocalRadius * Scale, 1.0);
		const double Along = FVector::DotProduct(Velocity, Forward);
		Vehicle.WheelSpin = FMath::Fmod(Vehicle.WheelSpin + FMath::RadiansToDegrees(Along * DeltaTime / Radius), 360.0);
		const double SteerAngle = Vehicle.Steer * 26.0;
		for (int32 Index = 0; Index < Wheels.Num(); ++Index)
		{
			const ASpaceship::FGroundVehicleWheel& Wheel = Wheels[Index];
			USceneComponent* Tire = Wheel.Tire.Get();
			if (!Tire)
			{
				continue;
			}
			const double Drop = Index < 4 ? Vehicle.WheelDrop[Index] : 0.0;
			// Rolling about the axle (the left tyres are turned around, so they roll the other way about theirs).
			const double Yaw = (Wheel.bFront ? SteerAngle : 0.0) + (Wheel.bLeft ? 180.0 : 0.0);
			Tire->SetRelativeLocationAndRotation(Wheel.LocalCenter - FVector(0.0, 0.0, Drop / Scale),
				FRotator(Wheel.bLeft ? Vehicle.WheelSpin : -Vehicle.WheelSpin, Yaw, 0.0));
		}
		// Rio 03.10: the chassis holds the tyres: the hubs ride with them (the front ones steer), the arms turn to reach
		// the hubs, and each damper turns toward its mount on the lower arm.
		if (UPoseableMeshComponent* Suspension = Ship->GetGroundVehicleSuspension())
		{
			using FBone = ASpaceship::FGroundVehicleSuspensionBone;
			const FQuat Steer = FRotator(0.0, SteerAngle, 0.0).Quaternion();
			for (const FBone& Bone : Ship->GetGroundVehicleSuspensionBones())
			{
				const double Drop = Bone.Wheel >= 0 && Bone.Wheel < 4 ? Vehicle.WheelDrop[Bone.Wheel] : 0.0;
				const FVector Travel(0.0, 0.0, -Drop / Scale);
				FTransform BonePose = Bone.Rest;
				if (Bone.Role == FBone::ERole::Hub)
				{
					BonePose.AddToTranslation(Travel);
					if (Bone.bSteers)
					{
						BonePose.SetRotation(Steer * Bone.Rest.GetRotation());
					}
				}
				else if (Bone.Role == FBone::ERole::Arm)
				{
					const FVector Root = Bone.Rest.GetLocation();
					BonePose.SetRotation(FQuat::FindBetweenVectors(Bone.Target - Root, Bone.Target + Travel - Root) * Bone.Rest.GetRotation());
				}
				else
				{
					const FQuat ArmTurn = FQuat::FindBetweenVectors(Bone.ArmEnd - Bone.ArmRoot, Bone.ArmEnd + Travel - Bone.ArmRoot);
					const FVector Mount = Bone.ArmRoot + ArmTurn.RotateVector(Bone.Target - Bone.ArmRoot);
					BonePose.SetRotation(FQuat::FindBetweenVectors(Bone.Target - Bone.Pivot, Mount - Bone.Pivot) * Bone.Rest.GetRotation());
					if (Bone.Role == FBone::ERole::DamperEnd)
					{
						BonePose.SetTranslation(Mount);
					}
				}
				Suspension->SetBoneTransformByName(Bone.Name, BonePose, EBoneSpaces::ComponentSpace);
			}
		}
	}
}

void UAPSShipFlightModel::ParkVehicle()
{
	ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return;
	}
	Ship->KinematicVelocity = FVector::ZeroVector;
	Ship->bEngineRunning = false;
	Ship->ApplyEngineState();
	Ship->SetActorTickEnabled(false);
	// Like the colony: attached to its world, so whatever moves the world moves the parked vehicle with it.
	AActor* Home = Ship->GetGroundVehicleHomeBody();
	if (IsValid(Home) && Ship->GetAttachParentActor() != Home)
	{
		Ship->AttachToActor(Home, FAttachmentTransformRules::KeepWorldTransform);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] %s parked and switched off (%s, %.0f cm over it)"),
		*Ship->GetGroundVehicleName(), Vehicle.bCollisionGround ? TEXT("on collision") : TEXT("on the WorldScape height"),
		Vehicle.GroundHeightCm);
	Vehicle.bInitialized = false;
}

bool UAPSShipFlightModel::ApplyVehicleTranslation(const float DeltaTime)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	if (!Ship || !Ship->SpaceshipHull || DeltaTime <= 0.0f)
	{
		return true;
	}
	EnsureKinematicHull();
	if (bStarDrive)
	{
		DisengageStarDrive(TEXT("ground vehicle"));
	}
	if (AutopilotTarget.IsValid() || bDebugDrive)
	{
		DisengageAutopilot(TEXT("ground vehicle"));
	}
	const FVector Up = -VehicleDown();
	if (!Vehicle.bInitialized)
	{
		ResetVehicleState();
	}
	Vehicle.Heading = OnPlane(Vehicle.Heading, Up).GetSafeNormal();
	if (Vehicle.Heading.IsNearlyZero())
	{
		ResetVehicleState();
	}
	const double Gravity = VehicleGravity();
	const FVehicleControls Controls = ReadVehicleControls(DeltaTime);
	const EAPSGroundVehicleKind Kind = Ship->GetGroundVehicleKind();
	if (Kind == EAPSGroundVehicleKind::Drone)
	{
		// Mouse yaw and look pitch, once a frame: the drone's heading and nose follow the mouse with a little weight.
		const double MouseScale = FMath::Max(CVarMouseSteer.GetValueOnGameThread(), 0.0f);
		if (Controls.bPowered)
		{
			Vehicle.PendingYaw = FMath::Clamp(Vehicle.PendingYaw + Controls.MouseYaw * DroneMouseDegrees * MouseScale, -90.0, 90.0);
			Vehicle.LookPitch = FMath::Clamp(Vehicle.LookPitch - Controls.MousePitch * DroneMouseDegrees * MouseScale, -60.0, 45.0);
		}
		else
		{
			Vehicle.PendingYaw = 0.0;
			Vehicle.LookPitch *= FMath::Exp(-2.0 * DeltaTime);
		}
	}
	const double FrameSeconds = FMath::Min(static_cast<double>(DeltaTime), MaxFrameSeconds);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(FrameSeconds / MaxStepSeconds), 1, MaxSteps);
	const double StepSeconds = FrameSeconds / Steps;
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		switch (Kind)
		{
		case EAPSGroundVehicleKind::Hover:
			StepHover(Controls, Up, Gravity, StepSeconds);
			break;
		case EAPSGroundVehicleKind::Drone:
			StepDrone(Controls, Up, Gravity, StepSeconds);
			break;
		case EAPSGroundVehicleKind::Rover:
		default:
			StepRover(Controls, Up, Gravity, StepSeconds);
			break;
		}
	}
	PoseVehicle(Up, FrameSeconds);

	if (CVarVehicleLog.GetValueOnGameThread() != 0)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - Vehicle.LastLogSeconds >= 1.0)
		{
			Vehicle.LastLogSeconds = Now;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Vehicles] %s speed=%s ground=%.0f cm (%s) slope=%.0f deg grounded=%d liquid=%d alt=%.0f m ceiling=%.0f m steer=%.2f yaw=%.0f deg/s powered=%d"),
				*Ship->GetGroundVehicleName(), *APSShipFlightModelLocal::FormatSpeed(Ship->KinematicVelocity.Size()),
				Vehicle.GroundHeightCm, Vehicle.bCollisionGround ? TEXT("collision") : TEXT("WorldScape height"),
				Vehicle.SlopeDegrees, Vehicle.bGrounded ? 1 : 0, Vehicle.bOnLiquid ? 1 : 0, Vehicle.AltitudeCm / 100.0,
				Vehicle.CeilingCm / 100.0, Vehicle.Steer, Vehicle.YawRate, Controls.bPowered ? 1 : 0);
		}
	}

	// After the pilot leaves it comes to rest by itself (brakes, sets down, lands), then parks: engine and tick off.
	if (!IsValid(Ship->Pilot))
	{
		Vehicle.ParkSeconds += DeltaTime;
		Vehicle.RestSeconds = Vehicle.bGrounded && Ship->KinematicVelocity.SizeSquared() < 400.0
			? Vehicle.RestSeconds + DeltaTime : 0.0;
		// A safety for a vehicle that never settles (sliding on a steep bank); a drone left high up needs its descent.
		const double ParkTimeout = Kind == EAPSGroundVehicleKind::Drone ? 300.0 : 30.0;
		if (Vehicle.RestSeconds >= 0.75 || Vehicle.ParkSeconds >= ParkTimeout)
		{
			ParkVehicle();
		}
	}
	else
	{
		Vehicle.ParkSeconds = 0.0;
		Vehicle.RestSeconds = 0.0;
	}
	return true;
}

bool UAPSShipFlightModel::SettleVehicle(const bool bRequireCollision)
{
	using namespace APSShipFlightModelVehicle;
	ASpaceship* Ship = GetShip();
	if (!Ship || !Ship->IsGroundVehicle() || IsValid(Ship->Pilot) || Ship->IsActorTickEnabled())
	{
		return false;
	}
	const FVector Up = -VehicleDown();
	// From just above first: under the motor pool's carport (its roof 4.8 m up) the deck is the ground, not the roof.
	// Then from well above: a vehicle placed on the WorldScape height may stand a little under (or over) the collision.
	Vehicle.NoiseGroundRadiusCm = -1.0;
	FVehicleGround Ground;
	if (!(ProbeVehicleGround(Up, 150.0, 3000.0, Ground) && Ground.bCollision)
		&& (!ProbeVehicleGround(Up, 1500.0, 3000.0, Ground) || (bRequireCollision && !Ground.bCollision)))
	{
		return false;
	}
	const EAPSGroundVehicleKind Kind = Ship->GetGroundVehicleKind();
	const FVector Normal = FVector::DotProduct(Ground.Normal, Up) >= 0.7 ? Ground.Normal : Up;
	const FVector BodyUp = Kind == EAPSGroundVehicleKind::Rover ? Normal : (Normal + Up).GetSafeNormal();
	FVector Heading = OnPlane(Ship->GetShipForwardVector(), BodyUp).GetSafeNormal();
	if (Heading.IsNearlyZero())
	{
		Heading = OnPlane(-Ship->GetShipUpVector(), BodyUp).GetSafeNormal();
	}
	if (Heading.IsNearlyZero())
	{
		return false;
	}
	const double Rest = Kind == EAPSGroundVehicleKind::Rover ? 0.0 : RestHeightCm;
	Ship->SetActorLocationAndRotation(Ship->GetActorLocation() - Up * (Ground.Height - Rest),
		Ship->GetActorRotationForFlightAxes(Heading, BodyUp), false, nullptr, ETeleportType::TeleportPhysics);
	Ship->KinematicVelocity = FVector::ZeroVector;
	Vehicle.bInitialized = false;
	return Ground.bCollision;
}

bool UAPSShipFlightModel::GetVehicleCameraFrame(FVector& OutForward, FVector& OutUp, double& OutPitchDegrees) const
{
	using namespace APSShipFlightModelVehicle;
	const ASpaceship* Ship = GetShip();
	if (!Ship || !Ship->IsGroundVehicle() || !Vehicle.bInitialized)
	{
		return false;
	}
	OutUp = -VehicleDown();
	OutForward = OnPlane(Vehicle.Heading, OutUp).GetSafeNormal();
	if (OutForward.IsNearlyZero())
	{
		return false;
	}
	if (Ship->GetGroundVehicleKind() == EAPSGroundVehicleKind::Drone)
	{
		OutPitchDegrees = Vehicle.LookPitch * 0.8;
	}
	else
	{
		// Half the pitch of the ground under it: up a hill the camera looks up the hill, without the bumps.
		const FVector BodyForward = OnPlane(Vehicle.Heading, Vehicle.BodyUp).GetSafeNormal();
		OutPitchDegrees = 0.5 * FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(BodyForward, OutUp), -1.0, 1.0)));
	}
	return true;
}

FString UAPSShipFlightModel::GetVehicleStatusText() const
{
	using namespace APSShipFlightModelVehicle;
	const ASpaceship* Ship = GetShip();
	if (!Ship)
	{
		return FString();
	}
	const EAPSGroundVehicleKind Kind = Ship->GetGroundVehicleKind();
	const double SpeedScale = FMath::Max(CVarSpeedScale.GetValueOnGameThread(), 0.05f);
	const double Speed = Ship->KinematicVelocity.Size();
	// Two short lines, like a ship's: what the vehicle does now, and what is under it.
	FString Line = FString::Printf(TEXT("%s   %s"), *Ship->GetGroundVehicleName(), *APSShipFlightModelLocal::FormatSpeed(Speed));
	if (Kind == EAPSGroundVehicleKind::Drone)
	{
		Line += FString::Printf(TEXT("   ALT %s   CEILING %s"),
			*UShipNavigationComponent::FormatDistance(FMath::Max(Vehicle.AltitudeCm, 0.0)),
			*UShipNavigationComponent::FormatDistance(Vehicle.CeilingCm));
	}
	else
	{
		const FDriveTuning& Tuning = Kind == EAPSGroundVehicleKind::Hover ? HoverTuning : RoverTuning;
		Line += FString::Printf(TEXT("   (%.0f km/h, up to %s)"), Speed * 0.036,
			*APSShipFlightModelLocal::FormatSpeed((Ship->bIsAccelerating ? Tuning.BoostTopSpeed : Tuning.TopSpeed) * SpeedScale));
	}
	if (Ship->bIsAccelerating)
	{
		Line += TEXT("   BOOST");
	}
	if (Ship->bIsDecelerating && Kind != EAPSGroundVehicleKind::Drone)
	{
		Line += Kind == EAPSGroundVehicleKind::Rover ? TEXT("   HANDBRAKE") : TEXT("   AIR BRAKE");
	}
	if (!Ship->bEngineRunning)
	{
		Line += TEXT("   ENGINE OFF (G)");
	}
	if (FPlatformTime::Seconds() < StarDriveNoticeUntil && !StarDriveNotice.IsEmpty())
	{
		Line += TEXT("   |   ") + StarDriveNotice;
	}

	FString Under;
	switch (Kind)
	{
	case EAPSGroundVehicleKind::Hover:
		Under = Vehicle.GroundHeightCm >= 0.0
			? FString::Printf(TEXT("HOVERING %.1f m OVER %s"), Vehicle.GroundHeightCm / 100.0,
				Vehicle.bOnLiquid ? TEXT("THE SEA") : TEXT("THE GROUND"))
			: FString(TEXT("HOVERING"));
		if (Vehicle.SlopeDegrees >= 1.0)
		{
			Under += FString::Printf(TEXT("   SLOPE %.0f DEG"), Vehicle.SlopeDegrees);
		}
		break;
	case EAPSGroundVehicleKind::Drone:
		Under = Vehicle.bGrounded ? FString(TEXT("LANDED: SPACE TO LIFT OFF"))
			: Vehicle.GroundHeightCm >= 0.0
			? FString::Printf(TEXT("%s OVER %s"), *UShipNavigationComponent::FormatDistance(Vehicle.GroundHeightCm),
				Vehicle.bOnLiquid ? TEXT("THE SEA") : TEXT("THE GROUND"))
			: FString(TEXT("OVER THE GROUND"));
		if (Vehicle.AltitudeCm > Vehicle.CeilingCm)
		{
			Under += TEXT("   |   ABOVE THE CEILING: DESCENDING");
		}
		else if (Vehicle.AltitudeCm > 0.97 * Vehicle.CeilingCm)
		{
			Under += TEXT("   |   AT THE CEILING");
		}
		break;
	case EAPSGroundVehicleKind::Rover:
	default:
		Under = Vehicle.bGrounded ? FString::Printf(TEXT("ON THE GROUND   SLOPE %.0f DEG"), Vehicle.SlopeDegrees)
			: Vehicle.GroundHeightCm > 0.0
			? FString::Printf(TEXT("AIRBORNE   %s OVER THE GROUND"), *UShipNavigationComponent::FormatDistance(Vehicle.GroundHeightCm))
			: FString(TEXT("AIRBORNE"));
		if (Vehicle.bOnLiquid)
		{
			Under += TEXT("   |   UNDER THE SEA SURFACE");
		}
		break;
	}
	return FString::Printf(TEXT("%s\n%s"), *Line, *Under);
}

FString UAPSShipFlightModel::GetVehicleHintText() const
{
	const ASpaceship* Ship = GetShip();
	// Rio 02.10: C switches the mouse between steering and the camera (a rover or a hover starts on the camera).
	const bool bMouseLook = Ship && Ship->IsMouseLookActive();
	switch (Ship ? Ship->GetGroundVehicleKind() : EAPSGroundVehicleKind::None)
	{
	case EAPSGroundVehicleKind::Drone:
		return bMouseLook
			? TEXT("W/S FORWARD   A/D TURN   SPACE UP   ALT OR CTRL DOWN   SHIFT BOOST   |   MOUSE: CAMERA (C: FLY BY MOUSE)   |   G ENGINE   F EXIT   |   N M NAV")
			: TEXT("W/S FORWARD   A/D STRAFE   SPACE UP   ALT OR CTRL DOWN   MOUSE YAW AND PITCH   SHIFT BOOST   |   C MOUSE: CAMERA   |   G ENGINE   F EXIT   |   N M NAV");
	case EAPSGroundVehicleKind::Hover:
		return bMouseLook
			? TEXT("W/S THRUST   A/D STEER   SPACE LIFT (HOLD)   SHIFT BOOST   CTRL AIR BRAKE   |   MOUSE: CAMERA (C: STEER)   |   G ENGINE   F EXIT   |   N M NAV")
			: TEXT("W/S THRUST   MOUSE OR A/D STEER   SPACE LIFT (HOLD)   SHIFT BOOST   CTRL AIR BRAKE   |   C MOUSE: CAMERA   |   G ENGINE   F EXIT   |   N M NAV");
	case EAPSGroundVehicleKind::Rover:
	default:
		return bMouseLook
			? TEXT("W/S DRIVE   A/D STEER   SHIFT BOOST   CTRL HANDBRAKE   |   MOUSE: CAMERA (C: STEER)   |   G ENGINE   F EXIT   |   N M NAV")
			: TEXT("W/S DRIVE   MOUSE OR A/D STEER   SHIFT BOOST   CTRL HANDBRAKE   |   C MOUSE: CAMERA   |   G ENGINE   F EXIT   |   N M NAV");
	}
}
