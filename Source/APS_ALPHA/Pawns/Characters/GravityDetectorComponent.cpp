#include "GravityDetectorComponent.h"

#include "Components/BoxComponent.h"
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

namespace APSGravityDetectorLocal
{
	/**
	 * F3 (Rio, 02.10: "walking by the ship is bad"): beside a world, a ship's artificial gravity holds only for a character
	 * in or on the ship: standing on it, or with the ship under its feet (its deck while jumping inside). Next to a
	 * landed ship the world pulls, so a ship resting on a slope no longer tilts the ground around it and the edge of its
	 * gravity sphere no longer flips the character between two "downs". A passenger aboard (attached, inside the hull's
	 * bounds) stays in, floating or high above a deck; "under its feet" is along the ship's own down, so a banked or
	 * inverted ship keeps its people (02.10, walking about a flying ship).
	 */
	bool IsInOrOnShip(const AActor* Self, const ASpaceship* Ship, const AActor* Body)
	{
		const ACharacter* Character = Cast<ACharacter>(Self);
		if (!Character || !Self->GetWorld()) return true;
		if (const UPrimitiveComponent* Base = Character->GetMovementBase(); Base && Base->GetOwner() == Ship) return true;
		if (Self->GetAttachParentActor() == Ship && Ship->InteractionBoundsComponent)
		{
			const UBoxComponent* Bounds = Ship->InteractionBoundsComponent;
			const FVector Local = Bounds->GetComponentTransform().InverseTransformPositionNoScale(Self->GetActorLocation());
			const FVector Extent = Bounds->GetScaledBoxExtent();
			if (FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z)
			{
				return true;
			}
		}
		(void)Body;
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;
		const FVector Start = Self->GetActorLocation();
		const FVector Down = -Ship->GetActorUpVector();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipGravityFloor), false, Self);
		FHitResult Hit;
		return Self->GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Down * (HalfHeight + 250.0f), ECC_Pawn, Params)
			&& Hit.GetActor() == Ship;
	}
}

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
		RunGravityCheckForActor(GetOwner());
	}
}

void UGravityDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutomaticDetection)
	{
		RunGravityCheckForActor(GetOwner());
	}
}

void UGravityDetectorComponent::RunGravityCheck(ACharacter* Self)

{
	RunGravityCheckForActor(Self);
}

void UGravityDetectorComponent::RunGravityCheckForActor(AActor* Self)
{
	if (!Self)
	{
		return;
	}

	AActor* OverlappingSource = FindBestOverlappingSource(Self);
	// Natural bodies: their overlap zones are coarse (a planet's reaches past its moons, a moon has none), so on a moon
	// the parent planet pulled (Rio, 01.10). Among natural bodies the one whose surface is nearest pulls; stations and
	// ships keep their place above them.
	if (OverlappingSource && OverlappingSource->IsA(AOrbitalBody::StaticClass()))
	{
		if (AActor* NearestBody = FindClosestFullScaleSource(Self); IsValid(NearestBody) && NearestBody != OverlappingSource)
		{
			OverlappingSource = NearestBody;
		}
	}
	else if (const ASpaceship* Ship = Cast<ASpaceship>(OverlappingSource); Ship && Self->IsA(ACharacter::StaticClass()))
	{
		if (AActor* Body = FindClosestFullScaleSource(Self); IsValid(Body)
			&& !APSGravityDetectorLocal::IsInOrOnShip(Self, Ship, Body))
		{
			OverlappingSource = Body;
		}
	}
	if (OverlappingSource)
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
		Self->GetOverlappingActors(OverlappingActors);
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

AActor* UGravityDetectorComponent::FindBestOverlappingSource(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	TArray<AActor*> OverlappingActors;
	Actor->GetOverlappingActors(OverlappingActors);

	// Vehicle collision is intentionally lightweight and may not overlap volumes
	// configured only for ECC_Pawn. Supplement it with point-in-volume checks while
	// keeping the character path unchanged and cheap.
	if (!Actor->IsA(ACharacter::StaticClass()) && Actor->GetWorld())
	{
		TArray<AActor*> GravitySources;
		UGameplayStatics::GetAllActorsWithInterface(
			Actor->GetWorld(), UGravitySource::StaticClass(), GravitySources);
		for (AActor* Candidate : GravitySources)
		{
			if (!IsValid(Candidate) || Candidate == Actor)
			{
				continue;
			}

			USphereComponent* GravitySphere = nullptr;
			if (const ASpaceStation* Station = Cast<ASpaceStation>(Candidate))
			{
				GravitySphere = Station->GravityCollisionZone;
			}
			else if (const ASpaceship* Ship = Cast<ASpaceship>(Candidate))
			{
				GravitySphere = Ship->ProvidesShipGravity() ? Ship->SphereCollisionComponent : nullptr;
			}

			if (GravitySphere && FVector::DistSquared(Actor->GetActorLocation(), GravitySphere->GetComponentLocation())
				<= FMath::Square(GravitySphere->GetScaledSphereRadius()))
			{
				OverlappingActors.AddUnique(Candidate);
			}
		}
	}

	AActor* BestSource = nullptr;
	int32 BestPriority = MIN_int32;
	double BestDistanceSquared = DBL_MAX;

	for (AActor* Candidate : OverlappingActors)
	{
		if (!IsValid(Candidate) || Candidate == Actor
			|| !Candidate->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
		{
			continue;
		}
		if (const ASpaceship* Ship = Cast<ASpaceship>(Candidate); Ship && !Ship->ProvidesShipGravity())
		{
			continue;
		}

		int32 Priority = 0;
		if (Candidate->IsA(ASpaceship::StaticClass())) Priority = 300;
		else if (Candidate->IsA(ASpaceStation::StaticClass())) Priority = 200;
		else if (Candidate->IsA(AOrbitalBody::StaticClass())) Priority = 100;

		const double DistanceSquared = FVector::DistSquared(Actor->GetActorLocation(), Candidate->GetActorLocation());
		if (Priority > BestPriority || (Priority == BestPriority && DistanceSquared < BestDistanceSquared))
		{
			BestSource = Candidate;
			BestPriority = Priority;
			BestDistanceSquared = DistanceSquared;
		}
	}

	return BestSource;
}

AWorldActor* UGravityDetectorComponent::FindClosestFullScaleSource(AActor* Actor) const
{
	if (!Actor || !Actor->GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(Actor->GetWorld(), AWorldActor::StaticClass(), WorldActors);

	AWorldActor* ClosestSource = nullptr;
	double ClosestSurfaceDistanceKm = DBL_MAX;

	for (AActor* CandidateActor : WorldActors)
	{
		AWorldActor* Candidate = Cast<AWorldActor>(CandidateActor);
		// Stations and ships are finite artificial gravity volumes and must never
		// be selected by the full-scale distance fallback after overlap ends.
		if (!Candidate || !Candidate->IsA(AOrbitalBody::StaticClass()) ||
			!Candidate->GetClass()->ImplementsInterface(UGravitySource::StaticClass()))
		{
			continue;
		}

		const double CenterDistanceKm = FVector::Distance(Actor->GetActorLocation(), Candidate->GetActorLocation()) / 100000.0;
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
