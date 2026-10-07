#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "APSFoliageWalkingRenderedProbe.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Dedicated opt-in locomotion proof. One documented initial character placement
// beside an EXISTING natural obstacle, then only normal AddMovementInput. No
// foliage placement, gravity/movement-mode/speed override or forced proxy tick.
// This exercises CharacterMovement, not keyboard mapping or ship dynamics.
namespace APSFoliageLocomotion
{
inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeFoliageLocomotion")); }
enum class EResult { Pending, Done, Failed };
struct FProbe
{
    TWeakObjectPtr<AWorldScapeRoot> Root;
    TWeakObjectPtr<ACharacter> Walker;
    TWeakObjectPtr<UStaticMesh> Mesh;
    TWeakObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<AActor> OriginalTarget;
    TWeakObjectPtr<ACameraActor> Camera;
    FTransform InstanceLocal;
    FVector CenterLocal, ForwardLocal, SideLocal, StartLocal;
    double PhaseStart = 0, LastTime = 0, BlockedTime = 0, Travel = 0, Clearance = 0;
    int32 Phase = 0, Frames = 0;
    FString Csv = TEXT("frame,phase,seconds,xCm,yCm,zCm,speedCm,grounded,targetContact,blockedSeconds\n");
    ~FProbe() { Restore(); }
    void Restore()
    {
        if (Walker.IsValid()) Walker->ConsumeMovementInputVector();
        if (Controller.IsValid() && Camera.IsValid() && OriginalTarget.IsValid()
            && Controller->GetViewTarget() == Camera.Get()) Controller->SetViewTarget(OriginalTarget.Get());
        if (Camera.IsValid()) Camera->Destroy();
        Camera.Reset(); Controller.Reset(); OriginalTarget.Reset(); Walker.Reset(); Root.Reset();
        if (Frames)
        {
            FFileHelper::SaveStringToFile(Csv, *(FPaths::ProjectSavedDir()/TEXT("FoliageLocomotion.csv")));
            Frames = 0;
        }
    }
    bool Begin(UWorld* World, AWorldScapeRoot* InRoot, double Now, FString& Error)
    {
        auto* PC = World ? World->GetFirstPlayerController() : nullptr;
        auto* Pawn = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
        auto* Capsule = Pawn ? Pawn->GetCapsuleComponent() : nullptr;
        if (!InRoot || !Capsule || !Pawn->GetCharacterMovement()->IsMovingOnGround() || PC->IsMoveInputIgnored())
        { Error = TEXT("Locomotion needs grounded real character and enabled movement input"); return false; }
        FCollisionQueryParams Query(SCENE_QUERY_STAT(APSFoliageLocomotion), false, Pawn);
        const auto Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
        TInlineComponentArray<UInstancedStaticMeshComponent*> Sources(InRoot);
        FVector Start, Target, Forward, Side, Up;
        FTransform Chosen;
        UStaticMesh* ChosenMesh = nullptr;
        const bool TreeOnly=FParse::Param(FCommandLine::Get(),TEXT("APSProbeFoliageWalkTree"));
        int32 Candidates = 0, GroundCandidates = 0;
        for (auto* ISM : Sources)
        {
            if (ChosenMesh) break;
            if (!IsValid(ISM) || !ISM->IsVisible() || !ISM->GetStaticMesh()
                || !ISM->GetStaticMesh()->GetPathName().StartsWith(FString(APSPlanetSurfaceScatter::Root)+TEXT("/"))) continue;
            if (TreeOnly && !ISM->GetStaticMesh()->GetName().StartsWith(TEXT("SM_APS_Scatter_Tree"))) continue;
            for (int32 I : ISM->GetInstancesOverlappingSphere(Pawn->GetActorLocation(), TreeOnly ? 15000. : 3900., true))
            {
                FTransform T; if (!ISM->GetInstanceTransform(I, T, true)) continue;
                auto* Box = APSFoliageWalkingRendered::MatchingProxy(InRoot, ISM->GetStaticMesh(), T);
                // Tree coverage may start outside the current 40m physics pool.
                // Read its existing visual transform; do not create a test box.
                // The real pool must publish a matching proxy after setup.
                if (!Box && !TreeOnly) continue;
                FVector BoxCenter, BoxExtent;
                if (!UAPSFoliageCollisionComponent::MakeLocalBox(ISM->GetStaticMesh()->GetName(), ISM->GetStaticMesh()->GetBounds(), BoxCenter, BoxExtent)) continue;
                FTransform BoxTransform=T;
                BoxTransform.SetLocation(T.TransformPosition(BoxCenter));
                const FVector ScaledExtent=BoxExtent*T.GetScale3D().GetAbs();
                ++Candidates;
                Target = BoxTransform.GetLocation();
                Up = (Target - InRoot->GetActorLocation()).GetSafeNormal();
                FVector Tangent, Other; Up.FindBestAxisVectors(Tangent, Other);
                // Reject step-height pebbles and use only existing cooked terrain.
                FHitResult Ground;
                FCollisionQueryParams FloorQuery = Query; if (Box) FloorQuery.AddIgnoredComponent(Box);
                const bool FloorHit = World->LineTraceSingleByChannel(Ground, Target+Up*1500., Target-Up*1500., ECC_Pawn, FloorQuery);
                UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_WALK_CANDIDATE mesh=%s floorHit=%d actor=%s component=%s slopeDot=%.4f bottomDelta=%.3f stepHeight=%.3f"),
                    *ISM->GetStaticMesh()->GetName(), FloorHit?1:0, *GetNameSafe(Ground.GetActor()), *GetNameSafe(Ground.GetComponent()),
                    Ground.ImpactNormal.Dot(Up), (Target-Ground.ImpactPoint).Dot(Up), Pawn->GetCharacterMovement()->MaxStepHeight);
                if (!FloorHit || Ground.GetActor() != InRoot || Ground.ImpactNormal.Dot(Up) < .9) continue;
                const FVector BoxUp = BoxTransform.GetRotation().GetAxisZ();
                const double Top = (Target-Ground.ImpactPoint).Dot(Up)
                    + FMath::Abs(BoxUp.Dot(Up))*ScaledExtent.Z;
                UE_LOG(LogTemp, Display, TEXT("APS_FOLIAGE_WALK_CANDIDATE_TOP mesh=%s topCm=%.3f"), *ISM->GetStaticMesh()->GetName(), Top);
                // Select above the actual step height, not an arbitrary 80cm
                // minimum that rejected the natural 74-76cm Frozen rocks.
                // Real grounded stall/contact below remains the acceptance gate.
                if (Top < Pawn->GetCharacterMovement()->MaxStepHeight + 10.) continue;
                ++GroundCandidates;
                const FVector E = ScaledExtent;
                Clearance = FMath::Sqrt(E.X*E.X+E.Y*E.Y) + Capsule->GetScaledCapsuleRadius() + 200.;
                Target = Ground.ImpactPoint+Up*(Capsule->GetScaledCapsuleHalfHeight()+3.);
                for (int32 Angle = 0; Angle < 8 && !ChosenMesh; ++Angle)
                {
                    Forward = Tangent.RotateAngleAxis(Angle*45., Up);
                    Side = Up.Cross(Forward).GetSafeNormal();
                    Start = Target-Forward*Clearance;
                    FHitResult Floor;
                    if (!World->LineTraceSingleByChannel(Floor, Start+Up*600., Start-Up*600., ECC_Pawn, Query)
                        || Floor.GetActor()!=InRoot || Floor.ImpactNormal.Dot(Up)<.9) continue;
                    Start = Floor.ImpactPoint+Up*(Capsule->GetScaledCapsuleHalfHeight()+3.);
                    if (World->OverlapBlockingTestByChannel(Start, Capsule->GetComponentQuat(), ECC_Pawn, Shape, Query)) continue;
                    FHitResult Hit;
                    if (Box && (!World->SweepSingleByChannel(Hit, Start, Start+Forward*(Clearance*2), Capsule->GetComponentQuat(), ECC_Pawn, Shape, Query)
                        || Hit.bStartPenetrating || Hit.GetComponent()!=Box || Hit.Distance<100.)) continue;
                    // A short three-sided route must have native floors and clear
                    // endpoints. Actual movement, not these queries, proves passage.
                    bool Clear = true;
                    for (const FVector& Offset : { -Forward*Clearance+Side*Clearance, Forward*Clearance+Side*Clearance, Forward*Clearance })
                    {
                        const FVector P = Target+Offset;
                        FHitResult F;
                        if (!World->LineTraceSingleByChannel(F, P+Up*600., P-Up*600., ECC_Pawn, Query)
                            || F.GetActor()!=InRoot || F.ImpactNormal.Dot(Up)<.9
                            || World->OverlapBlockingTestByChannel(F.ImpactPoint+Up*(Capsule->GetScaledCapsuleHalfHeight()+3.), Capsule->GetComponentQuat(), ECC_Pawn, Shape, Query)) { Clear=false; break; }
                    }
                    if (Clear) { Chosen=T; ChosenMesh=ISM->GetStaticMesh(); }
                }
                if (ChosenMesh) break;
            }
        }
        if (!ChosenMesh)
        { Error=FString::Printf(TEXT("No natural walkable collision target: proxies=%d tallWithNativeFloor=%d; no scene staging"), Candidates, GroundCandidates); return false; }
        Root=InRoot; Walker=Pawn; Mesh=ChosenMesh; Controller=PC; OriginalTarget=PC->GetViewTarget();
        const FTransform R=InRoot->GetActorTransform();
        InstanceLocal=Chosen.GetRelativeTransform(R);
        CenterLocal=R.InverseTransformPosition(Target); StartLocal=R.InverseTransformPosition(Start);
        ForwardLocal=R.InverseTransformVectorNoScale(Forward); SideLocal=R.InverseTransformVectorNoScale(Side);
        const double SetupDistance=FVector::Distance(Pawn->GetActorLocation(),Start);
        if (!Pawn->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics))
        { Error=TEXT("Initial test placement failed"); return false; }
        FActorSpawnParameters Spawn; Spawn.ObjectFlags|=RF_Transient;
        auto* C=World->SpawnActor<ACameraActor>(Spawn);
        if (!C) { Error=TEXT("Locomotion observer camera failed"); return false; }
        C->SetActorEnableCollision(false); C->AttachToActor(InRoot,FAttachmentTransformRules::KeepWorldTransform);
        const FVector Eye=Target-Side*(Clearance*2.7+500.)+Up*(Clearance*1.3+350.);
        C->SetActorLocationAndRotation(Eye,FRotationMatrix::MakeFromXZ(Target-Eye,Up).Rotator());
        C->GetCameraComponent()->FieldOfView=60; C->GetCameraComponent()->PostProcessBlendWeight=0;
        Camera=C; PC->SetViewTarget(C); PhaseStart=LastTime=Now;
        UE_LOG(LogTemp,Display,TEXT("APS_FOLIAGE_LOCOMOTION_BEGIN mesh=%s setupTeleportCm=%.3f clearanceCm=%.3f; one initial placement only; real AddMovementInput/CharacterMovement, no gravity/speed override; not keyboard mapping"),*ChosenMesh->GetName(),SetupDistance,Clearance);
        return true;
    }
    EResult Tick(double Now, TFunctionRef<bool(const TCHAR*,FString&)> Capture, FString& Error)
    {
        auto* R=Root.Get(); auto* Pawn=Walker.Get();
        if (!R || !Pawn || !Camera.IsValid() || !Controller.IsValid() || Controller->GetPawn()!=Pawn)
        { Error=TEXT("Locomotion lost real actor/observer"); return EResult::Failed; }
        auto* Movement=Pawn->GetCharacterMovement(); auto* Capsule=Pawn->GetCapsuleComponent();
        auto* Box=APSFoliageWalkingRendered::MatchingProxy(R,Mesh.Get(),InstanceLocal*R->GetActorTransform());
        if (!Box)
        {
            if (Phase==0 && Now-PhaseStart<3.) return EResult::Pending;
            Error=TEXT("Natural walking proxy disappeared/changed identity"); return EResult::Failed;
        }
        const FTransform RT=R->GetActorTransform();
        const FVector P=Pawn->GetActorLocation(), Local=RT.InverseTransformPosition(P);
        const FVector Up=(P-R->GetActorLocation()).GetSafeNormal();
        const FVector Forward=RT.TransformVectorNoScale(ForwardLocal), Side=RT.TransformVectorNoScale(SideLocal);
        const FVector Center=RT.TransformPosition(CenterLocal);
        const double Speed=FVector::VectorPlaneProject(Pawn->GetVelocity(),Up).Size();
        const double DT=FMath::Clamp(Now-LastTime,0.,.1); LastTime=Now;
        Travel=FMath::Max(Travel,FVector::VectorPlaneProject(Local-StartLocal,RT.InverseTransformVectorNoScale(Up)).Size());
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(APSFoliageWalkContact),false,Pawn);
        const FVector Inward=FVector::VectorPlaneProject(Center-P,Up).GetSafeNormal();
        const bool Contact=R->GetWorld()->SweepSingleByChannel(Hit,P,P+Inward*15.,Capsule->GetComponentQuat(),ECC_Pawn,
            FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Query)
            && !Hit.bStartPenetrating && Hit.GetComponent()==Box;
        Csv+=FString::Printf(TEXT("%llu,%d,%.6f,%.3f,%.3f,%.3f,%.3f,%d,%d,%.4f\n"),GFrameCounter,Phase,Now-PhaseStart,Local.X,Local.Y,Local.Z,Speed,Movement->IsMovingOnGround()?1:0,Contact?1:0,BlockedTime); ++Frames;
        if (Now-PhaseStart>10.) { Error=FString::Printf(TEXT("Walking phase%d timeout: speed=%.3f grounded=%d contact=%d travel=%.3f"),Phase,Speed,Movement->IsMovingOnGround(),Contact,Travel); return EResult::Failed; }
        if (Phase==0)
        {
            if (Now-PhaseStart<1. || !Movement->IsMovingOnGround()) return EResult::Pending;
            if (!Capture(TEXT("Start"),Error)) return EResult::Failed;
            Phase=1; PhaseStart=Now; return EResult::Pending;
        }
        if (Phase==1)
        {
            // Steer toward the obstacle after normal tangential collision slide.
            // A fixed direction can legitimately slide past a rotated box corner.
            Pawn->AddMovementInput(Inward,1.f);
            if ((P-Center).Dot(Forward)>Capsule->GetScaledCapsuleRadius())
            { Capture(TEXT("UnexpectedPass"),Error); Error=TEXT("Character passed through/over intended blocking obstacle"); return EResult::Failed; }
            if (Contact && Movement->IsMovingOnGround() && Speed<20. && Travel>100.) BlockedTime+=DT;
            else BlockedTime=0;
            if (BlockedTime<.75) return EResult::Pending;
            Pawn->ConsumeMovementInputVector();
            if (!Capture(TEXT("Blocked"),Error)) return EResult::Failed;
            UE_LOG(LogTemp,Display,TEXT("APS_FOLIAGE_LOCOMOTION_BLOCKED mesh=%s actualTravelCm=%.3f groundedStallSeconds=%.3f speedCm=%.3f targetContact=1"),*Mesh->GetName(),Travel,BlockedTime,Speed);
            Phase=2; PhaseStart=Now; return EResult::Pending;
        }
        const FVector Target=Center+(Phase==2 ? -Forward*Clearance+Side*Clearance : Phase==3 ? Forward*Clearance+Side*Clearance : Forward*Clearance);
        const FVector Delta=FVector::VectorPlaneProject(Target-P,Up);
        if (Delta.Size()>40.) { Pawn->AddMovementInput(Delta.GetSafeNormal(),1.f); return EResult::Pending; }
        if (!Movement->IsMovingOnGround()) return EResult::Pending;
        if (!Capture(Phase==2?TEXT("Aside"):Phase==3?TEXT("Around"):TEXT("Passed"),Error)) return EResult::Failed;
        if (Phase==4)
        {
            UE_LOG(LogTemp,Display,TEXT("APS_FOLIAGE_LOCOMOTION_COMPLETE mesh=%s farSideCm=%.3f travelCm=%.3f; grounded real movement around unchanged natural obstacle"),*Mesh->GetName(),(P-Center).Dot(Forward),Travel);
            return EResult::Done;
        }
        ++Phase; PhaseStart=Now; return EResult::Pending;
    }
};
}
#endif
