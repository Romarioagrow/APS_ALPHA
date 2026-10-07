#pragma once
#include "CoreMinimal.h"

namespace APSAudioPlayback
{
	/** Cadence uses locomotion velocity, never absolute position: floating-origin shifts cannot make footsteps. */
	struct FFootstepCadence
	{
		float Distance = 0.f;
		float Cooldown = 0.f;
		bool bHasStepped = false;

		void OnLanding()
		{
			// The landing sound already represents a contact; wait a regular stride.
			Distance = 0.f;
			Cooldown = 0.16f;
			bHasStepped = true;
		}

		bool Advance(bool Grounded, float TangentialSpeed, float DeltaTime)
		{
			if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return false;
			Cooldown = FMath::Max(0.f, Cooldown - DeltaTime);
			if (!Grounded || !FMath::IsFinite(TangentialSpeed) || TangentialSpeed < 20.f)
			{
				Distance = 0.f;
				bHasStepped = false;
				return false;
			}
			const float Stride = FMath::Clamp(65.f + TangentialSpeed * 0.12f, 70.f, 210.f);
			// The first contact should not wait a full stride from rest. A small
			// travel threshold avoids sounds from idle velocity noise or tiny taps.
			const float StepDistance = bHasStepped ? Stride : 10.f;
			Distance += TangentialSpeed * FMath::Min(DeltaTime, 0.1f);
			if (Distance < StepDistance || Cooldown > 0.f) return false;
			// Keep the fractional stride for stable timing at different frame rates,
			// but discard complete missed strides so a hitch cannot queue a burst.
			Distance = FMath::Fmod(Distance, StepDistance);
			bHasStepped = true;
			Cooldown = 0.16f;
			return true;
		}
	};

	inline float EngineLoad(double Speed, double PreviousSpeed, double Limit, float DeltaTime, bool Boost, bool Brake)
	{
		if (!FMath::IsFinite(Speed) || !FMath::IsFinite(PreviousSpeed) || !FMath::IsFinite(Limit)
			|| !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return 0.f;
		const double SafeLimit = FMath::Max(1.0, Limit);
		const double Cruise = FMath::Clamp(Speed / SafeLimit, 0.0, 1.0) * 0.45;
		const double Acceleration = DeltaTime > 0.f
			? FMath::Clamp(FMath::Abs(Speed - PreviousSpeed) / (FMath::Max(SafeLimit * 0.25, 10.0) * DeltaTime), 0.0, 1.0)
			: 0.0;
		return static_cast<float>(FMath::Max3(Cruise, Acceleration, Boost || Brake ? 0.85 : 0.0));
	}
}
