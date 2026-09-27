#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
// WorldScapeLod.h in the installed plugin lacks a guard. Root includes it through
// its own pragma-once boundary, avoiding duplicate UHT declarations when UBT puts
// this test beside another root consumer in a unity translation unit.
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "Engine/World.h"

// Exercises the loaded vendor binary: checking WorldScape's source alone misses
// stale precompiled Editor DLLs that still gamma-encode semantic vertex data.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeLodPayloadIdentityTest,
    "APS.Gameplay.World.PlanetSurface.LodPayloadIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSWorldScapeLodPayloadIdentityTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AActor* Owner = World->SpawnActor<AActor>();
    UWorldScapeLod* Lod = NewObject<UWorldScapeLod>(Owner);
    Lod->Mesh = NewObject<UWorldScapeMeshComponent>(Owner);
    Lod->Vertices = {FVector(0,0,0), FVector(100,0,0), FVector(0,100,0)};
    Lod->Triangles = {0,1,2};
    Lod->Normals.Init(FVector::UpVector,3);
    Lod->UV.Init(FVector2D::ZeroVector,3);
    Lod->Tangents.Init(FWorldScapeMeshTangent(FVector::ForwardVector,false),3);
    Lod->VerticesPA = Lod->VerticesPB = Lod->Vertices;
    Lod->VerticesNormalPA = Lod->VerticesNormalPB = Lod->Normals;
    Lod->UVPA = Lod->UVPB = Lod->UV;
    Lod->TangentsPA = Lod->TangentsPB = Lod->Tangents;
    Lod->RelativePosition = DVector(0,0,0);
    for (const float Height : {0.f,0.02f,0.18f,0.50f,0.80f,1.f})
    {
        Lod->VertexColors.Init(FLinearColor(Height,0.35f,0.6f,1),3);
        Lod->VerticesColorPA = Lod->VerticesColorPB = Lod->VertexColors;
        for (int32 Section=0; Section<3; ++Section)
            Lod->Mesh->CreateMeshSection_LinearColor(Section,Lod->Vertices,Lod->Triangles,
                Lod->Normals,Lod->UV,Lod->VertexColors,Lod->Tangents,false,false);
        const FColor Expected = Lod->Mesh->GetProcMeshSection(0)->PlanetVertexBuffer[0].Color;
        for (int32 Update=0; Update<2; ++Update)
        {
            Lod->UpdateMesh();
            for (int32 Section=0; Section<3; ++Section)
            {
                const FWorldScapeMeshVertex& Vertex = Lod->Mesh->GetProcMeshSection(Section)->PlanetVertexBuffer[0];
                TestEqual(FString::Printf(TEXT("Height %.2f section %d update %d preserves semantic RGB"),Height,Section,Update),Vertex.Color,Expected);
                TestEqual(TEXT("Material payload update preserves geometry"),Vertex.Position,Lod->Vertices[0]);
                TestEqual(TEXT("Material payload update preserves normal"),Vertex.Normal,Lod->Normals[0]);
            }
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif
