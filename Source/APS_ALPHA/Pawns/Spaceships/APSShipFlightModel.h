#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APSShipFlightModel.generated.h"

class AAstroGenerator;
class ASpaceship;

/** Speed class of the band flight model; keys 1-5 select it directly, Right Shift / Right Ctrl step through it. */
UENUM(BlueprintType)
enum class EAPSFlightBand : uint8
{
	/** Stations, hangars, landing: metres to a few hundred metres. */
	Maneuver UMETA(DisplayName = "Maneuver"),
	/** Flight over the surface and through the atmosphere: up to hundreds of kilometres. */
	Flight UMETA(DisplayName = "Flight"),
	/** Station to station, orbit, moons. */
	Orbital UMETA(DisplayName = "Orbital"),
	/** Between planets of one star. Needs a SpaceWrap-capable ship. */
	Cruise UMETA(DisplayName = "Cruise"),
	/** Between stars. Needs an Offset-capable ship. */
	Stellar UMETA(DisplayName = "Stellar")
};

/** How the held controls move the ship in a band. */
UENUM(BlueprintType)
enum class EAPSFlightBandControl : uint8
{
	/** Held keys set the velocity (all axes); releasing them stops the ship. Docking and landing. */
	Hover UMETA(DisplayName = "Hover"),
	/** W speeds up along the nose with less thrust near the limit; releasing W slows the ship down, like a car. */
	Drive UMETA(DisplayName = "Drive"),
	/** Like Drive, but releasing W keeps the speed: the engines only cancel drift so the ship flies where it points. */
	Assist UMETA(DisplayName = "Assist"),
	/** Cruise control: W raises the speed toward the (distance) limit, it holds without input, S lowers it. */
	Cruise UMETA(DisplayName = "Cruise")
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSFlightBandSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band")
	EAPSFlightBandControl Control{EAPSFlightBandControl::Hover};

	/** Speed limit without boost, m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.1"))
	double MaxSpeed{100.0};

	/** Thrust from rest, m/s^2 (scaled by the hull class). Drive/Assist thrust tapers off toward the limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.01"))
	double Acceleration{20.0};

	/** Held Left Shift multiplies the speed limit, the thrust and the distance factor by this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "1.0"))
	double BoostMultiplier{3.0};

	/** Cruise: how fast the speed approaches the limit while W is held, 1/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.05"))
	double CruiseResponse{0.8};

	/** Drive/Assist/Cruise: how fast the velocity turns toward the nose, 1/s (the drift the engines cancel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0"))
	double AssistRate{2.0};

	/** Drive: speed lost per second without W, as a share of the speed (atmospheric drag adds to it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0"))
	double ReleaseDrag{0.6};

	/** Strafe and vertical thrust reach this share of the band limit (Hover uses the full limit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double LateralFraction{0.25};

	/**
	 * Speed limit = factor x distance to the nearest surface (1/s), never below the ship's distance floor. The ship
	 * slows down on its own when closing on a planet, moon, star or station and cannot overshoot it. 0 disables.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0"))
	double DistanceSpeedFactor{0.0};

	/**
	 * Limit = factor x distance to the nearest star (1/s): the scale of a star system. 0.12 flies 60 c one AU from the
	 * sun and 10 000 c at the edge of the system, so the planets drift past while the stars stay put. 0 disables.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0"))
	double StarDistanceSpeedFactor{0.0};

	/**
	 * Cruise control: the speed grows by at most e^(this x sqrt(boost)) per second, 1/s; 0 = no cap. Leaving a planet or
	 * a star can no longer multiply the speed tenfold within a second (Rio, 30.09: the ship shot out of the system).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band", meta = (ClampMin = "0.0"))
	double MaxLogAcceleration{0.0};

	/**
	 * Stars limit the speed only along the course: the distance limit sees the system sphere (InterstellarDistanceAU) and
	 * the edge of charted space the ship is heading into, not the nearest star in any direction, so passing a star does
	 * not pump the speed. Inside a system the band is never slower than the band below it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band")
	bool bCourseLimit{false};

	/** Sweep the hull against the world while moving. Distance-limited bands rely on the distance limit instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Band")
	bool bSweepCollision{true};
};

/** Stateless steps of the band flight model, in centimetres and seconds. Shared with the automation tests. */
namespace APSFlightBandModel
{
	/** Limit for a band at boost multiplier Boost. Distances below zero are unknown and do not limit. */
	APS_ALPHA_API double SpeedLimitCm(const FAPSFlightBandSettings& Band, double Boost, double NearestSurfaceCm,
		double GroundClearanceCm, double DistanceFloorCm, double SurfaceFactor, double SurfaceFloorCm);

	/** What a band's limit depends on at one place, cm; below zero is unknown and does not limit. */
	struct FSpeedLimitInputs
	{
		/** Nearest surface of any body, star or the edge of charted space, after the departure factor. */
		double SurfaceCm{-1.0};
		/** The same without stars and the edge: the distance limit of a course-limited band. */
		double LocalSurfaceCm{-1.0};
		double GroundClearanceCm{-1.0};
		/** Nearest star surface, for the system scale (StarDistanceSpeedFactor). */
		double StarCm{-1.0};
		/** How far the course runs into a star system or out of charted space (CourseClearanceCm, CourseExitCm). */
		double CourseCm{-1.0};
		double DistanceFloorCm{0.0};
		double SurfaceFactor{0.0};
		double SurfaceFloorCm{0.0};
	};

	/**
	 * Full limit of a band at boost multiplier Boost: distance, system scale and course limits. BelowLimitCm (the band
	 * below's limit here, <0 none) is the floor of a course-limited band, so it never flies slower than that band.
	 */
	APS_ALPHA_API double BandSpeedLimitCm(const FAPSFlightBandSettings& Band, double Boost, const FSpeedLimitInputs& Inputs,
		double BelowLimitCm);

	/** An over-limit speed approaches the limit in log space: large excess drops fast, no hard cut. */
	APS_ALPHA_API double ShedOverspeed(double Speed, double Limit, double Rate, double DeltaTime);

	/**
	 * Shed rate for a distance limit of Factor (1/s, boost included). Closing on a body at Factor x distance shrinks
	 * the limit at that rate too; a log-space shed keeps up only above e x Factor, so it never drops below 4 x Factor.
	 */
	APS_ALPHA_API double ShedRate(double BaseRate, double DistanceFactor);

	/** The closing speed a frame may keep: never more than a quarter of the gap to a solid surface per frame. */
	APS_ALPHA_API double GuardClosingSpeed(double ClosingSpeed, double DistanceCm, double DeltaTime);

	/** A speed above PreviousSpeed grows by at most e^(MaxLogRate x DeltaTime), or MinAcceleration x DeltaTime from a low speed. */
	APS_ALPHA_API double CapSpeedGrowth(double PreviousSpeed, double Speed, double MaxLogRate, double MinAcceleration,
		double DeltaTime);

	/**
	 * How far a course along Direction runs before it enters a sphere (ToCenter = centre - ship): 0 inside it; the distance
	 * to the entry when the course hits it; past a miss that distance plus MissScale x the miss, so the limit changes
	 * smoothly while the nose sweeps across a star. Negative when the sphere is behind.
	 */
	APS_ALPHA_API double CourseClearanceCm(const FVector& ToCenter, const FVector& Direction, double RadiusCm,
		double MissScale);

	/** Distance along Direction to the boundary of a sphere the ship is inside of (FromCenter = ship - centre); 0 outside. */
	APS_ALPHA_API double CourseExitCm(const FVector& FromCenter, const FVector& Direction, double RadiusCm);

	/** Hover: the velocity moves toward Target by at most Acceleration x DeltaTime. */
	APS_ALPHA_API FVector HoverStep(const FVector& Velocity, const FVector& Target, double Acceleration, double DeltaTime);

	/**
	 * Speed toward Target with thrust Acceleration x (1 - (Speed / Target)^2): full thrust from rest, fading only near
	 * Target, like a car reaching its top speed (90% of Target in 1.47 x Target / Acceleration). A speed above Target
	 * is kept (overspeed is shed separately); a backward speed gets the full thrust.
	 */
	APS_ALPHA_API double TaperedSpeedStep(double Speed, double Target, double Acceleration, double DeltaTime);

	/** Turns the velocity toward +Forward (or -Forward when moving backward) at Rate, 1/s, keeping its magnitude. */
	APS_ALPHA_API FVector AlignToNose(const FVector& Velocity, const FVector& Forward, double Rate, double DeltaTime);

	/** Cruise speed: W approaches the limit, S bleeds speed, no input holds it (or drags in atmosphere). */
	APS_ALPHA_API double CruiseSpeedStep(double Speed, double ForwardInput, double Limit, double Response,
		double BrakeRate, double Drag, double DeltaTime);

	/** Left Ctrl: speed falls by max(BrakeAcceleration, Speed x BrakeRate) per second. */
	APS_ALPHA_API FVector BrakeStep(const FVector& Velocity, double BrakeAcceleration, double BrakeRate, double DeltaTime);

	/** Thrust scale of a hull class (ESpaceshipSizeClass as uint8): small hulls are nimble, titans ponderous. */
	APS_ALPHA_API double ClassAgility(uint8 SizeClass);
}

/**
 * Band flight model of every ASpaceship (Rio, 2026-09-29), replacing the power steps x engine modes while
 * aps.Ship.FlightModel is 1. Keys 1-5 pick Maneuver, Flight, Orbital, Cruise (SpaceWrap ships) or Stellar (Offset
 * ships); Right Shift / Right Ctrl step through them. Left Shift boosts, Left Ctrl brakes.
 *
 * Each band covers one range of distances with a speed limit, thrust and a control style (Hover, Drive, Assist,
 * Cruise). In space the engines cancel drift, so the ship flies where it points; in an atmosphere drag slows it
 * when W is released. Near planets, moons, stars and stations the limit shrinks with the distance to the surface, so
 * the ship slows down on approach by itself and never overshoots, and low flight keeps WorldScape streaming in reach.
 */
UCLASS(ClassGroup = (APS), meta = (BlueprintSpawnableComponent))
class APS_ALPHA_API UAPSShipFlightModel : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPSShipFlightModel();

	/** True while the band model moves the owning ship (aps.Ship.FlightModel 1, or bAlwaysUseBands). */
	bool IsBandFlightActive() const;

	/** Moves the ship for one frame; false when the band model is off or the engines are down. */
	bool ApplyTranslation(float DeltaTime);

	/** Picks a band by hand (keys 1-5, Right Shift/Ctrl, console); AUTO stays off until SetAutoBands. */
	UFUNCTION(BlueprintCallable, Category = "Ship|Flight Bands")
	void SetFlightBand(EAPSFlightBand NewBand);

	/** Next (+1) or previous (-1) band the hull supports, by hand. */
	void StepFlightBand(int32 Direction);

	/** Key 0: the ship picks the band from its surroundings and speed again (the default). */
	UFUNCTION(BlueprintCallable, Category = "Ship|Flight Bands")
	void SetAutoBands();

	UFUNCTION(BlueprintPure, Category = "Ship|Flight Bands")
	bool IsAutoBandActive() const { return bAutoBands && !bManualBand; }

	UFUNCTION(BlueprintPure, Category = "Ship|Flight Bands")
	EAPSFlightBand GetFlightBand() const { return FlightBand; }

	/** Cruise needs a SpaceWrap-capable hull, Stellar an Offset-capable one (see the class presets). */
	bool IsBandAvailable(EAPSFlightBand Band) const;

	const FAPSFlightBandSettings& GetBandSettings(EAPSFlightBand Band) const;
	static FAPSFlightBandSettings MakeDefaultBand(EAPSFlightBand Band);

	/** Current limit with boost, surface and distance limits, m/s. */
	UFUNCTION(BlueprintPure, Category = "Ship|Flight Bands")
	double GetCurrentSpeedLimit() const { return CurrentSpeedLimitCm / 100.0; }

	/** Distance to the nearest known surface, m; negative when nothing is known. */
	UFUNCTION(BlueprintPure, Category = "Ship|Flight Bands")
	double GetNearestSurfaceDistance() const { return NearestSurfaceDistanceCm / 100.0; }

	FString GetStatusText() const;
	FString GetHintText() const;

	/** Called by the ship when a pilot takes it: kinematic hull, fresh body and ground data. */
	void OnPossessed();

	/** Test drive without a keyboard (aps.ExpShip.Drive): holds forward input and boost until cleared. */
	void SetDebugDrive(bool bEnabled, float Forward, bool bBoost);

	/** Autopilot of the piloted ship (Rio 02.10): turns to the target, flies the bands, brakes and stops near it.
	 * Any helm input (thrust, strafe, brake, steering) takes the ship back. */
	void EngageAutopilot(AActor* Target);
	void DisengageAutopilot(const TCHAR* Reason);
	bool IsAutopilotEngaged() const { return AutopilotTarget.IsValid(); }
	/** Rio 03.10: the world shifted under the player (the floating origin): cached world centres move with it. */
	void ApplyWorldShift(const FVector& Offset);
	AActor* GetAutopilotTarget() const { return AutopilotTarget.Get(); }

	/**
	 * Star drive (Rio 02.10, key J): a separate mode for the flight between stars. It spools up for two seconds, then
	 * the ship cruises by itself at a speed that crosses the gap between neighbouring stars in about ten seconds; W
	 * raises and S lowers that speed smoothly, Ctrl brakes out of it. The course follows the nose with a lag, so the ship
	 * floats through a turn. Planets, moons and the systems on the course still slow it down, and entering another star
	 * system drops it at the CRUISE speed there. Needs an Offset-capable hull (the STELLAR band), outside an atmosphere.
	 */
	void ToggleStarDrive();
	bool EngageStarDrive();
	void DisengageStarDrive(const TCHAR* Reason);
	bool IsStarDriveActive() const { return bStarDrive; }

	/**
	 * Steering for ASpaceship::ApplyRotationInput (Rio 02.10: in the air the hull tossed about and jerked; the star drive
	 * steers like a yoke with a lag): scales of the hull's turn rate, angular response and passive damping (1 = as is).
	 */
	void GetSteeringFeel(double& OutRateScale, double& OutResponseScale, double& OutDampingScale) const;
	/** The raw pitch/yaw/roll input smoothed over a few frames where the feel asks for it (raw elsewhere). */
	FVector SmoothSteeringInput(const FVector& RawPitchYawRoll, float DeltaTime);

	/** One entry per EAPSFlightBand, in enum order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, Category = "Ship|Flight Bands")
	TArray<FAPSFlightBandSettings> Bands;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands")
	EAPSFlightBand InitialBand{EAPSFlightBand::Maneuver};

	/** Use the band model whatever aps.Ship.FlightModel says (the experimental ship). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands")
	bool bAlwaysUseBands{false};

	/**
	 * AUTO (Rio, 29.09): the ship shifts bands itself like an automatic gearbox. Held W shifts up once the next band
	 * would go faster here; closing on a body, entering a gravity well or an atmosphere, or slowing down shifts down
	 * (and sheds the speed). Stations and landing get the hover band when slow. Keys 1-5 override, key 0 returns.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands")
	bool bAutoBands{true};

	/**
	 * Radius of a star system without planets, AU; a star with planets reaches a quarter beyond its outermost orbit.
	 * Outside every system AUTO may shift into STELLAR (with an Offset engine), and STELLAR slows down for the system its
	 * course runs into, arriving at the CRUISE speed there. The generated cluster packs its stars about 1 AU apart
	 * (30.09), so a bare star is a small target.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.001"))
	double InterstellarDistanceAU{0.05};

	/** Course limit: a course passing a system sphere counts as this many times the miss further away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double CourseMissScale{1000.0};

	/** Seconds between scans of the star catalogue (nearest stars and the course) in CRUISE and STELLAR; 1 s below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.02"))
	float CatalogueScanInterval{0.1f};

	/** AUTO: slower than this near a station or this close to the ground, without W, the ship hovers (MANEUVER). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double HoverSpeed{30.0};

	/** AUTO: ground clearance for the landing hover, m. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double LandingClearance{40.0};

	/**
	 * Edge of charted space, light years from the world origin: the renderer loses precision beyond ~42 ly
	 * (DoubleFloat ensure in Rio's 455 c flight, 29.09), so the ship slows down for this sphere like for a body.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.0"))
	double MaxTravelRadiusLightYears{35.0};

	/** Distance-limited bands never go below this limit, m/s, so take-off and landing work in any band. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double DistanceLimitFloor{500.0};

	/**
	 * Flying straight away from a body multiplies the distance (and ground) limit by this, fading to 1 for a sideways
	 * or approaching course: leaving a planet or star cannot overshoot it, so it may speed up faster than approaching.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double DepartureSpeedFactor{2.0};

	/** Bands without a distance factor: limit = factor x ground clearance (1/s) near a planet. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.0"))
	double SurfaceSpeedFactor{2.0};

	/** The surface limit never goes below this, m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "1.0"))
	double SurfaceSpeedFloor{250.0};

	/** How fast an over-limit ship sheds speed after a band drop or when closing on a body (log space), 1/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.1"))
	double OverspeedShedRate{5.0};

	/** Left Ctrl: the band thrust times this, m/s^2 ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.0"))
	double BrakeAccelerationScale{2.0};

	/** ... or this share of the current speed per second, whichever is larger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.0"))
	double BrakeRate{1.5};

	/** Seconds between refreshes of the planets, moons, stars and stations used for the distance limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.1"))
	float BodyRefreshInterval{1.0f};

	/** Seconds between ground-clearance traces along gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "0.02"))
	float GroundProbeInterval{0.25f};

	/** Ground-clearance trace length, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Flight Bands", meta = (ClampMin = "100.0"))
	double GroundProbeLength{2000000.0};

protected:
	virtual void BeginPlay() override;

private:
	struct FFlightBody
	{
		TWeakObjectPtr<AActor> Actor;
		/** Actor-local centre of the collision bounds (stations) or the fixed world centre (generated stars). */
		FVector Center{FVector::ZeroVector};
		double RadiusCm{0.0};
		FString Name;
		bool bFixedLocation{false};
		/** Planets, moons and stars; a station's bounds are a sphere around open structure the ship may enter. */
		bool bSolid{true};
		bool bStar{false};
		/** Stars: the system sphere STELLAR and AUTO respect (InterstellarDistanceAU, or beyond the planets). */
		double SystemRadiusCm{0.0};
	};

	ASpaceship* GetShip() const;
	void EnsureBandSettings();
	void EnsureKinematicHull();
	bool IsInAtmosphere() const;
	void RefreshFlightBodies();
	/** Rebuilds CatalogueFromHome when the stellar view's catalogue changes; false while it is not ready. */
	bool RebuildStarCatalogue();
	/** Picks the nearest catalogue stars and the one whose system the course runs into. */
	void ScanStarCatalogue(const FVector& Location, const FVector& Heading);
	/** The same for the galaxy's drawn stars (APSGalaxyGpuStars' nearest-star index), into NearestGalaxyStars. */
	void ScanGalaxyStars(const FVector& Location, const FVector& Heading);
	void UpdateGroundProbe(float DeltaTime);
	/** Height above the WorldScape terrain (or sea level) of the nearest planet with a current surface; -1 if none. */
	double MeasureTerrainClearanceCm(const FVector& Location) const;
	void UpdateNearestSurface(float DeltaTime);
	/** A band's limit here at boost ramp Alpha (0 none, 1 full): distance, course, ground and floors included, cm/s. */
	double BandLimitCm(EAPSFlightBand Band, double Alpha) const;
	/** A band's boost multiplier at boost ramp Alpha. */
	double BandBoost(EAPSFlightBand Band, double Alpha) const;
	void UpdateAutoBand(double SpeedCm, double Throttle, float DeltaTime);
	void ApplyBand(EAPSFlightBand NewBand, const TCHAR* Reason);
	EAPSFlightBand NeighbourBand(EAPSFlightBand Band, int32 Direction) const;
	FVector StepVelocity(const FAPSFlightBandSettings& Band, const FVector& LocalInput, double Limit, double Boost,
		double Drag, float DeltaTime) const;

	EAPSFlightBand FlightBand{EAPSFlightBand::Maneuver};
	bool bManualBand{false};
	float AutoShiftHold{0.0f};
	/** After AUTO shifted down closing in on a body, no shift up for a while (Rio 04.10: ORBITAL <-> CRUISE every 3-5 s). */
	float UpShiftBlockSeconds{0.0f};
	/** Nearest star (actor or catalogue point), cm to its surface; -1 unknown. */
	double NearestStarDistanceCm{-1.0};
	/** Smallest distance outside a star system's sphere, cm (negative inside a system); unknown without stars. */
	double NearestSystemGapCm{0.0};
	bool bSystemGapKnown{false};
	double BoostAlpha{0.0};
	/** The boost the kept speed was built with: letting go of Shift keeps that much, and no more than was used. */
	double KeptBoostAlpha{0.0};
	double CurrentSpeedLimitCm{0.0};
	double NearestSurfaceDistanceCm{-1.0};
	/** Smallest body distance after the departure factor: what the distance limit sees. */
	double LimitSurfaceDistanceCm{-1.0};
	/** The same without stars and the edge of charted space: what a course-limited band's distance limit sees. */
	double LimitLocalDistanceCm{-1.0};
	/** How far the course runs into a star system or out of charted space (CourseClearanceCm); -1 unknown. */
	double CourseClearanceCm{-1.0};
	FString NearestBodyName;
	double GroundClearanceCm{-1.0};
	/** Ground clearance after the departure factor: what the surface limit sees. */
	double GroundLimitClearanceCm{-1.0};
	/** Nearest planet, moon, star or ground, and the direction away from it, for the closing guard. */
	double SolidSurfaceDistanceCm{-1.0};
	FVector SolidSurfaceOutward{FVector::ZeroVector};
	double ProbedGroundClearanceCm{-1.0};
	float GroundProbeElapsed{TNumericLimits<float>::Max()};
	float TimeSinceGroundProbe{0.0f};
	float BodyRefreshElapsed{TNumericLimits<float>::Max()};
	TArray<FFlightBody> FlightBodies;
	TMap<TWeakObjectPtr<AActor>, TPair<FVector, double>> StationBoundsCache;
	/**
	 * The cluster stars the stellar view draws and no star actor represents, as offsets from the home system: the
	 * ship's own cluster lookup ran before the generator and found a placeholder (30.09), and a world-origin rebase
	 * moves the home, not these offsets.
	 */
	TWeakObjectPtr<AAstroGenerator> CatalogueGenerator;
	TWeakObjectPtr<AActor> CatalogueHome;
	uint64 CatalogueBuildSerial{0};
	uint64 CatalogueBatchSerial{0};
	uint64 CatalogueMutationSerial{0};
	TArray<FVector> CatalogueFromHome;
	/** Bounding sphere of the catalogue around its centroid: charted space ends a little beyond it. */
	FVector CatalogueCenterFromHome{FVector::ZeroVector};
	double CatalogueRadiusCm{0.0};
	/** Median distance between neighbouring catalogue stars (F2 cap on the star term of the speed limit). */
	double CatalogueSpacingMedianCm{0.0};
	/** How far the ship is outside the catalogue sphere, cm; 0 inside (F2: bounded speed beyond the cluster). */
	double CatalogueGapCm{0.0};
	/** No ceiling (Rio 02.10): how far the held limit has crept above the band's limit (1 = not at all), and the band
	 * it crept in (a band change starts over). */
	double LimitCreep{1.0};
	EAPSFlightBand CreepBand{EAPSFlightBand::Flight};
	/** The last scan: the nearest catalogue stars and the one whose system the course runs into. */
	TArray<int32, TInlineAllocator<8>> NearestCatalogueStars;
	int32 CourseCatalogueStar{INDEX_NONE};
	/** Rio 03.10: the galaxy stars of the last scan, as offsets from the home system (xyz) and drawn radius (w), cm. */
	TArray<FVector4, TInlineAllocator<10>> NearestGalaxyStars;
	/** Their local spacing (the star drive's cruise past the cluster's edge), cm; 0 unknown. */
	double GalaxySpacingCm{0.0};
	float CatalogueScanElapsed{TNumericLimits<float>::Max()};
	FVector CatalogueScanHeading{FVector::ZeroVector};
	bool bCatalogueMissingLogged{false};
	bool bDebugDrive{false};
	float DebugForwardInput{0.0f};
	bool bDebugBoost{false};
	/** Autopilot state: the target, the stop distance from its surface, the rotation it set last frame (a different
	 * rotation now means the pilot steered), and the distance left for the HUD. */
	void UpdateAutopilot(float DeltaTime);
	TWeakObjectPtr<AActor> AutopilotTarget;
	double AutopilotArrivalCm{0.0};
	double AutopilotRemainingCm{0.0};
	FQuat AutopilotLastRotation{FQuat::Identity};
	bool bAutopilotRotated{false};
	/** The course the autopilot steers along (eased toward its aim), the level up it banks from, its bank (degrees) and
	 * the speed it holds to reach its stop evenly (no cap while off). */
	FVector AutopilotCourse{FVector::ZeroVector};
	FVector AutopilotLevelUp{FVector::ZeroVector};
	double AutopilotBankDegrees{0.0};
	double AutopilotSpeedCapCm{TNumericLimits<double>::Max()};
	/** Star drive: moves the ship for one frame (false: the drive dropped out and the bands fly this frame). */
	bool ApplyStarDrive(const FVector& LocalInput, float DeltaTime);
	/** The drive's speed without input: the median star spacing in aps.Ship.Drive.CrossSeconds. */
	double StarDriveCruiseCm() const;
	/** Moves the ship by Velocity for one frame: the closing guard, the sweep (always for short frames) and contacts. */
	void MoveShip(FVector Velocity, bool bSweepBand, float DeltaTime);
	bool bStarDrive{false};
	float StarDriveSeconds{0.0f};
	/** Seconds S has been held at the bottom of the drive's range (a second winds the drive down). */
	float StarDriveIdleSeconds{0.0f};
	/** The speed the pilot set (W/S), and the speed the spool started from, cm/s. */
	double StarDriveSetCm{0.0};
	double StarDriveFromCm{0.0};
	/** The drive's nearest-body distance last frame (-1: none): a growing one is a departure (aps.RealScale.DriveDepartFactor). */
	double StarDrivePreviousLocalCm{-1.0};
	/** Outside every star system since engaging: entering one now is an arrival and drops the drive. */
	bool bStarDriveLeftSystem{false};
	/** What holds the drive below the set speed (a body near, a system ahead or around), for the HUD; null: nothing. */
	const TCHAR* StarDriveHeldBy{nullptr};
	/** A short line for the HUD: why the drive refused or dropped out. */
	FString StarDriveNotice;
	double StarDriveNoticeUntil{0.0};
	FVector SmoothedSteering{FVector::ZeroVector};
	/** A/D's eased yaw while the mouse is on the camera (Rio 04.10: the keys turned too sharply in cruise). */
	double KeySteeringYaw{0.0};
	double LastLogSeconds{0.0};
	double LastContactLogSeconds{0.0};

public:
	/**
	 * Ground vehicles (Rio 02.10, ASpaceship::ConfigureAsGroundVehicle): sets a parked vehicle down at rest on the ground
	 * under it, aligned with it. The ground is the collision where it exists near the vehicle; without it, and unless
	 * bRequireCollision, the WorldScape height. True when the vehicle now rests on collision; false for a ship, a piloted
	 * or moving vehicle, or no ground yet.
	 */
	bool SettleVehicle(bool bRequireCollision);
	/** A ground vehicle's chase-camera frame: its heading, the gravity up and the look pitch (degrees, up positive). */
	bool GetVehicleCameraFrame(FVector& OutForward, FVector& OutUp, double& OutPitchDegrees) const;

private:
	/** The pilot's controls for one vehicle frame (nothing powered without a pilot or with the engine off). */
	struct FVehicleControls
	{
		double Throttle{0.0};
		double Steer{0.0};
		double Strafe{0.0};
		double Vertical{0.0};
		double MouseYaw{0.0};
		double MousePitch{0.0};
		bool bBoost{false};
		bool bBrake{false};
		bool bPowered{false};
	};
	/**
	 * What lies under a vehicle: the plane of its wheel or corner probes (else the WorldScape height) and the liquid
	 * surface of a sea world. Heights are the vehicle origin's over them along the gravity up, cm. Probes are ordered
	 * front-left, front-right, back-left, back-right.
	 */
	struct FVehicleGround
	{
		FVector Point{FVector::ZeroVector};
		FVector Normal{FVector::UpVector};
		double Height{0.0};
		double LiquidHeight{TNumericLimits<double>::Max()};
		FVector Contact[4]{FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector};
		bool bHit[4]{false, false, false, false};
		int32 Hits{0};
		bool bValid{false};
		bool bCollision{false};
	};
	/** A rover, hover or drone between frames. */
	struct FVehicleState
	{
		FVector Heading{FVector::ZeroVector};
		FVector BodyUp{FVector::ZeroVector};
		FVector GroundNormal{FVector::UpVector};
		FVector LastVelocity{FVector::ZeroVector};
		FVector NoiseSampleLocation{FVector::ZeroVector};
		double NoiseGroundRadiusCm{-1.0};
		double NoiseSampleSeconds{-1.0};
		double YawRate{0.0};
		double Steer{0.0};
		double PendingYaw{0.0};
		double LookPitch{0.0};
		double Roll{0.0};
		double Pitch{0.0};
		double WheelSpin{0.0};
		double WheelDrop[4]{0.0, 0.0, 0.0, 0.0};
		double GroundHeightCm{-1.0};
		double AltitudeCm{-1.0};
		double CeilingCm{0.0};
		double SlopeDegrees{0.0};
		double RestSeconds{0.0};
		double ParkSeconds{0.0};
		double BobSeconds{0.0};
		/** Hover: what is left of a Space hop's soft field, and whether Space is still held (one hop per press). */
		double HopSeconds{0.0};
		bool bHopHeld{false};
		double LastLogSeconds{0.0};
		bool bGrounded{false};
		bool bOnLiquid{false};
		bool bCollisionGround{false};
		bool bInitialized{false};
	};
	/** ApplyTranslation's branch for a ground vehicle: drives it in the gravity frame, never the bands. Always true. */
	bool ApplyVehicleTranslation(float DeltaTime);
	void ResetVehicleState();
	FVehicleControls ReadVehicleControls(float DeltaTime) const;
	/** bTrace false skips the collision probes (high over the ground, where WorldScape builds none). */
	bool ProbeVehicleGround(const FVector& Up, double AboveCm, double BelowCm, FVehicleGround& OutGround,
		bool bTrace = true);
	void StepRover(const FVehicleControls& Controls, const FVector& Up, double Gravity, double DeltaTime);
	void StepHover(const FVehicleControls& Controls, const FVector& Up, double Gravity, double DeltaTime);
	void StepDrone(const FVehicleControls& Controls, const FVector& Up, double Gravity, double DeltaTime);
	/** Sweeps the vehicle by Delta: walls stop it and it slides along them; a wheeled or hovering one drives on over a
	 * ground-like bump (its probes then lift it). */
	void MoveVehicle(FVector& Velocity, const FVector& Delta, const FVector& Up);
	/** Turns the hull to the body frame (heading, up, lean) and the rover's tyres (roll, steering, suspension). */
	void PoseVehicle(const FVector& Up, double DeltaTime);
	/** At rest without a pilot: velocity zero, engine and tick off, attached to its world like the colony. */
	void ParkVehicle();
	FVector VehicleDown() const;
	double VehicleGravity() const;
	FString GetVehicleStatusText() const;
	FString GetVehicleHintText() const;
	/** A short HUD line for a key a vehicle does not take (flight modes, star drive, autopilot). */
	void VehicleNotice(const TCHAR* Text);
	FVehicleState Vehicle;
};
