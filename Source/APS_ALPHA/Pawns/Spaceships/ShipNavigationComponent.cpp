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
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace APSShipNavigation
{
	FString SanitizeObjectName(FString Name)
	{
		Name.RemoveFromStart(TEXT("BP_"));
		Name.ReplaceInline(TEXT("_C_"), TEXT("_"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return Name.ToUpper();
	}

	TAutoConsoleVariable<float> CVarSystemFold(TEXT("aps.Nav.SystemFold"), 6.0f,
		TEXT("Rio 04.10 (\"outside a system its separate planets make no sense, show the system and its star\"): a star system ")
		TEXT("whose worlds lie within 1/N of the distance to it (N = this; 6 is about 9.5 degrees) shows as one card at its ")
		TEXT("star, its worlds, colonies, sites and fleet ships folded into it. 0 never folds."));

	/** A star's spectral colour, lifted toward white so a card's text stays readable on the dark sky. */
	FLinearColor CardColour(const FLinearColor& Star)
	{
		FLinearColor Colour = FMath::Lerp(Star, FLinearColor::White, 0.35f);
		Colour.A = 0.95f;
		return Colour;
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
	KeepStableId = PreviousStableId;
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

	// Star systems the civilization knows or holds (Rio 02.10: "beacons for the star systems"): their anchors.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(TEXT("APS.StarSystem")))
		{
			AddActorContact(*It, ObserverLocation);
		}
	}
	DiscoveredContactCount = Contacts.Num();
	FoldDistantSystems(ObserverLocation, PreviousStableId);
	AddGeneratedStarContacts(ObserverLocation);
	if (bShowNearStarLabels)
	{
		AddNearStarLabels(ObserverLocation);
	}
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
	if (!Actor->IsA<ACelestialBody>())
	{
		if (const AWorldActor* WorldActor = Cast<AWorldActor>(Actor);
			WorldActor && WorldActor->GetClass()->ImplementsInterface(UItemInfoInterface::StaticClass()))
		{
			const FText ItemName = IItemInfoInterface::Execute_GetInGameName(WorldActor);
			if (!ItemName.IsEmpty())
			{
				Contact.DisplayName = ItemName.ToString().ToUpper();
			}
		}
	}

	FGuid SystemId;
	if (FAPSStarSystems::AnchorSystem(Actor, SystemId))
	{
		// A system's beacon: its name and what is known; a claimed one is charted always, like the colony.
		const FAPSStarSystems* Stars = APSStarSystemsFind(GetWorld());
		const FAPSStarSystemInfo* Info = Stars ? Stars->Find(SystemId) : nullptr;
		Contact.Type = EShipNavigationContactType::StarSystem;
		Contact.TypeLabel = Stars && Stars->IsClaimed(SystemId) ? TEXT("BEACON") : TEXT("STAR SYSTEM");
		if (Info)
		{
			Contact.DisplayName = Info->Name;
			Contact.Detail = FString::Printf(TEXT("%s%s%s"), *Info->Spectral, Info->Spectral.IsEmpty() ? TEXT("") : TEXT("  "),
				*APSStars::KnowledgeName(Stars->GetKnowledge(SystemId)).ToString());
		}
		Contact.bOwnColony = Stars && Stars->IsClaimed(SystemId);
	}
	else if (const AStar* Star = Cast<AStar>(Actor))
	{
		Contact.Type = EShipNavigationContactType::Star;
		Contact.TypeLabel = TEXT("STAR");
		Contact.Detail = Star->FullSpectralClass.IsNone() ? TEXT("STELLAR BODY") : Star->FullSpectralClass.ToString().ToUpper();
		if (!Star->FullSpectralName.IsNone()) Contact.DisplayName = Star->FullSpectralName.ToString().ToUpper();
	}
	else if (Actor->IsA<AMoon>())
	{
		const AMoon* Moon = CastChecked<AMoon>(Actor);
		Contact.Type = EShipNavigationContactType::Moon;
		Contact.TypeLabel = TEXT("MOON");
		if (const UEnum* Enum = StaticEnum<EMoonType>())
		{
			Contact.Detail = Enum->GetDisplayNameTextByValue(static_cast<int64>(Moon->MoonType)).ToString().ToUpper();
		}
		// Rio 04.10: a world not surveyed yet says what the maps say (NO DATA), not its type.
		if (const FAPSFleetCommand* Fleet = APSFleetFind(GetWorld());
			Fleet && Fleet->GetSurvey(Moon) == APSFleet::ESurvey::Unknown)
		{
			Contact.Detail = APSFleet::SurveyName(APSFleet::ESurvey::Unknown).ToString();
		}
	}
	else if (Actor->IsA<APlanet>())
	{
		const APlanet* Planet = CastChecked<APlanet>(Actor);
		Contact.Type = EShipNavigationContactType::Planet;
		Contact.TypeLabel = TEXT("PLANET");
		if (const UEnum* Enum = StaticEnum<EPlanetType>())
		{
			Contact.Detail = Enum->GetDisplayNameTextByValue(static_cast<int64>(Planet->PlanetType)).ToString().ToUpper();
		}
		if (const FAPSFleetCommand* Fleet = APSFleetFind(GetWorld());
			Fleet && Fleet->GetSurvey(Planet) == APSFleet::ESurvey::Unknown)
		{
			Contact.Detail = APSFleet::SurveyName(APSFleet::ESurvey::Unknown).ToString();
		}
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
	// The pilot's own colony is always charted, with a marker of its own (Rio, 30.09).
	if (Actor->ActorHasTag(TEXT("APS.Civilization.Materialized")))
	{
		if (const UAPSCivilizationIdentityComponent* Identity = Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
			Identity && Identity->Role == EAPSCivilizationEntityRole::BaseModule)
		{
			Contact.bOwnColony = true;
			Contact.TypeLabel = TEXT("COLONY");
			Contact.DisplayName = TEXT("HOME COLONY");
		}
	}
	// The ancient sites (Gameplay/Ancients) are always charted, under their own label.
	const bool bAncient = Actor->ActorHasTag(TEXT("APS.Ancient.Site"));
	if (bAncient)
	{
		Contact.TypeLabel = TEXT("ANCIENT SITE");
	}
	if (!IsContactTypeVisible(Contact.Type) && !Contact.bOwnColony && Contact.StableId != PinnedCourseId && !bAncient
		&& Contact.StableId != KeepStableId)
	{
		return;
	}

	TArray<FString> Hierarchy;
	for (AActor* Parent = Actor->GetAttachParentActor(); Parent && Hierarchy.Num() < 4;
		Parent = Parent->GetAttachParentActor())
	{
		if (const ACelestialBody* ParentBody = Cast<ACelestialBody>(Parent); ParentBody && !ParentBody->AstroName.IsNone())
		{
			Hierarchy.Insert(ParentBody->AstroName.ToString().ToUpper(), 0);
		}
	}
	Contact.HierarchyLabel = FString::Join(Hierarchy, TEXT(" > "));

	Contacts.Add(MoveTemp(Contact));
}

void UShipNavigationComponent::AddGeneratedStarContacts(const FVector& ObserverLocation)
{
	const UWorld* World = GetWorld();
	if (!World || !bShowStarMarkers || MaximumVirtualStars <= 0)
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

	const UHierarchicalInstancedStaticMeshComponent* Instances = Cluster->StarMeshInstances;
	const int32 InstanceCount = Instances->GetInstanceCount();
	DiscoveredContactCount += InstanceCount;
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(InstanceIndex);
		if (Record && Record->bMaterialized)
		{
			continue;
		}
		FTransform InstanceTransform;
		if (!Instances->GetInstanceTransform(InstanceIndex, InstanceTransform, true))
		{
			continue;
		}

		FShipNavigationContact Contact;
		Contact.FixedWorldLocation = InstanceTransform.GetLocation();
		Contact.StableId = Record
			? FString::Printf(TEXT("CLUSTER_SYSTEM:%s"),
				*Record->StableId.ToString(EGuidFormats::DigitsWithHyphensLower))
			: FString::Printf(TEXT("%s:STAR:%d"), *Cluster->GetPathName(), InstanceIndex);
		Contact.DisplayName = FString::Printf(TEXT("SYSTEM %04d"), InstanceIndex + 1);
		Contact.TypeLabel = TEXT("STAR");
		Contact.Detail = TEXT("GENERATED CLUSTER CONTACT");
		Contact.HierarchyLabel = Cluster->GetName().ToUpper();
		Contact.Type = EShipNavigationContactType::Star;
		Contact.DistanceCentimeters = FVector::Distance(ObserverLocation, Contact.FixedWorldLocation);
		Contact.bVirtualContact = true;
		if (Record)
		{
			const UEnum* SpectralEnum = StaticEnum<ESpectralClass>();
			const FString SpectralClass = SpectralEnum
				? SpectralEnum->GetNameStringByValue(static_cast<int64>(Record->PrimaryStarModel.SpectralClass))
				: TEXT("UNKNOWN");
			Contact.Detail = FString::Printf(TEXT("%s%d // %d POTENTIAL PLANETS"),
				*SpectralClass.ToUpper(), Record->PrimaryStarModel.SpectralSubclass,
				Record->SystemModel.PotentialPlanetCount);
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

void UShipNavigationComponent::FoldDistantSystems(const FVector& ObserverLocation, const FString& KeepId)
{
	const TArray<FFoldedSystem> Before = MoveTemp(FoldedSystems);
	FoldedSystems.Reset();
	UWorld* World = GetWorld();
	const double Factor = APSShipNavigation::CVarSystemFold.GetValueOnGameThread();
	if (!World || !(Factor > 0.0))
	{
		return;
	}
	// Every star system with worlds: its system actor (the nearest above its planets; a generated home system may hang
	// under the cluster or the generator, shared with others), the star they circle, how far out they reach.
	struct FSystem
	{
		AActor* Root{nullptr};
		AStar* Star{nullptr};
		double ExtentCm{0.0};
		int32 Planets{0};
	};
	TArray<FSystem, TInlineAllocator<8>> Systems;
	for (TActorIterator<APlanet> It(World); It; ++It)
	{
		APlanet* Planet = *It;
		if (!IsValid(Planet))
		{
			continue;
		}
		AActor* Root = nullptr;
		AStar* Star = nullptr;
		for (AActor* Parent = Planet->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
		{
			Star = Star ? Star : Cast<AStar>(Parent);
			if (Parent->IsA<AStarSystem>())
			{
				Root = Parent;
				break;
			}
		}
		if (!Star)
		{
			continue;
		}
		Root = Root ? Root : Star;
		FSystem* System = Systems.FindByPredicate([Root](const FSystem& Each) { return Each.Root == Root; });
		if (!System)
		{
			System = &Systems.AddDefaulted_GetRef();
			System->Root = Root;
			System->Star = Star;
		}
		System->ExtentCm = FMath::Max(System->ExtentCm, FVector::Dist(Planet->GetActorLocation(), System->Star->GetActorLocation()));
		++System->Planets;
	}
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	for (const FSystem& System : Systems)
	{
		const FVector StarLocation = System.Star->GetActorLocation();
		const double Distance = FVector::Dist(ObserverLocation, StarLocation);
		// Folded once its worlds lie within a few degrees; it unfolds a fifth nearer, so the cards never flicker at the edge.
		const bool bWasFolded = Before.ContainsByPredicate([&System](const FFoldedSystem& Each) { return Each.Root.Get() == System.Root; });
		if (Distance < System.ExtentCm * Factor * (bWasFolded ? 0.8 : 1.0))
		{
			continue;
		}
		// Its worlds, colonies, stations and sites (attached under it, or loose within the reach of its worlds) fold in; a
		// course or the selected target stays apart.
		const double RadiusCm = System.ExtentCm * 1.25 + 1.0e9;
		FoldedSystems.Add({System.Root, System.Star, RadiusCm});
		const FString StarId = System.Star->GetPathName();
		Contacts.RemoveAll([&](const FShipNavigationContact& Contact)
		{
			const AActor* Actor = Contact.Actor.Get();
			if (!Actor || Contact.StableId == KeepId || Contact.StableId == PinnedCourseId || Contact.StableId == StarId)
			{
				return false;
			}
			for (const AActor* Parent = Actor; Parent; Parent = Parent->GetAttachParentActor())
			{
				if (Parent == System.Root)
				{
					return true;
				}
			}
			return FVector::DistSquared(Actor->GetActorLocation(), StarLocation) < FMath::Square(RadiusCm);
		});
		// The card: the system's catalogue name (as on the maps), its class and its worlds; the star's own contact (a
		// course to it) becomes the card.
		FShipNavigationContact* Summary = Contacts.FindByPredicate([&StarId](const FShipNavigationContact& Each)
		{
			return Each.StableId == StarId;
		});
		if (!Summary)
		{
			Summary = &Contacts.AddDefaulted_GetRef();
		}
		const FAPSStarSystemInfo* Info = Stars ? Stars->Get(Stars->FindContaining(StarLocation)) : nullptr;
		Summary->Actor = System.Star;
		Summary->FixedWorldLocation = StarLocation;
		Summary->StableId = StarId;
		Summary->Type = EShipNavigationContactType::StarSystem;
		Summary->bSystemSummary = true;
		Summary->bVirtualContact = false;
		Summary->DistanceCentimeters = Distance;
		Summary->HierarchyLabel.Reset();
		Summary->DisplayName = Info && !Info->Name.IsEmpty() ? Info->Name.ToUpper()
			: !System.Star->AstroName.IsNone() ? System.Star->AstroName.ToString().ToUpper()
			: APSShipNavigation::SanitizeObjectName(System.Star->GetName());
		Summary->TypeLabel = Info && Info->bHome ? TEXT("HOME SYSTEM") : TEXT("STAR SYSTEM");
		const FString Spectral = Info && !Info->Spectral.IsEmpty() ? Info->Spectral
			: System.Star->FullSpectralClass.IsNone() ? FString() : System.Star->FullSpectralClass.ToString().ToUpper();
		Summary->Detail = FString::Printf(TEXT("%s%s%d %s"), *Spectral, Spectral.IsEmpty() ? TEXT("") : TEXT("   |   "),
			System.Planets, System.Planets == 1 ? TEXT("PLANET") : TEXT("PLANETS"));
		Summary->MarkerColour = Info ? APSShipNavigation::CardColour(Info->Colour) : FLinearColor::Transparent;
	}
}

void UShipNavigationComponent::AddNearStarLabels(const FVector& ObserverLocation)
{
	UWorld* World = GetWorld();
	const FAPSStarSystems* Stars = APSStarSystemsFind(World);
	if (!World || !Stars || !Stars->IsReady() || NearStarLabelCount <= 0)
	{
		return;
	}
	// A system whose star stands here shows its worlds or its folded card already; a label would only double it.
	TArray<FVector, TInlineAllocator<8>> Standing;
	for (TActorIterator<AStar> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			Standing.Add(It->GetActorLocation());
		}
	}
	TArray<int32> Nearest;
	Stars->FindNearest(ObserverLocation, NearStarLabelCount + Standing.Num(), Nearest);
	int32 Added = 0;
	for (const int32 Index : Nearest)
	{
		const FAPSStarSystemInfo* Info = Stars->Get(Index);
		if (!Info || Added >= NearStarLabelCount)
		{
			continue;
		}
		const double Near = FMath::Max(Info->RoomCm * 0.5, APSStars::AstronomicalUnitCm);
		if (Standing.ContainsByPredicate([Info, Near](const FVector& At) { return FVector::DistSquared(At, Info->Location) < Near * Near; }))
		{
			continue;
		}
		FShipNavigationContact Contact;
		Contact.FixedWorldLocation = Info->Location;
		Contact.StableId = FString::Printf(TEXT("STAR_SYSTEM:%s"), *Info->Id.ToString(EGuidFormats::DigitsWithHyphensLower));
		Contact.DisplayName = Info->Name.ToUpper();
		Contact.TypeLabel = Stars->IsClaimed(Info->Id) ? TEXT("BEACON") : TEXT("STAR");
		Contact.Detail = FString::Printf(TEXT("%s%s%s"), *Info->Spectral, Info->Spectral.IsEmpty() ? TEXT("") : TEXT("   |   "),
			*APSStars::KnowledgeName(Stars->GetKnowledge(Info->Id)).ToString());
		Contact.Type = EShipNavigationContactType::StarSystem;
		Contact.DistanceCentimeters = FVector::Distance(ObserverLocation, Info->Location);
		Contact.bVirtualContact = true;
		Contact.bStarLabel = true;
		Contact.MarkerColour = APSShipNavigation::CardColour(Info->Colour);
		Contacts.Add(MoveTemp(Contact));
		++Added;
	}
}

bool UShipNavigationComponent::IsInFoldedSystem(const FVector& WorldLocation) const
{
	for (const FFoldedSystem& System : FoldedSystems)
	{
		const AActor* Star = System.Star.Get();
		if (Star && FVector::DistSquared(WorldLocation, Star->GetActorLocation()) < FMath::Square(System.RadiusCm))
		{
			return true;
		}
	}
	return false;
}

bool UShipNavigationComponent::IsContactTypeVisible(EShipNavigationContactType Type) const
{
	switch (Type)
	{
	case EShipNavigationContactType::Planet: return bShowPlanetMarkers;
	case EShipNavigationContactType::Moon: return bShowMoonMarkers;
	case EShipNavigationContactType::Star: return bShowStarMarkers;
	case EShipNavigationContactType::Station: return bShowStationMarkers;
	case EShipNavigationContactType::Settlement:
	case EShipNavigationContactType::Infrastructure: return bShowInfrastructureMarkers;
	default: return false;
	}
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

bool UShipNavigationComponent::SelectContact(const FString& StableId)
{
	const int32 Index = Contacts.IndexOfByPredicate([&StableId](const FShipNavigationContact& Contact)
	{
		return Contact.StableId == StableId;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	SelectedContactIndex = Index;
	return true;
}

bool UShipNavigationComponent::SetCourse(const FString& StableId)
{
	PinnedCourseId = StableId;
	if (const AActor* Owner = GetOwner())
	{
		RefreshContacts(Owner->GetActorLocation(), true);
	}
	if (SelectContact(StableId))
	{
		return true;
	}
	PinnedCourseId.Reset();
	return false;
}

void UShipNavigationComponent::CycleTarget(int32 Direction)
{
	// A target picked by hand releases a course pinned from the terminal.
	PinnedCourseId.Reset();
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
