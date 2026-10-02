#include "APSStarSystems.h"

#include "APSInfrastructure.h"
#include "APSMissions.h"
#include "APSSystemMaterializer.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Generation/APSBodyNames.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "APSStarSystems"

namespace APSStarSystemsLocal
{
	TMap<const UWorld*, FAPSStarSystems*> GRegistry;

	/** The home system's own reach in the relay network before any relay is built, AU. */
	constexpr double HomeReachAu = 3.0;
	/** A visit in person counts after this long inside a system's room. */
	constexpr float VisitSeconds = 3.0f;
	const FName AnchorTag(TEXT("APS.StarSystem"));
	const FString AnchorTagPrefix(TEXT("APS.StarSystem."));

	FString Digits(const FGuid& Id)
	{
		return Id.ToString(EGuidFormats::Digits);
	}
}

FText APSStars::KnowledgeName(const EKnowledge Knowledge)
{
	switch (Knowledge)
	{
	case EKnowledge::Scanned: return LOCTEXT("Scanned", "SCANNED");
	case EKnowledge::Surveyed: return LOCTEXT("Surveyed", "SURVEYED");
	default: return LOCTEXT("Catalogued", "CATALOGUED");
	}
}

FLinearColor APSStars::KnowledgeColour(const EKnowledge Knowledge)
{
	switch (Knowledge)
	{
	case EKnowledge::Scanned: return FLinearColor(0.30f, 0.78f, 1.0f, 1.0f);
	case EKnowledge::Surveyed: return FLinearColor(0.22f, 0.95f, 0.58f, 1.0f);
	default: return FLinearColor(0.62f, 0.68f, 0.74f, 1.0f);
	}
}

FString APSStars::SystemName(const int32 /*ClusterSeed*/, const FGuid& Id)
{
	// The stable id already carries the world (its cluster seed and index), so the menu and the game name a system alike
	// whatever seed each side has at hand.
	return APSBodyNames::Generate(0, TEXT("SYS-") + APSStarSystemsLocal::Digits(Id), APSBodyNames::EKind::Star).ToUpper();
}

FArchive& operator<<(FArchive& Ar, FAPSStarSystemState& State)
{
	uint8 Knowledge = static_cast<uint8>(State.Knowledge);
	Ar << Knowledge;
	State.Knowledge = static_cast<APSStars::EKnowledge>(FMath::Min<uint8>(Knowledge,
		static_cast<uint8>(APSStars::EKnowledge::Surveyed)));
	Ar << State.bClaimed;
	// Names as strings: an FName's index is not stable from one session to the next.
	TArray<FString> Structures;
	if (Ar.IsSaving())
	{
		for (const FName& Name : State.Structures) Structures.Add(Name.ToString());
	}
	Ar << Structures;
	if (Ar.IsLoading())
	{
		State.Structures.Reset();
		for (const FString& Name : Structures) State.Structures.Add(FName(*Name));
	}
	Ar << State.FirstContactSeconds;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FAPSStarSystemsSaveData& Data)
{
	uint8 Version = 2;
	Ar << Version;
	Ar << Data.Ids;
	Ar << Data.States;
	if (Version >= 2)
	{
		// Version 2: the anomaly each system's state reached.
		TArray<uint8> Anomalies;
		if (Ar.IsSaving())
		{
			for (const FAPSStarSystemState& State : Data.States) Anomalies.Add(State.Anomaly);
		}
		Ar << Anomalies;
		if (Ar.IsLoading())
		{
			for (int32 Index = 0; Index < Data.States.Num() && Index < Anomalies.Num(); ++Index) Data.States[Index].Anomaly = Anomalies[Index];
		}
	}
	return Ar;
}

namespace APSStarSystemsLocal
{
	constexpr int32 AnomalyKinds = 6;
	struct FAnomaly
	{
		FText Name;
		FText Story;
		TArray<APSInfrastructure::FAmount> Reward;
	};

	const FAnomaly& AnomalyOf(const int32 Kind)
	{
		using APSInfrastructure::EResource;
		static const TArray<FAnomaly> Kinds = {
			{LOCTEXT("AnomalyDerelict", "DERELICT SHIP"),
				LOCTEXT("StoryDerelict", "A hull older than our records drifts here, its reactor cold. The salvage crews strip it for alloys."),
				{{EResource::Metals, 150.0f}, {EResource::Research, 30.0f}}},
			{LOCTEXT("AnomalySignal", "SIGNAL SOURCE"),
				LOCTEXT("StorySignal", "A narrow pulse repeats every 11.3 hours. Someone built it; the linguists are already arguing."),
				{{EResource::Research, 80.0f}, {EResource::Influence, 20.0f}}},
			{LOCTEXT("AnomalyLens", "GRAVITATIONAL LENS"),
				LOCTEXT("StoryLens", "Light bends around nothing visible: a lens that magnifies the far side of the cluster."),
				{{EResource::Research, 120.0f}}},
			{LOCTEXT("AnomalyComets", "COMET SWARM"),
				LOCTEXT("StoryComets", "Thousands of icy nuclei on one orbit: water, ammonia and fuel for a generation."),
				{{EResource::Volatiles, 150.0f}, {EResource::Energy, 40.0f}}},
			{LOCTEXT("AnomalyDarkBody", "DARK BODY"),
				LOCTEXT("StoryDarkBody", "A mass with no light of its own pulls on the planets here. Its density makes no sense yet."),
				{{EResource::Research, 60.0f}, {EResource::Energy, 60.0f}}},
			{LOCTEXT("AnomalyBeacon", "ALIEN BEACON"),
				LOCTEXT("StoryBeacon", "A lattice of metal hums at the system edge and answers our hails with our own words."),
				{{EResource::Influence, 60.0f}, {EResource::Research, 60.0f}}},
		};
		return Kinds[FMath::Clamp(Kind, 0, Kinds.Num() - 1)];
	}
}

FText FAPSStarSystems::AnomalyName(const int32 Kind)
{
	return Kind == INDEX_NONE ? FText::GetEmpty() : APSStarSystemsLocal::AnomalyOf(Kind).Name;
}

int32 FAPSStarSystems::AnomalyKindOf(const FGuid& Id) const
{
	const int32 Index = IndexOf(Id);
	if (Index == INDEX_NONE || Index == HomeIndex) return INDEX_NONE;
	const uint32 Hash = HashCombine(GetTypeHash(Id), static_cast<uint32>(ClusterSeed) * 2654435761u);
	return Hash % 100 < 12 ? static_cast<int32>((Hash / 100) % APSStarSystemsLocal::AnomalyKinds) : INDEX_NONE;
}

bool FAPSStarSystems::InvestigateAnomaly(const FGuid& Id, const FText& By)
{
	const int32 Kind = AnomalyKindOf(Id);
	const FAPSStarSystemInfo* Info = Find(Id);
	if (Kind == INDEX_NONE || !Info) return false;
	FAPSStarSystemState& State = EditState(Id);
	if (State.Anomaly != 2) return false;
	State.Anomaly = 3;
	++Revision;
	const APSStarSystemsLocal::FAnomaly& Anomaly = APSStarSystemsLocal::AnomalyOf(Kind);
	if (FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get()))
	{
		for (const APSInfrastructure::FAmount& Amount : Anomaly.Reward) Infrastructure->AddStock(Amount.Resource, Amount.Value);
	}
	UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Anomalies"), FText::Format(
		LOCTEXT("SystemAnomalyInvestigated", "{0} investigated the {1} in {2}: {3} Gained {4}."), By, Anomaly.Name,
		FText::FromString(Info->Name), Anomaly.Story, APSInfrastructure::DescribeAmounts(Anomaly.Reward)));
	APSMissionsNotify(World.Get(), APSMissions::EObjective::InvestigateAnomaly, APSStarSystemsLocal::Digits(Id));
	return true;
}

FAPSStarSystems* APSStarSystemsFind(const UWorld* World)
{
	return World ? APSStarSystemsLocal::GRegistry.FindRef(World) : nullptr;
}

void APSStarSystemsRegister(const UWorld* World, FAPSStarSystems* Systems)
{
	if (!World) return;
	if (Systems) APSStarSystemsLocal::GRegistry.Add(World, Systems);
	else APSStarSystemsLocal::GRegistry.Remove(World);
}

FAPSStarSystems::FAPSStarSystems(UWorld* InWorld)
	: World(InWorld)
{
}

FAPSStarSystems::~FAPSStarSystems()
{
	for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : Anchors)
	{
		if (AActor* Anchor = Pair.Value.Get()) Anchor->Destroy();
	}
}

void FAPSStarSystems::Tick(const float DeltaSeconds)
{
	if (Systems.IsEmpty())
	{
		CatalogueRetry -= DeltaSeconds;
		if (CatalogueRetry > 0.0f) return;
		CatalogueRetry = 1.0f;
		if (!ReadCatalogue()) return;
	}
	FollowHome();
	ApplyPendingRestore();
	// Every known or held system has its beacon in the world, so navigation and the maps chart it.
	AnchorClock -= DeltaSeconds;
	if (AnchorClock <= 0.0f)
	{
		AnchorClock = 5.0f;
		TArray<FGuid> Wanted;
		for (const TPair<FGuid, FAPSStarSystemState>& Pair : States)
		{
			const int32 Index = IndexOf(Pair.Key);
			if (Index != INDEX_NONE && Index != HomeIndex && !Systems[Index].bInsideHome
				&& (Pair.Value.Knowledge != APSStars::EKnowledge::Catalogued || Pair.Value.bClaimed))
			{
				Wanted.Add(Pair.Key);
			}
		}
		for (const FGuid& Id : Wanted) GetAnchor(Id);
	}
	VisitClock += DeltaSeconds;
	if (VisitClock >= 0.25f)
	{
		UpdateVisit(VisitClock);
		if (!Materializer.IsValid())
		{
			Materializer = MakeUnique<FAPSSystemMaterializer>(World.Get());
		}
		Materializer->Update(*this, VisitClock);
		VisitClock = 0.0f;
	}
}

AAstroGenerator* FAPSStarSystems::GetGenerator() const
{
	return CatalogueGenerator.Get();
}

AStarCluster* FAPSStarSystems::GetCluster() const
{
	return CatalogueCluster.Get();
}

bool FAPSStarSystems::ReadCatalogue()
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld) return false;
	// The generator the stellar view draws in a game (as the flight model's star catalogue does).
	AAstroGenerator* Generator = nullptr;
	for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WorldGenerationPreview")) && !It->UsesContinuousPreviewFrame()
			&& It->GetCanonicalStellarProjectionDescriptor().bFinalized && IsValid(It->GetPreviewHomeSystem()))
		{
			Generator = *It;
			break;
		}
	}
	if (!Generator) return false;
	const FAPSCanonicalStellarProjectionDescriptor& Descriptor = Generator->GetCanonicalStellarProjectionDescriptor();
	AActor* HomeSystem = Generator->GetPreviewHomeSystem();
	AStarCluster* Cluster = nullptr;
	TArray<AActor*> Attached;
	Generator->GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached)
	{
		if ((Cluster = Cast<AStarCluster>(Actor)) != nullptr) break;
	}
	if (!IsValid(Cluster) || !IsValid(Cluster->StarMeshInstances) || Cluster->PotentialStarSystems.IsEmpty()) return false;
	if (Descriptor.bConsumedFinalizedDataset)
	{
		// Read the proxies exactly as drawn, once the stellar view has scaled the generator root back to physical size.
		const double RequiredScale = Descriptor.Galaxy.PositionScale > 0.0 ? 1.0 / Descriptor.Galaxy.PositionScale : 0.0;
		const double RootScale = Generator->GetActorScale3D().GetAbsMax();
		if (!FMath::IsFinite(RequiredScale) || RequiredScale <= 0.0 || FMath::Abs(RootScale / RequiredScale - 1.0) > 1.0e-3)
		{
			return false;
		}
	}

	Home = HomeSystem;
	HomeLocation = HomeSystem->GetActorLocation();
	ClusterSeed = Cluster->GenerationSeed;
	CatalogueGenerator = Generator;
	CatalogueCluster = Cluster;
	const int32 Count = Cluster->PotentialStarSystems.Num();
	Systems.Reset(Count);
	FromHome.Reset(Count);
	IndexById.Reset();
	HomeIndex = INDEX_NONE;
	const FTransform ComponentTransform = Cluster->StarMeshInstances->GetComponentTransform();
	const FAPSCanonicalStellarProjectionFrame& Frame = Descriptor.StarCluster;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FClusterStarSystemRecord& Record = Cluster->PotentialStarSystems[Index];
		FVector Offset = FVector::ZeroVector;
		const bool bHome = Record.bMaterialized && Record.MaterializedSystem.Get() == HomeSystem;
		if (!bHome)
		{
			Offset = Descriptor.bConsumedFinalizedDataset && Cluster->SystemProxyBaseTransforms.IsValidIndex(Index)
				? ComponentTransform.TransformPosition(Cluster->SystemProxyBaseTransforms[Index].GetLocation()) - HomeLocation
				: Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm;
		}
		FAPSStarSystemInfo& Info = Systems.AddDefaulted_GetRef();
		Info.Id = Record.StableId;
		Info.Record = Index;
		Info.bHome = bHome;
		Info.StarCount = FMath::Max(1, Record.SystemModel.AmountOfStars);
		Info.PotentialPlanets = Record.SystemModel.PotentialPlanetCount;
		Info.Spectral = Record.PrimaryStarModel.FullSpectralClass.IsNone() ? FString()
			: Record.PrimaryStarModel.FullSpectralClass.ToString();
		Info.Colour = Record.PrimaryStarModel.SurfaceTemperature > 0
			? FLinearColor::MakeFromColorTemperature(FMath::Clamp(float(Record.PrimaryStarModel.SurfaceTemperature), 1500.0f, 15000.0f))
			: FLinearColor(1.0f, 0.92f, 0.78f, 1.0f);
		Info.Location = HomeLocation + Offset;
		Info.HomeDistanceCm = Offset.Size();
		FromHome.Add(Offset);
		IndexById.Add(Info.Id, Index);
		if (bHome) HomeIndex = Index;
	}
	if (HomeIndex == INDEX_NONE)
	{
		// The nearest record stands for the home when none is marked materialized.
		double Best = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (FromHome[Index].Size() < Best) { Best = FromHome[Index].Size(); HomeIndex = Index; }
		}
		if (HomeIndex != INDEX_NONE) Systems[HomeIndex].bHome = true;
	}
	// Names: the catalogue's own, the home system after its star.
	const AStar* HomeStar = Generator->HomeStar;
	for (FAPSStarSystemInfo& Info : Systems)
	{
		Info.Name = Info.bHome && IsValid(HomeStar) && !HomeStar->AstroName.IsNone()
			? HomeStar->AstroName.ToString().ToUpper() : APSStars::SystemName(ClusterSeed, Info.Id);
		if (Info.bHome && IsValid(HomeStar) && !HomeStar->FullSpectralName.IsNone() && Info.Spectral.IsEmpty())
		{
			Info.Spectral = HomeStar->FullSpectralName.ToString();
		}
	}

	// Spacing and the grid: the median of sampled neighbour distances sets the cell.
	TArray<double> Samples;
	for (int32 Sample = 0; Sample < FMath::Min(Count, 96); ++Sample)
	{
		const int32 Index = static_cast<int32>((static_cast<int64>(Sample) * 7919) % Count);
		double Best = TNumericLimits<double>::Max();
		for (int32 Other = 0; Other < Count; ++Other)
		{
			if (Other == Index) continue;
			const double Distance = FVector::DistSquared(FromHome[Index], FromHome[Other]);
			if (Distance > 0.0) Best = FMath::Min(Best, Distance);
		}
		if (Best < TNumericLimits<double>::Max()) Samples.Add(FMath::Sqrt(Best));
	}
	Samples.Sort();
	const double Median = Samples.IsEmpty() ? APSStars::AstronomicalUnitCm : Samples[Samples.Num() / 2];
	CellCm = FMath::Max(Median * 2.0, 1.0e10);
	Grid.Reset();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Grid.FindOrAdd(CellOf(FromHome[Index])).Add(Index);
	}
	// Room: half the distance to the nearest neighbour (rings of cells until one is found).
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FIntVector Cell = CellOf(FromHome[Index]);
		double Best = TNumericLimits<double>::Max();
		for (int32 Ring = 1; Ring <= 6 && Best == TNumericLimits<double>::Max(); ++Ring)
		{
			for (int32 X = -Ring; X <= Ring; ++X)
			for (int32 Y = -Ring; Y <= Ring; ++Y)
			for (int32 Z = -Ring; Z <= Ring; ++Z)
			{
				if (FMath::Max3(FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)) != Ring && Ring > 1) continue;
				if (const TArray<int32>* Members = Grid.Find(Cell + FIntVector(X, Y, Z)))
				{
					for (const int32 Other : *Members)
					{
						if (Other == Index) continue;
						const double Distance = FVector::DistSquared(FromHome[Index], FromHome[Other]);
						if (Distance > 0.0) Best = FMath::Min(Best, Distance);
					}
				}
			}
			// The centre cell belongs to the first ring.
			if (Ring == 1) continue;
		}
		Systems[Index].RoomCm = Best < TNumericLimits<double>::Max() ? 0.5 * FMath::Sqrt(Best) : CellCm;
	}
	if (Systems.IsValidIndex(HomeIndex))
	{
		// The home system's room is its own sphere: its outermost orbit with a quarter more.
		double Outermost = 0.0;
		for (TActorIterator<APlanet> It(LiveWorld); It; ++It)
		{
			if (IsValid(*It) && IsValid(It->ParentStar))
			{
				Outermost = FMath::Max(Outermost, FVector::Dist(It->GetActorLocation(), It->ParentStar->GetActorLocation()));
			}
		}
		Systems[HomeIndex].RoomCm = FMath::Max(Systems[HomeIndex].RoomCm, Outermost * 1.25);
		FAPSStarSystemState& HomeState = States.FindOrAdd(Systems[HomeIndex].Id);
		HomeState.Knowledge = APSStars::EKnowledge::Surveyed;
		HomeState.bClaimed = true;

		// Rio 01.10 ("our system is realistic, but look how many stars fall into it"): the cluster stars inside the home
		// system's own sphere (a tenth beyond its room) are suppressed; neighbours begin outside it.
		const double HomeSphereCm = Systems[HomeIndex].RoomCm * 1.1;
		TArray<int32> Inside;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Index != HomeIndex && FromHome[Index].SizeSquared() < FMath::Square(HomeSphereCm))
			{
				Systems[Index].bInsideHome = true;
				const int32 Instance = Cluster->PotentialStarSystems[Index].InstanceIndex;
				Inside.Add(Instance != INDEX_NONE ? Instance : Index);
			}
		}
		const int32 Suppressed = Inside.IsEmpty() ? 0 : Generator->SuppressClusterProxies(Inside);
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars] home sphere %.2f AU: %d cluster stars inside it suppressed (%d newly)"),
			HomeSphereCm / APSStars::AstronomicalUnitCm, Inside.Num(), Suppressed);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] catalogue read: %d systems, home %s, spacing median %.2f AU, cell %.2f AU, seed %d"),
		Count, Systems.IsValidIndex(HomeIndex) ? *Systems[HomeIndex].Name : TEXT("-"), Median / APSStars::AstronomicalUnitCm,
		CellCm / APSStars::AstronomicalUnitCm, ClusterSeed);
	++Revision;
	return true;
}

FIntVector FAPSStarSystems::CellOf(const FVector& Offset) const
{
	const double Cell = FMath::Max(CellCm, 1.0);
	return FIntVector(FMath::FloorToInt(Offset.X / Cell), FMath::FloorToInt(Offset.Y / Cell), FMath::FloorToInt(Offset.Z / Cell));
}

void FAPSStarSystems::FollowHome()
{
	const AActor* HomeActor = Home.Get();
	if (!HomeActor) return;
	const FVector Now = HomeActor->GetActorLocation();
	if (Now.Equals(HomeLocation, 1.0)) return;
	// The home moved (an origin shift): every location and anchor follows it.
	HomeLocation = Now;
	for (int32 Index = 0; Index < Systems.Num(); ++Index)
	{
		Systems[Index].Location = HomeLocation + FromHome[Index];
	}
	for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : Anchors)
	{
		const int32 Index = IndexOf(Pair.Key);
		if (AActor* Anchor = Pair.Value.Get(); Anchor && Systems.IsValidIndex(Index))
		{
			Anchor->SetActorLocation(Systems[Index].Location);
		}
	}
}

const FAPSStarSystemInfo* FAPSStarSystems::Find(const FGuid& Id) const
{
	const int32* Index = IndexById.Find(Id);
	return Index ? Get(*Index) : nullptr;
}

int32 FAPSStarSystems::IndexOf(const FGuid& Id) const
{
	const int32* Index = IndexById.Find(Id);
	return Index ? *Index : INDEX_NONE;
}

void FAPSStarSystems::FindNearest(const FVector& Location, const int32 Count, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	if (Count <= 0 || Systems.IsEmpty()) return;
	// Keep the best Count by insertion: Count is small (a list on a screen), the catalogue a few tens of thousands.
	TArray<TPair<double, int32>> Best;
	Best.Reserve(Count + 1);
	for (int32 Index = 0; Index < Systems.Num(); ++Index)
	{
		if (Systems[Index].bInsideHome) continue;
		const double Distance = FVector::DistSquared(Location, Systems[Index].Location);
		if (Best.Num() == Count && Distance >= Best.Last().Key) continue;
		int32 At = Best.Num();
		while (At > 0 && Best[At - 1].Key > Distance) --At;
		Best.Insert(TPair<double, int32>(Distance, Index), At);
		if (Best.Num() > Count) Best.Pop(EAllowShrinking::No);
	}
	for (const TPair<double, int32>& Pair : Best) OutIndices.Add(Pair.Value);
}

void FAPSStarSystems::Search(const FString& Text, const int32 Limit, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	const FString Wanted = Text.TrimStartAndEnd();
	if (Wanted.IsEmpty() || Limit <= 0) return;
	TArray<int32> Found;
	for (int32 Index = 0; Index < Systems.Num(); ++Index)
	{
		if (!Systems[Index].bInsideHome && Systems[Index].Name.Contains(Wanted, ESearchCase::IgnoreCase)) Found.Add(Index);
	}
	Found.Sort([this](const int32 A, const int32 B) { return Systems[A].HomeDistanceCm < Systems[B].HomeDistanceCm; });
	for (int32 Each = 0; Each < Found.Num() && Each < Limit; ++Each) OutIndices.Add(Found[Each]);
}

int32 FAPSStarSystems::FindContaining(const FVector& Location) const
{
	if (Systems.IsEmpty()) return INDEX_NONE;
	if (Systems.IsValidIndex(HomeIndex)
		&& FVector::DistSquared(Location, Systems[HomeIndex].Location) <= FMath::Square(Systems[HomeIndex].RoomCm))
	{
		return HomeIndex;
	}
	const FIntVector Cell = CellOf(Location - HomeLocation);
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 X = -1; X <= 1; ++X)
	for (int32 Y = -1; Y <= 1; ++Y)
	for (int32 Z = -1; Z <= 1; ++Z)
	{
		if (const TArray<int32>* Members = Grid.Find(Cell + FIntVector(X, Y, Z)))
		{
			for (const int32 Index : *Members)
			{
				if (Systems[Index].bInsideHome) continue;
				const double Distance = FVector::DistSquared(Location, Systems[Index].Location);
				if (Distance <= FMath::Square(Systems[Index].RoomCm) && Distance < BestDistance)
				{
					BestDistance = Distance;
					Best = Index;
				}
			}
		}
	}
	return Best;
}

void FAPSStarSystems::GetKnown(TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	for (const TPair<FGuid, FAPSStarSystemState>& Pair : States)
	{
		const int32 Index = IndexOf(Pair.Key);
		if (Index != INDEX_NONE && !Systems[Index].bInsideHome
			&& (Pair.Value.Knowledge != APSStars::EKnowledge::Catalogued || Pair.Value.bClaimed))
		{
			OutIndices.Add(Index);
		}
	}
	OutIndices.Sort([this](const int32 A, const int32 B) { return Systems[A].HomeDistanceCm < Systems[B].HomeDistanceCm; });
}

FAPSStarSystemState FAPSStarSystems::GetState(const FGuid& Id) const
{
	const FAPSStarSystemState* State = States.Find(Id);
	return State ? *State : FAPSStarSystemState();
}

APSStars::EKnowledge FAPSStarSystems::GetKnowledge(const FGuid& Id) const
{
	const FAPSStarSystemState* State = States.Find(Id);
	return State ? State->Knowledge : APSStars::EKnowledge::Catalogued;
}

bool FAPSStarSystems::IsClaimed(const FGuid& Id) const
{
	const FAPSStarSystemState* State = States.Find(Id);
	return State && State->bClaimed;
}

FAPSStarSystemState& FAPSStarSystems::EditState(const FGuid& Id)
{
	return States.FindOrAdd(Id);
}

void FAPSStarSystems::Learn(const FGuid& Id, const APSStars::EKnowledge Knowledge, const FText& How)
{
	const FAPSStarSystemInfo* Info = Find(Id);
	if (!Info) return;
	FAPSStarSystemState& State = EditState(Id);
	if (State.Knowledge >= Knowledge) return;
	State.Knowledge = Knowledge;
	if (State.FirstContactSeconds <= 0.0 && World.IsValid()) State.FirstContactSeconds = World->GetTimeSeconds();
	++Revision;
	UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Expansion"), FText::Format(
		LOCTEXT("Learned", "{0} {1}: {2}. {3} star(s), up to {4} planet(s)."), FText::FromString(Info->Name),
		APSStars::KnowledgeName(Knowledge), How, FText::AsNumber(Info->StarCount), FText::AsNumber(Info->PotentialPlanets)));
	const FString Subject = APSStarSystemsLocal::Digits(Id);
	APSMissionsNotify(World.Get(), APSMissions::EObjective::ScanSystem, Subject);
	if (Knowledge == APSStars::EKnowledge::Surveyed)
	{
		APSMissionsNotify(World.Get(), APSMissions::EObjective::SurveySystem, Subject);
	}
	// Its anomaly, if it has one: a scan hears it, a survey pins it down.
	if (const int32 Kind = AnomalyKindOf(Id); Kind != INDEX_NONE)
	{
		const FText Name = AnomalyName(Kind);
		if (State.Anomaly < 1)
		{
			State.Anomaly = 1;
			UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Anomalies"), FText::Format(
				LOCTEXT("SystemAnomalyDetected", "Something answers from {0}: a {1}. Survey the star system to locate it."),
				FText::FromString(Info->Name), Name));
		}
		if (Knowledge >= APSStars::EKnowledge::Surveyed && State.Anomaly < 2)
		{
			State.Anomaly = 2;
			UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Anomalies"), FText::Format(
				LOCTEXT("SystemAnomalyLocated", "The {0} in {1} is located. Fly there yourself and stay inside the system, or send an expedition."),
				Name, FText::FromString(Info->Name)));
			APSMissionsNotify(World.Get(), APSMissions::EObjective::LocateAnomaly, Subject);
		}
	}
}

void FAPSStarSystems::AddStructure(const FGuid& Id, const FName StructureId, const bool bClaims)
{
	const FAPSStarSystemInfo* Info = Find(Id);
	if (!Info) return;
	FAPSStarSystemState& State = EditState(Id);
	State.Structures.Add(StructureId);
	const bool bNewClaim = bClaims && !State.bClaimed;
	State.bClaimed |= bClaims;
	++Revision;
	if (bNewClaim)
	{
		UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Expansion"), FText::Format(
			LOCTEXT("Claimed", "{0} is the civilization's: its beacon is lit."), FText::FromString(Info->Name)));
		APSMissionsNotify(World.Get(), APSMissions::EObjective::ClaimSystem, APSStarSystemsLocal::Digits(Id));
		TArray<TPair<int32, int32>> Links;
		GetNetwork(Links);
		APSMissionsNotify(World.Get(), APSMissions::EObjective::LinkSystems, FString(), 0);
	}
}

double FAPSStarSystems::ReachCm(const int32 Index) const
{
	if (!Systems.IsValidIndex(Index)) return 0.0;
	double Reach = Index == HomeIndex ? APSStarSystemsLocal::HomeReachAu * APSStars::AstronomicalUnitCm : 0.0;
	if (const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World.Get()))
	{
		Reach = FMath::Max(Reach, Infrastructure->RelayReachCm(Systems[Index].Id));
	}
	return Reach;
}

void FAPSStarSystems::GetNetwork(TArray<TPair<int32, int32>>& OutLinks) const
{
	OutLinks.Reset();
	TArray<int32> Claimed;
	for (const TPair<FGuid, FAPSStarSystemState>& Pair : States)
	{
		const int32 Index = IndexOf(Pair.Key);
		if (Index != INDEX_NONE && Pair.Value.bClaimed) Claimed.Add(Index);
	}
	for (int32 A = 0; A < Claimed.Num(); ++A)
	{
		for (int32 B = A + 1; B < Claimed.Num(); ++B)
		{
			const double Reach = FMath::Max(ReachCm(Claimed[A]), ReachCm(Claimed[B]));
			if (Reach > 0.0 && FVector::Dist(Systems[Claimed[A]].Location, Systems[Claimed[B]].Location) <= Reach)
			{
				OutLinks.Emplace(Claimed[A], Claimed[B]);
			}
		}
	}
}

bool FAPSStarSystems::IsInReach(const FGuid& Id) const
{
	const int32 Target = IndexOf(Id);
	if (Target == INDEX_NONE) return false;
	for (const TPair<FGuid, FAPSStarSystemState>& Pair : States)
	{
		const int32 Index = IndexOf(Pair.Key);
		if (Index == INDEX_NONE || !Pair.Value.bClaimed) continue;
		if (Index == Target) return true;
		const double Reach = ReachCm(Index);
		if (Reach > 0.0 && FVector::Dist(Systems[Index].Location, Systems[Target].Location) <= Reach) return true;
	}
	return false;
}

AActor* FAPSStarSystems::GetAnchor(const FGuid& Id)
{
	if (const TWeakObjectPtr<AActor>* Existing = Anchors.Find(Id); Existing && Existing->IsValid())
	{
		return Existing->Get();
	}
	UWorld* LiveWorld = World.Get();
	const FAPSStarSystemInfo* Info = Find(Id);
	if (!LiveWorld || !Info) return nullptr;
	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// A stable name: fleet orders aimed at the system find it again after a load.
	Parameters.Name = FName(*(TEXT("APS_StarSystem_") + APSStarSystemsLocal::Digits(Id)));
	Parameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	AActor* Anchor = LiveWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Info->Location), Parameters);
	if (!Anchor) return nullptr;
	USceneComponent* Root = NewObject<USceneComponent>(Anchor, TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	Anchor->SetRootComponent(Root);
	Root->RegisterComponent();
	Anchor->SetActorLocation(Info->Location);
	Anchor->Tags.Add(APSStarSystemsLocal::AnchorTag);
	Anchor->Tags.Add(FName(*(APSStarSystemsLocal::AnchorTagPrefix + APSStarSystemsLocal::Digits(Id))));
#if WITH_EDITOR
	Anchor->SetActorLabel(TEXT("StarSystem ") + Info->Name);
#endif
	Anchors.Add(Id, Anchor);
	return Anchor;
}

bool FAPSStarSystems::AnchorSystem(const AActor* Actor, FGuid& OutId)
{
	if (!Actor || !Actor->ActorHasTag(APSStarSystemsLocal::AnchorTag)) return false;
	for (const FName& Tag : Actor->Tags)
	{
		const FString Text = Tag.ToString();
		if (Text.StartsWith(APSStarSystemsLocal::AnchorTagPrefix)
			&& FGuid::Parse(Text.RightChop(APSStarSystemsLocal::AnchorTagPrefix.Len()), OutId))
		{
			return true;
		}
	}
	return false;
}

void FAPSStarSystems::UpdateVisit(const float DeltaSeconds)
{
	UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const APawn* Pilot = Controller ? Controller->GetPawn() : nullptr;
	if (!Pilot)
	{
		VisitIndex = INDEX_NONE;
		return;
	}
	const int32 Inside = FindContaining(Pilot->GetActorLocation());
	if (Inside != VisitIndex)
	{
		VisitIndex = Inside;
		VisitSeconds = 0.0f;
		return;
	}
	if (VisitIndex == INDEX_NONE || VisitIndex == HomeIndex) return;
	VisitSeconds += DeltaSeconds;
	const FGuid Id = Systems[VisitIndex].Id;
	if (VisitSeconds >= APSStarSystemsLocal::VisitSeconds && GetKnowledge(Id) < APSStars::EKnowledge::Surveyed)
	{
		Learn(Id, APSStars::EKnowledge::Surveyed, LOCTEXT("VisitedInPerson", "visited in person"));
		APSMissionsNotify(LiveWorld, APSMissions::EObjective::VisitSystem, APSStarSystemsLocal::Digits(Id));
	}
	// A located anomaly is investigated by the pilot who stays in its system.
	if (VisitSeconds >= APSStarSystemsLocal::VisitSeconds * 2.0f && GetState(Id).Anomaly == 2)
	{
		InvestigateAnomaly(Id, LOCTEXT("PilotInPerson", "The pilot in person"));
	}
}

void FAPSStarSystems::CaptureSave(FAPSStarSystemsSaveData& OutData) const
{
	OutData.Ids.Reset();
	OutData.States.Reset();
	for (const TPair<FGuid, FAPSStarSystemState>& Pair : States)
	{
		OutData.Ids.Add(Pair.Key);
		OutData.States.Add(Pair.Value);
	}
	// A save made before the catalogue was read keeps what the load brought.
	if (PendingRestore.IsSet())
	{
		for (int32 Index = 0; Index < PendingRestore->Ids.Num() && Index < PendingRestore->States.Num(); ++Index)
		{
			if (!States.Contains(PendingRestore->Ids[Index]))
			{
				OutData.Ids.Add(PendingRestore->Ids[Index]);
				OutData.States.Add(PendingRestore->States[Index]);
			}
		}
	}
}

void FAPSStarSystems::RestoreSave(FAPSStarSystemsSaveData&& Data)
{
	PendingRestore = MoveTemp(Data);
	if (!Systems.IsEmpty()) ApplyPendingRestore();
}

void FAPSStarSystems::ApplyPendingRestore()
{
	if (!PendingRestore.IsSet()) return;
	const FAPSStarSystemsSaveData Data = MoveTemp(PendingRestore.GetValue());
	PendingRestore.Reset();
	int32 Restored = 0;
	for (int32 Index = 0; Index < Data.Ids.Num() && Index < Data.States.Num(); ++Index)
	{
		if (!IndexById.Contains(Data.Ids[Index])) continue;
		FAPSStarSystemState& State = States.FindOrAdd(Data.Ids[Index]);
		const bool bHomeClaim = State.bClaimed;
		State = Data.States[Index];
		State.bClaimed |= bHomeClaim;
		++Restored;
	}
	++Revision;
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] restored %d of %d saved systems"), Restored, Data.Ids.Num());
}

void FAPSStarSystems::LogNearest(const int32 Count) const
{
	const UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	const FVector From = Controller && Controller->GetPawn() ? Controller->GetPawn()->GetActorLocation() : HomeLocation;
	TArray<int32> Nearest;
	FindNearest(From, Count, Nearest);
	UE_LOG(LogTemp, Log, TEXT("[APS.Stars] %d systems; nearest %d to the pilot:"), Systems.Num(), Nearest.Num());
	for (const int32 Index : Nearest)
	{
		const FAPSStarSystemInfo& Info = Systems[Index];
		const FAPSStarSystemState State = GetState(Info.Id);
		UE_LOG(LogTemp, Log, TEXT("[APS.Stars]   %-14s %-6s %.2f AU away, room %.2f AU, %d star(s), %d planet(s), %s%s"),
			*Info.Name, *Info.Spectral, FVector::Dist(From, Info.Location) / APSStars::AstronomicalUnitCm,
			Info.RoomCm / APSStars::AstronomicalUnitCm, Info.StarCount, Info.PotentialPlanets,
			*APSStars::KnowledgeName(State.Knowledge).ToString(), State.bClaimed ? TEXT(", claimed") : TEXT(""));
	}
}

namespace APSStarSystemsLocal
{
	FAutoConsoleCommandWithWorldAndArgs ListCommand(TEXT("aps.Stars.List"),
		TEXT("Logs the star systems nearest the pilot: aps.Stars.List [count]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* InWorld)
		{
			if (const FAPSStarSystems* Systems = APSStarSystemsFind(InWorld))
			{
				Systems->LogNearest(Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 200) : 12);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs LearnCommand(TEXT("aps.Stars.Learn"),
		TEXT("Test: raises what the civilization knows of a system: aps.Stars.Learn <name> <1 scanned | 2 surveyed>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* InWorld)
		{
			FAPSStarSystems* Systems = APSStarSystemsFind(InWorld);
			if (!Systems || Args.Num() < 2) return;
			TArray<int32> Found;
			Systems->Search(Args[0], 1, Found);
			if (const FAPSStarSystemInfo* Info = Found.IsEmpty() ? nullptr : Systems->Get(Found[0]))
			{
				Systems->Learn(Info->Id, static_cast<APSStars::EKnowledge>(FMath::Clamp(FCString::Atoi(*Args[1]), 1, 2)),
					LOCTEXT("ConsoleLearn", "by the console"));
			}
		}));
}

#undef LOCTEXT_NAMESPACE
