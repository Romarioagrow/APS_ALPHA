#include "APSAudioSubsystem.h"

#include "APSAudioBank.h"
#include "APSAudioSurfaceResolver.h"
#include "APSVehicleAudioPolicy.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/UI/Settings/APSAudioSettingsBridge.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSAudio, Log, All);

namespace
{
	TAutoConsoleVariable<int32> AudioEnabled(TEXT("aps.Audio.Enabled"), 1,
		TEXT("Enable the APOSFERA audio layer. 0 stops its sounds immediately."));
	const TCHAR* AdditionalFootstepSurfaces[] = {TEXT("Plastic"), TEXT("Stone"), TEXT("Wood"), TEXT("Snow")};
	const FSoftObjectPath BankPath(TEXT("/Game/APS/APS_ALPHA/Audio/DA_APSAudioBank.DA_APSAudioBank"));
}

bool UAPSAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return !IsRunningCommandlet() && !IsRunningDedicatedServer() && World
		&& (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	TArray<FSoftObjectPath> BankPaths{BankPath};
	for (const TCHAR* Surface : AdditionalFootstepSurfaces)
	{
		for (int32 I = 1; I <= 5; ++I)
			BankPaths.Emplace(FString::Printf(TEXT("/Game/APS/APS_ALPHA/Audio/Footsteps/SW_Step_%s_%02d.SW_Step_%s_%02d"), Surface, I, Surface, I));
	}
	BankHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(BankPaths,
		FStreamableDelegate::CreateUObject(this, &UAPSAudioSubsystem::BankLoaded));
	TArray<FSoftObjectPath> VehiclePaths;
	for (const APSVehicleAudio::FProfile& Profile : APSVehicleAudio::Profiles)
	{
		for (const TCHAR* Name : {Profile.Idle, Profile.Drive, Profile.Start})
			if (Name) VehiclePaths.Emplace(APSVehicleAudio::AssetPath(Name));
	}
	VehicleSoundsHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(VehiclePaths,
		FStreamableDelegate::CreateUObject(this, &UAPSAudioSubsystem::VehicleSoundsLoaded));
}

void UAPSAudioSubsystem::VehicleSoundsLoaded()
{
	for (const APSVehicleAudio::FProfile& Profile : APSVehicleAudio::Profiles)
	{
		for (const TCHAR* Name : {Profile.Idle, Profile.Drive, Profile.Start})
		{
			if (!Name) continue;
			const FSoftObjectPath Path(APSVehicleAudio::AssetPath(Name));
			USoundBase* Sound = Cast<USoundBase>(Path.ResolveObject());
			const bool bLoop = Name != Profile.Start;
			if (!Sound || Sound->IsLooping() != bLoop)
			{
				UE_LOG(LogAPSAudio, Warning, TEXT("Vehicle sound missing or invalid: %s"), *Path.ToString());
				continue;
			}
			VehicleSounds.Add(FName(Name), Sound);
			UGameplayStatics::PrimeSound(Sound);
		}
	}
	UE_LOG(LogAPSAudio, Log, TEXT("Vehicle audio ready: %d sounds"), VehicleSounds.Num());
}

void UAPSAudioSubsystem::BankLoaded()
{
	Bank = Cast<UAPSAudioBank>(BankPath.ResolveObject());
	if (!Bank)
	{
		UE_LOG(LogAPSAudio, Warning, TEXT("Audio bank is not installed: %s"), *BankPath.ToString());
		return;
	}
	if (!Bank->Mix || !Bank->MasterClass || !Bank->MusicClass || !Bank->AmbienceClass
		|| !Bank->EffectsClass || !Bank->UIClass)
	{
		UE_LOG(LogAPSAudio, Error, TEXT("Audio bank has incomplete mix routing; audio remains disabled."));
		Bank = nullptr;
		return;
	}
	for (USoundBase* Loop : {Bank->MenuMusic.Get(), Bank->ExplorationMusic.Get(), Bank->SpaceAmbience.Get(),
		Bank->InteriorAmbience.Get(), Bank->ShipIdleLoop.Get(), Bank->ShipThrustLoop.Get(), Bank->ShipWarpLoop.Get()})
	{
		if (Loop && !Loop->IsLooping())
		{
			UE_LOG(LogAPSAudio, Error, TEXT("Expected a seamless looping sound: %s. Audio bank rejected."), *Loop->GetPathName());
			Bank = nullptr;
			return;
		}
	}
	// Fill only missing surface sets; authored bank overrides remain authoritative.
	// This also works when the assets were imported before editor surface names
	// were registered. The shared async load above owns all waves before this callback.
	for (int32 SurfaceIndex = 0; SurfaceIndex < UE_ARRAY_COUNT(AdditionalFootstepSurfaces); ++SurfaceIndex)
	{
		const EPhysicalSurface Surface = static_cast<EPhysicalSurface>(SurfaceIndex + 3);
		const FAPSAudioFootsteps* Authored = Bank->SurfaceFootsteps.Find(Surface);
		if (Authored && !Authored->Sounds.IsEmpty()) continue;
		FAPSAudioFootsteps Set;
		for (int32 I = 1; I <= 5; ++I)
		{
			const TCHAR* Name = AdditionalFootstepSurfaces[SurfaceIndex];
			const FSoftObjectPath Path(FString::Printf(TEXT("/Game/APS/APS_ALPHA/Audio/Footsteps/SW_Step_%s_%02d.SW_Step_%s_%02d"), Name, I, Name, I));
			if (USoundBase* Sound = Cast<USoundBase>(Path.ResolveObject())) Set.Sounds.Add(Sound);
		}
		if (Set.Sounds.Num() != 5) UE_LOG(LogAPSAudio, Warning, TEXT("Incomplete %s footstep set: %d/5"), AdditionalFootstepSurfaces[SurfaceIndex], Set.Sounds.Num());
		if (!Set.Sounds.IsEmpty()) Bank->SurfaceFootsteps.Add(Surface, Set);
	}
	if (!Bank->SurfaceFootsteps.Contains(SurfaceType1)) Bank->SurfaceFootsteps.Add(SurfaceType1, Bank->DefaultFootsteps);
	if (!Bank->SurfaceFootsteps.Contains(SurfaceType2)) Bank->SurfaceFootsteps.Add(SurfaceType2, Bank->MetalFootsteps);
	// Request short contact sounds before the first walk, avoiding a cold
	// streaming request at the same moment as the first footstep.
	TSet<USoundBase*> ContactSounds;
	const auto AddContacts = [&ContactSounds](const FAPSAudioFootsteps& Set)
	{
		for (USoundBase* Sound : Set.Sounds) if (Sound) ContactSounds.Add(Sound);
	};
	AddContacts(Bank->DefaultFootsteps);
	AddContacts(Bank->MetalFootsteps);
	for (const auto& Surface : Bank->SurfaceFootsteps) AddContacts(Surface.Value);
	if (Bank->Landing) ContactSounds.Add(Bank->Landing);
	for (USoundBase* Sound : ContactSounds) UGameplayStatics::PrimeSound(Sound);
	// Queue the first music chunk before the first menu/gameplay playback request.
	if (Bank->MenuMusic) UGameplayStatics::PrimeSound(Bank->MenuMusic);
	if (Bank->ExplorationMusic) UGameplayStatics::PrimeSound(Bank->ExplorationMusic);
	// APS DEV 08.10 (same crash, independent diagnosis): keep the bank, its mix and classes alive for the whole game;
	// the audio device's override map holds raw pointers to them across world changes.
	if (!Bank->IsRooted()) Bank->AddToRoot();
	UGameplayStatics::PushSoundMixModifier(this, Bank->Mix);
	bMixPushed = true;
	ApplyMix();
	UE_LOG(LogAPSAudio, Log, TEXT("Audio bank ready: %s"), *Bank->GetPathName());
}

TStatId UAPSAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSAudioSubsystem, STATGROUP_Tickables);
}

void UAPSAudioSubsystem::ApplyMix()
{
	if (!Bank) return;
	const UAPSAudioPreferences* Settings = GetDefault<UAPSAudioPreferences>();
	const bool bEnabled = AudioEnabled.GetValueOnGameThread() != 0;
	USoundClass* Classes[] = {Bank->MasterClass, Bank->MusicClass, Bank->AmbienceClass, Bank->EffectsClass, Bank->UIClass};
	// Rio 06.10 ("make the sound settings work"): MASTER did nothing. SCL_Master lists no child classes
	// (SetParentClass in the asset commandlet does not add the child), so its override never reached a sound. Each
	// channel now carries MASTER itself; the master class keeps its own level for anything routed to it directly.
	const float Master = bEnabled ? Settings->Get(EAPSAudioChannel::Master) : 0.f;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Classes); ++Index)
	{
		const float Level = Index == 0 ? Master : Settings->Get(static_cast<EAPSAudioChannel>(Index)) * Master;
		if (!Classes[Index]) { UE_LOG(LogAPSAudio, Warning, TEXT("Audio bank: sound class %d unset; override skipped"), Index); continue; }
		UGameplayStatics::SetSoundMixClassOverride(this, Bank->Mix, Classes[Index],
			Level, 1.f, 0.1f, false);
	}
}

float UAPSAudioSubsystem::GetVolume(EAPSAudioChannel Channel) const
{
	return GetDefault<UAPSAudioPreferences>()->Get(Channel);
}

void UAPSAudioSubsystem::SetVolume(EAPSAudioChannel Channel, float Value)
{
	GetMutableDefault<UAPSAudioPreferences>()->Set(Channel, Value);
	ApplyMix();
}

void UAPSAudioSubsystem::SaveVolumes()
{
	GetMutableDefault<UAPSAudioPreferences>()->SaveConfig();
}

bool UAPSAudioSubsystem::ApplyButtonSounds(FButtonStyle& Style) const
{
	if (!Bank) return false;
	FSlateSound Click, Hover;
	if (AudioEnabled.GetValueOnGameThread() != 0)
	{
		Click.SetResourceObject(Bank->UIClick);
		Hover.SetResourceObject(Bank->UIHover);
	}
	Style.SetPressedSound(Click).SetHoveredSound(Hover);
	return true;
}

void UAPSAudioSubsystem::PlayConfirm()
{
	if (Bank) OneShot(Bank->UIConfirm, Bank->UIClass, 1.f, 1.f, true);
}

void UAPSAudioSubsystem::PlayBack()
{
	if (Bank) OneShot(Bank->UIBack, Bank->UIClass, 1.f, 1.f, true);
}

void UAPSAudioSubsystem::UpdateLoop(TObjectPtr<UAudioComponent>& Slot, USoundBase* Sound,
	USoundClass* Class, float Gain, float Pitch, float DeltaTime, bool bUI)
{
	if (!IsValid(Slot)) Slot = nullptr;
	if (Slot && !Slot->IsPlaying())
	{
		// A valid but stopped component must not leave a continuous layer silent.
		// Paused and virtualized components are still active and are preserved.
		Slot->DestroyComponent();
		Slot = nullptr;
	}
	if (Slot && (Slot->Sound != Sound || Gain <= 0.f))
	{
		Slot->FadeOut(0.7f, 0.f);
		OneShots.Add(Slot);
		Slot = nullptr;
	}
	if (!Sound || Gain <= 0.f) return;
	if (!Slot)
	{
		Slot = UGameplayStatics::CreateSound2D(this, Sound, 1.f, Pitch, 0.f, nullptr, false, true);
		if (!Slot) return;
		Slot->SoundClassOverride = Class;
		Slot->bIsUISound = bUI;
		Slot->bAllowSpatialization = false;
		Slot->FadeIn(1.2f, 1.f);
		Slot->SetVolumeMultiplier(0.f);
		if (Bank && Class == Bank->MusicClass)
			UE_LOG(LogAPSAudio, Log, TEXT("Music playback started: %s menu=%d"), *Sound->GetPathName(), bUI);
	}
	Slot->SetVolumeMultiplier(FMath::FInterpTo(Slot->VolumeMultiplier, FMath::Clamp(Gain, 0.f, 1.f), DeltaTime, 3.f));
	Slot->SetPitchMultiplier(FMath::FInterpTo(Slot->PitchMultiplier, FMath::Clamp(Pitch, 0.65f, 1.5f), DeltaTime, 3.f));
}

void UAPSAudioSubsystem::OneShot(USoundBase* Sound, USoundClass* Class, float Gain, float Pitch, bool bUI)
{
	if (!Sound || AudioEnabled.GetValueOnGameThread() == 0 || OneShots.Num() >= 12) return;
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(this, Sound, Gain, Pitch, 0.f, nullptr, false, true);
	if (!Component) return;
	Component->SoundClassOverride = Class;
	Component->bIsUISound = bUI;
	Component->Play();
	OneShots.Add(Component);
}

void UAPSAudioSubsystem::UnbindFootPose()
{
	if (USkeletalMeshComponent* Mesh = FootPoseMesh.Get())
		Mesh->UnregisterOnBoneTransformsFinalizedDelegate(FootPoseHandle);
	FootPoseMesh.Reset();
	FootPoseHandle.Reset();
	PoseContacts.Reset();
	LastFootPoseFrame = MAX_uint64;
}

void UAPSAudioSubsystem::OnFootPoseFinalized()
{
	// Finalized poses are on the game thread, after animation blending and IK.
	if (!Bank || bStopped || !GetWorld() || GetWorld()->IsPaused()
		|| AudioEnabled.GetValueOnGameThread() == 0 || LastFootPoseFrame == GFrameCounter) return;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(ObservedPawn.Get());
	if (!PC || !PC->IsLocalController() || !Character || PC->GetPawn() != Character) return;
	LastFootPoseFrame = GFrameCounter;
	UpdateFootsteps(Character, GetWorld()->GetDeltaSeconds(), true);
}

void UAPSAudioSubsystem::ResetPawnState(APawn* Pawn)
{
	UnbindFootPose();
	ObservedPawn = Pawn;
	if (ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pawn))
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Mesh && Mesh->DoesSocketExist(TEXT("root")) && Mesh->DoesSocketExist(TEXT("foot_l"))
			&& Mesh->DoesSocketExist(TEXT("foot_r")) && Mesh->DoesSocketExist(TEXT("ball_l"))
			&& Mesh->DoesSocketExist(TEXT("ball_r")))
		{
			FootPoseMesh = Mesh;
			FootPoseHandle = Mesh->RegisterOnBoneTransformsFinalizedDelegate(
				FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &UAPSAudioSubsystem::OnFootPoseFinalized));
		}
	}
	Cadence = {};
	AirTime = SmoothedThrust = 0.f;
	bWasGrounded = false;
	const ASpaceship* Ship = Cast<ASpaceship>(Pawn);
	PreviousShipSpeed = Ship ? Ship->GetShipSpeedMetersPerSecond() : 0.0;
	bPreviousEngine = Ship && !Ship->IsGroundVehicle() && Ship->GetEngineRunning();
	PreviousBand = Ship && Ship->FlightModel ? static_cast<int32>(Ship->FlightModel->GetFlightBand()) : INDEX_NONE;
}

void UAPSAudioSubsystem::UpdateShip(ASpaceship* Ship, float DeltaTime)
{
	const bool bRunning = Ship && Ship->GetEngineRunning();
	if (Ship && bRunning != bPreviousEngine)
		OneShot(bRunning ? Bank->EngineStart : Bank->EngineStop, Bank->EffectsClass);
	bPreviousEngine = bRunning;
	const UAPSShipFlightModel* Flight = Ship ? Ship->FlightModel : nullptr;
	const int32 Band = Flight && Flight->IsBandFlightActive() ? static_cast<int32>(Flight->GetFlightBand()) : INDEX_NONE;
	if (bRunning && PreviousBand != INDEX_NONE && Band != INDEX_NONE && PreviousBand != Band)
		OneShot(Bank->FlightModeChange, Bank->EffectsClass, 0.6f);
	PreviousBand = Band;
	const double RawLimit = Flight ? Flight->GetCurrentSpeedLimit() : 1600.0;
	const double Limit = FMath::IsFinite(RawLimit) ? FMath::Max(1.0, RawLimit) : 1600.0;
	const double RawSpeed = Ship ? Ship->GetShipSpeedMetersPerSecond() : 0.0;
	const double Speed = FMath::IsFinite(RawSpeed) ? FMath::Max(0.0, RawSpeed) : 0.0;
	const float SpeedRatio = static_cast<float>(FMath::Clamp(Speed / Limit, 0.0, 1.0));
	// Observe speed, acceleration and boost/brake state without changing the shared flight implementation.
	const float TargetThrust = bRunning ? APSAudioPlayback::EngineLoad(Speed, PreviousShipSpeed, Limit,
		DeltaTime, Ship->bIsAccelerating, Ship->bIsDecelerating) : 0.f;
	PreviousShipSpeed = Speed;
	SmoothedThrust = FMath::FInterpTo(SmoothedThrust, TargetThrust, DeltaTime, 3.f);
	UpdateLoop(EngineIdle, Bank->ShipIdleLoop, Bank->EffectsClass, bRunning ? 0.45f : 0.f, 0.85f + 0.2f * SpeedRatio, DeltaTime);
	UpdateLoop(EngineThrust, Bank->ShipThrustLoop, Bank->EffectsClass,
		bRunning ? 0.05f + 0.8f * SmoothedThrust : 0.f, 0.8f + 0.55f * SmoothedThrust, DeltaTime);
	UpdateLoop(EngineWarp, Bank->ShipWarpLoop, Bank->EffectsClass,
		bRunning && Band >= static_cast<int32>(EAPSFlightBand::Cruise) ? 0.2f + 0.45f * SpeedRatio : 0.f,
		0.85f + 0.25f * SpeedRatio, DeltaTime);
}

void UAPSAudioSubsystem::UpdateGroundVehicle(ASpaceship* Vehicle, float DeltaTime)
{
	const APSVehicleAudio::FProfile* Profile = APSVehicleAudio::FindProfile(Vehicle->GetGroundVehicleKind());
	if (!Profile)
	{
		UpdateShip(nullptr, DeltaTime);
		return;
	}
	const auto Sound = [this](const TCHAR* Name) -> USoundBase*
	{
		return Name ? VehicleSounds.FindRef(FName(Name)).Get() : nullptr;
	};
	const bool bRunning = Vehicle->GetEngineRunning() && Vehicle->HasPilot();
	if (bRunning && !bPreviousEngine) OneShot(Sound(Profile->Start), Bank->EffectsClass, .7f);
	bPreviousEngine = bRunning;
	PreviousBand = INDEX_NONE;
	APSVehicleAudio::FInput Input;
	Input.Kind = Profile->Kind;
	Input.bPowered = bRunning;
	Input.Translation = Vehicle->GetPilotTranslationInput();
	Input.bBoost = Vehicle->bIsAccelerating;
	Input.bBrake = Vehicle->bIsDecelerating;
	Input.SpeedMetersPerSecond = Vehicle->GetShipSpeedMetersPerSecond();
	static const auto* SpeedScale = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Vehicle.SpeedScale"));
	Input.SpeedScale = SpeedScale ? SpeedScale->GetFloat() : 1.f;
	const APSVehicleAudio::FMix Mix = APSVehicleAudio::Resolve(Input);
	// Separate recordings; the existing slots supply smooth fades on engine-off, exit and vehicle changes.
	UpdateLoop(EngineIdle, Sound(Profile->Idle), Bank->EffectsClass, Mix.IdleGain, Mix.IdlePitch, DeltaTime);
	UpdateLoop(EngineThrust, Sound(Profile->Drive), Bank->EffectsClass, Mix.DriveGain, Mix.DrivePitch, DeltaTime);
	UpdateLoop(EngineWarp, nullptr, Bank->EffectsClass, 0.f, 1.f, DeltaTime);
}

void UAPSAudioSubsystem::UpdateFootsteps(ACustomGravityCharacter* Character, float DeltaTime, bool bFromPose)
{
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || Character->IsSurfaceHandoffSuspended() || Character->bIsZeroG)
	{
		Cadence = {};
		PoseContacts.Reset();
		AirTime = 0.f;
		bWasGrounded = false;
		return;
	}
	const bool bGrounded = Movement && Movement->IsMovingOnGround() && Movement->CurrentFloor.IsWalkableFloor()
		&& !Character->bIsZeroG && !Character->IsSurfaceHandoffSuspended();
	if (!bGrounded)
	{
		AirTime += DeltaTime;
		Cadence.Advance(false, 0.f, DeltaTime);
		PoseContacts.Reset();
		bWasGrounded = false;
		return;
	}
	const bool bLandingContact = !bWasGrounded && AirTime > 0.25f;
	if (bLandingContact)
	{
		Cadence.OnLanding();
		PoseContacts.Reset(.16f);
	}
	AirTime = 0.f;
	bWasGrounded = true;
	const float Speed = static_cast<float>(FVector::VectorPlaneProject(Movement->Velocity, Character->GetGravityUpVector()).Size());
	bool bStepContact = false;
	FVector ContactLocation = Movement->CurrentFloor.HitResult.ImpactPoint;
	if (bFromPose && FootPoseMesh.IsValid())
	{
		const USkeletalMeshComponent* Mesh = FootPoseMesh.Get();
		const double RootZ = Mesh->GetSocketTransform(TEXT("root"), RTS_Component).GetLocation().Z;
		const auto SoleHeight = [Mesh, RootZ](const TCHAR* Foot, const TCHAR* Ball)
		{
			// Quinn's ankle is 8 cm above its sole. Use the lower heel/toe so
			// rolling onto the toe cannot emit a second step during stance.
			return static_cast<float>(FMath::Min(
				Mesh->GetSocketTransform(Foot, RTS_Component).GetLocation().Z - RootZ - 8.0,
				Mesh->GetSocketTransform(Ball, RTS_Component).GetLocation().Z - RootZ));
		};
		const int32 Foot = PoseContacts.Advance(bGrounded, Speed,
			SoleHeight(TEXT("foot_l"), TEXT("ball_l")), SoleHeight(TEXT("foot_r"), TEXT("ball_r")), DeltaTime);
		bStepContact = Foot != INDEX_NONE;
		if (bStepContact) ContactLocation = Mesh->GetSocketLocation(Foot == 0 ? TEXT("foot_l") : TEXT("foot_r"));
	}
	else bStepContact = Cadence.Advance(bGrounded, Speed, DeltaTime);
	if (!bLandingContact && !bStepContact) return;
	FHitResult Contact = Movement->CurrentFloor.HitResult;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(APSFootContact), true, Character);
	Query.bReturnPhysicalMaterial = true;
	Query.bReturnFaceIndex = true;
	FHitResult UnderFoot;
	const FVector Up = Character->GetGravityUpVector();
	if (GetWorld()->LineTraceSingleByChannel(UnderFoot, ContactLocation + Up * 25.f,
		ContactLocation - Up * 65.f, ECC_Visibility, Query)
		&& FVector::DotProduct(UnderFoot.ImpactNormal, Up) > .35f) Contact = UnderFoot;
	const FAPSAudioFootsteps* Set = &APSAudioPlayback::ResolveFootsteps(Contact, *Bank);
	if (bLandingContact && Set == &Bank->DefaultFootsteps && Bank->Landing)
	{
		OneShot(Bank->Landing, Bank->EffectsClass, 0.5f);
		return;
	}
	if (Set->Sounds.IsEmpty()) return;
	int32 Index = AudioRandom.RandRange(0, Set->Sounds.Num() - 1);
	if (Set->Sounds.Num() > 1 && Set->Sounds[Index] == LastFootstep.Get()) Index = (Index + 1) % Set->Sounds.Num();
	LastFootstep = Set->Sounds[Index];
	OneShot(Set->Sounds[Index], Bank->EffectsClass,
		bLandingContact ? 0.5f : AudioRandom.FRandRange(0.65f, 0.8f),
		bLandingContact ? 0.95f : AudioRandom.FRandRange(0.98f, 1.02f));
}

void UAPSAudioSubsystem::Tick(float DeltaTime)
{
	const double RealTime = FPlatformTime::Seconds();
	if (RealTime >= NextSettingsScanTime)
	{
		NextSettingsScanTime = RealTime + 0.25;
		UAPSAudioSettingsBridge::AttachWorldPanels(GetWorld());
	}
	// Slate settings keep working while paused; movement audio stays suspended.
	if (GetWorld()->IsPaused()) return;
	if (!Bank || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return;
	const bool bEnabled = AudioEnabled.GetValueOnGameThread() != 0;
	if (!bEnabled)
	{
		if (!bStopped) { StopAll(); ApplyMix(); bStopped = true; }
		return;
	}
	if (bStopped) { ApplyMix(); ResetPawnState(nullptr); bStopped = false; }
	OneShots.RemoveAll([](const TObjectPtr<UAudioComponent>& C) { return !IsValid(C) || !C->IsPlaying(); });
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController()) { StopAll(); return; }
	APawn* Pawn = PC->GetPawn();
	if (ObservedPawn.Get() != Pawn) ResetPawnState(Pawn);
	const bool bMenu = Cast<AMainMenuController>(PC) != nullptr;
	ASpaceship* Ship = bMenu ? nullptr : Cast<ASpaceship>(Pawn);
	ACustomGravityCharacter* Character = bMenu ? nullptr : Cast<ACustomGravityCharacter>(Pawn);
	const bool bInterior = Ship || (Character && (Cast<ASpaceStation>(Character->GravityTarget) || Cast<ASpaceship>(Character->GravityTarget)));
	const bool bSpace = Ship ? Ship->CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace
		: Character && Character->bIsZeroG;
	UpdateLoop(Music, bMenu ? Bank->MenuMusic : (Pawn ? Bank->ExplorationMusic : nullptr), Bank->MusicClass, 1.f, 1.f, DeltaTime, bMenu);
	USoundBase* Background = bInterior ? Bank->InteriorAmbience.Get() : (bSpace ? Bank->SpaceAmbience.Get() : nullptr);
	UpdateLoop(Ambience, Background, Bank->AmbienceClass, bMenu ? 0.35f : 0.65f, 1.f, DeltaTime, bMenu);
	if (Ship && Ship->IsGroundVehicle()) UpdateGroundVehicle(Ship, DeltaTime);
	else UpdateShip(Ship, DeltaTime);
	// A supported visible skeleton emits from its finalized pose, never from distance.
	if (Character && !FootPoseMesh.IsValid()) UpdateFootsteps(Character, DeltaTime);
}

void UAPSAudioSubsystem::StopAll()
{
	for (UAudioComponent* Component : {Music.Get(), Ambience.Get(), EngineIdle.Get(), EngineThrust.Get(), EngineWarp.Get()})
		if (IsValid(Component)) Component->Stop();
	for (UAudioComponent* Component : OneShots) if (IsValid(Component)) Component->Stop();
	Music = Ambience = EngineIdle = EngineThrust = EngineWarp = nullptr;
	OneShots.Reset();
}

void UAPSAudioSubsystem::Deinitialize()
{
	UnbindFootPose();
	if (BankHandle) { BankHandle->CancelHandle(); BankHandle.Reset(); }
	if (VehicleSoundsHandle) { VehicleSoundsHandle->CancelHandle(); VehicleSoundsHandle.Reset(); }
	StopAll();
	VehicleSounds.Reset();
	if (Bank && bMixPushed)
	{
		// Planets-v2 08.10 (crash in both 0.6.3 builds on entering L_WorldGeneration): the class overrides are keyed by
		// raw class pointers inside the audio device and outlive this world subsystem, its bank and its classes; the
		// next world's ApplyMix adds new entries beside the stale ones and FAudioDevice::ApplyClassAdjusters reads freed
		// memory (FName::ToString, audio worker). Clear every override before the pop.
		for (USoundClass* Class : {Bank->MasterClass.Get(), Bank->MusicClass.Get(), Bank->AmbienceClass.Get(), Bank->EffectsClass.Get(), Bank->UIClass.Get()})
		{
			if (Class) UGameplayStatics::ClearSoundMixClassOverride(this, Bank->Mix, Class, 0.f);
		}
		UGameplayStatics::PopSoundMixModifier(this, Bank->Mix);
	}
	bMixPushed = false;
	Bank = nullptr;
	Super::Deinitialize();
}
