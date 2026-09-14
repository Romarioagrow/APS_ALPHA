// Explicit editor diagnostics for the saved 12 September rendering checkpoint.
// Restores presentation only; never generates records or saves a world/package.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Rendering/APSGameplayStarAppearance.h"
#include "APS_ALPHA/Core/Rendering/APSGameplayStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSStarRenderStabilitySubsystem.h"
#include "APS_ALPHA/Generation/StarGenerator.h"

namespace APSStellarSavedProbe
{
static UWorld* GetCheckpointWorld()
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    return World && World->WorldType == EWorldType::PIE
        && World->GetName() == TEXT("L_WorldGeneration") ? World : nullptr;
}

static void ResizeProbeViewport()
{
    UWorld* World = GetCheckpointWorld();
    if (World && World->GetGameViewport() && World->GetGameViewport()->GetWindow().IsValid())
    {
        World->GetGameViewport()->GetWindow()->Resize(FVector2D(3396, 1208));
        UE_LOG(LogTemp, Display, TEXT("[APS.StellarProbe] Requested checkpoint viewport 3396x1208; no display-mode change."));
    }
}

struct FPreparedLayer
{
    TArray<FTransform> Base;
    TArray<FTransform> Presented;
    TArray<float> Data;
};

static void RestoreSavedPoints()
{
    UWorld* World = GetCheckpointWorld();
    if (!World) return;
    AGalaxy* Galaxy = nullptr;
    AStarCluster* Cluster = nullptr;
    int32 Galaxies = 0, Clusters = 0;
    for (TActorIterator<AGalaxy> It(World); It; ++It) { Galaxy = *It; ++Galaxies; }
    for (TActorIterator<AStarCluster> It(World); It; ++It) { Cluster = *It; ++Clusters; }
    if (Galaxies != 1 || Clusters != 1 || !Galaxy->StarMeshInstances || !Cluster->StarMeshInstances
        || Galaxy->StarMeshInstances->GetInstanceCount() != 0 || Cluster->StarMeshInstances->GetInstanceCount() != 0
        || !Galaxy->StarMeshInstances->GetStaticMesh() || !Cluster->StarMeshInstances->GetStaticMesh()
        || Galaxy->StarCatalog.GenerationSeed != 1867216637 || Galaxy->StarCatalog.ModeledStarCount != 100000000
        || Galaxy->StarCatalog.RenderedSampleCount != 25000 || Cluster->GenerationSeed != 2030576230
        || Cluster->PotentialStarSystems.Num() != 36455)
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.StellarProbe] Restore refused: not the empty saved checkpoint."));
        return;
    }
    const FClusterStarSystemRecord* Home = nullptr;
    for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
    {
        if (Record.bMaterialized)
        {
            if (Home) return;
            Home = &Record;
        }
    }
    if (!Home || Home->InstanceIndex != 39) return;
    const double GalaxyExtent = Galaxy->StarCatalog.CatalogHalfExtent.GetAbsMax();
    double ClusterExtent = Cluster->ClusterBounds.GetAbsMax() * 50.0;
    if (Cluster->ClusterType == EStarClusterType::GlobularCluster)
        ClusterExtent = FMath::Max(ClusterExtent, (Cluster->ClusterBounds.GetAbsMax() * 0.5 + 100000.0) * 100.0);
    FAPSCanonicalStellarProjectionFrame GalaxyFrame, ClusterFrame;
    const double LayerScale = FMath::Clamp(GalaxyExtent * 0.16 / ClusterExtent, 1.0e-9, 1.0);
    if (!APSCanonicalStellarProjection::ConfigureSharedHomeCentredFrames(
        GalaxyExtent, ClusterExtent, LayerScale, Home->ClusterLocalLocation, 0, GalaxyFrame, ClusterFrame)) return;
    const double ExpectedScale = 209710153.01185724;
    const FVector ExpectedOrigin(-72611177216484.95, -28024227245055.215, -6208662208566.655);
    if (!FMath::IsNearlyEqual(1.0 / GalaxyFrame.PositionScale, ExpectedScale, 1.0e-5))
    {
        UE_LOG(LogTemp, Error, TEXT("[APS.StellarProbe] Restore refused: affine scale mismatch %.17g."), 1.0 / GalaxyFrame.PositionScale);
        return;
    }
    for (UHierarchicalInstancedStaticMeshComponent* Source : {Galaxy->StarMeshInstances, Cluster->StarMeshInstances})
    {
        const FTransform Transform = Source->GetComponentTransform();
        if (!Transform.GetLocation().Equals(ExpectedOrigin, 0.1)
            || !Transform.GetScale3D().Equals(FVector(ExpectedScale), 1.0e-5)
            || !Transform.GetRotation().Equals(FQuat::Identity, 1.0e-8)) return;
    }
    // Exact checkpoint optics. This restores the previous presentation, not a new visual policy.
    const FVector Camera(-22334.30592223667, 57106.57101197382, -342.70972130562376);
    const double PixelTangent = 2.0 * FMath::Tan(FMath::DegreesToRadians(45.0)) / 3396.0;
    const double Compensation = APSCanonicalStellarProjection::FullScaleCanonicalCmPerUnit * GalaxyFrame.PositionScale;
    UStarGenerator* Photometry = GetMutableDefault<UStarGenerator>();
    const auto PreparePoint = [&](FPreparedLayer& Layer, UHierarchicalInstancedStaticMeshComponent* Source,
        const FAPSCanonicalStellarProjectionFrame& Frame, EAPSCanonicalStellarProxyLayer Kind,
        FVector Position, double Radius, double Luminosity, ESpectralClass SpectralClass,
        int32 Subclass, float Seed, float Potential, bool Hidden)
    {
        const FTransform Base(FQuat::Identity, Frame.ProjectCanonicalUnits(Position),
            FVector(UStarGenerator::GetFarStarVisualRadius(Radius) * Compensation));
        FTransform Presented = Base;
        const double MeshRadius = FMath::Max(Source->GetStaticMesh()->GetBounds().BoxExtent.GetMax(), 0.001);
        const double BaseRadius = MeshRadius * Base.GetScale3D().GetAbsMax();
        const double Distance = FVector::Distance(Base.GetLocation(), Source->GetComponentTransform().InverseTransformPosition(Camera));
        if (!FMath::IsFinite(BaseRadius) || BaseRadius <= 0.0 || !FMath::IsFinite(Distance)) return false;
        Presented.SetScale3D(Hidden ? FVector::ZeroVector : Base.GetScale3D()
            * (APSGameplayStellarProjection::GetFullScalePointRadius(Distance, BaseRadius, PixelTangent) / BaseRadius));
        const double AppliedRadius = APSCanonicalStellarProjection::GetAppliedVisualRadiusCm(Kind, Frame, Radius);
        const double Emission = UStarGenerator::GetFarStarVisualEmission(Radius,
            Photometry->CalculateEmission(static_cast<float>(Luminosity * 25.0)),
            APSCanonicalStellarProjection::UnprojectPhysicalRadiusSolar(Frame, AppliedRadius));
        const FLinearColor Color = UStarGenerator::GetStarColor(SpectralClass, Subclass);
        if (Base.ContainsNaN() || Presented.ContainsNaN() || !FMath::IsFinite(Emission)) return false;
        Layer.Base.Add(Base);
        Layer.Presented.Add(Presented);
        Layer.Data.Append({Color.R, Color.G, Color.B, static_cast<float>(Emission), Seed, Potential});
        return true;
    };
    FPreparedLayer GalaxyLayer, ClusterLayer;
    TArray<int64> CatalogIndices;
    const auto Order = APSCanonicalStellarProjection::MakeNestedCatalogPermutation(
        Galaxy->StarCatalog.GenerationSeed, Galaxy->StarCatalog.ModeledStarCount);
    for (int32 Index = 0; Index < Galaxy->StarCatalog.RenderedSampleCount; ++Index)
    {
        const int64 CatalogIndex = Order.Resolve(Index);
        FGalaxyCatalogStarRecord Record;
        if (!Galaxy->StarCatalog.ResolveStar(CatalogIndex, Record)) return;
        if (!PreparePoint(GalaxyLayer, Galaxy->StarMeshInstances, GalaxyFrame, EAPSCanonicalStellarProxyLayer::Galaxy,
            Record.GalaxyLocalLocation, APSCanonicalStellarProjection::GetCanonicalStellarRadiusSolar(Record.SpectralClass),
            APSCanonicalStellarProjection::GetCanonicalStellarLuminositySolar(Record.SpectralClass), Record.SpectralClass,
            Record.SpectralSubclass, static_cast<float>((Record.GenerationSeed & 0xffff) / 65535.0),
            Record.bPotentialStarSystem ? 1.0f : 0.0f, false)) return;
        CatalogIndices.Add(CatalogIndex);
    }
    for (int32 Index = 0; Index < Cluster->PotentialStarSystems.Num(); ++Index)
    {
        const FClusterStarSystemRecord& Record = Cluster->PotentialStarSystems[Index];
        if (Record.InstanceIndex != Index || !Record.StableId.IsValid()) return;
        FRandomStream VisualStream(Record.SystemModel.GenerationSeed);
        const FStarModel& Model = Record.PrimaryStarModel;
        if (!PreparePoint(ClusterLayer, Cluster->StarMeshInstances, ClusterFrame, EAPSCanonicalStellarProxyLayer::StarCluster,
            Record.ClusterLocalLocation, Model.Radius, Model.Luminosity, Model.SpectralClass, Model.SpectralSubclass,
            VisualStream.FRand(), Record.SystemModel.PotentialPlanetCount / 12.0f, Record.bMaterialized)) return;
    }
    // All records, dimensions and component frames validated before the first mutation.
    const auto Publish = [](UHierarchicalInstancedStaticMeshComponent* Source, const FPreparedLayer& Layer)
    {
        Source->bAutoRebuildTreeOnInstanceChanges = false;
        Source->SetNumCustomDataFloats(6);
        Source->AddInstances(Layer.Presented, false, false);
        for (int32 Index = 0; Index < Layer.Presented.Num(); ++Index)
            Source->SetCustomData(Index, TArrayView<const float>(Layer.Data.GetData() + Index * 6, 6), false);
        UAPSStarRenderStabilitySubsystem::StabilizeInstances(Source);
        Source->SetVisibility(true, false);
        Source->SetHiddenInGame(false, false);
        Source->BuildTreeIfOutdated(false, true);
        Source->MarkRenderStateDirty();
    };
    Galaxy->RenderedCatalogIndices = MoveTemp(CatalogIndices);
    Galaxy->RenderedProxyBaseTransforms = MoveTemp(GalaxyLayer.Base);
    Galaxy->CanonicalProjectionFrame = GalaxyFrame;
    Cluster->SystemProxyBaseTransforms = MoveTemp(ClusterLayer.Base);
    Cluster->CanonicalProjectionFrame = ClusterFrame;
    Publish(Galaxy->StarMeshInstances, GalaxyLayer);
    Publish(Cluster->StarMeshInstances, ClusterLayer);
    APSGameplayStarAppearance::Apply(World);
    UE_LOG(LogTemp, Display, TEXT("[APS.StellarProbe] Restored saved presentation: galaxy=%d cluster=%d scale=%.17g; catalog/records/root transforms unchanged; nothing saved."),
        Galaxy->StarMeshInstances->GetInstanceCount(), Cluster->StarMeshInstances->GetInstanceCount(), ExpectedScale);
}

static FAutoConsoleCommand RestoreCommand(TEXT("APS.Diagnostics.RestoreSavedStellarPoints"),
    TEXT("Restore only empty HISM presentation for the verified September 12 checkpoint. Editor diagnostic, no save or generation."),
    FConsoleCommandDelegate::CreateStatic(&RestoreSavedPoints), ECVF_Cheat);
static FAutoConsoleCommand ViewportCommand(TEXT("APS.Diagnostics.StellarCheckpointViewport"),
    TEXT("Resize only the current PIE window to the checkpoint viewport."),
    FConsoleCommandDelegate::CreateStatic(&ResizeProbeViewport), ECVF_Cheat);
}
#endif
