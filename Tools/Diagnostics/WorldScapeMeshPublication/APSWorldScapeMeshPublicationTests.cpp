#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WorldScapeMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldScapeMeshPublicationTest,
    "APS.Gameplay.World.PlanetSurface.MeshPublicationParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldScapeMeshPublicationTest::RunTest(const FString& Parameters)
{
    auto* Tasks = IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.MeshUpdateTasks"));
    if (!TestNotNull(TEXT("Publication task switch"), Tasks)) return false;
    const int32 SavedTasks = Tasks->GetInt();
    auto* Fast = IConsoleManager::Get().FindConsoleVariable(TEXT("worldscape.MeshFastCopy"));
    if (!TestNotNull(TEXT("Fast copy switch"), Fast)) return false;
    const int32 SavedFast = Fast->GetInt();
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Parity world"), World)) return false;
    AActor* Owner = World->SpawnActor<AActor>();
    if (!Owner) { World->DestroyWorld(false); return false; }
    auto* Mesh = NewObject<UWorldScapeMeshComponent>(Owner);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FName Tag(TEXT("APS.Collision.ParallelSamples"));
    int64 Checked = 0;
    for (int32 Count : {3, 257, 65536, 263169})
    {
        TArray<FVector> Positions, Normals;
        TArray<FVector2D> UV0, UV1, UV2, UV3;
        TArray<FColor> Colors;
        TArray<FWorldScapeMeshTangent> Tangents;
        for (int32 I = 0; I < Count; ++I)
        {
            Positions.Add(FVector(I * 1.125, I % 257 * -2.25, I % 13 * 0.375));
            Normals.Add(FVector(0.3, 0.4, 0.8660254038));
            UV0.Add(FVector2D(I * 0.25, I * 0.5));
            UV1.Add(FVector2D(I * 0.125, 1));
            UV2.Add(FVector2D(I * -0.25, 0.25));
            UV3.Add(FVector2D(0.75, I * -0.125));
            Colors.Add(FColor(I % 256, I % 127, I % 63, 255));
            Tangents.Add(FWorldScapeMeshTangent(FVector(0.8, -0.6, 0), (I & 1) != 0));
        }
        const TArray<int32> Triangles{0, 1, 2};
        // Missing channels must preserve the previous vertex fields, as in the
        // original API. Include an uneven batch size, the main grid and seams.
        for (bool bSparse : {false, true})
        {
            TArray<FWorldScapeMeshVertex> Expected;
            FBox ExpectedBox(ForceInit);
            for (int32 Variant = 0; Variant < 5; ++Variant)
            {
                Mesh->ClearAllMeshSections();
                // Start all optional fields at defaults so a skipped or stale
                // color/UV/normal copy cannot accidentally pass the comparison.
                Mesh->CreateMeshSection(0, Positions, Triangles, {}, {}, {}, {}, {}, {}, {}, false);
                TArray<FVector> UpdatedPositions = Positions;
                for (FVector& V : UpdatedPositions) V += FVector(1.5, -0.75, 0.125);
                if (Variant == 4) Owner->Tags.Remove(Tag); else Owner->Tags.AddUnique(Tag);
                Tasks->Set(Variant <= 1 ? 1 : Variant == 2 ? 2 : 4, ECVF_SetByCode);
                Fast->Set(Variant == 0 ? 0 : 1, ECVF_SetByCode);
                Mesh->UpdateMeshSection(0, UpdatedPositions, bSparse ? TArray<FVector>{} : Normals,
                    UV0, bSparse ? TArray<FVector2D>{} : UV1, UV2, UV3,
                    bSparse ? TArray<FColor>{} : Colors, bSparse ? TArray<FWorldScapeMeshTangent>{} : Tangents);
                const auto* Section = Mesh->GetProcMeshSection(0);
                if (!Section) { AddError(TEXT("Section missing")); continue; }
                if (Variant == 0) { Expected = Section->PlanetVertexBuffer; ExpectedBox = Section->SectionLocalBox; }
                else
                {
                    bool bExact = Section->PlanetVertexBuffer.Num() == Expected.Num();
                    for (int32 I = 0; I < Expected.Num() && bExact; ++I)
                    {
                        const auto& A = Section->PlanetVertexBuffer[I]; const auto& B = Expected[I];
                        bExact = A.Position == B.Position && A.Normal == B.Normal && A.Color == B.Color &&
                            A.UV0 == B.UV0 && A.UV1 == B.UV1 && A.UV2 == B.UV2 && A.UV3 == B.UV3 &&
                            A.Tangent.TangentX == B.Tangent.TangentX && A.Tangent.bFlipTangentY == B.Tangent.bFlipTangentY;
                    }
                    TestTrue(TEXT("Complete vertex payload exact, including missing channels"), bExact);
                    TestTrue(TEXT("Exact bounds retained"), Section->SectionLocalBox == ExpectedBox);
                    TestTrue(TEXT("Topology retained"), Section->PlanetIndexBuffer == TArray<uint32>{0, 1, 2});
                    Checked += Expected.Num();
                }
            }
        }
    }
    Tasks->Set(SavedTasks, ECVF_SetByCode);
    Fast->Set(SavedFast, ECVF_SetByCode);
    Mesh->DestroyComponent();
    World->DestroyWorld(false);
    AddInfo(FString::Printf(TEXT("Publication original/fast1/2/4/untagged exact parity: %lld vertices; not visual or flight performance acceptance"), Checked));
    return true;
}
#endif
