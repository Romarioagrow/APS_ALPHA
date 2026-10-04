#include "APSInfrastructure.h"

#include "APSMissions.h"
#include "APSStarSystems.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Megastructures/APSMegastructures.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Gameplay/Construction/APSConstructionCatalog.h"
#include "HAL/PlatformTime.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "APSInfrastructure"

namespace APSInfrastructureLocal
{
	using namespace APSInfrastructure;

	TMap<const UWorld*, FAPSInfrastructure*> GRegistry;

	constexpr int32 ResourceCount = static_cast<int32>(EResource::Count);
	/** What the colony starts with and what the home produces before anything is built (per minute). */
	constexpr float StartStocks[ResourceCount] = {300.0f, 60.0f, 150.0f, 50.0f, 50.0f};
	constexpr float HomeYield[ResourceCount] = {4.0f, 1.0f, 4.0f, 1.0f, 1.0f};
	/** Surface objects settle on the ground within this range of the pilot, searching this band around sea level. */
	constexpr double SettleRangeCm = 3000000.0;
	constexpr double SurfaceBandCm = 2000000.0;
	const FName BuiltTag(TEXT("APS.Infra.Built"));
	const FName SettledTag(TEXT("APS.Infra.Settled"));
	const FName SurfaceTag(TEXT("APS.Infra.Surface"));
	/** Placed by the surface field from afar; the terrain's collision near the pilot still refines it. */
	const FName FieldSettledTag(TEXT("APS.Infra.SettledField"));
	/** Raised by hand in build mode: it stands where the player put it. */
	const FName PlacedTag(TEXT("APS.Infra.Placed"));
	/** Restored props settle within this range of a pilot who has stood on the ground this long; moves under the
	 * tolerance are left alone, and the ground is looked for this far above and below. */
	constexpr double PropSettleRangeCm = 30000.0;
	constexpr float PropSettleAfterGroundSeconds = 2.0f;
	constexpr double PropSettleToleranceCm = 10.0;
	constexpr double PropSettleSearchCm = 4000.0;

	/** Surface actors still to settle, per world, refreshed every few seconds. */
	struct FSettleCache
	{
		TArray<TWeakObjectPtr<AActor>> Candidates;
		double NextScan{0.0};
	};
	TMap<TWeakObjectPtr<const UWorld>, FSettleCache> GSettleCaches;

	/**
	 * The ground, or the sea over it, under a direction from the body's centre by its WorldScape field, cm from the
	 * centre (the height the flight model measures); negative while the body's root does not carry its own surface.
	 */
	double FieldSurfaceRadius(const APlanetaryBody* Body, const FVector& Up)
	{
		APlanetarySurfaceGenerator* Surface = Body ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0 || !Surface->IsSurfaceProfileCurrent(Body))
		{
			return -1.0;
		}
		const FVector Centre = Root->GetActorLocation();
		double Radius = Root->PlanetScale + Root->GetGroundHeight(Centre + Up * Root->PlanetScale, false);
		if (Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None)
		{
			Radius = FMath::Max(Radius, Root->PlanetScale
				+ static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity);
		}
		return FMath::IsFinite(Radius) ? Radius : -1.0;
	}

	/** An even spread of directions (golden spiral) so structures at one place do not overlap. */
	FVector SpreadDirection(const int32 Ordinal, const FVector& Up)
	{
		const double Golden = UE_PI * (3.0 - FMath::Sqrt(5.0));
		const double Y = 1.0 - 2.0 * (FMath::Fmod(Ordinal * 0.618034, 1.0));
		const double Ring = FMath::Sqrt(FMath::Max(0.0, 1.0 - Y * Y));
		const double Angle = Golden * Ordinal;
		const FVector Local(FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, Y);
		const FQuat Frame = FRotationMatrix::MakeFromZ(Up.IsNearlyZero() ? FVector::UpVector : Up).ToQuat();
		return Frame.RotateVector(Local).GetSafeNormal();
	}

	bool IsGiant(const APlanetaryBody* Body)
	{
		return Body && (Body->PlanetType == EPlanetType::GasGiant || Body->PlanetType == EPlanetType::HotGiant
			|| Body->PlanetType == EPlanetType::IceGiant);
	}

	FText SiteName(const AActor* Site, const FAPSStarSystems* Stars)
	{
		FGuid Id;
		if (Stars && FAPSStarSystems::AnchorSystem(Site, Id))
		{
			const FAPSStarSystemInfo* Info = Stars->Find(Id);
			return Info ? FText::FromString(Info->Name) : FText::GetEmpty();
		}
		if (const ACelestialBody* Body = Cast<ACelestialBody>(Site); Body && !Body->AstroName.IsNone())
		{
			return FText::FromString(Body->AstroName.ToString().ToUpper());
		}
		return Site ? FText::FromString(Site->GetName()) : FText::GetEmpty();
	}
}

FArchive& operator<<(FArchive& Ar, FAPSInfrastructureSaveData& Data)
{
	// Version 2 (build mode, Rio 02.10) appends the props placed by hand and the names of the structures raised by hand;
	// a version 1 blob simply has neither.
	uint8 Version = 2;
	Ar << Version;
	int32 Count = Data.Structures.Num();
	Ar << Count;
	if (Ar.IsLoading())
	{
		Data.Structures.SetNum(FMath::Clamp(Count, 0, 100000));
	}
	for (FAPSInfrastructureSaveData::FStructure& Structure : Data.Structures)
	{
		FString Type = Structure.Type.ToString();
		Ar << Type;
		Structure.Type = FName(*Type);
		Ar << Structure.SiteKey;
		Ar << Structure.SystemId;
		Ar << Structure.ActorName;
		Ar << Structure.RelativeTransform;
		Ar << Structure.BuiltSeconds;
	}
	Ar << Data.Stocks;
	if (Version >= 2)
	{
		int32 PropCount = Data.Props.Num();
		Ar << PropCount;
		if (Ar.IsLoading())
		{
			Data.Props.SetNum(FMath::Clamp(PropCount, 0, 100000));
		}
		for (FAPSInfrastructureSaveData::FProp& Prop : Data.Props)
		{
			FString PropId = Prop.PropId.ToString();
			Ar << PropId;
			Prop.PropId = PropId.IsEmpty() ? NAME_None : FName(*PropId);
			Ar << Prop.MeshPath;
			Ar << Prop.SiteKey;
			Ar << Prop.RelativeTransform;
			Ar << Prop.Scale;
			uint8 Flags = Prop.bOnGround ? 1 : 0;
			Ar << Flags;
			Prop.bOnGround = (Flags & 1) != 0;
		}
		Ar << Data.PlacedStructures;
	}
	return Ar;
}

FAPSInfrastructure* APSInfrastructureFind(const UWorld* World)
{
	return World ? APSInfrastructureLocal::GRegistry.FindRef(World) : nullptr;
}

void APSInfrastructureRegister(const UWorld* World, FAPSInfrastructure* Infrastructure)
{
	if (!World) return;
	if (Infrastructure) APSInfrastructureLocal::GRegistry.Add(World, Infrastructure);
	else APSInfrastructureLocal::GRegistry.Remove(World);
}

FAPSInfrastructure::FAPSInfrastructure(UWorld* InWorld)
	: World(InWorld)
	, Yard(MakeUnique<FAPSMegastructureYard>(InWorld))
{
	for (int32 Index = 0; Index < APSInfrastructureLocal::ResourceCount; ++Index)
	{
		Stocks[Index] = APSInfrastructureLocal::StartStocks[Index];
	}
	RecountRates();
}

FAPSInfrastructure::~FAPSInfrastructure() = default;

void FAPSInfrastructure::Tick(const float DeltaSeconds)
{
	using namespace APSInfrastructureLocal;
	for (int32 Index = 0; Index < ResourceCount; ++Index)
	{
		Stocks[Index] = FMath::Max(0.0f, Stocks[Index] + Rates[Index] * DeltaSeconds / 60.0f);
	}
	if (PendingRestore.IsSet())
	{
		RestoreWait += DeltaSeconds;
		// The sites (bodies, the star catalogue) come with the generated hierarchy; give them a moment.
		const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		if (RestoreWait > 2.0f && ((Stars && Stars->IsReady()) || RestoreWait > 30.0f))
		{
			ApplyPendingRestore();
		}
	}
	SettleClock += DeltaSeconds;
	if (SettleClock >= 0.5f)
	{
		const float SettleElapsed = SettleClock;
		SettleClock = 0.0f;
		SettleSurfaceActors();
		SettlePlacedProps(SettleElapsed);
	}
	// Rio 03.10: the hubs and megastructures under construction grow with the construction ships' work (twice a second);
	// after a load only once the structures they stand on are back (a ring's scaffold hangs on its elevator).
	if (Yard && !PendingRestore.IsSet())
	{
		Yard->Tick(DeltaSeconds, *this);
	}
}

int32 FAPSInfrastructure::DepartmentLevel(const UWorld* World, const APSInfrastructure::EDepartment Department)
{
	using APSInfrastructure::EDepartment;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMainGameplayInstance* State = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	const UCivilization* Civ = State ? State->CurrentCivilization.Get() : nullptr;
	const FAPSFleetCommand* Fleet = APSFleetFind(World);
	const FAPSMissionBoard* Missions = APSMissionsFind(World);
	int32 Base = 0;
	int32 Earned = 0;
	switch (Department)
	{
	case EDepartment::Exploration:
		Base = Civ ? Civ->Divisions.Exploration : 1;
		Earned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Exploration) : 0;
		break;
	case EDepartment::Industry:
	case EDepartment::Transport:
		Base = Civ ? Civ->Divisions.Industry : 1;
		Earned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Construction) : 0;
		break;
	case EDepartment::Science:
		Base = Civ ? Civ->Divisions.Science : 1;
		Earned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::Science) : 0;
		break;
	case EDepartment::CivilAffairs:
		Base = Civ ? Civ->Divisions.CivilAffairs : 1;
		break;
	case EDepartment::Military:
		Base = Civ ? Civ->Divisions.Military : 1;
		Earned = Fleet ? Fleet->GetEarnedLevel(APSFleet::EDivision::MainFleet) : 0;
		break;
	case EDepartment::FleetCommand:
		Base = Civ ? Civ->Divisions.FleetCommand : 1;
		Earned = Fleet ? Fleet->GetEarnedFleetCommandLevel() : 0;
		break;
	default:
		break;
	}
	return FMath::Max(Base, 0) + Earned + (Missions ? Missions->GetEarnedLevels(Department) : 0);
}

bool FAPSInfrastructure::SiteSystem(const UWorld* World, const AActor* Site, FGuid& OutSystemId)
{
	if (FAPSStarSystems::AnchorSystem(Site, OutSystemId)) return true;
	if (Cast<AStar>(Site))
	{
		// The home star stands for the home system (the only star with an actor in a game).
		const FAPSStarSystems* Stars = APSStarSystemsFind(World);
		if (const FAPSStarSystemInfo* Home = Stars ? Stars->GetHome() : nullptr)
		{
			OutSystemId = Home->Id;
			return true;
		}
	}
	return false;
}

FString FAPSInfrastructure::SiteKeyOf(const AActor* Site, FGuid& OutSystemId) const
{
	OutSystemId.Invalidate();
	if (SiteSystem(World.Get(), Site, OutSystemId))
	{
		return TEXT("SYSTEM:") + OutSystemId.ToString(EGuidFormats::Digits);
	}
	return FAPSFleetCommand::KeyOf(Site);
}

FText FAPSInfrastructure::CheckBuild(const FName TypeId, const AActor* Site) const
{
	using namespace APSInfrastructure;
	const FType* Type = Find(TypeId);
	if (!Type) return LOCTEXT("UnknownType", "Unknown structure.");
	if (!Site) return LOCTEXT("NoSite", "Pick a world or a star system first.");
	UWorld* LiveWorld = World.Get();
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	FGuid SystemId;
	const bool bSystem = SiteSystem(LiveWorld, Site, SystemId);
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Site);
	if (Type->Placement == EPlacement::StarSystem && !bSystem)
	{
		return LOCTEXT("NeedsSystem", "Raised in a star system, around its star: pick a star system.");
	}
	if (Type->Placement != EPlacement::StarSystem && !Body)
	{
		return Type->Placement == EPlacement::Surface ? LOCTEXT("NeedsSurface", "Raised on a world's surface: pick a planet or moon.")
			: LOCTEXT("NeedsOrbit", "Raised in orbit of a world: pick a planet or moon.");
	}
	if (Type->bGiantOnly && !APSInfrastructureLocal::IsGiant(Body))
	{
		return LOCTEXT("NeedsGiant", "Only at a gas or ice giant.");
	}
	if (Type->Placement == EPlacement::Surface && APSInfrastructureLocal::IsGiant(Body))
	{
		return LOCTEXT("NoSurface", "A giant has no surface to build on.");
	}
	if (Type->bNeedsGround && APSInfrastructureLocal::IsGiant(Body))
	{
		return LOCTEXT("NoGround", "A giant has no ground to anchor it.");
	}
	if (bSystem)
	{
		const APSStars::EKnowledge Known = Stars ? Stars->GetKnowledge(SystemId) : APSStars::EKnowledge::Catalogued;
		if (static_cast<uint8>(Known) < Type->RequiredKnowledge)
		{
			return Type->RequiredKnowledge >= 2 ? LOCTEXT("NeedsSystemSurvey", "Survey the star system first (a ship there, or a visit in person).")
				: LOCTEXT("NeedsSystemScan", "Scan the star system first (a probe or a scanner in reach).");
		}
	}
	else if (const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld))
	{
		if (static_cast<uint8>(Fleet->GetSurvey(Body)) < Type->RequiredKnowledge)
		{
			return Type->RequiredKnowledge >= 2 ? LOCTEXT("NeedsStudy", "Study the world first (a science ship).")
				: LOCTEXT("NeedsSurvey", "Survey the world first (an exploration ship).");
		}
	}
	if (Type->bNeedsUnlock)
	{
		const FAPSMissionBoard* Missions = APSMissionsFind(LiveWorld);
		if (!Missions || !Missions->IsUnlocked(Type->Id))
		{
			return FText::Format(LOCTEXT("NeedsUnlock", "Unlocked by a {0} mission."), DepartmentName(Type->Department));
		}
	}
	// Rio 03.10, chains: each step stands on the one before it (the counts here, the rule in the catalogue).
	FChainState Chain;
	Chain.bStationHere = Type->bNeedsStationHere && HasStationAt(Site);
	Chain.RequiredHere = Type->RequiresAtSite.IsNone() ? 0 : CountAt(Site, Type->RequiresAtSite);
	Chain.RequiredInSystem = Type->RequiresInSystem.IsNone() ? 0 : CountInSystem(Site, Type->RequiresInSystem);
	if (const FText Refusal = ChainRefusal(*Type, Chain); !Refusal.IsEmpty())
	{
		return Refusal;
	}
	if (const int32 Level = DepartmentLevel(LiveWorld, Type->Department); Level < Type->RequiredLevel)
	{
		return FText::Format(LOCTEXT("NeedsLevel", "{0} level {1} needed (now {2})."), DepartmentName(Type->Department),
			FText::AsNumber(Type->RequiredLevel), FText::AsNumber(Level));
	}
	if (const int32 Limit = Type->LimitPerSite + BerthsAt(Site, *Type); CountAt(Site, Type->Id) >= Limit)
	{
		return FText::Format(LOCTEXT("AtLimit", "Already {0} here (the most one place takes)."), FText::AsNumber(Limit));
	}
	// Hubs and world megastructures need room: no moon may cross their orbits.
	if (APSInfrastructure::IsMegaVisual(Type->Visual))
	{
		if (const FText Room = APSMegastructures::CheckRoom(*Type, Site); !Room.IsEmpty())
		{
			return Room;
		}
	}
	if (!CanAfford(Type->Cost))
	{
		return FText::Format(LOCTEXT("CannotAfford", "Needs {0}."), DescribeAmounts(Type->Cost));
	}
	return FText::GetEmpty();
}

void FAPSInfrastructure::GetOptions(const AActor* Site, TArray<TPair<FName, FText>>& OutTypesAndRefusals) const
{
	using namespace APSInfrastructure;
	OutTypesAndRefusals.Reset();
	if (!Site) return;
	FGuid SystemId;
	const bool bSystem = SiteSystem(World.Get(), Site, SystemId);
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Site);
	for (const FType& Type : Types())
	{
		const bool bFits = Type.Placement == EPlacement::StarSystem ? bSystem : Body != nullptr;
		if (!bFits) continue;
		if (Type.bGiantOnly && !APSInfrastructureLocal::IsGiant(Body)) continue;
		if ((Type.Placement == EPlacement::Surface || Type.bNeedsGround) && APSInfrastructureLocal::IsGiant(Body)) continue;
		OutTypesAndRefusals.Emplace(Type.Id, CheckBuild(Type.Id, Site));
	}
}

bool FAPSInfrastructure::Reserve(const FName TypeId)
{
	const APSInfrastructure::FType* Type = APSInfrastructure::Find(TypeId);
	if (!Type || !CanAfford(Type->Cost)) return false;
	for (const APSInfrastructure::FAmount& Amount : Type->Cost)
	{
		Stocks[static_cast<int32>(Amount.Resource)] -= Amount.Value;
	}
	++Revision;
	return true;
}

void FAPSInfrastructure::Refund(const FName TypeId)
{
	if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(TypeId))
	{
		for (const APSInfrastructure::FAmount& Amount : Type->Cost)
		{
			Stocks[static_cast<int32>(Amount.Resource)] += Amount.Value;
		}
		++Revision;
	}
}

AActor* FAPSInfrastructure::SpawnVisual(const APSInfrastructure::FType& Type, AActor* Site, const FTransform& Transform,
	const FString& ActorName, const FText& Name)
{
	using APSInfrastructure::EVisual;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld) return nullptr;
	// Rio 03.10: hubs and megastructures wear the hand-made level's own meshes, fitted to their world
	// (Gameplay/Megastructures); the elevator's root stands on the ground and settles like every surface structure.
	AActor* Actor = nullptr;
	if (APSInfrastructure::IsMegaVisual(Type.Visual))
	{
		Actor = APSMegastructures::SpawnStructure(LiveWorld, Type, Site, Transform, ActorName, Name, *this);
		if (Actor && APSMegastructures::KindOf(Type.Visual) == APSMegastructures::EKind::SpaceElevator)
		{
			Actor->Tags.AddUnique(APSInfrastructureLocal::SurfaceTag);
		}
	}
	else
	{
		// The families the home complex was raised with (the menu's station, shipyard and headquarters choices).
		UClass* Class = AAutonomousOutpost::StaticClass();
		if (Type.Visual == EVisual::Station || Type.Visual == EVisual::Shipyard || Type.Visual == EVisual::Headquarters)
		{
			for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
			{
				UClass* Found = Type.Visual == EVisual::Shipyard ? It->BP_HomeSpaceShipyard.Get()
					: Type.Visual == EVisual::Headquarters ? It->BP_HomeSpaceHeadquarters.Get() : It->BP_HomeSpaceStation.Get();
				if (Found) { Class = Found; break; }
			}
		}
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (!ActorName.IsEmpty())
		{
			Parameters.Name = FName(*ActorName);
			Parameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
		}
		Actor = LiveWorld->SpawnActor<AActor>(Class, Transform.GetLocation(), Transform.Rotator(), Parameters);
		if (!Actor) return nullptr;
		const float Scale = Type.Visual == EVisual::Beacon ? 0.6f : Type.VisualScale;
		if (!FMath::IsNearlyEqual(Scale, 1.0f)) Actor->SetActorScale3D(FVector(Scale));
		if (Site) Actor->AttachToActor(Site, FAttachmentTransformRules::KeepWorldTransform);
		if (ASpaceStation* Station = Cast<ASpaceStation>(Actor)) Station->CalculateAffectionRadius();
	}
	if (!Actor) return nullptr;
	Actor->Tags.AddUnique(TEXT("APS.GeneratedCivilization"));
	Actor->Tags.AddUnique(APSInfrastructureLocal::BuiltTag);
	Actor->Tags.AddUnique(FName(*(TEXT("APS.Infrastructure.") + Type.Id.ToString())));
	if (Type.Placement == APSInfrastructure::EPlacement::Surface) Actor->Tags.AddUnique(APSInfrastructureLocal::SurfaceTag);
	// The world actor keeps its display name protected; it is a reflected property, so set it through reflection.
	if (FTextProperty* NameProperty = FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")))
	{
		NameProperty->SetPropertyValue_InContainer(Actor, Name);
	}
	return Actor;
}

AActor* FAPSInfrastructure::Complete(const FName TypeId, AActor* Site, const FVector* NearLocation)
{
	using namespace APSInfrastructure;
	const FType* Type = Find(TypeId);
	UWorld* LiveWorld = World.Get();
	if (!Type || !Site || !LiveWorld) return nullptr;
	FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	FGuid SystemId;
	const FString SiteKey = SiteKeyOf(Site, SystemId);
	const int32 Standing = [this, Site]()
	{
		TArray<const FAPSBuiltStructure*> Here;
		GetAt(Site, Here);
		return Here.Num();
	}();
	// Where it stands: beside the builder in orbit, on the ground below it, or around the star.
	const FVector Centre = Site->GetActorLocation();
	FVector Location = Centre;
	FVector Up = FVector::UpVector;
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Site))
	{
		const double Radius = FMath::Max(Body->GetWorldScapeBodyRadiusCm(), 100000.0);
		const FVector Toward = NearLocation && !(*NearLocation - Centre).IsNearlyZero()
			? (*NearLocation - Centre).GetSafeNormal() : APSInfrastructureLocal::SpreadDirection(Standing, Body->GetActorUpVector());
		if (Type->Placement == EPlacement::Surface)
		{
			// Spread over the ground around the point below the builder, a few kilometres apart.
			const FVector Side = APSInfrastructureLocal::SpreadDirection(Standing + 1, Toward);
			Up = (Toward + (Side - Toward * FVector::DotProduct(Side, Toward)) * (0.02 * FMath::Min(Standing, 8))).GetSafeNormal();
			Location = Centre + Up * Radius;
		}
		else
		{
			Up = Toward;
			const FVector Along = FVector::CrossProduct(Body->GetActorUpVector(), Toward).GetSafeNormal();
			const double Orbit = NearLocation ? FVector::Dist(*NearLocation, Centre) : APSFleet::SlotRadius(Radius) * 1.15;
			Location = Centre + Toward * Orbit - (Along.IsNearlyZero() ? FVector::ForwardVector : Along) * (700000.0 + 500000.0 * Standing);
		}
	}
	else
	{
		// Around the star, clear of its glare and of the other structures there.
		double StarRadius = 7.0e10;
		if (const AStar* Star = Cast<AStar>(Site); Star && Star->StarRadiusKM > 0) StarRadius = Star->StarRadiusKM * 100000.0;
		Up = APSInfrastructureLocal::SpreadDirection(Standing, FVector::UpVector);
		Location = Centre + Up * FMath::Max(0.05 * APSStars::AstronomicalUnitCm, StarRadius * 40.0) * (1.0 + 0.15 * Standing);
		// Rio 03.10 (the ADMINISTRATION HUB stood 48 AU out at a supergiant's edge: "put it by the planet"): what governs
		// the system stands in a high orbit of its home world, the planet with the colony, else the first; planets keep
		// their places, so the spot stays by the world.
		const AStar* SystemStar = Cast<AStar>(Site);
		if (const AStarSystem* System = SystemStar ? nullptr : Cast<AStarSystem>(Site))
		{
			SystemStar = System->MainStar;
		}
		if (Type->bClaims && IsValid(SystemStar))
		{
			TArray<APlanet*> Worlds = SystemStar->Planets;
			if (Worlds.IsEmpty() && IsValid(SystemStar->PlanetarySystem))
			{
				Worlds = SystemStar->PlanetarySystem->PlanetsActorsList;
			}
			const APlanet* Home = nullptr;
			for (APlanet* Candidate : Worlds)
			{
				if (!IsValid(Candidate))
				{
					continue;
				}
				Home = Home ? Home : Candidate;
				TArray<AActor*> Attached;
				Candidate->GetAttachedActors(Attached);
				if (Attached.ContainsByPredicate([](const AActor* Child) { return IsValid(Child) && Child->IsA<AColony>(); }))
				{
					Home = Candidate;
					break;
				}
			}
			if (Home)
			{
				const double WorldRadius = FMath::Max(Home->GetWorldScapeBodyRadiusCm(), 100000.0);
				Up = APSInfrastructureLocal::SpreadDirection(Standing + 3, Home->GetActorUpVector());
				Location = Home->GetActorLocation() + Up * APSFleet::SlotRadius(WorldRadius) * (1.6 + 0.1 * Standing);
			}
		}
	}
	FTransform Placement(FRotationMatrix::MakeFromZ(Up).ToQuat(), Location);
	// Rio 03.10: hubs and megastructures stand where their world's geometry puts them (the elevator on the equator, the
	// ring round it, a hub in a high orbit, a swarm round the star); their scaffold goes when they stand.
	if (APSInfrastructure::IsMegaVisual(Type->Visual))
	{
		APSMegastructures::PlaceRoot(*Type, *Site, *this, NearLocation, Placement);
		if (Yard)
		{
			Yard->Finish(Site, Type->Id);
		}
	}
	const FText Name = FText::Format(LOCTEXT("StructureName", "{0} {1}"), Type->Name, APSInfrastructureLocal::SiteName(Site, Stars));
	const FString ActorName = FString::Printf(TEXT("APS_Infra_%s_%d"), *Type->Id.ToString(), ++Serial);
	AActor* Actor = SpawnVisual(*Type, Site, Placement, ActorName, Name);
	if (!Actor) return nullptr;
	FAPSBuiltStructure& Built = Structures.AddDefaulted_GetRef();
	Built.Type = Type->Id;
	Built.SiteKey = SiteKey;
	Built.SystemId = SystemId;
	Built.Actor = Actor;
	Built.ActorName = Actor->GetName();
	Built.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Site->GetActorTransform());
	Built.BuiltSeconds = LiveWorld->GetTimeSeconds();
	RecountRates();
	++Revision;
	if (Stars && SystemId.IsValid()) Stars->AddStructure(SystemId, Type->Id, Type->bClaims);
	APSMissionsNotify(LiveWorld, APSMissions::EObjective::BuildStructure, Type->Id.ToString());
	UAPSCivilizationJournalSubsystem::Post(LiveWorld, TEXT("Infrastructure"), FText::Format(
		LOCTEXT("Built", "{0} stands. {1}"), Name, Type->Role));
	UE_LOG(LogTemp, Log, TEXT("[APS.Infra] built %s at %s (%s)"), *Type->Id.ToString(), *SiteKey, *Actor->GetName());
	return Actor;
}

AActor* FAPSInfrastructure::CompleteAt(const FName TypeId, AActor* Site, const FTransform& WorldTransform)
{
	using namespace APSInfrastructureLocal;
	// Rio 03.10: hubs and megastructures are raised by construction ships only, in stages, where their world puts them;
	// build mode's caller refunds the cost when this returns null.
	if (const FType* Fixed = Find(TypeId); Fixed && Fixed->bFleetOnly)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Infra] %s is raised by construction ships only, not by hand"), *TypeId.ToString());
		if (UWorld* LiveWorld = World.Get())
		{
			UAPSCivilizationJournalSubsystem::Post(LiveWorld, TEXT("Infrastructure"), FText::Format(LOCTEXT("FleetOnly",
				"{0} is raised by construction ships: order it from INFRASTRUCTURE, CONSTRUCTION CATALOGUE, BUILD..."), Fixed->Name));
		}
		return nullptr;
	}
	const FVector Near = WorldTransform.GetLocation();
	AActor* Actor = Complete(TypeId, Site, &Near);
	if (!Actor || !Site)
	{
		return Actor;
	}
	// Build mode (Rio 02.10: "the structure must appear where placed"): the player's spot and turn instead of the runtime's
	// own spread, and never settled again (the settling would turn it back to the bare up axis).
	Actor->SetActorLocationAndRotation(WorldTransform.GetLocation(), WorldTransform.GetRotation(), false, nullptr,
		ETeleportType::TeleportPhysics);
	Actor->Tags.AddUnique(SettledTag);
	Actor->Tags.AddUnique(PlacedTag);
	if (FAPSBuiltStructure* Built = Structures.FindByPredicate([Actor](const FAPSBuiltStructure& Structure)
		{
			return Structure.Actor.Get() == Actor;
		}))
	{
		Built->RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Site->GetActorTransform());
		Built->bPlaced = true;
	}
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Infra] %s raised by hand at %s"), *Actor->GetName(), *WorldTransform.GetLocation().ToCompactString());
	return Actor;
}

void FAPSInfrastructure::AddPlacedProp(AActor* Actor, AActor* Site, const FName PropId, const FString& MeshPath,
	const bool bOnGround)
{
	if (!Actor)
	{
		return;
	}
	FAPSPlacedProp& Prop = PlacedProps.AddDefaulted_GetRef();
	Prop.PropId = PropId;
	Prop.MeshPath = MeshPath;
	FGuid SystemId;
	Prop.SiteKey = Site ? SiteKeyOf(Site, SystemId) : FString();
	Prop.RelativeTransform = Site ? Actor->GetActorTransform().GetRelativeTransform(Site->GetActorTransform())
		: Actor->GetActorTransform();
	Prop.Scale = Actor->GetActorScale3D();
	Prop.bOnGround = bOnGround;
	Prop.Actor = Actor;
	++Revision;
}

bool FAPSInfrastructure::RemovePlacedProp(const AActor* Actor)
{
	const int32 Removed = Actor
		? PlacedProps.RemoveAll([Actor](const FAPSPlacedProp& Prop) { return Prop.Actor.Get() == Actor; }) : 0;
	if (Removed > 0)
	{
		++Revision;
	}
	return Removed > 0;
}

void FAPSInfrastructure::GetAt(const AActor* Site, TArray<const FAPSBuiltStructure*>& OutStructures) const
{
	OutStructures.Reset();
	FGuid SystemId;
	const FString SiteKey = SiteKeyOf(Site, SystemId);
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (Structure.SiteKey == SiteKey) OutStructures.Add(&Structure);
	}
}

int32 FAPSInfrastructure::CountAt(const AActor* Site, const FName Type) const
{
	TArray<const FAPSBuiltStructure*> Here;
	GetAt(Site, Here);
	int32 Count = 0;
	for (const FAPSBuiltStructure* Structure : Here) Count += Structure->Type == Type ? 1 : 0;
	return Count;
}

const FAPSBuiltStructure* FAPSInfrastructure::FindByActor(const AActor* Actor) const
{
	return Actor ? Structures.FindByPredicate([Actor](const FAPSBuiltStructure& Structure) { return Structure.Actor.Get() == Actor; })
		: nullptr;
}

AActor* FAPSInfrastructure::FindActorAt(const AActor* Site, const FName Type) const
{
	TArray<const FAPSBuiltStructure*> Here;
	GetAt(Site, Here);
	for (const FAPSBuiltStructure* Structure : Here)
	{
		if (Structure->Type == Type && Structure->Actor.IsValid())
		{
			return Structure->Actor.Get();
		}
	}
	return nullptr;
}

bool FAPSInfrastructure::HasStationAt(const AActor* Site) const
{
	using namespace APSInfrastructure;
	if (!Cast<APlanetaryBody>(Site))
	{
		return false;
	}
	TArray<const FAPSBuiltStructure*> Here;
	GetAt(Site, Here);
	for (const FAPSBuiltStructure* Structure : Here)
	{
		const FType* Type = Find(Structure->Type);
		if (Type && (Type->Category == ECategory::Station || Type->Category == ECategory::Hub))
		{
			return true;
		}
	}
	// The fleet's own: the generated home complex and the stations, shipyards and headquarters its ships built.
	if (const FAPSFleetCommand* Fleet = APSFleetFind(World.Get()))
	{
		for (const APSFleet::EStructure Kind : {APSFleet::EStructure::Station, APSFleet::EStructure::Shipyard, APSFleet::EStructure::Headquarters})
		{
			if (Fleet->CountStructures(Site, Kind) > 0)
			{
				return true;
			}
		}
	}
	return false;
}

int32 FAPSInfrastructure::CountInSystem(const AActor* Site, const FName Type) const
{
	// A planet or moon stands in the home system (the only one materialized); a star or an anchor names its own.
	UWorld* LiveWorld = World.Get();
	FGuid SystemId;
	const bool bSystemSite = SiteSystem(LiveWorld, Site, SystemId) && !Cast<APlanetaryBody>(Site);
	const FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	const FAPSStarSystemInfo* Home = Stars ? Stars->GetHome() : nullptr;
	const bool bHomeSystem = !bSystemSite || (Home && Home->Id == SystemId);
	int32 Count = 0;
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (Structure.Type != Type)
		{
			continue;
		}
		const bool bAtBody = !Structure.SystemId.IsValid();
		Count += (bAtBody && bHomeSystem) || (!bAtBody && bSystemSite && Structure.SystemId == SystemId)
			|| (!bAtBody && !bSystemSite && Home && Structure.SystemId == Home->Id) ? 1 : 0;
	}
	return Count;
}

int32 FAPSInfrastructure::BerthsAt(const AActor* Site, const APSInfrastructure::FType& Type) const
{
	// Rio 03.10: a hub's berths take more of the ordinary orbital stations at its world.
	if (Type.Category != APSInfrastructure::ECategory::Station)
	{
		return 0;
	}
	TArray<const FAPSBuiltStructure*> Here;
	GetAt(Site, Here);
	TArray<FName> Standing;
	for (const FAPSBuiltStructure* Structure : Here)
	{
		Standing.Add(Structure->Type);
	}
	return APSInfrastructure::HubBerthsFor(Type, Standing);
}

float FAPSInfrastructure::GetStock(const APSInfrastructure::EResource Resource) const
{
	const int32 Index = static_cast<int32>(Resource);
	return Index >= 0 && Index < APSInfrastructureLocal::ResourceCount ? Stocks[Index] : 0.0f;
}

float FAPSInfrastructure::GetRate(const APSInfrastructure::EResource Resource) const
{
	const int32 Index = static_cast<int32>(Resource);
	return Index >= 0 && Index < APSInfrastructureLocal::ResourceCount ? Rates[Index] : 0.0f;
}

bool FAPSInfrastructure::CanAfford(const TArray<APSInfrastructure::FAmount>& Cost) const
{
	for (const APSInfrastructure::FAmount& Amount : Cost)
	{
		if (GetStock(Amount.Resource) + 0.001f < Amount.Value) return false;
	}
	return true;
}

void FAPSInfrastructure::AddStock(const APSInfrastructure::EResource Resource, const float Value)
{
	const int32 Index = static_cast<int32>(Resource);
	if (Index < 0 || Index >= APSInfrastructureLocal::ResourceCount) return;
	Stocks[Index] = FMath::Max(0.0f, Stocks[Index] + Value);
	++Revision;
}

void FAPSInfrastructure::RecountRates()
{
	using namespace APSInfrastructureLocal;
	for (int32 Index = 0; Index < ResourceCount; ++Index) Rates[Index] = HomeYield[Index];
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (const FType* Type = Find(Structure.Type))
		{
			for (const FAmount& Amount : Type->Yield) Rates[static_cast<int32>(Amount.Resource)] += Amount.Value;
		}
	}
}

float FAPSInfrastructure::SurveySpeedBonus() const
{
	float Bonus = 0.0f;
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure.Type)) Bonus += Type->SurveySpeed;
	}
	return Bonus;
}

float FAPSInfrastructure::BuildSpeedBonus() const
{
	float Bonus = 0.0f;
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure.Type)) Bonus += Type->BuildSpeed;
	}
	return Bonus;
}

float FAPSInfrastructure::ShipSpeedBonus() const
{
	float Bonus = 0.0f;
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure.Type)) Bonus += Type->ShipSpeed;
	}
	return Bonus;
}

float FAPSInfrastructure::LocalWorkBonus(const AActor* Site) const
{
	TArray<const FAPSBuiltStructure*> Here;
	GetAt(Site, Here);
	float Bonus = 0.0f;
	for (const FAPSBuiltStructure* Structure : Here)
	{
		if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure->Type)) Bonus += Type->LocalWorkSpeed;
	}
	// Rio 03.10: hubs are the home system's logistics: work at its worlds and at its star goes faster everywhere.
	if (Cast<APlanetaryBody>(Site) || Cast<AStar>(Site))
	{
		for (const FAPSBuiltStructure& Structure : Structures)
		{
			const APSInfrastructure::FType* Type = Structure.SystemId.IsValid() ? nullptr : APSInfrastructure::Find(Structure.Type);
			Bonus += Type ? Type->SystemWorkSpeed : 0.0f;
		}
	}
	return Bonus;
}

double FAPSInfrastructure::RelayReachCm(const FGuid& SystemId) const
{
	double Reach = 0.0;
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (Structure.SystemId != SystemId) continue;
		if (const APSInfrastructure::FType* Type = APSInfrastructure::Find(Structure.Type))
		{
			Reach = FMath::Max(Reach, static_cast<double>(Type->RelayReachAu) * APSStars::AstronomicalUnitCm);
		}
	}
	return Reach;
}

void FAPSInfrastructure::SettleSurfaceActors()
{
	using namespace APSInfrastructureLocal;
	UWorld* LiveWorld = World.Get();
	const APlayerController* Player = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!Pawn) return;
	// Rio 02.10 ("the settlement's marker points under the ground; everything must stand exactly on the surface"):
	// generated settlements and surface structures were placed at a fixed height over sea level; once the terrain near
	// the pilot has collision they settle on it.
	// The candidates are refreshed every few seconds instead of walking the world twice a second.
	FSettleCache& Cache = GSettleCaches.FindOrAdd(LiveWorld);
	const double Now = FPlatformTime::Seconds();
	if (Now >= Cache.NextScan)
	{
		Cache.NextScan = Now + 5.0;
		Cache.Candidates.Reset();
		for (auto It = GSettleCaches.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid()) It.RemoveCurrent();
		}
		for (TActorIterator<AActor> It(LiveWorld); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor) || Actor->ActorHasTag(SettledTag)) continue;
			const bool bSettlement = Actor->IsA<AColony>() && Actor->ActorHasTag(TEXT("APS.Infrastructure.GroundSettlement"));
			if ((bSettlement || Actor->ActorHasTag(SurfaceTag)) && Cast<APlanetaryBody>(Actor->GetAttachParentActor()))
			{
				Cache.Candidates.Add(Actor);
			}
		}
	}
	const FVector PawnLocation = Pawn->GetActorLocation();
	for (const TWeakObjectPtr<AActor>& Weak : Cache.Candidates)
	{
		AActor* Actor = Weak.Get();
		if (!Actor || Actor->ActorHasTag(SettledTag)) continue;
		const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor->GetAttachParentActor());
		if (!Body) continue;
		const FVector Centre = Body->GetActorLocation();
		const double Radius = Body->GetWorldScapeBodyRadiusCm();
		const FVector Up = (Actor->GetActorLocation() - Centre).GetSafeNormal();
		if (Up.IsNearlyZero()) continue;
		// Close to the pilot the terrain's collision gives the ground exactly.
		if (FVector::DistSquared(PawnLocation, Actor->GetActorLocation()) <= FMath::Square(SettleRangeCm))
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(APSInfraSettle), false, Actor);
			Params.AddIgnoredActor(Pawn);
			FHitResult Hit;
			if (LiveWorld->LineTraceSingleByChannel(Hit, Centre + Up * (Radius + SurfaceBandCm), Centre + Up * (Radius - SurfaceBandCm),
				ECC_Visibility, Params) && !Cast<APawn>(Hit.GetActor()) && !Hit.GetActor()->ActorHasTag(BuiltTag))
			{
				// Never under the sea the field puts over this ground.
				const double FieldRadius = FieldSurfaceRadius(Body, Up);
				const FVector Ground = FieldRadius > FVector::Dist(Hit.ImpactPoint, Centre) + 50.0
					? Centre + Up * FieldRadius : Hit.ImpactPoint;
				Actor->SetActorLocationAndRotation(Ground, FRotationMatrix::MakeFromZ(Up).ToQuat(), false, nullptr,
					ETeleportType::TeleportPhysics);
				Actor->Tags.AddUnique(SettledTag);
				UE_LOG(LogTemp, Log, TEXT("[APS.Infra] %s settled on the ground %.0f m from sea level"), *Actor->GetName(),
					(FVector::Dist(Ground, Centre) - Radius) / 100.0);
				continue;
			}
		}
		// From afar the surface field, once the body's own surface is streamed: the marker no longer points underground.
		if (!Actor->ActorHasTag(FieldSettledTag))
		{
			const double FieldRadius = FieldSurfaceRadius(Body, Up);
			if (FieldRadius > 0.0)
			{
				Actor->SetActorLocationAndRotation(Centre + Up * FieldRadius, FRotationMatrix::MakeFromZ(Up).ToQuat(), false,
					nullptr, ETeleportType::TeleportPhysics);
				Actor->Tags.AddUnique(FieldSettledTag);
				UE_LOG(LogTemp, Log, TEXT("[APS.Infra] %s placed on the surface field %.0f m from sea level"), *Actor->GetName(),
					(FieldRadius - Radius) / 100.0);
			}
		}
	}
}

void FAPSInfrastructure::CaptureSave(FAPSInfrastructureSaveData& OutData) const
{
	OutData = FAPSInfrastructureSaveData();
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		FAPSInfrastructureSaveData::FStructure& Saved = OutData.Structures.AddDefaulted_GetRef();
		Saved.Type = Structure.Type;
		Saved.SiteKey = Structure.SiteKey;
		Saved.SystemId = Structure.SystemId;
		Saved.ActorName = Structure.ActorName;
		Saved.RelativeTransform = Structure.RelativeTransform;
		Saved.BuiltSeconds = Structure.BuiltSeconds;
	}
	if (PendingRestore.IsSet())
	{
		OutData.Structures.Append(PendingRestore->Structures);
	}
	for (int32 Index = 0; Index < APSInfrastructureLocal::ResourceCount; ++Index) OutData.Stocks.Add(Stocks[Index]);
	// Build mode: the props placed by hand (where they stand now, when alive) and the structures raised by hand.
	for (const FAPSPlacedProp& Prop : PlacedProps)
	{
		FAPSInfrastructureSaveData::FProp& Saved = OutData.Props.AddDefaulted_GetRef();
		Saved.PropId = Prop.PropId;
		Saved.MeshPath = Prop.MeshPath;
		Saved.SiteKey = Prop.SiteKey;
		Saved.RelativeTransform = Prop.RelativeTransform;
		Saved.Scale = Prop.Scale;
		Saved.bOnGround = Prop.bOnGround;
		const AActor* Actor = Prop.Actor.Get();
		const AActor* Site = Actor ? Actor->GetAttachParentActor() : nullptr;
		if (Actor && Site)
		{
			Saved.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Site->GetActorTransform());
			Saved.Scale = Actor->GetActorScale3D();
		}
	}
	for (const FAPSBuiltStructure& Structure : Structures)
	{
		if (Structure.bPlaced)
		{
			OutData.PlacedStructures.Add(Structure.ActorName);
		}
	}
	if (PendingRestore.IsSet())
	{
		OutData.Props.Append(PendingRestore->Props);
		OutData.PlacedStructures.Append(PendingRestore->PlacedStructures);
	}
}

void FAPSInfrastructure::RestoreSave(FAPSInfrastructureSaveData&& Data)
{
	for (int32 Index = 0; Index < APSInfrastructureLocal::ResourceCount && Index < Data.Stocks.Num(); ++Index)
	{
		Stocks[Index] = Data.Stocks[Index];
	}
	PendingRestore = MoveTemp(Data);
	RestoreWait = 0.0f;
	++Revision;
}

void FAPSInfrastructure::ApplyPendingRestore()
{
	if (!PendingRestore.IsSet()) return;
	UWorld* LiveWorld = World.Get();
	const FAPSInfrastructureSaveData Data = MoveTemp(PendingRestore.GetValue());
	PendingRestore.Reset();
	if (!LiveWorld) return;
	FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
	int32 Restored = 0;
	for (const FAPSInfrastructureSaveData::FStructure& Saved : Data.Structures)
	{
		const APSInfrastructure::FType* Type = APSInfrastructure::Find(Saved.Type);
		if (!Type) continue;
		AActor* Site = nullptr;
		if (Saved.SystemId.IsValid())
		{
			const FAPSStarSystemInfo* Home = Stars ? Stars->GetHome() : nullptr;
			if (Home && Home->Id == Saved.SystemId)
			{
				for (TActorIterator<AAstroGenerator> It(LiveWorld); It && !Site; ++It) Site = It->HomeStar;
			}
			if (!Site && Stars) Site = Stars->GetAnchor(Saved.SystemId);
		}
		else
		{
			for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
			{
				if (FAPSFleetCommand::KeyOf(*It) == Saved.SiteKey) { Site = *It; break; }
			}
		}
		if (!Site) continue;
		const FTransform Transform = Saved.RelativeTransform * Site->GetActorTransform();
		const FText Name = FText::Format(LOCTEXT("StructureName", "{0} {1}"), Type->Name, APSInfrastructureLocal::SiteName(Site, Stars));
		AActor* Actor = SpawnVisual(*Type, Site, Transform, Saved.ActorName, Name);
		if (!Actor) continue;
		FAPSBuiltStructure& Built = Structures.AddDefaulted_GetRef();
		Built.Type = Saved.Type;
		Built.SiteKey = Saved.SiteKey;
		Built.SystemId = Saved.SystemId;
		Built.Actor = Actor;
		Built.ActorName = Actor->GetName();
		Built.RelativeTransform = Saved.RelativeTransform;
		Built.BuiltSeconds = Saved.BuiltSeconds;
		if (Data.PlacedStructures.Contains(Saved.ActorName))
		{
			// Raised by hand: back exactly where the player put it, not settled again.
			Built.bPlaced = true;
			Actor->Tags.AddUnique(APSInfrastructureLocal::SettledTag);
			Actor->Tags.AddUnique(APSInfrastructureLocal::PlacedTag);
		}
		++Restored;
	}
	Serial = FMath::Max(Serial, Structures.Num());
	RecountRates();
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Infra] restored %d of %d structures"), Restored, Data.Structures.Num());
	RestorePlacedProps(Data.Props);
}

void FAPSInfrastructure::RestorePlacedProps(const TArray<FAPSInfrastructureSaveData::FProp>& Saved)
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || Saved.IsEmpty())
	{
		return;
	}
	TMap<FString, AActor*> Bodies;
	for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
	{
		Bodies.Add(FAPSFleetCommand::KeyOf(*It), *It);
	}
	int32 Restored = 0;
	for (const FAPSInfrastructureSaveData::FProp& Entry : Saved)
	{
		// Every record is kept: one whose body is not found here still goes into the next save.
		FAPSPlacedProp& Prop = PlacedProps.AddDefaulted_GetRef();
		Prop.PropId = Entry.PropId;
		Prop.MeshPath = Entry.MeshPath;
		Prop.SiteKey = Entry.SiteKey;
		Prop.RelativeTransform = Entry.RelativeTransform;
		Prop.Scale = Entry.Scale;
		Prop.bOnGround = Entry.bOnGround;
		AActor* Site = Bodies.FindRef(Entry.SiteKey);
		if (!Site)
		{
			continue;
		}
		FTransform Transform = Entry.RelativeTransform * Site->GetActorTransform();
		Transform.SetScale3D(Entry.Scale);
		if (AActor* Actor = APSConstruction::SpawnProp(LiveWorld, Entry.PropId, Entry.MeshPath, Transform, Site))
		{
			Prop.Actor = Actor;
			Prop.bPendingSettle = Entry.bOnGround;
			++Restored;
		}
	}
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] restored %d of %d placed props"), Restored, Saved.Num());
}

void FAPSInfrastructure::SettlePlacedProps(const float DeltaSeconds)
{
	using namespace APSInfrastructureLocal;
	if (!PlacedProps.ContainsByPredicate([](const FAPSPlacedProp& Prop) { return Prop.bPendingSettle; }))
	{
		PilotGroundedSeconds = 0.0f;
		return;
	}
	UWorld* LiveWorld = World.Get();
	const APlayerController* Player = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const ACharacter* Pilot = Player ? Cast<ACharacter>(Player->GetPawn()) : nullptr;
	const UCharacterMovementComponent* Movement = Pilot ? Pilot->GetCharacterMovement() : nullptr;
	// The terrain's collision near the pilot is trusted once the pilot has walked on it for a moment (never during the
	// surface handoff, when the character is held off the ground).
	PilotGroundedSeconds = Movement && Movement->IsMovingOnGround() ? PilotGroundedSeconds + DeltaSeconds : 0.0f;
	if (PilotGroundedSeconds < PropSettleAfterGroundSeconds)
	{
		return;
	}
	const FVector PilotLocation = Pilot->GetActorLocation();
	// A prop never settles on another placed prop (the stacked ones are not marked to settle at all).
	TArray<AActor*> PlacedActors;
	for (const FAPSPlacedProp& Prop : PlacedProps)
	{
		if (AActor* Actor = Prop.Actor.Get())
		{
			PlacedActors.Add(Actor);
		}
	}
	for (FAPSPlacedProp& Prop : PlacedProps)
	{
		AActor* Actor = Prop.Actor.Get();
		if (!Prop.bPendingSettle)
		{
			continue;
		}
		const AActor* Body = Actor ? Actor->GetAttachParentActor() : nullptr;
		if (!Body)
		{
			Prop.bPendingSettle = false;
			continue;
		}
		if (FVector::DistSquared(Actor->GetActorLocation(), PilotLocation) > FMath::Square(PropSettleRangeCm))
		{
			continue;
		}
		const FVector Up = (Actor->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal();
		const FBox Local = Actor->CalculateComponentsBoundingBoxInLocalSpace(true);
		if (Up.IsNearlyZero() || !Local.IsValid)
		{
			Prop.bPendingSettle = false;
			continue;
		}
		// The middle of its underside, and the ground under it: from a little over its top (not from a canopy far above).
		const FTransform& Transform = Actor->GetActorTransform();
		const FVector Bottom = Transform.TransformPosition(FVector(Local.GetCenter().X, Local.GetCenter().Y, Local.Min.Z));
		const double Height = (Transform.TransformPosition(FVector(Local.GetCenter().X, Local.GetCenter().Y, Local.Max.Z))
			- Bottom).Size();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSPlacedPropSettle), false);
		Params.AddIgnoredActors(PlacedActors);
		Params.AddIgnoredActor(Pilot);
		FHitResult Hit;
		if (!LiveWorld->LineTraceSingleByChannel(Hit, Bottom + Up * (Height + 300.0), Bottom - Up * PropSettleSearchCm,
			ECC_Visibility, Params))
		{
			// No collision under it yet: tried again later.
			continue;
		}
		Prop.bPendingSettle = false;
		const double Delta = FVector::DotProduct(Hit.ImpactPoint - Bottom, Up);
		if (FMath::Abs(Delta) < PropSettleToleranceCm || Cast<APawn>(Hit.GetActor()))
		{
			continue;
		}
		// Along the body's up only: the turn the player gave it stays.
		Actor->AddActorWorldOffset(Up * Delta, false, nullptr, ETeleportType::TeleportPhysics);
		Prop.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(Body->GetActorTransform());
		UE_LOG(LogTemp, Log, TEXT("[APS.Construction] %s settled %.2f m onto the ground"), *Actor->GetName(), Delta / 100.0);
	}
}

#undef LOCTEXT_NAMESPACE
