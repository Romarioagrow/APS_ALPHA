#pragma once

#include "CoreMinimal.h"

class ASpaceship;

/**
 * Reproducible flight load for the "FPS drops at speed near the surface" investigation.
 *
 * aps.Ship.Benchmark [AltitudeMeters=1000] [SecondsPerStep=8] [SpeedsMps=0,100,400,1600,6400,25600] [Sweep=1]
 *   Flies the piloted ship on a great circle around the nearest planet at a fixed altitude above the base
 *   sphere, one speed per step, through the ship's normal kinematic move (MoveShipKinematic). Each step logs
 *   frame time (avg/p95/p99/max), game/render/GPU time, the move call, WorldScape LODs in generation and
 *   blocked moves, and writes a CSV to Saved/Diagnostics/ShipFlight.
 * aps.Ship.BenchmarkStop
 * aps.Ship.Board [NameFilter]: the player's pawn takes the nearest free ship whose name contains the filter.
 * aps.Ship.AutoBenchmark <ShipNameFilter> [Altitude] [SecondsPerStep] [Speeds a/b/c] [Sweep] [quit] [warmup=15]
 *   Unattended -game run: waits for the player pawn, boards the ship, runs the benchmark, optionally quits.
 * aps.Ship.Report
 *   Logs what the piloted ship moves every frame: primitives, collision/overlap flags, body shapes,
 *   attached lights and shadow casters.
 * aps.Ship.Drive <Power 1-6> [Forward=1] [Boost=0] [aim=keep|planet|horizon|up] [pitch=Deg] [minalt=Km] [yaw=-1..1] [engine=1-3]
 *   Holds the piloted ship's own controls (power step, thrust, boost) as a player would, so the regular flight
 *   code moves it, and logs speed, altitude and frame/game/render/GPU time once per second.
 * aps.Ship.DriveStop
 * aps.Ship.StartGenerated [SpawnPlace=0] [ShipClassPath|-] [CharacterClassPath]
 *   In the main menu: opens the Civilization generator on the home planet and starts L_WorldGeneration the way the
 *   menu's own Start does (development builds only).
 */
class APS_ALPHA_API FAPSShipFlightBenchmark
{
public:
	/** Called by ASpaceship::Tick; returns true when the benchmark moved this ship this frame. */
	static bool TickShip(ASpaceship& Ship, float DeltaTime);

	static bool IsRunning();

	/** Actor rotation that points the ship's flight nose along Forward and its flight up along Up. */
	static FQuat GetRotationForFlightAxes(const ASpaceship& Ship, const FVector& Forward, const FVector& Up);

	/** Makes a physically simulated hull kinematic for the run, or restores it afterwards. */
	static void SetBenchmarkKinematic(ASpaceship& Ship, bool bKinematic, bool& bInOutHadPhysicalImpulse);

	static void SetKinematicVelocity(ASpaceship& Ship, const FVector& Velocity);

	/** The inputs a pilot holds: power step (1-6), thrust, boost, brake and yaw (-1..1 of the steering range). */
	static void SetPilotControls(ASpaceship& Ship, int32 Power, float Forward, bool bBoost, bool bBrake, float Yaw = 0.0f);

	/** The pilot's engine keys: 1 Impulse, 2 SpaceWrap, 3 Offset. */
	static void SelectEngine(ASpaceship& Ship, int32 Engine);

	static void LogShipReport(const ASpaceship& Ship);
};
