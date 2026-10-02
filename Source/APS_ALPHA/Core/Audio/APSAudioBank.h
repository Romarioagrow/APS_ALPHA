#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Chaos/ChaosEngineInterface.h"
#include "APSAudioBank.generated.h"

class USoundBase;
class USoundClass;
class USoundMix;

USTRUCT(BlueprintType)
struct FAPSAudioFootsteps
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TArray<TObjectPtr<USoundBase>> Sounds;
};

/** Small, curated bank. Marketplace source assets stay in their original folders. */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSAudioBank : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundMix> Mix;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundClass> MasterClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundClass> MusicClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundClass> AmbienceClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundClass> EffectsClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundClass> UIClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interface") TObjectPtr<USoundBase> UIClick;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interface") TObjectPtr<USoundBase> UIHover;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interface") TObjectPtr<USoundBase> UIConfirm;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interface") TObjectPtr<USoundBase> UIBack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music") TObjectPtr<USoundBase> MenuMusic;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music") TObjectPtr<USoundBase> ExplorationMusic;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ambience") TObjectPtr<USoundBase> SpaceAmbience;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ambience") TObjectPtr<USoundBase> InteriorAmbience;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> ShipIdleLoop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> ShipThrustLoop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> ShipWarpLoop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> EngineStart;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> EngineStop;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ship") TObjectPtr<USoundBase> FlightModeChange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footsteps") FAPSAudioFootsteps DefaultFootsteps;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footsteps") FAPSAudioFootsteps MetalFootsteps;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footsteps")
	TMap<TEnumAsByte<EPhysicalSurface>, FAPSAudioFootsteps> SurfaceFootsteps;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footsteps") TObjectPtr<USoundBase> Landing;
};
