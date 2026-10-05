#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "APSCanonicalCoverageFlight.h"
#include "APSPlanetBufferViewsProbe.h"
#include "APSPublishedTerrainDescentAudit.h"
#include "Components/PrimitiveComponent.h"

// Static fixture AFTER coverage preparation. The caller alone places the
// existing pawn/camera at HeightKm(), pitch75; no descent clock runs here.
// Coverage lighting stays1 in ALL phases. Only the slope scalar is0/1/0.
// A0 is therefore baseline SLOPE, NOT a complete original9541 material control.
namespace APSCanonicalSlopeAB
{
    constexpr double PitchDegrees = 75.0, WholeTestAllowanceSeconds = 90.0;
    enum class EResult : uint8 { Pending, Complete, Failed };
    inline bool Requested()
    {
        const TCHAR* Cursor=FCommandLine::Get(); FString Token;
        while(FParse::Token(Cursor,Token,false))
            if(Token.Equals(TEXT("-APSProbeCanonicalSlopeAB"),ESearchCase::IgnoreCase)
                || Token.StartsWith(TEXT("-APSProbeCanonicalSlopeAB="),ESearchCase::IgnoreCase)) return true;
        return false;
    }
    inline bool Parse(const TCHAR* CommandLine,bool& bRequested,FString& Error)
    {
        bRequested=false; Error.Reset(); int32 Count=0; FString Token;
        const TCHAR* Cursor=CommandLine;
        while(FParse::Token(Cursor,Token,false))
        {
            if(Token.Equals(TEXT("-APSProbeCanonicalSlopeAB"),ESearchCase::IgnoreCase)) ++Count;
            else if(Token.StartsWith(TEXT("-APSProbeCanonicalSlopeAB="),ESearchCase::IgnoreCase))
            { Error=TEXT("CanonicalSlopeAB takes no value"); return false; }
        }
        if(Count>1) { Error=TEXT("Duplicate CanonicalSlopeAB flag"); return false; }
        bRequested=Count==1; return true; // Harness owns full cross-mode guard.
    }

    class FScope
    {
        TStrongObjectPtr<UMaterialInstanceDynamic> Material{nullptr};
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<UGameViewportClient> Viewport;
        APSPlanetBufferViews::FScope Buffers;
        APSPublishedTerrainDescentAudit::FAudit Audit;
        FRenderCommandFence Fence;
        FVector Pose=FVector::ZeroVector;
        FQuat Rotation=FQuat::Identity;
        float Fov=0,Aspect=0,OldScalar=0,BoundScalar=0;
        uint32 Geometry=0;
        int32 PoseIndex=0,Pass=-1,Captures=0,TotalCaptures=0,StableFrames=0;
        uint64 LastFrame=MAX_uint64,LastStableFrame=MAX_uint64;
        double BeginTime=0,LastNow=0,PoseTime=0,StableSince=-1,PassTime=-1,NextCapture=0;
        bool bStarted=false,bBound=false,bBuffers=false,bComplete=false,bReleased=false,bReleaseOK=true;
        FString Error,ReleaseError;
        static FName ScalarName() { return TEXT("APS_CanonicalSlopeAvailable"); }
        const TCHAR* HeightLabel() const { return PoseIndex==0?TEXT("100km"):TEXT("2m"); }
        const TCHAR* PhaseLabel() const { return Pass<2?TEXT("A0"):Pass<4?TEXT("B1"):TEXT("A0-return"); }
        int32 BufferIndex() const { return Pass>=0 && (Pass%2)==1?1:0; }
        const TCHAR* BufferLabel() const { return BufferIndex()==1?TEXT("BaseColor"):TEXT("Lit"); }

        // Same established GT-published signature idiom as static ChartAB.
        // At most32LODs*3sections*16vertices, no staging buffers/full copies.
        bool ReadGeometry(AWorldScapeRoot* R,uint32& Hash) const
        {
            if(!IsValid(R) || R->IsHidden() || R->WorldScapeLod.IsEmpty()
                || R->WorldScapeLod.Num()>32 || !R->WorldScapeLodInGeneration.IsEmpty()) return false;
            Hash=GetTypeHash(R->WorldScapeLod.Num()); const FTransform Frame=R->GetActorTransform();
            for(const auto* Lod:R->WorldScapeLod)
            {
                auto* Mesh=IsValid(Lod)?Lod->Mesh:nullptr;
                if(!IsValid(Mesh) || !Mesh->IsVisible() || Mesh->bHiddenInGame || !Mesh->IsRegistered()
                    || Mesh->GetNumSections()!=3) return false;
                Hash=HashCombine(Hash,GetTypeHash(Mesh->GetUniqueID())); bool HasGeometry=false;
                for(int32 I=0;I<3;++I)
                {
                    const auto* S=Mesh->GetProcMeshSection(I);
                    if(!S || Mesh->GetMaterial(I)!=Material.Get()) return false;
                    Hash=HashCombine(Hash,GetTypeHash(S->PlanetVertexBuffer.Num()));
                    Hash=HashCombine(Hash,GetTypeHash(S->PlanetIndexBuffer.Num()));
                    Hash=HashCombine(Hash,GetTypeHash(S->bSectionVisible));
                    if(S->PlanetVertexBuffer.IsEmpty() && S->PlanetIndexBuffer.IsEmpty()) continue;
                    if(S->PlanetVertexBuffer.IsEmpty() || S->PlanetIndexBuffer.Num()<3 || !S->bSectionVisible) return false;
                    HasGeometry=true;
                    for(int32 J=0;J<16;++J)
                    {
                        const auto& V=S->PlanetVertexBuffer[(int64(J)*(S->PlanetVertexBuffer.Num()-1))/15];
                        const FVector P=Frame.InverseTransformPosition(Mesh->GetComponentTransform().TransformPosition(V.Position));
                        if(P.ContainsNaN() || V.Normal.ContainsNaN()) return false;
                        for(int32 Axis=0;Axis<3;++Axis) Hash=HashCombine(Hash,GetTypeHash(FMath::RoundToInt64(P[Axis]*10.0)));
                        Hash=HashCombine(Hash,GetTypeHash(V.Normal)); Hash=HashCombine(Hash,GetTypeHash(V.Color));
                        Hash=HashCombine(Hash,GetTypeHash(V.UV0)); Hash=HashCombine(Hash,GetTypeHash(V.UV1));
                    }
                }
                if(!HasGeometry) return false;
            }
            return true;
        }
        void LogRepresentations() const
        {
            auto* Body=Surface.IsValid()?Surface->PlanetaryBody:nullptr; if(!IsValid(Body)) return;
            TArray<AActor*> Actors; Body->GetAttachedActors(Actors,true,true);
            Actors.Insert(Body,0); int32 Read=0; bool Truncated=Actors.Num()>9;
            for(int32 I=0;I<FMath::Min(Actors.Num(),9);++I)
            {
                if(!IsValid(Actors[I])) continue;
                TInlineComponentArray<UPrimitiveComponent*> Components; Actors[I]->GetComponents(Components);
                for(const auto* C:Components)
                {
                    if(!IsValid(C)) continue;
                    if(Read>=32) { Truncated=true; break; } ++Read;
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope.Representation] height=%s component=%s owner=%s visible=%d hidden=%d ownerHidden=%d registered=%d slots=%d material0=%s boundsRadiusCm=%.3f"),
                        HeightLabel(),*C->GetPathName(),*GetPathNameSafe(C->GetOwner()),C->IsVisible(),C->bHiddenInGame,
                        C->GetOwner()?C->GetOwner()->IsHidden():1,C->IsRegistered(),C->GetNumMaterials(),*GetPathNameSafe(C->GetMaterial(0)),C->Bounds.SphereRadius);
                }
            }
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope.Representation] height=%s sampledComponents=%d actors=%d truncated=%d; body/attached primitives plus separately audited native LOD sections, GT only"),HeightLabel(),Read,Actors.Num(),Truncated);
        }
        EResult Fail(const FString& Why)
        {
            Error=Why; FString Cleanup; if(!Release(Cleanup)) Error+=TEXT("; cleanup: ")+Cleanup;
            return EResult::Failed;
        }
        bool ApplyPass(double Now)
        {
            BoundScalar=(Pass==2 || Pass==3)?1.f:0.f;
            Material->SetScalarParameterValue(ScalarName(),BoundScalar);
            if(!Buffers.Apply(BufferIndex(),Error)) return false;
            Captures=0; PassTime=-1; NextCapture=0; Fence.BeginFence();
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope] SET height=%s phase=%s view=%s slope=%.0f coverageLighting=1 t=%.3f sameMID=%s"),
                HeightLabel(),PhaseLabel(),BufferLabel(),BoundScalar,Now-BeginTime,*Material->GetPathName());
            return true;
        }
    public:
        ~FScope() { Release(); }
        bool Started() const { return bStarted; }
        bool Complete() const { return bComplete; }
        double HeightKm() const { return PoseIndex==0?100.0:0.002; }
        const FString& GetError() const { return Error; }

        // Capture(HeightLabel,PhaseLabel,BufferLabel,IndexWithinPass,Error).
        // False+emptyError may retry within8s; captures use distinct GT frames.
        template<typename TCapture>
        EResult Tick(APlanetarySurfaceGenerator* S,const FMinimalViewInfo& View,double Now,
            const APSCanonicalCoverageFlight::FLease& Coverage,TCapture Capture)
        {
            if(bComplete) return EResult::Complete;
            if(!Error.IsEmpty() || bReleased) return EResult::Failed;
            if(!IsInGameThread() || !WITH_EDITOR || !FApp::IsUnattended() || !Requested()
                || !FMath::IsFinite(Now) || (bStarted && Now<LastNow)) return Fail(TEXT("Invalid static slope context/clock"));
            if(!Coverage.Ready() || !Coverage.Validate(S,View,Error)) return Fail(Error.IsEmpty()?TEXT("Coverage must be ready before slope capture"):Error);
            auto* R=S->WorldScapeRootInstance; auto* M=S->ResolvedTerrainMaterialInstance;
            auto* V=AutomationCommon::GetAnyGameViewportClient();
            if(!IsValid(V) || V->GetWorld()!=S->GetWorld()) return Fail(TEXT("Slope capture lost gameplay viewport/world"));
            if(!bStarted)
            {
                if(!M->GetScalarParameterValue(FMaterialParameterInfo(ScalarName()),OldScalar) || OldScalar!=0)
                    return Fail(TEXT("Slope scalar missing or not initially disabled"));
                Material.Reset(M); Root=R; Surface=S; Viewport=V;
                if(!Buffers.Begin(Error)) return Fail(Error); bBuffers=true;
                bBound=true; BoundScalar=0; Material->SetScalarParameterValue(ScalarName(),0);
                bStarted=true; BeginTime=PoseTime=LastNow=Now;
            }
            LastNow=Now;
            if(Now-BeginTime>WholeTestAllowanceSeconds) return Fail(TEXT("Static slope comparison exceeded90s"));
            float Actual=-1;
            if(S!=Surface.Get() || R!=Root.Get() || M!=Material.Get() || V!=Viewport.Get()
                || !M->GetScalarParameterValue(FMaterialParameterInfo(ScalarName()),Actual) || Actual!=BoundScalar)
                return Fail(TEXT("Static slope owned scalar/root/MID/viewport changed"));
            const FTransform Frame=R->GetActorTransform();
            const FVector Local=Frame.InverseTransformPosition(View.Location);
            const FQuat LocalRotation=Frame.GetRotation().Inverse()*View.Rotation.Quaternion();
            const FVector WorldOut=(View.Location-R->GetActorLocation()).GetSafeNormal();
            const double Ground=R->GetGroundHeight(R->GetActorLocation()+WorldOut*R->PlanetScale,false);
            const double ActualHeightCm=Local.Size()-double(R->PlanetScale)-Ground;
            const bool HeightReady=FMath::IsFinite(Ground) && FMath::IsFinite(ActualHeightCm)
                && FMath::Abs(ActualHeightCm-HeightKm()*100000.0)<=1.0;
            uint32 CurrentGeometry=0; const bool GeometryReady=ReadGeometry(R,CurrentGeometry);
            if(Pass<0)
            {
                if(Now-PoseTime>30) return Fail(TEXT("Static slope pose/publication did not settle within30s"));
                if(!HeightReady || !GeometryReady || CurrentGeometry!=Geometry || !Local.Equals(Pose,.1)
                    || !LocalRotation.Equals(Rotation,1.e-6) || View.FOV!=Fov || View.AspectRatio!=Aspect)
                {
                    Geometry=CurrentGeometry; Pose=Local; Rotation=LocalRotation; Fov=View.FOV; Aspect=View.AspectRatio;
                    StableFrames=0; StableSince=Now;
                }
                else if(GFrameCounter!=LastStableFrame) ++StableFrames;
                LastStableFrame=GFrameCounter;
                if(!HeightReady || !GeometryReady || StableSince<0 || Now-StableSince<3 || StableFrames<12) return EResult::Pending;
                LogRepresentations(); Pass=0;
                if(!ApplyPass(Now)) return Fail(Error);
                return EResult::Pending;
            }
            if(!HeightReady || !GeometryReady || Geometry!=CurrentGeometry || !Local.Equals(Pose,.1)
                || !LocalRotation.Equals(Rotation,1.e-6) || View.FOV!=Fov || View.AspectRatio!=Aspect)
                return Fail(TEXT("Static slope camera or sampled published geometry changed across A/B"));
            if(!Buffers.Validate(BufferIndex(),Error)) return Fail(Error);
            if(PassTime<0)
            {
                if(!Fence.IsFenceComplete()) return EResult::Pending;
                PassTime=Now; NextCapture=Now+2;
            }
            if(Now-PassTime>8) return Fail(TEXT("Static slope view failed to capture two settled frames within8s"));
            if(Captures<2 && Now>=NextCapture && LastFrame!=GFrameCounter)
            {
                if(!Audit.Tick(R,View.Location,Now-BeginTime,PhaseLabel())) return Fail(Audit.GetError());
                FString CaptureError;
                if(!Capture(HeightLabel(),PhaseLabel(),BufferLabel(),Captures,CaptureError))
                    return CaptureError.IsEmpty()?EResult::Pending:Fail(CaptureError);
                LastFrame=GFrameCounter; ++Captures; ++TotalCaptures; NextCapture=Now+.25;
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope] CAPTURE height=%s actualHeightCm=%.6f phase=%s view=%s index=%d GTframe=%llu geometry=%08x localCamera=%s rotation=%s fov=%.6f aspect=%.6f; observedUnfenced=1 not visual acceptance"),
                    HeightLabel(),ActualHeightCm,PhaseLabel(),BufferLabel(),Captures-1,GFrameCounter,Geometry,*Local.ToString(),*LocalRotation.ToString(),View.FOV,View.AspectRatio);
            }
            if(Captures==2 && Now-PassTime>=3)
            {
                if(Pass<5) { ++Pass; if(!ApplyPass(Now)) return Fail(Error); }
                else if(PoseIndex==0)
                {
                    if(!Buffers.Apply(0,Error)) return Fail(Error);
                    PoseIndex=1; Pass=-1; PoseTime=Now; StableSince=-1; StableFrames=0;
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope] NEXT height=2m; settle ordinary live geometry, no freeze"));
                }
                else
                {
                    if(TotalCaptures!=24) return Fail(TEXT("Static slope capture contract incomplete"));
                    FString Cleanup; if(!Release(Cleanup)) return Fail(Cleanup);
                    bComplete=true;
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalSlope] COMPLETE captures=24 heights=100km/2m slope=0-1-0 Lit/BaseColor; scope restored, evidence only"));
                    return EResult::Complete;
                }
            }
            return EResult::Pending;
        }
        bool Release(FString& OutError)
        {
            if(bReleased) { OutError=ReleaseError; return bReleaseOK; }
            OutError.Reset();
            if(!IsInGameThread()) { OutError=TEXT("Slope release requiresGT"); return false; }
            bReleased=true;
            if(bBound && Material.IsValid())
            {
                float Current=-1;
                if(Material->GetScalarParameterValue(FMaterialParameterInfo(ScalarName()),Current) && Current==BoundScalar)
                    Material->SetScalarParameterValue(ScalarName(),OldScalar);
                else { OutError=TEXT("Slope scalar changed externally; restoration refused"); bReleaseOK=false; }
            }
            if(bBuffers)
            {
                FString BufferError;
                if(!Buffers.Apply(APSPlanetBufferViews::FScope::Count-1,BufferError)
                    || !Buffers.Validate(APSPlanetBufferViews::FScope::Count-1,BufferError))
                { OutError+=TEXT("; viewport restoration: ")+BufferError; bReleaseOK=false; }
                Buffers.Restore(); bBuffers=false;
            }
            Material.Reset(); ReleaseError=OutError; return bReleaseOK;
        }
        void Release()
        {
            FString Why; if(!Release(Why)) { UE_LOG(LogTemp,Error,TEXT("[APS.CanonicalSlope] cleanup failed: %s"),*Why); }
        }
    };
}
#endif
