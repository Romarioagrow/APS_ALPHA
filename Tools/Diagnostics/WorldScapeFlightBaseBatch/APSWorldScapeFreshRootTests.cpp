#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "WorldScapeRoot.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeFreshRootTest,
    "APS.Gameplay.World.PlanetSurface.FreshRootProfilePriming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeFreshRootTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("Fixture world"),World))return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    for(const bool Water : {false,true})
    for(const int32 Resolution : {64,256})
    {
        AWorldScapeRoot* Roots[3]{};
        double Milliseconds[3]{};
        for(int32 Mode=0;Mode<3;++Mode)
        {
            const FTransform Transform(FVector(0,0,1.e6));
            auto* Root=World->SpawnActorDeferred<AWorldScapeRoot>(AWorldScapeRoot::StaticClass(),Transform);
            if(!TestNotNull(TEXT("Fixture root"),Root))return false;
            Root->bGenerateWorldScape=false; Root->bGenerateFoliages=false;
            UGameplayStatics::FinishSpawningActor(Root,Transform);
            Roots[Mode]=Root;
            Root->PlanetScale=Root->PlanetScaleCode=6371.e5;
            Root->PlanetLocation=DVector(Transform.GetLocation());
            Root->MaxLod=10; Root->OceanMaxLod=10;
            Root->LodResolution=Root->OceanLodResolution=Resolution;
            Root->TriangleSize=Root->OceanTriangleSize=120;
            Root->bOcean=Water; Root->NoiseScale=419; Root->Seed=424242;
            Root->bGenerateCollision=false;
            TestTrue(TEXT("Profile starts dirty"),Root->CheckForRegenerate(false));
            if(Mode!=0) Root->CheckForRegenerate();
            TestTrue(TEXT("Cache priming creates no meshes or workers"),Root->WorldScapeLod.IsEmpty()
                && Root->WorldScapeLodOcean.IsEmpty() && Root->WorldScapeLodInGeneration.IsEmpty());
            const double Start=FPlatformTime::Seconds();
            if(Mode==2)
            {
                Root->SetActorHiddenInGame(true);
                Root->SetActorTickEnabled(false);
                int32 Calls=0;
                bool Complete=false;
                do
                {
                    const int32 Before=Root->WorldScapeLod.Num()+Root->WorldScapeLodOcean.Num();
                    Complete=Root->AdvanceBaseMeshInitialization(1);
                    TestEqual(TEXT("Only one LOD allocated per call"),Root->WorldScapeLod.Num()+Root->WorldScapeLodOcean.Num()-Before,1);
                    TestTrue(TEXT("No workers see partial base"),Root->WorldScapeLodInGeneration.IsEmpty());
                    TestEqual(TEXT("init only after full base"),Root->init,Complete);
                } while(!Complete && ++Calls<24);
                TestTrue(TEXT("Incremental initialization completed"),Complete);
                TestFalse(TEXT("Cannot append after initialization"),Root->AdvanceBaseMeshInitialization(1));
            }
            else { Root->GenerateBaseMesh(); Root->init=true; }
            // Exactly the native first-tick decision, without UpdatePosition or
            // asynchronous workers. Unprimed must rebuild; primed must not.
            const bool Again=Root->CheckForRegenerate();
            TestEqual(TEXT("Duplicate initial build removed"),Again,Mode==0);
            if(Again)Root->GenerateBaseMesh();
            Milliseconds[Mode]=(FPlatformTime::Seconds()-Start)*1000.;
            TestEqual(TEXT("Terrain count"),Root->WorldScapeLod.Num(),10);
            TestEqual(TEXT("Ocean count"),Root->WorldScapeLodOcean.Num(),Water?10:0);
            TestFalse(TEXT("No further profile regeneration"),Root->CheckForRegenerate(false));
        }
        int64 Vertices=0;
        for(int32 ComparedMode : {1,2})
        for(bool Ocean : {false,true})
        {
            const auto& A=Ocean?Roots[0]->WorldScapeLodOcean:Roots[0]->WorldScapeLod;
            const auto& B=Ocean?Roots[ComparedMode]->WorldScapeLodOcean:Roots[ComparedMode]->WorldScapeLod;
            for(int32 I=0;I<A.Num();++I)
            {
                auto* L=A[I];auto* R=B[I];
                TestTrue(TEXT("Main geometry identical"),L->Vertices==R->Vertices && L->Triangles==R->Triangles);
                TestTrue(TEXT("Sewn geometry identical"),L->VerticesPA==R->VerticesPA && L->VerticesPB==R->VerticesPB
                    && L->TrianglesPatchA==R->TrianglesPatchA && L->TrianglesPatchB==R->TrianglesPatchB);
                TestTrue(TEXT("Payload unchanged"),L->Normals==R->Normals && L->UV==R->UV && L->UV1==R->UV1 && L->VertexColors==R->VertexColors);
                for(int32 S=0;S<3;++S)
                {
                    const auto* LS=L->Mesh->GetProcMeshSection(S);const auto* RS=R->Mesh->GetProcMeshSection(S);
                    if(!TestTrue(TEXT("Published sections exist"),LS && RS))continue;
                    TestTrue(TEXT("Published index buffers identical"),LS->PlanetIndexBuffer==RS->PlanetIndexBuffer);
                    TestEqual(TEXT("Published vertex count identical"),LS->PlanetVertexBuffer.Num(),RS->PlanetVertexBuffer.Num());
                    Vertices+=LS->PlanetVertexBuffer.Num();
                }
            }
        }
        // A later real edit must still regenerate; priming is not freezing.
        Roots[1]->Seed++;
        TestTrue(TEXT("Later seed change detected"),Roots[1]->CheckForRegenerate(false));
        UE_LOG(LogTemp,Display,TEXT("[APS.FreshRoot] water=%d resolution=%d vertices=%lld legacyTwoBuildMs=%.3f primedOneBuildMs=%.3f (NullRHI microcheck, not frame timing)"),Water,Resolution,Vertices,Milliseconds[0],Milliseconds[1]);
        for(auto* Root:Roots){Root->CleanComponents();}
    }
    return true;
}
#endif
