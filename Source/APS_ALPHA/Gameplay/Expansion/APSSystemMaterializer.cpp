#include "APSSystemMaterializer.h"

#include "APSStarSystems.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyNearStars.h"
#include "APS_ALPHA/Core/World/APSRealScale.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APS_ALPHA/Generation/APSBodyNames.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/MoonGenerator.h"
#include "APS_ALPHA/Generation/PlanetGenerator.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Generation/StarSystemGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightBenchmark.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "PlanetaryAtmosphere.h"

namespace APSSystemMaterializerLocal
{
	TAutoConsoleVariable<int32> CVarMaterialize(TEXT("aps.Stars.Materialize"), 1,
		TEXT("1: the cluster system the pilot comes to (within two of its rooms) becomes real: its star, and planets and moons on ")
		TEXT("orbits drawn in to fit its room; it goes back to a catalogue point once the pilot is three rooms away. 0: points only."));
	TAutoConsoleVariable<float> CVarRoomShare(TEXT("aps.Stars.MaterializeRoomShare"), 0.7f,
		TEXT("The share of a system's room (half the distance to its nearest star) its outermost orbit may take."));
	TAutoConsoleVariable<float> CVarRealSystemMaxAu(TEXT("aps.RealScale.SystemMaxAU"), 50.0f,
		TEXT("Rio 05.10 (REAL SCALE, stage 2): in a REAL SCALE world a materialized system's worlds fit within this many AU ")
		TEXT("of its star, whatever its room among the neighbours (rooms there are ~0.5 pc wide)."));
	TAutoConsoleVariable<float> CVarRealMaterializeLy(TEXT("aps.RealScale.MaterializeLy"), 0.25f,
		TEXT("Rio 05.10 (REAL SCALE, stage 2): in a REAL SCALE world a system comes no farther out than this many light years ")
		TEXT("(two of its rooms would be parsecs: every star passed in the drive stood up and fell again, a hitch each), and ")
		TEXT("goes once the pilot is half as far again away. Its glyph shows the star until then. 0: rooms as everywhere."));
	constexpr double LightYearCm = 9.4607304725808e17;

	constexpr double AstronomicalUnitCm = 1.495978707e13;
	constexpr double EarthRadiusKm = 6371.0;
	/** The planets' and moons' meshes are spheres of 50 cm: the scale that gives one Earth radius. */
	constexpr double EarthRadiusScale = 12742000.0;
	/** Within two rooms the system starts to come, three rooms away it may go (after a few seconds there). */
	constexpr double ApproachRooms = 2.0;
	constexpr double LeaveRooms = 3.0;
	constexpr float LingerSeconds = 5.0f;
	constexpr float DrainTimeoutSeconds = 20.0f;
	constexpr double RetrySeconds = 30.0;
	constexpr double MaxMoonInclinationDegrees = 12.0;
	constexpr double MaxPlanetInclinationDegrees = 6.0;
	/** A star actor's scale per solar radius (AAstroGenerator::MaterializeClusterStarSystem). */
	constexpr double StarScalePerSolarRadius = 813684224.0;
	/**
	 * A system (a galaxy star; since Rio 04.10 a cluster system too) comes only to a pilot slow enough to take this long
	 * over its approach (two rooms), so a flight through a dense field does not stand up every star it passes.
	 */
	constexpr double GalaxyApproachSeconds = 4.0;

	/**
	 * How near the pilot comes before a system stands up (Rooms = ApproachRooms) or how far before it goes (LeaveRooms):
	 * that many of its rooms, and in a REAL SCALE world no more than aps.RealScale.MaterializeLy (half as far again to go).
	 */
	double ApproachCm(const FAPSStarSystemInfo& Info, const bool bRealScale, const double Rooms)
	{
		const double RoomsCm = Info.RoomCm * Rooms;
		const double CapCm = bRealScale ? static_cast<double>(CVarRealMaterializeLy.GetValueOnGameThread()) * LightYearCm : 0.0;
		return CapCm > 0.0 ? FMath::Min(RoomsCm, CapCm * (Rooms > ApproachRooms ? 1.5 : 1.0)) : RoomsCm;
	}

	/**
	 * Rio 03.10 ("every star must be reachable"): a galaxy catalogue star stood up as AAstroGenerator::
	 * MaterializeClusterStarSystem stands a cluster record: a star system actor and its primary at the catalogue position,
	 * from the star's class, size and seed. The sky stops drawing it: its GPU point inside the system's sphere (its room
	 * holds no other drawn star), its near photosphere, and an ISM-prefix point through the generator's suppression.
	 * Rio 05.10 night: in a REAL SCALE world its GPU point is an approach point by then (APSGalaxyGpuStars::
	 * UpdateApproachPoints, outside the system's sphere), which stays and crossfades into this star as its disc grows.
	 */
	AStarSystem* MaterializeGalaxyStar(UWorld& World, AAstroGenerator& Gen, const FAPSStarSystemInfo& Info,
		TSharedPtr<FStarModel>& OutStarModel, uint32& OutSeedHash)
	{
		const AGalaxy* Galaxy = APSGalaxyGpuStars::GetIndexedGalaxy(&World);
		FGalaxyCatalogStarRecord Record;
		if (!Galaxy || !Galaxy->StarCatalog.ResolveStar(Info.GalaxyIndex, Record) || !IsValid(Gen.StarGenerator)
			|| !IsValid(Gen.StarSystemGenerator) || !Gen.BP_StarSystemClass || !Gen.BP_StarClass)
		{
			return nullptr;
		}
		// The star the sky drew: its class and its size (the photosphere grew from that radius), the rest from its seed.
		TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();
		StarModel->StellarType = APSStars::GalaxyStellarType(Record.SpectralClass, Record.RadiusScale);
		StarModel->SpectralClass = Record.SpectralClass;
		Gen.StarGenerator->SetGenerationSeed(Record.GenerationSeed);
		Gen.StarGenerator->GenerateStarModel(StarModel);
		Gen.StarGenerator->ClearGenerationSeed();
		if (Info.StarRadiusCm > 0.0)
		{
			Gen.StarGenerator->ApplyRadiusOverrideSolar(*StarModel, Info.StarRadiusCm / APSCanonicalStellarProjection::SolarRadiusCm);
		}
		StarModel->SpectralSubclass = Record.SpectralSubclass;
		StarModel->FullSpectralClass = FName(*Info.Spectral);
		StarModel->Location = Record.GalaxyLocalLocation;
		FStarSystemModel SystemModel;
		Gen.StarSystemGenerator->GeneratePotentialStarSystemModel(SystemModel, *StarModel, Record.GenerationSeed);
		SystemModel.StableId = Record.StableId;
		SystemModel.PotentialPlanetCount = Info.PotentialPlanets;
		SystemModel.bHasPlanetarySystem = Info.PotentialPlanets > 0;

		const FTransform WorldTransform(FQuat::Identity, Info.Location);
		AStarSystem* StarSystem = World.SpawnActor<AStarSystem>(Gen.BP_StarSystemClass, WorldTransform);
		if (!StarSystem)
		{
			return nullptr;
		}
		Gen.StarSystemGenerator->ApplyModel(StarSystem, MakeShared<FStarSystemModel>(SystemModel));
		AStar* Star = World.SpawnActor<AStar>(Gen.BP_StarClass, WorldTransform);
		if (!Star)
		{
			StarSystem->Destroy();
			return nullptr;
		}
		Gen.StarGenerator->ApplyModel(Star, StarModel);
		Star->SetActorLocation(Info.Location);
		Star->SetActorScale3D(FVector(StarModel->Radius * StarScalePerSolarRadius));
		Star->StarRadiusKM = FMath::RoundToInt(StarModel->RadiusKM);
		Star->FullSpectralName = Star->GenerateFullSpectralName();
		Gen.StarGenerator->ApplySpectralMaterial(Star, StarModel);
		StarSystem->MainStar = Star;
		StarSystem->AddNewStar(Star);
		Star->AttachToActor(StarSystem, FAttachmentTransformRules::KeepWorldTransform);
		StarSystem->StarSystemRadius = FMath::Max(Info.RoomCm, Info.StarRadiusCm * 2.0);
		APSGalaxyNearStars::SetMaterialized(&World, Info.GalaxyIndex, true);
		Gen.SetGalaxyProxyMaterialized(Info.GalaxyIndex, true);
		OutStarModel = StarModel;
		OutSeedHash = HashCombineFast(GetTypeHash(Record.StableId), static_cast<uint32>(Record.GenerationSeed));
		return StarSystem;
	}

	int32 SurfaceSeed(const int32 SystemSeed, const FString& BodyAddress)
	{
		return 10 + static_cast<int32>(HashCombineFast(static_cast<uint32>(SystemSeed), GetTypeHash(BodyAddress)) % 999983u);
	}

	FRandomStream Stream(const int32 SystemSeed, const FString& BodyAddress, const TCHAR* Channel)
	{
		return FRandomStream(static_cast<int32>(HashCombineFast(HashCombineFast(static_cast<uint32>(SystemSeed),
			GetTypeHash(BodyAddress)), GetTypeHash(FString(Channel))) & 0x7fffffffu));
	}

	/** The materialized system's bodies: its planets and their moons. */
	void CollectBodies(const TArray<TWeakObjectPtr<APlanet>>& Planets, TArray<APlanetaryBody*>& OutBodies)
	{
		for (const TWeakObjectPtr<APlanet>& Weak : Planets)
		{
			if (APlanet* Planet = Weak.Get())
			{
				OutBodies.Add(Planet);
				for (AMoon* Moon : Planet->Moons)
				{
					if (IsValid(Moon)) OutBodies.Add(Moon);
				}
			}
		}
	}

	FAPSSystemMaterializer* Find(UWorld* World, FAPSStarSystems*& OutSystems)
	{
		OutSystems = APSStarSystemsFind(World);
		return OutSystems ? OutSystems->GetMaterializer() : nullptr;
	}

	/** aps.Stars.Visit [name] [planet]: the named system, or the nearest outside the home ("galaxy": of the galaxy's). */
	int32 FindVisitTarget(const FAPSStarSystems& Systems, const UWorld* World, const FString& Name)
	{
		const bool bGalaxy = Name.Equals(TEXT("galaxy"), ESearchCase::IgnoreCase);
		if (!Name.IsEmpty() && !bGalaxy && !Name.Equals(TEXT("nearest"), ESearchCase::IgnoreCase))
		{
			TArray<int32> Found;
			Systems.Search(Name, 1, Found);
			return Found.IsEmpty() ? INDEX_NONE : Found[0];
		}
		const FAPSStarSystemInfo* Home = Systems.GetHome();
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const FVector From = Controller && Controller->GetPawn() ? Controller->GetPawn()->GetActorLocation()
			: Home ? Home->Location : FVector::ZeroVector;
		if (bGalaxy)
		{
			int32 Best = INDEX_NONE;
			double BestDistance = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Systems.Num(); ++Index)
			{
				const FAPSStarSystemInfo* Info = Systems.Get(Index);
				if (Info && Info->GalaxyIndex != INDEX_NONE && FVector::DistSquared(From, Info->Location) < BestDistance)
				{
					BestDistance = FVector::DistSquared(From, Info->Location);
					Best = Index;
				}
			}
			return Best;
		}
		TArray<int32> Nearest;
		Systems.FindNearest(From, 4, Nearest);
		for (const int32 Index : Nearest)
		{
			const FAPSStarSystemInfo* Info = Systems.Get(Index);
			if (Info && !Info->bHome && !Info->bInsideHome) return Index;
		}
		return INDEX_NONE;
	}

	FAutoConsoleCommandWithWorldAndArgs VisitCommand(TEXT("aps.Stars.Visit"),
		TEXT("Test: moves the piloted ship to a cluster system's edge, facing its star: aps.Stars.Visit [name|nearest] [planet]. ")
		TEXT("With 'planet' (once the system stands), near its first planet."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			FAPSStarSystems* Systems = nullptr;
			FAPSSystemMaterializer* Materializer = Find(World, Systems);
			if (!Materializer)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] aps.Stars.Visit: the star catalogue is not read yet"));
				return;
			}
			const bool bPlanet = Args.Num() > 0 && Args.Last().Equals(TEXT("planet"), ESearchCase::IgnoreCase);
			const FString Name = Args.Num() > (bPlanet ? 1 : 0) ? Args[0] : FString();
			const int32 Index = bPlanet && Name.IsEmpty() && Materializer->GetActiveIndex() != INDEX_NONE
				? Materializer->GetActiveIndex() : FindVisitTarget(*Systems, World, Name);
			if (Index == INDEX_NONE || !Materializer->VisitForTest(*Systems, Index, bPlanet))
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] aps.Stars.Visit %s%s: nothing to visit"), *Name,
					bPlanet ? TEXT(" planet") : TEXT(""));
			}
		}));

	FAutoConsoleCommandWithWorld StateCommand(TEXT("aps.Stars.Materialized"),
		TEXT("Logs the cluster system that stands materialized, its star and its planets."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			FAPSStarSystems* Systems = nullptr;
			if (const FAPSSystemMaterializer* Materializer = Find(World, Systems))
			{
				Materializer->LogState(*Systems);
			}
		}));
}

FAPSSystemMaterializer::FAPSSystemMaterializer(UWorld* InWorld)
	: World(InWorld)
{
}

void FAPSSystemMaterializer::GetPlanets(TArray<APlanet*>& OutPlanets) const
{
	OutPlanets.Reset();
	for (const TWeakObjectPtr<APlanet>& Weak : Planets)
	{
		if (APlanet* Planet = Weak.Get()) OutPlanets.Add(Planet);
	}
}

void FAPSSystemMaterializer::Update(FAPSStarSystems& Systems, const float DeltaSeconds)
{
	using namespace APSSystemMaterializerLocal;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	if (Stage == EStage::Draining)
	{
		if (IsDrained(DeltaSeconds))
		{
			Finish();
		}
		return;
	}
	if (Stage != EStage::Idle && !System.IsValid())
	{
		// Something else took the system down (a regeneration): start clean.
		UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] materialized system %s vanished; starting over"), *ActiveName);
		Finish();
		return;
	}
	if (Stage == EStage::Spawning)
	{
		// One planet (with its moons) per update, so a system of eight does not stall a frame.
		if (!SpawnNextPlanet())
		{
			Stage = EStage::Ready;
			TArray<APlanetaryBody*> Bodies;
			CollectBodies(Planets, Bodies);
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars] %s stands: %d planet(s), %d bod(ies) in all"), *ActiveName, Planets.Num(),
				Bodies.Num());
		}
		return;
	}

	const bool bEnabled = CVarMaterialize.GetValueOnGameThread() != 0;
	const APlayerController* Controller = LiveWorld->GetFirstPlayerController();
	const APawn* Pilot = Controller ? Controller->GetPawn() : nullptr;
	const FVector PilotLocation = Pilot ? Pilot->GetActorLocation() : FVector::ZeroVector;
	const ASpaceship* PilotShip = Cast<ASpaceship>(Pilot);
	double PilotSpeedCm = PilotShip ? PilotShip->GetKinematicVelocity().Size() : Pilot ? Pilot->GetVelocity().Size() : 0.0;
	// The system the pilot is coming to: the nearest outside the home, within two of its rooms.
	// Rio 05.10 (real scale, stage 2): in a REAL SCALE world no farther out than aps.RealScale.MaterializeLy (ApproachCm).
	const bool bRealScale = APSRealScale::IsActive(LiveWorld);
	if (bRealScale && !PilotShip && Pilot)
	{
		// Rio 05.10 (real scale, stage 2): someone walking aboard moves with the ship (Rio got up at 100 ly/s and every star
		// passed within two rooms stood up behind him): the carrier's speed counts.
		for (const AActor* Carrier = Pilot->GetAttachParentActor(); Carrier; Carrier = Carrier->GetAttachParentActor())
		{
			if (const ASpaceship* CarrierShip = Cast<ASpaceship>(Carrier))
			{
				PilotSpeedCm = FMath::Max(PilotSpeedCm, CarrierShip->GetKinematicVelocity().Size());
				break;
			}
		}
	}
	int32 Wanted = INDEX_NONE;
	if (bEnabled && Pilot)
	{
		TArray<int32> Nearest;
		Systems.FindNearest(PilotLocation, 3, Nearest);
		for (const int32 Index : Nearest)
		{
			const FAPSStarSystemInfo* Info = Systems.Get(Index);
			if (!Info || Info->bInsideHome) continue;
			// Rio 04.10 (FPS dips in the star drive): the cluster's systems were built for a pass at any speed too, 9-16 a
			// minute, most torn down within 3 s for the next one ahead, each build and teardown an ~80 ms hitch. Every
			// system now waits, as the galaxy's did, until the ship would take a few seconds to cross its approach.
			const double Approach = ApproachCm(*Info, bRealScale, ApproachRooms);
			if (!Info->bHome && FVector::Dist(PilotLocation, Info->Location) <= Approach
				&& PilotSpeedCm * GalaxyApproachSeconds <= Approach)
			{
				Wanted = Index;
			}
			break;
		}
	}

	if (Stage == EStage::Ready)
	{
		const FAPSStarSystemInfo* Active = Systems.Get(ActiveIndex);
		const bool bNear = Active && Pilot
			&& FVector::Dist(PilotLocation, Active->Location) <= ApproachCm(*Active, bRealScale, LeaveRooms);
		if (bEnabled && bNear && (Wanted == INDEX_NONE || Wanted == ActiveIndex))
		{
			AwaySeconds = 0.0f;
			return;
		}
		AwaySeconds += DeltaSeconds;
		if (bEnabled && Wanted == INDEX_NONE && AwaySeconds < LingerSeconds)
		{
			return;
		}
		BeginDrain(!bEnabled ? TEXT("switched off") : Wanted != INDEX_NONE ? TEXT("another system ahead") : TEXT("the pilot left"));
		return;
	}

	if (Wanted != INDEX_NONE)
	{
		if (Wanted == FailedIndex && FPlatformTime::Seconds() < FailedUntilSeconds)
		{
			return;
		}
		if (!Begin(Systems, Wanted))
		{
			FailedIndex = Wanted;
			FailedUntilSeconds = FPlatformTime::Seconds() + RetrySeconds;
		}
	}
}

bool FAPSSystemMaterializer::Begin(FAPSStarSystems& Systems, const int32 Index)
{
	using namespace APSSystemMaterializerLocal;
	UWorld* LiveWorld = World.Get();
	// Rio 06.10 (still ship): far from every system a fast REAL SCALE ship owes its travel and only the sky moves. A system
	// stood up meanwhile spawns at its sky place and rides with the sky from then on (AddSkyMember below), so the ship
	// brakes into it while the far world stays still. Where systems do not ride with the sky (aps.RealScale.DeferBodyClearRadii
	// 0, a legacy world) the debt is paid first, so the catalogue's places are world places again.
	UAPSWorldOriginSubsystem* Origin = LiveWorld ? LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
	static IConsoleVariable* const BodyClearRadii = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.RealScale.DeferBodyClearRadii"));
	const bool bRidesWithSky = Origin && APSRealScale::IsActive(LiveWorld) && BodyClearRadii && BodyClearRadii->GetFloat() > 0.0f;
	if (Origin && !bRidesWithSky)
	{
		Origin->SettleDeferredTravel(TEXT("a star system materializes"));
	}
	AAstroGenerator* Gen = Systems.GetGenerator();
	AStarCluster* Cluster = Systems.GetCluster();
	const FAPSStarSystemInfo* Info = Systems.Get(Index);
	if (!LiveWorld || !IsValid(Gen) || !IsValid(Cluster) || !Info || !Gen->PlanetarySystemGenerator || !Gen->PlanetGenerator
		|| !Gen->MoonGenerator || !Gen->BP_PlanetarySystemClass || !Gen->BP_PlanetOrbitClass || !Gen->BP_PlanetClass
		|| !Gen->BP_MoonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] cannot materialize %s: the generator or its classes are missing"),
			Info ? *Info->Name : TEXT("?"));
		return false;
	}
	const double StartSeconds = FPlatformTime::Seconds();
	int32 Instance = INDEX_NONE;
	AStarSystem* NewSystem = nullptr;
	TSharedPtr<FStarModel> PrimaryModel;
	int32 PotentialPlanets = 0;
	uint32 SeedHash = 0;
	FString NewAddress;
	if (Info->GalaxyIndex != INDEX_NONE)
	{
		NewSystem = MaterializeGalaxyStar(*LiveWorld, *Gen, *Info, PrimaryModel, SeedHash);
		if (!NewSystem || !IsValid(NewSystem->MainStar))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] cannot materialize galaxy star %s (catalogue %lld)"), *Info->Name,
				Info->GalaxyIndex);
			if (NewSystem)
			{
				AAstroGenerator::DestroyActorTree(NewSystem);
				Gen->SetGalaxyProxyMaterialized(Info->GalaxyIndex, false);
				APSGalaxyNearStars::SetMaterialized(LiveWorld, Info->GalaxyIndex, false);
				APSGalaxyGpuStars::RescanExclusions(LiveWorld);
			}
			return false;
		}
		PotentialPlanets = Info->PotentialPlanets;
		NewAddress = FString::Printf(TEXT("G%lld"), Info->GalaxyIndex);
	}
	else
	{
		Instance = Cluster->PotentialStarSystems.IsValidIndex(Info->Record)
			? Cluster->PotentialStarSystems[Info->Record].InstanceIndex : INDEX_NONE;
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Instance);
		if (!Record || Record->bMaterialized)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] cannot materialize %s: record %d %s"), *Info->Name, Info->Record,
				Record ? TEXT("already stands") : TEXT("not found"));
			return false;
		}
		NewSystem = Gen->MaterializeClusterStarSystem(Instance);
		if (!NewSystem || !IsValid(NewSystem->MainStar))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] the generator did not materialize %s (instance %d)"), *Info->Name, Instance);
			if (NewSystem) Gen->DematerializeClusterStarSystem(Instance);
			return false;
		}
		PrimaryModel = MakeShared<FStarModel>(Record->PrimaryStarModel);
		PotentialPlanets = Record->SystemModel.PotentialPlanetCount;
		SeedHash = HashCombineFast(GetTypeHash(Record->StableId), static_cast<uint32>(Record->SystemModel.GenerationSeed));
		NewAddress = FString::Printf(TEXT("C%d"), Info->Record);
	}
	// Rio 05.10 night: the GPU layer takes the new system's sphere in this frame, not at its next half-second scan
	// (REAL SCALE with aps.Stars.ApproachPoint; legacy worlds keep the scan).
	// The system rides with the sky before the GPU layer takes its sphere (the review: the order mattered for its place).
	if (bRidesWithSky)
	{
		Origin->AddSkyMember(NewSystem);
	}
	APSGalaxyGpuStars::RescanExclusions(LiveWorld);
	AStar* NewStar = NewSystem->MainStar;
	ActiveIndex = Index;
	ActiveInstance = Instance;
	ActiveGalaxyIndex = Info->GalaxyIndex;
	ActiveName = Info->Name;
	Generator = Gen;
	System = NewSystem;
	Star = NewStar;
	Planets.Reset();
	Plan.Reset();
	NextPlanet = 0;
	AwaySeconds = 0.0f;
	// The star carries the system's catalogue name (the HUD, the maps and the menu say the same) and, like the home star,
	// does not shadow the planets it lights.
	NewStar->AstroName = FName(*Info->Name);
	if (IsValid(NewStar->StarMesh))
	{
		NewStar->StarMesh->SetCastShadow(false);
	}

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlanetarySystem* NewPlanetarySystem = LiveWorld->SpawnActor<APlanetarySystem>(Gen->BP_PlanetarySystemClass,
		NewStar->GetActorLocation(), FRotator::ZeroRotator, Parameters);
	PlanetarySystem = NewPlanetarySystem;
	SystemSeed = static_cast<int32>(SeedHash & 0x7fffffffu) | 1;
	WorldSeed = Systems.GetClusterSeed();
	Address = NewAddress;
	Model = MakeShared<FPlanetarySystemModel>();
	if (NewPlanetarySystem && PotentialPlanets > 0)
	{
		// The catalogue's number of worlds, laid out densely (Rio's C8), from the record's own seed: the same system comes
		// back with the same worlds. The planet and moon generators draw from the global stream, seeded here too.
		FMath::RandInit(SystemSeed);
		TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>(*PrimaryModel);
		Model->AmountOfPlanets = PotentialPlanets;
		Model->PlanetarySystemType = PotentialPlanets == 1 ? EPlanetarySystemType::SinglePlanetSystem
			: EPlanetarySystemType::MultiPlanetSystem;
		Model->OrbitDistributionType = EOrbitDistributionType::Dense;
		UPlanetarySystemGenerator* SystemGenerator = Gen->PlanetarySystemGenerator;
		SystemGenerator->SetGenerationSeed(SystemSeed);
		// Rio 05.10 (real scale experiment): a REAL SCALE world keeps real AU orbits (no REAL SCALE gameplay before stage 2).
		SystemGenerator->GenerateCustomPlanetarySystemModel(Model, StarModel, Gen->PlanetGenerator, Gen->MoonGenerator, nullptr,
			!Gen->UsesRealScale());
		SystemGenerator->ClearGenerationSeed();
		UPlanetarySystemGenerator::EnforcePlanetSurfaceClearance(*Model);
		SystemGenerator->ApplyModel(NewPlanetarySystem, Model);
	}
	if (NewPlanetarySystem)
	{
		NewStar->SetPlanetarySystem(NewPlanetarySystem);
		NewPlanetarySystem->SetStar(NewStar);
		NewPlanetarySystem->SetStarFullSpectralName(NewStar->FullSpectralName);
		NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
	}

	// Orbits drawn in to the room: one scale for all (their ratios stay), clear of the star and of each other; a planet
	// that does not fit before the room's edge, and every one after it, is left out ("planets only where there is room").
	const double StarRadiusCm = FMath::Max(static_cast<double>(NewStar->StarRadiusKM), 1.0) * 100000.0;
	// Rio 05.10 (real scale, stage 2): a REAL SCALE room is ~0.5 pc wide, and its 2% gap between worlds put six of them 2 500
	// AU apart (12 600 AU out in Rio's 05.10 run): there the worlds keep a star system's own size (aps.RealScale.SystemMaxAU).
	const double RoomUsableCm = Info->RoomCm * FMath::Clamp(CVarRoomShare.GetValueOnGameThread(), 0.2f, 0.95f);
	const double UsableCm = Gen->UsesRealScale()
		? FMath::Min(RoomUsableCm, FMath::Max(static_cast<double>(CVarRealSystemMaxAu.GetValueOnGameThread()), 1.0)
			* AstronomicalUnitCm)
		: RoomUsableCm;
	TArray<int32> Order;
	double OutermostPhysicalCm = 0.0;
	for (int32 PlanetIndex = 0; PlanetIndex < Model->PlanetsList.Num(); ++PlanetIndex)
	{
		const TSharedPtr<FPlanetData>& Data = Model->PlanetsList[PlanetIndex];
		if (Data.IsValid() && Data->PlanetModel.IsValid())
		{
			Order.Add(PlanetIndex);
			OutermostPhysicalCm = FMath::Max(OutermostPhysicalCm, Data->PlanetModel->OrbitDistance * AstronomicalUnitCm);
		}
	}
	Order.Sort([this](const int32 A, const int32 B)
	{
		return Model->PlanetsList[A]->PlanetModel->OrbitDistance < Model->PlanetsList[B]->PlanetModel->OrbitDistance;
	});
	const double Scale = OutermostPhysicalCm > UsableCm && OutermostPhysicalCm > 0.0 ? UsableCm / OutermostPhysicalCm : 1.0;
	const double MinimumGapCm = FMath::Max(UsableCm * 0.02, StarRadiusCm * 0.05);
	double PreviousOrbitCm = 0.0;
	double PreviousEnvelopeCm = 0.0;
	for (const int32 PlanetIndex : Order)
	{
		const FPlanetModel& PlanetModel = *Model->PlanetsList[PlanetIndex]->PlanetModel;
		const double PlanetRadiusKm = FMath::Max(static_cast<double>(PlanetModel.Radius) * EarthRadiusKm, 1.0);
		double MoonReachKm = 0.0;
		for (const TSharedPtr<FMoonData>& Moon : PlanetModel.MoonsList)
		{
			if (Moon.IsValid() && Moon->MoonModel.IsValid())
			{
				MoonReachKm = FMath::Max(MoonReachKm, PlanetRadiusKm * (1.0 + Moon->OrbitRadius)
					+ Moon->MoonModel->Radius * EarthRadiusKm);
			}
		}
		const double EnvelopeCm = FMath::Max(PlanetRadiusKm * 2.5, MoonReachKm * 1.2) * 100000.0;
		double OrbitCm = FMath::Max(PlanetModel.OrbitDistance * AstronomicalUnitCm * Scale,
			FMath::Max(StarRadiusCm * 1.35 + EnvelopeCm, StarRadiusCm + EnvelopeCm * 2.0));
		if (PreviousOrbitCm > 0.0)
		{
			OrbitCm = FMath::Max(OrbitCm, PreviousOrbitCm + PreviousEnvelopeCm + EnvelopeCm + MinimumGapCm);
		}
		if (OrbitCm + EnvelopeCm > UsableCm)
		{
			break;
		}
		FPlannedPlanet& Planned = Plan.AddDefaulted_GetRef();
		Planned.ModelIndex = PlanetIndex;
		Planned.OrbitCm = OrbitCm;
		FRandomStream Orbit = Stream(SystemSeed, FString::Printf(TEXT("%s/P%d"), *Address, Plan.Num() - 1), TEXT("orbit"));
		Planned.Orbit = UPlanetarySystemGenerator::SamplePlanetOrbitRotation(Orbit, MaxPlanetInclinationDegrees);
		PreviousOrbitCm = OrbitCm;
		PreviousEnvelopeCm = EnvelopeCm;
	}
	Stage = EStage::Spawning;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Stars] materializing %s: %s star of %d km, %.3f AU from home (%.0f km off its catalogue point), room %.3f AU; ")
		TEXT("%d of %d worlds fit (orbits x1/%.1f, outermost %.3f AU) | %.1f ms"),
		*ActiveName, *NewStar->FullSpectralName.ToString(), NewStar->StarRadiusKM,
		Info->HomeDistanceCm / AstronomicalUnitCm, FVector::Dist(NewStar->GetActorLocation(), Info->Location) / 100000.0,
		Info->RoomCm / AstronomicalUnitCm, Plan.Num(), Model->PlanetsList.Num(), Scale > 0.0 ? 1.0 / Scale : 0.0,
		PreviousOrbitCm / AstronomicalUnitCm, (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
	return true;
}

bool FAPSSystemMaterializer::SpawnNextPlanet()
{
	using namespace APSSystemMaterializerLocal;
	UWorld* LiveWorld = World.Get();
	AAstroGenerator* Gen = Generator.Get();
	AStar* LiveStar = Star.Get();
	APlanetarySystem* LivePlanetarySystem = PlanetarySystem.Get();
	if (!LiveWorld || !Gen || !LiveStar || !LivePlanetarySystem || !Model.IsValid() || !Plan.IsValidIndex(NextPlanet))
	{
		return false;
	}
	const double StartSeconds = FPlatformTime::Seconds();
	const int32 PlanetSlot = NextPlanet++;
	const FPlannedPlanet& Planned = Plan[PlanetSlot];
	const TSharedPtr<FPlanetData>& Data = Model->PlanetsList[Planned.ModelIndex];
	const TSharedPtr<FPlanetModel> PlanetModel = Data->PlanetModel;
	const FString PlanetAddress = FString::Printf(TEXT("%s/P%d"), *Address, PlanetSlot);

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlanetOrbit* Orbit = LiveWorld->SpawnActor<APlanetOrbit>(Gen->BP_PlanetOrbitClass, LivePlanetarySystem->GetActorLocation(),
		Planned.Orbit, Parameters);
	APlanet* Planet = Orbit ? LiveWorld->SpawnActor<APlanet>(Gen->BP_PlanetClass, LivePlanetarySystem->GetActorLocation(),
		FRotator::ZeroRotator, Parameters) : nullptr;
	if (!Planet)
	{
		if (Orbit) Orbit->Destroy();
		UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] %s: planet %d did not spawn"), *ActiveName, PlanetSlot);
		return Plan.IsValidIndex(NextPlanet);
	}
	Orbit->AttachToActor(LivePlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);
	// As the home system's planets (AAstroGenerator::GenerateStarSystemByModel), on the drawn-in orbit.
	Planet->bStreamWorldScapeSurface = true;
	Planet->bGenerateByDefault = false;
	Gen->PlanetGenerator->ApplyModel(Planet, PlanetModel);
	Planet->AstroName = FName(*APSBodyNames::Generate(WorldSeed, PlanetAddress, APSBodyNames::EKind::Planet));
	Planet->WorldScapeSeed = SurfaceSeed(SystemSeed, PlanetAddress);
	LiveStar->AddPlanet(Planet);
	Planet->SetParentStar(LiveStar);
	Planet->SetActorScale3D(FVector(PlanetModel->Radius * EarthRadiusScale));
	Planet->PlanetRadiusKM = FMath::RoundToInt(PlanetModel->Radius * EarthRadiusKm);
	if (Planet->RadiusKM <= 0.0)
	{
		Planet->RadiusKM = PlanetModel->Radius * EarthRadiusKm;
	}
	Planet->SetActorLocation(Orbit->GetActorLocation() + Orbit->GetActorQuat().RotateVector(FVector(Planned.OrbitCm, 0.0, 0.0)));
	Planet->AttachToActor(Orbit, FAttachmentTransformRules::KeepWorldTransform);
	const double OrbitAu = Planned.OrbitCm / AstronomicalUnitCm;
	Planet->SetOrbitDistance(OrbitAu);
	Planet->PlanetData.OrbitRadius = OrbitAu;
	Planet->PlanetData.PlanetModelData = *PlanetModel;
	Planet->PlanetData.PlanetHabitability = PlanetModel->PlanetHabitability;
	Data->OrbitRadius = OrbitAu;
	LivePlanetarySystem->PlanetsActorsList.Add(Planet);
	LivePlanetarySystem->PlanetOrbitsList.Add(Orbit);
	Orbit->Planet = Planet;

	double LastMoonDiameter = 0.0;
	FVector LastMoonLocation = FVector::ZeroVector;
	for (const TSharedPtr<FMoonData>& MoonData : PlanetModel->MoonsList)
	{
		if (!MoonData.IsValid() || !MoonData->MoonModel.IsValid())
		{
			continue;
		}
		APlanetOrbit* MoonOrbit = LiveWorld->SpawnActor<APlanetOrbit>(Gen->BP_PlanetOrbitClass, Planet->GetActorLocation(),
			FRotator::ZeroRotator, Parameters);
		AMoon* Moon = MoonOrbit ? LiveWorld->SpawnActor<AMoon>(Gen->BP_MoonClass, Planet->GetActorLocation(), FRotator::ZeroRotator,
			Parameters) : nullptr;
		if (!Moon)
		{
			if (MoonOrbit) MoonOrbit->Destroy();
			continue;
		}
		MoonOrbit->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);
		const FString MoonAddress = FString::Printf(TEXT("%s/M%d"), *PlanetAddress, Planet->Moons.Num());
		Moon->bStreamWorldScapeSurface = true;
		Moon->bGenerateByDefault = false;
		Moon->AstroName = FName(*APSBodyNames::Generate(WorldSeed, MoonAddress, APSBodyNames::EKind::Moon));
		Planet->AddMoon(Moon);
		Moon->SetParentPlanet(Planet);
		Planet->MoonOrbitsList.Add(MoonOrbit);
		Gen->MoonGenerator->ApplyModel(Moon, MoonData->MoonModel);
		Gen->MoonGenerator->ConnectMoonWithPlanet(Moon, Planet);
		Moon->WorldScapeSeed = SurfaceSeed(SystemSeed, MoonAddress);
		const double MoonRadius = MoonData->MoonModel->Radius;
		Moon->RadiusKM = MoonRadius * EarthRadiusKm;
		Moon->PlanetRadiusKM = FMath::Max(1, FMath::RoundToInt(Moon->RadiusKM));
		Moon->SetActorScale3D(FVector(MoonRadius * EarthRadiusScale));
		FVector BoundsOrigin;
		FVector BoundsExtent;
		Moon->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		Moon->AffectionRadiusKM = BoundsExtent.GetMax() / 100000.0;
		Moon->AddActorLocalOffset(FVector(0.0, (PlanetModel->RadiusKM + MoonData->OrbitRadius * PlanetModel->RadiusKM) * 100000.0, 0.0));
		Moon->AttachToActor(MoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
		// The moon's orbit plane, turned with the moon on it.
		FRandomStream MoonOrbitStream = Stream(SystemSeed, MoonAddress, TEXT("orbit"));
		MoonOrbit->SetActorRotation(UPlanetarySystemGenerator::SamplePlanetOrbitRotation(MoonOrbitStream, MaxMoonInclinationDegrees));
		LastMoonDiameter = MoonRadius * 2.0;
		LastMoonLocation = Moon->GetActorLocation();
		if (APlanetarySurfaceGenerator* MoonEnvironment = Moon->EnsurePlanetaryEnvironmentGenerator())
		{
			MoonEnvironment->InitAtmoScape(LiveWorld, Moon->RadiusKM, Moon);
		}
	}

	const double PlanetScale = Planet->GetActorScale3D().X;
	if (IsValid(Planet->PlanetaryZone) && PlanetScale > 0.0)
	{
		double ZoneRadius = 100.0;
		if (LastMoonDiameter > 0.0)
		{
			const FVector LastMoonEdge = LastMoonLocation + FVector(0.0, LastMoonDiameter * EarthRadiusKm, 0.0);
			ZoneRadius = FVector::Dist(Planet->GetActorLocation(), LastMoonEdge) / PlanetScale * 1.5;
		}
		Planet->PlanetaryZone->SetSphereRadius(ZoneRadius);
		Planet->AffectionRadiusKM = ZoneRadius * PlanetScale / 100000.0;
	}
	if (IsValid(Planet->GravityCollisionZone))
	{
		Planet->OrbitHeight = Planet->GravityCollisionZone->GetScaledSphereRadius() / 100000.0 - Planet->RadiusKM;
	}
	if (APlanetarySurfaceGenerator* Environment = Planet->EnsurePlanetaryEnvironmentGenerator())
	{
		Environment->InitEnviroment(Planet, LiveWorld);
		if (AAtmoScape* Atmosphere = Environment->PlanetAtmosphere)
		{
			Atmosphere->LightSource = LiveStar;
			Atmosphere->UpdateScale();
		}
	}
	Planets.Add(Planet);
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] %s: world %s (%s, %d km, %d moon(s)) at %.3f AU | %.1f ms"), *ActiveName,
		*Planet->AstroName.ToString(), *UEnum::GetValueAsString(Planet->PlanetType), Planet->PlanetRadiusKM, Planet->Moons.Num(),
		OrbitAu, (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
	return Plan.IsValidIndex(NextPlanet);
}

void FAPSSystemMaterializer::BeginDrain(const TCHAR* Reason)
{
	using namespace APSSystemMaterializerLocal;
	Stage = EStage::Draining;
	DrainSeconds = 0.0f;
	// Surfaces first: their workers drain through the streaming path, then the actors go.
	TArray<APlanetaryBody*> Bodies;
	CollectBodies(Planets, Bodies);
	for (APlanetaryBody* Body : Bodies)
	{
		Body->bStreamWorldScapeSurface = false;
		if (Body->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Unloaded || Body->IsWorldScapeStreamingActive())
		{
			Body->SetWorldScapeStreamingState(EWorldScapeSurfaceState::Unloaded);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] releasing %s (%s): %d bodies"), *ActiveName, Reason, Bodies.Num());
}

bool FAPSSystemMaterializer::IsDrained(const float DeltaSeconds)
{
	using namespace APSSystemMaterializerLocal;
	DrainSeconds += DeltaSeconds;
	TArray<APlanetaryBody*> Bodies;
	CollectBodies(Planets, Bodies);
	for (const APlanetaryBody* Body : Bodies)
	{
		const APlanetarySurfaceGenerator* Environment = Body->PlanetaryEnvironmentGenerator;
		if (IsValid(Environment) && IsValid(Environment->WorldScapeRootInstance))
		{
			if (DrainSeconds < DrainTimeoutSeconds)
			{
				return false;
			}
			UE_LOG(LogTemp, Warning, TEXT("[APS.Stars] %s: %s still holds a surface after %.0f s; releasing anyway"), *ActiveName,
				*Body->GetName(), DrainSeconds);
			break;
		}
	}
	return true;
}

void FAPSSystemMaterializer::Finish()
{
	// Rio 06.10 (still ship): the system no longer rides with the sky.
	if (UWorld* LiveWorld = World.Get(); LiveWorld && System.IsValid())
	{
		if (UAPSWorldOriginSubsystem* Origin = LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			Origin->RemoveSkyMember(System.Get());
		}
	}
	if (ActiveGalaxyIndex != INDEX_NONE)
	{
		// A galaxy star: its actors go and the sky draws it again.
		if (AStarSystem* LiveSystem = System.Get())
		{
			AAstroGenerator::DestroyActorTree(LiveSystem);
		}
		if (AAstroGenerator* Gen = Generator.Get())
		{
			Gen->SetGalaxyProxyMaterialized(ActiveGalaxyIndex, false);
		}
		APSGalaxyNearStars::SetMaterialized(World.Get(), ActiveGalaxyIndex, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] %s is a catalogue point again"), *ActiveName);
	}
	else if (AAstroGenerator* Gen = Generator.Get(); Gen && ActiveInstance != INDEX_NONE)
	{
		Gen->DematerializeClusterStarSystem(ActiveInstance);
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] %s is a catalogue point again"), *ActiveName);
	}
	// Rio 05.10 night: and the GPU layer drops its sphere in this frame (REAL SCALE with aps.Stars.ApproachPoint).
	if (UWorld* LiveWorld = World.Get())
	{
		APSGalaxyGpuStars::RescanExclusions(LiveWorld);
	}
	Stage = EStage::Idle;
	ActiveIndex = INDEX_NONE;
	ActiveInstance = INDEX_NONE;
	ActiveGalaxyIndex = INDEX_NONE;
	ActiveName.Reset();
	System.Reset();
	Star.Reset();
	PlanetarySystem.Reset();
	Model.Reset();
	Plan.Reset();
	Planets.Reset();
	NextPlanet = 0;
	AwaySeconds = 0.0f;
	DrainSeconds = 0.0f;
}

bool FAPSSystemMaterializer::VisitForTest(const FAPSStarSystems& Systems, const int32 Index, const bool bNearPlanet)
{
	UWorld* LiveWorld = World.Get();
	APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const FAPSStarSystemInfo* Info = Systems.Get(Index);
	if (!Pawn || !Info)
	{
		return false;
	}
	FVector Target;
	FVector LookAt;
	FString What;
	if (bNearPlanet)
	{
		// The first world with a surface, else the first: three of its radii out on its day side.
		const APlanet* Chosen = nullptr;
		for (const TWeakObjectPtr<APlanet>& Weak : Planets)
		{
			const APlanet* Planet = Weak.Get();
			if (!Planet) continue;
			if (!Chosen || (Chosen->IsNotGasGiant() == false && Planet->IsNotGasGiant())) Chosen = Planet;
			if (Chosen->IsNotGasGiant()) break;
		}
		if (Index != ActiveIndex || !Chosen)
		{
			return false;
		}
		const double Radius = Chosen->GetWorldScapeBodyRadiusCm();
		const FVector ToStar = (Info->Location - Chosen->GetActorLocation()).GetSafeNormal();
		Target = Chosen->GetActorLocation() + (ToStar.IsNearlyZero() ? FVector::UpVector : ToStar) * Radius * 3.0;
		LookAt = Chosen->GetActorLocation();
		What = Chosen->AstroName.ToString();
	}
	else
	{
		FVector From = Pawn->GetActorLocation() - Info->Location;
		if (From.IsNearlyZero()) From = FVector::ForwardVector;
		// Rio 05.10 (real scale, stage 2): a REAL SCALE system stands up well inside its room: the visit starts inside that.
		Target = APSRealScale::IsActive(LiveWorld)
			? Info->Location + From.GetSafeNormal() * FMath::Min(Info->RoomCm * 0.9,
				0.8 * APSSystemMaterializerLocal::ApproachCm(*Info, true, APSSystemMaterializerLocal::ApproachRooms))
			: Info->Location + From.GetSafeNormal() * Info->RoomCm * 0.9;
		LookAt = Info->Location;
		What = TEXT("its edge");
	}
	const FRotator Facing = (LookAt - Target).Rotation();
	FQuat Rotation = Facing.Quaternion();
	if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
	{
		// A hull's flight axes need not be the actor's X and Z (02.10 test shots looked away from the target).
		FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
		const FVector Forward = (LookAt - Target).GetSafeNormal();
		FVector Up = FVector::VectorPlaneProject(FVector::UpVector, Forward).GetSafeNormal();
		if (Up.IsNearlyZero()) Up = FVector::VectorPlaneProject(FVector::ForwardVector, Forward).GetSafeNormal();
		Rotation = FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Forward, Up);
	}
	Pawn->SetActorLocationAndRotation(Target, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	Controller->SetControlRotation(Facing);
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] test visit: %s to %s, %s (%.3f AU from its star)"), *Pawn->GetName(), *Info->Name, *What,
		FVector::Dist(Target, Info->Location) / APSSystemMaterializerLocal::AstronomicalUnitCm);
	return true;
}

void FAPSSystemMaterializer::LogState(const FAPSStarSystems& Systems) const
{
	using namespace APSSystemMaterializerLocal;
	static const TCHAR* StageNames[] = {TEXT("idle"), TEXT("spawning"), TEXT("ready"), TEXT("draining")};
	const FAPSStarSystemInfo* Info = Systems.Get(ActiveIndex);
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] materialized: %s, %s, star %s, %d of %d planned world(s)"),
		Info ? *Info->Name : TEXT("none"), StageNames[static_cast<uint8>(Stage)], *GetNameSafe(Star.Get()), Planets.Num(),
		Plan.Num());
	for (const TWeakObjectPtr<APlanet>& Weak : Planets)
	{
		if (const APlanet* Planet = Weak.Get(); Planet && Info)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Stars]   %s %s %d km, %.3f AU from the star, %d moon(s), surface %d"),
				*Planet->AstroName.ToString(), *UEnum::GetValueAsString(Planet->PlanetType), Planet->PlanetRadiusKM,
				FVector::Dist(Planet->GetActorLocation(), Info->Location) / AstronomicalUnitCm, Planet->Moons.Num(),
				static_cast<int32>(Planet->GetWorldScapeStreamingState()));
		}
	}
}
