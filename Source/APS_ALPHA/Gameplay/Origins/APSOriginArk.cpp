#include "APSOriginArk.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/APSM5EntranceRampComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSOriginArk"

namespace APSOriginArkLocal
{
	TAutoConsoleVariable<int32> CVarHonestStart(TEXT("aps.Origin.HonestStart"), 1,
		TEXT("Rio 18.10 (0.6.6 origins): 1: an ORIGIN world starts with its own kit (one hull, no escorts, outposts or settlements; ")
		TEXT("ARK: no headquarters, station or orbital yard, the ark landed and dark). 0: ORIGIN starts as SANDBOX does (the menu's numbers)."));
	TAutoConsoleVariable<FString> CVarArkShip(TEXT("aps.Origin.ArkShip"),
		TEXT("/Game/APS/APS_ALPHA/Core/Spaceships/L/BP_Spaceship_L_P1_08.BP_Spaceship_L_P1_08_C"),
		TEXT("Rio 18.10: the ARK's temporary ark until the A-00 exists, an L ship with an interior (read at the menu's commit)."));
	TAutoConsoleVariable<int32> CVarArkStartSpot(TEXT("aps.Origin.ArkStartSpot"), 0,
		TEXT("Where a new ARK start puts the pilot aboard: 0 at the top of the entrance ramp (the way out is behind them), ")
		TEXT("1 on the bridge behind the seat. Either falls back to the other when no deck answers there."));
	TAutoConsoleVariable<float> CVarWaveSpeedM(TEXT("aps.Origin.ArkWaveSpeedM"), 45.0f,
		TEXT("How fast the ark's lights come on from the pilot outward when it is powered, metres a second."));
	TAutoConsoleVariable<float> CVarEmergency(TEXT("aps.Origin.ArkEmergencyLight"), 0.10f,
		TEXT("The dark ark: the few lights nearest the pilot glow at this fraction of their intensity until it is powered."));
	TAutoConsoleVariable<float> CVarSettleMaxM(TEXT("aps.Origin.ArkSettleMaxM"), 100.0f,
		TEXT("ARK: how far the landed ark may move along its up so its ramp foot meets the ground (it sinks into a rise or the ground ")
		TEXT("under its belly; a grounded ark). Metres."));
	TAutoConsoleVariable<float> CVarMonumentSectorDeg(TEXT("aps.Origin.MonumentSectorDeg"), 40.0f,
		TEXT("ARK: the monument stands within this many degrees either side of the direction the ark's exit faces."));
	TAutoConsoleVariable<float> CVarExitReachM(TEXT("aps.Origin.ArkExitReachM"), 30.0f,
		TEXT("ARK: outside, the lift up to the bridge answers F within this many metres of the exit."));
	TAutoConsoleVariable<int32> CVarArkLightShadows(TEXT("aps.Origin.ArkLightShadows"), 0,
		TEXT("Rio 19.10 (ark-t5: 50 fps aboard the powered ark against 75 dark and 95 outside, 3440x1440): 1 keeps the ark's lamps' ")
		TEXT("authored shadows; 0 (default): they light without shadows (the daylight through its glass keeps the sun's)."));
	TAutoConsoleVariable<float> CVarArkLightDrawM(TEXT("aps.Origin.ArkLightDrawM"), 60.0f,
		TEXT("The ark's lamps are drawn within this many metres of the camera (fading over the last fifth); 0: as authored."));
	TAutoConsoleVariable<float> CVarLookAsideDeg(TEXT("aps.Origin.LookAsideDeg"), 14.0f,
		TEXT("Camera moments: the target stands this many degrees right of the frame's middle (the walker is in the middle)."));
	TAutoConsoleVariable<float> CVarMonumentMinKm(TEXT("aps.Origin.MonumentMinKm"), 9.0f,
		TEXT("ARK: the Builders' monument stands at least this far from the ark (on the horizon, too far to walk)."));
	TAutoConsoleVariable<float> CVarMonumentMaxKm(TEXT("aps.Origin.MonumentMaxKm"), 14.0f,
		TEXT("ARK: and at most this far."));

	struct FLight
	{
		TWeakObjectPtr<ULocalLightComponent> Light;
		float Intensity{0.0f};
		bool bAuthoredShadows{false};
		float AuthoredDrawCm{0.0f};
		float AuthoredFadeCm{0.0f};
		double DistanceCm{0.0};
		bool bOn{false};
	};

	struct FLook
	{
		bool bActive{false};
		FVector Target{FVector::ZeroVector};
		FVector From{FVector::ForwardVector};
		double Start{0.0};
		float Turn{1.2f};
		float Hold{1.6f};
	};

	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<ASpaceship> Ark;
		TArray<FLight> Lights;
		FBox LocalBox{ForceInit};
		/** The lift's door on the ground, 20 m past the hull's box on the side nearest the ramp foot, and that side's normal
		 * (ship-local, unscaled). */
		FVector ExitLocal{FVector::ZeroVector};
		FVector ExitNormalLocal{FVector::ZeroVector};
		bool bExit{false};
		double WaveStart{-1.0};
		FLook Look;
		/** The light budget last applied (aps.Origin.ArkLightShadows, aps.Origin.ArkLightDrawM); NaN: none yet. */
		int32 AppliedShadows{-1};
		float AppliedDrawM{-1.0f};
	};

	TArray<FState>& States()
	{
		static TArray<FState> Entries;
		return Entries;
	}

	FState* Find(const UWorld* World)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	FState& StateFor(UWorld* World)
	{
		if (FState* Existing = Find(World))
		{
			return *Existing;
		}
		TArray<FState>& Entries = States();
		Entries.RemoveAll([](const FState& Entry) { return !Entry.World.IsValid(); });
		FState& Added = Entries.AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	ACustomGravityCharacter* Walker(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
	}

	FVector CameraLocation(const APawn& Pawn)
	{
		const UCameraComponent* Camera = Pawn.FindComponentByClass<UCameraComponent>();
		return Camera ? Camera->GetComponentLocation() : Pawn.GetActorLocation();
	}

	FVector CameraForward(const APawn& Pawn)
	{
		const UCameraComponent* Camera = Pawn.FindComponentByClass<UCameraComponent>();
		return Camera ? Camera->GetForwardVector() : Pawn.GetActorForwardVector();
	}

	/** A deck of the ship under Point (within the reach), the hit's point; false when only air or other actors are there. */
	bool DeckUnder(const ASpaceship& Ship, const FVector& Point, const FVector& Up, FVector& OutDeck)
	{
		const UWorld* World = Ship.GetWorld();
		if (!World)
		{
			return false;
		}
		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSArkDeck), true);
		World->LineTraceMultiByChannel(Hits, Point + Up * 150.0, Point - Up * 450.0, ECC_Visibility, Params);
		for (const FHitResult& Hit : Hits)
		{
			const AActor* Owner = Hit.GetActor();
			for (const AActor* Current = Owner; Current; Current = Current->GetAttachParentActor())
			{
				if (Current == &Ship)
				{
					if (Hit.bBlockingHit && FVector::DotProduct(Hit.ImpactNormal, Up) > 0.7)
					{
						OutDeck = Hit.ImpactPoint;
						return true;
					}
					break;
				}
			}
		}
		return false;
	}

	bool CapsuleFree(const UWorld& World, const APawn& Pilot, const FVector& Centre, const FQuat& Rotation)
	{
		const UCapsuleComponent* Capsule = Pilot.FindComponentByClass<UCapsuleComponent>();
		const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 42.0f;
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSArkStand), false, &Pilot);
		return !World.OverlapBlockingTestByChannel(Centre, Rotation, ECC_Pawn,
			FCollisionShape::MakeCapsule(Radius * 0.95f, HalfHeight * 0.95f), Params);
	}

	/** The top of the entrance ramp and the way in from it (the foot mirrored through the ramp's middle). */
	bool RampTop(const ASpaceship& Ship, FVector& OutTop, FVector& OutInward)
	{
		const UAPSM5EntranceRampComponent* RampLogic = Ship.FindComponentByClass<UAPSM5EntranceRampComponent>();
		const UStaticMeshComponent* Hull = const_cast<ASpaceship&>(Ship).GetSpaceshipHull();
		if (!RampLogic || !Hull || RampLogic->DeploymentFootLocal.IsNearlyZero())
		{
			return false;
		}
		TInlineComponentArray<UStaticMeshComponent*> Meshes(&Ship);
		const UStaticMeshComponent* Ramp = nullptr;
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->ComponentHasTag(TEXT("APS.Ship.EntranceRamp")))
			{
				Ramp = Mesh;
				break;
			}
		}
		if (!Ramp)
		{
			return false;
		}
		const FVector Up = Ship.GetActorUpVector();
		const FVector Foot = Hull->GetComponentTransform().TransformPosition(RampLogic->DeploymentFootLocal);
		const FVector Middle = Ramp->GetComponentTransform().TransformPosition(Ramp->CalcLocalBounds().Origin);
		OutTop = Middle * 2.0 - Foot;
		OutInward = FVector::VectorPlaneProject(OutTop - Foot, Up).GetSafeNormal();
		return !OutInward.IsNearlyZero() && FVector::DotProduct(OutTop - Foot, Up) > 100.0;
	}

	/** Lights the few lights nearest Point at the emergency fraction (the dark ark, before the power). */
	void EmergencyGlow(FState& State, const FVector& Point)
	{
		TArray<FLight*> Order;
		for (FLight& Entry : State.Lights)
		{
			if (Entry.Light.IsValid())
			{
				Order.Add(&Entry);
			}
		}
		Order.Sort([&Point](const FLight& A, const FLight& B)
		{
			return FVector::DistSquared(A.Light->GetComponentLocation(), Point)
				< FVector::DistSquared(B.Light->GetComponentLocation(), Point);
		});
		const float Fraction = FMath::Clamp(CVarEmergency.GetValueOnGameThread(), 0.0f, 1.0f);
		for (int32 Index = 0; Index < Order.Num() && Index < 4 && Fraction > 0.0f; ++Index)
		{
			if (FVector::Dist(Order[Index]->Light->GetComponentLocation(), Point) > 3500.0)
			{
				break;
			}
			Order[Index]->Light->SetIntensity(Order[Index]->Intensity * Fraction);
			Order[Index]->Light->SetVisibility(true);
		}
	}

	/** The exit side of the ark's box nearest its ramp foot (or the bridge's spot without a ramp). */
	void FindExit(FState& State, const ASpaceship& Ark)
	{
		State.bExit = false;
		if (!State.LocalBox.IsValid)
		{
			return;
		}
		const UAPSM5EntranceRampComponent* RampLogic = Ark.FindComponentByClass<UAPSM5EntranceRampComponent>();
		const UStaticMeshComponent* Hull = const_cast<ASpaceship&>(Ark).GetSpaceshipHull();
		FVector Anchor = Ark.GetActorLocation();
		if (RampLogic && Hull && !RampLogic->DeploymentFootLocal.IsNearlyZero())
		{
			Anchor = Hull->GetComponentTransform().TransformPosition(RampLogic->DeploymentFootLocal);
		}
		else if (IsValid(Ark.PilotExitPoint))
		{
			Anchor = Ark.PilotExitPoint->GetComponentLocation();
		}
		const FVector Scale = Ark.GetActorScale3D().GetAbs().ComponentMax(FVector(KINDA_SMALL_NUMBER));
		const FVector Local = Ark.GetActorTransform().InverseTransformPosition(Anchor);
		const FBox& Box = State.LocalBox;
		struct FSide { double Cm; FVector Normal; };
		const FSide Sides[] = {
			{(Box.Max.X - Local.X) * Scale.X, FVector::ForwardVector}, {(Local.X - Box.Min.X) * Scale.X, -FVector::ForwardVector},
			{(Box.Max.Y - Local.Y) * Scale.Y, FVector::RightVector}, {(Local.Y - Box.Min.Y) * Scale.Y, -FVector::RightVector}};
		const FSide* Nearest = &Sides[0];
		for (const FSide& Side : Sides)
		{
			Nearest = Side.Cm < Nearest->Cm ? &Side : Nearest;
		}
		const double Beyond = FMath::Max(Nearest->Cm, 0.0) + 2000.0;
		State.ExitNormalLocal = Nearest->Normal;
		State.ExitLocal = Local + Nearest->Normal * (Beyond / (FMath::Abs(Nearest->Normal.X) > 0.5 ? Scale.X : Scale.Y));
		State.bExit = true;
	}

	/** The lift's door on the ground (world) and the way out; false without an exit or ground. */
	bool ExitOnGround(const FState& State, FVector& OutStand, FVector& OutFacing)
	{
		const ASpaceship* Ark = State.Ark.Get();
		const APlanetaryBody* Body = Ark ? Cast<APlanetaryBody>(Ark->GetAttachParentActor()) : nullptr;
		const APlanetarySurfaceGenerator* Surface = Body ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!State.bExit || !Root || Root->PlanetScale <= 0.0)
		{
			return false;
		}
		const FVector Door = Ark->GetActorTransform().TransformPosition(State.ExitLocal);
		const FVector Centre = Root->GetActorLocation();
		const FVector Direction = (Door - Centre).GetSafeNormal();
		const double Ground = Root->GetGroundHeight(Centre + Direction * Root->PlanetScale, false);
		if (!FMath::IsFinite(Ground) || FMath::Abs(Ground) > FMath::Max(2.0 * Root->NoiseIntensity, 100000.0))
		{
			return false;
		}
		OutStand = Centre + Direction * (Root->PlanetScale + Ground);
		OutFacing = FVector::VectorPlaneProject(Ark->GetActorTransform().TransformVectorNoScale(State.ExitNormalLocal), Direction)
			.GetSafeNormal();
		return !OutFacing.IsNearlyZero();
	}

	/** Puts the pilot on a deck of the ark: the bridge's spot or the ramp's first; false while no deck answers. */
	bool PlaceAboard(ASpaceship& Ship, APawn& Pilot, bool bBridgeFirst);

	/** The ark's own light budget: its lamps' shadows and draw distance (the console may change both at any time). */
	void ApplyLightBudget(FState& State)
	{
		const int32 Shadows = CVarArkLightShadows.GetValueOnGameThread() != 0 ? 1 : 0;
		const float DrawM = FMath::Max(CVarArkLightDrawM.GetValueOnGameThread(), 0.0f);
		if (Shadows == State.AppliedShadows && DrawM == State.AppliedDrawM)
		{
			return;
		}
		int32 Shadowed = 0;
		for (FLight& Entry : State.Lights)
		{
			ULocalLightComponent* Light = Entry.Light.Get();
			if (!Light)
			{
				continue;
			}
			Light->SetCastShadows(Shadows != 0 && Entry.bAuthoredShadows);
			Shadowed += Light->CastShadows ? 1 : 0;
			Light->MaxDrawDistance = DrawM > 0.0f ? DrawM * 100.0f : Entry.AuthoredDrawCm;
			Light->MaxDistanceFadeRange = DrawM > 0.0f ? FMath::Max(DrawM * 20.0f, 100.0f) : Entry.AuthoredFadeCm;
			Light->MarkRenderStateDirty();
		}
		State.AppliedShadows = Shadows;
		State.AppliedDrawM = DrawM;
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] light budget: %d lamps, %d with shadows, drawn within %s"), State.Lights.Num(), Shadowed,
			DrawM > 0.0f ? *FString::Printf(TEXT("%.0f m"), DrawM) : TEXT("their authored distances"));
	}

	void PowerUp(UWorld* World, FState& State, const FVector& From)
	{
		if (FAPSMissionBoard* Board = APSMissionsFind(World))
		{
			Board->Unlock(APSOriginArk::PowerToken());
		}
		State.WaveStart = World->GetTimeSeconds();
		for (FLight& Entry : State.Lights)
		{
			Entry.bOn = false;
			Entry.DistanceCm = Entry.Light.IsValid() ? FVector::Dist(Entry.Light->GetComponentLocation(), From) : 0.0;
		}
		if (UAPSAudioSubsystem* Audio = World->GetSubsystem<UAPSAudioSubsystem>())
		{
			Audio->PlayPowerUp();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] powered: %d lights come on from the pilot outward at %.0f m/s"),
			State.Lights.Num(), CVarWaveSpeedM.GetValueOnGameThread());
	}

	void TickWave(UWorld* World, FState& State)
	{
		if (State.WaveStart < 0.0)
		{
			return;
		}
		const double SpeedCm = FMath::Max(CVarWaveSpeedM.GetValueOnGameThread(), 1.0f) * 100.0;
		const double Radius = (World->GetTimeSeconds() - State.WaveStart) * SpeedCm;
		// Each lamp warms up over 0.4 s once the wave reaches it.
		const double FadeCm = SpeedCm * 0.4;
		bool bAllOn = true;
		for (FLight& Entry : State.Lights)
		{
			ULocalLightComponent* Light = Entry.Light.Get();
			if (Entry.bOn || !Light)
			{
				continue;
			}
			if (Radius < Entry.DistanceCm)
			{
				bAllOn = false;
				continue;
			}
			const float Alpha = static_cast<float>(FMath::Clamp((Radius - Entry.DistanceCm) / FadeCm, 0.0, 1.0));
			Light->SetVisibility(true);
			Light->SetIntensity(Entry.Intensity * Alpha);
			Entry.bOn = Alpha >= 1.0f;
			bAllOn &= Entry.bOn;
		}
		if (bAllOn)
		{
			State.WaveStart = -1.0;
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] every light of the ark is on"));
		}
	}

	void TickLook(UWorld* World, FState& State)
	{
		FLook& Look = State.Look;
		if (!Look.bActive)
		{
			return;
		}
		ACustomGravityCharacter* Character = Walker(World);
		const double Elapsed = World->GetTimeSeconds() - Look.Start;
		if (!Character || Elapsed > Look.Turn + Look.Hold)
		{
			Look.bActive = false;
			return;
		}
		const FVector Eye = CameraLocation(*Character);
		const FVector Toward = (Look.Target - Eye).GetSafeNormal();
		if (Toward.IsNearlyZero())
		{
			Look.bActive = false;
			return;
		}
		const float Alpha = Look.Turn > KINDA_SMALL_NUMBER
			? FMath::SmoothStep(0.0f, 1.0f, static_cast<float>(Elapsed / Look.Turn)) : 1.0f;
		// ark-t4 (19.10): the third-person walker stood right in front of the spire and the view climbed into the sky. The
		// target sits a little aside of the frame's middle, the camera near the horizon with the sky above the target.
		const FVector Up = Character->GetActorUpVector();
		const FVector FromPlanar = FVector::VectorPlaneProject(Look.From, Up).GetSafeNormal();
		FVector ToPlanar = FVector::VectorPlaneProject(Toward, Up).GetSafeNormal();
		if (FromPlanar.IsNearlyZero() || ToPlanar.IsNearlyZero())
		{
			Look.bActive = false;
			return;
		}
		ToPlanar = ToPlanar.RotateAngleAxis(-CVarLookAsideDeg.GetValueOnGameThread(), Up);
		const FVector Planar = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(FromPlanar, ToPlanar), Alpha)
			.RotateVector(FromPlanar).GetSafeNormal();
		const float FromPitch = static_cast<float>(FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(Look.From, Up), -1.0, 1.0))));
		const float TargetPitch = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(
			FMath::Asin(FMath::Clamp(FVector::DotProduct(Toward, Up), -1.0, 1.0)))), 2.0f, 10.0f);
		Character->SetViewDirection(Planar, FMath::Lerp(FromPitch, TargetPitch, Alpha));
	}
}

FName APSOriginArk::Tag()
{
	static const FName Name(TEXT("APS.Origin.Ark"));
	return Name;
}

FName APSOriginArk::PowerToken()
{
	static const FName Name(TEXT("ORIGIN_ARK_POWER"));
	return Name;
}

namespace APSOriginArkLocal
{
	/** The start kit the running world was founded with (USpawnParameters::OriginStartKit: 0 none, 1 origin, 2 trip). */
	uint8 KitOf(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		return Gameplay && Gameplay->SpawnParameters ? Gameplay->SpawnParameters->OriginStartKit : 0;
	}
}

bool APSOriginArk::IsHonestStart(const UWorld* World)
{
	if (APSOriginArkLocal::CVarHonestStart.GetValueOnGameThread() == 0 || !APSWorldRules::IsLadder(World)
		|| APSWorldRules::OfGame(World).Mode != APSWorldRules::EMode::Origin)
	{
		return false;
	}
	// Founded with the origin's kit (saved with the spawn parameters): an ORIGIN save from before 0.6.6 keeps its old start.
	return APSOriginArkLocal::KitOf(World) == 1;
}

bool APSOriginArk::IsTripKitWorld(const UWorld* World)
{
	return APSOriginArkLocal::CVarHonestStart.GetValueOnGameThread() != 0 && APSWorldRules::IsTrip(World)
		&& APSWorldRules::OfGame(World).Mode == APSWorldRules::EMode::SpaceTrips && APSOriginArkLocal::KitOf(World) == 2;
}

bool APSOriginArk::IsHullOnlyStart(const UWorld* World)
{
	return IsArkWorld(World) || IsTripKitWorld(World);
}

bool APSOriginArk::IsArkWorld(const UWorld* World)
{
	return IsHonestStart(World) && APSWorldRules::OfGame(World).Origin == APSWorldRules::EOrigin::Ark;
}

void APSOriginArk::ApplyStartKit(const UGeneratedWorld& World, USpawnParameters& Spawn)
{
	using namespace APSOriginArkLocal;
	const APSWorldRules::FRules Rules = APSWorldRules::Of(&World);
	if (CVarHonestStart.GetValueOnGameThread() != 0 && Rules.Mode == APSWorldRules::EMode::SpaceTrips)
	{
		// Rio 19.10 (SPACE TRIPS, "a sky for looking at"): the ship alone in orbit, the pilot in its seat; no complex, no
		// escorts, no colony in sight, no vehicles.
		Spawn.OriginStartKit = 2;
		Spawn.StartingFleetSize = 1;
		Spawn.StarOutposts = 0;
		Spawn.PlanetOutposts = 0;
		Spawn.OrbitalOutposts = 1;
		Spawn.GroundOutposts = 0;
		Spawn.ColonyStartPackage = EAPSColonyStartPackage::Outpost;
		Spawn.GroundVehicleMask = 0;
		Spawn.CharacterSpawnPlace = ECharSpawnPlace::SpaceShip;
		UE_LOG(LogTemp, Log, TEXT("[APS.Trip] start kit: the ship alone in orbit (%s), no complex, colony or vehicles"),
			*GetNameSafe(Spawn.BP_HomeSpaceship.Get()));
		return;
	}
	if (CVarHonestStart.GetValueOnGameThread() == 0 || Rules.Mode != APSWorldRules::EMode::Origin
		|| Rules.Reach != APSWorldRules::EReach::Ladder || Rules.Origin == APSWorldRules::EOrigin::None)
	{
		return;
	}
	// The origin's start, not the last civilization page's (ContinueAstronomicalGeneration skips that page for a card).
	Spawn.OriginStartKit = 1;
	Spawn.StartingFleetSize = 1;
	Spawn.StarOutposts = 0;
	Spawn.PlanetOutposts = 0;
	Spawn.OrbitalOutposts = 1;
	Spawn.GroundOutposts = 0;
	Spawn.ColonyStartPackage = EAPSColonyStartPackage::Outpost;
	// The vehicles stay wanted: in the ladder each waits for its token (the rover for the fabrication bay).
	Spawn.GroundVehicleMask = 7;
	FString Ship = TEXT("as chosen");
	if (Rules.Origin == APSWorldRules::EOrigin::Ark)
	{
		Spawn.CharacterSpawnPlace = ECharSpawnPlace::PlanetSurface;
		if (UClass* ArkClass = LoadClass<ASpaceship>(nullptr, *CVarArkShip.GetValueOnGameThread()))
		{
			Spawn.BP_HomeSpaceship = ArkClass;
			Ship = ArkClass->GetName();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Ark] aps.Origin.ArkShip '%s' does not load; the chosen ship is the ark"),
				*CVarArkShip.GetValueOnGameThread());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin] start kit %s: one hull (%s), no escorts, outposts or settlements"),
		APSWorldRules::OriginName(Rules.Origin), *Ship);
}

void APSOriginArk::ClearStartKit(USpawnParameters& Spawn)
{
	const USpawnParameters* Defaults = GetDefault<USpawnParameters>();
	const UClass* ArkClass = FindObject<UClass>(nullptr, *APSOriginArkLocal::CVarArkShip.GetValueOnGameThread());
	if (Spawn.BP_HomeSpaceship && Spawn.BP_HomeSpaceship.Get() == ArkClass)
	{
		// The menu's own default hull comes back (UWorldGenerationViewModel fills an empty slot).
		Spawn.BP_HomeSpaceship = nullptr;
	}
	Spawn.StartingFleetSize = Defaults->StartingFleetSize;
	Spawn.StarOutposts = Defaults->StarOutposts;
	Spawn.PlanetOutposts = Defaults->PlanetOutposts;
	Spawn.OrbitalOutposts = Defaults->OrbitalOutposts;
	Spawn.GroundOutposts = Defaults->GroundOutposts;
	Spawn.ColonyStartPackage = Defaults->ColonyStartPackage;
	Spawn.GroundVehicleMask = Defaults->GroundVehicleMask;
	Spawn.OriginStartKit = 0;
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin] back in the menu: the origin's start kit is cleared"));
}

bool APSOriginArk::IsArk(const AActor* Actor)
{
	return IsValid(Actor) && Actor->ActorHasTag(Tag());
}

ASpaceship* APSOriginArk::FindArk(const UWorld* World)
{
	using namespace APSOriginArkLocal;
	if (FState* State = Find(World); State && State->Ark.IsValid())
	{
		return State->Ark.Get();
	}
	return nullptr;
}

bool APSOriginArk::IsPowered(const UWorld* World)
{
	const FAPSMissionBoard* Board = APSMissionsFind(World);
	return Board && Board->IsUnlocked(PowerToken());
}

void APSOriginArk::AdoptArk(ASpaceship& Ship)
{
	using namespace APSOriginArkLocal;
	UWorld* World = Ship.GetWorld();
	if (!World)
	{
		return;
	}
	Ship.Tags.AddUnique(Tag());
	FState& State = StateFor(World);
	const bool bSameShip = State.Ark.Get() == &Ship;
	State.Ark = &Ship;
	State.LocalBox = UAPSSpawnPlacementSubsystem::VisualLocalBounds(&Ship);
	if (!State.LocalBox.IsValid)
	{
		State.LocalBox = Ship.CalculateComponentsBoundingBoxInLocalSpace(true);
	}
	if (!bSameShip)
	{
		State.Lights.Reset();
		TArray<ULocalLightComponent*> Lights;
		Ship.GetComponents<ULocalLightComponent>(Lights, true);
		for (ULocalLightComponent* Light : Lights)
		{
			if (IsValid(Light) && Light->IsVisible() && Light->Intensity > 0.0f)
			{
				FLight& Entry = State.Lights.AddDefaulted_GetRef();
				Entry.Light = Light;
				Entry.Intensity = Light->Intensity;
				Entry.bAuthoredShadows = Light->CastShadows;
				Entry.AuthoredDrawCm = Light->MaxDrawDistance;
				Entry.AuthoredFadeCm = Light->MaxDistanceFadeRange;
			}
		}
	}
	FindExit(State, Ship);
	State.AppliedShadows = -1;
	ApplyLightBudget(State);
	const bool bPowered = IsPowered(World);
	if (!bPowered)
	{
		for (FLight& Entry : State.Lights)
		{
			if (Entry.Light.IsValid())
			{
				Entry.Light->SetVisibility(false);
				Entry.bOn = false;
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] %s is the ark: %d lights, %s, hull box %s m"), *Ship.GetName(), State.Lights.Num(),
		bPowered ? TEXT("powered") : TEXT("dark"), *(State.LocalBox.GetSize() * Ship.GetActorScale3D().GetAbs() / 100.0).ToCompactString());
}

void APSOriginArk::SettleRampOnGround(ASpaceship& Ship, const APlanetaryBody& Body)
{
	const UAPSM5EntranceRampComponent* RampLogic = Ship.FindComponentByClass<UAPSM5EntranceRampComponent>();
	const UStaticMeshComponent* Hull = Ship.GetSpaceshipHull();
	const APlanetarySurfaceGenerator* Surface = Body.PlanetaryEnvironmentGenerator;
	AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
	if (!RampLogic || !Hull || !IsValid(Root) || Root->PlanetScale <= 0.0 || RampLogic->DeploymentFootLocal.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Ark] %s: no ramp foot or no ground to settle it on"), *Ship.GetName());
		return;
	}
	const FVector Centre = Root->GetActorLocation();
	const FVector Foot = Hull->GetComponentTransform().TransformPosition(RampLogic->DeploymentFootLocal);
	const FVector Direction = (Foot - Centre).GetSafeNormal();
	const double GroundCm = Root->GetGroundHeight(Centre + Direction * Root->PlanetScale, false);
	if (!FMath::IsFinite(GroundCm) || FMath::Abs(GroundCm) > FMath::Max(2.0 * Root->NoiseIntensity, 100000.0))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Ark] %s: broken ground sample under the ramp foot (%.0f m)"), *Ship.GetName(),
			GroundCm / 100.0);
		return;
	}
	// The foot 10 cm over the ground; the hull may sink into a rise or stand over a dip elsewhere (a grounded ark).
	const double Delta = (Root->PlanetScale + GroundCm + 10.0) - FVector::Dist(Foot, Centre);
	const double Limit = FMath::Max(APSOriginArkLocal::CVarSettleMaxM.GetValueOnGameThread(), 0.0f) * 100.0;
	const double Applied = FMath::Clamp(Delta, -Limit, Limit);
	FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(&Ship);
	const FVector Up = Ship.GetActorUpVector();
	const double FootOverBottom = Local.IsValid
		? FVector::DotProduct(Foot - Ship.GetActorLocation(), Up) - Local.Min.Z * Ship.GetActorScale3D().Z : 0.0;
	Ship.AddActorWorldOffset(Up * Applied, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] %s: ramp foot settled on the ground (hull moved %.1f m along its up%s; the foot stands %.1f m over the hull's lowest point, local %s)"),
		*Ship.GetName(), Applied / 100.0, Applied != Delta ? *FString::Printf(TEXT(", clamped from %.1f m"), Delta / 100.0) : TEXT(""),
		FootOverBottom / 100.0, *RampLogic->DeploymentFootLocal.ToCompactString());
}

bool APSOriginArk::PlacePilotAboard(ASpaceship& Ship, APawn& Pilot)
{
	return APSOriginArkLocal::PlaceAboard(Ship, Pilot, APSOriginArkLocal::CVarArkStartSpot.GetValueOnGameThread() == 1);
}

bool APSOriginArkLocal::PlaceAboard(ASpaceship& Ship, APawn& Pilot, const bool bBridgeFirst)
{
	using namespace APSOriginArkLocal;
	UWorld* World = Ship.GetWorld();
	if (!World)
	{
		return false;
	}
	const FVector Up = Ship.GetActorUpVector();
	const UCapsuleComponent* Capsule = Pilot.FindComponentByClass<UCapsuleComponent>();
	const double HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0;

	struct FSpot
	{
		FVector Point;
		FVector Facing;
		const TCHAR* Name;
	};
	TArray<FSpot, TInlineAllocator<16>> Spots;
	const auto AddRamp = [&]()
	{
		FVector Top;
		FVector Inward;
		if (RampTop(Ship, Top, Inward))
		{
			for (int32 Step = 1; Step <= 6; ++Step)
			{
				Spots.Add({Top + Inward * (150.0 * Step), Inward, TEXT("the ramp's top")});
			}
		}
	};
	const auto AddBridge = [&]()
	{
		if (IsValid(Ship.PilotExitPoint))
		{
			const FVector Facing = FVector::VectorPlaneProject(Ship.PilotExitPoint->GetForwardVector(), Up).GetSafeNormal();
			Spots.Add({Ship.PilotExitPoint->GetComponentLocation(), Facing.IsNearlyZero() ? Ship.GetActorForwardVector() : Facing,
				TEXT("the bridge")});
		}
	};
	if (bBridgeFirst)
	{
		AddBridge();
		AddRamp();
	}
	else
	{
		AddRamp();
		AddBridge();
	}
	for (const FSpot& Spot : Spots)
	{
		FVector Deck;
		if (!DeckUnder(Ship, Spot.Point, Up, Deck))
		{
			continue;
		}
		const FVector Standing = Deck + Up * (HalfHeight + 2.0);
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(Spot.Facing, Up).ToQuat();
		if (!CapsuleFree(*World, Pilot, Standing, Rotation))
		{
			continue;
		}
		Pilot.SetActorLocationAndRotation(Standing, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		if (ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(&Pilot))
		{
			Character->SetViewDirection(Spot.Facing, -4.0f);
		}
		if (APlayerController* Controller = Cast<APlayerController>(Pilot.GetController()))
		{
			Controller->SetControlRotation(Rotation.Rotator());
		}
		if (FState* State = Find(World); State && !APSOriginArk::IsPowered(World))
		{
			EmergencyGlow(*State, Standing);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] the pilot starts aboard the ark at %s, %.1f m over its pivot"), Spot.Name,
			FVector::DotProduct(Standing - Ship.GetActorLocation(), Up) / 100.0);
		return true;
	}
	return false;
}

bool APSOriginArk::IsOutside(const UWorld* World, const APawn* Pawn, const double MarginCm)
{
	using namespace APSOriginArkLocal;
	const FState* State = Find(World);
	const ASpaceship* Ark = State ? State->Ark.Get() : nullptr;
	if (!Pawn || !Ark || !State->LocalBox.IsValid)
	{
		return true;
	}
	const FVector Scale = Ark->GetActorScale3D().GetAbs();
	const FVector Local = Ark->GetActorTransform().InverseTransformPositionNoScale(Pawn->GetActorLocation());
	const FBox Box(State->LocalBox.Min * Scale - FVector(MarginCm), State->LocalBox.Max * Scale + FVector(MarginCm));
	return !Box.IsInside(Local);
}

namespace APSOriginArkLocal
{
	enum class EAction : uint8 { None, Power, LiftDown, LiftUp };

	bool NearExit(const FState& State, const APawn& Pawn)
	{
		FVector Stand;
		FVector Facing;
		return ExitOnGround(State, Stand, Facing)
			&& FVector::Dist(Pawn.GetActorLocation(), Stand) <= FMath::Max(CVarExitReachM.GetValueOnGameThread(), 1.0f) * 100.0;
	}

	EAction ActionOf(const APawn* Pawn, const AActor* Candidate)
	{
		const UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
		const FState* State = World ? Find(World) : nullptr;
		if (!APSOriginArk::IsArk(Candidate) || !State || State->Ark.Get() != Candidate)
		{
			return EAction::None;
		}
		const bool bAboard = !APSOriginArk::IsOutside(World, Pawn, 0.0);
		if (!APSOriginArk::IsPowered(World))
		{
			return bAboard ? EAction::Power : EAction::None;
		}
		if (bAboard)
		{
			return State->bExit ? EAction::LiftDown : EAction::None;
		}
		return NearExit(*State, *Pawn) ? EAction::LiftUp : EAction::None;
	}
}

FText APSOriginArk::ActionFor(const APawn* Pawn, const AActor* Candidate)
{
	using namespace APSOriginArkLocal;
	switch (ActionOf(Pawn, Candidate))
	{
	case EAction::Power: return LOCTEXT("PowerTheArk", "POWER THE ARK");
	case EAction::LiftDown: return LOCTEXT("LiftDown", "LIFT DOWN TO THE GROUND");
	case EAction::LiftUp: return LOCTEXT("LiftUp", "LIFT UP TO THE BRIDGE");
	default: return FText::GetEmpty();
	}
}

FText APSOriginArk::SubjectFor(const UWorld* World)
{
	return IsPowered(World) ? LOCTEXT("SubjectLift", "The ark's lift") : LOCTEXT("SubjectDark", "The ark: main bus down");
}

AActor* APSOriginArk::ExitCandidate(const APawn* Pawn)
{
	using namespace APSOriginArkLocal;
	const UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	const FState* State = World ? Find(World) : nullptr;
	ASpaceship* Ark = State ? State->Ark.Get() : nullptr;
	return Ark && IsPowered(World) && IsOutside(World, Pawn, 0.0) && NearExit(*State, *Pawn) ? Ark : nullptr;
}

bool APSOriginArk::TryInteract(APawn* Pawn, AActor* Candidate)
{
	using namespace APSOriginArkLocal;
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	const EAction Action = ActionOf(Pawn, Candidate);
	if (!World || Action == EAction::None)
	{
		return false;
	}
	FState& State = StateFor(World);
	if (Action == EAction::Power)
	{
		PowerUp(World, State, Pawn->GetActorLocation());
		return true;
	}
	if (Action == EAction::LiftDown)
	{
		FVector Stand;
		FVector Facing;
		if (!ExitOnGround(State, Stand, Facing))
		{
			return false;
		}
		const UCapsuleComponent* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>();
		const FVector Up = (Stand - Candidate->GetAttachParentActor()->GetActorLocation()).GetSafeNormal();
		Pawn->SetActorLocationAndRotation(Stand + Up * ((Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0) + 40.0),
			FRotationMatrix::MakeFromXZ(Facing, Up).ToQuat(), false, nullptr, ETeleportType::TeleportPhysics);
		if (ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pawn))
		{
			Character->SetViewDirection(Facing, 6.0f);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] the lift: down to the ground beside the exit"));
		return true;
	}
	// Up to the bridge (the bridge's spot first, then the ramp's).
	const bool bPlaced = PlaceAboard(*State.Ark.Get(), *Pawn, true);
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] the lift: up to the bridge %s"), bPlaced ? TEXT("") : TEXT("(no deck answered)"));
	return bPlaced;
}

void APSOriginArk::LookAt(UWorld* World, const FVector& Target, const float TurnSeconds, const float HoldSeconds)
{
	using namespace APSOriginArkLocal;
	const ACustomGravityCharacter* Character = Walker(World);
	if (!Character)
	{
		return;
	}
	FLook& Look = StateFor(World).Look;
	Look.bActive = true;
	Look.Target = Target;
	Look.From = CameraForward(*Character).GetSafeNormal();
	Look.Start = World->GetTimeSeconds();
	Look.Turn = FMath::Max(TurnSeconds, 0.0f);
	Look.Hold = FMath::Max(HoldSeconds, 0.0f);
	UE_LOG(LogTemp, Log, TEXT("[APS.Origin] camera moment: the view turns to %s (%.0f m away)"), *Target.ToCompactString(),
		FVector::Dist(Target, CameraLocation(*Character)) / 100.0);
}

AActor* APSOriginArk::FindMonument(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	static const FName MonumentTag(TEXT("APS.Ancient.Site.H_MONUMENT"));
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(MonumentTag))
		{
			return *It;
		}
	}
	return nullptr;
}

bool APSOriginArk::MonumentRing(const UWorld* World, const APlanetaryBody* Body, FVector& OutLocalCentre, FVector& OutLocalToward,
	double& OutMinCm, double& OutMaxCm, bool& bOutWait)
{
	using namespace APSOriginArkLocal;
	bOutWait = false;
	if (!IsValid(Body) || !IsArkWorld(World))
	{
		return false;
	}
	const ASpaceship* Ark = FindArk(World);
	if (!Ark)
	{
		bOutWait = true;
		return false;
	}
	if (Ark->GetAttachParentActor() != Body)
	{
		return false;
	}
	OutLocalCentre = Body->GetActorTransform().InverseTransformVectorNoScale(
		(Ark->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal());
	OutLocalToward = FVector::ZeroVector;
	if (const FState* State = Find(World); State && State->bExit)
	{
		OutLocalToward = Body->GetActorTransform().InverseTransformVectorNoScale(
			Ark->GetActorTransform().TransformVectorNoScale(State->ExitNormalLocal));
	}
	OutMinCm = FMath::Max(CVarMonumentMinKm.GetValueOnGameThread(), 1.0f) * 100000.0;
	OutMaxCm = FMath::Max(CVarMonumentMaxKm.GetValueOnGameThread() * 100000.0, OutMinCm);
	return !OutLocalCentre.IsNearlyZero();
}

FVector APSOriginArk::RingDirection(const FVector& LocalCentre, const FVector& LocalToward, const double BodyRadiusCm,
	const double MinCm, const double MaxCm, const uint32 Seed, const int32 Index)
{
	const FRandomStream Stream(static_cast<int32>(HashCombine(Seed, GetTypeHash(Index) * 2654435761u)));
	const double Distance = FMath::Lerp(MinCm, MaxCm, static_cast<double>(Stream.FRand()));
	const FVector Centre = LocalCentre.GetSafeNormal();
	FVector TangentA = FVector::VectorPlaneProject(LocalToward, Centre).GetSafeNormal();
	FVector TangentB;
	double Azimuth;
	if (TangentA.IsNearlyZero())
	{
		Centre.FindBestAxisVectors(TangentA, TangentB);
		Azimuth = Stream.FRandRange(0.0, UE_TWO_PI);
	}
	else
	{
		// Within the sector the exit faces; the first candidates nearest its middle.
		TangentB = FVector::CrossProduct(Centre, TangentA).GetSafeNormal();
		const double Sector = FMath::DegreesToRadians(FMath::Clamp(APSOriginArkLocal::CVarMonumentSectorDeg.GetValueOnGameThread(), 0.0f, 180.0f));
		const double Spread = Sector * FMath::Min(1.0, 0.25 + Index / 16.0);
		Azimuth = Stream.FRandRange(-Spread, Spread);
	}
	const double Angle = Distance / FMath::Max(BodyRadiusCm, 1.0);
	return (Centre * FMath::Cos(Angle) + (TangentA * FMath::Cos(Azimuth) + TangentB * FMath::Sin(Azimuth)) * FMath::Sin(Angle))
		.GetSafeNormal();
}

void APSOriginArk::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSOriginArkLocal;
	FState* State = World ? Find(World) : nullptr;
	if (!State)
	{
		return;
	}
	TickWave(World, *State);
	TickLook(World, *State);
	ApplyLightBudget(*State);
}

namespace APSOriginArkConsole
{
	using namespace APSOriginArkLocal;

	ASpaceship* ArkOf(UWorld* World)
	{
		return World ? APSOriginArk::FindArk(World) : nullptr;
	}

	// Test helpers for offscreen runs (no key presses there): the F at the ark, a walk out, a look at it, a report.
	FAutoConsoleCommandWithWorld UseCommand(TEXT("aps.Origin.ArkUse"),
		TEXT("Test: the walker's F at the ark (power, the lift down aboard, the lift up at the exit)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			ASpaceship* Ark = ArkOf(World);
			APawn* Pawn = World && World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
			const FText Action = APSOriginArk::ActionFor(Pawn, Ark);
			const bool bDone = Ark && Pawn && APSOriginArk::TryInteract(Pawn, Ark);
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] aps.Origin.ArkUse: %s %s"), Action.IsEmpty() ? TEXT("(nothing here)") : *Action.ToString(),
				bDone ? TEXT("done") : TEXT("not done"));
		}));

	FAutoConsoleCommandWithWorld PowerCommand(TEXT("aps.Origin.ArkPower"),
		TEXT("Test: the ark's F (powers it from where the walker stands, aboard or not)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			ASpaceship* Ark = ArkOf(World);
			APawn* Pawn = World && World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
			if (!Ark || !Pawn || APSOriginArk::IsPowered(World))
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Ark] aps.Origin.ArkPower: no ark, no pawn or powered already"));
				return;
			}
			if (!APSOriginArk::TryInteract(Pawn, Ark))
			{
				PowerUp(World, StateFor(World), Pawn->GetActorLocation());
			}
		}));

	FAutoConsoleCommandWithWorld OutCommand(TEXT("aps.Origin.ArkOut"),
		TEXT("Test: the walker stands on the ground 40 m out from the ark's ramp foot, facing the ark."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			ASpaceship* Ark = ArkOf(World);
			ACustomGravityCharacter* Character = Walker(World);
			const UAPSM5EntranceRampComponent* RampLogic = Ark ? Ark->FindComponentByClass<UAPSM5EntranceRampComponent>() : nullptr;
			const APlanetaryBody* Body = Ark ? Cast<APlanetaryBody>(Ark->GetAttachParentActor()) : nullptr;
			const APlanetarySurfaceGenerator* Surface = Body ? Body->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
			if (!Ark || !Character || !RampLogic || !Root || !Ark->GetSpaceshipHull())
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Origin.Ark] aps.Origin.ArkOut: no ark, walker, ramp or ground"));
				return;
			}
			// Out of the hull's box across its narrow side, 40 m past it, level with the ramp's side of the ship.
			const FVector Up = Ark->GetActorUpVector();
			const FVector Foot = Ark->GetSpaceshipHull()->GetComponentTransform().TransformPosition(RampLogic->DeploymentFootLocal);
			const FState* State = Find(World);
			const FVector Half = State && State->LocalBox.IsValid ? State->LocalBox.GetExtent() * Ark->GetActorScale3D().GetAbs()
				: FVector(10000.0);
			const bool bAcrossX = Half.X <= Half.Y;
			FVector Out = bAcrossX ? Ark->GetActorForwardVector() : Ark->GetActorRightVector();
			if (FVector::DotProduct(Foot - Ark->GetActorLocation(), Out) < 0.0)
			{
				Out = -Out;
			}
			const double Reach = (bAcrossX ? Half.X : Half.Y) + 4000.0;
			const FVector Centre = Root->GetActorLocation();
			const FVector Direction = (Ark->GetActorLocation() + Out * Reach - Centre).GetSafeNormal();
			const double Ground = Root->GetGroundHeight(Centre + Direction * Root->PlanetScale, false);
			const FVector Stand = Centre + Direction * (Root->PlanetScale + Ground + 150.0);
			Character->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
			Character->SetViewDirection((Ark->GetActorLocation() - Stand).GetSafeNormal(), 8.0f);
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] aps.Origin.ArkOut: the walker stands 40 m past the hull's side (%.0f m from its centre)"),
				Reach / 100.0);
		}));

	FAutoConsoleCommandWithWorldAndArgs LookCommand(TEXT("aps.Origin.ArkLook"),
		TEXT("Test: a camera moment toward the ark (or 'monument')."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const AActor* Target = ArkOf(World);
			if (Args.Num() > 0 && Args[0].Equals(TEXT("monument"), ESearchCase::IgnoreCase))
			{
				Target = APSOriginArk::FindMonument(World);
			}
			else if (Args.Num() > 0 && Args[0].Equals(TEXT("base"), ESearchCase::IgnoreCase) && World)
			{
				// The colony's site anchor (hidden; the modules stand around it).
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (IsValid(*It) && It->ActorHasTag(TEXT("APS.Placeholder.LegacyColony")))
					{
						Target = *It;
						break;
					}
				}
			}
			if (Target)
			{
				FVector Origin;
				FVector Extent;
				Target->GetActorBounds(false, Origin, Extent);
				APSOriginArk::LookAt(World, Origin, 0.8f, 3.0f);
			}
		}));

	FAutoConsoleCommandWithWorld ReportCommand(TEXT("aps.Origin.ArkReport"),
		TEXT("Logs the ark: where it stands, its ramp, its lights, the walker, the monument."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			ASpaceship* Ark = ArkOf(World);
			const FState* State = Find(World);
			if (!Ark || !State)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] report: no ark in this world (ark world %d)"), APSOriginArk::IsArkWorld(World) ? 1 : 0);
				return;
			}
			int32 Visible = 0;
			for (const FLight& Entry : State->Lights)
			{
				Visible += Entry.Light.IsValid() && Entry.Light->IsVisible() ? 1 : 0;
			}
			const UAPSM5EntranceRampComponent* RampLogic = Ark->FindComponentByClass<UAPSM5EntranceRampComponent>();
			const APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
			const AActor* Monument = APSOriginArk::FindMonument(World);
			if (const ACustomGravityCharacter* Character = Walker(World); Character && Monument)
			{
				FVector Origin;
				FVector Extent;
				Monument->GetActorBounds(false, Origin, Extent);
				const FVector Eye = CameraLocation(*Character);
				const double Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(
					CameraForward(*Character).GetSafeNormal(), (Origin - Eye).GetSafeNormal()), -1.0, 1.0)));
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(APSArkSight), true, Character);
				Params.AddIgnoredActor(Monument);
				const bool bBlocked = World->LineTraceSingleByChannel(Hit, Eye, Eye + (Origin - Eye).GetSafeNormal() * 300000.0,
					ECC_Visibility, Params);
				UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] report: the monument is %.0f deg off the view, its middle %s (%s)"), Angle,
					bBlocked ? TEXT("hidden") : TEXT("in sight"), bBlocked ? *GetNameSafe(Hit.GetActor()) : TEXT("-"));
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Origin.Ark] report: %s parent=%s powered=%d lights %d/%d visible ramp=%s walker=%s %s, %.0f m from the ark, monument %s"),
				*Ark->GetName(), *GetNameSafe(Ark->GetAttachParentActor()), APSOriginArk::IsPowered(World) ? 1 : 0, Visible,
				State->Lights.Num(), RampLogic ? (RampLogic->bRampDeployed ? TEXT("deployed") : TEXT("up")) : TEXT("none"),
				*GetNameSafe(Pawn), APSOriginArk::IsOutside(World, Pawn, 0.0) ? TEXT("outside") : TEXT("aboard"),
				Pawn ? FVector::Dist(Pawn->GetActorLocation(), Ark->GetActorLocation()) / 100.0 : -1.0,
				Monument ? *FString::Printf(TEXT("%.1f km from the ark"), FVector::Dist(Monument->GetActorLocation(), Ark->GetActorLocation()) / 100000.0)
					: TEXT("not standing"));
		}));
}

#undef LOCTEXT_NAMESPACE
