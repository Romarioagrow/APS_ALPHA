#if WITH_DEV_AUTOMATION_TESTS
#include "APSSharedLavaCoverageProbe.h"
#include "APSProductionSharedLiquidAssertions.h"
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedWaterMaterial.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "APSLavaShorelineProbe.h"
#include "APSUnifiedLavaSurfaceProbe.h"

namespace APSSharedLavaCoverage
{
constexpr int32 DepthSize = 128;
constexpr double CameraClearanceCm = 50000.0;
const TCHAR* Labels[] = { TEXT("00-context0-alpha0"), TEXT("01-context0-alpha1"),
    TEXT("02-context1-alpha0"), TEXT("03-context1-alpha1") };
// The first pair (1 / 1.25) was captured before and after crust correction.
// This opt-in refinement brackets the remaining useful range; production is
// always captured first and never modified by this candidate-only comparison.
constexpr float RadianceValues[] = {1.5f, 1.75f};

// Do not hash raw struct padding. Alpha is deliberately excluded; every other
// actual vertex field and topology element must stay bit-for-bit unchanged.
uint32 PayloadHash(const FWorldScapeMeshSection& S)
{
    uint32 H = GetTypeHash(S.PlanetVertexBuffer.Num());
    for (const FWorldScapeMeshVertex& V : S.PlanetVertexBuffer)
    {
        H = HashCombine(H, GetTypeHash(V.Position));
        H = HashCombine(H, GetTypeHash(V.Normal));
        H = HashCombine(H, GetTypeHash(V.Tangent.TangentX));
        H = HashCombine(H, GetTypeHash(V.Tangent.bFlipTangentY));
        for (const FVector2D& UV : {V.UV0, V.UV1, V.UV2, V.UV3}) H = HashCombine(H, GetTypeHash(UV));
        H = HashCombine(H, GetTypeHash(FColor(V.Color.R, V.Color.G, V.Color.B, 255)));
    }
    H = HashCombine(H, GetTypeHash(S.PlanetIndexBuffer.Num()));
    for (uint32 I : S.PlanetIndexBuffer) H = HashCombine(H, I);
    H = HashCombine(H, GetTypeHash(S.bEnableCollision));
    return HashCombine(H, GetTypeHash(S.bSectionVisible));
}

struct FSlot
{
    TWeakObjectPtr<UWorldScapeMeshComponent> Mesh;
    int32 Index = INDEX_NONE;
    TArray<FColor> OriginalColors;
    TStrongObjectPtr<UMaterialInterface> OriginalMaterial{nullptr};
    FTransform Transform;
    uint32 Hash = 0;
};
struct FVisibility
{
    TWeakObjectPtr<UWorldScapeMeshComponent> Mesh;
    bool bVisible = false, bHidden = false;
};

class FProbe final : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<APlanet> Planet;
    TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
    TWeakObjectPtr<AWorldScapeRoot> Root;
    TWeakObjectPtr<APlayerController> PC;
    TWeakObjectPtr<AActor> OriginalViewTarget;
    TWeakObjectPtr<ACameraActor> Camera;
    TWeakObjectPtr<USceneCaptureComponent2D> DepthCapture;
    TStrongObjectPtr<UTextureRenderTarget2D> DepthTarget{nullptr};
    TStrongObjectPtr<UMaterialInstanceDynamic> Candidate{nullptr};
    TStrongObjectPtr<UMaterialInterface> OriginalRootMaterial{nullptr};
    TArray<FSlot> Slots;
    TArray<FVisibility> GroundVisibility;
    TArray<uint8> Coverage[4];
    FVector OceanPoint = FVector::ZeroVector;
    FTransform RootTransform;
    double Start = 0, Next = 0, LastShaderDiagnostic = -1000;
    uint64 SetFrame = 0;
    int32 Step = 0, Case = 0;
    int32 ProductionCaptureCount = 0;
    int32 RadianceCase = 0;
    bool bRadianceAB = false;
    float SavedBrightness = 0.0f;
    bool bLeased = false, bRestored = false, bDone = false;
    bool bRootTick = false, bSurfaceTick = false, bPlanetTick = false, bFrozen = false;
    bool bSharedLiquidFamilies = false;
    EAPSPlanetLiquidType LiquidType = EAPSPlanetLiquidType::Lava;

    const TCHAR* Marker() const { return bSharedLiquidFamilies ? TEXT("SHARED_LIQUID_COVERAGE") : TEXT("LAVA_COVERAGE"); }
    const TCHAR* ErrorPrefix() const { return bSharedLiquidFamilies ? TEXT("[APS.SharedLiquidCoverage] ") : TEXT("[APS.LavaCoverage] "); }
    const TCHAR* Family() const
    {
        return LiquidType == EAPSPlanetLiquidType::Water ? TEXT("Water")
            : LiquidType == EAPSPlanetLiquidType::Ammonia ? TEXT("Ammonia") : TEXT("Lava");
    }

    UMaterialInstanceDynamic* CreateSharedLiquidCandidate(UMaterialInstanceDynamic* SourceMID)
    {
        // Exact family parents matter: Water and Ammonia share a master, not
        // their saved style/chemistry. Never copy a legacy MID's uniform values.
        if (!IsValid(SourceMID) || !IsValid(SourceMID->Parent)) return nullptr;
        const bool bWater = LiquidType == EAPSPlanetLiquidType::Water;
        const TCHAR* ExpectedParent = bWater ? APSSharedWaterMaterial::TemplatePath() : APSSharedAmmoniaMaterial::TemplatePath();
        const TCHAR* LegacyParent = bWater
            ? TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water.MI_APS_WS_Water")
            : TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Ammonia.MI_APS_WS_Ammonia");
        const FString SourceParent = SourceMID->Parent->GetPathName();
        if ((SourceParent != LegacyParent && SourceParent != ExpectedParent)
            || Root->OceanMaterial.DefaultMaterial != SourceMID) return nullptr;
        for (const FSlot& Slot : Slots)
            if (Slot.OriginalMaterial.Get() != SourceMID) return nullptr;
        UMaterialInstanceDynamic* MID = bWater
            ? APSSharedWaterMaterial::CreateUnboundCandidate(Root.Get(), Root->GetRootComponent(),
                Planet->WorldScapePresentationScale, false, Surface->ResolvedSurfaceProfile, Planet->IsManual, Root->bOcean)
            : APSSharedAmmoniaMaterial::Create(Root.Get(), Root->GetRootComponent(),
                Planet->WorldScapePresentationScale, false, Planet->IsManual, LiquidType);
        if (!IsValid(MID) || !IsValid(MID->Parent) || MID->Parent->GetPathName() != ExpectedParent
            || !APSSharedAmmoniaMaterial::IsSharedStack(MID) || MID->GetBlendMode() != BLEND_Masked) return nullptr;
        Test->AddInfo(FString::Printf(TEXT("SHARED_LIQUID_COVERAGE_CANDIDATE family=%s sourceParent=%s candidateParent=%s parameterAuthority=savedCandidateMIC legacyParametersCopied=0; physical frame/context only"),
            Family(), *SourceParent, *MID->Parent->GetPathName()));
        return MID;
    }

    bool Finish(const FString& Failure = FString())
    {
        if (!Failure.IsEmpty()) Test->AddError(FString(ErrorPrefix()) + Failure);
        Restore();
        bDone = true;
        return true;
    }

    bool PayloadIsUnchanged() const
    {
        if (!Root.IsValid() || !Root->GetActorTransform().Equals(RootTransform, 0.0)
            || Root->WorldScapeLodInGeneration.Num() != 0
            || Root->OceanMaterial.DefaultMaterial != OriginalRootMaterial.Get()) return false;
        for (const FSlot& Slot : Slots)
        {
            if (!Slot.Mesh.IsValid() || !Slot.Mesh->GetComponentTransform().Equals(Slot.Transform, 0.0)) return false;
            const FWorldScapeMeshSection* S = Slot.Mesh->GetProcMeshSection(Slot.Index);
            if (!S || S->PlanetVertexBuffer.Num() != Slot.OriginalColors.Num()
                || PayloadHash(*S) != Slot.Hash) return false;
        }
        return true;
    }

    void Restore()
    {
        if (bRestored) return;
        bRestored = true;
        int32 RestoredVertices = 0, RestoredSlots = 0;
        TSet<UWorldScapeMeshComponent*> ChangedMeshes;
        for (FSlot& Slot : Slots)
        {
            if (!Slot.Mesh.IsValid()) { Test->AddError(FString(ErrorPrefix()) + TEXT("mesh disappeared before restore")); continue; }
            FWorldScapeMeshSection* S = Slot.Mesh->GetProcMeshSection(Slot.Index);
            if (!S || S->PlanetVertexBuffer.Num() != Slot.OriginalColors.Num() || PayloadHash(*S) != Slot.Hash)
            {
                // Never overwrite a new/external generation's geometry or colors.
                Test->AddError(FString(ErrorPrefix()) + TEXT("payload changed; refusing to overwrite a foreign section"));
                continue;
            }
            for (int32 I = 0; I < S->PlanetVertexBuffer.Num(); ++I)
            {
                S->PlanetVertexBuffer[I].Color = Slot.OriginalColors[I];
                ++RestoredVertices;
            }
            Slot.Mesh->SetMaterial(Slot.Index, Slot.OriginalMaterial.Get());
            ++RestoredSlots;
            ChangedMeshes.Add(Slot.Mesh.Get());
        }
        for (UWorldScapeMeshComponent* Mesh : ChangedMeshes) Mesh->MarkRenderStateDirty();
        Test->TestEqual(TEXT("Every original ocean slot/color buffer was restored"), RestoredSlots, Slots.Num());
        for (const FVisibility& V : GroundVisibility)
            if (V.Mesh.IsValid()) { V.Mesh->SetVisibility(V.bVisible, false); V.Mesh->SetHiddenInGame(V.bHidden, false); }
        if (PC.IsValid() && Camera.IsValid() && PC->GetViewTarget() == Camera.Get() && OriginalViewTarget.IsValid())
            PC->SetViewTarget(OriginalViewTarget.Get());
        if (DepthCapture.IsValid()) DepthCapture->DestroyComponent();
        if (Camera.IsValid()) Camera->Destroy();
        if (bLeased)
        {
            if (Root.IsValid()) { Root->bFreezeGeneration = bFrozen; Root->SetActorTickEnabled(bRootTick); }
            if (Surface.IsValid()) Surface->SetActorTickEnabled(bSurfaceTick);
            if (Planet.IsValid()) Planet->SetActorTickEnabled(bPlanetTick);
        }
        Test->AddInfo(FString::Printf(TEXT("%s_RESTORE slots=%d/%d colors=%d cameraRestored=%d family=%s"),
            Marker(), RestoredSlots, Slots.Num(), RestoredVertices, PC.IsValid() && PC->GetViewTarget() == OriginalViewTarget.Get() ? 1 : 0, Family()));
    }

    bool Snapshot()
    {
        AWorldScapeRoot* R = Root.Get();
        APawn* Pawn = PC.IsValid() ? PC->GetPawn() : nullptr;
        if (!R || !Pawn || R->WorldScapeLodOcean.Num() != R->OceanMaxLod) return false;
        double BestDistance = TNumericLimits<double>::Max();
        int32 TotalVertices = 0;
        for (const UWorldScapeLod* Lod : R->WorldScapeLodOcean)
        {
            if (!IsValid(Lod) || !IsValid(Lod->Mesh) || Lod->Mesh->GetNumSections() != 3) return false;
            for (int32 SectionIndex = 0; SectionIndex < 3; ++SectionIndex)
            {
                const FWorldScapeMeshSection* S = Lod->Mesh->GetProcMeshSection(SectionIndex);
                if (!S) return false;
                FSlot& Slot = Slots.AddDefaulted_GetRef();
                Slot.Mesh = Lod->Mesh; Slot.Index = SectionIndex;
                Slot.OriginalMaterial.Reset(Lod->Mesh->GetMaterial(SectionIndex));
                Slot.Transform = Lod->Mesh->GetComponentTransform(); Slot.Hash = PayloadHash(*S);
                for (const FWorldScapeMeshVertex& V : S->PlanetVertexBuffer) Slot.OriginalColors.Add(V.Color);
                TotalVertices += Slot.OriginalColors.Num();
                if (Lod->Lod != 0 || !S->bSectionVisible) continue;
                // Aim at an actual nondegenerate LOD0 triangle, not a synthetic plane.
                for (int32 I = 0; I + 2 < S->PlanetIndexBuffer.Num(); I += 3)
                {
                    const uint32 A = S->PlanetIndexBuffer[I], B = S->PlanetIndexBuffer[I+1], C = S->PlanetIndexBuffer[I+2];
                    if (!S->PlanetVertexBuffer.IsValidIndex(A) || !S->PlanetVertexBuffer.IsValidIndex(B)
                        || !S->PlanetVertexBuffer.IsValidIndex(C)) return false;
                    const FVector P0 = S->PlanetVertexBuffer[A].Position, P1 = S->PlanetVertexBuffer[B].Position, P2 = S->PlanetVertexBuffer[C].Position;
                    if (FVector::CrossProduct(P1-P0, P2-P0).SizeSquared() < 1.0) continue;
                    const FVector P = Slot.Transform.TransformPosition((P0+P1+P2)/3.0);
                    const double Distance = FVector::DistSquared(P, Pawn->GetActorLocation());
                    if (Distance < BestDistance) { BestDistance = Distance; OceanPoint = P; }
                }
            }
        }
        if (BestDistance == TNumericLimits<double>::Max() || TotalVertices == 0) return false;
        for (const UWorldScapeLod* Lod : R->WorldScapeLod)
            if (IsValid(Lod) && IsValid(Lod->Mesh))
            {
                FVisibility& Saved = GroundVisibility.AddDefaulted_GetRef();
                Saved.Mesh = Lod->Mesh; Saved.bVisible = Lod->Mesh->IsVisible(); Saved.bHidden = !!Lod->Mesh->bHiddenInGame;
            }
        RootTransform = R->GetActorTransform();
        OriginalRootMaterial.Reset(R->OceanMaterial.DefaultMaterial);
        Test->AddInfo(FString::Printf(TEXT("%s_PAYLOAD root=%s lods=%d slots=%d vertices=%d source=%s frame=%llu family=%s"),
            Marker(), *R->GetPathName(), R->WorldScapeLodOcean.Num(), Slots.Num(), TotalVertices,
            *GetPathNameSafe(Surface->ResolvedOceanMaterialInstance), static_cast<unsigned long long>(GFrameCounter), Family()));
        return true;
    }

    bool PrepareCamera()
    {
        APlayerController* Controller = PC.Get();
        if (!Controller || !Controller->PlayerCameraManager) return false;
        OriginalViewTarget = Controller->GetViewTarget();
        const FVector Up = (OceanPoint - Root->GetActorLocation()).GetSafeNormal();
        if (Up.IsNearlyZero()) return false;
        FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACameraActor* NewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
            OceanPoint + Up * CameraClearanceCm, (-Up).Rotation(), Params);
        Camera = NewCamera;
        if (!NewCamera || !NewCamera->GetCameraComponent()) return false;
        const FMinimalViewInfo& Natural = Controller->PlayerCameraManager->GetCameraCacheView();
        UCameraComponent* Optical = NewCamera->GetCameraComponent();
        Optical->SetFieldOfView(Natural.FOV); Optical->SetAspectRatio(Natural.AspectRatio);
        Optical->SetConstraintAspectRatio(Natural.bConstrainAspectRatio);
        Optical->PostProcessSettings = Natural.PostProcessSettings;
        Optical->PostProcessBlendWeight = Natural.PostProcessBlendWeight;
        Controller->SetViewTarget(NewCamera);
        // Explicit isolation only. The natural-route fixture has three preceding
        // unchanged scene frames; the opt-in Lava orbital route does not claim
        // those frames. No light/exposure/foliage/material-style edits here.
        for (const FVisibility& V : GroundVisibility) if (V.Mesh.IsValid()) V.Mesh->SetVisibility(false, false);
        auto* Capture = NewObject<USceneCaptureComponent2D>(NewCamera);
        if (!Capture) return false;
        DepthCapture = Capture;
        Capture->SetupAttachment(NewCamera->GetRootComponent()); NewCamera->AddInstanceComponent(Capture);
        Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
        Capture->bAlwaysPersistRenderingState = false;
        Capture->CaptureSource = SCS_SceneDepth;
        Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        Capture->FOVAngle = Natural.FOV;
        Capture->ShowFlags.SetTemporalAA(false); // Auxiliary depth measurement only.
        TSet<UWorldScapeMeshComponent*> OceanMeshes;
        for (const FSlot& Slot : Slots) OceanMeshes.Add(Slot.Mesh.Get());
        for (UWorldScapeMeshComponent* Mesh : OceanMeshes) Capture->ShowOnlyComponent(Mesh);
        UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(NewCamera);
        if (!Target) return false;
        DepthTarget.Reset(Target);
        Target->ClearColor = FLinearColor::Black;
        Target->InitCustomFormat(DepthSize, DepthSize, PF_A32B32G32R32F, false);
        Target->UpdateResourceImmediate(true); Capture->TextureTarget = Target;
        Capture->RegisterComponent();
        return true;
    }

    bool CaptureProductionBaseline()
    {
        if (!PayloadIsUnchanged() || !Surface.IsValid() || !PC.IsValid() || PC->GetViewTarget() != Camera.Get()) return false;
        auto* Material = Surface->ResolvedOceanMaterialInstance;
        if (Material != OriginalRootMaterial.Get()) return false;
        FString Evidence, Error;
        if (!APSProductionSharedLiquidAssertions::Validate(Material, LiquidType, Root->GetRootComponent(),
            Planet->WorldScapePresentationScale, 0.0f, Evidence, Error))
        { Test->AddError(TEXT("PHYSICAL_PRODUCTION_LIQUID: ") + Error); return false; }
        int32 Vertices = 0;
        for (const FSlot& Slot : Slots)
        {
            if (Slot.OriginalMaterial.Get() != Material || Slot.Mesh->GetMaterial(Slot.Index) != Material) return false;
            const FWorldScapeMeshSection* Section = Slot.Mesh->GetProcMeshSection(Slot.Index);
            for (int32 I = 0; I < Slot.OriginalColors.Num(); ++I)
                if (Section->PlanetVertexBuffer[I].Color != Slot.OriginalColors[I]) return false;
            Vertices += Slot.OriginalColors.Num();
        }
        UGameViewportClient* Client = World->GetGameViewport();
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Client || !Client->GetGameViewportWidget().IsValid()
            || !FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y) return false;
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("Automation/SharedLiquidCoverage")
            / UEnum::GetValueAsString(Planet->PlanetType).Replace(TEXT("::"), TEXT("_"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        const FString Path = Folder / FString::Printf(TEXT("production-%02d-original-material-rgba.png"), ProductionCaptureCount);
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *Path)) return false;
        Test->AddInfo(FString::Printf(TEXT("PHYSICAL_PRODUCTION_LIQUID phase=%d family=%s noSubstitution=1 originalRGBA=1 slots=%d vertices=%d rootAndPayloadStable=1 camera=isolatedCoverage500m groundHidden=1 %s screenshot=%s"),
            ProductionCaptureCount, Family(), Slots.Num(), Vertices, *Evidence, *Path));
        return true;
    }

    bool ApplyRadianceCase()
    {
        if (!PayloadIsUnchanged() || LiquidType != EAPSPlanetLiquidType::Lava
            || !Candidate.IsValid() || !PC.IsValid() || PC->GetViewTarget() != Camera.Get()
            || RadianceCase < 0 || RadianceCase >= UE_ARRAY_COUNT(RadianceValues)) return false;
        // Test-only candidate; the production MID, saved MIC and original RGBA
        // remain untouched. Optics and camera are identical to the two baselines.
        Candidate->SetScalarParameterValue(TEXT("Brightness"), RadianceValues[RadianceCase]);
        for (const FSlot& Slot : Slots) Slot.Mesh->SetMaterial(Slot.Index, Candidate.Get());
        SetFrame = GFrameCounter; Next = FPlatformTime::Seconds() + 1.0;
        return true;
    }

    bool CaptureRadianceCase()
    {
        if (!PayloadIsUnchanged() || !PC.IsValid() || PC->GetViewTarget() != Camera.Get()) return false;
        float Actual = -1.0f, Production = -1.0f;
        if (!Candidate->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Actual)
            || Actual != RadianceValues[RadianceCase]
            || !OriginalRootMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), Production)
            || Production != SavedBrightness) return false;
        for (const FSlot& Slot : Slots)
        {
            if (Slot.Mesh->GetMaterial(Slot.Index) != Candidate.Get()) return false;
            const FWorldScapeMeshSection* Section = Slot.Mesh->GetProcMeshSection(Slot.Index);
            for (int32 I = 0; I < Slot.OriginalColors.Num(); ++I)
                if (Section->PlanetVertexBuffer[I].Color != Slot.OriginalColors[I]) return false;
        }
        UGameViewportClient* Client = World->GetGameViewport();
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Client || !Client->GetGameViewportWidget().IsValid()
            || !FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y) return false;
        const FString Folder = FPaths::ProjectSavedDir() / TEXT("Automation/LavaRadianceAB")
            / UEnum::GetValueAsString(Planet->PlanetType).Replace(TEXT("::"), TEXT("_"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        const FString Path = Folder / FString::Printf(TEXT("candidate-%02d-brightness-%.2f.png"), RadianceCase, Actual);
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *Path)) return false;
        Test->AddInfo(FString::Printf(TEXT("LAVA_RADIANCE_AB brightness=%.2f productionBrightness=%.2f originalRGBA=1 cameraUnchanged=1 savedAssetsUntouched=1 screenshot=%s"), Actual, Production, *Path));
        return true;
    }

    bool ApplyCase()
    {
        if (!PayloadIsUnchanged()) return false;
        Candidate->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), Case >= 2 ? 1.0f : 0.0f);
        TSet<UWorldScapeMeshComponent*> Changed;
        for (const FSlot& Slot : Slots)
        {
            FWorldScapeMeshSection* S = Slot.Mesh->GetProcMeshSection(Slot.Index);
            for (FWorldScapeMeshVertex& V : S->PlanetVertexBuffer) V.Color.A = (Case % 2) ? 255 : 0;
            Slot.Mesh->SetMaterial(Slot.Index, Candidate.Get()); Changed.Add(Slot.Mesh.Get());
        }
        for (UWorldScapeMeshComponent* Mesh : Changed) Mesh->MarkRenderStateDirty();
        SetFrame = GFrameCounter; Next = FPlatformTime::Seconds() + 0.75;
        return PayloadIsUnchanged();
    }

    bool CaptureCase()
    {
        if (!PayloadIsUnchanged() || !DepthCapture.IsValid() || !DepthTarget.IsValid()
            || !PC.IsValid() || PC->GetViewTarget() != Camera.Get()) return false;
        for (const FSlot& Slot : Slots) if (Slot.Mesh->GetMaterial(Slot.Index) != Candidate.Get()) return false;
        DepthCapture->CaptureScene();
        TArray<FLinearColor> Depth;
        if (!DepthTarget->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Depth)
            || Depth.Num() != DepthSize*DepthSize) return false;
        Coverage[Case].SetNumZeroed(Depth.Num()); TArray<FColor> Mask; Mask.Reserve(Depth.Num());
        int32 Covered = 0;
        for (int32 I = 0; I < Depth.Num(); ++I)
        {
            // Linear scene depth in cm from the real LocalVF draw. Only actual
            // ocean primitives are in this auxiliary pass; seabed cannot fake it.
            const bool bCovered = FMath::IsFinite(Depth[I].R) && Depth[I].R > 1.0f
                && Depth[I].R < CameraClearanceCm * 4.0;
            Coverage[Case][I] = bCovered ? 1 : 0; Covered += bCovered ? 1 : 0;
            Mask.Add(bCovered ? FColor::White : FColor::Black);
        }
        const FString Folder = FPaths::ProjectSavedDir()
            / (bSharedLiquidFamilies ? TEXT("Automation/SharedLiquidCoverage") : TEXT("Automation/LavaCoverage"))
            / UEnum::GetValueAsString(Planet->PlanetType).Replace(TEXT("::"), TEXT("_"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(DepthSize, DepthSize, Mask, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *(Folder / (FString(Labels[Case]) + TEXT("-depth-mask.png"))))) return false;
        UGameViewportClient* Client = World->GetGameViewport();
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Client || !Client->GetGameViewportWidget().IsValid()
            || !FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X*Size.Y) return false;
        Png.Reset(); FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
        if (!FFileHelper::SaveArrayToFile(Png, *(Folder / (FString(Labels[Case]) + TEXT("-isolated-viewport.png"))))) return false;
        Test->AddInfo(FString::Printf(TEXT("%s phase=%s covered=%d/%d actualWorldScapeSlots=%d geometryRGBUnchanged=1 parent=%s camera=%s family=%s"),
            Marker(), Labels[Case], Covered, Depth.Num(), Slots.Num(), *GetPathNameSafe(Candidate->Parent), *Camera->GetActorLocation().ToCompactString(), Family()));
        return true;
    }

public:
    FProbe(FAutomationTestBase* InTest, UWorld* InWorld, APlanet* InPlanet, bool bInSharedLiquidFamilies = false)
        : Test(InTest), World(InWorld), Planet(InPlanet), bSharedLiquidFamilies(bInSharedLiquidFamilies) {}
    ~FProbe() override { Restore(); }

    bool Update() override
    {
        if (bDone) return true;
        const double Now = FPlatformTime::Seconds(); if (Start == 0) Start = Now;
        if (Now-Start > 100) return Finish(TEXT("100s timeout; no coverage acceptance"));
        if (!World.IsValid() || !Planet.IsValid()) return Finish(TEXT("production world/planet disappeared"));
        if (Step > 0 && (!Root.IsValid() || !Surface.IsValid() || !PC.IsValid()
            || Surface->WorldScapeRootInstance != Root.Get())) return Finish(TEXT("leased production root was replaced"));
        if (Step == 0)
        {
            bRadianceAB = FParse::Param(FCommandLine::Get(), TEXT("APSLavaRadianceAB"));
            if (bRadianceAB && bSharedLiquidFamilies) return Finish(TEXT("radiance A/B is Lava-only"));
            Surface = Planet->PlanetaryEnvironmentGenerator;
            Root = Surface.IsValid() ? Surface->WorldScapeRootInstance : nullptr;
            PC = World->GetFirstPlayerController();
            if (Surface.IsValid()) LiquidType = Surface->ResolvedSurfaceProfile.LiquidType;
            const bool bAllowedFamily = bSharedLiquidFamilies
                ? LiquidType == EAPSPlanetLiquidType::Water || LiquidType == EAPSPlanetLiquidType::Ammonia
                : LiquidType == EAPSPlanetLiquidType::Lava;
            if (!Surface.IsValid() || !Root.IsValid() || !PC.IsValid() || Planet->IsManual
                || !Planet->bWorldScapeSurfaceReady || !Root->bOcean || Root->IsHidden()
                || Planet->WorldScapePresentationScale < 0.999
                || !bAllowedFamily)
                return Finish(TEXT("requires an actual ready generated full-scale ocean of the opted-in liquid family"));
            if (Root->bGenerateFoliages && !Root->Foliages.IsEmpty())
                return Finish(TEXT("refusing a foliage-active root without a public foliage-worker drain"));
            bRootTick = Root->IsActorTickEnabled(); bFrozen = Root->bFreezeGeneration;
            bSurfaceTick = Surface->IsActorTickEnabled(); bPlanetTick = Planet->IsActorTickEnabled();
            bLeased = true; Root->bFreezeGeneration = true; Root->SetActorTickEnabled(false);
            Surface->SetActorTickEnabled(false); Planet->SetActorTickEnabled(false);
            Step = 1;
        }
        if (Step == 1)
        {
            if (Root->WorldScapeLodInGeneration.Num() > 0) { Root->CheckForLodGeneration(); return false; }
            if (!Snapshot()) return Finish(TEXT("incomplete actual ocean topology/slots"));
            UMaterialInstanceDynamic* SourceMID = Surface->ResolvedOceanMaterialInstance;
            if (bSharedLiquidFamilies)
            {
                Candidate.Reset(CreateSharedLiquidCandidate(SourceMID));
                if (!Candidate.IsValid()) return Finish(TEXT("exact Water/Ammonia source slots or saved shared candidate invalid; no legacy/style fallback"));
            }
            else
            {
                // Production now owns the same saved Lava MIC as PLANET.
                if (!IsValid(SourceMID) || !IsValid(SourceMID->Parent)
                    || (!APSSharedLavaMaterial::IsSharedStack(SourceMID) && SourceMID->Parent->GetPathName() !=
                        TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Lava.MI_APS_WS_Lava")))
                    return Finish(TEXT("not the exact generated APS/native Lava source; refusing a custom material"));
                UMaterialInstanceDynamic* MID = APSSharedLavaMaterial::Create(Root.Get(), Root->GetRootComponent(),
                    Planet->WorldScapePresentationScale, false, false, EAPSPlanetLiquidType::Lava);
                if (!MID) return Finish(TEXT("shared candidate missing"));
                Candidate.Reset(MID);
                // Do not inherit stale legacy zero-emission or frame overrides.
                MID->SetScalarParameterValue(TEXT("APS_UsePresentationWaterMask"), 0.0f);
                if (!APSSharedLavaMaterial::BindFrame(MID, Root->GetRootComponent(), Planet->WorldScapePresentationScale))
                    return Finish(TEXT("candidate physical frame invalid"));
            }
            UMaterialInstanceDynamic* MID = Candidate.Get();
            if (bRadianceAB && (!SourceMID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), SavedBrightness)
                || !FMath::IsFinite(SavedBrightness) || SavedBrightness < 0.0f))
                return Finish(TEXT("radiance A/B requires finite saved production brightness"));
            FMaterialResource* Resource = MID->GetMaterialResource(World->GetFeatureLevel());
#if WITH_EDITOR
            if (Resource && !Resource->IsGameThreadShaderMapComplete()) Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
            FPSOPrecacheParams Params; Slots[0].Mesh->SetupPrecachePSOParams(Params);
            Params.bStaticLighting = false; Params.SetMobility(EComponentMobility::Movable);
            static_cast<UMaterialInterface*>(MID)->PrecachePSOs(&FLocalVertexFactory::StaticType, Params);
            Step = 2;
        }
        if (Step == 2)
        {
            FMaterialResource* Resource = Candidate->GetMaterialResource(World->GetFeatureLevel());
            const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (Resource && Resource->GetCompileErrors().Num()) return Finish(TEXT("candidate shader compilation error"));
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            {
                if (Now-LastShaderDiagnostic > 5)
                {
                    LastShaderDiagnostic = Now;
                    Test->AddInfo(FString::Printf(TEXT("%s_PENDING elapsed=%.2f frame=%llu material=%s feature=%d resource=%d map=%d complete=%d localVF=%d family=%s"),
                        Marker(), Now-Start, static_cast<unsigned long long>(GFrameCounter), *Candidate->GetPathName(), static_cast<int32>(World->GetFeatureLevel()),
                        Resource ? 1 : 0, Map ? 1 : 0, Resource && Resource->IsGameThreadShaderMapComplete() ? 1 : 0,
                        Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType) ? 1 : 0, Family()));
                }
                return false;
            }
            if (!PrepareCamera()) return Finish(TEXT("failed to lease the coverage camera"));
            // Original production slots/RGBA remain untouched. Capture twice
            // BEFORE candidate ApplyCase for all liquids, including Lava.
            Step = 4; SetFrame = GFrameCounter; Next = Now + 1.0;
            return false;
        }
        if (Now < Next || GFrameCounter-SetFrame < 8) return false;
        if (Step == 4)
        {
            if (!CaptureProductionBaseline()) return Finish(TEXT("actual production shared liquid baseline/frame/slot assertion failed before candidate substitution"));
            if (++ProductionCaptureCount < 2) { SetFrame = GFrameCounter; Next = Now + 1.0; return false; }
            if (bRadianceAB)
            {
                if (!ApplyRadianceCase()) return Finish(TEXT("failed to begin isolated radiance A/B"));
                Step = 5;
                return false;
            }
            if (!ApplyCase()) return Finish(TEXT("failed to begin matrix after production baseline"));
            Step = 3;
            return false;
        }
        if (Step == 5)
        {
            if (!CaptureRadianceCase()) return Finish(TEXT("radiance A/B capture or original parameter/payload continuity failed"));
            if (++RadianceCase < UE_ARRAY_COUNT(RadianceValues))
            {
                if (!ApplyRadianceCase()) return Finish(TEXT("failed to advance isolated radiance A/B"));
                return false;
            }
            Candidate->SetScalarParameterValue(TEXT("Brightness"), SavedBrightness);
            if (!ApplyCase()) return Finish(TEXT("failed to begin original-brightness coverage matrix after radiance A/B"));
            Step = 3;
            return false;
        }
        if (!CaptureCase()) return Finish(TEXT("capture/payload/slot continuity failed"));
        if (++Case < 4)
        {
            if (!ApplyCase()) return Finish(TEXT("non-alpha payload changed between cases"));
            return false;
        }
        int32 Opaque = 0, Masked = 0, DeltaA = 0, DeltaContext = 0;
        for (int32 I = 0; I < Coverage[0].Num(); ++I)
        {
            Opaque += Coverage[0][I]; Masked += Coverage[2][I];
            DeltaA += Coverage[0][I] != Coverage[1][I] ? 1 : 0;
            DeltaContext += Coverage[0][I] != Coverage[3][I] ? 1 : 0;
        }
        Test->TestTrue(FString::Printf(TEXT("Actual %s context0/A0 has nontrivial rendered coverage"), Family()), Opaque >= DepthSize*DepthSize/4);
        Test->TestEqual(TEXT("Gameplay Hole A0 vs A1 rendered coverage is identical"), DeltaA, 0);
        Test->TestEqual(TEXT("Presentation context1/A0 positive control is fully masked"), Masked, 0);
        Test->TestEqual(TEXT("Presentation context1/A1 restores the same rendered coverage"), DeltaContext, 0);
        Test->AddInfo(FString::Printf(TEXT("%s matrices are actual LocalVF SceneDepth; color captures are terrain-hidden isolation, not natural-scene or FPS acceptance; family=%s"), Marker(), Family()));
        return Finish(Opaque >= DepthSize*DepthSize/4 && DeltaA == 0 && Masked == 0 && DeltaContext == 0
            ? FString() : FString::Printf(TEXT("rendered %s coverage matrix failed"), Family()));
    }
};
}

TSharedPtr<IAutomationLatentCommand> APSCreateSharedLavaCoverageProbe(
    FAutomationTestBase* Test, UWorld* World, APlanet* Planet)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("APSUnifiedLavaProbe")))
        return MakeShared<APSUnifiedLavaProbe::FProbe>(Test, World, Planet);
    if (FParse::Param(FCommandLine::Get(), TEXT("APSLavaShoreline")))
        return MakeShared<APSLavaShoreline::FProbe>(Test, World, Planet);
    return MakeShared<APSSharedLavaCoverage::FProbe>(Test, World, Planet);
}

TSharedPtr<IAutomationLatentCommand> APSCreateSharedLiquidCoverageProbe(
    FAutomationTestBase* Test, UWorld* World, APlanet* Planet)
{
    return MakeShared<APSSharedLavaCoverage::FProbe>(Test, World, Planet, true);
}
#endif
