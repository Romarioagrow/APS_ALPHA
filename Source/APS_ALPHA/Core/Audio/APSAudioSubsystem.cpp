#include "APSAudioSubsystem.h"

#include "APSAudioBank.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSAudio, Log, All);

namespace
{
	TAutoConsoleVariable<int32> AudioEnabled(TEXT("aps.Audio.Enabled"), 1,
		TEXT("Enable the APOSFERA audio layer. 0 stops its sounds immediately."));
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
	BankHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(BankPath,
		FStreamableDelegate::CreateUObject(this, &UAPSAudioSubsystem::BankLoaded));
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
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Classes); ++Index)
	{
		const float Level = Index == 0 && !bEnabled ? 0.f : Settings->Get(static_cast<EAPSAudioChannel>(Index));
		UGameplayStatics::SetSoundMixClassOverride(this, Bank->Mix, Classes[Index],
			Level, 1.f, 0.1f, Index == 0);
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

void UAPSAudioSubsystem::ResetPawnState(APawn* Pawn)
{
	ObservedPawn = Pawn;
	Cadence = {};
	AirTime = SmoothedThrust = 0.f;
	bWasGrounded = false;
	const ASpaceship* Ship = Cast<ASpaceship>(Pawn);
	PreviousShipSpeed = Ship ? Ship->GetShipSpeedMetersPerSecond() : 0.0;
	bPreviousEngine = Ship && Ship->GetEngineRunning();
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

void UAPSAudioSubsystem::UpdateFootsteps(ACustomGravityCharacter* Character, float DeltaTime)
{
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || Character->IsSurfaceHandoffSuspended() || Character->bIsZeroG)
	{
		Cadence = {};
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
		bWasGrounded = false;
		return;
	}
	if (!bWasGrounded && AirTime > 0.25f)
	{
		OneShot(Bank->Landing, Bank->EffectsClass, 0.5f);
		Cadence.OnLanding();
	}
	AirTime = 0.f;
	bWasGrounded = true;
	const float Speed = static_cast<float>(FVector::VectorPlaneProject(Movement->Velocity, Character->GetGravityUpVector()).Size());
	if (!Cadence.Advance(bGrounded, Speed, DeltaTime)) return;
	const FHitResult& Floor = Movement->CurrentFloor.HitResult;
	const FAPSAudioFootsteps* Set = &Bank->DefaultFootsteps;
	const EPhysicalSurface Surface = UPhysicalMaterial::DetermineSurfaceType(Floor.PhysMaterial.Get());
	if (const FAPSAudioFootsteps* BySurface = Bank->SurfaceFootsteps.Find(Surface)) Set = BySurface;
	else if (Cast<ASpaceship>(Floor.GetActor()) || Cast<ASpaceStation>(Floor.GetActor())
		|| Cast<ASpaceship>(Character->GravityTarget) || Cast<ASpaceStation>(Character->GravityTarget)) Set = &Bank->MetalFootsteps;
	if (Set->Sounds.IsEmpty()) Set = &Bank->DefaultFootsteps;
	if (Set->Sounds.IsEmpty()) return;
	int32 Index = AudioRandom.RandRange(0, Set->Sounds.Num() - 1);
	if (Set->Sounds.Num() > 1 && Set->Sounds[Index] == LastFootstep.Get()) Index = (Index + 1) % Set->Sounds.Num();
	LastFootstep = Set->Sounds[Index];
	OneShot(Set->Sounds[Index], Bank->EffectsClass, AudioRandom.FRandRange(0.65f, 0.8f), AudioRandom.FRandRange(0.95f, 1.05f));
}

void UAPSAudioSubsystem::Tick(float DeltaTime)
{
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
	UpdateShip(Ship, DeltaTime);
	if (Character) UpdateFootsteps(Character, DeltaTime);
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
	if (BankHandle) { BankHandle->CancelHandle(); BankHandle.Reset(); }
	StopAll();
	if (Bank && bMixPushed) UGameplayStatics::PopSoundMixModifier(this, Bank->Mix);
	bMixPushed = false;
	Bank = nullptr;
	Super::Deinitialize();
}
