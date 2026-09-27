#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PlanetaryAtmosphere.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "WorldScapeCore/Public/WorldScapeMeshComponent.h"

// STAGED diagnostic only. No production binding, vertex, lighting or camera writes.
namespace APSUserGreenhouseReference
{
    inline bool IsRequested()
    {
        return FParse::Param(FCommandLine::Get(), TEXT("APSUserGreenhouseReference"));
    }

    inline void ConfigureModel(UGeneratedWorld& Model)
    {
        Model.PlanetSurfaceSeed = 883716;
        Model.PlanetRadius = 6750.0;
        Model.SurfaceFeatureScale = 3.549998;
        Model.SurfaceReliefScale = 1.999999;
        Model.SurfaceLandCoverageScale = 0.7;
        Model.AtmosphereHeight = 260.0;
        Model.AtmosphereOpacity = 10.0;
        Model.AtmosphereMultiScattering = 7.900006;
        Model.AtmosphereRayleighScattering = 48.0;
        UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Model] seed=%d radiusKm=%.17g feature=%.17g relief=%.17g land=%.17g atmosphereHeightKm=%.17g opacity=%.17g multi=%.17g rayleighHeightKm=%.17g; natural production landing; no pose/material/light overrides"),
            Model.PlanetSurfaceSeed, Model.PlanetRadius, Model.SurfaceFeatureScale,
            Model.SurfaceReliefScale, Model.SurfaceLandCoverageScale, Model.AtmosphereHeight,
            Model.AtmosphereOpacity, Model.AtmosphereMultiScattering, Model.AtmosphereRayleighScattering);
    }

    inline FString V(const FVector& Value)
    {
        return FString::Printf(TEXT("[%.17g,%.17g,%.17g]"), Value.X, Value.Y, Value.Z);
    }

    inline FString V4(const FVector4d& Value)
    {
        return FString::Printf(TEXT("[%.17g,%.17g,%.17g,%.17g]"), Value.X, Value.Y, Value.Z, Value.W);
    }

    inline FVector XYZ(const FVector4d& Value) { return FVector(Value.X, Value.Y, Value.Z); }

    inline FString C(const FLinearColor& Value)
    {
        return FString::Printf(TEXT("[%.17g,%.17g,%.17g,%.17g]"),
            static_cast<double>(Value.R), static_cast<double>(Value.G),
            static_cast<double>(Value.B), static_cast<double>(Value.A));
    }

    inline void LogComponent(const TCHAR* Label, const USceneComponent* Component)
    {
        if (!IsValid(Component)) return;
        const FTransform Transform = Component->GetComponentTransform();
        const FQuat Rotation = Transform.GetRotation();
        UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Transform] label=%s component=%s worldLocation=%s scale=%s rotation=[%.17g,%.17g,%.17g,%.17g] relativeLocation=%s relativeScale=%s"),
            Label, *Component->GetPathName(), *V(Transform.GetLocation()), *V(Transform.GetScale3D()),
            Rotation.X, Rotation.Y, Rotation.Z, Rotation.W,
            *V(Component->GetRelativeLocation()), *V(Component->GetRelativeScale3D()));
    }

    inline void LogAtmosphere(const AAtmoScape* Atmosphere)
    {
        if (!IsValid(Atmosphere))
        {
            UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Atmosphere] actor=MISSING"));
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Atmosphere] actor=%s keepRelativeScale=%d planetRadiusKm=%.17g heightKm=%.17g location=%s scale=%s hidden=%d tick=%d presentationPlanetRadiusCm=%.17g presentationAtmosphereRadiusCm=%.17g presentationOpacity=%.17g presentationLight=%.17g opacity=%.17g multi=%.17g rayleighCoef=%s rayleighHeightKm=%.17g mieCoef=%s mieHeightKm=%.17g miePhase=%.17g lightSource=%s"),
            *Atmosphere->GetPathName(), Atmosphere->bKeepRelativeScale ? 1 : 0,
            static_cast<double>(Atmosphere->PlanetRadius), static_cast<double>(Atmosphere->AtmosphereHeight),
            *V(Atmosphere->GetActorLocation()), *V(Atmosphere->GetActorScale3D()),
            Atmosphere->IsHidden() ? 1 : 0, Atmosphere->IsActorTickEnabled() ? 1 : 0,
            static_cast<double>(Atmosphere->PresentationPlanetRadiusCm),
            static_cast<double>(Atmosphere->PresentationAtmosphereRadiusCm),
            static_cast<double>(Atmosphere->PresentationOpacityScale), static_cast<double>(Atmosphere->PresentationLightIntensity),
            static_cast<double>(Atmosphere->AtmosphereOpacity), static_cast<double>(Atmosphere->MultiScatering),
            *C(Atmosphere->RayleighScattering), static_cast<double>(Atmosphere->RayleighHeight),
            *C(Atmosphere->MieScattering), static_cast<double>(Atmosphere->MieHeight), static_cast<double>(Atmosphere->MiePhase),
            *GetPathNameSafe(Atmosphere->LightSource));
        TInlineComponentArray<UStaticMeshComponent*> Components;
        Atmosphere->GetComponents(Components);
        const TCHAR* ShellNames[] = { TEXT("PlanetaryAtmoMesh"), TEXT("SpacePlanetaryAtmoMesh") };
        const TCHAR* ScalarNames[] = { TEXT("EarthRadius"), TEXT("AtmosRadius"), TEXT("ActorScale"),
            TEXT("AtmosOpacity"), TEXT("ScaleHeight_R"), TEXT("ScaleHeight_M"),
            TEXT("CustomLightSource"), TEXT("LightIntensity") };
        const TCHAR* VectorNames[] = { TEXT("coef_R"), TEXT("coef_M"), TEXT("LightPosition"), TEXT("SunlightIntensity") };
        for (const TCHAR* ShellName : ShellNames)
        {
            const UStaticMeshComponent* Shell = nullptr;
            for (const UStaticMeshComponent* Component : Components)
                if (IsValid(Component) && Component->GetName() == ShellName) { Shell = Component; break; }
            if (!Shell)
            {
                UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.AtmosphereShell] name=%s component=MISSING"), ShellName);
                continue;
            }
            LogComponent(ShellName, Shell);
            UMaterialInterface* Material = Shell->GetMaterial(0);
            UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.AtmosphereShell] name=%s component=%s registered=%d visible=%d visibleFlag=%d hiddenInGame=%d tick=%d material=%s dynamic=%d"),
                ShellName, *Shell->GetPathName(), Shell->IsRegistered() ? 1 : 0, Shell->IsVisible() ? 1 : 0,
                Shell->GetVisibleFlag() ? 1 : 0, Shell->bHiddenInGame ? 1 : 0, Shell->IsComponentTickEnabled() ? 1 : 0,
                *GetPathNameSafe(Material), Cast<UMaterialInstanceDynamic>(Material) ? 1 : 0);
            for (const TCHAR* Name : ScalarNames)
            {
                float Value = 0.0f;
                const bool bRead = IsValid(Material)
                    && Material->GetScalarParameterValue(FMaterialParameterInfo(Name), Value);
                const FString ReadValue = bRead ? FString::Printf(TEXT("%.17g"), static_cast<double>(Value)) : TEXT("MISSING");
                UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.AtmosphereMIDScalar] shell=%s name=%s available=%d value=%s"),
                    ShellName, Name, bRead ? 1 : 0, *ReadValue);
            }
            for (const TCHAR* Name : VectorNames)
            {
                FLinearColor Value(ForceInit);
                const bool bRead = IsValid(Material)
                    && Material->GetVectorParameterValue(FMaterialParameterInfo(Name), Value);
                const FString ReadValue = bRead ? C(Value) : TEXT("MISSING");
                UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.AtmosphereMIDVector] shell=%s name=%s available=%d value=%s"),
                    ShellName, Name, bRead ? 1 : 0, *ReadValue);
            }
        }
    }

    // Game-thread CPU readback of currently published mesh sections. This does not
    // inspect GPU shader intermediates and cannot prove a particular precision bug.
    inline bool LogBeforeFirstScreenshot(APlanet* Planet, APlanetarySurfaceGenerator* Surface,
        AWorldScapeRoot* Root, const FVector& CameraWorld, FString& NotReady)
    {
        NotReady.Reset();
        if (!IsValid(Planet) || !IsValid(Surface) || !IsValid(Root)
            || !IsValid(Root->GetRootComponent()) || CameraWorld.ContainsNaN())
        {
            NotReady = TEXT("Greenhouse coordinate readback lacks valid body/root/camera");
            return false;
        }
        if (Root->WorldScapeLodInGeneration.Num() != 0)
        {
            NotReady = TEXT("Greenhouse coordinate readback waits for terrain workers");
            return false;
        }
        const UWorldScapeLod* Lod0 = nullptr;
        for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
        {
            if (!IsValid(Lod) || Lod->WaterBody || Lod->Lod != 0) continue;
            if (Lod0)
            {
                NotReady = TEXT("Greenhouse coordinate readback found duplicate terrain LOD0");
                return false;
            }
            Lod0 = Lod;
        }
        if (!IsValid(Lod0) || !IsValid(Lod0->Mesh) || !Lod0->Mesh->IsRegistered()
            || Lod0->Mesh->GetNumSections() < 1)
        {
            NotReady = TEXT("Greenhouse coordinate readback waits for published terrain LOD0");
            return false;
        }
        const FWorldScapeMeshSection* FirstSection = Lod0->Mesh->GetProcMeshSection(0);
        if (!FirstSection || FirstSection->PlanetVertexBuffer.IsEmpty())
        {
            NotReady = TEXT("Greenhouse coordinate readback waits for LOD0 vertex payload");
            return false;
        }
        const USceneComponent* RootComponent = Root->GetRootComponent();
        const FVector RootCenter = RootComponent->GetComponentLocation();
        const FQuat RootRotation = RootComponent->GetComponentQuat().GetNormalized();
        const double EffectiveScale = Planet->WorldScapePresentationScale * RootComponent->GetComponentScale().GetAbsMax();
        const double ExpectedInverseScale = EffectiveScale > 0.0 ? 1.0 / EffectiveScale : 0.0;
        const FVector Outward = (CameraWorld - RootCenter).GetSafeNormal();
        const FVector ReferenceOutward = FVector(-0.31, -0.86, 0.41).GetSafeNormal();
        UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Runtime] type=%d seed=%d radiusKm=%.17g feature=%.17g relief=%.17g land=%.17g presentation=%.17g rootRadius=%.17g rootRadiusCode=%.17g noiseScale=%.17g noiseIntensity=%.17g rootCenter=%s cameraWorld=%s naturalOutward=%s requestedReferenceOutward=%s outwardDot=%.17g; reference direction recorded only, no relocation"),
            static_cast<int32>(Planet->PlanetType), Planet->WorldScapeSeed, Planet->RadiusKM,
            Planet->SurfaceFeatureScale, Planet->SurfaceReliefScale, Planet->SurfaceLandCoverageScale,
            Planet->WorldScapePresentationScale, static_cast<double>(Root->PlanetScale), Root->PlanetScaleCode,
            static_cast<double>(Root->NoiseScale), static_cast<double>(Root->NoiseIntensity),
            *V(RootCenter), *V(CameraWorld), *V(Outward), *V(ReferenceOutward), FVector::DotProduct(Outward, ReferenceOutward));
        UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.ExpectedFrame] doubleCenter=%s inverseScale=%.17g axisX=%s axisY=%s axisZ=%s lodAnchorEcef=%s"),
            *V(RootCenter), ExpectedInverseScale, *V(RootRotation.GetAxisX()),
            *V(RootRotation.GetAxisY()), *V(RootRotation.GetAxisZ()), *V(Lod0->RelativePosition.ToFVector()));
        LogComponent(TEXT("root"), RootComponent);
        LogComponent(TEXT("transformKeeper"), Root->TransformKeeper);
        LogComponent(TEXT("terrainLod0"), Lod0->Mesh);
        LogAtmosphere(Surface->PlanetAtmosphere);
        const FTransform MeshTransform = Lod0->Mesh->GetComponentTransform();
        const TCHAR* Names[5] = { TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"),
            TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ") };
        for (int32 SectionIndex = 0; SectionIndex < Lod0->Mesh->GetNumSections(); ++SectionIndex)
        {
            const FWorldScapeMeshSection* Section = Lod0->Mesh->GetProcMeshSection(SectionIndex);
            if (!Section || Section->PlanetVertexBuffer.IsEmpty()) continue;
            UMaterialInterface* Material = Lod0->Mesh->GetMaterial(SectionIndex);
            FVector4d Frame[5] = {
                FVector4d(0, 0, 0, 0), FVector4d(0, 0, 0, 0), FVector4d(0, 0, 0, 0),
                FVector4d(0, 0, 0, 0), FVector4d(0, 0, 0, 0) };
            FString ReadValues[5];
            bool bFrameComplete = IsValid(Material);
            uint32 ReadMask = 0;
            for (int32 Param = 0; Param < 5; ++Param)
            {
                Frame[Param] = FVector4d(0.0, 0.0, 0.0, 0.0);
                const bool bRead = IsValid(Material)
                    && Material->GetDoubleVectorParameterValue(FMaterialParameterInfo(Names[Param]), Frame[Param]);
                if (bRead) ReadMask |= 1u << Param;
                ReadValues[Param] = bRead ? V4(Frame[Param]) : TEXT("MISSING");
                bFrameComplete &= bRead && FMath::IsFinite(Frame[Param].X) && FMath::IsFinite(Frame[Param].Y)
                    && FMath::IsFinite(Frame[Param].Z) && FMath::IsFinite(Frame[Param].W);
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.MID] section=%d material=%s dynamic=%d sharedDoubleReadMask=0x%x doubleCenter=%s inverseScale=%s axisX=%s axisY=%s axisZ=%s centerErrorCm=%.17g inverseScaleError=%.17g; absent parameters are evidence, not repaired"),
                SectionIndex, *GetPathNameSafe(Material), Cast<UMaterialInstanceDynamic>(Material) ? 1 : 0, ReadMask,
                *ReadValues[0], *ReadValues[1], *ReadValues[2], *ReadValues[3], *ReadValues[4],
                bFrameComplete ? FVector::Distance(XYZ(Frame[0]), RootCenter) : -1.0,
                bFrameComplete ? FMath::Abs(Frame[1].X - ExpectedInverseScale) : -1.0);
            FBox LocalBounds(ForceInit), WorldBounds(ForceInit), PhysicalBounds(ForceInit);
            double LocalFloatErrorMaxCm = 0.0, LocalFloatErrorSquareSum = 0.0;
            double WorldAfterLocalCastMaxCm = 0.0, HypotheticalWorldFloatMaxCm = 0.0;
            double PhysicalFloatMaxCm = 0.0;
            int32 ValidVertices = 0, NonFiniteVertices = 0;
            for (const FWorldScapeMeshVertex& Vertex : Section->PlanetVertexBuffer)
            {
                const FVector Local = Vertex.Position;
                const FVector LocalRoundTrip{FVector3f(Local)};
                const FVector World = MeshTransform.TransformPosition(Local);
                const FVector WorldAfterLocalCast = MeshTransform.TransformPosition(LocalRoundTrip);
                if (Local.ContainsNaN() || LocalRoundTrip.ContainsNaN() || World.ContainsNaN()
                    || WorldAfterLocalCast.ContainsNaN()) { ++NonFiniteVertices; continue; }
                ++ValidVertices;
                LocalBounds += Local;
                WorldBounds += World;
                const double LocalError = FVector::Distance(Local, LocalRoundTrip);
                LocalFloatErrorMaxCm = FMath::Max(LocalFloatErrorMaxCm, LocalError);
                LocalFloatErrorSquareSum += LocalError * LocalError;
                WorldAfterLocalCastMaxCm = FMath::Max(WorldAfterLocalCastMaxCm, FVector::Distance(World, WorldAfterLocalCast));
                HypotheticalWorldFloatMaxCm = FMath::Max(HypotheticalWorldFloatMaxCm, FVector::Distance(World, FVector(FVector3f(World))));
                if (bFrameComplete)
                {
                    const FVector Relative = (World - XYZ(Frame[0])) * Frame[1].X;
                    const FVector Physical(FVector::DotProduct(Relative, XYZ(Frame[2])),
                        FVector::DotProduct(Relative, XYZ(Frame[3])), FVector::DotProduct(Relative, XYZ(Frame[4])));
                    if (Physical.ContainsNaN()) { ++NonFiniteVertices; continue; }
                    PhysicalBounds += Physical;
                    PhysicalFloatMaxCm = FMath::Max(PhysicalFloatMaxCm, FVector::Distance(Physical, FVector(FVector3f(Physical))));
                }
            }
            UE_LOG(LogTemp, Display, TEXT("[APS.GreenhouseReference.Vertices] section=%d visible=%d vertices=%d indices=%d finite=%d nonfinite=%d localMin=%s localMax=%s worldMin=%s worldMax=%s physicalMin=%s physicalMax=%s localFloatRoundtripMaxCm=%.17g localFloatRoundtripRmsCm=%.17g worldErrorAfterLocalInputCastMaxCm=%.17g hypotheticalWorldFloatMaxCm=%.17g hypotheticalPhysicalFloatMaxCm=%.17g; CPU published section plus explicit float-cast model, NOT GPU buffer/shader readback; hypothetical terms do not prove active demotion"),
                SectionIndex, Section->bSectionVisible ? 1 : 0, Section->PlanetVertexBuffer.Num(), Section->PlanetIndexBuffer.Num(),
                ValidVertices, NonFiniteVertices, *V(LocalBounds.Min), *V(LocalBounds.Max), *V(WorldBounds.Min), *V(WorldBounds.Max),
                PhysicalBounds.IsValid ? *V(PhysicalBounds.Min) : TEXT("MISSING"),
                PhysicalBounds.IsValid ? *V(PhysicalBounds.Max) : TEXT("MISSING"), LocalFloatErrorMaxCm,
                ValidVertices ? FMath::Sqrt(LocalFloatErrorSquareSum / ValidVertices) : 0.0,
                WorldAfterLocalCastMaxCm, HypotheticalWorldFloatMaxCm, bFrameComplete ? PhysicalFloatMaxCm : -1.0);
        }
        return true;
    }
}
