#include "ShipNavigationComponent.h"

#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"

namespace APSShipNavigation
{
	FString SanitizeObjectName(FString Name)
	{
		Name.RemoveFromStart(TEXT("BP_"));
		const int32 GeneratedClassMarker = Name.Find(TEXT("_C_"));
		if (GeneratedClassMarker != INDEX_NONE)
		{
			Name.LeftInline(GeneratedClassMarker);
		}
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return Name.ToUpper();
	}
}

UShipNavigationComponent::UShipNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const FShipNavigationContact* UShipNavigationComponent::GetContact(int32 Index) const
{
	return Contacts.IsValidIndex(Index) ? &Contacts[Index] : nullptr;
}

const FShipNavigationContact* UShipNavigationComponent::GetSelectedContact() const
{
	return GetContact(SelectedContactIndex);
}

void UShipNavigationComponent::RefreshContacts(const FVector& ObserverLocation, bool bForce)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	RefreshElapsed += World->GetDeltaSeconds();
	if (!bForce && RefreshElapsed < RefreshInterval)
	{
		return;
	}
	RefreshElapsed = 0.0f;

	const FString PreviousStableId = GetSelectedContact() ? GetSelectedContact()->StableId : FString();
	Contacts.Reset();
	DiscoveredContactCount = 0;

	for (TActorIterator<AWorldActor> It(World); It; ++It)
	{
		AWorldActor* WorldActor = *It;
		if (!IsValid(WorldActor) || WorldActor == GetOwner())
		{
			continue;
		}
		const bool bNavigatable = WorldActor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass());
		const bool bTechnological = WorldActor->IsA<ATechActor>();
		const bool bCluster = WorldActor->IsA<AStarCluster>();
		if (bNavigatable || bTechnological || bCluster)
		{
			AddActorContact(WorldActor, ObserverLocation);
		}
	}

	DiscoveredContactCount = Contacts.Num();
	AddGeneratedStarContacts(ObserverLocation);
	Contacts.Sort([](const FShipNavigationContact& Left, const FShipNavigationContact& Right)
	{
		return Left.DistanceCentimeters < Right.DistanceCentimeters;
	});
	if (Contacts.Num() > MaximumContacts)
	{
		Contacts.SetNum(MaximumContacts, EAllowShrinking::No);
	}
	RestoreSelection(PreviousStableId);
}

void UShipNavigationComponent::AddActorContact(AActor* Actor, const FVector& ObserverLocation)
{
	if (!IsValid(Actor))
	{
		return;
	}

	FShipNavigationContact Contact;
	Contact.Actor = Actor;
	Contact.FixedWorldLocation = Actor->GetActorLocation();
	Contact.StableId = Actor->GetPathName();
	Contact.DisplayName = APSShipNavigation::SanitizeObjectName(Actor->GetName());
	Contact.DistanceCentimeters = FVector::Distance(ObserverLocation, Actor->GetActorLocation());

	if (const ACelestialBody* Body = Cast<ACelestialBody>(Actor); Body && !Body->AstroName.IsNone())
	{
		Contact.DisplayName = Body->AstroName.ToString().ToUpper();
	}
	if (const AWorldActor* WorldActor = Cast<AWorldActor>(Actor);
		WorldActor && WorldActor->GetClass()->ImplementsInterface(UItemInfoInterface::StaticClass()))
	{
		const FText ItemName = IItemInfoInterface::Execute_GetInGameName(WorldActor);
		if (!ItemName.IsEmpty())
		{
			Contact.DisplayName = ItemName.ToString().ToUpper();
		}
	}

	if (const AStar* Star = Cast<AStar>(Actor))
	{
		Contact.Type = EShipNavigationContactType::Star;
		Contact.TypeLabel = TEXT("STAR");
		Contact.Detail = Star->FullSpectralClass.IsNone() ? TEXT("STELLAR BODY") : Star->FullSpectralClass.ToString().ToUpper();
		if (!Star->FullSpectralName.IsNone()) Contact.DisplayName = Star->FullSpectralName.ToString().ToUpper();
	}
	else if (Actor->IsA<AMoon>())
	{
		Contact.Type = EShipNavigationContactType::Moon;
		Contact.TypeLabel = TEXT("MOON");
	}
	else if (Actor->IsA<APlanet>())
	{
		Contact.Type = EShipNavigationContactType::Planet;
		Contact.TypeLabel = TEXT("PLANET");
	}
	else if (Actor->IsA<ASpaceStation>())
	{
		Contact.Type = EShipNavigationContactType::Station;
		Contact.TypeLabel = TEXT("STATION");
	}
	else if (Actor->IsA<AColony>())
	{
		Contact.Type = EShipNavigationContactType::Settlement;
		Contact.TypeLabel = TEXT("SETTLEMENT");
	}
	else if (Actor->IsA<ATechActor>())
	{
		Contact.Type = EShipNavigationContactType::Infrastructure;
		Contact.TypeLabel = TEXT("INFRASTRUCTURE");
	}
	else if (Actor->IsA<AStarSystem>())
	{
		Contact.Type = EShipNavigationContactType::StarSystem;
		Contact.TypeLabel = TEXT("STAR SYSTEM");
	}
	else if (Actor->IsA<AStarCluster>())
	{
		Contact.Type = EShipNavigationContactType::StarCluster;
		Contact.TypeLabel = TEXT("STAR CLUSTER");
	}
	else
	{
		Contact.TypeLabel = TEXT("NAV OBJECT");
	}

	Contacts.Add(MoveTemp(Contact));
}

void UShipNavigationComponent::AddGeneratedStarContacts(const FVector& ObserverLocation)
{
	const UWorld* World = GetWorld();
	if (!World || MaximumVirtualStars <= 0)
	{
		return;
	}

	TArray<FShipNavigationContact> VirtualStars;
	AStarCluster* Cluster = nullptr;
	for (TActorIterator<AStarCluster> ClusterIt(World); ClusterIt; ++ClusterIt)
	{
		if (IsValid(*ClusterIt) && (*ClusterIt)->StarMeshInstances
			&& (*ClusterIt)->StarMeshInstances->GetInstanceCount() > 0)
		{
			Cluster = *ClusterIt;
			break;
		}
	}
	if (!Cluster)
	{
		return;
	}

	const AAstroGenerator* Generator = nullptr;
	for (TActorIterator<AAstroGenerator> GeneratorIt(World); GeneratorIt; ++GeneratorIt)
	{
		Generator = *GeneratorIt;
		break;
	}
	const UHierarchicalInstancedStaticMeshComponent* Instances = Cluster->StarMeshInstances;
	const int32 InstanceCount = Instances->GetInstanceCount();
	DiscoveredContactCount += InstanceCount;
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
	{
		FTransform InstanceTransform;
		if (!Instances->GetInstanceTransform(InstanceIndex, InstanceTransform, true))
		{
			continue;
		}

		FShipNavigationContact Contact;
		Contact.FixedWorldLocation = InstanceTransform.GetLocation();
		Contact.StableId = FString::Printf(TEXT("%s:STAR:%d"), *Cluster->GetPathName(), InstanceIndex);
		Contact.DisplayName = FString::Printf(TEXT("STAR %04d"), InstanceIndex + 1);
		Contact.TypeLabel = TEXT("STAR");
		Contact.Detail = TEXT("GENERATED CLUSTER CONTACT");
		Contact.Type = EShipNavigationContactType::Star;
		Contact.DistanceCentimeters = FVector::Distance(ObserverLocation, Contact.FixedWorldLocation);
		Contact.bVirtualContact = true;
		if (Generator)
		{
			if (const TSharedPtr<FStarModel>* Model = Generator->StarIndexModelMap.Find(InstanceIndex);
				Model && Model->IsValid())
			{
				if (!(*Model)->FullSpectralName.IsNone()) Contact.DisplayName = (*Model)->FullSpectralName.ToString().ToUpper();
				if (!(*Model)->FullSpectralClass.IsNone()) Contact.Detail = (*Model)->FullSpectralClass.ToString().ToUpper();
			}
		}
		VirtualStars.Add(MoveTemp(Contact));
	}

	VirtualStars.Sort([](const FShipNavigationContact& Left, const FShipNavigationContact& Right)
	{
		return Left.DistanceCentimeters < Right.DistanceCentimeters;
	});
	if (VirtualStars.Num() > MaximumVirtualStars)
	{
		VirtualStars.SetNum(MaximumVirtualStars, EAllowShrinking::No);
	}
	Contacts.Append(MoveTemp(VirtualStars));
}

void UShipNavigationComponent::RestoreSelection(const FString& PreviousStableId)
{
	SelectedContactIndex = INDEX_NONE;
	if (!PreviousStableId.IsEmpty())
	{
		SelectedContactIndex = Contacts.IndexOfByPredicate([&PreviousStableId](const FShipNavigationContact& Contact)
		{
			return Contact.StableId == PreviousStableId;
		});
	}
	if (SelectedContactIndex == INDEX_NONE && !Contacts.IsEmpty())
	{
		SelectedContactIndex = 0;
	}
}

void UShipNavigationComponent::CycleTarget(int32 Direction)
{
	if (Contacts.IsEmpty())
	{
		SelectedContactIndex = INDEX_NONE;
		return;
	}
	const int32 Step = Direction >= 0 ? 1 : -1;
	SelectedContactIndex = SelectedContactIndex == INDEX_NONE
		? 0
		: (SelectedContactIndex + Step + Contacts.Num()) % Contacts.Num();
}

FString UShipNavigationComponent::FormatDistance(double DistanceCentimeters)
{
	const double Kilometers = DistanceCentimeters / 100000.0;
	if (Kilometers < 1.0) return FString::Printf(TEXT("%.0f m"), DistanceCentimeters / 100.0);
	if (Kilometers < 10000.0) return FString::Printf(TEXT("%.1f km"), Kilometers);
	if (Kilometers < 1000000.0) return FString::Printf(TEXT("%.1f Mm"), Kilometers / 1000.0);
	constexpr double AstronomicalUnitKm = 149597870.7;
	constexpr double LightYearKm = 9460730472580.8;
	if (Kilometers < AstronomicalUnitKm * 0.1) return FString::Printf(TEXT("%.2f Mkm"), Kilometers / 1000000.0);
	if (Kilometers < LightYearKm * 0.1) return FString::Printf(TEXT("%.3f AU"), Kilometers / AstronomicalUnitKm);
	return FString::Printf(TEXT("%.4f ly"), Kilometers / LightYearKm);
}
