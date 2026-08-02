#include "APSStellarVisualSubsystem.h"

#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSStellarVisuals, Log, All);

bool UAPSStellarVisualSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSStellarVisualSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Observer)
	{
		return;
	}

	if (!DirectionalLight.IsValid())
	{
		ResolveDirectionalLight();
	}
	SearchElapsed += DeltaTime;
	if (SearchElapsed >= 0.5f || !bHasTargetStar)
	{
		SearchElapsed = 0.0f;
		ResolveNearestStar(Observer->GetActorLocation());
	}

	ADirectionalLight* Light = DirectionalLight.Get();
	UDirectionalLightComponent* LightComponent = Light
		? Cast<UDirectionalLightComponent>(Light->GetLightComponent()) : nullptr;
	if (!LightComponent || !bHasTargetStar)
	{
		return;
	}

	const FVector LightRayDirection = (Observer->GetActorLocation() - TargetStarLocation).GetSafeNormal();
	if (!LightRayDirection.IsNearlyZero())
	{
		const FRotator DesiredRotation = LightRayDirection.Rotation();
		Light->SetActorRotation(FMath::RInterpTo(
			Light->GetActorRotation(), DesiredRotation, DeltaTime, 1.6f));
	}
	SmoothedLightColor = FMath::CInterpTo(
		SmoothedLightColor, TargetLightColor, DeltaTime, 1.3f);
	SmoothedLightIntensity = FMath::FInterpTo(
		SmoothedLightIntensity, TargetLightIntensity, DeltaTime, 1.3f);
	LightComponent->SetLightColor(SmoothedLightColor);
	LightComponent->SetIntensity(SmoothedLightIntensity);
}

TStatId UAPSStellarVisualSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSStellarVisualSubsystem, STATGROUP_Tickables);
}

void UAPSStellarVisualSubsystem::Deinitialize()
{
	if (ADirectionalLight* Light = DirectionalLight.Get(); bCapturedOriginalLight && Light)
	{
		if (UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
		{
			Component->SetLightColor(OriginalLightColor);
			Component->SetIntensity(OriginalLightIntensity);
			Component->SetMobility(OriginalMobility);
		}
		Light->SetActorRotation(OriginalLightRotation);
	}
	Super::Deinitialize();
}

void UAPSStellarVisualSubsystem::ResolveDirectionalLight()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		ADirectionalLight* Candidate = *It;
		UDirectionalLightComponent* Component = IsValid(Candidate)
			? Cast<UDirectionalLightComponent>(Candidate->GetLightComponent()) : nullptr;
		if (!Component)
		{
			continue;
		}
		DirectionalLight = Candidate;
		OriginalLightRotation = Candidate->GetActorRotation();
		OriginalLightColor = Component->GetLightColor();
		OriginalLightIntensity = Component->Intensity;
		OriginalMobility = Component->Mobility;
		SmoothedLightColor = OriginalLightColor;
		SmoothedLightIntensity = OriginalLightIntensity;
		Component->SetMobility(EComponentMobility::Movable);
		bCapturedOriginalLight = true;
		UE_LOG(LogAPSStellarVisuals, Log, TEXT("Using existing directional light %s"), *Candidate->GetName());
		break;
	}
}

void UAPSStellarVisualSubsystem::ResolveNearestStar(const FVector& ObserverLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	double BestDistanceSquared = TNumericLimits<double>::Max();
	FVector BestLocation = FVector::ZeroVector;
	FLinearColor BestColor = FLinearColor::White;
	float BestLuminosity = 1.0f;
	FString BestIdentity;

	for (TActorIterator<AStar> It(World); It; ++It)
	{
		const AStar* Star = *It;
		if (!IsValid(Star))
		{
			continue;
		}
		const double DistanceSquared = FVector::DistSquared(ObserverLocation, Star->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestLocation = Star->GetActorLocation();
			BestColor = UStarGenerator::GetStarColor(Star->SpectralClass, Star->SpectralSubclass);
			BestLuminosity = Star->Luminosity;
			BestIdentity = Star->GetPathName();
		}
	}

	for (TActorIterator<AStarCluster> ClusterIt(World); ClusterIt; ++ClusterIt)
	{
		const AStarCluster* Cluster = *ClusterIt;
		if (!IsValid(Cluster) || !Cluster->StarMeshInstances)
		{
			continue;
		}

		// Compare records in component-local space. Full-scale clusters use uniform scale, so this
		// preserves nearest ordering while avoiding tens of thousands of world transforms per search.
		const FTransform ClusterTransform = Cluster->StarMeshInstances->GetComponentTransform();
		const FVector LocalObserverLocation = ClusterTransform.InverseTransformPosition(ObserverLocation);
		const FClusterStarSystemRecord* ClosestClusterRecord = nullptr;
		double ClosestLocalDistanceSquared = TNumericLimits<double>::Max();
		for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
		{
			if (Record.bMaterialized || Record.InstanceIndex == INDEX_NONE)
			{
				continue;
			}
			const double LocalDistanceSquared = FVector::DistSquared(
				LocalObserverLocation, Record.ClusterLocalLocation);
			if (LocalDistanceSquared < ClosestLocalDistanceSquared)
			{
				ClosestLocalDistanceSquared = LocalDistanceSquared;
				ClosestClusterRecord = &Record;
			}
		}
		if (ClosestClusterRecord)
		{
			const FVector WorldLocation = ClusterTransform.TransformPosition(
				ClosestClusterRecord->ClusterLocalLocation);
			const double DistanceSquared = FVector::DistSquared(ObserverLocation, WorldLocation);
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestLocation = WorldLocation;
				BestColor = UStarGenerator::GetStarColor(
					ClosestClusterRecord->PrimaryStarModel.SpectralClass,
					ClosestClusterRecord->PrimaryStarModel.SpectralSubclass);
				BestLuminosity = ClosestClusterRecord->PrimaryStarModel.Luminosity;
				BestIdentity = ClosestClusterRecord->StableId.ToString(
					EGuidFormats::DigitsWithHyphensLower);
			}
		}
	}

	bHasTargetStar = BestDistanceSquared < TNumericLimits<double>::Max();
	if (!bHasTargetStar)
	{
		return;
	}
	TargetStarLocation = BestLocation;
	// Keep spectral identity readable without tinting the whole scene into an accessibility problem.
	TargetLightColor = FMath::Lerp(FLinearColor::White, BestColor.GetClamped(), 0.38f);
	TargetLightColor.A = 1.0f;
	TargetLightIntensity = 9.0f + FMath::Clamp(
		FMath::LogX(10.0f, FMath::Max(BestLuminosity, 0.0f) + 1.0f) * 1.8f, 0.0f, 9.0f);

	if (BestIdentity != ActiveStarIdentity)
	{
		ActiveStarIdentity = BestIdentity;
		UE_LOG(LogAPSStellarVisuals, Log, TEXT("Active star=%s color=%s intensity=%.2f"),
			*ActiveStarIdentity, *TargetLightColor.ToString(), TargetLightIntensity);
	}
}
