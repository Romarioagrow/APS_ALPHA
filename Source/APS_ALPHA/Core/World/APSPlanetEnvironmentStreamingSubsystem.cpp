#include "APSPlanetEnvironmentStreamingSubsystem.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSWorldScapeStreaming, Log, All);

bool UAPSPlanetEnvironmentStreamingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSPlanetEnvironmentStreamingSubsystem::Tick(float DeltaTime)
{
	UpdateElapsed += DeltaTime;
	if (UpdateElapsed < UpdateInterval)
	{
		return;
	}
	UpdateElapsed = 0.0f;
	UpdateActiveEnvironment();
}

TStatId UAPSPlanetEnvironmentStreamingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSPlanetEnvironmentStreamingSubsystem, STATGROUP_Tickables);
}

void UAPSPlanetEnvironmentStreamingSubsystem::UpdateActiveEnvironment()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Observer)
	{
		return;
	}

	APlanetaryBody* BestBody = nullptr;
	double BestNormalizedDistance = TNumericLimits<double>::Max();
	const FVector ObserverLocation = Observer->GetActorLocation();
	TArray<APlanetaryBody*> StreamedBodies;
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		APlanetaryBody* Body = *It;
		if (!IsValid(Body) || !Body->bStreamWorldScapeSurface)
		{
			continue;
		}
		StreamedBodies.Add(Body);

		const double Distance = FVector::Distance(ObserverLocation, Body->GetActorLocation());
		const bool bCurrentBody = ActiveBody.Get() == Body;
		const double Limit = bCurrentBody
			? Body->GetWorldScapeDeactivationRadiusCm()
			: Body->GetWorldScapeActivationRadiusCm();
		if (Limit <= UE_DOUBLE_SMALL_NUMBER || Distance > Limit)
		{
			continue;
		}

		const double NormalizedDistance = (Distance / Limit) * (bCurrentBody ? 0.8 : 1.0);
		if (NormalizedDistance < BestNormalizedDistance)
		{
			BestNormalizedDistance = NormalizedDistance;
			BestBody = Body;
		}
	}

	// Generation code and old maps can contain roots that started enabled. Bring every
	// body under the same single-active-surface budget on the first update as well.
	for (APlanetaryBody* Body : StreamedBodies)
	{
		if (Body != BestBody && Body->IsWorldScapeStreamingActive())
		{
			Body->SetWorldScapeStreamingActive(false);
		}
	}

	if (BestBody == ActiveBody.Get())
	{
		if (BestBody && !BestBody->IsWorldScapeStreamingActive())
		{
			BestBody->SetWorldScapeStreamingActive(true);
		}
		return;
	}

	if (APlanetaryBody* PreviousBody = ActiveBody.Get())
	{
		if (PreviousBody->IsWorldScapeStreamingActive())
		{
			PreviousBody->SetWorldScapeStreamingActive(false);
			UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Deactivated WorldScape surface: %s"), *PreviousBody->GetPathName());
		}
	}
	ActiveBody = BestBody;
	if (BestBody)
	{
		BestBody->SetWorldScapeStreamingActive(true);
		UE_LOG(LogAPSWorldScapeStreaming, Log, TEXT("Activated WorldScape surface: %s"), *BestBody->GetPathName());
	}
}
