#include "APSShipFlightModel.h"

#include "Spaceship.h"
#include "ShipNavigationComponent.h"
#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

	TAutoConsoleVariable<float> CVarSurfaceSpeedFactor(
		TEXT("aps.Ship.SurfaceSpeedFactor"), -1.0f,
		TEXT(">= 0 overrides the ships' SurfaceSpeedFactor (1/s); 0 lifts the low-altitude limit of the Maneuver and ")
		TEXT("Flight bands. -1 keeps each ship's value."));

	TAutoConsoleVariable<int32> CVarLog(
		TEXT("aps.Ship.FlightLog"), 0,
		TEXT("1 logs the piloted ship's band, limit, nearest surface and speed every second."));

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
			// Colliding bounds once per station: many components, and stations do not change shape in flight.
			FVector Origin;
			FVector Extent;
			Station->GetActorBounds(true, Origin, Extent);
			Bounds = &StationBoundsCache.Add(Station, TPair<FVector, double>(
				Station->GetActorTransform().InverseTransformPosition(Origin), Extent.Size()));
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
	const bool bFastBand = FlightBand >= EAPSFlightBand::Cruise;
	const bool bCourseTurned = FlightBand == EAPSFlightBand::Stellar && CatalogueScanElapsed >= 0.05f
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
	}
	// The edge of charted space is a surface too: the star field ends a little beyond the catalogue's bounding sphere
	// (Rio, 30.09: past the edge of the cluster the ship ran off into the void), and beyond ~42 light years from the
	// world origin the renderer loses precision (the DoubleFloat ensure, 29.09). The ship slows for it and stops at it.
	static const FString EdgeName(TEXT("EDGE OF CHARTED SPACE"));
	FVector EdgeCenter = FVector::ZeroVector;
	double EdgeRadiusCm = MaxTravelRadiusLightYears * APSShipFlightModelLocal::LightYearCm;
	if (const AActor* Home = CatalogueHome.Get(); Home && CatalogueRadiusCm > 0.0)
	{
		const FVector ChartedCenter = Home->GetActorLocation() + CatalogueCenterFromHome;
		const double ChartedRadiusCm = CatalogueRadiusCm * APSShipFlightModelLocal::ChartedSpaceScale
			+ APSShipFlightModelLocal::ChartedSpaceMarginCm;
		if (EdgeRadiusCm <= 0.0 || ChartedCenter.Size() + ChartedRadiusCm < EdgeRadiusCm)
		{
			EdgeCenter = ChartedCenter;
			EdgeRadiusCm = ChartedRadiusCm;
		}
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
	Inputs.CourseCm = CourseClearanceCm;
	Inputs.DistanceFloorCm = DistanceLimitFloor * 100.0;
	Inputs.SurfaceFactor = SurfaceFactorOverride >= 0.0f ? SurfaceFactorOverride : SurfaceSpeedFactor;
	Inputs.SurfaceFloorCm = SurfaceSpeedFloor * 100.0;
	const EAPSFlightBand Below = Settings.bCourseLimit ? NeighbourBand(InBand, -1) : InBand;
	return APSFlightBandModel::BandSpeedLimitCm(Settings, BandBoost(InBand, Alpha), Inputs,
		Below != InBand ? BandLimitCm(Below, Alpha) : -1.0);
}

void UAPSShipFlightModel::UpdateAutoBand(double SpeedCm, double Throttle, float DeltaTime)
{
	const ASpaceship* Ship = GetShip();
	if (!Ship || !bAutoBands || bManualBand)
	{
		return;
	}
	// Surroundings: which bands make sense here at all.
	const bool bStation = Cast<ASpaceStation>(Ship->ActiveGravitySource.Get()) != nullptr;
	const bool bInAir = IsInAtmosphere() && !bStation;
	// Outside every star system: a quarter beyond its planets, or InterstellarDistanceAU around a bare star.
	const bool bInterstellar = !bSystemGapKnown || NearestSystemGapCm > 0.0;
	EAPSFlightBand Lowest = bStation || bInAir ? EAPSFlightBand::Flight : EAPSFlightBand::Orbital;
	EAPSFlightBand Highest = bInterstellar ? EAPSFlightBand::Stellar : EAPSFlightBand::Cruise;
	while (Highest > Lowest && !IsBandAvailable(Highest))
	{
		Highest = static_cast<EAPSFlightBand>(static_cast<uint8>(Highest) - 1);
	}
	const bool bHover = Throttle <= 0.1 && SpeedCm < HoverSpeed * 100.0
		&& (bStation || (bInAir && GroundClearanceCm >= 0.0 && GroundClearanceCm < LandingClearance * 100.0));

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
		if (!(bStation || bInAir) || (Throttle > 0.5 && SpeedCm >= 0.85 * BandLimitCm(Wanted, BoostAlpha)))
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
		if (Up != Wanted && Up <= Highest && Throttle > 0.5 && SpeedCm >= 0.85 * Here
			&& BandLimitCm(Up, BoostAlpha) >= 1.25 * Here)
		{
			// Like an automatic gearbox: at the top of this band, and the next one would go faster here.
			Wanted = Up;
			Reason = TEXT("auto: shift up");
		}
		else if (Down != Wanted && Down >= Lowest)
		{
			const bool bNoGain = Here <= 1.05 * BandLimitCm(Down, BoostAlpha);
			const bool bSlowedDown = Throttle <= 0.1 && SpeedCm < 0.4 * GetBandSettings(Down).MaxSpeed * 100.0;
			AutoShiftHold = bNoGain || bSlowedDown ? AutoShiftHold + DeltaTime : 0.0f;
			if (AutoShiftHold >= 0.4f)
			{
				// Closing on a body (this band is no faster than the one below) or coasting slow: shift down.
				Wanted = Down;
				Reason = bNoGain ? TEXT("auto: closing in") : TEXT("auto: slowed down");
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
	if (!Ship || !IsBandFlightActive() || !Ship->bEngineRunning || !Ship->SpaceshipHull || DeltaTime <= 0.0f)
	{
		// Engine off or the previous model selected: ASpaceship keeps its own behaviour.
		return false;
	}
	EnsureKinematicHull();
	EnsureBandSettings();
	UpdateNearestSurface(DeltaTime);

	const bool bBoostHeld = Ship->bIsAccelerating || (bDebugDrive && bDebugBoost);
	BoostAlpha = FMath::FInterpConstantTo(BoostAlpha, bBoostHeld ? 1.0 : 0.0, static_cast<double>(DeltaTime), 2.5);
	const FVector LocalInput = FVector(bDebugDrive ? DebugForwardInput : Ship->ForwardInput, Ship->SideInput,
		Ship->VerticalInput).GetClampedToMaxSize(1.0);
	UpdateAutoBand(Ship->KinematicVelocity.Size(), LocalInput.X, DeltaTime);

	const FAPSFlightBandSettings& Band = GetBandSettings(FlightBand);
	const double Boost = BandBoost(FlightBand, BoostAlpha);
	// What held W reaches now (boost included).
	const double Limit = BandLimitCm(FlightBand, BoostAlpha);
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
	const double KeptLimit = bKeepsSpeed ? BandLimitCm(FlightBand, KeptBoostAlpha) : Limit;

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
		Velocity *= APSFlightBandModel::ShedOverspeed(Speed, KeptLimit,
			APSFlightBandModel::ShedRate(OverspeedShedRate, Factor), DeltaTime) / Speed;
	}
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
	const bool bSweep = Band.bSweepCollision
		|| Velocity.Size() * DeltaTime < APSShipFlightModelLocal::ShortMoveSweepCm;
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

FString UAPSShipFlightModel::GetStatusText() const
{
	if (!IsBandFlightActive())
	{
		return FString();
	}
	const ASpaceship* Ship = GetShip();
	const FAPSFlightBandSettings& Band = GetBandSettings(FlightBand);
	// Two short lines: what the ship does now, and what is near.
	FString Line = FString::Printf(TEXT("%s %s   %s   (up to %s)"), IsAutoBandActive() ? TEXT("AUTO") : TEXT("MANUAL"),
		*Band.Name, *APSShipFlightModelLocal::FormatSpeed(Ship ? Ship->KinematicVelocity.Size() : 0.0),
		*APSShipFlightModelLocal::FormatSpeed(CurrentSpeedLimitCm));
	if (BoostAlpha > 0.05)
	{
		Line += FString::Printf(TEXT("   BOOST x%.1f"), BandBoost(FlightBand, BoostAlpha));
	}
	if (Ship && !Ship->bEngineRunning)
	{
		Line += TEXT("   ENGINE OFF (G)");
	}
	const FString Nearest = NearestSurfaceDistanceCm >= 0.0
		? FString::Printf(TEXT("%s %s"), NearestBodyName.IsEmpty() ? TEXT("SURFACE") : *NearestBodyName,
			*UShipNavigationComponent::FormatDistance(NearestSurfaceDistanceCm))
		: FString(TEXT("OPEN SPACE"));
	return FString::Printf(TEXT("%s\n%s   |   %s"), *Line, *Nearest,
		APSShipFlightModelLocal::ControlLabel(Band.Control, IsInAtmosphere()));
}

FString UAPSShipFlightModel::GetHintText() const
{
	if (!IsBandFlightActive())
	{
		return FString();
	}
	// One line: flying needs W/S, Shift and Ctrl; the band keys are an override, 0 hands the choice back.
	return IsAutoBandActive()
		? FString(TEXT("W/S THRUST   SHIFT BOOST   CTRL BRAKE   |   1-5 MANUAL MODE   |   G ENGINE   F EXIT   |   N M T V NAV"))
		: FString(TEXT("W/S THRUST   SHIFT BOOST   CTRL BRAKE   |   0 AUTO   1-5 MODE   |   G ENGINE   F EXIT   |   N M T V NAV"));
}
