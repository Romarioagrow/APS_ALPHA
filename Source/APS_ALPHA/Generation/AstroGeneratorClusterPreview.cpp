#include "AstroGenerator.h"

#include "MoonGenerator.h"
#include "PlanetGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "PlanetarySurfaceGenerator.h"
#include "StarGenerator.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace APSGeneratedBodyIdentity
{
	FRandomStream Stream(int32 WorldSeed, const FString& Address, const TCHAR* Channel);
	FName Name(int32 WorldSeed, const FString& Address, const FString& Kind);
}

AStarSystem* AAstroGenerator::GetContinuousPreviewOwningSystem(const AActor* Actor) const
{
	for (const AActor* Parent = Actor; IsValid(Parent); Parent = Parent->GetAttachParentActor())
		if (const AStarSystem* System = Cast<AStarSystem>(Parent)) return const_cast<AStarSystem*>(System);
	return nullptr;
}

AStarSystem* AAstroGenerator::GetContinuousPreviewActiveSystem() const
{
	if (SelectedPreviewClusterSystemIndex != INDEX_NONE)
	{
		const TObjectPtr<AStarSystem>* System = ContinuousMaterializedSystems.Find(SelectedPreviewClusterSystemIndex);
		return System && IsValid(*System) ? System->Get() : nullptr;
	}
	return GeneratedHomeStarSystem;
}

FVector AAstroGenerator::GetContinuousPreviewSystemCenter(const AStarSystem* System) const
{
	if (System == GeneratedHomeStarSystem || !IsValid(System) || !IsValid(GeneratedStarCluster))
		return FVector::ZeroVector;
	for (const auto& Entry : ContinuousMaterializedSystems)
	{
		if (Entry.Value != System) continue;
		if (const FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystem(Entry.Key))
		{
			const FAPSCanonicalStellarProjectionFrame& Frame = GeneratedStarCluster->CanonicalProjectionFrame;
			return Frame.GetCanonicalRootPositionCm(Record->ClusterLocalLocation) - Frame.CanonicalAnchorCm;
		}
	}
	return FVector::ZeroVector;
}

bool AAstroGenerator::GetPreviewStarEditContext(FString& OutAddress, FStarModel& OutModel) const
{
	OutAddress.Reset();
	if (!UsesContinuousPreviewFrame()) return false;
	const AStarSystem* System = GetContinuousPreviewActiveSystem();
	if (!IsValid(System)) return false;
	const AStar* Star = ContinuousSelectedStar.Get();
	if (!IsValid(Star) || !Star->IsAttachedTo(System)) Star = System->MainStar;
	const int32 Index = System->GetStars().IndexOfByKey(Star);
	if (Index == INDEX_NONE) return false;
	const FString SystemAddress = System == GeneratedHomeStarSystem ? TEXT("SYS0")
		: TEXT("SYS-") + System->StableSystemId.ToString(EGuidFormats::Digits);
	OutAddress = FString::Printf(TEXT("%s/S%d"), *SystemAddress, Index);
	const TSharedPtr<FStarModel>* Model = PreviewResolvedStarModels.Find(OutAddress);
	if (!Model || !Model->IsValid()) { OutAddress.Reset(); return false; }
	OutModel = **Model;
	if (IsValid(GeneratedWorldModel)) GeneratedWorldModel->ApplyPreviewStarEditOverride(OutAddress, OutModel);
	return true;
}

bool AAstroGenerator::GetPreviewSystemEditContext(FString& OutAddress, FStarSystemModel& OutModel) const
{
	OutAddress.Reset();
	const AStarSystem* System = UsesContinuousPreviewFrame() ? GetContinuousPreviewActiveSystem()
		: SelectedPreviewClusterSystemIndex == INDEX_NONE ? GeneratedHomeStarSystem : nullptr;
	if (!IsValid(System)) return false;
	OutAddress = System == GeneratedHomeStarSystem ? TEXT("SYS0")
		: TEXT("SYS-") + System->StableSystemId.ToString(EGuidFormats::Digits);
	OutModel = FStarSystemModel();
	OutModel.StableId = System->StableSystemId;
	OutModel.GenerationSeed = System->GenerationSeed;
	OutModel.AmountOfStars = System->GetStars().Num();
	OutModel.StarSystemType = OutModel.AmountOfStars == 1 ? EStarType::SingleStar
		: OutModel.AmountOfStars == 2 ? EStarType::DoubleStar
		: OutModel.AmountOfStars == 3 ? EStarType::TripleStar : EStarType::MultipleStar;
	for (const AStar* Star : System->GetStars())
		if (IsValid(Star) && IsValid(Star->PlanetarySystem))
			OutModel.PotentialPlanetCount += Star->PlanetarySystem->PlanetsActorsList.Num();
	OutModel.bHasPlanetarySystem = OutModel.PotentialPlanetCount > 0;
	if (IsValid(GeneratedWorldModel))
		if (const FAPSPreviewSystemEditOverride* Edit = GeneratedWorldModel->FindPreviewSystemEditOverride(OutAddress))
			Edit->ApplyToSystem(OutModel);
	return true;
}

int32 AAstroGenerator::GetPreviewHomePlanetCount() const
{
	if (!IsValid(GeneratedHomeStarSystem) || !IsValid(GeneratedHomeStarSystem->MainStar)
		|| !IsValid(GeneratedHomeStarSystem->MainStar->PlanetarySystem)) return 0;
	const APlanetarySystem* Family = GeneratedHomeStarSystem->MainStar->PlanetarySystem;
	FPlanetarySystemModel Model;
	Model.AmountOfPlanets = Family->PlanetsActorsList.Num();
	Model.PlanetarySystemType = Family->PlanetarySystemType;
	if (IsValid(GeneratedWorldModel))
		if (const FAPSPreviewSystemEditOverride* Edit = GeneratedWorldModel->FindPreviewSystemEditOverride(TEXT("SYS0")))
			Edit->ApplyToFamily(Model, 0, Edit->StarCount == INDEX_NONE
				? GeneratedHomeStarSystem->GetStars().Num() : Edit->StarCount);
	return Model.AmountOfPlanets;
}

FVector AAstroGenerator::GetContinuousPreviewPhysicalPosition(const AActor* Actor) const
{
	if (!IsValid(Actor)) return FVector::ZeroVector;
	if (const AStarSystem* System = GetContinuousPreviewOwningSystem(Actor))
		return GetContinuousPreviewSystemCenter(System) + (Actor->GetActorLocation() - System->GetActorLocation());
	return Actor->GetActorLocation() - (IsValid(GeneratedHomeStarSystem)
		? GeneratedHomeStarSystem->GetActorLocation() : FVector::ZeroVector);
}

bool AAstroGenerator::BuildContinuousPreviewSystemLayout(const FClusterStarSystemRecord& Record,
	UStarGenerator* Stars, UPlanetarySystemGenerator* Families,
	TArray<FAPSContinuousPreviewStarLayout>& OutStars, double& OutRadiusCm)
{
	OutStars.Reset();
	OutRadiusCm = 0.0;
	const int32 StarCount = Record.SystemModel.AmountOfStars;
	if (!IsValid(GeneratedStarCluster) || !IsValid(Stars) || !IsValid(Families)
		|| !Record.StableId.IsValid() || StarCount < 1 || StarCount > 6
		|| Record.SystemModel.PotentialPlanetCount < 0 || Record.SystemModel.PotentialPlanetCount > 128) return false;
	const int32 Seed = Record.SystemModel.GenerationSeed;
	const FString Address = TEXT("SYS-") + Record.StableId.ToString(EGuidFormats::Digits);
	TSharedPtr<FStarClusterModel> Population = MakeShared<FStarClusterModel>();
	Population->StarClusterPopulation = GeneratedStarCluster->StarClusterPopulation;
	Population->StarClusterComposition = GeneratedStarCluster->StarClusterComposition;
	constexpr double AU = 14960000000000.0;
	constexpr double EarthRadiusCm = 6371.0 * 100000.0;
	for (int32 StarIndex = 0; StarIndex < StarCount; ++StarIndex)
	{
		FAPSContinuousPreviewStarLayout& Layout = OutStars.AddDefaulted_GetRef();
		const FString StarAddress = FString::Printf(TEXT("%s/S%d"), *Address, StarIndex);
		Stars->SetGenerationSeed(APSGeneratedBodyIdentity::Stream(Seed, StarAddress, TEXT("physics")).GetInitialSeed());
		Layout.StarModel = MakeShared<FStarModel>();
		const TSharedPtr<FStarModel>& StarModel = Layout.StarModel;
		if (StarIndex == 0) *StarModel = Record.PrimaryStarModel;
		else Stars->GenerateStarModelByProbability(StarModel, Population);
		if (IsValid(GeneratedWorldModel)) GeneratedWorldModel->ApplyPreviewStarEditOverride(StarAddress, *StarModel);
		if (!FMath::IsFinite(StarModel->RadiusKM) || StarModel->RadiusKM <= 0.0) return false;
		Layout.FamilyModel = MakeShared<FPlanetarySystemModel>();
		const TSharedPtr<FPlanetarySystemModel>& FamilyModel = Layout.FamilyModel;
		FamilyModel->AmountOfPlanets = Record.SystemModel.PotentialPlanetCount / StarCount
			+ (StarIndex < Record.SystemModel.PotentialPlanetCount % StarCount ? 1 : 0);
		FamilyModel->PlanetarySystemType = FamilyModel->AmountOfPlanets == 0
			? EPlanetarySystemType::NoPlanetSystem : EPlanetarySystemType::MultiPlanetSystem;
		FamilyModel->OrbitDistributionType = EOrbitDistributionType::Uniform;
		if (IsValid(GeneratedWorldModel))
			if (const FAPSPreviewSystemEditOverride* Edit = GeneratedWorldModel->FindPreviewSystemEditOverride(Address))
				Edit->ApplyToFamily(*FamilyModel, StarIndex, StarCount);
		Families->SetGenerationSeed(APSGeneratedBodyIdentity::Stream(Seed, StarAddress, TEXT("planets")).GetInitialSeed());
		Families->GenerateCustomPlanetarySystemModel(FamilyModel, StarModel, PlanetGenerator, MoonGenerator);
		ApplyPreviewBodyEditOverridesToModels(GeneratedWorldModel, StarIndex, *FamilyModel, Address);
		bool bEditedMoonHierarchy = false;
		for (int32 PlanetIndex = 0; PlanetIndex < FamilyModel->PlanetsList.Num(); ++PlanetIndex)
		{
			const TSharedPtr<FPlanetData>& Data = FamilyModel->PlanetsList[PlanetIndex];
			if (!Data || !Data->PlanetModel || !IsValid(GeneratedWorldModel)) continue;
			const FString PlanetAddress = FString::Printf(TEXT("%s/P%d"), *StarAddress, PlanetIndex);
			const FAPSPreviewBodyEditOverride* Override = GeneratedWorldModel->FindPreviewBodyEditOverride(PlanetAddress);
			if (Override && Override->MoonCount != INDEX_NONE && Data->PlanetModel->MoonsList.Num() != Override->MoonCount)
			{
				Families->GeneratePlanetMoonsList(PlanetGenerator, MoonGenerator,
					Data->PlanetModel, Data->PlanetModel->Radius, FMath::Clamp(Override->MoonCount, 0, 10), PlanetIndex);
				bEditedMoonHierarchy = true;
			}
		}
		if (bEditedMoonHierarchy) ApplyPreviewBodyEditOverridesToModels(GeneratedWorldModel, StarIndex, *FamilyModel, Address);
		UPlanetarySystemGenerator::EnforcePlanetSurfaceClearance(*FamilyModel);
		double Envelope = StarModel->RadiusKM * 1.0e5 * 1.35;
		double PreviousOrbit = 0.0;
		double PreviousEnvelope = 0.0;
		for (int32 PlanetIndex = 0; PlanetIndex < FamilyModel->PlanetsList.Num(); ++PlanetIndex)
		{
			const TSharedPtr<FPlanetData>& Data = FamilyModel->PlanetsList[PlanetIndex];
			if (!Data || !Data->PlanetModel) return false;
			const TSharedPtr<FPlanetModel>& Model = Data->PlanetModel;
			const FString PlanetAddress = FString::Printf(TEXT("%s/P%d"), *StarAddress, PlanetIndex);
			Model->SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(Model->SurfaceSeed, Seed, PlanetAddress);
			UPlanetarySystemGenerator::EnforceSafeMoonOrbitSpacing(*Model);
			const double PlanetEnvelope = Model->Radius * EarthRadiusCm * 2.5;
			double OrbitRadius = FMath::Max(Model->OrbitDistance * AU,
				StarModel->RadiusKM * 1.0e5 * 1.35 + PlanetEnvelope);
			if (PreviousOrbit > 0.0) OrbitRadius = FMath::Max(OrbitRadius,
				PreviousOrbit + PreviousEnvelope + PlanetEnvelope + FMath::Max(AU * 0.01, StarModel->RadiusKM * 1.0e5 * 0.08));
			PreviousOrbit = OrbitRadius;
			PreviousEnvelope = PlanetEnvelope;
			Model->OrbitDistance = Data->OrbitRadius = OrbitRadius / AU;
			Layout.PlanetOrbitRadiiCm.Add(OrbitRadius);
			double MoonEnvelope = Model->Radius * EarthRadiusCm * 1.1;
			for (int32 MoonIndex = 0; MoonIndex < Model->MoonsList.Num(); ++MoonIndex)
			{
				const TSharedPtr<FMoonData>& MoonData = Model->MoonsList[MoonIndex];
				if (!MoonData || !MoonData->MoonModel) return false;
				const FString MoonAddress = FString::Printf(TEXT("%s/M%d"), *PlanetAddress, MoonIndex);
				MoonData->MoonModel->SurfaceSeed = UGeneratedWorld::ResolveCanonicalSurfaceSeed(
					MoonData->MoonModel->SurfaceSeed, Seed, MoonAddress);
				const double MoonDistance = (1.0 + MoonData->OrbitRadius) * Model->Radius * EarthRadiusCm;
				const double MoonRadiusKm = MoonData->MoonModel->Radius * 6371.0;
				MoonEnvelope = FMath::Max(MoonEnvelope, MoonDistance + MoonRadiusKm * 1.0e5);
				MoonData->MoonModelData = *MoonData->MoonModel;
			}
			Layout.MoonEnvelopesCm.Add(MoonEnvelope);
			Model->MoonsListData = Model->GetMoonsData();
			Data->PlanetModelData = *Model;
			Envelope = FMath::Max(Envelope, OrbitRadius + FMath::Max(PlanetEnvelope, MoonEnvelope));
		}
		Layout.EnvelopeCm = Envelope * 1.08;
	}
	double Largest = 0.0;
	for (const FAPSContinuousPreviewStarLayout& Star : OutStars) Largest = FMath::Max(Largest, Star.EnvelopeCm);
	const double Ring = StarCount > 1 ? Largest * 1.18 / FMath::Max(FMath::Sin(UE_PI / StarCount), 0.25) : 0.0;
	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		const double Angle = UE_TWO_PI * Index / StarCount;
		OutStars[Index].OffsetCm = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Ring;
	}
	OutRadiusCm = (Ring + Largest) * 1.08;
	return true;
}

AStarSystem* AAstroGenerator::MaterializeContinuousPreviewSystem(const int32 InstanceIndex)
{
	if (!UsesContinuousPreviewFrame() || !IsValid(GeneratedStarCluster) || !IsValid(GeneratedHomeStarSystem))
		return nullptr;
	if (InstanceIndex == PendingHomeClusterInstanceIndex) return GeneratedHomeStarSystem;
	if (const TObjectPtr<AStarSystem>* Existing = ContinuousMaterializedSystems.Find(InstanceIndex);
		Existing && IsValid(*Existing))
	{
		ContinuousSystemRecency.Remove(InstanceIndex);
		ContinuousSystemRecency.Add(InstanceIndex);
		return Existing->Get();
	}
	const FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystem(InstanceIndex);
	if (!Record || !Record->StableId.IsValid() || Record->SystemModel.AmountOfStars < 1
		|| Record->SystemModel.AmountOfStars > 6 || Record->SystemModel.PotentialPlanetCount < 0
		|| Record->SystemModel.PotentialPlanetCount > 128) return nullptr;
	UWorld* World = GetWorld();
	if (!World || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetarySystemClass
		|| !BP_PlanetClass || !BP_MoonClass || !BP_PlanetOrbitClass) return nullptr;

	const double Started = FPlatformTime::Seconds();
	const int32 Seed = Record->SystemModel.GenerationSeed;
	const FString Address = TEXT("SYS-") + Record->StableId.ToString(EGuidFormats::Digits);
	UStarGenerator* Stars = NewObject<UStarGenerator>(this);
	UPlanetarySystemGenerator* Families = NewObject<UPlanetarySystemGenerator>(this);
	TArray<FAPSContinuousPreviewStarLayout> Layouts;
	double SystemRadiusCm = 0.0;
	if (!BuildContinuousPreviewSystemLayout(*Record, Stars, Families, Layouts, SystemRadiusCm)) return nullptr;
	// Local physics and sampling remain precise. Only the shared observer resolves
	// this bubble's immutable canonical address; no catalog transform is replaced.
	const FVector LocalOrigin = GeneratedHomeStarSystem->GetActorLocation();
	AStarSystem* System = World->SpawnActor<AStarSystem>(BP_StarSystemClass, LocalOrigin, FRotator::ZeroRotator);
	if (!System) return nullptr;
	System->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	bool bCommitted = false;
	ON_SCOPE_EXIT { if (!bCommitted && IsValid(System)) DestroyActorTree(System); };
	StarSystemGenerator->ApplyModel(System, MakeShared<FStarSystemModel>(Record->SystemModel));
	constexpr double EarthRadiusCm = 6371.0 * 100000.0;
	const int32 StarCount = Record->SystemModel.AmountOfStars;
	for (int32 StarIndex = 0; StarIndex < StarCount; ++StarIndex)
	{
		const FString StarAddress = FString::Printf(TEXT("%s/S%d"), *Address, StarIndex);
		const FAPSContinuousPreviewStarLayout& Layout = Layouts[StarIndex];
		const TSharedPtr<FStarModel>& StarModel = Layout.StarModel;
		const TSharedPtr<FPlanetarySystemModel>& FamilyModel = Layout.FamilyModel;
		AStar* Star = World->SpawnActor<AStar>(BP_StarClass, LocalOrigin, FRotator::ZeroRotator);
		if (!Star) return nullptr;
		Star->AttachToActor(System, FAttachmentTransformRules::KeepWorldTransform);
		APlanetarySystem* Family = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass, LocalOrigin, FRotator::ZeroRotator);
		if (!Family) return nullptr;
		Family->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
		Stars->ApplyModel(Star, StarModel);
		PreviewResolvedStarModels.Add(StarAddress, MakeShared<FStarModel>(*StarModel));
		Families->ApplyModel(Family, FamilyModel);
		Star->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
		Family->SetActorScale3D(FVector::OneVector);
		Star->StarRadiusKM = FMath::RoundToInt(StarModel->RadiusKM);
		Star->SetPlanetarySystem(Family);
		Family->SetStar(Star);
		System->AddNewStar(Star);
		if (StarIndex == 0) System->MainStar = Star;
		Star->FullSpectralName = Star->GenerateFullSpectralName();
		Star->AstroName = APSGeneratedBodyIdentity::Name(Seed, StarAddress, Star->FullSpectralName.ToString());
		Family->SetStarFullSpectralName(Star->FullSpectralName);
		Stars->ApplySpectralMaterial(Star, StarModel);
		for (int32 PlanetIndex = 0; PlanetIndex < FamilyModel->PlanetsList.Num(); ++PlanetIndex)
		{
			const TSharedPtr<FPlanetData>& Data = FamilyModel->PlanetsList[PlanetIndex];
			if (!Data || !Data->PlanetModel) return nullptr;
			const TSharedPtr<FPlanetModel>& Model = Data->PlanetModel;
			const FString PlanetAddress = FString::Printf(TEXT("%s/P%d"), *StarAddress, PlanetIndex);
			const double OrbitRadius = Layout.PlanetOrbitRadiiCm[PlanetIndex];
			APlanetOrbit* Orbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbitClass, LocalOrigin, FRotator::ZeroRotator);
			if (!Orbit) return nullptr;
			Orbit->AttachToActor(Family, FAttachmentTransformRules::KeepWorldTransform);
			FRandomStream Orientation = APSGeneratedBodyIdentity::Stream(Seed, PlanetAddress, TEXT("orbit"));
			Orbit->SetActorRotation(FRotator(Orientation.FRandRange(-8.0f, 8.0f), Orientation.FRandRange(0.0f, 360.0f), 0.0));
			const FVector Center = LocalOrigin + Orbit->GetActorQuat().GetAxisX() * OrbitRadius;
			APlanet* Planet = World->SpawnActor<APlanet>(BP_PlanetClass, Center, FRotator::ZeroRotator);
			if (!Planet) return nullptr;
			Planet->AttachToActor(Orbit, FAttachmentTransformRules::KeepWorldTransform);
			Planet->bStreamWorldScapeSurface = true;
			Planet->bGenerateByDefault = false;
			PlanetGenerator->ApplyModel(Planet, Model);
			Planet->AstroName = APSGeneratedBodyIdentity::Name(Seed, PlanetAddress, TEXT("Planet"));
			Star->AddPlanet(Planet);
			Planet->SetParentStar(Star);
			Planet->SetActorScale3D(FVector(Model->Radius * 12742000.0));
			Planet->PlanetRadiusKM = FMath::RoundToInt(Model->Radius * 6371.0);
			Planet->SetOrbitDistance(Model->OrbitDistance);
			Family->PlanetsActorsList.Add(Planet);
			Family->PlanetOrbitsList.Add(Orbit);
			Orbit->Planet = Planet;
			const double MoonEnvelope = Layout.MoonEnvelopesCm[PlanetIndex];
			for (int32 MoonIndex = 0; MoonIndex < Model->MoonsList.Num(); ++MoonIndex)
			{
				const TSharedPtr<FMoonData>& MoonData = Model->MoonsList[MoonIndex];
				if (!MoonData || !MoonData->MoonModel) return nullptr;
				const FString MoonAddress = FString::Printf(TEXT("%s/M%d"), *PlanetAddress, MoonIndex);
				APlanetOrbit* MoonOrbit = World->SpawnActor<APlanetOrbit>(BP_PlanetOrbitClass, Center, FRotator::ZeroRotator);
				if (!MoonOrbit) return nullptr;
				MoonOrbit->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);
				FRandomStream MoonOrientation = APSGeneratedBodyIdentity::Stream(Seed, MoonAddress, TEXT("orbit"));
				MoonOrbit->SetActorRotation(FRotator(MoonOrientation.FRandRange(-8.0f, 8.0f),
					MoonOrientation.FRandRange(0.0f, 360.0f), 0.0));
				const double MoonDistance = (1.0 + MoonData->OrbitRadius) * Model->Radius * EarthRadiusCm;
				AMoon* Moon = World->SpawnActor<AMoon>(BP_MoonClass,
					Center + MoonOrbit->GetActorQuat().GetAxisX() * MoonDistance, FRotator::ZeroRotator);
				if (!Moon) return nullptr;
				Moon->AttachToActor(MoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
				Moon->bStreamWorldScapeSurface = true;
				Moon->bGenerateByDefault = false;
				MoonGenerator->ApplyModel(Moon, MoonData->MoonModel);
				MoonGenerator->ConnectMoonWithPlanet(Moon, Planet);
				Moon->AstroName = APSGeneratedBodyIdentity::Name(Seed, MoonAddress, TEXT("Moon"));
				Moon->SetParentPlanet(Planet);
				Planet->AddMoon(Moon);
				Planet->MoonOrbitsList.Add(MoonOrbit);
				Moon->RadiusKM = MoonData->MoonModel->Radius * 6371.0;
				Moon->PlanetRadiusKM = FMath::RoundToInt(Moon->RadiusKM);
				Moon->SetActorScale3D(FVector(MoonData->MoonModel->Radius * 12742000.0));
				MoonData->MoonModelData = *MoonData->MoonModel;
				if (APlanetarySurfaceGenerator* Environment = Moon->EnsurePlanetaryEnvironmentGenerator())
					Environment->InitAtmoScape(World, Moon->RadiusKM, Moon);
			}
			Model->MoonsListData = Model->GetMoonsData();
			Data->PlanetModelData = *Model;
			Planet->PlanetData.PlanetModelData = *Model;
			Planet->PlanetData.OrbitRadius = Model->OrbitDistance;
			Planet->PlanetaryZone->SetSphereRadius(MoonEnvelope / Planet->GetActorScale3D().X);
			Planet->AffectionRadiusKM = MoonEnvelope / 1.0e5;
			if (APlanetarySurfaceGenerator* Environment = Planet->EnsurePlanetaryEnvironmentGenerator())
			{
				Environment->InitEnviroment(Planet, World);
				if (Environment->PlanetAtmosphere) Environment->PlanetAtmosphere->LightSource = Star;
			}
			// Replay atmosphere controls after the generated shell exists. Radii and
			// moon counts were already restored before layout and envelope calculation.
			ApplyPreviewBodyEditOverrideByKey(GeneratedWorldModel, PlanetAddress, Planet);
			for (int32 MoonIndex = 0; MoonIndex < Planet->Moons.Num(); ++MoonIndex)
				ApplyPreviewBodyEditOverrideByKey(GeneratedWorldModel,
					FString::Printf(TEXT("%s/M%d"), *PlanetAddress, MoonIndex), Planet->Moons[MoonIndex]);
		}
		Star->StarAffectionZoneRadius = Layout.EnvelopeCm;
		Star->PlanetarySystemZone->SetSphereRadius(Star->StarAffectionZoneRadius / Star->GetActorScale3D().X);
	}
	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		System->GetStars()[Index]->SetActorLocation(LocalOrigin + Layouts[Index].OffsetCm);
	}
	System->StarSystemRadius = SystemRadiusCm;
	System->StarSystemZone->SetSphereRadius(System->StarSystemRadius);
	System->StarSystemZone->SetHiddenInGame(true, false);
	System->StarSystemZone->SetVisibility(false, false);
	System->SetActorTickEnabled(false);
	TArray<AActor*> Descendants;
	System->GetAttachedActors(Descendants, true, true);
	for (AActor* Child : Descendants)
	{
		Child->SetActorTickEnabled(false);
		if (Child->IsA<AStar>() || Child->IsA<APlanetaryBody>()) ContinuousPreviewBodies.AddUnique(Child);
		if (APlanetaryBody* Body = Cast<APlanetaryBody>(Child))
		{
			const FString BodyKey = GetPreviewBodyStableKey(Body);
			if (const FQuat* Rotation = ContinuousRetiredBodyRotations.Find(BodyKey))
			{
				PreviewPlanetPresentationRotations.Add(Body, *Rotation);
				ContinuousRetiredBodyRotations.Remove(BodyKey);
			}
		}
	}
	ContinuousMaterializedSystems.Add(InstanceIndex, System);
	ContinuousSystemRecency.Remove(InstanceIndex);
	ContinuousSystemRecency.Add(InstanceIndex);
	bCommitted = true;
	// The immutable record and source HISM are untouched, including materialization
	// flags. This is a disposable actor cache of that record, not a catalog edit.
	UE_LOG(LogTemp, Log, TEXT("[APS.Preview.Cluster] Expanded %s: stars=%d planets=%d in %.3fs"),
		*Address, StarCount, Record->SystemModel.PotentialPlanetCount, FPlatformTime::Seconds() - Started);
	return System;
}
