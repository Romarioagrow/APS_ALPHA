#include "APSGameplayNativeStars.h"

#include "APSStellarVisualSubsystem.h"

#include "APSStellarMaterialContract.h"
#include "APSStellarOpticalSupport.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "MaterialShared.h"
#include "LocalVertexFactory.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(APSGameplayStars);

namespace
{
// Bounded view policy, independent of catalog size. Prewarming never hides a point.
constexpr int32 PairLimit = 64;
constexpr int32 BindBudgetPerFrame = 2;
constexpr double PreparePixels = 1.0;
constexpr double RetainPixels = 0.65;
constexpr double ResolvePixels = 3.0;
constexpr double KeepResolvedPixels = 2.5;

const FTransform* CurrentBase(const FAPSGameplayStellarKey& Key)
{
	const UHierarchicalInstancedStaticMeshComponent* Source = Key.Source.Get();
	if (!IsValid(Source)) return nullptr;
	if (const AGalaxy* Galaxy = Cast<AGalaxy>(Source->GetOwner()))
		return Galaxy->RenderedProxyBaseTransforms.IsValidIndex(Key.Index)
			? &Galaxy->RenderedProxyBaseTransforms[Key.Index] : nullptr;
	if (const AStarCluster* Cluster = Cast<AStarCluster>(Source->GetOwner()))
		return Cluster->SystemProxyBaseTransforms.IsValidIndex(Key.Index)
			? &Cluster->SystemProxyBaseTransforms[Key.Index] : nullptr;
	return nullptr;
}

bool GeometryCurrent(const AAstroGenerator* Generator, const FAPSGameplayNativeDemand& Demand)
{
	const FTransform* Base = CurrentBase(Demand.Key);
	return Generator->IsGameplayStellarKeyCurrent(Demand.Key) && Base
		&& Base->Equals(Demand.BaseTransform, 0.0)
		&& Demand.SourceMesh.IsValid()
		&& Demand.Key.Source->GetStaticMesh() == Demand.SourceMesh.Get();
}

void HidePair(FAPSGameplayNativePair& Pair)
{
	for (UStaticMeshComponent* Mesh : {Pair.Photosphere.Get(), Pair.Corona.Get()})
	{
		if (!IsValid(Mesh)) continue;
		Mesh->SetVisibility(false, false);
		Mesh->SetHiddenInGame(true, false);
	}
}

bool MaterialReady(UMaterialInterface* Material, UWorld* World, const TCHAR* BasePath)
{
	if (!World || !APSStellarMaterialContract::HasExactBase(Material, BasePath)) return false;
	const FMaterialResource* Resource = Material->GetMaterialResource(World->GetFeatureLevel());
	const FMaterialShaderMap* ShaderMap = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
	return Resource && Resource->IsCompilationFinished() && Resource->IsGameThreadShaderMapComplete()
		&& ShaderMap && ShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
}

bool PairReady(const FAPSGameplayNativePair& Pair, UWorld* World)
{
	UStaticMeshComponent* Surface = Pair.Photosphere.Get();
	UStaticMeshComponent* Corona = Pair.Corona.Get();
	return Pair.bBound && IsValid(Surface) && IsValid(Corona)
		&& Surface->IsRegistered() && Corona->IsRegistered()
		&& Surface->IsRenderStateCreated() && Corona->IsRenderStateCreated()
		&& IsValid(Surface->GetStaticMesh()) && Surface->GetStaticMesh()->GetRenderData()
		&& MaterialReady(Surface->GetMaterial(0), World, APSStellarMaterialContract::ActorBaseObjectPath)
		&& MaterialReady(Corona->GetMaterial(0), World, APSStellarMaterialContract::CoronaBaseObjectPath)
		&& GFrameCounter > Pair.BoundFrame;
}

void RestorePoint(AAstroGenerator* Generator, FAPSGameplayNativePair& Pair,
	const FVector& Camera, const double PixelTangent,
	TSet<UHierarchicalInstancedStaticMeshComponent*>& DirtySources)
{
	if (!Pair.bOwnsPoint) return;
	Pair.bOwnsPoint = false;
	// Never restore a saved mutable scale, a new catalog slot or an external zero.
	if (!IsValid(Generator) || !GeometryCurrent(Generator, Pair.Demand)
		|| Generator->GetGameplayStellarSuppression(Pair.Demand.Key) != 0) return;
	UHierarchicalInstancedStaticMeshComponent* Source = Pair.Demand.Key.Source.Get();
	FTransform Current;
	if (!Source->GetInstanceTransform(Pair.Demand.Key.Index, Current, false)
		|| !Current.Equals(Pair.LastPublishedPoint, 0.0)) return;
	const FTransform& Base = *CurrentBase(Pair.Demand.Key);
	const double ComponentScale = Source->GetComponentScale().GetAbsMax();
	const double Radius = ComponentScale > 0.0 ? Pair.Demand.PhysicalRadiusCm / ComponentScale : 0.0;
	const double BaseRadius = Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax()
		* Base.GetScale3D().GetAbsMax();
	if (!FMath::IsFinite(Radius) || Radius <= 0.0 || BaseRadius <= 0.0 || PixelTangent <= 0.0) return;
	const FVector LocalCamera = Source->GetComponentTransform().InverseTransformPosition(Camera);
	const double PixelRadius = FVector::Distance(Base.GetLocation(), LocalCamera) * PixelTangent;
	const auto Profile = APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData,
		Source->NumCustomDataFloats, Pair.Demand.Key.Index);
	const double Carrier = APSStellarOpticalSupport::CarrierRadius(Radius, PixelRadius, Profile);
	FTransform Restored = Base;
	Restored.SetScale3D(Base.GetScale3D() * (Carrier / BaseRadius));
	APSStellarOpticalSupport::Publish(Source, Pair.Demand.Key.Index,
		APSStellarOpticalSupport::CoreScale(APSStellarOpticalSupport::CoreRadius(Radius, PixelRadius), Carrier),
		APSStellarOpticalSupport::ResolvedRayStrength(Profile, Radius, PixelRadius));
	Source->UpdateInstanceTransform(Pair.Demand.Key.Index, Restored, false, false, true);
	DirtySources.Add(Source);
}

void FlushSources(const TSet<UHierarchicalInstancedStaticMeshComponent*>& Sources)
{
	for (UHierarchicalInstancedStaticMeshComponent* Source : Sources)
	{
		// UE coalesces concurrent async changes; never synchronously rebuild the
		// complete catalog tree for a bounded point/pair handoff.
		Source->BuildTreeIfOutdated(true, false);
		Source->MarkRenderStateDirty();
	}
}

bool AllocatePair(AAstroGenerator* Generator, FAPSGameplayNativePair& Pair,
	UStaticMeshComponent* Template)
{
	if (!IsValid(Template) || !IsValid(Template->GetStaticMesh())) return false;
	if (Template->GetStaticMesh()->GetBounds().BoxExtent.GetMax() <= 0.0) return false;
	UMaterial* SurfaceBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::ActorBaseObjectPath);
	UMaterial* CoronaBase = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::CoronaBaseObjectPath);
	if (!MaterialReady(SurfaceBase, Generator->GetWorld(), APSStellarMaterialContract::ActorBaseObjectPath)
		|| !MaterialReady(CoronaBase, Generator->GetWorld(), APSStellarMaterialContract::CoronaBaseObjectPath)) return false;
	if (UStaticMeshComponent* OldCorona = Pair.Corona.Get()) OldCorona->DestroyComponent();
	if (UStaticMeshComponent* OldSurface = Pair.Photosphere.Get()) OldSurface->DestroyComponent();
	const auto CreateMesh = [Generator, Template](const bool bCorona)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Generator, NAME_None, RF_Transient);
		Mesh->SetupAttachment(Generator->GetRootComponent());
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bDisallowNanite = bCorona || Template->bDisallowNanite;
		Mesh->SetForceDisableNanite(bCorona || Template->bForceDisableNanite);
		Mesh->SetStaticMesh(Template->GetStaticMesh());
		Mesh->SetForcedLodModel(1);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(false);
		Mesh->bAffectDynamicIndirectLighting = false;
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(bCorona ? 2 : Template->TranslucencySortPriority);
		Mesh->SetVisibility(false, false);
		Mesh->SetHiddenInGame(true, false);
		return Mesh;
	};
	Pair.Photosphere = CreateMesh(false);
	Pair.Corona = CreateMesh(true);
	Pair.Photosphere->SetMaterial(0, UMaterialInstanceDynamic::Create(SurfaceBase, Generator));
	Pair.Corona->SetMaterial(0, UMaterialInstanceDynamic::Create(CoronaBase, Generator));
	Pair.Photosphere->SetAbsolute(true, true, true);
	Pair.Corona->SetupAttachment(Pair.Photosphere.Get());
	Generator->AddInstanceComponent(Pair.Photosphere.Get());
	Generator->AddInstanceComponent(Pair.Corona.Get());
	Pair.Photosphere->RegisterComponent();
	Pair.Corona->RegisterComponent();
	return true;
}

bool BindPair(AAstroGenerator* Generator, FAPSGameplayNativePair& Pair)
{
	UStarGenerator* Stars = Generator->GetGameplayStellarAppearanceGenerator();
	UStaticMeshComponent* Surface = Pair.Photosphere.Get();
	UStaticMeshComponent* Corona = Pair.Corona.Get();
	if (!IsValid(Stars) || !IsValid(Surface) || !IsValid(Corona)) return false;
	UMaterialInstanceDynamic* SurfaceMID = Cast<UMaterialInstanceDynamic>(Surface->GetMaterial(0));
	UMaterialInstanceDynamic* CoronaMID = Cast<UMaterialInstanceDynamic>(Corona->GetMaterial(0));
	if (!MaterialReady(SurfaceMID, Generator->GetWorld(), APSStellarMaterialContract::ActorBaseObjectPath)
		|| !MaterialReady(CoronaMID, Generator->GetWorld(), APSStellarMaterialContract::CoronaBaseObjectPath)) return false;
	// Both directions of pool reuse restore all base defaults before applying identity.
	SurfaceMID->ClearParameterValues();
	CoronaMID->ClearParameterValues();
	FAPSStellarMaterialParameters Parameters;
	EStellarType Type = EStellarType::Unknown;
	AActor* Owner = Pair.Demand.Key.Source->GetOwner();
	if (const AStarCluster* Cluster = Cast<AStarCluster>(Owner))
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Pair.Demand.Key.Index);
		if (!Record || Record->StableId != Pair.Demand.Key.StableId) return false;
		Parameters = Stars->ApplySpectralMaterialParameters(SurfaceMID, MakeShared<FStarModel>(Record->PrimaryStarModel));
		Type = Record->PrimaryStarModel.StellarType;
	}
	else if (const AGalaxy* Galaxy = Cast<AGalaxy>(Owner))
	{
		FGalaxyCatalogStarRecord Record;
		if (!Galaxy->GetRenderedCatalogRecord(Pair.Demand.Key.Index, Record)
			|| Record.StableId != Pair.Demand.Key.StableId) return false;
		// Catalog visual preset: no generated physical model and no POINTS compensation.
		Parameters.Color = UStarGenerator::GetStarColor(Record.SpectralClass, Record.SpectralSubclass);
		Parameters.Emission = static_cast<float>(Stars->CalculateEmission(static_cast<float>(
			APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass) * 25.0)));
		Parameters.SurfaceSeed = static_cast<float>(Record.GenerationSeed & 0xffff) / 65535.0f;
		UMaterial* Base = SurfaceMID->GetBaseMaterial();
		for (const TCHAR* Name : {TEXT("SurfaceVariation"), TEXT("GranulationStrength"),
			TEXT("SpotStrength"), TEXT("CoronaStrength"), TEXT("StellarArchetype")})
		{
			float Value = 0.0f;
			if (!Base->GetScalarParameterValue(FMaterialParameterInfo(Name), Value)) return false;
			SurfaceMID->SetScalarParameterValue(Name, Value);
		}
		SurfaceMID->SetVectorParameterValue(TEXT("Color"), Parameters.Color);
		SurfaceMID->SetScalarParameterValue(TEXT("Multiplier"), Parameters.Emission);
		SurfaceMID->SetScalarParameterValue(TEXT("SurfaceSeed"), Parameters.SurfaceSeed);
	}
	else return false;
	AStar::ConfigureStellarPresentationComponents(Surface, Corona, CoronaMID, nullptr,
		Parameters.Color, Parameters.Emission, Parameters.SurfaceSeed, Type);
	Pair.bShowCorona = Type != EStellarType::BlackHole;
	Pair.BoundMutationSerial = Generator->GetCanonicalStellarProjectionDescriptor().TransformMutationSerial;
	Pair.BoundFrame = GFrameCounter;
	HidePair(Pair);
	return true;
}

void PresentPair(FAPSGameplayNativePair& Pair)
{
	UHierarchicalInstancedStaticMeshComponent* Source = Pair.Demand.Key.Source.Get();
	UStaticMeshComponent* Surface = Pair.Photosphere.Get();
	const FTransform SourceFrame = Source->GetComponentTransform();
	const FTransform& Base = Pair.Demand.BaseTransform;
	const FVector Center = SourceFrame.TransformPosition(Base.GetLocation());
	const double Radius = Pair.Demand.PhysicalRadiusCm;
	const FBoxSphereBounds AssetBounds = Surface->GetStaticMesh()->GetBounds();
	const double Scale = Radius / AssetBounds.BoxExtent.GetMax();
	const FQuat Rotation = SourceFrame.TransformRotation(Base.GetRotation());
	Surface->SetWorldTransform(FTransform(Rotation,
		Center - Rotation.RotateVector(AssetBounds.Origin * Scale), FVector(Scale)),
		false, nullptr, ETeleportType::TeleportPhysics);
	if (Surface->GetComponentScale() != FVector(Scale))
		Surface->UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
	Surface->UpdateBounds();
	Surface->SetHiddenInGame(false, false);
	Surface->SetVisibility(true, false);
	Pair.Corona->SetHiddenInGame(!Pair.bShowCorona, false);
	Pair.Corona->SetVisibility(Pair.bShowCorona, false);
}
}

double APSGameplayNativeStars::PhysicalRadiusCm(const FAPSGameplayStellarKey& Key)
{
	const UHierarchicalInstancedStaticMeshComponent* Source = Key.Source.Get();
	if (!IsValid(Source)) return 0.0;
	double RadiusSolar = 0.0;
	if (const AStarCluster* Cluster = Cast<AStarCluster>(Source->GetOwner()))
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Key.Index);
		if (Record && Record->StableId == Key.StableId) RadiusSolar = Record->PrimaryStarModel.Radius;
	}
	else if (const AGalaxy* Galaxy = Cast<AGalaxy>(Source->GetOwner()))
	{
		FGalaxyCatalogStarRecord Record;
		if (Galaxy->GetRenderedCatalogRecord(Key.Index, Record) && Record.StableId == Key.StableId)
			RadiusSolar = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass);
	}
	return FMath::IsFinite(RadiusSolar) && RadiusSolar > 0.0
		? RadiusSolar * APSCanonicalStellarProjection::SolarRadiusCm : 0.0;
}

FAPSGameplayStellarKey AAstroGenerator::MakeGameplayStellarKey(
	UHierarchicalInstancedStaticMeshComponent* Source, const int32 Index) const
{
	FAPSGameplayStellarKey Key;
	if (!IsValid(Source) || Index < 0 || Index >= Source->GetInstanceCount()) return Key;
	Key.Source = Source;
	Key.Index = Index;
	Key.InstanceCount = Source->GetInstanceCount();
	Key.BuildSerial = CanonicalStellarProjection.ProxyBuildSerial;
	if (const AGalaxy* Galaxy = Cast<AGalaxy>(Source->GetOwner()))
	{
		int64 CatalogIndex = INDEX_NONE;
		if (Galaxy->StarMeshInstances == Source && Galaxy->GetRenderedCatalogIndex(Index, CatalogIndex))
			Key.StableId = Galaxy->StarCatalog.MakeStableStarId(CatalogIndex);
	}
	else if (const AStarCluster* Cluster = Cast<AStarCluster>(Source->GetOwner()))
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Index);
		if (Cluster->StarMeshInstances == Source && Record) Key.StableId = Record->StableId;
	}
	return Key;
}

bool AAstroGenerator::IsGameplayStellarKeyCurrent(const FAPSGameplayStellarKey& Key) const
{
	return Key.StableId.IsValid() && Key.Source.IsValid()
		&& Key == MakeGameplayStellarKey(Key.Source.Get(), Key.Index);
}

uint8 AAstroGenerator::GetGameplayStellarSuppression(const FAPSGameplayStellarKey& Key) const
{
	if (!IsGameplayStellarKeyCurrent(Key)) return static_cast<uint8>(EAPSGameplayStellarSuppression::UnclassifiedExternal);
	uint8 Mask = GameplayStellarSuppression.FindRef(Key);
	if (const AStarCluster* Cluster = Cast<AStarCluster>(Key.Source->GetOwner()))
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Key.Index);
		if (Record && (Record->bMaterialized || Record->MaterializedSystem.IsValid()))
			Mask |= static_cast<uint8>(EAPSGameplayStellarSuppression::Materialized);
	}
	return Mask;
}

void AAstroGenerator::SetGameplayStellarSuppression(const FAPSGameplayStellarKey& Key,
	const EAPSGameplayStellarSuppression Reason, const bool bSuppressed)
{
	if (!IsGameplayStellarKeyCurrent(Key)) return;
	uint8 Mask = GameplayStellarSuppression.FindRef(Key);
	if (bSuppressed) Mask |= static_cast<uint8>(Reason);
	else Mask &= ~static_cast<uint8>(Reason);
	if (Mask) GameplayStellarSuppression.Add(Key, Mask);
	else GameplayStellarSuppression.Remove(Key);
}

void UAPSStellarVisualSubsystem::ResetGameplayNativeStars()
{
	TSet<UHierarchicalInstancedStaticMeshComponent*> DirtySources;
	for (FAPSGameplayNativePair& Pair : GameplayNativePairs)
	{
		HidePair(Pair);
		RestorePoint(GameplayStellarGenerator.Get(), Pair, GameplayNativeCamera, GameplayNativePixelTangent, DirtySources);
		if (UStaticMeshComponent* Corona = Pair.Corona.Get()) Corona->DestroyComponent();
		if (UStaticMeshComponent* Surface = Pair.Photosphere.Get()) Surface->DestroyComponent();
	}
	FlushSources(DirtySources);
	GameplayNativePairs.Reset();
	GameplayNativeOwners.Reset();
	GameplayNativeDemand.Reset();
	GameplayNativePhysicalRadii.Reset();
	GameplayNativeSizedDistances.Reset();
	GameplayNativeDemandCandidates.Reset();
	bGameplayNativeDemandCandidatesValid = false;
	GameplayNativeResizePasses.Reset();
	GameplayNativeMutationSerial = 0;
	GameplayNativeUnknownMutationSerial = 0;
	GameplayNativeTopologyHash = 0;
	GameplayNativeOverflowResolved = 0;
	GameplayNativeLastPresentedCount = GameplayNativeLastDemandCount = INDEX_NONE;
	bGameplayNativeInitialized = false;
}

void UAPSStellarVisualSubsystem::BeginGameplayNativeStars(AAstroGenerator* Generator,
	const bool bRefreshDemand, const FVector& Camera, const double PixelTangent,
	const FQuat& ViewRotation, const double TanHalfHorizontal, const double TanHalfVertical)
{
	GameplayNativeViewRotation = ViewRotation;
	GameplayNativeTanHalfHorizontal = TanHalfHorizontal;
	GameplayNativeTanHalfVertical = TanHalfVertical;
	GameplayNativeCamera = Camera;
	GameplayNativePixelTangent = PixelTangent;
	const uint64 UnknownSerial = Generator->GetGameplayUnknownStellarMutationSerial();
	if (bGameplayNativeInitialized && UnknownSerial != GameplayNativeUnknownMutationSerial)
	{
		for (FAPSGameplayNativePair& Pair : GameplayNativePairs)
		{
			if (!Pair.bOwnsPoint) continue;
			Generator->SetGameplayStellarSuppression(Pair.Demand.Key,
				EAPSGameplayStellarSuppression::UnclassifiedExternal, true);
			HidePair(Pair);
		}
		UE_LOG(LogTemp, Warning, TEXT("[APS.Gameplay.NativeStars] Unclassified proxy mutation; unresolved owned points remain suppressed until catalog rebuild."));
	}
	GameplayNativeUnknownMutationSerial = UnknownSerial;
	GameplayNativeMutationSerial = Generator->GetCanonicalStellarProjectionDescriptor().TransformMutationSerial;
	bGameplayNativeInitialized = true;
	if (bRefreshDemand)
	{
		GameplayNativeDemandRotation = ViewRotation;
		GameplayNativeDemand.Reset();
		GameplayNativeOverflowResolved = 0;
	}
}

bool UAPSStellarVisualSubsystem::ObserveGameplayNativePoint(AAstroGenerator* Generator,
	const FAPSGameplayStellarKey& Key, const FTransform& BaseTransform,
	const FTransform& CurrentTransform, bool& bOwned)
{
	bOwned = false;
	if (Generator->GetGameplayStellarSuppression(Key) != 0) return false;
	if (const int32* PairIndex = GameplayNativeOwners.Find(Key))
	{
		const FAPSGameplayNativePair& Pair = GameplayNativePairs[*PairIndex];
		bOwned = Pair.bOwnsPoint && Pair.Demand.BaseTransform.Equals(BaseTransform, 0.0)
			&& GeometryCurrent(Generator, Pair.Demand)
			&& CurrentTransform.Equals(Pair.LastPublishedPoint, 0.0);
	}
	if (CurrentTransform.GetScale3D() == FVector::ZeroVector && !bOwned)
	{
		Generator->SetGameplayStellarSuppression(Key, EAPSGameplayStellarSuppression::UnclassifiedExternal, true);
		return false;
	}
	return true;
}

void UAPSStellarVisualSubsystem::CollectGameplayNativeDemand(const FAPSGameplayStellarKey& Key,
	const FTransform& BaseTransform, const double PhysicalRadiusCm, const double PixelRadius)
{
	const bool bRetained = GameplayNativeOwners.Contains(Key);
	if (!FMath::IsFinite(PixelRadius) || PixelRadius < (bRetained ? RetainPixels : PreparePixels)) return;
	const FVector Offset = Key.Source->GetComponentTransform().TransformPosition(BaseTransform.GetLocation())
		- GameplayNativeCamera;
	// Budget the camera's candidates, not the largest stars on the opposite side
	// of the sky. A 32-pixel guard keeps admission stable at the viewport edge.
	const double GuardedRadius = PhysicalRadiusCm * 1.32 + Offset.Size() * GameplayNativePixelTangent * 32.0;
	if (!APSPreviewVisibility::SphereIntersectsView(GameplayNativeViewRotation.UnrotateVector(Offset),
		GuardedRadius, GameplayNativeTanHalfHorizontal, GameplayNativeTanHalfVertical)) return;
	// Rotation-only selection reads immutable centres/cached radii. Read back the
	// mutable HISM slot only for an angularly relevant, on-screen candidate so an
	// external zero or materialized star can never acquire native ownership.
	AAstroGenerator* Generator = GameplayStellarGenerator.Get();
	FTransform CurrentTransform;
	bool bOwned = false;
	if (!IsValid(Generator) || !Key.Source->GetInstanceTransform(Key.Index, CurrentTransform, false)
		|| !ObserveGameplayNativePoint(Generator, Key, BaseTransform, CurrentTransform, bOwned)) return;
	FAPSGameplayNativeDemand Demand;
	Demand.Key = Key;
	Demand.BaseTransform = BaseTransform;
	Demand.SourceMesh = Key.Source->GetStaticMesh();
	Demand.PhysicalRadiusCm = PhysicalRadiusCm;
	Demand.PixelRadius = PixelRadius;
	if (GameplayNativeDemand.Num() < PairLimit) GameplayNativeDemand.Add(MoveTemp(Demand));
	else
	{
		int32 Smallest = 0;
		for (int32 Index = 1; Index < GameplayNativeDemand.Num(); ++Index)
			if (GameplayNativeDemand[Index].PixelRadius < GameplayNativeDemand[Smallest].PixelRadius) Smallest = Index;
		if (PixelRadius > GameplayNativeDemand[Smallest].PixelRadius)
		{
			GameplayNativeOverflowResolved += GameplayNativeDemand[Smallest].PixelRadius >= ResolvePixels;
			GameplayNativeDemand[Smallest] = MoveTemp(Demand);
		}
		else GameplayNativeOverflowResolved += PixelRadius >= ResolvePixels;
	}
}

void UAPSStellarVisualSubsystem::PresentGameplayNativeStars(AAstroGenerator* Generator)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayNativeStars);
	TSet<UHierarchicalInstancedStaticMeshComponent*> DirtySources;
	GameplayNativeDemand.RemoveAll([&](FAPSGameplayNativeDemand& Demand)
	{
		// A day sky hides the other stars; their pairs are released below (UpdateGameplayDaylightStars).
		if (bGameplayDaylightStarsHidden) return true;
		if (!GeometryCurrent(Generator, Demand) || Generator->GetGameplayStellarSuppression(Demand.Key) != 0) return true;
		UHierarchicalInstancedStaticMeshComponent* Source = Demand.Key.Source.Get();
		const FVector Offset = Source->GetComponentTransform().TransformPosition(Demand.BaseTransform.GetLocation())
			- GameplayNativeCamera;
		const double PixelWorldRadius = Offset.Size() * GameplayNativePixelTangent;
		const double GuardedRadius = Demand.PhysicalRadiusCm * 1.32 + PixelWorldRadius * 32.0;
		if (!APSPreviewVisibility::SphereIntersectsView(GameplayNativeViewRotation.UnrotateVector(Offset),
			GuardedRadius, GameplayNativeTanHalfHorizontal, GameplayNativeTanHalfVertical)) return true;
		Demand.PixelRadius = PixelWorldRadius > 0.0 ? Demand.PhysicalRadiusCm / PixelWorldRadius : 0.0;
		return !FMath::IsFinite(Demand.PixelRadius) || Demand.PixelRadius < RetainPixels;
	});
	GameplayNativeDemand.Sort([](const FAPSGameplayNativeDemand& A, const FAPSGameplayNativeDemand& B)
	{
		if (A.PixelRadius != B.PixelRadius) return A.PixelRadius > B.PixelRadius;
		return A.Key.Index < B.Key.Index;
	});
	for (FAPSGameplayNativePair& Pair : GameplayNativePairs)
	{
		if (!Pair.bAssigned) continue;
		const bool bDemanded = GameplayNativeDemand.ContainsByPredicate([&](const FAPSGameplayNativeDemand& Demand) { return Demand.Key == Pair.Demand.Key; });
		if (bDemanded && GeometryCurrent(Generator, Pair.Demand)) continue;
		HidePair(Pair);
		RestorePoint(Generator, Pair, GameplayNativeCamera, GameplayNativePixelTangent, DirtySources);
		GameplayNativeOwners.Remove(Pair.Demand.Key);
		Pair.bAssigned = false;
		Pair.bBound = false;
	}
	AStarSystem* Home = Generator->GetPreviewHomeSystem();
	UStaticMeshComponent* Template = IsValid(Home) && IsValid(Home->MainStar) ? Home->MainStar->StarMesh : nullptr;
	if (GameplayNativeBindFrame != GFrameCounter)
	{
		GameplayNativeBindFrame = GFrameCounter;
		GameplayNativeBindAttempts = 0;
	}
	int32& BindAttempts = GameplayNativeBindAttempts;
	int32 UnmetResolved = GameplayNativeOverflowResolved;
	int32 PresentedCount = 0;
	for (const FAPSGameplayNativeDemand& Demand : GameplayNativeDemand)
	{
		int32 PairIndex = INDEX_NONE;
		if (const int32* Existing = GameplayNativeOwners.Find(Demand.Key)) PairIndex = *Existing;
		else if (BindAttempts < BindBudgetPerFrame)
		{
			for (int32 Index = 0; Index < GameplayNativePairs.Num(); ++Index)
				if (!GameplayNativePairs[Index].bAssigned) { PairIndex = Index; break; }
			if (PairIndex == INDEX_NONE && GameplayNativePairs.Num() < PairLimit) PairIndex = GameplayNativePairs.AddDefaulted();
			if (PairIndex != INDEX_NONE)
			{
				FAPSGameplayNativePair& Pair = GameplayNativePairs[PairIndex];
				Pair.Demand = Demand;
				Pair.bAssigned = true;
				Pair.bBound = false;
				GameplayNativeOwners.Add(Demand.Key, PairIndex);
			}
		}
		if (PairIndex == INDEX_NONE) { UnmetResolved += Demand.PixelRadius >= ResolvePixels; continue; }
		FAPSGameplayNativePair& Pair = GameplayNativePairs[PairIndex];
		Pair.Demand = Demand;
		if (!Pair.bBound || Pair.BoundMutationSerial != GameplayNativeMutationSerial)
		{
			HidePair(Pair);
			RestorePoint(Generator, Pair, GameplayNativeCamera, GameplayNativePixelTangent, DirtySources);
			Pair.bBound = false;
			if (BindAttempts < BindBudgetPerFrame)
			{
				++BindAttempts;
				if ((!Pair.Photosphere.IsValid() || !Pair.Corona.IsValid()) && !AllocatePair(Generator, Pair, Template))
				{
					UnmetResolved += Demand.PixelRadius >= ResolvePixels;
					continue;
				}
				Pair.bBound = BindPair(Generator, Pair);
			}
		}
		const bool bResolve = Demand.PixelRadius >= (Pair.bOwnsPoint ? KeepResolvedPixels : ResolvePixels);
		if (!bResolve || !PairReady(Pair, GetWorld()))
		{
			HidePair(Pair);
			RestorePoint(Generator, Pair, GameplayNativeCamera, GameplayNativePixelTangent, DirtySources);
			UnmetResolved += bResolve;
			continue;
		}
		// Revalidate producer reasons even on an optics-cache hit and immediately before handoff.
		if (!GeometryCurrent(Generator, Demand) || Generator->GetGameplayStellarSuppression(Demand.Key) != 0) continue;
		UHierarchicalInstancedStaticMeshComponent* Source = Demand.Key.Source.Get();
		FTransform Point;
		if (!Source->GetInstanceTransform(Demand.Key.Index, Point, false))
		{
			HidePair(Pair);
			continue;
		}
		if ((Pair.bOwnsPoint && !Point.Equals(Pair.LastPublishedPoint, 0.0))
			|| (!Pair.bOwnsPoint && Point.GetScale3D() == FVector::ZeroVector))
		{
			HidePair(Pair);
			Pair.bOwnsPoint = false;
			if (Point.GetScale3D() == FVector::ZeroVector)
				Generator->SetGameplayStellarSuppression(Demand.Key, EAPSGameplayStellarSuppression::UnclassifiedExternal, true);
			continue;
		}
		PresentPair(Pair);
		++PresentedCount;
		if (!Pair.bOwnsPoint)
		{
			Point.SetScale3D(FVector::ZeroVector);
			Source->UpdateInstanceTransform(Demand.Key.Index, Point, false, false, true);
			Pair.LastPublishedPoint = Point;
			Pair.bOwnsPoint = true;
			DirtySources.Add(Source);
		}
	}
	FlushSources(DirtySources);
	if (PresentedCount != GameplayNativeLastPresentedCount
		|| GameplayNativeDemand.Num() != GameplayNativeLastDemandCount)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars] generator=%s demand=%d presented=%d pool=%d pendingResolved=%d bindAttempts=%d limit=%d; physical radii, visible-view selection"),
			*GetNameSafe(Generator), GameplayNativeDemand.Num(), PresentedCount, GameplayNativePairs.Num(),
			UnmetResolved, BindAttempts, PairLimit);
		GameplayNativeLastPresentedCount = PresentedCount;
		GameplayNativeLastDemandCount = GameplayNativeDemand.Num();
	}
	CSV_CUSTOM_STAT(APSGameplayStars, NativePairCount, GameplayNativePairs.Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(APSGameplayStars, NativeBindAttempts, BindAttempts, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(APSGameplayStars, NativeUnmetResolved, UnmetResolved, ECsvCustomStatOp::Set);
}
