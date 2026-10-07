#include "APSPlanetEnvironmentStreamingSubsystem.h"

#include "APSPlanetArrivalForecast.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSPlanetArrival, Log, All);

namespace APSPlanetArrivalPrivate
{
	TAutoConsoleVariable<float> CVarArrivalLeadSeconds(
		TEXT("aps.Surface.ArrivalLeadSeconds"), 20.0f,
		TEXT("Start the surface of the planet or moon the observer flies to this many seconds before it arrives on its ")
		TEXT("present course and speed, beyond the distance-based preload radius (0 = off). Surfaces take about 2-11 s ")
		TEXT("to build."),
		ECVF_Default);
	/**
	 * Rio 05.10 evening (flight FPS, the way home from 2 ly): a ship whose speed is tied to its distance always has the
	 * same few seconds to go at its present speed, so the forecast held from a light year out. The surface then built
	 * around a root ~1e13 km from the world origin, and WorldScape rebuilt all its LODs for ~30 s, 3-7 hitches of
	 * 40-60 ms a second, until ~6e7 km. Within 16 activation radii such an approach still gives the surface ~6 s more.
	 */
	TAutoConsoleVariable<float> CVarArrivalReachFactor(
		TEXT("aps.Surface.ArrivalReachFactor"), 16.0f,
		TEXT("Rio 05.10: the arrival forecast starts only the surface of a body within this many of its activation radii ")
		TEXT("(0 = any distance, the old way)."),
		ECVF_Default);
}

APlanetaryBody* UAPSPlanetEnvironmentStreamingSubsystem::UpdateArrivalForecast(APawn* Observer,
	const TArray<APlanetaryBody*>& Bodies)
{
	using namespace APSPlanetArrivalForecast;
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const double Lead = APSPlanetArrivalPrivate::CVarArrivalLeadSeconds.GetValueOnGameThread();
	const double Step = Now - ForecastSeconds;
	const bool bSameObserver = IsValid(Observer) && Observer == ForecastObserver.Get();
	TMap<TWeakObjectPtr<APlanetaryBody>, FVector> Before = MoveTemp(ForecastRelative);
	ForecastRelative.Reset();
	ForecastObserver = Observer;
	ForecastSeconds = Now;
	if (!IsValid(Observer) || !(Lead > 0.0))
	{
		ArrivalTracker.Reset();
		return nullptr;
	}
	const FVector ObserverLocation = Observer->GetActorLocation();
	for (APlanetaryBody* Body : Bodies)
	{
		ForecastRelative.Add(Body, ObserverLocation - Body->GetActorLocation());
	}
	if (!bSameObserver)
	{
		// Boarding or leaving a ship is not motion: measure again from here.
		ArrivalTracker.Reset();
		return nullptr;
	}
	if (Step < 0.1 || Step > 2.0)
	{
		// No time passed (a paused or manually ticked world) or a long hitch: keep the forecast, measure again.
		return ArrivalTracker.Confirmed.IsValid() ? ArrivingBody.Get() : nullptr;
	}

	// A planet and its moons arrive together: the forecast follows the family, while which of its bodies leads may
	// change from pass to pass (01.10: confirmed only 0.9 s before reaching a planet with two moons).
	const auto FamilyOf = [this](APlanetaryBody* Body) -> APlanetaryBody*
	{
		APlanet* Family = ResolveFamilyPlanet(Body);
		return Family ? static_cast<APlanetaryBody*>(Family) : Body;
	};
	const APlanetaryBody* ConfirmedFamily = ArrivalTracker.Confirmed.Get();
	bool bConfirmedHolds = false;
	APlanetaryBody* Best = nullptr;
	FResult BestResult;
	APlanetaryBody* BestOfConfirmed = nullptr;
	double BestOfConfirmedSeconds = TNumericLimits<double>::Max();
	const double ReachFactor = APSPlanetArrivalPrivate::CVarArrivalReachFactor.GetValueOnGameThread();
	for (APlanetaryBody* Body : Bodies)
	{
		const FVector* Previous = Before.Find(Body);
		if (!Previous)
		{
			continue;
		}
		const FVector& Relative = ForecastRelative.FindChecked(Body);
		const double Reach = Body->GetWorldScapeActivationRadiusCm();
		if (ReachFactor > 0.0 && Relative.SizeSquared() > FMath::Square(ReachFactor * Reach))
		{
			continue;
		}
		const FResult Result = Evaluate(Relative, *Previous, Step, Body->GetWorldScapeBodyRadiusCm());
		if (ConfirmedFamily && FamilyOf(Body) == ConfirmedFamily && IsArriving(Result, Lead, Reach, true))
		{
			bConfirmedHolds = true;
			if (Result.SecondsToSurface < BestOfConfirmedSeconds)
			{
				BestOfConfirmed = Body;
				BestOfConfirmedSeconds = Result.SecondsToSurface;
			}
		}
		if (IsArriving(Result, Lead, Reach, false) && (!Best || Result.SecondsToSurface < BestResult.SecondsToSurface))
		{
			Best = Body;
			BestResult = Result;
		}
	}
	const bool bChanged = ArrivalTracker.Step(Best ? FamilyOf(Best) : nullptr, bConfirmedHolds);
	APlanetaryBody* Family = ArrivalTracker.Confirmed.Get();
	if (!Family)
	{
		ArrivingBody.Reset();
	}
	else if (BestOfConfirmed && Family == ConfirmedFamily)
	{
		ArrivingBody = BestOfConfirmed;
	}
	else if (Best && FamilyOf(Best) == Family)
	{
		ArrivingBody = Best;
	}
	else if (!ArrivingBody.IsValid() || FamilyOf(ArrivingBody.Get()) != Family)
	{
		ArrivingBody = Family;
	}
	if (bChanged)
	{
		if (Family)
		{
			UE_LOG(LogAPSPlanetArrival, Log,
				TEXT("Arrival forecast: %s (%s first) in %.1f s (%.0f km away, closing at %.0f km/s, line passes %.0f km from its centre)"),
				*Family->GetName(), *GetNameSafe(ArrivingBody.Get()), BestResult.SecondsToSurface,
				BestResult.DistanceCm / 1.0e5, BestResult.ClosingSpeedCmPerSecond / 1.0e5, BestResult.MissDistanceCm / 1.0e5);
		}
		else
		{
			UE_LOG(LogAPSPlanetArrival, Log, TEXT("Arrival forecast ended: %s"), *GetNameSafe(ConfirmedFamily));
		}
	}
	return ArrivingBody.Get();
}
