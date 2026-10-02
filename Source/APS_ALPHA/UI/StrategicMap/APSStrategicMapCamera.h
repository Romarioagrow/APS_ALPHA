#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"

class AActor;
class ACameraActor;
class APlayerController;
class UWorld;

/**
 * The strategic map's own camera (Rio 02.10: "HOME SYSTEM shows nothing, the camera falls into the star; in LIVE UNIVERSE
 * the planet is see-through; FPS drops when rotating"). The map used to drive the generator's preview presentation; now a
 * transient camera actor takes the player's view and the real world renders as it is: WorldScape planets with their
 * pawn-centred streaming untouched, stations, ships and the gameplay stellar view, which follows the view point.
 *
 * The camera circles a focus (an actor, a catalogue star system or a point beside one after a pan) at a distance, in the
 * home system's ecliptic frame. Moves between foci fly the van Wijk-Nuij zoom-pan path, so a jump from a planet to the
 * cluster first rises and then descends instead of streaking at one height. It ticks after the actors and before the
 * camera manager, so a focused ship that moves this frame stays centred. Plain C++: no reflection needed.
 */
class FAPSStrategicMapCamera final : public FTickableGameObject
{
public:
	/** What the camera circles. A pan keeps the actor or system and moves the offset, so origin shifts never break it. */
	struct FFocus
	{
		TWeakObjectPtr<AActor> Actor;
		int32 SystemIndex{INDEX_NONE};
		FVector Offset{FVector::ZeroVector};
		/** Zoom stops at 1.5 times this from the focus (cm). */
		double RadiusCm{0.0};
		bool bOnActor{false};
	};

	static constexpr double AstronomicalUnitCm = 1.495978707e13;
	/** Rio's brief: from about 1.5 body radii up to some 300 AU from home. */
	static constexpr double MaximumDistanceCm = 300.0 * AstronomicalUnitCm;
	static constexpr float MapFieldOfView = 50.0f;

	FAPSStrategicMapCamera() = default;
	virtual ~FAPSStrategicMapCamera() override;

	/**
	 * Spawns the camera exactly at the player's current view (no jump) and takes the controller's view target.
	 * Reference: the actor free points are kept relative to (the home star). FrameUp: the ecliptic's normal.
	 */
	bool Begin(APlayerController* InController, AActor* InReference, const FVector& FrameUp);
	/** Leaves the camera a moment for the controller's blend back to the pilot, then it goes. */
	void End();
	bool IsActive() const { return Camera.IsValid(); }

	/** Mouse input (screen pixels). Ignored while a flight runs. */
	void Orbit(const FVector2D& DeltaPixels);
	void Zoom(float WheelDelta, bool bFast);
	void Pan(const FVector2D& DeltaPixels, double ViewportWidthPixels);
	/** Flies to a focus and frames a sphere of this radius in the map region. Pitch/yaw in the ecliptic frame, degrees. */
	void FlyTo(const FFocus& Target, double FrameRadiusCm, TOptional<double> InPitch = TOptional<double>(),
		TOptional<double> InYaw = TOptional<double>());
	/** Flies to a focus and stops at this distance from it (a remembered view). */
	void FlyToDistance(const FFocus& Target, double EndDistanceCm, TOptional<double> InPitch = TOptional<double>(),
		TOptional<double> InYaw = TOptional<double>());
	/** Flies to a focus seen from a direction (from the focus toward the camera): a colony from above its ground. */
	void FlyToFrom(const FFocus& Target, double FrameRadiusCm, const FVector& ViewFrom);

	/**
	 * The map region inside the viewport (the side panels cover the rest): its centre's offset from the viewport centre and
	 * its half extent, both in viewport half-widths. The focus is kept in the region's centre and framing fits the region.
	 */
	void SetRegion(const FVector2D& CentreOffset, double FitFraction);
	/** The view has told where the map region is: framing is exact from now on. */
	bool HasRegion() const { return bHasRegion; }
	/** Bodies the camera never enters (centre actor and keep-out radius); read every tick, so moving bodies are followed. */
	void SetObstacles(TArray<TPair<TWeakObjectPtr<AActor>, double>>&& InObstacles);
	/** Where a catalogue system is now (FAPSStarSystems follows the home system through origin shifts). */
	void SetSystemLocator(TFunction<bool(int32, FVector&)> InLocator) { SystemLocator = MoveTemp(InLocator); }

	/** The point the camera looks at now (during a flight the moving one). */
	FVector GetLookLocation() const { return LookLocation; }
	FVector GetCameraLocation() const { return CameraLocation; }
	/** The focus the camera holds, or flies to. */
	const FFocus& GetFocus() const { return bFlying ? Flight.Target : Focus; }
	double GetDistance() const { return Distance; }
	double GetYaw() const { return Yaw; }
	double GetPitch() const { return Pitch; }
	/** Where the camera settles: the flight's end, else the wheel's target. */
	double GetTargetDistance() const { return bFlying ? Flight.EndDistance : DesiredDistance; }
	double GetTargetYaw() const { return bFlying ? Flight.EndYaw : Yaw; }
	double GetTargetPitch() const { return bFlying ? Flight.EndPitch : Pitch; }
	double GetFieldOfView() const { return FieldOfView; }
	bool IsFlying() const { return bFlying; }
	ACameraActor* GetCameraActor() const { return Camera.Get(); }

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override { return Camera.IsValid(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return World.Get(); }

private:
	/** The van Wijk-Nuij smooth zoom and pan (as d3.interpolateZoom), with view widths in cm. */
	struct FFlight
	{
		FFocus Target;
		/** The look point at the start, relative to the target's location then. */
		FVector StartRelative{FVector::ZeroVector};
		FQuat StartRotation{FQuat::Identity};
		double EndYaw{0.0};
		double EndPitch{0.0};
		double EndDistance{0.0};
		double StartFieldOfView{90.0};
		double StartRegionWeight{1.0};
		double Time{0.0};
		double Duration{1.0};
		double StartWidth{1.0};
		double EndWidth{1.0};
		double PanLength{0.0};
		double R0{0.0};
		double PathLength{0.0};
		double EndFraction{1.0};
		double EndWidthRaw{1.0};
		bool bZoomOnly{true};

		void Prepare(double InStartWidth, double InEndWidth, double InPanLength);
		/** Share of the pan done and the view width at Alpha (0..1), with exact ends. */
		void Evaluate(double Alpha, double& OutFraction, double& OutWidth) const;

	private:
		void EvaluateRaw(double Alpha, double& OutFraction, double& OutWidth) const;
	};

	FVector Locate(const FFocus& Target) const;
	double MinimumDistance(const FFocus& Target) const;
	double WidthPerDistance() const;
	FQuat OrbitRotation(double InYaw, double InPitch) const;
	void Apply();

	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<ACameraActor> Camera;
	TWeakObjectPtr<AActor> Reference;
	TFunction<bool(int32, FVector&)> SystemLocator;
	TArray<TPair<TWeakObjectPtr<AActor>, double>> Obstacles;
	/** The ecliptic frame: Z the home planets' orbit normal. */
	FQuat Frame{FQuat::Identity};

	FFocus Focus;
	double Yaw{0.0};
	double Pitch{-35.0};
	double Distance{2000.0};
	double DesiredDistance{2000.0};
	double FieldOfView{90.0};
	FQuat Rotation{FQuat::Identity};
	FVector LookLocation{FVector::ZeroVector};
	FVector CameraLocation{FVector::ZeroVector};
	/** Last resolved focus point: a focus whose actor vanished continues from here. */
	FVector LastFocusLocation{FVector::ZeroVector};

	FVector2D RegionOffset{FVector2D::ZeroVector};
	double RegionFit{0.6};
	bool bHasRegion{false};

	FFlight Flight;
	bool bFlying{false};
	/** From Begin to the first flight the camera holds the pilot's exact view. */
	bool bHolding{true};
	/** How much of the map region's offset applies: none at the pilot's view, all once the first flight is done. */
	double RegionWeight{0.0};
};
