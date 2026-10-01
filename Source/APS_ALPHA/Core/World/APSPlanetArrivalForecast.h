#pragma once

#include "CoreMinimal.h"

/**
 * Which planet or moon the observer is flying to, so that its surface starts building before the observer gets there
 * (Rio, 01.10: "planets do not load in time when I fly up fast; switch the WorldScape on in advance"). A surface takes
 * about 2-11 s to build, while a ship's approach (its speed tied to the distance to the surface) crosses the
 * distance-based preload radius in about 6 s. The forecast reads the observer's motion relative to each body: it
 * closes in, its straight line passes near the body, and at the present closing speed the surface is less than the
 * lead time away. A real approach slows down near the body, so the lead is a lower bound.
 */
namespace APSPlanetArrivalForecast
{
	struct FResult
	{
		/** Seconds to the surface at the present closing speed; the maximum when the observer is not closing in. */
		double SecondsToSurface{TNumericLimits<double>::Max()};
		/** How close the straight line passes to the body's centre (cm); the maximum when not closing in. */
		double MissDistanceCm{TNumericLimits<double>::Max()};
		double DistanceCm{0.0};
		double ClosingSpeedCmPerSecond{0.0};
	};

	/** RelativeNow and RelativeBefore are the observer's position minus the body's, StepSeconds apart. */
	APS_ALPHA_API FResult Evaluate(const FVector& RelativeNow, const FVector& RelativeBefore, double StepSeconds,
		double BodyRadiusCm);

	/**
	 * True when the observer arrives within LeadSeconds: the line passes within ReachCm of the centre or within a
	 * tenth of the distance (a narrow cone ahead), and the surface is within the lead. bCurrent loosens both by half
	 * again, so that a forecast which holds does not flicker at the edge.
	 */
	APS_ALPHA_API bool IsArriving(const FResult& Result, double LeadSeconds, double ReachCm, bool bCurrent);

	/**
	 * Confirms forecasts over passes: a new body must be the best on two passes in a row (a course, not a jump, a
	 * possession change or a turn), and the confirmed one stays while it still holds, or through a single miss.
	 */
	template <typename KeyType>
	struct TTracker
	{
		KeyType Confirmed{};
		KeyType Candidate{};
		int32 Misses{0};

		/** Best: this pass's best arriving body, or empty; bConfirmedHolds: the confirmed one still arrives. Returns
		 * true when the confirmed body changed. */
		bool Step(const KeyType& Best, const bool bConfirmedHolds)
		{
			const KeyType Empty{};
			if (Confirmed != Empty && bConfirmedHolds)
			{
				Misses = 0;
				Candidate = Empty;
				return false;
			}
			bool bChanged = false;
			if (Confirmed != Empty)
			{
				if (++Misses < 2)
				{
					return false;
				}
				Confirmed = Empty;
				Misses = 0;
				bChanged = true;
			}
			if (Best != Empty && Best == Candidate)
			{
				Confirmed = Best;
				Candidate = Empty;
				return true;
			}
			Candidate = Best;
			return bChanged;
		}

		void Reset()
		{
			Confirmed = KeyType{};
			Candidate = KeyType{};
			Misses = 0;
		}
	};
}
