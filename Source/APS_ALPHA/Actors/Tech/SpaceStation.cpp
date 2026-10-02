#include "SpaceStation.h"

#include "Components/StaticMeshComponent.h"

namespace APSStationGravity
{
	constexpr float MinimumGravityRadius = 1000.0f;
	constexpr float BoundsPadding = 500.0f;
	// A station is a few kilometres across. A mesh component left far from the actor (BP_SpaceHeadquarters_Alpha had
	// one at the world origin, 02.10) stretched the gravity volume to 1.6 AU: the whole home system pulled at 1 g
	// toward the HQ and ships flew at the 500 m/s floor. Such components are ignored and the volume is capped.
	constexpr double StrayComponentCm = 10000000.0;
	constexpr float MaximumGravityRadius = 5000000.0f;
}

ASpaceStation::ASpaceStation()
{
	GravityCollisionZone = CreateDefaultSubobject<USphereComponent>(TEXT("StationGravitySphereCollisionComponent"));
	if (!RootComponent)
	{
		RootComponent = GravityCollisionZone;
	}
	GravityCollisionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GravityCollisionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	GravityCollisionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GravityCollisionZone->SetGenerateOverlapEvents(true);
	// Collision primitives are gameplay-only volumes. Several legacy station
	// Blueprints serialized them as visible, which renders a huge red wire sphere
	// around the spawned character in PIE (seen edge-on as a screen-sized cross).
	// Visibility does not affect overlap/collision and is deliberately not
	// propagated to the station meshes attached below this root component.
	GravityCollisionZone->SetVisibility(false, false);
	GravityCollisionZone->SetHiddenInGame(true, false);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(GravityCollisionZone);
	SpawnPoint->SetWorldRotation(GetActorRotation());

	PlayerStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerStartPoint"));
	PlayerStartPoint->SetupAttachment(GravityCollisionZone);
}

FVector ASpaceStation::GetPlayerStartLocation() const
{
	if (IsValid(PlayerStartPoint) && !PlayerStartPoint->GetRelativeLocation().IsNearlyZero(1.0))
	{
		return PlayerStartPoint->GetComponentLocation();
	}
	if (IsValid(SpawnPoint) && !SpawnPoint->GetRelativeLocation().IsNearlyZero(1.0))
	{
		return SpawnPoint->GetComponentLocation();
	}
	// Nothing authored: the actor origin usually lies inside the hull, where the pilot falls through
	// back faces and stays trapped under the collision. Start just above the top of the visible
	// geometry instead; station gravity then lands the pilot on the roof.
	FBox VisualBounds(ForceInit);
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (const UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		// Stray components far from the station are not part of its hull (see APSStationGravity).
		if (IsValid(MeshComponent) && MeshComponent->GetStaticMesh() && MeshComponent->IsRegistered()
			&& FVector::Dist(MeshComponent->Bounds.Origin, GetActorLocation()) <= APSStationGravity::StrayComponentCm)
		{
			VisualBounds += MeshComponent->Bounds.GetBox();
		}
	}
	if (!VisualBounds.IsValid)
	{
		return IsValid(SpawnPoint) ? SpawnPoint->GetComponentLocation() : GetActorLocation();
	}
	const FVector Up = GetActorUpVector();
	const FVector Extent = VisualBounds.GetExtent();
	const double TopDistance = FMath::Abs(Up.X) * Extent.X + FMath::Abs(Up.Y) * Extent.Y + FMath::Abs(Up.Z) * Extent.Z;
	return VisualBounds.GetCenter() + Up * (TopDistance + 200.0);
}

void ASpaceStation::BeginPlay()
{
	// Repair legacy Blueprint component references before AActor::BeginPlay invokes
	// the Blueprint Event BeginPlay graph. BP_SpaceHeadquarters reads this property.
	ConfigureGravityVolume(false);
	Super::BeginPlay();
	// Blueprint startup code may overwrite collision/radius, so enforce the native
	// gravity-volume contract once more after it has finished.
	ConfigureGravityVolume(true);
}

void ASpaceStation::ConfigureGravityVolume(bool bWriteDiagnosticLog)
{
	if (!IsValid(GravityCollisionZone))
	{
		TArray<USphereComponent*> SphereComponents;
		GetComponents(SphereComponents);
		for (USphereComponent* SphereComponent : SphereComponents)
		{
			if (IsValid(SphereComponent) &&
				SphereComponent->GetFName() == TEXT("StationGravitySphereCollisionComponent"))
			{
				GravityCollisionZone = SphereComponent;
				break;
			}
		}

		if (!IsValid(GravityCollisionZone))
		{
			for (USphereComponent* SphereComponent : SphereComponents)
			{
				if (IsValid(SphereComponent) && SphereComponent->GetName().Contains(TEXT("Gravity")))
				{
					GravityCollisionZone = SphereComponent;
					break;
				}
			}
		}

		if (!IsValid(GravityCollisionZone))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Gravity] StationVolume actor=%s cannot resolve any gravity sphere component"),
				*GetName());
			return;
		}
	}

	// Enforce the finite gravity-volume contract even when an old station
	// Blueprint serialized a different collision profile.
	GravityCollisionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GravityCollisionZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	GravityCollisionZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GravityCollisionZone->SetGenerateOverlapEvents(true);
	// Blueprint construction/startup can restore the old serialized visibility,
	// so enforce the non-rendering collision-volume contract together with the
	// collision profile on both sides of Blueprint BeginPlay.
	GravityCollisionZone->SetVisibility(false, false);
	GravityCollisionZone->SetHiddenInGame(true, false);

	FBox VisualBounds(EForceInit::ForceInit);
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent) && MeshComponent->GetStaticMesh() && MeshComponent->IsRegistered())
		{
			if (FVector::Dist(MeshComponent->Bounds.Origin, GetActorLocation()) > APSStationGravity::StrayComponentCm)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Gravity] StationVolume actor=%s ignores stray mesh %s %.0f km from the station"),
					*GetName(), *MeshComponent->GetName(), FVector::Dist(MeshComponent->Bounds.Origin, GetActorLocation()) / 100000.0);
				continue;
			}
			VisualBounds += MeshComponent->Bounds.GetBox();
		}
	}

	if (!VisualBounds.IsValid)
	{
		if (bWriteDiagnosticLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Gravity] StationVolume actor=%s has no registered static-mesh bounds; location=%s up=%s"),
				*GetName(), *GetActorLocation().ToCompactString(),
				*GetActorUpVector().ToCompactString());
		}
		return;
	}

	const bool bGravityVolumeIsRoot = GravityCollisionZone == RootComponent;
	const bool bGravityVolumeWasDetached = !bGravityVolumeIsRoot
		&& !GravityCollisionZone->IsAttachedTo(RootComponent);
	if (bGravityVolumeWasDetached)
	{
		GravityCollisionZone->AttachToComponent(
			RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	}

	const FVector BoundsCenter = VisualBounds.GetCenter();
	const FVector BoundsExtent = VisualBounds.GetExtent();
	// Preserve authored component/SpawnPoint transforms on normal station BPs.
	// Only the broken detached legacy volume needs to be recentered.
	const FVector VolumeCenter = bGravityVolumeWasDetached
		? BoundsCenter
		: GravityCollisionZone->GetComponentLocation();
	const float RequiredWorldRadius = FVector::Distance(VolumeCenter, BoundsCenter)
		+ BoundsExtent.Size() + APSStationGravity::BoundsPadding;
	const float DesiredWorldRadius = FMath::Min(FMath::Max(
		APSStationGravity::MinimumGravityRadius,
		RequiredWorldRadius * GetGravityVolumeRadiusMultiplier()), APSStationGravity::MaximumGravityRadius);

	if (bGravityVolumeWasDetached)
	{
		GravityCollisionZone->SetWorldLocation(VolumeCenter);
	}

	if (bGravityVolumeWasDetached ||
		GravityCollisionZone->GetScaledSphereRadius() < DesiredWorldRadius)
	{
		const float ComponentScale = FMath::Max(
			GravityCollisionZone->GetComponentScale().GetAbsMax(), UE_SMALL_NUMBER);
		GravityCollisionZone->SetSphereRadius(DesiredWorldRadius / ComponentScale, true);
	}

	if (bWriteDiagnosticLog)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Gravity] StationVolume actor=%s class=%s detached=%d root=%s actorLocation=%s actorUp=%s volumeLocation=%s volumeRadius=%.1f boundsCenter=%s boundsExtent=%s"),
			*GetName(), *GetNameSafe(GetClass()), bGravityVolumeWasDetached,
			*GetNameSafe(RootComponent), *GetActorLocation().ToCompactString(),
			*GetActorUpVector().ToCompactString(),
			*GravityCollisionZone->GetComponentLocation().ToCompactString(),
			GravityCollisionZone->GetScaledSphereRadius(),
			*BoundsCenter.ToCompactString(), *BoundsExtent.ToCompactString());
	}
}
