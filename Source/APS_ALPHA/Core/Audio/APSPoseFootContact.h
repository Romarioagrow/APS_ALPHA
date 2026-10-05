#pragma once
#include <cmath>

namespace APSAudioPlayback
{
	/** Contact from the evaluated boot pose, in skeletal component centimetres.
	 * A foot must lift before it can land again; planted-foot jitter stays silent. */
	struct FPoseFootContact
	{
		bool Armed[2] = {false, false};
		float Cooldown = 0.f;

		void Reset(float QuietTime = 0.f)
		{
			Armed[0] = Armed[1] = false;
			Cooldown = QuietTime;
		}

		// Returns the contacting foot (0 left, 1 right), or -1.
		int Advance(bool Grounded, float Speed, float LeftHeight, float RightHeight, float Dt)
		{
			if (!std::isfinite(Dt) || Dt <= 0.f) return -1;
			Cooldown = Cooldown > Dt ? Cooldown - Dt : 0.f;
			if (!Grounded || !std::isfinite(Speed) || Speed < 20.f
				|| !std::isfinite(LeftHeight) || !std::isfinite(RightHeight) || Dt > .15f)
			{
				Reset(Cooldown);
				return -1;
			}
			const float Heights[2] = {LeftHeight, RightHeight};
			int Contact = -1;
			for (int Foot = 0; Foot < 2; ++Foot)
			{
				if (Heights[Foot] > 5.f) Armed[Foot] = true;
				if (Armed[Foot] && Heights[Foot] <= 2.f)
				{
					Armed[Foot] = false;
					if (Cooldown <= 0.f && (Contact < 0 || Heights[Foot] < Heights[Contact])) Contact = Foot;
				}
			}
			if (Contact >= 0) Cooldown = .10f;
			return Contact;
		}
	};
}
