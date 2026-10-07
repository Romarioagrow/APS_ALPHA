#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSAudioPreferences.h"
#include "APSAudioPlaybackPolicy.h"
#include "APSPoseFootContact.h"
#include "APSAudioSubsystem.generated.h"

class ACustomGravityCharacter;
class APawn;
class ASpaceship;
class UAPSAudioBank;
class UAudioComponent;
class USoundBase;
class USoundClass;
class USkeletalMeshComponent;
struct FButtonStyle;
struct FStreamableHandle;

/** Local presentation only: observes the existing pawn and never changes movement or world data. */
UCLASS()
class APS_ALPHA_API UAPSAudioSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintPure, Category="Audio") bool IsReady() const { return Bank != nullptr; }
	UFUNCTION(BlueprintCallable, Category="Audio") void SetVolume(EAPSAudioChannel Channel, float Value);
	UFUNCTION(BlueprintPure, Category="Audio") float GetVolume(EAPSAudioChannel Channel) const;
	UFUNCTION(BlueprintCallable, Category="Audio") void SaveVolumes();
	UFUNCTION(BlueprintCallable, Category="Audio") void PlayConfirm();
	UFUNCTION(BlueprintCallable, Category="Audio") void PlayBack();
	bool ApplyButtonSounds(FButtonStyle& Style) const;

private:
	void BankLoaded();
	void VehicleSoundsLoaded();
	void ApplyMix();
	void StopAll();
	void UpdateLoop(TObjectPtr<UAudioComponent>& Slot, USoundBase* Sound, USoundClass* Class,
		float Gain, float Pitch, float DeltaTime, bool bUI = false);
	void OneShot(USoundBase* Sound, USoundClass* Class, float Gain = 1.f, float Pitch = 1.f, bool bUI = false);
	void UpdateShip(ASpaceship* Ship, float DeltaTime);
	void UpdateGroundVehicle(ASpaceship* Vehicle, float DeltaTime);
	void UpdateFootsteps(ACustomGravityCharacter* Character, float DeltaTime, bool bFromPose = false);
	void OnFootPoseFinalized();
	void UnbindFootPose();
	void ResetPawnState(APawn* Pawn);

	UPROPERTY(Transient) TObjectPtr<UAPSAudioBank> Bank;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Ambience;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> EngineIdle;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> EngineThrust;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> EngineWarp;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> OneShots;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<USoundBase>> VehicleSounds;
	TSharedPtr<FStreamableHandle> BankHandle;
	TSharedPtr<FStreamableHandle> VehicleSoundsHandle;
	TWeakObjectPtr<APawn> ObservedPawn;
	TWeakObjectPtr<USoundBase> LastFootstep;
	APSAudioPlayback::FFootstepCadence Cadence;
	APSAudioPlayback::FPoseFootContact PoseContacts;
	TWeakObjectPtr<USkeletalMeshComponent> FootPoseMesh;
	FDelegateHandle FootPoseHandle;
	uint64 LastFootPoseFrame = MAX_uint64;
	double PreviousShipSpeed = 0.0;
	float AirTime = 0.f;
	float SmoothedThrust = 0.f;
	double NextSettingsScanTime = 0.0;
	int32 PreviousBand = INDEX_NONE;
	bool bPreviousEngine = false;
	bool bWasGrounded = false;
	bool bMixPushed = false;
	bool bStopped = false;
	FRandomStream AudioRandom{51423};
};
