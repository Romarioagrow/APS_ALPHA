#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "WorldScapeRoot.h"

// Diagnostic only. No worker-owned UWorldScapeLod buffers/positions/sizes are read.
// GetProcMeshSection is the GT-published CPU copy, NOT a GPU readback or fence.
// The caller owns cadence (e.g. 8 Hz) and this object's lifetime; Tick never waits.
namespace APSPublishedTerrainDescentAudit
{
    struct FRange
    {
        double Min = 0.0, Max = 0.0;
        bool HasSample = false;
        void Add(double Value)
        {
            if (!HasSample) { Min = Max = Value; HasSample = true; }
            else { Min = FMath::Min(Min, Value); Max = FMath::Max(Max, Value); }
        }
    };

    class FAudit
    {
    public:
        const FString& GetError() const { return Error; }

        // CameraWorld must be the actual capture camera, not a planned waypoint.
        bool Tick(AWorldScapeRoot* Root, const FVector& CameraWorld,
            double ElapsedSeconds, const TCHAR* Phase)
        {
            Error.Reset();
            if (!IsInGameThread()) return Fail(TEXT("audit requires game thread"));
            if (!IsValid(Root) || !Root->GetWorld()) return Fail(TEXT("missing root/world"));
            if (!Finite(CameraWorld) || !FMath::IsFinite(ElapsedSeconds))
                return Fail(TEXT("non-finite camera/time"));

            const FTransform RootFrame = Root->GetActorTransform();
            if (RootFrame.ContainsNaN()) return Fail(TEXT("invalid root transform"));
            UWorld* World = Root->GetWorld();
            const APlayerController* Controller = World->GetFirstPlayerController();
            const APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
            if (!IsValid(Pawn)) Fail(TEXT("missing local player pawn"));
            const FVector PawnWorld = IsValid(Pawn) ? Pawn->GetActorLocation() : FVector::ZeroVector;
            if (!Finite(PawnWorld)) Fail(TEXT("non-finite pawn position"));
            const FString CurrentPhase = Phase ? Phase : TEXT("unspecified");
            const uint32 RootId = Root->GetUniqueID();
            const bool NewRoot = RootId != PreviousRootId;
            const int32 Incomplete = Root->WorldScapeLodInGeneration.Num(); // GT-owned map only.
            const int32 LodCount = Root->WorldScapeLod.Num();
            const uint32 RootFrameHash = HashFrame(RootFrame);
            FVector4 MaterialCenter(0, 0, 0, 0);
            UMaterialInterface* Terrain = Root->TerrainMaterial.DefaultMaterial;
            const bool HasCenter = IsValid(Terrain) && Terrain->GetDoubleVectorParameterValue(
                FHashedMaterialParameterInfo(TEXT("APS_SharedPlanetCenter")), MaterialCenter);
            const FVector UniformCenter(MaterialCenter.X, MaterialCenter.Y, MaterialCenter.Z);
            const double CenterErrorCm = HasCenter ? FVector::Distance(UniformCenter, RootFrame.GetLocation()) : -1.0;
            if (!HasCenter || !Finite(UniformCenter) || CenterErrorCm > 0.1)
                Fail(FString::Printf(TEXT("material physical frame stale: centerAvailable=%d errorCm=%.6f"), HasCenter, CenterErrorCm));
            const bool ContextChanged = NewRoot || CurrentPhase != PreviousPhase
                || RootFrameHash != PreviousRootFrameHash || World->OriginLocation != PreviousOrigin
                || Incomplete != PreviousIncomplete || LodCount != PreviousLodCount;

            FSample Samples[3];
            bool Changed[3] = {};
            int32 PresentedLods = 0;
            bool AnyChanged = ContextChanged;
            for (int32 LodIndex = 0; LodIndex < 3; ++LodIndex)
            {
                FSample& Sample = Samples[LodIndex];
                UWorldScapeLod* Lod = Root->WorldScapeLod.IsValidIndex(LodIndex)
                    ? Root->WorldScapeLod[LodIndex] : nullptr;
                UWorldScapeMeshComponent* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                Sample.Mesh = Mesh;
                if (!Read(Root, Mesh, RootFrame, CameraWorld, Sample))
                    Fail(FString::Printf(TEXT("LOD%d: %s"), LodIndex, *Sample.Error));
                PresentedLods += Sample.PresentedSections == 3 ? 1 : 0;
                Changed[LodIndex] = NewRoot || !Previous[LodIndex].Seen
                    || Sample.GeometryHash != Previous[LodIndex].GeometryHash
                    || Sample.LayerHash != Previous[LodIndex].LayerHash
                    || Sample.BindingHash != Previous[LodIndex].BindingHash
                    || Sample.FrameHash != Previous[LodIndex].FrameHash
                    || Sample.Valid != Previous[LodIndex].Valid;
                AnyChanged |= Changed[LodIndex];
            }
            AnyChanged |= Error != PreviousError;
            if (AnyChanged)
            {
                ++ObservedSampleSequence;
                UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDescent] t=%.3f phase=%s sampleSeq=%llu frame=%llu root=%s rootWorld=%s pawnWorld=%s pawnValid=%d cameraWorld=%s cameraECEF=%s origin=%s lods=%d gtPresentedFirst3=%d incompleteAll=%d rootHidden=%d error=%s"),
                    ElapsedSeconds, *CurrentPhase, static_cast<unsigned long long>(ObservedSampleSequence),
                    static_cast<unsigned long long>(GFrameCounter), *Root->GetPathName(),
                    *RootFrame.GetLocation().ToString(), *PawnWorld.ToString(), IsValid(Pawn) ? 1 : 0,
                    *CameraWorld.ToString(), *Root->WorldToECEF(CameraWorld).ToFVector().ToString(),
                    *World->OriginLocation.ToString(), LodCount, PresentedLods, Incomplete,
                    Root->IsHidden() ? 1 : 0, Error.IsEmpty() ? TEXT("none") : *Error);
                if (NewRoot)
                    UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDescent] scope=GT-published-CPU publicationRevision=unavailable sampleSeq=observed-changes-not-publications gpuFrame=unobserved crossLodBoundary=not-measured materialUniforms=center-only maxVertexReadsPerLod=126"));
                UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDescent.Frame] t=%.3f center=%s centerErrorCm=%.6f"),
                    ElapsedSeconds, *UniformCenter.ToString(), CenterErrorCm);
                for (int32 LodIndex = 0; LodIndex < 3; ++LodIndex)
                {
                    const FSample& S = Samples[LodIndex];
                    if (!Changed[LodIndex] && !ContextChanged) continue;
                    const bool LayerOnlyChange = !NewRoot && Previous[LodIndex].Seen
                        && Previous[LodIndex].Valid && S.Valid
                        && S.GeometryHash == Previous[LodIndex].GeometryHash
                        && S.LayerHash != Previous[LodIndex].LayerHash;
                    UE_LOG(LogTemp, Display, TEXT("[APS.TerrainDescent.LOD] t=%.3f phase=%s lod=%d valid=%d mesh=%s gtPresentedSections=%d vertices=%d/%d/%d triangles=%d/%d/%d reads=%d geometryHash=%08x layerHash=%08x bindingHash=%08x frameHash=%08x ecefHash=%08x layerOnlyChangeSuspect=%d edgeCm=[%.3f,%.3f] radialKm=[%.6f,%.6f] slopeDeg=[%.3f,%.3f] normalVsFaceMinDot=%.6f degenerateSamples=%d colorBytes=%s uv0=%s uv1=%s nearestSampleWorld=%s nearestSampleECEF=%s nearestSampleNormalECEF=%s nearestSampleCameraCm=%.3f materials=%s error=%s"),
                        ElapsedSeconds, *CurrentPhase, LodIndex, S.Valid ? 1 : 0, *GetPathNameSafe(S.Mesh),
                        S.PresentedSections, S.Vertices[0], S.Vertices[1], S.Vertices[2],
                        S.Triangles[0], S.Triangles[1], S.Triangles[2], S.Reads,
                        S.GeometryHash, S.LayerHash, S.BindingHash, S.FrameHash, S.EcefHash,
                        LayerOnlyChange ? 1 : 0, S.Reads ? S.Edge.Min : 0.0, S.Reads ? S.Edge.Max : 0.0,
                        S.Reads ? S.Radius.Min / 100000.0 : 0.0, S.Reads ? S.Radius.Max / 100000.0 : 0.0,
                        S.Reads ? S.Slope.Min : 0.0, S.Reads ? S.Slope.Max : 0.0,
                        S.MinNormalFaceDot, S.DegenerateTriangles, *ColorRanges(S), *UvRanges(S, 0), *UvRanges(S, 1),
                        *S.NearestWorld.ToString(), *S.NearestEcef.ToString(), *S.NearestNormal.ToString(),
                        S.Reads ? FMath::Sqrt(S.NearestDistanceSq) : 0.0, *MaterialNames(S.Mesh),
                        S.Error.IsEmpty() ? TEXT("none") : *S.Error);
                }
            }
            for (int32 LodIndex = 0; LodIndex < 3; ++LodIndex)
            {
                const FSample& S = Samples[LodIndex];
                FPrevious& P = Previous[LodIndex];
                P.Seen = true; P.Valid = S.Valid; P.GeometryHash = S.GeometryHash;
                P.LayerHash = S.LayerHash; P.BindingHash = S.BindingHash; P.FrameHash = S.FrameHash;
            }
            PreviousRootId = RootId; PreviousRootFrameHash = RootFrameHash;
            PreviousPhase = CurrentPhase; PreviousOrigin = World->OriginLocation;
            PreviousIncomplete = Incomplete; PreviousLodCount = LodCount; PreviousError = Error;
            return Error.IsEmpty();
        }

    private:
        struct FPrevious
        {
            bool Seen = false, Valid = false;
            uint32 GeometryHash = 0, LayerHash = 0, BindingHash = 0, FrameHash = 0;
        };
        struct FSample
        {
            UWorldScapeMeshComponent* Mesh = nullptr; // Used only during this GT call.
            bool Valid = false;
            FString Error;
            int32 Vertices[3] = {}, Triangles[3] = {}, Reads = 0, PresentedSections = 0;
            int32 DegenerateTriangles = 0;
            uint32 GeometryHash = 0, LayerHash = 0, BindingHash = 0, FrameHash = 0, EcefHash = 0;
            FRange Edge, Radius, Slope, Color[4], Uv[2][2];
            double MinNormalFaceDot = 1.0, NearestDistanceSq = TNumericLimits<double>::Max();
            FVector NearestWorld = FVector::ZeroVector, NearestEcef = FVector::ZeroVector;
            FVector NearestNormal = FVector::ZeroVector;
        };

        static bool Finite(const FVector& V)
        { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); }
        static void Hash(uint32& Seed, double V) { Seed = FCrc::MemCrc32(&V, sizeof(V), Seed); }
        static void Hash(uint32& Seed, const FVector& V) { Hash(Seed, V.X); Hash(Seed, V.Y); Hash(Seed, V.Z); }
        static void Hash(uint32& Seed, const FVector2D& V) { Hash(Seed, V.X); Hash(Seed, V.Y); }
        static uint32 HashFrame(const FTransform& Frame)
        {
            uint32 Result = 0;
            Hash(Result, Frame.GetLocation()); Hash(Result, Frame.GetScale3D());
            const FQuat Q = Frame.GetRotation();
            Hash(Result, Q.X); Hash(Result, Q.Y); Hash(Result, Q.Z); Hash(Result, Q.W);
            return Result;
        }
        bool Fail(const FString& Message) { if (Error.IsEmpty()) Error = Message; return false; }

        static bool Read(AWorldScapeRoot* Root, UWorldScapeMeshComponent* Mesh,
            const FTransform& RootFrame, const FVector& CameraWorld, FSample& S)
        {
            auto Invalid = [&S](const TCHAR* Reason) { S.Error = Reason; return false; };
            if (!IsValid(Mesh)) return Invalid(TEXT("missing published mesh"));
            const FTransform MeshFrame = Mesh->GetComponentTransform();
            const FVector Scale = MeshFrame.GetScale3D();
            if (MeshFrame.ContainsNaN() || !Finite(Scale)
                || FMath::Abs(Scale.X) < UE_SMALL_NUMBER || FMath::Abs(Scale.Y) < UE_SMALL_NUMBER
                || FMath::Abs(Scale.Z) < UE_SMALL_NUMBER)
                return Invalid(TEXT("invalid/singular mesh transform"));
            S.FrameHash = HashFrame(MeshFrame);
            S.BindingHash = HashCombineFast(Mesh->GetUniqueID(), static_cast<uint32>(Mesh->GetNumSections()));
            const bool Visible = !Root->IsHidden() && Mesh->IsRegistered() && Mesh->IsVisible() && !Mesh->bHiddenInGame;
            S.BindingHash = HashCombineFast(S.BindingHash, Visible ? 1u : 0u);
            const FVector InverseScale(1.0 / Scale.X, 1.0 / Scale.Y, 1.0 / Scale.Z);
            for (int32 SectionIndex = 0; SectionIndex < 3; ++SectionIndex)
            {
                const FWorldScapeMeshSection* Section = Mesh->GetProcMeshSection(SectionIndex);
                const UMaterialInterface* Material = Mesh->GetMaterial(SectionIndex);
                S.BindingHash = HashCombineFast(S.BindingHash, IsValid(Material) ? Material->GetUniqueID() : 0u);
                if (!Section || !IsValid(Material)) return Invalid(TEXT("missing section/material"));
                S.Vertices[SectionIndex] = Section->PlanetVertexBuffer.Num();
                S.Triangles[SectionIndex] = Section->PlanetIndexBuffer.Num() / 3;
                S.GeometryHash = HashCombineFast(S.GeometryHash, static_cast<uint32>(S.Vertices[SectionIndex]));
                S.GeometryHash = HashCombineFast(S.GeometryHash, static_cast<uint32>(Section->PlanetIndexBuffer.Num()));
                S.BindingHash = HashCombineFast(S.BindingHash, Section->bSectionVisible ? 1u : 0u);
                if (S.Vertices[SectionIndex] < 3 || S.Triangles[SectionIndex] < 1
                    || Section->PlanetIndexBuffer.Num() % 3 != 0)
                    return Invalid(TEXT("empty/malformed published section"));
                S.PresentedSections += Visible && Section->bSectionVisible ? 1 : 0;
                const int32 Count = FMath::Min(14, S.Triangles[SectionIndex]);
                for (int32 SampleIndex = 0; SampleIndex < Count; ++SampleIndex)
                {
                    const int32 Triangle = Count == 1 ? 0 : static_cast<int32>(
                        static_cast<int64>(SampleIndex) * (S.Triangles[SectionIndex] - 1) / (Count - 1));
                    FVector WorldPoints[3], WorldNormals[3];
                    for (int32 Corner = 0; Corner < 3; ++Corner)
                    {
                        const uint32 Index = Section->PlanetIndexBuffer[Triangle * 3 + Corner];
                        if (Index >= static_cast<uint32>(S.Vertices[SectionIndex]))
                            return Invalid(TEXT("sampled index outside published vertex buffer"));
                        const FWorldScapeMeshVertex& V = Section->PlanetVertexBuffer[static_cast<int32>(Index)];
                        if (!Finite(V.Position) || !Finite(V.Normal) || V.Normal.IsNearlyZero()
                            || V.UV0.ContainsNaN() || V.UV1.ContainsNaN() || V.UV2.ContainsNaN() || V.UV3.ContainsNaN())
                            return Invalid(TEXT("non-finite sampled channel/zero normal"));
                        WorldPoints[Corner] = MeshFrame.TransformPosition(V.Position);
                        // Inverse transpose for normals, including non-uniform scale. ECEF is
                        // root inverse rotation only; a normal must never use WorldToECEF(position).
                        WorldNormals[Corner] = MeshFrame.GetRotation().RotateVector(V.Normal * InverseScale).GetSafeNormal();
                        const FVector Ecef = RootFrame.InverseTransformPositionNoScale(WorldPoints[Corner]);
                        const FVector NormalEcef = RootFrame.InverseTransformVectorNoScale(WorldNormals[Corner]).GetSafeNormal();
                        if (!Finite(Ecef) || !Finite(NormalEcef) || Ecef.IsNearlyZero() || NormalEcef.IsNearlyZero())
                            return Invalid(TEXT("invalid transformed sample"));
                        Hash(S.GeometryHash, V.Position); Hash(S.GeometryHash, V.Normal);
                        S.GeometryHash = HashCombineFast(S.GeometryHash, Index);
                        Hash(S.EcefHash, Ecef); Hash(S.EcefHash, NormalEcef);
                        S.LayerHash = HashCombineFast(S.LayerHash, GetTypeHash(V.Color));
                        Hash(S.LayerHash, V.UV0); Hash(S.LayerHash, V.UV1);
                        Hash(S.LayerHash, V.UV2); Hash(S.LayerHash, V.UV3);
                        S.Color[0].Add(V.Color.R); S.Color[1].Add(V.Color.G);
                        S.Color[2].Add(V.Color.B); S.Color[3].Add(V.Color.A);
                        S.Uv[0][0].Add(V.UV0.X); S.Uv[0][1].Add(V.UV0.Y);
                        S.Uv[1][0].Add(V.UV1.X); S.Uv[1][1].Add(V.UV1.Y);
                        S.Radius.Add(Ecef.Size());
                        S.Slope.Add(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
                            FVector::DotProduct(NormalEcef, Ecef.GetSafeNormal()), -1.0, 1.0))));
                        const double DistanceSq = FVector::DistSquared(WorldPoints[Corner], CameraWorld);
                        if (DistanceSq < S.NearestDistanceSq)
                        {
                            S.NearestDistanceSq = DistanceSq; S.NearestWorld = WorldPoints[Corner];
                            S.NearestEcef = Ecef; S.NearestNormal = NormalEcef;
                        }
                        ++S.Reads;
                    }
                    for (int32 Edge = 0; Edge < 3; ++Edge)
                        S.Edge.Add(FVector::Distance(WorldPoints[Edge], WorldPoints[(Edge + 1) % 3]));
                    // Match WorldScape CalculateNormal's c-a cross b-a convention.
                    const FVector Face = FVector::CrossProduct(WorldPoints[2] - WorldPoints[0],
                        WorldPoints[1] - WorldPoints[0]).GetSafeNormal();
                    if (Face.IsNearlyZero()) ++S.DegenerateTriangles;
                    else for (int32 Corner = 0; Corner < 3; ++Corner)
                        S.MinNormalFaceDot = FMath::Min(S.MinNormalFaceDot, FVector::DotProduct(Face, WorldNormals[Corner]));
                }
            }
            S.Valid = true;
            return true;
        }

        static FString MaterialNames(UWorldScapeMeshComponent* Mesh)
        {
            if (!IsValid(Mesh)) return TEXT("none");
            return FString::Printf(TEXT("[%s|%s|%s]"), *GetPathNameSafe(Mesh->GetMaterial(0)),
                *GetPathNameSafe(Mesh->GetMaterial(1)), *GetPathNameSafe(Mesh->GetMaterial(2)));
        }
        static FString ColorRanges(const FSample& S)
        {
            if (!S.Reads) return TEXT("unavailable");
            return FString::Printf(TEXT("R[%.0f,%.0f]G[%.0f,%.0f]B[%.0f,%.0f]A[%.0f,%.0f]"),
                S.Color[0].Min, S.Color[0].Max, S.Color[1].Min, S.Color[1].Max,
                S.Color[2].Min, S.Color[2].Max, S.Color[3].Min, S.Color[3].Max);
        }
        static FString UvRanges(const FSample& S, int32 Channel)
        {
            if (!S.Reads) return TEXT("unavailable");
            return FString::Printf(TEXT("U[%.6f,%.6f]V[%.6f,%.6f]"), S.Uv[Channel][0].Min,
                S.Uv[Channel][0].Max, S.Uv[Channel][1].Min, S.Uv[Channel][1].Max);
        }

        FString Error, PreviousError, PreviousPhase;
        FPrevious Previous[3];
        uint32 PreviousRootId = 0, PreviousRootFrameHash = 0;
        uint64 ObservedSampleSequence = 0;
        FIntVector PreviousOrigin = FIntVector::ZeroValue;
        int32 PreviousIncomplete = INDEX_NONE, PreviousLodCount = INDEX_NONE;
    };
}

#endif // WITH_DEV_AUTOMATION_TESTS
