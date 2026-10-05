#include "APSPlanetReliefRuntime.h"
#include "APSTerrainContinuityMaterial.h"
#include "APSCanonicalCoverageTexture.h"
#include "APS_ALPHA/Generation/APSCanonicalCoverageLayout.h"
#include "APS_ALPHA/Generation/APSCanonicalFilteredHeight.h"
#include "APS_ALPHA/Generation/APSNativeGlobeSnapshot.h"
#include "Async/Async.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderCommandFence.h"
#include "UObject/StrongObjectPtr.h"
#include <atomic>

namespace APSPlanetReliefPrivate
{
    TAutoConsoleVariable<int32> Enabled(TEXT("aps.Surface.CanonicalRelief"), 0,
        TEXT("Unpromoted fixed-front canonical lighting cache. 0=off; no moving/global coverage guarantee."), ECVF_Default);
    constexpr int32 Slots = APSCanonicalCoverageLayout::MaximumLevels, Vectors = Slots + 3;
    constexpr int64 LevelBytes = 11184808, RecordBudget = 96ll * 1024 * 1024, TotalBudget = 192ll * 1024 * 1024;
    // Logical texture payload budget, including the reserved worker result. Not a
    // bound on driver staging, UObject/platform duplication or CPU filter scratch.
    FName TextureName(int32 I) { return FName(*FString::Printf(TEXT("APS_CanonicalCoverageTexture%d"), I)); }
    FName AvailableName() { return TEXT("APS_CanonicalCoverageAvailable"); }
    FName VectorName(int32 I)
    {
        static const FName Names[] = {TEXT("APS_CanonicalCoverageCenter"), TEXT("APS_CanonicalCoverageU"), TEXT("APS_CanonicalCoverageV")};
        return I < 3 ? Names[I] : FName(*FString::Printf(TEXT("APS_CanonicalCoverageMetrics%d"), I - 3));
    }
    struct FReliefCacheKey
    {
        uint32 Signature = 0, Profile = 0;
        int32 Seed = 0, TerrainSeed = 0, BiomeSeed = 0;
        double Radius = 0, Scale = 0, Intensity = 0, Presentation = 0, Ocean = 0;
        bool Coast = false, Lava = false;
        FTransform RootInBody = FTransform::Identity;
        bool operator==(const FReliefCacheKey& B) const
        {
            return Signature == B.Signature && Profile == B.Profile && Seed == B.Seed
                && TerrainSeed == B.TerrainSeed && BiomeSeed == B.BiomeSeed && Radius == B.Radius
                && Scale == B.Scale && Intensity == B.Intensity && Presentation == B.Presentation
                && Ocean == B.Ocean && Coast == B.Coast && Lava == B.Lava
                && RootInBody.GetTranslation().Equals(B.RootInBody.GetTranslation(),1.e-3)
                && RootInBody.GetRotation().Equals(B.RootInBody.GetRotation(),1.e-10);
        }
    };
    struct FSubscriber
    {
        TWeakObjectPtr<UMaterialInstanceDynamic> MID;
        TStrongObjectPtr<UTexture> OldTextures[Slots];
        FLinearColor OldVectors[Vectors];
        float OldAvailable = 0, LastAvailable = 0;
        int32 Stage = 0; // 0 unbound, 1 disabled-bind fence, 2 smooth enable.
        double EnableAt = 0;
        FRenderCommandFence Fence;
        bool bConflict = false;
    };
    struct FRecord
    {
        TWeakObjectPtr<APlanetaryBody> Body;
        FReliefCacheKey Key;
        APSClosedGlobeMesh::FSamplingFrame Frame;
        APSCanonicalCoverageLayout::FLayout Layout;
        FVector3d RequestedAnchor = FVector3d::ZeroVector;
        TArray<TSharedPtr<FSubscriber>> Subscribers;
        TStrongObjectPtr<UTexture2D> Textures[Slots];
        FLinearColor Values[Vectors];
        FRenderCommandFence UploadFence;
        int32 Built = 0;
        bool bUploadPending = false, bFailed = false;
        double LastIdentityCheck = 0;
    };
    struct FWorldState
    {
        TArray<TSharedPtr<FRecord>> Records;
        TWeakObjectPtr<APlanetaryBody> Active, Arriving;
    };
    struct FResult { APSCanonicalFilteredHeight::FData Data; FString Error; bool bOK = false; };
    struct FJob
    {
        TWeakPtr<FRecord> Record;
        TWeakObjectPtr<UWorld> World;
        TSharedPtr<std::atomic_bool, ESPMode::ThreadSafe> Cancel;
        TFuture<FResult> Future;
        int32 Level = 0;
    };
    TMap<TWeakObjectPtr<UWorld>, FWorldState> Worlds;
    TUniquePtr<FJob> Job; // GLOBAL, not one worker per world. Cancelled work retains its slot until done.
    uint64 LastUploadFrame = MAX_uint64;

    bool Graph(UMaterialInstanceDynamic* M)
    {
        if (!IsValid(M) || !M->GetMaterial() || !M->Parent
            || M->GetMaterial()->GetPathName() != APSTerrainContinuityMaterial::MasterPath
            || M->Parent->GetPathName() != APSTerrainContinuityMaterial::TemplatePath) return false;
        float A = 0, Start = 0, End = 0;
        if (!M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), A) || !FMath::IsFinite(A)
            || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), Start)
            || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), End)
            || !FMath::IsFinite(Start) || !FMath::IsFinite(End) || Start <= 0 || End <= Start) return false;
        for (int32 I = 0; I < Slots; ++I) { UTexture* T = nullptr;
            if (!M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)), T) || !IsValid(T)) return false; }
        for (int32 I = 0; I < Vectors; ++I) { FLinearColor V;
            if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), V)) return false; }
        return true;
    }
    bool ShaderReady(UMaterialInstanceDynamic* M, UWorld* W)
    {
        if (!Graph(M) || !W) return false;
        auto* R = M->GetMaterialResource(W->GetFeatureLevel());
        const auto* Map = R ? R->GetGameThreadShaderMap() : nullptr;
#if WITH_EDITOR
        if (R && !R->GetCompileErrors().IsEmpty()) return false;
#endif
        return R && R->IsGameThreadShaderMapComplete() && Map && Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
    }
    UTexture2D* BoundTexture(const FRecord& R, int32 I) { return R.Textures[FMath::Min(I, R.Layout.Count - 1)].Get(); }
    bool Current(const FRecord& R)
    {
        auto* B = R.Body.Get(); auto* G = B ? B->PlanetaryEnvironmentGenerator : nullptr;
        if (!IsValid(B) || !IsValid(G) || B->GetActorTransform().ContainsNaN()
            || !B->GetActorScale3D().Equals(FVector::OneVector,1.e-9)) return false;
        if (const auto* Root=G->WorldScapeRootInstance; IsValid(Root))
        {
            const FTransform Relative=Root->GetActorTransform().GetRelativeTransform(B->GetActorTransform());
            if (Relative.ContainsNaN() || !Root->GetActorScale3D().Equals(FVector::OneVector,1.e-9)
                || !Relative.GetTranslation().Equals(R.Key.RootInBody.GetTranslation(),1.e-3)
                || !Relative.GetRotation().Equals(R.Key.RootInBody.GetRotation(),1.e-10)) return false;
        }
        return B->WorldScapeSeed == R.Key.Seed
            && B->WorldScapePresentationScale == 1.0
            && APSNativeGlobeSnapshot::Identity(B, G->SurfaceProfileCatalog) == R.Key.Signature;
    }
    bool Owned(const FRecord& R, const FSubscriber& S)
    {
        auto* M = S.MID.Get(); float A = -1;
        if (!Graph(M) || !M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), A) || A != S.LastAvailable) return false;
        for (int32 I = 0; I < Slots; ++I) { UTexture* T = nullptr;
            if (!M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)), T) || T != BoundTexture(R, I)) return false; }
        for (int32 I = 0; I < Vectors; ++I) { FLinearColor V;
            if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), V) || V != R.Values[I]) return false; }
        return true;
    }
    void Restore(FRecord& R)
    {
        for (auto& S : R.Subscribers) if (S->Stage && Graph(S->MID.Get()))
        {
            auto* M = S->MID.Get(); float A=-1;
            if (M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()),A) && A==S->LastAvailable)
                M->SetScalarParameterValue(AvailableName(),S->OldAvailable);
            // Do not clear broad overrides or overwrite a later external value.
            for (int32 I=0; I<Slots; ++I) { UTexture* T=nullptr;
                if (M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)),T) && T==BoundTexture(R,I))
                    M->SetTextureParameterValue(TextureName(I),S->OldTextures[I].Get()); }
            for (int32 I=0; I<Vectors; ++I) { FLinearColor V;
                if (M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)),V) && V==R.Values[I])
                    M->SetVectorParameterValue(VectorName(I),S->OldVectors[I]); }
            S->Stage=0;
        }
    }
    void Cancel(FRecord& R) { if (Job && Job->Record.Pin().Get() == &R) Job->Cancel->store(true, std::memory_order_relaxed); }
    int64 ResidentBytes()
    {
        int64 Bytes = Job ? LevelBytes : 0;
        for (const auto& W : Worlds) for (const auto& R : W.Value.Records) Bytes += int64(R->Built) * LevelBytes;
        return Bytes;
    }
    int32 HeavyRecords()
    {
        int32 Count=0;
        for (const auto& W:Worlds) for (const auto& R:W.Value.Records) if (R->Built>0) ++Count;
        if (Job) { const auto R=Job->Record.Pin(); if (!R || R->Built==0) ++Count; }
        return Count;
    }
    bool Anchor(FRecord& R, UWorld* W)
    {
        if (R.Layout.Count) return true;
        FVector3d C = R.RequestedAnchor;
        auto* B = R.Body.Get(); auto* PC = W->GetFirstPlayerController(); auto* Pawn = PC ? PC->GetPawn() : nullptr;
        if (C.IsNearlyZero())
        {
            if (!B || !Pawn) return false;
            // Preserve the captured NATIVE root frame even after root retirement.
            // Re-compose with the current body transform; origin shifts do not
            // alter this relative frame. Body and root scales were verified unit.
            const FTransform RootWorld=R.Key.RootInBody*B->GetActorTransform();
            C = RootWorld.InverseTransformPosition(Pawn->GetActorLocation()).GetSafeNormal();
        }
        if (C.ContainsNaN() || C.IsNearlyZero()) return false;
        C.Normalize(); FVector3d U, V; C.FindBestAxisVectors(U, V); V = FVector3d::CrossProduct(C, U).GetSafeNormal();
        APSCanonicalCoverageLayout::FFrame F{{C.X,C.Y,C.Z},{U.X,U.Y,U.Z},{V.X,V.Y,V.Z}};
        std::string Error;
        if (!APSCanonicalCoverageLayout::Build(R.Frame.Radius, F, R.Layout, Error)
            || int64(R.Layout.Count) * LevelBytes > RecordBudget) { R.bFailed = true; return false; }
        R.Values[0] = FLinearColor(C.X,C.Y,C.Z,float(R.Layout.Count));
        R.Values[1] = FLinearColor(U.X,U.Y,U.Z,0); R.Values[2] = FLinearColor(V.X,V.Y,V.Z,0);
        for (int32 I = 0; I < Slots; ++I) { const double Width = R.Layout.Levels[FMath::Min(I,R.Layout.Count-1)].WidthCm;
            R.Values[3+I] = FLinearColor(2.0*R.Frame.Radius/Width,.5/R.Layout.Resolution,Width,0); }
        UE_LOG(LogTemp, Display, TEXT("[APS.ReliefRuntime] queued body=%s signature=%u levels=%d fixedFrontOnly=1; unpromoted, logical texture budget excludes staging/scratch"),
            *GetNameSafe(B),R.Key.Signature,R.Layout.Count);
        return true;
    }
    void Bind(FRecord& R, UWorld* W, double Now)
    {
        if (R.bFailed || !R.Layout.Count || R.Built != R.Layout.Count || R.bUploadPending) return;
        for (auto& S : R.Subscribers)
        {
            auto* M = S->MID.Get(); if (S->bConflict || !ShaderReady(M,W)) continue;
            if (!S->Stage)
            {
                float A = -1; M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()),A);
                if (A != S->OldAvailable) { S->bConflict = true; continue; }
                for (int32 I=0; I<Slots; ++I) { UTexture* T=nullptr;
                    if (!M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)),T) || T!=S->OldTextures[I].Get()) S->bConflict=true; }
                for (int32 I=0; I<Vectors; ++I) { FLinearColor V;
                    if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)),V) || V!=S->OldVectors[I]) S->bConflict=true; }
                if (S->bConflict) continue;
                M->SetScalarParameterValue(AvailableName(),0); S->LastAvailable = 0;
                for (int32 I=0; I<Slots; ++I) M->SetTextureParameterValue(TextureName(I),BoundTexture(R,I));
                for (int32 I=0; I<Vectors; ++I) M->SetVectorParameterValue(VectorName(I),R.Values[I]);
                S->Stage=1; S->Fence.BeginFence(); continue;
            }
            if (!Owned(R,*S)) { S->bConflict=true; continue; }
            if (S->Stage==1) { if (!S->Fence.IsFenceComplete()) continue; S->Stage=2; S->EnableAt=Now; }
            const double T=FMath::Clamp((Now-S->EnableAt)/2.0,0.0,1.0);
            const float A=float(T*T*(3.0-2.0*T));
            if (A!=S->LastAvailable) { M->SetScalarParameterValue(AvailableName(),A); S->LastAvailable=A; }
        }
    }
    void DrainJob()
    {
        if (!Job || !Job->Future.IsReady() || LastUploadFrame==GFrameCounter) return;
        auto R=Job->Record.Pin(); FResult Result=Job->Future.Get();
        const int32 Level=Job->Level; const bool bCancelled=Job->Cancel->load(std::memory_order_relaxed);
        const bool Valid=!bCancelled && Job->World.IsValid() && R && Current(*R)
            && R->Body->GetWorld()==Job->World.Get() && R->Built==Level && !R->bFailed;
        Job.Reset(); // Releases the global reservation only after the worker finished.
        if (!Valid) return;
        if (!Result.bOK || Result.Data.Metadata.InputSignature!=R->Key.Signature
            || Result.Data.Metadata.Level!=Level || Result.Data.ByteCount!=LevelBytes)
        { R->bFailed=true; UE_LOG(LogTemp,Warning,TEXT("[APS.ReliefRuntime] build failed body=%s: %s; existing ready data retained"),*GetNameSafe(R->Body.Get()),*Result.Error); return; }
        FString Error; LastUploadFrame=GFrameCounter;
        R->Textures[Level].Reset(APSCanonicalCoverageTexture::CreateTransient(Result.Data,Error));
        if (!R->Textures[Level].IsValid()) { R->bFailed=true; UE_LOG(LogTemp,Warning,TEXT("[APS.ReliefRuntime] upload failed: %s"),*Error); return; }
        ++R->Built; R->bUploadPending=true; R->UploadFence.BeginFence();
    }
}

bool APSPlanetReliefRuntime::IsEnabled()
{
    return IsInGameThread() && !IsRunningCommandlet() && APSPlanetReliefPrivate::Enabled.GetValueOnGameThread()==1;
}

bool APSPlanetReliefRuntime::Register(APlanetaryBody* Body, const APSClosedGlobeMesh::FSamplingFrame& Frame,
    uint32 Signature, UMaterialInstanceDynamic* Material, const FVector3d* PlanetLocalAnchor)
{
    using namespace APSPlanetReliefPrivate;
    if (!IsEnabled() || !IsValid(Body) || !Body->GetWorld() || !Body->GetWorld()->IsGameWorld() || !Graph(Material)
        || !Signature || Frame.PresentationScale!=1 || !FMath::IsFinite(Frame.Radius) || Frame.Radius<=0
        || !FMath::IsFinite(Frame.NoiseScale) || Frame.NoiseScale<1 || !FMath::IsFinite(Frame.NoiseIntensity) || Frame.NoiseIntensity<0
        || !FMath::IsFinite(Frame.OceanHeight) || (PlanetLocalAnchor && (PlanetLocalAnchor->ContainsNaN() || PlanetLocalAnchor->IsNearlyZero()))) return false;
    auto* G=Body->PlanetaryEnvironmentGenerator;
    if (!IsValid(G) || APSNativeGlobeSnapshot::Identity(Body,G->SurfaceProfileCatalog)!=Signature) return false;
    const auto* Root=G->WorldScapeRootInstance;
    if (!IsValid(Root) || Root->GetOwner()!=Body || Root->GetWorld()!=Body->GetWorld()
        || Root->GetActorTransform().ContainsNaN() || Body->GetActorTransform().ContainsNaN()
        || !Root->GetActorScale3D().Equals(FVector::OneVector,1.e-9)
        || !Body->GetActorScale3D().Equals(FVector::OneVector,1.e-9)) return false;
    FReliefCacheKey K; K.Signature=Signature; K.Profile=UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Frame.Profile);
    K.Seed=Body->WorldScapeSeed; K.TerrainSeed=Frame.Profile.TerrainSeed; K.BiomeSeed=Frame.Profile.BiomeSeed;
    K.Radius=Frame.Radius; K.Scale=Frame.NoiseScale; K.Intensity=Frame.NoiseIntensity; K.Presentation=Frame.PresentationScale;
    K.Ocean=Frame.OceanHeight; K.Coast=Frame.bCoastalReliefCandidate; K.Lava=Frame.bApplyNativeLavaEnvelope;
    K.RootInBody=Root->GetActorTransform().GetRelativeTransform(Body->GetActorTransform());
    if (K.Radius!=double(float(Body->GetWorldScapeBodyRadiusCm()))
        || K.Profile!=UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(G->ResolvedSurfaceProfile)) return false;
    auto& W=Worlds.FindOrAdd(Body->GetWorld()); TSharedPtr<FRecord> R;
    for (int32 I=W.Records.Num()-1; I>=0; --I) if (W.Records[I]->Body.Get()==Body)
    {
        if (W.Records[I]->Key==K) R=W.Records[I];
        else { Cancel(*W.Records[I]); Restore(*W.Records[I]); W.Records.RemoveAt(I); }
    }
    if (!R)
    {
        // Cheap immutable metadata/subscriptions may precede selection. Only Tick
        // admits Active/Arriving to the global TWO heavy-cache slots, never the
        // first two bodies encountered by the ordinary materialization order.
        if (W.Records.Num()>=64) return false;
        R=MakeShared<FRecord>(); R->Body=Body; R->Key=K; R->Frame=Frame;
        if (PlanetLocalAnchor) R->RequestedAnchor=PlanetLocalAnchor->GetSafeNormal();
        W.Records.Add(R);
    }
    R->Subscribers.RemoveAll([](const auto& S){return !S->MID.IsValid();});
    for (const auto& S:R->Subscribers) if (S->MID.Get()==Material) return !R->bFailed;
    if (R->Subscribers.Num()>=8) return false;
    auto S=MakeShared<FSubscriber>(); S->MID=Material;
    bool CopiedBindings=false;
    if (R->Layout.Count && R->Built==R->Layout.Count)
        for (int32 I=0; I<Slots; ++I) { UTexture* T=nullptr; Material->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)),T); CopiedBindings|=T==BoundTexture(*R,I); }
    // A cloned globe may already contain this owner's bindings. Its restoration
    // baseline is the saved parent, not a strong reference back to our textures.
    UMaterialInterface* Before=CopiedBindings ? Material->Parent.Get() : Material;
    if (!Before->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()),S->OldAvailable) || S->OldAvailable!=0) return false;
    for (int32 I=0; I<Slots; ++I) { UTexture* T=nullptr;
        if (!Before->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)),T) || !IsValid(T)) return false; S->OldTextures[I].Reset(T); }
    for (int32 I=0; I<Vectors; ++I) if (!Before->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)),S->OldVectors[I])) return false;
    if (CopiedBindings)
    {
        Material->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()),S->LastAvailable);
        if (!Owned(*R,*S) || S->LastAvailable<0 || S->LastAvailable>1) return false;
        bool Adopted=false;
        for (const auto& Previous:R->Subscribers) if (Previous->Stage && Previous->LastAvailable==S->LastAvailable && Owned(*R,*Previous))
        { S->Stage=Previous->Stage; S->EnableAt=Previous->EnableAt; Adopted=true; break; }
        if (!Adopted && S->LastAvailable!=1) return false;
        if (!Adopted) { S->Stage=2; S->EnableAt=FPlatformTime::Seconds()-2.0; }
        if (S->Stage==1) S->Fence.BeginFence(); // Adopt even an in-progress ramp without jumping to 1.
    }
    R->Subscribers.Add(S); return !R->bFailed;
}

void APSPlanetReliefRuntime::Tick(UWorld* World, APlanetaryBody* ActiveBody, APlanetaryBody* ArrivingBody)
{
    using namespace APSPlanetReliefPrivate;
    if (!IsInGameThread()) return;
    for (auto It=Worlds.CreateIterator(); It; ++It) if (!It.Key().IsValid())
    { for (auto& R:It.Value().Records) { Cancel(*R); Restore(*R); } It.RemoveCurrent(); }
    if (!IsEnabled()) { TArray<TWeakObjectPtr<UWorld>> Keys; Worlds.GetKeys(Keys); for (const auto& K:Keys) Release(K.Get()); DrainJob(); return; }
    DrainJob(); auto* W=Worlds.Find(World); if (!W) return;
    W->Active=ActiveBody; W->Arriving=ArrivingBody; const double Now=FPlatformTime::Seconds();
    TSharedPtr<FRecord> Next; double Best=DBL_MAX;
    for (int32 I=W->Records.Num()-1; I>=0; --I)
    {
        auto R=W->Records[I]; R->Subscribers.RemoveAll([](const auto& S){return !S->MID.IsValid();});
        const bool CheckNow=Now-R->LastIdentityCheck>=.5; if (CheckNow) R->LastIdentityCheck=Now;
        if (!R->Body.IsValid() || (CheckNow && !Current(*R))
            || (R->Subscribers.IsEmpty() && R->Body.Get()!=ActiveBody && R->Body.Get()!=ArrivingBody))
        { Cancel(*R); Restore(*R); W->Records.RemoveAt(I); continue; }
        if (R->bUploadPending && R->UploadFence.IsFenceComplete())
        {
            auto* T=R->Textures[R->Built-1].Get();
            if (!T || !T->GetResource() || !T->GetResource()->IsInitialized()) R->bFailed=true;
            R->bUploadPending=false;
        }
        Bind(*R,World,Now);
        // Only active/forecast-arriving bodies spend CPU. Registering distant
        // closed globes does not launch a full chart cascade for every planet.
        const double Score=R->Body.Get()==ActiveBody ? 0.0 : R->Body.Get()==ArrivingBody ? 1.0 : 2.0;
        if (!R->bFailed && !R->bUploadPending && Score<2 && Anchor(*R,World) && R->Built<R->Layout.Count
            && Score<Best && (R->Built>0 || HeavyRecords()<2)
            && int64(R->Built+1)*LevelBytes<=RecordBudget && ResidentBytes()+LevelBytes<=TotalBudget)
        { Next=R; Best=Score; }
    }
    if (Job || !Next) return;
    Job=MakeUnique<FJob>(); Job->Record=Next; Job->World=World; Job->Level=Next->Built;
    Job->Cancel=MakeShared<std::atomic_bool,ESPMode::ThreadSafe>(false);
    const auto Token=Job->Cancel; const auto Frame=Next->Frame; const auto Layout=Next->Layout;
    const uint32 Signature=Next->Key.Signature; const int32 Level=Job->Level;
    Job->Future=Async(EAsyncExecution::ThreadPool,[Frame,Layout,Signature,Level,Token]()
    {
        FResult Result; APSCanonicalFilteredHeight::FOptions O; O.InputSignature=Signature;
        O.Cancellation=Token.Get(); O.MaxWorkers=2; O.Quadrature=APSCanonicalFilteredHeight::EQuadrature::Three;
        if (!Token->load(std::memory_order_relaxed)) Result.bOK=APSCanonicalFilteredHeight::BuildLevel(Frame,Layout,Level,O,Result.Data,Result.Error);
        return Result; // No UObject, weak-record/world access or blocking GT callback.
    });
}

void APSPlanetReliefRuntime::Release(UWorld* World)
{
    using namespace APSPlanetReliefPrivate;
    if (!IsInGameThread()) return;
    if (auto* W=Worlds.Find(World)) { for (auto& R:W->Records) { Cancel(*R); Restore(*R); } Worlds.Remove(World); }
    if (Job && Job->World.Get()==World) Job->Cancel->store(true,std::memory_order_relaxed);
    // A cancelled job still occupies the ONE global slot/reservation until it
    // exits cooperatively and the next Tick drains it. Never wait during teardown.
}
