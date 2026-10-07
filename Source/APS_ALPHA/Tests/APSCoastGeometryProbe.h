#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "WorldScapeRoot.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/AutomationTest.h"

// Read-only, bounded diagnosis of the actual submitted coastline triangles.
// This is intentionally not a material/mesh replacement or a performance test.
namespace APSCoastGeometryProbe
{
template <typename TReporter>
inline bool Capture(TReporter& Test, AWorldScapeRoot& Root,
    APlayerController& PC, const FString& OutputPath, int32 View)
{
    if (!Root.WorldScapeLodInGeneration.IsEmpty()) return false;
    int32 Width = 0, Height = 0;
    PC.GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return false;
    FString CSV = TEXT("lod,section,triangle,edge_m,edge_px,lattice_error_m,linear_depth_m,native_depth_m,interpolation_error_m,classification_disagrees\n");
    int32 TotalSamples = 0;
    for (UWorldScapeLod* Land : Root.WorldScapeLod)
    {
        if (!IsValid(Land) || !IsValid(Land->Mesh)) return false;
        UWorldScapeLod* Water = nullptr;
        for (UWorldScapeLod* Candidate : Root.WorldScapeLodOcean)
            if (IsValid(Candidate) && Candidate->Lod == Land->Lod) { Water = Candidate; break; }
        if (!Water || !IsValid(Water->Mesh)) return false;
        int32 Samples = 0, Disagreements = 0;
        double MaxPixelEdge = 0, MaxErrorM = 0, MaxLatticeM = 0;
        for (int32 SectionIndex = 0; SectionIndex < Land->Mesh->GetNumSections(); ++SectionIndex)
        {
            const auto* LS = Land->Mesh->GetProcMeshSection(SectionIndex);
            const auto* WS = Water->Mesh->GetProcMeshSection(SectionIndex);
            if (!LS || !WS || LS->PlanetIndexBuffer != WS->PlanetIndexBuffer
                || LS->PlanetVertexBuffer.Num() != WS->PlanetVertexBuffer.Num()) return false;
            struct FSample { int32 Tri; double Distance; };
            TArray<FSample> Candidates;
            // Retain a spatially relevant bounded sample of visible sign-crossing triangles.
            for (int32 Tri = 0; Tri + 2 < LS->PlanetIndexBuffer.Num(); Tri += 3)
            {
                FVector Center = FVector::ZeroVector;
                double MinDepth = TNumericLimits<double>::Max(), MaxDepth = -TNumericLimits<double>::Max();
                for (int32 Corner = 0; Corner < 3; ++Corner)
                {
                    const uint32 Index = LS->PlanetIndexBuffer[Tri + Corner];
                    if (!LS->PlanetVertexBuffer.IsValidIndex(Index)) return false;
                    const FVector LW = Land->Mesh->GetComponentTransform().TransformPosition(LS->PlanetVertexBuffer[Index].Position);
                    const FVector WW = Water->Mesh->GetComponentTransform().TransformPosition(WS->PlanetVertexBuffer[Index].Position);
                    const double Depth = Root.WorldToECEF(WW).ToFVector().Size() - Root.WorldToECEF(LW).ToFVector().Size();
                    MinDepth = FMath::Min(MinDepth, Depth); MaxDepth = FMath::Max(MaxDepth, Depth);
                    Center += LW / 3.0;
                }
                if (MinDepth > 0 || MaxDepth < 0) continue;
                FVector2D Screen;
                if (!PC.ProjectWorldLocationToScreen(Center, Screen) || Screen.X < 0 || Screen.Y < 0
                    || Screen.X >= Width || Screen.Y >= Height) continue;
                Candidates.Add({Tri, FVector2D::DistSquared(Screen, FVector2D(Width, Height) * 0.5)});
            }
            Candidates.Sort([](const FSample& A, const FSample& B) { return A.Distance < B.Distance; });
            for (int32 S = 0; S < FMath::Min(16, Candidates.Num()); ++S)
            {
                const int32 Tri = Candidates[S].Tri;
                FVector L[3], W[3]; FVector2D P[3];
                double LatticeM = 0;
                for (int32 C = 0; C < 3; ++C)
                {
                    const uint32 Index = LS->PlanetIndexBuffer[Tri + C];
                    const FVector LW = Land->Mesh->GetComponentTransform().TransformPosition(LS->PlanetVertexBuffer[Index].Position);
                    const FVector WW = Water->Mesh->GetComponentTransform().TransformPosition(WS->PlanetVertexBuffer[Index].Position);
                    L[C] = Root.WorldToECEF(LW).ToFVector(); W[C] = Root.WorldToECEF(WW).ToFVector();
                    if (!PC.ProjectWorldLocationToScreen(LW, P[C])) return false;
                    LatticeM = FMath::Max(LatticeM, FVector::Distance(L[C].GetSafeNormal(), W[C].GetSafeNormal()) * Root.PlanetScale / 100.0);
                }
                const FVector LC = (L[0] + L[1] + L[2]) / 3.0, WC = (W[0] + W[1] + W[2]) / 3.0;
                const FVector Direction = LC.GetSafeNormal();
                const double NativeGround = Root.GetGroundNoise(Direction * Root.PlanetScale, false).Height;
                const double NativeWater = Root.GetGroundNoise(Direction * Root.PlanetScale, true).Height;
                const double NativeDepth = (NativeWater - NativeGround) / 100.0;
                const double LinearDepth = (WC.Size() - LC.Size()) / 100.0;
                const double ErrorM = FMath::Abs(LinearDepth - NativeDepth);
                const double EdgeM = FMath::Max3(FVector::Distance(L[0], L[1]), FVector::Distance(L[1], L[2]), FVector::Distance(L[2], L[0])) / 100.0;
                const double EdgePx = FMath::Max3(FVector2D::Distance(P[0], P[1]), FVector2D::Distance(P[1], P[2]), FVector2D::Distance(P[2], P[0]));
                const bool Different = (LinearDepth > 0) != (NativeDepth > 0);
                if (!FMath::IsFinite(NativeDepth) || !FMath::IsFinite(ErrorM)) return false;
                CSV += FString::Printf(TEXT("%d,%d,%d,%.6f,%.6f,%.9f,%.6f,%.6f,%.6f,%d\n"),
                    Land->Lod, SectionIndex, Tri / 3, EdgeM, EdgePx, LatticeM, LinearDepth, NativeDepth, ErrorM, Different);
                ++Samples; Disagreements += Different ? 1 : 0;
                MaxPixelEdge = FMath::Max(MaxPixelEdge, EdgePx); MaxErrorM = FMath::Max(MaxErrorM, ErrorM);
                MaxLatticeM = FMath::Max(MaxLatticeM, LatticeM);
            }
        }
        TotalSamples += Samples;
        Test.AddInfo(FString::Printf(TEXT("COAST_GEOMETRY view=%d lod=%d samples=%d mismatch=%d maxEdgePx=%.2f maxErrorM=%.3f maxLatticeM=%.6f"),
            View, Land->Lod, Samples, Disagreements, MaxPixelEdge, MaxErrorM, MaxLatticeM));
    }
    Test.AddInfo(FString::Printf(TEXT("COAST_GEOMETRY_COMPLETE view=%d samples=%d file=%s; read-only sample, not visual acceptance or FPS"), View, TotalSamples, *OutputPath));
    return TotalSamples > 0 && FFileHelper::SaveStringToFile(CSV, *OutputPath);
}
}
#endif
