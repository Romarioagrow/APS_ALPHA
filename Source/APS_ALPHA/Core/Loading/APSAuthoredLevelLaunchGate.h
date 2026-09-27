#pragma once

/** Game-thread state machine: poll readiness, never join the asset workers. */
struct FAPSAuthoredLevelLaunchGate
{
	enum class EPhase { Idle, Loading, Compiling, Draining, Opening, Failed };
	EPhase Phase{EPhase::Idle};
	double StartedAt{0.0};
	double OpeningAt{0.0};
	int QuietPolls{0};

	bool IsPending() const
	{
		return Phase == EPhase::Loading || Phase == EPhase::Compiling
			|| Phase == EPhase::Draining || Phase == EPhase::Opening;
	}
	bool Begin(double Now)
	{
		if (IsPending()) return false;
		StartedAt = Now;
		OpeningAt = 0.0;
		QuietPolls = 0;
		Phase = EPhase::Loading;
		return true;
	}
	void Cancel() { Phase = EPhase::Idle; QuietPolls = 0; }
	void Fail() { Phase = EPhase::Failed; QuietPolls = 0; }
	bool CompleteOpening()
	{
		if (Phase != EPhase::Opening) return false;
		Cancel();
		return true;
	}

	/** Returns true exactly once after two consecutive ready polls. Opening also
	 * has a deadline; a lost travel notification must not leave the menu locked. */
	bool Advance(double Now, bool bLoaded, bool bLoadFailed, int RemainingAssets,
		bool bPreviewDrained, double TimeoutSeconds = 600.0, double TravelTimeoutSeconds = 60.0)
	{
		if (!IsPending()) return false;
		if (Phase == EPhase::Opening)
		{
			if (Now - OpeningAt >= TravelTimeoutSeconds) Fail();
			return false;
		}
		if (bLoadFailed || Now - StartedAt >= TimeoutSeconds) { Fail(); return false; }
		if (!bLoaded) { Phase = EPhase::Loading; QuietPolls = 0; return false; }
		if (RemainingAssets > 0) { Phase = EPhase::Compiling; QuietPolls = 0; return false; }
		Phase = EPhase::Draining;
		if (!bPreviewDrained) { QuietPolls = 0; return false; }
		if (++QuietPolls < 2) return false;
		OpeningAt = Now;
		Phase = EPhase::Opening;
		return true;
	}
};
