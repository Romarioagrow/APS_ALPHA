#include "SpaceStation.h"

#include "Components/StaticMeshComponent.h"

namespace APSStationGravity
{
	constexpr float MinimumGravityRadius = 1000.0f;
	constexpr float BoundsPadding = 500.0f;
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

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(GravityCollisionZone);
	SpawnPoint->SetWorldRotation(GetActorRotation());
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

	FBox VisualBounds(EForceInit::ForceInit);
	TArray<UStaticMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent) && MeshComponent->GetStaticMesh() && MeshComponent->IsRegistered())
		{
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
	const float DesiredWorldRadius = FMath::Max(
		APSStationGravity::MinimumGravityRadius, RequiredWorldRadius);

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
