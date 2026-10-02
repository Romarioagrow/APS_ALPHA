#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "APSAudioPreferences.generated.h"

UENUM(BlueprintType)
enum class EAPSAudioChannel : uint8 { Master, Music, Ambience, Effects, Interface };

/** Per-user audio settings, independent of generated-world saves. */
UCLASS(Config=APSAudioUser)
class APS_ALPHA_API UAPSAudioPreferences : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Config) float Master = 0.8f;
	UPROPERTY(Config) float Music = 0.35f;
	UPROPERTY(Config) float Ambience = 0.4f;
	UPROPERTY(Config) float Effects = 0.65f;
	UPROPERTY(Config) float Interface = 0.5f;

	float Get(EAPSAudioChannel Channel) const;
	void Set(EAPSAudioChannel Channel, float Value);
};
