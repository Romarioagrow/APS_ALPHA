#include "GravityDetectorComponent.h"

#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GravityCharacterPawn.h" // где объявлены SwitchGravityType, CurrentGravityType, UpdateGravityPhysicParams
#include "APS_ALPHA/Actors/Astro/OrbitalBody.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Gravity/GravitySource.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "GameFramework/Character.h"

UGravityDetectorComponent::UGravityDetectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = DetectionInterval;
}

void UGravityDetectorComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = DetectionInterval;

	if (bAutomaticDetection)
	{
		RunGravityCheck(Cast<ACharacter>(GetOwner()));
	}
}

void UGravityDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutomaticDetection)
	{
		RunGravityCheck(Cast<ACharacter>(GetOwner()));
	}
}

void UGravityDetectorComponent::RunGravityCheck(ACharacter* Self)
{
	if (!Self)
	{
		return;
	}

	if (AActor* OverlappingSource = FindBestOverlappingSource(Self))
	{
		if (OverlappingSource != GravityTargetActor)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Gravity] Select reason=LocalOverlap character=%s source=%s class=%s characterLocation=%s sourceLocation=%s sourceUp=%s"),
				*GetNameSafe(Self), *GetNameSafe(OverlappingSource),
				*GetNameSafe(OverlappingSource->GetClass()),
				*Self->GetActorLocation().ToCompactString(),
				*OverlappingSource->GetActorLocation().ToCompactString(),
				*OverlappingSource->GetActorUpVector().ToCompactString());
		}
		SwitchGravityType(OverlappingSource);
		return;
	}

	AActor* FullScaleSource = FindClosestFullScaleSource(Self);
	if (FullScaleSource != GravityTargetActor)
	{
		TArray<AActor*> OverlappingActors;
		Self->GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);
		FString SupportedOverlapNames;
		for (AActor* Candidate : OverlappingActors)
		{
			if (IsValid(Candidate) &&
				Candidate->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
			{
				if (!SupportedOverlapNames.IsEmpty())
				{
					SupportedOverlapNames += TEXT(",");
				}
				SupportedOverlapNames += Candidate->GetName();
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Gravity] Select reason=FullScaleFallback character=%s source=%s characterLocation=%s supportedOverlaps=[%s] totalOverlaps=%d"),
			*GetNameSafe(Self), *GetNameSafe(FullScaleSource),
			*Self->GetActorLocation().ToCompactString(), *SupportedOverlapNames,
			OverlappingActors.Num());
	}
	SwitchGravityType(FullScaleSource);
}

void UGravityDetectorComponent::SwitchGravityType(AActor* GravitySourceActor)
{
	if (!IsValid(GravitySourceActor))
	{
		ClearGravitySource();
		return;
	}

	const AActor* PreviousTarget = GravityTargetActor;
	const EGravityType PreviousType = CurrentGravityType;
	GravityTargetActor = GravitySourceActor;

	if (GravitySourceActor->IsA(ASpaceStation::StaticClass()) || GravitySourceActor->IsA(
		ASpaceHeadquarters::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnStation;
	}
	else if (GravitySourceActor->IsA(AOrbitalBody::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnPlanet;
	}
	else if (GravitySourceActor->IsA(ASpaceship::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnShip;
		CurrentSpaceship = Cast<ASpaceship>(GravitySourceActor);
	}
	else
	{
		ClearGravitySource();
		return;
	}

	if (CurrentGravityType != EGravityType::OnShip)
	{
		CurrentSpaceship = nullptr;
	}

	if (PreviousTarget != GravityTargetActor || PreviousType != CurrentGravityType)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Gravity] Active target=%s type=%s direction=%s"),
			*GetNameSafe(GravityTargetActor),
			*UEnum::GetValueAsString(CurrentGravityType),
			*GetGravityDirectionAtLocation(GetOwner()->GetActorLocation()).ToCompactString());
		OnClosestGravityBodyChanged.Broadcast(GravityTargetActor);
		OnGravityPhysicsParamChanged.Broadcast();
	}
}

FVector UGravityDetectorComponent::GetGravityDirectionAtLocation(const FVector& WorldLocation) const
{
	if (!IsValid(GravityTargetActor))
	{
		return FVector::ZeroVector;
	}

	if (CurrentGravityType == EGravityType::OnPlanet)
	{
		return (GravityTargetActor->GetActorLocation() - WorldLocation).GetSafeNormal();
	}

	if (CurrentGravityType == EGravityType::OnStation || CurrentGravityType == EGravityType::OnShip)
	{
		return -GravityTargetActor->GetActorUpVector();
	}

	return FVector::ZeroVector;
}

void UGravityDetectorComponent::ClearGravitySource()
{
	if (!GravityTargetActor && CurrentGravityType == EGravityType::ZeroG)
	{
		return;
	}

	GravityTargetActor = nullptr;
	CurrentSpaceship = nullptr;
	CurrentGravityType = EGravityType::ZeroG;
	OnClosestGravityBodyChanged.Broadcast(nullptr);
	OnGravityPhysicsParamChanged.Broadcast();
}

AActor* UGravityDetectorComponent::FindBestOverlappingSource(ACharacter* Character) const
{
	if (!Character || !Character->GetCapsuleComponent())
	{
		return nullptr;
	}

	TArray<AActor*> OverlappingActors;
	Character->GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);

	AActor* BestSource = nullptr;
	int32 BestPriority = MIN_int32;
	double BestDistanceSquared = DBL_MAX;

	for (AActor* Candidate : OverlappingActors)
	{
		if (!IsValid(Candidate) || !Candidate->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
		{
			continue;
		}

		int32 Priority = 0;
		if (Candidate->IsA(ASpaceship::StaticClass())) Priority = 300;
		else if (Candidate->IsA(ASpaceStation::StaticClass())) Priority = 200;
		else if (Candidate->IsA(AOrbitalBody::StaticClass())) Priority = 100;

		const double DistanceSquared = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
		if (Priority > BestPriority || (Priority == BestPriority && DistanceSquared < BestDistanceSquared))
		{
			BestSource = Candidate;
			BestPriority = Priority;
			BestDistanceSquared = DistanceSquared;
		}
	}

	return BestSource;
}

AWorldActor* UGravityDetectorComponent::FindClosestFullScaleSource(ACharacter* Character) const
{
	if (!Character || !Character->GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(Character->GetWorld(), AWorldActor::StaticClass(), WorldActors);

	AWorldActor* ClosestSource = nullptr;
	double ClosestSurfaceDistanceKm = DBL_MAX;

	for (AActor* Actor : WorldActors)
	{
		AWorldActor* Candidate = Cast<AWorldActor>(Actor);
		// Stations and ships are finite artificial gravity volumes and must never
		// be selected by the full-scale distance fallback after overlap ends.
		if (!Candidate || !Candidate->IsA(AOrbitalBody::StaticClass()) ||
			!Candidate->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
		{
			continue;
		}

		const double CenterDistanceKm = FVector::Distance(Character->GetActorLocation(), Candidate->GetActorLocation()) / 100000.0;
		const double SurfaceDistanceKm = FMath::Max(0.0, CenterDistanceKm - Candidate->RadiusKM);
		if (SurfaceDistanceKm < ClosestSurfaceDistanceKm)
		{
			ClosestSource = Candidate;
			ClosestSurfaceDistanceKm = SurfaceDistanceKm;
		}
	}

	return ClosestSource && ClosestSurfaceDistanceKm <= ClosestSource->AffectionRadiusKM
		? ClosestSource
		: nullptr;
}
