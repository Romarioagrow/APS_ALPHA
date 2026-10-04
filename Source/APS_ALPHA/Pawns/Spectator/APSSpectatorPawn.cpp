#include "APSSpectatorPawn.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Loading/APSArrivalCurtain.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Rendering/APSRenderSafety.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace APSFreeFlightLocal
{
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	constexpr double LightYearCm = 9.4607e17;
	constexpr double SolarRadiusCm = 6.957e10;
	/** Above this the body's sphere is close enough to its terrain (the ships' TerrainQueryAltitudeCm). */
	constexpr double TerrainQueryAltitudeCm = 5.0e7;
	/** Below this the terrain under the camera is sampled every frame, above it ten times a second. */
	constexpr double TerrainEveryFrameAltitudeCm = 2.0e6;
	constexpr double TerrainSampleSeconds = 0.1;
	constexpr double BodyRefreshSeconds = 0.25;
	constexpr int32 CatalogueNeighbours = 8;
	/** Speed tiers (mouse wheel, +/-): multipliers of the distance scale. */
	constexpr double Tiers[] = {0.1, 0.3, 1.0, 3.0, 10.0, 30.0, 100.0, 300.0, 1000.0};
	constexpr int32 TierCount = UE_ARRAY_COUNT(Tiers);
	constexpr int32 DefaultTier = 2;
	constexpr double BoostMultiplier = 10.0;
	constexpr double SlowMultiplier = 0.1;
	/** The tier/boost multiplier eases in log space with this time constant (95% in about 0.35 s). */
	constexpr double TierEaseSeconds = 0.12;
	/** The distance scale follows a farther surface over this time constant, a nearer one quickly (log space). */
	constexpr double AutoRiseSeconds = 0.45;
	constexpr double AutoFallSeconds = 0.08;
	constexpr double MinimumSpeedCm = 100.0;
	constexpr double MaxPitchDegrees = 89.0;
	/** The engine's legacy look scale over the 0.07 mouse axis sensitivity of DefaultInput.ini. */
	constexpr double MouseDegreesPerUnit = 2.5;
	constexpr double RollDegreesPerSecond = 70.0;
	constexpr double RollEaseSeconds = 0.12;
	/** After a roll the horizon assist waits this long before it levels the view to a world again. */
	constexpr double LevelHoldAfterRollSeconds = 2.0;
	/** A frame moving the camera farther than this tells the shadow cache first (the VSM int32 panning overflow). */
	constexpr double CameraJumpCm = 5.0e7;
	/** F: the standoff from the centre, in radii (planets and moons, stars) and for a station. */
	constexpr double PlanetStandoffRadii = 3.0;
	constexpr double StarStandoffRadii = 6.0;
	constexpr double StationStandoffCm = 30000.0;
	/** F picks a body this close to the crosshair (beyond its disc) when none is under it. */
	constexpr double AimToleranceRadians = 0.06;
	const TCHAR* const EdgeName = TEXT("EDGE OF FLIGHT RANGE");

	TAutoConsoleVariable<int32> CVarEnabled(TEXT("aps.FreeFlight"), 1,
		TEXT("Rio 03.10: 1 = a Generate Space world (no civilization) starts in the free-flight camera; 0 = the walker as ")
		TEXT("before. Read when the player spawns."));
	TAutoConsoleVariable<float> CVarSpeedFactor(TEXT("aps.FreeFlight.SpeedFactor"), 1.0f,
		TEXT("Free flight at tier x1: the speed is this many times the distance to the nearest surface per second."));
	TAutoConsoleVariable<float> CVarMinAltitude(TEXT("aps.FreeFlight.MinAltitude"), 3.0f,
		TEXT("Free flight stops this high above a planet's or moon's terrain (or sea), m."));
	TAutoConsoleVariable<float> CVarResponse(TEXT("aps.FreeFlight.Response"), 6.0f,
		TEXT("Free flight: how fast the movement keys ramp the speed up and down (critically damped), 1/s."));
	TAutoConsoleVariable<float> CVarLookSmoothing(TEXT("aps.FreeFlight.LookSmoothing"), 0.035f,
		TEXT("Free flight: mouse look smoothing time constant, s (0 = raw)."));
	TAutoConsoleVariable<float> CVarMouseSensitivity(TEXT("aps.FreeFlight.MouseSensitivity"), 1.0f,
		TEXT("Free flight: mouse look sensitivity multiplier."));
	TAutoConsoleVariable<int32> CVarInvertY(TEXT("aps.FreeFlight.InvertY"), 0,
		TEXT("Free flight: 1 inverts the mouse pitch."));
	TAutoConsoleVariable<float> CVarEdgeLightYears(TEXT("aps.FreeFlight.EdgeLightYears"), 30.0f,
		TEXT("Free flight slows down and stops this far from the world origin, ly (the renderer loses precision beyond ")
		TEXT("~42 ly; a calm camera is re-centred by the floating origin). 0 = no edge."));
	TAutoConsoleVariable<float> CVarFlyToResponse(TEXT("aps.FreeFlight.FlyToResponse"), 1.2f,
		TEXT("Free flight F: how fast the flight to a body eases along its way (log space), 1/s."));
	TAutoConsoleVariable<int32> CVarStartView(TEXT("aps.FreeFlight.StartView"), 1,
		TEXT("Free flight: 1 = a new Generate Space world opens on the home planet (three radii out, sunlit), behind the ")
		TEXT("loading curtain; 0 = the camera starts at the map's player start."));
	TAutoConsoleVariable<int32> CVarLog(TEXT("aps.FreeFlight.Log"), 0,
		TEXT("1 logs the free-flight speed, tier, nearest surface and stop gap once a second."));

	double EaseAlpha(const double DeltaSeconds, const double TimeConstant)
	{
		return TimeConstant > 0.0 ? 1.0 - FMath::Exp(-DeltaSeconds / TimeConstant) : 1.0;
	}

	double MinAltitudeCm()
	{
		return FMath::Max(static_cast<double>(CVarMinAltitude.GetValueOnGameThread()), 0.5) * 100.0;
	}

	double EdgeRadiusCm()
	{
		return FMath::Max(static_cast<double>(CVarEdgeLightYears.GetValueOnGameThread()), 0.0) * LightYearCm;
	}

	FVector AnyPerpendicular(const FVector& Axis)
	{
		const FVector Reference = FMath::Abs(Axis.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Axis, Reference).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::RightVector);
	}

	FString BodyName(const AActor* BodyActor)
	{
		if (const ACelestialBody* Body = Cast<ACelestialBody>(BodyActor); Body && !Body->AstroName.IsNone())
		{
			return Body->AstroName.ToString().ToUpper();
		}
		FString Name = BodyActor ? BodyActor->GetName() : FString();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.ReplaceInline(TEXT("_C_"), TEXT("_"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return Name.ToUpper();
	}

	FString GroupThousands(const double Value)
	{
		FString Digits = FString::Printf(TEXT("%.0f"), FMath::RoundToDouble(FMath::Max(Value, 0.0)));
		for (int32 At = Digits.Len() - 3; At > 0; At -= 3)
		{
			Digits.InsertAt(At, TEXT(' '));
		}
		return Digits;
	}

	FString FormatSpeed(const double CmPerSecond)
	{
		const double Metres = CmPerSecond / 100.0;
		if (Metres < 10.0) return FString::Printf(TEXT("%.1f m/s"), Metres);
		if (Metres < 1000.0) return FString::Printf(TEXT("%.0f m/s"), Metres);
		const double Kilometres = Metres / 1000.0;
		if (Kilometres < 10.0) return FString::Printf(TEXT("%.2f km/s"), Kilometres);
		if (Kilometres < 1.0e6) return FString::Printf(TEXT("%s km/s"), *GroupThousands(Kilometres));
		const double Au = CmPerSecond / AstronomicalUnitCm;
		if (Au < 10.0) return FString::Printf(TEXT("%.3f AU/s"), Au);
		if (Au < 5000.0) return FString::Printf(TEXT("%.1f AU/s"), Au);
		const double LightYears = CmPerSecond / LightYearCm;
		if (LightYears < 10.0) return FString::Printf(TEXT("%.3f ly/s"), LightYears);
		return FString::Printf(TEXT("%.1f ly/s"), LightYears);
	}

	FString FormatDistance(const double Cm)
	{
		const double Metres = Cm / 100.0;
		if (Metres < 1000.0) return FString::Printf(TEXT("%.0f m"), Metres);
		const double Kilometres = Metres / 1000.0;
		if (Kilometres < 10.0) return FString::Printf(TEXT("%.2f km"), Kilometres);
		if (Kilometres < 1.0e7) return FString::Printf(TEXT("%s km"), *GroupThousands(Kilometres));
		const double Au = Cm / AstronomicalUnitCm;
		if (Au < 10.0) return FString::Printf(TEXT("%.3f AU"), Au);
		if (Au < 5000.0) return FString::Printf(TEXT("%.1f AU"), Au);
		return FString::Printf(TEXT("%.3f ly"), Cm / LightYearCm);
	}

	FString FormatTier(const double Multiplier)
	{
		// The multiplication sign (U+00D7), from its code point so the source stays ASCII.
		FString Label;
		Label.AppendChar(static_cast<TCHAR>(0x00D7));
		Label += Multiplier < 1.0 ? FString::Printf(TEXT("%.1f"), Multiplier) : FString::Printf(TEXT("%.0f"), Multiplier);
		return Label;
	}

	bool HasAnyInput(const FVector& Move, const FVector2D& Look, const double Roll, const int32 TierSteps, const bool bFlyKey)
	{
		return !Move.IsNearlyZero() || !Look.IsNearlyZero(1.0e-4) || Roll != 0.0 || TierSteps != 0 || bFlyKey;
	}

	/**
	 * First fraction 0..1 of Step at which Start + fraction x Step enters the sphere from outside; false when it does
	 * not (or starts inside). Measured from the closest approach, so light-year steps keep centimetre precision.
	 */
	bool SegmentEntersSphere(const FVector& Start, const FVector& Step, const FVector& Centre, const double Radius,
		double& OutFraction)
	{
		const double LengthSquared = Step.SizeSquared();
		const FVector FromCentre = Start - Centre;
		const double RadiusSquared = Radius * Radius;
		if (LengthSquared <= 0.0 || FromCentre.SizeSquared() <= RadiusSquared)
		{
			return false;
		}
		const double Closest = -FVector::DotProduct(FromCentre, Step) / LengthSquared;
		if (Closest <= 0.0)
		{
			return false;
		}
		const double MissSquared = (FromCentre + Step * Closest).SizeSquared();
		if (MissSquared >= RadiusSquared)
		{
			return false;
		}
		const double Entry = Closest - FMath::Sqrt(RadiusSquared - MissSquared) / FMath::Sqrt(LengthSquared);
		if (Entry < 0.0 || Entry > 1.0)
		{
			return false;
		}
		OutFraction = Entry;
		return true;
	}

	/** Terrain (or sea) radius under Location from WorldScape's height noise, as the ships measure their clearance. */
	bool SampleTerrainRadius(const APlanetaryBody& Body, const FVector& Location, double& OutRadiusCm)
	{
		APlanetarySurfaceGenerator* Surface = Body.PlanetaryEnvironmentGenerator;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0f
			|| !Surface->IsSurfaceProfileCurrent(&Body))
		{
			return false;
		}
		const FVector Centre = Root->GetActorLocation();
		const FVector Offset = Location - Centre;
		const double Distance = Offset.Size();
		if (Distance <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}
		const double PlanetScale = Root->PlanetScale;
		const FVector Direction = Offset / Distance;
		double Radius = PlanetScale + Root->GetGroundHeight(Centre + Direction * PlanetScale, false);
		if (Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None)
		{
			Radius = FMath::Max(Radius, PlanetScale
				+ static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity);
		}
		if (!FMath::IsFinite(Radius) || Radius <= 0.0)
		{
			return false;
		}
		OutRadiusCm = Radius;
		return true;
	}
}

AAPSSpectatorPawn::AAPSSpectatorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	bCollideWhenPlacing = false;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BaseEyeHeight = 0.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SceneRoot);
	Camera->bUsePawnControlRotation = false;
}

bool AAPSSpectatorPawn::IsGeneratedSpaceRoute(const UWorld* World)
{
	if (!World || APSFreeFlightLocal::CVarEnabled.GetValueOnGameThread() == 0)
	{
		return false;
	}
	const UGameInstance* GameInstance = World->GetGameInstance();
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const UGeneratedWorld* Model = Gameplay ? Gameplay->NewGeneratedWorld : nullptr;
	// WorldGenerationViewModel::CommitAndOpenLevel: Generate Space and Create Planet commit the model without a
	// civilization, and Create Planet always as a single planet; a save replays the same flags (MainMenuController).
	return IsValid(Model) && !Gameplay->bUseAuthoredSinglePlayWorld && !Gameplay->bSpawnGeneratedCivilization
		&& Model->AstroGenerationLevel != EAstroGenerationLevel::SinglePlanet;
}

void AAPSSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();
	using namespace APSFreeFlightLocal;
	ViewQuat = GetActorQuat();
	ReferenceUp = FVector::UpVector;
	ApplyLook(0.0, 0.0);
	TierIndex = DefaultTier;
	LogMultiplier = FMath::Loge(Tiers[DefaultTier]);
	SpawnSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const UGameInstance* GameInstance = GetGameInstance();
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	// A load restores the saved camera (AGravityPlayerController::LoadWorld); only a new world opens on the home planet.
	const bool bLoading = Gameplay && (Gameplay->bIsLoadingMode || Gameplay->bPendingSavedWorldReplay);
	bStartViewPending = !bLoading && CVarStartView.GetValueOnGameThread() != 0;
	ApplyViewTransform();
	if (IsLocallyControlled())
	{
		CreateHud();
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] free-flight camera at %s (%s)"), *GetActorLocation().ToCompactString(),
		bLoading ? TEXT("load") : TEXT("new world"));
}

void AAPSSpectatorPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveHud();
	Super::EndPlay(EndPlayReason);
}

void AAPSSpectatorPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (HasActorBegunPlay() && IsLocallyControlled())
	{
		CreateHud();
	}
}

void AAPSSpectatorPawn::UnPossessed()
{
	RemoveHud();
	Super::UnPossessed();
}

FVector AAPSSpectatorPawn::GetVelocity() const
{
	// APawn reads a movement component this pawn does not have: the arrival curtain, the streaming forecast and the
	// floating origin see the camera's real motion here.
	return FlightVelocity;
}

void AAPSSpectatorPawn::ApplyWorldOffset(const FVector& InOffset, const bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);
	// The floating origin moved the world (and this camera with it): not a move from outside.
	AppliedLocation += InOffset;
}

void AAPSSpectatorPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace APSFreeFlightLocal;
	// A hitch moves the camera at most a tenth of a second's way.
	const double FrameSeconds = FMath::Min(static_cast<double>(DeltaSeconds), 0.1);
	if (!GetWorld() || FrameSeconds <= 0.0)
	{
		return;
	}
	AdoptExternalTransform();
	BodyRefreshElapsed += FrameSeconds;
	if (BodyRefreshElapsed >= BodyRefreshSeconds)
	{
		BodyRefreshElapsed = 0.0;
		RefreshBodies();
	}
	if (bStartViewPending)
	{
		TryStartView();
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const bool bBlocked = !PlayerController || IsFlightInputBlocked(*PlayerController);
	FFlightInput Input;
	if (!bBlocked)
	{
		ReadFlightInput(*PlayerController, Input);
	}
	bAnyInput = bAnyInput || HasAnyInput(Input.Move, Input.LookDegrees, Input.Roll, Input.TierSteps, Input.bToggleFlyTo);

	MeasureSurroundings(FrameSeconds);
	UpdateSpeedScale(Input, FrameSeconds);
	if (Input.bToggleFlyTo)
	{
		if (bFlyTo)
		{
			EndFlyTo(TEXT("F again"), true);
		}
		else
		{
			BeginFlyTo();
		}
	}
	if (bFlyTo && (bBlocked || !Input.Move.IsNearlyZero()))
	{
		EndFlyTo(bBlocked ? TEXT("a screen took the input") : TEXT("movement keys"), true);
	}
	if (bFlyTo && !Input.LookDegrees.IsNearlyZero(0.05))
	{
		// The mouse takes the view back; the flight goes on.
		bFlyToLook = false;
	}
	UpdateOrientation(Input, FrameSeconds);

	const FVector Wanted = bFlyTo ? StepFlyTo(FrameSeconds) : StepFromInput(Input, FrameSeconds);
	const double Flown = MoveSafely(Wanted, FrameSeconds);
	if (bFlyTo)
	{
		FlyToBlockedSeconds = Flown < 0.5 ? FlyToBlockedSeconds + FrameSeconds : 0.0;
		if (FlyToBlockedSeconds > 0.75)
		{
			EndFlyTo(TEXT("a body is in the way"), true);
		}
	}
	ApplyViewTransform();
	UpdateHudText();

	if (CVarLog.GetValueOnGameThread() != 0)
	{
		LogElapsed += FrameSeconds;
		if (LogElapsed >= 1.0)
		{
			LogElapsed = 0.0;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.FreeFlight] speed %s, full %s, tier %s, nearest %s %s, stop gap %s, %.0f km from 0,0,0%s"),
				*FormatSpeed(FlightVelocity.Size()), *FormatSpeed(CurrentSpeedCm), *FormatTier(Tiers[TierIndex]),
				*NearestName, *FormatDistance(FMath::Max(NearestAltitudeCm, 0.0)),
				bHasStopSurface ? *FormatDistance(FMath::Max(StopGapCm, 0.0)) : TEXT("none"),
				GetActorLocation().Size() / 1.0e5, bFlyTo ? TEXT(", flying to a body") : TEXT(""));
		}
	}
}

bool AAPSSpectatorPawn::IsFlightInputBlocked(const APlayerController& PlayerController) const
{
	const UWorld* LiveWorld = GetWorld();
	if (APSArrivalCurtain::IsUp(LiveWorld))
	{
		return true;
	}
	// The strategic map and the colony terminal hold the input with their own widgets (as for the walker).
	if (const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(&PlayerController);
		Gravity && Gravity->IsStrategicMapOpen())
	{
		return true;
	}
	if (const UAPSColonyTerminalSubsystem* Terminal = LiveWorld ? LiveWorld->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		Terminal && Terminal->IsTerminalOpen())
	{
		return true;
	}
	// The pause menu and other screens show the cursor.
	return PlayerController.ShouldShowMouseCursor();
}

void AAPSSpectatorPawn::ReadFlightInput(const APlayerController& PlayerController, FFlightInput& OutInput) const
{
	using namespace APSFreeFlightLocal;
	const auto Down = [&PlayerController](const FKey& Key) { return PlayerController.IsInputKeyDown(Key); };
	const auto Axis = [&Down](const FKey& Plus, const FKey& Minus) { return (Down(Plus) ? 1.0 : 0.0) - (Down(Minus) ? 1.0 : 0.0); };
	OutInput.Move.X = Axis(EKeys::W, EKeys::S);
	OutInput.Move.Y = Axis(EKeys::D, EKeys::A);
	const bool bDown = Down(EKeys::LeftControl) || Down(EKeys::RightControl) || Down(EKeys::C);
	OutInput.Move.Z = (Down(EKeys::SpaceBar) ? 1.0 : 0.0) - (bDown ? 1.0 : 0.0);
	OutInput.Roll = Axis(EKeys::E, EKeys::Q);

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	PlayerController.GetInputMouseDelta(MouseX, MouseY);
	const double LookScale = MouseDegreesPerUnit * FMath::Max(static_cast<double>(CVarMouseSensitivity.GetValueOnGameThread()), 0.0);
	OutInput.LookDegrees = FVector2D(MouseX * LookScale,
		MouseY * LookScale * (CVarInvertY.GetValueOnGameThread() != 0 ? -1.0 : 1.0));

	double Wheel = PlayerController.GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (FMath::IsNearlyZero(Wheel))
	{
		Wheel = (PlayerController.WasInputKeyJustPressed(EKeys::MouseScrollUp) ? 1.0 : 0.0)
			- (PlayerController.WasInputKeyJustPressed(EKeys::MouseScrollDown) ? 1.0 : 0.0);
	}
	int32 Steps = FMath::Clamp(FMath::RoundToInt32(Wheel), -3, 3);
	if (PlayerController.WasInputKeyJustPressed(EKeys::Equals) || PlayerController.WasInputKeyJustPressed(EKeys::Add))
	{
		++Steps;
	}
	if (PlayerController.WasInputKeyJustPressed(EKeys::Hyphen) || PlayerController.WasInputKeyJustPressed(EKeys::Subtract))
	{
		--Steps;
	}
	OutInput.TierSteps = Steps;
	OutInput.bBoost = Down(EKeys::LeftShift) || Down(EKeys::RightShift);
	OutInput.bSlow = Down(EKeys::LeftAlt) || Down(EKeys::RightAlt);
	OutInput.bToggleFlyTo = PlayerController.WasInputKeyJustPressed(EKeys::F);
}

void AAPSSpectatorPawn::RefreshBodies()
{
	using namespace APSFreeFlightLocal;
	FlightBodies.Reset();
	UWorld* LiveWorld = GetWorld();
	if (!LiveWorld)
	{
		return;
	}
	const double StopCm = MinAltitudeCm();
	for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
	{
		APlanetaryBody* Body = *It;
		const double Radius = IsValid(Body) ? Body->GetWorldScapeBodyRadiusCm() : 0.0;
		if (Radius <= 0.0 || !FMath::IsFinite(Radius))
		{
			continue;
		}
		FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
		Entry.Actor = Body;
		Entry.RadiusCm = Radius;
		Entry.StopAltitudeCm = StopCm;
		Entry.Name = BodyName(Body);
		Entry.bPlanetary = true;
	}
	for (TActorIterator<AStar> It(LiveWorld); It; ++It)
	{
		AStar* Star = *It;
		const double Radius = IsValid(Star) ? FMath::Max(static_cast<double>(Star->StarRadiusKM), Star->RadiusKM) * 1.0e5 : 0.0;
		if (Radius <= 0.0 || !FMath::IsFinite(Radius))
		{
			continue;
		}
		FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
		Entry.Actor = Star;
		Entry.RadiusCm = Radius;
		Entry.StopAltitudeCm = FMath::Max(StopCm, Radius * 0.02);
		Entry.Name = BodyName(Star);
		Entry.bStar = true;
	}
	for (TActorIterator<ASpaceStation> It(LiveWorld); It; ++It)
	{
		ASpaceStation* Station = *It;
		if (!IsValid(Station))
		{
			continue;
		}
		// The hull's visible meshes, as the ships measure a station (volumes and far-off parts left out).
		FBox Hull(ForceInit);
		const FVector StationLocation = Station->GetActorLocation();
		Station->ForEachComponent<UMeshComponent>(false, [&Hull, &StationLocation](const UMeshComponent* Mesh)
		{
			if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0
				&& FVector::Dist(Mesh->Bounds.Origin, StationLocation) < 10000000.0)
			{
				Hull += Mesh->Bounds.GetBox();
			}
		});
		if (!Hull.IsValid)
		{
			continue;
		}
		FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
		Entry.Actor = Station;
		Entry.LocalCentre = Station->GetActorTransform().InverseTransformPosition(Hull.GetCenter());
		Entry.RadiusCm = FMath::Min(Hull.GetExtent().Size(), 1000000.0);
		Entry.Name = BodyName(Station);
		Entry.bSolid = false;
		Entry.bStation = true;
	}
	// The cluster's other stars are catalogue points the stellar view draws; the nearest few count as bodies.
	if (const FAPSStarSystems* Systems = APSStarSystemsFind(LiveWorld); Systems && Systems->IsReady())
	{
		TArray<int32> Nearest;
		Systems->FindNearest(GetActorLocation(), CatalogueNeighbours, Nearest);
		for (const int32 Index : Nearest)
		{
			const FAPSStarSystemInfo* Info = Systems->Get(Index);
			if (!Info || Info->bHome || Info->bInsideHome)
			{
				continue;
			}
			FFlightBody& Entry = FlightBodies.AddDefaulted_GetRef();
			Entry.CatalogueIndex = Index;
			Entry.RadiusCm = SolarRadiusCm;
			Entry.StopAltitudeCm = SolarRadiusCm * 0.02;
			Entry.Name = Info->Name.ToUpper();
			Entry.bStar = true;
		}
	}
	// A flight to a catalogue star turns into one to its real star once the system stands (APSSystemMaterializer).
	if (bFlyTo && FlyToCatalogue != INDEX_NONE)
	{
		const FAPSStarSystems* Systems = APSStarSystemsFind(LiveWorld);
		const FAPSStarSystemInfo* Info = Systems ? Systems->Get(FlyToCatalogue) : nullptr;
		const FFlightBody* Found = nullptr;
		double FoundDistance = TNumericLimits<double>::Max();
		for (const FFlightBody& Body : FlightBodies)
		{
			const AActor* BodyActor = Body.Actor.Get();
			if (!Info || !Body.bStar || !BodyActor)
			{
				continue;
			}
			const double FromRecord = FVector::Distance(BodyActor->GetActorLocation(), Info->Location);
			if (FromRecord < FMath::Max(Info->RoomCm * 0.2, Body.RadiusCm * 2.0) && FromRecord < FoundDistance)
			{
				FoundDistance = FromRecord;
				Found = &Body;
			}
		}
		if (Found)
		{
			FlyToActor = Found->Actor;
			FlyToCatalogue = INDEX_NONE;
			FlyToName = Found->Name;
			AimFlyTo(Found->Actor->GetActorLocation(), Found->RadiusCm * StarStandoffRadii);
			UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] the system stands: flying on to its star %s"), *FlyToName);
		}
	}
}

bool AAPSSpectatorPawn::GetBodyCentre(const FFlightBody& Body, FVector& OutCentre) const
{
	if (Body.CatalogueIndex != INDEX_NONE)
	{
		const FAPSStarSystems* Systems = APSStarSystemsFind(GetWorld());
		const FAPSStarSystemInfo* Info = Systems ? Systems->Get(Body.CatalogueIndex) : nullptr;
		if (!Info)
		{
			return false;
		}
		OutCentre = Info->Location;
		return true;
	}
	const AActor* BodyActor = Body.Actor.Get();
	if (!BodyActor)
	{
		return false;
	}
	OutCentre = Body.bStation ? BodyActor->GetActorTransform().TransformPosition(Body.LocalCentre)
		: BodyActor->GetActorLocation();
	return true;
}

double AAPSSpectatorPawn::GetSurfaceRadius(const FFlightBody& Body) const
{
	const AActor* Terrain = TerrainBody.Get();
	return Body.bPlanetary && Terrain && Terrain == Body.Actor.Get() ? TerrainRadiusCm : Body.RadiusCm;
}

void AAPSSpectatorPawn::MeasureSurroundings(const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	const FVector Location = GetActorLocation();

	// The terrain under the camera: the planet or moon whose sphere is nearest, from WorldScape's height noise (it builds
	// collision only close to the ground), as the ships' MeasureTerrainClearanceCm does.
	APlanetaryBody* TerrainCandidate = nullptr;
	double CandidateAltitude = TNumericLimits<double>::Max();
	for (const FFlightBody& Body : FlightBodies)
	{
		FVector Centre;
		if (Body.bPlanetary && GetBodyCentre(Body, Centre))
		{
			const double SphereAltitude = FVector::Distance(Location, Centre) - Body.RadiusCm;
			if (SphereAltitude < CandidateAltitude)
			{
				CandidateAltitude = SphereAltitude;
				TerrainCandidate = Cast<APlanetaryBody>(Body.Actor.Get());
			}
		}
	}
	if (!TerrainCandidate || CandidateAltitude > TerrainQueryAltitudeCm)
	{
		TerrainBody.Reset();
	}
	else
	{
		TerrainSampleElapsed += DeltaSeconds;
		if (TerrainBody.Get() != TerrainCandidate || TerrainSampleElapsed >= TerrainSampleSeconds
			|| CandidateAltitude < TerrainEveryFrameAltitudeCm)
		{
			TerrainSampleElapsed = 0.0;
			double Radius = 0.0;
			if (SampleTerrainRadius(*TerrainCandidate, Location, Radius))
			{
				TerrainBody = TerrainCandidate;
				TerrainRadiusCm = Radius;
			}
			else
			{
				TerrainBody.Reset();
			}
		}
	}

	const double StopCm = MinAltitudeCm();
	double SpeedDistance = TNumericLimits<double>::Max();
	double LevelAltitude = TNumericLimits<double>::Max();
	bHasNearest = false;
	NearestName.Reset();
	bHasStopSurface = false;
	StopOutward = FVector::ZeroVector;
	LevelWeight = 0.0;
	LevelUp = FVector::ZeroVector;
	for (const FFlightBody& Body : FlightBodies)
	{
		FVector Centre;
		if (!GetBodyCentre(Body, Centre))
		{
			continue;
		}
		const FVector Offset = Location - Centre;
		const double Distance = Offset.Size();
		const FVector Outward = Distance > UE_DOUBLE_SMALL_NUMBER ? Offset / Distance : FVector::UpVector;
		const double Altitude = Distance - GetSurfaceRadius(Body);
		SpeedDistance = FMath::Min(SpeedDistance, FMath::Max(Altitude, 0.0));
		if (!bHasNearest || Altitude < NearestAltitudeCm)
		{
			bHasNearest = true;
			NearestAltitudeCm = Altitude;
			NearestName = Body.Name;
		}
		if (Body.bSolid)
		{
			const double Gap = Altitude - FMath::Max(Body.StopAltitudeCm, StopCm);
			if (!bHasStopSurface || Gap < StopGapCm)
			{
				bHasStopSurface = true;
				StopGapCm = Gap;
				StopOutward = Outward;
			}
		}
		if (Body.bPlanetary && Altitude < LevelAltitude)
		{
			LevelAltitude = Altitude;
			// Horizon assist: full in the lowest few percent of the radius, none above a third of it.
			LevelWeight = 1.0 - FMath::SmoothStep(0.02 * Body.RadiusCm, 0.35 * Body.RadiusCm, Altitude);
			LevelUp = Outward;
		}
	}
	// The flight range is a surface too: the renderer loses precision ~42 ly out (the ships' edge of charted space).
	if (const double EdgeCm = EdgeRadiusCm(); EdgeCm > 0.0)
	{
		const double FromOrigin = Location.Size();
		const double Gap = EdgeCm - FromOrigin;
		if (Gap > 0.0 && FromOrigin > 1.0)
		{
			SpeedDistance = FMath::Min(SpeedDistance, Gap);
			if (!bHasStopSurface || Gap < StopGapCm)
			{
				bHasStopSurface = true;
				StopGapCm = Gap;
				StopOutward = -Location / FromOrigin;
			}
			if (!bHasNearest || Gap < NearestAltitudeCm)
			{
				bHasNearest = true;
				NearestAltitudeCm = Gap;
				NearestName = EdgeName;
			}
		}
	}
	SpeedDistanceCm = SpeedDistance < TNumericLimits<double>::Max() ? SpeedDistance : 1.0e8;
}

void AAPSSpectatorPawn::UpdateSpeedScale(const FFlightInput& Input, const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	// Rio 03.10: the distance to the nearest surface sets the speed (fast far out, slow near a surface). It eases in log
	// space: a body coming into range or terrain streaming in never snaps it, and a nearer surface is followed quickly.
	const double Factor = FMath::Max(static_cast<double>(CVarSpeedFactor.GetValueOnGameThread()), 0.01);
	const double WantedAuto = FMath::Loge(FMath::Max(Factor * SpeedDistanceCm, MinimumSpeedCm));
	if (!bAutoSpeedReady)
	{
		LogAutoSpeed = WantedAuto;
		bAutoSpeedReady = true;
	}
	else
	{
		LogAutoSpeed += (WantedAuto - LogAutoSpeed)
			* EaseAlpha(DeltaSeconds, WantedAuto < LogAutoSpeed ? AutoFallSeconds : AutoRiseSeconds);
	}
	// The tier (wheel), boost (Shift) and slow (Alt) ease over about a third of a second.
	TierIndex = FMath::Clamp(TierIndex + Input.TierSteps, 0, TierCount - 1);
	bBoostHeld = Input.bBoost;
	bSlowHeld = Input.bSlow && !Input.bBoost;
	const double WantedMultiplier = Tiers[TierIndex] * (bBoostHeld ? BoostMultiplier : 1.0) * (bSlowHeld ? SlowMultiplier : 1.0);
	LogMultiplier += (FMath::Loge(WantedMultiplier) - LogMultiplier) * EaseAlpha(DeltaSeconds, TierEaseSeconds);
	CurrentSpeedCm = FMath::Exp(LogAutoSpeed + LogMultiplier);
}

void AAPSSpectatorPawn::UpdateOrientation(const FFlightInput& Input, const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	// Q/E roll the reference up about the view (the horizon tilts and stays tilted out in space).
	RollRateDegrees += (Input.Roll * RollDegreesPerSecond - RollRateDegrees) * EaseAlpha(DeltaSeconds, RollEaseSeconds);
	LevelHoldSeconds = Input.Roll != 0.0 ? LevelHoldAfterRollSeconds : FMath::Max(LevelHoldSeconds - DeltaSeconds, 0.0);
	if (FMath::Abs(RollRateDegrees) > 1.0e-3)
	{
		const double Angle = FMath::DegreesToRadians(RollRateDegrees * DeltaSeconds);
		const FVector Forward = ViewQuat.GetForwardVector();
		const FVector CameraUp = ViewQuat.GetUpVector();
		const FVector CameraRight = ViewQuat.GetRightVector();
		// Rolling right (E) tips the up toward the camera's right.
		ReferenceUp = (Forward * FVector::DotProduct(ReferenceUp, Forward)
			+ (CameraUp * FMath::Cos(Angle) + CameraRight * FMath::Sin(Angle)) * FVector::DotProduct(ReferenceUp, CameraUp))
			.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, CameraUp);
	}
	// Low over a planet or moon the horizon levels itself to the ground below.
	if (LevelHoldSeconds <= 0.0 && LevelWeight > 0.0 && !LevelUp.IsNearlyZero())
	{
		const FQuat Turn = FQuat::FindBetweenNormals(ReferenceUp, LevelUp);
		ReferenceUp = FQuat::Slerp(FQuat::Identity, Turn, 1.0 - FMath::Exp(-2.0 * LevelWeight * DeltaSeconds))
			.RotateVector(ReferenceUp).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, LevelUp);
	}
	// Mouse look with light smoothing: every count is applied, a few frames later at most.
	PendingLookDegrees += Input.LookDegrees;
	const double LookAlpha = EaseAlpha(DeltaSeconds, CVarLookSmoothing.GetValueOnGameThread());
	double Yaw = PendingLookDegrees.X * LookAlpha;
	double Pitch = PendingLookDegrees.Y * LookAlpha;
	PendingLookDegrees -= FVector2D(Yaw, Pitch);
	// F turns the view to the target until the mouse takes it back.
	FVector Centre;
	if (bFlyTo && bFlyToLook && GetFlyToCentre(Centre))
	{
		double TurnYaw = 0.0;
		double TurnPitch = 0.0;
		GetLookDeltaTo((Centre - GetActorLocation()).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, ViewQuat.GetForwardVector()),
			TurnYaw, TurnPitch);
		const double TurnAlpha = EaseAlpha(DeltaSeconds, 0.35);
		Yaw += TurnYaw * TurnAlpha;
		Pitch += TurnPitch * TurnAlpha;
	}
	ApplyLook(Yaw, Pitch);
}

void AAPSSpectatorPawn::ApplyLook(const double YawDegrees, const double PitchDegrees)
{
	using namespace APSFreeFlightLocal;
	const FVector Up = ReferenceUp.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
	const FVector Forward = ViewQuat.GetForwardVector();
	const double SinPitch = FMath::Clamp(FVector::DotProduct(Forward, Up), -1.0, 1.0);
	FVector Heading = Forward - Up * SinPitch;
	if (Heading.SizeSquared() < 1.0e-10)
	{
		// Straight along the up: the camera's own up shows the heading it pitched from.
		Heading = ViewQuat.GetUpVector() * -SinPitch;
		Heading -= Up * FVector::DotProduct(Heading, Up);
	}
	Heading = Heading.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, AnyPerpendicular(Up));
	if (YawDegrees != 0.0)
	{
		Heading = FQuat(Up, FMath::DegreesToRadians(YawDegrees)).RotateVector(Heading);
	}
	const double Pitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Asin(SinPitch)) + PitchDegrees,
		-MaxPitchDegrees, MaxPitchDegrees);
	const double PitchRadians = FMath::DegreesToRadians(Pitch);
	const FVector NewForward = (Heading * FMath::Cos(PitchRadians) + Up * FMath::Sin(PitchRadians)).GetSafeNormal(
		UE_DOUBLE_SMALL_NUMBER, Heading);
	ViewQuat = FRotationMatrix::MakeFromXZ(NewForward, Up).ToQuat();
	ReferenceUp = Up;
}

void AAPSSpectatorPawn::GetLookDeltaTo(const FVector& Direction, double& OutYawDegrees, double& OutPitchDegrees) const
{
	const FVector Up = ReferenceUp;
	const FVector Forward = ViewQuat.GetForwardVector();
	const double SinNow = FMath::Clamp(FVector::DotProduct(Forward, Up), -1.0, 1.0);
	const double SinWanted = FMath::Clamp(FVector::DotProduct(Direction, Up), -1.0, 1.0);
	const FVector HeadingNow = (Forward - Up * SinNow).GetSafeNormal();
	const FVector HeadingWanted = (Direction - Up * SinWanted).GetSafeNormal();
	OutYawDegrees = 0.0;
	if (!HeadingNow.IsNearlyZero() && !HeadingWanted.IsNearlyZero())
	{
		OutYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(
			FVector::DotProduct(FVector::CrossProduct(HeadingNow, HeadingWanted), Up),
			FVector::DotProduct(HeadingNow, HeadingWanted)));
	}
	OutPitchDegrees = FMath::RadiansToDegrees(FMath::Asin(SinWanted) - FMath::Asin(SinNow));
}

FVector AAPSSpectatorPawn::StepFromInput(const FFlightInput& Input, const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	// The keys drive a critically damped spring (camera frame): the speed ramps up and down without a jolt, and a
	// changed speed scale (tier, boost, distance) applies at once, already eased.
	FVector Target = Input.Move;
	if (Target.SizeSquared() > 1.0)
	{
		Target.Normalize();
	}
	const double Response = FMath::Max(static_cast<double>(CVarResponse.GetValueOnGameThread()), 0.5);
	const FVector Offset = ThrottleLocal - Target;
	const FVector Drive = ThrottleRate + Offset * Response;
	const double Decay = FMath::Exp(-Response * DeltaSeconds);
	ThrottleLocal = Target + (Offset + Drive * DeltaSeconds) * Decay;
	ThrottleRate = (ThrottleRate - Drive * (Response * DeltaSeconds)) * Decay;
	if (Target.IsNearlyZero() && ThrottleLocal.SizeSquared() < 1.0e-8 && ThrottleRate.SizeSquared() < 1.0e-8)
	{
		ThrottleLocal = FVector::ZeroVector;
		ThrottleRate = FVector::ZeroVector;
	}
	return ViewQuat.RotateVector(ThrottleLocal) * (CurrentSpeedCm * DeltaSeconds);
}

bool AAPSSpectatorPawn::BeginFlyTo()
{
	using namespace APSFreeFlightLocal;
	const FVector Location = GetActorLocation();
	const FVector Forward = ViewQuat.GetForwardVector();
	const FAPSStarSystems* Systems = APSStarSystemsFind(GetWorld());

	// The body under the crosshair (the nearest of those), else the one closest to it, else the nearest body.
	int32 OnDisc = INDEX_NONE;
	double OnDiscDistance = TNumericLimits<double>::Max();
	int32 Aimed = INDEX_NONE;
	double AimedMiss = AimToleranceRadians;
	int32 Nearest = INDEX_NONE;
	double NearestAltitude = TNumericLimits<double>::Max();
	const auto Consider = [&](const int32 Candidate, const FVector& Centre, const double Radius, const bool bCanBeNearest)
	{
		const FVector To = Centre - Location;
		const double Distance = To.Size();
		if (Distance <= Radius * 1.001)
		{
			return;
		}
		const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, To / Distance), -1.0, 1.0));
		const double Disc = FMath::Asin(FMath::Min(Radius / Distance, 1.0));
		if (Angle <= Disc)
		{
			if (Distance < OnDiscDistance)
			{
				OnDiscDistance = Distance;
				OnDisc = Candidate;
			}
		}
		else if (Angle - Disc < AimedMiss)
		{
			AimedMiss = Angle - Disc;
			Aimed = Candidate;
		}
		if (bCanBeNearest && Distance - Radius < NearestAltitude)
		{
			NearestAltitude = Distance - Radius;
			Nearest = Candidate;
		}
	};
	for (int32 Index = 0; Index < FlightBodies.Num(); ++Index)
	{
		FVector Centre;
		if (GetBodyCentre(FlightBodies[Index], Centre))
		{
			Consider(Index, Centre, GetSurfaceRadius(FlightBodies[Index]), true);
		}
	}
	// Any star of the cluster the crosshair is on, not only the nearest few (one pass, on the key press).
	const int32 CatalogueBase = FlightBodies.Num();
	if (Systems && Systems->IsReady())
	{
		for (int32 Index = 0; Index < Systems->Num(); ++Index)
		{
			const FAPSStarSystemInfo* Info = Systems->Get(Index);
			if (Info && !Info->bHome && !Info->bInsideHome)
			{
				Consider(CatalogueBase + Index, Info->Location, SolarRadiusCm, false);
			}
		}
	}
	const int32 Chosen = OnDisc != INDEX_NONE ? OnDisc : Aimed != INDEX_NONE ? Aimed : Nearest;
	if (Chosen == INDEX_NONE)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] F: no body to fly to"));
		return false;
	}

	FVector Centre = FVector::ZeroVector;
	double Standoff = 0.0;
	FlyToActor.Reset();
	FlyToCatalogue = INDEX_NONE;
	if (Chosen < CatalogueBase)
	{
		const FFlightBody& Body = FlightBodies[Chosen];
		GetBodyCentre(Body, Centre);
		FlyToActor = Body.Actor;
		FlyToCatalogue = Body.CatalogueIndex;
		FlyToName = Body.Name;
		Standoff = Body.bPlanetary ? Body.RadiusCm * PlanetStandoffRadii
			: Body.bStar ? Body.RadiusCm * StarStandoffRadii
			: FMath::Max(Body.RadiusCm * 4.0, StationStandoffCm);
	}
	else
	{
		const FAPSStarSystemInfo* Info = Systems ? Systems->Get(Chosen - CatalogueBase) : nullptr;
		if (!Info)
		{
			return false;
		}
		Centre = Info->Location;
		FlyToCatalogue = Chosen - CatalogueBase;
		FlyToName = Info->Name.ToUpper();
		Standoff = SolarRadiusCm * StarStandoffRadii;
	}
	bFlyTo = true;
	bFlyToLook = true;
	FlyToBlockedSeconds = 0.0;
	FlyToOffset = (Location - Centre).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, -Forward) * Standoff;
	AimFlyTo(Centre, Standoff);
	// The flight owns the motion now; the keys start from rest when it ends.
	ThrottleLocal = FVector::ZeroVector;
	ThrottleRate = FVector::ZeroVector;
	UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] F: flying to %s, %s to its standoff"), *FlyToName,
		*FormatDistance(FVector::Distance(Location, Centre + FlyToOffset)));
	return true;
}

void AAPSSpectatorPawn::AimFlyTo(const FVector& Centre, const double StandoffCm)
{
	// The way runs straight to the standoff point on the camera's side of the body. Its remaining length eases in log
	// space (a critically damped spring): a fast start, a long gentle arrival, the same few seconds for a moon and a star.
	const FVector Location = GetActorLocation();
	FlyToStandoffCm = StandoffCm;
	FlyToOffset = FlyToOffset.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, (Location - Centre).GetSafeNormal()) * StandoffCm;
	const FVector ToGoal = Centre + FlyToOffset - Location;
	const double Remaining = ToGoal.Size();
	FlyToDirection = Remaining > 1.0 ? ToGoal / Remaining : ViewQuat.GetForwardVector();
	FlyToEpsilonCm = FMath::Max(StandoffCm * 0.002, 100.0);
	FlyToLog = FMath::Loge(Remaining + FlyToEpsilonCm);
	// The motion already under way continues along the new way (no jolt when F is pressed in flight).
	FlyToLogRate = -FVector::DotProduct(FlightVelocity, FlyToDirection) / (Remaining + FlyToEpsilonCm);
}

bool AAPSSpectatorPawn::GetFlyToCentre(FVector& OutCentre) const
{
	if (FlyToCatalogue != INDEX_NONE && !FlyToActor.IsValid())
	{
		const FAPSStarSystems* Systems = APSStarSystemsFind(GetWorld());
		const FAPSStarSystemInfo* Info = Systems ? Systems->Get(FlyToCatalogue) : nullptr;
		if (!Info)
		{
			return false;
		}
		OutCentre = Info->Location;
		return true;
	}
	const AActor* Target = FlyToActor.Get();
	if (!Target)
	{
		return false;
	}
	OutCentre = Target->GetActorLocation();
	return true;
}

FVector AAPSSpectatorPawn::StepFlyTo(const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	FVector Centre;
	if (!GetFlyToCentre(Centre))
	{
		EndFlyTo(TEXT("the target is gone"), true);
		return FVector::ZeroVector;
	}
	const double Response = FMath::Max(static_cast<double>(CVarFlyToResponse.GetValueOnGameThread()), 0.1);
	const double Goal = FMath::Loge(FlyToEpsilonCm);
	const double Offset = FlyToLog - Goal;
	const double Drive = FlyToLogRate + Offset * Response;
	const double Decay = FMath::Exp(-Response * DeltaSeconds);
	FlyToLog = Goal + (Offset + Drive * DeltaSeconds) * Decay;
	FlyToLogRate = (FlyToLogRate - Drive * Response * DeltaSeconds) * Decay;
	const double Remaining = FMath::Max(FMath::Exp(FlyToLog) - FlyToEpsilonCm, 0.0);
	const FVector Wanted = Centre + FlyToOffset - FlyToDirection * Remaining;
	const FVector Step = Wanted - GetActorLocation();
	// There within half a percent of the standoff: the rest of the motion fades through the keys' spring.
	if (Remaining < FlyToStandoffCm * 0.005 && FMath::Abs(FlyToLogRate) < 0.5)
	{
		EndFlyTo(TEXT("arrived"), true);
	}
	return Step;
}

void AAPSSpectatorPawn::EndFlyTo(const TCHAR* Reason, const bool bKeepMotion)
{
	if (!bFlyTo)
	{
		return;
	}
	bFlyTo = false;
	bFlyToLook = false;
	ThrottleRate = FVector::ZeroVector;
	ThrottleLocal = FVector::ZeroVector;
	if (bKeepMotion && CurrentSpeedCm > 1.0)
	{
		// The keys' spring takes over the speed the flight had, so it ends without a jolt.
		ThrottleLocal = ViewQuat.UnrotateVector(FlightVelocity / CurrentSpeedCm).GetClampedToMaxSize(4.0);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] flight to %s ended: %s"), *FlyToName, Reason);
}

double AAPSSpectatorPawn::MoveSafely(FVector Step, const double DeltaSeconds)
{
	using namespace APSFreeFlightLocal;
	const FVector Start = GetActorLocation();
	const double Requested = Step.Size();
	if (bHasStopSurface && !StopOutward.IsNearlyZero())
	{
		const double Closing = -FVector::DotProduct(Step, StopOutward);
		if (StopGapCm < 0.0)
		{
			// Inside the stop sphere (terrain streamed in under the camera, a world appeared around it): out along its
			// normal over a fraction of a second, never further in.
			if (Closing > 0.0)
			{
				Step += StopOutward * Closing;
			}
			Step += StopOutward * (-StopGapCm * EaseAlpha(DeltaSeconds, 0.15));
		}
		else if (Closing > 0.25 * StopGapCm)
		{
			// The ships' guard (APSFlightBandModel::GuardClosingSpeed): a frame closes at most a quarter of the gap, so
			// the camera eases down to the minimum altitude and a hitch at any speed cannot carry it through.
			Step += StopOutward * (Closing - 0.25 * StopGapCm);
		}
	}
	// Bodies farther off: a long step (tier x1000, boost) never enters another stop sphere on its way.
	const double StopCm = MinAltitudeCm();
	double Reach = Step.Size();
	for (const FFlightBody& Body : FlightBodies)
	{
		FVector Centre;
		if (!Body.bSolid || Reach <= 0.0 || !GetBodyCentre(Body, Centre))
		{
			continue;
		}
		const double StopRadius = GetSurfaceRadius(Body) + FMath::Max(Body.StopAltitudeCm, StopCm);
		double Fraction = 1.0;
		if (FVector::Distance(Start, Centre) - StopRadius <= Reach
			&& SegmentEntersSphere(Start, Step, Centre, StopRadius, Fraction))
		{
			Step *= Fraction;
			Reach = Step.Size();
		}
	}
	// Never past the edge of the flight range.
	if (const double EdgeCm = EdgeRadiusCm(); EdgeCm > 0.0)
	{
		const FVector End = Start + Step;
		const double EndRadius = End.Size();
		const double StartRadius = Start.Size();
		if (EndRadius > EdgeCm && EndRadius > StartRadius)
		{
			if (StartRadius < EdgeCm)
			{
				Step = End * (EdgeCm / EndRadius) - Start;
			}
			else
			{
				const FVector Out = Start / StartRadius;
				Step -= Out * FMath::Max(FVector::DotProduct(Step, Out), 0.0);
			}
		}
	}
	if (Step.SizeSquared() > CameraJumpCm * CameraJumpCm)
	{
		// The shadow cache pans by an int32 page offset: panning off from this frame (APSRenderSafety).
		APSRenderSafety::MarkCameraJump(TEXT("free flight"));
	}
	if (!Step.IsNearlyZero(1.0e-3))
	{
		SetActorLocation(Start + Step, false, nullptr, ETeleportType::TeleportPhysics);
	}
	FlightVelocity = Step / DeltaSeconds;
	return Requested > 1.0e-3 ? Step.Size() / Requested : 1.0;
}

void AAPSSpectatorPawn::TryStartView()
{
	using namespace APSFreeFlightLocal;
	UWorld* LiveWorld = GetWorld();
	if (!LiveWorld)
	{
		return;
	}
	// Behind the loading curtain; without it, only in the first seconds and before any key or mouse move.
	const bool bCurtain = APSArrivalCurtain::IsUp(LiveWorld);
	if (!bCurtain && (bAnyInput || LiveWorld->GetTimeSeconds() - SpawnSeconds > 5.0))
	{
		bStartViewPending = false;
		UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] start view skipped: the camera starts where it spawned"));
		return;
	}
	// The live generator (never a menu preview one) knows the home planet and star.
	const AAstroGenerator* Generator = nullptr;
	for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && !It->ActorHasTag(TEXT("WorldGenerationPreview")) && !It->UsesContinuousPreviewFrame())
		{
			Generator = *It;
			break;
		}
	}
	AActor* Target = nullptr;
	double Radius = 0.0;
	double Distance = 0.0;
	if (Generator && IsValid(Generator->HomePlanet))
	{
		Target = Generator->HomePlanet;
		Radius = Generator->HomePlanet->GetWorldScapeBodyRadiusCm();
		Distance = Radius * 3.2;
	}
	else if (Generator && IsValid(Generator->HomeStar))
	{
		Target = Generator->HomeStar;
		Radius = FMath::Max(static_cast<double>(Generator->HomeStar->StarRadiusKM), Generator->HomeStar->RadiusKM) * 1.0e5;
		Distance = Radius * 12.0;
	}
	if (!Target || Radius <= 0.0 || !FMath::IsFinite(Distance))
	{
		return;
	}
	// Rio 03.10: open on the home world, sunlit three quarters (the star 40 degrees off the camera, a little above).
	const FVector Centre = Target->GetActorLocation();
	FVector ToStar = (GetActorLocation() - Centre).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
	if (const APlanet* Planet = Cast<APlanet>(Target); Planet && IsValid(Planet->ParentStar))
	{
		ToStar = (Planet->ParentStar->GetActorLocation() - Centre).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, ToStar);
	}
	const FVector Up = FVector::UpVector;
	FVector Side = FVector::CrossProduct(Up, ToStar).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = AnyPerpendicular(ToStar);
	}
	const FVector From = (ToStar * FMath::Cos(FMath::DegreesToRadians(40.0)) + Side * FMath::Sin(FMath::DegreesToRadians(40.0))
		+ Up * 0.22).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, ToStar);
	const FVector Where = Centre + From * Distance;
	ReferenceUp = Up;
	ViewQuat = FRotationMatrix::MakeFromXZ((Centre - Where).GetSafeNormal(), Up).ToQuat();
	ApplyLook(0.0, 0.0);
	APSRenderSafety::MarkCameraJump(TEXT("free flight start view"));
	SetActorLocationAndRotation(Where, ViewQuat, false, nullptr, ETeleportType::TeleportPhysics);
	ThrottleLocal = FVector::ZeroVector;
	ThrottleRate = FVector::ZeroVector;
	FlightVelocity = FVector::ZeroVector;
	PendingLookDegrees = FVector2D::ZeroVector;
	bAutoSpeedReady = false;
	BodyRefreshElapsed = BodyRefreshSeconds;
	bStartViewPending = false;
	UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] start view: %s from %.1f radii (%s from its centre)"), *BodyName(Target),
		Distance / Radius, *FormatDistance(Distance));
}

void AAPSSpectatorPawn::AdoptExternalTransform()
{
	const FVector Location = GetActorLocation();
	const FQuat Rotation = GetActorQuat();
	if (bHasAppliedTransform)
	{
		const bool bMoved = FVector::DistSquared(Location, AppliedLocation) > 1.0;
		const bool bTurned = Rotation.AngularDistance(AppliedRotation) > 1.0e-3;
		if (!bMoved && !bTurned)
		{
			return;
		}
		// A load (AGravityPlayerController::LoadWorld) or a test visit placed the camera: carry on from there, at rest.
		ViewQuat = Rotation;
		if (bTurned)
		{
			ReferenceUp = Rotation.GetUpVector();
		}
		ApplyLook(0.0, 0.0);
		ThrottleLocal = FVector::ZeroVector;
		ThrottleRate = FVector::ZeroVector;
		FlightVelocity = FVector::ZeroVector;
		PendingLookDegrees = FVector2D::ZeroVector;
		bAutoSpeedReady = false;
		BodyRefreshElapsed = APSFreeFlightLocal::BodyRefreshSeconds;
		bStartViewPending = false;
		EndFlyTo(TEXT("the camera was placed from outside"), false);
		UE_LOG(LogTemp, Log, TEXT("[APS.FreeFlight] placed from outside at %s"), *Location.ToCompactString());
	}
	AppliedLocation = Location;
	AppliedRotation = ViewQuat;
	bHasAppliedTransform = true;
}

void AAPSSpectatorPawn::ApplyViewTransform()
{
	if (!GetActorQuat().Equals(ViewQuat, 1.0e-9))
	{
		SetActorRotation(ViewQuat, ETeleportType::TeleportPhysics);
	}
	AppliedLocation = GetActorLocation();
	AppliedRotation = GetActorQuat();
	bHasAppliedTransform = true;
	// Whatever reads the control rotation (saves, the strategic map, the test visits) sees the view.
	if (AController* OwnController = GetController())
	{
		OwnController->SetControlRotation(ViewQuat.Rotator());
	}
}

void AAPSSpectatorPawn::UpdateHudText()
{
	using namespace APSFreeFlightLocal;
	FString Status = bFlyTo ? FString::Printf(TEXT("FLYING TO %s"), *FlyToName) : FString(TEXT("FREE FLIGHT"));
	Status += FString::Printf(TEXT("  |  %s  |  SPEED %s (%s)"), *FormatSpeed(FlightVelocity.Size()),
		*FormatTier(Tiers[TierIndex]), *FormatSpeed(CurrentSpeedCm));
	if (bBoostHeld)
	{
		Status += TEXT("  |  BOOST");
	}
	else if (bSlowHeld)
	{
		Status += TEXT("  |  SLOW");
	}
	HudStatus = FText::FromString(Status);
	HudNearest = bHasNearest
		? FText::FromString(FString::Printf(TEXT("NEAREST  %s  |  %s"), *NearestName,
			*FormatDistance(FMath::Max(NearestAltitudeCm, 0.0))))
		: FText::GetEmpty();
}

bool AAPSSpectatorPawn::IsHudShown() const
{
	const UWorld* LiveWorld = GetWorld();
	const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(GetController());
	const UAPSColonyTerminalSubsystem* Terminal = LiveWorld ? LiveWorld->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
	// The F10 map and the colony terminal leave gaps between their panels: the HUD steps aside under them (as the walker's).
	return GetController() && !APSArrivalCurtain::IsUp(LiveWorld) && !(Gravity && Gravity->IsStrategicMapOpen())
		&& !(Terminal && Terminal->IsTerminalOpen());
}

void AAPSSpectatorPawn::CreateHud()
{
	if (HudWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	const TWeakObjectPtr<AAPSSpectatorPawn> WeakThis(this);
	const FText Hint = FText::FromString(
		TEXT("WASD MOVE   SPACE / CTRL UP / DOWN   WHEEL SPEED   SHIFT BOOST   ALT SLOW   Q / E ROLL   F FLY TO"));
	// The walker's traversal HUD style: bottom left, a blurred dark card, cyan status, light hint.
	HudWidget =
		SNew(SOverlay)
		.Visibility_Lambda([WeakThis]()
		{
			const AAPSSpectatorPawn* Self = WeakThis.Get();
			return Self && Self->IsHudShown() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(28.0f, 0.0f, 0.0f, 28.0f)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(TOptional<int32>(8))
			.BlurStrength(6.0f)
			.bApplyAlphaToBlur(true)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.004f, 0.012f, 0.024f, 0.94f))
				.Padding(FMargin(16.0f, 11.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const AAPSSpectatorPawn* Self = WeakThis.Get();
							return Self ? Self->HudStatus : FText::GetEmpty();
						})
						.ColorAndOpacity(FLinearColor(0.2f, 0.82f, 1.0f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const AAPSSpectatorPawn* Self = WeakThis.Get();
							return Self ? Self->HudNearest : FText::GetEmpty();
						})
						.ColorAndOpacity(FLinearColor(0.72f, 0.9f, 1.0f, 0.96f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(Hint)
						.ColorAndOpacity(FLinearColor(0.82f, 0.87f, 0.92f, 1.0f))
					]
				]
			]
		];
	GEngine->GameViewport->AddViewportWidgetContent(HudWidget.ToSharedRef(), 40);
}

void AAPSSpectatorPawn::RemoveHud()
{
	if (HudWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(HudWidget.ToSharedRef());
	}
	HudWidget.Reset();
}
