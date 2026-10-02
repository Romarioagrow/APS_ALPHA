#include "APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Tech/TechInfrastructure.h"
#include "APS_ALPHA/Core/Planetary/APSAtmosphereModel.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Crc.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "APSFleetCommand"

namespace APSFleetAnomalyPrivate
{
	const FName AnomalyTag(TEXT("APS.Fleet.Anomaly"));
	/** How close to the site the pilot on foot must come (cm along the surface). */
	constexpr double InPersonReachCm = 50000.0;
	/** Above sea level, the pilot still counts as on the surface (the highest mountains are lower). */
	constexpr double SurfaceBandCm = 2000000.0;
	/** The beacon's pillar: seen from the air on the way down. */
	constexpr double BeaconHeightCm = 30000.0;
	/** The pilot's distance at which the surface under the beacon has collision to settle it on. */
	constexpr double GroundingRangeCm = 3000000.0;

	void SetInGameName(AActor* Actor, const FText& Name)
	{
		// The world actor keeps its display name protected; it is a reflected property, so set it through reflection.
		if (FTextProperty* NameProperty = Actor ? FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")) : nullptr)
		{
			NameProperty->SetPropertyValue_InContainer(Actor, Name);
		}
	}

	FVector SiteUp(const APlanetaryBody* Body, const FVector& LocalDirection)
	{
		return Body->GetActorTransform().TransformVectorNoScale(LocalDirection).GetSafeNormal();
	}
}

bool APSFleet::RollAnomaly(const FString& WorldKey, const FAnomalyTraits& Traits, int32& OutKind, FVector& OutDirection)
{
	FRandomStream Stream(static_cast<int32>(FCrc::StrCrc32(*(WorldKey + TEXT("/ANOMALY")))));
	if (Stream.FRand() >= 0.4f)
	{
		return false;
	}
	// What the world is like decides what it hides; a derelict probe and ruins can lie anywhere.
	const float Weights[AnomalyKindCount] = {
		1.0f,
		Traits.Temperature < 0.35f ? 2.0f : 0.2f,
		Traits.Seismic > 0.5f ? 2.0f : 0.3f,
		Traits.Biomass > 0.02f ? 2.5f : 0.0f,
		Traits.Metallic > 0.6f ? 2.0f : 0.4f,
		0.35f,
		Traits.bAirless ? 2.0f : 0.2f};
	float Total = 0.0f;
	for (const float Weight : Weights)
	{
		Total += Weight;
	}
	float Pick = Stream.FRand() * Total;
	OutKind = AnomalyKindCount - 1;
	for (int32 Kind = 0; Kind < AnomalyKindCount; ++Kind)
	{
		if (Pick < Weights[Kind])
		{
			OutKind = Kind;
			break;
		}
		Pick -= Weights[Kind];
	}
	// Away from the poles, where a landing site is hard to read.
	const double Latitude = FMath::DegreesToRadians(Stream.FRandRange(-60.0f, 60.0f));
	const double Longitude = FMath::DegreesToRadians(Stream.FRandRange(-180.0f, 180.0f));
	OutDirection = FVector(FMath::Cos(Latitude) * FMath::Cos(Longitude), FMath::Cos(Latitude) * FMath::Sin(Longitude),
		FMath::Sin(Latitude));
	return true;
}

FText APSFleet::AnomalyName(const int32 Kind)
{
	switch (Kind)
	{
	case 0: return LOCTEXT("AnomalyProbe", "DERELICT PROBE");
	case 1: return LOCTEXT("AnomalyCrystal", "CRYSTAL FIELD");
	case 2: return LOCTEXT("AnomalyVents", "THERMAL VENTS");
	case 3: return LOCTEXT("AnomalyLife", "BIOSIGNATURE");
	case 4: return LOCTEXT("AnomalyMagnetic", "MAGNETIC ANOMALY");
	case 5: return LOCTEXT("AnomalyRuins", "ANCIENT RUINS");
	default: return LOCTEXT("AnomalyCrater", "GLASSED CRATER");
	}
}

FText APSFleet::AnomalyStory(const int32 Kind)
{
	switch (Kind)
	{
	case 0: return LOCTEXT("StoryProbe", "a probe of unknown make, cold for ages; its memory core holds star charts nobody here drew.");
	case 1: return LOCTEXT("StoryCrystal", "crystals grown in a field as regular as a lattice: the ice remembers a slow, ordered freeze.");
	case 2: return LOCTEXT("StoryVents", "vents breathing heat and minerals from deep below: energy and rare salts for the colony.");
	case 3: return LOCTEXT("StoryLife", "simple life clustered where nothing should live: samples are on their way to science.");
	case 4: return LOCTEXT("StoryMagnetic", "a buried mass of iron and nickel bending the compass: a rich deposit for industry.");
	case 5: return LOCTEXT("StoryRuins", "walls cut by hands that were not ours, older than the colony's oldest record.");
	default: return LOCTEXT("StoryCrater", "an impact crater fused to glass by a heat no falling rock makes.");
	}
}

FText FAPSFleetCommand::AnomalySiteText(const FVector& Direction)
{
	const double Latitude = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Direction.Z, -1.0, 1.0)));
	const double Longitude = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	return FText::FromString(FString::Printf(TEXT("%.1f %s, %.1f %s"), FMath::Abs(Latitude), Latitude >= 0.0 ? TEXT("N") : TEXT("S"),
		FMath::Abs(Longitude), Longitude >= 0.0 ? TEXT("E") : TEXT("W")));
}

void FAPSFleetCommand::RollAnomaly(FAPSFleetBodyRecord& Record, const APlanetaryBody* Body) const
{
	if (!Body)
	{
		return;
	}
	// Anomalies are what ships find: the home planet, known from the start, hides none (a load restores its record
	// before the flagship marks it home, so this is decided here).
	if (UWorld* LiveWorld = World.Get())
	{
		for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It) && It->HomePlanet == Body)
			{
				return;
			}
		}
	}
	const FAPSResolvedPlanetSurfaceProfile Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body);
	APSFleet::FAnomalyTraits Traits;
	Traits.Temperature = Profile.Temperature;
	Traits.Biomass = Profile.Biomass;
	Traits.Metallic = Profile.Metallic;
	Traits.Seismic = Profile.SeismicActivity;
	Traits.bAirless = APSAtmosphereModel::Density(Body) < 0.05f;
	Record.bHasAnomaly = APSFleet::RollAnomaly(KeyOf(Body), Traits, Record.AnomalyKind, Record.AnomalyDirection);
}

void FAPSFleetCommand::RevealAnomaly(FAPSFleetBodyRecord& Record, const APSFleet::ESurvey Level, const FText& By,
	const bool bAnnounce)
{
	using namespace APSFleet;
	if (!Record.bHasAnomaly || Record.Anomaly == EAnomalyState::Investigated)
	{
		return;
	}
	const FText WorldName = DisplayName(Record.Body.Get());
	const FText Name = AnomalyName(Record.AnomalyKind);
	if (Level >= ESurvey::Surveyed && Record.Anomaly < EAnomalyState::Detected)
	{
		Record.Anomaly = EAnomalyState::Detected;
		if (bAnnounce)
		{
			Post(FText::Format(LOCTEXT("AnomalyDetected", "{0} detected an anomaly on {1}: {2}. A science ship can locate its site."),
				By, WorldName, Name));
		}
	}
	if (Level >= ESurvey::Studied && Record.Anomaly < EAnomalyState::Located)
	{
		Record.Anomaly = EAnomalyState::Located;
		SpawnAnomalyBeacon(Record);
		if (bAnnounce) APSMissionsNotify(World.Get(), APSMissions::EObjective::LocateAnomaly, KeyOf(Record.Body.Get()));
		if (bAnnounce)
		{
			Post(FText::Format(LOCTEXT("AnomalyLocated", "{0} located the {1} on {2} at {3}. Land there on foot or send an expedition; a beacon marks the site (map, course)."),
				By, Name, WorldName, AnomalySiteText(Record.AnomalyDirection)));
		}
	}
	++Revision;
}

void FAPSFleetCommand::InvestigateAnomaly(FAPSFleetBodyRecord& Record, const FText& By, const bool bInPerson,
	const bool bAnnounce)
{
	using namespace APSFleet;
	if (!Record.bHasAnomaly || Record.Anomaly == EAnomalyState::Investigated)
	{
		return;
	}
	Record.Anomaly = EAnomalyState::Investigated;
	Record.bAnomalyInPerson = bInPerson;
	if (bAnnounce) APSMissionsNotify(World.Get(), APSMissions::EObjective::InvestigateAnomaly, KeyOf(Record.Body.Get()));
	const FText Name = AnomalyName(Record.AnomalyKind);
	APSFleetAnomalyPrivate::SetInGameName(Record.AnomalyBeacon.Get(),
		FText::Format(LOCTEXT("BeaconInvestigated", "INVESTIGATED: {0}"), Name));
	if (bAnnounce)
	{
		Post(FText::Format(bInPerson
			? LOCTEXT("AnomalyInPerson", "{0} reached the {1} on {2} on foot: {3} Science counts a visit in person twice.")
			: LOCTEXT("AnomalyExpedition", "{0}'s expedition investigated the {1} on {2}: {3}"),
			By, Name, DisplayName(Record.Body.Get()), AnomalyStory(Record.AnomalyKind)));
	}
	++Revision;
}

void FAPSFleetCommand::SpawnAnomalyBeacon(FAPSFleetBodyRecord& Record)
{
	using namespace APSFleetAnomalyPrivate;
	UWorld* LiveWorld = World.Get();
	APlanetaryBody* Body = Record.Body.Get();
	if (!LiveWorld || !Body || Record.AnomalyBeacon.IsValid())
	{
		return;
	}
	const FVector Up = SiteUp(Body, Record.AnomalyDirection);
	const FVector Site = Body->GetActorLocation() + Up * Body->GetWorldScapeBodyRadiusCm();
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// A plain technological actor: ship navigation lists it as a contact, so a course can be set to the site.
	ATechInfrastructure* Beacon = LiveWorld->SpawnActor<ATechInfrastructure>(ATechInfrastructure::StaticClass(),
		FTransform::Identity, Parameters);
	if (!Beacon)
	{
		return;
	}
	USceneComponent* Root = NewObject<USceneComponent>(Beacon, TEXT("BeaconRoot"));
	Beacon->SetRootComponent(Root);
	Root->RegisterComponent();
	if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		// The engine cylinder is a metre across and a metre tall, centred: a 4 m pillar standing on the site.
		UStaticMeshComponent* Pillar = NewObject<UStaticMeshComponent>(Beacon, TEXT("BeaconPillar"));
		Pillar->SetStaticMesh(Cylinder);
		Pillar->SetupAttachment(Root);
		Pillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pillar->SetCastShadow(false);
		Pillar->SetRelativeScale3D(FVector(4.0, 4.0, BeaconHeightCm / 100.0));
		Pillar->SetRelativeLocation(FVector(0.0, 0.0, BeaconHeightCm * 0.5));
		Pillar->RegisterComponent();
	}
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Beacon, TEXT("BeaconLight"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.0, 0.0, BeaconHeightCm));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(400000.0f);
	Light->SetAttenuationRadius(300000.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.62f, 0.2f));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	Beacon->SetActorLocationAndRotation(Site, FRotationMatrix::MakeFromZ(Up).ToQuat(), false, nullptr,
		ETeleportType::TeleportPhysics);
	Beacon->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	Beacon->SetActorTickEnabled(false);
	Beacon->Tags.Add(AnomalyTag);
	SetInGameName(Beacon, FText::Format(LOCTEXT("BeaconName", "ANOMALY: {0}"), APSFleet::AnomalyName(Record.AnomalyKind)));
	Record.AnomalyBeacon = Beacon;
	Record.bBeaconGrounded = false;
	UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] anomaly beacon %s on %s at %s"), *Beacon->GetName(),
		*DisplayName(Body).ToString(), *AnomalySiteText(Record.AnomalyDirection).ToString());
}

void FAPSFleetCommand::TickAnomalies()
{
	using namespace APSFleet;
	using namespace APSFleetAnomalyPrivate;
	UWorld* LiveWorld = World.Get();
	const APlayerController* Player = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	for (FAPSFleetBodyRecord& Record : Bodies)
	{
		APlanetaryBody* Body = Record.Body.Get();
		if (!Body || !Record.bHasAnomaly || Record.Anomaly < EAnomalyState::Located)
		{
			continue;
		}
		const FVector Up = SiteUp(Body, Record.AnomalyDirection);
		const FVector Centre = Body->GetActorLocation();
		const double Radius = Body->GetWorldScapeBodyRadiusCm();
		// The beacon stands at sea level until the surface near the pilot has collision; then it settles on the ground.
		if (AActor* Beacon = Record.AnomalyBeacon.Get(); Beacon && !Record.bBeaconGrounded
			&& FVector::Dist(Pawn->GetActorLocation(), Beacon->GetActorLocation()) < GroundingRangeCm)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(APSAnomalyBeacon), false, Beacon);
			Params.AddIgnoredActor(Pawn);
			FHitResult Hit;
			if (LiveWorld->LineTraceSingleByChannel(Hit, Centre + Up * (Radius + SurfaceBandCm), Centre + Up * (Radius - SurfaceBandCm),
				ECC_Visibility, Params) && !Cast<APawn>(Hit.GetActor()))
			{
				Beacon->SetActorLocation(Hit.ImpactPoint, false, nullptr, ETeleportType::TeleportPhysics);
				Record.bBeaconGrounded = true;
				UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] anomaly beacon %s settled %.0f m from sea level"), *Beacon->GetName(),
					(FVector::Dist(Hit.ImpactPoint, Centre) - Radius) / 100.0);
			}
		}
		// The pilot on foot at the site investigates it in person.
		if (Record.Anomaly == EAnomalyState::Located && Pawn->IsA<ACustomGravityCharacter>())
		{
			const FVector FromCentre = Pawn->GetActorLocation() - Centre;
			const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(FromCentre.GetSafeNormal(), Up), -1.0, 1.0));
			if (Angle * Radius < InPersonReachCm && FromCentre.Size() - Radius < SurfaceBandCm)
			{
				InvestigateAnomaly(Record, LOCTEXT("ThePilot", "The pilot"), true, true);
			}
		}
	}
}

const FAPSFleetBodyRecord* FAPSFleetCommand::FindAnomalyByBeacon(const AActor* Beacon) const
{
	return Beacon ? Bodies.FindByPredicate([Beacon](const FAPSFleetBodyRecord& Record)
	{
		return Record.AnomalyBeacon.Get() == Beacon;
	}) : nullptr;
}

FText FAPSFleetCommand::DescribeAnomaly(const AActor* Body) const
{
	using namespace APSFleet;
	const FAPSFleetBodyRecord* Record = FindBody(Body);
	if (!Record || !Record->bHasAnomaly)
	{
		return FText::GetEmpty();
	}
	const FText Name = AnomalyName(Record->AnomalyKind);
	switch (Record->Anomaly)
	{
	case EAnomalyState::Detected:
		return FText::Format(LOCTEXT("DescribeDetected", "ANOMALY: {0}, site unknown. A science ship's study locates it."), Name);
	case EAnomalyState::Located:
		return FText::Format(LOCTEXT("DescribeLocated", "ANOMALY: {0} at {1}. Land there on foot, or order an EXPEDITION."),
			Name, AnomalySiteText(Record->AnomalyDirection));
	case EAnomalyState::Investigated:
		return FText::Format(Record->bAnomalyInPerson
			? LOCTEXT("DescribeInPerson", "ANOMALY INVESTIGATED ON FOOT: {0}: {1}")
			: LOCTEXT("DescribeInvestigated", "ANOMALY INVESTIGATED: {0}: {1}"), Name, AnomalyStory(Record->AnomalyKind));
	default:
		return FText::GetEmpty();
	}
}

void FAPSFleetCommand::LogAnomalies()
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		const FAPSFleetBodyRecord& Record = BodyRecord(*It);
		UE_LOG(LogTemp, Log, TEXT("[APS.Fleet] anomaly %s: %s state=%d site=%s beacon=%s"), *DisplayName(*It).ToString(),
			Record.bHasAnomaly ? *APSFleet::AnomalyName(Record.AnomalyKind).ToString() : TEXT("none"),
			static_cast<int32>(Record.Anomaly), *AnomalySiteText(Record.AnomalyDirection).ToString(),
			*GetNameSafe(Record.AnomalyBeacon.Get()));
	}
}

#undef LOCTEXT_NAMESPACE
