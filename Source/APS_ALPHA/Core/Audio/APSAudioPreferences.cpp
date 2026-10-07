#include "APSAudioPreferences.h"

float UAPSAudioPreferences::Get(EAPSAudioChannel Channel) const
{
	float Value = 0.f;
	switch (Channel)
	{
	case EAPSAudioChannel::Master: Value = Master; break;
	case EAPSAudioChannel::Music: Value = Music; break;
	case EAPSAudioChannel::Ambience: Value = Ambience; break;
	case EAPSAudioChannel::Effects: Value = Effects; break;
	case EAPSAudioChannel::Interface: Value = Interface; break;
	}
	return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
}

void UAPSAudioPreferences::Set(EAPSAudioChannel Channel, float Value)
{
	Value = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
	switch (Channel)
	{
	case EAPSAudioChannel::Master: Master = Value; break;
	case EAPSAudioChannel::Music: Music = Value; break;
	case EAPSAudioChannel::Ambience: Ambience = Value; break;
	case EAPSAudioChannel::Effects: Effects = Value; break;
	case EAPSAudioChannel::Interface: Interface = Value; break;
	}
}
