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
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "MaterialShared.h"
#include "StaticMeshResources.h"
#include "LocalVertexFactory.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(APSGameplayStars);

namespace
{
// Bounded view policy, independent of catalog size. Prewarming never hides a point.
constexpr int32 PairLimit = 64;
// Rio 02.10: a star must not grow into a large glyph before its sphere is ready; bind faster.
constexpr int32 BindBudgetPerFrame = 6;
// Rio 02.10: the translucent point glyph must not stand in for a visible star ("delete that transparent material"):
// the full star (opaque photosphere and corona) takes over from aps.Stars.ResolvePixels of radius; the glyph stays only
// for points too small to show it. Pairs are prepared just before, so the handoff never waits.
TAutoConsoleVariable<float> CVarResolvePixels(TEXT("aps.Stars.ResolvePixels"), 1.0f,
	TEXT("Pixel radius at which a catalogue point becomes the full star (photosphere and corona)."));
double ResolvePixels() { return FMath::Max(static_cast<double>(CVarResolvePixels.GetValueOnGameThread()), 1.0); }
double KeepResolvedPixels() { return ResolvePixels() * 0.83; }
double PreparePixels() { return FMath::Max(ResolvePixels() * 0.6, 0.5); }
double RetainPixels() { return PreparePixels() * 0.65; }

// 02.10: no pair was ever presented in any log (presented=0 in 9k lines). Once a second, why the largest demands wait.
TAutoConsoleVariable<int32> CVarNativeDiag(TEXT("aps.Stars.NativeDiag"), 0,
	TEXT("1: log once a second the state of the four largest native star demands (pair, bind, render state, materials)."));
TAutoConsoleVariable<int32> CVarNativeStrictMaterial(TEXT("aps.Stars.NativeStrictMaterial"), 0,
	TEXT("1: a native star waits until its materials' shader maps are complete; 0: an existing shader map is enough ")
	TEXT("(the missing shaders then compile on demand once the star draws)."));

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
	if (CVarNativeStrictMaterial.GetValueOnGameThread() == 0)
	{
		return ShaderMap != nullptr;
	}
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

/** Gives an owned catalogue point (scale zero, LastPublishedPoint) back its glyph size for the current optics. */
void RestoreOwnedPoint(AAstroGenerator* Generator, const FAPSGameplayNativeDemand& Demand,
	const FTransform& LastPublishedPoint, const FVector& Camera, const double PixelTangent,
	TSet<UHierarchicalInstancedStaticMeshComponent*>& DirtySources)
{
	// Never restore a saved mutable scale, a new catalog slot or an external zero.
	if (!IsValid(Generator) || !GeometryCurrent(Generator, Demand)
		|| Generator->GetGameplayStellarSuppression(Demand.Key) != 0) return;
	UHierarchicalInstancedStaticMeshComponent* Source = Demand.Key.Source.Get();
	FTransform Current;
	if (!Source->GetInstanceTransform(Demand.Key.Index, Current, false)
		|| !Current.Equals(LastPublishedPoint, 0.0)) return;
	const FTransform& Base = *CurrentBase(Demand.Key);
	const double ComponentScale = Source->GetComponentScale().GetAbsMax();
	const double Radius = ComponentScale > 0.0 ? Demand.PhysicalRadiusCm / ComponentScale : 0.0;
	const double BaseRadius = Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax()
		* Base.GetScale3D().GetAbsMax();
	if (!FMath::IsFinite(Radius) || Radius <= 0.0 || BaseRadius <= 0.0 || PixelTangent <= 0.0) return;
	const FVector LocalCamera = Source->GetComponentTransform().InverseTransformPosition(Camera);
	const double PixelRadius = FVector::Distance(Base.GetLocation(), LocalCamera) * PixelTangent;
	const auto Profile = APSStellarOpticalSupport::Select(Source->PerInstanceSMCustomData,
		Source->NumCustomDataFloats, Demand.Key.Index);
	const double Carrier = APSStellarOpticalSupport::CarrierRadius(Radius, PixelRadius, Profile);
	FTransform Restored = Base;
	Restored.SetScale3D(Base.GetScale3D() * (Carrier / BaseRadius));
	APSStellarOpticalSupport::Publish(Source, Demand.Key.Index,
		APSStellarOpticalSupport::CoreScale(APSStellarOpticalSupport::CoreRadius(Radius, PixelRadius), Carrier),
		APSStellarOpticalSupport::ResolvedRayStrength(Profile, Radius, PixelRadius));
	Source->UpdateInstanceTransform(Demand.Key.Index, Restored, false, false, true);
	DirtySources.Add(Source);
}

void RestorePoint(AAstroGenerator* Generator, FAPSGameplayNativePair& Pair,
	const FVector& Camera, const double PixelTangent,
	TSet<UHierarchicalInstancedStaticMeshComponent*>& DirtySources)
{
	if (!Pair.bOwnsPoint) return;
	Pair.bOwnsPoint = false;
	RestoreOwnedPoint(Generator, Pair.Demand, Pair.LastPublishedPoint, Camera, PixelTangent, DirtySources);
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
		// Rio 03.10: x the galaxy POPULATION luminosity (R^2 of its size factor, 1 for the historic mix).
		Parameters.Emission = static_cast<float>(Stars->CalculateEmission(static_cast<float>(
			APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
			* FMath::Clamp(static_cast<double>(Record.RadiusScale) * Record.RadiusScale, 1.0e-4, 1.0e4) * 25.0)));
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

// Rio 03.10: "some stars unload when the camera turns: they are seen, then they turn into blurred spots. Remove those
// spots. One material that loses detail with distance, no other blurred-glow material; everything in frame sharp, nothing
// blinks, unloads or vanishes", and "no visible FPS drop, no freeze". The pairs above drew the 64 largest stars in view;
// every other resolved star (up to ~700 in a cluster view, 03.10 log) stayed its translucent glyph, a blurred disc, and a
// turn re-selected the 64. Now every resolved catalogue star, in all directions, is one instance of an instanced
// photosphere per catalogue: the star material's instance path (M_SpectralStarMat_SUN reads the catalogue's custom-data
// slots 0..5 and fades its own detail by footprint). One draw per catalogue and nothing chosen by the view.
TAutoConsoleVariable<int32> CVarNativeMode(TEXT("aps.Stars.NativeMode"), 1,
	TEXT("Catalogue stars of at least aps.Stars.ResolvePixels radius. 1 (Rio 03.10): every one of them, in all directions, ")
	TEXT("is an instance of an instanced photosphere (the star material's own detail fade); a turn changes nothing. ")
	TEXT("0: the old 64 view-selected sphere and corona pairs, every other resolved star a blurred glyph."));
TAutoConsoleVariable<int32> CVarResolvedLimit(TEXT("aps.Stars.ResolvedLimit"), 8192,
	TEXT("NativeMode 1: most catalogue stars drawn as instanced photospheres at once."));
TAutoConsoleVariable<int32> CVarResolvedAddsPerFrame(TEXT("aps.Stars.ResolvedAddsPerFrame"), 64,
	TEXT("NativeMode 1: most photospheres added in one frame, largest first (a first fill spreads over a few frames)."));
TAutoConsoleVariable<int32> CVarResolvedForcedLod(TEXT("aps.Stars.ResolvedForcedLod"), 1,
	TEXT("NativeMode 1: forced LOD of the instanced photospheres (1 the finest, as the pairs had; 0 per-instance GPU LOD). ")
	TEXT("Read when a photosphere component is made."));
// The catalogue's own sphere has 256 triangles (16 sides): its outline stays within half a pixel up to a ~25 px radius.
TAutoConsoleVariable<float> CVarResolvedSmoothPixels(TEXT("aps.Stars.ResolvedSmoothPixels"), 20.0f,
	TEXT("NativeMode 1: from this radius in pixels a star's photosphere instance uses the home star's smooth sphere (the old ")
	TEXT("pairs' mesh) instead of the catalogue's 256-triangle one; back below 80% of it. 0: always the catalogue sphere."));

bool InstancedMode() { return CVarNativeMode.GetValueOnGameThread() != 0; }

/** Tier 0: the catalogue's own sphere (most stars, a few pixels); tier 1: the smooth sphere (large ones). */
constexpr int32 ResolvedTiers = 2;

struct FResolvedSlot
{
	FAPSGameplayNativeDemand Demand;
	FTransform LastPublishedPoint{FTransform::Identity};
	float Data[6]{};
	int32 Instance{INDEX_NONE};
	int32 Tier{0};
	bool bOwnsPoint{false};
};

struct FResolvedView
{
	TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
	/** Instance index -> its star; a removal swaps the last instance into the hole (SetRemoveSwap). */
	TArray<FAPSGameplayStellarKey> Keys;
};

struct FResolvedState
{
	TMap<FAPSGameplayStellarKey, FResolvedSlot> Slots;
	TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, FResolvedView> Views[ResolvedTiers];
	uint64 ValidatedSerial{MAX_uint64};
	int32 LoggedCount{INDEX_NONE};
	double LoggedSeconds{-1.0e9};
};

TMap<TWeakObjectPtr<const UAPSStellarVisualSubsystem>, FResolvedState> GResolvedStates;
TMap<TWeakObjectPtr<const UAPSStellarVisualSubsystem>, int32> GNativeModes;

FResolvedState* FindResolved(const UAPSStellarVisualSubsystem* Subsystem)
{
	return GResolvedStates.Find(TWeakObjectPtr<const UAPSStellarVisualSubsystem>(Subsystem));
}

/** Slots 0..5 of the catalogue layout with the colour, emission and seed the pairs and the materialized star use, so a
 * star keeps its tint and surface pattern from instance to actor. */
bool MakeResolvedData(UStarGenerator* Stars, const FAPSGameplayStellarKey& Key, float (&Data)[6])
{
	const UHierarchicalInstancedStaticMeshComponent* Source = Key.Source.Get();
	if (!IsValid(Stars) || !IsValid(Source)) return false;
	FLinearColor Color = FLinearColor::White;
	double Emission = 0.0;
	float Seed = 0.0f;
	if (const AStarCluster* Cluster = Cast<AStarCluster>(Source->GetOwner()))
	{
		const FClusterStarSystemRecord* Record = Cluster->FindPotentialSystem(Key.Index);
		if (!Record || Record->StableId != Key.StableId) return false;
		const FStarModel& Model = Record->PrimaryStarModel;
		Color = UStarGenerator::GetStarColor(Model.SpectralClass, Model.SpectralSubclass);
		Emission = Stars->CalculateEmission(Model.Luminosity * 25.0f);
		// UStarGenerator::ApplySpectralMaterialParameters' SurfaceSeed.
		Seed = static_cast<float>(FMath::Frac(FMath::Abs(Model.SurfaceTemperature * 0.000173f + Model.Mass * 0.137f
			+ Model.Radius * 0.071f + Model.Luminosity * 0.019f)));
	}
	else if (const AGalaxy* Galaxy = Cast<AGalaxy>(Source->GetOwner()))
	{
		FGalaxyCatalogStarRecord Record;
		if (!Galaxy->GetRenderedCatalogRecord(Key.Index, Record) || Record.StableId != Key.StableId) return false;
		// BindPair's catalogue preset.
		Color = UStarGenerator::GetStarColor(Record.SpectralClass, Record.SpectralSubclass);
		Emission = Stars->CalculateEmission(static_cast<float>(
			APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass)
			* FMath::Clamp(static_cast<double>(Record.RadiusScale) * Record.RadiusScale, 1.0e-4, 1.0e4) * 25.0));
		Seed = static_cast<float>(Record.GenerationSeed & 0xffff) / 65535.0f;
	}
	else return false;
	const int32 Stride = Source->NumCustomDataFloats;
	const int32 MarkerSlot = Key.Index * Stride + 5;
	Data[0] = Color.R;
	Data[1] = Color.G;
	Data[2] = Color.B;
	Data[3] = static_cast<float>(Emission);
	Data[4] = Seed;
	Data[5] = Stride > 5 && Source->PerInstanceSMCustomData.IsValidIndex(MarkerSlot)
		? Source->PerInstanceSMCustomData[MarkerSlot] : 0.0f;
	return FMath::IsFinite(Data[3]) && Data[3] > 0.0f;
}

/** The home star's own sphere (the old pairs' mesh), else the engine sphere: the smooth tier's mesh. */
UStaticMesh* SmoothSphereMesh(const AAstroGenerator* Generator)
{
	const AStarSystem* Home = Generator ? Generator->GetPreviewHomeSystem() : nullptr;
	if (IsValid(Home) && IsValid(Home->MainStar) && IsValid(Home->MainStar->StarMesh)
		&& IsValid(Home->MainStar->StarMesh->GetStaticMesh()))
	{
		return Home->MainStar->StarMesh->GetStaticMesh();
	}
	static TWeakObjectPtr<UStaticMesh> EngineSphere;
	if (!EngineSphere.IsValid())
	{
		EngineSphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	}
	return EngineSphere.Get();
}

/** One instanced photosphere per catalogue and tier, in the catalogue component's own frame (origin shifts move it). */
UInstancedStaticMeshComponent* EnsureResolvedView(FResolvedView& View, UHierarchicalInstancedStaticMeshComponent* Source,
	UWorld* World, UStaticMesh* SphereMesh)
{
	if (UInstancedStaticMeshComponent* Existing = View.Mesh.Get(); IsValid(Existing)) return Existing;
	UMaterial* Surface = APSStellarMaterialContract::LoadCanonicalBase(APSStellarMaterialContract::ActorBaseObjectPath);
	AActor* Owner = Source->GetOwner();
	if (!IsValid(Owner) || !IsValid(SphereMesh)
		|| !MaterialReady(Surface, World, APSStellarMaterialContract::ActorBaseObjectPath)) return nullptr;
	UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(Owner, NAME_None, RF_Transient);
	Mesh->SetupAttachment(Source);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->bDisallowNanite = true;
	Mesh->SetForceDisableNanite(true);
	Mesh->SetStaticMesh(SphereMesh);
	Mesh->SetForcedLodModel(FMath::Clamp(CVarResolvedForcedLod.GetValueOnGameThread(), 0, SphereMesh->GetNumLODs()));
	for (int32 Slot = 0; Slot < FMath::Max(Mesh->GetNumMaterials(), 1); ++Slot) Mesh->SetMaterial(Slot, Surface);
	Mesh->SetNumCustomDataFloats(6);
	Mesh->SetRemoveSwap();
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->bDisableCollision = true;
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	Mesh->bAffectDynamicIndirectLighting = false;
	Mesh->bAffectDistanceFieldLighting = false;
	Mesh->SetReceivesDecals(false);
	Owner->AddInstanceComponent(Mesh);
	Mesh->RegisterComponent();
	View.Mesh = Mesh;
	View.Keys.Reset();
	const FStaticMeshRenderData* RenderData = SphereMesh->GetRenderData();
	FString Triangles;
	for (int32 Lod = 0; RenderData && Lod < RenderData->LODResources.Num(); ++Lod)
	{
		Triangles += FString::Printf(TEXT("%s%d"), Lod ? TEXT("/") : TEXT(""), RenderData->LODResources[Lod].GetNumTriangles());
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars] instanced photospheres for %s: mesh %s (triangles per LOD %s), forced LOD %d"),
		*GetNameSafe(Owner), *GetNameSafe(SphereMesh), *Triangles, Mesh->ForcedLodModel);
	return Mesh;
}

void RemoveResolvedInstance(FResolvedState& State, FResolvedSlot& Slot)
{
	FResolvedView* View = State.Views[Slot.Tier].Find(Slot.Demand.Key.Source);
	if (!View || !View->Keys.IsValidIndex(Slot.Instance))
	{
		Slot.Instance = INDEX_NONE;
		return;
	}
	UInstancedStaticMeshComponent* Mesh = View->Mesh.Get();
	const int32 Last = View->Keys.Num() - 1;
	if (IsValid(Mesh) && Mesh->GetInstanceCount() == View->Keys.Num()) Mesh->RemoveInstance(Slot.Instance);
	if (Slot.Instance != Last)
	{
		View->Keys[Slot.Instance] = View->Keys[Last];
		if (FResolvedSlot* Moved = State.Slots.Find(View->Keys[Slot.Instance])) Moved->Instance = Slot.Instance;
	}
	View->Keys.Pop(EAllowShrinking::No);
	Slot.Instance = INDEX_NONE;
}

/** Adds the slot's photosphere to its tier's component, at its physical radius for that component's sphere. */
bool AddResolvedInstance(FResolvedState& State, FResolvedSlot& Slot, AAstroGenerator* Generator)
{
	UHierarchicalInstancedStaticMeshComponent* Source = Slot.Demand.Key.Source.Get();
	if (!IsValid(Source)) return false;
	UStaticMesh* SphereMesh = Slot.Tier == 1 ? SmoothSphereMesh(Generator) : Source->GetStaticMesh().Get();
	FResolvedView& View = State.Views[Slot.Tier].FindOrAdd(Slot.Demand.Key.Source);
	UInstancedStaticMeshComponent* Mesh = EnsureResolvedView(View, Source, Generator->GetWorld(), SphereMesh);
	if (!Mesh || !IsValid(Mesh->GetStaticMesh())) return false;
	const FBoxSphereBounds MeshBounds = Mesh->GetStaticMesh()->GetBounds();
	const double ComponentScale = Source->GetComponentScale().GetAbsMax();
	const double Scale = ComponentScale > 0.0 && MeshBounds.BoxExtent.GetMax() > 0.0
		? Slot.Demand.PhysicalRadiusCm / ComponentScale / MeshBounds.BoxExtent.GetMax() : 0.0;
	if (!FMath::IsFinite(Scale) || Scale <= 0.0) return false;
	const FQuat Rotation = Slot.Demand.BaseTransform.GetRotation();
	const FTransform Local(Rotation,
		Slot.Demand.BaseTransform.GetLocation() - Rotation.RotateVector(MeshBounds.Origin * Scale), FVector(Scale));
	const int32 Instance = Mesh->AddInstance(Local, false);
	if (Instance == INDEX_NONE) return false;
	Mesh->SetCustomData(Instance, TArrayView<const float>(Slot.Data, 6), false);
	if (View.Keys.Num() <= Instance) View.Keys.SetNum(Instance + 1);
	View.Keys[Instance] = Slot.Demand.Key;
	Slot.Instance = Instance;
	return true;
}

int32 WantedTier(const FResolvedSlot& Slot, const double PixelRadius)
{
	const double Smooth = CVarResolvedSmoothPixels.GetValueOnGameThread();
	if (Smooth <= 0.0) return 0;
	return PixelRadius >= (Slot.Tier == 1 ? Smooth * 0.8 : Smooth) ? 1 : 0;
}

void ReleaseResolved(const UAPSStellarVisualSubsystem* Subsystem, AAstroGenerator* Generator, const FVector& Camera,
	const double PixelTangent, TSet<UHierarchicalInstancedStaticMeshComponent*>& DirtySources)
{
	FResolvedState* State = FindResolved(Subsystem);
	if (!State) return;
	for (TPair<FAPSGameplayStellarKey, FResolvedSlot>& Entry : State->Slots)
	{
		if (Entry.Value.bOwnsPoint)
			RestoreOwnedPoint(Generator, Entry.Value.Demand, Entry.Value.LastPublishedPoint, Camera, PixelTangent, DirtySources);
	}
	for (TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, FResolvedView>& Views : State->Views)
	{
		for (TPair<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, FResolvedView>& Entry : Views)
		{
			if (UInstancedStaticMeshComponent* Mesh = Entry.Value.Mesh.Get()) Mesh->DestroyComponent();
		}
	}
	GResolvedStates.Remove(TWeakObjectPtr<const UAPSStellarVisualSubsystem>(Subsystem));
}

bool ResolvedOwnsPoint(const UAPSStellarVisualSubsystem* Subsystem, const AAstroGenerator* Generator,
	const FAPSGameplayStellarKey& Key, const FTransform& BaseTransform, const FTransform& CurrentTransform)
{
	const FResolvedState* State = FindResolved(Subsystem);
	const FResolvedSlot* Slot = State ? State->Slots.Find(Key) : nullptr;
	return Slot && Slot->bOwnsPoint && Slot->Demand.BaseTransform.Equals(BaseTransform, 0.0)
		&& CurrentTransform.Equals(Slot->LastPublishedPoint, 0.0) && GeometryCurrent(Generator, Slot->Demand);
}

void PresentResolvedStars(const UAPSStellarVisualSubsystem* Subsystem, AAstroGenerator* Generator,
	TArray<FAPSGameplayNativeDemand>& Demands, const FVector& Camera, const double PixelTangent, const bool bDaylightHidden)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_GameplayResolvedStars);
	FResolvedState& State = GResolvedStates.FindOrAdd(TWeakObjectPtr<const UAPSStellarVisualSubsystem>(Subsystem));
	const uint64 Serial = Generator->GetCanonicalStellarProjectionDescriptor().TransformMutationSerial;
	const bool bValidate = Serial != State.ValidatedSerial;
	State.ValidatedSerial = Serial;

	// Every demanded star of at least ResolvePixels (KeepResolvedPixels once drawn), in all directions; a day sky none.
	TSet<FAPSGameplayStellarKey> Wanted;
	TArray<const FAPSGameplayNativeDemand*> ToAdd;
	if (!bDaylightHidden)
	{
		Wanted.Reserve(Demands.Num());
		for (FAPSGameplayNativeDemand& Demand : Demands)
		{
			const UHierarchicalInstancedStaticMeshComponent* Source = Demand.Key.Source.Get();
			if (!IsValid(Source)) continue;
			const double PixelWorldRadius = FVector::Distance(
				Source->GetComponentTransform().TransformPosition(Demand.BaseTransform.GetLocation()), Camera) * PixelTangent;
			Demand.PixelRadius = PixelWorldRadius > 0.0 ? Demand.PhysicalRadiusCm / PixelWorldRadius : 0.0;
			FResolvedSlot* Drawn = State.Slots.Find(Demand.Key);
			const bool bDrawn = Drawn != nullptr;
			if (!FMath::IsFinite(Demand.PixelRadius) || Demand.PixelRadius < (bDrawn ? KeepResolvedPixels() : ResolvePixels()))
				continue;
			if (Drawn) Drawn->Demand.PixelRadius = Demand.PixelRadius;
			// Producer reasons (materialized, excluded, rebuilt) are read again when the catalogue changed, and for a new star.
			if ((bValidate || !bDrawn) && (!GeometryCurrent(Generator, Demand)
				|| Generator->GetGameplayStellarSuppression(Demand.Key) != 0)) continue;
			Wanted.Add(Demand.Key);
			if (!bDrawn) ToAdd.Add(&Demand);
		}
	}

	TSet<UHierarchicalInstancedStaticMeshComponent*> DirtySources;
	for (auto It = State.Slots.CreateIterator(); It; ++It)
	{
		FResolvedSlot& Slot = It.Value();
		bool bKeep = Wanted.Contains(It.Key());
		if (bKeep && bValidate && Slot.bOwnsPoint)
		{
			// A point someone else wrote since is theirs (as with the pairs): dropped without a restore.
			const UHierarchicalInstancedStaticMeshComponent* Source = Slot.Demand.Key.Source.Get();
			FTransform Point;
			if (!IsValid(Source) || !Source->GetInstanceTransform(Slot.Demand.Key.Index, Point, false)
				|| !Point.Equals(Slot.LastPublishedPoint, 0.0))
			{
				Slot.bOwnsPoint = false;
				bKeep = false;
			}
		}
		if (bKeep) continue;
		RemoveResolvedInstance(State, Slot);
		if (Slot.bOwnsPoint)
			RestoreOwnedPoint(Generator, Slot.Demand, Slot.LastPublishedPoint, Camera, PixelTangent, DirtySources);
		It.RemoveCurrent();
	}

	// A star that grew past aps.Stars.ResolvedSmoothPixels moves to the smooth sphere (and back below 80% of it); a
	// move is the same frame's remove and add, its point stays owned, so nothing shows in between.
	int32 Changes = 0;
	const int32 Budget = FMath::Max(CVarResolvedAddsPerFrame.GetValueOnGameThread(), 1);
	for (TPair<FAPSGameplayStellarKey, FResolvedSlot>& Entry : State.Slots)
	{
		FResolvedSlot& Slot = Entry.Value;
		if (Changes >= Budget) break;
		if (Slot.Instance == INDEX_NONE)
		{
			// Its component was lost (or a move could not add it): draw it again.
			Changes += AddResolvedInstance(State, Slot, Generator) ? 1 : 0;
			continue;
		}
		const int32 Tier = WantedTier(Slot, Slot.Demand.PixelRadius);
		if (Tier == Slot.Tier) continue;
		const int32 OldTier = Slot.Tier;
		RemoveResolvedInstance(State, Slot);
		Slot.Tier = Tier;
		if (!AddResolvedInstance(State, Slot, Generator))
		{
			Slot.Tier = OldTier;
			AddResolvedInstance(State, Slot, Generator);
		}
		++Changes;
	}

	// New ones, largest first, a bounded number a frame; one still waiting keeps its point for that frame.
	const int32 Room = FMath::Max(CVarResolvedLimit.GetValueOnGameThread() - State.Slots.Num(), 0);
	const int32 Adds = FMath::Min(FMath::Max(Budget - Changes, 0), Room);
	if (ToAdd.Num() > Adds)
	{
		ToAdd.Sort([](const FAPSGameplayNativeDemand& A, const FAPSGameplayNativeDemand& B) { return A.PixelRadius > B.PixelRadius; });
		ToAdd.SetNum(Adds, EAllowShrinking::No);
	}
	UStarGenerator* Stars = Generator->GetGameplayStellarAppearanceGenerator();
	for (const FAPSGameplayNativeDemand* Demand : ToAdd)
	{
		UHierarchicalInstancedStaticMeshComponent* Source = Demand->Key.Source.Get();
		FTransform Point;
		if (State.Slots.Contains(Demand->Key) || !IsValid(Source)
			|| !Source->GetInstanceTransform(Demand->Key.Index, Point, false)) continue;
		if (Point.GetScale3D() == FVector::ZeroVector)
		{
			// Someone else's zero: never taken over.
			Generator->SetGameplayStellarSuppression(Demand->Key, EAPSGameplayStellarSuppression::UnclassifiedExternal, true);
			continue;
		}
		FResolvedSlot NewSlot;
		NewSlot.Demand = *Demand;
		if (!MakeResolvedData(Stars, Demand->Key, NewSlot.Data)) continue;
		NewSlot.Tier = WantedTier(NewSlot, Demand->PixelRadius);
		if (!AddResolvedInstance(State, NewSlot, Generator)) continue;
		Point.SetScale3D(FVector::ZeroVector);
		Source->UpdateInstanceTransform(Demand->Key.Index, Point, false, false, true);
		NewSlot.LastPublishedPoint = Point;
		NewSlot.bOwnsPoint = true;
		State.Slots.Add(Demand->Key, NewSlot);
		DirtySources.Add(Source);
	}
	FlushSources(DirtySources);
	const double Now = FPlatformTime::Seconds();
	if (State.Slots.Num() != State.LoggedCount && Now - State.LoggedSeconds >= 2.0)
	{
		State.LoggedCount = State.Slots.Num();
		State.LoggedSeconds = Now;
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars] generator=%s instanced photospheres=%d demand=%d waiting=%d; ")
			TEXT("all directions, the star material's own detail fade"), *GetNameSafe(Generator), State.Slots.Num(),
			Demands.Num(), FMath::Max(Wanted.Num() - State.Slots.Num(), 0));
	}
	CSV_CUSTOM_STAT(APSGameplayStars, ResolvedInstances, State.Slots.Num(), ECsvCustomStatOp::Set);
}
}

bool APSGameplayNativeStars::UsesViewSelection()
{
	return !InstancedMode();
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
			RadiusSolar = APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass) * Record.RadiusScale;
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

int32 AAstroGenerator::SuppressClusterProxies(const TArray<int32>& InstanceIndices)
{
	UHierarchicalInstancedStaticMeshComponent* Component =
		IsValid(GeneratedStarCluster) ? GeneratedStarCluster->StarMeshInstances : nullptr;
	if (!IsValid(Component))
	{
		return 0;
	}
	int32 Suppressed = 0;
	for (const int32 InstanceIndex : InstanceIndices)
	{
		const FAPSGameplayStellarKey Key = MakeGameplayStellarKey(Component, InstanceIndex);
		if ((GetGameplayStellarSuppression(Key) & static_cast<uint8>(EAPSGameplayStellarSuppression::SafetyExclusion)) != 0)
		{
			continue;
		}
		SetGameplayStellarSuppression(Key, EAPSGameplayStellarSuppression::SafetyExclusion, true);
		FTransform Transform;
		if (Component->GetInstanceTransform(InstanceIndex, Transform, false) && Transform.GetScale3D() != FVector::ZeroVector)
		{
			Transform.SetScale3D(FVector::ZeroVector);
			Component->UpdateInstanceTransform(InstanceIndex, Transform, false, false, true);
		}
		++Suppressed;
	}
	if (Suppressed > 0)
	{
		Component->BuildTreeIfOutdated(true, true);
		// The flight model and the stellar view read the catalogue again.
		NoteCanonicalStellarProxyMutation(true);
	}
	return Suppressed;
}

bool AAstroGenerator::SetGalaxyProxyMaterialized(const int64 CatalogIndex, const bool bMaterialized)
{
	// Only the galaxy's ISM prefix has instances here; its GPU points are hidden by the materialized system's sphere.
	UHierarchicalInstancedStaticMeshComponent* Component =
		IsValid(GeneratedGalaxy) ? GeneratedGalaxy->StarMeshInstances : nullptr;
	const int32 InstanceIndex = IsValid(Component) ? GeneratedGalaxy->RenderedCatalogIndices.Find(CatalogIndex) : INDEX_NONE;
	if (InstanceIndex == INDEX_NONE || InstanceIndex >= Component->GetInstanceCount())
	{
		return false;
	}
	const FAPSGameplayStellarKey Key = MakeGameplayStellarKey(Component, InstanceIndex);
	SetGameplayStellarSuppression(Key, EAPSGameplayStellarSuppression::Materialized, bMaterialized);
	FTransform Transform;
	if (GetGameplayStellarSuppression(Key) != 0)
	{
		if (!Component->GetInstanceTransform(InstanceIndex, Transform, false)) return false;
		Transform.SetScale3D(FVector::ZeroVector);
	}
	else if (!GeneratedGalaxy->GetRenderedProxyBaseTransform(InstanceIndex, Transform))
	{
		return false;
	}
	Component->UpdateInstanceTransform(InstanceIndex, Transform, false, true, true);
	Component->BuildTreeIfOutdated(true, true);
	// As for a materialized cluster system: the stellar view sizes its points again, the resolved stars re-validate.
	NoteCanonicalStellarProxyMutation(true);
	return true;
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
	ReleaseResolved(this, GameplayStellarGenerator.Get(), GameplayNativeCamera, GameplayNativePixelTangent, DirtySources);
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
	// A switch of aps.Stars.NativeMode starts over from the catalogue points (the new mode fills from the next demand).
	int32& LastMode = GNativeModes.FindOrAdd(TWeakObjectPtr<const UAPSStellarVisualSubsystem>(this), INDEX_NONE);
	const int32 Mode = InstancedMode() ? 1 : 0;
	if (LastMode != INDEX_NONE && LastMode != Mode && bGameplayNativeInitialized)
	{
		ResetGameplayNativeStars();
	}
	LastMode = Mode;
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
		if (const FResolvedState* Resolved = FindResolved(this))
		{
			// Their instances go at the next presentation (suppressed), their zeros stay.
			for (const TPair<FAPSGameplayStellarKey, FResolvedSlot>& Entry : Resolved->Slots)
			{
				if (Entry.Value.bOwnsPoint)
					Generator->SetGameplayStellarSuppression(Entry.Key, EAPSGameplayStellarSuppression::UnclassifiedExternal, true);
			}
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
	if (!bOwned)
	{
		bOwned = ResolvedOwnsPoint(this, Generator, Key, BaseTransform, CurrentTransform);
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
	// Rio 03.10 (NativeMode 1): every resolved star in all directions, so a turn never re-selects (no blurred spots).
	const bool bInstanced = InstancedMode();
	const FResolvedState* Resolved = bInstanced ? FindResolved(this) : nullptr;
	const bool bRetained = bInstanced ? Resolved && Resolved->Slots.Contains(Key) : GameplayNativeOwners.Contains(Key);
	if (!FMath::IsFinite(PixelRadius) || PixelRadius < (bRetained ? RetainPixels() : PreparePixels())) return;
	const FVector Offset = Key.Source->GetComponentTransform().TransformPosition(BaseTransform.GetLocation())
		- GameplayNativeCamera;
	// Budget the camera's candidates, not the largest stars on the opposite side
	// of the sky. A 32-pixel guard keeps admission stable at the viewport edge.
	const double GuardedRadius = PhysicalRadiusCm * 1.32 + Offset.Size() * GameplayNativePixelTangent * 32.0;
	if (!bInstanced && !APSPreviewVisibility::SphereIntersectsView(GameplayNativeViewRotation.UnrotateVector(Offset),
		GuardedRadius, GameplayNativeTanHalfHorizontal, GameplayNativeTanHalfVertical)) return;
	const int32 DemandLimit = bInstanced ? FMath::Max(CVarResolvedLimit.GetValueOnGameThread(), 0) : PairLimit;
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
	if (GameplayNativeDemand.Num() < DemandLimit) GameplayNativeDemand.Add(MoveTemp(Demand));
	else if (DemandLimit <= 0) return;
	else
	{
		int32 Smallest = 0;
		for (int32 Index = 1; Index < GameplayNativeDemand.Num(); ++Index)
			if (GameplayNativeDemand[Index].PixelRadius < GameplayNativeDemand[Smallest].PixelRadius) Smallest = Index;
		if (PixelRadius > GameplayNativeDemand[Smallest].PixelRadius)
		{
			GameplayNativeOverflowResolved += GameplayNativeDemand[Smallest].PixelRadius >= ResolvePixels();
			GameplayNativeDemand[Smallest] = MoveTemp(Demand);
		}
		else GameplayNativeOverflowResolved += PixelRadius >= ResolvePixels();
	}
}

void UAPSStellarVisualSubsystem::PresentGameplayNativeStars(AAstroGenerator* Generator)
{
	if (InstancedMode())
	{
		PresentResolvedStars(this, Generator, GameplayNativeDemand, GameplayNativeCamera, GameplayNativePixelTangent,
			bGameplayDaylightStarsHidden);
		return;
	}
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
		return !FMath::IsFinite(Demand.PixelRadius) || Demand.PixelRadius < RetainPixels();
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
	if (!IsValid(Template) || !IsValid(Template->GetStaticMesh()))
	{
		// No home star mesh to copy: the engine sphere carries the photosphere (PresentPair scales by its bounds).
		static TWeakObjectPtr<UStaticMeshComponent> FallbackTemplate;
		if (!FallbackTemplate.IsValid())
		{
			if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
			{
				UStaticMeshComponent* Fallback = NewObject<UStaticMeshComponent>(Generator, NAME_None, RF_Transient);
				Fallback->SetStaticMesh(Sphere);
				FallbackTemplate = Fallback;
				UE_LOG(LogTemp, Warning, TEXT("[APS.Gameplay.NativeStars] home star mesh missing; native stars use the engine sphere"));
			}
		}
		Template = FallbackTemplate.Get();
	}
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
		if (PairIndex == INDEX_NONE) { UnmetResolved += Demand.PixelRadius >= ResolvePixels(); continue; }
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
					UnmetResolved += Demand.PixelRadius >= ResolvePixels();
					continue;
				}
				Pair.bBound = BindPair(Generator, Pair);
			}
		}
		const bool bResolve = Demand.PixelRadius >= (Pair.bOwnsPoint ? KeepResolvedPixels() : ResolvePixels());
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
	// Logged when what is shown changes, or the demand empties or fills: the demand itself moves with every camera turn
	// (700 lines in 40 minutes of play, 01.10).
	if (PresentedCount != GameplayNativeLastPresentedCount
		|| (GameplayNativeDemand.Num() == 0) != (GameplayNativeLastDemandCount == 0))
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars] generator=%s demand=%d presented=%d pool=%d pendingResolved=%d bindAttempts=%d limit=%d; physical radii, visible-view selection"),
			*GetNameSafe(Generator), GameplayNativeDemand.Num(), PresentedCount, GameplayNativePairs.Num(),
			UnmetResolved, BindAttempts, PairLimit);
		GameplayNativeLastPresentedCount = PresentedCount;
		GameplayNativeLastDemandCount = GameplayNativeDemand.Num();
	}
	static double LastDiagSeconds = 0.0;
	if (CVarNativeDiag.GetValueOnGameThread() > 0 && FPlatformTime::Seconds() - LastDiagSeconds >= 1.0)
	{
		LastDiagSeconds = FPlatformTime::Seconds();
		UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars.Diag] frame=%llu demand=%d presented=%d pool=%d template=%d templateMesh=%s daylightHidden=%d serial=%llu"),
			static_cast<unsigned long long>(GFrameCounter), GameplayNativeDemand.Num(), PresentedCount, GameplayNativePairs.Num(),
			IsValid(Template) ? 1 : 0, IsValid(Template) ? *GetNameSafe(Template->GetStaticMesh()) : TEXT("-"),
			bGameplayDaylightStarsHidden ? 1 : 0, static_cast<unsigned long long>(GameplayNativeMutationSerial));
		for (const TCHAR* BasePath : {APSStellarMaterialContract::ActorBaseObjectPath, APSStellarMaterialContract::CoronaBaseObjectPath})
		{
			UMaterial* Base = APSStellarMaterialContract::LoadCanonicalBase(BasePath);
			const FMaterialResource* Resource = IsValid(Base) && GetWorld() ? Base->GetMaterialResource(GetWorld()->GetFeatureLevel()) : nullptr;
			const FMaterialShaderMap* ShaderMap = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
			UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars.Diag]   base %s: loaded=%d resource=%d compiled=%d complete=%d shaderMap=%d localVF=%d ready=%d"),
				*FPaths::GetBaseFilename(BasePath), IsValid(Base) ? 1 : 0, Resource ? 1 : 0,
				Resource && Resource->IsCompilationFinished() ? 1 : 0, Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
				ShaderMap ? 1 : 0, ShaderMap && ShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0,
				MaterialReady(Base, GetWorld(), BasePath) ? 1 : 0);
		}
		for (int32 DemandIndex = 0; DemandIndex < FMath::Min(GameplayNativeDemand.Num(), 4); ++DemandIndex)
		{
			const FAPSGameplayNativeDemand& Demand = GameplayNativeDemand[DemandIndex];
			const int32* PairIndex = GameplayNativeOwners.Find(Demand.Key);
			const FAPSGameplayNativePair* Pair = PairIndex && GameplayNativePairs.IsValidIndex(*PairIndex) ? &GameplayNativePairs[*PairIndex] : nullptr;
			UStaticMeshComponent* Surface = Pair ? Pair->Photosphere.Get() : nullptr;
			UStaticMeshComponent* Corona = Pair ? Pair->Corona.Get() : nullptr;
			FTransform Point;
			const bool bPoint = Demand.Key.Source.IsValid() && Demand.Key.Source->GetInstanceTransform(Demand.Key.Index, Point, false);
			UE_LOG(LogTemp, Log, TEXT("[APS.Gameplay.NativeStars.Diag]   #%d %s[%d] px=%.2f pair=%d assigned=%d bound=%d boundFrame=%llu serialOk=%d owns=%d ")
				TEXT("valid=%d/%d registered=%d/%d renderState=%d/%d mesh=%d material=%d/%d visible=%d ready=%d suppression=%u geometry=%d pointScale=%.3g"),
				DemandIndex, *GetNameSafe(Demand.Key.Source.IsValid() ? Demand.Key.Source->GetOwner() : nullptr), Demand.Key.Index,
				Demand.PixelRadius, PairIndex ? *PairIndex : -1, Pair && Pair->bAssigned ? 1 : 0, Pair && Pair->bBound ? 1 : 0,
				static_cast<unsigned long long>(Pair ? Pair->BoundFrame : 0), Pair && Pair->BoundMutationSerial == GameplayNativeMutationSerial ? 1 : 0,
				Pair && Pair->bOwnsPoint ? 1 : 0, IsValid(Surface) ? 1 : 0, IsValid(Corona) ? 1 : 0,
				IsValid(Surface) && Surface->IsRegistered() ? 1 : 0, IsValid(Corona) && Corona->IsRegistered() ? 1 : 0,
				IsValid(Surface) && Surface->IsRenderStateCreated() ? 1 : 0, IsValid(Corona) && Corona->IsRenderStateCreated() ? 1 : 0,
				IsValid(Surface) && IsValid(Surface->GetStaticMesh()) && Surface->GetStaticMesh()->GetRenderData() ? 1 : 0,
				IsValid(Surface) && MaterialReady(Surface->GetMaterial(0), GetWorld(), APSStellarMaterialContract::ActorBaseObjectPath) ? 1 : 0,
				IsValid(Corona) && MaterialReady(Corona->GetMaterial(0), GetWorld(), APSStellarMaterialContract::CoronaBaseObjectPath) ? 1 : 0,
				IsValid(Surface) && Surface->IsVisible() ? 1 : 0, Pair && PairReady(*Pair, GetWorld()) ? 1 : 0,
				static_cast<uint32>(Generator->GetGameplayStellarSuppression(Demand.Key)), GeometryCurrent(Generator, Demand) ? 1 : 0,
				bPoint ? Point.GetScale3D().GetAbsMax() : -1.0);
		}
	}
	CSV_CUSTOM_STAT(APSGameplayStars, NativePairCount, GameplayNativePairs.Num(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(APSGameplayStars, NativeBindAttempts, BindAttempts, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(APSGameplayStars, NativeUnmetResolved, UnmetResolved, ECsvCustomStatOp::Set);
}
