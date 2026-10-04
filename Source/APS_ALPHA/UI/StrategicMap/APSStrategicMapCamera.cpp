#include "APSStrategicMapCamera.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"
#include "Misc/App.h"

namespace APSStrategicMapCameraLocal
{
	TAutoConsoleVariable<float> CVarFieldOfView(TEXT("aps.Map.FieldOfView"), 0.0f,
		TEXT("The strategic map's lens in degrees. 0 keeps the pilot's field of view from F10 to the return: no field of ")
		TEXT("view change, so the full-scale star catalogue is never re-sized for it (Rio 03.10, the F10 freezes). 20-120: ")
		TEXT("that lens, switched in one step halfway through a flight and back in one step on close (one ~50 ms ")
		TEXT("re-size each). 50 is the 02.10 map's lens."));

	/** The lens a map opened from this field of view settles on. */
	double LensFor(const double PilotFieldOfView)
	{
		const float Wanted = CVarFieldOfView.GetValueOnGameThread();
		return FMath::Clamp(Wanted > 0.0f ? static_cast<double>(Wanted) : PilotFieldOfView, 20.0, 120.0);
	}

	/** The distance factor that keeps the view width at the focus when the lens changes. */
	double LensRatio(const double FromDegrees, const double ToDegrees)
	{
		const double To = FMath::Tan(FMath::DegreesToRadians(ToDegrees * 0.5));
		const double Ratio = To > UE_DOUBLE_SMALL_NUMBER ? FMath::Tan(FMath::DegreesToRadians(FromDegrees * 0.5)) / To : 1.0;
		return FMath::IsFinite(Ratio) && Ratio > 0.0 ? Ratio : 1.0;
	}

	/** Look distance the rig starts with, just ahead of the player's own camera (cm). */
	constexpr double StartLookDistanceCm = 2000.0;
	/** The closest any focus may be approached (cm): a ship's hull still fits the view. */
	constexpr double NearestApproachCm = 5000.0;
	constexpr double OrbitDegreesPerPixel = 0.22;
	constexpr double PitchLimit = 88.0;

	double Asinh(const double Value)
	{
		// The stable form: no cancellation for the large arguments ten orders of zoom produce.
		const double Magnitude = FMath::Abs(Value);
		const double Result = FMath::Loge(Magnitude + FMath::Sqrt(Magnitude * Magnitude + 1.0));
		return Value < 0.0 ? -Result : Result;
	}

	double SmoothStep(const double Alpha)
	{
		const double Clamped = FMath::Clamp(Alpha, 0.0, 1.0);
		return Clamped * Clamped * (3.0 - 2.0 * Clamped);
	}
}

FAPSStrategicMapCamera::~FAPSStrategicMapCamera()
{
	End();
}

bool FAPSStrategicMapCamera::Begin(APlayerController* InController, AActor* InReference, const FVector& FrameUp)
{
	UWorld* InWorld = InController ? InController->GetWorld() : nullptr;
	if (!InWorld)
	{
		return false;
	}
	Controller = InController;
	World = InWorld;
	Reference = InReference;
	const FVector Up = FrameUp.GetSafeNormal();
	Frame = FRotationMatrix::MakeFromZ(Up.IsNearlyZero() ? FVector::UpVector : Up).ToQuat();

	// Start exactly at the player's view, so taking the view target needs no blend; the first flight then rises from it.
	FVector StartLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	float StartFieldOfView = 90.0f;
	const APlayerCameraManager* CameraManager = InController->PlayerCameraManager;
	if (CameraManager)
	{
		const FMinimalViewInfo& View = CameraManager->GetCameraCacheView();
		StartLocation = View.Location;
		StartRotation = View.Rotation;
		StartFieldOfView = View.FOV > 1.0f ? View.FOV : 90.0f;
	}
	else
	{
		InController->GetPlayerViewPoint(StartLocation, StartRotation);
	}

	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACameraActor* Spawned = InWorld->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), StartLocation, StartRotation,
		Parameters);
	if (!Spawned)
	{
		return false;
	}
	Spawned->SetActorEnableCollision(false);
	if (UCameraComponent* Component = Spawned->GetCameraComponent())
	{
		// The camera actor's default 16:9 constraint would letterbox other screens.
		Component->bConstrainAspectRatio = false;
		Component->SetFieldOfView(StartFieldOfView);
		if (CameraManager)
		{
			// Keep the player camera's exposure and grading, so the world keeps its look on the map.
			const FMinimalViewInfo& View = CameraManager->GetCameraCacheView();
			Component->PostProcessSettings = View.PostProcessSettings;
			Component->PostProcessBlendWeight = View.PostProcessBlendWeight;
		}
	}
	Camera = Spawned;

	// The rig state that reproduces this view: a look point just ahead, the same orientation and field of view.
	Rotation = StartRotation.Quaternion();
	FieldOfView = PilotFieldOfView = StartFieldOfView;
	LensFieldOfView = APSStrategicMapCameraLocal::LensFor(StartFieldOfView);
	Distance = DesiredDistance = APSStrategicMapCameraLocal::StartLookDistanceCm;
	LookLocation = StartLocation + Rotation.GetForwardVector() * Distance;
	CameraLocation = StartLocation;
	const FRotator InFrame = (Frame.Inverse() * Rotation).Rotator();
	Yaw = InFrame.Yaw;
	Pitch = FMath::Clamp(static_cast<double>(InFrame.Pitch), -APSStrategicMapCameraLocal::PitchLimit,
		APSStrategicMapCameraLocal::PitchLimit);
	Focus = FFocus();
	if (const AActor* ReferenceActor = Reference.Get())
	{
		Focus.Actor = Reference;
		Focus.bOnActor = true;
		Focus.Offset = LookLocation - ReferenceActor->GetActorLocation();
	}
	else
	{
		Focus.Offset = LookLocation;
	}
	LastFocusLocation = LookLocation;
	InController->SetViewTarget(Spawned);
	return true;
}

void FAPSStrategicMapCamera::End()
{
	if (ACameraActor* Spawned = Camera.Get())
	{
		// Rio 03.10: back to the pilot's lens in one step, or the controller's blend would change the field of view (and
		// re-size the star catalogue) every frame of the return; the focus keeps its place and size on the screen.
		if (!bHolding && !FMath::IsNearlyEqual(FieldOfView, PilotFieldOfView, 0.01))
		{
			Distance *= APSStrategicMapCameraLocal::LensRatio(FieldOfView, PilotFieldOfView);
			FieldOfView = PilotFieldOfView;
			Apply();
		}
		// The controller blends from this view back to the pilot's (CloseStrategicMap, 0.3 s): the camera holds still
		// that long, then goes by itself.
		Spawned->SetLifeSpan(1.0f);
	}
	Camera.Reset();
	bFlying = false;
}

TStatId FAPSStrategicMapCamera::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSStrategicMapCamera, STATGROUP_Tickables);
}

void FAPSStrategicMapCamera::Orbit(const FVector2D& DeltaPixels)
{
	if (bFlying || bHolding || !Camera.IsValid())
	{
		return;
	}
	Yaw = FMath::UnwindDegrees(Yaw + DeltaPixels.X * APSStrategicMapCameraLocal::OrbitDegreesPerPixel);
	// Dragging down tips the camera over the focus, as the world is grabbed and turned.
	Pitch = FMath::Clamp(Pitch - DeltaPixels.Y * APSStrategicMapCameraLocal::OrbitDegreesPerPixel,
		-APSStrategicMapCameraLocal::PitchLimit, APSStrategicMapCameraLocal::PitchLimit);
}

void FAPSStrategicMapCamera::Zoom(const float WheelDelta, const bool bFast)
{
	if (bFlying || bHolding || !Camera.IsValid())
	{
		return;
	}
	// Logarithmic: every notch scales the distance, from a hull's length to hundreds of AU.
	const double Step = bFast ? 2.4 : 1.32;
	DesiredDistance = FMath::Clamp(DesiredDistance * FMath::Pow(Step, -static_cast<double>(WheelDelta)),
		MinimumDistance(Focus), MaximumDistanceCm);
}

void FAPSStrategicMapCamera::Pan(const FVector2D& DeltaPixels, const double ViewportWidthPixels)
{
	if (bFlying || bHolding || !Camera.IsValid() || ViewportWidthPixels <= 1.0)
	{
		return;
	}
	// The focus moves with the cursor at its own depth: the world under the cursor stays under it.
	const double WorldPerPixel = Distance * WidthPerDistance() / ViewportWidthPixels;
	Focus.Offset += (-Rotation.GetRightVector() * DeltaPixels.X + Rotation.GetUpVector() * DeltaPixels.Y) * WorldPerPixel;
}

void FAPSStrategicMapCamera::FlyTo(const FFocus& Target, const double FrameRadiusCm, const TOptional<double> InPitch,
	const TOptional<double> InYaw)
{
	// Fit the sphere in the map region: the distance at which its angular radius equals the region's half angle. The
	// flight starts with the field of view of now; a lens switch on the way keeps the framing (Tick).
	const double HalfTangent = FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5));
	const double Fit = FMath::Max(RegionFit * HalfTangent, 0.05);
	const double Radius = FMath::Max(FrameRadiusCm, 1.0);
	FlyToDistance(Target, Radius * FMath::Sqrt(1.0 + 1.0 / (Fit * Fit)) * 1.06, InPitch, InYaw);
}

void FAPSStrategicMapCamera::FlyToDistance(const FFocus& Target, const double EndDistanceCm, const TOptional<double> InPitch,
	const TOptional<double> InYaw)
{
	if (!Camera.IsValid())
	{
		return;
	}
	const double EndPitch = FMath::Clamp(InPitch.Get(Pitch), -APSStrategicMapCameraLocal::PitchLimit,
		APSStrategicMapCameraLocal::PitchLimit);
	const double EndYaw = InYaw.Get(Yaw);
	const double EndDistance = FMath::Clamp(FMath::IsFinite(EndDistanceCm) ? EndDistanceCm : MaximumDistanceCm,
		MinimumDistance(Target), MaximumDistanceCm);

	Flight = FFlight();
	Flight.Target = Target;
	Flight.StartRelative = LookLocation - Locate(Target);
	Flight.StartRotation = Rotation;
	Flight.EndYaw = EndYaw;
	Flight.EndPitch = EndPitch;
	Flight.EndDistance = EndDistance;
	Flight.StartRegionWeight = RegionWeight;
	const double Width = WidthPerDistance();
	Flight.Prepare(Distance * Width, EndDistance * Width, Flight.StartRelative.Size());
	// Longer for a longer path through zoom space; never a crawl, never a cut.
	Flight.Duration = FMath::Clamp(0.45 + 0.085 * FMath::Abs(Flight.PathLength), 0.6, 2.2);
	bFlying = true;
	bHolding = false;
}

void FAPSStrategicMapCamera::FlyToFrom(const FFocus& Target, const double FrameRadiusCm, const FVector& ViewFrom)
{
	// The camera looks back along ViewFrom; its yaw and pitch in the ecliptic frame follow from that.
	const FVector Look = -ViewFrom.GetSafeNormal();
	if (Look.IsNearlyZero())
	{
		FlyTo(Target, FrameRadiusCm);
		return;
	}
	const FRotator InFrame = Frame.Inverse().RotateVector(Look).Rotation();
	FlyTo(Target, FrameRadiusCm, static_cast<double>(InFrame.Pitch), static_cast<double>(InFrame.Yaw));
}

void FAPSStrategicMapCamera::SetRegion(const FVector2D& CentreOffset, const double FitFraction)
{
	if (!CentreOffset.ContainsNaN() && FMath::IsFinite(FitFraction) && FitFraction > 0.01)
	{
		RegionOffset = CentreOffset;
		RegionFit = FMath::Clamp(FitFraction, 0.05, 1.5);
		bHasRegion = true;
	}
}

void FAPSStrategicMapCamera::SetObstacles(TArray<TPair<TWeakObjectPtr<AActor>, double>>&& InObstacles)
{
	Obstacles = MoveTemp(InObstacles);
}

FVector FAPSStrategicMapCamera::Locate(const FFocus& Target) const
{
	if (const AActor* Actor = Target.Actor.Get())
	{
		return Actor->GetActorLocation() + Target.Offset;
	}
	FVector SystemLocation;
	if (Target.SystemIndex != INDEX_NONE && SystemLocator && SystemLocator(Target.SystemIndex, SystemLocation))
	{
		return SystemLocation + Target.Offset;
	}
	// An actor focus whose actor is gone: where it was last seen.
	return Target.bOnActor ? LastFocusLocation : Target.Offset;
}

double FAPSStrategicMapCamera::MinimumDistance(const FFocus& Target) const
{
	return FMath::Max(Target.RadiusCm * 1.5, APSStrategicMapCameraLocal::NearestApproachCm);
}

double FAPSStrategicMapCamera::WidthPerDistance() const
{
	return 2.0 * FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5));
}

FQuat FAPSStrategicMapCamera::OrbitRotation(const double InYaw, const double InPitch) const
{
	return Frame * FRotator(InPitch, InYaw, 0.0).Quaternion();
}

void FAPSStrategicMapCamera::Tick(float)
{
	if (!Camera.IsValid())
	{
		return;
	}
	if (bHolding)
	{
		// Until the first flight the camera is exactly the pilot's view: no snap of roll, field of view or position.
		return;
	}
	// Real time: the map animates the same whatever the world's time dilation.
	const double Delta = FMath::Clamp(FApp::GetDeltaTime(), 0.0, 0.1);
	if (Focus.bOnActor && !Focus.Actor.IsValid())
	{
		// The focused actor is gone (a ship docked and was removed): hold the point where it was.
		Focus.bOnActor = false;
		Focus.SystemIndex = INDEX_NONE;
		if (const AActor* ReferenceActor = Reference.Get())
		{
			Focus.Actor = Reference;
			Focus.bOnActor = true;
			Focus.Offset = LastFocusLocation - ReferenceActor->GetActorLocation();
		}
		else
		{
			Focus.Offset = LastFocusLocation;
		}
	}

	if (bFlying)
	{
		Flight.Time += Delta;
		const double Alpha = FMath::Clamp(Flight.Time / FMath::Max(Flight.Duration, 0.01), 0.0, 1.0);
		const double Eased = APSStrategicMapCameraLocal::SmoothStep(Alpha);
		double Fraction = Eased;
		double Width = Flight.EndWidth;
		Flight.Evaluate(Eased, Fraction, Width);
		const FVector TargetLocation = Locate(Flight.Target);
		LookLocation = TargetLocation + Flight.StartRelative * (1.0 - Fraction);
		if (Eased >= 0.5 && !FMath::IsNearlyEqual(FieldOfView, LensFieldOfView, 0.01))
		{
			// Rio 03.10: another lens comes in one step halfway, where the view moves fastest, never frame by frame (each
			// changed frame re-sizes the whole star catalogue). The view width at the focus carries on unchanged.
			Flight.EndDistance = FMath::Clamp(Flight.EndDistance
				* APSStrategicMapCameraLocal::LensRatio(FieldOfView, LensFieldOfView), MinimumDistance(Flight.Target),
				MaximumDistanceCm);
			FieldOfView = LensFieldOfView;
		}
		Distance = FMath::Max(Width / WidthPerDistance(), 1.0);
		Rotation = FQuat::Slerp(Flight.StartRotation, OrbitRotation(Flight.EndYaw, Flight.EndPitch), Eased).GetNormalized();
		// The first flight leaves the pilot's view centred and moves the focus to the map region's middle as it goes.
		RegionWeight = FMath::Lerp(Flight.StartRegionWeight, 1.0, Eased);
		if (Alpha >= 1.0)
		{
			bFlying = false;
			RegionWeight = 1.0;
			Focus = Flight.Target;
			Yaw = Flight.EndYaw;
			Pitch = Flight.EndPitch;
			Distance = DesiredDistance = Flight.EndDistance;
			LookLocation = TargetLocation;
		}
		LastFocusLocation = TargetLocation;
	}
	else
	{
		// The wheel sets where the zoom goes; the distance follows on a log scale, so every notch feels the same.
		const double Follow = 1.0 - FMath::Exp(-14.0 * Delta);
		Distance = FMath::Exp(FMath::Lerp(FMath::Loge(FMath::Max(Distance, 1.0)), FMath::Loge(FMath::Max(DesiredDistance, 1.0)),
			Follow));
		LookLocation = Locate(Focus);
		LastFocusLocation = LookLocation;
		Rotation = OrbitRotation(Yaw, Pitch);
	}
	Apply();
	// Rio 04.10 ("from ~900 AU the home planet's layers slide apart on the map"): a still view of something far from 0,0,0
	// asks the floating origin to come to it (it decides: only with the pilot's ship in open space). The focus is held
	// relative to its actor or system, so the next frame finds it again wherever the world went.
	if (!bFlying)
	{
		if (UWorld* LiveWorld = World.Get())
		{
			if (UAPSWorldOriginSubsystem* Origin = LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
			{
				Origin->RequestMapView(LookLocation, Distance);
			}
		}
	}
}

void FAPSStrategicMapCamera::Apply()
{
	ACameraActor* Spawned = Camera.Get();
	if (!Spawned)
	{
		return;
	}
	// The focus sits in the middle of the map region, not of the viewport: the camera looks past it by the region's offset.
	const double HalfTangent = FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5));
	const FVector Forward = Rotation.GetForwardVector();
	const double Shift = Distance * HalfTangent * RegionWeight;
	const FVector Base = LookLocation - Rotation.GetRightVector() * (RegionOffset.X * Shift)
		+ Rotation.GetUpVector() * (RegionOffset.Y * Shift);
	FVector Location = Base - Forward * Distance;
	// Never inside a star, planet or moon (Rio 02.10: "the camera falls into the star"). Like a spring arm the camera
	// slides along its own ray to where the ray leaves the body, so the focus stays in the middle; only a ray that runs
	// through the whole body pushes it straight out instead.
	for (const TPair<TWeakObjectPtr<AActor>, double>& Obstacle : Obstacles)
	{
		const AActor* Body = Obstacle.Key.Get();
		const double Keep = Obstacle.Value;
		if (!Body || Keep <= 0.0)
		{
			continue;
		}
		const FVector Centre = Body->GetActorLocation();
		if (FVector::DistSquared(Location, Centre) >= Keep * Keep)
		{
			continue;
		}
		// Points Base - Forward * T with T between the roots lie inside the sphere.
		const FVector ToBase = Base - Centre;
		const double Along = FVector::DotProduct(Forward, ToBase);
		const double Discriminant = Along * Along - ToBase.SizeSquared() + Keep * Keep;
		bool bMoved = false;
		if (Discriminant >= 0.0)
		{
			const double Root = FMath::Sqrt(Discriminant);
			const double NearT = Along - Root;
			const double FarT = Along + Root;
			if (NearT > FMath::Max(Distance * 0.01, 100.0))
			{
				// The body is behind the focus: stop in front of it, between it and the focus.
				Location = Base - Forward * (NearT * 0.999);
				bMoved = true;
			}
			else if (FarT < Keep * 2.2)
			{
				Location = Base - Forward * (FarT * 1.001);
				bMoved = true;
			}
		}
		if (!bMoved)
		{
			const FVector Away = Location - Centre;
			const double Range = Away.Size();
			Location = Centre + (Range > 1.0 ? Away / Range : -Forward) * Keep;
		}
	}
	if (Location.ContainsNaN() || Rotation.ContainsNaN())
	{
		return;
	}
	CameraLocation = Location;
	Spawned->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCameraComponent* Component = Spawned->GetCameraComponent())
	{
		Component->SetFieldOfView(static_cast<float>(FieldOfView));
	}
}

void FAPSStrategicMapCamera::FFlight::Prepare(const double InStartWidth, const double InEndWidth, const double InPanLength)
{
	constexpr double Rho = UE_DOUBLE_SQRT_2;
	constexpr double Rho2 = 2.0;
	constexpr double Rho4 = 4.0;
	StartWidth = FMath::IsFinite(InStartWidth) ? FMath::Max(InStartWidth, 1.0) : 1.0;
	EndWidth = FMath::IsFinite(InEndWidth) ? FMath::Max(InEndWidth, 1.0) : StartWidth;
	PanLength = FMath::IsFinite(InPanLength) ? FMath::Max(InPanLength, 0.0) : 0.0;
	bZoomOnly = PanLength < 1.0e-6 * FMath::Max(StartWidth, EndWidth);
	if (bZoomOnly)
	{
		PathLength = FMath::Loge(EndWidth / StartWidth) / Rho;
	}
	else
	{
		const double B0 = (EndWidth * EndWidth - StartWidth * StartWidth + Rho4 * PanLength * PanLength)
			/ (2.0 * StartWidth * Rho2 * PanLength);
		const double B1 = (EndWidth * EndWidth - StartWidth * StartWidth - Rho4 * PanLength * PanLength)
			/ (2.0 * EndWidth * Rho2 * PanLength);
		R0 = -APSStrategicMapCameraLocal::Asinh(B0);
		const double R1 = -APSStrategicMapCameraLocal::Asinh(B1);
		PathLength = (R1 - R0) / Rho;
	}
	if (!FMath::IsFinite(PathLength))
	{
		bZoomOnly = true;
		PathLength = 0.0;
	}
	// The closed forms drift by rounding over ten orders of magnitude: scale both ends back to exact.
	EndFraction = 1.0;
	EndWidthRaw = EndWidth;
	double Fraction = 1.0;
	double Width = EndWidth;
	EvaluateRaw(1.0, Fraction, Width);
	EndFraction = FMath::IsFinite(Fraction) && FMath::Abs(Fraction) > 1.0e-9 ? Fraction : 1.0;
	EndWidthRaw = FMath::IsFinite(Width) && Width > 0.0 ? Width : EndWidth;
}

void FAPSStrategicMapCamera::FFlight::EvaluateRaw(const double Alpha, double& OutFraction, double& OutWidth) const
{
	constexpr double Rho = UE_DOUBLE_SQRT_2;
	constexpr double Rho2 = 2.0;
	if (bZoomOnly)
	{
		OutFraction = Alpha;
		OutWidth = StartWidth * FMath::Exp(Rho * Alpha * PathLength);
		return;
	}
	const double Along = Rho * Alpha * PathLength + R0;
	const double CoshR0 = FMath::Cosh(R0);
	OutFraction = StartWidth / (Rho2 * PanLength) * (CoshR0 * FMath::Tanh(Along) - FMath::Sinh(R0));
	OutWidth = StartWidth * CoshR0 / FMath::Cosh(Along);
}

void FAPSStrategicMapCamera::FFlight::Evaluate(const double Alpha, double& OutFraction, double& OutWidth) const
{
	EvaluateRaw(Alpha, OutFraction, OutWidth);
	OutFraction = FMath::IsFinite(OutFraction) ? OutFraction / EndFraction : Alpha;
	OutWidth = FMath::IsFinite(OutWidth) && OutWidth > 0.0
		? OutWidth * FMath::Pow(EndWidth / EndWidthRaw, Alpha)
		: FMath::Exp(FMath::Lerp(FMath::Loge(StartWidth), FMath::Loge(EndWidth), Alpha));
	if (Alpha >= 1.0)
	{
		OutFraction = 1.0;
		OutWidth = EndWidth;
	}
}
